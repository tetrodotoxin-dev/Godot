// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/utility/result.hpp"

namespace Godot::Modules {

// A callable address remains usable only while its executable module is loaded.
// Library owns that operating system resource, independently of any image or
// semantic contract found inside it. Moving this owner transfers the obligation
// to keep the code available and eventually close it.
//
// Acquiring an address proves neither its type nor its calling convention.
// Those belong to the entry contract that the caller already agreed to use.
// Symbols borrow the Library. A provider keeps it beside its factory until all
// retained images and Calls release that provider. Counting copied function
// pointers would not establish ownership, so borrowed bindings stay unchanged.
class Library {
 public:
  static auto open(
      Perimortem::Core::View::Bytes path,
      Perimortem::Memory::Allocator::Arena& errors)
      -> Perimortem::Utility::Result<Library, Perimortem::Core::View::Bytes>;

  Library(Library&& source);
  Library(const Library&) = delete;
  auto operator=(const Library&) -> Library& = delete;
  ~Library();

  auto symbol(
      Perimortem::Core::View::Bytes name,
      Perimortem::Memory::Allocator::Arena& errors) const
      -> Perimortem::Utility::Result<void*, Perimortem::Core::View::Bytes>;

 private:
  explicit Library(void* handle) : handle(handle) {}
  void* handle;
};

}  // namespace Godot::Modules
