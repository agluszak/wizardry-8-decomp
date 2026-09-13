#pragma once

/* Local Screens\MGSButtons.cpp. The combat sub-menu's panel, its five text
   rows and the two scroll-arrow buttons: construction, teardown and the
   per-frame name/action caption draw. */

struct Controls;
class W8TextControl;
class W8DialogButton;

/* Owned globals. */
extern Controls* gpSubMenuPanel;
extern W8DialogButton* g_submenu_scroll_buttons_69b858[2];
extern short g_submenu_entry_count_69b87e;
extern unsigned int g_submenu_clock_69b880;
extern unsigned char g_submenu_flag_69b8d4;
extern W8TextControl* g_submenu_rows_69b8ec[5];
extern const int g_scroll_button_positions_64c330[2][2];
/* The action-kind message indexes the caption draw maps through; owned by an
   unresolved gap TU, declared for the call site. */
extern const unsigned short g_action_kind_message_ids_61e988[];

/* Invalidate (when asked) then redraw the sub-menu panel. */
void RefreshSubMenuPanel005963E0(char invalidate); /* 0x005963E0 */
/* Close and rebuild the sub menu around the saved combat-end notification. */
void ResetSubMenuPanel00596CF0(void); /* 0x00596CF0 */
/* Create and lay out the two scroll-arrow buttons. */
unsigned char CreateSubMenuScrollButtons00596EC0(void); /* 0x00596EC0 */
/* Draw the selected character's name/profession line and the caption for
   their queued action. */
void DrawSubMenuCharacterAction00596FE0(void); /* 0x00596FE0 */

/* 0x00597550 / 0x00597560: the one-byte scroll-arrow callbacks, thin thunks
   over the scroll handler at 0x00597570. Unresolved gap, declared for the
   Configure call sites. */
void SubMenuScrollArrowUp00597550(W8DialogButton* button);
void SubMenuScrollArrowDown00597560(W8DialogButton* button);
/* 0x00595850: rebuilds gpSubMenuPanel and the five text rows.
   0x005990F0: the mode switch around a rebuild. Both unresolved gaps in this
   neighbourhood, declared for the call sites. */
unsigned char Function595850(int arg);
void Function5990F0(int arg);
