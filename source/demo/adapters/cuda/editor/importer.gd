# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

@tool
extends EditorImportPlugin

func _get_importer_name() -> String:
	return "ttx.cuda.source"

func _get_visible_name() -> String:
	return "CUDA Source"

func _get_recognized_extensions() -> PackedStringArray:
	return ["cu", "cuh"]

func _get_save_extension() -> String:
	return "res"

func _get_resource_type() -> String:
	return "Resource"

func _get_preset_count() -> int:
	return 0

func _get_import_options(_path: String, _preset: int) -> Array[Dictionary]:
	return []

# Device compilation belongs to the consuming policy. The import artifact only
# preserves text and its diagnostic name, so export includes the source even
# when the original file would otherwise be treated as an unknown extension.
func _import(path: String, save_path: String, _options: Dictionary,
		_platform_variants: Array[String], _generated: Array[String]) -> Error:
	var file := FileAccess.open(path, FileAccess.READ)
	if file == null:
		return FileAccess.get_open_error()
	var source := TtxCudaSource.new()
	source.code = file.get_as_text()
	source.source_name = path
	return ResourceSaver.save(source, save_path + ".res")
