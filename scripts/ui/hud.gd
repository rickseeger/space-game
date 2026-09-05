extends CanvasLayer
class_name CombatHUD

@onready var hull_bar: ProgressBar = $Root/VBox/HullBar
@onready var shield_bar: ProgressBar = $Root/VBox/ShieldBar
@onready var boost_bar: ProgressBar = $Root/VBox/BoostBar
@onready var score_label: Label = $Root/Top/ScoreLabel
@onready var sector_label: Label = $Root/Top/SectorLabel
@onready var warning_label: Label = $Root/WarningLabel
@onready var crosshair: Control = $Root/Crosshair

var _warn_timer: float = 0.0

func _ready() -> void:
	EventBus.hull_changed.connect(_on_hull)
	EventBus.shields_changed.connect(_on_shields)
	EventBus.boost_changed.connect(_on_boost)
	EventBus.score_changed.connect(_on_score)
	EventBus.warning.connect(_on_warning)
	EventBus.wave_started.connect(_on_wave)
	EventBus.hyperspace_ended.connect(_on_sector)
	warning_label.text = ""
	_on_score(GameState.score)

func _process(delta: float) -> void:
	if _warn_timer > 0.0:
		_warn_timer -= delta
		if _warn_timer <= 0.0:
			warning_label.text = ""

func _on_hull(cur: float, mx: float) -> void:
	hull_bar.max_value = mx
	hull_bar.value = cur
	hull_bar.modulate = Color(1, 0.3, 0.3) if cur / mx < 0.3 else Color.WHITE

func _on_shields(cur: float, mx: float) -> void:
	shield_bar.max_value = mx
	shield_bar.value = cur

func _on_boost(cur: float, mx: float) -> void:
	boost_bar.max_value = mx
	boost_bar.value = cur

func _on_score(s: int) -> void:
	score_label.text = "SCORE  %d" % s

func _on_wave(sector: int, wave: int) -> void:
	sector_label.text = "SECTOR %d  ·  WAVE %d" % [sector, wave]

func _on_sector(sector: int) -> void:
	sector_label.text = "SECTOR %d" % sector

func _on_warning(message: String, severity: int) -> void:
	warning_label.text = message
	match severity:
		2:
			warning_label.modulate = Color(1.0, 0.2, 0.2)
			_warn_timer = 2.5
		1:
			warning_label.modulate = Color(1.0, 0.85, 0.2)
			_warn_timer = 2.0
		_:
			warning_label.modulate = Color(0.5, 0.85, 1.0)
			_warn_timer = 1.8
