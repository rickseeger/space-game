extends RefCounted
class_name WaveDefinitions
## Declarative spawn tables. Progressive difficulty.

## Returns Array of Dictionaries: {type: String, count: int}
static func composition(sector: int, wave: int) -> Array:
	var s := clampi(sector, 1, 12)
	var w := clampi(wave, 1, 5)
	var result: Array = []

	# Base fighters always
	var fighters := 2 + w + int(s / 2)
	result.append({"type": "fighter", "count": fighters})

	if s >= 2 or w >= 2:
		var interceptors := maxi(0, w - 1 + int(s / 3))
		if interceptors > 0:
			result.append({"type": "interceptor", "count": interceptors})

	if s >= 3 and w >= 2:
		var heavies := 1 + int((s - 3) / 2)
		result.append({"type": "heavy", "count": heavies})

	if s >= 5 and w == 3:
		result.append({"type": "destroyer", "count": 1 + int((s - 5) / 3)})

	if s >= 7 and w >= 2:
		# Extra pressure late-game
		result.append({"type": "interceptor", "count": 2 + int(s / 4)})

	return result

static func total_enemies(sector: int, wave: int) -> int:
	var total := 0
	for entry in composition(sector, wave):
		total += int(entry["count"])
	return total

static func spawn_radius(sector: int) -> float:
	return 40.0 + sector * 2.0
