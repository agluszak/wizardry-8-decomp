#include "wiz8/local_code/GameplayCode.h"
#include <wchar.h>

#include "wiz8/local_screens/RCSCommon.h"
#include "wiz8/local_screens/MGSFormation.h"
#include "wiz8/local_screens/MGSPortraits.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_screens/RCSItemsPage.h"
#include "wiz8/local_screens/RCSStatsPage.h"
#include "wiz8/local_code/ControlsRect.h"
#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/layouts/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/layouts/targeting.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/dialog_code/MessageDialogBase.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/fonts.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/cursor.h"
#include "wiz8/npc_interaction.h"
#include "wiz8/regions.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/xstatus.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/item_video_object_vector.h"
#include "wiz8/learned_spells.h"
#include "Font.h"
#include "vsurface.h"

/*
 * Local Screens\RCSCommon.cpp.
 *
 * Two panels the review-character screens share. Each owns a Controls object
 * and one widget inside it, and each has the same pair of bodies: one that
 * redraws the panel and one that tears both down.
 */

// GLOBAL: WIZ8 0x0069c3c4
Controls* g_level_up_panel_0069c3c4;
// GLOBAL: WIZ8 0x0069c3c8
Controls* g_dismiss_panel_0069c3c8;

/* The eight item-page action buttons, indexed by the mode SetCampItemActionMode
   arms: one control per actionable entry-mode, cleared before each reselection. */
// GLOBAL: WIZ8 0x0069c3cc
W8TextControl* g_item_action_controls_69c3cc[8];
/* Five page buttons (help 2364-2368) packed immediately before the dismiss
   button; CreateCampButtonPanel005B4EB0 allocates them on the shared bottom Controls panel. */
// GLOBAL: WIZ8 0x0069c3ec
W8TextControl* g_camp_page_buttons_0069c3ec[5];
// GLOBAL: WIZ8 0x0069c3c0
W8TextControl* g_level_up_button_0069c3c0;
// GLOBAL: WIZ8 0x0069c400
W8TextControl* g_dismiss_button_0069c400;

/* The shared bottom panel CreateCampButtonPanel005B4EB0 parents the five page
   buttons and the eight item-action controls to. */
// GLOBAL: WIZ8 0x0069c404
Controls* g_item_actions_panel_0069c404;

/* 0x0064DAD0: the level-line format the header draws - name, level and the
   profession's level-band title. */
// GLOBAL: WIZ8 0x0064DAD0
const wchar_t g_format_s_d_paren_s_0064dad0[] = L"%s %d (%s)";

/* The page and item-action button callbacks CreateCampButtonPanel005B4EB0
   wires into m_primaryActivationCallback. */
static void CampPageAction005B5AE0(void);
static void CampPageAction005B5B30(void);
static void CampPageAction005B5B80(void);
static void CampPageAction005B5BD0(void);
static void CampPageDismissAction005B5C20(void);
static void CampItemAction005B5C30(void);
static void CampItemAction005B5C80(void);
static void CampItemAction005B5CD0(void);
static void CampItemAction005B5D10(void);
static void CampItemAction005B5D50(void);
static void CampItemAction005B5D90(void);
static void CampItemAction005B5DF0(void);
static void CampItemAction005B5E60(void);

void ShowDismissCharacterDialog(void);
void OnDismissCharacterDialogClosed(W8DialogBase* dialog);

/* Swap the reviewed party member: rebuild the item list or learned-spell
   scratch for the new character and repaint whichever page is showing. */
// FUNCTION: WIZ8 0x005B6B30
void SelectCampCharacter005B6B30(int slot)
{
    giReviewCharSlot = slot;
    g_value_0069c0f8 = &g_status_685170.buffers.characters[slot];
    SyncReviewCharInputRegion005A4570();
    switch (g_camp_screen_0069c0f4->page) {
    case 0:
        EnableCampActionButtons005B9270();
        if (g_camp_screen_0069c0f4->realm_flags[0] != 0) {
            g_camp_screen_0069c0f4->item_scroll = 0;
            RebuildCampItemList005A4A00();
        }
        if (g_camp_screen_0069c0f4->entry_mode != 3 && g_camp_screen_0069c0f4->entry_mode != 2 &&
            g_camp_screen_0069c0f4->entry_mode != 8) {
            SetCampItemActionMode005B59B0(0);
        }
        break;
    case 1:
        g_camp_screen_0069c0f4->effect_selection = 0;
        RebuildCampEffectList005C4EE0();
        g_camp_screen_0069c0f4->character_info->Invalidate(0);
        break;
    case 3:
        BuildLearnedSpellState004F9600(&g_camp_screen_0069c0f4->learned_spells, g_value_0069c0f8);
        RefreshCampSpellRanges005B7290();
        break;
    }
    g_camp_screen_0069c0f4->redraw_flags |= 0xfffffff;
}

/* Give the held item to a camp portrait on right-click: refuse dead/insane/
   stoned with a notice, check eligibility and combat action allowance, then
   AddItemToCharacter. */
// FUNCTION: WIZ8 0x005B6C10
void TryGiveHeldItemToCampPortrait005B6C10(int slot)
{
    W8Character* character;

    if (!g_status_685170.buffers.party_rows[slot].occupied) {
        return;
    }
    character = &g_status_685170.buffers.characters[slot];
    if (character->condition_turns[19] != 0) {
        ShowCampNoticeLine(gppStringList[0x241c / 4], 0, 1, 0);
        return;
    }
    if (character->condition_turns[14] != 0) {
        ShowCampNoticeLine(gppStringList[0x2420 / 4], 0, 1, 0);
        return;
    }
    if (character->condition_turns[13] != 0) {
        ShowCampNoticeLine(gppStringList[0x2424 / 4], 0, 1, 0);
        return;
    }
    if (!IsPartySlotEligible00524A10(slot)) {
        ShowCampNoticeLine(gppStringList[0x2404 / 4], 0, 1, 0);
        return;
    }
    if (IsCampActionAllowed005A6090(slot) == 0) {
        return;
    }
    AddItemToCharacter(character, &g_status_685170.item_in_hand_235b, 0, 0, 0);
}

// FUNCTION: WIZ8 0x005b6d20
bool CanSelectRcsPartySlot(int ui_slot)
{
    if (!g_status_685170.buffers.party_rows[ui_slot].occupied) {
        srAssertFail("gStatus.XChar[uiSlot].fOccupied",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\RCSCommon.cpp", 0x9ac, 0);
    }

    W8Character* character = &g_status_685170.buffers.characters[ui_slot];
    if (character->condition_turns[19] != 0) {
        return false;
    }
    if (character->condition_turns[14] != 0) {
        return false;
    }
    if (character->condition_turns[11] != 0) {
        return false;
    }
    if (character->condition_turns[13] != 0) {
        return false;
    }
    if (gXStatus.fCombatMode) {
        if (!g_combat_state->flag_a50) {
            return false;
        }
        if (g_status_685170.buffers.party_rows[ui_slot].pending_action != 9) {
            return false;
        }
    }
    return true;
}

// FUNCTION: WIZ8 0x005b6df0
void DrawRcsText(const wchar_t* text, int left, int top, int width, unsigned int layout_mode)
{
    W8ControlsRect bounds = {left, top, left + width, top + 12};
    W8TextBuffer buffer(&bounds, text, g_font_683660, layout_mode, 4);
    buffer.RenderToTarget(0, 0, -14);
}

// FUNCTION: WIZ8 0x005b6e90
void DrawRcsBoldText(const wchar_t* text, int left, int top, int width, unsigned int layout_mode)
{
    W8ControlsRect bounds = {left, top, left + width, top + 12};
    W8TextBuffer buffer(&bounds, text, g_wiz_text_bold_font_683664, layout_mode, 4);
    buffer.RenderToTarget(0, 0, -14);
}

// FUNCTION: WIZ8 0x005b6f30
void DrawTallRcsText(const wchar_t* text, int left, int top, int width, unsigned int layout_mode)
{
    W8ControlsRect bounds = {left, top, left + width, top + 18};
    W8TextBuffer buffer(&bounds, text, g_font_683660, layout_mode, 4);
    buffer.RenderToTarget(0, 0, -14);
}

/* Draws text honoring the same layout mask pairs as the buffered variants
   above, but through mprintf with the current font: the centered and right
   masks shift the start by the measured string length and the baseline is
   centered on the caller-provided height. */
// FUNCTION: WIZ8 0x005b6fd0
void DrawRcsTextJustified(const wchar_t* text, int left, int top, int width, int height,
                          unsigned int layout_mode)
{
    if (layout_mode == (g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C)) {
        left += (width - StringPixLength(const_cast<wchar_t*>(text), g_font_683660)) / 2;
    } else if (layout_mode ==
               (g_W8TextBufferLayoutMask005ED550 | g_W8TextBufferLayoutMask005ED554)) {
        left += width - StringPixLength(const_cast<wchar_t*>(text), g_font_683660);
    }
    top += (height - GetFontHeight(g_font_683660)) / 2;
    SetFont(g_font_683660);
    mprintf(left, top, L"%s", text);
}

// FUNCTION: WIZ8 0x005b6630
void OpenLevelUpCharacterScreen(void)
{
    if (!g_status_685170.buffers.party_rows[giReviewCharSlot].occupied) {
        srAssertFail("fCHAR_OCCUPIED(giReviewCharSlot)",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\RCSCommon.cpp", 0x888, 0);
    }
    g_current_screen_state.parameter_2 = giReviewCharSlot;
    g_current_screen_state.parameter_3 = g_value_0069c0f8;
    g_pending_screen_state.parameter_3 = &g_status_685170.buffers.characters[giReviewCharSlot];
    g_pending_screen_state.mode = 2;
    SetPendingScreenState(W8_SCREEN_CHARACTER);
}

/* Dismiss-confirm portrait/name hitbox (help 2368): click dismisses the
   reviewed party member; mouse enter/leave toggles unknown_d40[0]. */
// FUNCTION: WIZ8 0x005B5E90
unsigned char CampDismissPortraitRegionEvent(const InputAtom* event, W8Region* region)
{
    int us_event = event->usEvent;

    if (us_event == LEFT_BUTTON_DOWN) {
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        return 1;
    }
    if (us_event != LEFT_BUTTON_UP) {
        if (us_event != MOUSE_POS) {
            return 0;
        }
        if ((region->flags & W8_REGION_MOUSE_LEAVE) == 0) {
            if ((region->flags & W8_REGION_MOUSE_ENTER) == 0) {
                return 0;
            }
            g_camp_screen_0069c0f4->unknown_d40[0] = 1;
        } else {
            g_camp_screen_0069c0f4->unknown_d40[0] = 0;
        }
        g_camp_screen_0069c0f4->redraw_flags |= 0x100;
        return 0;
    }
    if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) == 0) {
        return 1;
    }
    DismissSelectedPartyCharacter();
    return 1;
}

/* Eight party portrait slots on the camp screen. Targeting modes 1 and 7 aim
   spells/items; otherwise a click selects the reviewed character. */
// FUNCTION: WIZ8 0x005B5F10
unsigned char CampPortraitSlotRegionEvent(const InputAtom* event, W8Region* region)
{
    int us_event = event->usEvent;
    unsigned int target_slot = region->callback_id;

    if (us_event < RIGHT_BUTTON_UP) {
        if (us_event == RIGHT_BUTTON_DOWN) {
            region->flags |= W8_REGION_RIGHT_BUTTON_HELD;
            return 1;
        }
        if (us_event == LEFT_BUTTON_DOWN) {
            region->flags |= W8_REGION_LEFT_BUTTON_HELD;
            return 1;
        }
        if (us_event != LEFT_BUTTON_UP) {
            return 0;
        }
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0 &&
            g_status_685170.buffers.party_rows[target_slot].occupied != 0) {
            if (gXStatus.iTargetingMode == 1) {
                if (g_camp_screen_0069c0f4->page != 0 ||
                    (g_camp_screen_0069c0f4->entry_mode != 7 &&
                     g_camp_screen_0069c0f4->entry_mode != 9)) {
                    if (!CanPartySlotParticipate(static_cast<int>(target_slot))) {
                        QueueCharacterEvent(&g_status_685170.buffers.characters[giReviewCharSlot],
                                            g_character_event_kind_005ee65c, 0,
                                            g_effect_argument_005ed8c8, g_effect_argument_005ed914);
                        return 1;
                    }
                    AimAtCharacter(giReviewCharSlot, static_cast<int>(target_slot),
                                   W8_TARGETING_CONTEXT_CURRENT);
                    StartBreathCycle(giReviewCharSlot, 0);
                    return 1;
                }
                if (g_status_685170.buffers.characters[target_slot].condition_turns[19] == 0) {
                    if (g_camp_screen_0069c0f4->entry_mode != 7) {
                        TargetCharacterWithHeldItem005BA8E0(target_slot);
                        return 1;
                    }
                    ReportCastResult005BA620(static_cast<int>(target_slot));
                    return 1;
                }
            } else {
                if (gXStatus.iTargetingMode == 7) {
                    if (IsDeadCharacterTargetable(static_cast<int>(target_slot)) == 0) {
                        QueueCharacterEvent(&g_status_685170.buffers.characters[giReviewCharSlot],
                                            g_character_event_kind_005ee65c, 0,
                                            g_effect_argument_005ed8c8, g_effect_argument_005ed914);
                        return 1;
                    }
                    AimAtCharacterIndirect(giReviewCharSlot, static_cast<int>(target_slot),
                                           W8_TARGETING_CONTEXT_CURRENT);
                    StartBreathCycle(giReviewCharSlot, 0);
                    return 1;
                }
                if (giReviewCharSlot != static_cast<int>(target_slot)) {
                    SelectCampCharacter005B6B30(static_cast<int>(target_slot));
                    return 1;
                }
            }
        }
    } else {
        if (us_event != RIGHT_BUTTON_UP) {
            if (us_event != MOUSE_POS) {
                return 0;
            }
            if ((region->flags & W8_REGION_MOUSE_TRANSITION_MASK) != 0) {
                g_camp_screen_0069c0f4->redraw_flags |= 1u << (region->callback_id & 0x1f);
                if ((g_camp_screen_0069c0f4->entry_mode == 7 ||
                     g_camp_screen_0069c0f4->entry_mode == 9) &&
                    g_status_685170.buffers.characters[target_slot].condition_turns[19] == 0) {
                    if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
                        UpdateItemCursorForState005BAD20(1, 0, static_cast<int>(target_slot));
                        return 0;
                    }
                    UpdateItemCursorForState005BAD20(0, 0, static_cast<int>(target_slot));
                }
            }
            return 0;
        }
        if ((region->flags & W8_REGION_RIGHT_BUTTON_HELD) != 0 &&
            g_status_685170.item_in_cursor != 0) {
            TryGiveHeldItemToCampPortrait005B6C10(static_cast<int>(target_slot));
        }
    }
    return 1;
}

/* Opens the character editor (W8_SCREEN_CHARACTER, mode 1) for the reviewed
   party member. Help 2362. */
// FUNCTION: WIZ8 0x005B61A0
unsigned char CampOpenCharacterScreenRegionEvent(const InputAtom* event, W8Region* region)
{
    int us_event = event->usEvent;

    if (us_event == LEFT_BUTTON_DOWN) {
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
    } else {
        if (us_event != LEFT_BUTTON_UP) {
            if (us_event != MOUSE_POS) {
                return 0;
            }
            if ((region->flags & W8_REGION_MOUSE_TRANSITION_MASK) != 0) {
                g_camp_screen_0069c0f4->redraw_flags |= 0x200;
            }
            return 0;
        }
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
            g_current_screen_state.parameter_3 = g_value_0069c0f8;
            g_current_screen_state.parameter_2 = giReviewCharSlot;
            g_pending_screen_state.mode = 1;
            g_pending_screen_state.parameter_3 = g_value_0069c0f8;
            SetPendingScreenState(W8_SCREEN_CHARACTER);
            return 1;
        }
    }
    return 1;
}

/* Name-edit / camp input-mode toggle (help 2363): arms input_mode on press and
   clears it on release, activating the hover region while editing. */
// FUNCTION: WIZ8 0x005B6220
unsigned char CampNameEditRegionEvent(const InputAtom* event, W8Region* region)
{
    int us_event = event->usEvent;

    if (us_event == LEFT_BUTTON_DOWN) {
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        SetCampInputMode005A4BC0(1);
        ActivateDialogRegion(g_camp_screen_0069c0f4->hover_region);
    } else {
        if (us_event != LEFT_BUTTON_UP) {
            if (us_event == MOUSE_POS) {
                if ((region->flags & W8_REGION_MOUSE_TRANSITION_MASK) != 0) {
                    g_camp_screen_0069c0f4->redraw_flags |= 0x200;
                }
                return 0;
            }
            return 0;
        }
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) == 0) {
            return 1;
        }
        SetCampInputMode005A4BC0(0);
        ClearActiveRegionIfMatches(g_camp_screen_0069c0f4->hover_region);
    }
    g_camp_screen_0069c0f4->redraw_flags |= 0x7ff;
    return 1;
}

/* Shared handler for the five bottom page buttons in g_camp_page_buttons_0069c3ec. */
// FUNCTION: WIZ8 0x005B62C0
unsigned char CampPageButtonRegionEvent(const InputAtom* event, W8Region* region)
{
    int us_event = event->usEvent;
    unsigned short callback_id = region->callback_id;

    if (us_event <= LEFT_BUTTON_REPEAT) {
        if (us_event == LEFT_BUTTON_REPEAT || us_event == LEFT_BUTTON_DOWN) {
            g_camp_page_buttons_0069c3ec[callback_id]->OnLeftButtonDown(0);
            region->flags |= W8_REGION_LEFT_BUTTON_HELD;
            return 1;
        }
        if (us_event == LEFT_BUTTON_UP) {
            g_camp_page_buttons_0069c3ec[callback_id]->OnLeftButtonUp(0);
            if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
                region->flags &= ~W8_REGION_LEFT_BUTTON_HELD;
            }
            return 1;
        }
    } else if (us_event == MOUSE_POS) {
        if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
            g_camp_page_buttons_0069c3ec[callback_id]->OnMouseLeave(0);
            return 1;
        }
        if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
            g_camp_page_buttons_0069c3ec[callback_id]->OnMouseEnter(0);
            return 1;
        }
    }
    return 0;
}

/* Shared handler for the eight item-action buttons. */
// FUNCTION: WIZ8 0x005B6360
unsigned char CampItemActionRegionEvent(const InputAtom* event, W8Region* region)
{
    int us_event = event->usEvent;
    unsigned short callback_id = region->callback_id;

    if (us_event <= LEFT_BUTTON_REPEAT) {
        if (us_event == LEFT_BUTTON_REPEAT || us_event == LEFT_BUTTON_DOWN) {
            g_item_action_controls_69c3cc[callback_id]->OnLeftButtonDown(0);
            region->flags |= W8_REGION_LEFT_BUTTON_HELD;
            return 1;
        }
        if (us_event == LEFT_BUTTON_UP) {
            g_item_action_controls_69c3cc[callback_id]->OnLeftButtonUp(0);
            if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
                region->flags &= ~W8_REGION_LEFT_BUTTON_HELD;
            }
            return 1;
        }
    } else if (us_event == MOUSE_POS) {
        if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
            g_item_action_controls_69c3cc[callback_id]->OnMouseLeave(0);
            return 1;
        }
        if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
            g_item_action_controls_69c3cc[callback_id]->OnMouseEnter(0);
            return 1;
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x005b6400
void CreateRcsLevelUpPanel(void)
{
    g_level_up_panel_0069c3c4 = 0;
    g_level_up_button_0069c3c0 = 0;

    g_level_up_panel_0069c3c4 = new Controls(0xe9, 0x3f, 0xfb, 0x51, -1, 0, -1);
    if (g_level_up_panel_0069c3c4 == 0) {
        srAssertFail("gpLevelUpPanel", "C:\\Projects\\Wizardry 8\\Local Screens\\RCSCommon.cpp",
                     0x84e, 0);
    }

    g_level_up_button_0069c3c0 = new W8TextControl(g_level_up_panel_0069c3c4, 0xe7, 0, 0, 0x12,
                                                   0x12, 0xa7, 0, 0, 2, 1, 4, 3);
    if (g_level_up_button_0069c3c0 == 0) {
        srAssertFail("gpLevelUpButton", "C:\\Projects\\Wizardry 8\\Local Screens\\RCSCommon.cpp",
                     0x852, 0);
    }
    g_level_up_button_0069c3c0->m_primaryActivationCallback = OpenLevelUpCharacterScreen;
    g_level_up_panel_0069c3c4->SetEnabled(1);
    g_level_up_button_0069c3c0->SetActive(0);
}
/* Ask the first panel to redraw all of itself. A null rectangle is how
   Controls::Invalidate spells "the whole area", so these are not a separate
   one-argument redraw slot - they are the panel class Local Code\Controls.cpp
   models, reached through its second vtable slot. */
// FUNCTION: WIZ8 0x005b6590
void RedrawRcsLevelUpPanel(void)
{
    g_level_up_panel_0069c3c4->Invalidate(0);
}

/* Redraw the second. */
// FUNCTION: WIZ8 0x005b68d0
void RedrawRcsDismissPanel(void)
{
    g_dismiss_panel_0069c3c8->Invalidate(0);
}

/* Tear the first panel down, then destroy its separate widget. */
// FUNCTION: WIZ8 0x005b6540
void DestroyRcsLevelUpPanel(void)
{
    Controls* panel = g_level_up_panel_0069c3c4;

    if (panel != 0) {
        delete panel;
        g_level_up_panel_0069c3c4 = 0;
    }
    if (g_level_up_button_0069c3c0 != 0) {
        delete g_level_up_button_0069c3c0;
        g_level_up_button_0069c3c0 = 0;
    }
}

// FUNCTION: WIZ8 0x005b65a0
void UpdateRcsLevelUpPanel(void)
{
    bool enabled = IsCharacterReadyToAdvance(giReviewCharSlot);
    if (!enabled || gXStatus.fCombatMode ||
        (!g_status_685170.buffers.party_rows[giReviewCharSlot].flag_105 &&
         g_status_685170.game_started) ||
        gXStatus.fCampMode) {
        if (g_level_up_button_0069c3c0->m_active) {
            g_level_up_button_0069c3c0->SetActive(0);
        }
    } else if (!g_level_up_button_0069c3c0->m_active) {
        g_level_up_button_0069c3c0->SetActive(1);
        g_level_up_panel_0069c3c4->Invalidate(0);
    }
    g_level_up_panel_0069c3c4->Redraw();
}

/* Single level-up button (help 1984). Assembly writes region flags through the
   second parameter; Ghidra wrongly folded them into the event pointer. */
// FUNCTION: WIZ8 0x005B66B0
unsigned char CampLevelUpButtonRegionEvent(const InputAtom* event, W8Region* region)
{
    switch (event->usEvent) {
    case LEFT_BUTTON_DOWN:
    case LEFT_BUTTON_REPEAT:
        g_level_up_button_0069c3c0->OnLeftButtonDown(0);
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        return 1;
    case LEFT_BUTTON_UP:
        g_level_up_button_0069c3c0->OnLeftButtonUp(0);
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
            region->flags &= ~W8_REGION_LEFT_BUTTON_HELD;
        }
        return 1;
    case MOUSE_POS:
        if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
            g_level_up_button_0069c3c0->OnMouseLeave(0);
            return 1;
        }
        if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
            g_level_up_button_0069c3c0->OnMouseEnter(0);
            return 1;
        }
        break;
    }
    return 0;
}

// FUNCTION: WIZ8 0x005b6740
void CreateRcsDismissPanel(void)
{
    g_dismiss_panel_0069c3c8 = 0;
    g_dismiss_button_0069c400 = 0;

    g_dismiss_panel_0069c3c8 = new Controls(0xa7, 0x41, 0xbe, 0x51, -1, 0, -1);
    if (g_dismiss_panel_0069c3c8 == 0) {
        srAssertFail("gpDismissPanel", "C:\\Projects\\Wizardry 8\\Local Screens\\RCSCommon.cpp",
                     0x8cf, 0);
    }

    g_dismiss_button_0069c400 = new W8TextControl(g_dismiss_panel_0069c3c8, 0xe8, 0, 0, 0x17, 0x10,
                                                  0x113, 0, 0, 2, 1, 2, 3);
    if (g_dismiss_button_0069c400 == 0) {
        srAssertFail("gpDismissButton", "C:\\Projects\\Wizardry 8\\Local Screens\\RCSCommon.cpp",
                     0x8d3, 0);
    }
    g_dismiss_button_0069c400->m_primaryActivationCallback = ShowDismissCharacterDialog;
    g_dismiss_panel_0069c3c8->SetEnabled(1);
    g_dismiss_button_0069c400->SetActive(0);
}

// FUNCTION: WIZ8 0x005b6950
void ShowDismissCharacterDialog(void)
{
    if (!g_status_685170.buffers.party_rows[giReviewCharSlot].occupied) {
        srAssertFail("fCHAR_OCCUPIED(giReviewCharSlot)",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\RCSCommon.cpp", 0x90a, 0);
    }

    W8MessageDialogBase* dialog = static_cast<W8MessageDialogBase*>(CreateDialogByKind(1));
    dialog->SetClientExtent(0xfa, 200);

    W8Character* character = &g_status_685170.buffers.characters[giReviewCharSlot];
    const wchar_t* format;
    if (character->condition_turns[19] == 0) {
        if (character->condition_turns[W8_CONDITION_DEAD] == 0) {
            format = gppStringList[0x92d];
        } else {
            format = gppStringList[0x92e];
        }
    } else {
        format = gppStringList[0x92f];
    }
    dialog->SetMessage(FormatWideString(format, character->name), 1, 0x32, 1, 1, 1, 1, 0, 0x15e);
    SetDialogDestroyCallback(dialog, OnDismissCharacterDialogClosed);
    DisplayCampDialog(dialog);
}

// FUNCTION: WIZ8 0x005b6a60
void OnDismissCharacterDialogClosed(W8DialogBase* base)
{
    if (GetDialogResult(base) &&
        g_status_685170.buffers.party_rows[giReviewCharSlot].animation_0fa != -1) {
        gXStatus.review_character_slot = static_cast<unsigned short>(giReviewCharSlot);
        DismissSelectedPartyCharacter();
    }
}

/* Single dismiss button (help 2387). Same region-flag pattern as the level-up
   button. */
// FUNCTION: WIZ8 0x005B6AA0
unsigned char CampDismissButtonRegionEvent(const InputAtom* event, W8Region* region)
{
    switch (event->usEvent) {
    case LEFT_BUTTON_DOWN:
    case LEFT_BUTTON_REPEAT:
        g_dismiss_button_0069c400->OnLeftButtonDown(0);
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        return 1;
    case LEFT_BUTTON_UP:
        g_dismiss_button_0069c400->OnLeftButtonUp(0);
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
            region->flags &= ~W8_REGION_LEFT_BUTTON_HELD;
        }
        return 1;
    case MOUSE_POS:
        if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
            g_dismiss_button_0069c400->OnMouseLeave(0);
            return 1;
        }
        if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
            g_dismiss_button_0069c400->OnMouseEnter(0);
            return 1;
        }
        break;
    }
    return 0;
}

/* The same for the second panel and its widget. */
// FUNCTION: WIZ8 0x005b6880
void DestroyRcsDismissPanel(void)
{
    Controls* panel = g_dismiss_panel_0069c3c8;

    if (panel != 0) {
        delete panel;
        g_dismiss_panel_0069c3c8 = 0;
    }
    if (g_dismiss_button_0069c400 != 0) {
        delete g_dismiss_button_0069c400;
        g_dismiss_button_0069c400 = 0;
    }
}

/* Bring the second panel up to date. Its widget is available only for the first
   two party slots, out of combat and out of camp; enabling it also invalidates the
   panel, disabling it does not. Either way the panel is then updated. */
// FUNCTION: WIZ8 0x005b68e0
void UpdateRcsDismissPanel(void)
{
    if ((giReviewCharSlot == 0 || giReviewCharSlot == 1) && gXStatus.fCombatMode == 0 &&
        gXStatus.fCampMode == 0) {
        if (!g_dismiss_button_0069c400->m_active) {
            g_dismiss_button_0069c400->SetActive(1);
            g_dismiss_panel_0069c3c8->Invalidate(0);
        }
    } else if (g_dismiss_button_0069c400->m_active) {
        g_dismiss_button_0069c400->SetActive(0);
    }
    g_dismiss_panel_0069c3c8->Redraw();
}

/* Formation storage. Physical retail TU attribution remains unresolved;
   the lifecycle interface is owned by MGSFormation.h. */

// GLOBAL: WIZ8 0x0069c2ec
Controls* g_formation_panel;
// GLOBAL: WIZ8 0x0069c344
W8TextControl* g_formation_cell_controls[15];
// GLOBAL: WIZ8 0x0069c384
W8TextControl* g_formation_cell_overlays[15];
// GLOBAL: WIZ8 0x0069c2f8
W8TextControl* g_formation_action_buttons[3];

/* Release the formation board, compass and overlay sprites. */
// FUNCTION: WIZ8 0x005B1C00
void ReleaseFormationBoard(void)
{
    if (g_level_block->formation_board_sprite != 0) {
        ReleaseObject004257F0(g_level_block->formation_board_sprite);
        g_level_block->formation_board_sprite = 0;
    }
    if (g_level_block->formation_compass_sprite != 0) {
        ReleaseObject004257F0(g_level_block->formation_compass_sprite);
        g_level_block->formation_compass_sprite = 0;
    }
    if (g_level_block->formation_overlay_sprite != 0) {
        ReleaseObject004257F0(g_level_block->formation_overlay_sprite);
        g_level_block->formation_overlay_sprite = 0;
    }
}

// FUNCTION: WIZ8 0x005B2580
void DestroyFormationPanel(void)
{
    Controls* panel = g_formation_panel;
    if (panel != 0) {
        delete panel;
        g_formation_panel = 0;
    }
    for (int index = 0; index < 15; ++index) {
        if (g_formation_cell_controls[index] != 0) {
            delete g_formation_cell_controls[index];
            g_formation_cell_controls[index] = 0;
        }
        if (g_formation_cell_overlays[index] != 0) {
            delete g_formation_cell_overlays[index];
            g_formation_cell_overlays[index] = 0;
        }
    }
    W8TextControl** control = g_formation_action_buttons;
    do {
        if (*control != 0) {
            delete *control;
            *control = 0;
        }
        ++control;
    } while (control < g_formation_action_buttons + 3);
}

/* The upper-left character block. While an input mode is up it lists the
   reviewed character's professions and their levels instead of the portrait;
   otherwise it repaints the portrait and vitals, the name/gender/profession
   caption, and the eight-slot party strip. */
// FUNCTION: WIZ8 0x005b4000
void DrawCampHeader005B4000(void)
{
    W8CampScreenState0069C0F4* state = g_camp_screen_0069c0f4;
    W8Character* character = g_value_0069c0f8;
    unsigned int row;
    unsigned int slot;
    int profession;
    int name_x;
    int level_x;
    int y;
    int left;
    int top;
    int band_x;
    int band_y;
    int image;
    int sub_image;

    if (state->input_mode != 0) {
        if ((state->redraw_flags & 0x7ff) != 0) {
            InvalidateRegion(0, 0, 0x136, 0xa5, 0);
            ColorFillVideoSurfaceArea(0xfffffff2, 0, 0, 0x136, 0xa5, 0x8000);
            DrawCatalogImage(-14, 0x123, 0, 0, 0, 0, 2, 0);
            DrawRcsText(gppStringList[0x8c2], 0xb, 0xd, 0x11e,
                        g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C);
            row = 0;
            for (profession = 0; profession < W8_PROFESSION_COUNT; ++profession) {
                if (character->profession_levels[profession] != 0 ||
                    character->current_profession == profession) {
                    if ((row & ~7u) == 0) {
                        name_x = 0xf;
                        level_x = 0x78;
                    } else {
                        name_x = 0xa1;
                        level_x = 0x10a;
                    }
                    y = (row & 7) * 0xe + 0x24;
                    DrawRcsText(
                        gppStringList[g_profession_name_message_ids_61e3f0[profession]], name_x, y,
                        0x68, g_W8TextBufferLayoutMask005ED548 | g_W8TextBufferLayoutMask005ED554);
                    swprintf(state->caption, g_format_d_0060aa20,
                             character->profession_levels[profession]);
                    DrawRcsText(state->caption, level_x, y, 0x20,
                                g_W8TextBufferLayoutMask005ED554 |
                                    g_W8TextBufferLayoutMask005ED54C);
                    ++row;
                }
            }
        }
        return;
    }

    if ((state->redraw_flags & 0x400) != 0) {
        InvalidateRegion(0, 0, 0x136, 0xa5, 0);
        ColorFillVideoSurfaceArea(0xfffffff2, 0, 0, 0x136, 0xa5, 0x8000);
        DrawCatalogImage(-14, 0x10f, 0, 0, 0, 0, 2, 0);
    }
    if ((state->redraw_flags & 0x100) != 0) {
        if (character->hp_current == 0) {
            DrawCatalogImage(-14, g_dead_portrait_catalog_ids_6488d0[character->race][1], 0, 0,
                             0xa4, 0xc, 2, 0);
        } else {
            RenderPartyPortrait0052EB00(character->table_value_0079, 0xa4, 0xc, 2, 1,
                                        giReviewCharSlot);
        }
        DrawCatalogImage(-14, 0x10f, 0, 9, 0xa4, 0xc, 2, 0);
        if (state->unknown_d40[0] != 0) {
            DrawCatalogImage(-14, 0x116, 0, 0, 0xa4, 0xc, 2, 0);
        }
        if (gXStatus.fCombatMode == 0) {
            g_level_up_panel_0069c3c4->Invalidate(0);
            g_dismiss_panel_0069c3c8->Invalidate(0);
        }
        if (character->highest_condition == 0) {
            DrawCatalogImage(-14, 0x2f, 0, 0, 0xa5, 0xe, 2, 0);
        } else {
            DrawCatalogImage(-14, character->highest_condition + 0xb6, 0, 0, 0xa5, 0xe, 2, 0);
            DrawCatalogImage(-14, 0x61, 0, 0, 0xa4, 0xd, 2, 0);
        }
        if (character->enchantment_top == 0) {
            DrawCatalogImage(-14, 0x30, 0, 0, 0xee, 0xe, 2, 0);
        } else {
            DrawCatalogImage(-14, character->enchantment_top + 0xc9, 0, 0, 0xee, 0xe, 2, 0);
            DrawCatalogImage(-14, 0x61, 0, 0, 0xed, 0xd, 2, 0);
        }
        InvalidateRegion(0xa4, 0xc, 0xfe, 0x54, 1);
        DrawCampVitals005B4790();
        DrawCampHands005B4BD0();
    }
    if ((state->redraw_flags & 0x200) != 0) {
        SetFont(g_wiz_text_bold_font_683664);
        SetObjectShade(g_wiz_text_bold_font_object_68365c, 4);
        if (state->hover_region == 0xf2) {
            SetFontObjectPalette16BPP(g_wiz_text_bold_font_683664, g_font_state_palettes_68ee1c[1]);
        }
        wcscpy(state->caption, character->name);
        gprintfDirty((0xba - StringPixLength(state->caption, g_wiz_text_bold_font_683664)) / 2 +
                         0x74,
                     0x60, const_cast<UINT16*>(g_format_s_006068e4), state->caption);
        SetFontObjectPalette16BPP(g_wiz_text_bold_font_683664, g_font_palette_wiz_text_bold_68ee0c);
        SetFont(g_font_683660);
        SetObjectShade(g_wiz_text_font_secondary_object_683680, 4);
        wcscpy(state->caption,
               gppStringList[g_gender_name_message_rows_61e430[character->gender][0]]);
        wcscat(state->caption, L" ");
        wcscat(state->caption, gppStringList[g_race_name_message_ids_61e3d0[character->race]]);
        gprintfDirty((0xba - StringPixLength(state->caption, g_font_683660)) / 2 + 0x74, 0x6f,
                     const_cast<UINT16*>(g_format_s_006068e4), state->caption);
        if (state->hover_region == 0xf3) {
            SetFontObjectPalette16BPP(g_font_683660, g_font_state_palettes_68ee1c[1]);
        }
        wcscpy(state->caption,
               gppStringList[g_profession_name_message_ids_61e3f0[character->current_profession]]);
        gprintfDirty((0xba - StringPixLength(state->caption, g_font_683660)) / 2 + 0x74, 0x7c,
                     const_cast<UINT16*>(g_format_s_006068e4), state->caption);
        SetFontObjectPalette16BPP(g_font_683660, g_colour_68ee08);
        SetFont(g_font_683660);
        SetObjectShade(g_wiz_text_font_secondary_object_683680, 4);
        swprintf(
            state->caption, g_format_s_d_paren_s_0064dad0, gppStringList[0x91a], character->level,
            gppStringList[g_profession_level_name_message_ids_61e688[character->current_profession]
                                                                    [character->level_band]]);
        gprintfDirty((0xba - StringPixLength(state->caption, g_font_683660)) / 2 + 0x74, 0x89,
                     const_cast<UINT16*>(g_format_s_006068e4), state->caption);
    }
    for (slot = 0; slot < 8; ++slot) {
        if ((state->redraw_flags & (1 << (slot & 0x1f))) != 0) {
            band_y = (slot >> 1) * 0x27;
            band_x = (slot & 1) * 0x30;
            left = band_x + 6;
            top = band_y + 5;
            if (giReviewCharSlot == -1 || g_status_685170.buffers.party_rows[slot].occupied == 0) {
                DrawCatalogImage(-14, 0x10f, 0, slot + 1, band_x + 5, band_y + 4, 2, 0);
            } else {
                W8Character* member = &g_status_685170.buffers.characters[slot];
                if (member->hp_current == 0) {
                    image = g_dead_portrait_catalog_ids_6488d0[member->race][0];
                    sub_image = 0;
                } else {
                    sub_image = member->table_value_0079;
                    image = 0x13;
                }
                DrawCatalogImage(-14, image, sub_image, 0, left, top, 2, 0);
                if (!CanSelectRcsPartySlot(slot)) {
                    ShadowVideoSurfaceRect(0xfffffff2, left, top, band_x + 0x32, band_y + 0x28);
                    ShadowVideoSurfaceRect(0xfffffff2, left, top, band_x + 0x32, band_y + 0x28);
                }
                if (state->hover_region == slot + 0xea) {
                    DrawCatalogImage(-14, 0x117, 0, 0, left, top, 2, 0);
                } else if (static_cast<int>(slot) == giReviewCharSlot) {
                    DrawCatalogImage(-14, 0x117, 0, 1, left, top, 2, 0);
                }
            }
            InvalidateRegion(left, top, band_x + 0x33, band_y + 0x29, 0);
        }
    }
}

/* The health/stamina/spell bars beside the portrait: the three frame images,
   the shaded depletion on each, the current-profession label and, under the
   numeric-hit-points option, the raw hit-point value. */
// FUNCTION: WIZ8 0x005b4790
void DrawCampVitals005B4790(void)
{
    int image;
    int numeric;
    int frame_column;
    int hp_row;
    int stamina_row;
    int spell_column;
    int hp_frame;
    int stamina_frame;
    int spell_frame;
    int bar_top;
    unsigned int hp_fill;
    unsigned int stamina_fill;
    unsigned int spell_fill;
    int spell_left;
    unsigned int gap;
    W8ControlsRect bounds;
    W8TextBuffer* text;
    wchar_t* formatted;

    if (g_settings_6850c8.numeric_hit_points == 0) {
        image = 1;
    } else {
        image = 3;
    }
    DrawCatalogImage(-14, 0x80, 0, image, 0x10c, 0xb, 2, 0);
    if (g_value_0069c0f8->hp_current == 0) {
        return;
    }
    if (g_settings_6850c8.numeric_hit_points == 0) {
        frame_column = 0x17;
        numeric = 0;
        hp_row = 6;
        stamina_row = 0xa;
        spell_column = 0xe;
        hp_frame = 0x4f;
        stamina_frame = 0x50;
        spell_frame = 0x51;
    } else {
        frame_column = 0x11;
        numeric = 1;
        hp_row = 3;
        stamina_row = 9;
        spell_column = 0xf;
        hp_frame = 0x52;
        stamina_frame = 0x53;
        spell_frame = 0x54;
    }
    if (g_value_0069c0f8->hp_max == 0) {
        srAssertFail("gpReviewPC->uiHPMax",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\RCSCommon.cpp", 0x373, 0);
    }
    hp_fill = g_value_0069c0f8->hp_current * 0x2d / g_value_0069c0f8->hp_max;
    if (g_value_0069c0f8->stamina_max == 0) {
        srAssertFail("gpReviewPC->uiStaminaMax",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\RCSCommon.cpp", 0x377, 0);
    }
    stamina_fill = (g_value_0069c0f8->stamina < 0 ? 0 : g_value_0069c0f8->stamina) * 0x2d /
                   g_value_0069c0f8->stamina_max;
    if (SumCharacterSpellPoints(g_value_0069c0f8) == 0) {
        spell_fill = 0;
    } else {
        spell_left = SumCharacterSpellPointsLeft(g_value_0069c0f8);
        if (spell_left < 0) {
            spell_left = 0;
        } else {
            spell_left = SumCharacterSpellPointsLeft(g_value_0069c0f8);
        }
        spell_fill = spell_left * 0x2d / SumCharacterSpellPoints(g_value_0069c0f8);
    }
    bar_top = frame_column + 0xb;
    DrawCatalogImage(-14, hp_frame, 0, 0, hp_row + 0x10c, bar_top, 2, 0);
    gap = 0x2d - hp_fill;
    if (gap != 0) {
        ShadeStatusBarGap0059A110(gap, hp_row + 0x10c, frame_column - numeric + 0xb);
    }
    DrawCatalogImage(-14, stamina_frame, 0, 0, stamina_row + 0x10c, bar_top, 2, 0);
    gap = 0x2d - stamina_fill;
    if (gap != 0) {
        ShadeStatusBarGap0059A110(gap, stamina_row + 0x10c, frame_column - numeric + 0xb);
    }
    if (SumCharacterSpellPoints(g_value_0069c0f8) != 0) {
        DrawCatalogImage(-14, spell_frame, 0, 0, spell_column + 0x10c, bar_top, 2, 0);
        gap = 0x2d - spell_fill;
        if (gap != 0) {
            ShadeStatusBarGap0059A110(gap, spell_column + 0x10c, frame_column - numeric + 0xb);
        }
    }
    bounds.left = 0x10e;
    bounds.right = 0x121;
    bounds.top = 0xd;
    bounds.bottom = 0x17;
    text = new W8TextBuffer(
        &bounds, 0, g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C, 0, 4);
    if (g_status_685170.game_started == 0) {
        text->m_fontStateIndex = 8;
    } else {
        text->m_fontStateIndex =
            g_status_685170.buffers.party_rows[giReviewCharSlot].party_order_index;
    }
    text->SetText(
        gppStringList[g_profession_name_message_ids_61e3f0[g_value_0069c0f8->current_profession +
                                                           0x10]],
        g_smfnt_font_683694);
    text->RenderToTarget(0, 0, -14);
    if (g_settings_6850c8.numeric_hit_points != 0) {
        bounds.left = 0x10e;
        bounds.right = 0x120;
        bounds.top = 0x49;
        bounds.bottom = 0x51;
        text->SetLayoutBounds(&bounds, 1, 0);
        text->m_fontStateIndex = -1;
        formatted = FormatWideString(g_format_d_0060aa20, g_value_0069c0f8->hp_current);
        text->SetText(formatted, g_smfnt_font_683694);
        text->RenderToTarget(0, 0, -14);
    }
    InvalidateRegion(0x10c, bar_top, 0x124, frame_column + 0x38, 0);
    delete text;
}

/* The two hands under the vitals: each draws its equipment icon (or the
   race's empty-hand image), the stack count when the item has one, and the
   armor-class line under the left hand in the load-category palette. The
   off-hand is skipped while a two-handed item fills the primary slot. */
// FUNCTION: WIZ8 0x005b4bd0
void DrawCampHands005B4BD0(void)
{
    bool two_handed;
    unsigned char count;
    int item_id;
    int hand_image;
    wchar_t text[4];
    unsigned short* palette;

    item_id = g_value_0069c0f8->equipment[6].item_id;
    if (item_id == -1 || (g_item_records[item_id].flags_041 & 4) == 0) {
        two_handed = false;
        hand_image = 0;
    } else {
        two_handed = true;
        hand_image = 2;
    }
    DrawCatalogImage(-14, 0x80, 0, hand_image, 0x81, 0xb, 2, 0);
    item_id = g_value_0069c0f8->equipment[6].item_id;
    if (item_id == -1) {
        DrawCatalogImage(-14, g_empty_hand_catalog_ids_649dd4[g_value_0069c0f8->race * 2], 0, 0,
                         0x85, 0x22, 2, 0);
    } else {
        DrawCatalogImage(-14, g_item_video_objects_68ec68.GetOrCreateVideoObject(item_id), 0, 2,
                         0x84, 0x22, 2, 0);
        count = g_value_0069c0f8->equipment[6].stack_count;
        if (count != 0) {
            swprintf(text, g_format_d_0060aa20, count);
            SetFont(g_smfnt_font_683694);
            SetFontObjectPalette16BPP(g_smfnt_font_683694, g_font_palette_smfnt_68ee10);
            gprintf(0x89 - (StringPixLength(text, g_smfnt_font_683694) + 1) / 2, 0x31,
                    const_cast<UINT16*>(g_format_s_006068e4), text);
        }
    }
    if (!two_handed) {
        item_id = g_value_0069c0f8->equipment[7].item_id;
        if (item_id == -1) {
            DrawCatalogImage(-14, g_empty_hand_catalog_ids_649dd4[g_value_0069c0f8->race * 2 + 1],
                             0, 0, 0x85, 0x3a, 2, 0);
        } else {
            DrawCatalogImage(-14, g_item_video_objects_68ec68.GetOrCreateVideoObject(item_id), 0, 2,
                             0x84, 0x3a, 2, 0);
            count = g_value_0069c0f8->equipment[7].stack_count;
            if (count != 0) {
                swprintf(text, g_format_d_0060aa20, count);
                SetFont(g_smfnt_font_683694);
                SetFontObjectPalette16BPP(g_smfnt_font_683694, g_font_palette_smfnt_68ee10);
                gprintf(0x8c - (StringPixLength(text, g_smfnt_font_683694) + 1) / 2, 0x49,
                        const_cast<UINT16*>(g_format_s_006068e4), text);
            }
        }
    }
    SetFont(g_smfnt_font_683694);
    swprintf(text, g_format_d_0060aa20, g_value_0069c0f8->armor_class_average);
    palette = g_font_palette_smfnt_68ee10;
    if (g_value_0069c0f8->load_category != 0) {
        palette = g_font_state_palettes_68ee1c
            [g_load_category_palettes_648c48[g_value_0069c0f8->load_category]];
    }
    SetFontObjectPalette16BPP(g_smfnt_font_683694, palette);
    gprintf((0xd - StringPixLength(text, g_smfnt_font_683694)) / 2 + 0x84, 0xe,
            const_cast<UINT16*>(g_format_s_006068e4), text);
    InvalidateRegion(0x81, 0xb, 0x95, 0x53, 0);
}

/* The shared bottom panel the five page buttons and the eight item-action
   controls live on; it is created with the camp screen and torn down by
   DestroyCampButtonPanel005B55F0. Returns 0 when any allocation fails. */
// FUNCTION: WIZ8 0x005b4eb0
int CreateCampButtonPanel005B4EB0(void)
{
    int index;

    g_item_actions_panel_0069c404 = 0;
    for (index = 0; index < 5; ++index) {
        if (g_camp_page_buttons_0069c3ec[index] != 0) {
            g_camp_page_buttons_0069c3ec[index] = 0;
        }
    }
    for (index = 0; index < 8; ++index) {
        if (g_item_action_controls_69c3cc[index] != 0) {
            g_item_action_controls_69c3cc[index] = 0;
        }
    }
    RegionSetEnable(0x2d);
    RegionSetEnable(0x2e);
    g_item_actions_panel_0069c404 = new Controls(0, 0x1c2, 0x280, 0x1e0, 0x110, 0, 0);
    if (g_item_actions_panel_0069c404 != 0) {
        g_camp_page_buttons_0069c3ec[0] = new W8TextControl(
            g_item_actions_panel_0069c404, 0x11f, 0x1a4, 0, 0x1d0, 0x1e, 0x111, 0, 0, 4, 1, 2, 3);
        g_camp_page_buttons_0069c3ec[1] =
            new W8TextControl(g_item_actions_panel_0069c404, 0x120, 0x1d0, 0, 0x1fc, 0x1e, 0x111, 0,
                              10, 0xe, 0xb, 0xc, 0xd);
        g_camp_page_buttons_0069c3ec[2] =
            new W8TextControl(g_item_actions_panel_0069c404, 0x121, 0x1fc, 0, 0x228, 0x1e, 0x111, 0,
                              0xf, 0x13, 0x10, 0x11, 0x12);
        g_camp_page_buttons_0069c3ec[3] = new W8TextControl(
            g_item_actions_panel_0069c404, 0x122, 0x228, 0, 0x254, 0x1e, 0x111, 0, 5, 9, 6, 7, 8);
        g_camp_page_buttons_0069c3ec[4] =
            new W8TextControl(g_item_actions_panel_0069c404, 0x123, 0x254, 0, 0x280, 0x1e, 0x106, 0,
                              0x10, -1, 0x11, 0x12, 0x13);
        for (index = 0; index < 5; ++index) {
            if (g_camp_page_buttons_0069c3ec[index] == 0) {
                return 0;
            }
        }
        g_camp_page_buttons_0069c3ec[0]->AddLayoutFlags(g_W8TextControlMask005ED578);
        g_camp_page_buttons_0069c3ec[1]->AddLayoutFlags(g_W8TextControlMask005ED578);
        g_camp_page_buttons_0069c3ec[2]->AddLayoutFlags(g_W8TextControlMask005ED578);
        g_camp_page_buttons_0069c3ec[3]->AddLayoutFlags(g_W8TextControlMask005ED578);
        g_camp_page_buttons_0069c3ec[0]->m_primaryActivationCallback = CampPageAction005B5AE0;
        g_camp_page_buttons_0069c3ec[1]->m_primaryActivationCallback = CampPageAction005B5B30;
        g_camp_page_buttons_0069c3ec[2]->m_primaryActivationCallback = CampPageAction005B5B80;
        g_camp_page_buttons_0069c3ec[3]->m_primaryActivationCallback = CampPageAction005B5BD0;
        g_camp_page_buttons_0069c3ec[4]->m_primaryActivationCallback =
            CampPageDismissAction005B5C20;
        g_item_action_controls_69c3cc[0] = new W8TextControl(
            g_item_actions_panel_0069c404, 0x124, 0, 0, 0x2c, 0x1e, 0x112, 0, 0, 2, 1, 4, 3);
        g_item_action_controls_69c3cc[1] =
            new W8TextControl(g_item_actions_panel_0069c404, 0x125, 0x2c, 0, 0x58, 0x1e, 0x112, 0,
                              10, 0xc, 0xb, 0xe, 0xd);
        g_item_action_controls_69c3cc[2] = new W8TextControl(
            g_item_actions_panel_0069c404, 0x126, 0x58, 0, 0x84, 0x1e, 0x112, 0, 5, 7, 6, 9, 8);
        g_item_action_controls_69c3cc[3] =
            new W8TextControl(g_item_actions_panel_0069c404, 0x127, 0x84, 0, 0xb0, 0x1e, 0x112, 0,
                              0xf, 0x11, 0x10, 0x13, 0x12);
        g_item_action_controls_69c3cc[4] =
            new W8TextControl(g_item_actions_panel_0069c404, 0x128, 0xb0, 0, 0xdc, 0x1e, 0x112, 0,
                              0x14, 0x16, 0x15, 0x18, 0x17);
        g_item_action_controls_69c3cc[5] =
            new W8TextControl(g_item_actions_panel_0069c404, 0x129, 0x134, 0, 0x160, 0x1e, 0x112, 0,
                              0x19, 0x1b, 0x1a, 0x1d, 0x1c);
        g_item_action_controls_69c3cc[6] =
            new W8TextControl(g_item_actions_panel_0069c404, 0x12a, 0xdc, 0, 0x108, 0x1e, 0x112, 0,
                              0x1e, 0x20, 0x1f, 0x22, 0x21);
        g_item_action_controls_69c3cc[7] =
            new W8TextControl(g_item_actions_panel_0069c404, 0x12b, 0x108, 0, 0x134, 0x1e, 0x112, 0,
                              0x23, 0x25, 0x24, 0x27, 0x26);
        index = 0;
        while (g_item_action_controls_69c3cc[index] != 0) {
            ++index;
            if (index > 7) {
                g_item_action_controls_69c3cc[0]->AddLayoutFlags(g_W8TextControlMask005ED578);
                g_item_action_controls_69c3cc[1]->AddLayoutFlags(g_W8TextControlMask005ED578);
                g_item_action_controls_69c3cc[2]->AddLayoutFlags(g_W8TextControlMask005ED578);
                g_item_action_controls_69c3cc[3]->AddLayoutFlags(g_W8TextControlMask005ED578);
                g_item_action_controls_69c3cc[4]->AddLayoutFlags(g_W8TextControlMask005ED578);
                g_item_action_controls_69c3cc[5]->AddLayoutFlags(g_W8TextControlMask005ED578);
                g_item_action_controls_69c3cc[6]->AddLayoutFlags(g_W8TextControlMask005ED578);
                g_item_action_controls_69c3cc[7]->AddLayoutFlags(g_W8TextControlMask005ED578);
                g_item_action_controls_69c3cc[0]->m_primaryActivationCallback =
                    CampItemAction005B5C30;
                g_item_action_controls_69c3cc[1]->m_primaryActivationCallback =
                    CampItemAction005B5C80;
                g_item_action_controls_69c3cc[2]->m_primaryActivationCallback =
                    CampItemAction005B5CD0;
                g_item_action_controls_69c3cc[3]->m_primaryActivationCallback =
                    CampItemAction005B5D10;
                g_item_action_controls_69c3cc[4]->m_primaryActivationCallback =
                    CampItemAction005B5D50;
                g_item_action_controls_69c3cc[5]->m_primaryActivationCallback =
                    CampItemAction005B5D90;
                g_item_action_controls_69c3cc[6]->m_primaryActivationCallback =
                    CampItemAction005B5DF0;
                g_item_action_controls_69c3cc[7]->m_primaryActivationCallback =
                    CampItemAction005B5E60;
                g_item_actions_panel_0069c404->SetEnabled(1);
                g_camp_page_buttons_0069c3ec[0]->EnableSecondaryState(0);
                return 1;
            }
        }
    }
    return 0;
}

/* Tear down the shared button panel: delete the Controls parent and each page
   and item-action control, then disable both region sets. */
// FUNCTION: WIZ8 0x005b55f0
void DestroyCampButtonPanel005B55F0(void)
{
    Controls* panel;
    int index;

    panel = g_item_actions_panel_0069c404;
    if (panel != 0) {
        delete panel;
        g_item_actions_panel_0069c404 = 0;
    }
    for (index = 0; index < 5; ++index) {
        if (g_camp_page_buttons_0069c3ec[index] != 0) {
            delete g_camp_page_buttons_0069c3ec[index];
            g_camp_page_buttons_0069c3ec[index] = 0;
        }
    }
    for (index = 0; index < 8; ++index) {
        if (g_item_action_controls_69c3cc[index] != 0) {
            delete g_item_action_controls_69c3cc[index];
            g_item_action_controls_69c3cc[index] = 0;
        }
    }
    RegionSetDisable(0x2e);
    RegionSetDisable(0x2d);
}

/* Refresh the enabled state of the eight item-action controls. Every button
   is enabled only while the items page is up and the party exists; the
   per-button cases then gate on the held item, combat, the reviewed slot's
   eligibility and the relevant item/spell tests. When invalidate is set the
   whole panel is additionally marked dirty before it is redrawn. */
// FUNCTION: WIZ8 0x005b5670
void RefreshCampItemActions005B5670(unsigned char invalidate)
{
    int index;
    W8SpellRuntimeRecord* spell;
    W8ItemInstance* held = &g_status_685170.item_in_hand_235b;

    for (index = 0; index < 8; ++index) {
        W8TextControl* control = g_item_action_controls_69c3cc[index];
        if (g_camp_screen_0069c0f4->page == 0 && g_status_685170.game_started != 0) {
            switch (index) {
            case 0:
                control->SetEnabled(IsPartySlotEligible00524A10(giReviewCharSlot) != 0);
                continue;
            case 1:
                control->SetEnabled(gXStatus.fCombatMode == 0 &&
                                    IsPartySlotEligible00524A10(giReviewCharSlot) != 0);
                continue;
            case 2:
                if (g_status_685170.item_in_cursor == 0) {
                    control->SetEnabled(1);
                    continue;
                }
                if (CanSplitItemStack005BAA50(held) == 0) {
                    control->SetEnabled(0);
                    continue;
                }
                if (gXStatus.fCombatMode == 0 ||
                    (g_combat_state->flag_a50 != 0 &&
                     g_status_685170.buffers.party_rows[giReviewCharSlot].pending_action == 9)) {
                    control->SetEnabled(1);
                    continue;
                }
                if (IsEquippableItemClass005A6310(held) != 0 &&
                    gXStatus.held_item_source == giReviewCharSlot) {
                    control->SetEnabled(1);
                    continue;
                }
                break;
            case 3:
                if (gXStatus.fCombatMode != 0) {
                    control->SetEnabled(0);
                    continue;
                }
                if (IsPartySlotEligible00524A10(giReviewCharSlot) == 0) {
                    control->SetEnabled(0);
                    continue;
                }
                if (gXStatus.fCampMode != 0 || gXStatus.fLockInteract != 0 ||
                    gXStatus.fTrapInteract != 0) {
                    control->SetEnabled(0);
                    continue;
                }
                if (g_status_685170.item_in_cursor == 0) {
                    control->SetEnabled(1);
                    continue;
                }
                control->SetEnabled(CanCharacterUseItemEntry005BAA10(g_value_0069c0f8, held) != 0);
                continue;
            case 4:
                control->SetEnabled(1);
                continue;
            case 5:
                control->SetEnabled(IsPartySlotEligible00524A10(giReviewCharSlot) != 0 &&
                                    CharacterHasTrait00547940(g_value_0069c0f8, 0xd) != 0);
                continue;
            case 6:
                if (g_value_0069c0f8->spell_learned[0x17] != 1) {
                    break;
                }
                if (IsPartySlotEligible00524A10(giReviewCharSlot) == 0 ||
                    gXStatus.fCombatMode != 0) {
                    break;
                }
                spell = &g_spell_records[0x17];
                if (spell->spell_point_cost > g_value_0069c0f8->sp_left[spell->realm]) {
                    break;
                }
                if (SpellUsableNow(0x17, 0) == 0) {
                    break;
                }
                if (g_status_685170.item_in_cursor == 0) {
                    control->SetEnabled(1);
                    continue;
                }
                control->SetEnabled(CanItemLeaveItsSlot(held) != 0);
                continue;
            case 7:
                if (g_value_0069c0f8->spell_learned[0x3a] != 1) {
                    break;
                }
                if (IsPartySlotEligible00524A10(giReviewCharSlot) == 0 ||
                    gXStatus.fCombatMode != 0) {
                    break;
                }
                spell = &g_spell_records[0x3a];
                if (spell->spell_point_cost > g_value_0069c0f8->sp_left[spell->realm]) {
                    break;
                }
                control->SetEnabled(SpellUsableNow(0x3a, 0) != 0);
                continue;
            }
            control->SetEnabled(0);
        } else {
            control->SetEnabled(0);
        }
    }
    if (invalidate != 0) {
        g_item_actions_panel_0069c404->Invalidate(0);
    }
    g_item_actions_panel_0069c404->Redraw();
}

/* Switch the items-page action mode: every action button's secondary state is
   cleared, the mode maps onto the targeting mode and the button it highlights,
   and mode zero also drops a held item cursor back into the pool. */
// FUNCTION: WIZ8 0x005b59b0
void SetCampItemActionMode005B59B0(char mode)
{
    int index;
    short selected = -1;
    int targeting;
    W8TextControl** control;

    for (control = g_item_action_controls_69c3cc, index = 8; index != 0; ++control, --index) {
        if (static_cast<unsigned char>((*control)->m_stateFlags & g_W8TextControlMask005ED570) !=
            0) {
            (*control)->DisableSecondaryState(0);
        }
    }
    g_camp_screen_0069c0f4->entry_mode = mode;
    switch (mode) {
    case 1:
        targeting = 6;
        selected = 1;
        break;
    case 2:
    case 8:
        targeting = 6;
        selected = 6;
        break;
    case 3:
        targeting = 6;
        selected = 0;
        break;
    case 4:
        targeting = 6;
        selected = 2;
        break;
    case 5:
        targeting = 6;
        selected = 3;
        break;
    case 6:
        targeting = 6;
        selected = 4;
        break;
    case 7:
        targeting = 1;
        selected = 5;
        break;
    case 9:
        targeting = 1;
        selected = 7;
        break;
    default:
        targeting = 0;
        break;
    }
    SetTargetingMode(targeting);
    if (mode == 0 && g_status_685170.item_in_cursor != 0) {
        SetItemCursor(0);
    }
    if (selected != -1 &&
        static_cast<unsigned char>(g_item_action_controls_69c3cc[selected]->m_stateFlags &
                                   g_W8TextControlMask005ED570) == 0) {
        g_item_action_controls_69c3cc[selected]->EnableSecondaryState(0);
    }
    g_camp_screen_0069c0f4->redraw_flags |= 0x1000;
}

/* The five page-button callbacks share one shape: while this button's
   secondary state is up, clear it on every page button and switch to the
   button's page; then raise the pressed button's secondary state. */
// FUNCTION: WIZ8 0x005b5ae0
static void CampPageAction005B5AE0(void)
{
    int index;

    if (static_cast<unsigned char>(g_camp_page_buttons_0069c3ec[0]->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        for (index = 0; index < 5; ++index) {
            if (static_cast<unsigned char>(g_camp_page_buttons_0069c3ec[index]->m_stateFlags &
                                           g_W8TextControlMask005ED570) != 0) {
                g_camp_page_buttons_0069c3ec[index]->DisableSecondaryState(0);
            }
        }
        SwitchCampPage005A4540(0);
    }
    g_camp_page_buttons_0069c3ec[0]->EnableSecondaryState(0);
}

// FUNCTION: WIZ8 0x005b5b30
static void CampPageAction005B5B30(void)
{
    int index;

    if (static_cast<unsigned char>(g_camp_page_buttons_0069c3ec[1]->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        for (index = 0; index < 5; ++index) {
            if (static_cast<unsigned char>(g_camp_page_buttons_0069c3ec[index]->m_stateFlags &
                                           g_W8TextControlMask005ED570) != 0) {
                g_camp_page_buttons_0069c3ec[index]->DisableSecondaryState(0);
            }
        }
        SwitchCampPage005A4540(3);
    }
    g_camp_page_buttons_0069c3ec[1]->EnableSecondaryState(0);
}

// FUNCTION: WIZ8 0x005b5b80
static void CampPageAction005B5B80(void)
{
    int index;

    if (static_cast<unsigned char>(g_camp_page_buttons_0069c3ec[2]->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        for (index = 0; index < 5; ++index) {
            if (static_cast<unsigned char>(g_camp_page_buttons_0069c3ec[index]->m_stateFlags &
                                           g_W8TextControlMask005ED570) != 0) {
                g_camp_page_buttons_0069c3ec[index]->DisableSecondaryState(0);
            }
        }
        SwitchCampPage005A4540(2);
    }
    g_camp_page_buttons_0069c3ec[2]->EnableSecondaryState(0);
}

// FUNCTION: WIZ8 0x005b5bd0
static void CampPageAction005B5BD0(void)
{
    int index;

    if (static_cast<unsigned char>(g_camp_page_buttons_0069c3ec[3]->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        for (index = 0; index < 5; ++index) {
            if (static_cast<unsigned char>(g_camp_page_buttons_0069c3ec[index]->m_stateFlags &
                                           g_W8TextControlMask005ED570) != 0) {
                g_camp_page_buttons_0069c3ec[index]->DisableSecondaryState(0);
            }
        }
        SwitchCampPage005A4540(1);
    }
    g_camp_page_buttons_0069c3ec[3]->EnableSecondaryState(0);
}

// FUNCTION: WIZ8 0x005b5c20
static void CampPageDismissAction005B5C20(void)
{
    DismissSelectedPartyCharacter();
}

/* Item-action button callbacks: while the button's secondary state is up the
   click performs the held-item action or arms the button's targeting mode;
   while it is down the click falls back to clearing the action mode. */
// FUNCTION: WIZ8 0x005b5c30
static void CampItemAction005B5C30(void)
{
    if (static_cast<unsigned char>(g_item_action_controls_69c3cc[0]->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        g_camp_entry_parameter_0069c0fc = g_value_0069c0f8;
        if (g_status_685170.item_in_cursor != 0) {
            IdentifyAndOpenItemInfo005BA370(&g_status_685170.item_in_hand_235b);
            return;
        }
        SetCampItemActionMode005B59B0(3);
        return;
    }
    SetCampItemActionMode005B59B0(0);
}

// FUNCTION: WIZ8 0x005b5c80
static void CampItemAction005B5C80(void)
{
    if (static_cast<unsigned char>(g_item_action_controls_69c3cc[1]->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        SetCampItemActionMode005B59B0(1);
        if (g_status_685170.item_in_cursor != 0) {
            SetHandCursors005BAFC0(0);
            g_camp_screen_0069c0f4->item_redraw_flags |= 0x3ffe00;
        }
        return;
    }
    SetCampItemActionMode005B59B0(0);
}

// FUNCTION: WIZ8 0x005b5cd0
static void CampItemAction005B5CD0(void)
{
    if (static_cast<unsigned char>(g_item_action_controls_69c3cc[2]->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        if (g_status_685170.item_in_cursor != 0) {
            OpenSplitStackDialog005BA400(&g_status_685170.item_in_hand_235b);
            return;
        }
        SetCampItemActionMode005B59B0(4);
        return;
    }
    SetCampItemActionMode005B59B0(0);
}

// FUNCTION: WIZ8 0x005b5d10
static void CampItemAction005B5D10(void)
{
    if (static_cast<unsigned char>(g_item_action_controls_69c3cc[3]->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        if (g_status_685170.item_in_cursor != 0) {
            UseItem005BA4F0(&g_status_685170.item_in_hand_235b);
            return;
        }
        SetCampItemActionMode005B59B0(5);
        return;
    }
    SetCampItemActionMode005B59B0(0);
}

// FUNCTION: WIZ8 0x005b5d50
static void CampItemAction005B5D50(void)
{
    if (static_cast<unsigned char>(g_item_action_controls_69c3cc[4]->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        if (g_status_685170.item_in_cursor != 0) {
            if (ResolvePendingCampCharacter005A5F30(1) != 0) {
                DropHeldItem005BA3D0();
            }
            return;
        }
        SetCampItemActionMode005B59B0(6);
        return;
    }
    SetCampItemActionMode005B59B0(0);
}

// FUNCTION: WIZ8 0x005b5d90
static void CampItemAction005B5D90(void)
{
    unsigned char eligible;
    unsigned char active;

    eligible = IsCampActionAllowed005A6090(giReviewCharSlot);
    active = static_cast<unsigned char>(g_item_action_controls_69c3cc[5]->m_stateFlags &
                                        g_W8TextControlMask005ED570);
    if (eligible == 0) {
        if (active != 0) {
            g_item_action_controls_69c3cc[5]->DisableSecondaryState(0);
            g_item_action_controls_69c3cc[5]->Invalidate(0);
        }
        return;
    }
    if (active != 0) {
        SetCampItemActionMode005B59B0(7);
        return;
    }
    SetCampItemActionMode005B59B0(0);
}

// FUNCTION: WIZ8 0x005b5df0
static void CampItemAction005B5DF0(void)
{
    if (static_cast<unsigned char>(g_item_action_controls_69c3cc[6]->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        giCasterCharSlot = giReviewCharSlot;
        if (g_status_685170.item_in_cursor != 0) {
            UseHeldItemOnItem005BA740(&g_status_685170.item_in_hand_235b);
            g_item_action_controls_69c3cc[6]->DisableSecondaryState(0);
            g_item_action_controls_69c3cc[6]->Invalidate(0);
            return;
        }
        SetCampItemActionMode005B59B0(8);
        return;
    }
    SetCampItemActionMode005B59B0(0);
}

// FUNCTION: WIZ8 0x005b5e60
static void CampItemAction005B5E60(void)
{
    if (static_cast<unsigned char>(g_item_action_controls_69c3cc[7]->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        giCasterCharSlot = giReviewCharSlot;
        SetCampItemActionMode005B59B0(9);
        return;
    }
    SetCampItemActionMode005B59B0(0);
}

// FUNCTION: WIZ8 0x005B2200
void CloseFormationPanel(void)
{
    DestroyFormationPanel();
    gXStatus.fReviewCharacterMode = 0;
    UpdateHeldItemCursor();
    RegionSetDisable(0x1b);
    RequestRedraw(0x200);
    ClearSurfaceRect(0xd6, 0x3c, 0x1ab, 0x12f);
    InvalidateRegion(0xd6, 0x3c, 0x1ab, 0x12f, 0);
    ResumeMainGameWorld();
}

// SYNTHETIC: WIZ8 0x005b1b90
// W8GrowableVector<W8CharacterPageEntry*>::`scalar deleting destructor'
