#!/usr/bin/env bash
# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
cd "$root"
bazel build //:addon --config=release --config=cuda
project=$(mktemp -d)
trap 'rm -rf -- "$project"' EXIT
sed '/^run\/main_scene=/d' demo/project.godot > "$project/project.godot"
cp -r demo/compute demo/policies "$project/"
tools/install-addon.sh "$project"
mkdir -p "$project/.godot"
printf '%s\n' res://addons/godot_ttx/godot_ttx.gdextension > "$project/.godot/extension_list.cfg"
godot --headless --editor --path "$project" --import
godot --headless --path "$project" --script "$root/tests/compute/check.gd"
