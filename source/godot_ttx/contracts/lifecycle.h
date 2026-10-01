// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef GODOT_CONTRACTS_LIFECYCLE_H
#define GODOT_CONTRACTS_LIFECYCLE_H

#include "ttx/concept/abstract.h"
#include "ttx/data/form/representation.h"

#define GODOT_LIFECYCLE_ID_HIGH 0x784bd16024e14580ULL
#define GODOT_LIFECYCLE_ID_LOW 0xb202072d03474a01ULL

// Observes when an instance enters the scene and becomes ready. The adapter
// delivers these callbacks in Godot's notification order while the provider
// defines their effects on its own state. The bound view preserves that
// instance's Abstract operations, so later queries retain its policy.
// Binding this view shares the supplying observation. A caller that needs
// longer access negotiates Borrow on the instance and releases that acquisition
// after its callbacks and other retained operations have finished.
typedef struct godot_lifecycle_operations {
  ttx_abstract_ops abstract;
  void (*entered)(const void* source);
  void (*ready)(const void* source);
} godot_lifecycle_operations;

typedef struct godot_lifecycle {
  const void* source;
  const godot_lifecycle_operations* operations;
} godot_lifecycle;

PERIMORTEM_C const ttx_representation* godot_lifecycle_representation(void);

#endif
