#pragma once
#include "common.h"
#include "ship.h"
#include "weapon.h"

namespace vd {

enum class AIState { Search, Pursue, Attack, Evade, Reposition };

struct Enemy {
    Ship ship;
    Weapon weapon;
    AIState state = AIState::Search;
    float stateTimer = 0.f;
    float aimJitter = 0.12f;
    float aggression = 0.7f;
    glm::vec3 searchTarget{0};
    float fireHold = 0.f; // prevent continuous fire spam
    float overshootTimer = 0.f;
};

void updateEnemyAI(Enemy& e, const Ship& player, std::vector<Laser>& lasers, float dt, RNG& rng);
const char* aiStateName(AIState s);

} // namespace vd
