// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef COUNTER_RUNTIME_H
#define COUNTER_RUNTIME_H

#include "extensions/counter/inspection.h"
#include "ttx/semantic/ownership/publication.h"
#include "ttx/data/status.h"

// Runtime publication owns counters and scene observations. A factory contains
// no graph pointer, so destroying discovery cannot affect later construction.
extern counter_statistics counter_metrics;
extern const counter_inspection_operations counter_inspection;

ttx_data_status counter_emit_factory(ttx_publication* output);

#endif
