#include "ship.h"

namespace vd {

void Ship::reset(const glm::vec3& pos, const glm::quat& ori) {
    position = pos;
    velocity = glm::vec3(0);
    orientation = glm::normalize(ori);
    angVel = glm::vec3(0);
    thrustInput = pitchInput = yawInput = rollInput = 0.f;
    dampAssist = false;
    health = cfg.maxHealth;
    shield = cfg.maxShield;
    timeSinceHit = 99.f;
    alive = true;
    engineGlow = 0.f;
}

void Ship::update(float dt) {
    if (!alive) return;
    timeSinceHit += dt;

    // Desired angular velocity from stick/mouse (local space)
    glm::vec3 desiredAng(
        pitchInput * cfg.pitchYawRate,
        yawInput * cfg.pitchYawRate,
        rollInput * cfg.rollRate
    );
    glm::vec3 angErr = desiredAng - angVel;
    angVel += angErr * std::min(1.f, cfg.angAccel * dt);
    // Soft damp residual spin when stick released
    float stick = std::fabs(pitchInput) + std::fabs(yawInput) + std::fabs(rollInput);
    float damp = (stick < 0.05f) ? cfg.angDamp : cfg.angDamp * 0.25f;
    angVel *= std::exp(-damp * dt);
    float angSpd = glm::length(angVel);
    if (angSpd > cfg.maxAngSpeed)
        angVel *= cfg.maxAngSpeed / angSpd;

    // Integrate orientation from local angular velocity
    // Turning changes orientation ONLY — does not rotate velocity
    glm::quat spin(0, angVel.x, angVel.y, angVel.z);
    orientation += (orientation * spin) * (0.5f * dt);
    orientation = glm::normalize(orientation);

    // Thrust along facing — gradually changes velocity
    glm::vec3 fwd = forward();
    float accel = 0.f;
    if (thrustInput > 0.f) accel = thrustInput * cfg.thrustAccel;
    else if (thrustInput < 0.f) accel = thrustInput * cfg.reverseAccel;
    velocity += fwd * accel * dt;

    // Light linear drag
    velocity *= std::exp(-cfg.linearDrag * dt);

    // Velocity damp / match-speed assist (Shift)
    if (dampAssist) {
        velocity *= std::exp(-cfg.dampAssist * dt);
    }

    float spd = glm::length(velocity);
    if (spd > cfg.maxSpeed)
        velocity *= cfg.maxSpeed / spd;

    position += velocity * dt;

    engineGlow = lerpf(engineGlow, std::max(0.f, thrustInput), 1.f - std::exp(-10.f * dt));

    // Shield regen after delay
    if (timeSinceHit > cfg.shieldDelay && shield < cfg.maxShield) {
        shield = std::min(cfg.maxShield, shield + cfg.shieldRegen * dt);
    }
}

void Ship::applyDamage(float dmg) {
    if (!alive) return;
    timeSinceHit = 0.f;
    if (shield > 0.f) {
        float absorb = std::min(shield, dmg);
        shield -= absorb;
        dmg -= absorb;
    }
    if (dmg > 0.f) {
        health -= dmg;
        if (health <= 0.f) {
            health = 0.f;
            alive = false;
        }
    }
}

} // namespace vd
