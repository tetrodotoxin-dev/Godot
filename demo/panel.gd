# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

extends PanelContainer

# Every instance observes one renderer node. The panel knows Godot textures,
# signals and timings, with no provider names or TTX image resource operations.
@export var renderer: TtxRender
var queued := false

func _ready() -> void:
	renderer.output_changed.connect(_queue_refresh)
	_queue_refresh()

func _queue_refresh() -> void:
	if not queued:
		queued = true
		_refresh.call_deferred()

func _refresh() -> void:
	queued = false
	var texture := renderer.get_texture()
	$Content/Title.text = renderer.display_name
	$Content/Image.texture = texture
	$Content/Error.text = renderer.get_error()
	$Content/Error.visible = texture == null
	$Content/Time.text = "%.3f ms compute" % renderer.get_compute_ms() if texture != null else "Unavailable"
	$Content/Caption.text = "Readback %.3f ms" % renderer.get_read_ms()
