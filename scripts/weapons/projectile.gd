extends Area3D
class_name Projectile

var direction: Vector3 = Vector3.FORWARD
var speed: float = 90.0
var damage: float = 10.0
var from_player: bool = true
var lifetime: float = 2.5
var _age: float = 0.0

func setup(dir: Vector3, dmg: float, player_owned: bool, inherit_vel: Vector3 = Vector3.ZERO) -> void:
	direction = dir.normalized()
	damage = dmg
	from_player = player_owned
	# Inherit a portion of ship velocity so shots feel natural
	speed = 90.0 + inherit_vel.dot(direction) * 0.35
	if from_player:
		collision_layer = 4  # player_projectile
		collision_mask = 2   # enemy
	else:
		collision_layer = 8  # enemy_projectile
		collision_mask = 1   # player
	# Tint
	var mesh := get_node_or_null("Mesh") as MeshInstance3D
	if mesh and mesh.material_override:
		var mat := mesh.material_override.duplicate() as StandardMaterial3D
		mat.albedo_color = Color(0.3, 0.8, 1.0) if from_player else Color(1.0, 0.35, 0.2)
		mat.emission = mat.albedo_color
		mesh.material_override = mat

func _ready() -> void:
	body_entered.connect(_on_body)
	area_entered.connect(_on_area)

func _physics_process(delta: float) -> void:
	global_position += direction * speed * delta
	_age += delta
	if _age >= lifetime:
		queue_free()

func _on_body(body: Node) -> void:
	_hit(body)

func _on_area(area: Node) -> void:
	_hit(area)

func _hit(target: Node) -> void:
	if from_player and target.is_in_group("player"):
		return
	if not from_player and target.is_in_group("enemy"):
		return
	if target.has_method("take_hit"):
		target.take_hit(damage)
	elif target.get_parent() and target.get_parent().has_method("take_hit"):
		target.get_parent().take_hit(damage)
	_spawn_impact()
	queue_free()

func _spawn_impact() -> void:
	var parent := get_tree().current_scene
	if parent == null:
		return
	var packed: PackedScene = load("res://scenes/effects/impact.tscn")
	if packed == null:
		return
	var fx: Node3D = packed.instantiate()
	parent.add_child(fx)
	fx.global_position = global_position
