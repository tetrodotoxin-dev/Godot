# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

extends SceneTree

func _initialize() -> void:
	_run.call_deferred()

func _run() -> void:
	# The modules publish the same Render declaration. Configuration names its
	# two generated Node classes. Neither backend knows those names or Godot.
	var cpu := TtxCpuRender.new()
	var cuda := TtxCudaRender.new()
	assert(cpu is Node and cuda is Node)
	root.add_child(cpu)
	root.add_child(cuda)

	var source := PackedByteArray([10, 20, 30, 255, 40, 50, 60, 127])
	var expected := PackedByteArray([245, 235, 225, 255, 215, 205, 195, 127])
	for renderer in [cpu, cuda]:
		assert(not renderer.invert())
		assert(renderer.upload(2, 1, source))
		assert(renderer.get_width() == 2 and renderer.get_height() == 1)
		assert(renderer.invert())
		assert(renderer.pixels() == expected)
		assert(not renderer.upload(-1, 1, source))
		assert(not renderer.get_error().is_empty())
		assert(renderer.pixels() == expected)

	# Concrete syntax additionally exercises typed ptrcall for a buffer input
	# and a buffer result. The loop above uses dynamic method dispatch.
	assert(cpu.upload(2, 1, source))
	assert(cpu.pixels() == source)
	assert(cuda.pixels() == expected)

	var packed := PackedScene.new()
	assert(packed.pack(cpu) == OK)
	var reconstructed := packed.instantiate() as TtxCpuRender
	assert(reconstructed != null and reconstructed.get_width() == 0)
	reconstructed.free()
	cpu.free()
	cuda.free()
	print("PASS generated Render Nodes: CPU/CUDA, typed buffers, instances and scenes")
	quit()
