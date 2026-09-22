# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

@tool
extends EditorPlugin

var cuda_importer := preload("cuda/editor/importer.gd").new()
var cuda_inspector := preload("cuda/editor/source.gd").new()
var image_inspector := preload("imaging/editor/inspector.gd").new()

func _enter_tree() -> void:
	image_inspector.undo = get_undo_redo()
	add_import_plugin(cuda_importer)
	add_inspector_plugin(cuda_inspector)
	add_inspector_plugin(image_inspector)

func _exit_tree() -> void:
	remove_inspector_plugin(image_inspector)
	remove_inspector_plugin(cuda_inspector)
	remove_import_plugin(cuda_importer)
