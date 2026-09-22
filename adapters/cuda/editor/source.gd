# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

@tool
extends EditorInspectorPlugin

func _can_handle(object: Object) -> bool:
	return object is TtxCudaSource

# The asset's source remains the authored .cu file. Previewing the imported text
# avoids presenting an editable copy which the next import would overwrite.
func _parse_begin(object: Object) -> void:
	var source := object as TtxCudaSource
	var content := VBoxContainer.new()
	var name := Label.new()
	name.text = source.source_name.get_file()
	content.add_child(name)
	var note := Label.new()
	note.text = "Imported source. Device compilation happens when its provider is requested."
	note.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	content.add_child(note)
	var code := CodeEdit.new()
	code.text = source.code
	code.editable = false
	code.custom_minimum_size.y = 260
	content.add_child(code)
	var open := Button.new()
	open.text = "Open source externally"
	open.pressed.connect(func(): OS.shell_open(ProjectSettings.globalize_path(source.source_name)))
	content.add_child(open)
	add_custom_control(content)
