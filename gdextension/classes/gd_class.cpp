// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "gdextension/classes/gd_class.hpp"

#include <godot_cpp/core/class_db.hpp>

#include "perimortem/core/null_terminated.hpp"

#include "gdextension/classes/class.hpp"
#include "ttx/concept/declarations/extensible.hpp"

using namespace Gdextension;
using namespace Perimortem;

auto Gdextension::Classes::GDClass::get_interface() const
    -> Ttx::Concept::Abstract {
  static const ttx_abstract_ops operations = {
    [](const void* source, perimortem_uuid id) -> ttx_binding_status {
      const System::Uuid contract(id);
      if (contract == Ttx::Concept::Abstract::contract_id ||
          contract == ::Gdextension::Contracts::GDClass::contract_id) {
        return TTX_BINDING_SATISFIED;
      }

      return static_cast<ttx_binding_status>(
          static_cast<const GDClass*>(source)->subject.supports(contract));
    },
    [](const void* source, perimortem_uuid id,
       ttx_storage requested) -> ttx_binding_status {
      const auto& policy = *static_cast<const GDClass*>(source);
      if (System::Uuid(id) == Ttx::Concept::Abstract::contract_id) {
        const ttx_abstract api = {source, &operations};
        return ttx_binding_provide(
            ttx_abstract_representation(), &api, requested);
      }

      if (System::Uuid(id) == ::Gdextension::Contracts::GDClass::contract_id) {
        static const godot_gdclass_operations own = {
          [](const void* source, ttx_publication* output) -> ttx_data_status {
            return static_cast<const GDClass*>(source)->emit().visit(
                [&](Ttx::Semantic::Ownership::Publication& value)
                    -> ttx_data_status {
                  *output = value.take();
                  return TTX_DATA_SUCCESS;
                },
                [](Ttx::Data::Status status) {
                  return static_cast<ttx_data_status>(status);
                });
          },
        };
        return static_cast<ttx_binding_status>(
            Ttx::Semantic::Negotiation::Binding::provide<
                ::Gdextension::Contracts::GDClass>(
                godot_gdclass(source, &own),
                Ttx::Data::Form::Storage(requested)));
      }

      return static_cast<ttx_binding_status>(policy.subject.get_query().bind(
          System::Uuid(id), Ttx::Data::Form::Storage(requested)));
    },
    [](const void* source) {
      auto value = static_cast<const GDClass*>(source)->subject.get_data();
      return perimortem_view_bytes{value.get_data(), value.get_size()};
    },
    [](const void* source) {
      return static_cast<const GDClass*>(source)->get_interface().get_abi();
    },
    [](const void* source, perimortem_view_bytes name) {
      return static_cast<const GDClass*>(source)
          ->subject.resolve_concept({name.data, name.size})
          .get_abi();
    },
    [](const void* source, ttx_concept_visitor visitor) {
      const auto value = static_cast<const GDClass*>(source)->subject.get_abi();
      value.operations->visit_concepts(value.source, visitor);
    },
  };
  return Ttx::Concept::Abstract(this, operations);
}

// A provisional or rejected declaration is not an absent method. Preserve
// that distinction until the compiler reports why this class cannot publish.
static auto declaration_failure(
    Ttx::Semantic::Negotiation::Binding::Failure failure) -> Ttx::Data::Status {
  switch (failure) {
  case Ttx::Semantic::Negotiation::Binding::Failure::Unsupported:
    return Ttx::Data::Status::Unsupported;
  case Ttx::Semantic::Negotiation::Binding::Failure::Pending:
    return Ttx::Data::Status::Busy;
  case Ttx::Semantic::Negotiation::Binding::Failure::Rejected:
    return Ttx::Data::Status::Denied;
  }

  return Ttx::Data::Status::Invalid;
}

static auto compile_member(
    Core::View::Bytes name,
    Ttx::Concept::Abstract member,
    U32 index)
    -> Utility::Result<Gdextension::Classes::Method, Ttx::Data::Status> {
  using Result =
      Utility::Result<Gdextension::Classes::Method, Ttx::Data::Status>;
  return member.bind<Ttx::Concept::Declarations::Callable>().visit(
      [&](Ttx::Concept::Declarations::Callable callable) -> Result {
        return callable.describe().visit(
            [&](Ttx::Concept::Declarations::Callable::Description description)
                -> Result {
              return Gdextension::Classes::Method::compile(
                  name, description, index);
            },
            [](Ttx::Semantic::Negotiation::Binding::Failure failure) -> Result {
              return declaration_failure(failure);
            });
      },
      [](Ttx::Semantic::Negotiation::Binding::Failure failure) -> Result {
        return declaration_failure(failure);
      });
}

static auto compile_members(Ttx::Concept::Abstract subject) -> Utility::Result<
    Memory::Dynamic::Vector<Gdextension::Classes::Method>,
    Ttx::Data::Status> {
  Memory::Dynamic::Vector<Gdextension::Classes::Method> methods;
  auto status = Ttx::Data::Status::Success;
  auto visitor = [&](Core::View::Bytes name, Ttx::Concept::Abstract member) {
    if (status != Ttx::Data::Status::Success) {
      return;
    }

    // A declaration can expose configuration and metadata beside its methods.
    // Discovery asks which children promise a callable, then binding still has
    // to establish the actual interface. A policy refusal remains
    // authoritative.
    const auto support =
        member.supports<Ttx::Concept::Declarations::Callable>();
    if (support == Ttx::Semantic::Negotiation::Binding::Status::Unsupported) {
      return;
    }

    if (support != Ttx::Semantic::Negotiation::Binding::Status::Satisfied) {
      status = declaration_failure(
          static_cast<Ttx::Semantic::Negotiation::Binding::Failure>(support));
      return;
    }

    status = compile_member(name, member, methods.get_size())
                 .visit(
                     [&](Gdextension::Classes::Method& method) {
                       if (methods.get_view().contains(
                               [&](const Gdextension::Classes::Method& other) {
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

static auto retain_class(
    const Ttx::Concept::Modules::Module& module,
    Ttx::Semantic::Ownership::Publication factory,
    godot::String name,
    godot::String base,
    Memory::Dynamic::Vector<Gdextension::Classes::Method> methods) -> Utility::
    Result<Ttx::Semantic::Ownership::Publication, Ttx::Data::Status> {
  using Result =
      Utility::Result<Ttx::Semantic::Ownership::Publication, Ttx::Data::Status>;
  return factory.get_query().bind<Ttx::Semantic::Ownership::Factory>().visit(
      [&](Ttx::Semantic::Ownership::Factory constructor) -> Result {
        auto memory =
            Core::Bibliotheca::check_out(sizeof(Gdextension::Classes::Class));
        auto* output = new (memory.ptr, Core::Placement::Construct)
            Gdextension::Classes::Class(
                module, Core::Data::take(factory), constructor, name, base,
                Core::Data::take(methods));
        return Ttx::Semantic::Ownership::Publication(
            {output->get_query(), [](const void* source) {
               auto* output = const_cast<Gdextension::Classes::Class*>(
                   static_cast<const Gdextension::Classes::Class*>(source));
               output->~Class();
               Core::Bibliotheca::remit(reinterpret_cast<U8*>(output));
             }});
      },
      [](Ttx::Semantic::Negotiation::Binding::Failure failure) -> Result {
        return declaration_failure(failure);
      });
}

auto Gdextension::Classes::GDClass::emit() const -> Utility::
    Result<Ttx::Semantic::Ownership::Publication, Ttx::Data::Status> {
  using Result =
      Utility::Result<Ttx::Semantic::Ownership::Publication, Ttx::Data::Status>;
  const auto class_name = godot::String::utf8(
      reinterpret_cast<const char*>(name.get_data()), name.get_size());
  const auto base_name = godot::String::utf8(
      reinterpret_cast<const char*>(base.get_data()), base.get_size());
  if (name.is_empty() || godot::ClassDB::class_exists(class_name) ||
      !godot::ClassDB::class_exists(base_name)) {
    error = "Class name is occupied or its native base is unavailable."_view;
    return Ttx::Data::Status::Invalid;
  }

  auto api = godot::ClassDB::class_get_api_type(base_name);
  if ((api != godot::ClassDB::API_CORE && api != godot::ClassDB::API_EDITOR) ||
      !godot::ClassDB::can_instantiate(base_name)) {
    error = "GDClass requires an instantiable native Godot base."_view;
    return Ttx::Data::Status::Unsupported;
  }

  // Bind before following another route. A resolved referent may be more
  // permissive than the policy through which this declaration was exported.
  return subject.bind<Ttx::Concept::Declarations::Extensible>().visit(
      [&](Ttx::Concept::Declarations::Extensible extensible) -> Result {
        return compile_members(subject).visit(
            [&](Memory::Dynamic::Vector<Method>& methods) -> Result {
              // Only after every declaration has a supported adapter do we
              // acquire runtime state. The terminal receives copies of the
              // registration facts and an independent factory/code lifetime.
              return extensible.emit_factory().visit(
                  [&](Ttx::Semantic::Ownership::Publication& factory)
                      -> Result {
                    return retain_class(
                               module, Core::Data::take(factory), class_name,
                               base_name, Core::Data::take(methods))
                        .visit(
                            [](Ttx::Semantic::Ownership::Publication& terminal)
                                -> Result {
                              return Core::Data::take(terminal);
                            },
                            [&](Ttx::Data::Status failure) -> Result {
                              error =
                                  "The emitted runtime does not supply Factory."_view;
                              return failure;
                            });
                  },
                  [&](Ttx::Data::Status failure) -> Result {
                    error =
                        "The declaration could not emit an independent factory."_view;
                    return failure;
                  });
            },
            [&](Ttx::Data::Status failure) -> Result {
              error =
                  "A member has no supported synchronous callable realization."_view;
              return failure;
            });
      },
      [&](Ttx::Semantic::Negotiation::Binding::Failure failure) -> Result {
        switch (failure) {
        case Ttx::Semantic::Negotiation::Binding::Failure::Unsupported:
          error = "The export is not an Extensible class."_view;
          return Ttx::Data::Status::Unsupported;
        case Ttx::Semantic::Negotiation::Binding::Failure::Pending:
          error = "The exported class is still pending."_view;
          return Ttx::Data::Status::Busy;
        case Ttx::Semantic::Negotiation::Binding::Failure::Rejected:
          error = "The exported policy rejected class exposure."_view;
          return Ttx::Data::Status::Denied;
        }

        return Ttx::Data::Status::Invalid;
      });
}
