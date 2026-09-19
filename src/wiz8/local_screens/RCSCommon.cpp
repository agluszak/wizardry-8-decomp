#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_screens/RCSCommon.h"
#include "wiz8/local_screens/MGSFormation.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/RCSItemsPage.h"
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
#include "wiz8/cursor.h"
#include "wiz8/npc_interaction.h"
#include "wiz8/regions.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/xstatus.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/learned_spells.h"

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
   button; Function5B4EB0 allocates them on the shared bottom Controls panel. */
// GLOBAL: WIZ8 0x0069c3ec
W8TextControl* g_camp_page_buttons_0069c3ec[5];
// GLOBAL: WIZ8 0x0069c3c0
W8TextControl* g_level_up_button_0069c3c0;
// GLOBAL: WIZ8 0x0069c400
W8TextControl* g_dismiss_button_0069c400;

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
        g_camp_screen_0069c0f4->skill_selection = 0;
        Function5C4EE0();
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
    if (Function5A6090(slot) == 0) {
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
    g_current_screen_state.parameter_3.character = g_value_0069c0f8;
    g_pending_screen_state.parameter_3.character =
        &g_status_685170.buffers.characters[giReviewCharSlot];
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
            g_current_screen_state.parameter_3.character = g_value_0069c0f8;
            g_current_screen_state.parameter_2 = giReviewCharSlot;
            g_pending_screen_state.mode = 1;
            g_pending_screen_state.parameter_3.character = g_value_0069c0f8;
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
        g_value_006840be = static_cast<unsigned short>(giReviewCharSlot);
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
