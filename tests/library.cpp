// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tests/library.hpp"

#include <dlfcn.h>

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/system/library.hpp"

#include "imaging/graph/image.hpp"
#include "tests/echo.hpp"

using namespace Godot;
using namespace Perimortem;

static void require(Bool condition, Core::View::Bytes message) {
  if (!condition) {
    Core::Diagnostics::Log::fatal(message);
  }
}

static auto loaded(Core::View::Bytes path) -> Bool {
  Memory::Dynamic::Bytes name(path);
  name.append(0);
  auto* module = dlopen(
      reinterpret_cast<const char*>(name.get_view().get_data()),
      RTLD_NOW | RTLD_NOLOAD);
  if (!module) {
    return False;
  }

  dlclose(module);
  return True;
}

void Tests::Library::check(
    Core::View::Bytes path,
    const Imaging::Graph::Vocabulary& vocabulary) {
  Memory::Allocator::Arena errors;
  require(
      !loaded(path),
      "Lifetime probe must begin with its module unloaded."_view);
  {
    auto& provider =
        Imaging::Graph::Provider::open(path, vocabulary, errors)
            .visit(
                [](Imaging::Graph::Provider& provider)
                    -> Imaging::Graph::Provider& { return provider; },
                [](Core::View::Bytes error) -> Imaging::Graph::Provider& {
                  Core::Diagnostics::Log::fatal(error);
                });
    const U8 pixels[] = {0, 2, 3, 255};
    auto image =
        Imaging::Graph::Image::create(provider, 1, 1, {pixels, 4}, errors)
            .visit(
                [](Imaging::Graph::Image& image) {
                  return Core::Data::take(image);
                },
                [](Core::View::Bytes error) -> Imaging::Graph::Image {
                  Core::Diagnostics::Log::fatal(error);
                });
    const auto operation = Imaging::Graph::Operation::unary<Echo>("echo"_view);
    auto binding = image.bind_operation(operation).visit(
        [](Imaging::Graph::Invocation binding) { return binding; },
        [](Ttx::Semantic::Negotiation::Binding::Failure)
            -> Imaging::Graph::Invocation {
          Core::Diagnostics::Log::fatal(
              "Lifetime probe could not bind Echo."_view);
        });

    // Image is the explicit lifetime owner of this borrowed binding. Releasing
    // the factory selection must neither unload the code nor invalidate its
    // receiver. The final image release must run before closing that code.
    provider.release();
    require(loaded(path), "Image did not retain its executable module."_view);
    binding.invoke(nullptr, nullptr)
        .visit(
            [](image_object result) {
              result.operations->release(result.source);
            },
            [](Core::View::Bytes error) {
              Core::Diagnostics::Log::fatal(error);
            });
  }

  require(
      !loaded(path),
      "Module remained loaded after its final owner ended."_view);
  {
    auto library =
        Perimortem::System::Library::open(path, errors)
            .visit(
                [](Perimortem::System::Library& library) {
                  return Core::Data::take(library);
                },
                [](Core::View::Bytes error) -> Perimortem::System::Library {
                  Core::Diagnostics::Log::fatal(error);
                });
    library.symbol("missing_test_entry"_view, errors)
        .visit(
            [](void*) { require(False, "Missing symbol was accepted."_view); },
            [](Core::View::Bytes error) {
              require(!error.is_empty(), "Loader lost symbol diagnostic."_view);
            });
  }

  require(!loaded(path), "Failed symbol lookup leaked its Library owner."_view);
  Core::Diagnostics::Log::info(
      "PASS: retained binding owner, final module unload and failed symbol cleanup"_view);
}
