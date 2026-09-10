#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/utility.h"
#include "wiz8/engine_code/GameData.h"
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
#include "wiz8/item_video_object_vector.h"
#include "wiz8/video_object_catalog.h"

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
    POINT point;
    SGPMouseGetPos(&point);
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
extern "C" {
// GLOBAL
unsigned char g_flag_6840bc;
// GLOBAL: WIZ8 0x00647ccc
unsigned char g_table_647ccc[128];
}
extern void Function5A1950(void);

// FUNCTION: WIZ8 0x0056aa30
void Function56AA30(void)
{
    g_flag_6840bc = 1;
    if (gXStatus.field_055 != 0) {
        DisableRegionSet1C();
    }
    if (gXStatus.fCombatMode == 0) {
        if (g_flag_006840bd != 0) {
            MoveTimer(1);
            EnableRegionInput(0x137);
            ActivateDialogRegion(0x137);
        }
        Function482990(0);
        MonsterForward453160();
        ResetLevelDataVectors0041F0D0();
    }
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
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
                MoveTimer(4);
                ClearActiveRegionIfMatches(0x137);
                DisableRegionInput(0x137);
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
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
            g_level_block->flag_327 == 0) {
            if (gXStatus.field_055 != 0) {
                Function5A1950();
            }
            ClearSurfaceRect(0xb1, 0x13f, 0x1cf, 0x153);
            InvalidateRegion(0xb1, 0x13f, 0x1cf, 0x153, 0);
            if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
                g_level_block != 0) {
                g_level_block->redraw_flags |= 0x8000;
            }
        }
    }
}

// GLOBAL: WIZ8 0x006874CA
unsigned char g_flag_006874ca;
// GLOBAL: WIZ8 0x006874CB
int g_value_006874cb;
// GLOBAL: WIZ8 0x00683FDB
int g_value_00683fdb;

/* Point the mouse cursor at an item's video object, blitting it down as well.
   A negative held item id means the cursor keeps whatever it has. */
// FUNCTION: WIZ8 0x0055F160
void Function55F160(int item_id)
{
    int object;
    unsigned short y_offset;
    unsigned int handle;

    if (g_value_006874cb != -1) {
        g_flag_006874ca = 1;
        object = g_item_video_objects_68ec68.GetOrCreateVideoObject(g_value_006874cb);
        y_offset = GetCatalogVideoObjectYOffset(object);
        handle = GetCatalogVideoObjectHandle(object, 0);
        SetMouseCursorFromVideoObject(handle, y_offset, 0, 0);
        y_offset = GetCatalogVideoObjectYOffset(item_id);
        handle = GetCatalogVideoObjectHandle(item_id, 0);
        BlitToMouseCursor(handle, y_offset, 0, 0);
        RefreshMouseCursorTexture();
        g_value_00683fdb = 7;
    }
}
