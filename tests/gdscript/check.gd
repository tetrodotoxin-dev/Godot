# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

extends SceneTree

const Factory = preload("fixture.gd")
const ScriptFactory = preload("res://addons/godot_ttx/gdscript/provider.gd")

var failed := false

func require(condition: bool, message: String) -> void:
    if not condition:
        failed = true
        push_error(message)

func _initialize() -> void:
    _run.call_deferred()

func _run() -> void:
    _ownership()
    _configuration()
    _delegation()
    _reentrancy()
    _release_observation()
    _failures()
    _composition("cpu")
    if "--cuda" in OS.get_cmdline_user_args():
        _composition("cuda")
    if not failed:
        print("PASS script provider: admission, immutable observations, lifetime, refusals, reentrancy, retained calls and mixed composition")
    quit(1 if failed else 0)

func _ownership() -> void:
    var factory := Factory.new()
    var factory_lifetime: WeakRef = weakref(factory)
    var source := TtxImage.new()
    source.set_provider_object(factory)
    var original := PackedByteArray([10, 20, 30, 255])
    require(source.set_rgba8(1, 1, original), source.get_error())
    if failed:
        return
    var script_lifetime: WeakRef = factory.publications[0]

    # Mutation on either side of publication must not change an old answer.
    # No test reference keeps the implementation alive after the final owner.
    original[0] = 99
    var observed := source.read_pixels()
    observed[0] = 88
    require(source.read_pixels() == PackedByteArray([10, 20, 30, 255]), "Published script bytes were mutable through a caller")
    var result := source.invert()
    require(result != null, source.get_error())
    if result == null:
        return
    var history := result.snapshot()
    factory = null
    source = null
    result = null
    require(factory_lifetime.get_ref() != null, "Native image lost its supplying factory lifetime")
    require(script_lifetime.get_ref() == null, "Snapshot unnecessarily retained the source script image")
    require(history.read_pixels() == PackedByteArray([245, 235, 225, 255]), "Returned GDScript image did not survive its source")
    history = null
    require(factory_lifetime.get_ref() == null, "Final image release retained the script factory")

func _configuration() -> void:
    var factory := Factory.new()
    var source := TtxImage.new()
    source.set_provider_object(factory)
    require(source.set_rgba8(1, 1, PackedByteArray([1, 2, 3, 255])), source.get_error())
    var result := source.invert()
    require(result != null, source.get_error())
    if result == null:
        return
    var snapshot := result.snapshot()
    require(snapshot.get_provider_object() == factory, "Derivation lost its object factory configuration")
    var before: int = factory.publications.size()
    require(snapshot.set_rgba8(1, 1, PackedByteArray([4, 5, 6, 255])), snapshot.get_error())
    require(factory.publications.size() == before + 1, "Snapshot publication silently used another provider")
    source.provider = "cpu"
    require(source.get_provider_object() == null, "Path selection did not clear object selection")
    require(snapshot.get_provider_object() == factory, "Changing the source also changed snapshot configuration")

func _delegation() -> void:
    var factory := Factory.new()
    factory.mode = "delegate"
    var source := TtxImage.new()
    source.set_provider_object(factory)
    require(source.set_rgba8(1, 1, PackedByteArray([1, 2, 3, 255])), source.get_error())
    var policy_lifetime: WeakRef = factory.publications[0]
    var result := source.invert()
    require(result != null, "Delegated script Callable was rejected")
    if result == null:
        return
    factory = null
    source = null
    require(policy_lifetime.get_ref() != null, "Call lost the policy supplying its receiver")
    require(result.read_pixels() == PackedByteArray([254, 253, 252, 255]), "Delegation changed the operation result")
    result = null
    require(policy_lifetime.get_ref() == null, "Final call release retained a delegated script policy")

func _reentrancy() -> void:
    for mode in ["factory_reentry", "bind_reentry", "operation_reentry", "read_reentry"]:
        var factory := Factory.new()
        var source := TtxImage.new()
        source.set_provider_object(factory)
        var target: WeakRef = weakref(source)
        var owner: WeakRef = weakref(factory)
        var observations := {}
        # Weak captures avoid making the test itself a source/factory cycle.
        # Mutation is attempted at each actual callback boundary, including
        # pixel observation after the graph has already returned its value.
        factory.action = func():
            var active: TtxImage = target.get_ref()
            observations["write"] = active.set_rgba8(1, 1, PackedByteArray([9, 9, 9, 255]))
            observations["error"] = active.get_error()
            active.provider = "cpu"
            observations["path"] = active.get_provider()
            active.set_provider_object(RefCounted.new())
            observations["owner"] = active.get_provider_object() == owner.get_ref()
        factory.mode = mode
        require(source.set_rgba8(1, 1, PackedByteArray([1, 2, 3, 255])), "Outer publication failed: " + mode)
        if mode in ["bind_reentry", "operation_reentry"]:
            var result := source.invert()
            require(result != null and result.read_pixels() == PackedByteArray([254, 253, 252, 255]), "Rejected nested publication changed the outer result")
        if mode == "read_reentry":
            require(source.read_pixels() == PackedByteArray([1, 2, 3, 255]), "Readback callback invalidated its borrowed publication")
        require(observations.get("write", true) == false, "Nested publication was accepted: " + mode)
        require("during image observation" in observations.get("error", ""), "Nested publication lost its diagnostic")
        require(observations.get("path", "") == "object" and observations.get("owner", false), "Callback changed provider selection")
        factory.mode = "normal"
        factory.action = Callable()
        require(source.set_rgba8(1, 1, PackedByteArray([4, 5, 6, 255])), "Publication remained blocked after the observation ended")
        require(source.read_pixels() == PackedByteArray([4, 5, 6, 255]), "Post-observation publication was not visible")

func _release_observation() -> void:
    var source := TtxImage.new()
    var factory := Factory.new()
    source.set_provider_object(factory)
    var target: WeakRef = weakref(source)
    var observations := {}
    factory.release_action = func():
        var active: TtxImage = target.get_ref()
        observations["pixels"] = active.read_pixels()
        observations["write"] = active.set_rgba8(1, 1, PackedByteArray([9, 9, 9, 255]))
    require(source.set_rgba8(1, 1, PackedByteArray([1, 2, 3, 255])), source.get_error())
    # Only the prior publication gets a destruction callback. The callback
    # must see the new complete value and cannot recursively replace it.
    factory.release_action = Callable()
    require(source.set_rgba8(1, 1, PackedByteArray([4, 5, 6, 255])), source.get_error())
    require(observations.get("pixels") == PackedByteArray([4, 5, 6, 255]), "Destruction callback observed a half-published source")
    require(observations.get("write", true) == false, "Destruction callback replaced the source recursively")
    require(source.read_pixels() == PackedByteArray([4, 5, 6, 255]), "Destruction callback changed the new source")

func _failures() -> void:
    var source := TtxImage.new()
    source.set_provider_object(RefCounted.new())
    require(not source.set_rgba8(1, 1, PackedByteArray([1, 2, 3, 255])), "Invalid existing factory was admitted")
    var factory := Factory.new()
    source.set_provider_object(factory)
    require(source.set_rgba8(1, 1, PackedByteArray([1, 2, 3, 255])), source.get_error())
    for mode in ["factory_error", "bad_factory_result", "bad_image"]:
        factory.mode = mode
        require(not source.set_rgba8(1, 1, PackedByteArray([9, 9, 9, 255])), "Bad factory output was accepted: " + mode)
        require(source.read_pixels() == PackedByteArray([1, 2, 3, 255]), "Failed factory creation replaced the prior source")

    # Refusal is tested through the public consumer. An invalid publication may
    # not acquire another implementation by falling through to the CPU backend.
    var expected := {"unsupported": "does not support", "pending": "cannot yet", "rejected": "rejected", "invalid_callable": "rejected", "invalid_answer": "rejected"}
    for mode in expected:
        factory.mode = mode
        require(source.set_rgba8(1, 1, PackedByteArray([1, 2, 3, 255])), source.get_error())
        require(source.invert() == null, "Refusal was bypassed: " + mode)
        require(expected[mode] in source.get_error(), "Refusal lost its distinction: " + mode)

    for mode in ["bad_result", "operation_error", "bad_signature"]:
        factory.mode = mode
        require(source.set_rgba8(1, 1, PackedByteArray([1, 2, 3, 255])), source.get_error())
        require(source.invert() == null and not source.get_error().is_empty(), "Invalid script result certified an image")
    factory.mode = "bad_pixels"
    require(source.set_rgba8(1, 1, PackedByteArray([1, 2, 3, 255])), source.get_error())
    require(source.read_pixels().is_empty() and not source.get_error().is_empty(), "Wrong script pixel extent was accepted")
    factory.mode = "normal"
    require(source.set_rgba8(1, 1, PackedByteArray([1, 2, 3, 255])), source.get_error())
    require(source.invert() != null, "Corrected script publication could not recover")

func _composition(backend: String) -> void:
    var background := TtxImage.new()
    background.provider = backend
    var overlay := TtxImage.new()
    var script_factory := Factory.new()
    overlay.set_provider_object(script_factory)
    require(background.set_rgba8(1, 1, PackedByteArray([20, 40, 60, 255])), background.get_error())
    require(overlay.set_rgba8(1, 1, PackedByteArray([35, 115, 195, 128])), overlay.get_error())
    if failed:
        return

    # The overlay itself is produced by GDScript through an actual TTX call.
    # CUDA then reads that publication through Block without knowing its owner.
    var filtered := background.convolve(PackedFloat32Array([1]), 1, 1)
    var scripted := overlay.invert()
    var combined := filtered.composite(scripted)
    require(combined != null and combined.read_pixels() == PackedByteArray([120, 90, 60, 255]), "Script output could not compose into " + backend)
    if combined == null:
        return
    var history := combined.snapshot()
    var before := background.get_provider_statistics()
    require(overlay.set_rgba8(1, 1, PackedByteArray([255, 255, 255, 255])), overlay.get_error())
    require(combined.read_pixels() == PackedByteArray([0, 0, 0, 255]), "Script source edit did not reach a retained native result")
    var after := background.get_provider_statistics()
    if backend == "cuda":
        require(after.uploads == before.uploads + 1, "Script overlay update did not perform exactly one CUDA upload")
        require(after.downloads == before.downloads + 1, "Script overlay update downloaded an extra CUDA intermediate")
        require(after.plan_builds == before.plan_builds, "Script overlay update rebuilt CUDA FFT plans")

    # Reverse the direction. One script receiver consumes native pixels more
    # than once, while its successful fulfillment remains cached on that image.
    var reverse := overlay.composite(background)
    require(reverse != null and reverse.read_pixels() == background.read_pixels(), "GDScript could not consume " + backend)
    var script_image: RefCounted = script_factory.publications.back().get_ref()
    var binds: int = script_image.bindings
    var calls: int = script_image.composites
    require(background.set_rgba8(1, 1, PackedByteArray([5, 6, 7, 255])), background.get_error())
    require(reverse.read_pixels() == PackedByteArray([5, 6, 7, 255]), "Native edit did not reach script composition")
    require(script_image.bindings == binds and script_image.composites == calls + 1, "Script invocation repeated successful fulfillment")
    require(history.read_pixels() == PackedByteArray([120, 90, 60, 255]), "Mixed composition snapshot changed after edits")
