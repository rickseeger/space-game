extends Node3D

func _ready() -> void:
	var p := $Particles as GPUParticles3D
	if p:
		p.emitting = true
	get_tree().create_timer(1.8).timeout.connect(queue_free)
