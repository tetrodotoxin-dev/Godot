// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/sampling/class/sampler.hpp"

#include "perimortem/core/null_terminated.hpp"

using namespace Godot::Demo;
using namespace Perimortem;

auto Sampling::Class::Sampler::configure(Core::View::Bytes provider) -> bool {
  using namespace Ttx::Semantic::Negotiation;
  Core::Option<Sampling::Function> replacement;
  auto receive = [&](Ttx::Concept::Abstract subject) {
    Sampling::Function::open(subject.get_query())
        .visit(
            [&](Sampling::Function& value) {
              replacement = Core::Data::take(value);
              return Binding::Status::Satisfied;
            },
            [&](Core::View::Bytes failure) {
              error = failure;
              return Binding::Status::Rejected;
            });
  };
  const perimortem_view_bytes input =
      perimortem_view_bytes(provider.get_data(), provider.get_size());
  const auto status = imports.visit(
      &input,
      Ttx::Data::Form::Compiled<Ttx::Data::Form::Native<
          perimortem_view_bytes>::reference>::get_representation(),
      receive);
  if (status != Binding::Status::Satisfied || !replacement) {
    if (status == Binding::Status::Unknown) {
      error = "The host does not recognize this sampling import."_view;
    }
    return false;
  }
  function = Core::Data::take(*replacement);
  error.clear();
  return true;
}

auto Sampling::Class::Sampler::count(S64 seed, S64 first, S64 size) -> S64 {
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
