#!/usr/bin/env bash
# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

set -euo pipefail
repo_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
if [[ $# != 1 || ! -f "$1/project.godot" ]]; then
    echo "Usage: demo/install.sh /path/to/existing-godot-project" >&2
    exit 1
fi
archive="$repo_root/.bin/bin/demo/godot_ttx.tar"
if [[ ! -f "$archive" ]]; then
    echo "Build //demo:addon with Bazel before installing." >&2
    exit 1
fi
addon_root="$1/addons/godot_ttx"
mkdir -p -- "$addon_root"
staging=$(mktemp -d "$addon_root/.install.XXXXXX")
trap 'rm -rf -- "$staging"' EXIT
tar -xf "$archive" -C "$staging"
# Replacing directory entries keeps mapped inodes valid in an open editor.
# Restart remains explicit because loaded providers retain their native state.
while IFS= read -r -d '' file; do
    relative=${file#"$staging"/}
    mkdir -p -- "$(dirname -- "$addon_root/$relative")"
    mv -f -- "$file" "$addon_root/$relative"
done < <(rg --files --hidden --null "$staging")
# Policy moved from a Node script to a Resource. Leaving the old generated
# script installed would register the same Godot class twice during upgrade.
rm -f -- "$addon_root/scene/policy.gd" "$addon_root/scene/policy.gd.uid"
for library in libcuda_provider.so libttx_cuda.so; do
    if ! tar -tf "$archive" | rg -Fxq "$library"; then
        rm -f -- "$addon_root/$library"
    fi
done
printf 'Installed Bazel addon in %s\n' "$addon_root"
