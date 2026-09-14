// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "providers/cuda/samples.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "providers/cuda/current_context.hpp"

using namespace Godot;
using namespace Perimortem;

static const sample_operations operations = {
  [](const void* source, U32 seed, U32 first, U32 count, U64* output)
      -> ttx_data_status {
    return const_cast<Providers::Cuda::Samples*>(
               static_cast<const Providers::Cuda::Samples*>(source))
        ->count(seed, first, count, output);
  },
};

Providers::Cuda::Samples::Samples(Runtime& runtime, CUdeviceptr counter)
    : runtime(runtime),
      counter(counter),
      publication(this, operations, [](const void* source) {
        Core::Object<>(reinterpret_cast<U8*>(const_cast<void*>(source)))
            .release();
      }) {}

Providers::Cuda::Samples::~Samples() {
  {
    CurrentContext current(runtime.get_context());
    if (current.status != CUDA_SUCCESS) {
      Core::Diagnostics::Log::fatal(
          "CUDA could not enter the sampling context."_view);
    }

    const auto status = cuMemFree(counter);
    if (status != CUDA_SUCCESS) {
      Core::Diagnostics::Log::fatal(
          "CUDA could not release sampling storage."_view);
    }
  }

  runtime.release();
}

static auto allocate(Providers::Cuda::Runtime& runtime, CUdeviceptr& counter)
    -> Ttx::Data::Status {
  Providers::Cuda::CurrentContext current(runtime.get_context());
  if (current.status != CUDA_SUCCESS) {
    return Ttx::Data::Status::IoError;
  }

  const auto status = cuMemAlloc(&counter, sizeof(U64));
  if (status != CUDA_SUCCESS) {
    return Ttx::Data::Status::IoError;
  }

  return Ttx::Data::Status::Success;
}

auto Providers::Cuda::Samples::create()
    -> Utility::Result<sample_provider, Ttx::Data::Status> {
  return Runtime::create().visit(
      [](Runtime& runtime)
          -> Utility::Result<sample_provider, Ttx::Data::Status> {
        CUdeviceptr counter;
        const auto status = allocate(runtime, counter);
        if (status != Ttx::Data::Status::Success) {
          runtime.release();
          return status;
        }

        static const Core::Object<>::Descriptor descriptor(
            sizeof(Samples), alignof(Samples), [](U8* storage) {
              reinterpret_cast<Samples*>(storage)->~Samples();
            });
        auto storage = Core::Object<>::create(descriptor).get_payload();
        auto& result = *new (storage, Core::Placement::Construct)
                           Samples(runtime, counter);
        return result.publication.get_provider();
      },
      [](Core::View::Bytes)
          -> Utility::Result<sample_provider, Ttx::Data::Status> {
        return Ttx::Data::Status::IoError;
      });
}

auto Providers::Cuda::Samples::count(U32 seed, U32 first, U32 size, U64* output)
    -> ttx_data_status {
  if (U64(first) + size > (U64(1) << 32)) {
    return TTX_DATA_BOUNDS;
  }

  if (size == 0) {
    *output = 0;
    return TTX_DATA_SUCCESS;
  }

  CurrentContext current(runtime.get_context());
  if (current.status != CUDA_SUCCESS) {
    return TTX_DATA_IO_ERROR;
  }

  const auto cleared = cuMemsetD32(counter, 0, 2);
  if (cleared != CUDA_SUCCESS) {
    return TTX_DATA_IO_ERROR;
  }

  void* arguments[] = {&seed, &first, &size, &counter};
  const U32 threads = size < 65536 ? size : 65536;
  const auto error =
      runtime.get_program().launch("sample_disk", threads, arguments);
  if (!error.is_empty()) {
    return TTX_DATA_IO_ERROR;
  }

  // Read into local storage first. The synchronous device copy establishes
  // completion, and a transport failure cannot publish a partial caller value.
  U64 hits;
  if (cuMemcpyDtoH(&hits, counter, sizeof(hits)) != CUDA_SUCCESS) {
    return TTX_DATA_IO_ERROR;
  }

  *output = hits;
  return TTX_DATA_SUCCESS;
}

PERIMORTEM_C __attribute__((visibility("default"))) ttx_data_status
    godot_sample_provider_open_v1(sample_provider* output) {
  return Providers::Cuda::Samples::create().visit(
      [&](sample_provider provider) -> ttx_data_status {
        *output = provider;
        return TTX_DATA_SUCCESS;
      },
      [](Ttx::Data::Status error) {
        return static_cast<ttx_data_status>(error);
      });
}
