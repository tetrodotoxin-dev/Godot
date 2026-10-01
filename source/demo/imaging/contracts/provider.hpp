// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "demo/imaging/contracts/image.hpp"
#include "demo/imaging/contracts/provider.h"

// Provider results use the actual C record layouts. Their function signatures
// therefore participate in the same canonical comparison as image operations.

TTX_DATA_RECORD(
    image_provider_statistics,
    TTX_DATA_MEMBER(image_provider_statistics, uploads),
    TTX_DATA_MEMBER(image_provider_statistics, downloads),
    TTX_DATA_MEMBER(image_provider_statistics, live_images),
    TTX_DATA_MEMBER(image_provider_statistics, plan_builds));

TTX_DATA_RECORD(
    image_provider_operations,
    TTX_DATA_MEMBER(image_provider_operations, release),
    TTX_DATA_MEMBER(image_provider_operations, statistics),
    TTX_DATA_MEMBER(image_provider_operations, create));

TTX_DATA_RECORD(
    image_provider,
    TTX_DATA_MEMBER(image_provider, source),
    TTX_DATA_MEMBER(image_provider, operations));
