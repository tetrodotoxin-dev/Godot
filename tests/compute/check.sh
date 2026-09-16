#!/usr/bin/env bash
# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
cd "$root"
bazel build //:addon --config=release --config=cuda
project=$(mktemp -d)
trap 'rm -rf -- "$project"' EXIT
cp demo/project.godot "$project/"
cp -r demo/compute demo/policies "$project/"
tools/install-addon.sh "$project"
mkdir -p "$project/.godot"
printf '%s\n' res://addons/godot_ttx/godot_ttx.gdextension > "$project/.godot/extension_list.cfg"
godot --headless --path "$project" --script "$root/tests/compute/check.gd"
