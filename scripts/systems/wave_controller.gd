extends Node
class_name WaveController
## Spawns waves from WaveDefinitions and tracks clear → hyperspace gating.

const WaveDefinitions = preload("res://scripts/systems/wave_definitions.gd")
const ScoreTable = preload("res://scripts/combat/score_table.gd")

signal request_hyperspace_hint

@export var enemy_scene: PackedScene
@export var spawn_root_path: NodePath

var _spawn_root: Node3D
var _between_wave_timer: float = -1.0
var _pending_next: bool = false

func _ready() -> void:
	_spawn_root = get_node(spawn_root_path)
	EventBus.wave_cleared.connect(_on_wave_cleared)
	EventBus.hyperspace_ended.connect(_on_hyperspace_ended)

func start_sector() -> void:
	GameState.wave = 0
	_spawn_next_wave()

func _process(delta: float) -> void:
	if _pending_next and GameState.phase == GameState.Phase.PLAYING:
		_between_wave_timer -= delta
		if _between_wave_timer <= 0.0:
			_pending_next = false
			_spawn_next_wave()

func _spawn_next_wave() -> void:
	GameState.begin_wave()
	var comp: Array = WaveDefinitions.composition(GameState.sector, GameState.wave)
	var radius: float = WaveDefinitions.spawn_radius(GameState.sector)
	var spawned := 0
	for entry in comp:
		var typ: String = entry["type"]
		var count: int = entry["count"]
		for i in count:
			_spawn_enemy(typ, radius, spawned)
			spawned += 1
	GameState.enemies_alive = spawned
	EventBus.warning.emit("SECTOR %d — WAVE %d" % [GameState.sector, GameState.wave], 0)

func _spawn_enemy(typ: String, radius: float, index: int) -> void:
	if enemy_scene == null or _spawn_root == null:
		return
	var e = enemy_scene.instantiate()
	_spawn_root.add_child(e)
	var origin := Vector3.ZERO
	var players := get_tree().get_nodes_in_group("player")
	if players.size() > 0 and players[0] is Node3D:
		origin = (players[0] as Node3D).global_position
	var angle := (TAU * float(index) / 8.0) + randf() * 0.35
	# Keep spawns near the flight plane — player has yaw-only controls.
	var height := origin.y + randf_range(-1.25, 1.25)
	e.global_position = origin + Vector3(cos(angle) * radius, height - origin.y, sin(angle) * radius)
	e.configure(typ)

func _on_wave_cleared(_sector: int, wave: int) -> void:
	var bonus: int = ScoreTable.wave_clear_bonus(GameState.sector, wave)
	GameState.add_score(bonus)
	EventBus.warning.emit("WAVE CLEAR +%d" % bonus, 0)
	if wave < GameState.WAVES_PER_SECTOR:
		_pending_next = true
		_between_wave_timer = 2.5
	else:
		request_hyperspace_hint.emit()

func _on_hyperspace_ended(_new_sector: int) -> void:
	var players := get_tree().get_nodes_in_group("player")
	for p in players:
		if p.has_method("take_hit") and p.get("health"):
			p.health.heal_hull(25.0)
			p.health.shields = minf(p.health.max_shields, p.health.shields + 30.0)
			EventBus.hull_changed.emit(p.health.hull, p.health.max_hull)
			EventBus.shields_changed.emit(p.health.shields, p.health.max_shields)
	call_deferred("start_sector")
