#pragma once

struct Controls;

extern Controls* g_panel_69b998;

extern int g_value_69b988;
extern int g_value_69b9a0;
extern int g_value_69b9a4;

void SetValue69B988(int value);
void RedrawPanel69B998(void);
void CloseUseItemSelection0059D950(void);
int GetSelectedOrFallbackValue0059E0D0(void); /* 0x0059E0D0 */
void SelectCurrentUseItemLine0059E0E0(void);
void SetValue69B9A4(int value);

void Function59D180(void);
void SelectUseItemLine0059DDC0(int line);

void CloseUseItemSelectView(void);   /* 0x0059CAC0 */
void Function59CC40(int party_slot); /* 0x0059CC40 */
/* 0x0059D690: after dropping a cursor item during use-item select, refresh
   the selected line. Unresolved gap body; declared for PortraitSelect. */
void Function59D690(void);
