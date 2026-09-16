# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

extends SceneTree

func _initialize() -> void:
	var value := TtxValues.new()
	var text := "A borrowed UTF8 result: λ " + "abcdef".repeat(1024)
	assert(value.echo(text) == text)
	assert(value.call("echo", text) == text)
	value.set_enabled(true)
	assert(value.is_enabled())
	assert(value.call("set_enabled", false) == null)
	assert(not value.call("is_enabled"))
	print("PASS method adapters: borrowed text, boolean and void through both routes")
	quit()
