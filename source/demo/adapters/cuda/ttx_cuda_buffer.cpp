// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/adapters/cuda/ttx_cuda_buffer.hpp"

#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/variant/utility_functions.hpp"

using namespace Godot::Demo::Adapters::Cuda;

void TtxCudaBuffer::_bind_methods() {
  godot::ClassDB::bind_method(
      godot::D_METHOD("write", "bytes", "offset"), &TtxCudaBuffer::write,
      DEFVAL(0));
  godot::ClassDB::bind_method(godot::D_METHOD("read"), &TtxCudaBuffer::read);
  godot::ClassDB::bind_method(
      godot::D_METHOD("get_size"), &TtxCudaBuffer::get_size);
}

#ifndef TOOLCHAIN_WASM

auto TtxCudaBuffer::adopt(ttx_retained acquired, Library& library)
    -> godot::Ref<TtxCudaBuffer> {
  const auto abstract = acquired.operations->get_abstract(acquired.context);
  cuda_buffer api{};
  if (abstract.operations->bind(
          abstract.context, {TTX_CUDA_BUFFER_HIGH, TTX_CUDA_BUFFER_LOW},
          &api) != TTX_NEGOTIATION_SATISFIED) {
    if (acquired.operations->release(acquired.context) != TTX_RELEASE_SUCCESS) {
      godot::UtilityFunctions::push_error("CUDA buffer cleanup failed.");
    }

    return {};
  }

  godot::Ref<TtxCudaBuffer> result;
  result.instantiate();
  result->retained = acquired;
  result->buffer = api;
  library.retain();
  result->library = &library;
  return result;
}

TtxCudaBuffer::~TtxCudaBuffer() {
  if (retained.operations &&
      retained.operations->release(retained.context) != TTX_RELEASE_SUCCESS) {
    godot::UtilityFunctions::push_error("CUDA buffer cleanup failed.");
  }

  if (library) {
    library->release();
  }
}

auto TtxCudaBuffer::get_address() const -> U64 {
  return buffer.operations ? buffer.operations->address(buffer.context) : 0;
}

auto TtxCudaBuffer::get_size() const -> GDExtensionInt {
  return buffer.operations ? buffer.operations->size(buffer.context) : 0;
}

auto TtxCudaBuffer::write(const godot::PackedByteArray& bytes, GDExtensionInt offset)
    -> bool {
  return buffer.operations && offset >= 0 &&
         buffer.operations->write(
             buffer.context, offset, {bytes.ptr(), Count(bytes.size())}) ==
             CUDA_SUCCESS_STATUS;
}

auto TtxCudaBuffer::read() const -> godot::PackedByteArray {
  godot::PackedByteArray result;
  if (!buffer.operations) {
    return result;
  }

  result.resize(buffer.operations->size(buffer.context));
  if (buffer.operations->read(
          buffer.context, 0, result.ptrw(), result.size()) !=
      CUDA_SUCCESS_STATUS) {
    result.clear();
  }

  return result;
}

#else

// Godot keeps the same Resource names so projects can report unavailable
// CUDA capabilities through their existing provider-selection UI.
TtxCudaBuffer::~TtxCudaBuffer() = default;
auto TtxCudaBuffer::get_address() const -> U64 {
  return 0;
}

auto TtxCudaBuffer::get_size() const -> GDExtensionInt {
  return 0;
}

auto TtxCudaBuffer::write(const godot::PackedByteArray& bytes, GDExtensionInt offset)
    -> bool {
  return false;
}

auto TtxCudaBuffer::read() const -> godot::PackedByteArray {
  return {};
}

#endif
