// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "contracts/samples.hpp"
#include "modules/library.hpp"
#include "sampling/provider.h"

namespace Godot::Sampling {

// A sampling function retains the supplying module and publication once, then
// lends its already fulfilled callable. Both remain valid across every count.
// Releasing provider state before the Library closes keeps its destruction code
// available. The function is worker confined, including its final destruction.
class Function {
 public:
  static auto open(
      Perimortem::Core::View::Bytes path,
      Perimortem::Memory::Allocator::Arena& errors)
      -> Perimortem::Utility::Result<Function, Perimortem::Core::View::Bytes>;
  Function(Function&& other);
  Function(const Function&) = delete;
  auto operator=(const Function&) -> Function& = delete;
  ~Function();
  auto get_handle() const -> Contracts::Samples::Handle { return handle; }

 private:
  // Admission must either construct this private lifetime owner or release the
  // transferred publication. Keeping that step here makes the obligation
  // visible without exposing a constructor for an unfulfilled function.
  static auto admit(Modules::Library&& library, sample_provider provider)
      -> Perimortem::Utility::Result<Function, Perimortem::Core::View::Bytes>;

  Function(
      Modules::Library&& library,
      sample_provider provider,
      Contracts::Samples::Handle handle);
  Modules::Library library;
  sample_provider provider;
  Contracts::Samples::Handle handle;
};

}  // namespace Godot::Sampling
