# Vector Drift — Development Report

## Summary

Replaced the prior Godot prototype with a from-scratch C++17 / OpenGL 3.3 arcade dogfighter. Goal: fun Newtonian flight + readable 3D combat, not a stationary shooting gallery.

## Build

- `./build.sh` → `build/vector_drift`
- Deps: g++, cmake, libglfw3-dev, libgl1-mesa-dev
- GLM via CMake FetchContent; Glad + miniaudio + stb vendored under `third_party/`

## Key design decisions

### Physics
- State: position, velocity, quaternion orientation, local angular velocity, thrust
- Mouse/Q/E change orientation only; velocity is independent (drift after turns)
- Light linear drag (`~0.28`), max speed `100`, rotational damping for arcade readability
- Left Shift / X: strong velocity damp assist without deleting inertia skill
- Player thrust accel `~62`, reverse `~40`

### AI (one excellent fighter type)
- States: Search → Pursue → Attack → Evade / Reposition
- Lead intercept aiming with jitter; burst fire + fire-hold (no continuous surround fire)
- Overshoot detection → reposition/flank; imperfect alignment required to shoot
- Scaled physics (slightly slower/less agile than player)
- Wave 1: 1 enemy, far spawn + ~4s fire grace

### HUD / radar
- Neon cyan/amber: reticle, target box, distance, closing speed, speed, shield/hull
- Corner tactical radar in player local frame (forward up on scope); elevation-tinted blips
- Off-screen chevrons when target not in view

### Visuals / audio
- Procedural faceted fighters (cyan player, amber/red enemies), engine glow
- Dense starfield with speed streaking; dust particles; glowing laser bolts
- Explosions: expanding sphere + sparks + debris shards
- Procedural SFX via miniaudio (lasers, hits, explosions, thrust, warnings)

## Playtest / iteration notes

1. First playable: flight + chase cam + stars + HUD/radar + 1 enemy wave
2. Tuned drag/thrust for clearer “slide then burn” feel; denser stars; bigger explosions
3. AI: longer fire holds, stricter aim gate, farther wave-1 spawn, spawn grace so openings aren’t unfair
4. Smoke-tested under `xvfb` + Mesa software GL; screenshots in `screenshots/`

## Polish passes (post-first-push)

2. Mouse look less twitchy; angular damp when stick released; laser muzzle sparks
3. Larger explosion flash; brighter enemies; bigger radar blips; title screenshot path

## Known limitations

- No gamepad; keyboard+mouse only
- Audio may fall back to no-device / silent on headless hosts without ALSA
- Single enemy archetype (intentionally); no multiplayer
- Software GL is fine for CI but soft on fill-rate / point sprites
- Simple stroke font (HUD), not TTF

## Layout

```
src/          main, game, ship, weapon, enemy_ai, renderer, hud, audio, particles, mesh, camera
include/      headers
shaders/      GLSL 330 core
third_party/  glad, miniaudio, stb_image_write
build.sh      Release build helper
```
