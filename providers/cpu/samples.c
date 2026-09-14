// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "providers/cpu/samples.h"

// This is the actual C entry installed in the fulfilled table. Each index is
// independent, allowing callers to partition work without coordinating mutable
// random state. The loop owns no input array or result allocation.
ttx_data_status godot_cpu_samples(const void* source, U32 seed, U32 first, U32 count, U64* output) {
  (void)source;
  if ((U64)first + count > ((U64)1 << 32)) {
    return TTX_DATA_BOUNDS;
  }

  U64 hits = 0;
  for (U64 i = 0; i < count; ++i) {
    U32 value = (first + (U32)i) ^ seed;
    value = ((value >> 16) ^ value) * (U32)0x045d9f3b;
    value = ((value >> 16) ^ value) * (U32)0x045d9f3b;
    value = (value >> 16) ^ value;
    const U64 x = value & 65535;
    const U64 y = value >> 16;
    hits += x * x + y * y < ((U64)1 << 32);
  }

  *output = hits;
  return TTX_DATA_SUCCESS;
}
