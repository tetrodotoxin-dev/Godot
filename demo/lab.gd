# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

extends Control

const Sample = preload("res://sample.gd")

@export var source_texture: ImageTexture
@export var overlay_texture: ImageTexture
@onready var mode_choice: OptionButton = %Mode
@onready var size_choice: OptionButton = %Size
@onready var kernel_choice: OptionButton = %Kernel
var sample_index := 0
var overlay_index := 0
var sample_key := ""
var dimensions := Vector2i.ZERO
var initialized := false

func _ready() -> void:
	for width in [128, 256, 512, 1024, 2048]:
		size_choice.add_item("%d × %d" % [width, width / 2], width)
	size_choice.select(0)
	await get_tree().process_frame
	compare()
	initialized = true

# The lab supplies one request to the configured scene. Renderer nodes own
# implementation selection, admission, data and propagation. Adding a panel
# and renderer changes the scene without adding another controller branch.
func compare() -> void:
	var started := Time.get_ticks_usec()
	var width := size_choice.get_selected_id()
	var height := width / 2
	var next_dimensions := Vector2i(width, height)
	var key := "%d:%d" % [width, sample_index]
	if sample_key != key:
		source_texture.set_image(Image.create_from_data(width, height, false,
			Image.FORMAT_RGBA8, Sample.pixels(width, height, sample_index)))
		sample_key = key
	if dimensions != next_dimensions:
		dimensions = next_dimensions
		_publish_overlay()

	_refresh_choices()
	if mode_choice.item_count == 0:
		%Status.text = "NO IMAGE OPERATIONS AVAILABLE"
		%Elapsed.text = "Request %.3f ms" % ((Time.get_ticks_usec() - started) / 1000.0)
		return
	var operation: Dictionary = mode_choice.get_item_metadata(mode_choice.selected)
	var diameter := kernel_choice.get_selected_id()
	var arguments: Array = []
	if operation.input == 1:
		arguments = [Sample.kernel(diameter), diameter, diameter]
	for renderer: TtxRender in _nodes("lab_filters"):
		renderer.request(operation.contract, arguments)
	_observe()
	%Elapsed.text = "Request %.3f ms" % ((Time.get_ticks_usec() - started) / 1000.0)

func next_sample() -> void:
	sample_index += 1
	compare()

func move_overlay() -> void:
	var started := Time.get_ticks_usec()
	overlay_index += 1
	_publish_overlay()
	_observe()
	%Elapsed.text = "Request %.3f ms" % ((Time.get_ticks_usec() - started) / 1000.0)

func _publish_overlay() -> void:
	overlay_texture.set_image(Image.create_from_data(dimensions.x, dimensions.y,
		false, Image.FORMAT_RGBA8, Sample.overlay(dimensions.x, dimensions.y, overlay_index)))

func _nodes(group: StringName) -> Array[Node]:
	return get_tree().get_nodes_in_group(group).filter(is_ancestor_of)

func get_renderers() -> Array[Node]:
	return _nodes("lab_outputs")

func _observe() -> void:
	var reference: Texture2D
	var reference_pixels := PackedByteArray()
	var maximum := 0
	var compared := 0
	var same_extent := true
	for renderer: TtxRender in get_renderers():
		var texture := renderer.get_texture()
		if texture == null:
			continue
		compared += 1

		# A single available renderer has no peer to compare. Keep its texture
		# for display and only read it back if another renderer supplies output.
		# Comparing the first image with itself used to walk millions of bytes
		# in GDScript, hiding the UI cost behind a much shorter kernel timing.
		if reference == null:
			reference = texture
			continue
		if texture.get_size() != reference.get_size():
			same_extent = false
			continue
		if reference_pixels.is_empty():
			reference_pixels = reference.get_image().get_data()
		var pixels := texture.get_image().get_data()

		# Exact agreement uses the engine's packed array comparison. Different
		# FFT implementations can round a channel differently, so only those
		# results need the slower walk that measures the allowed byte tolerance.
		if pixels == reference_pixels:
			continue
		for index in pixels.size():
			maximum = maxi(maximum, absi(pixels[index] - reference_pixels[index]))
	if compared == 1:
		%Status.text = "AVAILABLE FROM ONE RENDERER"
	elif compared >= 2 and same_extent and maximum <= 1:
		%Status.text = "%d RENDERERS MATCH  ·  %d / 255 max difference" % [compared, maximum]
	else:
		%Status.text = "COMPARISON INCOMPLETE"
	%Status.theme_type_variation = &"AccentLabel" if compared > 0 and same_extent and maximum <= 1 else &"WarningLabel"

# Offer identity and limits come from the encountered renderer policies. The
# controls combine choices by UUID and preserve every renderer's later refusal.
func _refresh_choices() -> void:
	var previous := ""
	if mode_choice.selected >= 0:
		previous = mode_choice.get_item_metadata(mode_choice.selected).contract
	var diameter := kernel_choice.get_selected_id() if kernel_choice.selected >= 0 else 15
	var by_contract := {}
	var sizes := {}
	for renderer: TtxRender in _nodes("lab_filters"):
		for offer: Dictionary in renderer.input.get_offers():
			if offer.input == 2:
				continue
			by_contract[offer.contract] = offer
			if offer.input == 1 and offer.step > 0:
				if not sizes.has(offer.contract):
					sizes[offer.contract] = {}
				for size in range(offer.minimum, offer.maximum + 1, offer.step):
					sizes[offer.contract][size] = true
	var offers := by_contract.values()
	offers.sort_custom(func(a, b): return a.name < b.name)
	mode_choice.clear()
	for offer: Dictionary in offers:
		var index := mode_choice.item_count
		mode_choice.add_item(str(offer.name).capitalize(), index)
		mode_choice.set_item_metadata(index, offer)
		if offer.contract == previous:
			mode_choice.select(index)
	var selected: Dictionary = mode_choice.get_item_metadata(mode_choice.selected) if mode_choice.selected >= 0 else {}
	var ordered: Array = sizes.get(selected.get("contract", ""), {}).keys()
	ordered.sort()
	kernel_choice.clear()
	for size: int in ordered:
		kernel_choice.add_item("%d × %d disk" % [size, size], size)
		if size == diameter:
			kernel_choice.select(kernel_choice.item_count - 1)
	kernel_choice.disabled = selected.get("input", 0) != 1
