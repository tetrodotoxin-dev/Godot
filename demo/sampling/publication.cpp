// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/sampling/publication.hpp"

using namespace Godot::Demo;
using namespace Perimortem;

auto Sampling::Publication::get_publication() const -> ttx_publication {
  return {
    {this,
     [](const void* source, perimortem_uuid id,
        ttx_storage requested) -> ttx_binding_status {
       if (System::Uuid(id) != Sampling::Contracts::Samples::contract_id) {
         return TTX_BINDING_UNSUPPORTED;
       }
       return static_cast<ttx_binding_status>(
           Ttx::Semantic::Negotiation::Binding::provide<
               Sampling::Contracts::Samples>(
               static_cast<const Publication*>(source)->binding,
               Ttx::Data::Form::Storage(requested)));
     },
     [](const void*, perimortem_uuid id) -> ttx_binding_status {
       return System::Uuid(id) == Sampling::Contracts::Samples::contract_id
                  ? TTX_BINDING_SATISFIED
                  : TTX_BINDING_UNSUPPORTED;
     }},
    [](const void* source) {
      const auto& publication = *static_cast<const Publication*>(source);
      publication.release(publication.binding.source);
    },
  };
}
