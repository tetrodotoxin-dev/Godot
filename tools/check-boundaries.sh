#!/usr/bin/env bash
# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

set -euo pipefail
repo_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd "$repo_root"
cuda=true
if [[ ${1:-} == --cpu ]]; then
    cuda=false
    shift
fi
if (($#)); then
    echo "Usage: tools/check-boundaries.sh [--cpu]" >&2
    exit 1
fi

# Verify actual dependencies, not folder names. The generic bridge can compile
# without any of the applications that happen to use it in this addon.
check() {
    local target=$1
    local forbidden=$2
    local dependencies
    dependencies=$(bazel query "deps($target)" --output=label)
    if rg "$forbidden" <<< "$dependencies"; then
        echo "Unexpected dependency in $target" >&2
        exit 1
    fi
    echo "PASS $target dependency boundary"
}
check //gdextension '^//(imaging|adapters|plugins|sampling)(/|:)|ttx_cuda|cuda_sdk|fftw_sdk'
check //gdextension/contracts 'godot_cpp|^//(imaging|adapters|plugins|sampling)(/|:)'
check //adapters:cuda '^//(imaging|sampling|plugins)(/|:)|cuda_sdk|fftw_sdk|//cuda/runtime|//cuda:plugin'
check //adapters:imaging 'ttx_cuda|cuda_sdk|fftw_sdk|^//imaging/(cpu|cuda)'
check //plugins/cpu:cpu_provider 'godot_cpp|ttx_cuda|cuda_sdk|^//(adapters|imaging/graph|imaging/operations)(/|:)'
for target in //extensions/sampling:sampler_extension //extensions/counter:counter_extension; do
    check "$target" 'godot_cpp|^//(adapters|imaging/graph|imaging/operations)(/|:)'
done
if [[ $cuda == true ]]; then
    check @ttx_cuda//cuda:plugin 'godot_cpp|fftw_sdk|:cufft$|^//(imaging|adapters|plugins|sampling)(/|:)'
    check //plugins/cuda:cuda_provider 'godot_cpp|fftw_sdk|^//(adapters|imaging/graph|imaging/operations|imaging/cpu)(/|:)'
fi
check //:addon '//(tetrodotoxin|puffer|toolchain)(/|:)'
