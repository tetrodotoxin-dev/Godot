# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

extends RefCounted

# The project chooses entry names and their argument forms. CUDA sees only
# source and those forms. ImagePublication supplies the separate behavioral
# promises that let this program substitute for other image providers.
const ImagePublication = preload("image.gd")
var program := TtxCudaProgram.new()
var kernels := {}
var error := ""

func _init(path: String = "res://compute/images.cu") -> void:
	if not program.compile_file(path):
		error = program.get_error()
		return
	var declarations := {
		"invert": ["buffer", "buffer", "u32"],
		"posterize": ["buffer", "buffer", "u32", "u32"],
		"convolve": ["buffer", "buffer", "u32", "u32", "buffer", "u32", "u32"],
		"composite": ["buffer", "buffer", "buffer", "u32"],
	}
	for entry in declarations:
		var kernel := program.prepare(entry, declarations[entry])
		if kernel == null:
			error = program.get_error()
			return
		kernels[entry] = kernel

func create_image(width: int, height: int, pixels: PackedByteArray) -> Variant:
	if not error.is_empty():
		return error
	var buffer := program.allocate(pixels.size())
	if buffer == null or not buffer.write(pixels):
		return "Project image upload failed."
	return ImagePublication.new(program, kernels, width, height, buffer)
