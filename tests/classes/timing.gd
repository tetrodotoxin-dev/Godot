# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

extends SceneTree

func _initialize() -> void:
	_run.call_deferred()

func _run() -> void:
	# Class preparation has already ended. These loops include native Godot
	# construction, provider state, initial callable fulfillment and destruction.
	# They deliberately exclude backend setup and the later retained call loop.
	var iterations := 10000
	var started := Time.get_ticks_usec()
	for index in iterations:
		var node := TtxCounter.new()
		node.free()
	print("FACTORY ", JSON.stringify({"class": "TtxCounter", "iterations": iterations,
		"create_release_us": float(Time.get_ticks_usec() - started) / iterations}))

	started = Time.get_ticks_usec()
	for index in iterations:
		var sampler := TtxSampler.new()
		sampler = null
	print("FACTORY ", JSON.stringify({"class": "TtxSampler", "iterations": iterations,
		"create_release_us": float(Time.get_ticks_usec() - started) / iterations}))

	var retained := TtxCounter.new()
	started = Time.get_ticks_usec()
	for index in 1000000:
		retained.advance(1)
	var elapsed := Time.get_ticks_usec() - started
	if retained.advance(0) != 1000000:
		push_error("Retained counter result changed")
		retained.free()
		quit(1)
		return

	retained.free()
	print("CALL ", JSON.stringify({"class": "TtxCounter", "iterations": 1000000,
		"mean_us": float(elapsed) / 1000000}))
	quit()
