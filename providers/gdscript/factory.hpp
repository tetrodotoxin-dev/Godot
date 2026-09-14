// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/callable.hpp>

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/utility/result.hpp"

#include "contracts/provider.h"

namespace Godot::Providers::Gdscript {

// GDScript already has an object and code lifetime supplied by Godot. Factory
// admits that existing RefCounted object as the same C factory used by native
// modules. It holds a strong reference through every source creation, while
// each returned image independently retains its script implementation.
//
// Loading a .gd resource is one convenience entry into that admission. Tests
// and applications can publish an existing object without a path or a loader.
// This owner depends on Godot, but has no dependency on the host image graph or
// either native image implementation.
class Factory {
 public:
  static auto open(
      const godot::String& path,
      Perimortem::Memory::Allocator::Arena& errors) -> Perimortem::Utility::
      Result<image_provider, Perimortem::Core::View::Bytes>;
  static auto create(const godot::Ref<godot::RefCounted>& object)
      -> Perimortem::Utility::
          Result<image_provider, Perimortem::Core::View::Bytes>;

 private:
  Factory(U8* allocation, const godot::Ref<godot::RefCounted>& object);
  U8* allocation;
  godot::Ref<godot::RefCounted> object;
  godot::Callable creation;
  mutable Perimortem::Memory::Dynamic::Bytes errors;
};

}  // namespace Godot::Providers::Gdscript
