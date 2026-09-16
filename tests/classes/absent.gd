# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

extends SceneTree

func _initialize() -> void:
	# The very same bridge binary starts without provider classes when imports
	# are absent. This rules out an unnoticed compiled registration fallback.
	var absent := not ClassDB.class_exists("TtxSampler") and not ClassDB.class_exists("TtxCounter")
	print("PASS no implicit classes" if absent else "FAIL implicit provider registration")
	quit(0 if absent else 1)
