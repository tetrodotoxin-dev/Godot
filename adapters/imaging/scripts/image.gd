# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

extends RefCounted

# These UUIDs name the same callable promises as contracts/image.h. The bridge
# asks only for the contract being bound. Returning a Callable supplies it;
# Unsupported, Pending and Rejected remain distinct decisions of this owner.
const INVERT = "aa57de9d-d269-4e29-8892-5a047fbf0601"
const CONVOLVE = "aa57de9d-d269-4e29-8892-5a047fbf0602"
const COMPOSITE = "aa57de9d-d269-4e29-8892-5a047fbf0603"
const UNSUPPORTED = 1
const PENDING = 2
const REJECTED = 3

var _width: int
var _height: int
var _pixels: PackedByteArray

func _init(width: int, height: int, pixels: PackedByteArray) -> void:
	_width = width
	_height = height
	_pixels = pixels

# A promise does not construct the callable or establish its native ABI.
func supports(contract: String) -> int:
	return 0 if contract in [INVERT, CONVOLVE, COMPOSITE] else UNSUPPORTED

func fulfill(contract: String) -> Variant:
	match contract:
		INVERT:
			return invert
		CONVOLVE:
			return convolve
		COMPOSITE:
			return composite
	return UNSUPPORTED

# Observation lends no mutable access to this publication. A caller may change
# the returned array without changing old snapshots or cached operation results.
func read_pixels() -> PackedByteArray:
	return _pixels.duplicate()

func invert() -> RefCounted:
	var output := _pixels.duplicate()
	for offset in range(0, output.size(), 4):
		output[offset] ^= 255
		output[offset + 1] ^= 255
		output[offset + 2] ^= 255
	return get_script().new(_width, _height, output)

# This implementation deliberately uses spatial convolution. It is a readable
# provider of the same centered, zero-padded contract, not a script call to FFTW.
# Large kernels therefore compare complete algorithm choices as well as runtimes.
func convolve(weights: PackedFloat32Array, width: int, height: int) -> Variant:
	var admission := admit(CONVOLVE, width, height)
	if admission.status != 0:
		return admission.reason
	var output := _pixels.duplicate()
	var center_x := width / 2
	var center_y := height / 2
	for y in _height:
		for x in _width:
			var red := 0.0
			var green := 0.0
			var blue := 0.0
			for ky in height:
				var sy := y + center_y - ky
				if sy < 0 or sy >= _height:
					continue
				for kx in width:
					var sx := x + center_x - kx
					if sx < 0 or sx >= _width:
						continue
					var weight := weights[ky * width + kx]
					var input := (sy * _width + sx) * 4
					red += _pixels[input] * weight
					green += _pixels[input + 1] * weight
					blue += _pixels[input + 2] * weight
			var offset := (y * _width + x) * 4
			output[offset] = clampi(int(floor(red + 0.5)), 0, 255)
			output[offset + 1] = clampi(int(floor(green + 0.5)), 0, 255)
			output[offset + 2] = clampi(int(floor(blue + 0.5)), 0, 255)
	return get_script().new(_width, _height, output)

# Foreign overlays arrive as one observed RGBA8 array. All arithmetic runs here,
# and the integer numerator delays division until the final channel rounding.
# The bridge does not compute or inspect the image's native implementation.
func composite(overlay: PackedByteArray) -> RefCounted:
	var output := _pixels.duplicate()
	for offset in range(0, output.size(), 4):
		var front := int(overlay[offset + 3])
		var back := int(_pixels[offset + 3])
		var alpha := front * 255 + back * (255 - front)
		for channel in 3:
			var color := int(overlay[offset + channel]) * front * 255
			color += int(_pixels[offset + channel]) * back * (255 - front)
			output[offset + channel] = (color + alpha / 2) / alpha if alpha else 0
		output[offset + 3] = (alpha + 127) / 255
	return get_script().new(_width, _height, output)

# A synchronous script can promise the operation while declining work that
# would monopolize its worker. The budget belongs here so every consumer sees
# the same offers and refusal, including callers that bypass the lab controls.
const CONVOLUTION_BUDGET = 2000000

func admit(contract: String, width: int = 0, height: int = 0) -> Dictionary:
	var status := supports(contract)
	if status != 0:
		return {"status": status, "reason": "Operation is unavailable in this policy."}
	if contract == CONVOLVE:
		if width <= 0 or height <= 0 or width > 1023 or height > 1023 or width % 2 == 0 or height % 2 == 0:
			return {"status": REJECTED, "reason": "Kernel dimensions must be odd and between one and 1023."}
		if _width * _height * width * height > CONVOLUTION_BUDGET:
			return {"status": REJECTED, "reason": "Convolution exceeds this provider's synchronous work budget."}
	return {"status": 0, "reason": ""}

func offers() -> Array:
	var result: Array = []
	if supports(INVERT) != UNSUPPORTED:
		result.append({"contract": INVERT, "name": "invert", "input": 0})
	if supports(CONVOLVE) != UNSUPPORTED:
		var maximum := mini(1023, int(sqrt(float(CONVOLUTION_BUDGET) / (_width * _height))))
		if maximum % 2 == 0:
			maximum = maxi(0, maximum - 1)
		result.append({"contract": CONVOLVE, "name": "convolve", "input": 1, "minimum": 1, "maximum": maximum, "step": 2})
	if supports(COMPOSITE) != UNSUPPORTED:
		result.append({"contract": COMPOSITE, "name": "composite", "input": 2})
	return result
