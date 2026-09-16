// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef COUNTER_COUNTER_H
#define COUNTER_COUNTER_H

#include "perimortem/core/perimortem.h"

// The counter's arithmetic is independent of publication and scene policy.
// Unsigned addition defines wrapping before the signed result is observed.
typedef struct counter {
  S64 value;
} counter;

PERIMORTEM_C S64 counter_advance(counter* source, S64 amount);

#endif
