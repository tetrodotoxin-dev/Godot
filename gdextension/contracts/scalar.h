// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef GODOT_CONTRACTS_SCALAR_H
#define GODOT_CONTRACTS_SCALAR_H

#include "ttx/concept/abstract.h"
#include "ttx/data/form/representation.h"

#define LAB_SCALAR_ID_HIGH 0xd47c73cc6f444f5aULL
#define LAB_BOOLEAN_ID_LOW 0x83b8ac5e73e41311ULL
#define LAB_INTEGER_ID_LOW 0x83b8ac5e73e41312ULL
#define LAB_REAL_ID_LOW 0x83b8ac5e73e41313ULL
#define LAB_TEXT_ID_LOW 0x83b8ac5e73e41314ULL
#define LAB_BUFFER_ID_HIGH LAB_SCALAR_ID_HIGH
#define LAB_BUFFER_ID_LOW 0x83b8ac5e73e41315ULL

// These are the lab's value promises, independent of its terminals. Boolean
// uses a U8 containing zero or one, Integer uses S64, Real uses R64, and Text
// uses a borrowed UTF8 byte view. Their UUIDs distinguish meaning from byte
// geometry. Another terminal may consume them without linking Godot.
//
// Kind is a convenience for this provider's authoring, not a core TTX type
// inventory. Consumers ask the published Abstract for the role they need.
// Within these roles, get_data supplies the declaration label. The terminal
// can use it as a parameter name after binding a role. Unqualified Abstract
// data carries no such text or naming promise.
typedef U8 lab_scalar_kind;
#define LAB_SCALAR_BOOLEAN ((lab_scalar_kind)1)
#define LAB_SCALAR_INTEGER ((lab_scalar_kind)2)
#define LAB_SCALAR_REAL ((lab_scalar_kind)3)
#define LAB_SCALAR_TEXT ((lab_scalar_kind)4)
#define LAB_SCALAR_EMPTY ((lab_scalar_kind)0)

typedef struct lab_scalar {
  perimortem_view_bytes name;
  lab_scalar_kind kind;
} lab_scalar;

PERIMORTEM_C ttx_abstract lab_scalar_abstract(const lab_scalar* scalar);
PERIMORTEM_C const ttx_representation* lab_scalar_representation(lab_scalar_kind kind);

#endif
