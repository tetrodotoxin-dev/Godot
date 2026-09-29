// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "demo/sampling/contracts/samples.hpp"
#include "ttx/concept/policies/borrowed.hpp"
#include "ttx/semantic/negotiation/library.hpp"

namespace Godot::Demo::Sampling {

// Function acquires the computation through Borrow and keeps its Samples API
// for repeated calls. A configured import service supplies the code lifetime.
// Direct native loading keeps a Library here and releases the computation
// before closing it.
class Function {
 public:
  static auto open(
      Perimortem::Core::View::Bytes path,
      Perimortem::Memory::Allocator::Arena& errors)
      -> Perimortem::Utility::Result<Function, Perimortem::Core::View::Bytes>;
  static auto open(
      Ttx::Semantic::Negotiation::Library module,
      Ttx::Semantic::Negotiation::Query host =
          Ttx::Semantic::Negotiation::Query())
      -> Perimortem::Utility::Result<Function, Perimortem::Core::View::Bytes>;
  static auto open(Ttx::Semantic::Negotiation::Query subject)
      -> Perimortem::Utility::Result<Function, Perimortem::Core::View::Bytes>;
  Function(Function&& other);
  Function(const Function&) = delete;
  auto operator=(const Function&) -> Function& = delete;
  ~Function();
  auto get_handle() const -> Sampling::Contracts::Samples { return handle; }

 private:
  Function(
      Perimortem::Core::Option<Ttx::Semantic::Negotiation::Library> module,
      Ttx::Concept::Policies::Borrowed publication,
      Sampling::Contracts::Samples handle);
  Perimortem::Core::Option<Ttx::Semantic::Negotiation::Library> module;
  Perimortem::Core::Option<Ttx::Concept::Policies::Borrowed> publication;
  Sampling::Contracts::Samples handle;
};

}  // namespace Godot::Demo::Sampling
