// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "adapters/cuda/ttx_cuda_buffer.hpp"

#include <godot_cpp/core/class_db.hpp>

using namespace Godot;
using namespace Perimortem;

auto Godot::Adapters::Cuda::TtxCudaBuffer::_bind_methods() -> void {
  godot::ClassDB::bind_method(
      godot::D_METHOD("write", "bytes", "offset"), &TtxCudaBuffer::write,
      DEFVAL(0));
  godot::ClassDB::bind_method(godot::D_METHOD("read"), &TtxCudaBuffer::read);
  godot::ClassDB::bind_method(
      godot::D_METHOD("get_size"), &TtxCudaBuffer::get_size);
}

auto Godot::Adapters::Cuda::TtxCudaBuffer::adopt(
    Ttx::Concept::Modules::Module module,
    Ttx::Semantic::Ownership::Publication publication)
    -> godot::Ref<TtxCudaBuffer> {
  godot::Ref<TtxCudaBuffer> result;
  publication.get_query().bind<::Cuda::Contracts::Buffer>().visit(
      [&](::Cuda::Contracts::Buffer buffer) {
        result.instantiate();
        result->module = Core::Data::take(module);
        result->owner = Core::Data::take(publication);
        result->buffer = buffer;
      },
      [](Ttx::Semantic::Negotiation::Binding::Failure) {});
  return result;
}

auto Godot::Adapters::Cuda::TtxCudaBuffer::get_address() const -> U64 {
  return buffer ? buffer->get_address() : 0;
}

auto Godot::Adapters::Cuda::TtxCudaBuffer::get_size() const -> int64_t {
  return buffer ? buffer->get_size() : 0;
}

auto Godot::Adapters::Cuda::TtxCudaBuffer::write(
    const godot::PackedByteArray& bytes,
    int64_t offset) -> bool {
  return buffer && offset >= 0 &&
         buffer->write(offset, {bytes.ptr(), Count(bytes.size())}) ==
             Ttx::Data::Status::Success;
}

auto Godot::Adapters::Cuda::TtxCudaBuffer::read() const
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
