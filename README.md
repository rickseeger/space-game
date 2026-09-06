# Space Hawk

Keyboard-driven **Godot 4** space combat. Fly a fighter through hostile sectors, burn boost for speed, dump lasers into escalating waves (fighters → interceptors → heavies → destroyers), then punch into hyperspace when the sector is clear.

Built for ordinary Linux laptops / integrated GPUs. All assets are procedural or engine-native — nothing proprietary; MIT-licensed and GitHub-friendly.

## Goals

- Sense of speed: engine trail, boost, speed-line VFX, hyperspace tunnel flash
- Cockpit-style HUD: hull / shields / boost bars, score, sector/wave, severity-colored warnings
- Progressive difficulty across sectors and waves, including larger capital threats
- Fun 1–2 hour sessions with retry loop (menu → play → death → retry)
- Accessible defaults: WASD + arrows, one-button fire, clear warnings

## Requirements

| Item | Version |
|------|---------|
| **Godot** | **4.3.stable** (exact CI / docs target) |
| OS | Pop!_OS 22.04+, Ubuntu 22.04+ (other Linux OK) |
| GPU | Integrated OK (Forward Plus, MSAA 2×) |

## Install (Pop!_OS / Ubuntu)

### Option A — Official Godot 4.3 binary (recommended)

```bash
# One-liner: download Godot 4.3.stable Linux x86_64 into ~/godot
mkdir -p ~/godot && cd ~/godot \
  && curl -fsSL -o godot.zip \
       https://github.com/godotengine/godot/releases/download/4.3-stable/Godot_v4.3-stable_linux.x86_64.zip \
  && unzip -o godot.zip && chmod +x Godot_v4.3-stable_linux.x86_64 \
  && ln -sf "$PWD/Godot_v4.3-stable_linux.x86_64" ~/godot/godot \
  && echo 'export PATH="$HOME/godot:$PATH"' >> ~/.bashrc && export PATH="$HOME/godot:$PATH" \
  && godot --version   # expect: 4.3.stable.official...
```

### Option B — Flatpak

```bash
flatpak install -y flathub org.godotengine.Godot
# Prefer 4.3.x when choosing the Flatpak branch if multiple are offered
```

### Clone & open

```bash
git clone https://github.com/rickseeger/space-game.git
cd space-game
godot --path .            # play immediately — no editor import step required
```

SFX are generated in code (no WAV import). Optional: `godot --editor .` if you want the editor.

## Play

```bash
cd space-game
godot --path .
```

Or open the project in the editor and press **F5**.

### Controls

| Action | Keys |
|--------|------|
| Thrust / brake | **W / S** or **↑ / ↓** |
| Turn | **A / D** or **← / →** |
| Strafe | **Q / E** |
| Fire | **Space** |
| Boost | **Shift** |
| Hyperspace (when ready) | **H** |
| Pause | **Esc** |

Clear all waves in a sector → HUD shows **HYPERSPACE READY** → press **H** to jump. Each sector heals you slightly and raises the stakes.

## Testing

### Automated (headless)

```bash
# Import once, then run the suite (damage, scoring, waves, input, hyperspace/state)
godot --headless --path . --quit-after 1
godot --headless --path . -s res://tests/run_tests.gd
```

Exit code `0` = all passed.

### Export / asset smoke

```bash
bash scripts/tools/smoke_export.sh
```

### CI

GitHub Actions (`.github/workflows/ci.yml`) on `ubuntu-latest`:

1. Installs Godot **4.3.stable**
2. Imports the project
3. Runs `tests/run_tests.gd` (fails the job on failure)
4. Runs export-preset / asset smoke checks

### Manual 10-minute play checklist

Use this before tagging a release:

1. [ ] Main menu loads; **START MISSION** enters combat
2. [ ] WASD / arrows move and turn; ship feels responsive
3. [ ] **Space** fires cyan bolts; impacts flash on hit
4. [ ] **Shift** boost drains bar and increases speed / trail
5. [ ] Enemy fighters spawn (sector 1 wave 1); they chase and shoot
6. [ ] Taking hits drains shields then hull; warnings appear
7. [ ] Destroying all enemies advances waves; score increases
8. [ ] After wave 3, hyperspace prompt appears; **H** plays jump VFX and loads next sector
9. [ ] Later sectors include interceptors / heavies / destroyers
10. [ ] Dying shows game over + score; **RETRY** restarts; **MAIN MENU** returns
11. [ ] **Esc** pauses and resumes
12. [ ] Runs ≥10 minutes on integrated GPU without hitching hard

## Project layout

```
scenes/           Main menu, game world, player, enemies, projectiles, VFX, UI
scripts/          Autoloads, combat, AI, systems, HUD
tests/            Headless unit / integration suite
.github/workflows CI
export_presets.cfg Linux x86_64 preset
```

## License

MIT — see [LICENSE](LICENSE). Engine: [Godot](https://godotengine.org) (MIT). No third-party proprietary assets.
