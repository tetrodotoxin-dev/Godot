// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef GODOT_CONTRACTS_IMAGE_H
#define GODOT_CONTRACTS_IMAGE_H

#include "ttx/graph/policies/retained.h"

#include "compute/image/pixels.h"

// Lab image operations preserve the source snapshot and publish an independent
// result through Retained. The result supplies Compute Pixels and may offer
// further image operations. The consumer releases each successful result once
// and keeps its supplying module loaded through that release.
//
// Empty diagnostic bytes mean success. Failure transfers no acquisition and
// leaves the output record unusable. Diagnostic bytes remain readable until
// the next call through the supplying API. The consumer copies them before
// another call or the end of that API's access duration.
typedef struct image_error {
  const U8* data;
  Count size;
} image_error;

// Coefficients describe centered linear convolution with zero padding. Width
// and height are odd, between one and 1023, and count equals their product.
// Values are finite with magnitude at most one million. Providers may refuse
// work beyond an advertised execution budget. The coefficient array remains
// readable until the synchronous call returns.
typedef struct image_kernel {
  U32 width;
  U32 height;
  const R32* values;
  Count count;
} image_kernel;

#define GODOT_IMAGE_INVERT_ID_HIGH 0xc5f32a681edf425eULL
#define GODOT_IMAGE_INVERT_ID_LOW 0xb81759d7620fa301ULL
#define GODOT_IMAGE_CONVOLVE_ID_HIGH 0xc5f32a681edf425eULL
#define GODOT_IMAGE_CONVOLVE_ID_LOW 0xb81759d7620fa302ULL
#define GODOT_IMAGE_COMPOSITE_ID_HIGH 0xc5f32a681edf425eULL
#define GODOT_IMAGE_COMPOSITE_ID_LOW 0xb81759d7620fa303ULL

// Invert complements RGB bytes and preserves alpha and dimensions.
typedef struct image_invert_ops {
  ttx_abstract (*get_abstract)(void* context);
  image_error (*apply)(void* context, ttx_retained* output);
} image_invert_ops;

typedef struct image_invert {
  void* context;
  const image_invert_ops* operations;
} image_invert;

// Convolve rounds clamped RGB results to the nearest byte and preserves source
// alpha. An implementation may differ by one byte through floating point
// rounding. A failed operation leaves the source snapshot unchanged.
typedef struct image_convolve_ops {
  ttx_abstract (*get_abstract)(void* context);
  image_error (
      *apply)(void* context, image_kernel kernel, ttx_retained* output);
} image_convolve_ops;

typedef struct image_convolve {
  void* context;
  const image_convolve_ops* operations;
} image_convolve;

// Composite places the overlay over the source using straight alpha. Both
// images must supply Pixels with equal dimensions. The provider interprets the
// overlay solely through its published operations and keeps no access to that
// Abstract after apply returns. RGB and alpha use nearest integer rounding.
typedef struct image_composite_ops {
  ttx_abstract (*get_abstract)(void* context);
  image_error (
      *apply)(void* context, ttx_abstract overlay, ttx_retained* output);
} image_composite_ops;

typedef struct image_composite {
  void* context;
  const image_composite_ops* operations;
} image_composite;

#endif
