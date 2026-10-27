# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

extends RefCounted

# This policy owns exposure, independently of how its referent computes pixels.
# Its permitted list contains contract UUIDs, so renaming a label does not
# change authority. Derived images keep the same policy, so an operation cannot shed restrictions
# by returning the underlying provider's next image.
var provider: RefCounted
var permitted: Array[String]
var maximum_kernel: int

func _init(source: RefCounted, operations: Array[String], limit: int = 127) -> void:
	provider = source
	permitted = operations.duplicate()
	maximum_kernel = limit

class View extends RefCounted:
	var implementation: RefCounted
	var permitted: Array[String]
	var maximum_kernel: int

	func _init(value: RefCounted, operations: Array[String], limit: int) -> void:
		implementation = value
		permitted = operations.duplicate()
		maximum_kernel = limit

	func _offer(contract: String) -> Dictionary:
		for offer: Dictionary in implementation.offers():
			if offer.contract == contract:
				return offer
		return {}

	func supports(contract: String) -> int:
		var offer := _offer(contract)
		if not offer.is_empty() and offer.contract not in permitted:
			return 3
		return implementation.supports(contract)

	func offers() -> Array:
		var result: Array = []
		for offered: Dictionary in implementation.offers():
			if offered.contract not in permitted:
				continue
			var item := offered.duplicate()
			if item.input == 1: item.maximum = mini(item.maximum, maximum_kernel)
			result.append(item)
		return result

	func admit(contract: String, width: int = 0, height: int = 0) -> Dictionary:
		if supports(contract) == 3:
			return {"status": 3, "reason": "Project policy does not expose this operation."}
		var offer := _offer(contract)
		if offer.get("input", -1) == 1 and (width > maximum_kernel or height > maximum_kernel):
			return {"status": 3, "reason": "Kernel exceeds the project policy limit."}
		return implementation.admit(contract, width, height)

	func _wrap(answer: Variant) -> Variant:
		if answer is String:
			return answer
		return View.new(answer, permitted, maximum_kernel)

	func fulfill(contract: String) -> Variant:
		var status := supports(contract)
		if status != 0:
			return status
		var callable: Variant = implementation.fulfill(contract)
		if not callable is Callable:
			return callable
		var offer := _offer(contract)
		if offer.is_empty():
			return callable
		match offer.get("input", -1):
			0:
				return func(): return _wrap(callable.call())
			1:
				return func(weights: PackedFloat32Array, width: int, height: int):
					var admission := admit(contract, width, height)
					if admission.status != 0:
						return admission.reason
					return _wrap(callable.call(weights, width, height))
			2:
				return func(overlay: PackedByteArray): return _wrap(callable.call(overlay))
		return 1

	func read_pixels() -> PackedByteArray:
		return implementation.read_pixels()

func create_image(width: int, height: int, pixels: PackedByteArray) -> Variant:
	var result: Variant = provider.create_image(width, height, pixels)
	if result is String:
		return result
	return View.new(result, permitted, maximum_kernel)
