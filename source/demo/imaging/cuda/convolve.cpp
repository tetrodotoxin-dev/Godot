// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/imaging/cuda/convolve.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "demo/imaging/contracts/kernel.hpp"

using namespace Godot::Demo;
using namespace Perimortem;

static auto transform(
    const Imaging::Cuda::Image& image,
    Imaging::Contracts::Kernel::Coefficients kernel)
    -> Utility::Result<Imaging::Cuda::Allocation, Core::View::Bytes> {
  auto& runtime = image.get_runtime();
  auto& fft = runtime.get_fourier();
  U32 w = image.get_width(), h = image.get_height(), kw = kernel.width,
      kh = kernel.height;
  U32 pw = 1, ph = 1;
  while (pw < w + kw - 1) {
    pw *= 2;
  }

  while (ph < h + kh - 1) {
    ph *= 2;
  }

  U32 count = pw * ph;
  if (count > 16777216) {
    return "Padded convolution exceeds 16 million samples."_view;
  }

  auto error = fft.prepare(runtime.get_context(), pw, ph);
  if (!error.is_empty()) {
    return error;
  }

  auto allocated = runtime.allocate(Count(w) * h * 4);
  return allocated.visit(
      [&](CUdeviceptr output)
          -> Utility::Result<Imaging::Cuda::Allocation, Core::View::Bytes> {
        Imaging::Cuda::Allocation pending(runtime, output);
        auto input = image.get_buffer();
        auto scratch = fft.get_buffer();
        auto coefficients = scratch + Count(count) * 4 * sizeof(cufftComplex);
        error = runtime.write(
            coefficients,
            {reinterpret_cast<const U8*>(kernel.values.get_data()),
             kernel.values.get_size() * sizeof(R32)});
        if (!error.is_empty()) {
          return error;
        }

        // The padded workspace holds RGB plus the kernel transform. Keeping
        // all four on the device lets later stages exchange only coordinates.
        void* padding[] = {&input, &scratch, &coefficients, &w, &h,
                           &kw,    &kh,      &pw,           &ph};
        error = runtime.get_program().launch("pad", count, padding);
        if (error.is_empty()) {
          error = fft.forward();
        }

        if (!error.is_empty()) {
          return error;
        }

        // Multiply each channel by the same kernel spectrum. Normalization
        // is part of the kernel, before cuFFT's unnormalized inverse.
        void* multiply[] = {&scratch, &count};
        error = runtime.get_program().launch("multiply", 3 * count, multiply);
        if (error.is_empty()) {
          error = fft.inverse();
        }

        if (!error.is_empty()) {
          return error;
        }

        // Crop the centered convolution to the original extent and preserve
        // source alpha. Publish only after queued device work has completed.
        void* crop[] = {&input, &output, &scratch, &w, &h, &kw, &kh, &pw, &ph};
        error = runtime.get_program().launch("crop", w * h, crop);
        if (error.is_empty()) {
          error = runtime.finish();
        }

        if (!error.is_empty()) {
          return error;
        }

        return Core::Data::take(pending);
      },
      [](Core::View::Bytes error)
          -> Utility::Result<Imaging::Cuda::Allocation, Core::View::Bytes> {
        return error;
      });
}

auto Imaging::Cuda::Convolve::apply(const Image& image, image_kernel kernel)
    -> Utility::Result<Image&, Core::View::Bytes> {
  const Core::View::Vector<R32> weights(kernel.values, kernel.count);
  const auto invalid = Imaging::Contracts::Kernel::validate(
      kernel.width, kernel.height, weights);
  if (!invalid.is_empty()) {
    return invalid;
  }

  auto output = transform(
      image, Imaging::Contracts::Kernel::Coefficients{
               kernel.width, kernel.height, weights});
  return output.visit(
      [&](Allocation& pixels) -> Utility::Result<Image&, Core::View::Bytes> {
        return image.derive(Core::Data::take(pixels));
      },
      [](Core::View::Bytes error)
          -> Utility::Result<Image&, Core::View::Bytes> { return error; });
}
