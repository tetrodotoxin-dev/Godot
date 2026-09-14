#!/usr/bin/env bash
# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

set -euo pipefail
repo_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd "$repo_root"
configuration=${CONFIGURATION:-release}
frames=$(mktemp -d)
trap 'rm -rf -- "$frames"' EXIT

bazel build //:addon --config="$configuration" --config=cuda
tools/install-addon.sh "$repo_root/demo"
godot --headless --editor --path "$repo_root/demo" --frame-delay 1000 --quit
godot --path "$repo_root/demo" --script "$repo_root/tests/visual_check.gd" -- "--capture=$frames"

# The animation contains actual viewport frames. Palette conversion keeps it
# embeddable on GitHub; correctness checks use the original unquantized textures.
ffmpeg -hide_banner -loglevel error -y -framerate 4 -i "$frames/%03d.png" \
    -filter_complex '[0:v]split[a][b];[a]palettegen=stats_mode=diff[p];[b][p]paletteuse=dither=bayer:bayer_scale=3' \
    -loop 0 "$repo_root/docs/lab.gif"
cp -- "$frames/000.png" "$repo_root/docs/lab.png"
echo "Captured docs/lab.gif and docs/lab.png from the running lab."
