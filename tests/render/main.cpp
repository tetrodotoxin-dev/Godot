// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "imaging/contracts/render.h"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/declarations/callable.hpp"
#include "ttx/concept/declarations/extensible.hpp"
#include "ttx/concept/modules/module.hpp"
#include "ttx/semantic/ownership/factory.hpp"
#include "ttx/semantic/realization/invocation.hpp"

using namespace Perimortem;
using namespace Ttx;

static auto require(Bool condition, Core::View::Bytes message) -> void {
  if (!condition) {
    Core::Diagnostics::Log::fatal(message);
  }
}

template <typename T, typename E>
static auto accepted(Utility::Result<T, E> result) -> T {
  return result.visit(
      [](T& value) -> T { return Core::Data::take(value); },
      [](E) -> T {
        Core::Diagnostics::Log::fatal(
            "Render contract was not satisfied."_view);
      });
}

// The consumer owns just the callable identity and its copied frame bytes.
// No declaration pointer survives the explicit discovery release below.
struct Method {
  System::Uuid contract;
  Memory::Dynamic::Bytes inputs;
  Memory::Dynamic::Bytes outputs;
  explicit Method(Concept::Declarations::Callable::Description description)
      : contract(description.get_contract()),
        inputs(description.get_inputs().get_representation().get_bytes()),
        outputs(description.get_outputs().get_representation().get_bytes()) {}

  auto connect(
      Semantic::Realization::Invocation& call,
      Semantic::Negotiation::Query instance) const -> void {
    const Data::Form::Representation input(
        inputs.get_view().get_data(), inputs.get_size());
    const Data::Form::Representation output(
        outputs.get_view().get_data(), outputs.get_size());
    require(
        call.connect(instance, contract, input, output) ==
            Semantic::Negotiation::Binding::Status::Satisfied,
        "Render runtime did not fulfill the copied description."_view);
  }
};

int main(int argc, char** argv) {
  Core::Diagnostics::Log::set_sink(Core::Diagnostics::Log::stderr_sink);
  require(argc == 2, "Supply one CPU or CUDA module."_view);
  Memory::Allocator::Arena errors;
  auto module = accepted(
      Concept::Modules::Module::load(
          Core::NullTerminated::to_view(argv[1]), errors));
  auto discovery = accepted(module.open());
  Ttx::Concept::Abstract root = discovery;
  auto declaration =
      root.resolve_concept("Imaging"_view).resolve_concept("Render"_view);
  const Core::View::Bytes names[] = {"upload"_view,     "invert"_view,
                                     "pixels"_view,     "get_width"_view,
                                     "get_height"_view, "get_error"_view};
  Memory::Dynamic::Vector<Method> methods;
  for (auto name : names) {
    methods.emplace(
        Method(accepted(accepted(declaration.resolve_concept(name)
                                     .bind<Concept::Declarations::Callable>())
                            .describe())));
  }
  auto factory =
      accepted(accepted(declaration.bind<Concept::Declarations::Extensible>())
                   .emit_factory());
  discovery.close();

  // This is the same artifact Godot loads. A second host can construct and
  // call it after discarding the graph, without Godot headers or registrations.
  auto instance = accepted(
      accepted(factory.get_query().bind<Semantic::Ownership::Factory>())
          .create());
  Semantic::Realization::Invocation calls[6];
  for (Count i = 0; i != 6; ++i) {
    methods[i].connect(calls[i], instance.get_query());
  }
  const U8 pixels[] = {10, 20, 30, 255, 40, 50, 60, 127};
  const render_upload upload{2, 1, {pixels, sizeof(pixels)}};
  U8 succeeded = 0;
  require(
      calls[0].invoke(&upload, &succeeded) == Data::Status::Success &&
          succeeded,
      "Render upload failed."_view);
  S64 width = 0;
  require(
      calls[3].invoke(nullptr, &width) == Data::Status::Success && width == 2,
      "Render dimensions disagree."_view);
  require(
      calls[1].invoke(nullptr, &succeeded) == Data::Status::Success &&
          succeeded,
      "Render inversion failed."_view);
  perimortem_view_bytes observed = {};
  require(
      calls[2].invoke(nullptr, &observed) == Data::Status::Success,
      "Render pixel observation failed."_view);
  const U8 expected[] = {245, 235, 225, 255, 215, 205, 195, 127};
  require(
      observed.size == sizeof(expected) &&
          Core::Data::compare(observed.data, expected, sizeof(expected)),
      "Independent pixel oracle disagrees."_view);

  const render_upload invalid{-1, 1, {pixels, sizeof(pixels)}};
  require(
      calls[0].invoke(&invalid, &succeeded) == Data::Status::Success &&
          !succeeded,
      "Invalid upload was accepted."_view);
  require(
      calls[2].invoke(nullptr, &observed) == Data::Status::Success &&
          Core::Data::compare(observed.data, expected, sizeof(expected)),
      "Failed upload replaced the previous image."_view);
  Core::Diagnostics::Log::info(
      "PASS Render: common module, released discovery, runtime frames and pixels\n"_view);
  return 0;
}
