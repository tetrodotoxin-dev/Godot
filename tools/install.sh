#!/usr/bin/env bash
# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

set -euo pipefail
repo_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
if [[ $# != 1 || ! -f "$1/project.godot" ]]; then
    echo "Usage: tools/install.sh /path/to/existing-godot-project" >&2
    exit 1
fi
archive="$repo_root/.bin/bin/demo_addon.tar"
if [[ ! -f "$archive" ]]; then
    echo "Build :demo_addon with Bazel before installing." >&2
    exit 1
fi
addon_root="$1/addons/godot_ttx"
mkdir -p -- "$addon_root"
staging=$(mktemp -d "$addon_root/.install.XXXXXX")
trap 'rm -rf -- "$staging"' EXIT
tar -xf "$archive" -C "$staging" --strip-components=2
# Replacing directory entries keeps mapped inodes valid in an open editor.
# Restart remains explicit because loaded providers retain their native state.
while IFS= read -r -d '' file; do
    relative=${file#"$staging"/}
    mkdir -p -- "$(dirname -- "$addon_root/$relative")"
    mv -f -- "$file" "$addon_root/$relative"
done < <(rg --files --hidden --null "$staging")
inventory=$(tar -tf "$archive" | sed 's@^addons/godot_ttx/@@')
for path in "$addon_root"/libcuda_provider.so "$addon_root"/libttx_cuda.so \
    "$addon_root"/libnvrtc*.so.* "$addon_root"/libcufft.so.* "$addon_root"/NVIDIA-*-LICENSE.txt; do
    library=${path##*/}
    if ! rg -Fxq "$library" <<< "$inventory"; then
        rm -f -- "$addon_root/$library"
    fi
done
printf 'Installed Bazel addon in %s\n' "$addon_root"
