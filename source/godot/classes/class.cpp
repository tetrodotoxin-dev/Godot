// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "godot/classes/class.hpp"

#include "toolchain/memory/lifetime.hpp"

#include "perimortem/memory/allocator/global.hpp"

#include "ttx/graph/capabilities/retain.hpp"
#include "ttx/graph/policies/none.hpp"

#include "godot/classes/instance.hpp"

#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/godot.hpp"
#include "godot_cpp/variant/utility_functions.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Graph;
using namespace Ttx::Semantic;
using namespace Godot::Extension;

auto Classes::Class::publish() -> bool {
  if (published || godot::ClassDB::class_exists(name)) {
    return false;
  }

  GDExtensionClassCreationInfo5 info{};
  info.is_exposed = true;
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
  GDExtensionObjectPtr output = nullptr;
  auto callback = [&](Abstract created) {
    created.bind<Capabilities::Retain>().visit(
        [&](Capabilities::Retain request) {
          request.retain().visit(
              [&](Policies::Retained acquired) {
                const godot::StringName base("RefCounted");
                output =
                    godot::gdextension_interface::classdb_construct_object2(
                        base._native_ptr());
                if (!output) {
                  acquired.release();
                  return;
                }

                const auto allocation =
                    Allocator::Global::allocate(sizeof(Instance));
                auto* instance =
                    &Toolchain::Memory::Lifetime::construct<Instance>(
                        allocation.get_data(),
                        [&] { return Instance(type, acquired); });
                ++type.instances;
                godot::gdextension_interface::object_set_instance(
                    output, type.name._native_ptr(), instance);
              },

              [&](Negotiation::Status status) {});
        },

        [] {});
  };

  type.factory.get_abstract().bind<Capabilities::Create>().visit(
      [&](Capabilities::Create create) {
        create.create(Policies::None::get_none().get_abstract(), callback);
      },

      [] {});
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

  if (factory.release() != Policies::Retained::Status::Success) {
    godot::UtilityFunctions::push_error("TTX factory cleanup failed: ", name);
  }
}
