# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

@tool
extends VBoxContainer

var renderer: TtxRender
var undo: EditorUndoRedoManager
var offers: Array = []
var choices := OptionButton.new()
var diameter := SpinBox.new()
var parameters := HBoxContainer.new()
var feedback := Label.new()
var compute_time := Label.new()
var image := TextureRect.new()
var updating := false

func _init(target: TtxRender, history: EditorUndoRedoManager) -> void:
	renderer = target
	undo = history
	var discover := Button.new()
	discover.text = "Discover operations"
	discover.tooltip_text = "Observe the selected input provider. This can compile project code."
	discover.pressed.connect(_discover)
	add_child(discover)
	choices.add_item("Discover the input's capabilities first")
	choices.disabled = true
	choices.item_selected.connect(_select)
	add_child(choices)
	var label := Label.new()
	label.text = "Disk kernel"
	parameters.add_child(label)
	diameter.value_changed.connect(func(_value):
		if not updating and choices.selected >= 0:
			_select(choices.selected))
	parameters.add_child(diameter)
	parameters.visible = false
	add_child(parameters)
	var preview := Button.new()
	preview.text = "Preview output"
	preview.pressed.connect(_preview)
	add_child(preview)
	feedback.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	add_child(feedback)
	compute_time.visible = false
	add_child(compute_time)
	image.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	image.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	image.custom_minimum_size.y = 160
	image.visible = false
	add_child(image)

func _discover() -> void:
	compute_time.hide()
	var source := renderer.input if is_instance_valid(renderer.input) else renderer
	offers = source.get_offers()
	choices.clear()
	choices.add_item("Pass through")
	var selected := renderer.operation.is_empty()
	for offer: Dictionary in offers:
		choices.add_item(str(offer.name).capitalize())
		if offer.input == 1 and offer.maximum < offer.minimum:
			choices.set_item_disabled(choices.item_count - 1, true)
		if offer.contract == renderer.operation:
			choices.select(choices.item_count - 1)
			selected = true
	if not selected:
		choices.add_item("Selected operation is unavailable")
		choices.select(choices.item_count - 1)
		choices.set_item_disabled(choices.item_count - 1, true)
	choices.disabled = false
	feedback.text = source.get_error() if offers.is_empty() else "Choose a published operation. The provider still admits each request."
	_show_parameters(choices.selected)

func _show_parameters(index: int) -> void:
	updating = true
	parameters.visible = index > 0 and index <= offers.size() and offers[index - 1].input == 1
	if parameters.visible:
		var offer: Dictionary = offers[index - 1]
		diameter.min_value = offer.minimum
		diameter.max_value = maxi(offer.minimum, offer.maximum)
		diameter.step = offer.step
		diameter.editable = offer.maximum >= offer.minimum
		diameter.value = renderer.arguments[1] if renderer.arguments.size() == 3 else offer.minimum
	updating = false

func _select(index: int) -> void:
	if index > offers.size():
		return
	var contract := "" if index == 0 else str(offers[index - 1].contract)
	var values: Array = []
	var selected_kernel: bool = index > 0 and offers[index - 1].input == 1
	if selected_kernel:
		var size := int(diameter.value) if parameters.visible else int(offers[index - 1].minimum)
		values = [TtxDiskKernel.weights(size), size, size]
	undo.create_action("Change renderer operation")
	undo.add_do_property(renderer, "operation", contract)
	undo.add_do_property(renderer, "arguments", values)
	undo.add_undo_property(renderer, "operation", renderer.operation)
	undo.add_undo_property(renderer, "arguments", renderer.arguments)
	undo.commit_action()
	_show_parameters(index)
	compute_time.hide()
	feedback.text = "Connect an Operand renderer for this operation." if index > 0 and offers[index - 1].input == 2 else "Preview to evaluate the configured operation."

func _preview() -> void:
	var texture := renderer.get_texture()
	image.texture = texture
	image.visible = texture != null
	renderer.update_configuration_warnings()
	compute_time.visible = texture != null
	if texture == null:
		feedback.text = renderer.get_error()
	else:
		feedback.text = "%d × %d" % [texture.get_width(), texture.get_height()]
		compute_time.text = "%.3f ms compute" % renderer.get_compute_ms()

	# This is an explicit observation. Ordinary resource and Inspector reads
	# leave evaluation with their caller instead of starting GPU work implicitly.
	renderer.output_changed.emit()
