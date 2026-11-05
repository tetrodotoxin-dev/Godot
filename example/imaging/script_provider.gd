# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

@tool
class_name TtxScriptImages extends TtxImageProvider

@export var factory: Script:
	set(value):
		factory = value
		emit_changed()

func create_image(width: int, height: int, pixels: PackedByteArray) -> Variant:
	if factory == null:
		return "Choose an image factory script."
	return factory.new().create_image(width, height, pixels)

func get_configuration_warnings() -> PackedStringArray:
	return ["Choose an image factory script."] if factory == null else []
