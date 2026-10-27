extends Label

func check(condition: bool, message: String) -> bool:
	if not condition:
		push_error(message)
		text = message
		get_tree().quit(1)
	return condition

func equal_pixels(first: PackedByteArray, second: PackedByteArray, tolerance := 0) -> bool:
	if first.size() != second.size():
		return false
	for i in first.size():
		if abs(int(first[i]) - int(second[i])) > tolerance:
			return false
	return true

func _ready() -> void:
	var input := PackedByteArray([10, 20, 30, 255, 40, 50, 60, 128, 70, 80, 90, 255, 100, 110, 120, 255])
	var cpu := TtxImage.new()
	cpu.provider = "cpu"
	if not check(cpu.set_rgba8(2, 2, input), "CPU create: " + cpu.get_error()): return
	var script := TtxImage.new()
	script.provider = "gdscript"
	if not check(script.set_rgba8(2, 2, input), "Script create: " + script.get_error()): return
	if not check(cpu.get_offers().size() == 3 and script.get_offers().size() == 3, "Image offers missing"): return
	var cpu_invert := cpu.invert()
	var script_invert := script.invert()
	if not check(cpu_invert != null and script_invert != null, "Inversion failed"): return
	if not check(cpu_invert.read_pixels() == script_invert.read_pixels(), "Inversion differs"): return
	var expected := input.duplicate()
	for i in range(0, expected.size(), 4):
		for c in 3: expected[i+c] ^= 255
	if not check(cpu_invert.read_pixels() == expected, "Inversion pixels incorrect"): return
	var weights := PackedFloat32Array([0.0,0.125,0.0,0.125,0.5,0.125,0.0,0.125,0.0])
	var cpu_blur := cpu.convolve(weights, 3, 3)
	var script_blur := script.convolve(weights, 3, 3)
	if not check(cpu_blur != null and script_blur != null, "Convolution failed"): return
	if not check(equal_pixels(cpu_blur.read_pixels(), script_blur.read_pixels(), 1), "FFT and script convolution differ"): return
	var cpu_composite := cpu_blur.composite(script)
	var script_composite := script_blur.composite(cpu)
	if not check(cpu_composite != null and script_composite != null, "Cross-provider composition failed"): return
	if not check(equal_pixels(cpu_composite.read_pixels(), script_composite.read_pixels(), 1), "Cross-provider composition differs"): return
	var snapshot := cpu.snapshot()
	var changed := input.duplicate()
	changed[0] = 200
	if not check(cpu.set_rgba8(2, 2, changed), "Source update failed"): return
	if not check(snapshot.read_pixels() == input, "Snapshot changed with source"): return
	if not check(cpu_invert.read_pixels()[0] == 55, "Derived result missed source revision"): return
	cpu = null
	script = null
	if not check(cpu_composite.read_pixels().size() == input.size(), "Result lost retained inputs"): return
	if not check(snapshot.read_pixels() == input, "Snapshot lost provider lifetime"): return
	if not check(cpu_blur.convolve(PackedFloat32Array([1.0]), 2, 2) == null, "Invalid kernel accepted"): return
	cpu_invert = null
	script_invert = null
	cpu_blur = null
	script_blur = null
	cpu_composite = null
	script_composite = null
	snapshot = null
	text = "CPU and GDScript images passed"
	print("TTX_IMAGE_SMOKE_PASS")
	if DisplayServer.get_name() == "headless": get_tree().quit()
