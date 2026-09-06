extends CharacterBody3D
class_name PlayerShip
## Keyboard-driven fighter with thrust, boost, shields, and cannons.

const THRUST := 28.0
const TURN_SPEED := 2.4
const STRAFE := 14.0
const MAX_SPEED := 55.0
const BOOST_MULT := 1.85
const DRAG := 1.8
const BOOST_MAX := 100.0
const BOOST_DRAIN := 35.0
const BOOST_REGEN := 18.0
const FIRE_COOLDOWN := 0.18
const PROJECTILE_DAMAGE := 18.0

@onready var health = $Health
@onready var muzzle: Marker3D = $Muzzle
@onready var engine: MeshInstance3D = $EngineGlow
@onready var trail: GPUParticles3D = $Trail

var boost_energy: float = BOOST_MAX
var _fire_cd: float = 0.0
var _projectile_scene: PackedScene
var yaw: float = 0.0

func _ready() -> void:
	_projectile_scene = load("res://scenes/projectiles/projectile.tscn")
	add_to_group("player")
	health.max_hull = 100.0
	health.max_shields = 60.0
	health.restore_full()
	health.damaged.connect(_on_damaged)
	health.died.connect(_on_died)
	health.shields_depleted.connect(_on_shields_down)

func _physics_process(delta: float) -> void:
	if GameState.phase != GameState.Phase.PLAYING and GameState.phase != GameState.Phase.HYPERSPACE:
		return
	if GameState.phase == GameState.Phase.HYPERSPACE:
		# Lock controls during hyperspace — scripted forward surge
		velocity = -global_transform.basis.z * MAX_SPEED * 2.5
		move_and_slide()
		return

	_handle_turning(delta)
	_handle_thrust(delta)
	_handle_weapons(delta)
	move_and_slide()
	_update_vfx()

func _down(action: String, keys: Array) -> bool:
	if Input.is_action_pressed(action):
		return true
	for k in keys:
		if Input.is_physical_key_pressed(k) or Input.is_key_pressed(k):
			return true
	return false

func _handle_turning(delta: float) -> void:
	var turn := 0.0
	if _down("turn_left", [KEY_A, KEY_LEFT]):
		turn += 1.0
	if _down("turn_right", [KEY_D, KEY_RIGHT]):
		turn -= 1.0
	yaw += turn * TURN_SPEED * delta
	rotation.y = yaw

func _handle_thrust(delta: float) -> void:
	var forward := 0.0
	var strafe := 0.0
	if _down("move_forward", [KEY_W, KEY_UP]):
		forward += 1.0
	if _down("move_back", [KEY_S, KEY_DOWN]):
		forward -= 0.45
	if _down("strafe_left", [KEY_Q]):
		strafe -= 1.0
	if _down("strafe_right", [KEY_E]):
		strafe += 1.0

	var boosting := _down("boost", [KEY_SHIFT]) and boost_energy > 0.0 and forward > 0.0
	if boosting:
		boost_energy = maxf(0.0, boost_energy - BOOST_DRAIN * delta)
	else:
		boost_energy = minf(BOOST_MAX, boost_energy + BOOST_REGEN * delta)
	EventBus.boost_changed.emit(boost_energy, BOOST_MAX)

	var thrust_mult := BOOST_MULT if boosting else 1.0
	var wish := (-transform.basis.z * forward * THRUST * thrust_mult) + (transform.basis.x * strafe * STRAFE)
	velocity += wish * delta
	# Drag
	velocity = velocity.lerp(Vector3.ZERO, 1.0 - exp(-DRAG * delta))
	var max_sp := MAX_SPEED * (BOOST_MULT if boosting else 1.0)
	if velocity.length() > max_sp:
		velocity = velocity.normalized() * max_sp

func _handle_weapons(delta: float) -> void:
	_fire_cd = maxf(0.0, _fire_cd - delta)
	if _down("fire", [KEY_SPACE]) and _fire_cd <= 0.0:
		_fire()
		_fire_cd = FIRE_COOLDOWN

func _fire() -> void:
	var p: Node3D = _projectile_scene.instantiate()
	get_tree().current_scene.add_child(p)
	p.global_transform = muzzle.global_transform
	if p.has_method("setup"):
		p.setup(-global_transform.basis.z, PROJECTILE_DAMAGE, true, velocity)
	AudioFx.play_laser()

func _update_vfx() -> void:
	var speed_ratio := clampf(velocity.length() / MAX_SPEED, 0.0, 1.5)
	if engine:
		engine.scale = Vector3.ONE * (0.6 + speed_ratio * 0.8)
	if trail:
		trail.emitting = speed_ratio > 0.15

func _on_damaged(amount: float, h: float, s: float) -> void:
	EventBus.hull_changed.emit(h, health.max_hull)
	EventBus.shields_changed.emit(s, health.max_shields)
	if amount > 0.0:
		AudioFx.play_hit()
		if h / health.max_hull < 0.3:
			EventBus.warning.emit("HULL CRITICAL", 2)
		elif s <= 0.0:
			EventBus.warning.emit("SHIELDS DOWN", 1)

func _on_shields_down() -> void:
	EventBus.warning.emit("SHIELDS DEPLETED", 1)

func _on_died() -> void:
	EventBus.warning.emit("SHIP DESTROYED", 2)
	GameState.mark_player_dead()
	visible = false
	set_physics_process(false)

func take_hit(amount: float) -> void:
	health.apply_damage(amount)
