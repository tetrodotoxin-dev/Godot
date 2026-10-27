// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/adapters/cuda/library.hpp"

#include "toolchain/memory/lifetime.hpp"

#include "perimortem/core/view/null_terminated.hpp"
#include "perimortem/memory/allocator/global.hpp"

#include "ttx/data/transfer.h"
#include "ttx/graph/abstract.hpp"
#include "ttx/semantic/negotiation/library.h"

#include "godot_cpp/classes/project_settings.hpp"

using namespace Perimortem;
using namespace Godot::Demo::Adapters::Cuda;

Library::Library(System::Process::Library module, ttx_abstract abstract)
    : module(Core::Data::take(module)), abstract(abstract) {}
void Library::retain() {
  ++references;
}

void Library::release() {
  if (--references == 0) {
    this->~Library();
    Memory::Allocator::Global::release(Core::Data::cast<U8>(this));
  }
}

auto Library::open(const godot::String& path, godot::String& error)
    -> Library* {

#ifdef TOOLCHAIN_WASM

  error = "CUDA is unavailable in the Web platform.";
  return nullptr;

#else

  const auto location =
      path.begins_with("res://")
          ? godot::ProjectSettings::get_singleton()->globalize_path(path)
          : path;
  const auto encoded = location.utf8();
  const Core::View::Bytes bytes(
      perimortem_view_bytes{
        Core::Data::cast<const U8>(encoded.get_data()),
        Count(encoded.length())});

  Memory::Allocator::Arena errors;
  Library* result = nullptr;
  auto failed = [&](Core::View::Bytes message) {
    error = godot::String::utf8(
        Core::Data::cast<const char>(message.get_data()), message.get_size());
  };

  System::Process::Library::open(bytes, errors)
      .visit(
          [&](System::Process::Library& module) {
            module.symbol("ttx_entry"_view, errors)
                .visit(
                    [&](void* symbol) {
                      const auto& provided =
                          *Core::Data::cast<const ttx_storage>(symbol);
                      ttx_semantic_query query{};
                      const ttx_storage target{
                        ttx_semantic_query_representation(),
                        Core::Data::cast<U8>(&query), sizeof(query)};

                      if (ttx_transfer(&provided, &target) !=
                              TTX_DATA_SUCCESS ||
                          !query.bind || !query.supports) {
                        error = "CUDA module has an incompatible entry.";
                        return;
                      }

                      Ttx::Semantic::Negotiation::Query(query)
                          .bind<Ttx::Graph::Abstract>()
                          .visit(
                              [&](Ttx::Graph::Abstract abstract) {
                                auto allocation =
                                    Memory::Allocator::Global::allocate(
                                        sizeof(Library));
                                result =
                                    &Toolchain::Memory::Lifetime::construct<
                                        Library>(allocation.get_data(), [&] {
                                      return Library(
                                          Core::Data::take(module),
                                          abstract.get_abi());
                                    });
                              },

                              [&](Ttx::Semantic::Negotiation::Status status) {
                                error =
                                    "CUDA module refused Abstract bootstrap.";
                              });
                    },

                    failed);
          },

          failed);
  return result;

#endif
}
