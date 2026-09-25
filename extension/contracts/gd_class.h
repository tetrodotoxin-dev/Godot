// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef GODOT_CONTRACTS_GD_CLASS_H
#define GODOT_CONTRACTS_GD_CLASS_H

#include "ttx/data/status.h"
#include "ttx/semantic/ownership/publication.h"

#define GODOT_GDCLASS_ID_HIGH 0x9450ab91b4c748c4ULL
#define GODOT_GDCLASS_ID_LOW 0xa3c2bec04bb21602ULL

// GDClass is a host policy over a constructible Abstract. Emission consumes
// that graph's declarations and returns an independent Class publication.
// The policy may then disappear with the source graph. Runtime registration,
// construction and invocation use only the emitted terminal's own state.
typedef struct godot_gdclass_operations {
  ttx_data_status (*emit)(const void* source, ttx_publication* output);
} godot_gdclass_operations;

typedef struct godot_gdclass {
  const void* source;
  const godot_gdclass_operations* operations;
} godot_gdclass;

#endif
