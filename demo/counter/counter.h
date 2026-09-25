// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef COUNTER_COUNTER_H
#define COUNTER_COUNTER_H

#include "perimortem/core/perimortem.h"

#define COUNTER_METHOD_HIGH 0x27b62d5b8e84474fULL
#define COUNTER_METHOD_LOW 0x8c5fba7554191210ULL

// The counter's arithmetic is independent of publication and scene policy.
// Unsigned addition defines wrapping before the signed result is observed.
typedef struct counter {
  S64 value;
} counter;

PERIMORTEM_C S64 counter_advance(counter* source, S64 amount);

#endif
