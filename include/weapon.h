#pragma once
#include "common.h"
#include "ship.h"

namespace vd {

struct Laser {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 color;
    float life = 1.2f;
    float maxLife = 1.2f;
    float damage = 18.f;
    float radius = 0.6f;
    int ownerId = -1;
    bool alive = true;
};

struct Weapon {
    float cooldown = 0.f;
    float fireInterval = 0.16f;
    float muzzleSpeed = 220.f;
    float damage = 18.f;
    float range = 260.f; // derived from speed * life
    glm::vec3 color{0.3f, 1.f, 1.f};

    bool tryFire(Ship& ship, std::vector<Laser>& out);
    void update(float dt);
};

void updateLasers(std::vector<Laser>& lasers, float dt);
bool laserHitsShip(const Laser& l, const Ship& s);

} // namespace vd
