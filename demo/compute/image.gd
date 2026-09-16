# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

extends RefCounted

# An image retains device storage and prepared entries. Operations build CUDA
# frames here, while consumers continue to ask the same image questions. Only
# read_pixels observes the buffer on the host. A foreign overlay arrives as
# observed RGBA8 bytes and is uploaded explicitly by composite.
const INVERT = "aa57de9d-d269-4e29-8892-5a047fbf0601"
const CONVOLVE = "aa57de9d-d269-4e29-8892-5a047fbf0602"
const COMPOSITE = "aa57de9d-d269-4e29-8892-5a047fbf0603"
const POSTERIZE = "b3ca4a5b-3a72-4f17-861c-24b37bf70001"
var program: TtxCudaProgram
var kernels: Dictionary
var width: int
var height: int
var pixels: TtxCudaBuffer

func _init(owner: TtxCudaProgram, prepared: Dictionary, w: int, h: int, buffer: TtxCudaBuffer) -> void:
	program = owner
	kernels = prepared.duplicate()
	width = w
	height = h
	pixels = buffer

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

func admit(contract: String, kw: int = 0, kh: int = 0) -> Dictionary:
	if supports(contract) != 0:
		return {"status": 1, "reason": "Operation is not provided by this project."}
	if contract == CONVOLVE and (kw <= 0 or kh <= 0 or kw > 127 or kh > 127 or kw % 2 == 0 or kh % 2 == 0):
		return {"status": 3, "reason": "This project program accepts odd kernels up to 127."}
	return {"status": 0, "reason": ""}

func read_pixels() -> PackedByteArray:
	return pixels.read()

func _result(buffer: TtxCudaBuffer) -> RefCounted:
	return get_script().new(program, kernels, width, height, buffer)

func _launch(entry: String, arguments: Array, count: int, output: TtxCudaBuffer) -> Variant:
	var kernel: TtxCudaKernel = kernels[entry]
	if not kernel.launch(arguments, Vector3i((count + 255) / 256, 1, 1), Vector3i(256, 1, 1)):
		return kernel.get_error()
	return _result(output)

func invert() -> Variant:
	var output := program.allocate(width * height * 4)
	if output == null:
		return program.get_error()
	return _launch("invert", [pixels, output, width * height * 4], width * height * 4, output)

func posterize() -> Variant:
	var output := program.allocate(width * height * 4)
	if output == null:
		return program.get_error()
	return _launch("posterize", [pixels, output, width * height * 4, 4], width * height * 4, output)

func convolve(weights: PackedFloat32Array, kw: int, kh: int) -> Variant:
	var admission := admit(CONVOLVE, kw, kh)
	if admission.status != 0:
		return admission.reason
	if weights.size() != kw * kh:
		return "Kernel coefficients do not match its dimensions."
	var coefficients := program.allocate(weights.size() * 4)
	var output := program.allocate(width * height * 4)
	if coefficients == null or output == null:
		return program.get_error()
	if not coefficients.write(weights.to_byte_array()):
		return "Kernel upload failed."
	return _launch("convolve", [pixels, output, width, height, coefficients, kw, kh], width * height, output)

func composite(overlay: PackedByteArray) -> Variant:
	if overlay.size() != width * height * 4:
		return "Overlay extent does not match this image."
	var front := program.allocate(overlay.size())
	var output := program.allocate(width * height * 4)
	if front == null or output == null:
		return program.get_error()
	if not front.write(overlay):
		return "Overlay upload failed."
	return _launch("composite", [pixels, front, output, width * height], width * height, output)
