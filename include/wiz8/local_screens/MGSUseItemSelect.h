#pragma once

#include "input.h"

struct Controls;
struct W8Region;
class W8TextControl;

extern Controls* g_panel_69b998;

/* 0x0069B950: up/down scroll buttons for the use-item select list. */
extern W8TextControl* g_use_item_select_scroll_buttons[2];
/* 0x0069B960: action/icon controls indexed by region callback_id (0..8). */
extern W8TextControl* g_use_item_select_controls[9];

struct W8ItemInstance;

extern int g_value_69b988;
extern W8ItemInstance* g_value_69b9a0;
extern W8ItemInstance* g_value_69b9a4;

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

void Function59D180(void);
void SelectUseItemLine0059DDC0(int line);

void CloseUseItemSelectView(void);   /* 0x0059CAC0 */
void Function59CC40(int party_slot); /* 0x0059CC40 */
/* 0x0059D690: after dropping a cursor item during use-item select, refresh
   the selected line. Unresolved gap body; declared for PortraitSelect. */
void Function59D690(void);
