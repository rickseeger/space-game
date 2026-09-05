extends SceneTree
## Headless scene builder — run once to generate .tscn files.

func _init() -> void:
	print("Building scenes...")
	_build_projectile()
	_build_impact()
	_build_explosion()
	_build_player()
	_build_enemy()
	_build_main_menu()
	_build_game()
	print("All scenes built.")
	quit(0)

func _mat(color: Color, emission: bool = false, e_energy: float = 1.0) -> StandardMaterial3D:
	var m := StandardMaterial3D.new()
	m.albedo_color = color
	m.roughness = 0.5
	if emission:
		m.emission_enabled = true
		m.emission = color
		m.emission_energy_multiplier = e_energy
	return m

func _save(scene: PackedScene, path: String) -> void:
	var err := ResourceSaver.save(scene, path)
	if err != OK:
		push_error("Failed to save %s: %s" % [path, err])
	else:
		print("  saved ", path)

func _build_projectile() -> void:
	var root := Area3D.new()
	root.name = "Projectile"
	root.collision_layer = 4
	root.collision_mask = 2
	var script := load("res://scripts/weapons/projectile.gd")
	root.set_script(script)

	var mesh := MeshInstance3D.new()
	mesh.name = "Mesh"
	var sphere := SphereMesh.new()
	sphere.radius = 0.25
	sphere.height = 0.5
	mesh.mesh = sphere
	mesh.material_override = _mat(Color(0.3, 0.85, 1.0), true, 2.0)
	root.add_child(mesh)
	mesh.owner = root

	var col := CollisionShape3D.new()
	col.name = "Collision"
	var shape := SphereShape3D.new()
	shape.radius = 0.3
	col.shape = shape
	root.add_child(col)
	col.owner = root

	var packed := PackedScene.new()
	packed.pack(root)
	_save(packed, "res://scenes/projectiles/projectile.tscn")
	root.free()

func _build_impact() -> void:
	var root := Node3D.new()
	root.name = "Impact"
	root.set_script(load("res://scripts/effects/impact_fx.gd"))

	var p := GPUParticles3D.new()
	p.name = "Particles"
	p.amount = 16
	p.lifetime = 0.4
	p.one_shot = true
	p.explosiveness = 1.0
	p.emitting = false
	var mat := ParticleProcessMaterial.new()
	mat.direction = Vector3(0, 1, 0)
	mat.spread = 180.0
	mat.initial_velocity_min = 4.0
	mat.initial_velocity_max = 12.0
	mat.gravity = Vector3.ZERO
	mat.scale_min = 0.1
	mat.scale_max = 0.35
	mat.color = Color(1.0, 0.8, 0.3)
	p.process_material = mat
	var draw := SphereMesh.new()
	draw.radius = 0.15
	draw.height = 0.3
	p.draw_pass_1 = draw
	root.add_child(p)
	p.owner = root

	var packed := PackedScene.new()
	packed.pack(root)
	_save(packed, "res://scenes/effects/impact.tscn")
	root.free()

func _build_explosion() -> void:
	var root := Node3D.new()
	root.name = "Explosion"
	root.set_script(load("res://scripts/effects/explosion_fx.gd"))

	var p := GPUParticles3D.new()
	p.name = "Particles"
	p.amount = 40
	p.lifetime = 0.8
	p.one_shot = true
	p.explosiveness = 0.95
	p.emitting = false
	var mat := ParticleProcessMaterial.new()
	mat.direction = Vector3(0, 1, 0)
	mat.spread = 180.0
	mat.initial_velocity_min = 6.0
	mat.initial_velocity_max = 22.0
	mat.gravity = Vector3.ZERO
	mat.scale_min = 0.2
	mat.scale_max = 0.7
	mat.color = Color(1.0, 0.45, 0.1)
	p.process_material = mat
	var draw := SphereMesh.new()
	draw.radius = 0.2
	draw.height = 0.4
	p.draw_pass_1 = draw
	root.add_child(p)
	p.owner = root

	var packed := PackedScene.new()
	packed.pack(root)
	_save(packed, "res://scenes/effects/explosion.tscn")
	root.free()

func _build_player() -> void:
	var root := CharacterBody3D.new()
	root.name = "Player"
	root.collision_layer = 1
	root.collision_mask = 2 | 16
	root.set_script(load("res://scripts/player/player_ship.gd"))

	var body := MeshInstance3D.new()
	body.name = "Body"
	var prism := PrismMesh.new()
	prism.size = Vector3(1.4, 0.5, 2.2)
	body.mesh = prism
	body.material_override = _mat(Color(0.25, 0.55, 0.95), true, 0.4)
	body.rotation_degrees = Vector3(90, 0, 0)
	root.add_child(body)
	body.owner = root

	var wing_l := MeshInstance3D.new()
	wing_l.name = "WingL"
	var box := BoxMesh.new()
	box.size = Vector3(1.2, 0.08, 0.6)
	wing_l.mesh = box
	wing_l.position = Vector3(-0.9, 0, 0.2)
	wing_l.material_override = _mat(Color(0.2, 0.4, 0.75))
	root.add_child(wing_l)
	wing_l.owner = root

	var wing_r := MeshInstance3D.new()
	wing_r.name = "WingR"
	wing_r.mesh = box.duplicate()
	wing_r.position = Vector3(0.9, 0, 0.2)
	wing_r.material_override = _mat(Color(0.2, 0.4, 0.75))
	root.add_child(wing_r)
	wing_r.owner = root

	var engine := MeshInstance3D.new()
	engine.name = "EngineGlow"
	var eng := SphereMesh.new()
	eng.radius = 0.25
	eng.height = 0.5
	engine.mesh = eng
	engine.position = Vector3(0, 0, 1.1)
	engine.material_override = _mat(Color(0.3, 0.7, 1.0), true, 3.0)
	root.add_child(engine)
	engine.owner = root

	var col := CollisionShape3D.new()
	col.name = "Collision"
	var shape := BoxShape3D.new()
	shape.size = Vector3(1.6, 0.6, 2.2)
	col.shape = shape
	root.add_child(col)
	col.owner = root

	var health := Node.new()
	health.name = "Health"
	health.set_script(load("res://scripts/combat/health.gd"))
	root.add_child(health)
	health.owner = root

	var muzzle := Marker3D.new()
	muzzle.name = "Muzzle"
	muzzle.position = Vector3(0, 0, -1.4)
	root.add_child(muzzle)
	muzzle.owner = root

	var cam := Camera3D.new()
	cam.name = "Camera3D"
	cam.position = Vector3(0, 4.5, 10)
	cam.rotation_degrees = Vector3(-18, 0, 0)
	cam.current = true
	cam.fov = 70
	root.add_child(cam)
	cam.owner = root

	var trail := GPUParticles3D.new()
	trail.name = "Trail"
	trail.amount = 48
	trail.lifetime = 0.6
	trail.emitting = false
	trail.position = Vector3(0, 0, 1.2)
	var tmat := ParticleProcessMaterial.new()
	tmat.direction = Vector3(0, 0, 1)
	tmat.spread = 8.0
	tmat.initial_velocity_min = 2.0
	tmat.initial_velocity_max = 6.0
	tmat.gravity = Vector3.ZERO
	tmat.scale_min = 0.08
	tmat.scale_max = 0.2
	tmat.color = Color(0.4, 0.75, 1.0, 0.7)
	trail.process_material = tmat
	var tdraw := SphereMesh.new()
	tdraw.radius = 0.1
	tdraw.height = 0.2
	trail.draw_pass_1 = tdraw
	root.add_child(trail)
	trail.owner = root

	var packed := PackedScene.new()
	packed.pack(root)
	_save(packed, "res://scenes/player.tscn")
	root.free()

func _build_enemy() -> void:
	var root := CharacterBody3D.new()
	root.name = "Enemy"
	root.collision_layer = 2
	root.collision_mask = 1 | 4 | 16
	root.set_script(load("res://scripts/enemies/enemy_ship.gd"))

	var body := MeshInstance3D.new()
	body.name = "Body"
	var box := BoxMesh.new()
	box.size = Vector3(1.2, 0.45, 1.8)
	body.mesh = box
	body.material_override = _mat(Color(0.85, 0.25, 0.2))
	root.add_child(body)
	body.owner = root

	var nose := MeshInstance3D.new()
	nose.name = "Nose"
	var prism := PrismMesh.new()
	prism.size = Vector3(0.8, 0.35, 0.9)
	nose.mesh = prism
	nose.position = Vector3(0, 0, -1.1)
	nose.rotation_degrees = Vector3(90, 0, 0)
	nose.material_override = _mat(Color(0.7, 0.15, 0.15))
	root.add_child(nose)
	nose.owner = root

	var col := CollisionShape3D.new()
	col.name = "Collision"
	var shape := BoxShape3D.new()
	shape.size = Vector3(1.3, 0.55, 2.2)
	col.shape = shape
	root.add_child(col)
	col.owner = root

	var health := Node.new()
	health.name = "Health"
	health.set_script(load("res://scripts/combat/health.gd"))
	root.add_child(health)
	health.owner = root

	var muzzle := Marker3D.new()
	muzzle.name = "Muzzle"
	muzzle.position = Vector3(0, 0, -1.3)
	root.add_child(muzzle)
	muzzle.owner = root

	var packed := PackedScene.new()
	packed.pack(root)
	_save(packed, "res://scenes/enemies/enemy.tscn")
	root.free()

func _build_main_menu() -> void:
	var root := Control.new()
	root.name = "MainMenu"
	root.set_anchors_preset(Control.PRESET_FULL_RECT)
	root.set_script(load("res://scripts/ui/main_menu.gd"))

	var bg := ColorRect.new()
	bg.name = "BG"
	bg.set_anchors_preset(Control.PRESET_FULL_RECT)
	bg.color = Color(0.02, 0.03, 0.08)
	root.add_child(bg)
	bg.owner = root

	var center := CenterContainer.new()
	center.name = "Center"
	center.set_anchors_preset(Control.PRESET_FULL_RECT)
	root.add_child(center)
	center.owner = root

	var vbox := VBoxContainer.new()
	vbox.name = "VBox"
	vbox.add_theme_constant_override("separation", 16)
	center.add_child(vbox)
	vbox.owner = root

	var title := Label.new()
	title.name = "Title"
	title.text = "SPACE HAWK"
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title.add_theme_font_size_override("font_size", 64)
	title.add_theme_color_override("font_color", Color(0.45, 0.8, 1.0))
	vbox.add_child(title)
	title.owner = root

	var subtitle := Label.new()
	subtitle.name = "Subtitle"
	subtitle.text = "Keyboard space combat"
	subtitle.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	subtitle.add_theme_font_size_override("font_size", 18)
	subtitle.add_theme_color_override("font_color", Color(0.6, 0.7, 0.85))
	vbox.add_child(subtitle)
	subtitle.owner = root

	var hs := Label.new()
	hs.name = "HighScore"
	hs.text = "High Score: 0"
	hs.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	hs.add_theme_font_size_override("font_size", 20)
	vbox.add_child(hs)
	hs.owner = root

	var spacer := Control.new()
	spacer.custom_minimum_size = Vector2(0, 24)
	vbox.add_child(spacer)
	spacer.owner = root

	var start := Button.new()
	start.name = "StartButton"
	start.text = "  START MISSION  "
	start.custom_minimum_size = Vector2(280, 48)
	vbox.add_child(start)
	start.owner = root

	var quitb := Button.new()
	quitb.name = "QuitButton"
	quitb.text = "  QUIT  "
	quitb.custom_minimum_size = Vector2(280, 40)
	vbox.add_child(quitb)
	quitb.owner = root

	var help := Label.new()
	help.name = "Help"
	help.text = "WASD / Arrows  move   ·   Space fire   ·   Shift boost   ·   H hyperspace"
	help.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	help.add_theme_font_size_override("font_size", 14)
	help.add_theme_color_override("font_color", Color(0.5, 0.55, 0.65))
	vbox.add_child(help)
	help.owner = root

	var packed := PackedScene.new()
	packed.pack(root)
	_save(packed, "res://scenes/main_menu.tscn")
	root.free()

func _build_game() -> void:
	var root := Node3D.new()
	root.name = "Game"
	root.set_script(load("res://scripts/systems/game_world.gd"))

	# World environment
	var env_node := WorldEnvironment.new()
	env_node.name = "WorldEnvironment"
	var env := Environment.new()
	env.background_mode = Environment.BG_COLOR
	env.background_color = Color(0.01, 0.015, 0.04)
	env.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	env.ambient_light_color = Color(0.15, 0.18, 0.28)
	env.ambient_light_energy = 0.8
	env.glow_enabled = true
	env.glow_intensity = 0.6
	env.glow_bloom = 0.3
	env_node.environment = env
	root.add_child(env_node)
	env_node.owner = root

	var sun := DirectionalLight3D.new()
	sun.name = "Sun"
	sun.rotation_degrees = Vector3(-40, 30, 0)
	sun.light_energy = 1.2
	sun.shadow_enabled = false
	root.add_child(sun)
	sun.owner = root

	# Starfield particles
	var stars := GPUParticles3D.new()
	stars.name = "Starfield"
	stars.amount = 400
	stars.lifetime = 8.0
	stars.visibility_aabb = AABB(Vector3(-80, -80, -80), Vector3(160, 160, 160))
	var smat := ParticleProcessMaterial.new()
	smat.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_BOX
	smat.emission_box_extents = Vector3(70, 40, 70)
	smat.direction = Vector3(0, 0, 1)
	smat.spread = 0.0
	smat.initial_velocity_min = 0.0
	smat.initial_velocity_max = 0.0
	smat.gravity = Vector3.ZERO
	smat.scale_min = 0.05
	smat.scale_max = 0.2
	smat.color = Color(0.85, 0.9, 1.0)
	stars.process_material = smat
	var sdraw := SphereMesh.new()
	sdraw.radius = 0.08
	sdraw.height = 0.16
	stars.draw_pass_1 = sdraw
	stars.emitting = true
	root.add_child(stars)
	stars.owner = root

	# Speed lines
	var lines := GPUParticles3D.new()
	lines.name = "SpeedLines"
	lines.amount = 64
	lines.lifetime = 0.35
	lines.emitting = false
	lines.visibility_aabb = AABB(Vector3(-30, -30, -30), Vector3(60, 60, 60))
	var lmat := ParticleProcessMaterial.new()
	lmat.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_BOX
	lmat.emission_box_extents = Vector3(12, 8, 8)
	lmat.direction = Vector3(0, 0, 1)
	lmat.spread = 5.0
	lmat.initial_velocity_min = 40.0
	lmat.initial_velocity_max = 80.0
	lmat.gravity = Vector3.ZERO
	lmat.scale_min = 0.05
	lmat.scale_max = 0.12
	lmat.color = Color(0.6, 0.85, 1.0, 0.5)
	lines.process_material = lmat
	var ldraw := BoxMesh.new()
	ldraw.size = Vector3(0.05, 0.05, 1.2)
	lines.draw_pass_1 = ldraw
	root.add_child(lines)
	lines.owner = root

	# Player instance
	var player_scene: PackedScene = load("res://scenes/player.tscn")
	var player := player_scene.instantiate()
	player.name = "Player"
	root.add_child(player)
	player.owner = root
	_set_owners_recursive(player, root)

	# Enemies root
	var enemies := Node3D.new()
	enemies.name = "Enemies"
	root.add_child(enemies)
	enemies.owner = root

	# Wave controller
	var wc := Node.new()
	wc.name = "WaveController"
	wc.set_script(load("res://scripts/systems/wave_controller.gd"))
	wc.set("enemy_scene", load("res://scenes/enemies/enemy.tscn"))
	wc.set("spawn_root_path", NodePath("../Enemies"))
	root.add_child(wc)
	wc.owner = root

	# Hyperspace overlay (UI)
	var hs_layer := CanvasLayer.new()
	hs_layer.name = "HyperspaceLayer"
	hs_layer.layer = 10
	root.add_child(hs_layer)
	hs_layer.owner = root

	var overlay := ColorRect.new()
	overlay.name = "HyperspaceOverlay"
	overlay.set_anchors_preset(Control.PRESET_FULL_RECT)
	overlay.color = Color(0.4, 0.7, 1.0, 0.0)
	overlay.visible = false
	overlay.mouse_filter = Control.MOUSE_FILTER_IGNORE
	hs_layer.add_child(overlay)
	overlay.owner = root

	var hc := Node.new()
	hc.name = "HyperspaceController"
	hc.set_script(load("res://scripts/systems/hyperspace_controller.gd"))
	hc.set("overlay_path", NodePath("../HyperspaceLayer/HyperspaceOverlay"))
	hc.set("speed_lines_path", NodePath("../SpeedLines"))
	root.add_child(hc)
	hc.owner = root

	# HUD
	var hud := CanvasLayer.new()
	hud.name = "HUD"
	hud.set_script(load("res://scripts/ui/hud.gd"))
	root.add_child(hud)
	hud.owner = root

	var hroot := Control.new()
	hroot.name = "Root"
	hroot.set_anchors_preset(Control.PRESET_FULL_RECT)
	hroot.mouse_filter = Control.MOUSE_FILTER_IGNORE
	hud.add_child(hroot)
	hroot.owner = root

	var top := HBoxContainer.new()
	top.name = "Top"
	top.set_anchors_preset(Control.PRESET_TOP_WIDE)
	top.offset_left = 24
	top.offset_top = 16
	top.offset_right = -24
	top.offset_bottom = 48
	top.add_theme_constant_override("separation", 40)
	hroot.add_child(top)
	top.owner = root

	var score_l := Label.new()
	score_l.name = "ScoreLabel"
	score_l.text = "SCORE  0"
	score_l.add_theme_font_size_override("font_size", 22)
	top.add_child(score_l)
	score_l.owner = root

	var sector_l := Label.new()
	sector_l.name = "SectorLabel"
	sector_l.text = "SECTOR 1  ·  WAVE 0"
	sector_l.add_theme_font_size_override("font_size", 22)
	sector_l.add_theme_color_override("font_color", Color(0.6, 0.85, 1.0))
	top.add_child(sector_l)
	sector_l.owner = root

	var vbox := VBoxContainer.new()
	vbox.name = "VBox"
	vbox.set_anchors_preset(Control.PRESET_BOTTOM_LEFT)
	vbox.offset_left = 24
	vbox.offset_top = -120
	vbox.offset_right = 280
	vbox.offset_bottom = -24
	vbox.add_theme_constant_override("separation", 6)
	hroot.add_child(vbox)
	vbox.owner = root

	var hull_lbl := Label.new()
	hull_lbl.text = "HULL"
	hull_lbl.add_theme_font_size_override("font_size", 12)
	vbox.add_child(hull_lbl)
	hull_lbl.owner = root

	var hull := ProgressBar.new()
	hull.name = "HullBar"
	hull.custom_minimum_size = Vector2(240, 14)
	hull.max_value = 100
	hull.value = 100
	hull.show_percentage = false
	vbox.add_child(hull)
	hull.owner = root

	var sh_lbl := Label.new()
	sh_lbl.text = "SHIELDS"
	sh_lbl.add_theme_font_size_override("font_size", 12)
	vbox.add_child(sh_lbl)
	sh_lbl.owner = root

	var shields := ProgressBar.new()
	shields.name = "ShieldBar"
	shields.custom_minimum_size = Vector2(240, 14)
	shields.max_value = 60
	shields.value = 60
	shields.show_percentage = false
	vbox.add_child(shields)
	shields.owner = root

	var boost_lbl := Label.new()
	boost_lbl.text = "BOOST"
	boost_lbl.add_theme_font_size_override("font_size", 12)
	vbox.add_child(boost_lbl)
	boost_lbl.owner = root

	var boost := ProgressBar.new()
	boost.name = "BoostBar"
	boost.custom_minimum_size = Vector2(240, 10)
	boost.max_value = 100
	boost.value = 100
	boost.show_percentage = false
	vbox.add_child(boost)
	boost.owner = root

	var warn := Label.new()
	warn.name = "WarningLabel"
	warn.set_anchors_preset(Control.PRESET_CENTER_TOP)
	warn.offset_top = 80
	warn.offset_bottom = 120
	warn.offset_left = -300
	warn.offset_right = 300
	warn.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	warn.add_theme_font_size_override("font_size", 28)
	warn.text = ""
	hroot.add_child(warn)
	warn.owner = root

	var cross := Control.new()
	cross.name = "Crosshair"
	cross.set_anchors_preset(Control.PRESET_CENTER)
	cross.custom_minimum_size = Vector2(20, 20)
	hroot.add_child(cross)
	cross.owner = root

	# Game over
	var go := CanvasLayer.new()
	go.name = "GameOver"
	go.layer = 20
	go.set_script(load("res://scripts/ui/game_over.gd"))
	root.add_child(go)
	go.owner = root

	var panel := PanelContainer.new()
	panel.name = "Panel"
	panel.set_anchors_preset(Control.PRESET_CENTER)
	panel.offset_left = -180
	panel.offset_top = -140
	panel.offset_right = 180
	panel.offset_bottom = 140
	panel.visible = false
	go.add_child(panel)
	panel.owner = root

	var govbox := VBoxContainer.new()
	govbox.name = "VBox"
	govbox.add_theme_constant_override("separation", 16)
	panel.add_child(govbox)
	govbox.owner = root

	var gotitle := Label.new()
	gotitle.text = "SHIP DESTROYED"
	gotitle.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	gotitle.add_theme_font_size_override("font_size", 28)
	gotitle.add_theme_color_override("font_color", Color(1.0, 0.35, 0.3))
	govbox.add_child(gotitle)
	gotitle.owner = root

	var goscore := Label.new()
	goscore.name = "ScoreLabel"
	goscore.text = "FINAL SCORE\n0"
	goscore.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	goscore.add_theme_font_size_override("font_size", 22)
	govbox.add_child(goscore)
	goscore.owner = root

	var retry := Button.new()
	retry.name = "RetryButton"
	retry.text = "RETRY"
	govbox.add_child(retry)
	retry.owner = root

	var menu := Button.new()
	menu.name = "MenuButton"
	menu.text = "MAIN MENU"
	govbox.add_child(menu)
	menu.owner = root

	var packed := PackedScene.new()
	packed.pack(root)
	_save(packed, "res://scenes/game.tscn")
	root.free()

func _set_owners_recursive(node: Node, owner: Node) -> void:
	for c in node.get_children():
		c.owner = owner
		_set_owners_recursive(c, owner)
