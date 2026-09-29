// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/sampling/class/declaration.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "demo/sampling/class/contracts.h"
#include "extension/contracts/scalar.hpp"
#include "ttx/concept/policies/none.h"
#include "ttx/semantic/realization/invocation.h"

using namespace Godot::Demo;
using namespace Perimortem;

static auto description(U32 index) -> ttx_callable_description {
  using ::Godot::Extension::Contracts::Scalar;
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
  return ttx_callable_description(
      {SAMPLER_METHOD_HIGH, SAMPLER_METHOD_LOW + index},
      ttx_invocation_representation(),
      {sampler_input_representation(index), arguments,
       index == 0   ? 1U
       : index == 1 ? 3U
                    : 0U},
      {sampler_output_representation(index), &results[index], 1});
}

Sampling::Class::Declaration::Declaration(
    Ttx::Semantic::Negotiation::Query host)
    : host(host),
      methods{
        Method("configure"_view, description(0)),
        Method("count"_view, description(1)),
        Method("get_error"_view, description(2)),
      } {}

auto Sampling::Class::Declaration::get_data() const -> Core::View::Bytes {
  return "Sampler"_view;
}

auto Sampling::Class::Declaration::borrow() const -> Utility::Result<
    Ttx::Concept::Policies::Borrowed,
    Ttx::Semantic::Negotiation::Binding::Failure> {
  auto* retained = references ? this : new Declaration(host);
  ++retained->references;
  return Ttx::Concept::Policies::Borrowed::provide(*retained);
}
auto Sampling::Class::Declaration::release() const -> void {
  if (!--references) {
    delete this;
  }
}
auto Sampling::Class::Declaration::supports(System::Uuid id) const
    -> Ttx::Semantic::Negotiation::Binding::Status {
  using namespace Ttx::Concept;
  using namespace Ttx::Semantic::Negotiation;
  return id == Capabilities::Borrow::contract_id ||
                 id == Capabilities::Create::contract_id ||
                 (references && id == Policies::Borrowed::contract_id)
             ? Binding::Status::Satisfied
             : Binding::Status::Unknown;
}
auto Sampling::Class::Declaration::bind_interface(
    System::Uuid id,
    Ttx::Data::Form::Storage target) const
    -> Ttx::Semantic::Negotiation::Binding::Status {
  using namespace Ttx::Concept;
  using namespace Ttx::Semantic::Negotiation;
  if (id == Capabilities::Borrow::contract_id) {
    return Binding::provide<Capabilities::Borrow>(
        Capabilities::Borrow::provide(*this).get_abi(), target);
  }
  if (references && id == Policies::Borrowed::contract_id) {
    return Binding::provide<Policies::Borrowed>(
        Policies::Borrowed::provide(*this).get_abi(), target);
  }
  if (id == Capabilities::Create::contract_id) {
    static const ttx_create_ops operations = ttx_create_ops(
        *Abstract::provide(*this).get_abi().operations,
        [](const void* source, ttx_abstract arguments, void* receiver,
           void (*receive)(void*, ttx_abstract)) {
          return static_cast<ttx_binding_status>(
              static_cast<const Declaration*>(source)->create(
                  Abstract(arguments), receiver, receive));
        });
    return Binding::provide<Capabilities::Create>(
        ttx_create(this, &operations), target);
  }
  return Binding::Status::Unknown;
}

auto Sampling::Class::Declaration::resolve_concept(Core::View::Bytes name) const
    -> Ttx::Concept::Abstract {
  for (const auto& method : methods) {
    if (method.get_data() == name) {
      return Ttx::Concept::Abstract::provide(method);
    }
  }

  return Ttx::Concept::Abstract(ttx_none());
}

auto Sampling::Class::Declaration::visit_concepts(
    Ttx::Concept::Abstract::Visitor visitor) const -> void {
  for (const auto& method : methods) {
    visitor(method.get_data(), Ttx::Concept::Abstract::provide(method));
  }
}
