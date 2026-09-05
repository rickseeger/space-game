# Contributing

1. Use **Godot 4.3.stable** to match CI.
2. Prefer procedural meshes/materials over large binaries.
3. Keep tests green: `godot --headless --path . -s res://tests/run_tests.gd`
4. Run `bash scripts/tools/smoke_export.sh` before opening a PR.
