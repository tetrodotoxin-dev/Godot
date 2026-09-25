// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"

#include "demo/imaging/graph/kernel.hpp"
#include "demo/imaging/graph/operation.hpp"
#include "demo/imaging/graph/provider.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/semantic/negotiation/query.hpp"

namespace Godot::Demo::Imaging::Graph {

// A provider can return an image whose pixels and implementation live outside
// the host. Keeping only its receiver pointer would lose both lifetimes when
// the source Resource is replaced. Image therefore retains the native value
// and its loaded Provider together, releasing the value before its code can
// unload. Copying this owner keeps the same immutable publication alive.
//
// As a host Abstract it also lends the provider's Query to graph consumers.
// That query preserves the encountered implementation. It does not recover a
// native CPU/CUDA class. Pixel readback is an explicit observation into new
// host storage, independent of acquiring an operation on the image.
class Image {
 public:
  Image(image_object borrowed, Provider& provider);
  Image(const Image&);
  Image(Image&&);
  ~Image();
  auto operator=(const Image&) -> Image&;
  auto operator=(Image&&) -> Image&;

  static auto create(
      Provider&,
      U32 width,
      U32 height,
      Perimortem::Core::View::Bytes pixels,
      Perimortem::Memory::Allocator::Arena& errors)
      -> Perimortem::Utility::Result<Image, Perimortem::Core::View::Bytes>;

  auto get_abi() const -> image_object { return value; }
  auto get_provider() const -> Provider& { return *provider; }
  auto get_width() const -> U32;
  auto get_height() const -> U32;
  auto get_query() const -> Ttx::Semantic::Negotiation::Query;
  auto supports(Perimortem::System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status;
  auto bind_interface(
      Perimortem::System::Uuid id,
      Ttx::Data::Form::Storage requested) const
      -> Ttx::Semantic::Negotiation::Binding::Status;

  auto bind_operation(const Operation&) const -> Operation::Acquisition;

  // The graph and Godot adapter present the same refusal without trying to
  // obtain another implementation while formatting the failure.
  static auto binding_error(
      Ttx::Semantic::Negotiation::Binding::Failure failure)
      -> Perimortem::Core::View::Bytes;

  // A cached binding belongs to this retained image. Call refreshes it when
  // its receiver publication changes. Changing only an overlay reuses it.
  auto apply(
      const Operation&,
      const Kernel* kernel,
      const Image* overlay,
      Perimortem::Memory::Allocator::Arena& errors,
      const Invocation* cached = nullptr) const
      -> Perimortem::Utility::Result<Image, Perimortem::Core::View::Bytes>;

  auto read_pixels(Perimortem::Memory::Allocator::Arena& errors) const
      -> Perimortem::Utility::Result<
          Perimortem::Memory::Dynamic::Bytes,
          Perimortem::Core::View::Bytes>;

  constexpr auto get_data() const -> Perimortem::Core::View::Bytes {
    return "Image"_view;
  }

 private:
  image_object value;
  Provider* provider;
};

}  // namespace Godot::Demo::Imaging::Graph
