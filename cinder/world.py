"""Procedural isometric world: castle, village, wilds, difficulty by distance."""
from __future__ import annotations

import math
import random
from dataclasses import dataclass, field

import pygame

from .constants import (
    MAP_W, MAP_H, CASTLE_CX, CASTLE_CY, VILLAGE_CX, VILLAGE_CY, BOSS_CX, BOSS_CY,
    GRASS, GRASS_DK, FOREST, PATH, PATH_DK, WATER, WATER_DK, STONE, STONE_LT,
    CASTLE, CASTLE_DK, ROOF, ROOF_TEAL, ROOF_YELLOW, WOOD, ASH, CREAM, GOLD,
    TILE_W, TILE_H, SKY, CYAN, MAGENTA, EMBER,
)
from . import iso

T_GRASS = 0
T_FOREST = 1
T_PATH = 2
T_WATER = 3
T_FLOOR = 4
T_WALL = 5
T_DOOR = 6
T_STAIRS = 7
T_RUG = 8
T_DIRT = 9
T_ASH = 10
T_FLOWER = 11


@dataclass
class World:
    tiles: list[list[int]] = field(default_factory=list)
    blocked: list[list[bool]] = field(default_factory=list)
    spawn: tuple[float, float] = (CASTLE_CX + 1.5, CASTLE_CY + 2.5)
    inn_spawn: tuple[float, float] = (VILLAGE_CX + 1.0, VILLAGE_CY + 1.5)
    boss_tile: tuple[int, int] = (BOSS_CX, BOSS_CY)
    labels: list[tuple[float, float, str]] = field(default_factory=list)
    roof_colors: dict[tuple[int, int], tuple] = field(default_factory=dict)

    def in_bounds(self, x: int, y: int) -> bool:
        return 0 <= x < MAP_W and 0 <= y < MAP_H

    def solid(self, x: float, y: float) -> bool:
        ix, iy = int(x), int(y)
        if not self.in_bounds(ix, iy):
            return True
        return self.blocked[iy][ix]

    def difficulty(self, x: float, y: float) -> float:
        dx = x - CASTLE_CX
        dy = y - CASTLE_CY
        dist = math.hypot(dx, dy)
        return max(0.0, (dist - 8) / 35.0)

    def is_town(self, x: float, y: float) -> bool:
        if abs(x - CASTLE_CX) < 6 and abs(y - CASTLE_CY) < 6:
            return True
        if abs(x - VILLAGE_CX) < 7 and abs(y - VILLAGE_CY) < 6:
            return True
        return False


def _noise(x: int, y: int, seed: int = 7) -> float:
    n = math.sin(x * 12.9898 + y * 78.233 + seed) * 43758.5453
    return n - math.floor(n)


def generate_world(rng: random.Random | None = None) -> World:
    rng = rng or random.Random(42)
    tiles = [[T_GRASS for _ in range(MAP_W)] for _ in range(MAP_H)]
    blocked = [[False for _ in range(MAP_W)] for _ in range(MAP_H)]
    roof_colors: dict[tuple[int, int], tuple] = {}

    for y in range(MAP_H):
        for x in range(MAP_W):
            n = _noise(x, y)
            n2 = _noise(x, y, 99)
            n3 = _noise(x, y, 33)
            dist_start = math.hypot(x - CASTLE_CX, y - CASTLE_CY)
            if n > 0.72 and dist_start > 10:
                tiles[y][x] = T_FOREST
                blocked[y][x] = n > 0.88
            elif n2 > 0.93 and dist_start > 14:
                tiles[y][x] = T_WATER
                blocked[y][x] = True
            elif n < 0.12 and dist_start > 20:
                tiles[y][x] = T_ASH
            elif n3 > 0.86 and dist_start > 6 and tiles[y][x] == T_GRASS:
                tiles[y][x] = T_FLOWER

    def carve_path(ax, ay, bx, by, wobble=0.4):
        steps = int(math.hypot(bx - ax, by - ay) * 2) + 1
        for i in range(steps + 1):
            t = i / max(1, steps)
            px = ax + (bx - ax) * t + math.sin(t * 9) * wobble
            py = ay + (by - ay) * t + math.cos(t * 7) * wobble
            for oy in (-1, 0, 1):
                for ox in (-1, 0):
                    ix, iy = int(px + ox), int(py + oy)
                    if 0 <= ix < MAP_W and 0 <= iy < MAP_H:
                        tiles[iy][ix] = T_PATH
                        blocked[iy][ix] = False

    carve_path(CASTLE_CX + 3, CASTLE_CY + 3, VILLAGE_CX, VILLAGE_CY)
    carve_path(VILLAGE_CX + 3, VILLAGE_CY + 2, BOSS_CX - 2, BOSS_CY - 2, 0.8)

    cx, cy = CASTLE_CX, CASTLE_CY
    for y in range(cy - 4, cy + 5):
        for x in range(cx - 4, cx + 5):
            if 0 <= x < MAP_W and 0 <= y < MAP_H:
                edge = x in (cx - 4, cx + 4) or y in (cy - 4, cy + 4)
                if edge:
                    tiles[y][x] = T_WALL
                    blocked[y][x] = True
                    roof_colors[(x, y)] = ROOF  # keep walls topped crimson
                else:
                    tiles[y][x] = T_FLOOR
                    blocked[y][x] = False
    tiles[cy + 4][cx] = T_DOOR
    tiles[cy + 4][cx + 1] = T_DOOR
    blocked[cy + 4][cx] = False
    blocked[cy + 4][cx + 1] = False
    tiles[cy - 2][cx] = T_RUG
    tiles[cy - 1][cx] = T_STAIRS
    tiles[cy][cx] = T_STAIRS
    blocked[cy - 1][cx] = False
    blocked[cy][cx] = False

    vx, vy = VILLAGE_CX, VILLAGE_CY
    for y in range(vy - 3, vy + 4):
        for x in range(vx - 4, vx + 5):
            if 0 <= x < MAP_W and 0 <= y < MAP_H:
                tiles[y][x] = T_DIRT
                blocked[y][x] = False

    buildings = [
        (vx - 3, vy - 2, 2, 2, ROOF),         # inn — red
        (vx + 2, vy - 2, 2, 2, ROOF_TEAL),    # shop — teal
        (vx - 3, vy + 1, 2, 2, ROOF_YELLOW),  # smith — yellow
        (vx + 2, vy + 1, 2, 2, (255, 120, 200)),  # house — pink
    ]
    for bx, by, bw, bh, rcol in buildings:
        for y in range(by, by + bh):
            for x in range(bx, bx + bw):
                tiles[y][x] = T_WALL
                blocked[y][x] = True
                roof_colors[(x, y)] = rcol
        tiles[by + bh - 1][bx] = T_DOOR
        blocked[by + bh - 1][bx] = False

    for y in range(BOSS_CY - 3, BOSS_CY + 4):
        for x in range(BOSS_CX - 3, BOSS_CX + 4):
            if 0 <= x < MAP_W and 0 <= y < MAP_H:
                tiles[y][x] = T_ASH
                blocked[y][x] = False

    labels = [
        (cx, cy - 5, "Ashfall Keep"),
        (vx, vy - 4, "Ember Village"),
        (BOSS_CX, BOSS_CY - 4, "Cinder Nest"),
    ]

    return World(
        tiles=tiles,
        blocked=blocked,
        spawn=(cx + 0.5, cy + 2.5),
        inn_spawn=(vx - 2.2, vy - 0.5),
        boss_tile=(BOSS_CX, BOSS_CY),
        labels=labels,
        roof_colors=roof_colors,
    )


_TILE_COLORS = {
    T_GRASS: (GRASS, GRASS_DK),
    T_FOREST: (FOREST, (20, 110, 45)),
    T_PATH: (PATH, PATH_DK),
    T_WATER: (WATER, WATER_DK),
    T_FLOOR: (STONE_LT, STONE),
    T_WALL: (CASTLE, CASTLE_DK),
    T_DOOR: (WOOD, (140, 80, 35)),
    T_STAIRS: ((230, 210, 120), (180, 150, 70)),
    T_RUG: ((255, 70, 110), (200, 40, 80)),
    T_DIRT: ((240, 210, 130), (210, 175, 90)),
    T_ASH: (ASH, (150, 90, 140)),
    T_FLOWER: (GRASS, GRASS_DK),
}

_FLOWER_COLORS = [
    (255, 90, 140), (255, 220, 60), (255, 140, 40),
    (180, 100, 255), (80, 220, 255), (255, 255, 255),
]


def draw_world(surf: pygame.Surface, world: World, cam_x: float, cam_y: float, font_sm) -> None:
    # soft sky fill already from BG; add horizon wash
    margin = 14
    x0 = max(0, int(cam_x - margin))
    x1 = min(MAP_W, int(cam_x + margin))
    y0 = max(0, int(cam_y - margin))
    y1 = min(MAP_H, int(cam_y + margin))

    cells = []
    for y in range(y0, y1):
        for x in range(x0, x1):
            cells.append((x + y, x, y))
    cells.sort()

    for _, x, y in cells:
        t = world.tiles[y][x]
        sx, sy = iso.world_to_screen(x + 0.5, y + 0.5, cam_x, cam_y)
        if sx < -TILE_W or sx > surf.get_width() + TILE_W:
            continue
        if sy < -80 or sy > surf.get_height() + 40:
            continue
        top, side = _TILE_COLORS.get(t, (GRASS, GRASS_DK))
        if t == T_WALL:
            iso.draw_block(surf, sx, sy, CASTLE, CASTLE_DK, (110, 120, 160), h=22)
            rcol = world.roof_colors.get((x, y), ROOF)
            iso.draw_diamond(surf, sx, sy - 22, rcol)
            # bright ridge highlight
            pygame.draw.line(surf, CREAM, (sx - 10, sy - 22), (sx, sy - 30), 1)
        elif t == T_WATER:
            iso.draw_diamond(surf, sx, sy, top, side)
            pygame.draw.line(surf, CYAN, (sx - 12, sy - 2), (sx + 10, sy - 6), 2)
            pygame.draw.circle(surf, (200, 240, 255), (int(sx + 4), int(sy + 2)), 2)
        elif t == T_FOREST:
            iso.draw_diamond(surf, sx, sy, top, side)
            pygame.draw.rect(surf, WOOD, (sx - 3, sy - 18, 6, 14))
            # layered bright canopy
            pygame.draw.circle(surf, (40, 170, 55), (int(sx), int(sy - 22)), 13)
            pygame.draw.circle(surf, (90, 220, 80), (int(sx - 4), int(sy - 26)), 9)
            pygame.draw.circle(surf, (160, 255, 100), (int(sx + 3), int(sy - 28)), 5)
        elif t == T_DOOR:
            iso.draw_block(surf, sx, sy, WOOD, (140, 80, 30), (200, 130, 50), h=18)
            pygame.draw.circle(surf, GOLD, (int(sx + 6), int(sy - 6)), 3)
        elif t == T_STAIRS:
            iso.draw_diamond(surf, sx, sy, top, side)
            for i in range(3):
                pygame.draw.line(surf, GOLD, (sx - 16 + i * 4, sy - 4 + i * 3),
                                 (sx + 16 - i * 4, sy - 4 + i * 3), 2)
        elif t == T_FLOWER:
            c = top if (x + y) % 2 == 0 else tuple(min(255, v + 12) for v in top)
            iso.draw_diamond(surf, sx, sy, c, side)
            fc = _FLOWER_COLORS[(x * 3 + y * 7) % len(_FLOWER_COLORS)]
            pygame.draw.circle(surf, fc, (int(sx), int(sy - 4)), 3)
            pygame.draw.circle(surf, GOLD, (int(sx), int(sy - 4)), 1)
        elif t == T_ASH:
            # lilac / magenta clearing for boss
            iso.draw_diamond(surf, sx, sy, ASH, (150, 80, 140))
            if (x + y) % 3 == 0:
                pygame.draw.circle(surf, MAGENTA, (int(sx), int(sy)), 2)
        else:
            c = top if (x + y) % 2 == 0 else tuple(min(255, v + 14) for v in top)
            iso.draw_diamond(surf, sx, sy, c, side)

    for lx, ly, text in world.labels:
        sx, sy = iso.world_to_screen(lx, ly, cam_x, cam_y)
        if -100 < sx < surf.get_width() + 100 and -40 < sy < surf.get_height():
            label = font_sm.render(text, True, GOLD)
            shadow = font_sm.render(text, True, (40, 50, 100))
            surf.blit(shadow, (sx - label.get_width() // 2 + 1, sy - 40))
            surf.blit(label, (sx - label.get_width() // 2, sy - 41))
