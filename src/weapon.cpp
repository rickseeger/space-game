#include "weapon.h"

namespace vd {

bool Weapon::tryFire(Ship& ship, std::vector<Laser>& out) {
    if (cooldown > 0.f || !ship.alive) return false;
    cooldown = fireInterval;
    Laser l;
    l.position = ship.position + ship.forward() * 2.5f + ship.right() * 0.8f;
    // Dual barrels offset alternating via life noise
    static int side = 0;
    side ^= 1;
    l.position = ship.position + ship.forward() * 2.8f
               + ship.right() * (side ? 1.1f : -1.1f)
               + ship.up() * (-0.2f);
    // Inherit some ship velocity + muzzle along facing
    l.velocity = ship.velocity + ship.forward() * muzzleSpeed;
    l.color = color;
    l.life = range / muzzleSpeed;
    l.maxLife = l.life;
    l.damage = damage;
    l.ownerId = ship.id;
    l.alive = true;
    out.push_back(l);
    // second barrel slight delay feel — fire twin
    Laser l2 = l;
    l2.position = ship.position + ship.forward() * 2.8f
                + ship.right() * (side ? -1.1f : 1.1f)
                + ship.up() * (-0.2f);
    out.push_back(l2);
    return true;
}

void Weapon::update(float dt) {
    if (cooldown > 0.f) cooldown -= dt;
}

void updateLasers(std::vector<Laser>& lasers, float dt) {
    for (auto& l : lasers) {
        if (!l.alive) continue;
        l.position += l.velocity * dt;
        l.life -= dt;
        if (l.life <= 0.f) l.alive = false;
    }
    lasers.erase(std::remove_if(lasers.begin(), lasers.end(),
        [](const Laser& l) { return !l.alive; }), lasers.end());
}

bool laserHitsShip(const Laser& l, const Ship& s) {
    if (!l.alive || !s.alive || l.ownerId == s.id) return false;
    return glm::length2(l.position - s.position) <= (l.radius + s.cfg.radius) * (l.radius + s.cfg.radius);
}

} // namespace vd
