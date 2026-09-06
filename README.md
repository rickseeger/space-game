# Cinder

**An isometric open-world fantasy RPG** — you are a wizard who starts extremely weak and earns power through XP, gold, gear, and spells.

Built with **Python 3.11+** and **pygame**. Procedural art, procedural audio, one authored story.

## Install & Run

```bash
python3 -m venv .venv
source .venv/bin/activate   # Windows: .venv\Scripts\activate
pip install -r requirements.txt
python -m cinder
```

Or:

```bash
./run.sh
```

Headless / CI smoke (needs Xvfb):

```bash
xvfb-run -a ./run.sh --smoke 3
xvfb-run -a ./run.sh --screenshot
```

## Controls

| Input | Action |
|-------|--------|
| WASD / Arrows | Move |
| Space / Left mouse | Attack / cast |
| E / Right mouse | Talk / interact |
| 1 | Staff melee |
| 2 | Emberbolt (unlocks at level 3) |
| H | Drink a health vial |
| I | Inventory |
| Q | Quest log |
| Esc | Pause |

## Story

You are an apprentice of **Ashfall Keep**. King Aldric will not name you a free mage until you recover the **Ember Relic**, stolen by the **Ash Drake** nesting in the **Cinder Nest** southeast of Ember Village.

Travel the overworld, grind in the wilds, gear up in town, then face the Drake in a glowing **duel bubble**. Return the relic to the King.

### Side quests

- **Ore for the Forge** — Blacksmith Brenna wants 3 Iron Ore (from bandits).
- **Pelts for the Inn** — Innkeeper Maro wants 4 Wolf Pelts (checkpoint + gold).

## Features (V1 slice)

- Isometric overworld: castle → village → scaling wilds
- Real-time staff combat; Emberbolt spell at level 3
- Hybrid **duel-bubble** boss fight (Ash Drake)
- NPCs, dialogue, shops, inventory, quests
- Death → wake at inn with partial gold loss
- Title, HUD, pause, procedural SFX

## Design notes

See [DESIGN.md](DESIGN.md) for how this interprets the design bible (hybrid combat, weak→strong fantasy, scope cuts).

## License

MIT — see [LICENSE](LICENSE).
