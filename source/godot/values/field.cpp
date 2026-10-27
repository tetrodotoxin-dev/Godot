// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "godot/values/field.hpp"

#include "perimortem/core/view/null_terminated.hpp"

#include "ttx/data/form/compiled.hpp"
#include "ttx/graph/capabilities/describe.hpp"

#include "godot_cpp/godot.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Data;
using namespace Ttx::Graph;
using namespace Godot::Extension;

auto Values::Field::get_representation() const -> const Form::Representation& {
  return info.type == godot::Variant::INT
             ? Form::Compiled<
                   Form::Native<S64>::reference>::get_representation()
             : Form::Compiled<
                   Form::Native<R64>::reference>::get_representation();
}

auto Values::Field::compile_type(Abstract abstract, const godot::String& name)
    -> Storage::Option<Field> {
  Storage::Option<Field> result;
  abstract.bind<Capabilities::Type>().visit(
      [&](Capabilities::Type type) {
        type.get_representation().visit(
            [&](const Form::Representation& form) {
              if (form.compatible(
                      Form::Compiled<Form::Native<S64>::reference>::
                          get_representation())) {
                result = Field(name, godot::Variant::INT);
              } else if (
                  form.compatible(
                      Form::Compiled<Form::Native<R64>::reference>::
                          get_representation())) {
                result = Field(name, godot::Variant::FLOAT);
              }
            },

            [] {});
      },

      [] {});
  return result;
}

auto Values::Field::compile(Abstract declaration) -> Storage::Option<Field> {
  Storage::Option<Field> result;
  declaration.bind<Capabilities::Describe>().visit(
      [&](Capabilities::Describe describe) {
        describe.get_name().visit(
            [&](View::Bytes label) {
              const auto name = godot::String::utf8(
                  Data::cast<const char>(label.get_data()), label.get_size());
              if (name.is_valid_identifier()) {
                result =
                    compile_type(declaration.resolve_route("type"_view), name);
              }
            },

            [] {});
      },

      [] {});
  return result;
}

auto Values::Field::result(Abstract type) -> Storage::Option<Field> {
  return compile_type(type, "result");
}

auto Values::Field::read(const void* source, bool variant, U8* frame) const
    -> void {
  if (info.type == godot::Variant::INT) {
    const S64 value = variant ? GDExtensionInt(*Data::cast<const godot::Variant>(source))
                              : *Data::cast<const GDExtensionInt>(source);
    Data::copy(frame + offset, Data::cast<const U8>(&value), sizeof(value));
  } else {
    const R64 value = variant ? R64(*Data::cast<const godot::Variant>(source))
                              : *Data::cast<const R64>(source);
    Data::copy(frame + offset, Data::cast<const U8>(&value), sizeof(value));
  }
}

auto Values::Field::put(const U8* frame, void* output, bool variant) const
    -> void {
  if (info.type == godot::Variant::INT) {
    S64 value;
    Data::copy(Data::cast<U8>(&value), frame + offset, sizeof(value));
    if (variant) {
      const godot::Variant converted{GDExtensionInt(value)};
      godot::gdextension_interface::variant_new_copy(
          output, converted._native_ptr());
    } else {
      *Data::cast<GDExtensionInt>(output) = value;
    }
  } else {
    R64 value;
    Data::copy(Data::cast<U8>(&value), frame + offset, sizeof(value));
    if (variant) {
      const godot::Variant converted{R64(value)};
      godot::gdextension_interface::variant_new_copy(
          output, converted._native_ptr());
    } else {
      *Data::cast<R64>(output) = value;
    }
  }
}
