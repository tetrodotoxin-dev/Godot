// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "providers/cuda/program.hpp"

#include <nvrtc.h>

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/writer/textual.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "providers/cuda/current_context.hpp"

using namespace Godot;
using namespace Perimortem;

#include "providers/cuda/kernels.inc"

static auto driver_error(CUresult status) -> Core::View::Bytes {
  const char* message = nullptr;
  cuGetErrorString(status, &message);
  return message ? Core::NullTerminated::to_view(message)
                 : "CUDA driver failure."_view;
}

Providers::Cuda::Program::~Program() {
  reset();
}

void Providers::Cuda::Program::reset() {
  if (module) {
    CurrentContext current(context);
    if (current.status != CUDA_SUCCESS) {
      Core::Diagnostics::Log::fatal(driver_error(current.status));
    }

    if (cuModuleUnload(module) != CUDA_SUCCESS) {
      Core::Diagnostics::Log::fatal(
          "CUDA could not unload its image program."_view);
    }
  }

  module = nullptr;
  context = nullptr;
}

auto Providers::Cuda::Program::prepare(CUdevice device, CUcontext supplied)
    -> Core::View::Bytes {
  reset();
  context = supplied;
  CurrentContext current(context);
  if (current.status != CUDA_SUCCESS) {
    return driver_error(current.status);
  }

  // The installed provider carries its source, but compiles for the actual
  // device when the factory opens. No shader source file or path into the build
  // tree is required by the deployed addon.
  CUresult status;
  int major = 0, minor = 0;
  status = cuDeviceGetAttribute(
      &major, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR, device);
  if (status != CUDA_SUCCESS) {
    return driver_error(status);
  }

  status = cuDeviceGetAttribute(
      &minor, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR, device);
  if (status != CUDA_SUCCESS) {
    return driver_error(status);
  }

  Core::Static::Bytes<64> architecture;
  Core::Writer::Textual writer(architecture);
  writer << "--gpu-architecture=compute_"_view << S32(major) << S32(minor)
         << '\0';
  const char* options[] = {
    reinterpret_cast<const char*>(architecture.get_data())};

  nvrtcProgram program = nullptr;
  auto compiled =
      nvrtcCreateProgram(&program, source, "images.cu", 0, nullptr, nullptr);
  if (compiled != NVRTC_SUCCESS) {
    return Core::NullTerminated::to_view(nvrtcGetErrorString(compiled));
  }

  compiled = nvrtcCompileProgram(program, 1, options);
  size_t size = 0;
  if (compiled == NVRTC_SUCCESS) {
    compiled = nvrtcGetPTXSize(program, &size);
  }

  Memory::Dynamic::Bytes ptx;
  if (compiled == NVRTC_SUCCESS) {
    ptx.forgetful_resize(size);
    compiled = nvrtcGetPTX(
        program, reinterpret_cast<char*>(ptx.get_access().get_data()));
  }

  nvrtcDestroyProgram(&program);
  if (compiled != NVRTC_SUCCESS) {
    return Core::NullTerminated::to_view(nvrtcGetErrorString(compiled));
  }

  status = cuModuleLoadData(&module, ptx.get_view().get_data());
  if (status != CUDA_SUCCESS) {
    return driver_error(status);
  }

  return {};
}

auto Providers::Cuda::Program::launch(
    const char* entry,
    U32 count,
    void** arguments) -> Core::View::Bytes {
  CurrentContext current(context);
  if (current.status != CUDA_SUCCESS) {
    return driver_error(current.status);
  }

  CUfunction function = nullptr;
  auto status = cuModuleGetFunction(&function, module, entry);
  if (status == CUDA_SUCCESS) {
    status = cuLaunchKernel(
        function, (count + 255) / 256, 1, 1, 256, 1, 1, 0, nullptr, arguments,
        nullptr);
  }

  return status == CUDA_SUCCESS ? Core::View::Bytes() : driver_error(status);
}
