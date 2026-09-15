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

/* Local Screens\MGSKeyboard.cpp owns the binding vector, its command-keyed
   lookup and the singleton ResetMGSKeyboardBindings installs.
   MGSKeyboard::LoadDefaults asserts Local Code\InputMapper.cpp and is defined
   there. The serialized record is exactly ten bytes; command lookups compare
   the unaligned integer at +0x06. */
// VTABLE: WIZ8 0x005ee8f0
class MGSKeyboard {
public:
    MGSKeyboard();
    virtual ~MGSKeyboard();

    int FindBinding(int command) const;
    MGSKeyBinding* GetBinding(int index) const;
    unsigned char IsCommandPressed(unsigned int command) const;
    void Clear();
    unsigned char Load(int handle, unsigned char clear);
    unsigned char Save(int handle) const;
    unsigned char LoadDefaults(const char* path);

private:
    W8GrowableVector<MGSKeyBinding*> m_bindings;
    W8HashTable<unsigned int, MGSKeyBinding*> m_command_index;
};

static_assert(sizeof(MGSKeyboard) == 0x24, "MGSKeyboard_size");

extern MGSKeyboard* g_mgs_keyboard;

struct Controls;
class W8TextControl;

/* The keyboard-action menu the main screen opens for a party slot: a panel,
   twelve menu/item-keyed rows plus a trailing row, and the per-row callback
   selection the availability refresh drives. */
extern short g_keyboard_menu_items_69b7ec[12];
extern Controls* g_keyboard_menu_panel_69b804;
extern short g_keyboard_menu_pages_69b808[12];
extern W8TextControl* g_keyboard_menu_rows_69b820[13];
/* The (x, y) of the twelve menu rows and the trailing close row. */
extern const int g_keyboard_row_positions_64c1cc[13][2];

void ResetMGSKeyboardBindings();

void DrainInputEventQueue0055D3C0(void);
/* Reset the slot's combat selection and tear down the menu panel and rows. */
void CloseKeyboardMenu(void); /* 0x00592E60 */
/* Build the panel and one row per selectable menu entry. */
unsigned char BuildKeyboardMenu(void); /* 0x00592F90 */
/* Re-enable the menu's region set and every row region. */
void EnableKeyboardMenuInput(void); /* 0x005932D0 */
/* Whether the cursor sits inside the menu panel rectangle. */
bool KeyboardMenuContainsCursor(void); /* 0x00593300 */
/* Re-evaluate every row's availability and restate its icon frames. */
void RefreshKeyboardMenuRows(void); /* 0x00593360 */
/* Install the row's primary callback for its (menu, item) pair. */
void AssignKeyboardMenuCallback(short menu, short item, W8TextControl* row); /* 0x005935E0 */
/* Invalidate (when asked) then redraw the menu panel. */
void RedrawKeyboardMenuPanel(unsigned char invalidate); /* 0x005936F0 */
