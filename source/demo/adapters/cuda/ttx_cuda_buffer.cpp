// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/adapters/cuda/ttx_cuda_buffer.hpp"

#include <godot_cpp/core/class_db.hpp>

using namespace Godot::Demo;
using namespace Perimortem;

auto Godot::Demo::Adapters::Cuda::TtxCudaBuffer::_bind_methods() -> void {
  godot::ClassDB::bind_method(
      godot::D_METHOD("write", "bytes", "offset"), &TtxCudaBuffer::write,
      DEFVAL(0));
  godot::ClassDB::bind_method(godot::D_METHOD("read"), &TtxCudaBuffer::read);
  godot::ClassDB::bind_method(
      godot::D_METHOD("get_size"), &TtxCudaBuffer::get_size);
}

auto Godot::Demo::Adapters::Cuda::TtxCudaBuffer::adopt(
    Ttx::Concept::Policies::Borrowed subject) -> godot::Ref<TtxCudaBuffer> {
  using namespace Ttx::Semantic::Negotiation;
  godot::Ref<TtxCudaBuffer> result;
  subject.bind<::Cuda::Contracts::Buffer>().visit(
      [&](::Cuda::Contracts::Buffer api) {
        result.instantiate();
        result->owner = subject;
        result->buffer = api;
      },
      [&](Binding::Failure) { subject.release(); });
  return result;
}

Godot::Demo::Adapters::Cuda::TtxCudaBuffer::~TtxCudaBuffer() {
  if (owner) {
    owner->release();
  }
}

auto Godot::Demo::Adapters::Cuda::TtxCudaBuffer::get_address() const -> U64 {
  return buffer ? buffer->get_address() : 0;
}

auto Godot::Demo::Adapters::Cuda::TtxCudaBuffer::get_size() const -> int64_t {
  return buffer ? buffer->get_size() : 0;
}

auto Godot::Demo::Adapters::Cuda::TtxCudaBuffer::write(
    const godot::PackedByteArray& bytes,
    int64_t offset) -> bool {
  return buffer && offset >= 0 &&
         buffer->write(offset, {bytes.ptr(), Count(bytes.size())}) ==
             Ttx::Data::Status::Success;
}

auto Godot::Demo::Adapters::Cuda::TtxCudaBuffer::read() const
    -> godot::PackedByteArray {
  godot::PackedByteArray result;
  if (!buffer) {
    return result;
  }

  result.resize(buffer->get_size());
  if (buffer->read(0, {result.ptrw(), Count(result.size())}) !=
      Ttx::Data::Status::Success) {
    result.clear();
  }

  return result;
}
