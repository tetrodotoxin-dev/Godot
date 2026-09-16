# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

extends SceneTree

func _initialize() -> void:
	var values := TtxValues.new()
	# The runner invokes each case in its own process and checks Godot's
	# diagnostic. Godot stops this script on a call error, so quit-after supplies
	# the exit condition. The callback must reject before entering the method.
	match OS.get_cmdline_user_args()[0]:
		"type":
			values.call("echo", 42)
		"few":
			values.call("set_enabled")
		"many":
			values.call("is_enabled", true)
