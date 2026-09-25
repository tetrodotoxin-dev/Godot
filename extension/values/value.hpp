// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <godot_cpp/godot.hpp>
#include <godot_cpp/variant/variant.hpp>

#include "perimortem/core/view/bytes.h"

namespace Godot::Extension::Values {

// Only the terminal converts Godot carriers. Provider calls use the declared
// TTX C carriers, and a temporary text observation remains alive through return
// conversion so an operation may safely return a view into its input.
template <typename Type>
class Value {
 public:
  static constexpr auto kind =
      __is_same(Type, S64) ? godot::Variant::INT : godot::Variant::BOOL;
  Value(const void* input, bool variant) {
    if (variant) {
      const auto& value = *static_cast<const godot::Variant*>(input);
      if constexpr (__is_same(Type, S64)) {
        scalar = int64_t(value);
      } else {
        scalar = bool(value);
      }
    } else {
      scalar = *static_cast<const Carrier*>(input);
    }
  }

  auto get() const -> Type { return Type(scalar); }
  static auto put(Type result, void* output, bool variant) -> void {
    Carrier value = Carrier(result);
    if (variant) {
      godot::gdextension_interface::get_variant_from_type_constructor(
          static_cast<GDExtensionVariantType>(kind))(output, &value);
    } else {
      *static_cast<Carrier*>(output) = value;
    }
  }

 private:
  using Carrier = decltype([] {
    if constexpr (__is_same(Type, S64)) {
      return int64_t();
    } else {
      return bool();
    }
  }());
  Carrier scalar;
};

template <>
class Value<perimortem_view_bytes> {
 public:
  static constexpr auto kind = godot::Variant::STRING;
  Value(const void* input, bool variant)
      : text((variant
                  ? godot::String(*static_cast<const godot::Variant*>(input))
                  : *static_cast<const godot::String*>(input))
                 .utf8()) {}
  auto get() const -> perimortem_view_bytes {
    return {reinterpret_cast<const U8*>(text.get_data()), U64(text.length())};
  }

  static auto put(perimortem_view_bytes result, void* output, bool variant)
      -> void {
    auto text = godot::String::utf8(
        reinterpret_cast<const char*>(result.data), result.size);
    if (variant) {
      godot::gdextension_interface::get_variant_from_type_constructor(
          GDEXTENSION_VARIANT_TYPE_STRING)(output, text._native_ptr());
    } else {
      *static_cast<godot::String*>(output) = text;
    }
  }

 private:
  godot::CharString text;
};

}  // namespace Godot::Extension::Values
