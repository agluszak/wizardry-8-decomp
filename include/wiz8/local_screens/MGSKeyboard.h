#pragma once

#include "input.h"
#include "wiz8/vector.h"
#include "wiz8/engine_code/stHash.hpp"

#pragma pack(push, 1)
struct MGSKeyBinding {
    unsigned short key;
    unsigned short modifiers;
    /* Serialized with the record and always written 1 when LoadDefaults
       creates the binding; no retail reader remains. */
    unsigned short active;
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
    /* Match a live InputAtom against the binding table; returns command or -1. */
    int FindCommandForEvent(const InputAtom* event) const; /* 0x0055D2A0 */
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

/* The command numbers MGSKeyBinding::command carries and DispatchMGSCommand
   switches on; Data\Strings\MGSKeyboard.INI binds keys to them and the
   options keymap labels them from the string table. Movement commands are
   polled via IsCommandPressed rather than dispatched. The shift-registered
   movement variants carry the run modifier; commands 1100+ are unbound
   debug entries. */
enum W8MGSCommand {
    W8_MGS_COMMAND_CANCEL = 0,
    W8_MGS_COMMAND_QUIT_GAME = 100,
    W8_MGS_COMMAND_TOGGLE_FULLSCREEN = 101,
    W8_MGS_COMMAND_LOAD_OPTIONS = 102,
    W8_MGS_COMMAND_SAVE_OPTIONS = 103,
    W8_MGS_COMMAND_COMBAT_DELAY_UP = 104,
    W8_MGS_COMMAND_COMBAT_DELAY_DOWN = 105,
    W8_MGS_COMMAND_TEXT_DELAY_UP = 106,
    W8_MGS_COMMAND_TEXT_DELAY_DOWN = 107,
    W8_MGS_COMMAND_SHOW_VERSION = 108,
    W8_MGS_COMMAND_MOVE_FORWARD = 200,
    W8_MGS_COMMAND_MOVE_FORWARD_RUN = 201,
    W8_MGS_COMMAND_MOVE_BACKWARD = 202,
    W8_MGS_COMMAND_MOVE_BACKWARD_RUN = 203,
    W8_MGS_COMMAND_TURN_LEFT = 204,
    W8_MGS_COMMAND_TURN_LEFT_ALT = 205,
    W8_MGS_COMMAND_TURN_RIGHT = 206,
    W8_MGS_COMMAND_TURN_RIGHT_ALT = 207,
    W8_MGS_COMMAND_STRAFE_LEFT = 208,
    W8_MGS_COMMAND_STRAFE_LEFT_RUN = 209,
    W8_MGS_COMMAND_STRAFE_RIGHT = 210,
    W8_MGS_COMMAND_STRAFE_RIGHT_RUN = 211,
    W8_MGS_COMMAND_LOOK_UP = 212,
    W8_MGS_COMMAND_LOOK_DOWN = 213,
    W8_MGS_COMMAND_LOOK_LEVEL = 214,
    W8_MGS_COMMAND_PAUSE_GAME = 300,
    W8_MGS_COMMAND_NEXT_LAYOUT = 301,
    W8_MGS_COMMAND_PREV_LAYOUT = 302,
    W8_MGS_COMMAND_AUTOMAP = 303,
    W8_MGS_COMMAND_OPTIONS = 304,
    W8_MGS_COMMAND_JOURNAL = 305,
    W8_MGS_COMMAND_INVENTORY = 306,
    W8_MGS_COMMAND_USE_ITEM = 307,
    W8_MGS_COMMAND_USE_LAST_ITEM = 308,
    W8_MGS_COMMAND_CAST_SPELL = 309,
    W8_MGS_COMMAND_CAST_LAST_SPELL = 310,
    W8_MGS_COMMAND_CAMP = 311,
    W8_MGS_COMMAND_TOGGLE_SEARCH = 312,
    W8_MGS_COMMAND_TOGGLE_COMBAT = 313,
    W8_MGS_COMMAND_QUICK_SAVE = 314,
    W8_MGS_COMMAND_QUICK_LOAD = 315,
    W8_MGS_COMMAND_RADAR_ZOOM = 316,
    W8_MGS_COMMAND_REPLAY_QUOTE = 317,
    W8_MGS_COMMAND_SWAP_WEAPONS = 318,
    W8_MGS_COMMAND_SWAP_ALL_WEAPONS = 319,
    W8_MGS_COMMAND_SELECT_RECRUITED_1 = 400,
    W8_MGS_COMMAND_SELECT_RECRUITED_2 = 401,
    W8_MGS_COMMAND_SELECT_PC_1 = 402,
    W8_MGS_COMMAND_SELECT_PC_2 = 403,
    W8_MGS_COMMAND_SELECT_PC_3 = 404,
    W8_MGS_COMMAND_SELECT_PC_4 = 405,
    W8_MGS_COMMAND_SELECT_PC_5 = 406,
    W8_MGS_COMMAND_SELECT_PC_6 = 407,
    W8_MGS_COMMAND_TEXTBOX_PAGE_UP = 500,
    W8_MGS_COMMAND_TEXTBOX_PAGE_DOWN = 501,
    W8_MGS_COMMAND_TEXTBOX_TOP = 502,
    W8_MGS_COMMAND_TEXTBOX_BOTTOM = 503,
    W8_MGS_COMMAND_TEXTBOX_CLEAR = 504,
    W8_MGS_COMMAND_START_COMBAT_ROUND = 600,
    W8_MGS_COMMAND_CONTINUOUS_COMBAT = 601,
    W8_MGS_COMMAND_CAMERA_LOCK = 602,
    W8_MGS_COMMAND_CYCLE_TARGET = 603,
    W8_MGS_COMMAND_ATTACK = 604,
    W8_MGS_COMMAND_BERSERK = 605,
    W8_MGS_COMMAND_BREATHE = 606,
    W8_MGS_COMMAND_TURN_UNDEAD = 607,
    W8_MGS_COMMAND_PRAY = 608,
    W8_MGS_COMMAND_DEFEND = 609,
    W8_MGS_COMMAND_PROTECT = 610,
    W8_MGS_COMMAND_EQUIP = 611,
    W8_MGS_COMMAND_PARTY_WALK = 612,
    W8_MGS_COMMAND_PARTY_RUN = 613,
    W8_MGS_COMMAND_REPEAT_ACTION = 614,
    W8_MGS_COMMAND_DEBUG_AUDIT_QUOTES = 1109,
    W8_MGS_COMMAND_DEBUG_NUMERIC_HP = 1110,
    W8_MGS_COMMAND_DEBUG_INJECT_CLICK = 1111,
    W8_MGS_COMMAND_TOGGLE_AUTO_ADVANCE = 1112,
    W8_MGS_COMMAND_TEXTBOX_SCROLL_UP = 1113,
    W8_MGS_COMMAND_TEXTBOX_SCROLL_DOWN = 1114,
    W8_MGS_COMMAND_DEBUG_TOGGLE_FLAG_271 = 1115,
    W8_MGS_COMMAND_DEBUG_MONSTER_SCRIPT = 1130
};

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
extern const int g_keyboard_row_positions[13][2];

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
void RedrawKeyboardMenuPanel(unsigned char invalidate); /* 0x005936F0 */
/* Region callback the thirteen keyboard-menu rows share. */
unsigned char KeyboardMenuRowRegionEvent(const InputAtom* event, W8Region* region); /* 0x00594760 */
/* Region callback for the keyboard-menu background: right-up closes the menu. */
unsigned char KeyboardMenuBackgroundRegionEvent(const InputAtom* event,
                                                W8Region* region); /* 0x005949A0 */
/* Dispatch a bound MGS command number through the keyboard system. */
void DispatchMGSCommand(int command); /* 0x00591960 */
/* Route one non-mouse input atom to text entry, dialogue, the trap text box,
   the MIPE editor, the record-mode console or a bound MGS command. */
unsigned char HandleMainGameInputEvent(const InputAtom* input); /* 0x00591890 */
