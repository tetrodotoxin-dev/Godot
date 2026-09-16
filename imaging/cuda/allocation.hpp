// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "imaging/cuda/runtime.hpp"

namespace Godot::Imaging::Cuda {

// A device pointer alone cannot release itself after its creating factory is
// gone. Allocation retains Runtime together with the buffer and frees the
// buffer before releasing that context owner. An operation holds its tentative
// output here until success transfers it into an Image. Early failure therefore
// follows the same cleanup path as ordinary image destruction.
class Allocation {
 public:
  Allocation(Runtime&, CUdeviceptr owned);
  Allocation(Allocation&&);
  ~Allocation();
  Allocation(const Allocation&) = delete;
  auto operator=(const Allocation&) -> Allocation& = delete;
  auto get() const -> CUdeviceptr { return data; }

 private:
  Runtime* runtime;
  CUdeviceptr data;
};

}  // namespace Godot::Imaging::Cuda
