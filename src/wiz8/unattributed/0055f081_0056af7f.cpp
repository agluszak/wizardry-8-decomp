#include "wiz8/utility.h"
#include "wiz8/cursor.h"
#include "wiz8/local_screens/MGSSpellCasting.h"
#include "wiz8/regions.h"
#include "wiz8/xstatus.h"
#include "wiz8/dirty_tiles.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/screen_state.h"
#include "wiz8/combat_state.h"
#include "input.h"
#include "mousesystem_macros.h"

extern "C" unsigned char g_table_647ccc[128];

/* Address quarantine 0055f081-0056af7f; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

// FUNCTION: WIZ8 0x0055F2B0
unsigned char GetTable647CCCEntry(char index)
{
    return g_table_647ccc[index];
}

/* Forward the four mouse-button event kinds through SGP's owned mouse-system
   hook at the current game-space cursor position. Keyboard and motion events
   are deliberately left to their own dispatch layers. */
// FUNCTION: WIZ8 0x00568950
unsigned int Function568950(const InputAtom* input)
{
    W8ScreenPoint point;
    GetScreenPoint004284F0(&point);
    switch (input->usEvent) {
    case LEFT_BUTTON_DOWN:
    case LEFT_BUTTON_UP:
    case RIGHT_BUTTON_DOWN:
    case RIGHT_BUTTON_UP:
        MSYS_SGP_Mouse_Handler_Hook(
            input->usEvent,
            static_cast<unsigned short>(point.x),
            static_cast<unsigned short>(point.y),
            gfLeftButtonState,
            gfRightButtonState);
        return 1;
    default:
        return 0;
    }
}

extern unsigned char g_flag_006840bd;
extern "C" unsigned char g_flag_6840bc;
extern float Function420B40(int value);
extern void Function482990(unsigned char enabled);
extern void MonsterForward453160(void);
extern void Function41F0D0(void);
extern void MonsterForward4531A0(void);
extern void ClearLevelDataFlag6(void);
extern void Function5A1950(void);
extern "C" void ClearSurfaceRect(
    int left, unsigned int top, int right, unsigned int bottom);

// FUNCTION: WIZ8 0x0056aa30
void Function56AA30(void)
{
    g_flag_6840bc = 1;
    if (gXStatus.field_055 != 0) {
        DisableRegionSet1C();
    }
    if (gXStatus.fCombatMode == 0) {
        if (g_flag_006840bd != 0) {
            Function420B40(1);
            ClearRegionModeBits(0x137);
            ActivateDialogRegion(0x137);
        }
        Function482990(0);
        MonsterForward453160();
        Function41F0D0();
    }
    if (g_screen_state_0068ec78.id == W8_SCREEN_MAIN_GAME &&
        g_level_block != 0) {
        g_level_block->redraw_flags |= 0x8000;
    }
}

// FUNCTION: WIZ8 0x0056aab0
void Function56AAB0(void)
{
    if (gXStatus.field_01d == 0 && gXStatus.field_01f == 0 &&
        gXStatus.fItemSelectMode == 0 && gXStatus.field_022 == 0) {
        if (gXStatus.fCombatMode == 0) {
            if (g_flag_006840bd != 0) {
                Function420B40(4);
                ClearActiveRegionIfMatches(0x137);
                SetRegionMode4(0x137);
            }
            Function482990(1);
            MonsterForward4531A0();
            if (gXStatus.field_020 == 0 && gXStatus.field_021 == 0 &&
                gXStatus.field_024 == 0 && gXStatus.field_025 == 0) {
                ClearLevelDataFlag6();
            }
        }
        g_flag_6840bc = 0;
        g_flag_006840bd = 0;
        if (g_screen_state_0068ec78.id == W8_SCREEN_MAIN_GAME &&
            g_level_block->flag_327 == 0) {
            if (gXStatus.field_055 != 0) {
                Function5A1950();
            }
            ClearSurfaceRect(0xb1, 0x13f, 0x1cf, 0x153);
            MarkScreenRectDirty(0xb1, 0x13f, 0x1cf, 0x153, 0);
            if (g_screen_state_0068ec78.id == W8_SCREEN_MAIN_GAME &&
                g_level_block != 0) {
                g_level_block->redraw_flags |= 0x8000;
            }
        }
    }
}
