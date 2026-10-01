// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <cuda.h>

#include "perimortem/core/object.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/utility/result.hpp"

#include "demo/imaging/cuda/fourier.hpp"
#include "demo/imaging/cuda/program.hpp"

namespace Godot::Demo::Imaging::Cuda {

// Device images can outlive the factory call and the Godot Resource that first
// uploaded them. Runtime retains their CUDA context and executable program for
// that shared lifetime. Each Allocation keeps Runtime alive until its buffer
// has been freed. FFT plans and scratch are released before the context ends.
//
// CUDA's current context is thread state. Operations install this context for
// their own work and restore the caller's context afterward, allowing other GPU
// users to coexist on the same worker. These APIs are synchronous and worker
// confined. Retaining Runtime does not make concurrent access safe.
class Runtime {
 public:
  static auto create()
      -> Perimortem::Utility::Result<Runtime&, Perimortem::Core::View::Bytes>;
  void retain();
  void release();
  auto allocate(Count) -> Perimortem::Utility::
      Result<CUdeviceptr, Perimortem::Core::View::Bytes>;
  auto upload(Perimortem::Core::View::Bytes) -> Perimortem::Utility::
      Result<CUdeviceptr, Perimortem::Core::View::Bytes>;
  auto read(CUdeviceptr source, Perimortem::Core::Access::Bytes target)
      -> Perimortem::Core::View::Bytes;
  auto write(CUdeviceptr, Perimortem::Core::View::Bytes)
      -> Perimortem::Core::View::Bytes;
  auto finish() -> Perimortem::Core::View::Bytes;
  void free_image(CUdeviceptr);
  auto get_context() const -> CUcontext { return context; }
  auto get_fourier() -> Fourier& { return fourier; }
  auto get_program() -> Program& { return program; }
  auto get_uploads() const -> U64 { return uploads; }
  auto get_downloads() const -> U64 { return downloads; }
  auto get_live_images() const -> U64 { return live_images; }
  auto get_plan_builds() const -> U64 { return fourier.get_builds(); }

 private:
  Runtime() = default;
  ~Runtime();
  auto prepare() -> Perimortem::Core::View::Bytes;
  CUdevice device = 0;
  CUcontext context = nullptr;
  Program program;
  Fourier fourier;
  U64 uploads = 0;
  U64 downloads = 0;
  U64 live_images = 0;
};

}  // namespace Godot::Demo::Imaging::Cuda
