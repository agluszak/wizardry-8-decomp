#pragma once

/* Local Screens\MGSPortraitCombat.cpp. The portrait combat sub-menu's bank
   buttons, its action rows and the (menu, entry) state tables: button-bank
   construction, panel building, per-row callbacks and entry usability. */

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

/* Owned globals: the icon paths, button positions and (menu, item)-keyed
   entry message/help tables emitted by this translation unit. */
extern const char g_submenu_icons_path_64c238[];
extern const char g_submenu_combat_icons_path_64c260[];
extern const char g_options_disk_path_64c388[];
extern const char g_attack_confirm_path_64c3b8[];
extern const char g_combat_stop_path_64c3e0[];
extern const char g_cont_start_path_64c404[];
extern const char g_cont_toggle_path_64c428[];
extern const char g_cont_pending_path_64c44c[];
extern const char g_roof_buttons_path_64c4a0[];
extern const char g_layout_arrows_path_64c4e8[];
extern const int g_submenu_button_positions_64c290[9][2];
extern const int g_scroll_button_positions_64c330[2][2];
extern const int g_submenu_panel_button_positions_64c378[2][2];
extern const int g_options_disk_position_64c3b0[2];
extern const int g_combat_stance_positions_64c478[5][2];
extern const int g_roof_button_positions_64c4d0[3][2];
extern const int g_layout_arrow_positions_64c518[6][2];
/* The (menu, item) keyed entry message/help indexes both menus build rows
   from; the keyboard menu shares them. */
extern const short g_submenu_entry_message_ids_64c548[25];
extern const int g_submenu_entry_help_ids_64c57c[25];

/* Create the nine-button bank and keep its state flag clear. */
unsigned char CreateSubMenuButtons(void); /* 0x00594AF0 */
/* Restate one bank button against the live mode flags. */
void UpdateSubMenuButton(int index); /* 0x00594D20 */
/* Drop the combat-end notification, rebuild the panel for the saved one. */
void ReopenSubMenuPanel(void); /* 0x00595600 */
/* Drop the combat-end notification and tear down the panel and its rows. */
void DestroySubMenuControls(void); /* 0x00595570 */
/* Enable the panel region set and one input region per live row. */
void EnableSubMenuRegions(void); /* 0x005957E0 */
/* Build the panel and one row per available entry of the notification's
   menu. */
unsigned char BuildSubMenuPanel(short notification); /* 0x00595850 */
/* Install the row's primary callback for its (W8SubMenuPage, entry) pair; the
   retail parameters are word-sized. */
void AssignSubMenuCallback(W8TextControl* row, short menu, short item); /* 0x00595EA0 */
/* The row availability states the refresh maps icon frames through. */
W8SubMenuEntryState GetSubMenuEntryState(short menu, short item, int party_slot); /* 0x00595FE0 */
/* Store the (W8SubMenuPage, entry) pair's pending command in the level block. */
void MapSubMenuSelection(short menu, short item); /* 0x00596240 */
/* Whether the slot may perform the pending command, in entry-state terms:
   USABLE/UNUSABLE or the _SELECTED variant when it is already queued. */
W8SubMenuEntryState CheckSubMenuActionUsable(int party_slot); /* 0x00596360 */
