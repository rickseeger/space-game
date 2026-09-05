extends Node
## Global signal hub for loosely coupled systems.

signal score_changed(new_score: int)
signal hull_changed(current: float, maximum: float)
signal shields_changed(current: float, maximum: float)
signal warning(message: String, severity: int)  # 0=info 1=warn 2=critical
signal enemy_destroyed(enemy_type: String, points: int)
signal wave_started(sector: int, wave: int)
signal wave_cleared(sector: int, wave: int)
signal hyperspace_started()
signal hyperspace_ended(new_sector: int)
signal player_died()
signal game_over(final_score: int)
signal boost_changed(energy: float, maximum: float)
