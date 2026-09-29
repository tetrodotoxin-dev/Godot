// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include <stdlib.h>
#include <string.h>

#include "demo/counter/runtime.h"
#include "extension/contracts/scalar.h"
#include "ttx/concept/capabilities/borrow.h"
#include "ttx/concept/capabilities/callable.h"
#include "ttx/concept/capabilities/create.h"
#include "ttx/concept/policies/borrowed.h"
#include "ttx/concept/policies/none.h"
#include "ttx/semantic/negotiation/library.h"
#include "ttx/semantic/realization/invocation.h"

// The namespace lends class and method descriptions during discovery. Borrow
// copies this small graph and keeps the selected node at the same position,
// preserving its routes and construction capability after the call ends.
typedef struct counter_declaration {
  U32 index;
  struct counter_graph* owner;
} counter_declaration;

typedef struct counter_graph {
  U64 references;
  counter_declaration nodes[6];
} counter_graph;

static void initialize(counter_graph* graph, U64 references) {
  graph->references = references;
  for (U32 i = 0; i != 6; ++i) {
    graph->nodes[i] = (counter_declaration){i, graph};
  }
}

static const char* names[] = {"Counter module",  "Counter",
                              "advance",         "get_entries",
                              "get_ready_count", "get_live_instances"};

static perimortem_view_bytes bytes(const char* value) {
  return (perimortem_view_bytes){(const U8*)value, strlen(value)};
}

static ttx_binding_status describe(
    const void* source,
    ttx_callable_description* output) {
  const counter_declaration* node = source;
  static const lab_scalar amount = {
    {(const U8*)"amount", 6}, LAB_SCALAR_INTEGER};
  static const lab_scalar result = {
    {(const U8*)"result", 6}, LAB_SCALAR_INTEGER};
  static ttx_callable_field argument;
  static ttx_callable_field returned;
  argument = (ttx_callable_field){lab_scalar_abstract(&amount), 0};
  returned = (ttx_callable_field){lab_scalar_abstract(&result), 0};
  *output = (ttx_callable_description){
    {COUNTER_METHOD_HIGH, COUNTER_METHOD_LOW + node->index - 2},
    ttx_invocation_representation(),
    {lab_scalar_representation(
         node->index == 2 ? LAB_SCALAR_INTEGER : LAB_SCALAR_EMPTY),
     &argument, node->index == 2 ? 1 : 0},
    {lab_scalar_representation(LAB_SCALAR_INTEGER), &returned, 1},
  };
  return TTX_BINDING_SATISFIED;
}

static const ttx_abstract_ops abstract_operations;
static const ttx_borrow_ops borrow_operations;
static const ttx_create_ops create_operations;
static const ttx_callable_operations callable_operations;
static const ttx_borrowed_ops borrowed_operations;

static ttx_abstract abstract(const counter_declaration* node) {
  return (ttx_abstract){node, &abstract_operations};
}

static void release(const void* source) {
  counter_graph* graph = ((const counter_declaration*)source)->owner;
  if (!--graph->references) {
    free(graph);
  }
}

static ttx_binding_status borrow(const void* source, ttx_borrowed* output) {
  const counter_declaration* node = source;
  counter_graph* graph = node->owner;
  if (graph->references) {
    ++graph->references;
  } else {
    graph = malloc(sizeof(*graph));
    if (!graph) {
      return TTX_BINDING_REJECTED;
    }
    initialize(graph, 1);
  }

  *output = (ttx_borrowed){&graph->nodes[node->index], &borrowed_operations};
  return TTX_BINDING_SATISFIED;
}

static ttx_binding_status
    bind_node(const void* source, perimortem_uuid id, ttx_storage requested) {
  const counter_declaration* node = source;
  if (id.high == TTX_ABSTRACT_ID_HIGH && id.low == TTX_ABSTRACT_ID_LOW) {
    const ttx_abstract api = {source, &abstract_operations};
    return ttx_binding_provide(ttx_abstract_representation(), &api, requested);
  }

  if (id.high == TTX_BORROW_ID_HIGH && id.low == TTX_BORROW_ID_LOW) {
    const ttx_borrow api = {source, &borrow_operations};
    return ttx_binding_provide(ttx_borrow_representation(), &api, requested);
  }
  if (node->owner->references && id.high == TTX_BORROWED_ID_HIGH &&
      id.low == TTX_BORROWED_ID_LOW) {
    const ttx_borrowed api = {source, &borrowed_operations};
    return ttx_binding_provide(ttx_borrowed_representation(), &api, requested);
  }
  if (node->index == 1 && id.high == TTX_CREATE_ID_HIGH &&
      id.low == TTX_CREATE_ID_LOW) {
    const ttx_create api = {source, &create_operations};
    return ttx_binding_provide(ttx_create_representation(), &api, requested);
  }

  if (node->index >= 2 && id.high == TTX_CALLABLE_ID_HIGH &&
      id.low == TTX_CALLABLE_ID_LOW) {
    const ttx_callable api = {source, &callable_operations};
    return ttx_binding_provide(ttx_callable_representation(), &api, requested);
  }

  return TTX_BINDING_UNKNOWN;
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

static ttx_binding_status supports_node(
    const void* source,
    perimortem_uuid id) {
  const counter_declaration* node = source;
  if (id.high == TTX_ABSTRACT_ID_HIGH && id.low == TTX_ABSTRACT_ID_LOW) {
    return TTX_BINDING_SATISFIED;
  }

  if ((id.high == TTX_BORROW_ID_HIGH && id.low == TTX_BORROW_ID_LOW) ||
      (node->owner->references && id.high == TTX_BORROWED_ID_HIGH &&
       id.low == TTX_BORROWED_ID_LOW) ||
      (node->index == 1 && id.high == TTX_CREATE_ID_HIGH &&
       id.low == TTX_CREATE_ID_LOW)) {
    return TTX_BINDING_SATISFIED;
  }

  return node->index >= 2 && id.high == TTX_CALLABLE_ID_HIGH &&
                 id.low == TTX_CALLABLE_ID_LOW
             ? TTX_BINDING_SATISFIED
             : TTX_BINDING_UNKNOWN;
}

#define COUNTER_GRAPH_OPS \
  supports_node, bind_node, get_data, resolve, lookup, visit
static const ttx_abstract_ops abstract_operations = {COUNTER_GRAPH_OPS};
static const ttx_borrow_ops borrow_operations = {{COUNTER_GRAPH_OPS}, borrow};
static const ttx_borrowed_ops borrowed_operations = {
  {COUNTER_GRAPH_OPS},
  release};

// The entry lends these descriptions only during the receiver callback.
// Borrowing copies this namespace so its selected node keeps the same routes.
PERIMORTEM_C __attribute__((visibility("default"))) ttx_binding_status
    ttx_query(ttx_semantic_query host, ttx_query_receiver receive) {
  (void)host;
  counter_graph graph;
  initialize(&graph, 0);
  const ttx_semantic_query query = {graph.nodes, bind_node, supports_node};
  return receive.receive(receive.source, query);
}

static const ttx_create_ops create_operations = {
  {COUNTER_GRAPH_OPS},
  counter_create};
static const ttx_callable_operations callable_operations = {
  {COUNTER_GRAPH_OPS},
  describe};
