extends Node3D
## Root gameplay scene controller.

@onready var wave_controller: WaveController = $WaveController
@onready var hyperspace: HyperspaceController = $HyperspaceController
@onready var player: PlayerShip = $Player
@onready var camera: Camera3D = $Player/Camera3D
@onready var starfield: GPUParticles3D = $Starfield
@onready var speed_lines: GPUParticles3D = $SpeedLines

func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	GameState.phase = GameState.Phase.PLAYING
	EventBus.hull_changed.emit(player.health.hull, player.health.max_hull)
	EventBus.shields_changed.emit(player.health.shields, player.health.max_shields)
	wave_controller.start_sector()
	# Pause
	set_process_unhandled_input(true)

func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("pause"):
		if GameState.phase == GameState.Phase.PLAYING:
			GameState.phase = GameState.Phase.PAUSED
			get_tree().paused = true
			EventBus.warning.emit("PAUSED — Esc to resume", 0)
		elif GameState.phase == GameState.Phase.PAUSED:
			get_tree().paused = false
			GameState.phase = GameState.Phase.PLAYING

func _process(_delta: float) -> void:
	if player and is_instance_valid(player) and player.visible:
		var sp := player.velocity.length()
		if speed_lines:
			speed_lines.emitting = sp > 35.0 or GameState.phase == GameState.Phase.HYPERSPACE
			speed_lines.global_position = player.global_position
