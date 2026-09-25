// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/counter/runtime.h"

#include <stdlib.h>

#include "extension/contracts/lifecycle.h"
#include "demo/counter/counter.h"
#include "ttx/semantic/ownership/factory.h"
#include "ttx/semantic/realization/invocation.h"
#include "extension/contracts/scalar.h"

static U64 live_instances;

typedef struct counter_instance {
  counter value;
  S64 entries;
  S64 ready;
  ttx_invocation calls[4];
} counter_instance;

static S64 advance(const void* source, S64 amount) {
  return counter_advance(&((counter_instance*)source)->value, amount);
}

static S64 entries(const void* source) {
  return ((const counter_instance*)source)->entries;
}

static S64 ready_count(const void* source) {
  return ((const counter_instance*)source)->ready;
}

static S64 live(const void* source) {
  (void)source;
  return (S64)live_instances;
}

static void entered(const void* source) {
  ++((counter_instance*)source)->entries;
}

static void ready(const void* source) {
  ++((counter_instance*)source)->ready;
}

static ttx_data_status invoke_advance(const void* source, const void* input, void* output) {
  *(S64*)output = advance(source, *(const S64*)input);
  return TTX_DATA_SUCCESS;
}
static ttx_data_status invoke_entries(const void* source, const void* input, void* output) {
  (void)input;
  *(S64*)output = entries(source);
  return TTX_DATA_SUCCESS;
}
static ttx_data_status invoke_ready(const void* source, const void* input, void* output) {
  (void)input;
  *(S64*)output = ready_count(source);
  return TTX_DATA_SUCCESS;
}
static ttx_data_status invoke_live(const void* source, const void* input, void* output) {
  (void)input;
  *(S64*)output = live(source);
  return TTX_DATA_SUCCESS;
}

static ttx_binding_status bind_instance(
    const void* source, perimortem_uuid id, ttx_storage requested) {
  if (id.high == COUNTER_METHOD_HIGH && id.low >= COUNTER_METHOD_LOW &&
      id.low < COUNTER_METHOD_LOW + 4) {
    const counter_instance* instance = source;
    return ttx_binding_provide(ttx_invocation_representation(),
        &instance->calls[id.low - COUNTER_METHOD_LOW], requested);
  }

  if (id.high == GODOT_LIFECYCLE_ID_HIGH && id.low == GODOT_LIFECYCLE_ID_LOW) {
    static const godot_lifecycle_operations operations = {entered, ready};
    const godot_lifecycle api = {source, &operations};
    return ttx_binding_provide(godot_lifecycle_representation(), &api, requested);
  }

  return TTX_BINDING_UNSUPPORTED;
}

static ttx_binding_status supports_instance(const void* source, perimortem_uuid id) {
  (void)source;
  return (id.high == COUNTER_METHOD_HIGH && id.low >= COUNTER_METHOD_LOW && id.low < COUNTER_METHOD_LOW + 4) ||
                 (id.high == GODOT_LIFECYCLE_ID_HIGH && id.low == GODOT_LIFECYCLE_ID_LOW)
             ? TTX_BINDING_SATISFIED : TTX_BINDING_UNSUPPORTED;
}

static void release_instance(const void* source) {
  --live_instances;
  free((void*)source);
}

static ttx_data_status create(const void* source, ttx_publication* output) {
  (void)source;
  counter_instance* value = calloc(1, sizeof(*value));
  if (!value) {
    return TTX_DATA_IO_ERROR;
  }

  value->calls[0] = (ttx_invocation){value, lab_scalar_representation(LAB_SCALAR_INTEGER), lab_scalar_representation(LAB_SCALAR_INTEGER), invoke_advance};
  value->calls[1] = (ttx_invocation){value, lab_scalar_representation(LAB_SCALAR_EMPTY), lab_scalar_representation(LAB_SCALAR_INTEGER), invoke_entries};
  value->calls[2] = (ttx_invocation){value, lab_scalar_representation(LAB_SCALAR_EMPTY), lab_scalar_representation(LAB_SCALAR_INTEGER), invoke_ready};
  value->calls[3] = (ttx_invocation){value, lab_scalar_representation(LAB_SCALAR_EMPTY), lab_scalar_representation(LAB_SCALAR_INTEGER), invoke_live};
  ++live_instances;
  *output = (ttx_publication){{value, bind_instance, supports_instance}, release_instance};
  return TTX_DATA_SUCCESS;
}

static ttx_binding_status
    bind_factory(const void* source, perimortem_uuid id, ttx_storage requested) {
  if (id.high == TTX_FACTORY_ID_HIGH && id.low == TTX_FACTORY_ID_LOW) {
    static const ttx_factory_operations operations = {create};
    const ttx_factory api = {source, &operations};
    return ttx_binding_provide(ttx_factory_representation(), &api, requested);
  }

  return TTX_BINDING_UNSUPPORTED;
}

static ttx_binding_status supports_factory(const void* source, perimortem_uuid id) {
  (void)source;
  return (id.high == TTX_FACTORY_ID_HIGH && id.low == TTX_FACTORY_ID_LOW)
             ? TTX_BINDING_SATISFIED : TTX_BINDING_UNSUPPORTED;
}

static void release_factory(const void* source) {
  (void)source;
}

ttx_data_status counter_emit_factory(ttx_publication* output) {
  *output = (ttx_publication){{NULL, bind_factory, supports_factory}, release_factory};
  return TTX_DATA_SUCCESS;
}
