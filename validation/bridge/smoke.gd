# Copyright (c) 2023-present Matt Kaes and contributors

extends Label

func _ready() -> void:
	if not ClassDB.class_exists("TtxCounter"):
		fail("Counter was not discovered")
		return
	var first = ClassDB.instantiate("TtxCounter")
	var second = ClassDB.instantiate("TtxCounter")
	if first == null or second == null:
		fail("Counter construction failed")
		return
	if not first.has_method("advance") or not first.has_method("get_value"):
		fail("Counter methods were not discovered")
		return
	if first.call("advance", 7) != 7 or first.call("advance", 35) != 42:
		fail("Counter invocation failed")
		return
	if first.call("get_value") != 42 or second.call("get_value") != 0:
		fail("Counter state is shared between instances")
		return
	first = null
	if second.call("advance", 9) != 9:
		fail("Independent release invalidated the second instance")
		return
	second = null
	text = "TTX bridge: discovery, construction, invocation and release passed"
	print("TTX_BRIDGE_SMOKE_PASS")
	if DisplayServer.get_name() == "headless":
		get_tree().quit(0)

func fail(message: String) -> void:
	text = message
	push_error(message)
	get_tree().quit(1)
