// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/dynamic/vector.hpp"

#include "extension/classes/class.hpp"
#include "extension/contracts/lifecycle.hpp"
#include "ttx/semantic/ownership/publication.hpp"
#include "ttx/semantic/realization/invocation.hpp"

namespace Godot::Extension::Classes {

// Instance owns runtime state and the callable bindings acquired during its
// construction. The declaring graph has already disappeared. Godot callbacks
// use the ready slots, then destroy this publication while its Class still
// retains the provider's code and factory.
class Instance {
 public:
  static auto create(
      Class& type,
      Ttx::Semantic::Ownership::Publication publication)
      -> Perimortem::Utility::Result<Instance*, Ttx::Data::Status>;
  ~Instance() { type.release_instance(); }
  auto get_invocation(U32 index) const
      -> const Ttx::Semantic::Realization::Invocation& {
    return bindings[index];
  }

  auto notify(S32 notification) -> void;

 private:
  Instance(
      Class& type,
      Ttx::Semantic::Ownership::Publication publication,
      Perimortem::Memory::Dynamic::Vector<
          Ttx::Semantic::Realization::Invocation> bindings,
      Perimortem::Core::Option<::Godot::Extension::Contracts::Lifecycle>
          lifecycle)
      : type(type),
        publication(Perimortem::Core::Data::take(publication)),
        bindings(Perimortem::Core::Data::take(bindings)),
        lifecycle(lifecycle) {}
  Class& type;
  Ttx::Semantic::Ownership::Publication publication;
  Perimortem::Memory::Dynamic::Vector<Ttx::Semantic::Realization::Invocation>
      bindings;
  Perimortem::Core::Option<::Godot::Extension::Contracts::Lifecycle> lifecycle;
};

}  // namespace Godot::Extension::Classes
