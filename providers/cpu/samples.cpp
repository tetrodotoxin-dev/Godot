// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "providers/cpu/samples.h"

#include "sampling/publication.hpp"

using namespace Godot;
using namespace Perimortem;

PERIMORTEM_C __attribute__((visibility("default"))) ttx_data_status
    godot_sample_provider_open_v1(sample_provider* output) {
  static const sample_operations operations = {godot_cpu_samples};
  static const Sampling::Publication publication(
      nullptr, operations, [](const void*) {});
  *output = publication.get_provider();
  return TTX_DATA_SUCCESS;
}
