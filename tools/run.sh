#!/usr/bin/env bash
# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

set -euo pipefail
repo_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd "$repo_root"
configuration=${CONFIGURATION:-release}
if (($#)); then
    echo "Usage: tools/run.sh" >&2
    exit 1
fi

bazel build :demo_addon --config="$configuration" --config=cuda
tools/install.sh "$repo_root/example"
# Finish class and Resource discovery before the scene starts. Frame pacing
# avoids Godot's first-import documentation race (godotengine/godot#111048).
godot --headless --editor --path "$repo_root/example" --import --frame-delay 1000
exec godot --path "$repo_root/example"
