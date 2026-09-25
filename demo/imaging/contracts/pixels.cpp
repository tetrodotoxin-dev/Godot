// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/imaging/contracts/pixels.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "ttx/semantic/flows/copy.hpp"

using namespace Godot::Demo;
using namespace Perimortem;

auto Imaging::Contracts::Pixels::validate(
    U32 w,
    U32 h,
    Core::View::Bytes pixels) -> Core::View::Bytes {
  if (!w || !h || w > 4096 || h > 4096) {
    return "Image requires positive dimensions up to 4096."_view;
  }

  if (Count(w) * h * 4 != pixels.get_size()) {
    return "Image requires exactly width times height RGBA8 pixels."_view;
  }

  return Core::View::Bytes();
}

auto Imaging::Contracts::Pixels::read(image_object borrowed)
    -> Utility::Result<Memory::Dynamic::Bytes, Core::View::Bytes> {
  const auto& representation =
      *borrowed.operations->representation(borrowed.source);
  Ttx::Semantic::Transport::Flow flow;
  const auto connected = flow.connect(
      Ttx::Semantic::Transport::Flow::reader(representation),
      Ttx::Semantic::Negotiation::Query(
          borrowed.operations->pixels(borrowed.source)));
  if (connected != Ttx::Semantic::Transport::Flow::Status::Success) {
    return "Image pixel transport could not be established."_view;
  }

  Memory::Dynamic::Bytes output;
  output.forgetful_resize(representation.get_extent());
  auto storage =
      Ttx::Data::Form::Storage::create(representation, output.get_access());
  const auto status = storage.visit(
      [&](Ttx::Data::Form::Storage target) {
        return Ttx::Semantic::Flows::Copy::flow(flow, target);
      },
      [](Ttx::Data::Status error) { return error; });
  if (status != Ttx::Data::Status::Success) {
    return "Image pixel observation failed."_view;
  }

  return output;
}
