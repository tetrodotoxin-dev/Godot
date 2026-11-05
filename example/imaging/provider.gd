# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

@tool
@abstract
class_name TtxImageProvider extends Resource

# Saved configuration can be shared by scenes without sharing runtime images.
# Each call returns one image publication with its own explicit resource
# obligations.
@abstract func create_image(width: int, height: int, pixels: PackedByteArray) -> Variant

func get_configuration_warnings() -> PackedStringArray:
	return []
