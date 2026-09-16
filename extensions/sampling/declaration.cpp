// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "extensions/sampling/declaration.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "extensions/sampling/contracts.h"
#include "gdextension/contracts/scalar.hpp"

using namespace Godot;
using namespace Perimortem;

static auto description(U32 index) -> ttx_callable_description {
  using ::Gdextension::Contracts::Scalar;
  static const Scalar provider("provider"_view, Scalar::Kind::Text);
  static const Scalar seed("seed"_view, Scalar::Kind::Integer);
  static const Scalar first("first"_view, Scalar::Kind::Integer);
  static const Scalar size("size"_view, Scalar::Kind::Integer);
  static const Scalar accepted("accepted"_view, Scalar::Kind::Boolean);
  static const Scalar total("total"_view, Scalar::Kind::Integer);
  static const Scalar error("error"_view, Scalar::Kind::Text);
  static const ttx_callable_field configure[] = {
    {provider.get_abstract().get_abi(), 0}};
  static const ttx_callable_field count[] = {
    {seed.get_abstract().get_abi(),
     __builtin_offsetof(sampler_count_input, seed)},
    {first.get_abstract().get_abi(),
     __builtin_offsetof(sampler_count_input, first)},
    {size.get_abstract().get_abi(),
     __builtin_offsetof(sampler_count_input, size)},
  };
  static const ttx_callable_field results[] = {
    {accepted.get_abstract().get_abi(), 0},
    {total.get_abstract().get_abi(), 0},
    {error.get_abstract().get_abi(), 0},
  };
  const auto* arguments = index == 0 ? configure : count;
  return {
    {SAMPLER_METHOD_HIGH, SAMPLER_METHOD_LOW + index},
    {sampler_input_representation(index), arguments,
     index == 0   ? 1U
     : index == 1 ? 3U
                  : 0U},
    {sampler_output_representation(index), &results[index], 1}};
}

Extensions::Sampling::Declaration::Declaration(
    Ttx::Semantic::Negotiation::Query host)
    : host(host),
      methods{
        Method("configure"_view, description(0)),
        Method("count"_view, description(1)),
        Method("get_error"_view, description(2)),
      } {}

auto Extensions::Sampling::Declaration::get_data() const -> Core::View::Bytes {
  return "Sampler"_view;
}

auto Extensions::Sampling::Declaration::supports(System::Uuid id) const
    -> Ttx::Semantic::Negotiation::Binding::Status {
  using Ttx::Semantic::Negotiation::Binding::Status;
  return id == Ttx::Concept::Declarations::Extensible::contract_id
             ? Status::Satisfied
             : Status::Unsupported;
}

auto Extensions::Sampling::Declaration::bind_interface(
    System::Uuid requested,
    Ttx::Data::Form::Storage target) const
    -> Ttx::Semantic::Negotiation::Binding::Status {
  if (requested == Ttx::Concept::Declarations::Extensible::contract_id) {
    static const ttx_extensible_operations operations = {
      [](const void* source, ttx_publication* output) -> ttx_data_status {
        return Runtime::emit(static_cast<const Declaration*>(source)->host)
            .visit(
                [&](Ttx::Semantic::Ownership::Publication& factory)
                    -> ttx_data_status {
                  *output = factory.take();
                  return TTX_DATA_SUCCESS;
                },
                [](Ttx::Data::Status status) {
                  return static_cast<ttx_data_status>(status);
                });
      },
    };
    return Ttx::Semantic::Negotiation::Binding::provide<
        Ttx::Concept::Declarations::Extensible>(
        ttx_extensible(this, &operations), target);
  }

  return Ttx::Semantic::Negotiation::Binding::Status::Unsupported;
}

auto Extensions::Sampling::Declaration::resolve_concept(
    Core::View::Bytes name) const -> Ttx::Concept::Abstract {
  for (const auto& method : methods) {
    if (method.get_data() == name) {
      return Ttx::Concept::Abstract::provide(method);
    }
  }

  return Ttx::Concept::Abstract(ttx_none());
}

auto Extensions::Sampling::Declaration::visit_concepts(
    Ttx::Concept::Abstract::Visitor visitor) const -> void {
  for (const auto& method : methods) {
    visitor(method.get_data(), Ttx::Concept::Abstract::provide(method));
  }
}
