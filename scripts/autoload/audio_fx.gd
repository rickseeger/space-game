extends Node
## Lightweight one-shot SFX player.

var _players: Array[AudioStreamPlayer] = []
const POOL := 8

func _ready() -> void:
	for i in POOL:
		var p := AudioStreamPlayer.new()
		p.bus = "Master"
		add_child(p)
		_players.append(p)
	EventBus.warning.connect(_on_warning)
	EventBus.hyperspace_started.connect(func(): play("res://assets/audio/hyperspace.wav", -4.0))
	EventBus.enemy_destroyed.connect(func(_t, _p): play("res://assets/audio/explosion.wav", -6.0))
	EventBus.player_died.connect(func(): play("res://assets/audio/explosion.wav", -2.0))

func play(path: String, db: float = 0.0) -> void:
	var stream := load(path)
	if stream == null:
		return
	for p in _players:
		if not p.playing:
			p.stream = stream
			p.volume_db = db
			p.play()
			return
	# Steal first
	_players[0].stream = stream
	_players[0].volume_db = db
	_players[0].play()

func play_laser() -> void:
	play("res://assets/audio/laser.wav", -8.0)

func play_hit() -> void:
	play("res://assets/audio/hit.wav", -6.0)

func _on_warning(_msg: String, severity: int) -> void:
	if severity >= 1:
		play("res://assets/audio/warning.wav", -10.0)
