// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/object.hpp"

#include "perimortem/utility/result.hpp"

#include "demo/imaging/contracts/image.h"

namespace Godot::Demo::Imaging::Graph {

// A graph selects operations at runtime, while each negotiated contract has
// its own native record type. Invocation owns that typed record and the host
// adapter chosen when the operation was registered. Its one allocation occurs
// when binding the receiver. Repeated image observations reuse the same code
// and record without negotiation or allocating dispatch state.
//
// The graph retains the supplying image and module separately. This owner
// preserves the acquired API, not the private receiver's lifetime. The cast in
// its adapter recovers our own typed allocation, never a provider's receiver.
class Invocation {
 public:
  using Result =
      Perimortem::Utility::Result<image_object, Perimortem::Core::View::Bytes>;

  template <typename Contract, typename Adapter>
  static auto create(Contract contract, Adapter) -> Invocation {
    static_assert(__is_empty(Adapter));
    static constexpr Perimortem::Core::Object<>::Descriptor descriptor(
        sizeof(Contract), alignof(Contract), [](U8* value) {
          Perimortem::Core::Data::cast<Contract>(value)->~Contract();
        });
    auto storage = Perimortem::Core::Object<>::create(descriptor);
    new (storage.get_payload(), Perimortem::Core::Placement::Construct)
        Contract(Perimortem::Core::Data::take(contract));
    return Invocation(
        storage,
        [](const void* value, const image_kernel* kernel,
           const image_object* image) -> Result {
          return Adapter()(*static_cast<const Contract*>(value), kernel, image);
        });
  }

  Invocation(const Invocation& other)
      : storage(other.storage), call(other.call) {
    storage.retain();
  }

  Invocation(Invocation&& other) : storage(other.storage), call(other.call) {
    other.storage = Perimortem::Core::Object<>();
  }

  auto operator=(const Invocation& other) -> Invocation& {
    Invocation retained(other);
    return *this = Perimortem::Core::Data::take(retained);
  }

  auto operator=(Invocation&& other) -> Invocation& {
    Perimortem::Core::Data::swap(storage, other.storage);
    Perimortem::Core::Data::swap(call, other.call);
    return *this;
  }

  ~Invocation() { storage.release(); }

  auto invoke(const image_kernel* kernel, const image_object* image) const
      -> Result {
    return call(storage.get_payload(), kernel, image);
  }

 private:
  using Call =
      Result (*)(const void*, const image_kernel*, const image_object*);
  Invocation(Perimortem::Core::Object<> storage, Call call)
      : storage(storage), call(call) {}

  Perimortem::Core::Object<> storage;
  Call call;
};

}  // namespace Godot::Demo::Imaging::Graph
