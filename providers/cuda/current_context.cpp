// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "providers/cuda/current_context.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Godot;
using namespace Perimortem;

Providers::Cuda::CurrentContext::CurrentContext(CUcontext context)
    : status(cuCtxPushCurrent(context)) {}
Providers::Cuda::CurrentContext::~CurrentContext() {
  if (status != CUDA_SUCCESS) {
    return;
  }

  CUcontext previous;
  if (cuCtxPopCurrent(&previous) != CUDA_SUCCESS) {
    Core::Diagnostics::Log::fatal(
        "CUDA could not restore the caller's current context."_view);
  }
}
