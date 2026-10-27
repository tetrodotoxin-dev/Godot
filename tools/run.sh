#!/usr/bin/env bash
# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

set -euo pipefail
repo_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd "$repo_root"
configuration=${CONFIGURATION:-release}
backend=(--config=cuda)
if [[ ${1:-} == --cpu ]]; then
    backend=()
    shift
fi
if (($#)); then
    echo "Usage: tools/run.sh [--cpu]" >&2
    exit 1
fi

bazel build //source/demo:addon --config="$configuration" "${backend[@]}"
tools/install.sh "$repo_root/example"
# Finish class and Resource discovery before the scene starts. Frame pacing
# avoids Godot's first-import documentation race (godotengine/godot#111048).
godot --headless --editor --path "$repo_root/example" --import --frame-delay 1000
exec godot --path "$repo_root/example"
