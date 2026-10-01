// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef SAMPLING_CONTRACTS_H
#define SAMPLING_CONTRACTS_H

#include "ttx/data/form/representation.h"

// Configure selects a compute import, Count observes its deterministic sample
// count, and Error lends the last diagnostic. Their signatures are published
// by the declaration graph and fulfilled by each runtime instance.
#define SAMPLER_METHOD_HIGH 0x678acb6c12fe4dceULL
#define SAMPLER_METHOD_LOW 0x9d30aa83aa413010ULL

// The payload records are the native wire forms agreed by the declaration and
// runtime adapter. Names remain graph metadata and never enter execution.
typedef struct sampler_count_input {
  S64 seed;
  S64 first;
  S64 size;
} sampler_count_input;

PERIMORTEM_C const ttx_representation* sampler_input_representation(U32 method);
PERIMORTEM_C const ttx_representation* sampler_output_representation(U32 method);

#endif
