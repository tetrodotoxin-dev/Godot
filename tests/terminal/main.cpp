// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include <dlfcn.h>
#include <stdlib.h>

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/time.hpp"
#include "perimortem/core/writer/textual.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "extensions/counter/inspection.hpp"
#include "extensions/sampling/contracts.h"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/answers/none.hpp"
#include "ttx/concept/answers/unknown.hpp"
#include "ttx/concept/declarations/callable.hpp"
#include "ttx/concept/declarations/extensible.hpp"
#include "ttx/concept/modules/import.hpp"
#include "ttx/concept/modules/module.hpp"
#include "ttx/semantic/ownership/factory.hpp"
#include "ttx/semantic/realization/invocation.hpp"

using namespace Ttx;
using namespace Perimortem;

static void require(Bool value, Core::View::Bytes message) {
  if (!value) {
    Core::Diagnostics::Log::fatal(message);
  }
}

template <typename Value, typename Error>
static auto accepted(Utility::Result<Value, Error> result) -> Value {
  return result.visit(
      [](Value& value) -> Value { return static_cast<Value&&>(value); },
      [](Error) -> Value {
        Core::Diagnostics::Log::fatal(
            "Terminal proof failed to acquire a required interface."_view);
      });
}

// The source owns the description, so retain only its operation identity and
// copied frame bytes. The same negotiated invocation can then run after every
// declaration callback and graph allocation has been released.
class Method {
 public:
  explicit Method(Concept::Declarations::Callable::Description description)
      : contract(description.get_contract()),
        inputs(description.get_inputs().get_representation().get_bytes()),
        outputs(description.get_outputs().get_representation().get_bytes()) {}

  auto connect(
      Semantic::Negotiation::Query instance,
      Semantic::Realization::Invocation& call) const -> void {
    const Data::Form::Representation input(
        inputs.get_view().get_data(), inputs.get_size());
    const Data::Form::Representation output(
        outputs.get_view().get_data(), outputs.get_size());
    require(
        call.connect(instance, contract, input, output) ==
            Semantic::Negotiation::Binding::Status::Satisfied,
        "Runtime did not agree to the published data frames."_view);
  }

 private:
  System::Uuid contract;
  Memory::Dynamic::Bytes inputs;
  Memory::Dynamic::Bytes outputs;
};

static auto inspect(Godot::Extensions::Counter::Inspection inspection)
    -> counter_statistics {
  return inspection.snapshot();
}

static auto loaded(const char* path) -> bool {
  void* handle = dlopen(path, RTLD_NOW | RTLD_NOLOAD);
  if (!handle) {
    return false;
  }

  dlclose(handle);
  return true;
}

static void policy(
    const Concept::Modules::Module& module,
    const char* selection,
    Semantic::Negotiation::Binding::Failure expected) {
  setenv("TTX_COUNTER_POLICY", selection, 1);
  auto discovery = accepted(module.open());
  Ttx::Concept::Abstract root = discovery;
  auto surface = root.resolve_concept("Counter"_view);
  require(
      surface.bind<Concept::Declarations::Extensible>().visit(
          [](const Concept::Declarations::Extensible&) { return False; },
          [&](Semantic::Negotiation::Binding::Failure actual) {
            return Bool(actual == expected);
          }),
      "An exported policy did not preserve its binding outcome."_view);
  // The source intentionally makes resolve expose a more permissive referent.
  // A consumer must bind the encountered policy rather than taking that route.
  accepted(surface.resolve().bind<Concept::Declarations::Extensible>());
  unsetenv("TTX_COUNTER_POLICY");
}

// Growth must transfer exclusive publication owners, not duplicate releases.
// The stack counters survive every relocation and reveal early or double close.
static void move_only_growth() {
  U32 releases[64] = {};
  {
    Memory::Dynamic::Vector<Semantic::Ownership::Publication> values;
    for (U32 index = 0; index < 64; ++index) {
      values.emplace(
          Semantic::Ownership::Publication(
              {{releases + index,
                [](const void*, perimortem_uuid, ttx_storage)
                    -> ttx_binding_status { return TTX_BINDING_UNSUPPORTED; }},
               [](const void* source) {
                 ++*const_cast<U32*>(static_cast<const U32*>(source));
               }}));
      require(
          releases[index] == 0,
          "A moved publication closed during growth."_view);
    }

    for (U32 count : releases) {
      require(count == 0, "Relocation released an owner."_view);
    }
  }

  for (U32 count : releases) {
    require(count == 1, "Publication was not released exactly once."_view);
  }
}

// The C++ provider gets the same service a Godot host supplies, but this host
// resolves only one alias. No Godot path, Variant or class registration enters
// either the provider project or this consumer.
static auto imports(Core::View::Bytes& cpu) -> Semantic::Negotiation::Query {
  return Semantic::Negotiation::Query(
      {&cpu,
       [](const void* source, perimortem_uuid id,
          ttx_storage requested) -> ttx_binding_status {
         if (System::Uuid(id) != Concept::Modules::Import::contract_id) {
           return TTX_BINDING_UNSUPPORTED;
         }

         static const ttx_import_operations operations = {
           [](const void* source, perimortem_view_bytes name,
              ttx_module* output) -> ttx_data_status {
             if (Core::View::Bytes(name.data, name.size) != "cpu"_view) {
               return TTX_DATA_UNSUPPORTED;
             }

             Memory::Allocator::Arena errors;
             return Concept::Modules::Module::load(
                        *static_cast<const Core::View::Bytes*>(source), errors)
                 .visit(
                     [&](Concept::Modules::Module& module) -> ttx_data_status {
                       *output = module.take();
                       return TTX_DATA_SUCCESS;
                     },
                     [](Core::View::Bytes) -> ttx_data_status {
                       return TTX_DATA_IO_ERROR;
                     });
           },
         };
         const ttx_import api = {source, &operations};
         return ttx_binding_provide(
             ttx_import_representation(), &api, requested);
       },
       [](const void*, perimortem_uuid id) -> ttx_binding_status {
         return System::Uuid(id) == Concept::Modules::Import::contract_id
                    ? TTX_BINDING_SATISFIED
                    : TTX_BINDING_UNSUPPORTED;
       }});
}

static void sampling(Core::View::Bytes path, Core::View::Bytes cpu) {
  Memory::Allocator::Arena errors;
  auto module = accepted(Concept::Modules::Module::load(path, errors));
  auto graph = accepted(module.open(imports(cpu)));
  Ttx::Concept::Abstract root = graph;
  auto declaration = root.resolve_concept("Sampler"_view);
  auto operation = [&](Core::View::Bytes name) {
    return Method(
        accepted(accepted(declaration.resolve_concept(name)
                              .bind<Concept::Declarations::Callable>())
                     .describe()));
  };
  const auto configure_method = operation("configure"_view);
  const auto count_method = operation("count"_view);
  const auto error_method = operation("get_error"_view);
  auto factory_publication =
      accepted(accepted(declaration.bind<Concept::Declarations::Extensible>())
                   .emit_factory());
  auto factory = accepted(
      factory_publication.get_query().bind<Semantic::Ownership::Factory>());

  // Runtime owns its receiver independently. Negotiation consumes the copied
  // forms and supplies a callable record through Data Flow before any call.
  graph.close();
  auto instance = accepted(factory.create());
  Semantic::Realization::Invocation configure;
  Semantic::Realization::Invocation count;
  Semantic::Realization::Invocation error;
  configure_method.connect(instance.get_query(), configure);
  count_method.connect(instance.get_query(), count);
  error_method.connect(instance.get_query(), error);

  const auto named = [](Core::View::Bytes text) {
    return perimortem_view_bytes{text.get_data(), text.get_size()};
  };
  auto provider = named("cpu"_view);
  U8 configured = 0;
  require(
      configure.invoke(&provider, &configured) == Data::Status::Success &&
          configured,
      "Injected CPU import failed."_view);
  const sampler_count_input arguments{0, 0, 17};
  S64 answer = 0;
  require(
      count.invoke(&arguments, &answer) == Data::Status::Success &&
          answer == 15,
      "Independent C++ sampler returned the wrong answer."_view);

  provider = named("absent"_view);
  perimortem_view_bytes message = {};
  require(
      configure.invoke(&provider, &configured) == Data::Status::Success &&
          !configured &&
          error.invoke(nullptr, &message) == Data::Status::Success &&
          message.size != 0,
      "Missing import did not report failure."_view);
  require(
      count.invoke(&arguments, &answer) == Data::Status::Success &&
          answer == 15,
      "Failed replacement lost the previous callable."_view);
  Core::Diagnostics::Log::info(
      "PASS C++ terminal: released graph, injected import and retained backend after rejection\n"_view);
}

int main(int argc, char** argv) {
  Core::Diagnostics::Log::set_sink(Core::Diagnostics::Log::stderr_sink);
  move_only_growth();
  require(
      argc == 2 || argc == 4,
      "Supply counter, optionally followed by sampler and CPU modules."_view);
  {
    Memory::Allocator::Arena errors;
    auto module = accepted(
        Concept::Modules::Module::load(
            Core::NullTerminated::to_view(argv[1]), errors));
    policy(module, "deny", Semantic::Negotiation::Binding::Failure::Rejected);
    policy(module, "pending", Semantic::Negotiation::Binding::Failure::Pending);
    policy(
        module, "unsupported",
        Semantic::Negotiation::Binding::Failure::Unsupported);

    auto discovery = accepted(module.open());
    Ttx::Concept::Abstract root = discovery;
    accepted(
        root.resolve_concept("missing"_view).bind<Concept::Answers::None>());
    accepted(
        Concept::Abstract(ttx_unknown()).bind<Concept::Answers::Unknown>());
    auto surface = root.resolve_concept("Counter"_view);
    Core::Option<Method> increment;
    U32 count = 0;
    auto visitor = [&](Core::View::Bytes name, Concept::Abstract member) {
      auto description = accepted(
          accepted(member.bind<Concept::Declarations::Callable>()).describe());
      if (name == "advance"_view) {
        increment = Method(description);
        require(
            description.get_inputs().get_size() == 1 &&
                description.get_outputs().get_size() == 1,
            "Counter's published frames differ."_view);
      }

      ++count;
    };
    surface.visit_concepts(Concept::Abstract::Visitor(visitor));
    require(count == 4, "Counter discovery lost members."_view);
    auto factory_publication =
        accepted(accepted(surface.bind<Concept::Declarations::Extensible>())
                     .emit_factory());
    auto factory = accepted(
        factory_publication.get_query().bind<Semantic::Ownership::Factory>());
    auto inspection =
        accepted(factory_publication.get_query()
                     .bind<Godot::Extensions::Counter::Inspection>());

    // Only copied frame bytes, the operation UUID and independently emitted
    // factory cross this boundary. No Abstract or borrowed description
    // survives.
    discovery.close();
    const auto released = inspect(inspection);
    require(
        released.graphs_opened == released.graphs_closed,
        "Discovery is still retained."_view);
    const auto construction_started = Core::Time::now();
    for (U32 index = 0; index < 10000; ++index) {
      auto temporary = accepted(factory.create());
    }

    const auto construction_elapsed =
        construction_started.measure().convert_to_nanoseconds();
    auto instance = accepted(factory.create());
    Semantic::Realization::Invocation advance;
    increment.visit(
        [] {
          Core::Diagnostics::Log::fatal(
              "Counter did not publish advance."_view);
        },
        [&](const Method& method) {
          method.connect(instance.get_query(), advance);
        });
    S64 amount = 40;
    S64 answer = 0;
    require(
        advance.invoke(&amount, &answer) == Data::Status::Success &&
            answer == 40,
        "The emitted operation returned the wrong answer."_view);
    amount = 2;
    require(
        advance.invoke(&amount, &answer) == Data::Status::Success &&
            answer == 42,
        "The emitted operation lost its receiver."_view);

    const auto before = inspect(inspection);
    const auto allocations = Core::Bibliotheca::check_out_requests();
    const auto started = Core::Time::now();
    amount = 1;
    bool succeeded = true;
    for (U32 index = 0; index < 1000000; ++index) {
      succeeded &= advance.invoke(&amount, &answer) == Data::Status::Success;
    }

    const auto elapsed = started.measure().convert_to_nanoseconds();
    const auto after = inspect(inspection);
    require(
        succeeded && answer == 1000042,
        "Repeated invocation changed the counter result."_view);
    require(
        after.graph_queries == released.graph_queries &&
            after.binds == before.binds,
        "Runtime calls queried a graph or renegotiated a binding."_view);
    require(
        Core::Bibliotheca::check_out_requests() == allocations,
        "Runtime dispatch allocated storage."_view);
    advance.close();
    instance.close();
    require(
        inspect(inspection).instances_opened ==
            inspect(inspection).instances_closed,
        "Instance release did not run."_view);
    factory_publication.close();
    require(
        loaded(argv[1]),
        "Code was unloaded while its module owner remained live."_view);

    Core::Static::Bytes<512> output;
    Core::Writer::Textual text(output.get_access());
    text
        << "PASS terminal: graph released, policy preserved, 1000000 negotiated calls in "_view
        << U64(elapsed) << " ns, no binds, graph queries or allocations\n"_view
        << "10000 runtime create/release pairs in "_view
        << U64(construction_elapsed) << " ns\n"_view;
    Core::Diagnostics::Log::info(Core::View::Bytes(text));
  }

  require(
      !loaded(argv[1]),
      "Module code remained loaded after its final owner."_view);
  if (argc == 4) {
    sampling(
        Core::NullTerminated::to_view(argv[2]),
        Core::NullTerminated::to_view(argv[3]));
  }

  return 0;
}
