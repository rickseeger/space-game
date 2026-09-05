extends Control

@onready var high_score_label: Label = $Center/VBox/HighScore
@onready var start_btn: Button = $Center/VBox/StartButton
@onready var quit_btn: Button = $Center/VBox/QuitButton

func _ready() -> void:
	GameState.phase = GameState.Phase.MENU
	high_score_label.text = "High Score: %d" % GameState.high_score
	start_btn.pressed.connect(_on_start)
	quit_btn.pressed.connect(_on_quit)
	# Prefer mouse/click; avoid Space being eaten by focused button forever.
	start_btn.focus_mode = Control.FOCUS_NONE
	quit_btn.focus_mode = Control.FOCUS_NONE
	get_viewport().gui_release_focus()

func _on_start() -> void:
	GameState.reset_run()
	get_tree().change_scene_to_file("res://scenes/game.tscn")

func _on_quit() -> void:
	get_tree().quit()

func _unhandled_input(event: InputEvent) -> void:
	if not event.is_pressed() or event.is_echo():
		return
	if event.is_action_pressed("fire"):
		_on_start()
		return
	if event is InputEventKey:
		var k := event as InputEventKey
		if k.keycode in [KEY_ENTER, KEY_KP_ENTER, KEY_SPACE] or k.physical_keycode in [KEY_ENTER, KEY_KP_ENTER, KEY_SPACE]:
			_on_start()
