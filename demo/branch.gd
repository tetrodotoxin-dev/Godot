# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

extends RefCounted

# All panels execute this same client. The source Resource supplies its provider
# choice; returned images retain that implementation through the native graph.
# Keeping branch state here separates image operations from UI presentation.
var source: TtxImage
var filtered: TtxImage
var composite: TtxImage
var error := ""
var compute_ms := 0.0
var read_ms := 0.0
var source_key := ""

func _init(image: TtxImage) -> void:
	source = image

func publish(width: int, height: int, pixels: PackedByteArray, key: String) -> bool:
	error = ""
	key = "%s:%s" % [key, source.get_provider()]
	if source_key == key:
		return true

	if not source.set_rgba8(width, height, pixels):
		error = source.get_error()
		return false

	source_key = key
	return true

func rebuild(weights: PackedFloat32Array, size: int, overlay: TtxImage, invert: bool) -> bool:
	filtered = null
	composite = null
	error = ""

	# First preparation can compile GPU code or build transform plans. Warm it
	# before timing a second call, so compute and explicit readback stay separate.
	var warm := _filter(weights, size, invert)
	if warm == null:
		error = source.get_error()
		return false

	warm = null
	var started := Time.get_ticks_usec()
	filtered = _filter(weights, size, invert)
	compute_ms = (Time.get_ticks_usec() - started) / 1000.0
	if filtered == null:
		error = source.get_error()
		return false

	composite = filtered.composite(overlay)
	if composite == null:
		error = filtered.get_error()
		return false

	return true

func observe() -> PackedByteArray:
	if composite == null:
		return PackedByteArray()

	var started := Time.get_ticks_usec()
	var pixels := composite.read_pixels()
	read_ms = (Time.get_ticks_usec() - started) / 1000.0
	error = composite.get_error()
	return pixels

func _filter(weights: PackedFloat32Array, size: int, inversion: bool) -> TtxImage:
	if inversion:
		return source.invert()
	return source.convolve(weights, size, size)
