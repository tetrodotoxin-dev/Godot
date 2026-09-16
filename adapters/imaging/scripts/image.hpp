// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/callable.hpp>

#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "adapters/imaging/scripts/invocation.hpp"
#include "imaging/publication/image.hpp"

namespace Godot::Adapters::Imaging::Scripts {

// A script image supplies behavior without exporting a native object layout.
// This publication retains that RefCounted object and exposes the existing C
// image contracts. Successful fulfillment retains the selected Callable, so a
// later invocation enters the script once for the complete image operation.
// The script owns its pixels and any objects to which its Callables delegate.
//
// The publication is immutable. Source replacement creates another script
// object, allowing old bindings and snapshots to keep their original answers.
// All calls and final releases run on the Godot worker that admitted the
// object.
class Image : public Godot::Imaging::Publication::Image {
 public:
  static auto
      create(U32 width, U32 height, const godot::Ref<godot::RefCounted>& object)
          -> Perimortem::Utility::Result<Image&, Perimortem::Core::View::Bytes>;

 protected:
  auto supports(Perimortem::System::Uuid contract) const
      -> Ttx::Semantic::Negotiation::Binding::Status override;
  auto fulfill(
      Perimortem::System::Uuid contract,
      Ttx::Data::Form::Storage requested) const
      -> Ttx::Semantic::Negotiation::Binding::Status override;
  auto visit_offers(image_offer_visitor visitor) const -> void override;
  auto admit(Perimortem::System::Uuid contract, U32 width, U32 height) const
      -> image_admission override;
  auto read_pixels(Perimortem::Core::Access::Bytes target) const
      -> Perimortem::Core::View::Bytes override;

 private:
  Image(
      U8* allocation,
      U32 width,
      U32 height,
      const godot::Ref<godot::RefCounted>& object);

  Image(
      U8* allocation,
      const Image& source,
      const godot::Ref<godot::RefCounted>& object);

  // These helpers use the same retained publication and diagnostic storage as
  // the C thunks. Keeping them here makes their borrowed result lifetime
  // visible without exposing mutable Callables or error buffers as another
  // interface.
  auto select(Perimortem::System::Uuid contract, godot::Callable& slot) const
      -> Perimortem::Core::Option<Ttx::Semantic::Negotiation::Binding::Failure>;
  auto receive(
      Perimortem::Utility::
          Result<godot::Variant, Perimortem::Core::View::Bytes>) const
      -> Perimortem::Utility::Result<Image&, Perimortem::Core::View::Bytes>;
  auto invert() const
      -> Perimortem::Utility::Result<Image&, Perimortem::Core::View::Bytes>;
  auto convolve(image_kernel, const godot::Callable&) const
      -> Perimortem::Utility::Result<Image&, Perimortem::Core::View::Bytes>;
  auto composite(image_object, const godot::Callable&) const
      -> Perimortem::Utility::Result<Image&, Perimortem::Core::View::Bytes>;

  // Dynamic publications keep their selected script receiver stable
  // until the image ends, even if another binding grows this owner's inventory.
  struct Selection {
    const Image* image = nullptr;
    godot::Callable callable;
  };
  mutable Perimortem::Memory::Dynamic::Vector<
      Perimortem::Core::Object<Selection>>
      selected;
  godot::Ref<godot::RefCounted> object;
  godot::Callable publication;
  godot::Callable pixels;
  mutable godot::Callable inversion;
  mutable godot::Callable convolution;
  mutable godot::Callable composition;
  mutable Perimortem::Memory::Dynamic::Bytes errors;
};

}  // namespace Godot::Adapters::Imaging::Scripts
