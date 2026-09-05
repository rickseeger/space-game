extends Node
## Attach to tests/test_main.tscn. Autoloads resolve normally.

var _passed: int = 0
var _failed: int = 0

func _ready() -> void:
	print("========================================")
	print(" Space Hawk — Automated Tests")
	print("========================================")
	_run_suite("Health / Damage", _test_health)
	_run_suite("Scoring", _test_scoring)
	_run_suite("Wave / Spawn definitions", _test_waves)
	_run_suite("Input actions", _test_input_actions)
	_run_suite("Hyperspace / state transitions", _test_hyperspace_state)
	_run_suite("GameState kill tracking", _test_kill_tracking)
	print("========================================")
	print(" Results: %d passed, %d failed" % [_passed, _failed])
	print("========================================")
	await get_tree().process_frame
	get_tree().quit(0 if _failed == 0 else 1)

func _run_suite(name: String, fn: Callable) -> void:
	print("\n-- %s --" % name)
	fn.call()

func assert_true(cond: bool, msg: String) -> void:
	if cond:
		_passed += 1
		print("  PASS  %s" % msg)
	else:
		_failed += 1
		print("  FAIL  %s" % msg)

func assert_eq(a: Variant, b: Variant, msg: String) -> void:
	assert_true(a == b, "%s (got %s, expected %s)" % [msg, str(a), str(b)])

func assert_gt(a: Variant, b: Variant, msg: String) -> void:
	assert_true(a > b, "%s (got %s, expected > %s)" % [msg, str(a), str(b)])

func _test_health() -> void:
	var h := Health.new()
	h.max_hull = 100.0
	h.max_shields = 50.0
	h.shield_regen_rate = 0.0
	h.hull = h.max_hull
	h.shields = h.max_shields

	var dealt := h.apply_damage(30.0)
	assert_eq(dealt, 30.0, "apply_damage returns amount")
	assert_eq(h.shields, 20.0, "shields absorb first 30 of 50")
	assert_eq(h.hull, 100.0, "hull untouched while shields remain")

	h.apply_damage(40.0)
	assert_eq(h.shields, 0.0, "shields depleted")
	assert_eq(h.hull, 80.0, "overflow damages hull")

	h.apply_damage(80.0)
	assert_true(not h.is_alive(), "hull zero => dead")
	assert_eq(h.hull, 0.0, "hull clamped at 0")

	var h2 := Health.new()
	h2.max_hull = 50.0
	h2.max_shields = 0.0
	h2.hull = 50.0
	h2.shields = 0.0
	h2.invulnerable = true
	h2.apply_damage(999.0)
	assert_eq(h2.hull, 50.0, "invulnerable ignores damage")

	h2.invulnerable = false
	h2.apply_damage(20.0)
	h2.heal_hull(10.0)
	assert_eq(h2.hull, 40.0, "heal_hull restores up to max")

	h2.free()
	h.free()

func _test_scoring() -> void:
	assert_eq(ScoreTable.points_for("fighter", 1), 100, "fighter base points")
	assert_eq(ScoreTable.points_for("interceptor", 1), 175, "interceptor base")
	assert_eq(ScoreTable.points_for("heavy", 1), 350, "heavy base")
	assert_eq(ScoreTable.points_for("destroyer", 1), 800, "destroyer base")
	assert_gt(ScoreTable.points_for("fighter", 5), ScoreTable.points_for("fighter", 1), "sector scales points")
	assert_eq(ScoreTable.wave_clear_bonus(2, 3), 250 * 2 + 100 * 3, "wave clear bonus formula")
	assert_eq(ScoreTable.hyperspace_bonus(3), 1500, "hyperspace bonus formula")
	assert_eq(ScoreTable.points_for("unknown", 1), 50, "unknown type fallback")

func _test_waves() -> void:
	var w1 := WaveDefinitions.composition(1, 1)
	assert_gt(w1.size(), 0, "sector1 wave1 has entries")
	assert_eq(w1[0]["type"], "fighter", "starts with fighters")
	assert_gt(int(w1[0]["count"]), 0, "fighter count > 0")

	var early_total := WaveDefinitions.total_enemies(1, 1)
	var late_total := WaveDefinitions.total_enemies(6, 3)
	assert_gt(late_total, early_total, "later sectors spawn more enemies")

	var s5w3 := WaveDefinitions.composition(5, 3)
	var types: Array = []
	for e in s5w3:
		types.append(e["type"])
	assert_true("destroyer" in types, "sector 5 wave 3 includes destroyer")

	var s3w2 := WaveDefinitions.composition(3, 2)
	types.clear()
	for e in s3w2:
		types.append(e["type"])
	assert_true("heavy" in types, "sector 3 wave 2 includes heavy")

	assert_gt(WaveDefinitions.spawn_radius(4), WaveDefinitions.spawn_radius(1), "spawn radius grows")

func _test_input_actions() -> void:
	var required := [
		"move_forward", "move_back", "turn_left", "turn_right",
		"strafe_left", "strafe_right", "fire", "boost", "hyperspace", "pause"
	]
	for action in required:
		assert_true(InputMap.has_action(action), "input action registered: %s" % action)
		assert_gt(InputMap.action_get_events(action).size(), 0, "action %s has events" % action)

func _test_hyperspace_state() -> void:
	GameState.reset_run()
	assert_eq(GameState.phase, GameState.Phase.PLAYING, "reset_run => PLAYING")
	assert_eq(GameState.sector, 1, "reset sector 1")
	assert_eq(GameState.wave, 0, "reset wave 0")
	assert_eq(GameState.score, 0, "reset score 0")

	assert_true(not GameState.can_hyperspace(), "cannot jump before ready")

	GameState.hyperspace_ready = true
	GameState.enemies_alive = 3
	assert_true(not GameState.can_hyperspace(), "cannot jump with hostiles")

	GameState.enemies_alive = 0
	assert_true(GameState.can_hyperspace(), "can jump when ready and clear")

	var started := GameState.start_hyperspace()
	assert_true(started, "start_hyperspace succeeds")
	assert_eq(GameState.phase, GameState.Phase.HYPERSPACE, "phase => HYPERSPACE")
	assert_true(not GameState.can_hyperspace(), "cannot re-enter mid-jump")

	GameState.finish_hyperspace()
	assert_eq(GameState.phase, GameState.Phase.PLAYING, "finish => PLAYING")
	assert_eq(GameState.sector, 2, "sector increments")
	assert_eq(GameState.wave, 0, "wave resets after jump")

	GameState.mark_player_dead()
	assert_eq(GameState.phase, GameState.Phase.DEAD, "death => DEAD")

func _test_kill_tracking() -> void:
	GameState.reset_run()
	GameState.begin_wave()
	assert_eq(GameState.wave, 1, "begin_wave increments")
	GameState.enemies_alive = 2
	GameState.register_kill("fighter", 100)
	assert_eq(GameState.enemies_alive, 1, "kill decrements alive")
	assert_eq(GameState.score, 100, "score added")
	assert_eq(GameState.session_kills, 1, "session kills tracked")
	GameState.register_kill("fighter", 100)
	assert_eq(GameState.enemies_alive, 0, "all dead")
	GameState.wave = GameState.WAVES_PER_SECTOR
	GameState.enemies_alive = 1
	GameState.register_kill("heavy", 350)
	assert_true(GameState.hyperspace_ready, "hyperspace ready after final wave clear")
