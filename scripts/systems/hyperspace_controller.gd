extends Node
class_name HyperspaceController
## Handles hyperspace jump VFX timing and sector transition.

const ScoreTable = preload("res://scripts/combat/score_table.gd")

@export var overlay_path: NodePath
@export var speed_lines_path: NodePath

var _overlay: CanvasItem
var _speed_lines: GPUParticles3D
var _timer: float = -1.0
var _active: bool = false

const JUMP_DURATION := 3.2

func _ready() -> void:
	if overlay_path:
		_overlay = get_node_or_null(overlay_path)
	if speed_lines_path:
		_speed_lines = get_node_or_null(speed_lines_path)
	if _overlay:
		_overlay.visible = false
	if _speed_lines:
		_speed_lines.emitting = false
	set_process_unhandled_input(true)

func _unhandled_input(event: InputEvent) -> void:
	if GameState.phase != GameState.Phase.PLAYING:
		return
	if event.is_action_pressed("hyperspace"):
		try_jump()
		return
	if event is InputEventKey and event.pressed and not event.echo:
		if event.keycode == KEY_H or event.physical_keycode == KEY_H:
			try_jump()

func _process(delta: float) -> void:
	if _active:
		_timer -= delta
		if _overlay and _overlay is ColorRect:
			var t := 1.0 - (_timer / JUMP_DURATION)
			(_overlay as ColorRect).color = Color(0.4, 0.7, 1.0, 0.15 + sin(t * PI) * 0.45)
		if _timer <= 0.0:
			_finish()

func try_jump() -> bool:
	if not GameState.can_hyperspace():
		if GameState.enemies_alive > 0:
			EventBus.warning.emit("CANNOT JUMP — HOSTILES PRESENT", 1)
		elif not GameState.hyperspace_ready:
			EventBus.warning.emit("HYPERDRIVE CHARGING", 0)
		return false
	if not GameState.start_hyperspace():
		return false
	_active = true
	_timer = JUMP_DURATION
	if _overlay:
		_overlay.visible = true
	if _speed_lines:
		_speed_lines.emitting = true
	EventBus.warning.emit("ENTERING HYPERSPACE", 0)
	return true

func _finish() -> void:
	_active = false
	if _overlay:
		_overlay.visible = false
	if _speed_lines:
		_speed_lines.emitting = false
	var bonus: int = ScoreTable.hyperspace_bonus(GameState.sector)
	GameState.add_score(bonus)
	GameState.finish_hyperspace()
	EventBus.warning.emit("SECTOR %d — +%d" % [GameState.sector, bonus], 0)

## Test helper: force jump readiness
func force_ready_for_tests() -> void:
	GameState.hyperspace_ready = true
	GameState.enemies_alive = 0
	GameState.phase = GameState.Phase.PLAYING
