#pragma once

#include "wiz8/vector.h"
#include "wiz8/engine_code/stHash.hpp"

#pragma pack(push, 1)
struct MGSKeyBinding {
    unsigned short key;
    unsigned short modifiers;
    unsigned short unknown_004;
    int command;
};
#pragma pack(pop)

static_assert(sizeof(MGSKeyBinding) == 0x0a, "MGSKeyBinding_size");

/* Local Screens\MGSKeyboard.cpp owns the binding vector and its command-keyed
   lookup. The serialized record is exactly ten bytes; command lookups compare
   the unaligned integer at +0x06. */
class MGSKeyboard {
public:
    virtual ~MGSKeyboard();

    int FindBinding(int command) const;
    MGSKeyBinding* GetBinding(int index) const;

private:
    W8GrowableVector<MGSKeyBinding*> m_bindings;
    W8HashTable<unsigned int, MGSKeyBinding*> m_command_index;
};

static_assert(sizeof(MGSKeyboard) == 0x24, "MGSKeyboard_size");

extern MGSKeyboard* g_mgs_keyboard;

void ResetMGSKeyboardBindings();
