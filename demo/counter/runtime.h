// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef COUNTER_RUNTIME_H
#define COUNTER_RUNTIME_H

#include "demo/counter/counter.h"
#include "ttx/semantic/ownership/publication.h"
#include "ttx/data/status.h"

// Instances own their value and scene observations. The factory borrows no
// declarations, so it remains usable after the export namespace is released.

ttx_data_status counter_emit_factory(ttx_publication* output);

#endif
