# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

extends Control

const Sample = preload("res://sample.gd")
const Branch = preload("res://branch.gd")

@export var cpu_source: TtxImage
@export var cuda_source: TtxImage
@export var script_source: TtxImage
@export var overlay_source: TtxImage

@onready var mode_choice: OptionButton = %Mode
@onready var size_choice: OptionButton = %Size
@onready var kernel_choice: OptionButton = %Kernel
@onready var source_view: TextureRect = %SourcePreview.get_node("Content/Image")
@onready var cpu_view: TextureRect = %CpuPreview.get_node("Content/Image")
@onready var cuda_view: TextureRect = %CudaPreview.get_node("Content/Image")
@onready var script_view: TextureRect = %ScriptPreview.get_node("Content/Image")
@onready var script_error: Label = %ScriptPreview.get_node("Content/Error")
@onready var cpu_error: Label = %CpuPreview.get_node("Content/Error")
@onready var cuda_error: Label = %CudaPreview.get_node("Content/Error")
@onready var status: Label = %Status
@onready var detail: Label = %Detail

var cpu: Branch
var cuda: Branch
var scripted: Branch
var scripted_overlay: TtxImage
var sample_index := 0
var overlay_index := 0
var source_pixels := PackedByteArray()
var sample_key := ""
var initialized := false
var dimensions := Vector2i.ZERO

func _ready() -> void:
	cpu = Branch.new(cpu_source)
	cuda = Branch.new(cuda_source)
	scripted = Branch.new(script_source)
	mode_choice.add_item("Lens blur", 0)
	mode_choice.add_item("Invert", 1)
	for width in [128, 256, 512, 1024, 2048]:
		size_choice.add_item("%d × %d" % [width, width / 2], width)
	size_choice.select(0)
	for diameter in [3, 7, 15, 63, 127]:
		kernel_choice.add_item("%d × %d disk" % [diameter, diameter], diameter)
	kernel_choice.select(2)

	# Let the scene display its initial state before synchronous native work.
	await get_tree().process_frame
	compare()
	initialized = true

func compare() -> void:
	var width := size_choice.get_selected_id()
	var height := width / 2
	dimensions = Vector2i(width, height)
	var key := "%d:%d" % [width, sample_index]
	if sample_key != key:
		source_pixels = Sample.pixels(width, height, sample_index)
		sample_key = key

	_show_pixels(source_view, width, height, source_pixels)
	if not _publish_overlay(width, height):
		return

	var diameter := kernel_choice.get_selected_id()
	var weights := Sample.kernel(diameter)
	var inversion := mode_choice.get_selected_id() == 1
	var script_allowed := inversion or width * height * diameter * diameter <= 2000000
	%CpuMetric.text = "C++ · WARM INVERT" if inversion else "C++ · WARM FFT CONVOLUTION"
	%CudaMetric.text = "CUDA · WARM INVERT" if inversion else "CUDA · WARM FFT CONVOLUTION"
	%ScriptMetric.text = "GDSCRIPT · WARM INVERT" if inversion else "GDSCRIPT · SPATIAL CONVOLUTION"
	%CpuPreview.get_node("Content/Caption").text = "RGB inversion + script overlay" if inversion else "FFTW blur + script overlay"
	%CudaPreview.get_node("Content/Caption").text = "RGB inversion + script overlay" if inversion else "cuFFT blur + script overlay"
	%ScriptPreview.get_node("Content/Caption").text = "Script inversion + script overlay" if inversion else "Spatial blur + script overlay"
	var cpu_ok: bool = cpu.publish(width, height, source_pixels, key)
	var cuda_ok: bool = cuda.publish(width, height, source_pixels, key)
	var script_ok: bool = scripted.publish(width, height, source_pixels, key)
	if cpu_ok:
		cpu_ok = cpu.rebuild(weights, diameter, scripted_overlay, inversion)
	if cuda_ok:
		cuda_ok = cuda.rebuild(weights, diameter, scripted_overlay, inversion)

	if script_ok and script_allowed:
		script_ok = scripted.rebuild(weights, diameter, scripted_overlay, inversion)
	elif script_ok:
		# The synchronous spatial loop is still available through the API.
		# Keep the interactive comparison bounded rather than freezing its UI
		# for billions of samples or quietly labeling a native FFT as script.
		script_ok = false
		scripted.error = "Not timed for this workload.\nChoose inversion or a smaller blur."
	if not script_ok:
		scripted.composite = null

	# A missing GPU remains a visible failure of that panel. It never silently
	# substitutes the CPU implementation or prevents the CPU example rendering.
	if not cpu_ok:
		cpu.composite = null
	if not cuda_ok:
		cuda.composite = null
	_observe(width, height, false)

func next_sample() -> void:
	sample_index += 1
	compare()

func move_overlay() -> void:
	if cpu.composite == null and cuda.composite == null and scripted.composite == null:
		return

	overlay_index += 1
	var width := dimensions.x
	var height := dimensions.y
	if not _publish_overlay(width, height):
		return

	# No new convolution or composite is constructed. Reading the retained
	# results pulls the changed overlay through their existing dependency edges.
	_observe(width, height, true)

func _publish_overlay(width: int, height: int) -> bool:
	if overlay_source.set_rgba8(width, height, Sample.overlay(width, height, overlay_index)):
		if scripted_overlay == null:
			scripted_overlay = overlay_source.invert()
		if scripted_overlay != null:
			return true

	status.text = "OVERLAY UNAVAILABLE"
	detail.text = overlay_source.get_error()
	return false

func _observe(width: int, height: int, moved: bool) -> void:
	var cpu_pixels: PackedByteArray = cpu.observe()
	var cuda_pixels: PackedByteArray = cuda.observe()
	var script_pixels: PackedByteArray = scripted.observe()
	_show_branch(cpu_view, cpu_error, cpu_pixels, cpu.error, width, height)
	_show_branch(cuda_view, cuda_error, cuda_pixels, cuda.error, width, height)
	_show_branch(script_view, script_error, script_pixels, scripted.error, width, height)
	%CpuTime.text = "%.2f ms" % cpu.compute_ms if not cpu_pixels.is_empty() else "Unavailable"
	%CudaTime.text = "%.2f ms" % cuda.compute_ms if not cuda_pixels.is_empty() else "Unavailable"
	%ScriptTime.text = "%.2f ms" % scripted.compute_ms if not script_pixels.is_empty() else "Not measured"
	var observation := "Refresh + readback" if moved else "Readback"
	%Readback.text = "%s: C++ %.2f  /  CUDA %.2f  /  GD %.2f ms" % [observation, cpu.read_ms, cuda.read_ms, scripted.read_ms]

	var maximum := 0
	var compared := 0
	var reference := PackedByteArray()
	for pixels in [cpu_pixels, cuda_pixels, script_pixels]:
		if pixels.is_empty():
			continue
		if reference.is_empty():
			reference = pixels
		for index in pixels.size():
			maximum = maxi(maximum, absi(pixels[index] - reference[index]))
		compared += 1
	var agree := compared >= 2 and maximum <= 1
	status.text = "%d PROVIDERS MATCH  ·  %d / 255 max difference" % [compared, maximum] if agree else "COMPARISON INCOMPLETE"
	status.modulate = Color("81e6c1") if agree else Color("ff8899")
	%Ratio.text = "%.1f×" % (cpu.compute_ms / maxf(cuda.compute_ms, 0.001)) if not cpu_pixels.is_empty() and not cuda_pixels.is_empty() else "—"
	detail.text = "Script overlay updated. Existing filter results reused." if moved else "Same client and image contract. Convolution compares spatial and FFT algorithms."

func _show_branch(view: TextureRect, failure: Label, pixels: PackedByteArray,
		message: String, width: int, height: int) -> void:
	failure.text = message
	failure.visible = pixels.is_empty()
	if pixels.is_empty():
		view.texture = null
	else:
		_show_pixels(view, width, height, pixels)

func _show_pixels(view: TextureRect, width: int, height: int, pixels: PackedByteArray) -> void:
	view.texture = ImageTexture.create_from_image(
		Image.create_from_data(width, height, false, Image.FORMAT_RGBA8, pixels))
