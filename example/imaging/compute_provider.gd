# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

@tool
@abstract
class_name TtxComputeImages extends TtxImageProvider

const ComputeImage = preload("compute_image.gd")
var _factory: RefCounted
var _engine: RefCounted

@abstract func get_factory_name() -> StringName
@abstract func get_engine_name() -> String

func _factory_instance() -> RefCounted:
	if _factory == null and ClassDB.class_exists(get_factory_name()):
		_factory = ClassDB.instantiate(get_factory_name())
	return _factory

# The provider owns one engine for all images it creates. CUDA keeps compiled
# command kernels on that engine, while every Buffer independently preserves
# the engine state required by its storage.
func get_engine() -> RefCounted:
	if _engine == null:
		var factory := _factory_instance()
		if factory != null:
			_engine = factory.create_engine()
	return _engine

func create_image(width: int, height: int, pixels: PackedByteArray) -> Variant:
	if width <= 0 or height <= 0 or pixels.size() != width * height * 4:
		return "RGBA8 dimensions and pixel count must agree."
	var engine := get_engine()
	if engine == null:
		return "The %s module is unavailable." % get_engine_name()
	var buffer = engine.allocate(pixels.size())
	var status := 1
	if buffer != null:
		status = buffer.write(0, pixels)
	if status != 0:
		if buffer != null:
			buffer.release()
		return "The %s module could not upload the image." % get_engine_name()
	return ComputeImage.new(self, width, height, buffer)

func get_configuration_warnings() -> PackedStringArray:
	return [] if ClassDB.class_exists(get_factory_name()) else [
		"Load the %s module." % get_engine_name()
	]

func _notification(what: int) -> void:
	if what == NOTIFICATION_PREDELETE and _engine != null:
		var status: int = _engine.release()
		if status != 0:
			push_error("The %s engine could not be released cleanly." % get_engine_name())
		_engine = null
