extends Node3D

func _ready() -> void:
	var p := $Particles as GPUParticles3D
	if p:
		p.emitting = true
		p.finished.connect(queue_free)
	get_tree().create_timer(1.2).timeout.connect(queue_free)
