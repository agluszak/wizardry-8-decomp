#include "wiz8/local_screens/MGSSpellCasting.h"

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "timer.h"
#include "wiz8/character_event_queue.h"
#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/dialog_code/SpellInfoDialog.h"
#include "wiz8/engine_code/Cursor3d.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/fonts.h"
#include "wiz8/layouts/character.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/layouts/learned_spells.h"
#include "wiz8/learned_spells.h"
#include "wiz8/layouts/targeting.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/PartyImport.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/NPCInteractionSubscreen.h"
#include "wiz8/local_screens/MGSButtons.h"
#include "wiz8/local_screens/MGSPortraitCombat.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_screens/OptionsScreen.h"
#include "wiz8/local_screens/RCSItemsPage.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/npc_interaction.h"
#include "wiz8/regions.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/world_cursor.h"
#include "wiz8/cursor.h"
#include "wiz8/local_screens/MGSUseItemSelect.h"
#include "wiz8/xstatus.h"

#include "himage.h"
#include "vsurface.h"

#define SPELLCASTING_CPP "C:\\Projects\\Wizardry 8\\Local Screens\\MGSSpellCasting.cpp"

/* The first realm skill id; character->skill_unlocks[first + realm] counts
   the spells known in that realm. */
enum { W8_SKILL_FIRST_REALM = 0x1c };

/* Realm-name string ids for the spell-casting realm-button tooltips. */
// GLOBAL: WIZ8 0x0064C840
int g_spell_realm_help_string_ids_0064c840[6] = {0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c};

// GLOBAL: WIZ8 0x0064C934
const wchar_t g_format_s_parens_s_colon_d_0064c934[] = L"%s (%s: %d)";

// GLOBAL: WIZ8 0x0069BF3C
W8SpellCastingView* gpSCSV;

int GetSpellCastingPowerIndex(void)
{
    if (gpSCSV == 0) {
        return -1;
    }
    return gpSCSV->iSpellPower;
}

static void CreateSpellCastingViewControls(void);
static void ReleaseSpellCastingViewControls(void);
void SelectSpellRealm005A0320(void);
void SelectSpellRealm005A0330(void);
void SelectSpellRealm005A0340(void);
void SelectSpellRealm005A0350(void);
void SelectSpellRealm005A0360(void);
void SelectSpellRealm005A0370(void);
void SelectSpellPowerLevel005A0500(void);
void SelectSpellPowerLevel005A0510(void);
void SelectSpellPowerLevel005A0520(void);
void SelectSpellPowerLevel005A0530(void);
void SelectSpellPowerLevel005A0540(void);
void SelectSpellPowerLevel005A0550(void);
void SelectSpellPowerLevel005A0560(void);
void SelectSpellPowerLevel005A0570(void);
void SelectSpellPowerLevel005A0660(void);
void SpellCastingDialogResult005A0AE0(W8DialogBase* dialog);
void PreviewSpellPowerPipHover005A0910(int power_level);
static void UpdateSpellRealmPointDisplays(void);
static void UpdateSpellPowerPips(void);
static void RefreshSpellPowerPip(int pip);
static void RebuildSpellCastingList(int spell_id);
static void SetSpellListLineColor(int index, char color);
static void SelectSpellCastingRow(int index);
static void SelectSpellCastingRealm005A0380(int realm);
static void SelectSpellCastingListRow005A1150(int index);
static void TryCommitSpellCast(void);
static void ShowSpellCastingError(int spell_id);

/* Builds the three spell-casting panels and all of their controls: the six
   realm buttons with their animated icons, the nine power-level pips, the
   spell-name label and the cancel button. */
// FUNCTION: WIZ8 0x0059E1F0
static void CreateSpellCastingViewControls(void)
{
    Controls* panel;
    unsigned int index;

    gpSCSV->panels[0] = new Controls(0x17, 0x166, 0xe5, 0x1c2, 400, 0, 0);
    gpSCSV->panels[1] = new Controls(0xe6, 0x166, 0x1a3, 0x1c2, 400, 0, 1);
    gpSCSV->panels[2] = new Controls(0x1a4, 0x166, 0x268, 0x1c2, 400, 0, 2);

    panel = gpSCSV->panels[0];
    gpSCSV->realm_buttons[0] =
        new W8TextControl(panel, 0x8b, 0x1e, 0xd, 0x69, 0x23, 0x191, 0, 0, 3, 1, 3, 2);
    gpSCSV->realm_buttons[1] =
        new W8TextControl(panel, 0x8c, 0x71, 0xd, 0xbc, 0x23, 0x191, 0, 4, 7, 5, 7, 6);
    gpSCSV->realm_buttons[2] =
        new W8TextControl(panel, 0x8d, 0x12, 0x23, 0x5d, 0x39, 0x191, 0, 0, 3, 1, 3, 2);
    gpSCSV->realm_buttons[3] =
        new W8TextControl(panel, 0x8e, 0x7d, 0x23, 200, 0x39, 0x191, 0, 4, 7, 5, 7, 6);
    gpSCSV->realm_buttons[4] =
        new W8TextControl(panel, 0x8f, 0x1e, 0x39, 0x69, 0x4f, 0x191, 0, 0, 3, 1, 3, 2);
    gpSCSV->realm_buttons[5] =
        new W8TextControl(panel, 0x90, 0x71, 0x39, 0xbc, 0x4f, 0x191, 0, 4, 7, 5, 7, 6);
    for (index = 0; index < W8_SPELL_REALM_COUNT; ++index) {
        gpSCSV->realm_buttons[index]->AddLayoutFlags(g_W8TextControlMask005ED578);
    }
    for (index = 0; index < W8_SPELL_REALM_COUNT; ++index) {
        gpSCSV->realm_buttons[index]->m_textBuffer.SetLayoutMode(g_W8TextBufferLayoutMask005ED554 |
                                                                 g_W8TextBufferLayoutMask005ED54C);
    }
    gpSCSV->realm_buttons[0]->UpdateTextBounds(0x1e, 0xd, 0x55, 0x23);
    gpSCSV->realm_buttons[1]->UpdateTextBounds(0x85, 0xd, 0xbc, 0x23);
    gpSCSV->realm_buttons[2]->UpdateTextBounds(0x12, 0x23, 0x49, 0x39);
    gpSCSV->realm_buttons[3]->UpdateTextBounds(0x91, 0x23, 200, 0x39);
    gpSCSV->realm_buttons[4]->UpdateTextBounds(0x1e, 0x39, 0x55, 0x4f);
    gpSCSV->realm_buttons[5]->UpdateTextBounds(0x85, 0x39, 0xbc, 0x4f);
    gpSCSV->realm_buttons[0]->m_primaryActivationCallback = SelectSpellRealm005A0320;
    gpSCSV->realm_buttons[1]->m_primaryActivationCallback = SelectSpellRealm005A0330;
    gpSCSV->realm_buttons[2]->m_primaryActivationCallback = SelectSpellRealm005A0340;
    gpSCSV->realm_buttons[3]->m_primaryActivationCallback = SelectSpellRealm005A0350;
    gpSCSV->realm_buttons[4]->m_primaryActivationCallback = SelectSpellRealm005A0360;
    gpSCSV->realm_buttons[5]->m_primaryActivationCallback = SelectSpellRealm005A0370;

    gpSCSV->realm_icons[0] =
        new W8TextControl(panel, -1, 0x55, 0xf, 0x67, 0x21, 0x193, 0,
                          g_spell_realm_animations_00648c90[0].initial_frame, -1, -1, -1,
                          g_spell_realm_animations_00648c90[0].frame_count);
    gpSCSV->realm_icons[1] =
        new W8TextControl(panel, -1, 0x73, 0xf, 0x85, 0x21, 0x194, 0,
                          g_spell_realm_animations_00648c90[1].initial_frame, -1, -1, -1,
                          g_spell_realm_animations_00648c90[1].frame_count);
    gpSCSV->realm_icons[2] =
        new W8TextControl(panel, -1, 0x49, 0x25, 0x5b, 0x37, 0x195, 0,
                          g_spell_realm_animations_00648c90[2].initial_frame, -1, -1, -1,
                          g_spell_realm_animations_00648c90[2].frame_count);
    gpSCSV->realm_icons[3] =
        new W8TextControl(panel, -1, 0x7f, 0x25, 0x91, 0x37, 0x196, 0,
                          g_spell_realm_animations_00648c90[3].initial_frame, -1, -1, -1,
                          g_spell_realm_animations_00648c90[3].frame_count);
    gpSCSV->realm_icons[4] =
        new W8TextControl(panel, -1, 0x55, 0x3b, 0x67, 0x4d, 0x197, 0,
                          g_spell_realm_animations_00648c90[4].initial_frame, -1, -1, -1,
                          g_spell_realm_animations_00648c90[4].frame_count);
    gpSCSV->realm_icons[5] =
        new W8TextControl(panel, -1, 0x73, 0x3b, 0x85, 0x4d, 0x198, 0,
                          g_spell_realm_animations_00648c90[5].initial_frame, -1, -1, -1,
                          g_spell_realm_animations_00648c90[5].frame_count);

    panel = gpSCSV->panels[2];
    gpSCSV->power_pips[0] =
        new W8TextControl(panel, 0x91, 0x38, 2, 0x47, 0x11, 0x192, 0, 0, 2, 1, 4, 3);
    gpSCSV->power_pips[1] =
        new W8TextControl(panel, 0x92, 0x48, 4, 0x59, 0x15, 0x192, 0, 6, 8, 7, 10, 9);
    gpSCSV->power_pips[2] =
        new W8TextControl(panel, 0x93, 0x56, 0x10, 0x69, 0x23, 0x192, 0, 0xc, 0xe, 0xd, 0x10, 0xf);
    gpSCSV->power_pips[3] = new W8TextControl(panel, 0x94, 0x5b, 0x23, 0x70, 0x38, 0x192, 0, 0x12,
                                              0x14, 0x13, 0x16, 0x15);
    gpSCSV->power_pips[4] = new W8TextControl(panel, 0x95, 0x52, 0x38, 0x69, 0x4f, 0x192, 0, 0x18,
                                              0x1a, 0x19, 0x1c, 0x1b);
    gpSCSV->power_pips[5] = new W8TextControl(panel, 0x96, 0x3a, 0x41, 0x53, 0x5a, 0x192, 0, 0x1e,
                                              0x20, 0x1f, 0x22, 0x21);
    gpSCSV->power_pips[6] = new W8TextControl(panel, 0x97, 0x1f, 0x3a, 0x3a, 0x55, 0x192, 0, 0x24,
                                              0x26, 0x25, 0x28, 0x27);
    gpSCSV->power_pips[7] = new W8TextControl(panel, 0x98, 0x10, 0x1f, 0x2d, 0x3c, 0x192, 0, 0x30,
                                              0x32, 0x31, 0x34, 0x33);
    gpSCSV->power_pips[8] = new W8TextControl(panel, 0x99, 0x10, 0x1f, 0x2d, 0x3c, 0x192, 0, 0x2a,
                                              0x2a, 0x2b, 0x2b, 0x2d);
    gpSCSV->power_pips[0]->SetFlaggedRegionBounds(0x3f, 9, 7);
    gpSCSV->power_pips[1]->SetFlaggedRegionBounds(0x50, 0xc, 8);
    gpSCSV->power_pips[2]->SetFlaggedRegionBounds(0x5f, 0x19, 9);
    gpSCSV->power_pips[3]->SetFlaggedRegionBounds(0x65, 0x2d, 10);
    gpSCSV->power_pips[4]->SetFlaggedRegionBounds(0x5d, 0x43, 0xb);
    gpSCSV->power_pips[5]->SetFlaggedRegionBounds(0x46, 0x4d, 0xc);
    gpSCSV->power_pips[6]->SetFlaggedRegionBounds(0x2c, 0x47, 0xd);
    gpSCSV->power_pips[7]->SetFlaggedRegionBounds(0x1e, 0x2d, 0xe);
    gpSCSV->power_pips[8]->SetFlaggedRegionBounds(0x1e, 0x2d, 0xe);
    for (index = 0; index < 9; ++index) {
        gpSCSV->power_pips[index]->AddLayoutFlags(g_W8TextControlMask005ED578);
    }
    gpSCSV->power_pips[0]->m_primaryActivationCallback = SelectSpellPowerLevel005A0500;
    gpSCSV->power_pips[1]->m_primaryActivationCallback = SelectSpellPowerLevel005A0510;
    gpSCSV->power_pips[2]->m_primaryActivationCallback = SelectSpellPowerLevel005A0520;
    gpSCSV->power_pips[3]->m_primaryActivationCallback = SelectSpellPowerLevel005A0530;
    gpSCSV->power_pips[4]->m_primaryActivationCallback = SelectSpellPowerLevel005A0540;
    gpSCSV->power_pips[5]->m_primaryActivationCallback = SelectSpellPowerLevel005A0550;
    gpSCSV->power_pips[6]->m_primaryActivationCallback = SelectSpellPowerLevel005A0560;
    gpSCSV->power_pips[7]->m_primaryActivationCallback = SelectSpellPowerLevel005A0570;
    gpSCSV->power_pips[8]->m_primaryActivationCallback = SelectSpellPowerLevel005A0660;

    gpSCSV->spell_name =
        new W8TextControl(panel, 0x9a, 0xe, 10, 0x30, 0x18, -1, -1, -1, -1, -1, -1, -1);
    gpSCSV->spell_name->m_textBuffer.SetLayoutMode(g_W8TextBufferLayoutMask005ED554 |
                                                   g_W8TextBufferLayoutMask005ED54C);
    gpSCSV->cancel_button =
        new W8TextControl(panel, 0x9b, 0x96, 6, 0xb1, 0x21, 0x8e, 0, 4, -1, 5, 6, 7);
    gpSCSV->cancel_button->m_primaryActivationCallback = ResetSpellCastingSelection005A0B90;

    gpSCSV->panels[0]->SetEnabled(true);
    gpSCSV->panels[1]->SetEnabled(true);
    gpSCSV->panels[2]->SetEnabled(true);

    if (gXStatus.fCampMode != 0) {
        for (index = 7; index < 0xf; ++index) {
            RegionSetDisable(index);
            DisableRegionSetInput(index);
            DisableRegionInput(index + 0x53);
        }
    }
}

/* Releases the three panels and every text control the view owns. */
// FUNCTION: WIZ8 0x0059F060
static void ReleaseSpellCastingViewControls(void)
{
    int index;

    for (index = 0; index < 3; ++index) {
        if (gpSCSV->panels[index] != 0) {
            delete gpSCSV->panels[index];
        }
    }
    for (index = 0; index < W8_SPELL_REALM_COUNT; ++index) {
        if (gpSCSV->realm_buttons[index] != 0) {
            delete gpSCSV->realm_buttons[index];
        }
        if (gpSCSV->realm_icons[index] != 0) {
            delete gpSCSV->realm_icons[index];
        }
    }
    for (index = 0; index < 9; ++index) {
        if (gpSCSV->power_pips[index] != 0) {
            delete gpSCSV->power_pips[index];
        }
    }
    if (gpSCSV->spell_name != 0) {
        delete gpSCSV->spell_name;
    }
    if (gpSCSV->cancel_button != 0) {
        delete gpSCSV->cancel_button;
    }
}

/* Opens the spell-casting view for one party slot: allocates and clears the
   view state, swaps the game mode, builds the panels and redirects the region
   sets the view takes over. */
// FUNCTION: WIZ8 0x0059F0E0
unsigned char OpenSpellCastingView(int party_slot)
{
    W8MainUiMode mode;

    if (IsPartySlotEligible00524A10(party_slot) == 0) {
        return 0;
    }
    if (gpSCSV == 0) {
        gpSCSV = static_cast<W8SpellCastingView*>(malloc(sizeof(W8SpellCastingView)));
        if (gpSCSV == 0) {
            return 0;
        }
        memset(gpSCSV, 0, sizeof(W8SpellCastingView));
    }
    UpdateScreenOverlays(0);
    gXStatus.fSpellCastMode = 1;
    if (g_level_block->combat_end_notification != -1) {
        DestroySubMenuControls();
    }
    gpSCSV->input_blocked_570 = 0;
    gpSCSV->field_574 = 1;
    gpSCSV->location_id = -1;
    gpSCSV->interact_id = -1;
    CloseMainGameOverlays();
    mode = g_settings_6850c8.main_ui_mode;
    if (mode == W8_MAIN_UI_MODE_RADAR) {
        ApplyMainGameModeFlag(W8_MAIN_UI_MODE_FORMATION, 0);
    } else {
        SetViewportMode(GetMainGameViewportMode());
    }
    gpSCSV->saved_game_mode = mode;
    CreateSpellCastingViewControls();
    RegionSetEnable(0x14);
    EnableRegionInput(0x52);
    EnableRegionInput(0x53);
    EnableRegionInput(0x54);
    EnableRegionInput(0x55);
    g_level_block->action_panel_visible = 1;
    DisableRegionInput(0x59);
    DisableRegionInput(0x56);
    DisableRegionInput(0x57);
    DisableRegionInput(0x58);
    RegionSetEnable(0x19);
    SelectTextBox(2);
    ResetEditorStatusLine0058AA20(-1);
    g_level_block->text_box_visible_271 = 0;
    SetTextBoxRegionBounds(0xea, 0x16e, 0x18c, 0x1ba);
    gpSCSV->iSpellRealm = -1;
    SelectSpellCastingPartySlot(party_slot);
    gpSCSV->selected_spell_index = -1;
    SelectSpellCastingCharacter(party_slot);
    RequestRedraw(0x200);
    RequestRedraw(0x100);
    RequestRedraw(0x1000);
    PauseMainGameWorld();
    gpSCSV->override_spell_104 = 0;
    return 1;
}

/* Tears the spell-casting view down: restores the regions and game mode,
   frees the view state, and resumes the world. A pending interact id picks
   its NPC interaction back up. */
// FUNCTION: WIZ8 0x0059F2B0
void CloseSpellCastingView(void)
{
    int location_id;
    int interact_id;
    unsigned int monster_index;
    W8NpcState* npc;

    if (gpSCSV->closing == 0) {
        RegionSetDisable(0x19);
        DisableRegionInput(0x52);
        DisableRegionInput(0x53);
        DisableRegionInput(0x54);
        DisableRegionInput(0x55);
        g_level_block->action_panel_visible = 0;
        SetTargetingMode(0);
        if (gXStatus.fCampMode == 0) {
            ResetEditorStatusLine0058AA20(-1);
        }
        g_level_block->text_box_visible_271 = 1;
        SelectTextBox(gXStatus.fCombatMode != 0);
        ReleaseSpellCastingViewControls();
        SetTextBoxRegionBounds(0xa8, 0x16e, 0x1c4, 0x1ba);
        gXStatus.fSpellCastMode = 0;
        ApplyMainGameModeFlag(gpSCSV->saved_game_mode, 1);
        RequestRedraw(0x200);
        RequestRedraw(0x100);
        RequestRedraw(0x1000);
        location_id = gpSCSV->location_id;
        interact_id = gpSCSV->interact_id;
        free(gpSCSV);
        gpSCSV = 0;
        ResumeMainGameWorld();
        if (IsWorldCursorVisible() != 0) {
            ToggleWorldCursor();
        }
        if (gXStatus.fLockInteract != 0 && IsScreenTransitionPending() == 0) {
            OpenLockInteraction00587510(0);
        }
        if (gXStatus.fTrapInteract != 0 && IsScreenTransitionPending() == 0) {
            OpenTrapInteraction0058A470(0);
        }
        if (interact_id != -1 && IsScreenTransitionPending() == 0) {
            monster_index = MonsterGetIndexByLocationID(0x1cf, SPELLCASTING_CPP, location_id, 1);
            npc = FindNpcBindingForMonster(monster_index);
            BeginNpcDialogue(npc, 0, -1, 0, 1);
            SwitchNpcDialogueLayout(interact_id);
        }
    }
}

/* Re-enables the region set and inputs the spell-casting view owns. */
// FUNCTION: WIZ8 0x0059F440
void RestoreSpellCastingRegions(void)
{
    RegionSetEnable(0x14);
    EnableRegionInput(0x52);
    EnableRegionInput(0x53);
    EnableRegionInput(0x54);
    EnableRegionInput(0x55);
    g_level_block->action_panel_visible = 1;
    DisableRegionInput(0x59);
    DisableRegionInput(0x56);
    DisableRegionInput(0x57);
    DisableRegionInput(0x58);
}

/* Points the view at a new caster: rebuilds the learned-spell buckets and the
   realm/power displays, then either restores the pending cast's realm and
   list or clears a realm the new caster has no spell points in. */
// FUNCTION: WIZ8 0x0059F490
void SelectSpellCastingCharacter(int party_slot)
{
    int realm;

    if (CharacterHasCastableSpell(&g_status_685170.buffers.Char[party_slot]) == 0 ||
        IsPartySlotEligible00524A10(party_slot) == 0) {
        ResetEditorStatusLine0058AA20(-1);
        CloseSpellCastingView();
        return;
    }
    gpSCSV->caster = &g_status_685170.buffers.Char[party_slot];
    BuildLearnedSpellState004F9600(&gpSCSV->learned, gpSCSV->caster);
    UpdateSpellRealmPointDisplays();
    gpSCSV->uiSpellToCast = 0;
    gpSCSV->iSpellPowerClass = -1;
    gpSCSV->uiSpellIndex = -1;
    SelectSpellCastingRow(-1);
    gpSCSV->uiPowerLevels = 0;
    SelectSpellPowerLevel005A06F0(-1);
    gpSCSV->dialog_confirmed = 0;
    UpdateSpellPowerPips();
    SelectSpellCastingPartySlot(g_status_685170.selected_character);
    RequestRedraw(0x200);
    if (GetAffordableSpellPowerLevel(party_slot) != 0) {
        int spell_id = g_status_685170.buffers.XChar[party_slot].spell_id;
        SelectSpellCastingRealm005A0380(g_spell_records[spell_id].realm);
        if (spell_id == 0x17) {
            gpSCSV->uiSpellIndex = -1;
            return;
        }
        RebuildSpellCastingList(spell_id);
        return;
    }
    realm = gpSCSV->iSpellRealm;
    if (realm == -1) {
        return;
    }
    if (gpSCSV->caster->sp_max[realm] == 0) {
        gpSCSV->realm_icons[realm]->m_normalSprite =
            g_spell_realm_animations_00648c90[realm].initial_frame;
        gpSCSV->realm_icons[gpSCSV->iSpellRealm]->Invalidate(0);
        gpSCSV->iSpellRealm = -1;
        ResetEditorStatusLine0058AA20(-1);
        return;
    }
    RebuildSpellCastingList(0);
}

/* Writes each realm's "current/max" spell-point caption and enables the realm
   button and icon for the pools the caster actually has. */
// FUNCTION: WIZ8 0x0059F660
static void UpdateSpellRealmPointDisplays(void)
{
    int realm;

    for (realm = 0; realm < W8_SPELL_REALM_COUNT; ++realm) {
        bool has_realm = gpSCSV->caster->sp_max[realm] != 0;
        gpSCSV->realm_buttons[realm]->SetEnabled(has_realm);
        gpSCSV->realm_icons[realm]->SetEnabled(has_realm);
        gpSCSV->realm_buttons[realm]->m_textBuffer.SetText(
            FormatWideString(g_format_d_slash_d_00614b58,
                             GetCharacterRealmSpellPoints(gpSCSV->caster, realm),
                             gpSCSV->caster->sp_max[realm]),
            g_font_683660);
    }
}

/* Enables and frames the power-level pips for the chosen spell's power class:
   class zero shows one pip per affordable level, class one keeps the
   seventh pip live out of combat and class two shows the max pip alone. */
// FUNCTION: WIZ8 0x0059F710
static void UpdateSpellPowerPips(void)
{
    unsigned int pip;
    unsigned int failure;

    if (gpSCSV->uiPowerLevels == 0 || gpSCSV->iSpellPowerClass == 3) {
        for (pip = 0; pip < 9; ++pip) {
            gpSCSV->power_pips[pip]->SetEnabled(0);
        }
    } else {
        RefreshSpellPowerPip(7);
        RefreshSpellPowerPip(8);
        if (gpSCSV->iSpellPowerClass == 0) {
            for (pip = 0; pip < gpSCSV->uiPowerLevels; ++pip) {
                RefreshSpellPowerPip(pip);
            }
            if (gpSCSV->uiPowerLevels < 7) {
                for (pip = gpSCSV->uiPowerLevels; pip < 7; ++pip) {
                    gpSCSV->power_pips[pip]->SetEnabled(0);
                }
            }
            gpSCSV->power_pips[7]->SetActive(0);
            gpSCSV->power_pips[8]->SetActive(0);
        } else if (gpSCSV->iSpellPowerClass == 1) {
            gpSCSV->power_pips[7]->SetActive(gXStatus.fCombatMode == 0);
            for (pip = 0; pip < gpSCSV->uiPowerLevels; ++pip) {
                RefreshSpellPowerPip(pip);
            }
            if (gpSCSV->uiPowerLevels < 7) {
                for (pip = gpSCSV->uiPowerLevels; pip < 7; ++pip) {
                    gpSCSV->power_pips[pip]->SetEnabled(0);
                }
            }
            gpSCSV->power_pips[8]->SetActive(0);
        } else if (gpSCSV->iSpellPowerClass == 2) {
            for (pip = 0; pip < 7; ++pip) {
                gpSCSV->power_pips[pip]->SetEnabled(0);
            }
            gpSCSV->power_pips[8]->SetActive(1);
            gpSCSV->power_pips[7]->SetActive(0);
        }
    }
    if (gpSCSV->iSpellPowerClass == 2) {
        if (gpSCSV->uiPowerLevels != 0) {
            for (pip = 0; pip < gpSCSV->uiPowerLevels; ++pip) {
                if (gpSCSV->uiSpellToCast == 0) {
                    srAssertFail("gpSCSV->uiSpellToCast != SPELL_NONE", SPELLCASTING_CPP, 0x338, 0);
                }
                gpSCSV->power_pips[pip]->SetEnabled(0);
                failure =
                    GetSpellFailureChanceForCast(gpSCSV->caster, gpSCSV->uiSpellToCast, pip + 1);
                gpSCSV->power_pips[pip]->m_disabledSprite = (pip + failure * 9) * 6;
            }
        }
        if (gpSCSV->uiPowerLevels < 7) {
            for (pip = gpSCSV->uiPowerLevels; pip < 7; ++pip) {
                gpSCSV->power_pips[pip]->m_disabledSprite = pip * 6 + 3;
            }
            return;
        }
    } else {
        for (pip = 0; pip < 7; ++pip) {
            gpSCSV->power_pips[pip]->m_disabledSprite = pip * 6 + 3;
        }
    }
}

/* Recomputes one pip's four sprite frames from the spell's failure chance at
   the level the pip stands for; pips seven and eight price the max cast. */
// FUNCTION: WIZ8 0x0059F9D0
static void RefreshSpellPowerPip(int pip)
{
    unsigned int failure;
    int level;
    int frame;
    W8TextControl* control;

    if (gpSCSV->uiSpellToCast == 0) {
        srAssertFail("gpSCSV->uiSpellToCast != SPELL_NONE", SPELLCASTING_CPP, 0x303, 0);
    }
    if (pip == 7 || pip == 8) {
        level = 8;
    } else {
        level = pip + 1;
    }
    gpSCSV->power_pips[pip]->SetEnabled(1);
    failure = GetSpellFailureChanceForCast(gpSCSV->caster, gpSCSV->uiSpellToCast, level);
    frame = (pip + failure * 9) * 6;
    control = gpSCSV->power_pips[pip];
    control->m_normalSprite = frame;
    if (pip == 8) {
        gpSCSV->power_pips[8]->m_alternateNormalSprite = frame;
    } else {
        control->m_alternateNormalSprite = frame + 1;
    }
    control->m_pressedSprite = frame + 2;
    if (pip == 8) {
        gpSCSV->power_pips[8]->m_alternatePressedSprite = frame + 2;
    } else {
        control->m_alternatePressedSprite = frame + 4;
    }
}

/* Rebuilds the realm's spell list: pass zero appends every castable spell,
   pass one appends the unaffordable and blocked ones with their own colors.
   A nonzero spell id scrolls the list to and selects that spell's row. */
// FUNCTION: WIZ8 0x0059FAD0
static void RebuildSpellCastingList(int spell_id)
{
    wchar_t line[120];
    int index;
    int realm;
    int selected;
    unsigned char pass;
    W8SpellRuntimeRecord* spell;

    selected = -1;
    ResetEditorStatusLine0058AA20(-1);
    gpSCSV->uiSpellIndex = -1;
    gpSCSV->selected_spell_index = -1;
    gpSCSV->uiSpellsInList = 0;
    realm = gpSCSV->iSpellRealm;
    for (index = 0; index < 0x15e; ++index) {
        gpSCSV->uiSpells[index] = 0;
        gpSCSV->alt_colors[index] = 0xff;
    }
    if (gpSCSV->caster->skill_unlocks[W8_SKILL_FIRST_REALM + realm] != 0) {
        pass = 0;
        do {
            index = 0;
            while (index <
                   static_cast<int>(gpSCSV->caster->skill_unlocks[W8_SKILL_FIRST_REALM + realm])) {
                int id = gpSCSV->learned.spell_ids_by_realm[realm][index];
                gpSCSV->override_spell_104 = id;
                spell = &g_spell_records[id];
                if (spell->spell_point_cost <= gpSCSV->caster->iSPLeft[realm] &&
                    SpellUsableNow(id, 0) != 0 &&
                    SpellHasAnyValidTarget(CharacterPointerToPartySlot(gpSCSV->caster), id, 0) !=
                        0 &&
                    !IsTeleportCastMissingAnchor00501D00(gpSCSV->caster, id)) {
                    if (IsSpellBlockedForCharacter(gpSCSV->caster, id) != 0) {
                        if (pass == 1 &&
                            ((gXStatus.fCampMode == 0 && gXStatus.fLockInteract == 0 &&
                              gXStatus.fTrapInteract == 0) ||
                             SpellUsableNow(id, 0) != 0) &&
                            gpSCSV->uiSpellsInList + 1 <= 0x15e) {
                            ++gpSCSV->uiSpellsInList;
                            gpSCSV->uiSpells[gpSCSV->uiSpellsInList - 1] = id;
                            gpSCSV->alt_colors[gpSCSV->uiSpellsInList - 1] = 4;
                            swprintf(
                                line, g_format_s_space_s_00617584,
                                g_spell_target_parentheticals_60d4e0[GetSpellTargetType(id, 0)],
                                spell->display_name);
                            ShowNotice(0xf, line, 2, -1, 0);
                            AppendTextBoxLine0058B300(
                                FormatWideString(g_format_d_0060aa20, spell->spell_point_cost), 2);
                            SetSpellListLineColor(gpSCSV->uiSpellsInList - 1, 4);
                        }
                    } else if (pass == 0) {
                        if (gpSCSV->uiSpellsInList + 1 <= 0x15e) {
                            ++gpSCSV->uiSpellsInList;
                            gpSCSV->uiSpells[gpSCSV->uiSpellsInList - 1] = id;
                            gpSCSV->alt_colors[gpSCSV->uiSpellsInList - 1] = 0xf;
                            swprintf(
                                line, g_format_s_space_s_00617584,
                                g_spell_target_parentheticals_60d4e0[GetSpellTargetType(id, 0)],
                                spell->display_name);
                            ShowNotice(0xf, line, 2, -1, 0);
                            AppendTextBoxLine0058B300(
                                FormatWideString(g_format_d_0060aa20, spell->spell_point_cost), 2);
                        }
                        if (id == spell_id) {
                            selected = gpSCSV->uiSpellsInList - 1;
                        }
                    }
                } else if (pass == 1 &&
                           ((gXStatus.fCampMode == 0 && gXStatus.fLockInteract == 0 &&
                             gXStatus.fTrapInteract == 0) ||
                            SpellUsableNow(id, 0) != 0) &&
                           gpSCSV->uiSpellsInList + 1 <= 0x15e) {
                    ++gpSCSV->uiSpellsInList;
                    gpSCSV->uiSpells[gpSCSV->uiSpellsInList - 1] = id;
                    gpSCSV->alt_colors[gpSCSV->uiSpellsInList - 1] = 0;
                    swprintf(line, g_format_s_space_s_00617584,
                             g_spell_target_parentheticals_60d4e0[GetSpellTargetType(id, 0)],
                             spell->display_name);
                    ShowNotice(0xf, line, 2, -1, 0);
                    AppendTextBoxLine0058B300(
                        FormatWideString(g_format_d_0060aa20, spell->spell_point_cost), 2);
                    SetSpellListLineColor(gpSCSV->uiSpellsInList - 1, 0);
                }
                ++index;
            }
            gpSCSV->override_spell_104 = 0;
            ++pass;
        } while (pass < 2);
    }
    if (spell_id == 0 || selected == -1) {
        ScrollTextBoxTo(0);
    } else {
        SelectSpellCastingListRow005A1150(selected);
        ScrollTextBoxTo(selected);
    }
    RequestRedraw(0x800);
}

/* Recolours one spell-list line; -1 keeps the row's own color, or the
   selected-row color when the row is the selected spell. */
// FUNCTION: WIZ8 0x0059FFA0
static void SetSpellListLineColor(int index, char color)
{
    W8MessageStorageRecord* line;
    size_t length;

    if (gpSCSV->uiSpellsInList < static_cast<unsigned int>(index)) {
        srAssertFail("uiSpellIndex <= gpSCSV->uiSpellsInList", SPELLCASTING_CPP, 0x3df, 0);
    }
    if (color == -1) {
        if (gpSCSV->uiSpellIndex == index) {
            color = 3;
        } else {
            color = gpSCSV->alt_colors[index];
            if (color == -1) {
                srAssertFail("bAltColor != -1", SPELLCASTING_CPP, 0x3f1, 0);
            }
        }
    }
    line = &g_message_storage_68f2d8[2][index];
    line->highlight_start = 2;
    length = wcslen(line->wString);
    line->highlight_color = color;
    line->highlight_stop = static_cast<unsigned char>(length);
}

/* Marks one spell-list row as selected, restoring the previous row's own
   color first; -1 just clears the selection. */
// FUNCTION: WIZ8 0x005A0040
static void SelectSpellCastingRow(int index)
{
    int previous;

    if (index == -1) {
        if (gpSCSV->selected_spell_index != -1) {
            SetSpellListLineColor(gpSCSV->selected_spell_index, -1);
            gpSCSV->selected_spell_index = -1;
            RequestRedraw(0x800);
            return;
        }
    } else {
        if (index < 0) {
            srAssertFail("iSpellIndex >= 0", SPELLCASTING_CPP, 0x406, 0);
        }
        if (gpSCSV->uiSpellsInList < static_cast<unsigned int>(index)) {
            // c-style-cast-ok: the assertion text itself spells (INT32)
            srAssertFail("iSpellIndex <= (INT32) gpSCSV->uiSpellsInList", SPELLCASTING_CPP, 0x407,
                         0);
        }
        previous = gpSCSV->selected_spell_index;
        if (index != previous && previous != -1) {
            SetSpellListLineColor(previous, -1);
        }
        SetSpellListLineColor(index, 5);
        gpSCSV->selected_spell_index = index;
    }
    RequestRedraw(0x800);
}

/* Opens the spell-casting view when needed and selects the given spell's
   realm and list row. The interact and location ids ride along so a pending
   interaction can resume after the cast. */
// FUNCTION: WIZ8 0x005A0110
void BeginSpellCast005A0110(int spell_id, int location_id, int interact_id)
{
    int realm;

    if (gXStatus.fNpcDialogueMode == 0) {
        if (gXStatus.fLockInteractMode != 0) {
            EndLockInteractMode(0);
        }
    } else {
        EndNpcDialogueSession0056E800(0);
    }
    if (gXStatus.fSpellCastMode == 0) {
        OpenSpellCastingView(g_status_685170.selected_character);
    }
    gpSCSV->interact_id = interact_id;
    gpSCSV->location_id = location_id;
    if (CanCharacterCastSpell(&g_status_685170.buffers.Char[g_status_685170.selected_character],
                              spell_id) == 0) {
        return;
    }
    realm = -1;
    switch (g_spell_records[spell_id].realm) {
    case W8_SPELL_REALM_FIRE:
        SelectSpellCastingRealm005A0380(W8_SPELL_REALM_FIRE);
        RebuildSpellCastingList(spell_id);
        return;
    case W8_SPELL_REALM_WATER:
        SelectSpellCastingRealm005A0380(W8_SPELL_REALM_WATER);
        RebuildSpellCastingList(spell_id);
        return;
    case W8_SPELL_REALM_AIR:
        SelectSpellCastingRealm005A0380(W8_SPELL_REALM_AIR);
        RebuildSpellCastingList(spell_id);
        return;
    case W8_SPELL_REALM_EARTH:
        SelectSpellCastingRealm005A0380(W8_SPELL_REALM_EARTH);
        RebuildSpellCastingList(spell_id);
        return;
    case W8_SPELL_REALM_MENTAL:
        SelectSpellCastingRealm005A0380(W8_SPELL_REALM_MENTAL);
        RebuildSpellCastingList(spell_id);
        return;
    case W8_SPELL_REALM_DIVINE:
        realm = W8_SPELL_REALM_DIVINE;
        break;
    default:
        break;
    }
    SelectSpellCastingRealm005A0380(realm);
    RebuildSpellCastingList(spell_id);
}

/* Redraws every live spell-casting panel; the middle panel's pending text
   update is flushed through the message storage. */
// FUNCTION: WIZ8 0x005A0270
void SetSpellCastingPanelsActive005A0270(unsigned char active)
{
    int index;
    bool text_pending;

    text_pending = false;
    for (index = 0; index < 3; ++index) {
        Controls* panel = gpSCSV->panels[index];
        if (panel->m_fEnabled) {
            if (active != 0) {
                panel->Invalidate(0);
            }
            if (index == 1) {
                Controls* description = gpSCSV->panels[1];
                text_pending = description->m_fEnabled &&
                               (description->m_fDirty || description->m_fLayoutDirty);
            }
            panel->Redraw();
            if (text_pending) {
                RedrawTextBoxScrollChrome();
            }
        }
    }
}

// FUNCTION: WIZ8 0x005A02F0
void SpellCastingNoticeClosed005A02F0(W8DialogBase* dialog)
{
    RequestRedraw(0x200);
}

// FUNCTION: WIZ8 0x005A0300
void InvalidateSpellCastingDescription005A0300(void)
{
    gpSCSV->panels[1]->Invalidate(0);
}

// FUNCTION: WIZ8 0x005A0320
void SelectSpellRealm005A0320(void)
{
    SelectSpellCastingRealm005A0380(0);
}

// FUNCTION: WIZ8 0x005A0330
void SelectSpellRealm005A0330(void)
{
    SelectSpellCastingRealm005A0380(1);
}

// FUNCTION: WIZ8 0x005A0340
void SelectSpellRealm005A0340(void)
{
    SelectSpellCastingRealm005A0380(2);
}

// FUNCTION: WIZ8 0x005A0350
void SelectSpellRealm005A0350(void)
{
    SelectSpellCastingRealm005A0380(3);
}

// FUNCTION: WIZ8 0x005A0360
void SelectSpellRealm005A0360(void)
{
    SelectSpellCastingRealm005A0380(4);
}

// FUNCTION: WIZ8 0x005A0370
void SelectSpellRealm005A0370(void)
{
    SelectSpellCastingRealm005A0380(5);
}

/* Switches the view to another realm: the old realm's icon returns to its
   first frame and the new one starts its frame timer, then the whole
   selection state resets. */
// FUNCTION: WIZ8 0x005A0380
static void SelectSpellCastingRealm005A0380(int realm)
{
    int previous;

    previous = gpSCSV->iSpellRealm;
    if (previous != realm) {
        if (previous != -1) {
            gpSCSV->realm_icons[previous]->m_normalSprite =
                g_spell_realm_animations_00648c90[previous].initial_frame;
            gpSCSV->realm_icons[gpSCSV->iSpellRealm]->Invalidate(0);
            gpSCSV->realm_buttons[gpSCSV->iSpellRealm]->DisableSecondaryState(0);
            gpSCSV->realm_buttons[gpSCSV->iSpellRealm]->Invalidate(0);
        }
        gpSCSV->iSpellRealm = realm;
        gpSCSV->realm_anim_frame =
            g_spell_realm_animations_00648c90[gpSCSV->iSpellRealm].initial_frame;
        gpSCSV->realm_icons[gpSCSV->iSpellRealm]->Invalidate(0);
        gpSCSV->realm_anim_timer = SetCountdownClock(0x32);
        RebuildSpellCastingList(0);
        gpSCSV->uiSpellToCast = 0;
        gpSCSV->iSpellPowerClass = -1;
        gpSCSV->uiSpellIndex = -1;
        SelectSpellCastingRow(-1);
        gpSCSV->uiPowerLevels = 0;
        SelectSpellPowerLevel005A06F0(-1);
        gpSCSV->dialog_confirmed = 0;
        UpdateSpellPowerPips();
        SelectSpellCastingPartySlot(g_status_685170.selected_character);
        RequestRedraw(0x200);
    }
    if (realm != -1) {
        gpSCSV->realm_buttons[realm]->EnableSecondaryState(0);
        gpSCSV->realm_buttons[realm]->Invalidate(0);
    }
}

// FUNCTION: WIZ8 0x005A0500
void SelectSpellPowerLevel005A0500(void)
{
    SelectSpellPowerLevel005A06F0(0);
}

// FUNCTION: WIZ8 0x005A0510
void SelectSpellPowerLevel005A0510(void)
{
    SelectSpellPowerLevel005A06F0(1);
}

// FUNCTION: WIZ8 0x005A0520
void SelectSpellPowerLevel005A0520(void)
{
    SelectSpellPowerLevel005A06F0(2);
}

// FUNCTION: WIZ8 0x005A0530
void SelectSpellPowerLevel005A0530(void)
{
    SelectSpellPowerLevel005A06F0(3);
}

// FUNCTION: WIZ8 0x005A0540
void SelectSpellPowerLevel005A0540(void)
{
    SelectSpellPowerLevel005A06F0(4);
}

// FUNCTION: WIZ8 0x005A0550
void SelectSpellPowerLevel005A0550(void)
{
    SelectSpellPowerLevel005A06F0(5);
}

// FUNCTION: WIZ8 0x005A0560
void SelectSpellPowerLevel005A0560(void)
{
    SelectSpellPowerLevel005A06F0(6);
}

/* The seventh power pip: while its secondary state is lit it clears the lower
   pips and prices the name label with the max cast, otherwise it empties the
   label. */
// FUNCTION: WIZ8 0x005A0570
void SelectSpellPowerLevel005A0570(void)
{
    int pip;

    if (static_cast<unsigned char>(gpSCSV->power_pips[7]->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        for (pip = 0; pip < 7; ++pip) {
            W8TextControl* control = gpSCSV->power_pips[pip];
            if (control->m_enabled) {
                control->DisableSecondaryState(0);
            }
        }
        gpSCSV->spell_name->m_textBuffer.SetRenderMode(4);
        gpSCSV->spell_name->m_textBuffer.SetText(L"?", g_font_683660);
        gpSCSV->iSpellPower = 7;
        gpSCSV->spell_name->Invalidate(1);
        return;
    }
    gpSCSV->spell_name->m_textBuffer.SetRenderMode(4);
    gpSCSV->spell_name->m_textBuffer.SetText(&g_wchar_00689b34, g_font_683660);
    gpSCSV->iSpellPower = -1;
    gpSCSV->spell_name->Invalidate(1);
}

/* The max-power pip: while its secondary state is lit it prices the name
   label with the max cast, otherwise it relights and repaints the label. */
// FUNCTION: WIZ8 0x005A0660
void SelectSpellPowerLevel005A0660(void)
{
    if (static_cast<unsigned char>(gpSCSV->power_pips[8]->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        gpSCSV->spell_name->m_textBuffer.SetRenderMode(4);
        gpSCSV->spell_name->m_textBuffer.SetText(L"?", g_font_683660);
        gpSCSV->iSpellPower = 7;
        gpSCSV->spell_name->Invalidate(1);
        return;
    }
    gpSCSV->power_pips[8]->EnableSecondaryState(0);
    gpSCSV->spell_name->Invalidate(1);
}

/* Records the chosen power level: pips through the level light, pips above it
   go dark, and the name label shows the cast's total cost. */
// FUNCTION: WIZ8 0x005A06F0
void SelectSpellPowerLevel005A06F0(int power_level)
{
    int pip;
    const wchar_t* text;

    gpSCSV->panels[2]->Invalidate(0);
    if (power_level == -1 ||
        (static_cast<unsigned char>(gpSCSV->power_pips[power_level]->m_stateFlags &
                                    g_W8TextControlMask005ED570) == 0 &&
         gpSCSV->iSpellPower == power_level)) {
        for (pip = 0; pip < 7; ++pip) {
            gpSCSV->power_pips[pip]->DisableSecondaryState(0);
        }
        gpSCSV->iSpellPower = -1;
        if (gpSCSV->iSpellPowerClass == 2) {
            gpSCSV->power_pips[8]->EnableSecondaryState(0);
            if (static_cast<unsigned char>(gpSCSV->power_pips[8]->m_stateFlags &
                                           g_W8TextControlMask005ED570) == 0) {
                gpSCSV->power_pips[8]->EnableSecondaryState(0);
            } else {
                gpSCSV->spell_name->m_textBuffer.SetRenderMode(4);
                gpSCSV->spell_name->m_textBuffer.SetText(L"?", g_font_683660);
                gpSCSV->iSpellPower = 7;
            }
            gpSCSV->spell_name->Invalidate(1);
            return;
        }
        gpSCSV->spell_name->m_textBuffer.SetRenderMode(4);
        if (gpSCSV->iSpellPowerClass == 3) {
            text = FormatWideString(g_format_d_0060aa20,
                                    g_spell_records[gpSCSV->uiSpellToCast].spell_point_cost);
        } else {
            text = &g_wchar_00689b34;
        }
    } else {
        gpSCSV->iSpellPower = power_level;
        if (power_level >= 0) {
            for (pip = 0; pip <= power_level; ++pip) {
                gpSCSV->power_pips[pip]->EnableSecondaryState(0);
            }
        }
        if (power_level + 1 < 8) {
            for (pip = power_level + 1; pip < 8; ++pip) {
                gpSCSV->power_pips[pip]->DisableSecondaryState(0);
            }
        }
        gpSCSV->spell_name->m_textBuffer.SetRenderMode(4);
        text = FormatWideString(g_format_d_0060aa20,
                                (power_level + 1) *
                                    g_spell_records[gpSCSV->uiSpellToCast].spell_point_cost);
    }
    gpSCSV->spell_name->m_textBuffer.SetText(text, g_font_683660);
}

/* Hover preview for a power pip: clears the spell-name plate and shows the
   cast cost for the hovered level (or restores the selected cost / empty
   label when the pointer leaves). Only power classes 0 and 1 participate. */
// FUNCTION: WIZ8 0x005A0910
void PreviewSpellPowerPipHover005A0910(int power_level)
{
    W8TextControl* spell_name;
    Controls* panel;
    const wchar_t* text;

    if (power_level < 7) {
        if ((gpSCSV->iSpellPowerClass == 0 || gpSCSV->iSpellPowerClass == 1) &&
            (power_level == -1 || (gpSCSV->power_pips[power_level]->m_active != 0 &&
                                   gpSCSV->power_pips[power_level]->m_enabled != 0))) {
            spell_name = gpSCSV->spell_name;
            panel = gpSCSV->panels[2];
            ColorFillVideoSurfaceArea(
                -0xe, spell_name->m_left + panel->origin_x, spell_name->m_top + panel->origin_y,
                spell_name->m_right + panel->origin_x, spell_name->m_bottom + panel->origin_y,
                Get16BPPColor(0x10101));
            spell_name->Invalidate(0);
            if (power_level != -1) {
                spell_name->m_textBuffer.SetRenderMode(6);
                text = FormatWideString(
                    g_format_d_0060aa20,
                    (power_level + 1) * g_spell_records[gpSCSV->uiSpellToCast].spell_point_cost);
                spell_name->m_textBuffer.SetText(text, g_font_683660);
                return;
            }
            if (gpSCSV->iSpellPowerClass == 0) {
                spell_name->m_textBuffer.SetRenderMode(4);
                if (gpSCSV->iSpellPower != -1) {
                    text = FormatWideString(
                        g_format_d_0060aa20,
                        (gpSCSV->iSpellPower + 1) *
                            g_spell_records[gpSCSV->uiSpellToCast].spell_point_cost);
                    spell_name->m_textBuffer.SetText(text, g_font_683660);
                    return;
                }
                spell_name->m_textBuffer.SetText(&g_wchar_00689b34, g_font_683660);
            }
        }
    }
}

/* The confirmation dialog's destroy callback: a cancelled dialog unwinds the
   whole spell selection. */
// FUNCTION: WIZ8 0x005A0AE0
void SpellCastingDialogResult005A0AE0(W8DialogBase* dialog)
{
    int index;

    index = gpSCSV->uiSpellIndex;
    if (GetDialogResult(dialog) != 0) {
        gpSCSV->dialog_confirmed = 1;
        return;
    }
    gpSCSV->uiSpellToCast = 0;
    gpSCSV->iSpellPowerClass = -1;
    gpSCSV->uiSpellIndex = -1;
    SelectSpellCastingRow(-1);
    gpSCSV->uiPowerLevels = 0;
    SelectSpellPowerLevel005A06F0(-1);
    gpSCSV->dialog_confirmed = 0;
    UpdateSpellPowerPips();
    SelectSpellCastingPartySlot(g_status_685170.selected_character);
    RequestRedraw(0x200);
    SetSpellListLineColor(index, -1);
}

// FUNCTION: WIZ8 0x005A0B90
void ResetSpellCastingSelection005A0B90(void)
{
    if (gXStatus.fCampMode != 0) {
        ResetEditorStatusLine0058AA20(-1);
    }
    CloseSpellCastingView();
    ClearSlotTargeting0053B050(g_status_685170.selected_character);
}

/* Per-frame spell-casting pump: advances the selected realm's icon animation
   on its own clock, then lets a ready cast commit. */
// FUNCTION: WIZ8 0x005A0BC0
void CommitSpellCastingSelection005A0BC0(void)
{
    if (gpSCSV->iSpellRealm != -1) {
        if (ClockIsTicking(gpSCSV->realm_anim_timer) == 0) {
            ++gpSCSV->realm_anim_frame;
            if (gpSCSV->realm_anim_frame ==
                g_spell_realm_animations_00648c90[gpSCSV->iSpellRealm].frame_count) {
                gpSCSV->realm_anim_frame = 0;
            }
            gpSCSV->realm_icons[gpSCSV->iSpellRealm]->m_normalSprite = gpSCSV->realm_anim_frame;
            gpSCSV->realm_icons[gpSCSV->iSpellRealm]->Invalidate(0);
            gpSCSV->realm_anim_timer = SetCountdownClock(0x32);
            RequestRedraw(0x80000000);
        }
    }
    TryCommitSpellCast();
}

/* Realm button/icon region callback (catalog ids 0..5): presses the matching
   realm button, invalidates its icon, and on enter sets the realm tooltip
   (plain name when disabled, or name with skill level when enabled). */
// FUNCTION: WIZ8 0x005A0C80
unsigned char SpellRealmButtonRegionEvent(const InputAtom* event, W8Region* region)
{
    int us_event = event->usEvent;

    if (us_event <= LEFT_BUTTON_REPEAT) {
        if (us_event == LEFT_BUTTON_REPEAT || us_event == LEFT_BUTTON_DOWN) {
            gpSCSV->realm_buttons[region->callback_id]->OnLeftButtonDown(0);
            gpSCSV->realm_icons[region->callback_id]->Invalidate(0);
            region->flags |= W8_REGION_LEFT_BUTTON_HELD;
            return 1;
        }
        if (us_event == LEFT_BUTTON_UP) {
            gpSCSV->realm_buttons[region->callback_id]->OnLeftButtonUp(0);
            gpSCSV->realm_icons[region->callback_id]->Invalidate(0);
            if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
                region->flags &= ~W8_REGION_LEFT_BUTTON_HELD;
            }
            return 1;
        }
    } else if (us_event == MOUSE_POS) {
        if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
            gpSCSV->realm_buttons[region->callback_id]->OnMouseLeave(0);
            gpSCSV->realm_icons[region->callback_id]->Invalidate(0);
            return 1;
        }
        if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
            unsigned int realm = region->callback_id;
            gpSCSV->realm_buttons[realm]->OnMouseEnter(0);
            if (gpSCSV->realm_buttons[realm]->m_enabled == 0) {
                SetRegionHelpText(gppStringList[g_spell_realm_help_string_ids_0064c840[realm]]);
            } else {
                SetRegionHelpText(FormatWideString(
                    g_format_s_parens_s_colon_d_0064c934,
                    gppStringList[g_spell_realm_help_string_ids_0064c840[realm]],
                    gppStringList[0xb4 / 4],
                    g_status_685170.buffers.Char[g_status_685170.selected_character]
                        .skills[W8_SKILL_FIRST_REALM + realm]
                        .level));
            }
            gpSCSV->realm_icons[realm]->Invalidate(0);
            return 1;
        }
    }
    return 0;
}

/* Power-pip / cancel-button region callback. callback_id indexes the contiguous
   control pointers from power_pips[0] (ids 0..8 and cancel at 10). While
   input_blocked_570 is set the handler swallows input. */
// FUNCTION: WIZ8 0x005A0E50
unsigned char SpellPowerPipRegionEvent(const InputAtom* event, W8Region* region)
{
    int us_event;
    unsigned int callback_id;

    if (gpSCSV->input_blocked_570 != 0) {
        return 1;
    }

    us_event = event->usEvent;
    if (us_event <= LEFT_BUTTON_REPEAT) {
        if (us_event == LEFT_BUTTON_REPEAT || us_event == LEFT_BUTTON_DOWN) {
            (&gpSCSV->power_pips[0])[region->callback_id]->OnLeftButtonDown(0);
            region->flags |= W8_REGION_LEFT_BUTTON_HELD;
            return 1;
        }
        if (us_event == LEFT_BUTTON_UP) {
            (&gpSCSV->power_pips[0])[region->callback_id]->OnLeftButtonUp(0);
            callback_id = region->callback_id;
            if (callback_id < 7 && ((&gpSCSV->power_pips[0])[callback_id]->m_stateFlags &
                                    g_W8TextControlMask005ED570) == 0) {
                PreviewSpellPowerPipHover005A0910(callback_id);
            }
            if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
                region->flags &= ~W8_REGION_LEFT_BUTTON_HELD;
            }
            return 1;
        }
    } else if (us_event == MOUSE_POS) {
        if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
            PreviewSpellPowerPipHover005A0910(-1);
            (&gpSCSV->power_pips[0])[region->callback_id]->OnMouseLeave(0);
            return 1;
        }
        if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
            PreviewSpellPowerPipHover005A0910(region->callback_id);
            (&gpSCSV->power_pips[0])[region->callback_id]->OnMouseEnter(0);
            return 1;
        }
    }
    return 0;
}

/* Spell-list text-box body region event: the hovered row is recomputed from
   the cursor position for every event and drives SelectSpellCastingRow when it
   changes. Left release casts the hovered spell; right release opens the
   spell-info dialog with the target cursor saved for restore on close; leaving
   the box clears the hover row. The held bits are armed on button down but are
   never cleared on release. */
// FUNCTION: WIZ8 0x005A0F70
unsigned char SpellCastTextBoxRegionEvent(const InputAtom* event, W8Region* region)
{
    W8SpellInfoDialog* dialog;
    int line;

    if (g_status_685170.selected_character == -1) {
        return 0;
    }
    if (gpSCSV->iSpellRealm == -1) {
        return 0;
    }
    line = g_level_block->text_lines[g_status_685170.text_line_cursor_1795] +
           (GetAtomCursorY004285A0(event) - region->y1) / 0xb;
    if (line >= static_cast<int>(gpSCSV->uiSpellsInList) || line < -1) {
        line = -1;
    }
    if (line != gpSCSV->selected_spell_index) {
        SelectSpellCastingRow(line);
    }
    switch (event->usEvent) {
    case LEFT_BUTTON_UP:
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) == 0) {
            return 1;
        }
        if (line == -1) {
            return 1;
        }
        SelectSpellCastingListRow005A1150(line);
        return 1;
    case LEFT_BUTTON_DOWN:
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        return 1;
    case RIGHT_BUTTON_DOWN:
        region->flags |= W8_REGION_RIGHT_BUTTON_HELD;
        return 1;
    case MOUSE_POS:
        if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
            SelectSpellCastingRow(-1);
        }
        return 0;
    default:
        return 0;
    case RIGHT_BUTTON_UP:
        if ((region->flags & W8_REGION_RIGHT_BUTTON_HELD) == 0) {
            return 1;
        }
        if (line == -1) {
            return 1;
        }
        g_saved_target_cursor_0069bf30 = gXStatus.iCurrentCursor;
        dialog = new W8SpellInfoDialog(gpSCSV->uiSpells[line]);
        dialog->SetText(&g_wchar_00689b34);
        dialog->m_destroy_callback = RestoreTargetCursor59D930;
        OpenModal(dialog);
        return 1;
    }
}

// FUNCTION: WIZ8 0x005A1140
unsigned char IgnoreSpellCastingInput(const InputAtom* event, W8Region*)
{
    return 0;
}

/* The spell-list row click: re-colors the rows, remembers the spell, then
   either prices its power levels or reports why it cannot be cast. */
// FUNCTION: WIZ8 0x005A1150
static void SelectSpellCastingListRow005A1150(int index)
{
    char color;
    int cost;
    int power_class;
    int previous;
    int spell_id;
    int target_type;
    unsigned int levels;
    unsigned char needed;

    previous = gpSCSV->uiSpellIndex;
    if (index == -1) {
        return;
    }
    gpSCSV->uiSpellIndex = index;
    if (previous != -1) {
        SetSpellListLineColor(previous, -1);
    }
    spell_id = gpSCSV->uiSpells[gpSCSV->uiSpellIndex];
    if (spell_id == 0) {
        return;
    }
    color = gpSCSV->alt_colors[index];
    if (color == 0 || color == 4) {
        gpSCSV->uiSpellIndex = -1;
        ConfigureSpellTargetFilter(-1, 0);
        ShowSpellCastingError(spell_id);
        QueueCharacterEvent(&g_status_685170.buffers.Char[g_status_685170.selected_character],
                            g_character_event_kind_005ee65c, 0,
                            g_character_event_flags_mask_005ed8e4 | g_effect_argument_005ed8c8,
                            g_effect_argument_005ed914);
        return;
    }
    SetSpellListLineColor(gpSCSV->uiSpellIndex, 3);
    power_class = g_spell_records[spell_id].field_12b;
    if (power_class == -1) {
        srAssertFail("iSpellPowerClass != BAD_INDEX", SPELLCASTING_CPP, 0x834, 0);
    }
    cost = g_spell_records[spell_id].spell_point_cost;
    levels = GetCharacterRealmSpellPoints(
                 &g_status_685170.buffers.Char[g_status_685170.selected_character],
                 g_spell_records[spell_id].realm) /
             cost;
    ClampUnsignedInteger(&levels, 0, 7);
    if (levels == 0) {
        gpSCSV->uiSpellIndex = -1;
        return;
    }
    gpSCSV->iSpellPowerClass = power_class;
    gpSCSV->uiPowerLevels = levels;
    gpSCSV->uiSpellToCast = spell_id;
    needed = GetTargetNeededForSpellFriendly(spell_id, 0, W8_TARGETING_CONTEXT_CURRENT);
    target_type = GetSpellTargetType(spell_id, 0);
    ConfigureSpellTargetFilter(target_type, needed);
    SelectSpellPowerLevel005A06F0(-1);
    UpdateSpellPowerPips();
    RequestRedraw(0x200);
}

// FUNCTION: WIZ8 0x005A1330
void SetSpellCastingMode005A1330(W8MainUiMode value)
{
    gpSCSV->saved_game_mode = value;
}

// FUNCTION: WIZ8 0x005A1350
int GetSpellCastingSelection005A1350(void)
{
    int result;

    result = gpSCSV->override_spell_104;
    if (result == 0) {
        result = gpSCSV->uiSpellToCast;
    }
    return result;
}

/* Lets a fully priced spell commit once its target is valid; spells 0x49 and
   0x4b first raise a confirmation notice unless one was already answered. */
// FUNCTION: WIZ8 0x005A1370
static void TryCommitSpellCast(void)
{
    bool ready;

    ready = false;
    if (gpSCSV->uiSpellToCast != 0 &&
        (gpSCSV->iSpellPower != -1 || gpSCSV->iSpellPowerClass == 3)) {
        ready = IsSpellTargetOfNeededKind(g_status_685170.selected_character,
                                          gpSCSV->uiSpellToCast) != 0;
    }
    if (gpSCSV->uiSpellToCast == 0x4b && gpSCSV->dialog_confirmed == 0) {
        if (IsModalOpen() == 0 &&
            g_status_685170.buffers.Char[g_status_685170.selected_character].has_saved_location) {
            ShowMainGameNoticeLine(gppStringList[0x7a4], SpellCastingDialogResult005A0AE0, 1, 1);
        }
    } else if (gpSCSV->uiSpellToCast == 0x49 && gpSCSV->dialog_confirmed == 0 &&
               IsModalOpen() == 0) {
        ShowMainGameNoticeLine(gppStringList[0x7a5], SpellCastingDialogResult005A0AE0, 1, 1);
    }
    if (ready && IsModalOpen() == 0) {
        gpSCSV->closing = 1;
        CommitSelectedSpellTarget();
        gpSCSV->closing = 0;
        if (gpSCSV->uiSpellToCast != 0x17) {
            if (gXStatus.fCampMode != 0) {
                ResetEditorStatusLine0058AA20(-1);
            }
            SetCharacterSpell(gpSCSV->caster, gpSCSV->uiSpellToCast, gpSCSV->iSpellPower + 1);
            CloseSpellCastingView();
            return;
        }
        OpenCharacterScreenForPartySlot(CharacterPointerToPartySlot(gpSCSV->caster), 1);
    }
}

/* Shows the notice line naming why a spell cannot be cast, then clears the
   detail spell. */
// FUNCTION: WIZ8 0x005A14D0
static void ShowSpellCastingError(int spell_id)
{
    gpSCSV->override_spell_104 = spell_id;
    if (SpellUsableNow(spell_id, 0) == 0) {
        ShowMainGameNoticeLine(gppStringList[0x79e], SpellCastingNoticeClosed005A02F0, 1, 0);
    } else if (GetCharacterRealmSpellPoints(gpSCSV->caster, gpSCSV->iSpellRealm) <
               g_spell_records[spell_id].spell_point_cost) {
        ShowMainGameNoticeLine(gppStringList[0x7a0], SpellCastingNoticeClosed005A02F0, 1, 0);
    } else if (SpellHasAnyValidTarget(CharacterPointerToPartySlot(gpSCSV->caster), spell_id, 0) ==
               0) {
        ShowMainGameNoticeLine(gppStringList[0x7a1], SpellCastingNoticeClosed005A02F0, 1, 0);
    } else if (IsTeleportCastMissingAnchor00501D00(gpSCSV->caster, spell_id)) {
        ShowMainGameNoticeLine(gppStringList[0x7a2], SpellCastingNoticeClosed005A02F0, 1, 0);
    } else if (IsSpellBlockedForCharacter(gpSCSV->caster, spell_id) != 0) {
        ShowMainGameNoticeLine(gppStringList[0x79f], SpellCastingNoticeClosed005A02F0, 1, 0);
    }
    gpSCSV->override_spell_104 = 0;
}
