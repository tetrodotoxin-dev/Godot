// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/null_terminated.hpp"

#include "imaging/contracts/composite.hpp"
#include "imaging/contracts/convolve.hpp"
#include "imaging/contracts/invert.hpp"
#include "imaging/graph/invocation.hpp"
#include "ttx/semantic/negotiation/query.hpp"

namespace Godot::Imaging::Graph {

// Operation connects a Godot method name and input family to a particular TTX
// contract. Registration captures the typed adapter, so two methods with the
// same Godot arguments can still require different callable representations.
// Binding produces an Invocation containing the actual negotiated contract.
// Neither the provider nor TTX needs to understand these host argument
// families.
class Operation {
 public:
  enum class Input { None, Kernel, Image };
  using Acquisition = Perimortem::Utility::
      Result<Invocation, Ttx::Semantic::Negotiation::Binding::Failure>;

  template <typename Contract>
  static constexpr auto unary(Perimortem::Core::View::Bytes name) -> Operation {
    return create<Contract, Input::None>(name);
  }

  template <typename Contract>
  static constexpr auto convolution(Perimortem::Core::View::Bytes name)
      -> Operation {
    return create<Contract, Input::Kernel>(name);
  }

  template <typename Contract>
  static constexpr auto binary(Perimortem::Core::View::Bytes name)
      -> Operation {
    return create<Contract, Input::Image>(name);
  }

  // Discovery can supply another behavioral UUID using an existing image call
  // family. The complete API representation is still checked at acquisition.
  static constexpr auto described(
      Perimortem::Core::View::Bytes name,
      Perimortem::System::Uuid id,
      Input input) -> Operation {
    switch (input) {
    case Input::Kernel:
      return create<Imaging::Contracts::Convolve, Input::Kernel>(name, id);
    case Input::Image:
      return create<Imaging::Contracts::Composite, Input::Image>(name, id);
    case Input::None:
      return create<Imaging::Contracts::Invert, Input::None>(name, id);
    }
    return create<Imaging::Contracts::Invert, Input::None>(name, id);
  }

  constexpr auto get_data() const -> Perimortem::Core::View::Bytes {
    return name;
  }
  constexpr auto get_id() const -> Perimortem::System::Uuid { return id; }
  constexpr auto get_input() const -> Input { return input; }
  auto get_representation() const -> const Ttx::Data::Form::Representation& {
    return representation();
  }

  auto bind(Ttx::Semantic::Negotiation::Query query) const -> Acquisition {
    return acquire(query, id);
  }

 private:
  using Representation = const Ttx::Data::Form::Representation& (*)();
  using Acquire = Acquisition (*)(
      Ttx::Semantic::Negotiation::Query,
      Perimortem::System::Uuid);

  // Registration selects the argument adapter. Binding still requests this
  // operation's UUID together with its complete API form, so sharing an
  // argument carrier never substitutes one behavioral promise for another.
  template <typename Contract, Input input>
  static constexpr auto create(
      Perimortem::Core::View::Bytes name,
      Perimortem::System::Uuid id = Contract::contract_id) -> Operation {
    return Operation(
        name, id, input, Contract::get_representation,
        [](Ttx::Semantic::Negotiation::Query query,
           Perimortem::System::Uuid id) -> Acquisition {
          typename Contract::Api api = {};
          const auto& form = Contract::get_representation();
          const auto status = query.bind(
              id, Ttx::Data::Form::Storage(
                      {&form, reinterpret_cast<U8*>(&api), sizeof(api)}));
          if (status !=
              Ttx::Semantic::Negotiation::Binding::Status::Satisfied) {
            return static_cast<Ttx::Semantic::Negotiation::Binding::Failure>(
                status);
          }
          return Invocation::create(
              Contract(api),
              [](const Contract& contract, const image_kernel* kernel,
                 const image_object* image) -> Invocation::Result {
                if constexpr (input == Input::Kernel) {
                  if (!kernel) {
                    return "Image operation requires a kernel."_view;
                  }

                  return contract.apply(*kernel);
                } else if constexpr (input == Input::Image) {
                  if (!image) {
                    return "Image operation requires a second image."_view;
                  }

                  return contract.apply(*image);
                } else {
                  return contract.apply();
                }
              });
        });
  }

  constexpr Operation(
      Perimortem::Core::View::Bytes name,
      Perimortem::System::Uuid id,
      Input input,
      Representation representation,
      Acquire acquire)
      : name(name),
        id(id),
        input(input),
        representation(representation),
        acquire(acquire) {}

  Perimortem::Core::View::Bytes name;
  Perimortem::System::Uuid id;
  Input input;
  Representation representation;
  Acquire acquire;
};

}  // namespace Godot::Imaging::Graph
