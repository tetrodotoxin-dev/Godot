// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "imaging/publication/image.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "ttx/data/form/compiler.hpp"
#include "ttx/semantic/transport/block.hpp"

using namespace Godot;
using namespace Perimortem;

Imaging::Publication::Image::Image(U8* allocation, U32 w, U32 h)
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

Imaging::Publication::Image::Image(U8* allocation, const Image& source)
    : allocation(allocation),
      dimensions(source.dimensions),
      description(source.description),
      representation(source.representation) {}

auto Imaging::Publication::Image::get_abi() const -> image_object {
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
      // pointer. A CPU image may elect the same contract without pretending
      // to implement every faster or slower protocol as an inheritance
      // hierarchy.
      return {
        source,
        [](const void* source, perimortem_uuid id,
           ttx_storage requested) -> ttx_binding_status {
          if (System::Uuid(id) !=
              Ttx::Semantic::Transport::Block::Access::contract_id) {
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

          return static_cast<ttx_binding_status>(
              Ttx::Semantic::Negotiation::Binding::provide<
                  Ttx::Semantic::Transport::Block::Access>(
                  ttx_block_access(source, &table),
                  Ttx::Data::Form::Storage(requested)));
        },
        [](const void*, perimortem_uuid id) -> ttx_binding_status {
          return System::Uuid(id) ==
                         Ttx::Semantic::Transport::Block::Access::contract_id
                     ? TTX_BINDING_SATISFIED
                     : TTX_BINDING_UNSUPPORTED;
        }};
    },
    [](const void* source) {
      return &static_cast<const Image*>(source)->representation;
    },
  };

  return {this, &operations};
}

auto Imaging::Publication::Image::get_query() const
    -> Ttx::Semantic::Negotiation::Query {
  return Ttx::Semantic::Negotiation::Query(
      {this,
       [](const void* source, perimortem_uuid id,
          ttx_storage requested) -> ttx_binding_status {
         if (System::Uuid(id) == Imaging::Contracts::Offers::contract_id) {
           const image_offers api{
             source,
             [](const void* source, image_offer_visitor visitor) {
               static_cast<const Image*>(source)->visit_offers(visitor);
             },
             [](const void* source, perimortem_uuid contract, U32 width,
                U32 height) {
               return static_cast<const Image*>(source)->admit(
                   System::Uuid(contract), width, height);
             }};
           return static_cast<ttx_binding_status>(
               Ttx::Semantic::Negotiation::Binding::provide<
                   Imaging::Contracts::Offers>(
                   api, Ttx::Data::Form::Storage(requested)));
         }
         return static_cast<ttx_binding_status>(
             static_cast<const Image*>(source)->fulfill(
                 System::Uuid(id), Ttx::Data::Form::Storage(requested)));
       },
       [](const void* source, perimortem_uuid id) -> ttx_binding_status {
         if (System::Uuid(id) == Imaging::Contracts::Offers::contract_id) {
           return TTX_BINDING_SATISFIED;
         }
         return static_cast<ttx_binding_status>(
             static_cast<const Image*>(source)->supports(System::Uuid(id)));
       }});
}

// The native helper offers the standard image operations it actually supplies.
// A provider can override this observation to advertise its own choices and
// limits. Consumers never need to identify the native implementation.
auto Imaging::Publication::Image::standard_offers(
    image_offer_visitor visitor) const -> void {
  const image_offer offers[] = {
    {{GODOT_IMAGE_INVERT_ID_HIGH, GODOT_IMAGE_INVERT_ID_LOW},
     {reinterpret_cast<const U8*>("invert"), 6},
     IMAGE_INPUT_NONE,
     0,
     0,
     0},
    {{GODOT_IMAGE_CONVOLVE_ID_HIGH, GODOT_IMAGE_CONVOLVE_ID_LOW},
     {reinterpret_cast<const U8*>("convolve"), 8},
     IMAGE_INPUT_KERNEL,
     1,
     1023,
     2},
    {{GODOT_IMAGE_COMPOSITE_ID_HIGH, GODOT_IMAGE_COMPOSITE_ID_LOW},
     {reinterpret_cast<const U8*>("composite"), 9},
     IMAGE_INPUT_IMAGE,
     0,
     0,
     0},
  };
  for (const auto& offer : offers) {
    if (supports(System::Uuid(offer.contract)) !=
        Ttx::Semantic::Negotiation::Binding::Status::Unsupported) {
      visitor.visit(visitor.source, offer);
    }
  }
}

auto Imaging::Publication::Image::standard_admission(
    System::Uuid contract,
    U32 width,
    U32 height) const -> image_admission {
  const auto status = supports(contract);
  if (status != Ttx::Semantic::Negotiation::Binding::Status::Satisfied) {
    return {static_cast<ttx_binding_status>(status), {nullptr, 0}};
  }
  if (contract ==
          System::Uuid(
              GODOT_IMAGE_CONVOLVE_ID_HIGH, GODOT_IMAGE_CONVOLVE_ID_LOW) &&
      (!width || !height || width > 1023 || height > 1023 || !(width & 1) ||
       !(height & 1))) {
    constexpr auto reason =
        "Kernel dimensions must be odd and between one and 1023."_view;
    return {TTX_BINDING_REJECTED, {reason.get_data(), reason.get_size()}};
  }
  return {TTX_BINDING_SATISFIED, {nullptr, 0}};
}
