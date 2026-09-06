#pragma once
#include "common.h"

namespace vd {

class Audio {
public:
    bool init();
    void shutdown();
    void update(float dt);

    void playLaser(bool player);
    void playImpact();
    void playExplosion();
    void playThrust(float intensity); // continuous
    void playWarning();
    void playUI();
    void playHit();

private:
    void* engine = nullptr; // ma_engine*
    bool ok = false;
    float thrustLevel = 0.f;
    float warningCooldown = 0.f;
    // Procedural oneshots via miniaudio waveform / noise buffers
    void playTone(float freq, float dur, float vol, int type);
    void playNoise(float dur, float vol, float cutoff);
};

} // namespace vd
