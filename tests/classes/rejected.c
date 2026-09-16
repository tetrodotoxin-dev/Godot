// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include <stdio.h>

#include "ttx/concept/modules/module.h"

// Acquisition itself can be declined. Any temporary initialization belongs to
// the provider on failure, so it must finish cleanup before its code unloads.
// The caller receives no invented Abstract or release obligation.
__attribute__((destructor)) static void unload(void) {
  puts("REJECTED module unloaded");
}

PERIMORTEM_C __attribute__((visibility("default"))) ttx_data_status
    ttx_module_open(ttx_semantic_query host, ttx_module_acquisition* output) {
  (void)host;
  (void)output;
  puts("REJECTED publication released");
  return TTX_DATA_DENIED;
}
