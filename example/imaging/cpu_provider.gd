# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

@tool
class_name TtxCpuImages extends TtxComputeImages

func get_factory_name() -> StringName:
	return &"ComputeCpu"

func get_engine_name() -> String:
	return "Compute CPU"
