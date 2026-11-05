#!/usr/bin/env bash
# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

set -euo pipefail

runfiles=${RUNFILES_DIR:-"$0.runfiles"}
if [[ ! -d "$runfiles/_main" ]]; then
    echo "Godot validation runfiles are unavailable." >&2
    exit 1
fi
project="$runfiles/_main/validation/compute"
work=$(mktemp -d "${TMPDIR:-/tmp}/godot-ttx-compute.XXXXXX")
trap 'rm -rf -- "$work"' EXIT

cp -- "$project/project.godot" "$project/smoke.gd" \
    "$project/smoke.tscn" "$work"
tar -xf "$project/addon.tar" -C "$work"

export XDG_CONFIG_HOME="$work/config"
export XDG_DATA_HOME="$work/data"
export XDG_CACHE_HOME="$work/cache"

# Runtime validation selects the installed extension directly. The editor and
# export pipeline generate this inventory when they import the project.
mkdir -p -- "$work/.godot"
printf '%s\n' 'res://addons/godot_ttx/godot_ttx.gdextension' \
    > "$work/.godot/extension_list.cfg"
godot --headless --path "$work" --quit-after 120
