# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

@tool
@abstract
class_name TtxImageProvider extends Resource

# Saved configuration can be shared by scenes without sharing their mutable
# image sources. Each renderer asks for its own source and retains the resulting
# provider publication for as long as its observations need it.
@abstract func create_source() -> TtxImage

func get_configuration_warnings() -> PackedStringArray:
	return []
