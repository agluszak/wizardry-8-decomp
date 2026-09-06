#pragma once

struct Controls;

extern Controls* g_panel_69b998;

extern "C" {
extern int g_value_69b988;
extern int g_value_69b9a0;
extern int g_value_69b9a4;
}

void SetValue69B988(int value);
void RedrawPanel69B998(void);
void CloseUseItemSelection0059D950(void);
int GetSelectedOrFallbackValue0059E0D0(void);
void SelectCurrentUseItemLine0059E0E0(void);
