#pragma once
#include "common.h"

namespace vd {

struct Particle {
    glm::vec3 pos, vel, color;
    float life, maxLife, size;
    bool streak = false;
};

struct ParticleSystem {
    std::vector<Particle> particles;
    void emitBurst(const glm::vec3& pos, const glm::vec3& color, int n, float speed, float life, RNG& rng);
    void emitExplosion(const glm::vec3& pos, const glm::vec3& baseColor, RNG& rng);
    void emitEngine(const glm::vec3& pos, const glm::vec3& back, float intensity, RNG& rng);
    void emitSparks(const glm::vec3& pos, const glm::vec3& normal, RNG& rng);
    void emitDust(const glm::vec3& camPos, const glm::vec3& vel, RNG& rng);
    void update(float dt);
};

struct Debris {
    glm::vec3 pos, vel, angVel;
    glm::quat ori{1,0,0,0};
    glm::vec3 color;
    float life, size;
    bool alive = true;
};

struct DebrisField {
    std::vector<Debris> pieces;
    void spawn(const glm::vec3& pos, const glm::vec3& color, RNG& rng);
    void update(float dt);
};

} // namespace vd
