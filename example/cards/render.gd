# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

@tool
extends PanelContainer

# This scene owns both its presentation and renderer pipeline. The lab can
# inject shared textures, but the defaults make the card useful when run alone
# or dropped into another scene. Configuration Resources are shared data, while
# each child renderer constructs and retains its own runtime source.
@export var display_name := "Renderer":
	set(value):
		display_name = value
		_configure()

@export var provider: TtxImageProvider = preload("res://providers/cpu.tres"):
	set(value):
		provider = value
		_configure()

@export var source_texture: Texture2D = preload("res://assets/source.tres"):
	set(value):
		source_texture = value
		_configure()

@export var overlay_texture: Texture2D = preload("res://assets/overlay.tres"):
	set(value):
		overlay_texture = value
		_configure()

@export var policy: TtxRenderPolicy:
	set(value):
		policy = value
		_configure()

@export var show_source := false:
	set(value):
		show_source = value
		_configure()

@export_tool_button("Preview card", "ImageTexture") var preview_action = preview

var renderer: TtxRender
var queued := false

func _ready() -> void:
	_configure()

func _configure() -> void:
	if not is_node_ready():
		return
	%Input.provider = provider
	%Input.source_texture = source_texture
	%Input.policy = policy
	%Overlay.source_texture = overlay_texture
	for pair in [[%Filter, "lab_filters"], [%Output, "lab_outputs"]]:
		if show_source:
			pair[0].remove_from_group(pair[1])
		else:
			pair[0].add_to_group(pair[1])
	if is_instance_valid(renderer) and renderer.output_changed.is_connected(_queue_refresh):
		renderer.output_changed.disconnect(_queue_refresh)
	renderer = %Input if show_source else %Output
	renderer.display_name = display_name
	renderer.output_changed.connect(_queue_refresh)
	%Title.text = display_name
	_queue_refresh()

func _get_configuration_warnings() -> PackedStringArray:
	if not is_node_ready():
		return []
	var warnings: PackedStringArray = %Input._get_configuration_warnings()
	if renderer != null and not renderer.get_error().is_empty():
		warnings.append(renderer.get_error())
	return warnings

func _queue_refresh() -> void:
	# Inspector edits are configuration work. Device compilation and expensive
	# script operations require Preview in the editor, or an ordinary runtime
	# observation after Play. Several runtime edits share one deferred refresh.
	if Engine.is_editor_hint():
		update_configuration_warnings()
		return
	if queued:
		return
	queued = true
	_refresh.call_deferred()

func preview() -> void:
	if is_node_ready():
		_refresh()

func _refresh() -> void:
	queued = false
	var texture := renderer.get_texture()
	%Image.texture = texture
	%Error.text = renderer.get_error()
	%Error.visible = texture == null
	%Time.text = "%.3f ms compute" % renderer.get_compute_ms() if texture != null else "Unavailable"
	%Caption.text = "Readback %.3f ms" % renderer.get_read_ms()
	renderer.update_configuration_warnings()
