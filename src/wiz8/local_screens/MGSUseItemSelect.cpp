#include "wiz8/game_status.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/local_screens/MGSUseItemSelect.h"

extern void Function0059CAC0(void);
extern void ClearTargetingMode0053B050(int party_slot);
extern void SelectUseItemLine0059DDC0(int line);

// GLOBAL: WIZ8 0x0069B95C
int g_selected_use_item_line_0069b95c;
// GLOBAL: WIZ8 0x0069B9A0
int g_value_69b9a0;
// GLOBAL: WIZ8 0x0069B9A4
int g_value_69b9a4;

// FUNCTION: WIZ8 0x0059CF30
void SetValue69B988(int value)
{
    g_value_69b988 = value;
}
// FUNCTION: WIZ8 0x0059CF40
void RedrawPanel69B998(void)
{
    g_panel_69b998->Invalidate(0);
}

// FUNCTION: WIZ8 0x0059D950
void CloseUseItemSelection0059D950(void)
{
    Function0059CAC0();
    ClearTargetingMode0053B050(g_status_685170.selected_character);
}

// FUNCTION: WIZ8 0x0059E0D0
int GetSelectedOrFallbackValue0059E0D0(void)
{
    int value = g_value_69b9a4;
    if (value == 0) {
        value = g_value_69b9a0;
    }
    return value;
}

// FUNCTION: WIZ8 0x0059E0E0
void SelectCurrentUseItemLine0059E0E0(void)
{
    SelectUseItemLine0059DDC0(g_selected_use_item_line_0069b95c);
}
