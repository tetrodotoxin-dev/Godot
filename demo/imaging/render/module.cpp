// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/imaging/render/module.hpp"

#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/option.hpp"

#include "demo/imaging/render/runtime.hpp"
#include "demo/sampling/contracts/samples.hpp"
#include "extension/contracts/scalar.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/declarations/callable.hpp"
#include "ttx/concept/declarations/extensible.hpp"
#include "ttx/concept/modules/module.hpp"
#include "ttx/semantic/ownership/publication.hpp"

using namespace Perimortem;
using namespace Godot::Demo;

// Pixels and text share a byte view carrier, but they answer different
// questions. This field exposes the Buffer role without promising UTF8.
class BufferField {
 public:
  auto get_data() const -> Core::View::Bytes { return "pixels"_view; }
  auto supports(System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    using Ttx::Semantic::Negotiation::Binding::Status;
    return id == System::Uuid(LAB_BUFFER_ID_HIGH, LAB_BUFFER_ID_LOW)
               ? Status::Satisfied
               : Status::Unsupported;
  }
  auto bind_interface(System::Uuid id, Ttx::Data::Form::Storage requested) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    if (id == System::Uuid(LAB_BUFFER_ID_HIGH, LAB_BUFFER_ID_LOW)) {
      return Ttx::Semantic::Negotiation::Binding::marker(requested);
    }
    return Ttx::Semantic::Negotiation::Binding::Status::Unsupported;
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
  return {
    {LAB_RENDER_METHOD_HIGH, LAB_RENDER_METHOD_LOW + index},
    {&Imaging::Render::Runtime::input(index), upload, index == 0 ? 3U : 0U},
    {&Imaging::Render::Runtime::output(index), &returned[index], 1}};
}

// Nodes contain discovery relationships only. Emitting a factory copies the
// backend creation operation, never a pointer to this node or its containing
// array. Ordinary Abstract edges expose both the namespace and class members.
class Export {
 public:
  Export* root;
  U32 index;
  Imaging::Render::Module::OpenImages images;

  auto supports(System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    using Ttx::Semantic::Negotiation::Binding::Status;
    return (index == 1 &&
            id == Ttx::Concept::Declarations::Extensible::contract_id) ||
                   (index >= 2 &&
                    id == Ttx::Concept::Declarations::Callable::contract_id)
               ? Status::Satisfied
               : Status::Unsupported;
  }

  auto get_data() const -> Core::View::Bytes {
    const Core::View::Bytes names[] = {
      "Imaging"_view, "Render"_view,    "upload"_view,     "invert"_view,
      "pixels"_view,  "get_width"_view, "get_height"_view, "get_error"_view};
    return names[index];
  }

  auto bind_interface(System::Uuid id, Ttx::Data::Form::Storage requested) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    if (index == 1 &&
        id == Ttx::Concept::Declarations::Extensible::contract_id) {
      static const ttx_extensible_operations operations = {
        [](const void* source, ttx_publication* output) {
          return Imaging::Render::Runtime::emit(
              static_cast<const Export*>(source)->images, output);
        },
      };
      return Ttx::Semantic::Negotiation::Binding::provide<
          Ttx::Concept::Declarations::Extensible>(
          ttx_extensible(this, &operations), requested);
    }
    if (index >= 2 && id == Ttx::Concept::Declarations::Callable::contract_id) {
      static const ttx_callable_operations operations = {
        [](const void* source,
           ttx_callable_description* output) -> ttx_binding_status {
          *output = description(static_cast<const Export*>(source)->index - 2);
          return TTX_BINDING_SATISFIED;
        },
      };
      return Ttx::Semantic::Negotiation::Binding::provide<
          Ttx::Concept::Declarations::Callable>(
          ttx_callable(this, &operations), requested);
    }
    return Ttx::Semantic::Negotiation::Binding::Status::Unsupported;
  }

  auto visit_concepts(Ttx::Concept::Abstract::Visitor visitor) const -> void {
    const U32 first = index == 0 ? 1 : 2;
    const U32 end = index == 0 ? 2 : index == 1 ? 8 : first;
    for (U32 i = first; i != end; ++i) {
      visitor(root[i].get_data(), Ttx::Concept::Abstract::provide(root[i]));
    }
  }

  auto resolve_concept(Core::View::Bytes name) const -> Ttx::Concept::Abstract {
    auto result = Ttx::Concept::Abstract(ttx_none());
    auto find = [&](Core::View::Bytes observed,
                    Ttx::Concept::Abstract subject) {
      if (observed == name) {
        result = subject;
      }
    };
    visit_concepts(Ttx::Concept::Abstract::Visitor(find));
    return result;
  }
};

class Discovery {
 public:
  Discovery(
      Imaging::Render::Module::OpenImages images,
      Imaging::Render::Module::OpenSamples samples,
      Ttx::Semantic::Negotiation::Query capabilities)
      : samples(samples), capabilities(capabilities) {
    for (U32 i = 0; i != 8; ++i) {
      exports[i] = Export(exports, i, images);
    }
  }

  auto get_data() const -> Core::View::Bytes { return "Images"_view; }

  auto supports(System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    using Ttx::Semantic::Negotiation::Binding::Status;
    // The export promises sampling without initializing a CUDA program just
    // to answer a property query. Binding retains responsibility for
    // realization.
    if (samples && id == Sampling::Contracts::Samples::contract_id) {
      return Ttx::Semantic::Negotiation::Binding::Status::Satisfied;
    }
    return capabilities.is_set()
               ? capabilities.supports(id)
               : Ttx::Semantic::Negotiation::Binding::Status::Unsupported;
  }

  auto resolve_concept(Core::View::Bytes name) const -> Ttx::Concept::Abstract {
    return name == exports[0].get_data()
               ? Ttx::Concept::Abstract::provide(exports[0])
               : Ttx::Concept::Abstract(ttx_none());
  }

  auto visit_concepts(Ttx::Concept::Abstract::Visitor visitor) const -> void {
    visitor(exports[0].get_data(), Ttx::Concept::Abstract::provide(exports[0]));
  }

  auto bind_interface(System::Uuid id, Ttx::Data::Form::Storage requested) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    // Sampling is a separate optional capability on the acquired subject.
    // Inspecting declarations does not initialize a CUDA sampling program.
    // Its optional publication stays private to this provider and lives until
    // the enclosing acquisition releases it.
    if (!samples || id != Sampling::Contracts::Samples::contract_id) {
      return capabilities.is_set()
                 ? capabilities.bind(id, requested)
                 : Ttx::Semantic::Negotiation::Binding::Status::Unsupported;
    }

    if (!compute) {
      ttx_publication publication = {};
      if (samples(&publication) != TTX_DATA_SUCCESS) {
        return Ttx::Semantic::Negotiation::Binding::Status::Rejected;
      }
      compute = Ttx::Semantic::Ownership::Publication(publication);
    }
    return compute->get_query().bind(id, requested);
  }

 private:
  Imaging::Render::Module::OpenSamples samples;
  Ttx::Semantic::Negotiation::Query capabilities;
  Export exports[8];
  mutable Core::Option<Ttx::Semantic::Ownership::Publication> compute;
};

auto Imaging::Render::Module::open(
    OpenImages images,
    OpenSamples samples,
    ttx_module_acquisition* output,
    Ttx::Semantic::Negotiation::Query capabilities) -> ttx_data_status {
  auto memory = Core::Bibliotheca::check_out(sizeof(Discovery));
  auto* graph = new (memory.ptr, Core::Placement::Construct)
      Discovery(images, samples, capabilities);
  *output = {
    Ttx::Concept::Abstract::provide(*graph).get_abi(), graph,
    [](const void* source) {
      auto* graph =
          const_cast<Discovery*>(static_cast<const Discovery*>(source));
      graph->~Discovery();
      Core::Bibliotheca::remit(reinterpret_cast<U8*>(graph));
    }};
  return TTX_DATA_SUCCESS;
}
