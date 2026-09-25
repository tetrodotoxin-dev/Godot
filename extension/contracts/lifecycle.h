// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef GODOT_CONTRACTS_LIFECYCLE_H
#define GODOT_CONTRACTS_LIFECYCLE_H

#include "ttx/data/form/representation.h"

#define GODOT_LIFECYCLE_ID_HIGH 0x784bd16024e14580ULL
#define GODOT_LIFECYCLE_ID_LOW 0xb202072d03474a01ULL

// A scene policy can observe entry and readiness without exposing engine
// notification numbers or native Node storage. The Godot adapter preserves
// engine ordering when invoking these optional callbacks. The subject owns
// their meaning and its own state, and remains borrowed for each call.
typedef struct godot_lifecycle_operations {
  void (*entered)(const void* source);
  void (*ready)(const void* source);
} godot_lifecycle_operations;

typedef struct godot_lifecycle {
  const void* source;
  const godot_lifecycle_operations* operations;
} godot_lifecycle;

PERIMORTEM_C const ttx_representation* godot_lifecycle_representation(void);

#endif
