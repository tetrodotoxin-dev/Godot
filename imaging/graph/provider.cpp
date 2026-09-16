// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "imaging/graph/provider.hpp"

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/object.hpp"

#include "imaging/contracts/render.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/declarations/extensible.hpp"

using namespace Godot;
using namespace Perimortem;

Imaging::Graph::Provider::Provider(
    image_provider factory,
    const Vocabulary& contract,
    Core::Option<Ttx::Concept::Modules::Module> module)
    : module(Core::Data::take(module)), factory(factory), contract(contract) {}

Imaging::Graph::Provider::~Provider() {
  factory.operations->release(factory.source);
}

auto Imaging::Graph::Provider::adopt(
    image_provider owned,
    const Vocabulary& vocabulary,
    Core::Option<Ttx::Concept::Modules::Module> module) -> Provider& {
  static const Core::Object<>::Descriptor descriptor(
      sizeof(Provider), alignof(Provider),
      [](U8* value) { reinterpret_cast<Provider*>(value)->~Provider(); });
  auto storage = Core::Object<>::create(descriptor).get_payload();
  return *new (storage, Core::Placement::Construct)
      Provider(owned, vocabulary, Core::Data::take(module));
}

auto Imaging::Graph::Provider::statistics() const -> image_provider_statistics {
  return factory.operations->statistics(factory.source);
}

void Imaging::Graph::Provider::retain() {
  Core::Object<>(reinterpret_cast<U8*>(this)).retain();
}

void Imaging::Graph::Provider::release() {
  Core::Object<>(reinterpret_cast<U8*>(this)).release();
}

auto Imaging::Graph::Provider::create(
    U32 w,
    U32 h,
    Core::View::Bytes pixels,
    image_object& output) const -> image_error {
  return factory.operations->create(
      factory.source, w, h, pixels.get_data(), pixels.get_size(), &output);
}

// Only the independently owned backend leaves Provider::open. The declaration
// and emitted factory can both be released before any image is created. Copy
// diagnostics inside that lifetime because their owner may be one of those
// temporary publications.
static auto open_backend(
    Ttx::Concept::Declarations::Extensible declaration,
    Memory::Allocator::Arena& errors)
    -> Utility::Result<image_provider, Core::View::Bytes> {
  using Result = Utility::Result<image_provider, Core::View::Bytes>;
  return declaration.emit_factory().visit(
      [&](Ttx::Semantic::Ownership::Publication& factory) -> Result {
        return factory.get_query().bind<Imaging::Contracts::Render>().visit(
            [&](Imaging::Contracts::Render render) -> Result {
              return render.open().visit(
                  [](image_provider owned) -> Result { return owned; },
                  [&](Core::View::Bytes error) -> Result {
                    return errors.proxy(error);
                  });
            },
            [](Ttx::Semantic::Negotiation::Binding::Failure) -> Result {
              return "Render factory does not supply persistent images."_view;
            });
      },
      [](Ttx::Data::Status) -> Result {
        return "Render could not emit an independent factory."_view;
      });
}

static auto find_backend(
    Ttx::Concept::Abstract root,
    Memory::Allocator::Arena& errors)
    -> Utility::Result<image_provider, Core::View::Bytes> {
  using Result = Utility::Result<image_provider, Core::View::Bytes>;
  const auto declaration =
      root.resolve_concept("Imaging"_view).resolve_concept("Render"_view);
  return declaration.bind<Ttx::Concept::Declarations::Extensible>().visit(
      [&](Ttx::Concept::Declarations::Extensible extensible) -> Result {
        return open_backend(extensible, errors);
      },
      [](Ttx::Semantic::Negotiation::Binding::Failure) -> Result {
        return "Module does not expose an Extensible Render."_view;
      });
}

auto Imaging::Graph::Provider::open(
    Core::View::Bytes path,
    const Vocabulary& vocabulary,
    Memory::Allocator::Arena& errors)
    -> Utility::Result<Provider&, Core::View::Bytes> {
  using Result = Utility::Result<Provider&, Core::View::Bytes>;
  return Ttx::Concept::Modules::Module::load(path, errors)
      .visit(
          [&](Ttx::Concept::Modules::Module& module) -> Result {
            return module.open().visit(
                [&](Ttx::Concept::Modules::Module::Acquisition& discovery)
                    -> Result {
                  return find_backend(discovery, errors)
                      .visit(
                          [&](image_provider owned) -> Result {
                            return adopt(
                                owned, vocabulary, Core::Data::take(module));
                          },
                          [](Core::View::Bytes error) -> Result {
                            return error;
                          });
                },
                [](Ttx::Data::Status) -> Result {
                  return "Module discovery could not open."_view;
                });
          },
          [](Core::View::Bytes error) -> Result { return error; });
}
