# Vector Drift

A 3D space-combat arcade game in C++ / OpenGL. Newtonian inertia flight, dogfighting AI, neon HUD + radar.

**Feel:** classic vector-arcade combat + free-flight 3D dogfighting. Turning changes facing only — thrust changes velocity. Drift is a skill.

## Requirements (Debian/Ubuntu)

```bash
sudo apt install g++ cmake libglfw3-dev libgl1-mesa-dev pkg-config
```

Optional for headless smoke tests: `xvfb`.

## Build

```bash
./build.sh
```

Produces `build/vector_drift` (Release).

```bash
./build.sh Debug   # optional
```

## Run

```bash
./build/vector_drift
```

Headless / CI:

```bash
LIBGL_ALWAYS_SOFTWARE=1 xvfb-run -a ./build/vector_drift
```

## Controls

| Input | Action |
|-------|--------|
| Mouse | Pitch / yaw |
| Q / E | Roll |
| W | Thrust |
| S | Reverse / brake |
| LMB or Space | Fire |
| Left Shift or X | Velocity damp / match-speed assist |
| Esc | Pause / resume |
| R | Restart after death |
| Space (title) | Launch |

Controls are also shown on the title and pause screens.

## Gameplay

- Title → dogfight waves → death / score → restart
- Start with 1 enemy and spawn grace; waves escalate
- Score from kills, wave clears, and survival
- Use radar (bottom-right) for off-screen contacts
- Same inertia physics for you and enemies — intercept, overshoot, evade

## Tech

- C++17, CMake, OpenGL 3.3 core, GLFW, GLM (FetchContent), Glad, miniaudio (procedural SFX)
- All graphics procedural — no copyrighted assets

## License

See `LICENSE`.
