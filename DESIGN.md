# Cinder — Design Interpretation

Source: `space-game-design/DESIGN_BIBLE.md` (brother conversation transcript).

## Product

**Cinder** is a Python/pygame isometric open-world wizard RPG. V1 is a polished vertical slice: castle hub, village economy, soft-gated wilds, one main proving quest, two side quests, and a signature duel-bubble boss.

## How we read the bible

| Bible theme | V1 interpretation |
|-------------|-------------------|
| Weak → strong (Minecraft first night / Gandalf the Brown) | Start with wooden staff, 40 HP, no spells. Level 3 unlocks Emberbolt. Gear ladder in shop + quest rewards. |
| Open world, not mission-rail | Free roam after accepting the King's quest; grind, shop, side quests anytime. |
| One consistent story | Fixed through-line: recover the Ember Relic from the Ash Drake. |
| NPCs / economy / loot | 7 talkable NPCs; shop buy+equip; ore/pelt drops; gold sinks. |
| Difficulty by distance | Enemy tier & stats scale with distance from Ashfall Keep. Towns are safe. |
| Gameplay > graphics | Procedural isometric tiles & characters; readable swings/spells; ember/ash palette. |
| No C++ | Python + pygame only. |

## Combat hybrid (user decision)

- **Overworld:** real-time isometric action — staff melee vs trash (slimes, wolves, bandits).
- **Boss / key fights:** engaging the **Ash Drake** (first solid hit or duel trigger) locks both fighters in a **glowing duel bubble**. Spectators/other mobs ignored; tighter arcade duel; juice (ring, banner, SFX). Exit on win (loot relic) or death (inn reload, boss resets).

This maps the LucasArts-style duel idea onto isometric without a cinematic 3D camera.

## Story beat (concrete)

**Prove Your Worth:** King Aldric tasks you with retrieving the Ember Relic from the Ash Drake in the Cinder Nest (SE). Completing it grants gold, XP, and the title of free mage — then the world stays open for grinding.

## Cut from V1 (per bible)

- Dynamic cinematic camera / full 3D
- Party companions
- Procedural life-story each run
- Environmental tree-leap magic as core combat
- Crafting deep systems (shop/equip stands in)

## Visual palette (post-steering)

Classic DQ/FF energy — saturated grass greens, sky-blue void, turquoise water, red/teal/yellow/pink village roofs, vivid NPC clothes. Wizard keeps warm violet + ember accents. Spells flash orange/cyan/magenta; duel bubble is neon magenta/cyan. HUD uses royal-blue panels with bright gold borders. Avoid muddy gray-brown gloom.

## Tech

- `cinder/` package, `python -m cinder`
- Modules: world, entities, combat/duel in `game.py`, quests, ui, items, audio, iso
