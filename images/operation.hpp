// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/null_terminated.hpp"

#include "contracts/image.h"
#include "ttx/concept/abstract.hpp"
#include "ttx/data/form/representation.hpp"
#include "ttx/semantic/binding.hpp"

namespace Godot::Images {

// Godot needs a name and an argument form to register a method, while TTX needs
// the exact contract that the selected provider will fulfill. Operation keeps
// those two descriptions together. Its factories capture the typed invocation
// at publication, so two methods with identical arguments can still request and
// invoke different contracts without being mistaken for the same operation.
//
// This is host registration metadata. Providers only consume the public C
// contracts, and neither Data nor Semantic needs to understand these Godot
// argument families. A new family belongs with the host conversion that can
// present it to GDScript.
class Operation : public Ttx::Concept::Abstract {
 public:
  enum class Input { None, Kernel, Image };

  template <typename Contract>
  static constexpr auto unary(Perimortem::Core::View::Bytes name) -> Operation {
    return Operation(
        name, Contract::contract_id, Input::None, Contract::get_representation,
        [](const Ttx::Semantic::Binding& binding, const image_kernel*,
           const image_object*) -> Result {
          return binding.get<Contract>().apply();
        });
  }

  template <typename Contract>
  static constexpr auto convolution(Perimortem::Core::View::Bytes name)
      -> Operation {
    return Operation(
        name, Contract::contract_id, Input::Kernel,
        Contract::get_representation,
        [](const Ttx::Semantic::Binding& binding, const image_kernel* kernel,
           const image_object*) -> Result {
          if (!kernel) {
            return "Image operation requires a kernel."_view;
          }

          return binding.get<Contract>().apply(*kernel);
        });
  }

  template <typename Contract>
  static constexpr auto binary(Perimortem::Core::View::Bytes name)
      -> Operation {
    return Operation(
        name, Contract::contract_id, Input::Image, Contract::get_representation,
        [](const Ttx::Semantic::Binding& binding, const image_kernel*,
           const image_object* image) -> Result {
          if (!image) {
            return "Image operation requires a second image."_view;
          }

          return binding.get<Contract>().apply(*image);
        });
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return name;
  }

  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return Ttx::Concept::Documentation::get_empty();
  }

  constexpr auto get_id() const -> Perimortem::System::Uuid { return id; }
  constexpr auto get_input() const -> Input { return input; }
  auto get_representation() const -> const Ttx::Data::Form::Representation& {
    return representation();
  }

  auto invoke(
      const Ttx::Semantic::Binding& binding,
      const image_kernel* kernel,
      const image_object* image) const -> Perimortem::Utility::
      Result<image_object, Perimortem::Core::View::Bytes> {
    return invocation(binding, kernel, image);
  }

 private:
  using Result =
      Perimortem::Utility::Result<image_object, Perimortem::Core::View::Bytes>;
  using Representation = const Ttx::Data::Form::Representation& (*)();
  using Invocation = Result (*)(
      const Ttx::Semantic::Binding&,
      const image_kernel*,
      const image_object*);

  constexpr Operation(
      Perimortem::Core::View::Bytes name,
      Perimortem::System::Uuid id,
      Input input,
      Representation representation,
      Invocation invocation)
      : name(name),
        id(id),
        input(input),
        representation(representation),
        invocation(invocation) {}

  Perimortem::Core::View::Bytes name;
  Perimortem::System::Uuid id;
  Input input;
  Representation representation;
  Invocation invocation;
};

}  // namespace Godot::Images
