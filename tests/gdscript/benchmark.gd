# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

extends SceneTree

const ScriptFactory = preload("res://addons/godot_ttx/gdscript/provider.gd")
var failed := false

func _initialize() -> void:
    _run.call_deferred()

func _run() -> void:
    var results: Array[Dictionary] = []
    if "--profile" in OS.get_cmdline_user_args():
        # A long single-path batch lets perf sample the actual product stack
        # without mixing GPU initialization or another provider into that path.
        var backend := "gdscript"
        if "--direct" in OS.get_cmdline_user_args():
            backend = "gdscript_direct"
        elif "--cpu" in OS.get_cmdline_user_args():
            backend = "cpu"
        _measure(results, 1, 1, 1000000, "invert", 0, [backend])
    else:
        # Large native cases make useful work visible above launch and graph
        # setup costs. Script convolution keeps its own explicit workload limit.
        for width in [1, 256, 1024, 2048]:
            var height := maxi(width / 2, 1)
            var iterations := 10000 if width == 1 else 5
            _measure(results, width, height, iterations, "invert", 0)
        _measure(results, 64, 32, 5, "convolve", 7)
        _measure(results, 1024, 512, 3, "convolve", 63)
        _measure(results, 2048, 1024, 3, "convolve", 63)
    print("MEASURE ", JSON.stringify(results))
    quit(1 if failed else 0)

func _measure(results: Array[Dictionary], width: int, height: int,
        iterations: int, operation: String, kernel_size: int,
        only: Array = []) -> void:
    var pixels := PackedByteArray()
    pixels.resize(width * height * 4)
    for index in pixels.size():
        pixels[index] = (index * 37 + index / 19) % 256
    var weights := PackedFloat32Array()
    weights.resize(kernel_size * kernel_size)
    if not weights.is_empty():
        weights.fill(1.0 / weights.size())
    var implementations := ["gdscript_direct", "gdscript", "cpu"]
    if "--cuda" in OS.get_cmdline_user_args():
        implementations.append("cuda")
    if not only.is_empty():
        implementations = only
    var reference := PackedByteArray()

    for backend in implementations:
        # Setup is outside the timer. Direct GDScript calls and calls through
        # TTX execute the same script algorithm on equal publications. Native
        # convolution uses FFTW/cuFFT, so that row compares algorithm choices.
        var source: RefCounted
        if backend == "gdscript_direct":
            source = ScriptFactory.new().create_image(width, height, pixels)
        else:
            var image := TtxImage.new()
            image.provider = backend
            if not image.set_rgba8(width, height, pixels):
                push_error(image.get_error())
                failed = true
                return
            source = image

        var requested := operation
        if backend == "gdscript_direct":
            requested = source.CONVOLVE if operation == "convolve" else source.INVERT
        var admission: Dictionary = source.admit(requested, kernel_size, kernel_size)
        if admission.status != 0:
            results.append({"backend": backend, "operation": operation,
                "width": width, "height": height, "kernel": kernel_size,
                "skipped": admission.reason})
            continue

        var warmups := 3 if backend in ["cpu", "cuda"] else 1
        var result: RefCounted
        for warmup in warmups:
            result = _operation(source, operation, weights, kernel_size)
        if result == null:
            push_error("Warm operation failed: " + backend)
            failed = true
            return

        # Only the backend is warm. Each TtxImage operation below constructs a
        # new graph Call and result Resource, and that Call fulfills its binding.
        # The direct script control creates only its own script result image.
        # The native mechanics benchmark measures retained handles separately.
        var samples := iterations
        if backend in ["cpu", "cuda"]:
            samples = maxi(samples, 50 if operation == "invert" else 10)

        var started := Time.get_ticks_usec()
        for iteration in samples:
            result = _operation(source, operation, weights, kernel_size)
        var elapsed := Time.get_ticks_usec() - started
        if result == null:
            failed = true
            return

        var read_started := Time.get_ticks_usec()
        var observed: PackedByteArray = result.read_pixels()
        var readback := Time.get_ticks_usec() - read_started
        if observed.size() != pixels.size():
            push_error("Measured image has the wrong byte extent: " + backend)
            failed = true
            return

        if reference.is_empty():
            reference = observed
        for index in observed.size():
            if absi(observed[index] - reference[index]) > (1 if operation == "convolve" else 0):
                push_error("Measured implementations disagree")
                failed = true
                return

        results.append({"backend": backend, "operation": operation,
            "width": width, "height": height, "kernel": kernel_size,
            "iterations": samples, "warmups": warmups,
            "path": "direct_script" if backend == "gdscript_direct" else "fresh_graph_operation",
            "mean_us": float(elapsed) / samples, "readback_us": readback})

func _operation(source: RefCounted, operation: String,
        weights: PackedFloat32Array, kernel_size: int) -> RefCounted:
    if operation == "invert":
        return source.invert()
    return source.convolve(weights, kernel_size, kernel_size)
