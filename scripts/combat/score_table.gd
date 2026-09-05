extends RefCounted
class_name ScoreTable
## Point values and multipliers for scoring.

const BASE := {
	"fighter": 100,
	"interceptor": 175,
	"heavy": 350,
	"destroyer": 800,
}

static func points_for(enemy_type: String, sector: int = 1) -> int:
	var base: int = BASE.get(enemy_type, 50)
	var sector_mult := 1.0 + (sector - 1) * 0.15
	return int(round(base * sector_mult))

static func wave_clear_bonus(sector: int, wave: int) -> int:
	return 250 * sector + 100 * wave

static func hyperspace_bonus(sector: int) -> int:
	return 500 * sector
