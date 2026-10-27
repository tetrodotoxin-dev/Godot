// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef GODOT_CONTRACTS_PROVIDER_H
#define GODOT_CONTRACTS_PROVIDER_H

#include "demo/imaging/contracts/image.h"

#define GODOT_IMAGE_PROVIDER_ID_HIGH 0x4ed63fba57314caeULL
#define GODOT_IMAGE_PROVIDER_ID_LOW 0xa9e21ce423faf911ULL

// These counters report provider work for the lab. Images and the factory may
// share counters. These diagnostic values describe work across the factory
// and its images, independently of image identity and synchronization.
typedef struct image_provider_statistics {
  U64 uploads;
  U64 downloads;
  U64 live_images;
  U64 plan_builds;
} image_provider_statistics;

// Provider creates stable RGBA8 snapshots. Dimensions are positive, at most
// 4096, and size must equal width * height * 4. Creation copies the caller's
// pixels before returning. A successful creation supplies one Retained image.
// A failed creation supplies diagnostic bytes under image_error. The host keeps the factory and its executable module
// available through every created image's final release.
typedef struct image_provider_ops {
  ttx_abstract (*get_abstract)(void* context);
  image_provider_statistics (*statistics)(void* context);
  image_error (*create)(
      void* context,
      U32 width,
      U32 height,
      const U8* pixels,
      Count size,
      ttx_retained* output);
} image_provider_ops;

typedef struct image_provider {
  void* context;
  const image_provider_ops* operations;
} image_provider;

#endif
