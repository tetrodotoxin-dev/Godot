#!/usr/bin/env bash
# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

# Compile the two provider projects outside Bazel's Godot dependency graph.
# Only the public TTX headers and the already built canonical runtime are
# available to these invocations. No Godot SDK or generated binding is supplied.
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
cd "$root"
bazel build //:addon //tests/terminal:headless --config="${CONFIGURATION:-release}" "$@"
project=$(mktemp -d)
trap 'rm -rf -- "$project"' EXIT
mkdir -p "$project/c" "$project/cpp" "$project/addons/godot_ttx" "$project/.godot"
tar -xf .bin/bin/godot_ttx.tar -C "$project/addons/godot_ttx"
runtime="$project/addons/godot_ttx"
common=(-O3 -fPIC -fvisibility=hidden -DPERI_LINUX -DPERI_RELEASE -DPERI_EDITOR -I"$root" -I"$root/../tetrodotoxin")
link=(-shared -Wl,-z,defs -Wl,-rpath,'$ORIGIN' -L"$runtime" -llab_support -lttx_runtime)
(
    cd "$project/c"
    clang -std=c17 "${common[@]}" \
        "$root/extensions/counter/counter.c" "$root/extensions/counter/runtime.c" \
        "$root/extensions/counter/exports.c" "$root/extensions/counter/inspection.c" "${link[@]}" -o "$runtime/libcounter_extension.so"
)
(
    cd "$project/cpp"
    clang++ -std=c++23 -fno-rtti "${common[@]}" \
        "$root/extensions/sampling/exports.cpp" "$root/extensions/sampling/declaration.cpp" \
        "$root/extensions/sampling/runtime.cpp" "$root/extensions/sampling/sampler.cpp" \
        "$root/extensions/sampling/contracts.cpp" \
        "$root/sampling/function.cpp" "${link[@]}" -o "$runtime/libsampler_extension.so"
)

# Exactly these artifacts are opened by both consumers. Factory emission must
# survive graph destruction before the headless consumer asks for an instance.
bazel run //tests/terminal:headless --config="${CONFIGURATION:-release}" "$@" -- \
    "$runtime/libcounter_extension.so" "$runtime/libsampler_extension.so" "$runtime/libcpu_provider.so"
cp tests/classes/counter.tscn "$project/"
printf '%s\n' res://addons/godot_ttx/godot_ttx.gdextension > "$project/.godot/extension_list.cfg"
python3 tests/classes/configure.py "$project" valid
godot --headless --path "$project" --script "$root/tests/classes/check.gd"
echo 'PASS standalone C and C++ projects, same modules consumed by headless TTX and Godot'
