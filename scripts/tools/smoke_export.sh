#!/usr/bin/env bash
# Smoke-check export preset and critical game assets (no GPU required).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

echo "== Export / asset smoke check =="

fail=0
need() {
  if [ ! -e "$1" ]; then
    echo "MISSING: $1"
    fail=1
  else
    echo "OK: $1"
  fi
}

need project.godot
need export_presets.cfg
need scenes/main_menu.tscn
need scenes/game.tscn
need scenes/player.tscn
need scenes/enemies/enemy.tscn
need scenes/projectiles/projectile.tscn
need scenes/effects/impact.tscn
need scenes/effects/explosion.tscn
need scripts/autoload/game_state.gd
need scripts/autoload/event_bus.gd
need scripts/combat/health.gd
need scripts/combat/score_table.gd
need scripts/systems/wave_definitions.gd
need tests/run_tests.gd
need assets/audio/laser.wav
need assets/audio/explosion.wav
need icon.svg
need LICENSE
need README.md

grep -q 'name="Linux"' export_presets.cfg || { echo "FAIL: Linux preset name"; fail=1; }
grep -q 'platform="Linux"' export_presets.cfg || { echo "FAIL: Linux platform"; fail=1; }
grep -q 'run/main_scene="res://scenes/main_menu.tscn"' project.godot || { echo "FAIL: main scene"; fail=1; }
grep -q 'move_forward=' project.godot || { echo "FAIL: input map"; fail=1; }
grep -q 'hyperspace=' project.godot || { echo "FAIL: hyperspace action"; fail=1; }

# Count GDScript files
script_count=$(find scripts -name '*.gd' | wc -l)
echo "GDScript files: $script_count"
if [ "$script_count" -lt 10 ]; then
  echo "FAIL: unexpectedly few scripts"
  fail=1
fi

if [ "$fail" -ne 0 ]; then
  echo "SMOKE FAILED"
  exit 1
fi
echo "SMOKE PASSED"
