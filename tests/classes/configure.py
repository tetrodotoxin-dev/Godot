#!/usr/bin/env python3
# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Write host exposure policy without modifying any provider binary."""
import json
from pathlib import Path
import sys

project = Path(sys.argv[1])
mode = sys.argv[2]
imports = {name: f"res://addons/godot_ttx/{library}" for name, library in {
    "counter": "libcounter_extension.so",
    "sampler": "libsampler_extension.so",
    "cpu": "libcpu_provider.so",
    "cuda": "libcuda_provider.so",
    "values": "libvalues_extension.so",
    "rejected": "librejected_extension.so",
    "missing": "missing.so",
}.items()}

def expose(module, export, name, base="RefCounted"):
    return {"module": module, "export": export, "name": name, "base": base}

classes = [expose("counter", "Counter", "TtxCounter", "Node"),
           expose("sampler", "Sampler", "TtxSampler")]
if mode == "none":
    classes = []
elif mode == "values":
    classes = [expose("values", "Values", "TtxValues")]
elif mode == "rejected":
    classes = [expose("missing", "Missing", "Missing"),
               expose("rejected", "Rejected", "Rejected"), *classes,
               expose("sampler", "Sampler", "TtxSampler")]
elif mode == "counter":
    classes = classes[:1]
elif mode == "bad_base":
    classes += [expose("counter", "Counter", "BadChild", "TtxCounter")]

(project / "project.godot").write_text(
    'config_version=5\n[application]\nconfig/name="TTX terminal check"\n[ttx]\n'
    + "imports=" + json.dumps(imports) + "\nclasses=" + json.dumps(classes) + "\n")
