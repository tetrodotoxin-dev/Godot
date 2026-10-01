// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "demo/imaging/render/runtime.hpp"

#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/object.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "demo/imaging/contracts/invert.hpp"
#include "demo/imaging/contracts/render.hpp"
#include "demo/sampling/contracts/samples.hpp"
#include "godot_ttx/contracts/scalar.hpp"
#include "ttx/concept/capabilities/borrow.hpp"
#include "ttx/data/form/compiled.hpp"
#include "ttx/semantic/flows/copy.hpp"
#include "ttx/semantic/negotiation/receiver.hpp"
#include "ttx/semantic/realization/invocation.hpp"
#include "ttx/semantic/realization/simulacra.hpp"

using namespace Perimortem;
using namespace Godot::Demo;

// Backend storage stays private even when the generated class presents mutable
// observations. Replacing the current image commits only after the provider
// succeeds. A failed upload or operation leaves the previous image available.
class RenderInstance {
 public:
  explicit RenderInstance(image_provider provider) : provider(provider) {
    calls[0] = {
      this, &Imaging::Render::Runtime::input(0),
      &Imaging::Render::Runtime::output(0),
      [](const void* self, const void* input, void* output) -> ttx_data_status {
        auto& instance = *const_cast<RenderInstance*>(
            static_cast<const RenderInstance*>(self));
        *static_cast<U8*>(output) =
            instance.upload(*static_cast<const render_upload*>(input));
        return TTX_DATA_SUCCESS;
      }};
    calls[1] = {
      this, &Imaging::Render::Runtime::input(1),
      &Imaging::Render::Runtime::output(1),
      [](const void* self, const void*, void* output) -> ttx_data_status {
        auto& instance = *const_cast<RenderInstance*>(
            static_cast<const RenderInstance*>(self));
        *static_cast<U8*>(output) = instance.invert();
        return TTX_DATA_SUCCESS;
      }};
    calls[2] = {
      this, &Imaging::Render::Runtime::input(2),
      &Imaging::Render::Runtime::output(2),
      [](const void* self, const void*, void* output) -> ttx_data_status {
        return const_cast<RenderInstance*>(
                   static_cast<const RenderInstance*>(self))
            ->pixels(*static_cast<perimortem_view_bytes*>(output));
      }};
    calls[3] = {
      this, &Imaging::Render::Runtime::input(3),
      &Imaging::Render::Runtime::output(3),
      [](const void* self, const void*, void* output) -> ttx_data_status {
        const auto& instance = *static_cast<const RenderInstance*>(self);
        *static_cast<S64*>(output) =
            instance.image.operations
                ? instance.image.operations->dimensions(instance.image.source)
                      .width
                : 0;
        return TTX_DATA_SUCCESS;
      }};
    calls[4] = {
      this, &Imaging::Render::Runtime::input(4),
      &Imaging::Render::Runtime::output(4),
      [](const void* self, const void*, void* output) -> ttx_data_status {
        const auto& instance = *static_cast<const RenderInstance*>(self);
        *static_cast<S64*>(output) =
            instance.image.operations
                ? instance.image.operations->dimensions(instance.image.source)
                      .height
                : 0;
        return TTX_DATA_SUCCESS;
      }};
    calls[5] = {
      this, &Imaging::Render::Runtime::input(5),
      &Imaging::Render::Runtime::output(5),
      [](const void* self, const void*, void* output) -> ttx_data_status {
        const auto error =
            static_cast<const RenderInstance*>(self)->error.get_view();
        *static_cast<perimortem_view_bytes*>(output) = {
          error.get_data(), error.get_size()};
        return TTX_DATA_SUCCESS;
      }};
  }

  auto retain() const -> void { ++references; }
  auto release() const -> void {
    if (!--references) {
      delete this;
    }
  }

  ~RenderInstance() {
    if (image.operations) {
      image.operations->release(image.source);
    }
    provider.operations->release(provider.source);
  }

  auto get_data() const -> Core::View::Bytes { return Core::View::Bytes(); }
  auto borrow() const -> Utility::Result<
      Ttx::Concept::Policies::Borrowed,
      Ttx::Semantic::Negotiation::Binding::Failure> {
    retain();
    return Ttx::Concept::Policies::Borrowed::provide(*this);
  }
  auto supports(System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    using namespace Ttx::Concept;
    using namespace Ttx::Semantic::Negotiation;
    return id == Capabilities::Borrow::contract_id ||
                   id == Policies::Borrowed::contract_id ||
                   (static_cast<perimortem_uuid>(id).high ==
                        LAB_RENDER_METHOD_HIGH &&
                    static_cast<perimortem_uuid>(id).low >=
                        LAB_RENDER_METHOD_LOW &&
                    static_cast<perimortem_uuid>(id).low <
                        LAB_RENDER_METHOD_LOW + 6)
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
    if (id == Policies::Borrowed::contract_id) {
      return Binding::provide<Policies::Borrowed>(
          Policies::Borrowed::provide(*this).get_abi(), target);
    }
    if (static_cast<perimortem_uuid>(id).high != LAB_RENDER_METHOD_HIGH ||
        static_cast<perimortem_uuid>(id).low < LAB_RENDER_METHOD_LOW ||
        static_cast<perimortem_uuid>(id).low >= LAB_RENDER_METHOD_LOW + 6) {
      return Binding::Status::Unknown;
    }
    return static_cast<Binding::Status>(ttx_binding_provide(
        ttx_invocation_representation(),
        &calls[static_cast<perimortem_uuid>(id).low - LAB_RENDER_METHOD_LOW],
        target.get_abi()));
  }

 private:
  // Each operation observes the instance's current image. Upload publishes a
  // completed replacement before returning the previous image, so callbacks
  // from its release observe the committed state.
  auto upload(const render_upload& input) -> U8 {
    if (input.width <= 0 || input.height <= 0 || input.width > U32(-1) ||
        input.height > U32(-1)) {
      error = "Image dimensions are outside the provider range."_view;
      return 0;
    }
    image_object next = image_object();
    const auto failed = provider.operations->create(
        provider.source, input.width, input.height, input.pixels.data,
        input.pixels.size, &next);
    return replace(next, failed);
  }

  auto invert() -> U8 {
    if (!image.operations) {
      error = "No image has been uploaded."_view;
      return 0;
    }
    return Ttx::Semantic::Realization::Simulacra::fulfill<
               Imaging::Contracts::Invert>(
               Ttx::Semantic::Negotiation::Query(
                   image.operations->query(image.source)))
        .visit(
            [&](Imaging::Contracts::Invert operation) -> U8 {
              return operation.apply().visit(
                  [&](image_object next) -> U8 { return replace(next, {}); },
                  [&](Core::View::Bytes failed) -> U8 {
                    error = failed;
                    return 0;
                  });
            },
            [&](Ttx::Semantic::Negotiation::Binding::Failure) -> U8 {
              error = "Image does not supply inversion."_view;
              return 0;
            });
  }

  auto replace(image_object next, image_error failed) -> U8 {
    if (failed.size) {
      error = Core::View::Bytes(failed.data, failed.size);
      return 0;
    }
    if (image.operations) {
      image.operations->release(image.source);
    }
    image = next;
    error.clear();
    return 1;
  }

  auto pixels(perimortem_view_bytes& output) -> ttx_data_status {
    if (!image.operations) {
      return TTX_DATA_INVALID;
    }
    const auto& form = *image.operations->representation(image.source);
    observed.forgetful_resize(form.get_extent());
    Ttx::Semantic::Transport::Flow flow;
    const auto status = flow.connect(
        Ttx::Semantic::Transport::Flow::consumer(form),
        Ttx::Semantic::Negotiation::Query(
            image.operations->pixels(image.source)));
    if (status != Ttx::Semantic::Transport::Flow::Status::Success) {
      return TTX_DATA_DENIED;
    }
    const auto result = Ttx::Semantic::Flows::Copy::flow(
        flow,
        Ttx::Data::Form::Storage(
            {&form, observed.get_access().get_data(), observed.get_size()}));
    if (result == Ttx::Data::Status::Success) {
      output = {observed.get_view().get_data(), observed.get_size()};
    }
    return static_cast<ttx_data_status>(result);
  }

  Count references = 1;
  image_provider provider;
  image_object image = image_object();
  Memory::Dynamic::Bytes observed;
  Memory::Dynamic::Bytes error;
  ttx_invocation calls[6];
};

auto Imaging::Render::Runtime::create(
    Runtime::OpenImages images,
    Runtime::OpenSamples samples,
    Ttx::Semantic::Negotiation::Query capabilities) -> Runtime& {
  static const Core::Object<>::Descriptor descriptor(
      sizeof(Runtime), alignof(Runtime),
      [](U8* source) { reinterpret_cast<Runtime*>(source)->~Runtime(); });
  return *new (
      Core::Object<>::create(descriptor).get_payload(),
      Core::Placement::Construct) Runtime(images, samples, capabilities);
}

Imaging::Render::Runtime::~Runtime() {
  if (compute) {
    compute->release();
  }
}

auto Imaging::Render::Runtime::retain() const -> void {
  Core::Object<>(reinterpret_cast<U8*>(const_cast<Runtime*>(this))).retain();
}

auto Imaging::Render::Runtime::create_instance(
    void* receiver,
    void (*receive)(void*, ttx_abstract)) const
    -> Ttx::Semantic::Negotiation::Binding::Status {
  image_provider provider = image_provider();
  if (images(&provider).size) {
    return Ttx::Semantic::Negotiation::Binding::Status::Rejected;
  }
  auto* instance = new RenderInstance(provider);
  receive(receiver, Ttx::Concept::Abstract::provide(*instance).get_abi());
  instance->release();
  return Ttx::Semantic::Negotiation::Binding::Status::Satisfied;
}

auto Imaging::Render::Runtime::release() const -> void {
  Core::Object<>(reinterpret_cast<U8*>(const_cast<Runtime*>(this))).release();
}

auto Imaging::Render::Runtime::get_query() const
    -> Ttx::Semantic::Negotiation::Query {
  using namespace Ttx::Semantic::Negotiation;
  return Query(
      {this,
       [](const void* source, perimortem_uuid id,
          ttx_storage requested) -> ttx_binding_status {
         const auto& runtime = *static_cast<const Runtime*>(source);
         if (System::Uuid(id) == Imaging::Contracts::Render::contract_id) {
           static const render_provider_operations operations =
               render_provider_operations(
                   [](const void* source, image_provider* output) {
                     return static_cast<const Runtime*>(source)->images(output);
                   });
           return static_cast<ttx_binding_status>(
               Binding::provide<Imaging::Contracts::Render>(
                   render_api(source, &operations),
                   Ttx::Data::Form::Storage(requested)));
         }
         if (runtime.samples &&
             System::Uuid(id) == Sampling::Contracts::Samples::contract_id) {
           if (!runtime.compute) {
             auto acquire = [&](Query query) -> Binding::Status {
               return query.bind<Ttx::Concept::Capabilities::Borrow>().visit(
                   [&](Ttx::Concept::Capabilities::Borrow policy) {
                     return policy.borrow().visit(
                         [&](Ttx::Concept::Policies::Borrowed acquired) {
                           runtime.compute = acquired;
                           return Binding::Status::Satisfied;
                         },
                         [](Binding::Failure failure) {
                           return static_cast<Binding::Status>(failure);
                         });
                   },
                   [](Binding::Failure failure) {
                     return static_cast<Binding::Status>(failure);
                   });
             };
             const auto status = runtime.samples(Receiver(acquire).get_abi());
             if (status != TTX_BINDING_SATISFIED) {
               return status;
             }
           }
           return static_cast<ttx_binding_status>(
               runtime.compute->bind_interface(
                   System::Uuid(id), Ttx::Data::Form::Storage(requested)));
         }
         return TTX_BINDING_UNKNOWN;
       },
       [](const void* source, perimortem_uuid id) -> ttx_binding_status {
         const auto& runtime = *static_cast<const Runtime*>(source);
         return System::Uuid(id) == Imaging::Contracts::Render::contract_id ||
                        (runtime.samples &&
                         System::Uuid(id) ==
                             Sampling::Contracts::Samples::contract_id)
                    ? TTX_BINDING_SATISFIED
                    : TTX_BINDING_UNKNOWN;
       }});
}

auto Imaging::Render::Runtime::input(U32 method)
    -> const Ttx::Data::Form::Representation& {
  if (method != 0) {
    return ::Godot::Extension::Contracts::Scalar::get_representation(
        ::Godot::Extension::Contracts::Scalar::Kind::Empty);
  }
  using Ttx::Data::Form::Schema;
  static constexpr auto integer = Schema::primitive(Schema::Value::S64);
  static constexpr auto pointer = Schema::primitive(Schema::Value::Pointer);
  static constexpr auto count = Schema::primitive(Schema::Value::U64);
  static constexpr Schema::Position view_fields[] = {
    Schema::Position(pointer, 0), Schema::Position(count, 8)};
  static constexpr auto view = Schema::composite(
      view_fields, sizeof(perimortem_view_bytes),
      alignof(perimortem_view_bytes));
  static constexpr Schema::Position positions[] = {
    Schema::Position(integer, __builtin_offsetof(render_upload, width)),
    Schema::Position(integer, __builtin_offsetof(render_upload, height)),
    Schema::Position(view, __builtin_offsetof(render_upload, pixels)),
  };
  static constexpr auto schema = Schema::composite(
      positions, sizeof(render_upload), alignof(render_upload));
  return Ttx::Data::Form::Compiled<schema>::get_representation();
}

auto Imaging::Render::Runtime::output(U32 method)
    -> const Ttx::Data::Form::Representation& {
  using ::Godot::Extension::Contracts::Scalar;
  const Scalar::Kind kinds[] = {Scalar::Kind::Boolean, Scalar::Kind::Boolean,
                                Scalar::Kind::Text,    Scalar::Kind::Integer,
                                Scalar::Kind::Integer, Scalar::Kind::Text};
  return Scalar::get_representation(kinds[method]);
}
