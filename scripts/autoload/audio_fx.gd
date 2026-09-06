extends Node
## Procedural one-shot SFX — no imported WAV files required.

var _players: Array[AudioStreamPlayer] = []
const POOL := 8

var _laser: AudioStreamWAV
var _hit: AudioStreamWAV
var _explosion: AudioStreamWAV
var _warning: AudioStreamWAV
var _hyperspace: AudioStreamWAV

func _ready() -> void:
	_laser = _tone_blip(880.0, 0.06, 0.25, true)
	_hit = _noise_burst(0.08, 0.35, 1800.0)
	_explosion = _noise_burst(0.35, 0.55, 600.0)
	_warning = _tone_blip(420.0, 0.12, 0.3, false)
	_hyperspace = _sweep(200.0, 1200.0, 0.45, 0.28)
	for i in POOL:
		var p := AudioStreamPlayer.new()
		p.bus = "Master"
		add_child(p)
		_players.append(p)
	EventBus.warning.connect(_on_warning)
	EventBus.hyperspace_started.connect(func(): _play_stream(_hyperspace, -4.0))
	EventBus.enemy_destroyed.connect(func(_t, _p): _play_stream(_explosion, -6.0))
	EventBus.player_died.connect(func(): _play_stream(_explosion, -2.0))

func _play_stream(stream: AudioStream, db: float = 0.0) -> void:
	if stream == null:
		return
	for p in _players:
		if not p.playing:
			p.stream = stream
			p.volume_db = db
			p.play()
			return
	_players[0].stream = stream
	_players[0].volume_db = db
	_players[0].play()

func play_laser() -> void:
	_play_stream(_laser, -8.0)

func play_hit() -> void:
	_play_stream(_hit, -6.0)

func _on_warning(_msg: String, severity: int) -> void:
	if severity >= 1:
		_play_stream(_warning, -10.0)

func _tone_blip(freq: float, dur: float, amp: float, fall: bool) -> AudioStreamWAV:
	var rate := 22050
	var n := int(dur * rate)
	var data := PackedByteArray()
	data.resize(n * 2)
	for i in n:
		var t := float(i) / float(rate)
		var env := 1.0 - (float(i) / float(n)) if fall else 1.0
		var s := sin(TAU * freq * t) * amp * env
		var v := int(clampf(s, -1.0, 1.0) * 32767.0)
		data[i * 2] = v & 0xFF
		data[i * 2 + 1] = (v >> 8) & 0xFF
	return _wav_from_pcm(data, rate)

func _noise_burst(dur: float, amp: float, lowpass_hz: float) -> AudioStreamWAV:
	var rate := 22050
	var n := int(dur * rate)
	var data := PackedByteArray()
	data.resize(n * 2)
	var prev := 0.0
	var alpha := clampf(lowpass_hz / 8000.0, 0.05, 0.95)
	for i in n:
		var env := 1.0 - (float(i) / float(n))
		env *= env
		var raw := (randf() * 2.0 - 1.0) * amp
		prev = prev + alpha * (raw - prev)
		var s := prev * env
		var v := int(clampf(s, -1.0, 1.0) * 32767.0)
		data[i * 2] = v & 0xFF
		data[i * 2 + 1] = (v >> 8) & 0xFF
	return _wav_from_pcm(data, rate)

func _sweep(f0: float, f1: float, dur: float, amp: float) -> AudioStreamWAV:
	var rate := 22050
	var n := int(dur * rate)
	var data := PackedByteArray()
	data.resize(n * 2)
	var phase := 0.0
	for i in n:
		var u := float(i) / float(n)
		var freq := lerpf(f0, f1, u)
		phase += TAU * freq / float(rate)
		var env := sin(PI * u)
		var s := sin(phase) * amp * env
		var v := int(clampf(s, -1.0, 1.0) * 32767.0)
		data[i * 2] = v & 0xFF
		data[i * 2 + 1] = (v >> 8) & 0xFF
	return _wav_from_pcm(data, rate)

func _wav_from_pcm(pcm16: PackedByteArray, rate: int) -> AudioStreamWAV:
	var stream := AudioStreamWAV.new()
	stream.format = AudioStreamWAV.FORMAT_16_BITS
	stream.mix_rate = rate
	stream.stereo = false
	stream.data = pcm16
	return stream
