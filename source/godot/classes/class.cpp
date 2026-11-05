// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "godot/classes/class.hpp"

#include "toolchain/memory/lifetime.hpp"

#include "perimortem/memory/allocator/global.hpp"

#include "godot/classes/instance.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/godot.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Graph;
using namespace Ttx::Semantic;
using namespace Godot::Extension;
using namespace Toolchain::Memory;

static U8 value_token;
static const GDExtensionInstanceBindingCallbacks value_binding{};

auto Classes::Class::depends_on(const Class& other) const -> bool {
  for (Count i = 0; i < methods.get_size(); ++i) {
    const auto& method = methods.get_data()[i];
    const auto inputs = method.get_inputs().get_fields();
    const auto outputs = method.get_outputs().get_fields();
    for (Count j = 0; j < inputs.get_size(); ++j) {
      if (inputs.get_data()[j].uses_materializer(&other)) {
        return true;
      }
    }

    for (Count j = 0; j < outputs.get_size(); ++j) {
      if (outputs.get_data()[j].uses_materializer(&other)) {
        return true;
      }
    }
  }

  return false;
}

static auto instance(GDExtensionObjectPtr object) -> Classes::Instance* {
  return object ? Data::cast<Classes::Instance>(
                      godot::gdextension_interface::object_get_instance_binding(
                          object, &value_token, &value_binding))
                : nullptr;
}

auto Classes::Class::get_materializer() -> Values::Field::Materializer {
  return {
    name, this,
    [](void* context, const U8* bytes) -> GDExtensionObjectPtr {
      return Data::cast<Class>(context)->materialize(bytes);
    },

    [](void* context, GDExtensionObjectPtr object, U8* output) -> bool {
      const auto* expected = Data::cast<Class>(context);
      const auto* value = instance(object);
      // TODO: Representation compatibility establishes call layout rather than
      // semantic Type identity. Require this exact generated Class or a
      // portable Coherence Type identity before accepting represented object
      // arguments.
      if (!value || !value->get_type().get_representation().compatible(
                        expected->value_form)) {
        return false;
      }

      const auto bytes = value->get_value();
      if (bytes.get_size() != expected->value_form.get_extent()) {
        return false;
      }

      Data::copy(output, bytes.get_data(), bytes.get_size());
      return true;
    }};
}

auto Classes::Class::materialize(const U8* bytes) -> GDExtensionObjectPtr {
  if (!published || !represented) {
    return nullptr;
  }

  const godot::StringName base("RefCounted");
  auto output = godot::gdextension_interface::classdb_construct_object2(
      base._native_ptr());
  if (!output) {
    return nullptr;
  }

  auto allocation = Allocator::Global::allocate(sizeof(Instance));
  auto* value = &Lifetime::construct<Instance>(allocation.get_data(), [&] {
    return Instance(
        *this,
        View::Bytes(perimortem_view_bytes{bytes, value_form.get_extent()}));
  });

  ++instances;
  godot::gdextension_interface::object_set_instance(
      output, name._native_ptr(), value);
  godot::gdextension_interface::object_set_instance_binding(
      output, &value_token, value, &value_binding);
  return output;
}

auto Classes::Class::publish() -> bool {
  if (published || godot::ClassDB::class_exists(name)) {
    return false;
  }

  GDExtensionClassCreationInfo5 info{};
  info.is_exposed = true;
  info.is_abstract = represented;
  info.class_userdata = this;
  info.create_instance_func = create;
  info.free_instance_func = destroy;

  const godot::StringName base("RefCounted");
  godot::gdextension_interface::classdb_register_extension_class5(
      godot::gdextension_interface::library, name._native_ptr(),
      base._native_ptr(), &info);
  for (Count index = 0; index < methods.get_size(); ++index) {
    methods.get_data()[index].publish(name);
  }

  published = true;
  return true;
}

auto Classes::Class::create(void* context, GDExtensionBool notify)
    -> GDExtensionObjectPtr {
  auto& type = *Data::cast<Class>(context);
  if (type.represented) {
    return nullptr;
  }

  const godot::StringName base("RefCounted");
  auto output = godot::gdextension_interface::classdb_construct_object2(
      base._native_ptr());
  if (!output) {
    return nullptr;
  }

  const auto allocation = Allocator::Global::allocate(sizeof(Instance));
  auto* instance = &Lifetime::construct<Instance>(
      allocation.get_data(), [&] { return Instance(type); });
  ++type.instances;
  godot::gdextension_interface::object_set_instance(
      output, type.name._native_ptr(), instance);
  return output;
}

auto Classes::Class::destroy(void* context, GDExtensionClassInstancePtr pointer)
    -> void {
  auto* instance = Data::cast<Instance>(pointer);
  instance->~Instance();
  Allocator::Global::release(Data::cast<U8>(instance));
}

Classes::Class::~Class() {
  if (published) {
    godot::gdextension_interface::classdb_unregister_extension_class(
        godot::gdextension_interface::library, name._native_ptr());
  }
}
