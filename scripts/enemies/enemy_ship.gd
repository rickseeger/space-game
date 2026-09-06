extends CharacterBody3D
class_name EnemyShip
## Base hostile with simple pursuit + shoot AI.

const ScoreTable = preload("res://scripts/combat/score_table.gd")

@export var enemy_type: String = "fighter"
@export var move_speed: float = 22.0
@export var turn_rate: float = 1.8
@export var fire_range: float = 35.0
@export var fire_cooldown: float = 1.1
@export var projectile_damage: float = 8.0
@export var prefer_distance: float = 18.0

@onready var health = $Health
@onready var muzzle: Marker3D = $Muzzle

var _player: Node3D
var _fire_cd: float = 0.0
var _projectile_scene: PackedScene
var _alive: bool = true

func _ready() -> void:
	add_to_group("enemy")
	_projectile_scene = load("res://scenes/projectiles/projectile.tscn")
	health.died.connect(_on_died)
	_fire_cd = randf_range(0.2, fire_cooldown)
	call_deferred("_find_player")

func configure(type: String) -> void:
	enemy_type = type
	match type:
		"fighter":
			health.max_hull = 40.0
			health.max_shields = 20.0
			move_speed = 24.0
			projectile_damage = 8.0
			fire_cooldown = 1.0
			scale = Vector3.ONE
		"interceptor":
			health.max_hull = 35.0
			health.max_shields = 15.0
			move_speed = 34.0
			turn_rate = 2.6
			projectile_damage = 6.0
			fire_cooldown = 0.7
			prefer_distance = 22.0
			scale = Vector3(0.85, 0.85, 0.85)
		"heavy":
			health.max_hull = 120.0
			health.max_shields = 60.0
			move_speed = 14.0
			turn_rate = 1.1
			projectile_damage = 16.0
			fire_cooldown = 1.4
			fire_range = 45.0
			prefer_distance = 28.0
			scale = Vector3(1.6, 1.4, 1.8)
		"destroyer":
			health.max_hull = 400.0
			health.max_shields = 150.0
			move_speed = 9.0
			turn_rate = 0.7
			projectile_damage = 28.0
			fire_cooldown = 0.9
			fire_range = 55.0
			prefer_distance = 35.0
			scale = Vector3(3.0, 2.2, 4.0)
	health.restore_full()
	_recolor()

func _recolor() -> void:
	var mesh := get_node_or_null("Body") as MeshInstance3D
	if mesh == null:
		return
	var mat := StandardMaterial3D.new()
	mat.roughness = 0.55
	match enemy_type:
		"fighter":
			mat.albedo_color = Color(0.85, 0.25, 0.2)
		"interceptor":
			mat.albedo_color = Color(0.95, 0.55, 0.15)
		"heavy":
			mat.albedo_color = Color(0.55, 0.15, 0.45)
		"destroyer":
			mat.albedo_color = Color(0.4, 0.1, 0.1)
			mat.emission_enabled = true
			mat.emission = Color(0.5, 0.05, 0.05)
			mat.emission_energy_multiplier = 0.6
	mesh.material_override = mat

func _find_player() -> void:
	var nodes := get_tree().get_nodes_in_group("player")
	if nodes.size() > 0:
		_player = nodes[0]

func _physics_process(delta: float) -> void:
	if not _alive or GameState.phase != GameState.Phase.PLAYING:
		return
	if _player == null or not is_instance_valid(_player) or not _player.visible:
		_find_player()
		return

	var to_player := _player.global_position - global_position
	var dist := to_player.length()
	# Flatten chase into the yaw plane so yaw-only player aim can connect.
	var flat := Vector3(to_player.x, 0.0, to_player.z)
	var desired_dir := flat.normalized() if flat.length_squared() > 0.01 else to_player.normalized()

	# Face player (guard against near-vertical aim)
	if absf(desired_dir.dot(Vector3.UP)) < 0.98 and desired_dir.length_squared() > 0.001:
		var target_basis := Basis.looking_at(desired_dir, Vector3.UP)
		global_transform.basis = global_transform.basis.slerp(target_basis, clampf(turn_rate * delta, 0.0, 1.0))

	# Maintain preferred distance
	var approach := 1.0
	if dist < prefer_distance * 0.7:
		approach = -0.6
	elif dist < prefer_distance:
		approach = 0.15

	velocity = -global_transform.basis.z * move_speed * approach
	# Drift toward player's altitude so fights stay readable
	velocity.y += clampf((_player.global_position.y - global_position.y) * 2.5, -move_speed * 0.35, move_speed * 0.35)
	# Slight lateral weave
	velocity += global_transform.basis.x * sin(Time.get_ticks_msec() * 0.002 + float(get_instance_id() % 100)) * move_speed * 0.25
	move_and_slide()

	_fire_cd -= delta
	if _fire_cd <= 0.0 and dist <= fire_range and approach >= 0.0:
		_shoot()
		_fire_cd = fire_cooldown * randf_range(0.85, 1.15)

func _shoot() -> void:
	var p: Node3D = _projectile_scene.instantiate()
	get_tree().current_scene.add_child(p)
	p.global_transform = muzzle.global_transform
	var aim_point := _player.global_position
	if _player is CharacterBody3D:
		aim_point += (_player as CharacterBody3D).velocity * 0.25
	# Bias aim onto the flight plane so shots aren't pure vertical snipes
	aim_point.y = lerpf(muzzle.global_position.y, aim_point.y, 0.35)
	var aim := (aim_point - muzzle.global_position).normalized()
	if p.has_method("setup"):
		p.setup(aim, projectile_damage, false, velocity)

func take_hit(amount: float) -> void:
	if _alive:
		health.apply_damage(amount)

func _on_died() -> void:
	if not _alive:
		return
	_alive = false
	var pts: int = ScoreTable.points_for(enemy_type, GameState.sector)
	GameState.register_kill(enemy_type, pts)
	_spawn_explosion()
	queue_free()

func _spawn_explosion() -> void:
	var parent := get_tree().current_scene
	if parent == null:
		return
	var packed: PackedScene = load("res://scenes/effects/explosion.tscn")
	if packed == null:
		return
	var fx: Node3D = packed.instantiate()
	parent.add_child(fx)
	fx.global_position = global_position
	if enemy_type == "destroyer":
		fx.scale = Vector3(2.5, 2.5, 2.5)
