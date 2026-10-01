// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "demo/imaging/cuda/allocation.hpp"
#include "demo/imaging/publication/image.hpp"

namespace Godot::Demo::Imaging::Cuda {

// A CUDA image keeps its payload in a device allocation so a chain of filters
// can execute without round trips through host memory. Derivation transfers a
// new allocation into another immutable image. Its Runtime remains retained by
// that allocation. The public pixel observation downloads only when requested.
// CPU inputs are consumed through their contract, never as this native class.
class Image : public Imaging::Publication::Image {
 public:
  static auto create(
      Runtime& runtime,
      U32 width,
      U32 height,
      Perimortem::Core::View::Bytes pixels)
      -> Perimortem::Utility::Result<Image&, Perimortem::Core::View::Bytes>;
  auto derive(Allocation&& pixels) const -> Image&;
  auto get_runtime() const -> Runtime& { return runtime; }
  auto get_buffer() const -> CUdeviceptr { return data.get(); }

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
      Runtime& runtime,
      U32 width,
      U32 height,
      Allocation&& pixels);
  Image(U8* allocation, const Image& source, Allocation&& pixels);
  Runtime& runtime;
  Allocation data;
};

}  // namespace Godot::Demo::Imaging::Cuda
