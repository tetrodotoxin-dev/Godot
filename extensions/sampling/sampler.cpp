// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "extensions/sampling/sampler.hpp"

#include "perimortem/core/null_terminated.hpp"

using namespace Godot;
using namespace Perimortem;

auto Extensions::Sampling::Sampler::configure(Core::View::Bytes provider)
    -> bool {
  return imports.open(provider).visit(
      [&](Ttx::Concept::Modules::Module& module) {
        return Godot::Sampling::Function::open(Core::Data::take(module), host)
            .visit(
                [&](Godot::Sampling::Function& value) {
                  function = Core::Data::take(value);
                  error.clear();
                  return true;
                },
                [&](Core::View::Bytes message) {
                  error = message;
                  return false;
                });
      },
      [&](Ttx::Data::Status) {
        error = "The host could not import the requested sampling module."_view;
        return false;
      });
}

auto Extensions::Sampling::Sampler::count(S64 seed, S64 first, S64 size)
    -> S64 {
  if (!function) {
    error = "Configure a sampling provider before calling it."_view;
    return -1;
  }

  if (seed < 0 || seed > 0xffffffffLL || first < 0 || first > 0xffffffffLL ||
      size < 0 || size > 0xffffffffLL) {
    error = "Sampling arguments must be unsigned 32-bit integers."_view;
    return -1;
  }

  return function->get_handle()
      .count(U32(seed), U32(first), U32(size))
      .visit(
          [&](U64 hits) -> S64 {
            error.clear();
            return S64(hits);
          },
          [&](Ttx::Data::Status status) -> S64 {
            error =
                status == Ttx::Data::Status::Bounds
                    ? "Sampling interval exceeds the 32-bit domain."_view
                    : "Sampling provider could not complete the operation."_view;
            return -1;
          });
}
