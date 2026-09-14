// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef GODOT_CONTRACTS_IMAGE_H
#define GODOT_CONTRACTS_IMAGE_H

#include "ttx/data/form/representation.h"
#include "ttx/semantic/query.h"

// An image can keep its pixels in CPU memory, on a device, or behind another
// implementation entirely. These records describe what crosses the module
// boundary so the host never needs the provider's native object layout.
// The module entry or host admission establishes the System V AMD64 calling
// agreement.
//
// A successful creation or operation writes one owned image reference. Failure
// leaves that output unused and lends diagnostic bytes until the next provider
// call. The caller copies a diagnostic before releasing its supplying module.
typedef struct image_error {
  const U8* data;
  Count size;
} image_error;

typedef struct image_dimensions {
  U32 width;
  U32 height;
} image_dimensions;

// Creation supplies this bootstrap table so the caller can manage lifetime
// before requesting optional image operations. Query acquires those callables.
// Pixels independently publishes a Data transport for the RGBA8 observation.
// Representation describes that public pixel surface, not the private payload.
// Binding an operation therefore need not download a device image.
typedef struct image_operations {
  void (*retain)(const void*);
  void (*release)(const void*);
  image_dimensions (*dimensions)(const void*);
  ttx_semantic_query (*query)(const void*);
  ttx_semantic_query (*pixels)(const void*);
  const ttx_representation* (*representation)(const void*);
} image_operations;

// The receiver belongs to the provider and is interpreted only by its thunks.
// Copying these fields does not retain the image or the executable code. The
// host pairs an owned image reference with the module lifetime and invokes
// release before that module can unload. Stateless operations may use null
// source. A usable operation table is always present.
typedef struct image_object {
  const void* source;
  const image_operations* operations;
} image_object;

// A convolution borrows float32 coefficients in row order for one synchronous
// call. The caller may keep them in an authored graph node or a temporary
// vector. Neither representation becomes part of the provider's object model.
// The odd dimensions select the exact center used by convolution with zero
// padding.
typedef struct image_kernel {
  U32 width;
  U32 height;
  const R32* values;
  Count count;
} image_kernel;

#define GODOT_IMAGE_INVERT_ID_HIGH ((U64)0xaa57de9dd2694e29ULL)
#define GODOT_IMAGE_INVERT_ID_LOW ((U64)0x88925a047fbf0601ULL)
#define GODOT_IMAGE_CONVOLVE_ID_HIGH ((U64)0xaa57de9dd2694e29ULL)
#define GODOT_IMAGE_CONVOLVE_ID_LOW ((U64)0x88925a047fbf0602ULL)
#define GODOT_IMAGE_COMPOSITE_ID_HIGH ((U64)0xaa57de9dd2694e29ULL)
#define GODOT_IMAGE_COMPOSITE_ID_LOW ((U64)0x88925a047fbf0603ULL)

// Each UUID fixes its complete signature and behavior. These tables happen to
// have the same storage shape, but their functions cannot be interchanged on
// that evidence. Fulfillment supplies the table for the requested contract.
// All operations preserve the source and return an independently retained
// result. Convolution and composition can fail without certifying an image.
typedef struct image_invert_operations {
  image_error (*apply)(const void*, image_object*);
} image_invert_operations;

typedef struct image_convolve_operations {
  image_error (*apply)(const void*, image_kernel, image_object*);
} image_convolve_operations;

typedef struct image_composite_operations {
  image_error (*apply)(const void*, image_object, image_object*);
} image_composite_operations;

#endif
