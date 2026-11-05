# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

@tool
extends RefCounted

const INVERT = "c5f32a68-1edf-425e-b817-59d7620fa301"
const CONVOLVE = "c5f32a68-1edf-425e-b817-59d7620fa302"
const COMPOSITE = "c5f32a68-1edf-425e-b817-59d7620fa303"
const POSTERIZE = "b3ca4a5b-3a72-4f17-861c-24b37bf70001"

var provider: RefCounted
var width: int
var height: int
var buffer: RefCounted

func _init(owner: RefCounted, image_width: int, image_height: int, storage: RefCounted) -> void:
	provider = owner
	width = image_width
	height = image_height
	buffer = storage

func dispose() -> void:
	if buffer != null:
		buffer.release()
		buffer = null

func get_width() -> int:
	return width

func get_height() -> int:
	return height

func read_pixels() -> PackedByteArray:
	return buffer.read() if buffer != null else PackedByteArray()

func supports(contract: String) -> int:
	return 0 if contract in [INVERT, CONVOLVE, COMPOSITE, POSTERIZE] else 1

func fulfill(contract: String) -> Variant:
	match contract:
		INVERT: return invert
		CONVOLVE: return convolve
		COMPOSITE: return composite
		POSTERIZE: return posterize
	return 1

func offers() -> Array:
	return [
		{"contract": INVERT, "name": "invert", "input": 0},
		{"contract": CONVOLVE, "name": "convolve", "input": 1, "minimum": 1, "maximum": 127, "step": 2},
		{"contract": COMPOSITE, "name": "composite", "input": 2},
		{"contract": POSTERIZE, "name": "posterize", "input": 0},
	]

func admit(contract: String, kernel_width: int = 0, kernel_height: int = 0) -> Dictionary:
	if supports(contract) != 0:
		return {"status": 1, "reason": "Operation is not provided by this engine."}
	if contract == CONVOLVE:
		if kernel_width <= 0 or kernel_height <= 0 or kernel_width > 127 or kernel_height > 127 or kernel_width % 2 == 0 or kernel_height % 2 == 0:
			return {"status": 3, "reason": "Kernel dimensions must be odd and between one and 127."}
	return {"status": 0, "reason": ""}

func _record(name: String, arguments: Array, output: RefCounted, temporary: Array = []) -> Variant:
	var engine = provider.get_engine()
	if engine == null:
		output.release()
		for value: RefCounted in temporary:
			value.release()
		return "The Compute engine is unavailable."
	var commands = engine.create_commands()
	if commands == null:
		output.release()
		for value: RefCounted in temporary:
			value.release()
		return "The Compute engine could not create Commands."
	var status: int = commands.callv(name, arguments)
	if status == 0:
		status = commands.submit()
	if status == 0:
		status = commands.wait()
	commands.release()
	for value: RefCounted in temporary:
		value.release()
	if status != 0:
		output.release()
		return "The Compute command failed with status %d." % status
	return get_script().new(provider, width, height, output)

func _output() -> Variant:
	var engine = provider.get_engine()
	if engine == null:
		return null
	return engine.allocate(width * height * 4)

func invert() -> Variant:
	var output = _output()
	if output == null:
		return "The Compute engine could not allocate output."
	return _record("invert", [buffer, output, width * height * 4], output)

func posterize() -> Variant:
	var output = _output()
	if output == null:
		return "The Compute engine could not allocate output."
	return _record("posterize", [buffer, output, width * height * 4, 4], output)

func convolve(weights: PackedFloat32Array, kernel_width: int, kernel_height: int) -> Variant:
	var admission := admit(CONVOLVE, kernel_width, kernel_height)
	if admission.status != 0:
		return admission.reason
	if weights.size() != kernel_width * kernel_height:
		return "Kernel coefficients do not match its dimensions."
	var engine = provider.get_engine()
	if engine == null:
		return "The Compute engine is unavailable."
	var coefficients = engine.allocate(weights.size() * 4)
	var output = engine.allocate(width * height * 4)
	var status := 1
	if coefficients != null:
		status = coefficients.write(0, weights.to_byte_array())
	if status != 0 or output == null:
		if coefficients != null:
			coefficients.release()
		if output != null:
			output.release()
		return "The Compute engine could not prepare convolution storage."
	return _record("convolve", [buffer, output, coefficients, width, height, kernel_width, kernel_height], output, [coefficients])

func composite(overlay: RefCounted) -> Variant:
	var pixels: PackedByteArray = overlay.read_pixels()
	if pixels.size() != width * height * 4:
		return "Overlay extent does not match this image."
	var engine = provider.get_engine()
	if engine == null:
		return "The Compute engine is unavailable."
	var front = engine.allocate(pixels.size())
	var output = engine.allocate(width * height * 4)
	var status := 1
	if front != null:
		status = front.write(0, pixels)
	if status != 0 or output == null:
		if front != null:
			front.release()
		if output != null:
			output.release()
		return "The Compute engine could not prepare composite storage."
	return _record("composite", [buffer, front, output, width * height], output, [front])
