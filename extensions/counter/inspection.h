// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef COUNTER_INSPECTION_H
#define COUNTER_INSPECTION_H

#include "ttx/data/form/representation.h"

#define COUNTER_INSPECTION_HIGH 0x27b62d5b8e84474fULL
#define COUNTER_INSPECTION_LOW 0x8c5fba7554191200ULL
#define COUNTER_METHOD_HIGH 0x27b62d5b8e84474fULL
#define COUNTER_METHOD_LOW 0x8c5fba7554191210ULL

// The lab observes lifetime and dispatch costs through a separate capability.
// These counters belong to this example module, not to the TTX allocator or
// global test infrastructure. A snapshot remains usable after graph release.
typedef struct counter_statistics {
  U64 graphs_opened;
  U64 graphs_closed;
  U64 graph_queries;
  U64 factories_opened;
  U64 factories_closed;
  U64 instances_opened;
  U64 instances_closed;
  U64 binds;
  U64 calls;
} counter_statistics;

typedef struct counter_inspection_operations {
  counter_statistics (*snapshot)(const void* source);
} counter_inspection_operations;

typedef struct counter_inspection_api {
  const void* source;
  const counter_inspection_operations* operations;
} counter_inspection_api;
PERIMORTEM_C const ttx_representation* counter_inspection_representation(void);

#endif
