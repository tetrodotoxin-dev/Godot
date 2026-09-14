#!/usr/bin/env bash
# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

set -euo pipefail
repo_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
if [[ $# == 0 ]]; then
    echo "Pass the exact C++ paths to format." >&2
    exit 1
fi
paths=()
for path in "$@"; do
    paths+=("$(realpath -- "$path")")
done
exec "$repo_root/../tetrodotoxin/.vscode/format.sh" "${paths[@]}"
