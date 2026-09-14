// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/callable.hpp>

#include "perimortem/core/option.hpp"

#include "providers/gdscript/invocation.hpp"
#include "providers/image.hpp"

namespace Godot::Providers::Gdscript {

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
class Image : public Providers::Image {
 public:
  static auto
      create(U32 width, U32 height, const godot::Ref<godot::RefCounted>& object)
          -> Perimortem::Utility::Result<Image&, Perimortem::Core::View::Bytes>;

 protected:
  auto fulfill(Perimortem::System::Uuid contract) const -> Perimortem::Utility::
      Result<Ttx::Semantic::Binding, Ttx::Semantic::Binding::Failure> override;
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
      -> Perimortem::Core::Option<Ttx::Semantic::Binding::Failure>;
  auto receive(
      Perimortem::Utility::
          Result<godot::Variant, Perimortem::Core::View::Bytes>) const
      -> Perimortem::Utility::Result<Image&, Perimortem::Core::View::Bytes>;
  auto invert() const
      -> Perimortem::Utility::Result<Image&, Perimortem::Core::View::Bytes>;
  auto convolve(image_kernel) const
      -> Perimortem::Utility::Result<Image&, Perimortem::Core::View::Bytes>;
  auto composite(image_object) const
      -> Perimortem::Utility::Result<Image&, Perimortem::Core::View::Bytes>;

  godot::Ref<godot::RefCounted> object;
  godot::Callable publication;
  godot::Callable pixels;
  mutable godot::Callable inversion;
  mutable godot::Callable convolution;
  mutable godot::Callable composition;
  mutable Perimortem::Memory::Dynamic::Bytes errors;
};

}  // namespace Godot::Providers::Gdscript
