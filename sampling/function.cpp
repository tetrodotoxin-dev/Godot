// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "sampling/function.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "ttx/semantic/simulacra.hpp"

using namespace Godot;
using namespace Perimortem;

Sampling::Function::Function(
    Modules::Library&& library,
    sample_provider provider,
    Contracts::Samples::Handle handle)
    : library(Core::Data::take(library)), provider(provider), handle(handle) {}

Sampling::Function::Function(Function&& other)
    : library(Core::Data::take(other.library)),
      provider(other.provider),
      handle(other.handle) {
  other.provider.release = nullptr;
}

Sampling::Function::~Function() {
  if (provider.release) {
    provider.release(provider.query.source);
  }
}

auto Sampling::Function::admit(
    Modules::Library&& library,
    sample_provider provider) -> Utility::Result<Function, Core::View::Bytes> {
  return Ttx::Semantic::Simulacra::fulfill<Contracts::Samples>(
             Ttx::Semantic::Query(provider.query))
      .visit(
          [&](const Contracts::Samples::Handle& handle)
              -> Utility::Result<Function, Core::View::Bytes> {
            return Function(Core::Data::take(library), provider, handle);
          },
          [&](Ttx::Semantic::Binding::Failure)
              -> Utility::Result<Function, Core::View::Bytes> {
            provider.release(provider.query.source);
            return "Sampling provider could not fulfill the callable."_view;
          });
}

static auto load(
    const Modules::Library& library,
    Memory::Allocator::Arena& errors)
    -> Utility::Result<sample_provider, Core::View::Bytes> {
  return library.symbol("godot_sample_provider_open_v1"_view, errors)
      .visit(
          [](void* address)
              -> Utility::Result<sample_provider, Core::View::Bytes> {
            const auto open = reinterpret_cast<sample_provider_open>(address);
            sample_provider provider = {};
            if (open(&provider) != TTX_DATA_SUCCESS) {
              return "Sampling provider could not initialize."_view;
            }

            return provider;
          },
          [](Core::View::Bytes error)
              -> Utility::Result<sample_provider, Core::View::Bytes> {
            return error;
          });
}

auto Sampling::Function::open(
    Core::View::Bytes path,
    Memory::Allocator::Arena& errors)
    -> Utility::Result<Function, Core::View::Bytes> {
  return Modules::Library::open(path, errors)
      .visit(
          [&](Modules::Library& library)
              -> Utility::Result<Function, Core::View::Bytes> {
            return load(library, errors)
                .visit(
                    [&](sample_provider provider)
                        -> Utility::Result<Function, Core::View::Bytes> {
                      return admit(Core::Data::take(library), provider);
                    },
                    [](Core::View::Bytes error)
                        -> Utility::Result<Function, Core::View::Bytes> {
                      return error;
                    });
          },
          [](Core::View::Bytes error)
              -> Utility::Result<Function, Core::View::Bytes> {
            return error;
          });
}
