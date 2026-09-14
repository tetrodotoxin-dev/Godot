// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <cuda.h>

#include "perimortem/core/view/bytes.hpp"

namespace Godot::Providers::Cuda {

// The provider ships one CUDA program while device architecture is chosen on
// the deployment machine. Program compiles the embedded source through NVRTC,
// loads its executable module and launches named kernels. This separates code
// preparation from Runtime's allocation and context lifetime responsibilities.
//
// Its context is borrowed from Runtime. Runtime resets Program before releasing
// that context, including after partial factory preparation. Launch only queues
// device work. The enclosing image operation completes it before publishing.
class Program {
 public:
  Program() = default;
  Program(const Program&) = delete;
  auto operator=(const Program&) -> Program& = delete;
  ~Program();

  auto prepare(CUdevice device, CUcontext context)
      -> Perimortem::Core::View::Bytes;
  auto launch(const char* entry, U32 count, void** arguments)
      -> Perimortem::Core::View::Bytes;
  void reset();

 private:
  CUcontext context = nullptr;
  CUmodule module = nullptr;
};

}  // namespace Godot::Providers::Cuda
