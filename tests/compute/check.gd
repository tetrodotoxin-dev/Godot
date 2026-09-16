# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

extends SceneTree

const ScriptFactory = preload("res://addons/godot_ttx/gdscript/provider.gd")
const ProjectFactory = preload("res://compute/provider.gd")
const ProjectImage = preload("res://compute/image.gd")
const RenderPolicy = preload("res://policies/render.gd")
var failed := false

# An exposure policy owns image questions only. A separate capability remains
# callable through the wrapper without being converted into an image result.
class ExtraCapability extends RefCounted:
	func offers() -> Array:
		return []
	func supports(_contract: String) -> int:
		return 0
	func fulfill(_contract: String) -> Variant:
		return func(): return 23
	func admit(_contract: String, _width: int, _height: int) -> Dictionary:
		return {"status": 0, "reason": ""}

func require(value: bool, message: String) -> void:
	if not value:
		failed = true
		push_error(message)

func _initialize() -> void:
	_run.call_deferred()

func _run() -> void:
	_general_arguments()
	_admission()
	_offered_arguments()
	_project_images()
	if not failed: print("PASS project compute: general CUDA, retained programs, offers, restrictions and new effect")
	quit(1 if failed else 0)

# The same provider accepts a different parameter count and an aggregate. The
# source comes from this consumer and has no image operation interpretation.
func _general_arguments() -> void:
	var program := TtxCudaProgram.new()
	var source := """
#include "parameters.h"
#ifndef PROJECT_TTX
#error Project compiler options were lost
#endif
extern "C" __global__ void evaluate(float* out, unsigned n, Params p, double extra) {
  unsigned i = threadIdx.x;
  if (i < n) out[i] = i * p.scale + p.bias + extra;
}
extern "C" __global__ void empty() {}
extern "C" __global__ void numeric(double* out, unsigned char a, unsigned short b,
    unsigned c, unsigned long long d, signed char e, short f, int g,
    long long h, float x, double y) { *out = double(a) + b + c + d + e + f + g + h + x + y; }
"""
	require(program.compile(source, {"parameters.h": "struct Params { float scale; unsigned bias; };"}, PackedStringArray(["--std=c++17", "-DPROJECT_TTX=1"])), program.get_error())
	if failed: return
	var output := program.allocate(16)
	var kernel := program.prepare("evaluate", ["buffer", "u32", {"size": 8, "alignment": 4}, "r64"])
	require(kernel != null and output != null, program.get_error())
	if failed: return
	var parameters := PackedByteArray()
	parameters.resize(8)
	parameters.encode_float(0, 2.0)
	parameters.encode_u32(4, 3)
	require(kernel.launch([output, 4, parameters, 0.5], Vector3i.ONE, Vector3i(32, 1, 1)), kernel.get_error())
	var expected := PackedFloat32Array([3.5, 5.5, 7.5, 9.5]).to_byte_array()
	require(output.read() == expected, "General CUDA arguments produced incorrect values")
	require(not kernel.launch([output, -1, parameters, 0.5], Vector3i.ONE, Vector3i(32, 1, 1)), "Unsigned narrowing admitted a negative value")
	require(program.prepare("evaluate", ["buffer"]) == null, "Incorrect kernel signature was admitted")
	var numeric := program.prepare("numeric", ["buffer", "u8", "u16", "u32", "u64", "s8", "s16", "s32", "s64", "r32", "r64"])
	var scalar := program.allocate(8)
	require(numeric != null and numeric.launch([scalar, 255, 65535, 10, 11, -1, -2, -3, -4, 1.25, 0.5], Vector3i.ONE, Vector3i.ONE), "Primitive argument family did not launch")
	require(scalar.read().decode_double(0) == 65802.75, "Primitive arguments changed their values")
	require(not numeric.launch([scalar, 256, 65535, 10, 11, -1, -2, -3, -4, 1.25, 0.5], Vector3i.ONE, Vector3i.ONE), "Narrow integer overflow was accepted")
	var empty := program.prepare("empty", [])
	require(empty != null and empty.launch([], Vector3i.ONE, Vector3i.ONE), "No-argument kernel failed")
	require(not program.compile("invalid CUDA source") and not program.get_error().is_empty(), "Invalid source lost its compilation error")
	require(program.prepare("empty", []) != null, "Failed compilation replaced the previous program")
	# Recompilation changes future kernels. Existing kernels keep the old module.
	require(program.compile(source.replace("p.bias + extra", "p.bias + extra + 10"), {"parameters.h": "struct Params { float scale; unsigned bias; };"}, PackedStringArray(["--std=c++17", "-DPROJECT_TTX=1"])), program.get_error())
	var changed := program.prepare("evaluate", ["buffer", "u32", {"size": 8, "alignment": 4}, "r64"])
	require(changed != null and changed.launch([output, 4, parameters, 0.5], Vector3i.ONE, Vector3i(32, 1, 1)), "Replacement kernel could not launch")
	require(output.read().decode_float(0) == 13.5, "Project source replacement did not change execution")
	program = null
	require(kernel.launch([output, 4, parameters, 0.5], Vector3i.ONE, Vector3i(32, 1, 1)), kernel.get_error())
	require(output.read() == expected, "Replacing the program changed a retained kernel")

func _admission() -> void:
	var extra := RenderPolicy.View.new(ExtraCapability.new(), [], 3)
	require(extra.supports("extra") == 0 and extra.fulfill("extra").call() == 23, "Policy hid an unrelated capability")
	require(extra.admit("extra", 100, 100).status == 0, "Kernel policy intercepted an unrelated request")
	var factory := ScriptFactory.new()
	var pixels := PackedByteArray()
	pixels.resize(100 * 80 * 4)
	var direct: RefCounted = factory.create_image(100, 80, pixels)
	require(direct.admit(direct.CONVOLVE, 15, 15).status == 0, "Provider rejected a request below its budget")
	require(direct.admit(direct.CONVOLVE, 17, 17).status == 3, "Provider admitted a request above its budget")
	var weights := PackedFloat32Array()
	weights.resize(17 * 17)
	require(direct.convolve(weights, 17, 17) is String, "Direct script call bypassed the workload policy")
	var source := TtxImage.new()
	source.set_provider_object(factory)
	require(source.set_rgba8(100, 80, pixels), source.get_error())
	require(source.admit("convolve", 17, 17).status == 3, "Native observation lost provider refusal")
	require(source.convolve(weights, 17, 17) == null, "Bound invocation bypassed provider refusal")
	var found := false
	for offer: Dictionary in source.get_offers():
		if offer.name == "convolve":
			found = true
			require(offer.maximum == 15, "Kernel choices did not reflect source dimensions")
	require(found, "Convolution offer was lost")
	var restricted = RenderPolicy.new(factory, [ProjectImage.CONVOLVE], 3)
	source.set_provider_object(restricted)
	require(source.set_rgba8(1, 1, PackedByteArray([5, 6, 7, 255])), source.get_error())
	require(source.invert() == null and "rejected" in source.get_error(), "A project restriction exposed its referent")
	require(source.admit("convolve", 5, 5).status == 3, "Project parameter restriction was ignored")
	var result := source.convolve(PackedFloat32Array([1.0]), 1, 1)
	require(result != null and result.invert() == null, "An operation result shed its project policy")

func _project_images() -> void:
	var factory := ProjectFactory.new()
	require(factory.error.is_empty(), factory.error)
	if failed: return
	var source := TtxImage.new()
	source.set_provider_object(RenderPolicy.new(factory, [ProjectImage.INVERT, ProjectImage.CONVOLVE, ProjectImage.COMPOSITE, ProjectImage.POSTERIZE], 15))
	var original := PackedByteArray([10, 100, 170, 127, 250, 20, 30, 255])
	require(source.set_rgba8(2, 1, original), source.get_error())
	var inverted := source.invert()
	require(inverted != null, source.get_error())
	if failed: return
	require(inverted.read_pixels() == PackedByteArray([245, 155, 85, 127, 5, 235, 225, 255]), "Project CUDA disagrees with the independent inversion oracle")
	var identity := source.invoke("convolve", [PackedFloat32Array([1.0]), 1, 1])
	require(identity != null and identity.read_pixels() == original, "Discovered operation lost its argument owners")
	var posterized := source.invoke("posterize")
	require(posterized != null, source.get_error())
	if failed: return
	require(posterized.read_pixels() == PackedByteArray([0, 85, 170, 127, 255, 0, 0, 255]), "New effect did not execute through its own contract")
	var retained := posterized.snapshot()
	source = null
	factory = null
	posterized = null
	require(retained.read_pixels()[1] == 85, "Project publication did not retain its CUDA program")

# An unfamiliar UUID can use each established image argument form. These
# implementations deliberately choose their own behavior instead of pretending
# every kernel means convolution or every pair of images means compositing.
class OfferedImage extends RefCounted:
	const KERNEL = "b3ca4a5b-3a72-4f17-861c-24b37bf70011"
	const IMAGE = "b3ca4a5b-3a72-4f17-861c-24b37bf70012"
	var pixels: PackedByteArray
	func _init(values: PackedByteArray) -> void:
		pixels = values
	func read_pixels() -> PackedByteArray:
		return pixels
	func supports(contract: String) -> int:
		return 0 if contract in [KERNEL, IMAGE] else 1
	func offers() -> Array:
		return [
			{"contract": KERNEL, "name": "set_red", "input": 1, "minimum": 1, "maximum": 1, "step": 1},
			{"contract": IMAGE, "name": "replace", "input": 2},
		]
	func admit(contract: String, width: int = 0, height: int = 0) -> Dictionary:
		return {"status": supports(contract) if contract != KERNEL or (width == 1 and height == 1) else 3, "reason": "A single coefficient is required."}
	func fulfill(contract: String) -> Variant:
		match contract:
			KERNEL:
				return func(weights: PackedFloat32Array, width: int, height: int):
					if width != 1 or height != 1:
						return "A single coefficient is required."
					var result := pixels.duplicate()
					result[0] = int(weights[0])
					return OfferedImage.new(result)
			IMAGE:
				return func(overlay: PackedByteArray): return OfferedImage.new(overlay)
		return 1

class OfferedFactory extends RefCounted:
	func create_image(_width: int, _height: int, pixels: PackedByteArray) -> RefCounted:
		return OfferedImage.new(pixels)

func _offered_arguments() -> void:
	var source := TtxImage.new()
	source.set_provider_object(RenderPolicy.new(OfferedFactory.new(), [OfferedImage.KERNEL, OfferedImage.IMAGE], 1))
	require(source.set_rgba8(1, 1, PackedByteArray([1, 2, 3, 255])), source.get_error())
	var red := source.invoke(OfferedImage.KERNEL, [PackedFloat32Array([23.0]), 1, 1])
	require(red != null and red.read_pixels()[0] == 23, "New kernel contract lost its selected Callable")
	var overlay := TtxImage.new()
	require(overlay.set_rgba8(1, 1, PackedByteArray([9, 8, 7, 255])), overlay.get_error())
	var replaced := source.invoke(OfferedImage.IMAGE, [overlay])
	require(replaced != null and replaced.read_pixels() == overlay.read_pixels(), "New image contract lost its selected Callable")
