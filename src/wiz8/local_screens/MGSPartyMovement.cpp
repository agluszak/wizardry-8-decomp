#include "wiz8/local_screens/MGSPartyMovement.h"

#include "timer.h"
#include "wiz8/wiz8_windows.h"
#include "vobject_blitters.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/float_constants.h"
#include "wiz8/fonts.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatPartyMovement.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/local_code/ControlsRect.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/regions.h"
#include "wiz8/sr_api.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/xstatus.h"

#define PARTY_MOVEMENT_CPP "C:\\Projects\\Wizardry 8\\Local Screens\\MGSPartyMovement.cpp"

/* The assertion at 0x005A1A0E names this value ACTION_STATUS_FINISHED. */
enum { W8_ACTION_STATUS_FINISHED = 3 };

// GLOBAL: WIZ8 0x0069BF40
W8TextControl* g_panel_69bf40;
// GLOBAL: WIZ8 0x0069BF44
W8TextControl* g_panel_69bf44;
// GLOBAL: WIZ8 0x0069BF48
unsigned int g_movement_timer_69bf48;
// GLOBAL: WIZ8 0x0069BF4C
Controls* g_panel_69bf4c;
// GLOBAL: WIZ8 0x0069BF50
W8TextBuffer* g_text_buffer_69bf50;
// GLOBAL: WIZ8 0x0069BF54
unsigned int g_movement_frame_69bf54;

// GLOBAL: WIZ8 0x005EECD0
const float g_float_005eecd0 = 0.0004f;

static void DrawPartyMovementGauge005A1C40(short right, short image, char panel_live, int caption);

/* Builds the party-movement panel, its text buffer and the two buttons, then
   enables region set 0x1c and fills both movement budgets. */
// FUNCTION: WIZ8 0x005A1640
unsigned char CreatePartyMovementPanel005A1640(void)
{
    W8ControlsRect bounds;

    g_movement_frame_69bf54 = 0;
    g_panel_69bf4c = 0;
    g_text_buffer_69bf50 = 0;
    g_panel_69bf40 = 0;
    g_panel_69bf44 = 0;

    g_panel_69bf4c = new Controls(0xb1, 0x13f, 0x1cf, 0x153, 0x93, 0, 0);

    bounds.left = 0xc3;
    bounds.right = 0x1b9;
    bounds.top = 0x13f;
    bounds.bottom = 0x153;
    g_text_buffer_69bf50 = new W8TextBuffer(
        &bounds, 0, g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C, 0, 4);

    g_panel_69bf40 =
        new W8TextControl(g_panel_69bf4c, 200, 0x10a, 0, 0x11e, 0x14, 0x94, 0, 4, 6, 5, 6, 7);
    g_panel_69bf44 =
        new W8TextControl(g_panel_69bf4c, 0xc9, 0x10a, 0, 0x11e, 0x14, 0x94, 0, 4, 6, 5, 6, 7);

    if (g_panel_69bf4c != 0 && g_panel_69bf40 != 0 && g_panel_69bf44 != 0) {
        g_panel_69bf40->m_primaryActivationCallback = BeginFreeTurnPhase;
        g_panel_69bf44->m_primaryActivationCallback = Function4F0860;
        RegionSetEnable(0x1c);
        g_panel_69bf4c->SetEnabled(1);
        g_panel_69bf40->SetActive(0);
        gXStatus.fPartyMovementUi = 1;
        g_level_block->move_budget_2dc = 100;
        g_level_block->move_budget_2e0 = 100;
        g_panel_69bf4c->Invalidate(0);
        return 1;
    }
    ReleasePartyMovement();
    return 0;
}

// FUNCTION: WIZ8 0x005A1890
void ReleasePartyMovement(void)
{
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
        ClearSurfaceRect(0xb1, 0x13f, 0x1cf, 0x153);
        InvalidateRegion(0xb1, 0x13f, 0x1cf, 0x153, 0);
    }
    RequestRedraw(0x8000);
    RegionSetDisable(0x1c);
    for (W8TextControl** control = &g_panel_69bf40; control <= &g_panel_69bf44; ++control) {
        if (*control != 0) {
            delete *control;
            *control = 0;
        }
    }
    if (g_panel_69bf4c != 0) {
        delete g_panel_69bf4c;
        g_panel_69bf4c = 0;
    }
    if (g_text_buffer_69bf50 != 0) {
        delete g_text_buffer_69bf50;
        g_text_buffer_69bf50 = 0;
    }
    gXStatus.fPartyMovementUi = 0;
}

// FUNCTION: WIZ8 0x005A1950
void UpdatePartyMovementPanel005A1950(void)
{
    RegionSetEnable(0x1c);
    if (CanPartyMove() != 0) {
        g_panel_69bf44->SetActive(1);
        g_panel_69bf40->SetActive(0);
        return;
    }
    g_panel_69bf40->SetActive(1);
    g_panel_69bf44->SetActive(0);
}

// FUNCTION: WIZ8 0x005A19B0
void DrawPartyMovementPanel005A19B0(void)
{
    SGPRect previous;
    SGPRect clip;
    char panel_live;
    int caption;
    short right;
    int image;

    if (gXStatus.fCombatMode == 0) {
        srAssertFail("gXStatus.fCombatMode", PARTY_MOVEMENT_CPP, 0xf6, 0);
    }
    if ((g_combat_state->uiCurrentPartyAction == 0 ||
         g_combat_state->uiCurrentPartyActionStatus == W8_ACTION_STATUS_FINISHED) &&
        g_combat_state->uiNextPartyAction == 0) {
        srAssertFail("!(gpCombat->uiCurrentPartyAction==0 || "
                     "gpCombat->uiCurrentPartyActionStatus==3) && gpCombat->uiNextPartyAction!=0",
                     PARTY_MOVEMENT_CPP, 0xfb, 0);
    }
    panel_live = g_panel_69bf4c->m_fEnabled != 0 &&
                 (g_panel_69bf4c->m_fDirty != 0 || g_panel_69bf4c->m_fLayoutDirty != 0);
    if (CanPartyMove() == 0) {
        if (g_panel_69bf40->m_active == 0) {
            g_panel_69bf40->SetActive(1);
            g_panel_69bf40->Invalidate(0);
        }
        if (g_panel_69bf44->m_active != 0) {
            g_panel_69bf44->SetActive(0);
        }
    } else {
        if (g_panel_69bf44->m_active == 0) {
            g_panel_69bf44->SetActive(1);
            g_panel_69bf44->Invalidate(0);
        }
        if (g_panel_69bf40->m_active != 0) {
            g_panel_69bf40->SetActive(0);
        }
        if (g_combat_state->flag_001 == 0) {
            g_panel_69bf44->SetEnabled(0);
        } else {
            g_panel_69bf44->SetEnabled(1);
        }
    }
    g_panel_69bf4c->Redraw();
    right = 0x1b9 - g_level_block->move_budget_2dc * 0xf6 / 100;
    if (CanPartyMove() == 0) {
        if (GetLevelDataFlag6() == 0) {
            image = 1;
            caption = ((g_level_block->move_budget_2dc != 100) - 1 & 0x77f) - 1;
        } else {
            image = 2;
            caption = 0x77d;
        }
    } else {
        image = 0;
        if (g_combat_state->flag_000 != 0 && IsPartyEngaged() == 0) {
            caption = (g_combat_state->uiCurrentPartyAction != 2) + 0x77b;
        } else {
            caption = (g_combat_state->uiNextPartyAction != 2) + 0x77b;
        }
    }
    if (panel_live != 0) {
        GetClippingRect(&previous);
        clip.iLeft = right;
        clip.iTop = 0;
        clip.iRight = 0x1b9;
        clip.iBottom = 0x1e0;
        SetClippingRect(&clip);
        DrawCatalogImageAndInvalidate(-0xe, 0x96, 0, image, 0xc3, 0x144, 2, 0);
        SetClippingRect(&previous);
    }
    DrawPartyMovementGauge005A1C40(right, image, panel_live, caption);
    if (panel_live != 0) {
        Function563890();
    }
}

/* The movement gauge: a static end frame for the empty/full states and the
   twelve-frame animated bar otherwise, rearming its own countdown each tick. */
// FUNCTION: WIZ8 0x005A1C40
static void DrawPartyMovementGauge005A1C40(short right, short image, char panel_live, int caption)
{
    bool rearm;
    bool advanced;
    int frame;
    int timer_length;
    int frame_base;

    frame_base = 0;
    rearm = false;
    advanced = false;
    if (image == 0) {
        rearm = true;
        frame = 0x18;
    } else if (image == 2) {
        rearm = true;
        frame = 0xc;
    } else {
        if (ClockIsTicking(g_movement_timer_69bf48) == 0) {
            g_movement_frame_69bf54 = g_movement_frame_69bf54 + 1;
            if (g_movement_frame_69bf54 > 0xb) {
                g_movement_frame_69bf54 = 0;
            }
            rearm = true;
            advanced = true;
        }
        frame = 0;
    }
    unsigned int action;
    if (CanPartyMove() == 0 || (g_combat_state->flag_000 != 0 && IsPartyEngaged() == 0)) {
        action = g_combat_state->uiCurrentPartyAction;
    } else {
        action = g_combat_state->uiNextPartyAction;
    }
    if (action == 2) {
        timer_length = 0x32;
    } else {
        frame_base = 0x24;
        timer_length = 0x5a;
    }
    if (rearm) {
        g_movement_timer_69bf48 = SetCountdownClock(timer_length);
    }
    if (panel_live == 0) {
        if (advanced) {
            g_panel_69bf4c->Invalidate(0);
        }
        return;
    }
    DrawCatalogImageAndInvalidate(-0xe, 0x95, 0, g_movement_frame_69bf54 + frame_base + frame,
                                  right - 0xf, 0x142, 2, 0);
    InvalidateRegion(right - 0xf, 0x142, right - 1, 0x150, 0);
    if (caption != -1) {
        g_text_buffer_69bf50->SetText(gppStringList[caption], g_font_683660);
        g_text_buffer_69bf50->RenderToTarget(0, 0, -0xe);
    }
}

// FUNCTION: WIZ8 0x005A19A0
void DisableRegionSet1C(void)
{
    RegionSetDisable(0x1c);
}

// FUNCTION: WIZ8 0x005A1DD0
void RedrawPanel69BF4C(void)
{
    g_panel_69bf4c->Invalidate(0);
}

// FUNCTION: WIZ8 0x005A1E90
void DisablePanel69BF40005A1E90(void)
{
    g_panel_69bf40->SetEnabled(0);
}

// FUNCTION: WIZ8 0x005A1EA0
void EnablePanel69BF40005A1EA0(void)
{
    g_panel_69bf40->SetEnabled(1);
}

/* Per-frame movement/fatigue processing: each occupied party slot with stamina
   and a bearable load accumulates distance scaled by load, doubled in combat
   and raised again while the second condition runs, until the accumulator
   crosses 2500 and converts into real fatigue. */
// FUNCTION: WIZ8 0x005A1EB0
unsigned char HandlePartyMovement005A1EB0(float* real_elapsed, float* frame_elapsed)
{
    int party_slot;
    float amount;
    float multiplier;
    unsigned int ticks;

    if (Function41F170(real_elapsed, frame_elapsed) == 0) {
        return 0;
    }
    for (party_slot = 0; party_slot < W8_PARTY_SLOT_COUNT; ++party_slot) {
        W8PartySlotRow* row = &g_status_685170.buffers.party_rows[party_slot];
        W8Character* character = &g_status_685170.buffers.characters[party_slot];
        if (row->occupied == 0 || character->stamina <= 0 || character->highest_condition >= 0xf) {
            continue;
        }
        amount = *frame_elapsed;
        if (g_status_685170.flag_238f != 0 || gXStatus.fCombatMode != 0) {
            amount = *real_elapsed * g_float_005ebc7c + amount;
        }
        switch (character->load_category) {
        case 0:
            multiplier = 1.0f;
            break;
        case 1:
            multiplier = 1.25f;
            break;
        case 2:
            multiplier = 1.5f;
            break;
        case 3:
            multiplier = 2.0f;
            break;
        case 4:
            multiplier = 3.0f;
            break;
        default:
            srAssertFail("FALSE", PARTY_MOVEMENT_CPP, 0x2a9,
                         "HandlePartyMovement: ERROR - Invalid load category");
        }
        if (gXStatus.fCombatMode != 0) {
            multiplier = multiplier + multiplier;
        }
        if (character->condition_turns[2] != 0) {
            multiplier = multiplier * g_float_005ec3b8;
        }
        row->movement_fatigue = multiplier * amount + row->movement_fatigue;
        if (row->movement_fatigue > g_position_height_epsilon_005ebfdc) {
            ticks = static_cast<unsigned int>(row->movement_fatigue * g_float_005eecd0);
            FatigueCharacter(party_slot, ticks, 0, 0);
            row->movement_fatigue = row->movement_fatigue - static_cast<float>(ticks * 0x9c4);
        }
    }
    return 1;
}
