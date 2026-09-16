// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <cuda.h>

#include "perimortem/core/view/bytes.hpp"

#include "cuda/runtime/program.hpp"

namespace Godot::Imaging::Cuda {

// The native image algorithms refer to fixed kernel names. This facade lends
// that convention over the same compiled Program owner used by project code.
// It keeps the bundled source as a default publication rather than maintaining
// another compiler or device lifetime implementation.
//
// Launch queues work for the enclosing image operation to finish. Reset
// releases this program's context reference after its image resources have
// finished.
class Program {
 public:
  Program() = default;
  Program(const Program&) = delete;
  auto operator=(const Program&) -> Program& = delete;
  ~Program();

  auto prepare(CUdevice device) -> Perimortem::Core::View::Bytes;
  auto launch(const char* entry, U32 count, void** arguments)
      -> Perimortem::Core::View::Bytes;
  void reset();

 private:
  ::Cuda::Runtime::Program* program = nullptr;
  Perimortem::Memory::Dynamic::Bytes error;
};

}  // namespace Godot::Imaging::Cuda
