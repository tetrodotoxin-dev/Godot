// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/null_terminated.hpp"

#include "godot_ttx/contracts/lifecycle.hpp"
#include "godot_ttx/contracts/scalar.hpp"
#include "ttx/concept/capabilities/borrow.hpp"
#include "ttx/concept/capabilities/callable.hpp"
#include "ttx/concept/capabilities/create.hpp"
#include "ttx/concept/policies/none.hpp"
#include "ttx/semantic/negotiation/library.h"
#include "ttx/semantic/realization/invocation.hpp"

namespace {

using Godot::Extension::Contracts::Lifecycle;
using Godot::Extension::Contracts::Scalar;
using Perimortem::Core::View::Bytes;
using Perimortem::System::Uuid;
using Ttx::Concept::Abstract;
using Ttx::Concept::Capabilities::Borrow;
using Ttx::Concept::Capabilities::Callable;
using Ttx::Concept::Capabilities::Create;
using Ttx::Concept::Policies::Borrowed;
using Ttx::Data::Form::Storage;
namespace Binding = Ttx::Semantic::Negotiation::Binding;
using Status = Binding::Status;
using Loan = Perimortem::Utility::Result<Borrowed, Binding::Failure>;

constexpr U64 method_high = 0x27b62d5b8e84474fULL;
constexpr U64 method_low = 0x8c5fba7554191210ULL;
U64 live_instances = 0;

// Each created counter keeps its own state. The creation callback holds the
// first reference, and Godot borrows the instance before that callback ends.
// Scene notifications and bound invocations use that retained instance.
class Counter {
 public:
  Counter() { ++live_instances; }

  auto get_data() const -> Bytes { return Bytes(); }

  auto supports(Uuid id) const -> Status {
    return id == Borrow::contract_id || id == Borrowed::contract_id ||
                   id == Lifecycle::contract_id ||
                   (id.get_value().high == method_high &&
                    id.get_value().low >= method_low &&
                    id.get_value().low < method_low + 4)
               ? Status::Satisfied
               : Status::Unknown;
  }

  auto bind_interface(Uuid id, Storage requested) const -> Status {
    if (id == Borrow::contract_id) {
      return Binding::provide<Borrow>(
          Borrow::provide(*this).get_abi(), requested);
    }

    if (id == Borrowed::contract_id) {
      return Binding::provide<Borrowed>(
          Borrowed::provide(*this).get_abi(), requested);
    }

    if (id == Lifecycle::contract_id) {
      return Binding::provide<Lifecycle>(
          Lifecycle::provide(*this).get_abi(), requested);
    }

    if (id.get_value().high == method_high &&
        id.get_value().low >= method_low &&
        id.get_value().low < method_low + 4) {
      using Invoke = ttx_data_status (*)(const void*, const void*, void*);
      static constexpr Invoke calls[] = {
        invoke<0>, invoke<1>, invoke<2>, invoke<3>};
      const auto index = id.get_value().low - method_low;
      const auto api = ttx_invocation(
          this,
          godot_scalar_representation(
              index == 0 ? GODOT_SCALAR_INTEGER : GODOT_SCALAR_EMPTY),
          godot_scalar_representation(GODOT_SCALAR_INTEGER), calls[index]);
      return Binding::provide(api, *ttx_invocation_representation(), requested);
    }

    return Status::Unknown;
  }

  auto borrow() const -> Loan {
    ++references;
    return Borrowed::provide(*this);
  }

  auto release() const -> void {
    if (--references == 0) {
      --live_instances;
      delete this;
    }
  }

  auto entered() const -> void { ++entries; }

  auto ready() const -> void { ++readiness; }

 private:
  template <U32 Index>
  static auto invoke(const void* source, const void* input, void* output)
      -> ttx_data_status {
    const auto& counter = *static_cast<const Counter*>(source);
    S64 result;

    if constexpr (Index == 0) {
      // Unsigned addition defines wrapping before observing the signed result.
      counter.value = static_cast<S64>(
          static_cast<U64>(counter.value) +
          static_cast<U64>(*static_cast<const S64*>(input)));
      result = counter.value;
    } else if constexpr (Index == 1) {
      result = counter.entries;
    } else if constexpr (Index == 2) {
      result = counter.readiness;
    } else {
      result = static_cast<S64>(live_instances);
    }

    *static_cast<S64*>(output) = result;
    return TTX_DATA_SUCCESS;
  }

  U64 references = 1;
  S64 value = 0;
  S64 entries = 0;
  S64 readiness = 0;
};

// These descriptions live with the loaded library. A retained constructor can
// create independent instances after discovery has ended. Its release operation
// returns the borrowed access while the host continues to own the library.
class Declaration {
 public:
  explicit constexpr Declaration(U32 index) : index(index) {}

  auto get_data() const -> Bytes;

  auto resolve_concept(Bytes route) const -> Abstract;

  auto visit_concepts(Abstract::Visitor visitor) const -> void;

  auto supports(Uuid id) const -> Status {
    return id == Borrow::contract_id || id == Borrowed::contract_id ||
                   (index == 1 && id == Create::contract_id) ||
                   (index >= 2 && id == Callable::contract_id)
               ? Status::Satisfied
               : Status::Unknown;
  }

  auto bind_interface(Uuid id, Storage requested) const -> Status {
    if (id == Borrow::contract_id) {
      return Binding::provide<Borrow>(
          Borrow::provide(*this).get_abi(), requested);
    }

    if (id == Borrowed::contract_id) {
      return Binding::provide<Borrowed>(
          Borrowed::provide(*this).get_abi(), requested);
    }

    if (index == 1 && id == Create::contract_id) {
      return Binding::provide<Create>(
          Create::provide(*this).get_abi(), requested);
    }

    if (index >= 2 && id == Callable::contract_id) {
      static const Callable::Operations operations = Callable::Operations(
          *Abstract::provide(*this).get_abi().operations,
          [](const void* source, ttx_callable_description* output) {
            *output = static_cast<const Declaration*>(source)->describe();
            return TTX_BINDING_SATISFIED;
          });
      return Binding::provide<Callable>(
          ttx_callable(this, &operations), requested);
    }

    return Status::Unknown;
  }

  auto borrow() const -> Loan { return Borrowed::provide(*this); }

  auto release() const -> void {}

  template <typename Receiver>
  auto create(Abstract arguments, Receiver& receive) const -> Status {
    if (arguments.supports<Ttx::Concept::Policies::None>() !=
        Status::Satisfied) {
      return Status::Rejected;
    }

    auto* counter = new Counter();
    receive(Abstract::provide(*counter));
    counter->release();
    return Status::Satisfied;
  }

 private:
  auto describe() const -> ttx_callable_description {
    static const Scalar amount("amount"_view, Scalar::Kind::Integer);
    static const Scalar result("result"_view, Scalar::Kind::Integer);
    static const ttx_callable_field argument(
        amount.get_abstract().get_abi(), 0);
    static const ttx_callable_field returned(
        result.get_abstract().get_abi(), 0);
    return ttx_callable_description(
        perimortem_uuid(method_high, method_low + index - 2),
        ttx_invocation_representation(),
        ttx_callable_frame(
            godot_scalar_representation(
                index == 2 ? GODOT_SCALAR_INTEGER : GODOT_SCALAR_EMPTY),
            &argument, index == 2 ? 1 : 0),
        ttx_callable_frame(
            godot_scalar_representation(GODOT_SCALAR_INTEGER), &returned, 1));
  }

  U32 index;
};

constexpr Declaration declarations[] = {Declaration(0), Declaration(1),
                                        Declaration(2), Declaration(3),
                                        Declaration(4), Declaration(5)};
constexpr Bytes names[] = {"Counter module"_view,  "Counter"_view,
                           "advance"_view,         "get_entries"_view,
                           "get_ready_count"_view, "get_live_instances"_view};

auto Declaration::get_data() const -> Bytes {
  return names[index];
}

auto Declaration::visit_concepts(Abstract::Visitor visitor) const -> void {
  const U32 first = index == 0 ? 1 : 2;
  const U32 end = index == 0 ? 2 : (index == 1 ? 6 : 2);

  for (U32 next = first; next != end; ++next) {
    visitor(names[next], Abstract::provide(declarations[next]));
  }
}

auto Declaration::resolve_concept(Bytes route) const -> Abstract {
  auto result = Abstract(Ttx::Concept::Policies::None::get_none());
  auto find = [&](Bytes name, Abstract subject) {
    if (route == name) {
      result = subject;
    }
  };
  visit_concepts(Abstract::Visitor(find));
  return result;
}

}  // namespace

extern "C" __attribute__((visibility("default"))) auto ttx_query(
    ttx_semantic_query,
    ttx_query_receiver receive) -> ttx_binding_status {
  return receive.receive(
      receive.source, Abstract::provide(declarations[0]).get_query());
}
