#pragma once

#include "input.h"

struct Controls;
struct W8Region;
class W8TextControl;
class W8DialogBase;
struct W8ItemInstance;

/* 0x0069B994: the three use-item select panels; element 1 is the list panel
   iterated together with its siblings by the per-frame update. */
extern Controls* g_use_item_select_panels_69b994[3];

/* 0x0069B950: up/down scroll buttons for the use-item select list. */
extern W8TextControl* g_use_item_select_scroll_buttons[2];
/* 0x0069B960: action/icon controls indexed by region callback_id (0..8). */
extern W8TextControl* g_use_item_select_controls[9];

struct W8ItemInstance;

extern int g_value_69b988;
extern W8ItemInstance* g_value_69b9a0;
extern W8ItemInstance* g_value_69b9a4;

/* 0x0069BF30: gXStatus.iCurrentCursor saved while an item/spell info dialog
   is open; RestoreTargetCursor59D930 puts it back on dialog destroy. */
extern int g_saved_target_cursor_0069bf30;

void SetValue69B988(int value);
void RedrawPanel69B998(void);
void CloseUseItemSelection0059D950(void);
/* Scroll-button region callback for use-item select (ids 0 and 1). */
unsigned char UseItemSelectScrollRegionEvent(const InputAtom* event,
                                             W8Region* region); /* 0x0059D970 */
/* Action/icon control region callback (catalog ids 0, 3, 8). */
unsigned char UseItemSelectControlRegionEvent(const InputAtom* event,
                                              W8Region* region); /* 0x0059DA30 */
W8ItemInstance* GetSelectedOrFallbackValue0059E0D0(void);        /* 0x0059E0D0 */
void SelectCurrentUseItemLine0059E0E0(void);
void SetValue69B9A4(W8ItemInstance* value);

void CommitSelectedItemUse(void);         /* 0x0059D180: fItemSelectMode per-frame commit */
void SelectUseItemLine0059DDC0(int line); /* 0x0059DDC0 */
void OpenUseItemAssayDialog59D880(W8ItemInstance* item);     /* 0x0059D880 */
void RestoreTargetCursor59D930(W8DialogBase* dialog);        /* 0x0059D930 */
void UpdateUseItemDetailPanel0059DFA0(W8ItemInstance* item); /* 0x0059DFA0 */
void TakeUseItemIntoHand0059E0F0(void);                      /* 0x0059E0F0 */

void CloseUseItemSelectView(void); /* 0x0059CAC0 */
/* 0x0059CC40: retarget the open use-item list at party_slot (drag, recorded
   item, or plain refresh paths). */
void RefreshUseItemSelectionForSlot0059CC40(int party_slot);
/* 0x0059CF50: fItemSelectMode per-frame update: panel redraws plus the
   pending-use commit check. */
void UpdateUseItemSelect0059CF50(unsigned char active);
/* 0x0059D690: after dropping a cursor item during use-item select, refresh
   the selected line. */
void RefreshUseItemSelection(void);
