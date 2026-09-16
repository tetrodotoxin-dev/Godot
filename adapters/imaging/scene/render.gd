# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

class_name TtxRender extends Node

# The scene owns rendering relationships. A renderer can import an ordinary
# Godot texture or consume another renderer, then publish a Texture2D that stock
# engine nodes can display. TTX image publications stay private so scene users
# need neither a native provider handle nor a second dependency graph API.
#
# Source edits invalidate connected nodes synchronously. Rendering is pulled
# when a consumer asks for the texture, which coalesces several Inspector or
# script changes into one observation. The image operation and host readback
# have separate timings because a device result need not occupy host memory.
signal output_changed

@export var display_name := "Renderer":
	set(value):
		if not _change_allowed():
			return
		display_name = value
		output_changed.emit()

@export var input: TtxRender:
	set(value):
		if not _change_allowed():
			return
		if is_instance_valid(value) and value._reaches(self):
			_error = "Renderer inputs cannot form a cycle."
			return
		_watch(input, value, "output_changed")
		input = value
		_invalidate()

@export var operand: TtxRender:
	set(value):
		if not _change_allowed():
			return
		if is_instance_valid(value) and value._reaches(self):
			_error = "Renderer inputs cannot form a cycle."
			return
		_watch(operand, value, "output_changed")
		operand = value
		_invalidate()

@export var source_texture: Texture2D:
	set(value):
		if not _change_allowed():
			return
		_watch(source_texture, value, "changed")
		source_texture = value
		_invalidate()

# Provider configuration belongs only to source nodes. A provider node can
# supply a script factory, including one built from project CUDA source.
# Derived renderers inherit their input's implementation through its image API.
@export var provider := "cpu":
	set(value):
		if not _change_allowed():
			return
		provider = value
		_source = null
		_invalidate()

@export var provider_node: Node:
	set(value):
		if not _change_allowed():
			return
		provider_node = value
		_source = null
		_invalidate()

@export var policy: TtxRenderPolicy:
	set(value):
		if not _change_allowed():
			return
		_watch(policy, value, "changed")
		policy = value
		_invalidate()

@export var operation := "":
	set(value):
		if not _change_allowed():
			return
		operation = value
		_invalidate()

@export var arguments: Array = []:
	set(value):
		if not _change_allowed():
			return
		arguments = value.duplicate()
		_invalidate()

var _source: TtxImage
var _result: TtxImage
var _texture: ImageTexture
var _dirty := true
var _building := false
var _error := ""
var _compute_ms := 0.0
var _read_ms := 0.0
var _revision := 0
var _generation := 0

# A provider may call back into the engine. It cannot replace this node's
# configuration while the node lends it current inputs. External textures can
# still signal an edit, which invalidates that observation before publication.
func _change_allowed() -> bool:
	if _building:
		_error = "Renderer configuration cannot change during observation."
		return false
	return true

func _reaches(target: TtxRender) -> bool:
	return self == target or (is_instance_valid(input) and input._reaches(target)) or (is_instance_valid(operand) and operand._reaches(target))

func _watch(previous: Object, next: Object, event: String) -> void:
	if is_instance_valid(previous):
		if previous.is_connected(event, _invalidate):
			previous.disconnect(event, _invalidate)
		if previous is Node and previous.tree_exited.is_connected(_invalidate):
			previous.tree_exited.disconnect(_invalidate)
	if is_instance_valid(next):
		if not next.is_connected(event, _invalidate):
			next.connect(event, _invalidate)
		if next is Node and not next.tree_exited.is_connected(_invalidate):
			next.tree_exited.connect(_invalidate)

func _invalidate() -> void:
	_generation += 1
	_texture = null
	if _dirty:
		return
	_dirty = true
	output_changed.emit()

func _base() -> TtxImage:
	if is_instance_valid(input):
		if not input.render():
			_error = input.get_error()
			return null
		return input._result
	if source_texture == null:
		_error = "Connect an input renderer or a source texture."
		return null
	if _source == null:
		_source = TtxImage.new()
		if is_instance_valid(provider_node):
			_source.set_provider_object(provider_node.create_provider())
		else:
			_source.provider = provider
	var image := source_texture.get_image()
	if image == null or image.is_empty():
		_error = "The source texture has no observable image."
		return null
	image = image.duplicate()
	image.convert(Image.FORMAT_RGBA8)
	if not _source.set_rgba8(image.get_width(), image.get_height(), image.get_data()):
		_error = _source.get_error()
		return null
	return _source

# Each observation starts from the actual output publication. Ancestor scene
# policies remain visible, but an ancestor's old capability inventory does not
# stand in for the capabilities of a newly produced image.
func _restrict_offers(offers: Array) -> Array:
	if is_instance_valid(input):
		offers = input._restrict_offers(offers)
	return policy.filter_offers(offers) if is_instance_valid(policy) else offers

func _permission(contract: String, width: int, height: int) -> Dictionary:
	if is_instance_valid(input):
		var inherited := input._permission(contract, width, height)
		if inherited.status != 0:
			return inherited
	return policy.admit(contract, width, height) if is_instance_valid(policy) else {"status": 0, "reason": ""}

func get_offers() -> Array:
	return _restrict_offers(_result.get_offers()) if render() else []

func admit(contract: String, width: int = 0, height: int = 0) -> Dictionary:
	var permission := _permission(contract, width, height)
	if permission.status != 0:
		return permission
	if render():
		return _result.admit(contract, width, height)
	return {"status": 3, "reason": _error}

func request(contract: String, values: Array = []) -> void:
	operation = contract
	arguments = values

func render() -> bool:
	if _building:
		_error = "Renderer observation cannot reenter itself."
		return false
	if not _dirty:
		return _result != null
	_building = true
	var generation := _generation
	_error = ""
	_compute_ms = 0.0
	_read_ms = 0.0
	var base := _base()
	var result: TtxImage = base
	if base != null and not operation.is_empty():
		var values := arguments.duplicate()
		if is_instance_valid(operand):
			if operand.render():
				values = [operand._result]
			else:
				_error = operand.get_error()
				result = null
		if result != null:
			var width := int(values[1]) if values.size() == 3 else 0
			var height := int(values[2]) if values.size() == 3 else 0
			var permission := _permission(operation, width, height)
			if permission.status == 0:
				permission = base.admit(operation, width, height)
			if permission.status != 0:
				_error = permission.reason
				result = null
			else:
				var started := Time.get_ticks_usec()
				result = base.invoke(operation, values)
				_compute_ms = (Time.get_ticks_usec() - started) / 1000.0
				if result == null:
					_error = base.get_error()
	_result = result
	_dirty = generation != _generation
	_building = false
	_revision += 1
	if _dirty:
		_error = "Renderer inputs changed during observation."
		return false
	return _result != null

func get_texture() -> Texture2D:
	if not render():
		return null
	if _texture == null:
		var generation := _generation
		_building = true
		var started := Time.get_ticks_usec()
		var pixels := _result.read_pixels()
		_building = false
		if generation != _generation:
			_error = "Renderer inputs changed during observation."
			return null
		_read_ms = (Time.get_ticks_usec() - started) / 1000.0
		if pixels.is_empty():
			_error = _result.get_error()
			return null
		_texture = ImageTexture.create_from_image(Image.create_from_data(
			_result.get_width(), _result.get_height(), false, Image.FORMAT_RGBA8, pixels))
	return _texture

func get_compute_ms() -> float:
	return _compute_ms + (input.get_compute_ms() if is_instance_valid(input) else 0.0)

func get_read_ms() -> float:
	return _read_ms

func get_error() -> String:
	return _error

# Revision counts executed observations, so scene tests can distinguish an
# unaffected upstream node from a downstream texture that refreshed after edit.
func get_revision() -> int:
	return _revision
