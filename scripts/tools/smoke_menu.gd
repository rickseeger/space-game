extends SceneTree
func _init() -> void:
	call_deferred("_go")
func _go() -> void:
	var err := change_scene_to_file("res://scenes/main_menu.tscn")
	if err != OK:
		push_error("menu load fail %s" % err)
		quit(1)
		return
	print("SMOKE_MENU: OK")
	quit(0)
