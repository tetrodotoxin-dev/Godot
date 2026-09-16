// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include <dlfcn.h>

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/time.hpp"
#include "perimortem/core/writer/textual.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/system/library.hpp"

#include "sampling/function.hpp"
#include "ttx/semantic/realization/simulacra.hpp"

using namespace Godot;
using namespace Perimortem;

static void require(Bool condition, Core::View::Bytes message) {
  if (!condition) {
    Core::Diagnostics::Log::fatal(message);
  }
}

template <typename Value, typename Error>
static auto accepted(Utility::Result<Value, Error> result) -> Value {
  return result.visit(
      [](Value& value) -> Value { return static_cast<Value&&>(value); },
      [](Error) -> Value {
        Core::Diagnostics::Log::fatal("Sampling check failed."_view);
      });
}

static auto loaded(Core::View::Bytes path) -> Bool {
  Memory::Dynamic::Bytes terminated(path);
  terminated.append(0);
  auto* module = dlopen(
      reinterpret_cast<const char*>(terminated.get_view().get_data()),
      RTLD_NOW | RTLD_NOLOAD);
  if (!module) {
    return False;
  }

  dlclose(module);
  return True;
}

// Expected answers were calculated independently from the integer contract.
// They include a tail interval that ends exactly at 2^32, where a U32 loop
// cursor could otherwise wrap and never terminate.
static void answers(Sampling::Contracts::Samples function) {
  require(
      accepted(function.count(0, 0, 1)) == 1, "Origin sample differs."_view);
  require(
      accepted(function.count(0, 0, 17)) == 15, "Small sample differs."_view);
  require(
      accepted(function.count(7, 123, 1024)) == 824,
      "Offset sample differs."_view);
  require(
      accepted(function.count(0xffffffff, 0xffffff00, 256)) == 209,
      "Tail interval differs."_view);
  require(
      accepted(function.count(0, 0xffffffff, 1)) == 1,
      "Final sample differs."_view);
  require(
      accepted(function.count(42, 5, 0)) == 0, "Empty interval differs."_view);
  require(
      function.count(0, 0xffffffff, 2)
          .visit(
              [](U64) { return False; },
              [](Ttx::Data::Status status) -> Bool {
                return status == Ttx::Data::Status::Bounds;
              }),
      "Overflowing interval was accepted."_view);
}

// Exercise C output and refusal behavior independently of the C++ result view.
// Only the negotiated operation table is interpreted. Its receiver stays
// opaque.
static void boundary(Core::View::Bytes path) {
  Memory::Allocator::Arena errors;
  auto library = accepted(Perimortem::System::Library::open(path, errors));
  auto address = accepted(library.symbol("ttx_module_open"_view, errors));
  const auto open = reinterpret_cast<ttx_module_entry>(address);
  ttx_module_acquisition provider = {};
  require(
      open(ttx_semantic_query{}, &provider) == TTX_DATA_SUCCESS,
      "Could not open C publication."_view);
  const auto query = Ttx::Concept::Abstract(provider.root).get_query();
  using Samples = Sampling::Contracts::Samples;
  using Ttx::Semantic::Negotiation::Binding::Status;
  Samples::Api binding = {};
  const auto& form = Samples::get_representation();
  const Ttx::Data::Form::Storage target(
      ttx_storage{&form, reinterpret_cast<U8*>(&binding), sizeof(binding)});
  require(
      query.bind(System::Uuid(1, 2), target) == Status::Unsupported,
      "Matching API geometry admitted the wrong contract."_view);

  using Ttx::Data::Form::Schema;
  static constexpr auto byte = Schema::primitive(Schema::Value::U8);
  const auto& wrong_form =
      Ttx::Data::Form::Compiled<byte>::get_representation();
  U8 untouched = 123;
  const Ttx::Data::Form::Storage wrong(ttx_storage{&wrong_form, &untouched, 1});
  require(
      query.bind(Samples::contract_id, wrong) == Status::Rejected &&
          untouched == 123,
      "Incompatible API storage was accepted or modified."_view);
  require(
      query.bind(Samples::contract_id, target) == Status::Satisfied,
      "C callable record did not agree."_view);

  U64 output = 123;
  const auto& table = *binding.operations;
  require(
      table.count(binding.source, 0, 0xffffffff, 2, &output) ==
              TTX_DATA_BOUNDS &&
          output == 123,
      "Failed call published an output."_view);
  provider.release(provider.owner);
}

// Same interval, different placement. This deliberately executes synchronously
// on one host worker. It proves partitioning and recombination across modules,
// not a network scheduler or overlapping CPU/GPU execution.
static void partition(
    Sampling::Contracts::Samples left,
    Sampling::Contracts::Samples right) {
  constexpr U32 seed = 73;
  constexpr U32 first = 109;
  constexpr U32 count = 65539;
  constexpr U32 split = 17003;
  const auto whole = accepted(left.count(seed, first, count));
  const auto a = accepted(left.count(seed, first, split));
  const auto b = accepted(right.count(seed, first + split, count - split));
  require(a + b == whole, "Mixed partition changed the result."_view);
  require(
      accepted(right.count(seed, first, count)) == whole,
      "Providers disagree on the complete interval."_view);
}

static void measure(
    Core::View::Bytes backend,
    Sampling::Contracts::Samples function,
    U32 size,
    Count iterations) {
  const auto expected = accepted(function.count(13, 0, size));
  const auto allocations = Core::Bibliotheca::check_out_requests();
  const auto start = Core::Time::now();
  U64 total = 0;
  for (Count i = 0; i < iterations; ++i) {
    total += accepted(function.count(13, 0, size));
  }

  const auto elapsed = start.measure().convert_to_nanoseconds();
  const auto requests = Core::Bibliotheca::check_out_requests() - allocations;
  require(total == expected * iterations, "Measured calls lost results."_view);
  require(requests == 0, "Retained sampling requested host allocation."_view);
  Core::Static::Bytes<256> text;
  Core::Writer::Textual writer(text);
  writer << "SAMPLES "_view << backend << " size="_view << size << " ns="_view
         << R64(elapsed) / iterations << " allocation_requests="_view
         << requests << " hits="_view << expected;
  Core::Diagnostics::Log::info(Core::View::Bytes(writer));
}

static void check(Core::View::Bytes cpu_path, Core::View::Bytes cuda_path) {
  Memory::Allocator::Arena errors;
  require(!loaded(cpu_path), "CPU sampling module already loaded."_view);
  auto cpu = accepted(Sampling::Function::open(cpu_path, errors));
  require(loaded(cpu_path), "Function did not retain its module."_view);
  auto moved = Core::Data::take(cpu);
  const auto function = moved.get_handle();
  answers(function);
  partition(function, function);
  measure("cpu"_view, function, 0, 1000000);
  measure("cpu"_view, function, 1, 1000000);
  measure("cpu"_view, function, 1048576, 5);
  measure("cpu"_view, function, 16777216, 3);

  if (!cuda_path.is_empty()) {
    auto cuda = accepted(Sampling::Function::open(cuda_path, errors));
    const auto device = cuda.get_handle();
    answers(device);
    partition(function, device);
    measure("cuda"_view, device, 0, 1000000);
    measure("cuda"_view, device, 1, 10000);
    measure("cuda"_view, device, 1048576, 5);
    measure("cuda"_view, device, 16777216, 3);
  }
}

int main(int argc, char** argv) {
  Core::Diagnostics::Log::set_sink(Core::Diagnostics::Log::stderr_sink);
  require(argc == 2 || argc == 3, "Pass CPU and optional CUDA providers."_view);
  const auto cpu = Core::NullTerminated::to_view(argv[1]);
  const auto cuda =
      argc == 3 ? Core::NullTerminated::to_view(argv[2]) : Core::View::Bytes();
  boundary(cpu);
  if (!cuda.is_empty()) {
    boundary(cuda);
  }

  check(cpu, cuda);
  require(!loaded(cpu), "CPU module survived the final function."_view);
  if (!cuda.is_empty()) {
    require(!loaded(cuda), "CUDA module survived the final function."_view);
  }

  Core::Diagnostics::Log::info(
      "PASS: scalar answers, mixed partitions, zero host allocations and module lifetime"_view);
}
