"""Player, NPCs, enemies, projectiles, and particles."""
from __future__ import annotations

import math
import random
from dataclasses import dataclass, field
from typing import Optional, Callable

import pygame

from .constants import (
    PLAYER_MAX_HP, PLAYER_SPEED, PLAYER_MELEE_DMG, PLAYER_MELEE_RANGE, PLAYER_MELEE_CD,
    PLAYER_XP_BASE, EMBER, EMBER_HOT, CREAM, INK, GOLD, HP_RED, ASH, WOOD, STONE,
    CYAN, MAGENTA, DUEL_GLOW, DUEL_GLOW2,
)
from .items import Inventory, CATALOG
from . import iso


def _norm(dx: float, dy: float) -> tuple[float, float]:
    l = math.hypot(dx, dy)
    if l < 1e-6:
        return 0.0, 0.0
    return dx / l, dy / l


@dataclass
class Particle:
    x: float
    y: float
    vx: float
    vy: float
    life: float
    color: tuple
    size: float = 3.0
    gravity: float = 0.0

    def update(self, dt: float) -> bool:
        self.x += self.vx * dt
        self.y += self.vy * dt
        self.vy += self.gravity * dt
        self.life -= dt
        return self.life > 0


@dataclass
class Projectile:
    x: float
    y: float
    vx: float
    vy: float
    dmg: int
    life: float = 0.9
    radius: float = 8.0
    from_player: bool = True
    color: tuple = EMBER_HOT

    def update(self, dt: float) -> bool:
        self.x += self.vx * dt
        self.y += self.vy * dt
        self.life -= dt
        return self.life > 0


@dataclass
class Player:
    x: float
    y: float
    hp: int = PLAYER_MAX_HP
    max_hp: int = PLAYER_MAX_HP
    xp: int = 0
    level: int = 1
    xp_next: int = PLAYER_XP_BASE
    inv: Inventory = field(default_factory=Inventory)
    facing: tuple[float, float] = (0.0, 1.0)
    attack_cd: float = 0.0
    attack_flash: float = 0.0
    hurt_flash: float = 0.0
    spell_unlocked: bool = False
    spell_mode: bool = False  # False=melee, True=emberbolt
    i_frames: float = 0.0
    alive: bool = True

    @property
    def speed(self) -> float:
        return PLAYER_SPEED + self.inv.boots_item().speed

    @property
    def melee_dmg(self) -> int:
        return self.inv.weapon_item().dmg + (self.level - 1)

    @property
    def armor(self) -> int:
        return self.inv.armor_item().armor

    def xp_to_level(self) -> None:
        while self.xp >= self.xp_next:
            self.xp -= self.xp_next
            self.level += 1
            self.xp_next = int(PLAYER_XP_BASE * (1.45 ** (self.level - 1)))
            self.max_hp += 8
            self.hp = self.max_hp
            if self.level >= 3:
                self.spell_unlocked = True
            yield self.level

    def take_damage(self, raw: int) -> int:
        if self.i_frames > 0 or not self.alive:
            return 0
        dmg = max(1, raw - self.armor)
        self.hp -= dmg
        self.hurt_flash = 0.25
        self.i_frames = 0.45
        if self.hp <= 0:
            self.hp = 0
            self.alive = False
        return dmg

    def heal(self, amount: int) -> int:
        before = self.hp
        self.hp = min(self.max_hp, self.hp + amount)
        return self.hp - before

    def update(self, dt: float, world, keys) -> None:
        if not self.alive:
            return
        self.attack_cd = max(0.0, self.attack_cd - dt)
        self.attack_flash = max(0.0, self.attack_flash - dt)
        self.hurt_flash = max(0.0, self.hurt_flash - dt)
        self.i_frames = max(0.0, self.i_frames - dt)

        mx = my = 0.0
        if keys[pygame.K_w] or keys[pygame.K_UP]:
            my -= 1
        if keys[pygame.K_s] or keys[pygame.K_DOWN]:
            my += 1
        if keys[pygame.K_a] or keys[pygame.K_LEFT]:
            mx -= 1
        if keys[pygame.K_d] or keys[pygame.K_RIGHT]:
            mx += 1
        if mx or my:
            mx, my = _norm(mx, my)
            # iso-friendly: screen-up is -x-y-ish; keep cartesian tile move
            self.facing = (mx, my)
            nx = self.x + mx * self.speed * dt / 64.0
            ny = self.y + my * self.speed * dt / 64.0
            # collision — separate axes
            if not world.solid(nx, self.y):
                self.x = nx
            if not world.solid(self.x, ny):
                self.y = ny
            # clamp
            self.x = max(0.3, min(79.7, self.x))
            self.y = max(0.3, min(79.7, self.y))

    def try_attack(self) -> Optional[str]:
        if self.attack_cd > 0 or not self.alive:
            return None
        if self.spell_mode and self.spell_unlocked:
            self.attack_cd = 0.55
            self.attack_flash = 0.2
            return "spell"
        self.attack_cd = PLAYER_MELEE_CD
        self.attack_flash = 0.18
        return "melee"


@dataclass
class NPC:
    x: float
    y: float
    name: str
    role: str
    color: tuple
    lines: list[str]
    shop: list[str] = field(default_factory=list)  # item ids
    quest_id: Optional[str] = None
    talk_idx: int = 0

    def next_line(self) -> str:
        line = self.lines[self.talk_idx % len(self.lines)]
        self.talk_idx += 1
        return line


@dataclass
class Enemy:
    x: float
    y: float
    kind: str
    hp: int
    max_hp: int
    dmg: int
    speed: float
    xp: int
    gold: int
    color: tuple
    radius: float = 0.35
    aggro: float = 5.5
    attack_range: float = 0.7
    attack_cd: float = 0.0
    flash: float = 0.0
    alive: bool = True
    boss: bool = False
    drop: Optional[str] = None
    wander_t: float = 0.0
    wx: float = 0.0
    wy: float = 0.0

    def update(self, dt: float, player: Player, world, in_duel: bool = False) -> Optional[int]:
        if not self.alive:
            return None
        self.attack_cd = max(0.0, self.attack_cd - dt)
        self.flash = max(0.0, self.flash - dt)
        dx = player.x - self.x
        dy = player.y - self.y
        dist = math.hypot(dx, dy)
        hit = None
        chase = in_duel or (dist < self.aggro and not world.is_town(player.x, player.y))
        if chase and player.alive:
            if dist > self.attack_range:
                nx, ny = _norm(dx, dy)
                sp = self.speed * (1.25 if in_duel else 1.0)
                tx = self.x + nx * sp * dt / 64.0
                ty = self.y + ny * sp * dt / 64.0
                if not world.solid(tx, self.y):
                    self.x = tx
                if not world.solid(self.x, ty):
                    self.y = ty
            elif self.attack_cd <= 0:
                self.attack_cd = 0.85 if not self.boss else 0.7
                hit = self.dmg
        else:
            self.wander_t -= dt
            if self.wander_t <= 0:
                self.wander_t = random.uniform(1.2, 2.8)
                self.wx = random.uniform(-1, 1)
                self.wy = random.uniform(-1, 1)
            tx = self.x + self.wx * self.speed * 0.35 * dt / 64.0
            ty = self.y + self.wy * self.speed * 0.35 * dt / 64.0
            if not world.solid(tx, self.y):
                self.x = tx
            if not world.solid(self.x, ty):
                self.y = ty
        return hit

    def take_damage(self, dmg: int) -> bool:
        self.hp -= dmg
        self.flash = 0.2
        if self.hp <= 0:
            self.hp = 0
            self.alive = False
            return True
        return False


def make_enemy(kind: str, x: float, y: float, diff: float) -> Enemy:
    d = max(0.0, diff)
    if kind == "slime":
        return Enemy(x, y, kind, hp=12 + int(8 * d), max_hp=12 + int(8 * d),
                     dmg=4 + int(3 * d), speed=55 + 10 * d, xp=6 + int(4 * d),
                     gold=random.randint(1, 3 + int(2 * d)),
                     color=(90, 240, 100), aggro=4.5)
    if kind == "wolf":
        return Enemy(x, y, kind, hp=18 + int(12 * d), max_hp=18 + int(12 * d),
                     dmg=7 + int(4 * d), speed=95 + 15 * d, xp=12 + int(6 * d),
                     gold=random.randint(3, 8 + int(3 * d)),
                     color=(180, 160, 220), aggro=6.5, drop="wolf_pelt" if __import__("random").random() < 0.7 else None)
    if kind == "bandit":
        return Enemy(x, y, kind, hp=28 + int(14 * d), max_hp=28 + int(14 * d),
                     dmg=9 + int(5 * d), speed=80 + 10 * d, xp=18 + int(8 * d),
                     gold=random.randint(6, 14 + int(4 * d)),
                     color=(255, 90, 70), aggro=6.0, drop="iron_ore")
    if kind == "ash_drake":
        return Enemy(x, y, kind, hp=160, max_hp=160, dmg=14, speed=100,
                     xp=120, gold=80, color=(255, 80, 50), aggro=8.0,
                     attack_range=1.0, boss=True, drop="ember_relic", radius=0.55)
    return make_enemy("slime", x, y, d)


def draw_shadow(surf, sx, sy, r=10):
    s = pygame.Surface((r * 2, r), pygame.SRCALPHA)
    pygame.draw.ellipse(s, (0, 0, 0, 70), (0, 0, r * 2, r))
    surf.blit(s, (sx - r, sy - r // 2))


def draw_player(surf, player: Player, cam_x, cam_y, t: float) -> None:
    sx, sy = iso.world_to_screen(player.x, player.y, cam_x, cam_y)
    draw_shadow(surf, sx, sy + 4, 12)
    bob = math.sin(t * 8) * 1.5 if (abs(player.facing[0]) + abs(player.facing[1])) else 0
    body = (255, 210, 160) if player.hurt_flash <= 0 else (255, 120, 140)
    # robe — vivid violet with ember trim; upgrades to crimson-orange
    robe = (150, 70, 220) if player.level < 5 else (230, 60, 90)
    trim = EMBER_HOT
    pygame.draw.ellipse(surf, robe, (sx - 9, sy - 18 + bob, 18, 22))
    pygame.draw.ellipse(surf, trim, (sx - 9, sy - 4 + bob, 18, 6), 2)
    # head
    pygame.draw.circle(surf, body, (int(sx), int(sy - 22 + bob)), 7)
    # hat — bright with gold band
    pygame.draw.polygon(surf, (90, 40, 180) if player.level < 5 else (200, 40, 60), [
        (sx - 9, sy - 24 + bob), (sx + 9, sy - 24 + bob), (sx, sy - 38 + bob)
    ])
    pygame.draw.polygon(surf, EMBER, [
        (sx - 4, sy - 34 + bob), (sx + 4, sy - 34 + bob), (sx, sy - 40 + bob)
    ])
    pygame.draw.line(surf, GOLD, (sx - 8, sy - 24 + bob), (sx + 8, sy - 24 + bob), 2)
    # staff
    fx, fy = player.facing
    staff_x = sx + fx * 14 - fy * 4
    staff_y = sy - 10 + bob + fy * 8
    pygame.draw.line(surf, WOOD, (sx + 4, sy - 8 + bob), (staff_x, staff_y - 16), 3)
    tip = EMBER_HOT if player.spell_mode and player.spell_unlocked else GOLD
    pygame.draw.circle(surf, tip, (int(staff_x), int(staff_y - 16)), 5)
    pygame.draw.circle(surf, CREAM, (int(staff_x), int(staff_y - 16)), 2)
    if player.attack_flash > 0:
        if player.spell_mode and player.spell_unlocked:
            pygame.draw.circle(surf, EMBER, (int(staff_x), int(staff_y - 16)), 14, 2)
            pygame.draw.circle(surf, CYAN, (int(staff_x), int(staff_y - 16)), 8, 1)
        else:
            ang = math.atan2(fy, fx)
            for a in (-0.6, -0.2, 0.2, 0.6):
                px = sx + math.cos(ang + a) * 22
                py = sy - 12 + math.sin(ang + a) * 14
                pygame.draw.circle(surf, EMBER_HOT, (int(px), int(py)), 4)
                pygame.draw.circle(surf, CREAM, (int(px), int(py)), 2)


def draw_npc(surf, npc: NPC, cam_x, cam_y, t: float) -> None:
    sx, sy = iso.world_to_screen(npc.x, npc.y, cam_x, cam_y)
    draw_shadow(surf, sx, sy + 3, 10)
    bob = math.sin(t * 3 + npc.x) * 1.2
    pygame.draw.ellipse(surf, npc.color, (sx - 8, sy - 16 + bob, 16, 20))
    pygame.draw.circle(surf, (230, 190, 150), (int(sx), int(sy - 20 + bob)), 6)
    # talk indicator
    pygame.draw.circle(surf, (255, 240, 80), (int(sx), int(sy - 34 + bob)), 5)
    pygame.draw.circle(surf, (255, 120, 40), (int(sx), int(sy - 34 + bob)), 3)
    pygame.draw.circle(surf, CREAM, (int(sx), int(sy - 34 + bob)), 1)


def draw_enemy(surf, e: Enemy, cam_x, cam_y, t: float) -> None:
    if not e.alive:
        return
    sx, sy = iso.world_to_screen(e.x, e.y, cam_x, cam_y)
    draw_shadow(surf, sx, sy + 3, 12 if e.boss else 9)
    col = (255, 255, 255) if e.flash > 0 else e.color
    if e.kind == "slime":
        pygame.draw.ellipse(surf, col, (sx - 10, sy - 10, 20, 14))
        pygame.draw.circle(surf, INK, (int(sx - 3), int(sy - 6)), 2)
        pygame.draw.circle(surf, INK, (int(sx + 4), int(sy - 6)), 2)
    elif e.kind == "wolf":
        pygame.draw.ellipse(surf, col, (sx - 12, sy - 12, 24, 14))
        pygame.draw.circle(surf, col, (int(sx + 10), int(sy - 12)), 5)
        pygame.draw.polygon(surf, col, [(sx + 8, sy - 16), (sx + 10, sy - 22), (sx + 14, sy - 14)])
    elif e.kind == "bandit":
        pygame.draw.ellipse(surf, col, (sx - 8, sy - 16, 16, 20))
        pygame.draw.circle(surf, (200, 160, 120), (int(sx), int(sy - 20)), 6)
        pygame.draw.line(surf, STONE, (sx + 6, sy - 10), (sx + 16, sy - 18), 2)
    elif e.kind == "ash_drake":
        # dragon-ish
        pygame.draw.ellipse(surf, col, (sx - 18, sy - 20, 36, 24))
        pygame.draw.circle(surf, col, (int(sx + 16), int(sy - 24)), 10)
        pygame.draw.polygon(surf, (180, 50, 30), [
            (sx - 10, sy - 22), (sx - 4, sy - 40), (sx + 4, sy - 22)
        ])
        pygame.draw.polygon(surf, (180, 50, 30), [
            (sx + 4, sy - 22), (sx + 12, sy - 38), (sx + 16, sy - 20)
        ])
        eye = CYAN if math.sin(t * 6) > 0 else EMBER_HOT
        pygame.draw.circle(surf, eye, (int(sx + 20), int(sy - 26)), 3)
        # HP bar for boss always
    # small HP if damaged
    if e.hp < e.max_hp:
        bw = 28 if e.boss else 20
        pygame.draw.rect(surf, (40, 20, 20), (sx - bw // 2, sy - 36 if e.boss else sy - 28, bw, 4))
        pygame.draw.rect(surf, HP_RED, (sx - bw // 2, sy - 36 if e.boss else sy - 28,
                                        int(bw * e.hp / e.max_hp), 4))


def draw_projectile(surf, p: Projectile, cam_x, cam_y) -> None:
    sx, sy = iso.world_to_screen(p.x, p.y, cam_x, cam_y)
    r = int(p.radius)
    pygame.draw.circle(surf, CYAN, (int(sx), int(sy - 10)), r + 4, 2)
    pygame.draw.circle(surf, p.color, (int(sx), int(sy - 10)), r)
    pygame.draw.circle(surf, MAGENTA, (int(sx), int(sy - 10)), max(3, r // 2))
    pygame.draw.circle(surf, CREAM, (int(sx), int(sy - 10)), 2)


def spawn_hit_particles(parts: list, x: float, y: float, color=EMBER_HOT, n=8) -> None:
    for _ in range(n):
        ang = random.uniform(0, math.tau)
        sp = random.uniform(20, 70)
        parts.append(Particle(
            x, y, math.cos(ang) * sp / 64, math.sin(ang) * sp / 64,
            life=random.uniform(0.2, 0.5), color=color, size=random.uniform(2, 5)
        ))
