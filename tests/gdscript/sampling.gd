# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

extends SceneTree

var failed := false

func _initialize() -> void:
    _run.call_deferred()

func _expect(condition: bool, message: String) -> void:
    if not condition:
        failed = true
        push_error(message)

# This reference follows the integer specification independently of the native
# implementations. Masks make U32 wrapping explicit within GDScript's signed
# 64-bit arithmetic. The multiplier products fit that intermediate range.
func _reference(seed: int, first: int, size: int) -> int:
    var hits := 0
    for index in range(first, first + size):
        var value := index ^ seed
        value = (((value >> 16) ^ value) * 0x045d9f3b) & 0xffffffff
        value = (((value >> 16) ^ value) * 0x045d9f3b) & 0xffffffff
        value = (value >> 16) ^ value
        var x := value & 65535
        var y := value >> 16
        if x * x + y * y < (1 << 32):
            hits += 1

    return hits

func _measure(sampler: TtxSampler, backend: String, size: int, iterations: int) -> void:
    var expected := sampler.count(13, 0, size)
    _expect(expected >= 0, sampler.get_error())
    var total := 0
    var started := Time.get_ticks_usec()
    for iteration in iterations:
        total += sampler.count(13, 0, size)
    var elapsed := Time.get_ticks_usec() - started

    _expect(total == expected * iterations, "Measured scalar answers changed")
    print("SCALAR ", JSON.stringify({"backend": backend, "samples": size,
        "iterations": iterations, "mean_us": float(elapsed) / iterations,
        "hits": expected}))

func _run() -> void:
    # Construction and fulfillment happen once. Each count below returns an
    # ordinary int, with no Resource, image, or dependency node being created.
    var cpu := TtxSampler.new()
    _expect(cpu.count(0, 0, 1) == -1, "Unconfigured function was accepted")
    if not cpu.configure("cpu"):
        push_error(cpu.get_error())
        quit(1)
        return

    for request in [[0, 0, 17], [7, 123, 1024], [0xffffffff, 0xffffff00, 256]]:
        _expect(cpu.count(request[0], request[1], request[2]) == _reference(
            request[0], request[1], request[2]), "C implementation differs from script reference")

    _expect(cpu.count(0, 0xffffffff, 2) == -1, "Overflow interval accepted")
    _expect(cpu.count(-1, 0, 1) == -1, "Negative argument accepted")
    _expect(cpu.count(0, 0, 0) == 0, "Empty interval did not recover")
    _expect(cpu.get_error().is_empty(), "Successful call retained an old error")
    _expect(not cpu.configure("missing_sampling_provider"), "Missing provider accepted")
    _expect(cpu.count(0, 0, 17) == 15, "Failed reconfiguration lost the retained function")

    _measure(cpu, "cpu", 0, 1000000)
    _measure(cpu, "cpu", 1, 1000000)
    _measure(cpu, "cpu", 1048576, 5)
    _measure(cpu, "cpu", 16777216, 3)

    if "--cuda" in OS.get_cmdline_user_args():
        var cuda := TtxSampler.new()
        if not cuda.configure("cuda"):
            push_error(cuda.get_error())
            quit(1)
            return

        var seed := 123
        var first := 97
        var total := 16777216
        var split := 1048576
        var expected := cpu.count(seed, first, total)

        # Placement is a caller decision. The same immutable sample identities
        # reach each provider, so their disjoint results compose by addition.
        # These calls are synchronous and sequential, not a concurrency demo.
        var combined := cpu.count(seed, first, split)
        combined += cuda.count(seed, first + split, total - split)
        _expect(combined == expected, "Mixed CPU/CUDA work changed the answer")
        _expect(cuda.count(seed, first, total) == expected, "Complete GPU answer differs")
        _expect(cuda.count(0, 0xffffffff, 2) == -1, "GPU overflow interval accepted")
        _expect(cuda.count(0, 0, 17) == 15, "GPU did not recover after rejection")

        # Zero measures the CUDA callable boundary without launching device
        # work. Large intervals exercise the actual kernel and scalar readback.
        _measure(cuda, "cuda", 0, 1000000)
        _measure(cuda, "cuda", 1, 10000)
        _measure(cuda, "cuda", 1048576, 5)
        _measure(cuda, "cuda", 16777216, 3)

    print("PASS scalar functions: reference, partitioning, failures and retained calls" if not failed else "FAIL scalar functions")
    quit(1 if failed else 0)
