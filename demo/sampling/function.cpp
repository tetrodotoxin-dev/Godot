// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/sampling/function.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "ttx/semantic/realization/simulacra.hpp"

using namespace Godot::Demo;
using namespace Perimortem;

Sampling::Function::Function(
    Ttx::Concept::Modules::Module module,
    Ttx::Concept::Modules::Module::Acquisition publication,
    Sampling::Contracts::Samples handle)
    : module(Core::Data::take(module)),
      publication(Core::Data::take(publication)),
      handle(handle) {}

Sampling::Function::Function(Function&& other)
    : module(Core::Data::take(other.module)),
      publication(Core::Data::take(other.publication)),
      handle(other.handle) {}

auto Sampling::Function::open(
    Ttx::Concept::Modules::Module module,
    Ttx::Semantic::Negotiation::Query host)
    -> Utility::Result<Function, Core::View::Bytes> {
  using Result = Utility::Result<Function, Core::View::Bytes>;
  return module.open(host).visit(
      [&](Ttx::Concept::Modules::Module::Acquisition& publication) -> Result {
        return Ttx::Semantic::Realization::Simulacra::fulfill<
                   Sampling::Contracts::Samples>(publication.get_query())
            .visit(
                [&](Sampling::Contracts::Samples handle) -> Result {
                  return Function(
                      Core::Data::take(module), Core::Data::take(publication),
                      handle);
                },
                [](Ttx::Semantic::Negotiation::Binding::Failure) -> Result {
                  return "Sampling module did not fulfill the callable."_view;
                });
      },
      [](Ttx::Data::Status) -> Result {
        return "Sampling module could not initialize."_view;
      });
}

auto Sampling::Function::open(
    Core::View::Bytes path,
    Memory::Allocator::Arena& errors)
    -> Utility::Result<Function, Core::View::Bytes> {
  using Result = Utility::Result<Function, Core::View::Bytes>;
  return Ttx::Concept::Modules::Module::load(path, errors)
      .visit(
          [](Ttx::Concept::Modules::Module& module) -> Result {
            return open(Core::Data::take(module));
          },
          [](Core::View::Bytes error) -> Result { return error; });
}
