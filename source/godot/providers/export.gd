# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

@tool
extends EditorExportPlugin

func _get_name() -> String:
	return "TTX providers"

func _runtime_files(library: String) -> Array:
	# SDK archives describe their runtime closure in sdk.json. Flattened addon
	# packages preserve that inventory beside the primary library instead.
	var directory := library.get_base_dir()
	var manifest := library + ".json"
	if not FileAccess.file_exists(manifest):
		directory = directory.get_base_dir()
		manifest = directory.path_join("sdk.json")
	var files: Array = [library]
	if not FileAccess.file_exists(manifest):
		return files
	var document = JSON.parse_string(FileAccess.get_file_as_string(manifest))
	if not document is Dictionary or not document.get("library") is String:
		return files
	if directory.path_join(document.library).simplify_path() != library.simplify_path():
		return files
	var runtime = document.get("runtime", [])
	if not runtime is Array:
		return files
	for entry in runtime:
		if entry is Dictionary and entry.get("library") is String:
			var path: String = directory.path_join(entry.library).simplify_path()
			if path.begins_with("res://") and FileAccess.file_exists(path):
				files.append(path)
	return files

func _export_begin(features: PackedStringArray, _debug: bool, _path: String, _flags: int) -> void:
	# Match the export's feature overrides rather than the editor host's OS.
	var modules: Array = ProjectSettings.get_setting("ttx/modules", [])
	for setting in ProjectSettings.get_property_list():
		var name: String = setting.name
		if not name.begins_with("ttx/modules."):
			continue
		var matches := true
		for feature in name.trim_prefix("ttx/modules.").split("."):
			matches = matches and feature in features
		if matches:
			modules = ProjectSettings.get_setting(name)
	var included := {}
	for library in modules:
		if not library is String or not library.begins_with("res://"):
			continue
		# Missing optional providers remain unavailable in the exported project.
		# Relative resource paths preserve the loader's configured location.
		for file in _runtime_files(library):
			if file not in included and FileAccess.file_exists(file):
				included[file] = true
				add_shared_object(file, PackedStringArray(), file.get_base_dir().trim_prefix("res://"))
