// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/option.hpp"
#include "perimortem/core/time.hpp"
#include "perimortem/core/writer/textual.hpp"

#include "contracts/invert.hpp"
#include "images/call.hpp"
#include "images/image.hpp"
#include "images/kernel.hpp"
#include "images/source.hpp"
#include "images/vocabulary.hpp"
#include "operations/standard.hpp"
#include "tests/forms.hpp"
#include "tests/fulfillment.hpp"
#include "tests/library.hpp"
#include "ttx/semantic/simulacra.hpp"

using namespace Godot;
using namespace Perimortem;

static void report(Core::View::Bytes text) {
  Core::Diagnostics::Log::info(text);
}

static void require(Bool value, Core::View::Bytes message) {
  if (!value) {
    Core::Diagnostics::Log::fatal(message);
  }
}

template <typename Value>
static auto accepted(Utility::Result<Value, Core::View::Bytes> result)
    -> Value {
  return result.visit(
      [](Value& value) -> Value { return Core::Data::take(value); },
      [](Core::View::Bytes error) -> Value {
        Core::Diagnostics::Log::fatal(error);
      });
}

static auto definition(const Images::Image& image, Core::View::Bytes name)
    -> const Images::Operation& {
  const auto* operation = image.get_provider().get_vocabulary().find(name);
  require(operation != nullptr, "Missing operation publication"_view);
  return *operation;
}

static auto invert(const Images::Image& image, Memory::Allocator::Arena& errors)
    -> Utility::Result<Images::Image, Core::View::Bytes> {
  return image.apply(
      definition(image, "invert"_view), nullptr, nullptr, errors);
}

static auto convolve(
    const Images::Image& image,
    const Images::Kernel& kernel,
    Memory::Allocator::Arena& errors)
    -> Utility::Result<Images::Image, Core::View::Bytes> {
  return image.apply(
      definition(image, "convolve"_view), &kernel, nullptr, errors);
}

// This local token comparison tests retained publication identity only. It
// does not grant access to the provider's native representation.
static auto same(image_object left, image_object right) -> Bool {
  return left.source == right.source && left.operations == right.operations;
}

static auto difference(Core::View::Bytes a, Core::View::Bytes b) -> U32 {
  require(a.get_size() == b.get_size(), "Output size mismatch"_view);
  U32 maximum = 0;
  for (Count i = 0; i < a.get_size(); ++i) {
    const U32 error = a[i] > b[i] ? a[i] - b[i] : b[i] - a[i];
    if (i % 4 == 3) {
      require(error == 0, "Alpha changed"_view);
    }

    if (error > maximum) {
      maximum = error;
    }
  }

  return maximum;
}

static auto source(U32 w, U32 h) -> Memory::Dynamic::Bytes {
  Memory::Dynamic::Bytes data;
  data.forgetful_resize(Count(w) * h * 4);
  auto bytes = data.get_access();
  for (Count i = 0; i < bytes.get_size(); ++i) {
    bytes.get_data()[i] = U8(i * 37 + i / 19);
  }

  return data;
}

// This oracle uses direct spatial convolution in double precision. Its index
// expression makes the orientation, zero boundary and centered crop independent
// of the two FFT implementations and their transform padding.
static auto spatial(
    Core::View::Bytes input,
    U32 w,
    U32 h,
    Core::View::Vector<R32> kernel,
    U32 kw,
    U32 kh) -> Memory::Dynamic::Bytes {
  Memory::Dynamic::Bytes output;
  output.forgetful_resize(input.get_size());
  auto pixels = output.get_access();
  for (S32 y = 0; y < S32(h); ++y) {
    for (S32 x = 0; x < S32(w); ++x) {
      const Count i = (Count(y) * w + x) * 4;
      for (U32 c = 0; c < 3; ++c) {
        R64 sum = 0;
        for (S32 ky = 0; ky < S32(kh); ++ky) {
          for (S32 kx = 0; kx < S32(kw); ++kx) {
            const S32 sx = x + S32(kw / 2) - kx, sy = y + S32(kh / 2) - ky;
            if (sx >= 0 && sy >= 0 && sx < S32(w) && sy < S32(h)) {
              sum += R64(input[(Count(sy) * w + sx) * 4 + c]) *
                     kernel[Count(ky) * kw + kx];
            }
          }
        }

        pixels.get_data()[i + c] = sum <= 0 ? 0
                                   : sum >= 255
                                       ? 255
                                       : U8(__builtin_floor(sum + 0.5));
      }

      pixels.get_data()[i + 3] = input[i + 3];
    }
  }

  return output;
}

static void small(Images::Image image, Core::View::Bytes original) {
  Memory::Allocator::Arena errors;
  require(
      image.get_width() == 19 && image.get_height() == 11,
      "Image extents lost"_view);
  auto inverted = accepted(invert(image, errors));
  require(
      !same(image.get_abi(), inverted.get_abi()),
      "Operation reused source identity"_view);
  auto bytes = accepted(inverted.read_pixels(errors));
  const auto view = bytes.get_view();
  for (Count i = 0; i < view.get_size(); ++i) {
    require(
        view[i] == (i % 4 == 3 ? original[i] : U8(original[i] ^ 0xff)),
        "Inversion oracle failed"_view);
  }

  auto restored = accepted(invert(inverted, errors));
  require(
      accepted(restored.read_pixels(errors)).get_view() == original,
      "Chained inversion failed"_view);
  require(
      accepted(image.read_pixels(errors)).get_view() == original,
      "Source was mutated"_view);
  const R32 weights[] = {0.03f, 0.07f, 0.12f, -0.02f, 0.33f,
                         0.09f, 0.14f, 0.04f, 0.2f};
  const Images::Kernel kernel(3, 3, {weights, 9});
  const auto expected = spatial(original, 19, 11, {weights, 9}, 3, 3);
  auto filtered = accepted(convolve(image, kernel, errors));
  require(
      difference(accepted(filtered.read_pixels(errors)), expected) <= 1,
      "Convolution disagrees with spatial oracle"_view);
  // A copied handle and a derived image keep their own supply after the caller
  // replaces its source. Neither can depend on the old host resource's address.
  auto retained = image;
  image = accepted(
      Images::Image::create(image.get_provider(), 1, 1, "rgba"_view, errors));
  require(
      difference(accepted(filtered.read_pixels(errors)), expected) <= 1,
      "Derived image lost its supplying storage"_view);
  require(
      accepted(retained.read_pixels(errors)).get_view() == original,
      "Retained source did not survive replacement"_view);
  const Images::Kernel malformed(2, 2, {weights, 4});
  auto rejected = convolve(retained, malformed, errors);
  require(
      rejected.visit(
          [](Images::Image&) { return false; },
          [](Core::View::Bytes error) { return bool(!error.is_empty()); }),
      "Even kernel was accepted"_view);
  const R32 nonfinite[] = {__builtin_nanf("")};
  const Images::Kernel invalid(1, 1, {nonfinite, 1});
  auto failed = convolve(retained, invalid, errors);
  require(
      failed.visit(
          [](Images::Image&) { return false; },
          [](Core::View::Bytes error) { return bool(!error.is_empty()); }),
      "Nonfinite kernel was accepted"_view);
}

static auto disk(U32 size) -> Memory::Dynamic::Bytes {
  Memory::Dynamic::Bytes storage;
  storage.forgetful_resize(Count(size) * size * sizeof(R32));
  auto weights = reinterpret_cast<R32*>(storage.get_access().get_data());
  U32 sum = 0;
  for (S32 y = 0; y < S32(size); ++y) {
    for (S32 x = 0; x < S32(size); ++x) {
      const S32 dx = x - S32(size / 2), dy = y - S32(size / 2);
      weights[Count(y) * size + x] =
          dx * dx + dy * dy <= S32(size / 2) * S32(size / 2) ? 1 : 0;
      sum += U32(weights[Count(y) * size + x]);
    }
  }

  for (Count i = 0; i < Count(size) * size; ++i) {
    weights[i] /= R32(sum);
  }

  return storage;
}

static auto benchmark(
    Images::Image& image,
    const Images::Kernel& kernel,
    Core::View::Bytes label) -> Memory::Dynamic::Bytes {
  Memory::Allocator::Arena errors;
  auto warm = accepted(convolve(image, kernel, errors));
  R64 elapsed = 0;
  Core::Option<Images::Image> output;
  for (U32 i = 0; i < 3; ++i) {
    const auto started = Core::Time::now();
    output = accepted(convolve(image, kernel, errors));
    elapsed += started.measure().convert_to_milliseconds();
  }

  const auto started = Core::Time::now();
  auto bytes = accepted(output->read_pixels(errors));
  Core::Static::Bytes<240> text;
  Core::Writer::Textual writer(text);
  writer << label << ": 1024x512, 63x63 disk, warm mean "_view << elapsed / 3
         << " ms, readback "_view << started.measure().convert_to_milliseconds()
         << " ms"_view;
  report(writer);
  return bytes;
}

// This oracle uses normalized double precision alpha rather than the integer
// numerator used by either implementation. It also covers invisible color when
// both inputs are transparent, which a simple opaque overlay would conceal.
static auto source_over(Core::View::Bytes background, Core::View::Bytes overlay)
    -> Memory::Dynamic::Bytes {
  Memory::Dynamic::Bytes output;
  output.forgetful_resize(background.get_size());
  auto pixels = output.get_access().get_data();
  for (Count i = 0; i < background.get_size(); i += 4) {
    const R64 a = R64(overlay[i + 3]) / 255, b = R64(background[i + 3]) / 255;
    const R64 alpha = a + b * (1 - a);
    for (U32 c = 0; c < 3; ++c) {
      const R64 color =
          alpha == 0
              ? 0
              : (overlay[i + c] * a + background[i + c] * b * (1 - a)) / alpha;
      pixels[i + c] = U8(__builtin_floor(color + 0.5));
    }

    pixels[i + 3] = U8(__builtin_floor(alpha * 255 + 0.5));
  }

  return output;
}

static auto current(
    Images::Expression& expression,
    Memory::Allocator::Arena& errors) -> const Images::Image& {
  auto result = expression.evaluate(errors);
  return result.visit(
      [](const Images::Image& image) -> const Images::Image& { return image; },
      [](Core::View::Bytes error) -> const Images::Image& {
        Core::Diagnostics::Log::fatal(error);
      });
}

static void composition(
    Images::Provider& background_provider,
    Images::Provider& overlay_provider,
    bool gpu) {
  Memory::Allocator::Arena errors;
  const auto& contract = background_provider.get_vocabulary();
  const auto background = source(19, 11);
  auto overlay = source(19, 11);
  for (Count i = 0; i < overlay.get_size(); ++i) {
    overlay.get_access().get_data()[i] ^= 255;
  }

  // Include the alpha endpoints as well as the generated fractional coverage.
  overlay.get_access().get_data()[3] = 0;
  overlay.get_access().get_data()[7] = 255;
  auto& base = Images::Source::create(accepted(
      Images::Image::create(background_provider, 19, 11, background, errors)));
  auto& front = Images::Source::create(accepted(
      Images::Image::create(overlay_provider, 19, 11, overlay, errors)));
  const R32 weights[] = {0.03f, 0.07f, 0.12f, -0.02f, 0.33f,
                         0.09f, 0.14f, 0.04f, 0.2f};
  Memory::Allocator::Arena constants;
  const auto stored =
      constants.proxy({reinterpret_cast<const U8*>(weights), sizeof(weights)});
  const auto& kernel = constants.construct<Images::Kernel>(
      3, 3,
      Core::View::Vector<R32>(
          reinterpret_cast<const R32*>(stored.get_data()), 9));
  const Images::Call::Argument filter_arguments[] = {{nullptr, &kernel}};
  const auto& filter = *contract.find("convolve"_view);
  const auto& blend = *contract.find("composite"_view);
  auto& blur = Images::Call::create(
      base, filter, filter_arguments, Core::Data::take(constants));
  const Images::Call::Argument blend_arguments[] = {{&front, nullptr}};
  auto& combined = Images::Call::create(blur, blend, blend_arguments, {});
  Images::Image history = current(combined, errors);
  const auto original = accepted(history.read_pixels(errors));
  const auto blurred = accepted(current(blur, errors).read_pixels(errors));
  const auto blur_identity = current(blur, errors).get_abi();
  require(
      difference(original, source_over(blurred, overlay)) <= 1,
      "Source over disagrees with independent alpha oracle"_view);
  require(
      combined.get_interface()
              .resolve_concept("operation"_view)
              .get_identity() == blend.get_interface().get_identity(),
      "Call lost its operation identity"_view);
  // Navigation lends the current image's policy. Fulfillment must reach its
  // provider through that bound edge, without selecting a native CPU/CUDA type
  // or causing a pixel transfer just to obtain the callable interface.
  const auto before_binding = background_provider.statistics();
  const auto visible = combined.get_interface().resolve_concept("value"_view);
  Ttx::Semantic::Simulacra::fulfill<Contracts::Invert>(visible.get_query())
      .visit(
          [&](const Contracts::Invert::Handle& handle) {
            handle.apply().visit(
                [](image_object native) {
                  native.operations->release(native.source);
                },
                [&](Core::View::Bytes) {
                  require(
                      False, "Navigated image could not invoke invert"_view);
                });
          },
          [&](Ttx::Semantic::Binding::Failure) {
            require(False, "Navigated image lost its provider policy"_view);
          });
  require(
      background_provider.statistics().downloads == before_binding.downloads &&
          background_provider.statistics().uploads == before_binding.uploads,
      "Callable fulfillment or inversion materialized source pixels"_view);
  const auto idle = background_provider.statistics();
  require(
      same(current(combined, errors).get_abi(), history.get_abi()) &&
          combined.get_evaluations() == 1 && blur.get_evaluations() == 1,
      "Unchanged graph recomputed"_view);
  require(
      background_provider.statistics().uploads == idle.uploads &&
          background_provider.statistics().downloads == idle.downloads,
      "Cached graph transferred image data"_view);
  for (Count i = 0; i < overlay.get_size(); ++i) {
    if (i % 4 != 3) {
      overlay.get_access().get_data()[i] ^= 255;
    }
  }

  front.publish(accepted(
      Images::Image::create(overlay_provider, 19, 11, overlay, errors)));
  const auto before = background_provider.statistics();
  const auto& changed = current(combined, errors);
  const auto after = background_provider.statistics();
  require(
      combined.get_evaluations() == 2 && blur.get_evaluations() == 1 &&
          same(current(blur, errors).get_abi(), blur_identity),
      "Overlay invalidated the unrelated blur"_view);
  if (gpu) {
    require(
        after.uploads == before.uploads + 1 &&
            after.downloads == before.downloads &&
            after.plan_builds == before.plan_builds,
        "Overlay update transferred or rebuilt the CUDA background"_view);
  }

  require(
      difference(
          accepted(changed.read_pixels(errors)),
          source_over(blurred, overlay)) <= 1,
      "Changed overlay produced stale composition"_view);
  front.publish(accepted(
      Images::Image::create(overlay_provider, 1, 1, "rgba"_view, errors)));
  auto failure = combined.evaluate(errors);
  require(
      failure.visit(
          [](const Images::Image&) { return false; },
          [](Core::View::Bytes error) { return bool(!error.is_empty()); }),
      "Mismatched composite dimensions were accepted"_view);
  combined.evaluate(errors);
  require(
      combined.get_evaluations() == 3 && blur.get_evaluations() == 1,
      "Failed evaluation was not cached independently"_view);
  front.publish(accepted(
      Images::Image::create(overlay_provider, 19, 11, overlay, errors)));
  require(
      difference(
          accepted(current(combined, errors).read_pixels(errors)),
          source_over(blurred, overlay)) <= 1,
      "Graph did not recover after corrected input"_view);
  base.publish(accepted(
      Images::Image::create(background_provider, 19, 11, overlay, errors)));
  current(combined, errors);
  require(
      blur.get_evaluations() == 2 && combined.get_evaluations() == 5,
      "Source change did not reach its full dependent branch"_view);
  combined.release();
  blur.release();
  front.release();
  base.release();
  require(
      accepted(history.read_pixels(errors)).get_view() == original.get_view(),
      "Retained history changed after graph publication or destruction"_view);
  report(
      gpu ? "PASS: mixed CPU/CUDA composition, one overlay upload, resident blur, cached failure and selective invalidation"_view
          : "PASS: CPU composition, dependency propagation and retained history"_view);
}

static void shared_ancestors(Images::Provider& provider) {
  Memory::Allocator::Arena errors;
  Images::Expression* graph = &Images::Source::create(
      accepted(Images::Image::create(provider, 1, 1, "1234"_view, errors)));
  const auto& blend = *provider.get_vocabulary().find("composite"_view);
  // There are 33 nodes but exponentially many paths to the shared source.
  // A pull visits each node once instead of walking every possible path.
  for (U32 i = 0; i < 32; ++i) {
    const Images::Call::Argument arguments[] = {{graph, nullptr}};
    auto& next = Images::Call::create(*graph, blend, arguments);
    graph->release();
    graph = &next;
  }

  for (U32 i = 0; i < 2; ++i) {
    const auto bytes = accepted(current(*graph, errors).read_pixels(errors));
    const auto pixels = bytes.get_view();
    require(
        pixels.get_size() == 4 && pixels[0] == '1' && pixels[1] == '2' &&
            pixels[2] == '3' && pixels[3] == 255,
        "Shared ancestor evaluation changed source over"_view);
  }

  graph->release();
}

static int check(int argc, char** argv) {
  require(
      argc == 3 || argc == 4,
      "Supply CPU, probe, and optionally CUDA module paths."_view);
  const auto& contract = Operations::Standard::get_vocabulary();
  Memory::Allocator::Arena errors;
  auto& cpu_provider =
      Images::Provider::open(
          Core::NullTerminated::to_view(argv[1]), contract, errors)
          .visit(
              [](Images::Provider& value) -> Images::Provider& {
                return value;
              },
              [](Core::View::Bytes error) -> Images::Provider& {
                Core::Diagnostics::Log::fatal(error);
              });
  const auto pixels = source(19, 11);
  small(
      accepted(Images::Image::create(cpu_provider, 19, 11, pixels, errors)),
      pixels);
  auto malformed =
      Images::Image::create(cpu_provider, 1, 1, "abc"_view, errors);
  require(
      malformed.visit(
          [](Images::Image&) { return false; },
          [](Core::View::Bytes error) { return bool(!error.is_empty()); }),
      "Malformed source accepted"_view);
  const auto large = source(1024, 512);
  const auto coefficients = disk(63);
  const Images::Kernel kernel(
      63, 63,
      {reinterpret_cast<const R32*>(coefficients.get_view().get_data()),
       63 * 63});
  auto cpu =
      accepted(Images::Image::create(cpu_provider, 1024, 512, large, errors));
  Tests::Forms::check(cpu_provider, False);
  composition(cpu_provider, cpu_provider, false);
  shared_ancestors(cpu_provider);
  const auto expected = benchmark(cpu, kernel, "CPU FFTW"_view);
  Tests::Fulfillment::check(Core::NullTerminated::to_view(argv[2]), contract);
  Tests::Library::check(Core::NullTerminated::to_view(argv[2]), contract);
  if (argc == 4) {
    auto& provider =
        Images::Provider::open(
            Core::NullTerminated::to_view(argv[3]), contract, errors)
            .visit(
                [](Images::Provider& value) -> Images::Provider& {
                  return value;
                },
                [](Core::View::Bytes error) -> Images::Provider& {
                  Core::Diagnostics::Log::fatal(error);
                });
    {
      Tests::Forms::check(provider, True);
      composition(provider, cpu_provider, true);
      small(
          accepted(Images::Image::create(provider, 19, 11, pixels, errors)),
          pixels);
      require(
          provider.statistics().live_images == 0,
          "CUDA images leaked after final handle release"_view);
      auto cuda =
          accepted(Images::Image::create(provider, 1024, 512, large, errors));
      const auto before = provider.statistics();
      auto inverted = accepted(invert(cuda, errors));
      auto restored = accepted(invert(inverted, errors));
      auto filtered = accepted(convolve(restored, kernel, errors));
      const auto after = provider.statistics();
      require(
          before.uploads == after.uploads &&
              before.downloads == after.downloads,
          "CUDA chain performed an image transfer"_view);
      require(
          difference(accepted(filtered.read_pixels(errors)), expected) <= 1,
          "Persistent chain changed convolution result"_view);
      const auto plans = after.plan_builds;
      require(
          difference(benchmark(cuda, kernel, "CUDA cuFFT"_view), expected) <= 1,
          "Large CPU and CUDA convolution differ"_view);
      require(
          provider.statistics().plan_builds == plans,
          "Repeated shape rebuilt cuFFT plans"_view);
    }

    require(
        provider.statistics().live_images == 0,
        "CUDA image resources remain live"_view);
    auto survivor =
        accepted(Images::Image::create(provider, 19, 11, pixels, errors));
    provider.release();
    require(
        accepted(survivor.read_pixels(errors)).get_view() == pixels.get_view(),
        "Image did not retain its supplying module"_view);
  }

  cpu_provider.release();
  report(
      "PASS: persistent ownership, immutable chains, exact inversion, spatial convolution oracle, malformed input and requested FFT execution"_view);
  return 0;
}

int main(int argc, char** argv) {
  Core::Diagnostics::Log::set_sink(Core::Diagnostics::Log::stderr_sink);
  const auto result = check(argc, argv);
  return result;
}
