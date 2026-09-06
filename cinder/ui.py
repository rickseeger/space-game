"""HUD, dialogue, shop, inventory, title, pause overlays — vibrant RPG UI."""
from __future__ import annotations

import math
from typing import Optional

import pygame

from .constants import (
    WIDTH, HEIGHT, UI_PANEL, UI_BORDER, CREAM, GOLD, HP_RED, HP_BG, XP_BLUE,
    INK, EMBER, EMBER_HOT, ASH, CONTROLS_HELP, TITLE, VERSION, SKY, CYAN,
    MAGENTA, DUEL_GLOW, BG,
)


def make_fonts():
    pygame.font.init()
    return {
        "title": pygame.font.SysFont("georgia", 64, bold=True),
        "lg": pygame.font.SysFont("georgia", 32, bold=True),
        "md": pygame.font.SysFont("dejavusans", 20),
        "sm": pygame.font.SysFont("dejavusans", 16),
        "tiny": pygame.font.SysFont("dejavusans", 13),
    }


def panel(surf, rect, alpha=230):
    s = pygame.Surface((rect.w, rect.h), pygame.SRCALPHA)
    # cheerful royal blue → purple gradient feel
    s.fill((*UI_PANEL, alpha))
    surf.blit(s, rect.topleft)
    pygame.draw.rect(surf, UI_BORDER, rect, 3, border_radius=8)
    # inner cyan highlight
    pygame.draw.rect(surf, (*CYAN, 80), rect.inflate(-6, -6), 1, border_radius=6)


def draw_bar(surf, x, y, w, h, frac, fill, bg=HP_BG):
    pygame.draw.rect(surf, bg, (x, y, w, h), border_radius=3)
    fw = max(0, int(w * max(0.0, min(1.0, frac))))
    if fw:
        pygame.draw.rect(surf, fill, (x, y, fw, h), border_radius=3)
    pygame.draw.rect(surf, UI_BORDER, (x, y, w, h), 1, border_radius=3)


def draw_hud(surf, fonts, player, quest_hint: str, duel: bool = False) -> None:
    panel(surf, pygame.Rect(12, 12, 290, 96))
    name = fonts["sm"].render(f"Apprentice  Lv {player.level}", True, GOLD)
    surf.blit(name, (24, 20))
    draw_bar(surf, 24, 44, 200, 12, player.hp / max(1, player.max_hp), HP_RED)
    hp_t = fonts["tiny"].render(f"HP {player.hp}/{player.max_hp}", True, CREAM)
    surf.blit(hp_t, (230, 42))
    draw_bar(surf, 24, 64, 200, 10, player.xp / max(1, player.xp_next), XP_BLUE)
    xp_t = fonts["tiny"].render(f"XP {player.xp}/{player.xp_next}", True, CREAM)
    surf.blit(xp_t, (230, 62))
    gold_t = fonts["sm"].render(f"{player.inv.gold} g", True, GOLD)
    surf.blit(gold_t, (24, 80))

    mode = "EMBERBOLT" if player.spell_mode and player.spell_unlocked else "STAFF"
    mode_c = EMBER_HOT if player.spell_mode and player.spell_unlocked else CYAN
    mt = fonts["tiny"].render(f"[{mode}]", True, mode_c)
    surf.blit(mt, (120, 82))

    # Quest hint — bright banner
    panel(surf, pygame.Rect(WIDTH // 2 - 250, 12, 500, 40))
    hint = fonts["sm"].render(quest_hint, True, GOLD)
    surf.blit(hint, (WIDTH // 2 - hint.get_width() // 2, 22))

    if duel:
        pulse = abs(math.sin(pygame.time.get_ticks() / 180))
        col = (
            int(255),
            int(80 + 120 * pulse),
            int(255 * pulse),
        )
        t = fonts["lg"].render("DUEL", True, col)
        outline = fonts["lg"].render("DUEL", True, CYAN)
        surf.blit(outline, (WIDTH // 2 - t.get_width() // 2 + 2, 58))
        surf.blit(t, (WIDTH // 2 - t.get_width() // 2, 56))


def draw_dialogue(surf, fonts, speaker: str, text: str, options: Optional[list[str]] = None) -> None:
    rect = pygame.Rect(80, HEIGHT - 180, WIDTH - 160, 150)
    panel(surf, rect, 240)
    sp = fonts["md"].render(speaker, True, GOLD)
    surf.blit(sp, (100, HEIGHT - 168))
    words = text.split()
    lines, cur = [], ""
    for w in words:
        test = (cur + " " + w).strip()
        if fonts["sm"].size(test)[0] > WIDTH - 220:
            lines.append(cur)
            cur = w
        else:
            cur = test
    if cur:
        lines.append(cur)
    for i, line in enumerate(lines[:5]):
        surf.blit(fonts["sm"].render(line, True, CREAM), (100, HEIGHT - 130 + i * 22))
    tip = fonts["tiny"].render("E / Enter — continue", True, CYAN)
    surf.blit(tip, (WIDTH - 280, HEIGHT - 48))
    if options:
        for i, opt in enumerate(options):
            surf.blit(fonts["sm"].render(f"{i+1}. {opt}", True, EMBER_HOT),
                      (100, HEIGHT - 50 - (len(options) - i) * 20))


def draw_shop(surf, fonts, shop_name: str, stock: list[str], inv, sel: int) -> None:
    from .items import CATALOG
    rect = pygame.Rect(WIDTH // 2 - 260, 80, 520, 480)
    panel(surf, rect, 245)
    title = fonts["lg"].render(shop_name, True, GOLD)
    surf.blit(title, (WIDTH // 2 - title.get_width() // 2, 96))
    gold = fonts["md"].render(f"Your gold: {inv.gold}", True, EMBER_HOT)
    surf.blit(gold, (WIDTH // 2 - 220, 140))
    for i, iid in enumerate(stock):
        item = CATALOG[iid]
        y = 180 + i * 44
        col = MAGENTA if i == sel else CREAM
        can = inv.gold >= item.price
        label = f"{item.name}  —  {item.price}g"
        if not can:
            col = (160, 160, 190)
        surf.blit(fonts["md"].render(label, True, col), (WIDTH // 2 - 220, y))
        surf.blit(fonts["tiny"].render(item.desc, True, CYAN), (WIDTH // 2 - 220, y + 22))
    tip = fonts["tiny"].render("Up/Down select · Enter buy · Esc close", True, GOLD)
    surf.blit(tip, (WIDTH // 2 - tip.get_width() // 2, 520))


def draw_inventory(surf, fonts, player) -> None:
    from .items import CATALOG
    rect = pygame.Rect(WIDTH // 2 - 260, 70, 520, 520)
    panel(surf, rect, 245)
    title = fonts["lg"].render("Inventory", True, GOLD)
    surf.blit(title, (WIDTH // 2 - title.get_width() // 2, 86))
    inv = player.inv
    lines = [
        f"Weapon: {inv.weapon_item().name}  (DMG {inv.weapon_item().dmg})",
        f"Armor:  {inv.armor_item().name}  (ARM {inv.armor_item().armor})",
        f"Boots:  {inv.boots_item().name}  (+{int(inv.boots_item().speed)} spd)",
        f"Gold: {inv.gold}",
        "",
        "Bag:",
    ]
    y = 140
    for line in lines:
        surf.blit(fonts["sm"].render(line, True, CREAM), (WIDTH // 2 - 220, y))
        y += 26
    if not inv.bag:
        surf.blit(fonts["sm"].render("(empty)", True, (160, 160, 190)), (WIDTH // 2 - 200, y))
    else:
        for iid, n in inv.bag.items():
            item = CATALOG[iid]
            surf.blit(fonts["sm"].render(f"  {item.name} x{n}", True, EMBER_HOT), (WIDTH // 2 - 220, y))
            y += 24
    tip = fonts["tiny"].render("H — drink Health Vial · Esc/I close", True, CYAN)
    surf.blit(tip, (WIDTH // 2 - tip.get_width() // 2, 550))


def draw_quest_log(surf, fonts, quests) -> None:
    rect = pygame.Rect(WIDTH // 2 - 280, 70, 560, 500)
    panel(surf, rect, 245)
    title = fonts["lg"].render("Quest Log", True, GOLD)
    surf.blit(title, (WIDTH // 2 - title.get_width() // 2, 86))
    y = 140
    for q in quests.quests.values():
        if not q.active and not q.complete:
            status, col = "—", (150, 150, 180)
        elif q.complete:
            status, col = "DONE", (80, 230, 120)
        else:
            status, col = "ACTIVE", MAGENTA
        surf.blit(fonts["md"].render(f"[{status}] {q.title}", True, col), (WIDTH // 2 - 240, y))
        y += 28
        words = q.desc.split()
        cur = ""
        for w in words:
            test = (cur + " " + w).strip()
            if fonts["tiny"].size(test)[0] > 480:
                surf.blit(fonts["tiny"].render(cur, True, CREAM), (WIDTH // 2 - 230, y))
                y += 18
                cur = w
            else:
                cur = test
        if cur:
            surf.blit(fonts["tiny"].render(cur, True, CREAM), (WIDTH // 2 - 230, y))
            y += 18
        if q.active and not q.complete:
            surf.blit(fonts["tiny"].render(f"Hint: {q.hint}", True, GOLD), (WIDTH // 2 - 230, y))
            y += 18
        y += 16
    tip = fonts["tiny"].render("Esc / Q — close", True, CYAN)
    surf.blit(tip, (WIDTH // 2 - tip.get_width() // 2, 540))


def draw_title(surf, fonts, t: float, menu_sel: int = 0) -> None:
    # cheerful sky gradient
    for y in range(HEIGHT):
        blend = y / HEIGHT
        r = int(90 + blend * 40)
        g = int(170 - blend * 30)
        b = int(240 - blend * 40)
        pygame.draw.line(surf, (r, g, b), (0, y), (WIDTH, y))
    # rolling green hills silhouette at bottom
    for i in range(0, WIDTH, 8):
        h = 80 + int(30 * math.sin(i * 0.01 + t * 0.4))
        pygame.draw.rect(surf, (60, 180, 70), (i, HEIGHT - h, 10, h))
        pygame.draw.rect(surf, (100, 220, 90), (i, HEIGHT - h, 10, 8))

    # floating colorful sparks (not just orange)
    spark_cols = [EMBER, EMBER_HOT, CYAN, MAGENTA, GOLD, (120, 255, 160)]
    for i in range(50):
        x = (math.sin(t * 0.35 + i * 1.7) * 0.5 + 0.5) * WIDTH
        y = HEIGHT - 100 - ((t * 40 + i * 53) % (HEIGHT - 80))
        r = 2 + (i % 4)
        pygame.draw.circle(surf, spark_cols[i % len(spark_cols)], (int(x), int(y)), r)

    title = fonts["title"].render(TITLE, True, EMBER_HOT)
    outline = fonts["title"].render(TITLE, True, MAGENTA)
    surf.blit(outline, (WIDTH // 2 - title.get_width() // 2 + 3, 133))
    surf.blit(title, (WIDTH // 2 - title.get_width() // 2, 130))
    sub = fonts["md"].render("A weak wizard. A stolen relic. An open world of ash and gold.", True, INK)
    surf.blit(sub, (WIDTH // 2 - sub.get_width() // 2, 210))

    # menu card
    panel(surf, pygame.Rect(WIDTH // 2 - 180, 280, 360, 200), 220)
    items = ["New Journey", "Controls", "Quit"]
    for i, label in enumerate(items):
        col = GOLD if i == menu_sel else CREAM
        prefix = "▸ " if i == menu_sel else "  "
        txt = fonts["lg"].render(prefix + label, True, col)
        surf.blit(txt, (WIDTH // 2 - 120, 300 + i * 50))

    ver = fonts["tiny"].render(f"v{VERSION}  ·  pygame", True, INK)
    surf.blit(ver, (WIDTH // 2 - ver.get_width() // 2, HEIGHT - 36))


def draw_controls_screen(surf, fonts) -> None:
    # sky backdrop
    surf.fill(SKY)
    panel(surf, pygame.Rect(WIDTH // 2 - 260, 100, 520, 420), 240)
    title = fonts["lg"].render("Controls", True, GOLD)
    surf.blit(title, (WIDTH // 2 - title.get_width() // 2, 120))
    y = 180
    for line in CONTROLS_HELP:
        surf.blit(fonts["md"].render(line, True, CREAM), (WIDTH // 2 - 200, y))
        y += 36
    tip = fonts["tiny"].render("Esc — back", True, CYAN)
    surf.blit(tip, (WIDTH // 2 - tip.get_width() // 2, 480))


def draw_pause(surf, fonts, sel: int = 0) -> None:
    overlay = pygame.Surface((WIDTH, HEIGHT), pygame.SRCALPHA)
    overlay.fill((30, 50, 120, 140))
    surf.blit(overlay, (0, 0))
    panel(surf, pygame.Rect(WIDTH // 2 - 160, 200, 320, 260), 245)
    title = fonts["lg"].render("Paused", True, GOLD)
    surf.blit(title, (WIDTH // 2 - title.get_width() // 2, 220))
    items = ["Resume", "Controls", "Title Screen"]
    for i, label in enumerate(items):
        col = GOLD if i == sel else CREAM
        prefix = "▸ " if i == sel else "  "
        txt = fonts["md"].render(prefix + label, True, col)
        surf.blit(txt, (WIDTH // 2 - 80, 280 + i * 40))


def draw_death(surf, fonts) -> None:
    overlay = pygame.Surface((WIDTH, HEIGHT), pygame.SRCALPHA)
    overlay.fill((80, 20, 60, 170))
    surf.blit(overlay, (0, 0))
    t = fonts["lg"].render("You fall…", True, CREAM)
    surf.blit(t, (WIDTH // 2 - t.get_width() // 2, HEIGHT // 2 - 40))
    s = fonts["md"].render("You wake at the inn — some gold lost. Enter to continue.", True, GOLD)
    surf.blit(s, (WIDTH // 2 - s.get_width() // 2, HEIGHT // 2 + 20))


def draw_victory_banner(surf, fonts, text: str, t: float) -> None:
    pulse = int(abs(math.sin(t * 3)) * 40)
    banner = fonts["lg"].render(text, True, (255, 220 - pulse // 2, 60))
    glow = fonts["lg"].render(text, True, MAGENTA)
    surf.blit(glow, (WIDTH // 2 - banner.get_width() // 2 + 2, 102))
    surf.blit(banner, (WIDTH // 2 - banner.get_width() // 2, 100))
