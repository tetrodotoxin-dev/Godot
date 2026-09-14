#!/usr/bin/env bash
# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

set -euo pipefail
repo_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd "$repo_root"
configuration=${CONFIGURATION:-release}
backend=(--config=cuda)
visual=true
for argument in "$@"; do
    case "$argument" in
        --cpu) backend=(); visual=false ;;
        --no-visual) visual=false ;;
        *) echo "Usage: tools/check.sh [--cpu] [--no-visual]" >&2; exit 1 ;;
    esac
done

bazel build //:addon --config="$configuration" "${backend[@]}"
bazel test //tests:image_check //tests/sampling:check --config="$configuration" "${backend[@]}"
if ((${#backend[@]})); then
    tools/check-boundaries.sh
else
    tools/check-boundaries.sh --cpu
fi
tools/install-addon.sh "$repo_root/demo"
godot --headless --editor --path "$repo_root/demo" --frame-delay 1000 --quit
# Also exercise destruction of the ordinary scene. A script-only oracle does
# not necessarily construct every owner whose lifetime ends during shutdown.
godot --headless --path "$repo_root/demo" --quit-after 3
for provider in cpu gdscript; do
    TTX_IMAGE_PROVIDER="$provider" godot --headless --path "$repo_root/demo" \
        --script res://addons/godot_ttx/godot_check.gd
done
script_options=()
if ((${#backend[@]})); then
    script_options=(--cuda)
fi
godot --headless --path "$repo_root/demo" --script "$repo_root/tests/gdscript/check.gd" -- "${script_options[@]}"
godot --headless --path "$repo_root/demo" --script "$repo_root/tests/gdscript/sampling.gd" -- "${script_options[@]}"
if ((${#backend[@]})); then
    TTX_IMAGE_PROVIDER=cuda godot --headless --path "$repo_root/demo" \
        --script res://addons/godot_ttx/godot_check.gd
else
    echo "CUDA execution not requested (CPU-only check)."
fi
if [[ $visual == true ]]; then
    godot --path "$repo_root/demo" --script "$repo_root/tests/visual_check.gd"
else
    echo "Visual comparison not requested; no rendered-output claim."
fi
