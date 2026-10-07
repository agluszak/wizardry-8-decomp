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
    /* Always written 1 when LoadDefaults creates the binding; never read. */
    unsigned short active;
    W8MGSCommand command;
};
#pragma pack(pop)

static_assert(sizeof(MGSKeyBinding) == 0x0a, "MGSKeyBinding_size");

// VTABLE: WIZ8 0x005ee8f0
class MGSKeyboard {
public:
    virtual ~MGSKeyboard();

    int FindBinding(W8MGSCommand command) const;
    /* Match a live InputAtom against the binding table; returns command or -1. */
    W8MGSCommand FindCommandForEvent(const InputAtom* event) const;
    MGSKeyBinding* GetBinding(int index) const;
    bool IsCommandPressed(W8MGSCommand command) const;
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
/* Each keyboard-menu row's (W8SubMenuPage, entry) selection. */
extern short g_keyboard_menu_items[12];
extern Controls* g_keyboard_menu_panel;
extern short g_keyboard_menu_pages[12];
extern W8TextControl* g_keyboard_menu_rows[13];
/* The (x, y) of the twelve menu rows and the trailing close row. */
extern srVector2i g_keyboard_row_positions[13];

void ResetMGSKeyboardBindings();

/* Discard every queued input atom (screen-entry stale-input flush). */
void DrainInputEventQueue(void);
/* Reset the slot's combat selection and tear down the menu panel and rows. */
inline void CloseKeyboardMenu(void);
/* Open the keyboard-action menu for one party slot. */
void OpenKeyboardMenuForSlot(int slot);
/* Build the panel and one row per selectable menu entry. */
unsigned char BuildKeyboardMenu(void);
/* Re-enable the menu's region set and every row region. */
void EnableKeyboardMenuInput(void);
/* Whether the cursor sits inside the menu panel rectangle. */
bool KeyboardMenuContainsCursor(void);
/* Re-evaluate every row's availability and restate its icon frames. */
void RefreshKeyboardMenuRows(void);
/* Install the row's primary callback for its (menu, item) pair. */
/* Install the row's primary callback for its (W8SubMenuPage, entry) pair. */
void AssignKeyboardMenuCallback(short menu, short item, W8TextControl* row);
/* Invalidate (when asked) then redraw the menu panel. */
void RedrawKeyboardMenuPanel(bool invalidate);
/* Region callback the thirteen keyboard-menu rows share. */
unsigned char KeyboardMenuRowRegionEvent(const InputAtom* event, W8Region* region);
/* Region callback for the keyboard-menu background: right-up closes the menu. */
unsigned char KeyboardMenuBackgroundRegionEvent(const InputAtom* event, W8Region* region);
/* Dispatch a bound MGS command number through the keyboard system. */
void DispatchMGSCommand(W8MGSCommand command);
/* Route one non-mouse input atom to text entry, dialogue, the trap text box,
   the MIPE editor, the record-mode console or a bound MGS command. */
unsigned char HandleMainGameInputEvent(const InputAtom* input);
/* Poll the camera/movement command bindings and set the world-render flags. */
void HandleManualCameraHotkeys(void);
void ApplyCameraMotionHotkeys(void);
