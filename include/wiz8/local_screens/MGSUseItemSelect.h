#pragma once

#include "input.h"
#include "wiz8/layouts/main_game_screen.h"

struct Controls;
struct W8Region;
class W8TextControl;
class W8DialogBase;
struct W8ItemInstance;

/* The three use-item select panels; element 1 is the list panel
   iterated together with its siblings by the per-frame update. */
extern Controls* g_use_item_select_panels[3];

/* Up/down scroll buttons for the use-item select list plus the
   caption label at index 2; CloseUseItemSelectView deletes all three in one
   pass ending at g_selected_use_item_line. */
extern W8TextControl* g_use_item_select_scroll_buttons[3];
/* Action/icon controls indexed by region callback_id (0..8). */
extern W8TextControl* g_use_item_select_controls[9];

struct W8ItemInstance;

extern W8MainUiMode g_use_item_select_return_mode;
extern W8ItemInstance* g_use_item_selected;
extern W8ItemInstance* g_use_item_select_override_item;

/* GXStatus.iCurrentCursor saved while an item/spell info dialog
   is open; RestoreTargetCursor puts it back on dialog destroy. */
extern int g_saved_target_cursor;

void SetUseItemSelectReturnMode(W8MainUiMode value);
void InvalidateUseItemSelectPanel(void);
void CloseUseItemSelection(void);
/* Scroll-button region callback for use-item select (ids 0 and 1). */
unsigned char UseItemSelectScrollRegionEvent(const InputAtom* event, W8Region* region);
/* Action/icon control region callback (catalog ids 0, 3, 8). */
unsigned char UseItemSelectControlRegionEvent(const InputAtom* event, W8Region* region);
/* Text-box body callback while use-item select is active. */
unsigned char UseItemSelectTextBoxRegionEvent(const InputAtom* event, W8Region* region);
/* Text-box wheel helper while use-item select is active. */
void UseItemSelectTextBoxWheelAt(short x, unsigned short y, bool flag);
W8ItemInstance* GetSelectedOrFallbackValue(void);
void SelectCurrentUseItemLine(void);
void SetUseItemSelectOverrideItem(W8ItemInstance* value);

void CommitSelectedItemUse(void); /* FItemSelectMode per-frame commit */
void SelectUseItemLine(int line);
void OpenUseItemAssayDialog(W8ItemInstance* item);
void RestoreTargetCursor(W8DialogBase* dialog);
void UpdateUseItemDetailPanel(W8ItemInstance* item);
void TakeUseItemIntoHand(void);

void CloseUseItemSelectView(void);
/* Retarget the open use-item list at party_slot (drag, recorded
   item, or plain refresh paths). */
void RefreshUseItemSelectionForSlot(int party_slot);
/* FItemSelectMode per-frame update: panel redraws plus the
   pending-use commit check. */
void UpdateUseItemSelect(bool active);
/* After dropping a cursor item during use-item select, refresh
   the selected line. */
void RefreshUseItemSelection(void);
