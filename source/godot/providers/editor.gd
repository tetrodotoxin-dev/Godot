# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

@tool
extends EditorPlugin

var exporter := preload("export.gd").new()

func _enter_tree() -> void:
	add_export_plugin(exporter)

func _exit_tree() -> void:
	remove_export_plugin(exporter)
