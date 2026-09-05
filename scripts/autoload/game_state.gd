extends Node
## Central game state: score, sector, wave, phase machine.

enum Phase { MENU, PLAYING, HYPERSPACE, DEAD, PAUSED }

var phase: Phase = Phase.MENU
var score: int = 0
var sector: int = 1
var wave: int = 0
var enemies_alive: int = 0
var high_score: int = 0
var hyperspace_ready: bool = false
var session_kills: int = 0

const MAX_SECTORS := 8
const WAVES_PER_SECTOR := 3

func _ready() -> void:
	high_score = 0

func reset_run() -> void:
	score = 0
	sector = 1
	wave = 0
	enemies_alive = 0
	hyperspace_ready = false
	session_kills = 0
	phase = Phase.PLAYING

func add_score(points: int) -> void:
	score += points
	if score > high_score:
		high_score = score
	EventBus.score_changed.emit(score)

func register_kill(enemy_type: String, points: int) -> void:
	session_kills += 1
	enemies_alive = maxi(0, enemies_alive - 1)
	add_score(points)
	EventBus.enemy_destroyed.emit(enemy_type, points)
	if enemies_alive <= 0 and phase == Phase.PLAYING:
		_on_wave_cleared()

func begin_wave() -> void:
	wave += 1
	hyperspace_ready = false
	EventBus.wave_started.emit(sector, wave)

func _on_wave_cleared() -> void:
	EventBus.wave_cleared.emit(sector, wave)
	if wave >= WAVES_PER_SECTOR:
		hyperspace_ready = true
		EventBus.warning.emit("HYPERSPACE READY — Press H", 1)
	else:
		# Next wave after brief delay handled by WaveController
		pass

func can_hyperspace() -> bool:
	return hyperspace_ready and phase == Phase.PLAYING and enemies_alive <= 0

func start_hyperspace() -> bool:
	if not can_hyperspace():
		return false
	phase = Phase.HYPERSPACE
	hyperspace_ready = false
	EventBus.hyperspace_started.emit()
	return true

func finish_hyperspace() -> void:
	sector += 1
	wave = 0
	phase = Phase.PLAYING
	EventBus.hyperspace_ended.emit(sector)
	if sector > MAX_SECTORS:
		EventBus.warning.emit("ALL SECTORS CLEARED — CONTINUE!", 0)

func set_phase(p: Phase) -> void:
	phase = p

func mark_player_dead() -> void:
	phase = Phase.DEAD
	EventBus.player_died.emit()
	EventBus.game_over.emit(score)
