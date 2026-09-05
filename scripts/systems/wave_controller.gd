extends Node
class_name WaveController
## Spawns waves from WaveDefinitions and tracks clear → hyperspace gating.

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
	var comp := WaveDefinitions.composition(GameState.sector, GameState.wave)
	var radius := WaveDefinitions.spawn_radius(GameState.sector)
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
	var e: EnemyShip = enemy_scene.instantiate()
	_spawn_root.add_child(e)
	var angle := (TAU * float(index) / 8.0) + randf() * 0.4
	var height := randf_range(-6.0, 6.0)
	e.global_position = Vector3(cos(angle) * radius, height, sin(angle) * radius)
	e.configure(typ)

func _on_wave_cleared(_sector: int, wave: int) -> void:
	var bonus := ScoreTable.wave_clear_bonus(GameState.sector, wave)
	GameState.add_score(bonus)
	EventBus.warning.emit("WAVE CLEAR +%d" % bonus, 0)
	if wave < GameState.WAVES_PER_SECTOR:
		_pending_next = true
		_between_wave_timer = 2.5
	else:
		request_hyperspace_hint.emit()

func _on_hyperspace_ended(_new_sector: int) -> void:
	# Mild heal between sectors
	var players := get_tree().get_nodes_in_group("player")
	for p in players:
		if p is PlayerShip and p.health:
			p.health.heal_hull(25.0)
			p.health.shields = minf(p.health.max_shields, p.health.shields + 30.0)
			EventBus.hull_changed.emit(p.health.hull, p.health.max_hull)
			EventBus.shields_changed.emit(p.health.shields, p.health.max_shields)
	call_deferred("start_sector")
