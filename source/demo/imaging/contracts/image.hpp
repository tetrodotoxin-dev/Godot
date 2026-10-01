// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "demo/imaging/contracts/image.h"
#include "ttx/semantic/negotiation/query.hpp"

// Image's native declarations supply their actual callable descriptions.
// Output image ownership and error borrowing remain the Image contract's
// promises. The Data form establishes the concrete boundary used to call it.

TTX_DATA_RECORD(
    image_error,
    TTX_DATA_MEMBER(image_error, data),
    TTX_DATA_MEMBER(image_error, size));

TTX_DATA_RECORD(
    image_dimensions,
    TTX_DATA_MEMBER(image_dimensions, width),
    TTX_DATA_MEMBER(image_dimensions, height));

TTX_DATA_RECORD(
    image_operations,
    TTX_DATA_MEMBER(image_operations, retain),
    TTX_DATA_MEMBER(image_operations, release),
    TTX_DATA_MEMBER(image_operations, dimensions),
    TTX_DATA_MEMBER(image_operations, query),
    TTX_DATA_MEMBER(image_operations, pixels),
    TTX_DATA_MEMBER(image_operations, representation));

TTX_DATA_RECORD(
    image_object,
    TTX_DATA_MEMBER(image_object, source),
    TTX_DATA_MEMBER(image_object, operations));

TTX_DATA_RECORD(
    image_kernel,
    TTX_DATA_MEMBER(image_kernel, width),
    TTX_DATA_MEMBER(image_kernel, height),
    TTX_DATA_MEMBER(image_kernel, values),
    TTX_DATA_MEMBER(image_kernel, count));
