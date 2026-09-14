// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef GODOT_SAMPLING_PROVIDER_H
#define GODOT_SAMPLING_PROVIDER_H

#include "ttx/semantic/query.h"
#include "ttx/data/status.h"

// The module entry transfers a publication before any optional callable is
// negotiated. Release receives query.source and closes that publication even
// if fulfillment fails. The caller keeps the module loaded through release.
// This bootstrap uses System V AMD64, like the lab's image provider entry.
// It creates no image factory and requires no Godot object.
typedef struct sample_provider {
  ttx_semantic_query query;
  void (*release)(const void*);
} sample_provider;

typedef ttx_data_status (*sample_provider_open)(sample_provider*);

PERIMORTEM_C __attribute__((visibility("default"))) ttx_data_status
    godot_sample_provider_open_v1(sample_provider* output);

#endif
