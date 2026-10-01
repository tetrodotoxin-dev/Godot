// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/option.hpp"

#include "demo/imaging/contracts/render.h"
#include "demo/imaging/render/runtime.hpp"
#include "demo/sampling/contracts/samples.hpp"
#include "godot_ttx/contracts/scalar.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/capabilities/borrow.hpp"
#include "ttx/concept/capabilities/callable.hpp"
#include "ttx/concept/capabilities/create.hpp"
#include "ttx/concept/policies/borrowed.hpp"
#include "ttx/concept/policies/none.h"
#include "ttx/concept/policies/unknown.hpp"
#include "ttx/semantic/negotiation/library.hpp"
#include "ttx/semantic/realization/invocation.h"

using namespace Perimortem;
using namespace Godot::Demo;

// Pixels and text share a byte view carrier. The Buffer role tells the
// consumer to interpret these bytes as image storage.
class BufferField {
 public:
  auto get_data() const -> Core::View::Bytes { return "pixels"_view; }
  auto supports(System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    using Ttx::Semantic::Negotiation::Binding::Status;
    return id == System::Uuid(GODOT_BUFFER_ID_HIGH, GODOT_BUFFER_ID_LOW)
               ? Status::Satisfied
               : Status::Unknown;
  }
  auto bind_interface(System::Uuid id, Ttx::Data::Form::Storage requested) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    if (id == System::Uuid(GODOT_BUFFER_ID_HIGH, GODOT_BUFFER_ID_LOW)) {
      return Ttx::Semantic::Negotiation::Binding::marker(requested);
    }
    return Ttx::Semantic::Negotiation::Binding::Status::Unknown;
  }
};

static auto description(U32 index) -> ttx_callable_description {
  using ::Godot::Extension::Contracts::Scalar;
  static const Scalar width("width"_view, Scalar::Kind::Integer);
  static const Scalar height("height"_view, Scalar::Kind::Integer);
  static const Scalar success("success"_view, Scalar::Kind::Boolean);
  static const Scalar error("error"_view, Scalar::Kind::Text);
  static const BufferField pixels;
  static const ttx_callable_field upload[] = {
    {width.get_abstract().get_abi(), __builtin_offsetof(render_upload, width)},
    {height.get_abstract().get_abi(),
     __builtin_offsetof(render_upload, height)},
    {Ttx::Concept::Abstract::provide(pixels).get_abi(),
     __builtin_offsetof(render_upload, pixels)},
  };
  static const ttx_callable_field returned[] = {
    {success.get_abstract().get_abi(), 0},
    {success.get_abstract().get_abi(), 0},
    {Ttx::Concept::Abstract::provide(pixels).get_abi(), 0},
    {width.get_abstract().get_abi(), 0},
    {height.get_abstract().get_abi(), 0},
    {error.get_abstract().get_abi(), 0},
  };
  return ttx_callable_description(
      {LAB_RENDER_METHOD_HIGH, LAB_RENDER_METHOD_LOW + index},
      ttx_invocation_representation(),
      {&Imaging::Render::Runtime::input(index), upload, index == 0 ? 3U : 0U},
      {&Imaging::Render::Runtime::output(index), &returned[index], 1});
}

class Discovery;

// Every retained node keeps its position in the same exported graph. Runtime
// state is shared while the original discovery storage can be released.
class Export {
 public:
  Discovery* owner;
  U32 index;
  auto get_data() const -> Core::View::Bytes;
  auto supports(System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status;
  auto bind_interface(System::Uuid id, Ttx::Data::Form::Storage target) const
      -> Ttx::Semantic::Negotiation::Binding::Status;
  auto borrow() const -> Utility::Result<
      Ttx::Concept::Policies::Borrowed,
      Ttx::Semantic::Negotiation::Binding::Failure>;
  auto release() const -> void;
  auto resolve_concept(Core::View::Bytes name) const -> Ttx::Concept::Abstract;
  auto visit_concepts(Ttx::Concept::Abstract::Visitor visitor) const -> void;
};

class Discovery {
 public:
  explicit Discovery(Imaging::Render::Runtime& runtime) : runtime(runtime) {
    for (U32 i = 0; i != 8; ++i) {
      exports[i] = Export(this, i);
    }
  }
  ~Discovery() { runtime.release(); }
  auto get_data() const -> Core::View::Bytes { return "Images"_view; }
  auto supports(System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    using namespace Ttx::Concept;
    using namespace Ttx::Semantic::Negotiation;
    return id == Capabilities::Borrow::contract_id ||
                   (references && id == Policies::Borrowed::contract_id)
               ? Binding::Status::Satisfied
               : Binding::Status::Unknown;
  }
  auto bind_interface(System::Uuid id, Ttx::Data::Form::Storage target) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    using namespace Ttx::Concept;
    using namespace Ttx::Semantic::Negotiation;
    if (id == Capabilities::Borrow::contract_id) {
      return Binding::provide<Capabilities::Borrow>(
          Capabilities::Borrow::provide(*this).get_abi(), target);
    }
    if (references && id == Policies::Borrowed::contract_id) {
      return Binding::provide<Policies::Borrowed>(
          Policies::Borrowed::provide(*this).get_abi(), target);
    }
    return Binding::Status::Unknown;
  }
  auto retain() const -> const Discovery& {
    if (references) {
      ++references;
      return *this;
    }
    runtime.retain();
    auto* copy = new Discovery(runtime);
    copy->references = 1;
    return *copy;
  }
  auto borrow() const -> Utility::Result<
      Ttx::Concept::Policies::Borrowed,
      Ttx::Semantic::Negotiation::Binding::Failure> {
    return Ttx::Concept::Policies::Borrowed::provide(retain());
  }
  auto release() const -> void {
    if (!--references) {
      delete this;
    }
  }
  auto compiler() const -> Ttx::Concept::Abstract {
    return runtime.get_compiler().bind<Ttx::Concept::Abstract>().visit(
        [](Ttx::Concept::Abstract subject) { return subject; },
        [](Ttx::Semantic::Negotiation::Binding::Failure failure)
            -> Ttx::Concept::Abstract {
          return failure ==
                         Ttx::Semantic::Negotiation::Binding::Failure::Unknown
                     ? Ttx::Concept::Abstract(ttx_unknown())
                     : Ttx::Concept::Abstract(ttx_none());
        });
  }
  auto resolve_concept(Core::View::Bytes name) const -> Ttx::Concept::Abstract {
    if (name == "Imaging"_view) {
      return Ttx::Concept::Abstract::provide(exports[0]);
    }
    if (name == "Compiler"_view && runtime.get_compiler().is_set()) {
      return compiler();
    }
    return Ttx::Concept::Abstract(ttx_none());
  }
  auto visit_concepts(Ttx::Concept::Abstract::Visitor visitor) const -> void {
    visitor("Imaging"_view, Ttx::Concept::Abstract::provide(exports[0]));
    if (runtime.get_compiler().is_set()) {
      visitor("Compiler"_view, compiler());
    }
  }
  Imaging::Render::Runtime& runtime;
  Export exports[8];
  Count references = 0;
};

auto Export::get_data() const -> Core::View::Bytes {
  const Core::View::Bytes names[] = {
    "Imaging"_view, "Render"_view,    "upload"_view,     "invert"_view,
    "pixels"_view,  "get_width"_view, "get_height"_view, "get_error"_view};
  return names[index];
}
auto Export::supports(System::Uuid id) const
    -> Ttx::Semantic::Negotiation::Binding::Status {
  using namespace Ttx::Concept;
  using namespace Ttx::Semantic::Negotiation;
  if (id == Capabilities::Borrow::contract_id ||
      (owner->references && id == Policies::Borrowed::contract_id) ||
      (index == 1 && id == Capabilities::Create::contract_id) ||
      (index >= 2 && id == Capabilities::Callable::contract_id)) {
    return Binding::Status::Satisfied;
  }
  return index == 1 ? owner->runtime.get_query().supports(id)
                    : Binding::Status::Unknown;
}
auto Export::bind_interface(System::Uuid id, Ttx::Data::Form::Storage target)
    const -> Ttx::Semantic::Negotiation::Binding::Status {
  using namespace Ttx::Concept;
  using namespace Ttx::Semantic::Negotiation;
  if (id == Capabilities::Borrow::contract_id) {
    return Binding::provide<Capabilities::Borrow>(
        Capabilities::Borrow::provide(*this).get_abi(), target);
  }
  if (owner->references && id == Policies::Borrowed::contract_id) {
    return Binding::provide<Policies::Borrowed>(
        Policies::Borrowed::provide(*this).get_abi(), target);
  }
  if (index == 1 && id == Capabilities::Create::contract_id) {
    static const ttx_create_ops operations = ttx_create_ops(
        *Abstract::provide(*this).get_abi().operations,
        [](const void* source, ttx_abstract, void* receiver,
           void (*receive)(void*, ttx_abstract)) {
          return static_cast<ttx_binding_status>(
              static_cast<const Export*>(source)
                  ->owner->runtime.create_instance(receiver, receive));
        });
    return Binding::provide<Capabilities::Create>(
        ttx_create(this, &operations), target);
  }
  if (index >= 2 && id == Capabilities::Callable::contract_id) {
    static const ttx_callable_operations operations = ttx_callable_operations(
        *Abstract::provide(*this).get_abi().operations,
        [](const void* source,
           ttx_callable_description* output) -> ttx_binding_status {
          *output = description(static_cast<const Export*>(source)->index - 2);
          return TTX_BINDING_SATISFIED;
        });
    return Binding::provide<Capabilities::Callable>(
        {this, &operations}, target);
  }
  return index == 1 ? owner->runtime.get_query().bind(id, target)
                    : Binding::Status::Unknown;
}
auto Export::borrow() const -> Utility::Result<
    Ttx::Concept::Policies::Borrowed,
    Ttx::Semantic::Negotiation::Binding::Failure> {
  return Ttx::Concept::Policies::Borrowed::provide(
      owner->retain().exports[index]);
}
auto Export::release() const -> void {
  owner->release();
}
auto Export::visit_concepts(Ttx::Concept::Abstract::Visitor visitor) const
    -> void {
  const U32 first = index == 0 ? 1 : 2;
  const U32 end = index == 0 ? 2 : index == 1 ? 8 : first;
  for (U32 i = first; i != end; ++i) {
    visitor(
        owner->exports[i].get_data(),
        Ttx::Concept::Abstract::provide(owner->exports[i]));
  }
}
auto Export::resolve_concept(Core::View::Bytes name) const
    -> Ttx::Concept::Abstract {
  auto result = Ttx::Concept::Abstract(ttx_none());
  auto find = [&](Core::View::Bytes observed, Ttx::Concept::Abstract subject) {
    if (observed == name) {
      result = subject;
    }
  };
  visit_concepts(Ttx::Concept::Abstract::Visitor(find));
  return result;
}

auto Imaging::Render::Runtime::visit(
    OpenImages images,
    OpenSamples samples,
    ttx_query_receiver receive,
    Ttx::Semantic::Negotiation::Query capabilities) -> ttx_binding_status {
  const Discovery graph(Runtime::create(images, samples, capabilities));
  return receive.receive(
      receive.source, Ttx::Concept::Abstract::provide(graph).get_query());
}
