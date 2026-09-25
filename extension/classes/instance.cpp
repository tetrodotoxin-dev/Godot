// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "extension/classes/instance.hpp"

#include <godot_cpp/classes/node.hpp>

using namespace Godot::Extension;
using namespace Perimortem;

auto Godot::Extension::Classes::Instance::create(
    Class& type,
    Ttx::Semantic::Ownership::Publication publication)
    -> Utility::Result<Instance*, Ttx::Data::Status> {
  // A registered class promises that every instance can supply these methods.
  // Fulfill all slots before attaching an instance to Godot so a late refusal
  // releases the runtime publication without exposing a partly usable object.
  Memory::Dynamic::Vector<Ttx::Semantic::Realization::Invocation> bindings;
  const auto methods = type.get_methods();
  for (Count index = 0; index < methods.get_size(); ++index) {
    const auto& method = methods.get_data()[index];
    Ttx::Semantic::Realization::Invocation invocation;
    const auto input = method.get_inputs().get_representation();
    const auto output = method.get_outputs().get_representation();
    const auto status = invocation.connect(
        publication.get_query(), method.get_contract(), input, output);
    if (status != Ttx::Semantic::Negotiation::Binding::Status::Satisfied) {
      return Ttx::Data::Status::Incompatible;
    }

    bindings.emplace(Core::Data::take(invocation));
  }

  // Computation alone does not imply scene behavior. Only Node instances ask
  // for this optional policy, and a provisional answer cannot fall through to
  // an underlying implementation that might expose more than was permitted.
  Core::Option<::Godot::Extension::Contracts::Lifecycle> lifecycle;
  if (type.is_node()) {
    auto status =
        publication.get_query()
            .bind<::Godot::Extension::Contracts::Lifecycle>()
            .visit(
                [&](::Godot::Extension::Contracts::Lifecycle value) {
                  lifecycle = value;
                  return Ttx::Data::Status::Success;
                },
                [](Ttx::Semantic::Negotiation::Binding::Failure failure) {
                  return failure == Ttx::Semantic::Negotiation::Binding::
                                        Failure::Unsupported
                             ? Ttx::Data::Status::Success
                             : Ttx::Data::Status::Denied;
                });
    if (status != Ttx::Data::Status::Success) {
      return status;
    }
  }

  auto memory = Core::Bibliotheca::check_out(sizeof(Instance));
  return new (memory.ptr, Core::Placement::Construct) Instance(
      type, Core::Data::take(publication), Core::Data::take(bindings),
      lifecycle);
}

auto Godot::Extension::Classes::Instance::notify(S32 notification) -> void {
  if (!lifecycle) {
    return;
  }

  // Engine notification numbers belong here. The optional TTX policy receives
  // named lifecycle observations while the underlying computation stays unaware
  // of Godot's numbering and native Node representation.
  if (notification == godot::Node::NOTIFICATION_ENTER_TREE) {
    lifecycle->entered();
  } else if (notification == godot::Node::NOTIFICATION_READY) {
    lifecycle->ready();
  }
}
