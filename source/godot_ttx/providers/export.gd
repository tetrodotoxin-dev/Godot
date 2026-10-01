# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

@tool
extends EditorExportPlugin

func _get_name() -> String:
	return "TTX providers"

func _export_begin(features: PackedStringArray, _debug: bool, _path: String, _flags: int) -> void:
	# Match the export's feature overrides rather than the editor host's OS.
	var imports: Dictionary = ProjectSettings.get_setting("ttx/imports", {})
	for setting in ProjectSettings.get_property_list():
		var name: String = setting.name
		if not name.begins_with("ttx/imports."):
			continue
		var matches := true
		for feature in name.trim_prefix("ttx/imports.").split("."):
			matches = matches and feature in features
		if matches:
			imports = ProjectSettings.get_setting(name)
	var included := {}
	for library in imports.values():
		if library in included:
			continue
		included[library] = true
		# Missing optional providers remain unavailable in the exported project.
		# Relative resource paths preserve the loader's configured location.
		if library is String and library.begins_with("res://") and FileAccess.file_exists(library):
			add_shared_object(library, PackedStringArray(), library.get_base_dir().trim_prefix("res://"))
