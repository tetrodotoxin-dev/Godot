# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

@tool
class_name TtxDiskKernel extends RefCounted

# A disk is an authoring choice for the image contract's coefficient array.
# Providers still receive ordinary weights and dimensions and need not know
# which editor or language prepared them.
static func weights(diameter: int) -> PackedFloat32Array:
	var values := PackedFloat32Array()
	values.resize(diameter * diameter)
	var radius := diameter / 2
	var count := 0
	for y in diameter:
		for x in diameter:
			var dx := x - radius
			var dy := y - radius
			if dx * dx + dy * dy <= radius * radius:
				values[y * diameter + x] = 1.0
				count += 1
	for index in values.size():
		values[index] /= count
	return values
