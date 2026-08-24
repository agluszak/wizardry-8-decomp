#include "wiz8/unattributed/quarantine_common.h"
#include "wiz8/character.h"

// GLOBAL: WIZ8 0x0069B920
W8TextControl005ED604* g_portrait_controls_0069b920[8];

// FUNCTION: WIZ8 0x0059BB40
void DisablePortraitControls0059BB40(void)
{
    RegionSetDisable(5);
    W8TextControl005ED604** control = g_portrait_controls_0069b920;
    do {
        (*control)->SetEnabled(0);
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
            SetRegionMode4(party_slot + 0x12);
        } else {
            ClearRegionModeBits(party_slot + 0x12);
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
