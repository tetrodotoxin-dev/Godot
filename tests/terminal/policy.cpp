// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include <stdlib.h>

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "extensions/counter/inspection.hpp"
#include "gdextension/classes/gd_class.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/declarations/callable.hpp"
#include "ttx/concept/declarations/extensible.hpp"

using namespace Godot;
using namespace Perimortem;

static void require(Bool result) {
  if (!result) {
    Core::Diagnostics::Log::fatal("GDClass policy forwarding failed."_view);
  }
}

template <typename T, typename E>
static auto accepted(Utility::Result<T, E> result) -> T {
  return result.visit(
      [](T& value) -> T { return Core::Data::take(value); },
      [](E) -> T {
        Core::Diagnostics::Log::fatal("Policy fixture could not bind."_view);
      });
}

int main(int argc, char** argv) {
  require(argc == 2);
  Memory::Allocator::Arena errors;
  auto module = accepted(
      Ttx::Concept::Modules::Module::load(
          Core::NullTerminated::to_view(argv[1]), errors));
  const char* selections[] = {"deny", "pending", "unsupported"};
  const Ttx::Semantic::Negotiation::Binding::Failure outcomes[] = {
    Ttx::Semantic::Negotiation::Binding::Failure::Rejected,
    Ttx::Semantic::Negotiation::Binding::Failure::Pending,
    Ttx::Semantic::Negotiation::Binding::Failure::Unsupported,
  };
  for (U32 index = 0; index < 3; ++index) {
    setenv("TTX_COUNTER_POLICY", selections[index], 1);
    auto discovery = accepted(module.open());
    Ttx::Concept::Abstract root = discovery;
    auto source = root.resolve_concept("Counter"_view);
    Gdextension::Classes::GDClass policy(
        source, module, "Counter"_view, "Node"_view);
    auto wrapped = policy.get_interface();
    const auto before_support =
        accepted(source.bind<Extensions::Counter::Inspection>()).snapshot();
    require(
        wrapped.supports<::Gdextension::Contracts::GDClass>() ==
        Ttx::Semantic::Negotiation::Binding::Status::Satisfied);
    require(
        wrapped.supports<Ttx::Concept::Declarations::Extensible>() ==
        static_cast<Ttx::Semantic::Negotiation::Binding::Status>(
            outcomes[index]));
    require(
        wrapped.supports<Extensions::Counter::Inspection>() ==
        Ttx::Semantic::Negotiation::Binding::Status::Satisfied);
    const auto after_support =
        accepted(source.bind<Extensions::Counter::Inspection>()).snapshot();
    require(before_support.factories_opened == after_support.factories_opened);
    require(before_support.binds == after_support.binds);
    accepted(wrapped.bind<::Gdextension::Contracts::GDClass>());
    require(wrapped.bind<Ttx::Concept::Declarations::Extensible>().visit(
        [](Ttx::Concept::Declarations::Extensible) { return False; },
        [&](Ttx::Semantic::Negotiation::Binding::Failure failure) {
          return Bool(failure == outcomes[index]);
        }));

    // The extra capability is supplied by the original C policy. GDClass has
    // no knowledge of its operations or receiver and must forward it unchanged.
    auto expected =
        accepted(source.bind<Extensions::Counter::Inspection>()).get_abi();
    auto observed =
        accepted(wrapped.bind<Extensions::Counter::Inspection>()).get_abi();
    require(
        expected.source == observed.source &&
        expected.operations == observed.operations);
    require(wrapped.get_data() == source.get_data());
    require(wrapped.resolve().get_identity() == wrapped.get_identity());
    accepted(wrapped.resolve_concept("advance"_view)
                 .bind<Ttx::Concept::Declarations::Callable>());
  }

  unsetenv("TTX_COUNTER_POLICY");
  return 0;
}
