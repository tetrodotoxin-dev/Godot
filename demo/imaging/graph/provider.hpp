// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/utility/result.hpp"

#include "demo/imaging/contracts/provider.h"
#include "demo/imaging/graph/vocabulary.hpp"
#include "ttx/concept/modules/module.hpp"

namespace Godot::Demo::Imaging::Graph {

// A source can come from a loaded module or an existing object in the host.
// Provider retains the supplied factory independently of how it was acquired.
// Every Image retains this owner, so factory state survives the Resources that
// first selected it and the callable bindings borrowed by their Calls.
//
// Native loading also transfers a Module here. The factory is released before
// that optional code owner is destroyed. A host factory instead retains its own
// state and relies on the enclosing host's executable lifetime. Neither route
// asks Data or Semantic to infer ownership from a successful binding.
class Provider {
 public:
  // Admission consumes one factory reference. Its release operation owns the
  // supplying state. Code that can unload independently travels with it as a
  // Module, while a factory embedded in Godot uses the host lifetime.
  static auto adopt(
      image_provider owned,
      const Vocabulary& vocabulary,
      Perimortem::Core::Option<Ttx::Concept::Modules::Module> module = {})
      -> Provider&;

  // The host resolves imports before image acquisition. Taking the acquired
  // module keeps its executable lifetime with the factory and its images,
  // even after the project's import configuration changes.
  static auto open(
      Ttx::Concept::Modules::Module module,
      const Vocabulary& contract,
      Perimortem::Memory::Allocator::Arena& errors)
      -> Perimortem::Utility::Result<Provider&, Perimortem::Core::View::Bytes>;

  static auto open(
      Perimortem::Core::View::Bytes path,
      const Vocabulary& contract,
      Perimortem::Memory::Allocator::Arena& errors)
      -> Perimortem::Utility::Result<Provider&, Perimortem::Core::View::Bytes>;

  auto statistics() const -> image_provider_statistics;
  void retain();
  void release();
  auto get_vocabulary() const -> const Vocabulary& { return contract; }
  auto create(
      U32 width,
      U32 height,
      Perimortem::Core::View::Bytes pixels,
      image_object& output) const -> image_error;

 private:
  Provider(
      image_provider factory,
      const Vocabulary& contract,
      Perimortem::Core::Option<Ttx::Concept::Modules::Module> module);
  ~Provider();

  Perimortem::Core::Option<Ttx::Concept::Modules::Module> module;
  image_provider factory;
  const Vocabulary& contract;
};

}  // namespace Godot::Demo::Imaging::Graph
