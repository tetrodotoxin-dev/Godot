// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/sampling/function.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "ttx/concept/capabilities/borrow.hpp"
#include "ttx/semantic/realization/simulacra.hpp"

using namespace Godot::Demo;
using namespace Perimortem;

Sampling::Function::Function(
    Core::Option<Ttx::Semantic::Negotiation::Library> module,
    Ttx::Concept::Policies::Borrowed publication,
    Sampling::Contracts::Samples handle)
    : module(Core::Data::take(module)),
      publication(publication),
      handle(handle) {}

Sampling::Function::Function(Function&& other)
    : module(Core::Data::take(other.module)),
      publication(other.publication),
      handle(other.handle) {
  other.publication = {};
}

Sampling::Function::~Function() {
  if (publication) {
    publication->release();
  }
}

auto Sampling::Function::open(Ttx::Semantic::Negotiation::Query subject)
    -> Utility::Result<Function, Core::View::Bytes> {
  using Result = Utility::Result<Function, Core::View::Bytes>;
  using namespace Ttx::Semantic::Negotiation;
  if (subject.supports<Sampling::Contracts::Samples>() ==
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
              return acquired.bind<Sampling::Contracts::Samples>().visit(
                  [&](Sampling::Contracts::Samples api) -> Result {
                    return Function({}, acquired, api);
                  },
                  [&](Binding::Failure) -> Result {
                    acquired.release();
                    return "Imported subject did not supply Samples."_view;
                  });
            },
            [](Binding::Failure) -> Result {
              return "Sampling provider could not retain its computation."_view;
            });
      },
      [](Binding::Failure) -> Result {
        return "Sampling provider did not supply Borrow."_view;
      });
}

auto Sampling::Function::open(
    Ttx::Semantic::Negotiation::Library library,
    Ttx::Semantic::Negotiation::Query host)
    -> Utility::Result<Function, Core::View::Bytes> {
  using namespace Ttx::Semantic::Negotiation;
  Core::Option<Function> result;
  Core::View::Bytes error = "Sampling provider did not supply a Query."_view;
  auto receive = [&](Query query) {
    return open(query).visit(
        [&](Function& value) {
          value.module = library;
          result = Core::Data::take(value);
          return Binding::Status::Satisfied;
        },
        [&](Core::View::Bytes failure) {
          error = failure;
          return Binding::Status::Rejected;
        });
  };
  if (library.visit(host, Receiver(receive)) != Binding::Status::Satisfied ||
      !result) {
    return error;
  }
  return Core::Data::take(*result);
}

auto Sampling::Function::open(
    Core::View::Bytes path,
    Memory::Allocator::Arena& errors)
    -> Utility::Result<Function, Core::View::Bytes> {
  using Result = Utility::Result<Function, Core::View::Bytes>;
  return Ttx::Semantic::Negotiation::Library::open(path, errors)
      .visit(
          [](Ttx::Semantic::Negotiation::Library& module) -> Result {
            return open(Core::Data::take(module));
          },
          [](Core::View::Bytes error) -> Result { return error; });
}
