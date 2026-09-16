// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"

#include "imaging/publication/image.hpp"

namespace Godot::Imaging::Cpu {

// The CPU provider owns ordinary RGBA8 bytes and can lend them directly to its
// own algorithms. A derived result takes ownership of a new buffer, leaving
// earlier publications intact for snapshots and cached graph branches.
// Foreign consumers receive the public image contract. Only this module's
// thunks recover the native Image and its writable storage.
class Image : public Imaging::Publication::Image {
 public:
  static auto
      create(U32 width, U32 height, Perimortem::Core::View::Bytes pixels)
          -> Perimortem::Utility::Result<Image&, Perimortem::Core::View::Bytes>;
  auto derive(Perimortem::Memory::Dynamic::Bytes&& pixels) const -> Image&;
  auto get_pixels() const -> Perimortem::Core::View::Bytes {
    return data.get_view();
  }

 protected:
  auto supports(Perimortem::System::Uuid contract) const
      -> Ttx::Semantic::Negotiation::Binding::Status override;
  auto fulfill(
      Perimortem::System::Uuid contract,
      Ttx::Data::Form::Storage requested) const
      -> Ttx::Semantic::Negotiation::Binding::Status override;
  auto read_pixels(Perimortem::Core::Access::Bytes target) const
      -> Perimortem::Core::View::Bytes override;

 private:
  Image(
      U8* allocation,
      U32 width,
      U32 height,
      Perimortem::Memory::Dynamic::Bytes&& pixels);
  Image(
      U8* allocation,
      const Image& source,
      Perimortem::Memory::Dynamic::Bytes&& pixels);
  Perimortem::Memory::Dynamic::Bytes data;
};

}  // namespace Godot::Imaging::Cpu
