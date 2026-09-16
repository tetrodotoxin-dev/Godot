# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

extends SceneTree

func _initialize() -> void:
	# The declaration and emitted factory were accepted. Runtime creation or
	# callable fulfillment can still fail, and no partial object may escape.
	var instance = ClassDB.instantiate("TtxValues")
	assert(instance == null)
	print("PASS runtime construction failure")
	quit()
