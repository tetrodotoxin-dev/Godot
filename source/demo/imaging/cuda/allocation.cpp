// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/imaging/cuda/allocation.hpp"

using namespace Godot::Demo;

Imaging::Cuda::Allocation::Allocation(Runtime& runtime, CUdeviceptr data)
    : runtime(&runtime), data(data) {
  runtime.retain();
}

Imaging::Cuda::Allocation::Allocation(Allocation&& other)
    : runtime(other.runtime), data(other.data) {
  other.runtime = nullptr;
}

Imaging::Cuda::Allocation::~Allocation() {
  if (runtime) {
    runtime->free_image(data);
    runtime->release();
  }
}
