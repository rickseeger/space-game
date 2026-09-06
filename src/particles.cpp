#include "particles.h"

namespace vd {

void ParticleSystem::emitBurst(const glm::vec3& pos, const glm::vec3& color, int n, float speed, float life, RNG& rng) {
    for (int i = 0; i < n; i++) {
        Particle p;
        p.pos = pos;
        p.vel = rng.unit() * rng.uniform(speed * 0.3f, speed);
        p.color = color * rng.uniform(0.7f, 1.2f);
        p.life = p.maxLife = life * rng.uniform(0.5f, 1.f);
        p.size = rng.uniform(1.5f, 4.f);
        particles.push_back(p);
    }
}

void ParticleSystem::emitExplosion(const glm::vec3& pos, const glm::vec3& baseColor, RNG& rng) {
    emitBurst(pos, baseColor, 70, 70.f, 1.1f, rng);
    emitBurst(pos, glm::vec3(1.f, 0.95f, 0.6f), 40, 45.f, 0.6f, rng);
    emitBurst(pos, glm::vec3(1.f, 0.35f, 0.08f), 35, 90.f, 0.85f, rng);
    emitBurst(pos, glm::vec3(1.f, 1.f, 1.f), 15, 25.f, 0.25f, rng);
    // lingering smoke-ish
    for (int i = 0; i < 15; i++) {
        Particle p;
        p.pos = pos + rng.inSphere(2.f);
        p.vel = rng.unit() * rng.uniform(2.f, 12.f);
        p.color = glm::vec3(0.4f, 0.35f, 0.3f) * rng.uniform(0.5f, 1.f);
        p.life = p.maxLife = rng.uniform(0.8f, 1.6f);
        p.size = rng.uniform(4.f, 10.f);
        particles.push_back(p);
    }
}

void ParticleSystem::emitEngine(const glm::vec3& pos, const glm::vec3& back, float intensity, RNG& rng) {
    if (intensity < 0.05f) return;
    int n = 1 + (int)(intensity * 3.f);
    for (int i = 0; i < n; i++) {
        Particle p;
        p.pos = pos + rng.inSphere(0.3f);
        p.vel = back * rng.uniform(20.f, 50.f) * intensity + rng.unit() * 3.f;
        p.color = glm::mix(glm::vec3(0.2f, 0.6f, 1.f), glm::vec3(1.f, 0.8f, 0.3f), rng.uniform());
        p.life = p.maxLife = rng.uniform(0.08f, 0.2f);
        p.size = rng.uniform(1.2f, 2.8f) * intensity;
        particles.push_back(p);
    }
}

void ParticleSystem::emitSparks(const glm::vec3& pos, const glm::vec3& normal, RNG& rng) {
    for (int i = 0; i < 12; i++) {
        Particle p;
        p.pos = pos;
        p.vel = safeNormalize(normal + rng.unit() * 0.8f) * rng.uniform(15.f, 45.f);
        p.color = glm::vec3(1.f, rng.uniform(0.5f, 1.f), 0.2f);
        p.life = p.maxLife = rng.uniform(0.15f, 0.4f);
        p.size = rng.uniform(1.f, 2.5f);
        particles.push_back(p);
    }
}

void ParticleSystem::emitDust(const glm::vec3& camPos, const glm::vec3& vel, RNG& rng) {
    float spd = glm::length(vel);
    if (spd < 8.f) return;
    if (rng.uniform() > 0.4f) return;
    Particle p;
    p.pos = camPos + rng.unit() * rng.uniform(5.f, 40.f);
    p.vel = -vel * 0.15f + rng.unit() * 2.f;
    p.color = glm::vec3(0.55f, 0.7f, 0.9f) * rng.uniform(0.3f, 0.7f);
    p.life = p.maxLife = rng.uniform(0.3f, 0.8f);
    p.size = rng.uniform(0.8f, 2.f);
    p.streak = true;
    particles.push_back(p);
}

void ParticleSystem::update(float dt) {
    for (auto& p : particles) {
        p.pos += p.vel * dt;
        p.vel *= std::exp(-1.5f * dt);
        p.life -= dt;
        float t = p.life / p.maxLife;
        p.color *= 0.995f;
        p.size = p.size; // keep
        (void)t;
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(),
        [](const Particle& p) { return p.life <= 0.f; }), particles.end());
    if (particles.size() > 2500)
        particles.erase(particles.begin(), particles.begin() + (particles.size() - 2500));
}

void DebrisField::spawn(const glm::vec3& pos, const glm::vec3& color, RNG& rng) {
    for (int i = 0; i < 22; i++) {
        Debris d;
        d.pos = pos + rng.inSphere(1.5f);
        d.vel = rng.unit() * rng.uniform(8.f, 40.f);
        d.angVel = rng.unit() * rng.uniform(2.f, 8.f);
        d.ori = glm::normalize(glm::quat(rng.uniform(-1,1), rng.uniform(-1,1), rng.uniform(-1,1), rng.uniform(-1,1)));
        d.color = color * rng.uniform(0.5f, 1.1f);
        d.life = rng.uniform(1.2f, 2.8f);
        d.size = rng.uniform(0.25f, 0.9f);
        d.alive = true;
        pieces.push_back(d);
    }
}

void DebrisField::update(float dt) {
    for (auto& d : pieces) {
        if (!d.alive) continue;
        d.pos += d.vel * dt;
        d.vel *= std::exp(-0.6f * dt);
        glm::quat spin(0, d.angVel.x, d.angVel.y, d.angVel.z);
        d.ori = glm::normalize(d.ori + (d.ori * spin) * (0.5f * dt));
        d.life -= dt;
        if (d.life <= 0.f) d.alive = false;
    }
    pieces.erase(std::remove_if(pieces.begin(), pieces.end(),
        [](const Debris& d) { return !d.alive; }), pieces.end());
}

} // namespace vd
