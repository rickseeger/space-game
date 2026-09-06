"""Isometric helpers: world <-> screen projection and diamond drawing."""
from __future__ import annotations

import pygame
from .constants import TILE_W, TILE_H, WIDTH, HEIGHT


def world_to_screen(wx: float, wy: float, cam_x: float, cam_y: float) -> tuple[float, float]:
    """Map tile-space (float) to screen pixels given camera center in tile-space."""
    # Camera is in world tile coords; convert relative offset to iso
    dx = wx - cam_x
    dy = wy - cam_y
    sx = (dx - dy) * (TILE_W / 2) + WIDTH / 2
    sy = (dx + dy) * (TILE_H / 2) + HEIGHT / 2
    return sx, sy


def screen_to_world(sx: float, sy: float, cam_x: float, cam_y: float) -> tuple[float, float]:
    rx = sx - WIDTH / 2
    ry = sy - HEIGHT / 2
    dx = (rx / (TILE_W / 2) + ry / (TILE_H / 2)) / 2
    dy = (ry / (TILE_H / 2) - rx / (TILE_W / 2)) / 2
    return cam_x + dx, cam_y + dy


def draw_diamond(surf: pygame.Surface, cx: float, cy: float, color, outline=None) -> None:
    pts = [
        (cx, cy - TILE_H // 2),
        (cx + TILE_W // 2, cy),
        (cx, cy + TILE_H // 2),
        (cx - TILE_W // 2, cy),
    ]
    pygame.draw.polygon(surf, color, pts)
    if outline:
        pygame.draw.polygon(surf, outline, pts, 1)


def draw_block(surf: pygame.Surface, cx: float, cy: float, top, left, right, h: int = 16) -> None:
    """Draw a simple isometric block (building / wall)."""
    top_pts = [
        (cx, cy - TILE_H // 2 - h),
        (cx + TILE_W // 2, cy - h),
        (cx, cy + TILE_H // 2 - h),
        (cx - TILE_W // 2, cy - h),
    ]
    left_pts = [
        (cx - TILE_W // 2, cy - h),
        (cx, cy + TILE_H // 2 - h),
        (cx, cy + TILE_H // 2),
        (cx - TILE_W // 2, cy),
    ]
    right_pts = [
        (cx + TILE_W // 2, cy - h),
        (cx, cy + TILE_H // 2 - h),
        (cx, cy + TILE_H // 2),
        (cx + TILE_W // 2, cy),
    ]
    pygame.draw.polygon(surf, left, left_pts)
    pygame.draw.polygon(surf, right, right_pts)
    pygame.draw.polygon(surf, top, top_pts)
    pygame.draw.polygon(surf, (0, 0, 0), top_pts, 1)
