#!/usr/bin/env bash
# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
cd "$root"
bazel build //:addon --config=release --config=cuda
project=$(mktemp -d)
trap 'rm -rf -- "$project"' EXIT
mkdir -p "$project/addons/godot_ttx" "$project/.godot"
tar -xf .bin/bin/godot_ttx.tar -C "$project/addons/godot_ttx"
printf '%s\n' res://addons/godot_ttx/godot_ttx.gdextension > "$project/.godot/extension_list.cfg"
cat > "$project/project.godot" <<'CONFIG'
config_version=5
[application]
config/name="TTX Render export proof"
[ttx]
imports={"cpu": "res://addons/godot_ttx/libcpu_provider.so", "cuda": "res://addons/godot_ttx/libcuda_provider.so"}
classes=[{"module":"cpu", "export":["Imaging","Render"], "name":"TtxCpuRender", "base":"Node"}, {"module":"cuda", "export":["Imaging","Render"], "name":"TtxCudaRender", "base":"Node"}]
CONFIG
godot --headless --path "$project" --script "$root/tests/render/check.gd"
