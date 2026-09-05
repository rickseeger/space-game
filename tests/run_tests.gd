extends SceneTree
## CLI entry: godot --headless --path . -s res://tests/run_tests.gd
## Loads the test scene so autoloads (GameState, EventBus) are in scope.

func _init() -> void:
	call_deferred("_boot")

func _boot() -> void:
	var err := change_scene_to_file("res://tests/test_main.tscn")
	if err != OK:
		push_error("Failed to load test scene: %s" % err)
		quit(1)
