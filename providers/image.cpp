// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "providers/image.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "contracts/operation.hpp"
#include "ttx/data/form/compiler.hpp"
#include "ttx/semantic/block.hpp"
#include "ttx/semantic/thunk.hpp"

using namespace Godot;
using namespace Perimortem;

Providers::Image::Image(U8* allocation, U32 w, U32 h)
    : allocation(allocation), dimensions{w, h} {
  using Ttx::Data::Form::Schema;
  const auto byte = Schema::primitive(Schema::Value::U8);
  const Count size = Count(w) * h * 4;
  const auto pixels = Schema::range(byte, size, 1, size, 1);
  Ttx::Data::Form::Compiler compiler;
  if (compiler.compile(pixels) != Ttx::Data::Status::Success) {
    Core::Diagnostics::Log::fatal("Image pixel representation failed."_view);
  }

  description = Core::Object<U8>(compiler.get_size());
  compiler.write(
      Core::Access::Bytes(
          description.get_access().get_data(), compiler.get_size()));
  representation = Ttx::Data::Form::Representation(
      description.get_view().get_data(), compiler.get_size());
}

Providers::Image::Image(U8* allocation, const Image& source)
    : allocation(allocation),
      dimensions(source.dimensions),
      description(source.description),
      representation(source.representation) {}

auto Providers::Image::get_abi() const -> image_object {
  static const image_operations operations = {
    [](const void* source) {
      Core::Object<>(static_cast<const Image*>(source)->allocation).retain();
    },
    [](const void* source) { static_cast<const Image*>(source)->release(); },
    [](const void* source) {
      return static_cast<const Image*>(source)->dimensions;
    },
    [](const void* source) -> ttx_semantic_query {
      return static_cast<const Image*>(source)->get_query();
    },
    [](const void* source) -> ttx_semantic_query {
      // Pixel publication offers only Block. A CUDA image cannot lend a host
      // pointer. A CPU image may elect the same contract without pretending to
      // implement every faster or slower protocol as an inheritance hierarchy.
      return {
        source,
        [](const void* source, perimortem_uuid id,
           ttx_binding* output) -> ttx_binding_status {
          if (System::Uuid(id) != Ttx::Semantic::Block::Access::contract_id) {
            return TTX_BINDING_UNSUPPORTED;
          }

          static const ttx_block_access_operations table = {
            [](const void* source) {
              return &static_cast<const Image*>(source)->representation;
            },
            [](const void* source,
               ttx_block_surface surface) -> ttx_data_status {
              const auto error = static_cast<const Image*>(source)->read_pixels(
                  Core::Access::Bytes(surface.data, surface.size));
              return error.is_empty() ? TTX_DATA_SUCCESS : TTX_DATA_IO_ERROR;
            },
          };

          *output = {source, &table};
          return TTX_BINDING_SATISFIED;
        }};
    },
    [](const void* source) {
      return &static_cast<const Image*>(source)->representation;
    },
  };

  return {this, &operations};
}

auto Providers::Image::get_query() const -> Ttx::Semantic::Query {
  return Ttx::Semantic::Query(
      {this,
       [](const void* source, perimortem_uuid id,
          ttx_binding* output) -> ttx_binding_status {
         if (System::Uuid(id) != Ttx::Semantic::Thunk::contract_id) {
           return TTX_BINDING_UNSUPPORTED;
         }

         static const ttx_thunk_operations table = {
           [](const void* source, perimortem_uuid contract,
              ttx_calling_convention convention,
              const ttx_representation* representation,
              ttx_binding* output) -> ttx_binding_status {
             if (convention != TTX_CALLING_SYSTEM_V_AMD64) {
               return TTX_BINDING_UNSUPPORTED;
             }

             if (!Contracts::Operation::get_representation().compatible(
                     *representation)) {
               return TTX_BINDING_REJECTED;
             }

             return static_cast<const Image*>(source)
                 ->fulfill(System::Uuid(contract))
                 .visit(
                     [&](const Ttx::Semantic::Binding& binding)
                         -> ttx_binding_status {
                       *output = binding.get_abi();
                       return TTX_BINDING_SATISFIED;
                     },
                     [](Ttx::Semantic::Binding::Failure failure)
                         -> ttx_binding_status {
                       return static_cast<ttx_binding_status>(failure);
                     });
           },
         };

         *output = {source, &table};
         return TTX_BINDING_SATISFIED;
       }});
}
