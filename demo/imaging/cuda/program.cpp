// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/imaging/cuda/program.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "cuda/runtime/current_context.hpp"

using namespace Godot::Demo;
using namespace Perimortem;

#include "demo/imaging/cuda/kernels.inc"

static auto driver_error(CUresult status) -> Core::View::Bytes {
  const char* message = nullptr;
  cuGetErrorString(status, &message);
  return message ? Core::NullTerminated::to_view(message)
                 : "CUDA driver failure."_view;
}

Imaging::Cuda::Program::~Program() {
  reset();
}

void Imaging::Cuda::Program::reset() {
  if (program) {
    program->release();
    program = nullptr;
  }
}

auto Imaging::Cuda::Program::prepare(CUdevice device) -> Core::View::Bytes {
  reset();
  error.clear();
  const Core::View::Bytes code(
      reinterpret_cast<const U8*>(source), sizeof(source) - 1);
  constexpr auto name = "images.cu"_view;
  const cuda_compile_request request{
    {{name.get_data(), name.get_size()}, {code.get_data(), code.get_size()}},
    nullptr,
    0,
    nullptr,
    0,
    device};
  const cuda_diagnostics diagnostics{
    &error, [](void* owner, perimortem_view_bytes message) {
      *static_cast<Memory::Dynamic::Bytes*>(owner) =
          Memory::Dynamic::Bytes({message.data, message.size});
    }};
  ::Cuda::Runtime::Program::create(request, diagnostics)
      .visit(
          [&](::Cuda::Runtime::Program& prepared) { program = &prepared; },
          [&](Ttx::Data::Status) {
            if (error.is_empty()) {
              error = "CUDA image program could not compile."_view;
            }
          });
  return error.get_view();
}

auto Imaging::Cuda::Program::launch(
    const char* entry,
    U32 count,
    void** arguments) -> Core::View::Bytes {
  ::Cuda::Runtime::CurrentContext current(program->get_context());
  if (current.status != CUDA_SUCCESS) {
    return driver_error(current.status);
  }

  CUfunction function = nullptr;
  auto status = cuModuleGetFunction(&function, program->get_module(), entry);
  if (status == CUDA_SUCCESS) {
    status = cuLaunchKernel(
        function, (count + 255) / 256, 1, 1, 256, 1, 1, 0, nullptr, arguments,
        nullptr);
  }

  return status == CUDA_SUCCESS ? Core::View::Bytes() : driver_error(status);
}
