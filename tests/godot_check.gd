extends SceneTree

func _initialize() -> void:
	# The same script runs under either host composition. It never names a provider.
	var source := TtxImage.new()
	var width := 19
	var height := 11
	var pixels := PackedByteArray()
	pixels.resize(width * height * 4)
	for i in pixels.size():
		pixels[i] = (i * 37 + i / 19) % 256
	if not source.set_rgba8(width, height, pixels):
		fail(source.get_error())
		return
	var inverted := source.invert()
	if inverted == null:
		fail(source.get_error())
		return
	var actual := inverted.read_pixels()
	var expected := pixels.duplicate()
	for i in expected.size():
		if i % 4 != 3:
			expected[i] = pixels[i] ^ 255
	if actual != expected or source.read_pixels() != pixels:
		fail("Independent inversion oracle or source immutability failed")
		return
	var weights := PackedFloat32Array([0.03, 0.07, 0.12, -0.02, 0.33, 0.09, 0.14, 0.04, 0.2])
	var result := source.convolve(weights, 3, 3)
	if result == null:
		fail(source.get_error())
		return
	result = result.snapshot()
	if not source.set_rgba8(1, 1, PackedByteArray([1, 2, 3, 4])):
		fail(source.get_error())
		return
	source = null
	inverted = null
	actual = result.read_pixels()
	if result.get_width() != width or result.get_height() != height or actual.size() != pixels.size():
		fail("Derived image lost its supplying storage or dimensions")
		return
	var maximum := 0
	for y in height:
		for x in width:
			var i := (y * width + x) * 4
			if actual[i + 3] != pixels[i + 3]:
				fail("Convolution changed alpha")
				return
			for c in 3:
				var total := 0.0
				for ky in 3:
					for kx in 3:
						var sx := x + 1 - kx
						var sy := y + 1 - ky
						if sx >= 0 and sy >= 0 and sx < width and sy < height:
							total += pixels[(sy * width + sx) * 4 + c] * weights[ky * 3 + kx]
				var reference := clampi(int(floor(total + 0.5)), 0, 255)
				maximum = maxi(maximum, absi(actual[i + c] - reference))
	if maximum > 1:
		fail("FFT differs from spatial oracle by %d bytes" % maximum)
		return
	if result.convolve(PackedFloat32Array([1, 1, 1, 1]), 2, 2) != null or result.get_error().is_empty():
		fail("Malformed kernel was accepted")
		return
	if result.set_rgba8(1, 1, PackedByteArray([1, 2, 3])) or result.read_pixels() != actual:
		fail("Failed source replacement changed the old image")
		return
	# Each node keeps its native dependency edges after the script drops sources.
	var background := TtxImage.new()
	var overlay := TtxImage.new()
	overlay.provider = "cpu"
	if not background.set_rgba8(1, 1, PackedByteArray([20, 40, 60, 255])) or not overlay.set_rgba8(1, 1, PackedByteArray([220, 140, 60, 128])):
		fail("Could not publish composition inputs")
		return
	var blurred := background.convolve(PackedFloat32Array([1]), 1, 1)
	var combined := blurred.composite(overlay)
	if combined == null or combined.read_pixels() != PackedByteArray([120, 90, 60, 255]):
		fail("Mixed provider source over failed")
		return
	var history := combined.snapshot()
	if not overlay.set_rgba8(1, 1, PackedByteArray([0, 0, 0, 255])) or combined.read_pixels() != PackedByteArray([0, 0, 0, 255]):
		fail("Overlay change did not reach the existing result resource")
		return
	background = null
	overlay = null
	blurred = null
	combined = null
	if history.read_pixels() != PackedByteArray([120, 90, 60, 255]):
		fail("Retained composition history changed")
		return
	print("PASS Godot: unchanged client under selected composition, persistent images, exact invert, spatial convolution, alpha and failure behavior; max error=", maximum)
	quit(0)

func fail(message: String) -> void:
	printerr(message)
	quit(1)
