#!/usr/bin/env python3
# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Run the pinned upstream generator when Bazel materializes its repository."""

import json
from pathlib import Path
import sys

source = Path(sys.argv[1])
sys.path.insert(0, str(source))
from binding_generator import generate_bindings
from build_profile import generate_trimmed_api

api = generate_trimmed_api(str(source / "gdextension/extension_api-4-7.json"), sys.argv[2])
trimmed = source / "extension_api.json"
trimmed.write_text(json.dumps(api))
generate_bindings(str(trimmed), str(source / "gdextension/gdextension_interface.json"),
                  True, sys.argv[3], "single", str(source))
