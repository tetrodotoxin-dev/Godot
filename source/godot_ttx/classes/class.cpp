// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "godot_ttx/classes/class.hpp"

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "godot_ttx/classes/instance.hpp"
#include "ttx/concept/capabilities/borrow.hpp"
#include "ttx/concept/policies/none.hpp"

using namespace Godot::Extension;
using namespace Perimortem;

Godot::Extension::Classes::Class::Class(
    Ttx::Concept::Policies::Borrowed factory,
    Ttx::Concept::Capabilities::Create constructor,
    godot::String name,
    godot::String base,
    Memory::Dynamic::Vector<Method> methods)
    : factory(factory),
      constructor(constructor),
      name(name),
      base(base),
      methods(Core::Data::take(methods)) {
  node = base == "Node" || godot::ClassDB::is_parent_class(base, "Node");
}

auto Godot::Extension::Classes::Class::publish() -> Ttx::Data::Status {
  if (published || godot::ClassDB::class_exists(name)) {
    return Ttx::Data::Status::Invalid;
  }

  // Registration names and method descriptions live in this owner alongside
  // the retained constructor. Godot can keep userdata pointers into that state
  // for the registration's lifetime, including after discovery has ended.
  GDExtensionClassCreationInfo5 info = GDExtensionClassCreationInfo5();
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
  using namespace Ttx::Semantic::Negotiation;
  GDExtensionObjectPtr object = nullptr;
  auto receive = [&](Ttx::Concept::Abstract subject) {
    subject.bind<Ttx::Concept::Capabilities::Borrow>().visit(
        [&](Ttx::Concept::Capabilities::Borrow policy) -> Binding::Status {
          return policy.borrow().visit(
              [&](Ttx::Concept::Policies::Borrowed acquired)
                  -> Binding::Status {
                return Instance::create(type, acquired)
                    .visit(
                        [&](Instance* instance) -> Binding::Status {
                          // Constructing the native base is the first Godot
                          // side effect. The provider and all method bindings
                          // are ready before it can receive even a construction
                          // notification.
                          ++type.instances;
                          object = godot::gdextension_interface::
                              classdb_construct_object2(
                                  type.base._native_ptr());
                          if (!object) {
                            instance->~Instance();
                            Core::Bibliotheca::remit(
                                reinterpret_cast<U8*>(instance));
                            return Ttx::Semantic::Negotiation::Binding::Status::
                                Rejected;
                          }

                          godot::gdextension_interface::object_set_instance(
                              object, type.name._native_ptr(), instance);
                          if (notify) {
                            static const auto method = godot::
                                gdextension_interface::classdb_get_method_bind(
                                    godot::StringName("Object")._native_ptr(),
                                    godot::StringName("notification")
                                        ._native_ptr(),
                                    4023243586);
                            int64_t notification =
                                godot::Object::NOTIFICATION_POSTINITIALIZE;
                            bool reversed = false;
                            const void* arguments[] = {
                              &notification, &reversed};
                            godot::gdextension_interface::
                                object_method_bind_ptrcall(
                                    method, object, arguments, nullptr);
                          }

                          return Binding::Status::Satisfied;
                        },
                        [](Ttx::Data::Status) {
                          return Binding::Status::Rejected;
                        });
              },
              [](Binding::Failure failure) {
                return static_cast<Binding::Status>(failure);
              });
        },
        [](Binding::Failure failure) {
          return static_cast<Binding::Status>(failure);
        });
  };
  const auto status = type.constructor.create(
      Ttx::Concept::Policies::None::get_none(), receive);
  return status == Binding::Status::Satisfied ? object : nullptr;
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
  factory.release();
}
