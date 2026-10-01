// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef GODOT_CONTRACTS_SCALAR_H
#define GODOT_CONTRACTS_SCALAR_H

#include "ttx/concept/abstract.h"
#include "ttx/data/form/representation.h"

#define GODOT_SCALAR_ID_HIGH 0xd47c73cc6f444f5aULL
#define GODOT_BOOLEAN_ID_LOW 0x83b8ac5e73e41311ULL
#define GODOT_INTEGER_ID_LOW 0x83b8ac5e73e41312ULL
#define GODOT_REAL_ID_LOW 0x83b8ac5e73e41313ULL
#define GODOT_TEXT_ID_LOW 0x83b8ac5e73e41314ULL
#define GODOT_BUFFER_ID_HIGH GODOT_SCALAR_ID_HIGH
#define GODOT_BUFFER_ID_LOW 0x83b8ac5e73e41315ULL

// Describes fields that the Godot exporter can project into Variant values.
// Boolean uses a U8 containing zero or one, Integer uses S64, Real uses R64,
// and Text and Buffer use borrowed byte views. Text interprets its bytes as
// UTF8 while Buffer preserves them as PackedByteArray data. The role UUID
// identifies that meaning separately from the carrier's representation.
//
// A field's get_data supplies its parameter or result label after the exporter
// accepts the role. The provider keeps the label and descriptor available for
// that observation. Kind is an authoring convenience for these field records.
// Other providers can publish the same roles through their own Abstracts.
typedef U8 godot_scalar_kind;
#define GODOT_SCALAR_BOOLEAN ((godot_scalar_kind)1)
#define GODOT_SCALAR_INTEGER ((godot_scalar_kind)2)
#define GODOT_SCALAR_REAL ((godot_scalar_kind)3)
#define GODOT_SCALAR_TEXT ((godot_scalar_kind)4)
#define GODOT_SCALAR_BUFFER ((godot_scalar_kind)5)
#define GODOT_SCALAR_EMPTY ((godot_scalar_kind)0)

typedef struct godot_scalar {
  perimortem_view_bytes name;
  godot_scalar_kind kind;
} godot_scalar;

PERIMORTEM_C ttx_abstract godot_scalar_abstract(const godot_scalar* scalar);
PERIMORTEM_C const ttx_representation* godot_scalar_representation(godot_scalar_kind kind);

#endif
