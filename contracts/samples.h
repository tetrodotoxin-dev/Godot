// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef GODOT_CONTRACTS_SAMPLES_H
#define GODOT_CONTRACTS_SAMPLES_H

#include "ttx/data/status.h"

// Count samples inside a quarter disk without constructing a result object.
// A sample is determined by its index and seed, so splitting an interval across
// providers and adding their counts must give exactly the same answer.
//
// For each U32 index, start with v = index XOR seed. Twice replace v with
// (v XOR (v >> 16)) * 0x045d9f3b modulo 2^32, then XOR v with v >> 16.
// x is its low 16 bits and y its high 16 bits. Count x*x + y*y < 2^32 using
// U64 arithmetic. The permutation is a deterministic workload, not a promise
// of cryptographic randomness or statistical error bounds.
//
// The half-open interval must end at or before 2^32. Empty intervals succeed
// with zero. Success writes one U64 into caller storage. Failure leaves it
// unchanged. Calls are synchronous and serialized per publication. Its owner
// establishes the worker and executable lifetime. The receiver can be null
// for a stateless provider.
typedef struct sample_operations {
  ttx_data_status (*count)(const void*, U32 seed, U32 first, U32 count, U64* output);
} sample_operations;

#define GODOT_SAMPLES_ID_HIGH ((U64)0x17f8491dd3bc4c4dULL)
#define GODOT_SAMPLES_ID_LOW ((U64)0xb6c46f4f4530a901ULL)

#endif
