// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <cuda.h>
#include <cufft.h>

#include "perimortem/core/view/bytes.hpp"

namespace Godot::Imaging::Cuda {

// Rebuilding cuFFT plans and scratch for every filter would make setup dominate
// small repeated operations. Fourier retains the last padded shape for its
// Runtime, reusing it until a different shape is requested. Its context is
// borrowed, so Runtime resets this workspace before releasing the context.
// Partially created resources remain owned here and are cleaned on retry or
// destruction. Dimensions are published only after preparation succeeds.
class Fourier {
 public:
  Fourier() = default;
  Fourier(const Fourier&) = delete;
  auto operator=(const Fourier&) -> Fourier& = delete;
  ~Fourier();
  auto prepare(CUcontext, U32 width, U32 height)
      -> Perimortem::Core::View::Bytes;
  auto forward() -> Perimortem::Core::View::Bytes;
  auto inverse() -> Perimortem::Core::View::Bytes;
  void reset();
  auto get_buffer() const -> CUdeviceptr { return scratch; }
  auto get_builds() const -> U64 { return builds; }

 private:
  CUcontext context = nullptr;
  CUdeviceptr scratch = 0;
  cufftHandle images = 0;
  cufftHandle kernel = 0;
  U32 width = 0;
  U32 height = 0;
  U64 builds = 0;
};

}  // namespace Godot::Imaging::Cuda
