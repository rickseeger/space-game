#include "enemy_ai.h"
#include <cmath>

namespace vd {

const char* aiStateName(AIState s) {
    switch (s) {
        case AIState::Search: return "SEARCH";
        case AIState::Pursue: return "PURSUE";
        case AIState::Attack: return "ATTACK";
        case AIState::Evade: return "EVADE";
        case AIState::Reposition: return "REPOS";
    }
    return "?";
}

static void aimToward(Ship& ship, const glm::vec3& desiredFwd, float strength) {
    glm::vec3 fwd = ship.forward();
    glm::vec3 axis = glm::cross(fwd, desiredFwd);
    float sinA = glm::length(axis);
    float cosA = clampf(glm::dot(fwd, desiredFwd), -1.f, 1.f);
    if (sinA < 1e-5f) {
        ship.pitchInput = ship.yawInput = 0.f;
        return;
    }
    axis /= sinA;
    // Convert world rotation axis into local pitch/yaw/roll desires
    glm::vec3 local = glm::inverse(ship.orientation) * axis;
    float angle = std::atan2(sinA, cosA);
    float cmd = clampf(angle * strength, -1.f, 1.f);
    ship.pitchInput = clampf(local.x * cmd * 2.5f, -1.f, 1.f);
    ship.yawInput   = clampf(local.y * cmd * 2.5f, -1.f, 1.f);
    ship.rollInput  = clampf(-local.z * cmd * 0.8f, -1.f, 1.f);
}

static glm::vec3 interceptPoint(const Ship& self, const Ship& target, float shotSpeed) {
    glm::vec3 to = target.position - self.position;
    glm::vec3 relVel = target.velocity - self.velocity;
    // Quadratic intercept approximation
    float a = glm::dot(relVel, relVel) - shotSpeed * shotSpeed;
    float b = 2.f * glm::dot(to, relVel);
    float c = glm::dot(to, to);
    float t = 0.f;
    if (std::fabs(a) < 1e-3f) {
        t = (std::fabs(b) > 1e-3f) ? -c / b : 0.f;
    } else {
        float disc = b * b - 4.f * a * c;
        if (disc > 0.f) {
            float s = std::sqrt(disc);
            float t1 = (-b - s) / (2.f * a);
            float t2 = (-b + s) / (2.f * a);
            t = (t1 > 0.f) ? t1 : t2;
        }
    }
    t = clampf(t, 0.f, 2.5f);
    return target.position + target.velocity * t;
}

void updateEnemyAI(Enemy& e, const Ship& player, std::vector<Laser>& lasers, float dt, RNG& rng) {
    Ship& s = e.ship;
    if (!s.alive) return;

    e.stateTimer -= dt;
    e.fireHold = std::max(0.f, e.fireHold - dt);
    e.overshootTimer = std::max(0.f, e.overshootTimer - dt);
    e.weapon.update(dt);

    float dist = glm::length(player.position - s.position);
    float closing = glm::dot(safeNormalize(player.position - s.position), s.velocity - player.velocity);
    bool playerAlive = player.alive;

    // State transitions
    auto go = [&](AIState st, float minTime) {
        e.state = st;
        e.stateTimer = minTime + rng.uniform(0.f, 0.6f);
    };

    if (!playerAlive) {
        go(AIState::Search, 2.f);
    } else if (e.state == AIState::Search) {
        if (dist < 320.f) go(AIState::Pursue, 1.5f);
        if (e.stateTimer <= 0.f) {
            e.searchTarget = player.position + rng.inSphere(180.f);
            e.stateTimer = 3.f;
        }
    } else if (e.state == AIState::Pursue) {
        if (dist < 90.f) go(AIState::Attack, 1.2f);
        else if (dist > 400.f) go(AIState::Search, 2.f);
        if (s.health < s.cfg.maxHealth * 0.35f && rng.uniform() < 0.01f)
            go(AIState::Evade, 1.5f);
    } else if (e.state == AIState::Attack) {
        if (dist < 28.f || e.overshootTimer > 0.f) {
            go(AIState::Reposition, 1.0f);
            e.overshootTimer = 0.8f + rng.uniform(0.f, 0.5f);
        } else if (dist > 160.f) {
            go(AIState::Pursue, 1.f);
        } else if (e.stateTimer <= 0.f && rng.uniform() > e.aggression) {
            go(AIState::Reposition, 0.8f);
        }
    } else if (e.state == AIState::Evade) {
        if (e.stateTimer <= 0.f) go(AIState::Reposition, 1.f);
    } else if (e.state == AIState::Reposition) {
        if (e.stateTimer <= 0.f) {
            if (dist < 120.f) go(AIState::Attack, 1.f);
            else go(AIState::Pursue, 1.f);
        }
    }

    // Control outputs
    s.pitchInput = s.yawInput = s.rollInput = 0.f;
    s.thrustInput = 0.f;
    s.dampAssist = false;

    glm::vec3 toPlayer = player.position - s.position;
    glm::vec3 dirPlayer = safeNormalize(toPlayer);

    switch (e.state) {
    case AIState::Search: {
        glm::vec3 dest = e.searchTarget;
        if (playerAlive) dest = glm::mix(dest, player.position, 0.35f);
        aimToward(s, safeNormalize(dest - s.position), 0.9f);
        s.thrustInput = 0.55f;
        break;
    }
    case AIState::Pursue: {
        glm::vec3 lead = interceptPoint(s, player, e.weapon.muzzleSpeed * 0.85f);
        // Approach from side sometimes
        glm::vec3 side = safeNormalize(glm::cross(dirPlayer, s.up()));
        lead += side * std::sin(e.stateTimer * 2.f) * 20.f;
        aimToward(s, safeNormalize(lead - s.position), 1.1f);
        s.thrustInput = (dist > 100.f) ? 0.95f : 0.5f;
        break;
    }
    case AIState::Attack: {
        glm::vec3 aimPt = interceptPoint(s, player, e.weapon.muzzleSpeed);
        // Imperfect aim
        aimPt += rng.unit() * e.aimJitter * dist * 0.22f;
        glm::vec3 aimDir = safeNormalize(aimPt - s.position);
        aimToward(s, aimDir, 1.3f);
        s.thrustInput = 0.45f + 0.3f * e.aggression;

        float align = glm::dot(s.forward(), aimDir);
        float maxRange = e.weapon.range * 0.9f;
        // Burst fire with cooldown windows — do NOT continuous surround fire
        if (align > 0.94f && dist < maxRange && dist > 42.f && e.fireHold <= 0.f) {
            if (e.weapon.tryFire(s, lasers)) {
                // Short burst then hold
                static thread_local int burst = 0;
                burst++;
                if (burst >= 2) {
                    burst = 0;
                    e.fireHold = 0.75f + rng.uniform(0.2f, 0.7f);
                }
            }
        }
        // Detect overshoot
        if (closing < -10.f && dist < 50.f)
            e.overshootTimer = 1.f;
        break;
    }
    case AIState::Evade: {
        glm::vec3 away = -dirPlayer + s.right() * (rng.uniform() > 0.5f ? 1.f : -1.f) + s.up() * rng.uniform(-0.5f, 0.5f);
        aimToward(s, safeNormalize(away), 1.4f);
        s.thrustInput = 1.f;
        s.rollInput = (rng.uniform() > 0.5f ? 1.f : -1.f);
        break;
    }
    case AIState::Reposition: {
        glm::vec3 flank = safeNormalize(glm::cross(dirPlayer, glm::vec3(0,1,0)));
        if (glm::length2(flank) < 0.1f) flank = s.right();
        glm::vec3 dest = player.position + flank * 90.f + rng.unit() * 30.f;
        aimToward(s, safeNormalize(dest - s.position), 1.0f);
        s.thrustInput = 0.85f;
        break;
    }
    }

    // Scale AI ship physics slightly
    s.update(dt);
}

} // namespace vd
