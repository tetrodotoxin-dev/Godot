// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "extension/classes/class.hpp"

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "extension/classes/instance.hpp"

using namespace Godot::Extension;
using namespace Perimortem;

Godot::Extension::Classes::Class::Class(
    Ttx::Concept::Modules::Module module,
    Ttx::Semantic::Ownership::Publication factory,
    Ttx::Semantic::Ownership::Factory constructor,
    godot::String name,
    godot::String base,
    Memory::Dynamic::Vector<Method> methods)
    : module(Core::Data::take(module)),
      factory(Core::Data::take(factory)),
      constructor(constructor),
      name(name),
      base(base),
      methods(Core::Data::take(methods)) {
  node = base == "Node" || godot::ClassDB::is_parent_class(base, "Node");
}

auto Godot::Extension::Classes::Class::get_query() const -> ttx_semantic_query {
  return {
    this,
    [](const void* source, perimortem_uuid id,
       ttx_storage requested) -> ttx_binding_status {
      if (System::Uuid(id) !=
          ::Godot::Extension::Contracts::Class::contract_id) {
        return TTX_BINDING_UNSUPPORTED;
      }

      static const godot_class_operations operations = {
        [](const void* source) {
          return static_cast<ttx_data_status>(
              const_cast<Class*>(static_cast<const Class*>(source))->publish());
        },
      };
      return static_cast<ttx_binding_status>(
          Ttx::Semantic::Negotiation::Binding::provide<
              ::Godot::Extension::Contracts::Class>(
              godot_class(source, &operations),
              Ttx::Data::Form::Storage(requested)));
    },
    [](const void*, perimortem_uuid id) -> ttx_binding_status {
      return System::Uuid(id) ==
                     ::Godot::Extension::Contracts::Class::contract_id
                 ? TTX_BINDING_SATISFIED
                 : TTX_BINDING_UNSUPPORTED;
    }};
}

auto Godot::Extension::Classes::Class::publish() -> Ttx::Data::Status {
  if (published || godot::ClassDB::class_exists(name)) {
    return Ttx::Data::Status::Invalid;
  }

  // Preparation has already copied every registration name and retained the
  // independent factory. Godot can now keep userdata pointers into this owner
  // without borrowing the discovery publication that produced it.
  GDExtensionClassCreationInfo5 info = {};
  info.is_exposed = true;
  info.class_userdata = this;
  info.create_instance_func = create;
  info.free_instance_func = destroy;
  info.notification_func = [](void* instance, int32_t notification,
                              GDExtensionBool) {
    static_cast<Instance*>(instance)->notify(notification);
  };
  godot::gdextension_interface::classdb_register_extension_class5(
      godot::gdextension_interface::library, name._native_ptr(),
      base._native_ptr(), &info);
  for (Count index = 0; index < methods.get_size(); ++index) {
    methods.get_data()[index].publish(name);
  }

  published = true;
  return Ttx::Data::Status::Success;
}

auto Godot::Extension::Classes::Class::create(
    void* source,
    GDExtensionBool notify) -> GDExtensionObjectPtr {
  auto& type = *static_cast<Class*>(source);
  return type.constructor.create().visit(
      [&](Ttx::Semantic::Ownership::Publication& publication)
          -> GDExtensionObjectPtr {
        return Instance::create(type, Core::Data::take(publication))
            .visit(
                [&](Instance* instance) -> GDExtensionObjectPtr {
                  // Constructing the native base is the first Godot side
                  // effect. The provider and all method bindings are ready
                  // before it can receive even a construction notification.
                  ++type.instances;
                  auto object =
                      godot::gdextension_interface::classdb_construct_object2(
                          type.base._native_ptr());
                  if (!object) {
                    instance->~Instance();
                    Core::Bibliotheca::remit(reinterpret_cast<U8*>(instance));
                    return nullptr;
                  }

                  godot::gdextension_interface::object_set_instance(
                      object, type.name._native_ptr(), instance);
                  if (notify) {
                    static const auto method =
                        godot::gdextension_interface::classdb_get_method_bind(
                            godot::StringName("Object")._native_ptr(),
                            godot::StringName("notification")._native_ptr(),
                            4023243586);
                    int64_t notification =
                        godot::Object::NOTIFICATION_POSTINITIALIZE;
                    bool reversed = false;
                    const void* arguments[] = {&notification, &reversed};
                    godot::gdextension_interface::object_method_bind_ptrcall(
                        method, object, arguments, nullptr);
                  }

                  return object;
                },
                [](Ttx::Data::Status) -> GDExtensionObjectPtr {
                  return nullptr;
                });
      },
      [](Ttx::Data::Status) -> GDExtensionObjectPtr { return nullptr; });
}

auto Godot::Extension::Classes::Class::destroy(
    void*,
    GDExtensionClassInstancePtr source) -> void {
  auto* instance = static_cast<Instance*>(source);
  instance->~Instance();
  Core::Bibliotheca::remit(reinterpret_cast<U8*>(instance));
}

Godot::Extension::Classes::Class::~Class() {
  if (instances) {
    Core::Diagnostics::Log::fatal(
        "Godot class still has runtime instances at teardown."_view);
  }

  if (published) {
    godot::gdextension_interface::classdb_unregister_extension_class(
        godot::gdextension_interface::library, name._native_ptr());
  }
}
