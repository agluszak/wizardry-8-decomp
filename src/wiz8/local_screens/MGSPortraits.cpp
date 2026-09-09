#include "wiz8/local_code/TextControl.h"
#include "wiz8/character.h"
#include "wiz8/game_status.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/regions.h"

// GLOBAL: WIZ8 0x0069B940
Controls* g_panel_69b940;

// GLOBAL: WIZ8 0x0069B920
W8TextControl* g_portrait_controls_0069b920[8];

// The condition-buttons panel and its eight buttons, created together by the
// 0x0059BDB0 setup. The panel assert there names them gpConditionButtonsPanel
// and gpConditionButtons_uiSlot.
// GLOBAL: WIZ8 0x0069B900
W8TextControl* g_condition_buttons_0069b900[8];
// GLOBAL: WIZ8 0x0069B944
Controls* g_condition_buttons_panel_0069b944;

// FUNCTION: WIZ8 0x0059BAD0
void Function59BAD0(void)
{
    RegionSetDisable(5);
    W8TextControl** control = g_portrait_controls_0069b920;
    do {
        (*control)->SetActive(0);
        ++control;
    } while (control < g_portrait_controls_0069b920 + 8);
    Controls* panel = g_panel_69b940;
    if (panel != 0) {
        panel->~Controls();
        ::operator delete(panel);
        g_panel_69b940 = 0;
    }
    control = g_portrait_controls_0069b920;
    do {
        if (*control != 0) {
            delete *control;
            *control = 0;
        }
        ++control;
    } while (control < g_portrait_controls_0069b920 + 8);
}

// FUNCTION: WIZ8 0x0059BF70
void Function59BF70(void)
{
    Controls* panel = g_condition_buttons_panel_0069b944;
    if (panel != 0) {
        panel->~Controls();
        ::operator delete(panel);
        g_condition_buttons_panel_0069b944 = 0;
    }
    W8TextControl** control = g_condition_buttons_0069b900;
    do {
        if (*control != 0) {
            delete *control;
            *control = 0;
        }
        ++control;
    } while (control < g_condition_buttons_0069b900 + 8);
}

// FUNCTION: WIZ8 0x0059BB40
void DisablePortraitControls0059BB40(void)
{
    RegionSetDisable(5);
    W8TextControl** control = g_portrait_controls_0069b920;
    do {
        (*control)->SetActive(0);
        ++control;
    } while (control < g_portrait_controls_0069b920 + 8);
}

// FUNCTION: WIZ8 0x0059BB70
void EnablePortraitAdvanceRegions0059BB70(void)
{
    RegionSetEnable(5);
    unsigned int state_offset = 0;
    int party_slot = 0;
    do {
        if (!IsCharacterReadyToAdvance(party_slot) ||
            reinterpret_cast<unsigned char*>(g_status_685170.buffers.party_rows)[state_offset + 0x103] == 0) {
            DisableRegionInput(party_slot + 0x12);
        } else {
            EnableRegionInput(party_slot + 0x12);
        }
        state_offset += 0x106;
        ++party_slot;
    } while (state_offset < 0x830);
}

// FUNCTION: WIZ8 0x0059BBD0
void InvalidatePortraitControl0059BBD0(unsigned int party_slot)
{
    unsigned char* state = reinterpret_cast<unsigned char*>(g_status_685170.buffers.party_rows);
    if (party_slot < 8 && state[party_slot * 0x106]) {
        g_portrait_controls_0069b920[party_slot]->Invalidate(0);
    }
}

// FUNCTION: WIZ8 0x0059BC00
void RedrawPanel69B940(void)
{
    g_panel_69b940->Invalidate(0);
}
