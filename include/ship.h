#pragma once
#include "common.h"

namespace vd {

struct ShipConfig {
    float mass = 1.f;
    float thrustAccel = 55.f;
    float reverseAccel = 35.f;
    float maxSpeed = 90.f;
    float linearDrag = 0.35f;       // light drag — inertia skill remains
    float dampAssist = 2.8f;        // Left Shift match-speed assist
    float pitchYawRate = 2.6f;      // rad/s target from mouse
    float rollRate = 3.2f;
    float angAccel = 14.f;
    float angDamp = 6.5f;
    float maxAngSpeed = 4.5f;
    float radius = 1.8f;
    float maxHealth = 100.f;
    float maxShield = 100.f;
    float shieldRegen = 8.f;
    float shieldDelay = 2.5f;
};

struct Ship {
    glm::vec3 position{0};
    glm::vec3 velocity{0};
    glm::quat orientation{1, 0, 0, 0};
    glm::vec3 angVel{0}; // local-space angular velocity
    float thrustInput = 0.f; // -1..1
    float pitchInput = 0.f;
    float yawInput = 0.f;
    float rollInput = 0.f;
    bool dampAssist = false;
    float health = 100.f;
    float shield = 100.f;
    float timeSinceHit = 99.f;
    bool alive = true;
    bool isPlayer = false;
    ShipConfig cfg;
    glm::vec3 color{0.2f, 0.9f, 1.f};
    float engineGlow = 0.f;
    int id = 0;

    glm::vec3 forward() const { return orientation * glm::vec3(0, 0, -1); }
    glm::vec3 up() const { return orientation * glm::vec3(0, 1, 0); }
    glm::vec3 right() const { return orientation * glm::vec3(1, 0, 0); }
    float speed() const { return glm::length(velocity); }

    void reset(const glm::vec3& pos, const glm::quat& ori = glm::quat(1,0,0,0));
    void update(float dt);
    void applyDamage(float dmg);
};

} // namespace vd
