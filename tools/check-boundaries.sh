#!/usr/bin/env bash
# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

set -euo pipefail
repo_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd "$repo_root"

# The provider packages may share contracts and runtime support, but importing
# the host graph or the other implementation would defeat this example's boundary.
backends=(cpu cuda gdscript)
if [[ ${1:-} == --cpu ]]; then
    backends=(cpu gdscript)
    shift
fi
if (($#)); then
    echo "Usage: tools/check-boundaries.sh [--cpu]" >&2
    exit 1
fi

for backend in "${backends[@]}"; do
    target="//providers/$backend:${backend}_provider"
    [[ $backend != gdscript ]] || target="//providers/gdscript:gdscript"
    dependencies=$(bazel query "deps($target)" --output=label)
    while IFS= read -r target; do
        case "$target" in
            //images:*|//operations:*|//:libgodot_ttx.so|*//ttx:concept|*//ttx:model|*//ttx:lexical)
                echo "$backend provider crosses the host boundary: $target" >&2
                exit 1
                ;;
            *godot_cpp*)
                [[ $backend == gdscript ]] || { echo "$backend depends on Godot: $target" >&2; exit 1; }
                ;;
            //providers/gdscript:*)
                [[ $backend == gdscript ]] || { echo "$backend depends on GDScript: $target" >&2; exit 1; }
                ;;
            //providers/cpu:*)
                [[ $backend == cpu ]] || { echo "CUDA depends on CPU: $target" >&2; exit 1; }
                ;;
            //providers/cuda:*)
                [[ $backend == cuda ]] || { echo "CPU depends on CUDA: $target" >&2; exit 1; }
                ;;
        esac
    done <<< "$dependencies"
    echo "PASS $backend provider dependency boundary"
done
