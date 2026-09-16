# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

extends SceneTree

var failed := false

func _initialize() -> void:
	_run.call_deferred()

func _expect(condition: bool, message: String) -> void:
	if not condition:
		failed = true
		push_error(message)

func _run() -> void:
	# The C module supplies a real engine Node. Adding it to the tree must
	# deliver engine notifications to the foreign state, and a PackedScene
	# must reconstruct that same published class through its registered factory.
	var first := TtxCounter.new()
	_expect(first is Node, "Publication is not an engine Node")
	_expect(first.get_entries() == 0, "Node entered before attachment")
	_expect(first.get_live_instances() == 1, "Unexpected initial instance count")
	root.add_child(first)
	_expect(first.get_entries() == 1, "Native tree entry did not reach C")
	_expect(first.get_ready_count() == 1, "Native ready did not reach C")
	_expect(first.advance(40) == 40, "Typed C ptrcall failed")
	_expect(first.call("advance", 2) == 42, "Variant C call failed")

	var disk_scene := load("res://counter.tscn") as PackedScene
	var disk_node := disk_scene.instantiate() as TtxCounter
	_expect(disk_node != null, "Authored scene did not resolve the provider class")
	disk_node.free()

	var scene := PackedScene.new()
	_expect(scene.pack(first) == OK, "Node could not be packed")
	var second := scene.instantiate() as TtxCounter
	_expect(second != null, "PackedScene lost the published Node type")
	_expect(second.get_live_instances() == 2, "Scene did not construct independent state")
	_expect(second.advance(3) == 3, "Instance state is shared")
	root.add_child(second)
	_expect(second.get_entries() == 1 and second.get_ready_count() == 1, "Restored node missed lifecycle")
	second.free()
	_expect(first.get_live_instances() == 1, "Scene instance did not release C state")
	root.remove_child(first)
	root.add_child(first)
	_expect(first.get_entries() == 2 and first.get_ready_count() == 1, "Reentry changed Godot lifecycle")
	first.free()

	# TtxSampler came from a different module. Both invocation routes use the
	# same ordinary C++ member functions, including temporary String conversion.
	var sampler := TtxSampler.new()
	_expect(sampler.call("configure", "cpu"), "Variant configure failed")
	_expect(sampler.call("count", 0, 0, 17) == 15, "Variant sample answer differs")
	_expect(sampler.call("count", 0.0, 0.0, 17.0) == 15, "Godot numeric conversion differs")
	_expect(sampler.get_error().is_empty(), "String result differs")
	_expect(not sampler.call("configure", "missing_provider"), "Missing module accepted")
	_expect(not sampler.call("get_error").is_empty(), "Variant error string disappeared")
	_expect(sampler.count(0, 0, 17) == 15, "Failed replacement lost its old function")
	print("PASS published classes: native Node, scene recreation, C/C++ calls and lifetimes" if not failed else "FAIL published classes")
	quit(1 if failed else 0)
