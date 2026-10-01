// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/sampling/provider.hpp"

using namespace Godot::Demo;
using namespace Perimortem;
using namespace Ttx::Semantic::Negotiation;
using namespace Ttx::Concept;

auto Sampling::Provider::get_query() const -> ttx_semantic_query {
  return Abstract::provide(*this).get_query();
}
auto Sampling::Provider::borrow() const
    -> Utility::Result<Policies::Borrowed, Binding::Failure> {
  retain_value(binding.source);
  return Policies::Borrowed::provide(*this);
}
auto Sampling::Provider::supports(System::Uuid id) const -> Binding::Status {
  return id == Sampling::Contracts::Samples::contract_id ||
                 id == Capabilities::Borrow::contract_id ||
                 id == Policies::Borrowed::contract_id
             ? Binding::Status::Satisfied
             : Binding::Status::Unknown;
}
auto Sampling::Provider::bind_interface(
    System::Uuid id,
    Ttx::Data::Form::Storage target) const -> Binding::Status {
  if (id == Capabilities::Borrow::contract_id) {
    return Binding::provide<Capabilities::Borrow>(
        Capabilities::Borrow::provide(*this).get_abi(), target);
  }
  if (id == Policies::Borrowed::contract_id) {
    return Binding::provide<Policies::Borrowed>(
        Policies::Borrowed::provide(*this).get_abi(), target);
  }
  return id == Sampling::Contracts::Samples::contract_id
             ? Binding::provide<Sampling::Contracts::Samples>(binding, target)
             : Binding::Status::Unknown;
}
