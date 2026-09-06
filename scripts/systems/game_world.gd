extends Node3D
## Root gameplay scene controller.

@onready var wave_controller = $WaveController
@onready var hyperspace = $HyperspaceController
@onready var player = $Player
@onready var camera: Camera3D = $Player/Camera3D
@onready var starfield: GPUParticles3D = $Starfield
@onready var speed_lines: GPUParticles3D = $SpeedLines

func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	GameState.phase = GameState.Phase.PLAYING
	# Don't let HUD/menu focus swallow keyboard (esp. Space).
	get_viewport().gui_release_focus()
	_disable_hud_focus()
	EventBus.hull_changed.emit(player.health.hull, player.health.max_hull)
	EventBus.shields_changed.emit(player.health.shields, player.health.max_shields)
	wave_controller.start_sector()
	set_process_unhandled_input(true)

func _unhandled_input(event: InputEvent) -> void:
	var pause_hit := event.is_action_pressed("pause")
	if event is InputEventKey and event.pressed and not event.echo:
		if event.keycode == KEY_ESCAPE or event.physical_keycode == KEY_ESCAPE:
			pause_hit = true
	if pause_hit:
		if GameState.phase == GameState.Phase.PLAYING:
			GameState.phase = GameState.Phase.PAUSED
			get_tree().paused = true
			EventBus.warning.emit("PAUSED — Esc to resume", 0)
		elif GameState.phase == GameState.Phase.PAUSED:
			get_tree().paused = false
			GameState.phase = GameState.Phase.PLAYING
			EventBus.warning.emit("", 0)

func _process(_delta: float) -> void:
	if player and is_instance_valid(player) and player.visible:
		var sp: float = player.velocity.length()
		if starfield:
			starfield.global_position = player.global_position
		if speed_lines:
			speed_lines.emitting = sp > 35.0 or GameState.phase == GameState.Phase.HYPERSPACE
			speed_lines.global_position = player.global_position

func _disable_hud_focus() -> void:
	var hud := get_node_or_null("HUD")
	if hud == null:
		return
	for n in hud.find_children("*", "Control", true, false):
		(n as Control).focus_mode = Control.FOCUS_NONE
		(n as Control).mouse_filter = Control.MOUSE_FILTER_IGNORE
