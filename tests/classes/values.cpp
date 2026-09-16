// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include <stdio.h>
#include <stdlib.h>

#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "extensions/sampling/method.hpp"
#include "gdextension/contracts/scalar.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/declarations/extensible.hpp"
#include "ttx/concept/modules/module.h"
#include "ttx/data/form/compiled.hpp"
#include "ttx/semantic/ownership/factory.h"
#include "ttx/semantic/realization/invocation.hpp"

using namespace Ttx;
using namespace Perimortem;

static constexpr U64 method_high = 0x86f2e74629df47acULL;
static constexpr U64 method_low = 0xb4061c3692059700ULL;

// These counters live in the module rather than its discovery allocation.
// Unload is the final oracle: all graph, factory and instance releases must
// already have executed, including when fulfillment rejects construction.
static U32 graphs = 0;
static U32 factories = 0;
static U32 instances = 0;

__attribute__((destructor)) static void unloaded() {
  if (graphs || factories || instances) {
    abort();
  }
  fputs("VALUES module unloaded after all owners\n", stderr);
}

// This independent C++ fixture publishes only TTX declarations and runtime
// thunks. Echo lends its input back to the terminal, exercising the argument
// lifetime through result conversion without importing any Godot carrier.
struct ScoreInput {
  S64 value;
  R64 scale;
  U8 enabled;
  R64 adjustment;
};

static auto input_form(U32 index) -> const ttx_representation& {
  using ::Gdextension::Contracts::Scalar;
  if (index != 3) {
    const Scalar::Kind kinds[] = {
      Scalar::Kind::Text, Scalar::Kind::Boolean, Scalar::Kind::Empty};
    return Scalar::get_representation(kinds[index]);
  }

  using Ttx::Data::Form::Schema;
  static constexpr auto integer = Schema::primitive(Schema::Value::S64);
  static constexpr auto real = Schema::primitive(Schema::Value::R64);
  static constexpr auto boolean = Schema::primitive(Schema::Value::U8);
  static constexpr Schema::Position positions[] = {
    Schema::Position(integer, __builtin_offsetof(ScoreInput, value)),
    Schema::Position(real, __builtin_offsetof(ScoreInput, scale)),
    Schema::Position(boolean, __builtin_offsetof(ScoreInput, enabled)),
    Schema::Position(real, __builtin_offsetof(ScoreInput, adjustment)),
  };
  static constexpr auto schema =
      Schema::composite(positions, sizeof(ScoreInput), alignof(ScoreInput));
  return Ttx::Data::Form::Compiled<schema>::get_representation();
}

static auto output_form(U32 index) -> const ttx_representation& {
  using ::Gdextension::Contracts::Scalar;
  const Scalar::Kind kinds[] = {
    Scalar::Kind::Text, Scalar::Kind::Empty, Scalar::Kind::Boolean,
    Scalar::Kind::Real};
  return Scalar::get_representation(kinds[index]);
}

struct ValueInstance {
  U8 enabled = 0;
  ttx_invocation calls[4] = {
    {this, &input_form(0), &output_form(0),
     [](const void*, const void* input, void* output) -> ttx_data_status {
       *static_cast<perimortem_view_bytes*>(output) =
           *static_cast<const perimortem_view_bytes*>(input);
       return TTX_DATA_SUCCESS;
     }},
    {this, &input_form(1), &output_form(1),
     [](const void* source, const void* input, void*) -> ttx_data_status {
       const_cast<ValueInstance*>(static_cast<const ValueInstance*>(source))
           ->enabled = *static_cast<const U8*>(input);
       return TTX_DATA_SUCCESS;
     }},
    {this, &input_form(2), &output_form(2),
     [](const void* source, const void*, void* output) -> ttx_data_status {
       *static_cast<U8*>(output) =
           static_cast<const ValueInstance*>(source)->enabled;
       return TTX_DATA_SUCCESS;
     }},
    {this, &input_form(3), &output_form(3),
     [](const void*, const void* input, void* output) -> ttx_data_status {
       const auto& value = *static_cast<const ScoreInput*>(input);
       *static_cast<R64*>(output) =
           value.enabled ? (value.value + value.adjustment) * value.scale : 0.0;
       return TTX_DATA_SUCCESS;
     }},
  };
};

static auto bind_instance(
    const void* source,
    perimortem_uuid id,
    ttx_storage requested) -> ttx_binding_status {
  if (id.high != method_high || id.low < method_low ||
      id.low >= method_low + 4) {
    return TTX_BINDING_UNSUPPORTED;
  }
  if (getenv("TTX_VALUES_REFUSE_INSTANCE")) {
    return TTX_BINDING_REJECTED;
  }

  const auto& call =
      static_cast<const ValueInstance*>(source)->calls[id.low - method_low];
  return ttx_binding_provide(ttx_invocation_representation(), &call, requested);
}

static auto supports_instance(const void*, perimortem_uuid id)
    -> ttx_binding_status {
  if (id.high != method_high || id.low < method_low ||
      id.low >= method_low + 4) {
    return TTX_BINDING_UNSUPPORTED;
  }

  return getenv("TTX_VALUES_REFUSE_INSTANCE") ? TTX_BINDING_REJECTED
                                              : TTX_BINDING_SATISFIED;
}

static auto create(const void*, ttx_publication* output) -> ttx_data_status {
  if (getenv("TTX_VALUES_REFUSE_CREATE")) {
    return TTX_DATA_DENIED;
  }

  auto memory = Core::Bibliotheca::check_out(sizeof(ValueInstance));
  auto* instance = new (memory.ptr, Core::Placement::Construct) ValueInstance();
  ++instances;
  *output = {
    {instance, bind_instance, supports_instance}, [](const void* source) {
      auto* instance =
          const_cast<ValueInstance*>(static_cast<const ValueInstance*>(source));
      instance->~ValueInstance();
      --instances;
      fputs("VALUES instance released\n", stderr);
      Core::Bibliotheca::remit(reinterpret_cast<U8*>(instance));
    }};
  return TTX_DATA_SUCCESS;
}

static auto emit(const void*, ttx_publication* output) -> ttx_data_status {
  ++factories;
  *output = {
    {nullptr,
     [](const void* source, perimortem_uuid id,
        ttx_storage requested) -> ttx_binding_status {
       if (id.high != TTX_FACTORY_ID_HIGH || id.low != TTX_FACTORY_ID_LOW) {
         return TTX_BINDING_UNSUPPORTED;
       }

       static const ttx_factory_operations operations = {create};
       const ttx_factory api = {source, &operations};
       return ttx_binding_provide(
           ttx_factory_representation(), &api, requested);
     },
     [](const void*, perimortem_uuid id) -> ttx_binding_status {
       return id.high == TTX_FACTORY_ID_HIGH && id.low == TTX_FACTORY_ID_LOW
                  ? TTX_BINDING_SATISFIED
                  : TTX_BINDING_UNSUPPORTED;
     }},
    [](const void*) {
      --factories;
      fputs("VALUES factory released\n", stderr);
    }};
  return TTX_DATA_SUCCESS;
}

static auto description(U32 index) -> ttx_callable_description {
  using ::Gdextension::Contracts::Scalar;
  static const Scalar text("value"_view, Scalar::Kind::Text);
  static const Scalar flag("enabled"_view, Scalar::Kind::Boolean);
  static const Scalar integer("value"_view, Scalar::Kind::Integer);
  static const Scalar scale("scale"_view, Scalar::Kind::Real);
  static const Scalar adjustment("adjustment"_view, Scalar::Kind::Real);
  static const Scalar unsupported("unknown"_view, Scalar::Kind::Empty);
  static const ttx_callable_field text_field[] = {
    {text.get_abstract().get_abi(), 0}};
  static const ttx_callable_field flag_field[] = {
    {flag.get_abstract().get_abi(), 0}};
  static const ttx_callable_field score_fields[] = {
    {integer.get_abstract().get_abi(), __builtin_offsetof(ScoreInput, value)},
    {scale.get_abstract().get_abi(), __builtin_offsetof(ScoreInput, scale)},
    {flag.get_abstract().get_abi(), __builtin_offsetof(ScoreInput, enabled)},
    {adjustment.get_abstract().get_abi(),
     __builtin_offsetof(ScoreInput, adjustment)},
  };
  static const ttx_callable_field real_field[] = {
    {scale.get_abstract().get_abi(), 0}};
  static const ttx_callable_field unknown_field[] = {
    {unsupported.get_abstract().get_abi(), 0}};
  const ttx_callable_field* inputs[] = {
    text_field, flag_field, nullptr, score_fields};
  const Count counts[] = {1, 1, 0, 4};
  const ttx_callable_field* outputs[] = {
    text_field, nullptr, flag_field, real_field};
  return {
    {method_high, method_low + index},
    {&input_form(index), inputs[index], counts[index]},
    {&output_form(index),
     getenv("TTX_VALUES_BAD_SIGNATURE") ? unknown_field : outputs[index],
     index == 1 ? 0U : 1U}};
}

class Declaration {
 public:
  Declaration()
      : methods{
          Godot::Extensions::Sampling::Method("echo"_view, description(0)),
          Godot::Extensions::Sampling::Method(
              "set_enabled"_view,
              description(1)),
          Godot::Extensions::Sampling::Method(
              "is_enabled"_view,
              description(2)),
          Godot::Extensions::Sampling::Method("score"_view, description(3)),
        } {}
  auto get_data() const -> Core::View::Bytes { return "Values"_view; }

  auto supports(System::Uuid id) const
      -> Semantic::Negotiation::Binding::Status {
    using Semantic::Negotiation::Binding::Status;
    return id == Concept::Declarations::Extensible::contract_id
               ? Status::Satisfied
               : Status::Unsupported;
  }

  auto bind_interface(System::Uuid id, Data::Form::Storage requested) const
      -> Semantic::Negotiation::Binding::Status {
    if (id == Concept::Declarations::Extensible::contract_id) {
      static const ttx_extensible_operations operations = {emit};
      return Semantic::Negotiation::Binding::provide<
          Concept::Declarations::Extensible>(
          ttx_extensible(this, &operations), requested);
    }

    return Ttx::Semantic::Negotiation::Binding::Status::Unsupported;
  }

  auto visit_concepts(Ttx::Concept::Abstract::Visitor visitor) const -> void {
    // A class can describe itself alongside its methods. This child has no
    // Callable capability and must not prevent the real methods registering.
    visitor(revision.get_abstract().get_data(), revision.get_abstract());
    for (const auto& method : methods) {
      visitor(method.get_data(), Ttx::Concept::Abstract::provide(method));
    }
  }

  auto resolve_concept(Core::View::Bytes name) const -> Ttx::Concept::Abstract {
    if (name == revision.get_abstract().get_data()) {
      return revision.get_abstract();
    }
    for (const auto& method : methods) {
      if (method.get_data() == name) {
        return Ttx::Concept::Abstract::provide(method);
      }
    }

    return Ttx::Concept::Abstract(ttx_none());
  }

 private:
  ::Gdextension::Contracts::Scalar revision{
    "revision"_view, ::Gdextension::Contracts::Scalar::Kind::Integer};
  Godot::Extensions::Sampling::Method methods[4];
};

class Exports {
 public:
  auto get_data() const -> Core::View::Bytes { return "Values module"_view; }

  auto visit_concepts(Ttx::Concept::Abstract::Visitor visitor) const -> void {
    visitor(
        declaration.get_data(), Ttx::Concept::Abstract::provide(declaration));
  }

  auto resolve_concept(Core::View::Bytes name) const -> Ttx::Concept::Abstract {
    return name == declaration.get_data()
               ? Ttx::Concept::Abstract::provide(declaration)
               : Ttx::Concept::Abstract(ttx_none());
  }

 private:
  Declaration declaration;
};

PERIMORTEM_C __attribute__((visibility("default"))) ttx_data_status
    ttx_module_open(ttx_semantic_query, ttx_module_acquisition* output) {
  auto storage = Core::Bibliotheca::check_out(sizeof(Exports));
  auto* exports = new (storage.ptr, Core::Placement::Construct) Exports();
  ++graphs;
  *output = {
    Ttx::Concept::Abstract::provide(*exports).get_abi(), exports,
    [](const void* source) {
      auto* exports = const_cast<Exports*>(static_cast<const Exports*>(source));
      exports->~Exports();
      --graphs;
      fputs("VALUES discovery released\n", stderr);
      Core::Bibliotheca::remit(reinterpret_cast<U8*>(exports));
    }};
  return TTX_DATA_SUCCESS;
}
