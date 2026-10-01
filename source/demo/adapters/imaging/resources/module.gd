# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

@tool
class_name TtxModuleImages extends TtxImageProvider

@export var module := "cpu":
	set(value):
		module = value
		emit_changed()

func create_source() -> TtxImage:
	var source := TtxImage.new()
	source.provider = module
	return source

func get_configuration_warnings() -> PackedStringArray:
	var imports: Dictionary = ProjectSettings.get_setting_with_override("ttx/imports")
	return [] if imports.has(module) else ["Choose a module configured in ttx/imports."]
