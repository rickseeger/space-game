"""Main game state: overworld, combat, duel bubble, shops, quests."""
from __future__ import annotations

import math
import random
import sys
from typing import Optional

import pygame

from .constants import (
    WIDTH, HEIGHT, FPS, BG, GOLD_LOSS_FRAC, PLAYER_MELEE_RANGE, EMBER, EMBER_HOT,
    CREAM, GOLD, DUEL_GLOW, CASTLE_CX, CASTLE_CY, VILLAGE_CX, VILLAGE_CY, BOSS_CX, BOSS_CY,
    TITLE,
)
from .world import generate_world, draw_world
from .entities import (
    Player, NPC, Enemy, Projectile, Particle, make_enemy,
    draw_player, draw_npc, draw_enemy, draw_projectile, spawn_hit_particles, _norm,
)
from .items import CATALOG
from .quests import QuestLog
from .audio import SFX
from . import ui
from . import iso


class Game:
    def __init__(self, screenshot_mode: bool = False) -> None:
        pygame.init()
        flags = 0
        self.screen = pygame.display.set_mode((WIDTH, HEIGHT), flags)
        pygame.display.set_caption(f"{TITLE} — Apprentice's Journey")
        self.clock = pygame.time.Clock()
        self.fonts = ui.make_fonts()
        self.sfx = SFX()
        self.rng = random.Random(42)
        self.screenshot_mode = screenshot_mode
        self.time = 0.0
        self.state = "title"  # title, controls, play, dialogue, shop, inventory, quests, pause, death, win
        self.menu_sel = 0
        self.pause_sel = 0
        self.shop_sel = 0
        self.active_shop: Optional[NPC] = None
        self.dialogue_npc: Optional[NPC] = None
        self.dialogue_text = ""
        self.dialogue_queue: list[str] = []
        self.toast = ""
        self.toast_t = 0.0
        self.floating: list[tuple[float, float, str, float, tuple]] = []
        self.world = generate_world(self.rng)
        self.player = Player(*self.world.spawn)
        self.quests = QuestLog()
        self.npcs: list[NPC] = []
        self.enemies: list[Enemy] = []
        self.projectiles: list[Projectile] = []
        self.particles: list[Particle] = []
        self.cam_x = self.player.x
        self.cam_y = self.player.y
        self.duel = False
        self.duel_enemy: Optional[Enemy] = None
        self.duel_cx = 0.0
        self.duel_cy = 0.0
        self.duel_r = 4.2
        self.checkpoint = self.world.inn_spawn
        self.story_won = False
        self._build_npcs()
        self._spawn_enemies()
        self._auto_shots = 0
        self._respawn_queue: list[tuple[float, Enemy]] = []

    def _build_npcs(self) -> None:
        cx, cy = CASTLE_CX, CASTLE_CY
        vx, vy = VILLAGE_CX, VILLAGE_CY
        self.npcs = [
            NPC(cx + 0.5, cy - 1.5, "King Aldric", "king", (255, 200, 40), [
                "Apprentice. Ashfall Keep remembers softer days.",
                "The Ember Relic was torn from our vault by the Ash Drake.",
                "Retrieve it from the Cinder Nest southeast of the village.",
                "Return with the relic, and I will name you free mage of these lands.",
            ], quest_id="prove"),
            NPC(cx - 1.5, cy + 1.0, "Captain Rhea", "guard", (70, 140, 255), [
                "Keep your staff ready beyond the gate.",
                "Wolves first. Bandits farther. The Drake last — if you're ready.",
                "The village blacksmith can arm you… for gold.",
            ]),
            NPC(vx - 2.5, vy - 0.8, "Maro", "innkeeper", (255, 130, 60), [
                "Welcome to the Cinder Cup. Rest is free once you've earned my favor.",
                "Bring me 4 Wolf Pelts and I'll keep a bed warm — and pay you.",
                "Dying in the wilds? You'll wake here. Try not to make a habit of it.",
            ], quest_id="pelts"),
            NPC(vx + 2.8, vy - 0.8, "Lira", "shopkeeper", (40, 200, 190), [
                "Potions, staves, cloaks — if you have coin, I have wares.",
                "Emberbolt comes with experience. Gear comes with gold.",
                "Rumor: the Drake sleeps in ash. Strike when it rears.",
            ], shop=["health_vial", "greater_vial", "ashwood_staff", "leather_vest",
                     "traveler_boots", "ember_rod", "ember_cloak", "boots_of_speed"]),
            NPC(vx - 2.5, vy + 2.2, "Brenna", "blacksmith", (255, 90, 50), [
                "Forge is hungry. Bandits carry Iron Ore in the eastern wilds.",
                "Bring me 3 ores and I'll hammer you something worth swinging.",
                "A weak wizard who pays? That's my favorite kind of customer.",
            ], quest_id="ore"),
            NPC(vx + 2.8, vy + 2.2, "Old Fenn", "villager", (120, 220, 90), [
                "I saw fire on the southeastern ridge last night.",
                "Distance makes monsters crueler. Come home often.",
                "You look like a stiff breeze could knock you over. Good luck.",
            ]),
            NPC(cx + 2.0, cy + 2.5, "Squire Tom", "squire", (255, 140, 200), [
                "His Majesty waits on the stairs. Don't keep him.",
                "I wanted to be a knight. Then I saw a wolf. Now I sweep floors.",
            ]),
        ]

    def _spawn_enemies(self) -> None:
        self.enemies.clear()
        # Soft ring of threats by distance
        placements = []
        for _ in range(28):
            while True:
                x = self.rng.uniform(8, 75)
                y = self.rng.uniform(8, 75)
                if self.world.is_town(x, y) or self.world.solid(x, y):
                    continue
                if math.hypot(x - BOSS_CX, y - BOSS_CY) < 5:
                    continue
                diff = self.world.difficulty(x, y)
                if diff < 0.15:
                    continue
                placements.append((x, y, diff))
                break
        for x, y, diff in placements:
            if diff < 0.35:
                kind = "slime"
            elif diff < 0.65:
                kind = self.rng.choice(["slime", "wolf", "wolf"])
            else:
                kind = self.rng.choice(["wolf", "bandit", "bandit"])
            self.enemies.append(make_enemy(kind, x, y, diff))
        # Boss
        self.enemies.append(make_enemy("ash_drake", BOSS_CX + 0.5, BOSS_CY + 0.5, 1.0))

    def toast_msg(self, msg: str) -> None:
        self.toast = msg
        self.toast_t = 3.0

    def float_text(self, x, y, text, color=CREAM) -> None:
        self.floating.append((x, y, text, 1.0, color))

    def start_new(self) -> None:
        self.world = generate_world(self.rng)
        self.player = Player(*self.world.spawn)
        self.quests = QuestLog()
        self.projectiles.clear()
        self.particles.clear()
        self.duel = False
        self.duel_enemy = None
        self.story_won = False
        self.checkpoint = self.world.inn_spawn
        self._build_npcs()
        self._spawn_enemies()
        self.state = "play"
        self.toast_msg("Ashfall Keep — prove yourself to the King.")
        self.sfx.play(self.sfx.talk)

    def nearest_npc(self, radius: float = 1.2) -> Optional[NPC]:
        best, bd = None, radius
        for n in self.npcs:
            d = math.hypot(n.x - self.player.x, n.y - self.player.y)
            if d < bd:
                best, bd = n, d
        return best

    def begin_dialogue(self, npc: NPC) -> None:
        self.dialogue_npc = npc
        self.sfx.play(self.sfx.talk)
        self._open_shop_after = False
        self._pending_king_complete = False
        q = self.quests
        # Context-aware dialogue
        if npc.role == "king":
            prove = q.get("prove")
            if prove.complete:
                self.dialogue_queue = [
                    "Rise, mage of Ashfall. The Keep is in your debt.",
                    "Wander where you will. The wilds still hunger — grow stronger.",
                ]
            elif self.player.inv.count("ember_relic") > 0:
                self.dialogue_queue = [
                    "The Ember Relic… warm as the day it was forged.",
                    "You have proven your worth. I name you free mage of Ashfall!",
                ]
                # complete on dismiss last line — handled in advance_dialogue
                self._pending_king_complete = True
            elif prove.stage == 0:
                self.dialogue_queue = npc.lines[:]
                prove.stage = 1
                self.toast_msg("Quest updated: Hunt the Ash Drake")
            else:
                self.dialogue_queue = [
                    "The Drake still holds our heartstone. Southeast — past Ember Village.",
                    "Return alive. Preferably with the relic.",
                ]
        elif npc.role == "innkeeper":
            pelts = q.get("pelts")
            if not pelts.active and not pelts.complete:
                pelts.active = True
                self.dialogue_queue = [
                    "Wolves thin the flocks. Bring me 4 Wolf Pelts.",
                    "Do that and the Cinder Cup is your checkpoint — free rest, fair pay.",
                ]
                self.toast_msg("Side quest: Pelts for the Inn")
            elif pelts.active and self.player.inv.count("wolf_pelt") >= 4:
                self.player.inv.remove("wolf_pelt", 4)
                self.player.inv.gold += 40
                self.player.heal(999)
                pelts.complete = True
                pelts.active = False
                self.checkpoint = (npc.x, npc.y + 0.8)
                self.dialogue_queue = [
                    "Fine pelts! Here's 40 gold — and a bed whenever you fall.",
                    "Your checkpoint is set at my inn.",
                ]
                self.sfx.play(self.sfx.coin)
                self.toast_msg("Quest complete! +40g · Inn checkpoint set")
            else:
                have = self.player.inv.count("wolf_pelt")
                self.dialogue_queue = [
                    f"Pelts so far: {have}/4. Wolves roam the forest paths.",
                    "Rest anytime — you'll wake here if the wilds take you.",
                ]
                self.checkpoint = (npc.x, npc.y + 0.8)
                self.player.heal(8)
        elif npc.role == "blacksmith":
            ore = q.get("ore")
            if not ore.active and not ore.complete:
                ore.active = True
                self.dialogue_queue = [
                    "Bandits east of here haul Iron Ore. I need 3 chunks.",
                    "Fetch them and I'll put an Ashwood Staff in your hands — free.",
                ]
                self.toast_msg("Side quest: Ore for the Forge")
            elif ore.active and self.player.inv.count("iron_ore") >= 3:
                self.player.inv.remove("iron_ore", 3)
                # reward staff into bag then equip
                self.player.inv.add("ashwood_staff", 1)
                self.player.inv.equip("ashwood_staff")
                ore.complete = True
                ore.active = False
                self.dialogue_queue = [
                    "Solid ore. This Ashwood Staff is yours — swing harder out there.",
                ]
                self.sfx.play(self.sfx.shop)
                self.toast_msg("Quest complete! Equipped Ashwood Staff")
            else:
                have = self.player.inv.count("iron_ore")
                self.dialogue_queue = [f"Ore: {have}/3. Hit bandits. Bring metal."]
        elif npc.shop:
            self.dialogue_queue = [npc.next_line(), "Shall I open the shop? (Enter shop after)"]
            self._open_shop_after = True
        else:
            self.dialogue_queue = [npc.next_line()]

        self.advance_dialogue()

    def advance_dialogue(self) -> None:
        if self.dialogue_queue:
            self.dialogue_text = self.dialogue_queue.pop(0)
            self.state = "dialogue"
            self.sfx.play(self.sfx.talk)
            return
        # end
        npc = self.dialogue_npc
        pending_shop = getattr(self, "_open_shop_after", False)
        pending_king = getattr(self, "_pending_king_complete", False)
        self._open_shop_after = False
        if pending_king and npc and npc.role == "king":
            if self.player.inv.count("ember_relic") > 0:
                self.player.inv.remove("ember_relic", 1)
                self.player.inv.gold += 100
                self.player.xp += 80
                for _ in self.player.xp_to_level():
                    self.sfx.level_up()
                    self.toast_msg(f"Level up! Now level {self.player.level}")
                self.quests.complete("prove")
                self.story_won = True
                self.sfx.play(self.sfx.win)
                self.toast_msg("Main quest complete! You are free mage of Ashfall.")
            self._pending_king_complete = False
        self.dialogue_npc = None
        self.dialogue_text = ""
        if pending_shop and npc and npc.shop:
            self.active_shop = npc
            self.shop_sel = 0
            self.state = "shop"
            self.sfx.play(self.sfx.shop)
        else:
            self.state = "play"

    def buy_selected(self) -> None:
        if not self.active_shop:
            return
        stock = self.active_shop.shop
        if not stock:
            return
        iid = stock[self.shop_sel]
        item = CATALOG[iid]
        if self.player.inv.gold < item.price:
            self.toast_msg("Not enough gold.")
            self.sfx.play(self.sfx.hurt)
            return
        self.player.inv.gold -= item.price
        if item.kind in ("weapon", "armor", "boots"):
            self.player.inv.add(iid, 1)
            self.player.inv.equip(iid)
            self.toast_msg(f"Equipped {item.name}")
        else:
            self.player.inv.add(iid, 1)
            self.toast_msg(f"Bought {item.name}")
        self.sfx.play(self.sfx.coin)

    def try_interact(self) -> None:
        npc = self.nearest_npc()
        if npc:
            self.begin_dialogue(npc)

    def try_attack(self) -> None:
        kind = self.player.try_attack()
        if not kind:
            return
        if kind == "melee":
            self.sfx.play(self.sfx.swing)
            self._melee_hit()
        else:
            self.sfx.play(self.sfx.spell)
            fx, fy = self.player.facing
            if fx == 0 and fy == 0:
                fx, fy = 0, 1
            fx, fy = _norm(fx, fy)
            dmg = 10 + self.player.level * 2 + self.player.inv.weapon_item().dmg // 3
            self.projectiles.append(Projectile(
                self.player.x, self.player.y,
                fx * 7.5, fy * 7.5, dmg=dmg, life=0.85, color=(255, 140, 40)
            ))

    def _melee_hit(self) -> None:
        px, py = self.player.x, self.player.y
        fx, fy = self.player.facing
        reach = PLAYER_MELEE_RANGE / 64.0
        for e in self.enemies:
            if not e.alive:
                continue
            if self.duel and e is not self.duel_enemy:
                continue
            dx, dy = e.x - px, e.y - py
            dist = math.hypot(dx, dy)
            if dist > reach + e.radius:
                continue
            # must be roughly in facing cone
            if fx or fy:
                dot = (dx * fx + dy * fy) / max(0.01, dist)
                if dot < 0.15 and dist > 0.4:
                    continue
            dmg = self.player.melee_dmg
            dead = e.take_damage(dmg)
            self.float_text(e.x, e.y, str(dmg), EMBER_HOT)
            spawn_hit_particles(self.particles, e.x, e.y, (255, 180, 40), 10)
            self.sfx.play(self.sfx.hit)
            if dead:
                self._on_enemy_killed(e)
            elif e.boss and not self.duel:
                self._start_duel(e)
            break

    def _on_enemy_killed(self, e: Enemy) -> None:
        self.sfx.play(self.sfx.kill)
        self.player.xp += e.xp
        self.player.inv.gold += e.gold
        self.float_text(e.x, e.y - 0.3, f"+{e.gold}g", (232, 196, 80))
        for lvl in self.player.xp_to_level():
            self.sfx.level_up()
            self.toast_msg(f"Level {lvl}! Max HP up." + (" Emberbolt unlocked (press 2)!" if lvl == 3 else ""))
            spawn_hit_particles(self.particles, self.player.x, self.player.y, (100, 180, 255), 16)
        if e.drop:
            self.player.inv.add(e.drop, 1)
            self.toast_msg(f"Got {CATALOG[e.drop].name}")
            if e.drop == "ember_relic":
                self.quests.set_stage("prove", 2)
                self.toast_msg("Ember Relic obtained! Return to King Aldric.")
        if self.duel and e is self.duel_enemy:
            self._end_duel(won=True)
        # respawn trash later away from towns
        if not e.boss:
            e.alive = False
            # schedule respawn elsewhere so wilds stay threatening
            import random as _r
            self._respawn_queue.append((45.0, e))

    def _start_duel(self, e: Enemy) -> None:
        self.duel = True
        self.duel_enemy = e
        self.duel_cx = (self.player.x + e.x) / 2
        self.duel_cy = (self.player.y + e.y) / 2
        # pull both toward center slightly
        self.player.x = self.duel_cx - 1.2
        self.player.y = self.duel_cy
        e.x = self.duel_cx + 1.2
        e.y = self.duel_cy
        self.sfx.play(self.sfx.duel)
        self.toast_msg("DUEL! Defeat the Ash Drake inside the ember bubble!")
        spawn_hit_particles(self.particles, self.duel_cx, self.duel_cy, DUEL_GLOW, 20)

    def _end_duel(self, won: bool) -> None:
        self.duel = False
        self.duel_enemy = None
        if won:
            self.sfx.play(self.sfx.win)
            self.toast_msg("Duel won! The bubble fades…")
        else:
            self.sfx.play(self.sfx.lose)

    def _duel_constrain(self) -> None:
        if not self.duel:
            return
        for ent in (self.player, self.duel_enemy):
            if ent is None:
                continue
            dx = ent.x - self.duel_cx
            dy = ent.y - self.duel_cy
            dist = math.hypot(dx, dy)
            if dist > self.duel_r:
                nx, ny = _norm(dx, dy)
                ent.x = self.duel_cx + nx * self.duel_r
                ent.y = self.duel_cy + ny * self.duel_r

    def handle_death(self) -> None:
        loss = int(self.player.inv.gold * GOLD_LOSS_FRAC)
        self.player.inv.gold = max(0, self.player.inv.gold - loss)
        self.player.hp = self.player.max_hp
        self.player.alive = True
        self.player.x, self.player.y = self.checkpoint
        self.player.i_frames = 2.0
        if self.duel:
            # reset boss if fled via death
            if self.duel_enemy and not self.duel_enemy.alive:
                pass
            elif self.duel_enemy:
                self.duel_enemy.hp = self.duel_enemy.max_hp
                self.duel_enemy.x, self.duel_enemy.y = BOSS_CX + 0.5, BOSS_CY + 0.5
            self._end_duel(won=False)
        self.state = "play"
        self.toast_msg(f"Woke at the inn. Lost {loss} gold.")
        self.sfx.play(self.sfx.heal)

    def update_play(self, dt: float) -> None:
        keys = pygame.key.get_pressed()
        self.player.update(dt, self.world, keys)
        self._duel_constrain()

        # enemies
        for e in self.enemies:
            if not e.alive:
                continue
            if self.duel and e is not self.duel_enemy:
                continue
            if self.duel and e is self.duel_enemy:
                hit = e.update(dt, self.player, self.world, in_duel=True)
            else:
                # don't aggro in towns
                if self.world.is_town(e.x, e.y):
                    continue
                hit = e.update(dt, self.player, self.world, in_duel=False)
            if hit:
                dealt = self.player.take_damage(hit)
                if dealt:
                    self.float_text(self.player.x, self.player.y, str(dealt), (255, 80, 80))
                    self.sfx.play(self.sfx.hurt)
                    spawn_hit_particles(self.particles, self.player.x, self.player.y, (255, 80, 80), 6)
                if not self.player.alive:
                    self.state = "death"
                    self.sfx.play(self.sfx.lose)
                    return

        # projectiles
        for p in self.projectiles[:]:
            if not p.update(dt):
                self.projectiles.remove(p)
                continue
            if p.from_player:
                for e in self.enemies:
                    if not e.alive:
                        continue
                    if self.duel and e is not self.duel_enemy:
                        continue
                    if math.hypot(e.x - p.x, e.y - p.y) < e.radius + p.radius / 64:
                        dead = e.take_damage(p.dmg)
                        self.float_text(e.x, e.y, str(p.dmg), EMBER)
                        spawn_hit_particles(self.particles, e.x, e.y, EMBER)
                        self.sfx.play(self.sfx.hit)
                        if p in self.projectiles:
                            self.projectiles.remove(p)
                        if dead:
                            self._on_enemy_killed(e)
                        elif e.boss and not self.duel:
                            self._start_duel(e)
                        break
            if self.world.solid(p.x, p.y) and p in self.projectiles:
                self.projectiles.remove(p)

        # particles
        self.particles = [pt for pt in self.particles if pt.update(dt)]

        # floating damage
        nf = []
        for x, y, text, life, col in self.floating:
            life -= dt
            if life > 0:
                nf.append((x, y - dt * 0.6, text, life, col))
        self.floating = nf

        # trash respawns
        still = []
        for wait, en in self._respawn_queue:
            wait -= dt
            if wait <= 0:
                # relocate
                for _ in range(30):
                    x = self.rng.uniform(10, 72)
                    y = self.rng.uniform(10, 72)
                    if self.world.is_town(x, y) or self.world.solid(x, y):
                        continue
                    if math.hypot(x - self.player.x, y - self.player.y) < 8:
                        continue
                    en.x, en.y = x, y
                    en.hp = en.max_hp
                    en.alive = True
                    break
            else:
                still.append((wait, en))
        self._respawn_queue = still

        if self.toast_t > 0:
            self.toast_t -= dt

        # camera lerp
        self.cam_x += (self.player.x - self.cam_x) * min(1.0, dt * 6)
        self.cam_y += (self.player.y - self.cam_y) * min(1.0, dt * 6)

        # engage duel if close to boss and hit already handled; also auto-engage on proximity attack
        for e in self.enemies:
            if e.boss and e.alive and not self.duel:
                if math.hypot(e.x - self.player.x, e.y - self.player.y) < 1.5:
                    # wait for player attack to start — or start if player very close and attacking
                    pass

    def draw_duel_bubble(self) -> None:
        if not self.duel:
            return
        from .constants import DUEL_GLOW, DUEL_GLOW2, CYAN, MAGENTA
        pulse = 0.2 * math.sin(self.time * 6)
        steps = 56
        pts = []
        pts2 = []
        for i in range(steps):
            ang = i / steps * math.tau
            wx = self.duel_cx + math.cos(ang) * (self.duel_r + pulse)
            wy = self.duel_cy + math.sin(ang) * (self.duel_r + pulse)
            pts.append(iso.world_to_screen(wx, wy, self.cam_x, self.cam_y))
            wx2 = self.duel_cx + math.cos(ang) * (self.duel_r + pulse + 0.25)
            wy2 = self.duel_cy + math.sin(ang) * (self.duel_r + pulse + 0.25)
            pts2.append(iso.world_to_screen(wx2, wy2, self.cam_x, self.cam_y))
        if len(pts) > 2:
            s = pygame.Surface((WIDTH, HEIGHT), pygame.SRCALPHA)
            # neon magenta fill + cyan outer ring
            pygame.draw.polygon(s, (255, 60, 255, 55), pts)
            pygame.draw.polygon(s, (80, 255, 255, 180), pts, 4)
            pygame.draw.polygon(s, (255, 255, 120, 200), pts2, 2)
            self.screen.blit(s, (0, 0))
            # sparkles on rim
            for i in range(0, steps, 4):
                px, py = pts[i]
                col = CYAN if (i // 4 + int(self.time * 8)) % 2 == 0 else MAGENTA
                pygame.draw.circle(self.screen, col, (int(px), int(py)), 3)

    def draw_play(self) -> None:
        self.screen.fill(BG)
        draw_world(self.screen, self.world, self.cam_x, self.cam_y, self.fonts["sm"])
        self.draw_duel_bubble()

        # depth sort entities
        draw_list = []
        for n in self.npcs:
            draw_list.append((n.x + n.y, "npc", n))
        for e in self.enemies:
            if e.alive:
                if self.duel and e is not self.duel_enemy:
                    continue
                draw_list.append((e.x + e.y, "enemy", e))
        draw_list.append((self.player.x + self.player.y, "player", self.player))
        draw_list.sort(key=lambda t: t[0])
        for _, kind, obj in draw_list:
            if kind == "npc":
                draw_npc(self.screen, obj, self.cam_x, self.cam_y, self.time)
            elif kind == "enemy":
                draw_enemy(self.screen, obj, self.cam_x, self.cam_y, self.time)
            else:
                draw_player(self.screen, obj, self.cam_x, self.cam_y, self.time)

        for p in self.projectiles:
            draw_projectile(self.screen, p, self.cam_x, self.cam_y)

        for pt in self.particles:
            sx, sy = iso.world_to_screen(pt.x, pt.y, self.cam_x, self.cam_y)
            pygame.draw.circle(self.screen, pt.color, (int(sx), int(sy - 8)), max(1, int(pt.size)))

        for x, y, text, life, col in self.floating:
            sx, sy = iso.world_to_screen(x, y, self.cam_x, self.cam_y)
            t = self.fonts["sm"].render(text, True, col)
            t.set_alpha(int(255 * min(1.0, life)))
            self.screen.blit(t, (sx - t.get_width() // 2, sy - 40))

        ui.draw_hud(self.screen, self.fonts, self.player, self.quests.main_hint(), duel=self.duel)

        if self.toast_t > 0:
            t = self.fonts["md"].render(self.toast, True, CREAM)
            bg = pygame.Surface((t.get_width() + 24, t.get_height() + 12), pygame.SRCALPHA)
            bg.fill((20, 14, 16, 200))
            self.screen.blit(bg, (WIDTH // 2 - bg.get_width() // 2, HEIGHT - 70))
            self.screen.blit(t, (WIDTH // 2 - t.get_width() // 2, HEIGHT - 64))

        # interact prompt
        if self.state == "play" and self.nearest_npc():
            n = self.nearest_npc()
            prompt = self.fonts["sm"].render(f"E — Talk to {n.name}", True, GOLD)
            self.screen.blit(prompt, (WIDTH // 2 - prompt.get_width() // 2, HEIGHT - 100))

        if self.story_won and self.state == "play":
            ui.draw_victory_banner(self.screen, self.fonts, "Ashfall names you Free Mage!", self.time)

    def handle_event(self, event: pygame.event.Event) -> bool:
        """Return False to quit."""
        if event.type == pygame.QUIT:
            return False

        if event.type == pygame.KEYDOWN:
            k = event.key
            if self.state == "title":
                if k in (pygame.K_UP, pygame.K_w):
                    self.menu_sel = (self.menu_sel - 1) % 3
                    self.sfx.play(self.sfx.click)
                elif k in (pygame.K_DOWN, pygame.K_s):
                    self.menu_sel = (self.menu_sel + 1) % 3
                    self.sfx.play(self.sfx.click)
                elif k in (pygame.K_RETURN, pygame.K_SPACE):
                    if self.menu_sel == 0:
                        self.start_new()
                    elif self.menu_sel == 1:
                        self._controls_from_pause = False
                        self.state = "controls"
                    else:
                        return False
                elif k == pygame.K_ESCAPE:
                    return False
            elif self.state == "controls":
                if k == pygame.K_ESCAPE:
                    # Came from pause menu item 1, else title
                    self.state = "pause" if getattr(self, "_controls_from_pause", False) else "title"
                    self._controls_from_pause = False
            elif self.state == "pause":
                if k in (pygame.K_UP, pygame.K_w):
                    self.pause_sel = (self.pause_sel - 1) % 3
                elif k in (pygame.K_DOWN, pygame.K_s):
                    self.pause_sel = (self.pause_sel + 1) % 3
                elif k in (pygame.K_RETURN, pygame.K_SPACE):
                    if self.pause_sel == 0:
                        self.state = "play"
                    elif self.pause_sel == 1:
                        self._controls_from_pause = True
                        self.state = "controls"
                    else:
                        self.state = "title"
                elif k == pygame.K_ESCAPE:
                    self.state = "play"
            elif self.state == "dialogue":
                if k in (pygame.K_e, pygame.K_RETURN, pygame.K_SPACE):
                    self.advance_dialogue()
            elif self.state == "shop":
                if k in (pygame.K_UP, pygame.K_w):
                    self.shop_sel = (self.shop_sel - 1) % max(1, len(self.active_shop.shop))
                    self.sfx.play(self.sfx.click)
                elif k in (pygame.K_DOWN, pygame.K_s):
                    self.shop_sel = (self.shop_sel + 1) % max(1, len(self.active_shop.shop))
                    self.sfx.play(self.sfx.click)
                elif k in (pygame.K_RETURN, pygame.K_SPACE):
                    self.buy_selected()
                elif k == pygame.K_ESCAPE:
                    self.state = "play"
                    self.active_shop = None
            elif self.state == "inventory":
                if k in (pygame.K_i, pygame.K_ESCAPE):
                    self.state = "play"
                elif k == pygame.K_h:
                    if self.player.inv.remove("health_vial", 1):
                        healed = self.player.heal(CATALOG["health_vial"].heal)
                        self.toast_msg(f"Healed {healed}")
                        self.sfx.play(self.sfx.heal)
                    elif self.player.inv.remove("greater_vial", 1):
                        healed = self.player.heal(CATALOG["greater_vial"].heal)
                        self.toast_msg(f"Healed {healed}")
                        self.sfx.play(self.sfx.heal)
                    else:
                        self.toast_msg("No vials.")
            elif self.state == "quests":
                if k in (pygame.K_q, pygame.K_ESCAPE):
                    self.state = "play"
            elif self.state == "death":
                if k in (pygame.K_RETURN, pygame.K_SPACE, pygame.K_e):
                    self.handle_death()
            elif self.state == "play":
                if k == pygame.K_ESCAPE:
                    self.state = "pause"
                    self.pause_sel = 0
                elif k == pygame.K_e:
                    self.try_interact()
                elif k == pygame.K_SPACE:
                    self.try_attack()
                elif k == pygame.K_i:
                    self.state = "inventory"
                elif k == pygame.K_q:
                    self.state = "quests"
                elif k == pygame.K_1:
                    self.player.spell_mode = False
                    self.toast_msg("Staff melee")
                elif k == pygame.K_2:
                    if self.player.spell_unlocked:
                        self.player.spell_mode = True
                        self.toast_msg("Emberbolt ready")
                    else:
                        self.toast_msg("Emberbolt unlocks at level 3")
                elif k == pygame.K_h:
                    if self.player.inv.remove("health_vial", 1):
                        healed = self.player.heal(25)
                        self.toast_msg(f"Healed {healed}")
                        self.sfx.play(self.sfx.heal)

        if event.type == pygame.MOUSEBUTTONDOWN and self.state == "play":
            if event.button == 1:
                # face toward mouse world pos
                mx, my = event.pos
                wx, wy = iso.screen_to_world(mx, my, self.cam_x, self.cam_y)
                self.player.facing = _norm(wx - self.player.x, wy - self.player.y)
                self.try_attack()
            elif event.button == 3:
                self.try_interact()

        return True

    def run(self, max_seconds: Optional[float] = None) -> None:
        running = True
        elapsed = 0.0
        while running:
            dt = self.clock.tick(FPS) / 1000.0
            dt = min(dt, 0.05)
            self.time += dt
            elapsed += dt
            for event in pygame.event.get():
                if not self.handle_event(event):
                    running = False

            if self.state == "play":
                self.update_play(dt)
            elif self.state == "controls" and self.pause_sel == 1:
                pass

            # draw
            if self.state == "title":
                ui.draw_title(self.screen, self.fonts, self.time, self.menu_sel)
            elif self.state == "controls":
                self.screen.fill(BG)
                ui.draw_controls_screen(self.screen, self.fonts)
            else:
                self.draw_play()
                if self.state == "dialogue":
                    ui.draw_dialogue(self.screen, self.fonts,
                                     self.dialogue_npc.name if self.dialogue_npc else "",
                                     self.dialogue_text)
                elif self.state == "shop" and self.active_shop:
                    ui.draw_shop(self.screen, self.fonts, f"{self.active_shop.name}'s Wares",
                                 self.active_shop.shop, self.player.inv, self.shop_sel)
                elif self.state == "inventory":
                    ui.draw_inventory(self.screen, self.fonts, self.player)
                elif self.state == "quests":
                    ui.draw_quest_log(self.screen, self.fonts, self.quests)
                elif self.state == "pause":
                    ui.draw_pause(self.screen, self.fonts, self.pause_sel)
                elif self.state == "death":
                    ui.draw_death(self.screen, self.fonts)

            pygame.display.flip()

            if max_seconds is not None and elapsed >= max_seconds:
                running = False

        pygame.quit()
