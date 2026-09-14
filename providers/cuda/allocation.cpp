// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "providers/cuda/allocation.hpp"

using namespace Godot;

Providers::Cuda::Allocation::Allocation(Runtime& runtime, CUdeviceptr data)
    : runtime(&runtime), data(data) {
  runtime.retain();
}

Providers::Cuda::Allocation::Allocation(Allocation&& other)
    : runtime(other.runtime), data(other.data) {
  other.runtime = nullptr;
}

Providers::Cuda::Allocation::~Allocation() {
  if (runtime) {
    runtime->free_image(data);
    runtime->release();
  }
}
