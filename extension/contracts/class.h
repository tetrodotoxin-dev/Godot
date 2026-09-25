// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef GODOT_CONTRACTS_CLASS_H
#define GODOT_CONTRACTS_CLASS_H

#include "ttx/data/status.h"

#define GODOT_CLASS_ID_HIGH 0x9450ab91b4c748c4ULL
#define GODOT_CLASS_ID_LOW 0xa3c2bec04bb21601ULL

// An emitted class owns prepared registration and runtime construction data.
// Publishing installs it into Godot. Ending its enclosing publication removes
// that registration after all instances have died, then releases its factory
// and code. No source Abstract is retained by this terminal.
typedef struct godot_class_operations {
  ttx_data_status (*publish)(const void* source);
} godot_class_operations;

typedef struct godot_class {
  const void* source;
  const godot_class_operations* operations;
} godot_class;

#endif
