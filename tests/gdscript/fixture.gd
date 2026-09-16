# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

extends RefCounted

const ScriptImage = preload("res://addons/godot_ttx/gdscript/image.gd")

# The factory retains only weak observations of its publications. Tests can
# therefore distinguish native ownership from an accidental test reference.
var mode := "normal"
var action: Callable
var release_action: Callable
var publications: Array[WeakRef] = []

class Publication extends ScriptImage:
    var mode := "normal"
    var action: Callable
    var release_action: Callable
    var bindings := 0
    var composites := 0

    func supports(contract: String) -> int:
        match mode:
            "unsupported":
                return UNSUPPORTED
            "pending":
                return PENDING
            "rejected":
                return REJECTED
        return super.supports(contract)

    func fulfill(contract: String) -> Variant:
        bindings += 1
        if mode == "bind_reentry":
            action.call()
        match mode:
            "unsupported":
                return UNSUPPORTED
            "pending":
                return PENDING
            "rejected":
                return REJECTED
            "invalid_callable":
                return Callable()
            "invalid_answer":
                return 0
        if mode == "operation_reentry" and contract == INVERT:
            return during_operation
        if mode == "bad_signature" and contract == INVERT:
            return needs_argument
        if mode == "bad_result" and contract == INVERT:
            return bad_result
        if mode == "operation_error" and contract == INVERT:
            return operation_error
        return super.fulfill(contract)

    func during_operation() -> RefCounted:
        action.call()
        return super.invert()

    func _notification(what: int) -> void:
        if what == NOTIFICATION_PREDELETE and release_action.is_valid():
            release_action.call()

    func needs_argument(_value: int) -> RefCounted:
        return self

    func bad_result() -> int:
        return 17

    func operation_error() -> String:
        return "Script operation deliberately failed."

    func read_pixels() -> PackedByteArray:
        if mode == "read_reentry":
            action.call()
        if mode == "bad_pixels":
            return PackedByteArray([1])
        return super.read_pixels()

    func composite(overlay: PackedByteArray) -> RefCounted:
        composites += 1
        return super.composite(overlay)

# This is a separate policy owner, rather than a subclass of the implementation.
# Its retained implementation can provide Callables with a different receiver.
class Delegation extends RefCounted:
    var implementation: RefCounted

    func _init(image: RefCounted) -> void:
        implementation = image

    func supports(contract: String) -> int:
        return implementation.supports(contract)

    func fulfill(contract: String) -> Variant:
        return implementation.fulfill(contract)

    func offers() -> Array:
        return implementation.offers()

    func admit(contract: String, width: int = 0, height: int = 0) -> Dictionary:
        return implementation.admit(contract, width, height)

    func read_pixels() -> PackedByteArray:
        return implementation.read_pixels()

func create_image(width: int, height: int, pixels: PackedByteArray) -> Variant:
    if mode == "factory_reentry":
        action.call()
    if mode == "factory_error":
        return "Script factory deliberately failed."
    if mode == "bad_factory_result":
        return 17
    if mode == "bad_image":
        return RefCounted.new()
    var image := Publication.new(width, height, pixels.duplicate())
    image.mode = mode
    image.action = action
    image.release_action = release_action
    if mode == "delegate":
        var delegated := Delegation.new(image)
        publications.append(weakref(delegated))
        return delegated
    publications.append(weakref(image))
    return image
