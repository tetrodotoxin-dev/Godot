// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "contracts/kernel.hpp"

#include "perimortem/core/null_terminated.hpp"

using namespace Godot;
using namespace Perimortem;

auto Contracts::Kernel::validate(U32 w, U32 h, Core::View::Vector<R32> values)
    -> Core::View::Bytes {
  if (!w || !h || w > 1023 || h > 1023) {
    return "Kernel dimensions must be positive and at most 1023."_view;
  }

  if (!(w & 1) || !(h & 1)) {
    return "Kernel dimensions must be odd to select one center."_view;
  }

  if (Count(w) * h != values.get_size()) {
    return "Kernel requires exactly width times height coefficients."_view;
  }

  for (Count i = 0; i < values.get_size(); ++i) {
    if (!__builtin_isfinite(values[i]) ||
        __builtin_fabsf(values[i]) > 1000000) {
      return "Kernel coefficients must be finite with magnitude at most one million"_view;
    }
  }

  return {};
}
