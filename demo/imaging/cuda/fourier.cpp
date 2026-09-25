// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/imaging/cuda/fourier.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "cuda/runtime/current_context.hpp"

using namespace Godot::Demo;
using namespace Perimortem;

Imaging::Cuda::Fourier::~Fourier() {
  reset();
}

void Imaging::Cuda::Fourier::reset() {
  if (!context) {
    return;
  }

  ::Cuda::Runtime::CurrentContext current(context);
  if (current.status != CUDA_SUCCESS) {
    Core::Diagnostics::Log::fatal(
        "cuFFT context ended before its resources."_view);
  }

  if (images && cufftDestroy(images) != CUFFT_SUCCESS) {
    Core::Diagnostics::Log::fatal(
        "CUDA could not release its image FFT plan."_view);
  }

  if (kernel && cufftDestroy(kernel) != CUFFT_SUCCESS) {
    Core::Diagnostics::Log::fatal(
        "CUDA could not release its kernel FFT plan."_view);
  }

  if (scratch && cuMemFree(scratch) != CUDA_SUCCESS) {
    Core::Diagnostics::Log::fatal(
        "CUDA could not release convolution scratch."_view);
  }

  images = 0;
  kernel = 0;
  scratch = 0;
  width = 0;
  height = 0;
  context = nullptr;
}

auto Imaging::Cuda::Fourier::prepare(CUcontext supplied, U32 w, U32 h)
    -> Core::View::Bytes {
  if (context == supplied && width == w && height == h) {
    return {};
  }

  reset();
  context = supplied;
  ::Cuda::Runtime::CurrentContext current(context);
  if (current.status != CUDA_SUCCESS) {
    return "cuFFT context is unavailable."_view;
  }

  const Count count = Count(w) * h;
  const auto allocated =
      cuMemAlloc(&scratch, count * (4 * sizeof(cufftComplex) + sizeof(R32)));
  if (allocated != CUDA_SUCCESS) {
    return "cuFFT scratch allocation failed."_view;
  }

  int dimensions[] = {int(h), int(w)};
  auto status = cufftPlanMany(
      &images, 2, dimensions, nullptr, 1, count, nullptr, 1, count, CUFFT_C2C,
      3);
  if (status == CUFFT_SUCCESS) {
    status = cufftPlan2d(&kernel, h, w, CUFFT_C2C);
  }

  if (status != CUFFT_SUCCESS) {
    return "cuFFT could not create convolution plans."_view;
  }

  // A matching shape is reusable only after both plans and their workspace
  // exist. Failed preparation leaves the dimensions unpublished so a retry
  // resets and releases any partially created resources first.
  width = w;
  height = h;
  ++builds;
  return {};
}

auto Imaging::Cuda::Fourier::forward() -> Core::View::Bytes {
  ::Cuda::Runtime::CurrentContext current(context);
  if (current.status != CUDA_SUCCESS) {
    return "cuFFT context is unavailable."_view;
  }

  auto planes = reinterpret_cast<cufftComplex*>(scratch);
  auto weights = planes + 3 * Count(width) * height;
  auto status = cufftExecC2C(images, planes, planes, CUFFT_FORWARD);
  if (status == CUFFT_SUCCESS) {
    status = cufftExecC2C(kernel, weights, weights, CUFFT_FORWARD);
  }

  return status == CUFFT_SUCCESS ? Core::View::Bytes()
                                 : "cuFFT forward transform failed."_view;
}

auto Imaging::Cuda::Fourier::inverse() -> Core::View::Bytes {
  ::Cuda::Runtime::CurrentContext current(context);
  if (current.status != CUDA_SUCCESS) {
    return "cuFFT context is unavailable."_view;
  }

  auto planes = reinterpret_cast<cufftComplex*>(scratch);
  return cufftExecC2C(images, planes, planes, CUFFT_INVERSE) == CUFFT_SUCCESS
             ? Core::View::Bytes()
             : "cuFFT inverse transform failed."_view;
}
