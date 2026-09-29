// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/imaging/graph/provider.hpp"

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/object.hpp"

#include "demo/imaging/contracts/render.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/capabilities/borrow.hpp"
#include "ttx/concept/policies/borrowed.hpp"

using namespace Godot::Demo;
using namespace Perimortem;

Imaging::Graph::Provider::Provider(
    image_provider factory,
    const Vocabulary& contract,
    Core::Option<Ttx::Semantic::Negotiation::Library> module)
    : module(Core::Data::take(module)), factory(factory), contract(contract) {}

Imaging::Graph::Provider::~Provider() {
  factory.operations->release(factory.source);
  if (lifetime) {
    lifetime->release();
  }
}

auto Imaging::Graph::Provider::adopt(
    image_provider owned,
    const Vocabulary& vocabulary,
    Core::Option<Ttx::Semantic::Negotiation::Library> module) -> Provider& {
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

auto Imaging::Graph::Provider::open(
    Ttx::Semantic::Negotiation::Query subject,
    const Vocabulary& vocabulary,
    Memory::Allocator::Arena& errors)
    -> Utility::Result<Provider&, Core::View::Bytes> {
  using Result = Utility::Result<Provider&, Core::View::Bytes>;
  using namespace Ttx::Semantic::Negotiation;
  if (subject.supports<Imaging::Contracts::Render>() ==
      Binding::Status::Unknown) {
    const auto selected = subject.bind<Ttx::Concept::Abstract>().visit(
        [](Ttx::Concept::Abstract root) {
          return root.resolve_concept("Imaging"_view)
              .resolve_concept("Render"_view)
              .get_query();
        },
        [](Binding::Failure) { return Query(); });
    subject = selected;
  }
  return subject.bind<Ttx::Concept::Capabilities::Borrow>().visit(
      [&](Ttx::Concept::Capabilities::Borrow policy) -> Result {
        return policy.borrow().visit(
            [&](Ttx::Concept::Policies::Borrowed acquired) -> Result {
              return acquired.bind<Imaging::Contracts::Render>().visit(
                  [&](Imaging::Contracts::Render render) -> Result {
                    return render.open().visit(
                        [&](image_provider backend) -> Result {
                          auto& provider = adopt(backend, vocabulary);
                          provider.lifetime = acquired;
                          return provider;
                        },
                        [&](Core::View::Bytes error) -> Result {
                          const auto message = errors.proxy(error);
                          acquired.release();
                          return message;
                        });
                  },
                  [&](Binding::Failure) -> Result {
                    acquired.release();
                    return "Imported subject did not supply Render."_view;
                  });
            },
            [](Binding::Failure) -> Result {
              return "Render provider could not retain its runtime."_view;
            });
      },
      [](Binding::Failure) -> Result {
        return "Render provider did not supply Borrow."_view;
      });
}

auto Imaging::Graph::Provider::open(
    Core::View::Bytes path,
    const Vocabulary& vocabulary,
    Memory::Allocator::Arena& errors)
    -> Utility::Result<Provider&, Core::View::Bytes> {
  using Result = Utility::Result<Provider&, Core::View::Bytes>;
  return Ttx::Semantic::Negotiation::Library::open(path, errors)
      .visit(
          [&](Ttx::Semantic::Negotiation::Library& library) -> Result {
            return open(Core::Data::take(library), vocabulary, errors);
          },
          [](Core::View::Bytes error) -> Result { return error; });
}

auto Imaging::Graph::Provider::open(
    Ttx::Semantic::Negotiation::Library library,
    const Vocabulary& vocabulary,
    Memory::Allocator::Arena& errors)
    -> Utility::Result<Provider&, Core::View::Bytes> {
  using namespace Ttx::Semantic::Negotiation;
  Core::Option<Provider&> result;
  Core::View::Bytes error = "Library did not supply a rendering Query."_view;
  auto receive = [&](Query subject) {
    return open(subject, vocabulary, errors)
        .visit(
            [&](Provider& value) {
              value.module = library;
              result = value;
              return Binding::Status::Satisfied;
            },
            [&](Core::View::Bytes failure) {
              error = failure;
              return Binding::Status::Rejected;
            });
  };
  if (library.visit(Query(), Receiver(receive)) != Binding::Status::Satisfied ||
      !result) {
    return error;
  }
  return *result;
}
