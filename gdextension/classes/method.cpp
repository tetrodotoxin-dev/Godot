// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "gdextension/classes/method.hpp"

#include <godot_cpp/godot.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "gdextension/classes/instance.hpp"

using namespace Gdextension;
using namespace Perimortem;
using Gdextension::Values::Field;

Gdextension::Classes::Method::Method(
    Core::View::Bytes name,
    System::Uuid contract,
    U32 index,
    Gdextension::Values::Frame inputs,
    Gdextension::Values::Frame outputs)
    : name(
          godot::String::utf8(
              reinterpret_cast<const char*>(name.get_data()),
              name.get_size())),
      contract(contract),
      index(index),
      inputs(Core::Data::take(inputs)),
      outputs(Core::Data::take(outputs)) {}

auto Gdextension::Classes::Method::compile(
    Core::View::Bytes name,
    Ttx::Concept::Declarations::Callable::Description description,
    U32 index) -> Utility::Result<Method, Ttx::Data::Status> {
  using Result = Utility::Result<Method, Ttx::Data::Status>;
  return Gdextension::Values::Frame::compile(description.get_inputs())
      .visit(
          [&](Gdextension::Values::Frame& inputs) -> Result {
            return Gdextension::Values::Frame::compile(
                       description.get_outputs())
                .visit(
                    [&](Gdextension::Values::Frame& outputs) -> Result {
                      return Method(
                          name, description.get_contract(), index,
                          Core::Data::take(inputs), Core::Data::take(outputs));
                    },
                    [](Ttx::Data::Status status) -> Result { return status; });
          },
          [](Ttx::Data::Status status) -> Result { return status; });
}

static auto align_frame(void* source, Count alignment) -> U8* {
  const Count address = reinterpret_cast<Count>(source);
  const Count offset = (-address) & (alignment - 1);
  return static_cast<U8*>(source) + offset;
}

static auto invoke(
    const Gdextension::Classes::Method& method,
    Gdextension::Classes::Instance& instance,
    const void* const* arguments,
    void* output,
    bool variant) -> Ttx::Data::Status {
  const auto& input = method.get_inputs();
  const auto& returned = method.get_outputs();
  const auto fields = input.get_fields();

  // The concrete frames are small call storage, not persistent instance state.
  // Stack allocation preserves nested synchronous calls without a lock, shared
  // scratch buffer or heap allocation per dispatch. The allocation also honors
  // the published aggregate alignment, which may exceed scalar alignment.
  // Text owners live until result
  // conversion, because a provider may return a view of an input string.
  auto* bytes = align_frame(
      __builtin_alloca(input.get_extent() + input.get_alignment() - 1),
      input.get_alignment());
  auto* results = align_frame(
      __builtin_alloca(returned.get_extent() + returned.get_alignment() - 1),
      returned.get_alignment());
  auto* borrows = static_cast<Field::Borrow*>(
      __builtin_alloca(fields.get_size() * sizeof(Field::Borrow)));
  for (Count i = 0; i != fields.get_size(); ++i) {
    new (borrows + i, Core::Placement::Construct) Field::Borrow();
    fields.get_data()[i].read(arguments[i], variant, bytes, borrows[i]);
  }

  const auto status = instance.get_invocation(method.get_index())
                          .invoke(
                              input.get_extent() ? bytes : nullptr,
                              returned.get_extent() ? results : nullptr);
  if (status == Ttx::Data::Status::Success) {
    const auto outputs = returned.get_fields();
    if (outputs.get_size() == 1) {
      outputs.get_data()[0].put(results, output, variant);
    } else if (outputs.get_size() > 1) {
      godot::Array values;
      values.resize(outputs.get_size());
      for (Count i = 0; i != outputs.get_size(); ++i) {
        values[i] = outputs.get_data()[i].value(results);
      }

      if (variant) {
        godot::internal::
            gdextension_interface_get_variant_from_type_constructor(
                GDEXTENSION_VARIANT_TYPE_ARRAY)(output, values._native_ptr());
      } else {
        *static_cast<godot::Array*>(output) = values;
      }
    } else if (variant) {
      godot::internal::gdextension_interface_variant_new_nil(output);
    }
  }

  for (Count i = fields.get_size(); i != 0; --i) {
    borrows[i - 1].~Borrow();
  }

  return status;
}

static void ptrcall(
    void* method,
    void* instance,
    const GDExtensionConstTypePtr* arguments,
    GDExtensionTypePtr output) {
  const auto& description =
      *static_cast<const Gdextension::Classes::Method*>(method);
  if (invoke(
          description, *static_cast<Gdextension::Classes::Instance*>(instance),
          arguments, output, false) != Ttx::Data::Status::Success) {
    // Godot's ptrcall has no status channel. Its caller already has an output
    // type, so construct that type's empty value and report the failed call.
    const auto fields = description.get_outputs().get_fields();
    if (fields.get_size() == 1) {
      const auto type = fields.get_data()[0].get_info().type;
      godot::internal::gdextension_interface_variant_get_ptr_constructor(
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
      *static_cast<const Gdextension::Classes::Method*>(method);
  const auto fields = description.get_inputs().get_fields();
  *error = {};
  if (count != GDExtensionInt(fields.get_size())) {
    error->error = count < GDExtensionInt(fields.get_size())
                       ? GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS
                       : GDEXTENSION_CALL_ERROR_TOO_MANY_ARGUMENTS;
    error->expected = fields.get_size();
    godot::internal::gdextension_interface_variant_new_nil(output);
    return;
  }

  for (Count i = 0; i != fields.get_size(); ++i) {
    const auto type = fields.get_data()[i].get_info().type;
    if (!godot::internal::gdextension_interface_variant_can_convert_strict(
            godot::internal::gdextension_interface_variant_get_type(
                arguments[i]),
            static_cast<GDExtensionVariantType>(type))) {
      error->error = GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT;
      error->argument = i;
      error->expected = type;
      godot::internal::gdextension_interface_variant_new_nil(output);
      return;
    }
  }

  if (invoke(
          description, *static_cast<Gdextension::Classes::Instance*>(instance),
          arguments, output, true) != Ttx::Data::Status::Success) {
    error->error = GDEXTENSION_CALL_ERROR_INVALID_METHOD;
    godot::internal::gdextension_interface_variant_new_nil(output);
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

auto Gdextension::Classes::Method::publish(
    const godot::StringName& class_name) const -> void {
  // Godot copies property records during registration. Method userdata is the
  // retained address, and Class keeps this vector stationary until unregister.
  Memory::Dynamic::Vector<GDExtensionPropertyInfo> properties;
  Memory::Dynamic::Vector<GDExtensionClassMethodArgumentMetadata> metadata;
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
  info.method_userdata = const_cast<Method*>(this);
  info.call_func = call;
  info.ptrcall_func = ptrcall;
  info.method_flags = GDEXTENSION_METHOD_FLAG_NORMAL;
  info.has_return_value = results.get_size() != 0;
  info.return_value_info = info.has_return_value ? &returned_info : nullptr;
  info.argument_count = properties.get_size();
  info.arguments_info = properties.get_data();
  info.arguments_metadata = metadata.get_data();
  godot::internal::
      gdextension_interface_classdb_register_extension_class_method(
          godot::internal::library, class_name._native_ptr(), &info);
}
