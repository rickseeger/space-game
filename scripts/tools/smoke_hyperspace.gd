extends SceneTree
## Verifies hyperspace recenters the player and the next wave spawns nearby.

var _frames := 0
var _phase := "boot"

func _init() -> void:
	call_deferred("_start")

func _start() -> void:
	var err := change_scene_to_file("res://scenes/game.tscn")
	if err != OK:
		push_error("HYPER_SMOKE: failed to load game: %s" % err)
		quit(1)
		return
	print("HYPER_SMOKE: loaded")

func _process(_dt: float) -> bool:
	_frames += 1
	var gs = root.get_node_or_null("GameState")
	if gs == null:
		if _frames > 30:
			push_error("HYPER_SMOKE: no GameState")
			quit(1)
			return true
		return false

	if _phase == "boot" and _frames >= 45:
		print("HYPER_SMOKE: enemies=", gs.enemies_alive, " wave=", gs.wave)
		if gs.enemies_alive <= 0:
			push_error("HYPER_SMOKE: no enemies")
			quit(1)
			return true
		for e in get_nodes_in_group("enemy"):
			e.queue_free()
		gs.enemies_alive = 0
		gs.wave = gs.WAVES_PER_SECTOR
		gs.hyperspace_ready = true
		gs.phase = gs.Phase.PLAYING
		var players = get_nodes_in_group("player")
		players[0].global_position = Vector3(0, 0, -400)
		var hyper = _find_hyper()
		if hyper == null:
			push_error("HYPER_SMOKE: no controller")
			quit(1)
			return true
		if not hyper.try_jump():
			push_error("HYPER_SMOKE: try_jump failed")
			quit(1)
			return true
		hyper._timer = 0.0
		hyper._finish()
		print("HYPER_SMOKE: forced finish sector=", gs.sector, " player=", players[0].global_position)
		_phase = "check"
		return false

	if _phase == "check" and _frames >= 90:
		var players = get_nodes_in_group("player")
		var ppos: Vector3 = players[0].global_position
		print("HYPER_SMOKE: check player=", ppos, " enemies=", gs.enemies_alive, " wave=", gs.wave)
		if ppos.length() > 5.0:
			push_error("HYPER_SMOKE: FAIL not recentered len=%.1f" % ppos.length())
			quit(1)
			return true
		if gs.enemies_alive <= 0:
			push_error("HYPER_SMOKE: FAIL no wave after jump")
			quit(1)
			return true
		var nearest := 9999.0
		for e in get_nodes_in_group("enemy"):
			var d: float = e.global_position.distance_to(ppos)
			if d < nearest:
				nearest = d
		print("HYPER_SMOKE: nearest=", nearest)
		if nearest > 80.0:
			push_error("HYPER_SMOKE: FAIL enemies far %.1f" % nearest)
			quit(1)
			return true
		print("HYPER_SMOKE: PASS")
		quit(0)
		return true

	if _frames > 300:
		push_error("HYPER_SMOKE: timeout phase=%s" % _phase)
		quit(1)
		return true
	return false

func _find_hyper() -> Node:
	for n in root.get_children():
		var h = n.find_child("HyperspaceController", true, false)
		if h:
			return h
	return null
