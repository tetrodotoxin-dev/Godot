// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "godot/classes/method.hpp"

#include "ttx/graph/capabilities/describe.hpp"

#include "godot/classes/instance.hpp"

#include "godot_cpp/godot.hpp"
#include "godot_cpp/variant/array.hpp"
#include "godot_cpp/variant/utility_functions.hpp"

using namespace Godot::Extension;
using namespace Perimortem;
using Godot::Extension::Values::Field;

auto Godot::Extension::Classes::Method::compile(
    Core::View::Bytes route,
    Ttx::Graph::Abstract declaration) -> Core::Storage::Option<Method> {
  using Option = Core::Storage::Option<Method>;
  Option result;
  declaration.bind<Ttx::Graph::Capabilities::Describe>().visit(
      [&](Ttx::Graph::Capabilities::Describe describe) {
        describe.get_name().visit(
            [&](Core::View::Bytes label) {
              const auto name = godot::String::utf8(
                  Core::Data::cast<const char>(label.get_data()),
                  label.get_size());
              if (!name.is_valid_identifier()) {
                return;
              }

              declaration.bind<Ttx::Graph::Capabilities::Callable>().visit(
                  [&](Ttx::Graph::Capabilities::Callable callable) {
                    Godot::Extension::Values::Frame::arguments(callable).visit(
                        [&](Godot::Extension::Values::Frame& inputs) {
                          Godot::Extension::Values::Frame::result(callable)
                              .visit(
                                  [&](Godot::Extension::Values::Frame&
                                          outputs) {
                                    result = Method(
                                        route, name, Core::Data::take(inputs),
                                        Core::Data::take(outputs));
                                  },

                                  [] {});
                        },

                        [] {});
                  },

                  [] {});
            },

            [] {});
      },

      [] {});
  return result;
}

static auto align_frame(void* source, Count alignment) -> U8* {
  const Count address = reinterpret_cast<Count>(source);
  const Count offset = (-address) & (alignment - 1);
  return static_cast<U8*>(source) + offset;
}

static auto invoke(
    const Godot::Extension::Classes::Method& method,
    Godot::Extension::Classes::Instance& instance,
    const void* const* arguments,
    void* output,
    bool variant) -> Ttx::Data::Status {
  const auto& input = method.get_inputs();
  const auto& returned = method.get_outputs();
  const auto fields = input.get_fields();

  // The concrete frames are small call storage, not persistent instance state.
  // Each validated frame contains at most 64 numeric arguments. Independent
  // stack storage keeps nested synchronous calls from sharing payload bytes.
  auto* bytes = align_frame(
      __builtin_alloca(input.get_extent() + input.get_alignment() - 1),
      input.get_alignment());
  auto* results = align_frame(
      __builtin_alloca(returned.get_extent() + returned.get_alignment() - 1),
      returned.get_alignment());
  for (Count i = 0; i != fields.get_size(); ++i) {
    fields.get_data()[i].read(arguments[i], variant, bytes);
  }

  const auto status = instance.invoke(
      method, input.get_extent() ? bytes : nullptr,
      returned.get_extent() ? results : nullptr);
  if (status == Ttx::Data::Status::Success) {
    const auto outputs = returned.get_fields();
    if (outputs.get_size() == 1) {
      outputs.get_data()[0].put(results, output, variant);
    } else if (variant) {
      godot::gdextension_interface::variant_new_nil(output);
    }
  }

  return status;
}

static void ptrcall(
    void* method,
    void* instance,
    const GDExtensionConstTypePtr* arguments,
    GDExtensionTypePtr output) {
  const auto& description =
      *static_cast<const Godot::Extension::Classes::Method*>(method);
  if (invoke(
          description,
          *static_cast<Godot::Extension::Classes::Instance*>(instance),
          arguments, output, false) != Ttx::Data::Status::Success) {
    // Godot's ptrcall has no status channel. Its caller already has an output
    // type, so construct that type's empty value and report the failed call.
    const auto fields = description.get_outputs().get_fields();
    if (fields.get_size() == 1) {
      const auto type = fields.get_data()[0].get_info().type;
      godot::gdextension_interface::variant_get_ptr_constructor(
          static_cast<GDExtensionVariantType>(type), 0)(output, nullptr);
    } else if (fields.get_size() > 1) {
      *static_cast<godot::Array*>(output) = godot::Array();
    }

    godot::UtilityFunctions::push_error(
        "TTX invocation failed: ", description.get_name());
  }
}

static void call(
    void* method,
    void* instance,
    const GDExtensionConstVariantPtr* arguments,
    GDExtensionInt count,
    GDExtensionVariantPtr output,
    GDExtensionCallError* error) {
  const auto& description =
      *static_cast<const Godot::Extension::Classes::Method*>(method);
  const auto fields = description.get_inputs().get_fields();
  *error = {};
  if (count != GDExtensionInt(fields.get_size())) {
    error->error = count < GDExtensionInt(fields.get_size())
                       ? GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS
                       : GDEXTENSION_CALL_ERROR_TOO_MANY_ARGUMENTS;
    error->expected = fields.get_size();
    godot::gdextension_interface::variant_new_nil(output);
    return;
  }

  for (Count i = 0; i != fields.get_size(); ++i) {
    const auto type = fields.get_data()[i].get_info().type;
    if (!godot::gdextension_interface::variant_can_convert_strict(
            godot::gdextension_interface::variant_get_type(arguments[i]),
            static_cast<GDExtensionVariantType>(type))) {
      error->error = GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT;
      error->argument = i;
      error->expected = type;
      godot::gdextension_interface::variant_new_nil(output);
      return;
    }
  }

  if (invoke(
          description,
          *static_cast<Godot::Extension::Classes::Instance*>(instance),
          arguments, output, true) != Ttx::Data::Status::Success) {
    error->error = GDEXTENSION_CALL_ERROR_INVALID_METHOD;
    godot::gdextension_interface::variant_new_nil(output);
  }
}

static auto property(const godot::PropertyInfo& value)
    -> GDExtensionPropertyInfo {
  return {
    static_cast<GDExtensionVariantType>(value.type),
    value.name._native_ptr(),
    value.class_name._native_ptr(),
    value.hint,
    value.hint_string._native_ptr(),
    value.usage};
}

auto Godot::Extension::Classes::Method::publish(
    const godot::StringName& class_name) -> void {
  // Godot copies property records during registration. Method userdata is the
  // retained address, and Class keeps this vector stationary until unregister.
  Memory::Global::Vector<GDExtensionPropertyInfo> properties;
  Memory::Global::Vector<GDExtensionClassMethodArgumentMetadata> metadata;
  const auto fields = inputs.get_fields();
  for (Count index = 0; index != fields.get_size(); ++index) {
    properties.insert(property(fields.get_data()[index].get_info()));
    metadata.insert(GDEXTENSION_METHOD_ARGUMENT_METADATA_NONE);
  }

  const auto results = outputs.get_fields();
  godot::PropertyInfo returned;
  if (results.get_size() == 1) {
    returned = results.get_data()[0].get_info();
  } else if (results.get_size() > 1) {
    returned = godot::PropertyInfo(godot::Variant::ARRAY, "");
  }

  auto returned_info = property(returned);
  GDExtensionClassMethodInfo info = {};
  info.name = name._native_ptr();
  info.method_userdata = this;
  info.call_func = call;
  info.ptrcall_func = ptrcall;
  info.method_flags = GDEXTENSION_METHOD_FLAG_NORMAL;
  info.has_return_value = results.get_size() != 0;
  info.return_value_info = info.has_return_value ? &returned_info : nullptr;
  info.argument_count = properties.get_size();
  info.arguments_info = properties.get_data();
  info.arguments_metadata = metadata.get_data();
  godot::gdextension_interface::classdb_register_extension_class_method(
      godot::gdextension_interface::library, class_name._native_ptr(), &info);
}
