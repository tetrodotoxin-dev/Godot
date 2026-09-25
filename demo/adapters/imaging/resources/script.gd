# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

@tool
class_name TtxScriptImages extends TtxImageProvider

# The script implements the existing image factory contract with a no argument
# constructor. Factory instances belong to individual renderer sources, while
# this Resource only retains the reusable script selection.
@export var factory: Script:
	set(value):
		factory = value
		emit_changed()

func create_source() -> TtxImage:
	if factory == null:
		return null
	var source := TtxImage.new()
	source.set_provider_object(factory.new())
	return source

func get_configuration_warnings() -> PackedStringArray:
	return ["Choose an image factory script."] if factory == null else []
