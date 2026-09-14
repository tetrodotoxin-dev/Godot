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
    echo "Usage: tools/run-lab.sh [--cpu]" >&2
    exit 1
fi

bazel build //:addon --config="$configuration" "${backend[@]}"
tools/install-addon.sh "$repo_root/demo"
# This is the first-import synchronization workaround for Godot's editor path.
# Runtime execution itself is synchronous and needs no artificial frame delay.
godot --headless --editor --path "$repo_root/demo" --frame-delay 1000 --quit
exec godot --path "$repo_root/demo"
