// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "imaging/graph/image.hpp"

#include "imaging/contracts/pixels.hpp"
#include "imaging/graph/observation.hpp"
#include "ttx/semantic/realization/simulacra.hpp"

using namespace Godot;
using namespace Perimortem;

Imaging::Graph::Image::Image(image_object borrowed, Provider& provider)
    : value(borrowed), provider(&provider) {
  provider.retain();
  value.operations->retain(value.source);
}

Imaging::Graph::Image::Image(const Image& other)
    : Image(other.value, *other.provider) {}

Imaging::Graph::Image::Image(Image&& other)
    : value(other.value), provider(other.provider) {
  other.provider = nullptr;
}

Imaging::Graph::Image::~Image() {
  if (provider) {
    value.operations->release(value.source);
    provider->release();
  }
}

auto Imaging::Graph::Image::operator=(const Image& other) -> Image& {
  Image retained(other);
  return *this = Core::Data::take(retained);
}

auto Imaging::Graph::Image::operator=(Image&& other) -> Image& {
  if (this == &other) {
    return *this;
  }

  if (provider) {
    value.operations->release(value.source);
    provider->release();
  }

  value = other.value;
  provider = other.provider;
  other.provider = nullptr;
  return *this;
}

auto Imaging::Graph::Image::create(
    Provider& provider,
    U32 w,
    U32 h,
    Core::View::Bytes pixels,
    Memory::Allocator::Arena& errors)
    -> Utility::Result<Image, Core::View::Bytes> {
  Observation observation;
  image_object output = {};
  const auto error = provider.create(w, h, pixels, output);
  if (error.size) {
    return errors.proxy(Core::View::Bytes(error.data, error.size));
  }

  Image result(output, provider);
  output.operations->release(output.source);
  return result;
}

auto Imaging::Graph::Image::get_width() const -> U32 {
  Observation observation;
  return value.operations->dimensions(value.source).width;
}

auto Imaging::Graph::Image::get_height() const -> U32 {
  Observation observation;
  return value.operations->dimensions(value.source).height;
}

auto Imaging::Graph::Image::get_query() const
    -> Ttx::Semantic::Negotiation::Query {
  Observation observation;
  return Ttx::Semantic::Negotiation::Query(
      value.operations->query(value.source));
}

auto Imaging::Graph::Image::supports(System::Uuid id) const
    -> Ttx::Semantic::Negotiation::Binding::Status {
  Observation observation;
  return get_query().supports(id);
}

auto Imaging::Graph::Image::bind_interface(
    System::Uuid id,
    Ttx::Data::Form::Storage requested) const
    -> Ttx::Semantic::Negotiation::Binding::Status {
  Observation observation;
  return get_query().bind(id, requested);
}

auto Imaging::Graph::Image::bind_operation(const Operation& operation) const
    -> Operation::Acquisition {
  Observation observation;
  return operation.bind(get_query());
}

auto Imaging::Graph::Image::binding_error(
    Ttx::Semantic::Negotiation::Binding::Failure failure) -> Core::View::Bytes {
  switch (failure) {
  case Ttx::Semantic::Negotiation::Binding::Failure::Unsupported:
    return "The image provider does not support this operation."_view;
  case Ttx::Semantic::Negotiation::Binding::Failure::Pending:
    return "The image operation cannot yet be fulfilled."_view;
  case Ttx::Semantic::Negotiation::Binding::Failure::Rejected:
    return "The image provider rejected this operation contract."_view;
  }

  return "The image provider returned an invalid binding failure."_view;
}

auto Imaging::Graph::Image::apply(
    const Operation& operation,
    const Kernel* kernel,
    const Image* overlay,
    Memory::Allocator::Arena& errors,
    const Invocation* cached) const
    -> Utility::Result<Image, Core::View::Bytes> {
  Observation observation;
  Core::Option<Invocation> acquired;
  Core::View::Bytes failure;
  if (!cached) {
    bind_operation(operation).visit(
        [&](const Invocation& binding) { acquired = binding; },
        [&](Ttx::Semantic::Negotiation::Binding::Failure error) {
          failure = binding_error(error);
        });
    if (!acquired) {
      return failure;
    }

    cached = &*acquired;
  }

  const image_kernel coefficients = kernel ? kernel->get_abi() : image_kernel();
  const image_object other = overlay ? overlay->get_abi() : image_object();
  return cached
      ->invoke(kernel ? &coefficients : nullptr, overlay ? &other : nullptr)
      .visit(
          [&](image_object output)
              -> Utility::Result<Image, Core::View::Bytes> {
            Image image(output, *provider);
            output.operations->release(output.source);
            return image;
          },
          [&](Core::View::Bytes error)
              -> Utility::Result<Image, Core::View::Bytes> {
            return errors.proxy(error);
          });
}

auto Imaging::Graph::Image::read_pixels(Memory::Allocator::Arena&) const
    -> Utility::Result<Memory::Dynamic::Bytes, Core::View::Bytes> {
  Observation observation;
  return Imaging::Contracts::Pixels::read(value);
}
