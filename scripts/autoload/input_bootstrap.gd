extends Node
## Rebuilds InputMap with proper keycode + physical_keycode (keycode=0-only maps fail on some Linux installs).

func _ready() -> void:
	_bind("move_forward", [KEY_W, KEY_UP])
	_bind("move_back", [KEY_S, KEY_DOWN])
	_bind("turn_left", [KEY_A, KEY_LEFT])
	_bind("turn_right", [KEY_D, KEY_RIGHT])
	_bind("strafe_left", [KEY_Q])
	_bind("strafe_right", [KEY_E])
	_bind("fire", [KEY_SPACE])
	_bind("boost", [KEY_SHIFT])
	_bind("hyperspace", [KEY_H])
	_bind("pause", [KEY_ESCAPE])
	# Keep Enter for menus, drop Space from ui_accept so it doesn't fight fire/start.
	if InputMap.has_action("ui_accept"):
		InputMap.action_erase_events("ui_accept")
		_add_key("ui_accept", KEY_ENTER)
		_add_key("ui_accept", KEY_KP_ENTER)

func _bind(action: String, keys: Array) -> void:
	if not InputMap.has_action(action):
		InputMap.add_action(action, 0.5)
	InputMap.action_erase_events(action)
	for k in keys:
		_add_key(action, k)

func _add_key(action: String, keycode: Key) -> void:
	var e := InputEventKey.new()
	e.keycode = keycode
	e.physical_keycode = keycode
	e.pressed = true
	InputMap.action_add_event(action, e)
