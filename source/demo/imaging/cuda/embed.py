#!/usr/bin/env python3
# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Keep CUDA source readable while embedding it in the installed provider."""

from pathlib import Path
import sys

source, destination = map(Path, sys.argv[1:])
text = source.read_text()
delimiter = "TTX_CUDA"
if f'){delimiter}"' in text:
    raise ValueError("CUDA source conflicts with the embedding delimiter")
destination.write_text(f'static constexpr const char source[] = R"{delimiter}(\n{text}){delimiter}";\n')
