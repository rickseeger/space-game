extends CanvasLayer

@onready var panel: Control = $Panel
@onready var score_label: Label = $Panel/VBox/ScoreLabel
@onready var retry_btn: Button = $Panel/VBox/RetryButton
@onready var menu_btn: Button = $Panel/VBox/MenuButton

func _ready() -> void:
	panel.visible = false
	EventBus.game_over.connect(_on_game_over)
	retry_btn.pressed.connect(_on_retry)
	menu_btn.pressed.connect(_on_menu)

func _on_game_over(final_score: int) -> void:
	panel.visible = true
	score_label.text = "FINAL SCORE\n%d" % final_score
	retry_btn.grab_focus()

func _on_retry() -> void:
	GameState.reset_run()
	get_tree().reload_current_scene()

func _on_menu() -> void:
	GameState.phase = GameState.Phase.MENU
	get_tree().change_scene_to_file("res://scenes/main_menu.tscn")
