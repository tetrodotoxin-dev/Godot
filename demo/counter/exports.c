// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/modules/module.h"

#include <stdlib.h>
#include <string.h>

#include "demo/counter/runtime.h"
#include "ttx/concept/declarations/extensible.h"
#include "ttx/concept/answers/none.h"
#include "ttx/concept/declarations/callable.h"
#include "extension/contracts/scalar.h"

// The export namespace owns the class and method declarations. An emitted
// factory borrows none of them, so the consumer can release this namespace
// before constructing runtime instances.
typedef struct counter_declaration {
  U32 index;
} counter_declaration;

static const char* names[] = {
  "Counter module", "Counter", "advance",
  "get_entries", "get_ready_count", "get_live_instances"};

static perimortem_view_bytes bytes(const char* value) {
  return (perimortem_view_bytes){(const U8*)value, strlen(value)};
}

static ttx_binding_status describe(
    const void* source,
    ttx_callable_description* output) {
  const counter_declaration* node = source;
  static const lab_scalar amount = {{(const U8*)"amount", 6}, LAB_SCALAR_INTEGER};
  static const lab_scalar result = {{(const U8*)"result", 6}, LAB_SCALAR_INTEGER};
  static ttx_callable_field argument;
  static ttx_callable_field returned;
  argument = (ttx_callable_field){lab_scalar_abstract(&amount), 0};
  returned = (ttx_callable_field){lab_scalar_abstract(&result), 0};
  *output = (ttx_callable_description){
    {COUNTER_METHOD_HIGH, COUNTER_METHOD_LOW + node->index - 2},
    {lab_scalar_representation(node->index == 2 ? LAB_SCALAR_INTEGER : LAB_SCALAR_EMPTY),
     &argument, node->index == 2 ? 1 : 0},
    {lab_scalar_representation(LAB_SCALAR_INTEGER), &returned, 1},
  };
  return TTX_BINDING_SATISFIED;
}

static ttx_data_status emit(const void* source, ttx_publication* output) {
  (void)source;
  return counter_emit_factory(output);
}

static const ttx_abstract_ops abstract_operations;

static ttx_abstract abstract(const counter_declaration* node) {
  return (ttx_abstract){node, &abstract_operations};
}

static ttx_binding_status
    bind_node(const void* source, perimortem_uuid id, ttx_storage requested) {
  const counter_declaration* node = source;
  if (id.high == TTX_ABSTRACT_ID_HIGH && id.low == TTX_ABSTRACT_ID_LOW) {
    const ttx_abstract api = {source, &abstract_operations};
    return ttx_binding_provide(ttx_abstract_representation(), &api, requested);
  }

  if ((node->index == 1) &&
      id.high == TTX_EXTENSIBLE_ID_HIGH && id.low == TTX_EXTENSIBLE_ID_LOW) {
    static const ttx_extensible_operations operations = {emit};
    const ttx_extensible api = {source, &operations};
    return ttx_binding_provide(ttx_extensible_representation(), &api, requested);
  }

  if (node->index >= 2 && id.high == TTX_CALLABLE_ID_HIGH &&
      id.low == TTX_CALLABLE_ID_LOW) {
    static const ttx_callable_operations operations = {describe};
    const ttx_callable api = {source, &operations};
    return ttx_binding_provide(ttx_callable_representation(), &api, requested);
  }

  return TTX_BINDING_UNSUPPORTED;
}

static perimortem_view_bytes get_data(const void* source) {
  return bytes(names[((const counter_declaration*)source)->index]);
}

static ttx_abstract resolve(const void* source) {
  const counter_declaration* node = source;
  return abstract(node);
}

static ttx_abstract lookup(const void* source, perimortem_view_bytes name) {
  const counter_declaration* node = source;
  const counter_declaration* root = node - node->index;
  const U32 first = node->index == 0 ? 1 : 2;
  const U32 end = node->index == 0 ? 2 : (node->index == 1 ? 6 : 2);
  for (U32 index = first; index < end; ++index) {
    if (name.size == strlen(names[index]) &&
        memcmp(name.data, names[index], name.size) == 0) {
      return abstract(root + index);
    }
  }

  return ttx_none();
}

static void visit(const void* source, ttx_concept_visitor visitor) {
  const counter_declaration* node = source;
  const counter_declaration* root = node - node->index;
  const U32 first = node->index == 0 ? 1 : 2;
  const U32 end = node->index == 0 ? 2 : (node->index == 1 ? 6 : 2);
  for (U32 index = first; index < end; ++index) {
    visitor.receive(
        visitor.source, bytes(names[index]), abstract(root + index));
  }
}

static ttx_binding_status supports_node(const void* source, perimortem_uuid id) {
  const counter_declaration* node = source;
  if (id.high == TTX_ABSTRACT_ID_HIGH && id.low == TTX_ABSTRACT_ID_LOW) {
    return TTX_BINDING_SATISFIED;
  }

  if ((node->index == 1) && id.high == TTX_EXTENSIBLE_ID_HIGH && id.low == TTX_EXTENSIBLE_ID_LOW) {
    return TTX_BINDING_SATISFIED;
  }

  return node->index >= 2 && id.high == TTX_CALLABLE_ID_HIGH && id.low == TTX_CALLABLE_ID_LOW
             ? TTX_BINDING_SATISFIED : TTX_BINDING_UNSUPPORTED;
}

static const ttx_abstract_ops abstract_operations = {
  supports_node, bind_node, get_data, resolve, lookup, visit};

static void release_graph(const void* source) {
  free((void*)source);
}

PERIMORTEM_C __attribute__((visibility("default"))) ttx_data_status
    ttx_module_open(ttx_semantic_query host, ttx_module_acquisition* output) {
  (void)host;
  counter_declaration* graph = calloc(6, sizeof(*graph));
  if (!graph) {
    return TTX_DATA_IO_ERROR;
  }

  for (U32 index = 0; index < 6; ++index) {
    graph[index].index = index;
  }

  *output = (ttx_module_acquisition){abstract(graph), graph, release_graph};
  return TTX_DATA_SUCCESS;
}
