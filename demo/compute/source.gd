# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

@tool
extends TtxImageProvider

@export var source: TtxCudaSource:
	set(value):
		if source != null and source.changed.is_connected(emit_changed):
			source.changed.disconnect(emit_changed)
		source = value
		if source != null:
			source.changed.connect(emit_changed)
		emit_changed()

@export var options := PackedStringArray():
	set(value):
		options = value.duplicate()
		emit_changed()

func create_source() -> TtxImage:
	if source == null:
		return null
	var factory := preload("provider.gd").new(source, options)
	var image := TtxImage.new()
	image.set_provider_object(factory)
	return image

func get_configuration_warnings() -> PackedStringArray:
	return ["Assign a CUDA source asset."] if source == null else []
