// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "imaging/cuda/runtime.hpp"
#include "sampling/publication.hpp"

namespace Godot::Sampling::Cuda {

// Scalar reduction reuses one device counter for the lifetime of a fulfilled
// function. Its mutable scratch is private implementation state, so callers
// still observe a pure function of seed and interval. Calls remain synchronous
// on the admitting worker, and only the final integer crosses back to the host.
class Samples {
 public:
  Samples(const Samples&) = delete;
  auto operator=(const Samples&) -> Samples& = delete;

  static auto create()
      -> Perimortem::Utility::Result<ttx_publication, Ttx::Data::Status>;
  auto count(U32 seed, U32 first, U32 size, U64* output) -> ttx_data_status;

 private:
  Samples(Godot::Imaging::Cuda::Runtime& runtime, CUdeviceptr counter);
  ~Samples();
  Godot::Imaging::Cuda::Runtime& runtime;
  CUdeviceptr counter;
  Sampling::Publication publication;
};

}  // namespace Godot::Sampling::Cuda
