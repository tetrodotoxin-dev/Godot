// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "toolchain/validation/test.hpp"

#include "perimortem/core/view/null_terminated.hpp"
#include "perimortem/system/process/library.hpp"

#include "ttx/data/transfer.h"
#include "ttx/graph/capabilities/callable.hpp"
#include "ttx/graph/capabilities/create.hpp"
#include "ttx/graph/capabilities/describe.hpp"
#include "ttx/graph/capabilities/retain.hpp"
#include "ttx/graph/capabilities/type.hpp"
#include "ttx/graph/policies/none.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Toolchain::Validation;
using namespace Ttx::Data;
using namespace Ttx::Graph;
using namespace Ttx::Semantic;

static Harness Counter = {.name = "Godot::Counter"};

// The provider has no Godot dependency. Its public module entry must establish
// the same represented Query and Abstract used by the engine bridge, then keep
// independently created instances usable after their creation callbacks end.
VALIDATION_TEST(Counter, independent_instances) {
  Allocator::Arena errors;
  Process::Library::open(
      ".bin/bin/source/demo/counter/libcounter_extension.so"_view, errors)
      .visit(
          [&](Process::Library& library) {
            library.symbol("ttx_entry"_view, errors)
                .visit(
                    [&](void* symbol) {
                      const auto& source =
                          *Data::cast<const ttx_storage>(symbol);
                      ttx_semantic_query query{};
                      const ttx_storage destination{
                        ttx_semantic_query_representation(),
                        Data::cast<U8>(&query), sizeof(query)};

                      ASSERT_EQ(
                          ttx_transfer(&source, &destination),
                          TTX_DATA_SUCCESS);
                      Negotiation::Query(query).bind<Abstract>().visit(
                          [&](Abstract root) {
                            const auto factory =
                                root.resolve_route("counter"_view);
                            EXPECT(factory.bind<Capabilities::Type>());
                            factory.bind<Capabilities::Describe>().visit(
                                [&](Capabilities::Describe describe) {
                                  describe.get_name().visit(
                                      [&](View::Bytes name) {
                                        EXPECT_EQ(name, "TtxCounter"_view);
                                      },

                                      [&] { EXPECT(False); });
                                },

                                [&] { EXPECT(False); });

                            Storage::Option<Policies::Retained> instances[2];
                            for (Count i = 0; i < 2; ++i) {
                              auto callback = [&](Abstract created) {
                                created.bind<Capabilities::Retain>().visit(
                                    [&](Capabilities::Retain retain) {
                                      retain.retain().visit(
                                          [&](Policies::Retained result) {
                                            instances[i] = result;
                                          },

                                          [&](Negotiation::Status status) {
                                            EXPECT(False);
                                          });
                                    },

                                    [&] { EXPECT(False); });
                              };

                              factory.bind<Capabilities::Create>().visit(
                                  [&](Capabilities::Create create) {
                                    EXPECT(
                                        create.create(
                                            Policies::None::get_none()
                                                .get_abstract(),
                                            callback) ==
                                        Negotiation::Status::Satisfied);
                                  },

                                  [&] { EXPECT(False); });
                            }

                            for (Count i = 0; i < 2; ++i) {
                              instances[i].visit(
                                  [&](Policies::Retained retained) {
                                    retained.get_abstract()
                                        .resolve_route("advance"_view)
                                        .bind<Capabilities::Callable>()
                                        .visit(
                                            [&](Capabilities::Callable
                                                    callable) {
                                              callable.get_invocation().visit(
                                                  [&](Realization::Invocation
                                                          invocation) {
                                                    const auto& integer = Form::
                                                        Compiled<Form::Native<
                                                            S64>::reference>::
                                                            get_representation();
                                                    ASSERT(invocation.accepts(
                                                        integer, integer));

                                                    const S64 input = 7 + i;
                                                    S64 output = 0;
                                                    EXPECT(
                                                        invocation.invoke(
                                                            &input, &output) ==
                                                        Status::Success);
                                                    EXPECT_EQ(output, input);
                                                  },

                                                  [&](Negotiation::Status
                                                          status) {
                                                    EXPECT(False);
                                                  });
                                            },

                                            [&] { EXPECT(False); });
                                    EXPECT(
                                        retained.release() ==
                                        Policies::Retained::Status::Success);
                                  },

                                  [&] { EXPECT(False); });
                            }
                          },

                          [&](Negotiation::Status status) { EXPECT(False); });
                    },

                    [&](View::Bytes error) {
                      EXPECT_EQ(error, View::Bytes());
                    });
          },

          [&](View::Bytes error) { EXPECT_EQ(error, View::Bytes()); });
}
