// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "extensions/sampling/runtime.hpp"

#include "perimortem/core/bibliotheca.hpp"

#include "extensions/sampling/contracts.h"
#include "extensions/sampling/sampler.hpp"
#include "ttx/semantic/realization/invocation.hpp"

using namespace Godot;
using namespace Perimortem;

// Each instance owns the public invocation records as well as its private
// sampler. The callable receiver points at that sampler, while negotiation
// uses the enclosing owner. Neither address is a native type proof for callers.
struct RuntimeInstance {
  Extensions::Sampling::Sampler sampler;
  ttx_invocation calls[3];

  RuntimeInstance(
      Ttx::Concept::Modules::Import imports,
      Ttx::Semantic::Negotiation::Query host)
      : sampler(imports, host),
        calls{
          {&sampler, sampler_input_representation(0),
           sampler_output_representation(0),
           [](const void* self, const void* input, void* output)
               -> ttx_data_status {
             auto& sampler = *const_cast<Extensions::Sampling::Sampler*>(
                 static_cast<const Extensions::Sampling::Sampler*>(self));
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
             auto& sampler = *const_cast<Extensions::Sampling::Sampler*>(
                 static_cast<const Extensions::Sampling::Sampler*>(self));
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
                 static_cast<const Extensions::Sampling::Sampler*>(self)
                     ->get_error();
             *static_cast<perimortem_view_bytes*>(output) = {
               text.get_data(), text.get_size()};
             return TTX_DATA_SUCCESS;
           }},
        } {}
};

// The method UUID selects this instance's record. Generic binding checks the
// actual invocation ABI. Invocation checks its declared payload forms once.
static auto bind_instance(
    const void* source,
    perimortem_uuid id,
    ttx_storage requested) -> ttx_binding_status {
  if (id.high != SAMPLER_METHOD_HIGH || id.low < SAMPLER_METHOD_LOW ||
      id.low >= SAMPLER_METHOD_LOW + 3) {
    return TTX_BINDING_UNSUPPORTED;
  }

  const auto& call = static_cast<const RuntimeInstance*>(source)
                         ->calls[id.low - SAMPLER_METHOD_LOW];
  return ttx_binding_provide(ttx_invocation_representation(), &call, requested);
}

static auto supports_instance(const void*, perimortem_uuid id)
    -> ttx_binding_status {
  return id.high == SAMPLER_METHOD_HIGH && id.low >= SAMPLER_METHOD_LOW &&
                 id.low < SAMPLER_METHOD_LOW + 3
             ? TTX_BINDING_SATISFIED
             : TTX_BINDING_UNSUPPORTED;
}

auto Extensions::Sampling::Runtime::get_query() const -> ttx_semantic_query {
  return {
    this,
    [](const void* source, perimortem_uuid id,
       ttx_storage requested) -> ttx_binding_status {
      if (System::Uuid(id) != Ttx::Semantic::Ownership::Factory::contract_id) {
        return TTX_BINDING_UNSUPPORTED;
      }

      static const ttx_factory_operations operations = {
        [](const void* source, ttx_publication* output) -> ttx_data_status {
          const auto& factory = *static_cast<const Runtime*>(source);
          auto memory = Core::Bibliotheca::check_out(sizeof(RuntimeInstance));
          auto* instance = new (memory.ptr, Core::Placement::Construct)
              RuntimeInstance(factory.imports, factory.host);
          *output = {
            {instance, bind_instance, supports_instance},
            [](const void* source) {
              auto* instance = const_cast<RuntimeInstance*>(
                  static_cast<const RuntimeInstance*>(source));
              instance->~RuntimeInstance();
              Core::Bibliotheca::remit(reinterpret_cast<U8*>(instance));
            }};
          return TTX_DATA_SUCCESS;
        },
      };
      const ttx_factory api = {source, &operations};
      return ttx_binding_provide(ttx_factory_representation(), &api, requested);
    },
    [](const void*, perimortem_uuid id) -> ttx_binding_status {
      return System::Uuid(id) == Ttx::Semantic::Ownership::Factory::contract_id
                 ? TTX_BINDING_SATISFIED
                 : TTX_BINDING_UNSUPPORTED;
    }};
}

auto Extensions::Sampling::Runtime::emit(Ttx::Semantic::Negotiation::Query host)
    -> Utility::
        Result<Ttx::Semantic::Ownership::Publication, Ttx::Data::Status> {
  using Result =
      Utility::Result<Ttx::Semantic::Ownership::Publication, Ttx::Data::Status>;
  return host.bind<Ttx::Concept::Modules::Import>().visit(
      [&](Ttx::Concept::Modules::Import imports) -> Result {
        auto memory = Core::Bibliotheca::check_out(sizeof(Runtime));
        auto* runtime =
            new (memory.ptr, Core::Placement::Construct) Runtime(imports, host);
        return Ttx::Semantic::Ownership::Publication(
            {runtime->get_query(), [](const void* source) {
               auto* runtime =
                   const_cast<Runtime*>(static_cast<const Runtime*>(source));
               runtime->~Runtime();
               Core::Bibliotheca::remit(reinterpret_cast<U8*>(runtime));
             }});
      },
      [](Ttx::Semantic::Negotiation::Binding::Failure failure) -> Result {
        switch (failure) {
        case Ttx::Semantic::Negotiation::Binding::Failure::Unsupported:
          return Ttx::Data::Status::Unsupported;
        case Ttx::Semantic::Negotiation::Binding::Failure::Pending:
          return Ttx::Data::Status::Busy;
        case Ttx::Semantic::Negotiation::Binding::Failure::Rejected:
          return Ttx::Data::Status::Denied;
        }

        return Ttx::Data::Status::Invalid;
      });
}
