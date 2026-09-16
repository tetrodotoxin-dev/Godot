// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/time.hpp"
#include "perimortem/core/writer/textual.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "imaging/contracts/invert.hpp"
#include "imaging/graph/call.hpp"
#include "imaging/graph/image.hpp"
#include "imaging/graph/provider.hpp"
#include "imaging/graph/source.hpp"
#include "imaging/operations/standard.hpp"
#include "ttx/data/form/compiler.hpp"
#include "ttx/semantic/realization/simulacra.hpp"

using namespace Godot;
using namespace Perimortem;

static void require(Bool condition, Core::View::Bytes message) {
  if (!condition) {
    Core::Diagnostics::Log::fatal(message);
  }
}

template <typename Value>
static auto accepted(Utility::Result<Value, Core::View::Bytes> result)
    -> Value {
  return result.visit(
      [](Value& value) -> Value { return static_cast<Value&&>(value); },
      [](Core::View::Bytes error) -> Value {
        Core::Diagnostics::Log::fatal(error);
      });
}

// Each action contributes a value that is checked after the timed batch. The
// provider lives in another module, so the compiler cannot replace its calls
// with a locally inferred answer. Clock reads and formatting sit outside the
// loop. Allocation requests describe Perimortem's allocator traffic, not OS
// allocations.
template <typename Action>
static void measure(Core::View::Bytes name, Count iterations, Action action) {
  require(
      action() == 1, "Mechanics warm call returned an invalid answer."_view);
  const auto requests = Core::Bibliotheca::check_out_requests();
  const auto started = Core::Time::now();
  U64 sum = 0;
  for (Count i = 0; i < iterations; ++i) {
    sum += action();
  }

  const auto elapsed = started.measure().convert_to_nanoseconds();
  const auto allocations = Core::Bibliotheca::check_out_requests() - requests;
  require(
      sum == iterations,
      "Mechanics loop did not observe every expected answer."_view);
  Core::Static::Bytes<512> text;
  Core::Writer::Textual writer(text);
  writer << "MECHANICS "_view << name << " iterations="_view << iterations
         << " ns="_view << R64(elapsed) / iterations
         << " allocation_requests="_view << R64(allocations) / iterations;
  Core::Diagnostics::Log::info(Core::View::Bytes(writer));
}

static auto bound(
    const Imaging::Graph::Image& image,
    const Imaging::Graph::Operation& operation) -> Imaging::Graph::Invocation {
  return image.bind_operation(operation).visit(
      [](Imaging::Graph::Invocation binding) { return binding; },
      [](Ttx::Semantic::Negotiation::Binding::Failure)
          -> Imaging::Graph::Invocation {
        Core::Diagnostics::Log::fatal(
            "Mechanics could not fulfill inversion."_view);
      });
}

static void measure_provider(Core::View::Bytes path, Count operations) {
  Core::Diagnostics::Log::info(path);
  Memory::Allocator::Arena errors;
  auto& provider = accepted(
      Imaging::Graph::Provider::open(
          path, Imaging::Operations::Standard::get_vocabulary(), errors));
  const U8 pixels[] = {1, 2, 3, 255};
  auto image = accepted(
      Imaging::Graph::Image::create(
          provider, 1, 1, Core::View::Bytes(pixels, 4), errors));
  const auto* operation = provider.get_vocabulary().find("invert"_view);
  require(operation != nullptr, "Missing inversion publication."_view);
  const auto binding = bound(image, *operation);
  const auto handle =
      image.get_query().bind<Imaging::Contracts::Invert>().visit(
          [](Imaging::Contracts::Invert value) { return value; },
          [](Ttx::Semantic::Negotiation::Binding::Failure failure)
              -> Imaging::Contracts::Invert {
            Core::Diagnostics::Log::fatal(
                Imaging::Graph::Image::binding_error(failure));
          });
  const auto control = image.get_abi();
  const auto query = image.get_query();

  // These two rows answer a stored dimension without constructing an image.
  // The first is a direct bootstrap table dispatch. The second includes the
  // host's observation policy. Neither performs UUID negotiation in its loop.
  measure("direct_control"_view, 10000000, [&]() -> U64 {
    return control.operations->dimensions(control.source).width;
  });
  measure("host_dimension"_view, 1000000, [&]() -> U64 {
    return image.get_width();
  });

  measure("core_fulfillment"_view, 100000, [&]() -> U64 {
    return Ttx::Semantic::Realization::Simulacra::fulfill<
               Imaging::Contracts::Invert>(query)
        .visit(
            [](const Imaging::Contracts::Invert&) -> U64 { return 1; },
            [](Ttx::Semantic::Negotiation::Binding::Failure) -> U64 {
              return 0;
            });
  });
  measure("host_fulfillment"_view, 100000, [&]() -> U64 {
    return image.bind_operation(*operation)
        .visit(
            [](const Imaging::Graph::Invocation&) -> U64 { return 1; },
            [](Ttx::Semantic::Negotiation::Binding::Failure) -> U64 {
              return 0;
            });
  });

  // Retaining the callable removes negotiation. Inversion still creates an
  // immutable result, including its pixel bytes and prepared Representation.
  // Release is inside each action so the loop measures a steady ownership
  // cycle.
  measure("retained_invert"_view, operations, [&]() -> U64 {
    auto result = accepted(handle.apply());
    result.operations->release(result.source);
    return 1;
  });
  measure("host_invert_retained"_view, operations, [&]() -> U64 {
    auto result =
        accepted(image.apply(*operation, nullptr, nullptr, errors, &binding));
    return 1;
  });
  measure("host_invert_rebind"_view, operations, [&]() -> U64 {
    auto result = accepted(image.apply(*operation, nullptr, nullptr, errors));
    return 1;
  });

  auto& source = Imaging::Graph::Source::create(image);
  measure("fresh_graph_call"_view, operations, [&]() -> U64 {
    auto& call = Imaging::Graph::Call::create(
        source, *operation,
        Core::View::Vector<Imaging::Graph::Call::Argument>());
    auto result = call.evaluate(errors);
    require(
        result.visit(
            [](const Imaging::Graph::Image&) { return True; },
            [](Core::View::Bytes) { return False; }),
        "Fresh graph call failed."_view);
    const auto evaluations = call.get_evaluations();
    call.release();
    return evaluations;
  });

  // When neither input changed, the Call can reuse its published image.
  // This row measures the cost of checking that retained result.
  // It is useful to measure, but reporting this row as callable speed would
  // hide that the implementation did no image work after the first evaluation.
  auto& cached = Imaging::Graph::Call::create(
      source, *operation, Core::View::Vector<Imaging::Graph::Call::Argument>());
  measure("cached_graph_observation"_view, 1000000, [&]() -> U64 {
    cached.evaluate(errors).visit(
        [](const Imaging::Graph::Image&) {},
        [](Core::View::Bytes error) { Core::Diagnostics::Log::fatal(error); });
    return cached.get_evaluations();
  });
  cached.release();
  source.release();

  // This is the preparation performed in Imaging::Publication::Image's
  // constructor for every result. Compile and write both remain in the batch.
  // The descriptions are identical, so this row identifies work that a shared
  // form could reuse.
  const auto byte =
      Ttx::Data::Form::Schema::primitive(Ttx::Data::Form::Schema::Value::U8);
  const auto schema = Ttx::Data::Form::Schema::range(byte, 4, 1, 4, 1);
  measure("pixel_form_compile"_view, 100000, [&]() -> U64 {
    Ttx::Data::Form::Compiler compiler;
    if (compiler.compile(schema) != Ttx::Data::Status::Success) {
      return 0;
    }

    Memory::Dynamic::Bytes bytes;
    bytes.forgetful_resize(compiler.get_size());
    compiler.write(bytes.get_access());
    return bytes.get_size() != 0 ? 1 : 0;
  });

  const auto observed = accepted(image.read_pixels(errors));
  require(
      observed.get_view() == Core::View::Bytes(pixels, 4),
      "Mechanics changed the source image."_view);
  const auto inverted =
      accepted(image.apply(*operation, nullptr, nullptr, errors, &binding));
  const U8 expected[] = {254, 253, 252, 255};
  require(
      accepted(inverted.read_pixels(errors)).get_view() ==
          Core::View::Bytes(expected, 4),
      "Measured inversion returned incorrect pixels."_view);
  provider.release();
}

int main(int argc, char** argv) {
  Core::Diagnostics::Log::set_sink(Core::Diagnostics::Log::stderr_sink);
  require(
      argc == 2 || argc == 3,
      "Pass the CPU and optional CUDA provider paths."_view);
  measure_provider(Core::NullTerminated::to_view(argv[1]), 100000);
  if (argc == 3) {
    // CUDA's retained invocation still allocates device output, launches work
    // and synchronizes it. A smaller batch keeps that provider work practical
    // while its setup and observation rows use the same counts as CPU.
    measure_provider(Core::NullTerminated::to_view(argv[2]), 10000);
  }

  return 0;
}
