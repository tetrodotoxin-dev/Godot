# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

extends Node

# The scene selects project source. Existing images keep their prepared
# kernels when this node or a later program publication is replaced.
@export_file("*.cu") var source_file := "res://compute/images.cu"

func create_provider() -> RefCounted:
	return preload("provider.gd").new(source_file)
