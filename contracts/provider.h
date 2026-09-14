// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef GODOT_CONTRACTS_PROVIDER_H
#define GODOT_CONTRACTS_PROVIDER_H

#include "contracts/image.h"

// A factory publication establishes enough agreement to release its state
// even when no optional image operation can be fulfilled. The versioned module
// entry and admission of an existing host object both transfer one factory
// reference with this table. Its release operation owns that supplying state.
// Images keep their required resources independently, while their host owners
// retain the factory and any loaded code through the final release.
//
// Statistics expose the device work needed to review the lab: uploads,
// downloads, live image allocations and FFT plan builds. They describe this
// implementation's work, not the behavioral identity of an image operation.
typedef struct image_provider_statistics {
  U64 uploads;
  U64 downloads;
  U64 live_images;
  U64 plan_builds;
} image_provider_statistics;

typedef struct image_provider_operations {
  void (*release)(const void*);
  image_provider_statistics (*statistics)(const void*);
  image_error (*create)(
      const void*,
      U32 width,
      U32 height,
      const U8* pixels,
      Count size,
      image_object* output);
} image_provider_operations;

typedef struct image_provider {
  const void* source;
  const image_provider_operations* operations;
} image_provider;

typedef image_error (*image_provider_open)(image_provider* output);

PERIMORTEM_C __attribute__((visibility("default"))) image_error
    godot_image_provider_open_v2(image_provider* output);

#endif
