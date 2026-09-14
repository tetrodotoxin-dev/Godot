// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "images/call.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/object.hpp"
#include "perimortem/core/writer/textual.hpp"

using namespace Godot;
using namespace Perimortem;

Images::Call::Call(
    U8* allocation,
    Expression& receiver,
    const Operation& operation,
    Core::View::Vector<Argument> arguments,
    Core::Option<Memory::Allocator::Arena> storage)
    : Expression(allocation),
      receiver(receiver),
      operation(operation),
      constants(Core::Data::take(storage)) {
  receiver.retain();
  for (const auto& argument : arguments) {
    if (bool(argument.expression) == bool(argument.constant)) {
      Core::Diagnostics::Log::fatal(
          "Image argument requires one expression or constant."_view);
    }

    if (argument.expression) {
      argument.expression->retain();
    }

    inputs.insert({argument});
  }
}

Images::Call::~Call() {
  for (const auto& input : inputs.get_view()) {
    if (input.argument.expression) {
      input.argument.expression->release();
    }
  }

  receiver.release();
}

auto Images::Call::create(
    Expression& receiver,
    const Operation& operation,
    Core::View::Vector<Argument> arguments,
    Core::Option<Memory::Allocator::Arena> constants) -> Call& {
  static const Core::Object<>::Descriptor descriptor(
      sizeof(Call), alignof(Call),
      [](U8* bytes) { reinterpret_cast<Call*>(bytes)->~Call(); });
  auto storage = Core::Object<>::create(descriptor).get_payload();
  return *new (storage, Core::Placement::Construct) Call(
      storage, receiver, operation, arguments, Core::Data::take(constants));
}

auto Images::Call::cached(Memory::Allocator::Arena& errors)
    -> Utility::Result<const Image&, Core::View::Bytes> {
  if (!error.is_empty()) {
    return errors.proxy(error.get_view());
  }

  return *output;
}

auto Images::Call::evaluate_value(Memory::Allocator::Arena& errors, U64 pull)
    -> Utility::Result<const Image&, Core::View::Bytes> {
  if (last_pull == pull) {
    return cached(errors);
  }

  if (evaluating) {
    return "Image evaluation reentered the same call."_view;
  }

  evaluating = true;
  Core::View::Bytes failed;
  const Image* target = nullptr;
  auto selected = receiver.evaluate(errors);
  selected.visit(
      [&](const Image& image) { target = &image; },
      [&](Core::View::Bytes message) { failed = message; });
  const bool receiver_changed = receiver_revision != receiver.get_revision();
  bool changed = evaluations == 0 || receiver_changed;
  receiver_revision = receiver.get_revision();
  for (Count i = 0; i < inputs.get_size(); ++i) {
    auto& input = inputs[i];
    if (!input.argument.expression) {
      continue;
    }

    auto& dependency = *input.argument.expression;
    auto result = dependency.evaluate(errors);
    result.visit(
        [](const Image&) {},
        [&](Core::View::Bytes message) {
          if (failed.is_empty()) {
            failed = message;
          }
        });
    changed = changed || input.revision != dependency.get_revision();
    input.revision = dependency.get_revision();
  }

  // Pulling dependencies establishes whether the cached answer is still
  // current. An unchanged branch keeps both its result and binding. Otherwise
  // only a changed receiver needs fulfillment again. An overlay edit can use
  // the existing compositor while supplying the new argument image.
  if (!changed) {
    last_pull = pull;
    evaluating = false;
    return cached(errors);
  }

  ++evaluations;
  if (failed.is_empty()) {
    if (!binding || receiver_changed) {
      binding = {};
      target->bind_operation(operation).visit(
          [&](const Ttx::Semantic::Binding& value) { binding = value; },
          [&](Ttx::Semantic::Binding::Failure failure) {
            failed = Image::binding_error(failure);
          });
    }

    const Kernel* kernel = nullptr;
    const Image* overlay = nullptr;
    for (const auto& input : inputs.get_view()) {
      if (input.argument.constant) {
        kernel = input.argument.constant;
      } else {
        input.argument.expression->evaluate(errors).visit(
            [&](const Image& image) { overlay = &image; },
            [&](Core::View::Bytes error) { failed = error; });
      }
    }

    if (failed.is_empty()) {
      target->apply(operation, kernel, overlay, errors, &*binding)
          .visit(
              [&](Image& image) { output = Core::Data::take(image); },
              [&](Core::View::Bytes message) { failed = message; });
    }
  }

  error = Memory::Dynamic::Bytes(failed);
  if (!failed.is_empty()) {
    output = {};
  }

  advance();
  last_pull = pull;
  evaluating = false;
  return cached(errors);
}

auto Images::Call::resolve_concept(Core::View::Bytes name) const
    -> const Abstract& {
  if (name == "operation"_view) {
    return operation;
  }

  if (name == "receiver"_view) {
    return receiver;
  }

  if (name == "value"_view && output) {
    return *output;
  }

  for (Count i = 0; i < inputs.get_size(); ++i) {
    Core::Static::Bytes<20> storage;
    Core::Writer::Textual index(storage);
    index << i;
    if (name == Core::View::Bytes(index)) {
      const auto& argument = inputs[i].argument;
      return argument.expression
                 ? static_cast<const Abstract&>(*argument.expression)
                 : *argument.constant;
    }
  }

  return Abstract::resolve_concept(name);
}

void Images::Call::visit_concepts(Visitor visitor) const {
  visitor("operation"_view, operation);
  visitor("receiver"_view, receiver);
  if (output) {
    visitor("value"_view, *output);
  }

  for (Count i = 0; i < inputs.get_size(); ++i) {
    Core::Static::Bytes<20> storage;
    Core::Writer::Textual index(storage);
    index << i;
    const auto& argument = inputs[i].argument;
    visitor(
        Core::View::Bytes(index),
        argument.expression ? static_cast<const Abstract&>(*argument.expression)
                            : *argument.constant);
  }
}
