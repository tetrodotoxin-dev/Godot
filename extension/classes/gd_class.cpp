// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "extension/classes/gd_class.hpp"

#include <godot_cpp/core/class_db.hpp>

#include "perimortem/core/null_terminated.hpp"

#include "extension/classes/class.hpp"
#include "ttx/concept/capabilities/borrow.hpp"
#include "ttx/concept/capabilities/callable.hpp"

using namespace Godot::Extension;
using namespace Perimortem;

static auto compile_member(
    Core::View::Bytes name,
    Ttx::Concept::Abstract member,
    U32 index)
    -> Utility::Result<Godot::Extension::Classes::Method, Ttx::Data::Status> {
  using Result =
      Utility::Result<Godot::Extension::Classes::Method, Ttx::Data::Status>;
  return member.bind<Ttx::Concept::Capabilities::Callable>().visit(
      [&](Ttx::Concept::Capabilities::Callable callable) -> Result {
        return callable.describe().visit(
            [&](Ttx::Concept::Capabilities::Callable::Description description)
                -> Result {
              return Godot::Extension::Classes::Method::compile(
                  name, description, index);
            },
            [](Ttx::Semantic::Negotiation::Binding::Failure failure) -> Result {
              return failure == Ttx::Semantic::Negotiation::Binding::Failure::
                                    Unknown
                         ? Ttx::Data::Status::Unsupported
                         : Ttx::Data::Status::Denied;
            });
      },
      [](Ttx::Semantic::Negotiation::Binding::Failure failure) -> Result {
        return failure == Ttx::Semantic::Negotiation::Binding::Failure::Unknown
                   ? Ttx::Data::Status::Unsupported
                   : Ttx::Data::Status::Denied;
      });
}

static auto compile_members(Ttx::Concept::Abstract subject) -> Utility::Result<
    Memory::Dynamic::Vector<Godot::Extension::Classes::Method>,
    Ttx::Data::Status> {
  Memory::Dynamic::Vector<Godot::Extension::Classes::Method> methods;
  auto status = Ttx::Data::Status::Success;
  auto visitor = [&](Core::View::Bytes name, Ttx::Concept::Abstract member) {
    if (status != Ttx::Data::Status::Success) {
      return;
    }

    // A subject can expose configuration and metadata beside its methods.
    // The exporter selects children that accept Callable, then binds their
    // descriptions to determine the method surface it can register.
    const auto support =
        member.supports<Ttx::Concept::Capabilities::Callable>();
    if (support != Ttx::Semantic::Negotiation::Binding::Status::Satisfied) {
      return;
    }

    status =
        compile_member(name, member, methods.get_size())
            .visit(
                [&](Godot::Extension::Classes::Method& method) {
                  if (methods.get_view().contains(
                          [&](const Godot::Extension::Classes::Method& other) {
                            return other.get_name() == method.get_name();
                          })) {
                    return Ttx::Data::Status::Invalid;
                  }

                  methods.emplace(Core::Data::take(method));
                  return Ttx::Data::Status::Success;
                },
                [](Ttx::Data::Status failure) { return failure; });
  };
  subject.visit_concepts(Ttx::Concept::Abstract::Visitor(visitor));
  if (status != Ttx::Data::Status::Success) {
    return status;
  }

  return methods;
}

auto Godot::Extension::Classes::GDClass::supports(System::Uuid id) const
    -> Ttx::Semantic::Negotiation::Binding::Status {
  using namespace Ttx::Semantic::Negotiation;
  return id == Ttx::Concept::Capabilities::Export::contract_id
             ? Binding::Status::Satisfied
             : Binding::Status::Unknown;
}

auto Godot::Extension::Classes::GDClass::bind_interface(
    System::Uuid id,
    Ttx::Data::Form::Storage target) const
    -> Ttx::Semantic::Negotiation::Binding::Status {
  using namespace Ttx::Semantic::Negotiation;
  return id == Ttx::Concept::Capabilities::Export::contract_id
             ? Binding::provide<Ttx::Concept::Capabilities::Export>(
                   Ttx::Concept::Capabilities::Export::provide(*this).get_abi(),
                   target)
             : Binding::Status::Unknown;
}

auto Godot::Extension::Classes::GDClass::expose(Ttx::Concept::Abstract subject)
    const -> Ttx::Semantic::Negotiation::Binding::Status {
  using namespace Ttx::Semantic::Negotiation;
  const auto class_name = godot::String::utf8(
      reinterpret_cast<const char*>(name.get_data()), name.get_size());
  const auto base_name = godot::String::utf8(
      reinterpret_cast<const char*>(base.get_data()), base.get_size());
  if (name.is_empty() || godot::ClassDB::class_exists(class_name) ||
      !godot::ClassDB::class_exists(base_name)) {
    error = "Class name is occupied or its native base is unavailable."_view;
    return Binding::Status::Rejected;
  }
  const auto api = godot::ClassDB::class_get_api_type(base_name);
  if ((api != godot::ClassDB::API_CORE && api != godot::ClassDB::API_EDITOR) ||
      !godot::ClassDB::can_instantiate(base_name)) {
    error = "Godot class export requires an instantiable native base."_view;
    return Binding::Status::Rejected;
  }
  return compile_members(subject).visit(
      [&](Memory::Dynamic::Vector<Method>& methods) {
        // The retained runtime supplies construction independently of method
        // discovery. Its policy decides which state needs to survive the call.
        return subject.bind<Ttx::Concept::Capabilities::Borrow>().visit(
            [&](Ttx::Concept::Capabilities::Borrow policy) {
              return policy.borrow().visit(
                  [&](Ttx::Concept::Policies::Borrowed runtime) {
                    return runtime.bind<Ttx::Concept::Capabilities::Create>().visit(
                        [&](Ttx::Concept::Capabilities::Create create) {
                          auto* output = new Class(
                              runtime, create, class_name, base_name,
                              Core::Data::take(methods));
                          auto status = Binding::Status::Rejected;
                          if (output->publish() == Ttx::Data::Status::Success) {
                            registrations.insert(output);
                            return Binding::Status::Satisfied;
                          } else {
                            error =
                                "The completed class could not register with Godot."_view;
                          }
                          delete output;
                          return status;
                        },
                        [&](Binding::Failure failure) {
                          runtime.release();
                          error =
                              "The retained subject did not supply Create."_view;
                          return static_cast<Binding::Status>(failure);
                        });
                  },
                  [&](Binding::Failure failure) {
                    error =
                        "The offered subject could not retain its runtime capabilities."_view;
                    return static_cast<Binding::Status>(failure);
                  });
            },
            [&](Binding::Failure failure) {
              error = "The offered subject did not supply Borrow."_view;
              return static_cast<Binding::Status>(failure);
            });
      },
      [&](Ttx::Data::Status) {
        error = "A selected method has no supported callable realization."_view;
        return Binding::Status::Rejected;
      });
}
