// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef GODOT_PROVIDERS_CPU_SAMPLES_H
#define GODOT_PROVIDERS_CPU_SAMPLES_H

#include "demo/sampling/contracts/samples.h"

PERIMORTEM_C ttx_data_status godot_cpu_samples(const void*, U32, U32, U32, U64*);

#endif
