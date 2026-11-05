// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "godot/classes/method.hpp"

#include "godot/classes/class.hpp"
#include "godot/classes/instance.hpp"
#include "godot_cpp/godot.hpp"
#include "godot_cpp/variant/array.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "ttx/graph/capabilities/describe.hpp"

using namespace Godot::Extension;
using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Data;
using namespace Ttx::Graph;

auto Classes::Method::compile(
    Abstract declaration,
    Values::Field::Resolver resolver,
    const Abstract* receiver) -> Storage::Option<Method> {
  using Option = Storage::Option<Method>;
  Option result;
  declaration.bind<Capabilities::Describe>().visit(
      [&](Capabilities::Describe describe) {
        describe.get_name().visit(
            [&](View::Bytes label) {
              const auto name = godot::String::utf8(
                  Data::cast<const char>(label.get_data()), label.get_size());
              if (!name.is_valid_identifier()) {
                return;
              }

              declaration.bind<Coherence::TypeSystem::Callable::Contract>()
                  .visit(
                      [&](Coherence::TypeSystem::Callable::Contract callable) {
                        const auto compiled =
                            receiver
                                ? Coherence::Compilation::Compiler::
                                      compile_member(*receiver, declaration)
                                : Coherence::Compilation::Compiler::
                                      compile_callable(declaration);
                        compiled.visit(
                            [&](const Coherence::Compilation::Executable&
                                    executable) {
                              Values::Frame::arguments(
                                  callable, receiver, executable.get_inputs(),
                                  resolver)
                                  .visit(
                                      [&](Values::Frame& inputs) {
                                        Values::Frame::result(
                                            callable, executable.get_output(),
                                            resolver)
                                            .visit(
                                                [&](Values::Frame& outputs) {
                                                  Storage::Option<Abstract>
                                                      selected;
                                                  if (receiver) {
                                                    selected = *receiver;
                                                  }

                                                  result = Method(
                                                      name, declaration,
                                                      selected,
                                                      Data::take(inputs),
                                                      Data::take(outputs));
                                                },

                                                [] {});
                                      },

                                      [] {});
                            },

                            [](Status) {});
                      },

                      [] {});
            },

            [] {});
      },

      [] {});
  return result;
}

auto Classes::Method::compile() const
    -> Coherence::Compilation::Compiler::Result<
        Coherence::Compilation::Executable> {
  return receiver.visit(
      [&](Ttx::Graph::Abstract type) {
        return Coherence::Compilation::Compiler::compile_member(type, callable);
      },

      [&] {
        return Coherence::Compilation::Compiler::compile_callable(callable);
      });
}

static auto align_frame(void* source, Count alignment) -> U8* {
  return Data::cast<U8>(__builtin_align_up(source, alignment));
}

static auto invoke(
    const Classes::Method& method,
    Classes::Instance& instance,
    const void* const* arguments,
    void* output,
    bool variant) -> Status {
  const auto& input = method.get_inputs();
  const auto& returned = method.get_outputs();
  const auto fields = input.get_fields();
  godot::PackedByteArray byte_arguments[64];

  // Validated frames contain at most 64 arguments and occupy at most 64 KiB.
  // Independent stack storage keeps nested synchronous calls from sharing
  // payload bytes. Persistent represented values are copied out of this frame.
  auto* bytes = align_frame(
      __builtin_alloca(input.get_extent() + input.get_alignment() - 1),
      input.get_alignment());
  auto* results = align_frame(
      __builtin_alloca(returned.get_extent() + returned.get_alignment() - 1),
      returned.get_alignment());
  const Count first = method.has_receiver() ? 1 : 0;
  if (first) {
    const auto value = instance.get_value();
    if (!fields.get_size() ||
        value.get_size() !=
            fields.get_data()[0].get_representation().get_extent() ||
        !instance.get_type().get_representation().compatible(
            fields.get_data()[0].get_representation())) {
      return Status::Incompatible;
    }

    Data::copy(
        bytes + fields.get_data()[0].get_offset(), value.get_data(),
        value.get_size());
  }

  for (Count i = first; i != fields.get_size(); ++i) {
    if (!fields.get_data()[i].read(
            arguments[i - first], variant, bytes, byte_arguments[i])) {
      return Status::Incompatible;
    }
  }

  const auto status = instance.invoke(
      method, input.get_extent() ? bytes : nullptr,
      returned.get_extent() ? results : nullptr);
  if (status == Status::Success) {
    const auto outputs = returned.get_fields();
    if (outputs.get_size() == 1) {
      if (!outputs.get_data()[0].put(results, output, variant)) {
        return Status::Overflow;
      }
    } else if (variant) {
      godot::gdextension_interface::variant_new_nil(output);
    }
  }

  return status;
}

// Object results use null when Invocation returns Denied. GDScript must check
// that result before calling its methods. This lets a script handle creation
// refusal during provider selection. Signature and argument errors continue
// through Godot's call error channel.
static auto refused_object(const Classes::Method& method, Status status)
    -> bool {
  const auto fields = method.get_outputs().get_fields();
  return status == Status::Denied && fields.get_size() == 1 &&
         fields.get_data()[0].get_info().type == godot::Variant::OBJECT;
}

static auto ptrcall(
    void* method,
    void* instance,
    const GDExtensionConstTypePtr* arguments,
    GDExtensionTypePtr output) -> void {
  const auto& description = *Data::cast<const Classes::Method>(method);
  const auto status = invoke(
      description, *Data::cast<Classes::Instance>(instance), arguments, output,
      false);
  if (status != Status::Success) {
    // Godot's ptrcall has no status channel. Its caller already has an output
    // type, so construct that type's empty value and report the failed call.
    const auto fields = description.get_outputs().get_fields();
    if (fields.get_size() == 1) {
      const auto type = fields.get_data()[0].get_info().type;
      godot::gdextension_interface::variant_get_ptr_constructor(
          GDExtensionVariantType(type), 0)(output, nullptr);
    } else if (fields.get_size() > 1) {
      *Data::cast<godot::Array>(output) = godot::Array();
    }

    if (!refused_object(description, status)) {
      godot::UtilityFunctions::push_error(
          "TTX invocation failed: ", description.get_name());
    }
  }
}

static auto call(
    void* method,
    void* instance,
    const GDExtensionConstVariantPtr* arguments,
    GDExtensionInt count,
    GDExtensionVariantPtr output,
    GDExtensionCallError* error) -> void {
  const auto& description = *Data::cast<const Classes::Method>(method);
  const auto fields = description.get_inputs().get_fields();
  *error = {};

  const Count first = description.has_receiver() ? 1 : 0;
  const Count expected = fields.get_size() - first;
  if (count != GDExtensionInt(expected)) {
    error->error = count < GDExtensionInt(expected)
                       ? GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS
                       : GDEXTENSION_CALL_ERROR_TOO_MANY_ARGUMENTS;
    error->expected = expected;
    godot::gdextension_interface::variant_new_nil(output);
    return;
  }

  for (Count i = 0; i != expected; ++i) {
    const auto type = fields.get_data()[i + first].get_info().type;
    if (!godot::gdextension_interface::variant_can_convert_strict(
            godot::gdextension_interface::variant_get_type(arguments[i]),
            GDExtensionVariantType(type))) {
      error->error = GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT;
      error->argument = i;
      error->expected = type;
      godot::gdextension_interface::variant_new_nil(output);
      return;
    }
  }

  const auto status = invoke(
      description, *Data::cast<Classes::Instance>(instance), arguments, output,
      true);
  if (status != Status::Success) {
    if (!refused_object(description, status)) {
      error->error = GDEXTENSION_CALL_ERROR_INVALID_METHOD;
      godot::UtilityFunctions::push_error(
          "TTX invocation failed: ", description.get_name(), " status ",
          U8(status));
    }

    godot::gdextension_interface::variant_new_nil(output);
  }
}

static auto property(const godot::PropertyInfo& value)
    -> GDExtensionPropertyInfo {
  return {GDExtensionVariantType(value.type), value.name._native_ptr(),
          value.class_name._native_ptr(),     value.hint,
          value.hint_string._native_ptr(),    value.usage};
}

auto Classes::Method::publish(const godot::StringName& class_name) -> void {
  // Godot copies property records during registration. Method userdata is the
  // retained address, and Class keeps this vector stationary until unregister.
  Global::Vector<GDExtensionPropertyInfo> properties;
  Global::Vector<GDExtensionClassMethodArgumentMetadata> metadata;
  const auto fields = inputs.get_fields();
  for (Count index = receiver ? 1 : 0; index != fields.get_size(); ++index) {
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
