// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/imaging/cpu/convolve.hpp"

#include <pocketfft_hdronly.h>

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "demo/imaging/contracts/kernel.hpp"

using namespace Godot::Demo;
using namespace Perimortem;

// Three image planes and one kernel plane share the same padded dimensions.
// Zeroing the surrounding region makes the transform describe linear
// convolution rather than wrapping opposite edges of the source together.
static void pad(
    const Imaging::Cpu::Image& image,
    U32 kw,
    U32 kh,
    Core::View::Vector<R32> weights,
    U32 pw,
    U32 ph,
    R32* planes) {
  const auto input = image.get_pixels();
  const Count count = Count(pw) * ph;
  auto kernel = planes + 3 * count;
  __builtin_memset(planes, 0, count * 4 * sizeof(R32));
  for (U32 y = 0; y < image.get_height(); ++y) {
    for (U32 x = 0; x < image.get_width(); ++x) {
      for (U32 c = 0; c < 3; ++c) {
        planes[c * count + Count(y) * pw + x] =
            input[(Count(y) * image.get_width() + x) * 4 + c];
      }
    }
  }

  for (U32 y = 0; y < kh; ++y) {
    for (U32 x = 0; x < kw; ++x) {
      kernel[Count(y) * pw + x] = weights[Count(y) * kw + x];
    }
  }
}

// Normalization belongs to the pointwise product. Each frequency sample is
// already being read and written here, so the inverse needs no scaling pass.
static void multiply(std::complex<R32>* planes, Count count, R32 scale) {
  const auto kernel = planes + 3 * count;
  for (Count i = 0; i < 3 * count; ++i) {
    const R32 real = planes[i].real(), imaginary = planes[i].imag();
    const auto& k = kernel[i % count];
    planes[i] = std::complex<R32>(
        (real * k.real() - imaginary * k.imag()) * scale,
        (real * k.imag() + imaginary * k.real()) * scale);
  }
}

static auto crop(
    const Imaging::Cpu::Image& image,
    U32 kw,
    U32 kh,
    U32 pw,
    U32 ph,
    const R32* planes) -> Memory::Dynamic::Bytes {
  const auto input = image.get_pixels();
  const Count count = Count(pw) * ph;
  Memory::Dynamic::Bytes output;
  output.forgetful_resize(input.get_size());
  auto pixels = output.get_access();
  // Full zero padded convolution is cropped around the authored kernel center.
  // Alpha is carried from the source, so filtering RGB cannot erase coverage.
  for (U32 y = 0; y < image.get_height(); ++y) {
    for (U32 x = 0; x < image.get_width(); ++x) {
      const Count target = (Count(y) * image.get_width() + x) * 4;
      for (U32 c = 0; c < 3; ++c) {
        const R32 value =
            planes[c * count + Count(y + kh / 2) * pw + x + kw / 2];
        pixels.get_data()[target + c] = value <= 0     ? 0
                                        : value >= 255 ? 255
                                                       : U8(value + R32(0.5));
      }

      pixels.get_data()[target + 3] = input[target + 3];
    }
  }

  return Core::Data::take(output);
}

static auto transform(
    const Imaging::Cpu::Image& image,
    U32 kw,
    U32 kh,
    Core::View::Vector<R32> weights)
    -> Utility::Result<Memory::Dynamic::Bytes, Core::View::Bytes> {
  U32 pw = 1;
  U32 ph = 1;
  while (pw < image.get_width() + kw - 1) {
    pw *= 2;
  }

  while (ph < image.get_height() + kh - 1) {
    ph *= 2;
  }

  const Count count = Count(pw) * ph;
  if (count > 16777216) {
    return "Padded convolution exceeds 16 million samples."_view;
  }

  // Pixels and kernel weights are real values. Their spectra are Hermitian,
  // so each row only needs width / 2 + 1 complex values. The same real buffer
  // receives the inverse output before crop publishes independently owned RGBA.
  const Count columns = pw / 2 + 1;
  const Count frequencies = columns * ph;
  Memory::Dynamic::Vector<R32> real;
  Memory::Dynamic::Vector<std::complex<R32>> spectrum;
  real.resize(4 * count);
  spectrum.resize(4 * frequencies);
  pad(image, kw, kh, weights, pw, ph, real.get_data());

  pocketfft::shape_t shape({4, ph, pw});
  const pocketfft::stride_t real_stride(
      {std::ptrdiff_t(count * sizeof(R32)), std::ptrdiff_t(pw * sizeof(R32)),
       sizeof(R32)});
  const pocketfft::stride_t spectrum_stride(
      {std::ptrdiff_t(frequencies * sizeof(std::complex<R32>)),
       std::ptrdiff_t(columns * sizeof(std::complex<R32>)),
       sizeof(std::complex<R32>)});
  pocketfft::r2c(
      shape, real_stride, spectrum_stride, pocketfft::shape_t({1, 2}),
      pocketfft::FORWARD, real.get_data(), spectrum.get_data(), R32(1));
  multiply(spectrum.get_data(), frequencies, R32(1) / count);

  // The general inverse preserves its input by copying the spectrum. This
  // operation owns the scratch, so we can invert the vertical axis in place
  // before reconstructing the real rows, without another image sized buffer.
  shape[0] = 3;
  shape[2] = columns;
  pocketfft::c2c(
      shape, spectrum_stride, spectrum_stride, pocketfft::shape_t({1}),
      pocketfft::BACKWARD, spectrum.get_data(), spectrum.get_data(), R32(1));
  shape[2] = pw;
  pocketfft::c2r(
      shape, spectrum_stride, real_stride, CppSize(2), pocketfft::BACKWARD,
      spectrum.get_data(), real.get_data(), R32(1));
  return crop(image, kw, kh, pw, ph, real.get_data());
}

auto Imaging::Cpu::Convolve::apply(const Image& image, image_kernel kernel)
    -> Utility::Result<Image&, Core::View::Bytes> {
  const Core::View::Vector<R32> weights(kernel.values, kernel.count);
  const auto invalid = Imaging::Contracts::Kernel::validate(
      kernel.width, kernel.height, weights);
  if (!invalid.is_empty()) {
    return invalid;
  }

  // PocketFFT reports allocation and argument failures with C++ exceptions.
  // They end inside this provider. Callers still receive the image contract's
  // borrowed diagnostic rather than an exception crossing a foreign call.
  try {
    return transform(image, kernel.width, kernel.height, weights)
        .visit(
            [&](Memory::Dynamic::Bytes& pixels)
                -> Utility::Result<Image&, Core::View::Bytes> {
              return image.derive(Core::Data::take(pixels));
            },
            [](Core::View::Bytes error)
                -> Utility::Result<Image&, Core::View::Bytes> {
              return error;
            });
  } catch (const std::bad_alloc&) {
    return "CPU convolution could not allocate transform storage."_view;
  } catch (const std::exception&) {
    return "CPU convolution could not construct its transform."_view;
  }
}
