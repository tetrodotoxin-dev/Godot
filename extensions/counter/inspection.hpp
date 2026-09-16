// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "extensions/counter/inspection.h"
#include "ttx/semantic/negotiation/query.hpp"

TTX_DATA_RECORD(
    counter_statistics,
    TTX_DATA_MEMBER(counter_statistics, graphs_opened),
    TTX_DATA_MEMBER(counter_statistics, graphs_closed),
    TTX_DATA_MEMBER(counter_statistics, graph_queries),
    TTX_DATA_MEMBER(counter_statistics, factories_opened),
    TTX_DATA_MEMBER(counter_statistics, factories_closed),
    TTX_DATA_MEMBER(counter_statistics, instances_opened),
    TTX_DATA_MEMBER(counter_statistics, instances_closed),
    TTX_DATA_MEMBER(counter_statistics, binds),
    TTX_DATA_MEMBER(counter_statistics, calls));

TTX_DATA_RECORD(
    counter_inspection_operations,
    TTX_DATA_MEMBER(counter_inspection_operations, snapshot));

TTX_DATA_RECORD(
    counter_inspection_api,
    TTX_DATA_MEMBER(counter_inspection_api, source),
    TTX_DATA_MEMBER(counter_inspection_api, operations));

namespace Godot::Extensions::Counter {

// Inspection borrows the module's independent counters, so a test can observe
// graph release and retained runtime behavior through the same checked API.
class Inspection {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    COUNTER_INSPECTION_HIGH, COUNTER_INSPECTION_LOW};
  using Api = counter_inspection_api;
  using Operations = counter_inspection_operations;
  explicit constexpr Inspection(Api api) : api(api) {}
  constexpr auto get_abi() const -> Api { return api; }
  auto snapshot() const -> counter_statistics {
    return api.operations->snapshot(api.source);
  }

 private:
  Api api;
};

}  // namespace Godot::Extensions::Counter
