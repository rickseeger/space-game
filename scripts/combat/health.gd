extends Node
class_name Health
## Shields + hull. Shields absorb first and regenerate after a delay.

signal died
signal damaged(amount: float, remaining_hull: float, remaining_shields: float)
signal shields_depleted

@export var max_hull: float = 100.0
@export var max_shields: float = 50.0
@export var shield_regen_rate: float = 8.0
@export var shield_regen_delay: float = 3.0

var hull: float
var shields: float
var _regen_timer: float = 0.0
var invulnerable: bool = false

func _ready() -> void:
	hull = max_hull
	shields = max_shields

func _process(delta: float) -> void:
	if hull <= 0.0:
		return
	if _regen_timer > 0.0:
		_regen_timer -= delta
	elif shields < max_shields:
		shields = minf(max_shields, shields + shield_regen_rate * delta)
		damaged.emit(0.0, hull, shields)

func apply_damage(amount: float) -> float:
	if invulnerable or amount <= 0.0 or hull <= 0.0:
		return 0.0
	var remaining := amount
	if shields > 0.0:
		var absorbed := minf(shields, remaining)
		shields -= absorbed
		remaining -= absorbed
		if shields <= 0.0:
			shields = 0.0
			shields_depleted.emit()
	if remaining > 0.0:
		hull = maxf(0.0, hull - remaining)
	_regen_timer = shield_regen_delay
	damaged.emit(amount, hull, shields)
	if hull <= 0.0:
		died.emit()
	return amount

func heal_hull(amount: float) -> void:
	hull = minf(max_hull, hull + amount)
	damaged.emit(0.0, hull, shields)

func restore_full() -> void:
	hull = max_hull
	shields = max_shields
	_regen_timer = 0.0
	damaged.emit(0.0, hull, shields)

func is_alive() -> bool:
	return hull > 0.0

func hull_ratio() -> float:
	return 0.0 if max_hull <= 0.0 else hull / max_hull

func shield_ratio() -> float:
	return 0.0 if max_shields <= 0.0 else shields / max_shields
