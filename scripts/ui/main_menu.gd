extends Control

@onready var high_score_label: Label = $Center/VBox/HighScore
@onready var start_btn: Button = $Center/VBox/StartButton
@onready var quit_btn: Button = $Center/VBox/QuitButton

func _ready() -> void:
	GameState.phase = GameState.Phase.MENU
	high_score_label.text = "High Score: %d" % GameState.high_score
	start_btn.pressed.connect(_on_start)
	quit_btn.pressed.connect(_on_quit)
	start_btn.grab_focus()

func _on_start() -> void:
	GameState.reset_run()
	get_tree().change_scene_to_file("res://scenes/game.tscn")

func _on_quit() -> void:
	get_tree().quit()

func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("fire") or (event is InputEventKey and event.pressed and event.keycode == KEY_ENTER):
		_on_start()
