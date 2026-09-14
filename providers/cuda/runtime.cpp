// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "providers/cuda/runtime.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "providers/cuda/current_context.hpp"

using namespace Godot;
using namespace Perimortem;

static auto driver_error(CUresult status) -> Core::View::Bytes {
  const char* message = nullptr;
  cuGetErrorString(status, &message);
  return message ? Core::NullTerminated::to_view(message)
                 : "CUDA driver failure"_view;
}

auto Providers::Cuda::Runtime::create()
    -> Utility::Result<Runtime&, Core::View::Bytes> {
  static const Core::Object<>::Descriptor descriptor(
      sizeof(Runtime), alignof(Runtime),
      [](U8* payload) { reinterpret_cast<Runtime*>(payload)->~Runtime(); });
  auto storage = Core::Object<>::create(descriptor).get_payload();
  auto runtime = new (storage, Core::Placement::Construct) Runtime();
  const auto error = runtime->prepare();
  if (!error.is_empty()) {
    runtime->release();
    return error;
  }

  return *runtime;
}

void Providers::Cuda::Runtime::retain() {
  Core::Object<>(reinterpret_cast<U8*>(this)).retain();
}

void Providers::Cuda::Runtime::release() {
  Core::Object<>(reinterpret_cast<U8*>(this)).release();
}

auto Providers::Cuda::Runtime::prepare() -> Core::View::Bytes {
  auto status = cuInit(0);
  if (status != CUDA_SUCCESS) {
    return driver_error(status);
  }

  status = cuDeviceGet(&device, 0);
  if (status != CUDA_SUCCESS) {
    return driver_error(status);
  }

  status = cuDevicePrimaryCtxRetain(&context, device);
  if (status != CUDA_SUCCESS) {
    return driver_error(status);
  }

  return program.prepare(device, context);
}

auto Providers::Cuda::Runtime::allocate(Count size)
    -> Utility::Result<CUdeviceptr, Core::View::Bytes> {
  CurrentContext current(context);
  if (current.status != CUDA_SUCCESS) {
    return driver_error(current.status);
  }

  CUdeviceptr data = 0;
  const auto status = cuMemAlloc(&data, size);
  if (status != CUDA_SUCCESS) {
    return driver_error(status);
  }

  ++live_images;
  return data;
}

auto Providers::Cuda::Runtime::write(CUdeviceptr data, Core::View::Bytes bytes)
    -> Core::View::Bytes {
  CurrentContext current(context);
  if (current.status != CUDA_SUCCESS) {
    return driver_error(current.status);
  }

  const auto status = cuMemcpyHtoD(data, bytes.get_data(), bytes.get_size());
  return status == CUDA_SUCCESS ? Core::View::Bytes() : driver_error(status);
}

auto Providers::Cuda::Runtime::upload(Core::View::Bytes bytes)
    -> Utility::Result<CUdeviceptr, Core::View::Bytes> {
  auto allocated = allocate(bytes.get_size());
  return allocated.visit(
      [&](CUdeviceptr data) -> Utility::Result<CUdeviceptr, Core::View::Bytes> {
        const auto error = write(data, bytes);
        if (!error.is_empty()) {
          free_image(data);
          return error;
        }

        ++uploads;
        return data;
      },
      [](Core::View::Bytes error)
          -> Utility::Result<CUdeviceptr, Core::View::Bytes> { return error; });
}

auto Providers::Cuda::Runtime::read(
    CUdeviceptr data,
    Core::Access::Bytes target) -> Core::View::Bytes {
  CurrentContext current(context);
  if (current.status != CUDA_SUCCESS) {
    return driver_error(current.status);
  }

  const auto status = cuMemcpyDtoH(target.get_data(), data, target.get_size());
  if (status != CUDA_SUCCESS) {
    return driver_error(status);
  }

  ++downloads;
  return Core::View::Bytes();
}

void Providers::Cuda::Runtime::free_image(CUdeviceptr data) {
  CurrentContext current(context);
  if (current.status != CUDA_SUCCESS) {
    Core::Diagnostics::Log::fatal(driver_error(current.status));
  }

  const auto status = cuMemFree(data);
  if (status != CUDA_SUCCESS) {
    Core::Diagnostics::Log::fatal(driver_error(status));
  }

  --live_images;
}

auto Providers::Cuda::Runtime::finish() -> Core::View::Bytes {
  CurrentContext current(context);
  if (current.status != CUDA_SUCCESS) {
    return driver_error(current.status);
  }

  const auto status = cuCtxSynchronize();
  return status == CUDA_SUCCESS ? Core::View::Bytes() : driver_error(status);
}

Providers::Cuda::Runtime::~Runtime() {
  if (!context) {
    return;
  }

  // Child resources borrow this context. End them while it is still retained,
  // including when preparation stopped partway through creating the program.
  fourier.reset();
  program.reset();

  if (cuDevicePrimaryCtxRelease(device) != CUDA_SUCCESS) {
    Core::Diagnostics::Log::fatal(
        "CUDA could not release its primary context."_view);
  }
}
