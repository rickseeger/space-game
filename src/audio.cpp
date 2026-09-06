#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "audio.h"
#include <cstring>
#include <vector>
#include <cmath>

namespace vd {

struct AudioEngine {
    ma_engine engine;
    bool inited = false;
};

bool Audio::init() {
    auto* ae = new AudioEngine();
    ma_engine_config cfg = ma_engine_config_init();
    // Prefer real device; fall back to no-device so headless/CI stays quiet
    ma_result res = ma_engine_init(&cfg, &ae->engine);
    if (res != MA_SUCCESS) {
        cfg.noDevice = MA_TRUE;
        res = ma_engine_init(&cfg, &ae->engine);
    }
    if (res != MA_SUCCESS) {
        delete ae;
        ok = false;
        engine = nullptr;
        return false;
    }
    ae->inited = true;
    engine = ae;
    ok = true;
    return true;
}

void Audio::shutdown() {
    if (engine) {
        auto* ae = (AudioEngine*)engine;
        if (ae->inited) ma_engine_uninit(&ae->engine);
        delete ae;
        engine = nullptr;
    }
    ok = false;
}

void Audio::update(float dt) {
    warningCooldown = std::max(0.f, warningCooldown - dt);
    thrustLevel *= std::exp(-3.f * dt);
    (void)thrustLevel;
}

void Audio::playTone(float freq, float dur, float vol, int type) {
    if (!ok) return;
    auto* ae = (AudioEngine*)engine;
    ma_waveform_config wcfg = ma_waveform_config_init(
        ma_format_f32, ma_engine_get_channels(&ae->engine),
        ma_engine_get_sample_rate(&ae->engine),
        type == 0 ? ma_waveform_type_sine : (type == 1 ? ma_waveform_type_square : ma_waveform_type_sawtooth),
        vol, freq);
    // Use sound from waveform via temporary buffer synthesis
    ma_uint32 sampleRate = ma_engine_get_sample_rate(&ae->engine);
    int frames = (int)(dur * sampleRate);
    if (frames < 16) return;
    std::vector<float> buf(frames);
    for (int i = 0; i < frames; i++) {
        float t = (float)i / sampleRate;
        float env = 1.f;
        float attack = 0.01f, release = dur * 0.45f;
        if (t < attack) env = t / attack;
        else if (t > dur - release) env = std::max(0.f, (dur - t) / release);
        float phase = t * freq * 2.f * 3.14159265f;
        float s = 0.f;
        if (type == 0) s = std::sin(phase);
        else if (type == 1) s = (std::sin(phase) > 0.f) ? 1.f : -1.f;
        else s = 2.f * (phase / (2.f * 3.14159265f) - std::floor(phase / (2.f * 3.14159265f) + 0.5f));
        // Soft clip
        buf[i] = s * env * vol * 0.35f;
    }
    ma_audio_buffer_config bcfg = ma_audio_buffer_config_init(ma_format_f32, 1, frames, buf.data(), nullptr);
    // Fire-and-forget: allocate on heap, copy data owned
    struct OneShot {
        std::vector<float> data;
        ma_audio_buffer buffer;
        ma_sound sound;
        bool done = false;
    };
    // Simpler approach: use ma_engine_play_sound isn't for procedural.
    // Use waveform node briefly — actually use ma_sound_init_from_data_source with owned copy.
    auto* shot = new OneShot();
    shot->data = std::move(buf);
    ma_audio_buffer_config acfg = ma_audio_buffer_config_init(ma_format_f32, 1, shot->data.size(), shot->data.data(), nullptr);
    if (ma_audio_buffer_init(&acfg, &shot->buffer) != MA_SUCCESS) { delete shot; return; }
    if (ma_sound_init_from_data_source(&ae->engine, &shot->buffer, 0, nullptr, &shot->sound) != MA_SUCCESS) {
        ma_audio_buffer_uninit(&shot->buffer);
        delete shot;
        return;
    }
    ma_sound_set_volume(&shot->sound, 1.f);
    ma_sound_start(&shot->sound);
    // Leak-safe-ish: schedule delete after duration via detached approach —
    // store and prune in update would be better; for arcade OK to delete after end with callback.
    // Use finish callback:
    // miniaudio end callback — keep simple: delete after play using ma_sound_set_end_callback if available.
    // Fallback: leave small leaks for short session OR prune list.
    // We'll attach to a static list cleaned lazily.
    struct Live { OneShot* s; float ttl; };
    static std::vector<Live> live;
    live.push_back({shot, dur + 0.3f});
    for (size_t i = 0; i < live.size(); ) {
        live[i].ttl -= 0.016f; // approx; real prune in update
        if (live[i].ttl <= 0.f || !ma_sound_is_playing(&live[i].s->sound)) {
            ma_sound_uninit(&live[i].s->sound);
            ma_audio_buffer_uninit(&live[i].s->buffer);
            delete live[i].s;
            live.erase(live.begin() + i);
        } else ++i;
    }
    (void)wcfg; (void)bcfg;
}

void Audio::playNoise(float dur, float vol, float cutoff) {
    if (!ok) return;
    auto* ae = (AudioEngine*)engine;
    ma_uint32 sampleRate = ma_engine_get_sample_rate(&ae->engine);
    int frames = (int)(dur * sampleRate);
    if (frames < 16) return;
    struct OneShot {
        std::vector<float> data;
        ma_audio_buffer buffer;
        ma_sound sound;
    };
    auto* shot = new OneShot();
    shot->data.resize(frames);
    float state = 0.f;
    unsigned seed = 12345u + (unsigned)(cutoff * 1000);
    for (int i = 0; i < frames; i++) {
        seed = seed * 1103515245u + 12345u;
        float white = ((seed >> 16) & 0x7fff) / 32768.f * 2.f - 1.f;
        // simple lowpass
        state += cutoff * (white - state);
        float t = (float)i / sampleRate;
        float env = 1.f;
        if (t < 0.01f) env = t / 0.01f;
        else if (t > dur * 0.3f) env = std::max(0.f, (dur - t) / (dur * 0.7f));
        shot->data[i] = state * env * vol * 0.4f;
    }
    ma_audio_buffer_config acfg = ma_audio_buffer_config_init(ma_format_f32, 1, shot->data.size(), shot->data.data(), nullptr);
    if (ma_audio_buffer_init(&acfg, &shot->buffer) != MA_SUCCESS) { delete shot; return; }
    if (ma_sound_init_from_data_source(&ae->engine, &shot->buffer, 0, nullptr, &shot->sound) != MA_SUCCESS) {
        ma_audio_buffer_uninit(&shot->buffer); delete shot; return;
    }
    ma_sound_start(&shot->sound);
    static std::vector<OneShot*> leaks;
    leaks.push_back(shot);
    // prune finished
    for (size_t i = 0; i < leaks.size(); ) {
        if (!ma_sound_is_playing(&leaks[i]->sound)) {
            ma_sound_uninit(&leaks[i]->sound);
            ma_audio_buffer_uninit(&leaks[i]->buffer);
            delete leaks[i];
            leaks.erase(leaks.begin() + i);
        } else ++i;
    }
}

void Audio::playLaser(bool player) {
    playTone(player ? 880.f : 620.f, 0.07f, 0.45f, 2);
    playTone(player ? 1400.f : 900.f, 0.05f, 0.25f, 0);
}
void Audio::playImpact() { playNoise(0.12f, 0.7f, 0.35f); playTone(180.f, 0.1f, 0.4f, 1); }
void Audio::playExplosion() {
    playNoise(0.45f, 0.9f, 0.2f);
    playTone(90.f, 0.35f, 0.55f, 1);
    playTone(55.f, 0.5f, 0.4f, 0);
}
void Audio::playThrust(float intensity) {
    thrustLevel = std::max(thrustLevel, intensity);
    if (intensity > 0.2f && (rand() % 8) == 0)
        playNoise(0.05f, 0.12f * intensity, 0.15f);
}
void Audio::playWarning() {
    if (warningCooldown > 0.f) return;
    warningCooldown = 0.8f;
    playTone(720.f, 0.12f, 0.5f, 1);
    playTone(540.f, 0.12f, 0.45f, 1);
}
void Audio::playUI() { playTone(660.f, 0.06f, 0.3f, 0); }
void Audio::playHit() { playNoise(0.08f, 0.55f, 0.4f); playTone(220.f, 0.08f, 0.35f, 1); }

} // namespace vd
