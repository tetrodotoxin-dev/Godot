// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/sampling/class/contracts.h"
#include "demo/sampling/class/declaration.hpp"
#include "demo/sampling/class/sampler.hpp"
#include "ttx/concept/capabilities/import.hpp"
#include "ttx/semantic/realization/invocation.hpp"

using namespace Godot::Demo;
using namespace Perimortem;

// Each instance owns the public invocation records as well as its private
// sampler. The callable receiver points at that sampler, while negotiation
// uses the enclosing owner. Neither address is a native type proof for callers.
struct RuntimeInstance {
  Sampling::Class::Sampler sampler;
  ttx_invocation calls[3];
  mutable Count references = 1;
  auto get_data() const -> Core::View::Bytes { return Core::View::Bytes(); }
  auto supports(System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status;
  auto bind_interface(System::Uuid id, Ttx::Data::Form::Storage target) const
      -> Ttx::Semantic::Negotiation::Binding::Status;
  auto borrow() const -> Utility::Result<
      Ttx::Concept::Policies::Borrowed,
      Ttx::Semantic::Negotiation::Binding::Failure> {
    ++references;
    return Ttx::Concept::Policies::Borrowed::provide(*this);
  }
  auto release() const -> void {
    if (!--references) {
      delete this;
    }
  }

  RuntimeInstance(
      Ttx::Concept::Capabilities::Import imports,
      Ttx::Semantic::Negotiation::Query host)
      : sampler(imports, host),
        calls{
          {&sampler, sampler_input_representation(0),
           sampler_output_representation(0),
           [](const void* self, const void* input, void* output)
               -> ttx_data_status {
             auto& sampler = *const_cast<Sampling::Class::Sampler*>(
                 static_cast<const Sampling::Class::Sampler*>(self));
             const auto text =
                 *static_cast<const perimortem_view_bytes*>(input);
             *static_cast<U8*>(output) =
                 sampler.configure(Core::View::Bytes(text.data, text.size));
             return TTX_DATA_SUCCESS;
           }},
          {&sampler, sampler_input_representation(1),
           sampler_output_representation(1),
           [](const void* self, const void* input, void* output)
               -> ttx_data_status {
             auto& sampler = *const_cast<Sampling::Class::Sampler*>(
                 static_cast<const Sampling::Class::Sampler*>(self));
             const auto& values =
                 *static_cast<const sampler_count_input*>(input);
             *static_cast<S64*>(output) =
                 sampler.count(values.seed, values.first, values.size);
             return TTX_DATA_SUCCESS;
           }},
          {&sampler, sampler_input_representation(2),
           sampler_output_representation(2),
           [](const void* self, const void*, void* output) -> ttx_data_status {
             const auto text =
                 static_cast<const Sampling::Class::Sampler*>(self)
                     ->get_error();
             *static_cast<perimortem_view_bytes*>(output) = {
               text.get_data(), text.get_size()};
             return TTX_DATA_SUCCESS;
           }},
        } {}
};

// Runtime bindings select the invocation records once for each Godot instance.
// Repeated method calls use those records while the instance holds its borrow.
auto RuntimeInstance::supports(System::Uuid id) const
    -> Ttx::Semantic::Negotiation::Binding::Status {
  using namespace Ttx::Concept;
  using namespace Ttx::Semantic::Negotiation;
  return id == Capabilities::Borrow::contract_id ||
                 id == Policies::Borrowed::contract_id ||
                 (static_cast<perimortem_uuid>(id).high ==
                      SAMPLER_METHOD_HIGH &&
                  static_cast<perimortem_uuid>(id).low >= SAMPLER_METHOD_LOW &&
                  static_cast<perimortem_uuid>(id).low < SAMPLER_METHOD_LOW + 3)
             ? Binding::Status::Satisfied
             : Binding::Status::Unknown;
}
auto RuntimeInstance::bind_interface(
    System::Uuid id,
    Ttx::Data::Form::Storage target) const
    -> Ttx::Semantic::Negotiation::Binding::Status {
  using namespace Ttx::Concept;
  using namespace Ttx::Semantic::Negotiation;
  if (id == Capabilities::Borrow::contract_id) {
    return Binding::provide<Capabilities::Borrow>(
        Capabilities::Borrow::provide(*this).get_abi(), target);
  }
  if (id == Policies::Borrowed::contract_id) {
    return Binding::provide<Policies::Borrowed>(
        Policies::Borrowed::provide(*this).get_abi(), target);
  }
  if (static_cast<perimortem_uuid>(id).high != SAMPLER_METHOD_HIGH ||
      static_cast<perimortem_uuid>(id).low < SAMPLER_METHOD_LOW ||
      static_cast<perimortem_uuid>(id).low >= SAMPLER_METHOD_LOW + 3) {
    return Binding::Status::Unknown;
  }
  return static_cast<Binding::Status>(ttx_binding_provide(
      ttx_invocation_representation(),
      &calls[static_cast<perimortem_uuid>(id).low - SAMPLER_METHOD_LOW],
      target.get_abi()));
}

auto Sampling::Class::Declaration::create(
    Ttx::Concept::Abstract,
    void* receiver,
    void (*receive)(void*, ttx_abstract)) const
    -> Ttx::Semantic::Negotiation::Binding::Status {
  using namespace Ttx::Semantic::Negotiation;
  return host.bind<Ttx::Concept::Capabilities::Import>().visit(
      [&](Ttx::Concept::Capabilities::Import imports) {
        auto* instance = new RuntimeInstance(imports, host);
        receive(receiver, Ttx::Concept::Abstract::provide(*instance).get_abi());
        instance->release();
        return Binding::Status::Satisfied;
      },
      [](Binding::Failure failure) {
        return static_cast<Binding::Status>(failure);
      });
}
