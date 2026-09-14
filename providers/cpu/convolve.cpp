// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "providers/cpu/convolve.hpp"

#include <fftw3.h>

#include "perimortem/core/null_terminated.hpp"

#include "contracts/kernel.hpp"

using namespace Godot;
using namespace Perimortem;

// FFTW owns each executable plan, while Perimortem owns the storage it
// addresses. This local owner releases successfully created plans on every exit
// path and ends before the surrounding transform releases its borrowed scratch
// buffers.
class Plans {
 public:
  Plans() = default;
  Plans(const Plans&) = delete;
  auto operator=(const Plans&) -> Plans& = delete;
  ~Plans() {
    if (image) {
      fftwf_destroy_plan(image);
    }

    if (kernel) {
      fftwf_destroy_plan(kernel);
    }

    if (inverse) {
      fftwf_destroy_plan(inverse);
    }
  }

  fftwf_plan image = nullptr;
  fftwf_plan kernel = nullptr;
  fftwf_plan inverse = nullptr;
};

// Planning can overwrite the buffers while FFTW measures candidate codelets.
// Complete it before populating source pixels and coefficients. Plans owns any
// handles already created if a later plan fails.
static auto prepare(Plans& plans, U32 pw, U32 ph, fftwf_complex* planes)
    -> Core::View::Bytes {
  const Count count = Count(pw) * ph;
  auto kernel = planes + 3 * count;
  int dimensions[2] = {int(ph), int(pw)};
  plans.image = fftwf_plan_many_dft(
      2, dimensions, 3, planes, nullptr, 1, count, planes, nullptr, 1, count,
      FFTW_FORWARD, FFTW_MEASURE);
  plans.kernel =
      fftwf_plan_dft_2d(ph, pw, kernel, kernel, FFTW_FORWARD, FFTW_MEASURE);
  plans.inverse = fftwf_plan_many_dft(
      2, dimensions, 3, planes, nullptr, 1, count, planes, nullptr, 1, count,
      FFTW_BACKWARD, FFTW_MEASURE);
  if (!plans.image || !plans.kernel || !plans.inverse) {
    return "FFTW could not create convolution plans"_view;
  }

  return Core::View::Bytes();
}

// Three image planes and one kernel plane share the same padded dimensions.
// Zeroing the surrounding region makes the transform describe linear
// convolution rather than wrapping opposite edges of the source together.
static void pad(
    const Providers::Cpu::Image& image,
    U32 kw,
    U32 kh,
    Core::View::Vector<R32> weights,
    U32 pw,
    U32 ph,
    fftwf_complex* planes) {
  const auto input = image.get_pixels();
  const Count count = Count(pw) * ph;
  auto kernel = planes + 3 * count;
  __builtin_memset(planes, 0, count * 4 * sizeof(fftwf_complex));
  for (U32 y = 0; y < image.get_height(); ++y) {
    for (U32 x = 0; x < image.get_width(); ++x) {
      for (U32 c = 0; c < 3; ++c) {
        planes[c * count + Count(y) * pw + x][0] =
            input[(Count(y) * image.get_width() + x) * 4 + c];
      }
    }
  }

  for (U32 y = 0; y < kh; ++y) {
    for (U32 x = 0; x < kw; ++x) {
      kernel[Count(y) * pw + x][0] = weights[Count(y) * kw + x];
    }
  }
}

// FFTW's inverse is unnormalized. Incorporating that scale in the spectral
// product avoids another pass through the reconstructed image.
static void multiply(fftwf_complex* planes, Count count) {
  const auto kernel = planes + 3 * count;
  const R32 scale = R32(1) / count;
  for (Count i = 0; i < 3 * count; ++i) {
    const R32 real = planes[i][0], imaginary = planes[i][1];
    const auto& k = kernel[i % count];
    planes[i][0] = (real * k[0] - imaginary * k[1]) * scale;
    planes[i][1] = (real * k[1] + imaginary * k[0]) * scale;
  }
}

static auto crop(
    const Providers::Cpu::Image& image,
    U32 kw,
    U32 kh,
    U32 pw,
    U32 ph,
    const fftwf_complex* planes) -> Memory::Dynamic::Bytes {
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
            planes[c * count + Count(y + kh / 2) * pw + x + kw / 2][0];
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
    const Providers::Cpu::Image& image,
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

  // The aligned view is borrowed from one Perimortem allocation. Keeping the
  // original Bytes owner on this stack lets FFTW use vector codelets without
  // making its allocator part of the image contract.
  Memory::Dynamic::Bytes storage;
  storage.forgetful_resize(count * 4 * sizeof(fftwf_complex) + 63);
  auto planes = reinterpret_cast<fftwf_complex*>(Core::Data::align<64>(
      reinterpret_cast<Count>(storage.get_access().get_data())));
  Plans plans;
  const auto error = prepare(plans, pw, ph, planes);
  if (!error.is_empty()) {
    return error;
  }

  pad(image, kw, kh, weights, pw, ph, planes);
  fftwf_execute(plans.image);
  fftwf_execute(plans.kernel);
  multiply(planes, count);
  fftwf_execute(plans.inverse);
  return crop(image, kw, kh, pw, ph, planes);
}

auto Providers::Cpu::Convolve::apply(const Image& image, image_kernel kernel)
    -> Utility::Result<Image&, Core::View::Bytes> {
  const Core::View::Vector<R32> weights(kernel.values, kernel.count);
  const auto invalid =
      Contracts::Kernel::validate(kernel.width, kernel.height, weights);
  if (!invalid.is_empty()) {
    return invalid;
  }

  auto output = transform(image, kernel.width, kernel.height, weights);
  return output.visit(
      [&](Memory::Dynamic::Bytes& pixels)
          -> Utility::Result<Image&, Core::View::Bytes> {
        return image.derive(Core::Data::take(pixels));
      },
      [](Core::View::Bytes error)
          -> Utility::Result<Image&, Core::View::Bytes> { return error; });
}
