#pragma once

/* Local Screens\MGSButtons.cpp. The combat sub-menu's panel, its five text
   rows and the two scroll-arrow buttons: construction, teardown and the
   per-frame name/action caption draw. */

struct Controls;
class W8TextControl;
class W8DialogButton;

/* The five sub-menu pages the bank buttons open; the entry message and help
   tables are indexed page * 5 + entry, and the keyboard menu keys its pages
   the same way. Names follow the entry help captions. */
enum W8SubMenuPage {
    W8_SUBMENU_ATTACK = 0, /* Attack, Berserk, Breathe, Turn Undead, Pray */
    W8_SUBMENU_DEFEND = 1, /* Defend, Protect */
    W8_SUBMENU_ITEMS = 2,  /* Equip, Use Item, Use Last Item */
    W8_SUBMENU_SPELLS = 3, /* Cast Spell, Cast Last Spell */
    W8_SUBMENU_MOVE = 4    /* Walk, Run */
};

/* The row states GetSubMenuEntryState returns. _SELECTED marks the entry that
   is already the slot's unsettled action; UNUSABLE means the action exists but
   fails the current targeting/usability check, and UNAVAILABLE means the slot
   lacks the capability entirely, so BuildSubMenuPanel creates no row for it. */
enum W8SubMenuEntryState {
    W8_SUBMENU_ENTRY_USABLE = 0,
    W8_SUBMENU_ENTRY_USABLE_SELECTED = 1,
    W8_SUBMENU_ENTRY_UNUSABLE = 2,
    W8_SUBMENU_ENTRY_UNUSABLE_SELECTED = 3,
    W8_SUBMENU_ENTRY_UNAVAILABLE = 4
};

/* Owned globals. */
extern Controls* gpSubMenuPanel;
extern W8DialogButton* g_submenu_scroll_buttons_69b858[2];
extern W8DialogButton* g_submenu_panel_buttons_69b860[2];
extern short g_submenu_entry_count_69b87e;
extern unsigned int g_submenu_clock_69b880;
extern unsigned char g_submenu_flag_69b8d4;
/* Exactly five: the next known global begins at 0x0069B900. The cancel row
   lands at [built-1] where built is the available-entry count plus one, so
   the array is only safe because W8_SUBMENU_ATTACK's Berserk (fighter trait
   0x14) and Pray (priest trait 0x0b) entries can never be available together -
   current_profession is one index into g_profession_abilities. */
extern W8TextControl* g_submenu_rows_69b8ec[5];
extern W8DialogButton* g_submenu_buttons_69b8b0[9];
extern const int g_scroll_button_positions_64c330[2][2];
/* The action-kind message indexes the caption draw maps through. */
extern const unsigned short g_action_kind_message_ids_61e988[12];
/* The (menu, item) keyed entry message/help indexes both menus build rows
   from; the keyboard menu shares them. */
extern const short g_submenu_entry_message_ids_64c548[25];
extern const int g_submenu_entry_help_ids_64c57c[25];
extern const char g_submenu_icons_path_64c238[];
extern const char g_submenu_combat_icons_path_64c260[];
extern const int g_submenu_button_positions_64c290[9][2];
extern const int g_submenu_panel_left_64c378;
/* Which menu the open sub-menu panel serves (W8SubMenuPage value), each row's
   entry index and each row's W8SubMenuEntryState; the retail storage is
   word-sized, so the enum values ride in shorts. */
extern short g_submenu_menu_69b854;
extern short g_submenu_entries_69b868[6];
extern short g_submenu_entry_states_69b874[5];

/* Drop the combat-end notification and tear down the panel and its rows. */
void DestroySubMenuControls(void); /* 0x00595570 */
/* Invalidate (when asked) then redraw the sub-menu panel. */
void RefreshSubMenuPanel(char invalidate); /* 0x005963E0 */
/* Close and rebuild the sub menu around the saved combat-end notification. */
void ResetSubMenuPanel(void); /* 0x00596CF0 */
/* Create and lay out the two scroll-arrow buttons. */
unsigned char CreateSubMenuScrollButtons(void); /* 0x00596EC0 */
/* Draw the selected character's name/profession line and the caption for
   their queued action. */
void DrawSubMenuCharacterAction(void); /* 0x00596FE0 */

/* The scroll-arrow callbacks are one-argument thunks over the shared handler:
   up steps toward lower slots, down toward higher, both wrapping and skipping
   ineligible slots. */
void SubMenuScrollArrowUp(W8DialogButton* button);   /* 0x00597550 */
void SubMenuScrollArrowDown(W8DialogButton* button); /* 0x00597560 */
/* Move selected_character to the next eligible party slot (0 steps toward
   slot 0, 1 toward slot 7), wrapping, and raise the portrait redraw masks. */
void ScrollSubMenuCharacter(char direction); /* 0x00597570 */
/* Create the nine-button bank and keep its state flag clear. */
unsigned char CreateSubMenuButtons(void); /* 0x00594AF0 */
/* Restate one bank button against the live mode flags. */
void UpdateSubMenuButton(int index); /* 0x00594D20 */
/* Drop the combat-end notification, rebuild the panel for the saved one. */
void ReopenSubMenuPanel(void); /* 0x00595600 */
/* Enable the panel region set and one input region per live row. */
void EnableSubMenuRegions(void); /* 0x005957E0 */
/* Build the panel and one row per available entry of the notification's
   menu. */
unsigned char BuildSubMenuPanel(short notification); /* 0x00595850 */
/* Install the row's primary callback for its (W8SubMenuPage, entry) pair; the
   retail parameters are word-sized. */
void AssignSubMenuCallback(W8TextControl* row, short menu, short item); /* 0x00595EA0 */
/* Store the (W8SubMenuPage, entry) pair's pending command in the level block. */
void MapSubMenuSelection(short menu, short item); /* 0x00596240 */
/* Whether the slot may perform the pending command, in entry-state terms:
   USABLE/UNUSABLE or the _SELECTED variant when it is already queued. */
W8SubMenuEntryState CheckSubMenuActionUsable(int party_slot); /* 0x00596360 */
/* The row availability states the refresh maps icon frames through. */
W8SubMenuEntryState GetSubMenuEntryState(short menu, short item,
                                         int party_slot); /* 0x00595FE0 */

/* Applies SetTooltipEnabled to all nine bank buttons, both scroll arrows and
   both panel buttons; the submenu rebuild paths call it around teardown. */
void SetSubMenuButtonTooltips(int enabled); /* 0x005990F0 */
