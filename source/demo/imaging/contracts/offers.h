// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef GODOT_CONTRACTS_OFFERS_H
#define GODOT_CONTRACTS_OFFERS_H

#include "perimortem/core/view/bytes.h"

#include "demo/imaging/contracts/image.h"

#define GODOT_IMAGE_OFFERS_ID_HIGH ((U64)0x0d49e5ea975747f1ULL)
#define GODOT_IMAGE_OFFERS_ID_LOW ((U64)0x86e8aac5dde60552ULL)

#define IMAGE_INPUT_NONE ((U8)0)
#define IMAGE_INPUT_KERNEL ((U8)1)
#define IMAGE_INPUT_IMAGE ((U8)2)

// An image knows which operations and parameter choices it can supply at this
// observation. Offers lets a client build controls from that knowledge without
// recognizing an implementation or predicting its computational cost. Names
// are presentation labels. The UUID remains the behavioral contract.
//
// A kernel offer describes square choices from minimum through maximum with
// the given step. Admission also accepts rectangular requests and remains the
// authority for an exact request. Providers enforce that same policy inside
// execution, so a client that never enumerates offers receives the same
// refusal. Visits keep each label readable until the callback returns.
// Admission diagnostics survive until the next observation on this image.
// Neither observation runs the image operation or produces an output image.
typedef struct image_offer {
  perimortem_uuid contract;
  perimortem_view_bytes name;
  U8 input;
  U32 minimum;
  U32 maximum;
  U32 step;
} image_offer;

typedef struct image_offer_visitor {
  void* callback_context;
  void (*callback)(void* callback_context, image_offer offer);
} image_offer_visitor;

typedef struct image_admission {
  ttx_negotiation_status status;
  perimortem_view_bytes reason;
} image_admission;

typedef struct image_offers_ops {
  ttx_abstract (*get_abstract)(void* context);
  void (*visit)(void* context, image_offer_visitor visitor);
  image_admission (
      *admit)(void* context, perimortem_uuid contract, U32 width, U32 height);
} image_offers_ops;

typedef struct image_offers {
  void* context;
  const image_offers_ops* operations;
} image_offers;

#endif
