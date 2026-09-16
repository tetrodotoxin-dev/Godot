// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "plugins/render/runtime.hpp"

#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "gdextension/contracts/scalar.hpp"
#include "imaging/contracts/invert.hpp"
#include "imaging/contracts/render.hpp"
#include "ttx/data/form/compiled.hpp"
#include "ttx/semantic/flows/copy.hpp"
#include "ttx/semantic/ownership/factory.hpp"
#include "ttx/semantic/realization/invocation.hpp"
#include "ttx/semantic/realization/simulacra.hpp"

using namespace Perimortem;
using namespace Godot;

// Backend storage stays private even when the generated class presents mutable
// observations. Replacing the current image commits only after the provider
// succeeds. A failed upload or operation leaves the previous image available.
class RenderInstance {
 public:
  explicit RenderInstance(image_provider provider) : provider(provider) {
    calls[0] = {
      this, &Plugins::Render::Runtime::input(0),
      &Plugins::Render::Runtime::output(0),
      [](const void* self, const void* input, void* output) -> ttx_data_status {
        auto& instance = *const_cast<RenderInstance*>(
            static_cast<const RenderInstance*>(self));
        *static_cast<U8*>(output) =
            instance.upload(*static_cast<const render_upload*>(input));
        return TTX_DATA_SUCCESS;
      }};
    calls[1] = {
      this, &Plugins::Render::Runtime::input(1),
      &Plugins::Render::Runtime::output(1),
      [](const void* self, const void*, void* output) -> ttx_data_status {
        auto& instance = *const_cast<RenderInstance*>(
            static_cast<const RenderInstance*>(self));
        *static_cast<U8*>(output) = instance.invert();
        return TTX_DATA_SUCCESS;
      }};
    calls[2] = {
      this, &Plugins::Render::Runtime::input(2),
      &Plugins::Render::Runtime::output(2),
      [](const void* self, const void*, void* output) -> ttx_data_status {
        return const_cast<RenderInstance*>(
                   static_cast<const RenderInstance*>(self))
            ->pixels(*static_cast<perimortem_view_bytes*>(output));
      }};
    calls[3] = {
      this, &Plugins::Render::Runtime::input(3),
      &Plugins::Render::Runtime::output(3),
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
      this, &Plugins::Render::Runtime::input(4),
      &Plugins::Render::Runtime::output(4),
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
      this, &Plugins::Render::Runtime::input(5),
      &Plugins::Render::Runtime::output(5),
      [](const void* self, const void*, void* output) -> ttx_data_status {
        const auto error =
            static_cast<const RenderInstance*>(self)->error.get_view();
        *static_cast<perimortem_view_bytes*>(output) = {
          error.get_data(), error.get_size()};
        return TTX_DATA_SUCCESS;
      }};
  }

  ~RenderInstance() {
    if (image.operations) {
      image.operations->release(image.source);
    }
    provider.operations->release(provider.source);
  }

  auto get_query() const -> ttx_semantic_query {
    return {
      this,
      [](const void* source, perimortem_uuid id,
         ttx_storage requested) -> ttx_binding_status {
        if (id.high != LAB_RENDER_METHOD_HIGH ||
            id.low < LAB_RENDER_METHOD_LOW ||
            id.low >= LAB_RENDER_METHOD_LOW + 6) {
          return TTX_BINDING_UNSUPPORTED;
        }
        const auto& api = static_cast<const RenderInstance*>(source)
                              ->calls[id.low - LAB_RENDER_METHOD_LOW];
        return ttx_binding_provide(
            ttx_invocation_representation(), &api, requested);
      },
      [](const void*, perimortem_uuid id) -> ttx_binding_status {
        return id.high == LAB_RENDER_METHOD_HIGH &&
                       id.low >= LAB_RENDER_METHOD_LOW &&
                       id.low < LAB_RENDER_METHOD_LOW + 6
                   ? TTX_BINDING_SATISFIED
                   : TTX_BINDING_UNSUPPORTED;
      }};
  }

 private:
  // These helpers perform one transaction on the same current image and its
  // diagnostic. Keeping them with the instance makes commit ordering explicit
  // without exposing mutable backend state to the declaration policy.
  auto upload(const render_upload& input) -> U8 {
    if (input.width <= 0 || input.height <= 0 || input.width > U32(-1) ||
        input.height > U32(-1)) {
      error = "Image dimensions are outside the provider range."_view;
      return 0;
    }
    image_object next = {};
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
        Ttx::Semantic::Transport::Flow::reader(form),
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

  image_provider provider;
  image_object image = {};
  Memory::Dynamic::Bytes observed;
  Memory::Dynamic::Bytes error;
  ttx_invocation calls[6];
};

struct RenderFactory {
  Plugins::Render::Module::OpenImages open;
};

auto Plugins::Render::Runtime::emit(
    Module::OpenImages open,
    ttx_publication* output) -> ttx_data_status {
  auto memory = Core::Bibliotheca::check_out(sizeof(RenderFactory));
  auto* factory =
      new (memory.ptr, Core::Placement::Construct) RenderFactory(open);
  *output = {
    {factory,
     [](const void* source, perimortem_uuid id,
        ttx_storage requested) -> ttx_binding_status {
       if (id.high == LAB_RENDER_PROVIDER_ID_HIGH &&
           id.low == LAB_RENDER_PROVIDER_ID_LOW) {
         static const render_provider_operations operations = {
           [](const void* source, image_provider* output) {
             return static_cast<const RenderFactory*>(source)->open(output);
           },
         };
         const render_api api = {source, &operations};
         return static_cast<ttx_binding_status>(
             Ttx::Semantic::Negotiation::Binding::provide(
                 api,
                 Ttx::Data::Form::Compiled<Ttx::Data::Form::Native<
                     render_api>::reference>::get_representation(),
                 Ttx::Data::Form::Storage(requested)));
       }
       if (System::Uuid(id) == Ttx::Semantic::Ownership::Factory::contract_id) {
         static const ttx_factory_operations operations = {
           [](const void* source, ttx_publication* output) -> ttx_data_status {
             image_provider provider = {};
             const auto failed =
                 static_cast<const RenderFactory*>(source)->open(&provider);
             if (failed.size) {
               return TTX_DATA_IO_ERROR;
             }
             auto memory = Core::Bibliotheca::check_out(sizeof(RenderInstance));
             auto* instance = new (memory.ptr, Core::Placement::Construct)
                 RenderInstance(provider);
             *output = {
               instance->get_query(), [](const void* source) {
                 auto* instance = const_cast<RenderInstance*>(
                     static_cast<const RenderInstance*>(source));
                 instance->~RenderInstance();
                 Core::Bibliotheca::remit(reinterpret_cast<U8*>(instance));
               }};
             return TTX_DATA_SUCCESS;
           },
         };
         const ttx_factory api = {source, &operations};
         return static_cast<ttx_binding_status>(
             Ttx::Semantic::Negotiation::Binding::provide(
                 api,
                 Ttx::Data::Form::Compiled<Ttx::Data::Form::Native<
                     ttx_factory>::reference>::get_representation(),
                 Ttx::Data::Form::Storage(requested)));
       }
       return TTX_BINDING_UNSUPPORTED;
     },
     [](const void*, perimortem_uuid id) -> ttx_binding_status {
       return System::Uuid(id) ==
                          Ttx::Semantic::Ownership::Factory::contract_id ||
                      (id.high == LAB_RENDER_PROVIDER_ID_HIGH &&
                       id.low == LAB_RENDER_PROVIDER_ID_LOW)
                  ? TTX_BINDING_SATISFIED
                  : TTX_BINDING_UNSUPPORTED;
     }},
    [](const void* source) {
      auto* factory =
          const_cast<RenderFactory*>(static_cast<const RenderFactory*>(source));
      factory->~RenderFactory();
      Core::Bibliotheca::remit(reinterpret_cast<U8*>(factory));
    }};
  return TTX_DATA_SUCCESS;
}

auto Plugins::Render::Runtime::input(U32 method)
    -> const Ttx::Data::Form::Representation& {
  if (method != 0) {
    return ::Gdextension::Contracts::Scalar::get_representation(
        ::Gdextension::Contracts::Scalar::Kind::Empty);
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

auto Plugins::Render::Runtime::output(U32 method)
    -> const Ttx::Data::Form::Representation& {
  using ::Gdextension::Contracts::Scalar;
  const Scalar::Kind kinds[] = {Scalar::Kind::Boolean, Scalar::Kind::Boolean,
                                Scalar::Kind::Text,    Scalar::Kind::Integer,
                                Scalar::Kind::Integer, Scalar::Kind::Text};
  return Scalar::get_representation(kinds[method]);
}
