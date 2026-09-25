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

/* Retail callback IDs 0 and 1 index this base, and ReleasePartyMovement walks
   the two-pointer span. */
// GLOBAL: WIZ8 0x0069BF40
W8TextControl* g_party_movement_buttons[2];
// GLOBAL: WIZ8 0x0069BF48
unsigned int g_party_movement_animation_clock;
// GLOBAL: WIZ8 0x0069BF4C
Controls* g_party_movement_panel;
// GLOBAL: WIZ8 0x0069BF50
W8TextBuffer* g_party_movement_caption;
// GLOBAL: WIZ8 0x0069BF54
unsigned int g_party_movement_animation_frame;

// GLOBAL: WIZ8 0x005EECD0
const float g_float_005eecd0 = 0.0004f;

static void DrawPartyMovementGauge(short right, short image, char panel_live, int caption);

/* Builds the party-movement panel, its text buffer and the two buttons, then
   enables region set 0x1c and fills both movement budgets. */
// FUNCTION: WIZ8 0x005A1640
unsigned char CreatePartyMovementPanel(void)
{
    W8ControlsRect bounds;

    g_party_movement_animation_frame = 0;
    g_party_movement_panel = 0;
    g_party_movement_caption = 0;
    g_party_movement_buttons[0] = 0;
    g_party_movement_buttons[1] = 0;

    g_party_movement_panel = new Controls(0xb1, 0x13f, 0x1cf, 0x153, 0x93, 0, 0);

    bounds.left = 0xc3;
    bounds.right = 0x1b9;
    bounds.top = 0x13f;
    bounds.bottom = 0x153;
    g_party_movement_caption = new W8TextBuffer(
        &bounds, 0, g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C, 0, 4);

    g_party_movement_buttons[0] = new W8TextControl(g_party_movement_panel, 200, 0x10a, 0, 0x11e,
                                                    0x14, 0x94, 0, 4, 6, 5, 6, 7);
    g_party_movement_buttons[1] = new W8TextControl(g_party_movement_panel, 0xc9, 0x10a, 0, 0x11e,
                                                    0x14, 0x94, 0, 4, 6, 5, 6, 7);

    if (g_party_movement_panel != 0 && g_party_movement_buttons[0] != 0 &&
        g_party_movement_buttons[1] != 0) {
        g_party_movement_buttons[0]->m_primaryActivationCallback = BeginFreeTurnPhase;
        g_party_movement_buttons[1]->m_primaryActivationCallback = CancelPartyMovement;
        RegionSetEnable(0x1c);
        g_party_movement_panel->SetEnabled(1);
        g_party_movement_buttons[0]->SetActive(0);
        gXStatus.fPartyMovementUi = true;
        g_level_block->move_budget_2dc = 100;
        g_level_block->move_budget_2e0 = 100;
        g_party_movement_panel->Invalidate(0);
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
    for (W8TextControl** button = g_party_movement_buttons; button != g_party_movement_buttons + 2;
         ++button) {
        if (*button != 0) {
            delete *button;
            *button = 0;
        }
    }
    if (g_party_movement_panel != 0) {
        delete g_party_movement_panel;
        g_party_movement_panel = 0;
    }
    if (g_party_movement_caption != 0) {
        delete g_party_movement_caption;
        g_party_movement_caption = 0;
    }
    gXStatus.fPartyMovementUi = false;
}

// FUNCTION: WIZ8 0x005A1950
void UpdatePartyMovementPanel(void)
{
    RegionSetEnable(0x1c);
    if (CanPartyMove() != 0) {
        g_party_movement_buttons[1]->SetActive(1);
        g_party_movement_buttons[0]->SetActive(0);
        return;
    }
    g_party_movement_buttons[0]->SetActive(1);
    g_party_movement_buttons[1]->SetActive(0);
}

// FUNCTION: WIZ8 0x005A19B0
void DrawPartyMovementPanel(void)
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
    panel_live =
        g_party_movement_panel->m_fEnabled != 0 &&
        (g_party_movement_panel->m_fDirty != 0 || g_party_movement_panel->m_fLayoutDirty != 0);
    if (CanPartyMove() == 0) {
        if (g_party_movement_buttons[0]->m_active == 0) {
            g_party_movement_buttons[0]->SetActive(1);
            g_party_movement_buttons[0]->Invalidate(0);
        }
        if (g_party_movement_buttons[1]->m_active != 0) {
            g_party_movement_buttons[1]->SetActive(0);
        }
    } else {
        if (g_party_movement_buttons[1]->m_active == 0) {
            g_party_movement_buttons[1]->SetActive(1);
            g_party_movement_buttons[1]->Invalidate(0);
        }
        if (g_party_movement_buttons[0]->m_active != 0) {
            g_party_movement_buttons[0]->SetActive(0);
        }
        if (g_combat_state->round_active_001 == 0) {
            g_party_movement_buttons[1]->SetEnabled(0);
        } else {
            g_party_movement_buttons[1]->SetEnabled(1);
        }
    }
    g_party_movement_panel->Redraw();
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
        if (g_combat_state->combat_over_000 != 0 && IsPartyEngaged() == 0) {
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
    DrawPartyMovementGauge(right, image, panel_live, caption);
    if (panel_live != 0) {
        RefreshTrackedPortraitOverlay();
    }
}

/* The movement gauge: a static end frame for the empty/full states and the
   twelve-frame animated bar otherwise, rearming its own countdown each tick. */
// FUNCTION: WIZ8 0x005A1C40
static void DrawPartyMovementGauge(short right, short image, char panel_live, int caption)
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
        if (ClockIsTicking(g_party_movement_animation_clock) == 0) {
            g_party_movement_animation_frame = g_party_movement_animation_frame + 1;
            if (g_party_movement_animation_frame > 0xb) {
                g_party_movement_animation_frame = 0;
            }
            rearm = true;
            advanced = true;
        }
        frame = 0;
    }
    unsigned int action;
    if (CanPartyMove() == 0 || (g_combat_state->combat_over_000 != 0 && IsPartyEngaged() == 0)) {
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
        g_party_movement_animation_clock = SetCountdownClock(timer_length);
    }
    if (panel_live == 0) {
        if (advanced) {
            g_party_movement_panel->Invalidate(0);
        }
        return;
    }
    DrawCatalogImageAndInvalidate(-0xe, 0x95, 0,
                                  g_party_movement_animation_frame + frame_base + frame,
                                  right - 0xf, 0x142, 2, 0);
    InvalidateRegion(right - 0xf, 0x142, right - 1, 0x150, 0);
    if (caption != -1) {
        g_party_movement_caption->SetText(gppStringList[caption], g_font_683660);
        g_party_movement_caption->RenderToTarget(0, 0, -0xe);
    }
}

// FUNCTION: WIZ8 0x005A19A0
void DisablePartyMovementRegions(void)
{
    RegionSetDisable(0x1c);
}

// FUNCTION: WIZ8 0x005A1DD0
void InvalidatePartyMovementPanel(void)
{
    g_party_movement_panel->Invalidate(0);
}

/* Free-turn (id 0) and cancel-party-movement (id 1) button regions. */
// FUNCTION: WIZ8 0x005A1DE0
unsigned char FreeTurnButtonRegionEvent(const InputAtom* event, W8Region* region)
{
    int us_event;
    unsigned int callback_id;

    if (gXStatus.fPartyMovementUi == 0) {
        return 0;
    }
    us_event = event->usEvent;
    callback_id = region->callback_id;
    if (us_event <= LEFT_BUTTON_REPEAT) {
        if (us_event == LEFT_BUTTON_REPEAT || us_event == LEFT_BUTTON_DOWN) {
            g_party_movement_buttons[callback_id]->OnLeftButtonDown(0);
            region->flags |= W8_REGION_LEFT_BUTTON_HELD;
            return 1;
        }
        if (us_event == LEFT_BUTTON_UP) {
            g_party_movement_buttons[callback_id]->OnLeftButtonUp(0);
            if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
                region->flags &= ~W8_REGION_LEFT_BUTTON_HELD;
            }
            return 1;
        }
    } else if (us_event == MOUSE_POS) {
        if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
            g_party_movement_buttons[callback_id]->OnMouseLeave(0);
            return 1;
        }
        if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
            g_party_movement_buttons[callback_id]->OnMouseEnter(0);
            return 1;
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x005A1E90
void DisableFreeTurnButton(void)
{
    g_party_movement_buttons[0]->SetEnabled(0);
}

// FUNCTION: WIZ8 0x005A1EA0
void EnableFreeTurnButton(void)
{
    g_party_movement_buttons[0]->SetEnabled(1);
}

/* Per-frame movement/fatigue processing: each occupied party slot with stamina
   and a bearable load accumulates distance scaled by load, doubled in combat
   and raised again while the second condition runs, until the accumulator
   crosses 2500 and converts into real fatigue. */
// FUNCTION: WIZ8 0x005A1EB0
unsigned char HandlePartyMovement(float* real_elapsed, float* frame_elapsed)
{
    int party_slot;
    float amount;
    float multiplier;
    unsigned int ticks;

    if (ConsumeLevelElapsedTime0041F170(real_elapsed, frame_elapsed) == 0) {
        return 0;
    }
    for (party_slot = 0; party_slot < W8_PARTY_SLOT_COUNT; ++party_slot) {
        W8PartySlotRow* row = &g_status_685170.buffers.XChar[party_slot];
        W8Character* character = &g_status_685170.buffers.Char[party_slot];
        if (row->fOccupied == 0 || character->stamina <= 0 || character->highest_condition >= 0xf) {
            continue;
        }
        amount = *frame_elapsed;
        if (g_status_685170.search_mode != 0 || gXStatus.fCombatMode != 0) {
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
        if (character->uiCondition[2] != 0) {
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
