// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "toolchain/memory/lifetime.hpp"

#include "perimortem/core/view/null_terminated.hpp"
#include "perimortem/memory/allocator/global.hpp"

#include "ttx/data/transfer.h"
#include "ttx/graph/capabilities/callable.hpp"
#include "ttx/graph/capabilities/create.hpp"
#include "ttx/graph/capabilities/describe.hpp"
#include "ttx/graph/capabilities/retain.hpp"
#include "ttx/graph/capabilities/type.hpp"
#include "ttx/graph/policies/none.hpp"
#include "ttx/graph/types.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Data;
using namespace Ttx::Graph;

class Counter;

// Declarations carry their own graph context. A bound invocation separately
// receives the selected Counter. The factory can describe the same methods
// before any application instance exists.
struct Declaration {
  Counter* counter;
  U8 kind;
};

static Declaration root{nullptr, 0}, factory{nullptr, 1}, advance{nullptr, 3},
    value{nullptr, 4}, amount{nullptr, 5};
static auto graph(void* context) -> ttx_abstract;
static auto name(void* context, perimortem_view_bytes* output) -> U8;
static auto description(void* context, perimortem_view_bytes* output) -> U8;
static auto supports(void* context, perimortem_uuid id)
    -> ttx_negotiation_status;
static auto bind(void* context, perimortem_uuid id, void* output)
    -> ttx_negotiation_status;
static auto route(void* context, perimortem_view_bytes path) -> ttx_abstract;
static auto visit(void* context, ttx_graph_visitor callback) -> void;
static auto state(void* context) -> ttx_abstract_state;

class Counter {
 public:
  Declaration declarations[3]{{this, 2}, {this, 3}, {this, 4}};
  Count references = 1;
  S64 value = 0;
};

static auto release_counter(Counter& counter) -> ttx_release_status {
  if (--counter.references == 0) {
    counter.~Counter();
    Allocator::Global::release(Data::cast<U8>(&counter));
  }

  return TTX_RELEASE_SUCCESS;
}

static auto graph(void* context) -> ttx_abstract {
  static const ttx_abstract_ops operations{supports, bind, route, visit, state};
  return {context, &operations};
}

static auto retained_graph(void* context) -> ttx_abstract {
  return graph(&Data::cast<Counter>(context)->declarations[0]);
}

static auto release(void* context) -> ttx_release_status {
  return release_counter(*Data::cast<Counter>(context));
}

static auto release_factory(void* context) -> ttx_release_status {
  return TTX_RELEASE_SUCCESS;
}

static auto retain(void* context, ttx_retained* output)
    -> ttx_negotiation_status {
  auto& declaration = *Data::cast<Declaration>(context);
  static const ttx_retained_ops factory_operations{graph, release_factory};
  static const ttx_retained_ops instance_operations{retained_graph, release};
  if (!declaration.counter) {
    *output = {context, &factory_operations};
  } else {
    if (declaration.counter->references == Count(-1)) {
      return TTX_NEGOTIATION_REJECTED;
    }

    ++declaration.counter->references;
    *output = {declaration.counter, &instance_operations};
  }

  return TTX_NEGOTIATION_SATISFIED;
}

static auto create(
    void* context,
    ttx_abstract arguments,
    void* callback_context,
    void (*callback)(void*, ttx_abstract)) -> ttx_negotiation_status {
  if (Abstract(arguments).get_state() != Abstract::State::None) {
    return TTX_NEGOTIATION_REJECTED;
  }

  const auto allocation = Allocator::Global::allocate(sizeof(Counter));
  auto& counter =
      Toolchain::Memory::Lifetime::construct<Counter>(allocation.get_data());
  callback(callback_context, graph(&counter.declarations[0]));
  release_counter(counter);
  return TTX_NEGOTIATION_SATISFIED;
}

static auto representation(void* context, ttx_representation* output) -> U8 {
  return 0;
}

static auto fields(
    void* context,
    void* callback_context,
    void (*callback)(void*, ttx_abstract)) -> void {
  auto& declaration = *Data::cast<Declaration>(context);
  callback(
      callback_context,
      graph(
          declaration.counter ? &declaration.counter->declarations[2]
                              : &value));
}

static auto arguments(
    void* context,
    void* callback_context,
    void (*callback)(void*, ttx_abstract)) -> void {
  if (Data::cast<Declaration>(context)->kind == 3) {
    callback(callback_context, graph(&amount));
  }
}

static auto result(void* context) -> ttx_abstract {
  return Types::named("s64"_view).get_abi();
}

static auto increment(void* context, const void* input, void* output)
    -> ttx_data_status {
  auto& counter = *Data::cast<Counter>(context);
  counter.value = S64(U64(counter.value) + U64(*Data::cast<const S64>(input)));
  *Data::cast<S64>(output) = counter.value;
  return TTX_DATA_SUCCESS;
}

static auto read(void* context, const void* input, void* output)
    -> ttx_data_status {
  *Data::cast<S64>(output) = Data::cast<Counter>(context)->value;
  return TTX_DATA_SUCCESS;
}

static auto invocation(void* context, ttx_invocation* output)
    -> ttx_negotiation_status {
  auto& declaration = *Data::cast<Declaration>(context);
  if (!declaration.counter) {
    return TTX_NEGOTIATION_REJECTED;
  }

  static constexpr auto empty = Form::Schema::composite({}, 0);
  const auto& integer =
      Form::Compiled<Form::Native<S64>::reference>::get_representation();
  const auto& inputs = declaration.kind == 3
                           ? integer
                           : Form::Compiled<empty>::get_representation();
  *output = {
    declaration.counter, &inputs.get_abi(), &integer.get_abi(),
    declaration.kind == 3 ? increment : read};

  return TTX_NEGOTIATION_SATISFIED;
}

static auto supports(void* context, perimortem_uuid id)
    -> ttx_negotiation_status {
  const auto& declaration = *Data::cast<Declaration>(context);
  const Identity::Uuid contract(id);
  if (!id.high && id.low <= 1) {
    return TTX_NEGOTIATION_REJECTED;
  }

  if (contract == Abstract::contract_id ||
      contract == Capabilities::Describe::contract_id ||
      (declaration.kind <= 2 &&
       contract == Capabilities::Retain::contract_id) ||
      (declaration.kind == 1 &&
       contract == Capabilities::Create::contract_id) ||
      ((declaration.kind == 1 || declaration.kind == 2) &&
       contract == Capabilities::Type::contract_id) ||
      ((declaration.kind == 3 || declaration.kind == 4) &&
       contract == Capabilities::Callable::contract_id)) {
    return TTX_NEGOTIATION_SATISFIED;
  }

  return TTX_NEGOTIATION_UNKNOWN;
}

static auto bind(void* context, perimortem_uuid id, void* output)
    -> ttx_negotiation_status {
  const auto status = supports(context, id);
  if (status != TTX_NEGOTIATION_SATISFIED) {
    return status;
  }

  static const ttx_describe_ops descriptions{graph, name, description};
  static const ttx_retain_ops retention{graph, retain};
  static const ttx_create_ops creation{graph, create};
  static const ttx_callable_operations callables{
    graph, arguments, result, invocation};

  static const ttx_type_ops types{graph, representation, fields};
  const Identity::Uuid contract(id);
  if (contract == Capabilities::Describe::contract_id) {
    *Data::cast<ttx_describe>(output) = {context, &descriptions};
  } else if (contract == Capabilities::Retain::contract_id) {
    *Data::cast<ttx_retain>(output) = {context, &retention};
  } else if (contract == Capabilities::Create::contract_id) {
    *Data::cast<ttx_create>(output) = {context, &creation};
  } else if (contract == Capabilities::Callable::contract_id) {
    *Data::cast<ttx_callable>(output) = {context, &callables};
  } else if (contract == Capabilities::Type::contract_id) {
    *Data::cast<ttx_type>(output) = {context, &types};
  } else {
    *Data::cast<ttx_abstract>(output) = graph(context);
  }

  return status;
}

static auto name(void* context, perimortem_view_bytes* output) -> U8 {
  static constexpr View::Bytes names[]{"Counter module"_view, "TtxCounter"_view,
                                       "TtxCounter"_view,     "advance"_view,
                                       "get_value"_view,      "amount"_view};

  *output = names[Data::cast<Declaration>(context)->kind];
  return 1;
}

static auto description(void* context, perimortem_view_bytes* output) -> U8 {
  return 0;
}

static auto state(void* context) -> ttx_abstract_state {
  return TTX_ABSTRACT_PRESENT;
}

static auto visit(void* context, ttx_graph_visitor callback) -> void {
  auto& declaration = *Data::cast<Declaration>(context);
  if (declaration.kind == 0) {
    callback.callback(
        callback.callback_context, "counter"_view, graph(&factory));
  } else if (declaration.kind <= 2) {
    callback.callback(
        callback.callback_context, "advance"_view,
        graph(
            declaration.counter ? &declaration.counter->declarations[1]
                                : &advance));
    callback.callback(
        callback.callback_context, "value"_view,
        graph(
            declaration.counter ? &declaration.counter->declarations[2]
                                : &value));
  }
}

static auto route(void* context, perimortem_view_bytes path) -> ttx_abstract {
  if (View::Bytes(path) == "type"_view) {
    return Types::named("s64"_view).get_abi();
  }

  Abstract selected(ttx_none());
  auto callback = [&](View::Bytes name, Abstract value) {
    if (name == View::Bytes(path)) {
      selected = value;
    }
  };

  Abstract(graph(context)).visit_routes(Abstract::Visitor(callback));
  return selected.get_abi();
}

// The C entry uses static address initializers. Its exported name therefore
// remains independent when several provider modules publish ttx_entry.
C_LINKAGE HIDDEN constinit const ttx_representation
    counter_query_representation =
        Form::Compiled<
            Form::Native<ttx_semantic_query>::reference>::get_representation()
            .get_abi();

C_LINKAGE HIDDEN auto counter_factory() -> ttx_abstract {
  return graph(&root);
}
