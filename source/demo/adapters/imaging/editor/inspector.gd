# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

@tool
extends EditorInspectorPlugin

var undo: EditorUndoRedoManager

func _can_handle(object: Object) -> bool:
	return object is TtxRender

func _parse_begin(object: Object) -> void:
	add_custom_control(preload("operations.gd").new(object, undo))

func _parse_property(_object: Object, _type: Variant.Type, name: String,
		_hint: PropertyHint, _hint_text: String, _usage: int, _wide: bool) -> bool:
	# UUIDs and argument carriers remain serialized, but the author selects
	# behavior from the encountered provider instead of editing wire identities.
	return name in ["operation", "arguments"]
