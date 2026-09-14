// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "images/image.hpp"

#include "contracts/pixels.hpp"
#include "images/observation.hpp"
#include "ttx/semantic/simulacra.hpp"

using namespace Godot;
using namespace Perimortem;

Images::Image::Image(image_object borrowed, Provider& provider)
    : value(borrowed), provider(&provider) {
  provider.retain();
  value.operations->retain(value.source);
}

Images::Image::Image(const Image& other)
    : Image(other.value, *other.provider) {}

Images::Image::Image(Image&& other)
    : value(other.value), provider(other.provider) {
  other.provider = nullptr;
}

Images::Image::~Image() {
  if (provider) {
    value.operations->release(value.source);
    provider->release();
  }
}

auto Images::Image::operator=(const Image& other) -> Image& {
  Image retained(other);
  return *this = Core::Data::take(retained);
}

auto Images::Image::operator=(Image&& other) -> Image& {
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

auto Images::Image::create(
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

auto Images::Image::get_width() const -> U32 {
  Observation observation;
  return value.operations->dimensions(value.source).width;
}

auto Images::Image::get_height() const -> U32 {
  Observation observation;
  return value.operations->dimensions(value.source).height;
}

auto Images::Image::get_query() const -> Ttx::Semantic::Query {
  Observation observation;
  return Ttx::Semantic::Query(value.operations->query(value.source));
}

auto Images::Image::bind_interface(System::Uuid id) const -> Utility::
    Result<Ttx::Semantic::Binding, Ttx::Semantic::Binding::Failure> {
  Observation observation;
  return get_query().bind(id);
}

auto Images::Image::bind_operation(const Operation& operation) const
    -> Utility::
        Result<Ttx::Semantic::Binding, Ttx::Semantic::Binding::Failure> {
  Observation observation;
  return Ttx::Semantic::Simulacra::fulfill(
      get_query(), operation.get_id(),
      Ttx::Semantic::Thunk::Convention::SystemVAMD64,
      operation.get_representation());
}

auto Images::Image::binding_error(Ttx::Semantic::Binding::Failure failure)
    -> Core::View::Bytes {
  switch (failure) {
  case Ttx::Semantic::Binding::Failure::Unsupported:
    return "The image provider does not support this operation."_view;
  case Ttx::Semantic::Binding::Failure::Pending:
    return "The image operation cannot yet be fulfilled."_view;
  case Ttx::Semantic::Binding::Failure::Rejected:
    return "The image provider rejected this operation contract."_view;
  }

  return "The image provider returned an invalid binding failure."_view;
}

auto Images::Image::apply(
    const Operation& operation,
    const Kernel* kernel,
    const Image* overlay,
    Memory::Allocator::Arena& errors,
    const Ttx::Semantic::Binding* cached) const
    -> Utility::Result<Image, Core::View::Bytes> {
  Observation observation;
  Core::Option<Ttx::Semantic::Binding> acquired;
  Core::View::Bytes failure;
  if (!cached) {
    bind_operation(operation).visit(
        [&](const Ttx::Semantic::Binding& binding) { acquired = binding; },
        [&](Ttx::Semantic::Binding::Failure error) {
          failure = binding_error(error);
        });
    if (!acquired) {
      return failure;
    }

    cached = &*acquired;
  }

  const image_kernel coefficients = kernel ? kernel->get_abi() : image_kernel();
  const image_object other = overlay ? overlay->get_abi() : image_object();
  return operation
      .invoke(
          *cached, kernel ? &coefficients : nullptr, overlay ? &other : nullptr)
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

auto Images::Image::read_pixels(Memory::Allocator::Arena&) const
    -> Utility::Result<Memory::Dynamic::Bytes, Core::View::Bytes> {
  Observation observation;
  return Contracts::Pixels::read(value);
}
