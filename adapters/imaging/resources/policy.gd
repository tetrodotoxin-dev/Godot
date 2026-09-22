# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

@tool
class_name TtxRenderPolicy extends Resource

# Policy Resources restrict the questions exposed by a renderer. Descendants
# still ask through their input node, so the restriction survives composition.
# The underlying provider independently admits its actual workload as well.
# An enabled contract restriction with an empty list exposes no operations.
# A zero maximum leaves kernel limits with the provider.

@export var restrict_contracts := false:
	set(value):
		restrict_contracts = value
		emit_changed()

@export var contracts: PackedStringArray = []:
	set(value):
		contracts = value.duplicate()
		emit_changed()

@export var maximum_kernel := 0:
	set(value):
		maximum_kernel = value
		emit_changed()

func filter_offers(offers: Array) -> Array:
	var result: Array = []
	for offer: Dictionary in offers:
		if restrict_contracts and offer.contract not in contracts:
			continue
		var permitted := offer.duplicate()
		if permitted.input == 1 and maximum_kernel > 0:
			permitted.maximum = mini(permitted.maximum, maximum_kernel)
		result.append(permitted)
	return result

func admit(contract: String, width: int, height: int) -> Dictionary:
	if restrict_contracts and contract not in contracts:
		return {"status": 3, "reason": "The scene policy does not expose this operation."}
	if maximum_kernel > 0 and (width > maximum_kernel or height > maximum_kernel):
		return {"status": 3, "reason": "Kernel exceeds the scene policy limit."}
	return {"status": 0, "reason": ""}
