# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

extends SceneTree

func _initialize() -> void:
	# An unsupported signature or rejected source policy must not leave a class
	# whose factory only fails later. The runner checks the matching diagnostic.
	assert(not ClassDB.class_exists(OS.get_cmdline_user_args()[0]))
	print("PASS class compilation declined without publication")
	quit()
