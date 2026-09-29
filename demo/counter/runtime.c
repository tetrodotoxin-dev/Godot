// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/counter/runtime.h"

#include <stdlib.h>

#include "demo/counter/counter.h"
#include "extension/contracts/lifecycle.h"
#include "extension/contracts/scalar.h"
#include "ttx/concept/capabilities/borrow.h"
#include "ttx/concept/capabilities/create.h"
#include "ttx/concept/policies/borrowed.h"
#include "ttx/concept/policies/none.h"
#include "ttx/semantic/realization/invocation.h"

static U64 live_instances;

typedef struct counter_instance {
  counter value;
  U64 borrows;
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

static ttx_data_status
    invoke_advance(const void* source, const void* input, void* output) {
  *(S64*)output = advance(source, *(const S64*)input);
  return TTX_DATA_SUCCESS;
}
static ttx_data_status
    invoke_entries(const void* source, const void* input, void* output) {
  (void)input;
  *(S64*)output = entries(source);
  return TTX_DATA_SUCCESS;
}
static ttx_data_status
    invoke_ready(const void* source, const void* input, void* output) {
  (void)input;
  *(S64*)output = ready_count(source);
  return TTX_DATA_SUCCESS;
}
static ttx_data_status
    invoke_live(const void* source, const void* input, void* output) {
  (void)input;
  *(S64*)output = live(source);
  return TTX_DATA_SUCCESS;
}

static const ttx_abstract_ops instance_operations;
static const ttx_borrow_ops borrow_operations;
static const ttx_borrowed_ops borrowed_operations;
static ttx_abstract instance_query(const counter_instance* source) {
  return (ttx_abstract){source, &instance_operations};
}

static void release_instance(const void* source) {
  counter_instance* instance = (counter_instance*)source;
  if (--instance->borrows == 0) {
    --live_instances;
    free(instance);
  }
}

static ttx_binding_status borrow_instance(
    const void* source,
    ttx_borrowed* output) {
  ++((counter_instance*)source)->borrows;
  *output = (ttx_borrowed){source, &borrowed_operations};
  return TTX_BINDING_SATISFIED;
}

static ttx_binding_status bind_instance(
    const void* source,
    perimortem_uuid id,
    ttx_storage requested) {
  if (id.high == TTX_ABSTRACT_ID_HIGH && id.low == TTX_ABSTRACT_ID_LOW) {
    const ttx_abstract api = instance_query(source);
    return ttx_binding_provide(ttx_abstract_representation(), &api, requested);
  }
  if (id.high == TTX_BORROW_ID_HIGH && id.low == TTX_BORROW_ID_LOW) {
    const ttx_borrow api = {source, &borrow_operations};
    return ttx_binding_provide(ttx_borrow_representation(), &api, requested);
  }
  if (id.high == TTX_BORROWED_ID_HIGH && id.low == TTX_BORROWED_ID_LOW) {
    const ttx_borrowed api = {source, &borrowed_operations};
    return ttx_binding_provide(ttx_borrowed_representation(), &api, requested);
  }
  if (id.high == COUNTER_METHOD_HIGH && id.low >= COUNTER_METHOD_LOW &&
      id.low < COUNTER_METHOD_LOW + 4) {
    const counter_instance* instance = source;
    return ttx_binding_provide(
        ttx_invocation_representation(),
        &instance->calls[id.low - COUNTER_METHOD_LOW], requested);
  }

  if (id.high == GODOT_LIFECYCLE_ID_HIGH && id.low == GODOT_LIFECYCLE_ID_LOW) {
    static const godot_lifecycle_operations operations = {entered, ready};
    const godot_lifecycle api = {source, &operations};
    return ttx_binding_provide(
        godot_lifecycle_representation(), &api, requested);
  }

  return TTX_BINDING_UNKNOWN;
}

static ttx_binding_status supports_instance(
    const void* source,
    perimortem_uuid id) {
  (void)source;
  return (id.high == TTX_ABSTRACT_ID_HIGH && id.low == TTX_ABSTRACT_ID_LOW) ||
                 (id.high == TTX_BORROW_ID_HIGH &&
                  id.low == TTX_BORROW_ID_LOW) ||
                 (id.high == TTX_BORROWED_ID_HIGH &&
                  id.low == TTX_BORROWED_ID_LOW) ||
                 (id.high == COUNTER_METHOD_HIGH &&
                  id.low >= COUNTER_METHOD_LOW &&
                  id.low < COUNTER_METHOD_LOW + 4) ||
                 (id.high == GODOT_LIFECYCLE_ID_HIGH &&
                  id.low == GODOT_LIFECYCLE_ID_LOW)
             ? TTX_BINDING_SATISFIED
             : TTX_BINDING_UNKNOWN;
}

static perimortem_view_bytes data(const void* source) {
  return (perimortem_view_bytes){
    (const U8*)&((const counter_instance*)source)->value, sizeof(counter)};
}
static ttx_abstract resolve(const void* source) {
  return instance_query(source);
}
static ttx_abstract lookup(const void* source, perimortem_view_bytes route) {
  (void)source;
  (void)route;
  return ttx_none();
}
static void visit(const void* source, ttx_concept_visitor receiver) {
  (void)source;
  (void)receiver;
}
#define COUNTER_INSTANCE_OPS \
  supports_instance, bind_instance, data, resolve, lookup, visit
static const ttx_abstract_ops instance_operations = {COUNTER_INSTANCE_OPS};
static const ttx_borrow_ops borrow_operations = {
  {COUNTER_INSTANCE_OPS},
  borrow_instance};
static const ttx_borrowed_ops borrowed_operations = {
  {COUNTER_INSTANCE_OPS},
  release_instance};

ttx_binding_status counter_create(
    const void* source,
    ttx_abstract arguments,
    void* receiver,
    void (*receive)(void*, ttx_abstract)) {
  (void)source;
  (void)arguments;
  counter_instance* value = calloc(1, sizeof(*value));
  if (!value) {
    return TTX_BINDING_REJECTED;
  }

  value->calls[0] = (ttx_invocation){
    value, lab_scalar_representation(LAB_SCALAR_INTEGER),
    lab_scalar_representation(LAB_SCALAR_INTEGER), invoke_advance};
  value->calls[1] = (ttx_invocation){
    value, lab_scalar_representation(LAB_SCALAR_EMPTY),
    lab_scalar_representation(LAB_SCALAR_INTEGER), invoke_entries};
  value->calls[2] = (ttx_invocation){
    value, lab_scalar_representation(LAB_SCALAR_EMPTY),
    lab_scalar_representation(LAB_SCALAR_INTEGER), invoke_ready};
  value->calls[3] = (ttx_invocation){
    value, lab_scalar_representation(LAB_SCALAR_EMPTY),
    lab_scalar_representation(LAB_SCALAR_INTEGER), invoke_live};
  value->borrows = 1;
  ++live_instances;
  receive(receiver, instance_query(value));
  release_instance(value);
  return TTX_BINDING_SATISFIED;
}
