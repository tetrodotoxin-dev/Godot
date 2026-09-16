// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "sampling/contracts/samples.hpp"
#include "ttx/concept/modules/module.hpp"

namespace Godot::Sampling {

// Function retains one module and runtime publication beside its fulfilled
// Samples binding. An injected importer can supply the module, while a native
// command line owner can load a path directly. Neither choice enters the hot
// count operation, and publication release always precedes code release.
class Function {
 public:
  static auto open(
      Perimortem::Core::View::Bytes path,
      Perimortem::Memory::Allocator::Arena& errors)
      -> Perimortem::Utility::Result<Function, Perimortem::Core::View::Bytes>;
  static auto open(
      Ttx::Concept::Modules::Module module,
      Ttx::Semantic::Negotiation::Query host =
          Ttx::Semantic::Negotiation::Query())
      -> Perimortem::Utility::Result<Function, Perimortem::Core::View::Bytes>;
  Function(Function&& other);
  Function(const Function&) = delete;
  auto operator=(const Function&) -> Function& = delete;
  ~Function() = default;
  auto get_handle() const -> Sampling::Contracts::Samples { return handle; }

 private:
  Function(
      Ttx::Concept::Modules::Module module,
      Ttx::Concept::Modules::Module::Acquisition publication,
      Sampling::Contracts::Samples handle);
  Ttx::Concept::Modules::Module module;
  Ttx::Concept::Modules::Module::Acquisition publication;
  Sampling::Contracts::Samples handle;
};

}  // namespace Godot::Sampling
