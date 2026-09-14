// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "extension/ttx_sampler.hpp"

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>

using namespace Godot;
using namespace Perimortem;

void Extension::TtxSampler::_bind_methods() {
  godot::ClassDB::bind_method(
      godot::D_METHOD("configure", "provider"), &TtxSampler::configure);
  godot::ClassDB::bind_method(
      godot::D_METHOD("count", "seed", "first", "size"), &TtxSampler::count);
  godot::ClassDB::bind_method(
      godot::D_METHOD("get_error"), &TtxSampler::get_error);
}

auto Extension::TtxSampler::configure(const godot::String& provider) -> bool {
  godot::String path;
  godot::internal::gdextension_interface_get_library_path(
      godot::internal::library, path._native_ptr());
  path = provider.contains("/")
             ? provider
             : path.get_base_dir().path_join("lib" + provider + "_provider.so");
  const auto encoded =
      godot::ProjectSettings::get_singleton()->globalize_path(path).utf8();
  Memory::Allocator::Arena errors;
  return Sampling::Function::open(
             Core::View::Bytes(
                 reinterpret_cast<const U8*>(encoded.get_data()),
                 encoded.length()),
             errors)
      .visit(
          [&](Sampling::Function& value) {
            function = Core::Data::take(value);
            error = godot::String();
            return true;
          },
          [&](Core::View::Bytes message) {
            error = godot::String::utf8(
                reinterpret_cast<const char*>(message.get_data()),
                message.get_size());
            return false;
          });
}

auto Extension::TtxSampler::count(int64_t seed, int64_t first, int64_t size)
    -> int64_t {
  if (!function) {
    error = "Configure a sampling provider before calling it.";
    return -1;
  }

  if (seed < 0 || seed > 0xffffffffLL || first < 0 || first > 0xffffffffLL ||
      size < 0 || size > 0xffffffffLL) {
    error = "Sampling arguments must be unsigned 32-bit integers.";
    return -1;
  }

  return function->get_handle()
      .count(U32(seed), U32(first), U32(size))
      .visit(
          [&](U64 hits) -> int64_t {
            if (!error.is_empty()) {
              error = godot::String();
            }

            return int64_t(hits);
          },
          [&](Ttx::Data::Status status) -> int64_t {
            error = status == Ttx::Data::Status::Bounds
                        ? "Sampling interval exceeds the 32-bit domain."
                        : "Sampling provider could not complete the operation.";
            return -1;
          });
}
