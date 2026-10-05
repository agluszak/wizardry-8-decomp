#pragma once

#include "input.h"
#include "wiz8/mgs_commands.h"
#include "surrender/srMath.h"
#include "wiz8/vector.h"
#include "wiz8/engine_code/stHash.hpp"

#pragma pack(push, 1)
struct MGSKeyBinding {
    unsigned short key;
    unsigned short modifiers;
    /* Serialized with the record and always written 1 when LoadDefaults
       creates the binding; no retail reader remains. */
    unsigned short active;
    W8MGSCommand command;
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
    virtual ~MGSKeyboard();

    int FindBinding(W8MGSCommand command) const;
    /* Match a live InputAtom against the binding table; returns command or -1. */
    W8MGSCommand FindCommandForEvent(const InputAtom* event) const; /* 0x0055D2A0 */
    MGSKeyBinding* GetBinding(int index) const;
    unsigned char IsCommandPressed(W8MGSCommand command) const;
    void Clear();
    unsigned char Load(int handle, bool clear);
    unsigned char Save(int handle) const;
    unsigned char LoadDefaults(const char* path);

private:
    W8Vector<MGSKeyBinding*> m_bindings;
    W8HashTable<unsigned int, MGSKeyBinding*> m_command_index;
};

static_assert(sizeof(MGSKeyboard) == 0x24, "MGSKeyboard_size");

extern MGSKeyboard* g_mgs_keyboard;

struct Controls;
struct W8Region;
class W8TextControl;

/* The keyboard-action menu the main screen opens for a party slot: a panel,
   twelve menu/item-keyed rows plus a trailing row, and the per-row callback
   selection the availability refresh drives. */
/* Each keyboard-menu row's (W8SubMenuPage, entry) selection; retail storage is
   word-sized, so the enum values ride in shorts. */
extern short g_keyboard_menu_items[12];
extern Controls* g_keyboard_menu_panel;
extern short g_keyboard_menu_pages[12];
extern W8TextControl* g_keyboard_menu_rows[13];
/* The (x, y) of the twelve menu rows and the trailing close row. */
extern srVector2i g_keyboard_row_positions[13];

void ResetMGSKeyboardBindings();

/* Discard every queued input atom (screen-entry stale-input flush). */
void DrainInputEventQueue(void); /* 0x0055D3C0 */
/* Reset the slot's combat selection and tear down the menu panel and rows.
   Retail inlines the whole body at all twelve MGSKeyboard.cpp call sites but
   emits real calls from MainGameScreen.cpp: the authored definition was an
   inline function visible only in its own TU, kept addressable for the
   row-callback tables. */
inline void CloseKeyboardMenu(void); /* 0x00592E60 */
/* Open the keyboard-action menu for one party slot. */
void OpenKeyboardMenuForSlot(int slot); /* 0x00592C70 */
/* Build the panel and one row per selectable menu entry. */
unsigned char BuildKeyboardMenu(void); /* 0x00592F90 */
/* Re-enable the menu's region set and every row region. */
void EnableKeyboardMenuInput(void); /* 0x005932D0 */
/* Whether the cursor sits inside the menu panel rectangle. */
bool KeyboardMenuContainsCursor(void); /* 0x00593300 */
/* Re-evaluate every row's availability and restate its icon frames. */
void RefreshKeyboardMenuRows(void); /* 0x00593360 */
/* Install the row's primary callback for its (menu, item) pair. */
/* Install the row's primary callback for its (W8SubMenuPage, entry) pair; the
   retail parameters are word-sized. */
void AssignKeyboardMenuCallback(short menu, short item, W8TextControl* row); /* 0x005935E0 */
/* Invalidate (when asked) then redraw the menu panel. */
void RedrawKeyboardMenuPanel(bool invalidate); /* 0x005936F0 */
/* Region callback the thirteen keyboard-menu rows share. */
unsigned char KeyboardMenuRowRegionEvent(const InputAtom* event, W8Region* region); /* 0x00594760 */
/* Region callback for the keyboard-menu background: right-up closes the menu. */
unsigned char KeyboardMenuBackgroundRegionEvent(const InputAtom* event,
                                                W8Region* region); /* 0x005949A0 */
/* Dispatch a bound MGS command number through the keyboard system. */
void DispatchMGSCommand(W8MGSCommand command); /* 0x00591960 */
/* Route one non-mouse input atom to text entry, dialogue, the trap text box,
   the MIPE editor, the record-mode console or a bound MGS command. */
unsigned char HandleMainGameInputEvent(const InputAtom* input); /* 0x00591890 */
/* Poll the camera/movement command bindings and set the world-render flags;
   retail 0x005929D0/0x00592A10 bounded inside the demo MGSKeyboard.cpp hull. */
void HandleManualCameraHotkeys(void);
void ApplyCameraMotionHotkeys(void);
