# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

@tool
extends RefCounted

const ScriptImage = preload("image.gd")

# The factory creates immutable publications. It receives an owned Godot array
# from the native bridge, and the image takes an independent copy so direct
# script callers receive the same preservation guarantee as TTX consumers.
func create_image(width: int, height: int, pixels: PackedByteArray) -> RefCounted:
	return ScriptImage.new(width, height, pixels.duplicate())
