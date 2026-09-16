// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "extensions/counter/inspection.h"

#include <stddef.h>
#include <stdlib.h>

// The C provider authors its record independently of the C++ consumer's
// Native description. Both use the canonical compiler, while this fixture
// remains buildable with a C compiler and the public TTX runtime alone.
static const ttx_schema integer = {
  8, 8, TTX_SCHEMA_VALUE, {.value = {TTX_SCHEMA_U64, TTX_SCHEMA_LITTLE_ENDIAN}}};
#define STAT(member) {{&integer, 0}, offsetof(counter_statistics, member)}
static const ttx_schema_position fields[] = {
  STAT(graphs_opened), STAT(graphs_closed), STAT(graph_queries),
  STAT(factories_opened), STAT(factories_closed), STAT(instances_opened),
  STAT(instances_closed), STAT(binds), STAT(calls)};
#undef STAT
static const ttx_schema statistics = {
  sizeof(counter_statistics), _Alignof(counter_statistics), TTX_SCHEMA_COMPOSITE,
  {.composite = {fields, 9}}};
static const ttx_schema_argument arguments[] = {
  {{NULL, TTX_SCHEMA_REFERENCE_POINTER}, 1}};
static const ttx_schema snapshot = {
  8, 8, TTX_SCHEMA_CALLABLE,
  {.callable = {arguments, 1, {&statistics, 0}, TTX_SCHEMA_SYSTEM_V_AMD64}}};
static const ttx_schema_position operations[] = {
  {{&snapshot, 0}, offsetof(counter_inspection_operations, snapshot)}};
static const ttx_schema table = {
  sizeof(counter_inspection_operations), _Alignof(counter_inspection_operations),
  TTX_SCHEMA_COMPOSITE, {.composite = {operations, 1}}};
static const ttx_schema_position api_fields[] = {
  {{NULL, TTX_SCHEMA_REFERENCE_POINTER}, offsetof(counter_inspection_api, source)},
  {{&table, TTX_SCHEMA_REFERENCE_POINTER}, offsetof(counter_inspection_api, operations)}};
static const ttx_schema api = {
  sizeof(counter_inspection_api), _Alignof(counter_inspection_api),
  TTX_SCHEMA_COMPOSITE, {.composite = {api_fields, 2}}};

// The fixture is admitted on one host worker. Its first inspection prepares
// this description, which remains in the module through the final release.
static _Alignas(16) U8 storage[1024];
static const ttx_representation* representation;

static void* allocate(void* source, Count size, Count alignment) {
  (void)source;
  return size <= sizeof(storage) && alignment <= 16 ? storage : NULL;
}

const ttx_representation* counter_inspection_representation(void) {
  if (!representation) {
    const ttx_representation_allocator allocator = {NULL, allocate};
    if (ttx_representation_compile(
            (ttx_schema_reference){&api, 0}, allocator, &representation) !=
        TTX_DATA_SUCCESS) {
      abort();
    }
  }

  return representation;
}
