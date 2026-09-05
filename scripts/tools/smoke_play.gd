extends SceneTree

var _frames := 0
const MAX_FRAMES := 120

func _init() -> void:
	call_deferred("_start")

func _start() -> void:
	var err := change_scene_to_file("res://scenes/game.tscn")
	if err != OK:
		push_error("Failed to load game: %s" % err)
		quit(1)
		return
	print("SMOKE_PLAY: game scene loaded")

func _process(_dt: float) -> bool:
	_frames += 1
	if _frames == 45:
		var gs = root.get_node("GameState")
		print("SMOKE_PLAY: frame 45 enemies=", gs.enemies_alive, " wave=", gs.wave, " phase=", gs.phase)
	if _frames >= MAX_FRAMES:
		var gs = root.get_node("GameState")
		print("SMOKE_PLAY: PASS frames=", _frames, " score=", gs.score, " enemies=", gs.enemies_alive)
		quit(0)
	return false
