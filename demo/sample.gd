# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

extends RefCounted

# These inputs make the effects visible without depending on an external image
# asset. Changing the seed changes the source; moving the overlay changes only
# the foreground publication, which lets the lab demonstrate selective reuse.
static func pixels(width: int, height: int, seed: int) -> PackedByteArray:
	var output := PackedByteArray()
	output.resize(width * height * 4)
	for y in height:
		for x in width:
			var offset := (y * width + x) * 4
			output[offset] = (x * 255 / maxi(width - 1, 1) + seed * 31) % 256
			output[offset + 1] = (y * 255 / maxi(height - 1, 1) + seed * 59) % 256
			output[offset + 2] = 220 if ((x / maxi(width / 16, 1) + y / maxi(height / 8, 1) + seed) % 2 == 0) else 55
			output[offset + 3] = 255

	return output

static func overlay(width: int, height: int, step: int) -> PackedByteArray:
	var image := Image.create(width, height, false, Image.FORMAT_RGBA8)
	image.fill(Color.TRANSPARENT)
	var side := maxi(height / 3, 1)
	var x := (width / 5 + step * width / 7) % maxi(width - side, 1)
	image.fill_rect(Rect2i(x, height / 3, side, side), Color(1.0, 0.14, 0.45, 0.65))
	return image.get_data()
