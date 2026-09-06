"""Entry point for Cinder."""
from __future__ import annotations

import argparse
import os
import sys


def main(argv: list[str] | None = None) -> None:
    parser = argparse.ArgumentParser(description="Cinder — isometric wizard RPG")
    parser.add_argument("--screenshot", action="store_true",
                        help="Capture screenshots under screenshots/ then exit")
    parser.add_argument("--smoke", type=float, default=0.0,
                        help="Run N seconds then exit (for CI/smoke)")
    args = parser.parse_args(argv)

    # Ensure we can run headless under xvfb
    if not os.environ.get("DISPLAY"):
        print("No DISPLAY set — use xvfb-run or a desktop session.", file=sys.stderr)

    from .game import Game

    if args.screenshot:
        capture_screenshots()
        return

    g = Game()
    max_s = args.smoke if args.smoke > 0 else None
    g.run(max_seconds=max_s)


def capture_screenshots() -> None:
    """Drive a short scripted session and save PNGs."""
    import pygame
    from .game import Game
    from .constants import WIDTH, HEIGHT

    os.makedirs("screenshots", exist_ok=True)
    g = Game(screenshot_mode=True)

    def snap(name: str) -> None:
        path = os.path.join("screenshots", name)
        pygame.image.save(g.screen, path)
        print(f"Wrote {path}")

    # Title
    g.state = "title"
    g.time = 1.5
    for _ in range(3):
        g.clock.tick(60)
        from . import ui
        ui.draw_title(g.screen, g.fonts, g.time, 0)
        pygame.display.flip()
    snap("01_title.png")

    # Start play near castle
    g.start_new()
    g.cam_x, g.cam_y = g.player.x, g.player.y
    g.draw_play()
    pygame.display.flip()
    snap("02_castle.png")

    # Move to village
    g.player.x, g.player.y = 28.0, 22.0
    g.cam_x, g.cam_y = g.player.x, g.player.y
    g.draw_play()
    pygame.display.flip()
    snap("03_village.png")

    # Wilds + enemy
    g.player.x, g.player.y = 45.0, 40.0
    g.cam_x, g.cam_y = g.player.x, g.player.y
    g.player.level = 3
    g.player.spell_unlocked = True
    g.draw_play()
    pygame.display.flip()
    snap("04_wilds.png")

    # Duel bubble
    boss = next(e for e in g.enemies if e.boss)
    g.player.x, g.player.y = boss.x - 1.2, boss.y
    g._start_duel(boss)
    g.cam_x, g.cam_y = g.duel_cx, g.duel_cy
    g.draw_play()
    pygame.display.flip()
    snap("05_duel.png")

    # Dialogue
    g.state = "dialogue"
    g.dialogue_npc = g.npcs[0]
    g.dialogue_text = "Retrieve the Ember Relic from the Ash Drake. Return, and be named free mage."
    g.draw_play()
    from . import ui
    ui.draw_dialogue(g.screen, g.fonts, "King Aldric", g.dialogue_text)
    pygame.display.flip()
    snap("06_dialogue.png")

    # Shop
    g.state = "shop"
    g.active_shop = next(n for n in g.npcs if n.shop)
    g.shop_sel = 0
    g.player.inv.gold = 200
    g.draw_play()
    ui.draw_shop(g.screen, g.fonts, f"{g.active_shop.name}'s Wares",
                 g.active_shop.shop, g.player.inv, g.shop_sel)
    pygame.display.flip()
    snap("07_shop.png")

    pygame.quit()
    print("Screenshots complete.")


if __name__ == "__main__":
    main()
