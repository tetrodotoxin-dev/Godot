// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "godot/values/field.hpp"

#include "perimortem/core/view/null_terminated.hpp"

#include "coherence/compilation/compiler.hpp"
#include "godot_cpp/godot.hpp"
#include "ttx/data/form/compiled.hpp"
#include "ttx/graph/capabilities/describe.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Data;
using namespace Ttx::Graph;
using namespace Godot::Extension;

auto Values::Field::get_representation() const -> const Form::Representation& {
  return form;
}

auto Values::Field::compile_type(
    Abstract abstract,
    const godot::String& name,
    Resolver resolver) -> Storage::Option<Field> {
  Storage::Option<Field> result;
  Coherence::Compilation::Compiler::compile(abstract).visit(
      [&](Form::Representation& supplied) {
        godot::Variant::Type host = godot::Variant::NIL;
        if (supplied.compatible(
                Form::Compiled<Form::Native<perimortem_view_bytes>::reference>::
                    get_representation())) {
          host = godot::Variant::PACKED_BYTE_ARRAY;
        } else if (
            supplied.compatible(
                Form::Compiled<
                    Form::Native<S64>::reference>::get_representation()) ||
            supplied.compatible(
                Form::Compiled<
                    Form::Native<U64>::reference>::get_representation()) ||
            supplied.compatible(
                Form::Compiled<
                    Form::Native<U8>::reference>::get_representation())) {
          host = godot::Variant::INT;
        } else if (
            supplied.compatible(
                Form::Compiled<
                    Form::Native<R64>::reference>::get_representation())) {
          host = godot::Variant::FLOAT;
        }

        Materializer materializer;
        if (host == godot::Variant::NIL) {
          materializer = resolver.resolve(resolver.context, abstract, supplied);
          if (!materializer.context) {
            return;
          }

          host = godot::Variant::OBJECT;
        }

        Field field(name, host);
        field.form = Data::take(supplied);
        field.materializer = materializer;
        if (materializer.context) {
          field.info.class_name = materializer.class_name;
        }

        result = Data::take(field);
      },

      [](Status) {});
  return result;
}

auto Values::Field::compile(Abstract declaration, Resolver resolver)
    -> Storage::Option<Field> {
  Storage::Option<Field> result;
  declaration.bind<Capabilities::Describe>().visit(
      [&](Capabilities::Describe describe) {
        describe.get_name().visit(
            [&](View::Bytes label) {
              const auto name = godot::String::utf8(
                  Data::cast<const char>(label.get_data()), label.get_size());
              if (name.is_valid_identifier()) {
                result = compile_type(
                    declaration.resolve_route("type"_view), name, resolver);
              }
            },

            [] {});
      },

      [] {});
  return result;
}

auto Values::Field::result(Abstract type, Resolver resolver)
    -> Storage::Option<Field> {
  return compile_type(type, "result", resolver);
}

auto Values::Field::read(
    const void* source,
    bool variant,
    U8* frame,
    godot::PackedByteArray& bytes) const -> bool {
  if (info.type == godot::Variant::OBJECT) {
    GDExtensionObjectPtr object = nullptr;
    if (variant) {
      godot::Variant value(*Data::cast<const godot::Variant>(source));
      godot::gdextension_interface::get_variant_to_type_constructor(
          GDEXTENSION_VARIANT_TYPE_OBJECT)(&object, value._native_ptr());
    } else {
      object = *Data::cast<const GDExtensionObjectPtr>(source);
    }

    return materializer.copy(materializer.context, object, frame + offset);
  }

  if (info.type == godot::Variant::PACKED_BYTE_ARRAY) {
    bytes =
        variant
            ? godot::PackedByteArray(*Data::cast<const godot::Variant>(source))
            : *Data::cast<const godot::PackedByteArray>(source);

    const perimortem_view_bytes value{
      bytes.is_empty() ? nullptr : bytes.ptr(), Count(bytes.size())};

    Data::copy(frame + offset, Data::cast<const U8>(&value), sizeof(value));
    return true;
  }

  if (info.type == godot::Variant::INT) {
    const S64 value =
        variant ? GDExtensionInt(*Data::cast<const godot::Variant>(source))
                : *Data::cast<const GDExtensionInt>(source);
    const auto extent = form.get_extent();
    const auto unsigned_value = form.compatible(
        Form::Compiled<Form::Native<U64>::reference>::get_representation());
    if ((extent == 1 && (value < 0 || value > 255)) ||
        (unsigned_value && value < 0)) {
      return false;
    }

    Data::copy(frame + offset, Data::cast<const U8>(&value), extent);
  } else {
    const R64 value = variant ? R64(*Data::cast<const godot::Variant>(source))
                              : *Data::cast<const R64>(source);
    Data::copy(frame + offset, Data::cast<const U8>(&value), sizeof(value));
  }

  return true;
}

auto Values::Field::put(const U8* frame, void* output, bool variant) const
    -> bool {
  if (info.type == godot::Variant::OBJECT) {
    // TODO: Invocation has already produced this record. If Godot object
    // construction fails, an acquired typed result can be orphaned without its
    // required release. Materialization needs a prepared destination or an
    // ownership-aware rollback before acquired results are safe on this path.
    auto object = materializer.create(materializer.context, frame + offset);
    if (!object) {
      return false;
    }

    if (variant) {
      godot::gdextension_interface::get_variant_from_type_constructor(
          GDEXTENSION_VARIANT_TYPE_OBJECT)(output, &object);
    } else {
      *Data::cast<GDExtensionObjectPtr>(output) = object;
    }

    return true;
  }

  if (info.type == godot::Variant::PACKED_BYTE_ARRAY) {
    perimortem_view_bytes value{};
    Data::copy(Data::cast<U8>(&value), frame + offset, sizeof(value));

    const View::Bytes view(value);
    godot::PackedByteArray bytes;
    bytes.resize(view.get_size());
    if (!view.is_empty()) {
      Data::copy(bytes.ptrw(), view.get_data(), view.get_size());
    }

    if (variant) {
      const godot::Variant converted(bytes);
      godot::gdextension_interface::variant_new_copy(
          output, converted._native_ptr());
    } else {
      *Data::cast<godot::PackedByteArray>(output) = bytes;
    }

    return true;
  }

  if (info.type == godot::Variant::INT) {
    S64 value = 0;
    Data::copy(Data::cast<U8>(&value), frame + offset, form.get_extent());
    if (value < 0 &&
        form.compatible(
            Form::Compiled<
                Form::Native<U64>::reference>::get_representation())) {
      return false;
    }

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

  return true;
}
