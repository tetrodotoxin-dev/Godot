# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

@tool
class_name TtxCudaSource extends Resource

# Import preserves source text so an exported project can compile on its target
# device. Importing and inspecting this asset never loads a CUDA context. The
# consuming policy chooses when to compile and retains the resulting program.
@export_storage var code := ""
@export_storage var source_name := ""
