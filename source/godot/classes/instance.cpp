// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "godot/classes/instance.hpp"

#include "godot/classes/class.hpp"

#include "godot_cpp/variant/utility_functions.hpp"

using namespace Ttx::Data;
using namespace Ttx::Graph;
using namespace Ttx::Semantic;
using namespace Godot::Extension;

Classes::Instance::~Instance() {
  type.release_instance();
  if (publication.release() != Policies::Retained::Status::Success) {
    godot::UtilityFunctions::push_error("TTX instance cleanup failed.");
  }
}

auto Classes::Instance::invoke(
    const Method& method,
    const void* input,
    void* output) -> Status {
  return publication.get_abstract()
      .resolve_route(method.get_route())
      .bind<Capabilities::Callable>()
      .visit(
          [&](Capabilities::Callable callable) {
            return callable.get_invocation().visit(
                [&](Realization::Invocation invocation) {
                  if (!invocation.accepts(
                          method.get_inputs().get_representation(),
                          method.get_outputs().get_representation())) {
                    return Status::Incompatible;
                  }

                  return invocation.invoke(input, output);
                },

                [](Negotiation::Status status) { return Status::Unsupported; });
          },

          [] { return Status::Unsupported; });
}
