// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/dynamic/vector.hpp"

#include "godot_ttx/classes/class.hpp"
#include "godot_ttx/contracts/lifecycle.hpp"
#include "ttx/concept/policies/borrowed.hpp"
#include "ttx/semantic/realization/invocation.hpp"

namespace Godot::Extension::Classes {

// Instance owns a Borrowed provider answer and the operations bound during
// construction. Godot callbacks invoke those operations directly. Destruction
// releases the answer while the host import service keeps its code loaded.
class Instance {
 public:
  Instance(const Instance&) = delete;
  auto operator=(const Instance&) -> Instance& = delete;
  static auto create(Class& type, Ttx::Concept::Policies::Borrowed publication)
      -> Perimortem::Utility::Result<Instance*, Ttx::Data::Status>;
  ~Instance() {
    publication.release();
    type.release_instance();
  }
  auto get_invocation(U32 index) const
      -> const Ttx::Semantic::Realization::Invocation& {
    return bindings[index];
  }

  auto notify(S32 notification) -> void;

 private:
  Instance(
      Class& type,
      Ttx::Concept::Policies::Borrowed publication,
      Perimortem::Memory::Dynamic::Vector<
          Ttx::Semantic::Realization::Invocation> bindings,
      Perimortem::Core::Option<::Godot::Extension::Contracts::Lifecycle>
          lifecycle)
      : type(type),
        publication(publication),
        bindings(Perimortem::Core::Data::take(bindings)),
        lifecycle(lifecycle) {}
  Class& type;
  Ttx::Concept::Policies::Borrowed publication;
  Perimortem::Memory::Dynamic::Vector<Ttx::Semantic::Realization::Invocation>
      bindings;
  Perimortem::Core::Option<::Godot::Extension::Contracts::Lifecycle> lifecycle;
};

}  // namespace Godot::Extension::Classes
