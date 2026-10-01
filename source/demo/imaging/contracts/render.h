// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef GODOT_CONTRACTS_RENDER_H
#define GODOT_CONTRACTS_RENDER_H

#include "perimortem/core/view/bytes.h"

#include "demo/imaging/contracts/provider.h"

#define LAB_RENDER_PROVIDER_ID_HIGH 0x6c99a360ed434d6fULL
#define LAB_RENDER_PROVIDER_ID_LOW 0xb7879c9c06d9e583ULL
#define LAB_RENDER_METHOD_HIGH 0x6c99a360ed434d6fULL
#define LAB_RENDER_METHOD_LOW 0xb7879c9c06d9e600ULL

// Render supplies the lab's persistent image operations. An image expression
// graph can bind it directly, while a Node exporter can separately use the
// same subject's Create capability. Opening transfers an independent backend
// reference. The caller retains supplying code through its final release.
typedef struct render_provider_operations {
  image_error (*open)(const void* source, image_provider* output);
} render_provider_operations;

// Upload borrows RGBA8 bytes for one synchronous observation. The declaration
// associates each field with a value policy rather than requiring Godot types.
typedef struct render_upload {
  S64 width;
  S64 height;
  perimortem_view_bytes pixels;
} render_upload;

typedef struct render_api {
  const void* source;
  const render_provider_operations* operations;
} render_api;

#endif
