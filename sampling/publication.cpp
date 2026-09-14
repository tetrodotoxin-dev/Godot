// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "sampling/publication.hpp"

using namespace Godot;
using namespace Perimortem;

auto Sampling::Publication::get_provider() const -> sample_provider {
  return {
    {this,
     [](const void* source, perimortem_uuid id,
        ttx_binding* output) -> ttx_binding_status {
       if (System::Uuid(id) != Ttx::Semantic::Thunk::contract_id) {
         return TTX_BINDING_UNSUPPORTED;
       }

       static const ttx_thunk_operations operations = {
         [](const void* source, perimortem_uuid id,
            ttx_calling_convention convention,
            const ttx_representation* representation,
            ttx_binding* output) -> ttx_binding_status {
           if (System::Uuid(id) != Contracts::Samples::contract_id ||
               convention != TTX_CALLING_SYSTEM_V_AMD64) {
             return TTX_BINDING_UNSUPPORTED;
           }

           if (!Contracts::Samples::get_representation().compatible(
                   *representation)) {
             return TTX_BINDING_REJECTED;
           }

           *output = static_cast<const Publication*>(source)->binding.get_abi();
           return TTX_BINDING_SATISFIED;
         },
       };
       *output = {source, &operations};
       return TTX_BINDING_SATISFIED;
     }},
    [](const void* source) {
      const auto& publication = *static_cast<const Publication*>(source);
      publication.release(publication.binding.get_abi().source);
    },
  };
}
