// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "extensions/counter/counter.h"

S64 counter_advance(counter* source, S64 amount) {
  source->value = (S64)((U64)source->value + (U64)amount);
  return source->value;
}
