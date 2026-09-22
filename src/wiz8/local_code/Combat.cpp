#include "wiz8/local_screens/Screens.h"
#include "wiz8/layouts/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/Search.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/GroupAttacks.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/engine_code/SpellVisual.h"
#include "wiz8/engine_code/Missile.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/notices.h"
#include "wiz8/xstatus.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/CombatPartyMovement.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/npc_interaction.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/local_code/Strings.h"
#include "random.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/local_code/FormationAndFacing.h"
#include "wiz8/local_code/LoadSaveGame.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/MonsterAI.h"
#include "wiz8/engine_code/Cursor3d.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/local_screens/MGSPortraits.h"
#include "wiz8/local_screens/MGSUseItemSelect.h"
#include "wiz8/local_screens/MGSRadarMap.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_screens/MGSPartyMovement.h"
#include "wiz8/local_screens/MGSSpellCasting.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/Gameloop.h"
#include "timer.h"
#include "soundman.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/float_constants.h"
#include "wiz8/dice.h"
#include "wiz8/music_playlist.h"
#include "wiz8/wiz8_windows.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/local_code/GameplayTime.h"
#include "wiz8/local_code/Sight.h"
#include "wiz8/engine_code/Camera.h"
#include "wiz8/engine_code/OctPath.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_screens/MGSButtons.h"
#include "wiz8/local_code/CombatHostility.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/local_code/Noise.h"
#include "wiz8/local_screens/NPCInteractionSubscreen.h"
#include "wiz8/local_screens/OptionsScreen.h"
#include "wiz8/string_database.h"
#include "wiz8/utility.h"
#include "wiz8/character_event_queue.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"
#include "wiz8/cursor.h"

#include <stdarg.h>
#include <stdio.h>
#include "wiz8/layouts/game_status.h"

// GLOBAL: WIZ8 0x006836a8
W8CombatState* g_combat_state;
/* 0x0068506C: a friendly NPC's combat-entry script notice was queued by
   StartCombat; while it is set the next call checks the dialogue and script
   deferral gates before clearing it and proceeding. */
/* 0x006850B4: the once-per-combat difficulty evaluation result - 0 easy,
   1 normal, 2 hard; picks the combat music and gates the victory event. */

/*
 * Local Code\Combat.cpp.
 *
 * The round bookkeeping: who is engaged, what each slot has chosen to do, and
 * the running list of characters who died this round.
 */

/* 0x00524A10 */

/* 0x00547940 */
/* The per-character combat rows live at +0x18 of the combat state and run
   0xd4 bytes apart; the state's leading 0x18 bytes are its own header. */
// GLOBAL: WIZ8 0x0068d810
unsigned char g_combat_log_enabled_0068d810;
// GLOBAL: WIZ8 0x00617664
extern const wchar_t g_combat_log_format_00617664[] = L"%hs";
// GLOBAL: WIZ8 0x006175B0
extern const wchar_t g_format_s_dash_dash_006175b0[] = L"%s -- ";
// GLOBAL: WIZ8 0x006175C0
extern const wchar_t g_format_s_bang_006175c0[] = L"%s!";
// GLOBAL: WIZ8 0x0061EC8C
int g_breath_notice_id_0061ec8c = 0x65b;
/* 0x0053AC30 */

/* 0x004E7590 */
void ResetPartyCombatRows004E7590(void);

/* Enter combat mode: queue a friendly NPC's combat-entry script notice when
   one is still owed, refuse while dialogue or a script event defers it, then
   reset the world, UI and per-monster state and allocate the combat state. */
// FUNCTION: WIZ8 0x004E7090
unsigned char StartCombat(int surprise)
{
    W8MonsterInfo* monster_info;
    W8MonsterInfo* nearest_info;
    W8CombatSlot chosen;
    W8NpcState* npc;
    float nearest_distance;
    unsigned int index;

    if (AnyCharacterActive() == 0) {
        return 0;
    }
    if (static_cast<char>(GetLevelDataFlag4()) == 0) {
        return 0;
    }
    if (g_status_685170.value_2435 != 0) {
        ClearMainGameTargetState();
    }
    if (gXStatus.npc_combat_notice_pending == 0) {
        for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
            monster_info = MonsterGetScriptPartByLocationIndex(index);
            if (monster_info->fInCombat != 0 && monster_info->ubDisposition == DISP_HOSTILE &&
                monster_info->hp_current > 0 && monster_info->highest_condition < 0x10) {
                npc = GetNpcStateForMonsterInfo(monster_info, 0);
                if (npc != 0 && npc->record->unknown_2ef[0] != 0) {
                    QueueNpcScriptNotice(npc, 0, -1, 0, 0);
                    gXStatus.npc_combat_notice_pending = 1;
                    BeginScriptedWorldAction();
                    break;
                }
            }
        }
    }
    if (gXStatus.npc_combat_notice_pending != 0) {
        if (gXStatus.fNpcDialogueMode != 0 || ShouldDeferCharacterEventForNpcScript(0)) {
            return 0;
        }
        gXStatus.npc_combat_notice_pending = 0;
        ClearMainGameTargetState();
    }
    if (g_status_685170.value_498b == 1) {
        g_status_685170.value_498b = 2;
    }
    UpdateHeldItemCursor();
    if (gXStatus.fSurprisePossible != 0) {
        ResolveSurpriseHold00502810();
    }
    if (GetViewDistance() != g_sight_default_005ec254) {
        ResetSight();
    }
    UpdateScreenOverlays(0);
    AutoSaveIfAllowed(1);
    ResetLevelDataVectors0041F0D0();
    SetEnvironmentTimeEnabled00482990(0);
    MonsterForward453160();
    SetFlag6081E4(0);
    if (gXStatus.fNpcDialogueMode == 0 && gXStatus.fSpellCastMode == 0 &&
        gXStatus.fItemSelectMode == 0) {
        SelectTextBox(1);
    }
    ResetEditorStatusLine0058AA20(1);
    g_combat_state = static_cast<W8CombatState*>(malloc(sizeof(W8CombatState)));
    if (g_combat_state == 0) {
        return 0;
    }
    memset(static_cast<void*>(g_combat_state), 0, sizeof(W8CombatState));
    g_combat_state->value_004 = 0;
    g_combat_state->combat_result_00c = 0;
    g_combat_state->value_010 = 0;
    g_combat_state->value_014 = 0;
    g_combat_state->flag_a54 = gXStatus.hostile_monster_count > 0;
    g_combat_state->flag_000 = 0;
    g_combat_state->flag_001 = 1;
    g_combat_state->action_clock_7ac = GetClock();
    g_combat_state->eCombatActionStatus = 0;
    g_combat_state->iActionChar = -1;
    g_combat_state->pActionMonsterInfo = 0;
    g_combat_state->hit_sound_active_7c0 = 0;
    g_combat_state->engaged_missile = 0;
    g_combat_state->uiNextPartyAction = 0;
    g_combat_state->uiCurrentPartyAction = 0;
    g_combat_state->unengaged_rounds_a56 = 0;
    g_combat_state->combat_update_count = 0;
    g_combat_state->notice_scroll_pending_a57 = 0;
    g_combat_state->flag_a62 = 0;
    gXStatus.fCombatMode = 1;
    gXStatus.fPartyMovementUi = 0;
    gXStatus.fPartyMovementMode = 0;
    DisablePortraitControls0059BB40();
    EnableMainRegionSet();
    g_level_block->pick_changed_154 = 0;
    RequestRedraw(0x810ff);
    CheckMonsterGroupsEnterCombat();
    ShowNotice(0xc, gppStringList[0x224], 1, 0xffffffff, 0);
    RollCombatSurprise004ECF50(static_cast<char>(surprise));

    nearest_distance = 999999.0f;
    nearest_info = 0;
    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        monster_info = MonsterGetScriptPartByLocationIndex(index);
        if (monster_info->fActive != 0 && monster_info->fInCombat != 0 &&
            monster_info->hp_current > 0 && monster_info->ubDisposition == DISP_HOSTILE) {
            float distance = monster_info->monster->GetDistanceToPlayer004C7CB0();
            if (distance < nearest_distance) {
                nearest_distance = distance;
                nearest_info = monster_info;
            }
        }
    }
    if (nearest_info != 0) {
        PointCameraAtMonster(nearest_info, 0, 1);
    }
    RefreshAllSight();
    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        monster_info = MonsterGetScriptPartByLocationIndex(index);
        if (monster_info->fInCombat != 0) {
            monster_info->action_kind = -1;
            monster_info->action_detail = 0;
            monster_info->pCombat->phase = 0;
            monster_info->pCombat->active = 1;
            monster_info->pCombat->value_14c = 0;
            RequestRedraw(0x100000);
            if (monster_info->hp_current > 0 && monster_info->highest_condition < 0xe &&
                monster_info->condition_turns[0xc] == 0) {
                MonsterChooseTarget(monster_info, &chosen, 3);
                if (chosen.iType == 2) {
                    MonsterForwardReferencePosition(monster_info->monster, 0);
                } else if (chosen.iType == 3) {
                    MonsterAimAtMonster004C62C0(monster_info->monster,
                                                GetMonsterByLocationID(chosen.iMonsterID), 0);
                }
            }
            monster_info->flag_253 = 0;
        }
    }
    ResetPartyCombatRows004E7590();
    DetectMonsterGroups004E4AB0();
    RequestRedrawParty();
    memset(gXStatus.unknown_049, 0xff, sizeof(gXStatus.unknown_049));
    CopyPartyFormationState(&gXStatus.edited_formation, &g_status_685170.formation);
    SaveCombatFormation();
    g_combat_state->unknown_a61 = g_status_685170.search_mode;
    if (g_status_685170.search_mode != 0) {
        ToggleSearchMode();
    }
    if (GetLevelDataFlag8() != 0) {
        ClearLevelDataFlag8();
    }
    if (g_settings_6850c8.continuous_combat != 0 && g_combat_state->party_surprised_a52 == 0) {
        g_combat_state->combat_ui_timer_7a8 = SetCountdownClock(g_settings_6850c8.field_019);
    }
    SoundPlay("Data\\Sound\\Misc\\Ready_Weapons.wav", 0);
    return 1;
}

/* Reset every occupied party slot's combat bookkeeping on combat entry: clear
   the pending and attack-mode words, empty both combat target slots, re-commit
   the chosen action, and mark the combat row when the character cannot act. */
// FUNCTION: WIZ8 0x004E7590
void ResetPartyCombatRows004E7590(void)
{
    for (unsigned int slot = 0; slot < 8; ++slot) {
        if (g_status_685170.buffers.XChar[slot].fOccupied == 0) {
            continue;
        }
        g_status_685170.buffers.XChar[slot].pending_action = -1;
        g_status_685170.buffers.XChar[slot].attack_mode[0] = -1;
        g_status_685170.buffers.XChar[slot].attack_mode[1] = -1;
        ResetCombatSlot(&g_status_685170.buffers.XChar[slot].target_out_of_combat);
        if (GetCurrentTargetingContext(slot) != W8_TARGETING_CONTEXT_SHARED) {
            ClearTargetHighlights(slot, &g_status_685170.buffers.XChar[slot].target_in_combat);
        }
        ResetCombatSlot(&g_status_685170.buffers.XChar[slot].target_in_combat);
        RequestPartySlotRedraw(slot);
        if (slot == static_cast<unsigned int>(g_status_685170.selected_character)) {
            RequestRedrawParty();
        }
        SetCharacterCombatAction(slot, g_status_685170.buffers.XChar[slot].action_kind,
                                 g_status_685170.buffers.XChar[slot].action_detail, 0, 0);
        g_combat_state->characters[slot].combat_status_8c = -1;
        g_combat_state->characters[slot].portrait_image_084 = -1;
        g_combat_state->characters[slot].portrait_image_alternate_088 = -1;
        g_combat_state->characters[slot].flag_34 =
            g_status_685170.buffers.Char[slot].hp_current == 0 ||
            g_status_685170.buffers.Char[slot].highest_condition >= 0xf;
        g_status_685170.buffers.Char[slot].conditions_1817[0].value_08 = 0;
        g_status_685170.buffers.Char[slot].conditions_1817[0].value_00 = 0;
        g_status_685170.buffers.Char[slot].conditions_1817[0].value_04 = 0;
    }
}

/* Whether anybody in the party is engaged with something. */
// FUNCTION: WIZ8 0x004e7ca0
bool AnyCharacterEngaged(void)
{
    unsigned int party_slot;

    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (IsPartySlotEligible00524A10(party_slot)) {
            return 1;
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x004ED460
bool CombatMayAdvanceContinuously(void)
{
    if (gXStatus.fCombatMode != 0) {
        for (int slot = 0; slot < 2; ++slot) {
            W8PartySlotRow* row = &g_status_685170.buffers.XChar[slot];
            W8Character* character = &g_status_685170.buffers.Char[slot];
            if (row->fOccupied && character->hp_current != 0 &&
                character->highest_condition < 0xf &&
                g_combat_state->npc_combat_script_pending[slot]) {
                return false;
            }
        }
    }
    if (AnyMonsterEngaged()) {
        return true;
    }
    for (int slot = 0; slot < 8; ++slot) {
        W8PartySlotRow* row = &g_status_685170.buffers.XChar[slot];
        W8Character* character = &g_status_685170.buffers.Char[slot];
        if (row->fOccupied && character->hp_current != 0 &&
            CharacterCanSwitchTo(slot, W8_TARGETING_CONTEXT_IN_COMBAT, 0, 0) &&
            CharacterActionTargetsEnemies(character, row->action_03d, row->action_detail_041,
                                          &row->action_detail_045)) {
            return true;
        }
    }
    return false;
}

// FUNCTION: WIZ8 0x004ED710
bool QueueNpcCombatScript(void)
{
    if (gXStatus.fCombatMode == 0) {
        return false;
    }
    for (int slot = 0; slot < 2; ++slot) {
        W8PartySlotRow* row = &g_status_685170.buffers.XChar[slot];
        W8Character* character = &g_status_685170.buffers.Char[slot];
        if (!row->fOccupied || character->hp_current == 0 || character->highest_condition >= 0xb ||
            g_combat_state->npc_combat_script_pending[slot]) {
            continue;
        }
        W8NpcState* npc = GetNpcState(row->animation_0fa);
        if (npc == 0) {
            continue;
        }
        int faction = npc->record->faction_5f;
        bool hostile_group_present = false;
        if (faction != 0 && faction != 1) {
            for (unsigned int index = 0; index < PLLength(gXStatus.plsMonsterGroupList); ++index) {
                W8MonsterGroup* group = GetMonsterGroupByListIndex(index);
                if (group->flag_28 && group->fInCombat && group->ubDisposition == DISP_HOSTILE &&
                    MonsterGroupGetRecord(group)->faction_id_25f == faction) {
                    hostile_group_present = true;
                    break;
                }
            }
        }
        if (!hostile_group_present && npc->name_style == 0x38) {
            for (unsigned int index = 0; index < PLLength(gXStatus.plsMonsterGroupList); ++index) {
                W8MonsterGroup* group = GetMonsterGroupByListIndex(index);
                if (group->flag_28 && group->fInCombat && group->ubDisposition == DISP_HOSTILE &&
                    MonsterGroupGetRecord(group)->faction_id_25f == 4) {
                    hostile_group_present = true;
                    break;
                }
            }
        }
        if (hostile_group_present && CanPlaceNpcNearParty(slot)) {
            BeginScriptedWorldAction();
            ShowString(FormatWideString(g_format_s_space_s_00617584, npc->record->source_name_004,
                                        gppStringList[0x272]));
            QueueNpcMessageLine(W8_NPC_MSG_PARTY_SLOT_EVENT_18, slot);
            QueueNpcMessageLine(W8_NPC_MSG_CLEAR_NPC_COMBAT, slot);
            g_combat_state->npc_combat_script_pending[slot] = true;
            return true;
        }
    }
    return false;
}

// FUNCTION: WIZ8 0x004ED550
bool CombatHasContinuingEffects(void)
{
    unsigned int enchantment;
    unsigned int slot;
    unsigned int index;

    for (enchantment = 0; enchantment < 8; ++enchantment) {
        for (slot = 0; slot < 8; ++slot) {
            if (g_status_685170.buffers.XChar[slot].fOccupied &&
                g_status_685170.buffers.Char[slot].enchantments[enchantment].value_08 > 0) {
                return true;
            }
        }
        for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
            W8MonsterInfo* monster = MonsterGetScriptPartByLocationIndex(index);
            if (monster->enchantments[enchantment].value_08 > 0) {
                return true;
            }
        }
    }
    for (slot = 0; slot < 9; ++slot) {
        if (g_combat_state->effect_slots[slot].active) {
            return true;
        }
        for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
            W8MonsterInfo* monster = MonsterGetScriptPartByLocationIndex(index);
            if (monster->fActive && monster->fInCombat &&
                monster->pCombat->effect_slots_3e[slot].active) {
                return true;
            }
        }
    }
    for (slot = 0; slot < 6; ++slot) {
        if (g_combat_state->effect_slots_85a[slot].active) {
            return true;
        }
        for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
            W8MonsterInfo* monster = MonsterGetScriptPartByLocationIndex(index);
            if (monster->fActive && monster->fInCombat &&
                monster->pCombat->effect_slots_d7[slot].active) {
                return true;
            }
        }
    }
    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        W8MonsterInfo* monster = MonsterGetScriptPartByLocationIndex(index);
        if (monster->fActive && monster->hp_current != 0 && monster->condition_turns[0x12] == 0 &&
            monster->summoned_2da != 0) {
            return true;
        }
    }
    return false;
}

// FUNCTION: WIZ8 0x004EA1F0
void ApplyCombatEndEffects(void)
{
    int slot;
    for (slot = 0; slot < 2; ++slot) {
        W8PartySlotRow* row = &g_status_685170.buffers.XChar[slot];
        if (row->fOccupied) {
            W8NpcState* npc = GetNpcState(row->animation_0fa);
            if (npc != 0) {
                W8Character* character = &g_status_685170.buffers.Char[slot];
                if (character->highest_condition == 0x12) {
                    if (npc->unknown_ef[1]) {
                        ApplyItemEffectToRandomCharacter(g_effect_005ee5bc, -1, 0,
                                                         g_effect_argument_005ed8c8);
                    } else if (npc->unknown_ef[2]) {
                        ApplyItemEffectToRandomCharacter(g_effect_005ee5d8, -1, 0,
                                                         g_effect_argument_005ed8c8);
                    }
                }
            }
        }
    }
    for (slot = 0; slot < 8; ++slot) {
        W8PartySlotRow* row = &g_status_685170.buffers.XChar[slot];
        W8Character* character = &g_status_685170.buffers.Char[slot];
        if (row->fOccupied && character->uiCondition[0x12] == 0 &&
            character->uiCondition[0x13] != 0 &&
            gXStatus.monster_manager_entries[slot].condition_19_latch == 0 &&
            (!g_status_685170.flag_2487 ||
             g_status_685170.pending_condition_party_slot_248f != slot)) {
            ApplyItemEffectToRandomCharacter(g_effect_005ee628, slot, 0,
                                             g_effect_argument_005ed8c8);
        }
    }
}

// FUNCTION: WIZ8 0x004E8370
void BeginCombatExecution004E8370(void)
{
    int slot;
    unsigned int index;
    if ((g_combat_state->value_004 == 0 && g_combat_state->flag_a54 != 0) ||
        g_combat_state->uiCurrentPartyActionStatus == 3) {
        AlertWorldNoise004F1100();
    }
    if (g_level_block->combat_end_notification != -1) {
        if (g_settings_6850c8.continuous_combat == 0) {
            DestroySubMenuControls();
        } else {
            ReopenSubMenuPanel();
        }
    }
    if (g_settings_6850c8.continuous_combat == 0 && gXStatus.iTargetingMode != 0) {
        SetTargetingMode(0);
    }
    g_combat_state->flag_000 = 1;
    g_combat_state->pending_death_count = 0;
    g_combat_state->flag_a55 = 1;
    g_level_block->pick_changed_154 = false;
    g_combat_state->uiCurrentPartyAction = g_combat_state->uiNextPartyAction;
    g_combat_state->uiCurrentPartyActionStatus = 0;
    UpdatePartyMovementControl();
    if (g_combat_state->uiNextPartyAction != 0) {
        ClearPendingPartyMovement(-1);
    }
    if (g_settings_6850c8.continuous_combat == 0) {
        g_level_block->refresh_party_panel = 1;
        g_combat_state->flag_001 = 0;
        ClearCombatSelection();
    }
    ++g_combat_state->value_004;
    ++g_value_659c14;
    if (g_combat_state->uiCurrentPartyAction == 1 || g_combat_state->uiCurrentPartyAction == 2) {
        gXStatus.flPartyMoveDistLimit = GetPartyMovementSpeed();
        ResetLevelMovement0041EEE0(gXStatus.flPartyMoveDistLimit, 0,
                                   g_combat_state->uiCurrentPartyAction == 2);
    } else {
        gXStatus.flPartyMoveDistLimit = 0.0f;
        ResetLevelMovement0041EEE0(gXStatus.flPartyMoveDistLimit, 1, 0);
    }
    g_combat_state->round_counter = GetPhaseStep();
    if (CombatMayAdvanceContinuously()) {
        ShowNotice(0xc, &g_wchar_00689b34, -1, -1, false);
        ShowNoticef(0xc, gppStringList[0x227], g_combat_state->value_004);
    }

    for (slot = 0; slot < 8; ++slot) {
        W8PartySlotRow* party = &g_status_685170.buffers.XChar[slot];
        W8Character* character = &g_status_685170.buffers.Char[slot];
        W8CombatCharacterRow* row = &g_combat_state->characters[slot];
        if (party->fOccupied) {
            row->flag_81 = 0;
            row->flag_80 = 0;
            row->spot_attempts_90 = 0;
            row->pending_action_repick_count = 0;
            row->interception_count = 0;
            row->flag_34 = character->hp_current == 0 || character->highest_condition >= 0xf;
            row->phase_clock_stamp = 0;
            row->flag_a4 = 0;
            CalcArmorClasses(character);
        }
    }
    RepickInvalidCombatTargets00536400();
    for (slot = 0; slot < 8; ++slot) {
        W8PartySlotRow* party = &g_status_685170.buffers.XChar[slot];
        W8CombatCharacterRow* row = &g_combat_state->characters[slot];
        if (party->fOccupied) {
            row->unknown_82[0] = 0;
            if (party->pending_action == 0) {
                W8Character* character = &g_status_685170.buffers.Char[slot];
                if (CharacterHasTrait00547940(character, 0x10) &&
                    character->Hand[0].weapon_skill == 0 &&
                    Random(100) < static_cast<unsigned char>(ScaleValueByProfessionLevel005479B0(
                                      character, 0x10, 12.0f))) {
                    row->unknown_82[0] = 1;
                }
            }
        }
    }

    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        W8MonsterInfo* monster = MonsterGetScriptPartByLocationIndex(index);
        if (monster->fInCombat) {
            W8MonsterCombatState* state = monster->pCombat;
            state->unknown_015 = 0;
            state->spot_attempts_13d = 0;
            state->pending_action_repick_count = 0;
            state->interception_count = 0;
            state->active = false;
            state->turn_started = 0;
            ++state->value_14c;
            state->unknown_151[1] = Random(100) < 75;
        }
    }
    ++g_combat_state->combat_update_count;
    UpdateAllMonsterAI();
    AssignCombatPhases004E89D0();
    RequestRedraw(0x1000ff);
    RequestRedraw(0x80000);

    if (g_settings_6850c8.verbose_combat_messages != 0 &&
        g_combat_state->uiCurrentPartyAction == 0) {
        for (slot = 0; slot < 8; ++slot) {
            W8PartySlotRow* party = &g_status_685170.buffers.XChar[slot];
            if (!IsPartySlotEligible00524A10(slot) ||
                !CharacterCanSwitchTo(slot, W8_TARGETING_CONTEXT_IN_COMBAT, 0, 0)) {
                continue;
            }
            if (party->action_03d == W8_ACTION_DEFEND) {
                PostCharacterNotice(slot, gppStringList[0x228]);
            }
            if (party->action_03d == 5) {
                if (party->target_in_combat.iType == W8_TARGET_KIND_CHARACTER) {
                    PostCharacterNotice(
                        slot, gppStringList[0x229],
                        g_status_685170.buffers.Char[party->target_in_combat.iChar].name);
                } else if (party->target_in_combat.iType == W8_TARGET_KIND_MONSTER) {
                    unsigned int index = MonsterGetIndexByLocationID(
                        0x5e2, "C:\\Projects\\Wizardry 8\\Local Code\\Combat.cpp",
                        party->target_in_combat.iMonsterID, 1);
                    W8MonsterInfo* target = MonsterGetScriptPartByLocationIndex(index);
                    PostCharacterNotice(slot, gppStringList[0x229], GetMonsterName(target, 0, 0));
                } else {
                    srAssertFail("FALSE", "C:\\Projects\\Wizardry 8\\Local Code\\Combat.cpp", 0x5e6,
                                 0);
                }
            }
        }
    }
    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        W8MonsterInfo* monster = MonsterGetScriptPartByLocationIndex(index);
        if (!monster->fInCombat || monster->hp_current == 0 || monster->highest_condition >= 0xe) {
            continue;
        }
        if (monster->action_kind == 1) {
            PostMonsterNotice(monster, gppStringList[0x228]);
        }
        if (monster->action_kind == 8) {
            if (monster->Target.iType == W8_TARGET_KIND_MONSTER) {
                unsigned int target_index = MonsterGetIndexByLocationID(
                    0x601, "C:\\Projects\\Wizardry 8\\Local Code\\Combat.cpp",
                    monster->Target.iMonsterID, 1);
                W8MonsterInfo* target = MonsterGetScriptPartByLocationIndex(target_index);
                PostMonsterNotice(monster, gppStringList[0x229], GetMonsterName(target, 0, 0));
            } else if (monster->Target.iType == W8_TARGET_KIND_CHARACTER) {
                PostMonsterNotice(monster, gppStringList[0x229],
                                  g_status_685170.buffers.Char[monster->Target.iChar].name);
            } else {
                srAssertFail("FALSE", "C:\\Projects\\Wizardry 8\\Local Code\\Combat.cpp", 0x60a, 0);
            }
        }
    }
    g_combat_state->unknown_a60 = 1;
}

/* Which of the two engagement counts to report - the forced one when combat
   says so, the derived one otherwise. */
// FUNCTION: WIZ8 0x004ed2b0
int GetEngagementCount(void)
{
    if (g_combat_state->uiCurrentPartyActionStatus != 0) {
        return g_combat_state->uiNextPartyAction;
    }
    return g_combat_state->uiCurrentPartyAction;
}

/* Whether the party is engaged at all. Being told so outright settles it;
   otherwise, with nothing derived, one conscious character whose combat row is
   flagged is enough. */
// FUNCTION: WIZ8 0x004e7e70
int IsPartyEngaged(void)
{
    unsigned int party_slot;

    if (g_combat_state->uiCurrentPartyActionStatus != 0) {
        return 1;
    }
    if (g_combat_state->uiCurrentPartyAction == 0) {
        for (party_slot = 0; party_slot < 8; ++party_slot) {
            if (g_status_685170.buffers.XChar[party_slot].fOccupied != 0 &&
                g_status_685170.buffers.Char[party_slot].hp_current != 0 &&
                g_status_685170.buffers.Char[party_slot].highest_condition < 0xf &&
                g_combat_state->characters[party_slot].flag_34 != 0) {
                return 1;
            }
        }
    }
    return 0;
}

/* Note that one character died this round. */
// FUNCTION: WIZ8 0x004ecdd0
void RecordCharacterDeath(int party_slot)
{
    if (gXStatus.fCombatMode != 0) {
        g_combat_state->pending_deaths[g_combat_state->pending_death_count] = party_slot;
        ++g_combat_state->pending_death_count;
    }
}

/* Record what one slot has chosen to do, and cache whether it is the first
   kind beside it. */
// FUNCTION: WIZ8 0x004e8290
void SetSlotAction(int party_slot, int action_kind, int action_detail)
{
    W8PartySlotRow* row = &g_status_685170.buffers.XChar[party_slot];

    row->action_kind = action_kind;
    row->action_detail = action_detail;
    row->action_is_berserk = action_kind == W8_ACTION_BERSERK;
}

/* Post one line to the combat log, if the log is on. The whole line is
   formatted whether or not it will be shown. */
// FUNCTION: WIZ8 0x004ed260
void CombatLog(const char* format, ...)
{
    char line[200];
    va_list arguments;

    va_start(arguments, format);
    vsprintf(line, format, arguments);
    if (g_combat_log_enabled_0068d810 != 0) {
        ShowNoticef(7, g_combat_log_format_00617664, line);
    }
}

/* Start a fresh round: lift the one action that does not survive it and clear
   the two round flags. */
// FUNCTION: WIZ8 0x004ecf00
void BeginCombatRound(void)
{
    int party_slot;

    if (gXStatus.fCombatMode == 0) {
        return;
    }
    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_status_685170.buffers.XChar[party_slot].pending_action == W8_ACTION_EQUIP) {
            g_status_685170.buffers.XChar[party_slot].pending_action = -1;
        }
    }
    g_combat_state->flag_a50 = 0;
    g_combat_state->flag_a51 = 0;
}

/* Bring one combatant's clock up to the current round: advance it by however
   many rounds have passed since it last caught up, clamp it, and remember
   where it got to. */
// FUNCTION: WIZ8 0x004eceb0
void CatchUpCombatActor(W8CombatCharacterRow* row)
{
    row->phase += g_combat_state->round_counter - row->phase_clock_stamp;
    ClampUnsignedInteger(&row->phase, g_combat_state->round_counter, 100);
    RoundPhaseToStep(&row->phase, g_combat_state->round_counter);
    row->phase_clock_stamp = g_combat_state->round_counter;
}

/* Whether one character can breathe again. The name is the Magic.cpp:5320
   assertion's own - assert(CanCharReBreathe(uiChar)) - and the rule is that
   they have to be free of the condition that forbids it and still hold a fifth
   of their stamina, the same fifth a run costs. */
// FUNCTION: WIZ8 0x004ebc80
bool CanCharReBreathe(int party_slot)
{
    W8Character* character = &g_status_685170.buffers.Char[party_slot];

    if (!CharacterHasCondition(character, 0x1c)) {
        return false;
    }
    return character->uiStaminaMax / 5 <= character->stamina;
}

/* Take one character out of the round: hand back whatever they were aiming at,
   clear the slot, and re-choose an action of the same kind. */
// FUNCTION: WIZ8 0x004e82e0
void DropCharacterFromRound(int party_slot)
{
    W8PartySlotRow* row = &g_status_685170.buffers.XChar[party_slot];

    if (GetCurrentTargetingContext(party_slot) != 2) {
        ClearTargetHighlights(party_slot, &row->target_in_combat);
    }
    ResetCombatSlot(&row->target_in_combat);
    RequestPartySlotRedraw(party_slot);
    if (party_slot == g_status_685170.selected_character) {
        RequestRedrawParty();
    }
    SetCharacterCombatAction(party_slot, row->action_kind, row->action_detail, 0, 0);
}

/* Tell every monster within short range about something. A monster has to be
   in combat, alive, not on its way out, free of whatever 0x087 records, and
   in the engaged state before it is told. */
// FUNCTION: WIZ8 0x004ecaa0
void NotifyNearbyMonsters(int what)
{
    unsigned int index;
    W8MonsterInfo* monster_info;

    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        monster_info = MonsterGetScriptPartByLocationIndex(index);
        if (monster_info->fInCombat != 0 && monster_info->hp_current != 0 &&
            monster_info->highest_condition < 0xe && monster_info->condition_turns[12] == 0 &&
            monster_info->ubDisposition == 1) {
            if (monster_info->monster->GetDistanceToPlayer004C7CB0() <=
                CalcRangeDistance(W8_RANGE_SHORT)) {
                NotifyMonsterOfSound(monster_info->monster, what);
            }
        }
    }
}

/* Whether the party notices what is coming. The first character in shape to
   act rolls against half their sixth attribute; nobody in shape at all, or the
   global being clear, and the answer is yes by default. */
// FUNCTION: WIZ8 0x004ed1c0
int PartyAvoidsSurprise(void)
{
    unsigned int party_slot = 0;

    if (gXStatus.fSurprisePossible == 0) {
        return 0;
    }
    while (g_status_685170.buffers.XChar[party_slot].fOccupied == 0 ||
           g_status_685170.buffers.Char[party_slot].hp_current == 0 ||
           g_status_685170.buffers.Char[party_slot].highest_condition > 10) {
        ++party_slot;
        if (party_slot > 7) {
            return 1;
        }
    }
    Random(1);
    if (g_status_685170.buffers.Char[party_slot].attributes[6].effective >> 1 <= Random(100)) {
        return 1;
    }
    return 0;
}

/* 0x004C62C0 */
// GLOBAL: WIZ8 0x005ed908
unsigned int g_flee_chance_005ed908 = 15;
// GLOBAL: WIZ8 0x005ed490
float g_movement_speed_step_005ed490 = 0.009999999776482582f;
/* What one character's whole turn is worth. A character whose turn combat has
   already set up uses the values it saved; anyone else is asked afresh. A
   phase of exactly a hundred is worth one whatever the hands say. */
// FUNCTION: WIZ8 0x004ec860
int GetCharacterTurnValue(int party_slot)
{
    W8CombatCharacterRow* row = &g_combat_state->characters[party_slot];
    int chosen;
    int total = 0;
    unsigned int hand;
    int value;

    ChooseCombatAction(party_slot, row->flag_34 == 0, &chosen, 0, 0, 0);
    if (chosen != 0 && chosen != 1) {
        return 1;
    }

    for (hand = 0; hand < 2; ++hand) {
        if (row->flag_34 == 0) {
            value = GetHandAttackValue(party_slot, hand);
        } else {
            value = row->saved_attack_value[hand];
        }
        if (row->phase == 100) {
            value = 1;
        }
        total += value;
    }
    return total;
}

/* Whether a wounded character panics. Only a character target counts, they
   have to be below the fraction of their hit points that triggers it, and then
   it is a roll - so the same wound does not always panic. */
// FUNCTION: WIZ8 0x004ece00
unsigned char TryPanicWoundedCharacter(const W8CombatSlot* target)
{
    W8Character* character;

    if (target->iType != W8_TARGET_KIND_CHARACTER) {
        return 0;
    }
    character = &g_status_685170.buffers.Char[target->iChar];
    if ((character->hp_current * 100) / static_cast<unsigned int>(character->uiHPMax) >=
        g_flee_hp_fraction_005ed8f8) {
        return 0;
    }
    if (Random(100) >= g_flee_chance_005ed908) {
        return 0;
    }
    ApplyCharacterEffect(character, g_effect_005ee610, 0, g_effect_argument_005ed8c8,
                         g_effect_argument_005ed914);
    return 1;
}

/* End one monster's turn: forget what it was doing, mark its combat state
   inactive, and - if it is still in the fight - either stand it down or turn
   it to face whoever it settled on. */
// FUNCTION: WIZ8 0x004e76f0
void EndMonsterTurn(W8MonsterInfo* monster_info)
{
    W8CombatSlot chosen;
    int target_location;

    monster_info->action_kind = -1;
    monster_info->action_detail = 0;
    monster_info->pCombat->phase = 0;
    monster_info->pCombat->active = 1;
    monster_info->pCombat->value_14c = 0;
    RequestRedraw(0x100000);

    if (monster_info->hp_current != 0 && monster_info->highest_condition < 0xe &&
        monster_info->condition_turns[12] == 0) {
        MonsterChooseTarget(monster_info, &chosen, 3);
        if (chosen.iType == 2) {
            NotifyMonsterIdle(monster_info->monster, 0);
            monster_info->flag_253 = 0;
            return;
        }
        if (chosen.iType == 3) {
            target_location = chosen.iMonsterID;
            NotifyMonsterFacing(monster_info->monster, GetMonsterByLocationID(target_location), 0);
        }
    }
    monster_info->flag_253 = 0;
}

/* Set one monster's turn up, once. How fast it moves through the turn depends
   on what it chose - fleeing is half speed and one action is half again - and
   an enchanted monster is quickened by ten per point instead of slowed by the
   condition it is under. */
// FUNCTION: WIZ8 0x004eb8c0
void SetUpMonsterTurn(W8MonsterInfo* monster_info)
{
    unsigned int speed = 100;
    float scale;

    if (monster_info->pCombat->turn_started != 0) {
        monster_info->monster->movement_complete_026 = 0;
        return;
    }

    scale = GetMonsterCombatMoveRange(monster_info);
    if (monster_info->action_kind == 9) {
        speed = 0x32;
    } else {
        if (monster_info->action_kind == 7) {
            speed = 0x96;
        }
        if (monster_info->enchantments[5].value_08 == 0) {
            if (monster_info->condition_turns[5] != 0) {
                speed -= 0x32;
            }
        } else {
            speed += monster_info->enchantments[5].value_00 * 10;
        }
    }

    monster_info->monster->SetMonsterTurnSpeed(speed * scale * g_movement_speed_step_005ed490);
    /* Four bytes inside cycle eight's block, cleared together. */
    monster_info->monster->movement_0c0.callback_progress_05c = 0.0f;
    monster_info->pCombat->turn_started = 1;
    monster_info->monster->movement_complete_026 = 0;
}

/* Finish one monster's attack: charge it the fatigue, drop the party's
   selection, and give it its next phase if it still has attacks left - which
   divides whatever is left of the round between them. */
// FUNCTION: WIZ8 0x004eb7f0
void EndMonsterAttack(W8MonsterInfo* monster_info)
{
    W8MonsterCombatState* combat = monster_info->pCombat;
    unsigned int next;

    GetMonsterDataForInfo(monster_info);
    FatigueMonster(monster_info, MonsterActionFatigueCost(monster_info), 0);
    MonsterSetNavigatorFlag25(monster_info->monster, 1);
    g_combat_state->eCombatActionStatus = 0;
    g_combat_state->pActionMonsterInfo = 0;

    if (combat->active == 0) {
        return;
    }
    if (monster_info->action_kind == 0 && combat->attacks_per_round != 0) {
        if (ChooseRandomMonsterAction(monster_info, 1, 0, 0)) {
            next = combat->phase +
                   (100 - g_combat_state->round_counter) / (combat->attacks_per_round + 1);
            combat->phase = next;
            if (next < 0x65) {
                RoundPhaseToStep(&combat->phase, g_combat_state->round_counter);
                return;
            }
        }
    }
    combat->phase = 0;
}

/* Whether one character may take an action, and take it if asked. A character
   whose turn combat has set up already only agrees to the action they are
   already on; anyone else may switch, except into the fourth action while
   something else forbids it. */
// FUNCTION: WIZ8 0x004ed2d0
unsigned char TryCharacterAction(int party_slot, int action, char commit)
{
    W8Character* character = &g_status_685170.buffers.Char[party_slot];

    if (character->hp_current == 0 || character->highest_condition >= 0xf) {
        return 0;
    }
    if (g_combat_state->characters[party_slot].flag_34 != 0) {
        return g_status_685170.buffers.XChar[party_slot].pending_action == action;
    }
    if (g_status_685170.buffers.XChar[party_slot].action_03d != action) {
        if (action != W8_ACTION_DEFEND) {
            return 0;
        }
        if (CharacterCanSwitchTo(party_slot, W8_TARGETING_CONTEXT_IN_COMBAT, 0, 0)) {
            return 0;
        }
    }
    if (commit) {
        SwitchCharacterTo(party_slot, action);
    }
    return 1;
}

/* Put one character on the defend or protect action: record the action on the
   slot row, clear both hands' chosen attack modes, and mark the combat row so
   the target display rebuilds. Protect additionally copies the current target
   into the out-of-combat target slot, and a switch to defend raises the
   combat row's attack flag unless the row is already on that action. */
// FUNCTION: WIZ8 0x004ed390
void SwitchCharacterTo(int party_slot, int action)
{
    if (action != W8_ACTION_DEFEND && action != W8_ACTION_PROTECT) {
        srAssertFail("(iCharAction == CHAR_ACTION_DEFEND) || (iCharAction == CHAR_ACTION_PROTECT)",
                     "C:\\Projects\\Wizardry 8\\Local Code\\Combat.cpp", 0x1686, 0);
    }
    W8PartySlotRow* row = &g_status_685170.buffers.XChar[party_slot];

    row->pending_action = action;
    row->attack_mode[0] = -1;
    row->attack_mode[1] = -1;
    if (action == W8_ACTION_PROTECT) {
        row->target_out_of_combat = row->target_in_combat;
    }
    g_combat_state->characters[party_slot].flag_34 = 1;
    RequestRedraw(1 << party_slot | 0x100000);
    if (action == W8_ACTION_DEFEND && row->action_03d != action) {
        g_combat_state->characters[party_slot].flag_a4 = 1;
    }
}

/* Take the party out of combat: end the free-turn phase and the current round,
   drop the temporary conditions, finish every engaged monster group, fold the
   surviving characters back into exploration state, release the combat state
   and restore the main-game UI. The mode passes through to the end-of-combat
   monster pass and gates the world reset when zero. */
// FUNCTION: WIZ8 0x004ea310
void EndCombat004EA310(int mode)
{
    if (gXStatus.fPartyMovementMode != 0) {
        BeginFreeTurnPhase();
    }
    if (gXStatus.fPartyMovementUi != 0) {
        ReleasePartyMovement();
    }
    RequestRedrawCombatBar();
    ClearAllMonsterHighlights();
    SetTargetingMode(0);
    RemoveConditionFromEveryone(5);
    RemoveConditionFromParty(0xd);
    RemoveAllEnchantments();
    ResetCombatEffects();
    ProcessMonstersAtCombatEnd(mode);
    unsigned int group_count = PLLength(gXStatus.plsMonsterGroupList);
    for (unsigned int group_index = 0; group_index < group_count; ++group_index) {
        W8MonsterGroup* group = GetMonsterGroupByListIndex(group_index);
        if (group->fInCombat != 0) {
            MonsterGroupLeaveCombat(group);
        }
        group_count = PLLength(gXStatus.plsMonsterGroupList);
    }
    if (g_combat_state->flag_a54 != 0) {
        ShowNotice(0xc, gppStringList[0x233], 1, -1, 0);
    }
    unsigned int active = CountActiveCharacters();
    if (active != 0 && g_combat_state->value_010 != 0) {
        if (g_status_685170.current_level < W8_LEVEL_COUNT) {
            g_status_685170.level_progress[g_status_685170.current_level].combat_end_count_01 += 1;
        }
        g_combat_state->value_010 /= active;
        AwardPartyExperience004EEF10(g_combat_state->value_014 + g_combat_state->value_010, 1);
        int* entry = g_status_685170.tail_3121.facts.status_ints_3121;
        int* end = entry + 1000;
        while (entry < end) {
            if (*entry == 1) {
                *entry = 2;
            }
            ++entry;
        }
    }
    gXStatus.combat_countdown = SetCountdownClock(120000);
    if (g_combat_state->uiNextPartyAction != 0) {
        ClearPendingPartyMovement(-1);
    }
    UpdateScreenOverlays(0);
    RestoreCombatFormation();
    ReconcilePartyEquipmentAfterCombat0053CD60();
    gXStatus.fCombatMode = 0;
    if (g_combat_state->unknown_a61 != 0) {
        ToggleSearchMode();
    }
    ZoomRadarMapOut();
    EnablePortraitAdvanceRegions0059BB70();
    DisableMainRegionSet();
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
        g_level_block->review_transition_active == 0) {
        ClearSurfaceRect(0x17, 0x34, 0x2d, 0x159);
        ClearSurfaceRect(0x253, 0x34, 0x269, 0x159);
        RequestRedraw(0x810ff);
    }
    if (gXStatus.fNpcDialogueMode == 0 && gXStatus.fSpellCastMode == 0 &&
        gXStatus.fItemSelectMode == 0) {
        SelectTextBox(0);
    }
    free(g_combat_state);
    g_combat_state = 0;
    SetFlag6081E4(1);
    MonsterForward4531A0();
    if (mode == 0) {
        SetEnvironmentTimeEnabled00482990(1);
    }
    ResetLivingMonstersAfterCombat();
    for (int slot = 0; slot < 8; ++slot) {
        if (g_status_685170.buffers.XChar[slot].fOccupied != 0) {
            CalcArmorClasses(&g_status_685170.buffers.Char[slot]);
        }
    }
    if (gXStatus.world_update_blocked != 0) {
        ResumeMainGameWorld();
    }
    ClearLevelDataFlags5To7();
    RequestRedrawParty();
    SetFloat60AB48();
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
        ApplyCombatEndEffects();
        ReportSaveFailed(1);
    }
}

/* Record a chosen action on the slot row: outside combat it fills the
   pending-action block, inside combat it fills the chosen-action record and
   dispatches the action's follow-up. */
// FUNCTION: WIZ8 0x004e7cc0
void ChooseAction(int party_slot, int action, int detail, const W8ActionDetailBlock* data,
                  int arg_5, int arg_6)
{
    W8PartySlotRow* row = &g_status_685170.buffers.XChar[party_slot];

    if (gXStatus.fCombatMode == 0) {
        row->pending_action = action;
        row->attack_mode[0] = detail;
        row->attack_mode[1] = -1;
        if (data == 0) {
            memset(&row->pending_action_detail_015, 0, sizeof(row->pending_action_detail_015));
        } else {
            memcpy(&row->pending_action_detail_015, data, sizeof(row->pending_action_detail_015));
        }
        if (arg_5 == 0) {
            ExecuteCharacterAction004EA5C0(party_slot);
        }
        ClearPartySlotMonsterHighlights(party_slot);
    } else {
        ApplyPartyCombatAction(party_slot, action, detail, data, arg_5, arg_6);
        switch (action) {
        case W8_ACTION_ATTACK:
        case W8_ACTION_BERSERK:
        case W8_ACTION_DEFEND:
        case W8_ACTION_PROTECT:
            row->action_kind = action;
            row->action_detail = detail;
            row->action_is_berserk = (action == W8_ACTION_BERSERK);
            break;
        default:
            if (action != W8_ACTION_WALK && action != W8_ACTION_RUN) {
                row->queued_action = static_cast<unsigned char>(action);
            }
        }
    }
    switch (action) {
    case W8_ACTION_TURN_UNDEAD:
    case W8_ACTION_DEFEND:
    case W8_ACTION_PRAY:
    case W8_ACTION_EQUIP:
        StartBreathCycle(party_slot, 0);
        return;
    case W8_ACTION_WALK:
    case W8_ACTION_RUN:
        ApplyItemEffectToRandomCharacter(g_special_event_0068c50c, -1, 0,
                                         g_effect_argument_005ed8c8);
        break;
    }
}

/* Apply a chosen in-combat action for party-move kinds 10/11, otherwise record
   the action on the slot row and refresh targeting UI state. */
// FUNCTION: WIZ8 0x004e7ee0
void ApplyPartyCombatAction(int party_slot, int action, int detail, const W8ActionDetailBlock* data,
                            int arg_5, int notify)
{
    unsigned int party_slot_index;
    W8Character* character;
    W8CombatCharacterRow* row;

    if (action != W8_ACTION_WALK && action != W8_ACTION_RUN) {
        SetCharacterCombatAction(party_slot, action, detail, data, arg_5);
        return;
    }
    if (g_combat_state->flag_000 == 0 || g_combat_state->uiCurrentPartyActionStatus != 0) {
        SetPendingMoveKind(action);
    } else {
        if (g_combat_state->uiCurrentPartyAction == 0) {
            party_slot_index = 0;
            do {
                character = &g_status_685170.buffers.Char[party_slot_index];
                row = &g_combat_state->characters[party_slot_index];
                if (g_status_685170.buffers.XChar[party_slot_index].fOccupied != 0 &&
                    character->hp_current != 0 && character->highest_condition < 0xf &&
                    row->flag_34 != 0) {
                    SetPendingMoveKind(action);
                    goto finish_move_ui;
                }
                ++party_slot_index;
            } while (party_slot_index < 8);
        }
        if (g_combat_state->eCombatActionStatus == 1 && g_combat_state->iActionChar != -1) {
            g_combat_state->eCombatActionStatus = 0;
            g_combat_state->iActionChar = -1;
        }
        StartPartyMovementAction((action != W8_ACTION_WALK) + 1);
    }
finish_move_ui:
    if (gXStatus.fPartyMovementUi == 0) {
        CreatePartyMovementPanel();
        SetTargetingMode(0);
        return;
    }
    InvalidatePartyMovementPanel();
    SetTargetingMode(0);
}

/* Record the chosen in-combat action on the slot row, copy its detail block,
   then aim and validate that choice for a still-active character. */
// FUNCTION: WIZ8 0x004e8000
void SetCharacterCombatAction(int party_slot, int action_kind, int action_detail,
                              const W8ActionDetailBlock* data, int notify)
{
    if (gXStatus.fSpellCastMode == 0 && gXStatus.fItemSelectMode == 0) {
        g_status_685170.buffers.XChar[party_slot].action_03d = -1;
        AimByKind(party_slot, W8_TARGET_KIND_NONE, W8_TARGETING_CONTEXT_CURRENT);
    }
    W8TargetSource source;
    W8CombatCharacterRow* row = &g_combat_state->characters[party_slot];
    SetTargetSourceToCharacter(party_slot, &source);
    if (action_kind > 0xb) {
        srAssertFail("iAction < CHAR_ACTION_COUNT",
                     "C:\\Projects\\Wizardry 8\\Local Code\\Combat.cpp", 0x484, 0);
    }
    g_status_685170.buffers.XChar[party_slot].action_03d = action_kind;
    g_status_685170.buffers.XChar[party_slot].action_detail_041 = action_detail;
    if (data == 0) {
        memset(&g_status_685170.buffers.XChar[party_slot].action_detail_045, 0,
               sizeof(g_status_685170.buffers.XChar[party_slot].action_detail_045));
    } else {
        memcpy(&g_status_685170.buffers.XChar[party_slot].action_detail_045, data,
               sizeof(g_status_685170.buffers.XChar[party_slot].action_detail_045));
    }
    row->action_changed_a7 = 1;
    W8Character* character = &g_status_685170.buffers.Char[party_slot];
    if (character->hp_current != 0 && character->highest_condition < 0xd && action_kind != -1) {
        if (CharacterCanSwitchTo(party_slot, W8_TARGETING_CONTEXT_IN_COMBAT, 1, notify) == 0) {
            AimByKind(party_slot, W8_TARGET_KIND_NONE, W8_TARGETING_CONTEXT_IN_COMBAT);
        } else if (TargetIsInPlay(party_slot, 2) == 0 &&
                   RepickActionTarget(party_slot, W8_TARGETING_CONTEXT_IN_COMBAT, notify) == 1) {
            PointCameraAtCombatTarget(&source,
                                      &g_status_685170.buffers.XChar[party_slot].target_in_combat);
        }
        if (notify != 0) {
            RevalidateSelectedTarget(party_slot);
        }
    }
    if (gXStatus.fCombatMode == 0) {
        return;
    }
    if (action_detail != action_kind && g_combat_state->flag_000 != 0 && row->flag_34 == 0) {
        row->phase += g_combat_state->round_counter - row->phase_clock_stamp;
        ClampUnsignedInteger(&row->phase, g_combat_state->round_counter, 100);
        RoundPhaseToStep(&row->phase, g_combat_state->round_counter);
        row->phase_clock_stamp = g_combat_state->round_counter;
    }
    RequestRedraw(1 << (party_slot & 0x1f));
    g_level_block->pick_changed_154 = 0;
    if (action_detail == 9) {
        PostCharacterNotice(party_slot, gppStringList[0x225]);
    } else if (action_kind == 9 &&
               !(g_combat_state->iActionChar == party_slot &&
                 g_status_685170.buffers.XChar[party_slot].pending_action == 9)) {
        PostCharacterNotice(party_slot, gppStringList[0x226]);
    }
    CalcArmorClasses(character);
}

/* Ask the slot's currently selected action which context its outputs hold:
   the chosen action kind plus three context-dependent words. */
// FUNCTION: WIZ8 0x004e77b0
void ChooseCombatAction(int party_slot, int context, int* out_kind, int* out_action,
                        W8CombatSlot** out_target, W8ActionDetailBlock** out_detail)
{
    W8PartySlotRow* row = &g_status_685170.buffers.XChar[party_slot];
    int kind;
    int value_a;
    W8CombatSlot* target;
    W8ActionDetailBlock* detail;

    if (context == W8_TARGETING_CONTEXT_CURRENT) {
        context = GetCombatActionContext0053BC90(party_slot);
    }
    switch (context) {
    case 0:
        kind = row->pending_action;
        value_a = row->attack_mode[0];
        target = &row->target_out_of_combat;
        detail = &row->pending_action_detail_015;
        break;
    case 1:
        kind = row->action_03d;
        value_a = row->action_detail_041;
        target = &row->target_in_combat;
        detail = &row->action_detail_045;
        break;
    case 2:
        if (gXStatus.fSpellCastMode == 0) {
            if (gXStatus.fItemSelectMode == 0) {
                srAssertFail("gXStatus.fItemSelectMode",
                             "C:\\Projects\\Wizardry 8\\Local Code\\Combat.cpp", 0x2e0, 0);
            }
            gXStatus.shared_action_detail.item_use.item = GetSelectedOrFallbackValue0059E0D0();
            kind = 8;
            value_a = -1;
            target = &gXStatus.shared_target;
            detail = &gXStatus.shared_action_detail;
        } else {
            kind = 7;
            value_a = GetSpellCastingSelection005A1350();
            target = &gXStatus.shared_target;
            detail = &gXStatus.shared_action_detail;
        }
        break;
    case 3:
        value_a = row->spell_id;
        kind = 7;
        target = &row->spell_target;
        detail = &row->spell_detail;
        break;
    case 4:
        value_a = -1;
        kind = 8;
        target = &row->item_target;
        detail = &row->item_detail;
        break;
    case 5:
        value_a = -1;
        target = &row->target_context_5;
        kind = 2;
        detail = 0;
        break;
    case 7:
        kind = g_level_block->move_budget_2dc;
        value_a = g_level_block->move_budget_2e0;
        target = 0;
        detail = 0;
        break;
    case 8:
        kind = 0;
        value_a = -1;
        target = 0;
        detail = 0;
        break;
    default:
        srAssertFail("FALSE", "C:\\Projects\\Wizardry 8\\Local Code\\Combat.cpp", 0x310, 0);
        value_a = context;
        target = reinterpret_cast<W8CombatSlot*>(
            context); // reinterpret-ok: retail stores the context word into the generic output slots after the FALSE assert
        detail = reinterpret_cast<W8ActionDetailBlock*>(
            context); // reinterpret-ok: retail stores the context word into the generic output slots after the FALSE assert
        kind = context;
        break;
    }
    if (CanPartySlotParticipate(context) == 0 && kind != W8_ACTION_WALK && kind != W8_ACTION_RUN) {
        kind = -1;
        value_a = -1;
        target = 0;
        detail = 0;
    }
    if (out_kind != 0) {
        *out_kind = kind;
    }
    if (out_action != 0) {
        *out_action = value_a;
    }
    if (out_target != 0) {
        *out_target = target;
    }
    if (out_detail != 0) {
        *out_detail = detail;
    }
}

/* Whether a slot may switch to the given action context, optionally repairing
   the aim state first when the caller allows it. */
// FUNCTION: WIZ8 0x004e79a0
bool CharacterCanSwitchTo(int party_slot, W8TargetingContext context, int arg_3, int arg_4)
{
    W8Character* character = &g_status_685170.buffers.Char[party_slot];

    if (gXStatus.iTargetingMode == 0 && IsScreenIdle() != 0) {
        for (unsigned int slot = 0; slot < 8; ++slot) {
            if (IsPartySlotEligible00524A10(slot) != 0) {
                return 1;
            }
        }
        return 0;
    }
    if (CanPartySlotParticipate(party_slot) == 0) {
        return 0;
    }
    if (character->highest_condition > 0xd) {
        return 0;
    }
    if (context == W8_TARGETING_CONTEXT_CURRENT) {
        context = GetCombatActionContext0053BC90(party_slot);
    }
    int chosen;
    int value_a;
    W8ActionDetailBlock* detail;
    ChooseCombatAction(party_slot, context, &chosen, &value_a, 0, &detail);
    if (chosen == -1) {
        return 0;
    }
    switch (chosen) {
    case W8_ACTION_ATTACK:
        if (CanAnyHandReachTarget(party_slot) == 0) {
            if (arg_4 == 0) {
                return 0;
            }
            if (character->EquippedItem[6].iItemNo != -1) {
                if (character->EquippedItem[7].iItemNo == -1) {
                    EquipMatchingPartnerItem(character, &character->EquippedItem[6], -1, 7);
                }
                if (character->EquippedItem[6].uses_or_charges == 0) {
                    MergeMatchingPartnerItem(character, &character->EquippedItem[6]);
                }
            }
            if (CanAnyHandReachTarget(party_slot) == 0) {
                return 0;
            }
        }
        break;
    case W8_ACTION_BERSERK:
        if (CanCharacterBerserk(party_slot) == 0) {
            if (arg_4 == 0) {
                return 0;
            }
            if (character->EquippedItem[6].iItemNo != -1) {
                if (character->EquippedItem[7].iItemNo == -1) {
                    EquipMatchingPartnerItem(character, &character->EquippedItem[6], -1, 7);
                }
                if (character->EquippedItem[6].uses_or_charges == 0) {
                    MergeMatchingPartnerItem(character, &character->EquippedItem[6]);
                }
            }
            if (CanCharacterBerserk(party_slot) == 0) {
                return 0;
            }
        }
        break;
    case W8_ACTION_BREATHE:
        if (CharacterHasTrait00547940(character, W8_TRAIT_BREATHE) == 0 ||
            character->stamina <
                static_cast<int>(static_cast<unsigned int>(character->uiStaminaMax) / 5)) {
            return 0;
        }
        break;
    case W8_ACTION_DEFEND:
        return 1;
    case W8_ACTION_PROTECT:
        if (CanCharacterAttack(party_slot) == 0) {
            return 0;
        }
        break;
    case W8_ACTION_CAST_SPELL:
        if (context == W8_TARGETING_CONTEXT_DIALOGUE && CharacterHasCastableSpell(character) != 0) {
            return 1;
        }
        if (value_a == 0) {
            return 0;
        }
        if (CanCharacterCastSpell(character, value_a) == 0) {
            return 0;
        }
        break;
    case W8_ACTION_USE_ITEM:
        if (context == W8_TARGETING_CONTEXT_DIALOGUE && detail == 0) {
            return 1;
        }
        if (detail->item_use.item == 0) {
            return 0;
        }
        if (CanCharacterActivateItem(character, detail->item_use.item) == 0) {
            return 0;
        }
        break;
    }
    if (arg_3 == 0) {
        W8TargetingContext validated = GetValidatedTargetingContext(party_slot, context);
        if (TargetIsInPlay(party_slot, 2, validated) == 0) {
            return 0;
        }
    }
    return 1;
}

/* Face the monster toward whatever its combat slot targets before the attack
   starts: its own side, a specific monster, the nearest visible group member,
   or a point on the ground. `alternate` picks the immediate versus the
   animated turn. */
// FUNCTION: WIZ8 0x004EC900
void OrientMonsterTowardTarget(W8MonsterInfo* monster_info, char alternate)
{
    W8MonsterInfo* target;
    unsigned int index;
    int location_id;
    srVector3T<float> point;

    switch (monster_info->Target.iType) {
    case W8_TARGET_KIND_NONE:
    case W8_TARGET_KIND_FIVE:
        break;
    default:
        if (IsMonsterFacingParty(monster_info) == 0) {
            MonsterForwardReferencePosition(monster_info->monster, alternate);
        }
        break;
    case W8_TARGET_KIND_MONSTER:
        location_id = monster_info->Target.iMonsterID;
        if (monster_info->location_id != location_id) {
            index = MonsterGetIndexByLocationID(
                0x1320, "C:\\Projects\\Wizardry 8\\Local Code\\Combat.cpp", location_id, 1);
            target = MonsterGetScriptPartByLocationIndex(index);
            if (IsMonsterFacingMonster(monster_info, target) == 0) {
                MonsterAimAtMonster004C62C0(monster_info->monster, target->monster, alternate);
                return;
            }
        }
        break;
    case W8_TARGET_KIND_GROUP:
        location_id =
            FindNearestVisibleGroupMonster(monster_info, monster_info->Target.iGroupID, 3);
        if (location_id != -1) {
            index = MonsterGetIndexByLocationID(
                0x136e, "C:\\Projects\\Wizardry 8\\Local Code\\Combat.cpp", location_id, 1);
            target = MonsterGetScriptPartByLocationIndex(index);
            if (IsMonsterFacingMonster(monster_info, target) == 0) {
                MonsterAimAtMonster004C62C0(monster_info->monster, target->monster, alternate);
                return;
            }
        }
        break;
    case W8_TARGET_KIND_PLACE:
        point = monster_info->Target.point;
        if (IsPartyLookingAt(monster_info, point) == 0) {
            if (alternate != 0) {
                monster_info->monster->SetFacingToward(&point);
                return;
            }
            monster_info->monster->AimAtPosition(&point);
            return;
        }
    }
}

/* Whether combat must keep waiting on a live monster's pending cycle, running
   animation or unfinished turn before the action state machine may advance. */
// FUNCTION: WIZ8 0x004e8c30
bool AnyCombatMonsterBusy004E8C30(void)
{
    unsigned int monster_count = PLLength(gXStatus.plsMonsterList);
    for (unsigned int monster_index = 0; monster_index < monster_count; ++monster_index) {
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
        W8Monster* monster = monster_info->monster;
        int attack_mode = MonsterQuery(monster, 6);
        if (attack_mode == 0x15 && GetMonsterDataForInfo(monster_info)->record_id_187 == 0x234) {
            return true;
        }
        if (monster_info->fInCombat != 0) {
            if (MonsterHasPendingCycle(monster) != 0 || MonsterHasCycle19Flag3(monster) != 0) {
                return true;
            }
            if (MonsterIsAnimating(monster) != 0 && attack_mode != 1 && attack_mode != 2 &&
                attack_mode != 4 && attack_mode != 0x15) {
                if (g_combat_state->eCombatActionStatus != 2) {
                    return true;
                }
                if (g_combat_state->pActionMonsterInfo != monster_info) {
                    return true;
                }
                if (NormalizeAttackMode(attack_mode) != 6) {
                    if (NormalizeAttackMode(attack_mode) != 7) {
                        return true;
                    }
                    return monster_info->f_missile_released == 0;
                }
                if (MonsterQuery(monster, 4) < MonsterQuery(monster, 0) / 2) {
                    return true;
                }
            }
            if (fabsf(monster->movement_0c0.target_yaw - monster->movement_0c0.yaw) >=
                    g_camera_transition_epsilon_005ebc84 &&
                monster_info->highest_condition < 0xe) {
                return true;
            }
        }
        monster_count = PLLength(gXStatus.plsMonsterList);
    }
    return false;
}

/* While the auto-advance option is on and the party is in combat with nothing
   else capturing input, move the selection to the next party slot that is a
   live member without a combat action to take. */
// FUNCTION: WIZ8 0x004e8da0
void AutoAdvanceSelectedCharacter004E8DA0(void)
{
    if (g_settings_6850c8.auto_advance_character == 0 || g_level_block->pick_changed_154 != 0 ||
        g_combat_state->flag_001 == 0 || gXStatus.iTargetingMode != 0 ||
        gXStatus.fSpellCastMode != 0 || gXStatus.fItemSelectMode != 0) {
        return;
    }
    int selected = g_status_685170.selected_character;
    if (CharacterCanSwitchTo(selected, W8_TARGETING_CONTEXT_IN_COMBAT, 0, 0) == 0 &&
        IsPartySlotEligible00524A10(selected) != 0) {
        return;
    }
    if (selected == -1) {
        srAssertFail("gStatus.iSelectedCharacter != BAD_INDEX",
                     "C:\\Projects\\Wizardry 8\\Local Code\\Combat.cpp", 0x72d, 0);
    }
    int party_slot = selected + 1;
    if (party_slot > 7) {
        party_slot = 0;
    }
    if (party_slot == selected) {
        return;
    }
    do {
        if (CharacterCanSwitchTo(party_slot, W8_TARGETING_CONTEXT_IN_COMBAT, 0, 0) == 0 &&
            IsPartySlotEligible00524A10(party_slot) != 0) {
            if (g_status_685170.selected_character == party_slot) {
                return;
            }
            SelectPartyCharacter(party_slot);
            return;
        }
        party_slot = party_slot + 1;
        if (party_slot > 7) {
            party_slot = 0;
        }
    } while (party_slot != g_status_685170.selected_character);
}

/* Assign each combatant's phase for the round: the living, active party slots
   and in-combat monsters roll initiative into a phase under ninety steps,
   everyone else is parked with no phase. */
// FUNCTION: WIZ8 0x004e89d0
void AssignCombatPhases004E89D0(void)
{
    for (int party_slot = 0; party_slot < 8; ++party_slot) {
        W8PartySlotRow* party_row = &g_status_685170.buffers.XChar[party_slot];
        W8Character* character = &g_status_685170.buffers.Char[party_slot];
        W8CombatCharacterRow* row = &g_combat_state->characters[party_slot];
        if (party_row->fOccupied == 0 || character->hp_current == 0 ||
            character->highest_condition > 0xe || g_combat_state->party_surprised_a52 != 0) {
            party_row->pending_action = -1;
            row->phase = 0;
            row->flag_34 = 1;
        } else {
            int value = Random(4) + character->initiative;
            int action = party_row->action_03d;
            if (action == W8_ACTION_ATTACK || action == W8_ACTION_BERSERK) {
                int hand = ChooseCharacterAttackHand(party_slot);
                if (hand != -1) {
                    value += character->Hand[hand].damage_bonus;
                }
            }
            ClampInteger(&value, -10, 0x59);
            row->phase = 0x5a - value;
            RoundPhaseToStep(&row->phase, 10);
            if (gXStatus.hostile_monster_count != 0 &&
                character->skills[W8_SKILL_SNAKESPEED].flag_00 != 0 && Random(5) == 0) {
                PracticeCharacterSkill(character, W8_SKILL_SNAKESPEED, 1, 0);
            }
        }
    }
    if (g_combat_state->uiCurrentPartyAction != 0) {
        InitializePartyMovementPhase();
    }
    unsigned int monster_count = PLLength(gXStatus.plsMonsterList);
    for (unsigned int monster_index = 0; monster_index < monster_count; ++monster_index) {
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
        if (monster_info->fInCombat != 0) {
            bool side_done;
            if (monster_info->ubDisposition == DISP_HOSTILE) {
                side_done = g_combat_state->monsters_surprised_a53;
            } else {
                side_done = g_combat_state->party_surprised_a52;
            }
            if (monster_info->hp_current == 0 || monster_info->highest_condition > 0xe ||
                side_done != 0) {
                monster_info->action_kind = -1;
                monster_info->pCombat->phase = 0;
                monster_info->pCombat->active = 1;
            } else {
                int value = Random(4) + GetMonsterDataForInfo(monster_info)->initiative_0e4;
                ClampInteger(&value, -10, 0x59);
                monster_info->pCombat->phase = 0x5a - value;
                RoundPhaseToStep(&monster_info->pCombat->phase, 10);
            }
        }
        monster_count = PLLength(gXStatus.plsMonsterList);
    }
}

/* Award the round's skill practice: each hand record's successful-swing count
   rolls its weapon and combat skills, and the three defensive skills flagged
   during the round each earn two usage points. */
// FUNCTION: WIZ8 0x004e99c0
void PracticeCombatRoundSkills004E99C0(int party_slot)
{
    W8Character* character = &g_status_685170.buffers.Char[party_slot];
    W8CombatCharacterRow* row = &g_combat_state->characters[party_slot];
    for (int hand = 0; hand < 2; ++hand) {
        W8CombatHandRecord* hand_record = &row->hand_records[hand];
        unsigned int chance;
        if (hand_record->dual_wielding == 0) {
            chance = hand_record->weapon_skill != W8_SKILL_PICKPOCKET ? 100 : 50;
        } else {
            chance = 0x4b;
        }
        while (hand_record->score != 0) {
            if (Random(100) < chance) {
                int skill_id = 0;
                int pick = Random((hand_record->dual_wielding != 0) + 2);
                if (pick == 0) {
                    skill_id = hand_record->weapon_skill;
                } else if (pick == 1) {
                    skill_id = hand_record->combat_skill;
                } else if (pick == 2) {
                    skill_id = W8_SKILL_DUAL_WEAPONS;
                }
                PracticeCharacterSkill(character, skill_id, 1, 0);
                if (skill_id == hand_record->weapon_skill &&
                    hand_record->combat_skill == W8_SKILL_CLOSE_COMBAT && Random(100) < 0x19) {
                    PracticeCharacterSkill(character, skill_id, 1, 0);
                }
            }
            hand_record->score = hand_record->score - 1;
        }
    }
    for (int skill_id = 0; skill_id < W8_SKILL_COUNT; ++skill_id) {
        if (row->skill_use_flags[skill_id] != 0 &&
            (skill_id == W8_SKILL_SHIELD || skill_id == W8_SKILL_LOCKS_TRAPS ||
             skill_id == W8_SKILL_REFLEXTION)) {
            PracticeCharacterSkill(character, skill_id, 2, 0);
            row->skill_use_flags[skill_id] = 0;
        }
    }
}

/* The round boundary. Restore stamina to resting combatants, award the round's
   skill practice, track how long nobody could engage, advance the clock, and
   refresh engagement, sight and targets for the next round. */
// FUNCTION: WIZ8 0x004e9b20
void AdvanceCombatRound004E9B20(void)
{
    W8Dice dice;
    unsigned int index;

    g_combat_state->flag_a62 = 0;
    if (gXStatus.fPartyMovementMode != 0) {
        BeginFreeTurnPhase();
    }
    if (g_settings_6850c8.continuous_combat == 0 && g_combat_state->uiNextPartyAction != 0) {
        ClearPendingPartyMovement(-1);
    }
    ReconcilePartyFormation(&gXStatus.edited_formation, &g_status_685170.formation);
    if (gXStatus.hostile_monster_count != 0) {
        ShowNoticef(0xc, gppStringList[0x22b], g_combat_state->value_004);
        SoundPlay("Data\\Sound\\Misc\\EndTurnChime.wav", 0);
    }
    g_combat_state->party_surprised_a52 = 0;
    g_combat_state->monsters_surprised_a53 = 0;
    if (g_combat_state->flag_a55 == 0) {
        g_combat_state->unengaged_rounds_a56 = 0;
    } else {
        index = 0;
        while (index < PLLength(gXStatus.plsMonsterGroupList)) {
            W8MonsterGroup* monster_group = GetMonsterGroupByListIndex(index);
            if (monster_group->flag_28 != 0 && monster_group->fInCombat != 0 &&
                MonsterGroupCanEngage(monster_group) != 0) {
                goto groups_checked;
            }
            ++index;
        }
        g_combat_state->unengaged_rounds_a56 = g_combat_state->unengaged_rounds_a56 + 1;
    }
groups_checked:
    AdvanceEnvironmentTime00482A20(120000);
    if (g_camera_sway_active_652da4 != 0) {
        UpdateCampFatigue005044D0(10);
    }
    RequestRedrawCombatBar();
    g_combat_state->flag_000 = 0;
    g_combat_state->flag_001 = 1;
    if (g_combat_state->pending_death_count != 0) {
        int active_characters = CountActiveCharacters();
        if (active_characters != 0) {
            if (active_characters == 1) {
                QueueLastSurvivorEvent();
            } else if (g_combat_state->pending_death_count > 1) {
                QueueTurnReactionEvent();
            }
        }
    }
    for (int party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_status_685170.buffers.XChar[party_slot].fOccupied != 0) {
            W8Character* character = &g_status_685170.buffers.Char[party_slot];
            if (g_status_685170.buffers.XChar[party_slot].pending_action == 4 ||
                character->highest_condition == 0xf || character->highest_condition == 0x11) {
                int sides = static_cast<unsigned int>(character->uiStaminaMax) / 0x14 - 1;
                if (sides == 0) {
                    sides = 1;
                }
                SetDice(&dice, 2, sides, 0);
                int amount = RollDice(&dice);
                if (CharacterHasTrait00547940(character, 0) != 0) {
                    amount = static_cast<int>(
                        ScaleValueByProfessionLevel005479B0(character, 0, 3.3f) * amount);
                }
                RestoreCharacterStamina(party_slot, amount,
                                        character->stamina < character->uiStaminaMax);
            }
            PracticeCombatRoundSkills004E99C0(party_slot);
            g_combat_state->characters[party_slot].flag_34 =
                (character->hp_current == 0 || character->highest_condition > 0xe) ? 1 : 0;
            CalcArmorClasses(character);
        }
    }
    index = 0;
    while (index < PLLength(gXStatus.plsMonsterList)) {
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(index);
        if (monster_info->fInCombat != 0) {
            if (monster_info->action_kind == 1 || monster_info->highest_condition == 0xf ||
                monster_info->highest_condition == 0x11) {
                int sides = static_cast<unsigned int>(monster_info->stamina_max) / 0x14 - 1;
                if (sides == 0) {
                    sides = 1;
                }
                SetDice(&dice, 2, sides, 0);
                RestoreMonsterStamina(monster_info, RollDice(&dice), 0);
            }
            if (monster_info->pCombat->unknown_145[1] != 0 && monster_info->action_kind != 3) {
                monster_info->pCombat->unknown_145[1] = monster_info->pCombat->unknown_145[1] - 1;
            }
        }
        ++index;
    }
    RequestRedraw(0x1000ff);
    CheckMonsterGroupsLeaveCombat();
    UpdateMonsterGroupEngagement();
    CheckMonsterGroupsEnterCombat();
    DetectMonsterGroups004E4AB0();
    RefreshOutwardSightForAllMonsters();
    RefreshInwardSightForAllMonsters();
    RepickInvalidCombatTargets00536400();
    index = 0;
    while (index < PLLength(gXStatus.plsMonsterList)) {
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(index);
        if (monster_info->fInCombat != 0 && monster_info->hp_current != 0 &&
            monster_info->highest_condition < 0xe && monster_info->condition_turns[0xc] == 0) {
            W8CombatSlot chosen;
            MonsterChooseTarget(monster_info, &chosen, 3);
            if (chosen.iType == 2) {
                MonsterForwardReferencePosition(monster_info->monster, 0);
            } else if (chosen.iType == 3) {
                MonsterAimAtMonster004C62C0(monster_info->monster,
                                            GetMonsterByLocationID(chosen.iMonsterID), 0);
            }
        }
        ++index;
    }
}

/* Whether combat is over and how it ended. Returns 1 once the combat has been
   torn down - the all-dead defeat, the no-kill end, or the victory fanfare -
   and 0 while it may continue. */
// FUNCTION: WIZ8 0x004e9f90
int CheckCombatEnd004E9F90(unsigned int arg_1)
{
    bool any_active = AnyCharacterActive();
    RecountCombatMonsters();
    if (any_active == 0) {
        ShowNotice(0xc, gppStringList[0x230], 0, -1, 0);
        for (int party_slot = 0; party_slot < 8; ++party_slot) {
            W8Character* character = &g_status_685170.buffers.Char[party_slot];
            if (g_status_685170.buffers.XChar[party_slot].fOccupied != 0 &&
                (character->hp_current != 0 || character->highest_condition < 0x12)) {
                SetCharacterCondition(party_slot, 0x12, 9999, 0, 0, 1);
            }
        }
    } else {
        if (((arg_1 == 0 || gXStatus.hostile_monster_count != 0) ||
             gXStatus.hostile_group_count != 0) &&
            g_combat_state->unengaged_rounds_a56 < 2) {
            return 0;
        }
        if (g_combat_state->combat_result_00c == 0) {
            if (g_combat_state->flag_a54 != 0) {
                StartLevelMusic(1, 1);
                EndCombat004EA310(0);
                return 1;
            }
            ShowNotice(0xc, gppStringList[0x22e], 0, -1, 0);
            if (CombatHasContinuingEffects()) {
                ShowNotice(0xc, gppStringList[0x22f], 0, -1, 0);
                StartLevelMusic(1, 1);
                EndCombat004EA310(0);
                return 1;
            }
        } else {
            unsigned int party_slot;
            W8CharacterEvent* event;
            ShowNotice(0xc, gppStringList[0x22c], 0, -1, 0);
            FormatNotice(0xc, 0, gppStringList[0x22d], g_combat_state->combat_result_00c,
                         g_combat_state->combat_result_00c == 1 ? L"kill" : L"kills");
            StartMusicResource0048FC10("CombatWin.MPL", 0, 1);
            ServiceMusicPlaylist0048F9E0();
            StartLevelMusic(1, 0);
            if (gXStatus.combat_difficulty != 0 &&
                GetRandomPartySlots(0, 0, -1, &party_slot, 1, 0) != 0 &&
                (event =
                     QueueCharacterEvent(&g_status_685170.buffers.Char[party_slot],
                                         g_effect_005ee614, g_effect_argument_005ed8d8,
                                         g_effect_argument_005ed8cc, g_effect_argument_005ed914),
                 event != 0)) {
                event->dispatch_delay_ms = 0x5dc;
                event->dispatch_delay_start = GetTickCount();
                EndCombat004EA310(0);
                return 1;
            }
        }
    }
    EndCombat004EA310(0);
    return 1;
}

/* Roll for surprise at combat start: the party side through the first
   threatening monster's opposed check plus level/search bonuses, and the
   hostile side when none of them can see the party while surprise is
   possible. Mutual surprise cancels. */
// FUNCTION: WIZ8 0x004ecf50
void RollCombatSurprise004ECF50(char arg_1)
{
    bool search_surprise = false;
    bool level_surprise = false;
    unsigned int index;
    W8MonsterInfo* monster_info;

    if (gXStatus.fCombatMode == 0) {
        srAssertFail("gXStatus.fCombatMode", "C:\\Projects\\Wizardry 8\\Local Code\\Combat.cpp",
                     0x1552, 0);
    }
    if (arg_1 == 0) {
        if (g_status_685170.party_modifiers_22e3.sight_override_4a == 0) {
            index = 0;
            while (index < PLLength(gXStatus.plsMonsterList)) {
                monster_info = MonsterGetScriptPartByLocationIndex(index);
                if (monster_info->fActive != 0 && monster_info->fInCombat != 0 &&
                    monster_info->hp_current != 0 && monster_info->highest_condition < 0x10 &&
                    monster_info->ubDisposition == 1 &&
                    monster_info->party_threat.sight_state_04 == W8_SIGHT_SEEN) {
                    if (PartyAvoidsSurprise() == 0) {
                        goto party_surprise_done;
                    }
                    break;
                }
                ++index;
            }
            g_combat_state->party_surprised_a52 = 1;
        }
    party_surprise_done:
        if (g_combat_state->party_surprised_a52 == 0) {
            if (GetLevelDataFlag8() != 0) {
                if (Random(100) < 0x14) {
                    g_combat_state->party_surprised_a52 = 1;
                    level_surprise = true;
                }
            } else if (g_status_685170.search_mode != 0 && Random(100) < 0x14) {
                g_combat_state->party_surprised_a52 = 1;
                search_surprise = true;
            }
        }
    }
    index = 0;
    while (index < PLLength(gXStatus.plsMonsterList)) {
        monster_info = MonsterGetScriptPartByLocationIndex(index);
        if (monster_info->fActive != 0 && monster_info->fInCombat != 0 &&
            monster_info->hp_current != 0 && monster_info->highest_condition < 0x10 &&
            monster_info->ubDisposition == 1 &&
            monster_info->player_visibility.sight_state_04 == W8_SIGHT_SEEN) {
            goto monsters_checked;
        }
        ++index;
    }
    if (gXStatus.fSurprisePossible == 0) {
        g_combat_state->monsters_surprised_a53 = 1;
    }
monsters_checked:
    if (g_combat_state->party_surprised_a52 != 0 && g_combat_state->monsters_surprised_a53 != 0) {
        g_combat_state->party_surprised_a52 = 0;
        g_combat_state->monsters_surprised_a53 = 0;
    }
    if (g_combat_state->party_surprised_a52 == 0) {
        if (g_combat_state->monsters_surprised_a53 != 0) {
            ShowNotice(0xc, gppStringList[0x241], -1, -1, 0);
        }
        return;
    }
    g_combat_state->flag_001 = 0;
    if (level_surprise) {
        ShowNotice(0xc, gppStringList[0x23e], -1, -1, 0);
        return;
    }
    if (!search_surprise) {
        ShowNotice(0xc, gppStringList[0x240], -1, -1, 0);
        return;
    }
    ShowNotice(0xc, gppStringList[0x23f], -1, -1, 0);
}

/* `relationship` is only read on the interrupt-8/9 retarget paths that assign
   it; the post-switch `if (interrupt == 8)/else if (interrupt == 9)` tails are
   dead code the authored source carried and clang cannot prove unreachable. */
/* Commit the slot's chosen combat action and run it. The in-combat action the
   chooser stored becomes the live pending action; a condition interrupt can
   cancel or redirect it first, then the action-kind switch executes it and
   the tail flags engagement and charges the fatigue. */
// FUNCTION: WIZ8 0x004EA5C0
void ExecuteCharacterAction004EA5C0(int party_slot)
{
    W8Character* character = &g_status_685170.buffers.Char[party_slot];
    W8PartySlotRow* slot = &g_status_685170.buffers.XChar[party_slot];
    W8TargetSource source;
    W8TargetSource enemy_source;
    int interrupt = -1;
    int fatigue_cost = -1;
    unsigned char result = 1;
    bool saved_flag_34;
    int action;
    int detail;

    FaceCameraToSelection(party_slot);
    if (gXStatus.fCombatMode != '\0') {
        saved_flag_34 = g_combat_state->characters[party_slot].flag_34;
        if (saved_flag_34 == '\0') {
            slot->pending_action = slot->action_03d;
            slot->attack_mode[0] = slot->action_detail_041;
            slot->attack_mode[1] = -1;
            slot->pending_action_detail_015 = slot->action_detail_045;
            slot->target_out_of_combat = slot->target_in_combat;
            g_combat_state->characters[party_slot].action_changed_a7 = 0;
            if ((slot->pending_action == 0 || slot->pending_action == 1) &&
                CharacterCanSwitchTo(party_slot, W8_TARGETING_CONTEXT_OUT_OF_COMBAT, 0, 0) != 0) {
                PrepareCharacterAttacks(party_slot);
            }
        } else if ((slot->pending_action == 0 || slot->pending_action == 1) &&
                   g_combat_state->characters[party_slot].flag_81 != '\0') {
            W8CombatCharacterRow* row = &g_combat_state->characters[party_slot];
            row->hand_attack_values_40[0] = 0;
            row->hand_attack_values_40[1] = 0;
            row->uiSwingsRemaining = 0;
            slot->pending_action = -1;
        }
    }
    action = slot->pending_action;
    detail = slot->attack_mode[0];
    if (gXStatus.fCombatMode != '\0') {
        if (action != 4 && action != 5) {
            g_combat_state->characters[party_slot].flag_34 = 1;
            RequestRedraw((1 << party_slot) | 0x100000);
        }
        if (CharacterCanSwitchTo(party_slot, W8_TARGETING_CONTEXT_OUT_OF_COMBAT, 0, 0) == 0) {
            slot->pending_action = -1;
            action = -1;
        }
        SetTargetSourceToCharacter(party_slot, &source);
        interrupt = GetConditionInterrupt004EC1E0(&source);
        if (interrupt != -1) {
            char relationship = 0;
            PostCharacterNotice(party_slot,
                                gppStringList[g_condition_notices_0061E570[interrupt + 0x74]]);
            switch (interrupt) {
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 10:
                slot->pending_action = -1;
                action = -1;
                break;
            case 7:
                slot->pending_action = -1;
                action = -1;
                RemoveCharacterCondition(party_slot, 0xe, 1);
                goto interrupt_done;
            case 8:
                g_combat_state->characters[party_slot].flag_80 = 1;
                relationship = 2;
                goto ranged_retarget;
            case 9:
                relationship = 1;
                goto melee_retarget;
            }
            if (interrupt == 8) {
            ranged_retarget:
                if (GetBestHandRangeCategory(character) < W8_RANGE_LONG) {
                    action = 1;
                    slot->pending_action = 1;
                } else {
                melee_retarget:
                    action = 0;
                    slot->pending_action = 0;
                }
                int target = PickReachableSlotByDisposition(party_slot, relationship);
                if (target == -1) {
                    FormatDebugMessage(1, "ERROR: %ls is attacking friends with nobody in range",
                                       character->name);
                    slot->pending_action = -1;
                    action = -1;
                } else {
                    if (saved_flag_34 == '\0') {
                        PrepareCharacterAttacks(party_slot);
                    }
                    AimAtCharacter(party_slot, target, W8_TARGETING_CONTEXT_OUT_OF_COMBAT);
                }
            } else if (interrupt == 9) {
                goto melee_retarget;
            }
        }
    interrupt_done:
        if (CharacterCanSwitchTo(party_slot, W8_TARGETING_CONTEXT_OUT_OF_COMBAT, 0, 0) != 0) {
            FaceCharacterTowardCombatTarget(party_slot, &slot->target_out_of_combat);
        }
    }
    if (CharacterActionTargetsEnemies(character, action, detail,
                                      &slot->pending_action_detail_015) != 0 &&
        (gXStatus.fCombatMode == '\0' ||
         (interrupt == -1 && g_combat_state->characters[party_slot].flag_80 == '\0'))) {
        SetTargetSourceToCharacter(party_slot, &enemy_source);
        MakeTargetGroupHostile(&enemy_source, &slot->target_out_of_combat);
    }
    switch (action) {
    case -1:
        goto action_failed;
    case 0:
        result = StartCharacterAttack(party_slot, -1);
        break;
    case 1:
        result = StartCharacterAttack(party_slot, 3);
        break;
    case 2:
        result = CreateCharacterBreathEffect(party_slot);
        break;
    case 3:
        if (gXStatus.hostile_monster_count != 0 &&
            (TurnUndead(party_slot, &fatigue_cost, 1), fatigue_cost != -1)) {
            goto action_done;
        }
        goto action_failed;
    case 4:
        if (g_combat_state->characters[party_slot].flag_a4 == '\0') {
            goto action_done;
        }
        goto defend_notice;
    case 5:
        result = CanCharacterAttackItsTarget(party_slot);
        break;
    case 6:
        fatigue_cost = CharacterPrayAction00547FE0(party_slot);
        if (fatigue_cost == 0) {
            goto action_failed;
        }
        goto action_done;
    case 7: {
        unsigned int power = slot->pending_action_detail_015.spell.power_level;
        int step_cost;
        int step;
        fatigue_cost = 0;
        do {
            step = ExecuteCharacterSpellCast(party_slot, detail, power, &step_cost, 0);
            if (step == 0) {
                result = '\0';
            }
            fatigue_cost += step_cost;
        } while (step == 2 && power == 8 &&
                 SpellCastFatigueCost(detail, 1) + fatigue_cost <= character->stamina &&
                 g_spell_records[detail].spell_point_cost <=
                     character->iSPLeft[g_spell_records[detail].realm]);
        break;
    }
    case 8:
        if (UseItem(character, slot->pending_action_detail_015.item_use.item, &fatigue_cost) == 0) {
            goto action_failed;
        }
        goto action_done;
    case 9:
        goto action_done;
    default:
        FormatDebugMessage(1, "ERROR: Char %d executed %ls as a character action for char %d",
                           party_slot, gppStringList[g_action_kind_message_ids_61e988[action]]);
        goto action_done;
    }
    if (result == '\0') {
    action_failed:
        action = -1;
        if (interrupt == -1 && saved_flag_34 == '\0') {
            action = 4;
            slot->pending_action = 4;
        defend_notice:
            if (g_settings_6850c8.verbose_combat_messages != '\0') {
                PostCharacterNotice(party_slot, gppStringList[0x234]);
            }
        } else {
            slot->pending_action = -1;
        }
    }
action_done:
    if (g_combat_state != NULL) {
        if (action != 4 && action != 5) {
            g_combat_state->flag_a55 = 0;
        }
        if (CharacterActionTargetsEnemies(character, action, detail,
                                          &slot->pending_action_detail_015) != 0 &&
            slot->target_out_of_combat.iType >= W8_TARGET_KIND_MONSTER &&
            slot->target_out_of_combat.iType <= W8_TARGET_KIND_PLACE) {
            g_combat_state->combat_update_count = 0;
        }
    }
    if (action != 0 && action != 1) {
        if (fatigue_cost == -1) {
            fatigue_cost = CharacterActionFatigueCost(party_slot, action);
        }
        FatigueCharacter(party_slot, fatigue_cost, '\x01', NULL);
    }
    if (gXStatus.fCombatMode != '\0') {
        g_combat_state->eCombatActionStatus = 2;
    }
}

/* Derive the row's next action phase and settle which hand the slot swings
   with. Only an attack pending action that can still reach a live target
   earns a phase; anything else leaves the row idle for the round. When both
   hands can attack the off hand wins unless it is strictly worse on both the
   reach value and the damage bonus, and a switched hand charges the phase
   the difference in the hands' damage bonuses. */
// FUNCTION: WIZ8 0x004EAC90
void ComputeCharacterActionPhase004EAC90(int party_slot)
{
    W8CombatCharacterRow* row = &g_combat_state->characters[party_slot];
    int action = g_status_685170.buffers.XChar[party_slot].pending_action;
    if ((action == 0 || action == 1) && CanAnyHandReachTarget(party_slot) != 0 &&
        TargetIsInPlay(party_slot, 2, W8_TARGETING_CONTEXT_OUT_OF_COMBAT) != 0) {
        W8Character* character = &g_status_685170.buffers.Char[party_slot];
        int current = row->current_hand;
        int other = current == 0;
        unsigned int current_value = row->hand_attack_values_40[current];
        unsigned int other_value = row->hand_attack_values_40[other];
        int chosen = current;
        if (other_value != 0) {
            chosen = other;
            if (other_value < current_value &&
                character->Hand[other].damage_bonus < character->Hand[current].damage_bonus) {
                chosen = current;
            }
        }
        unsigned int attack_value = row->hand_attack_values_40[chosen];
        if (attack_value != 0) {
            unsigned int phase;
            if (chosen == current || other_value <= current_value) {
                phase =
                    row->phase + (row->phase_clock_stamp - g_combat_state->round_counter + 100) /
                                     (attack_value + 1);
            } else {
                phase = g_combat_state->round_counter;
            }
            row->phase = phase;
            if (phase < 0x65) {
                row->current_hand = chosen;
                if (chosen != current) {
                    row->phase = character->Hand[current].damage_bonus -
                                 character->Hand[chosen].damage_bonus + phase;
                }
                ClampUnsignedInteger(&row->phase, g_combat_state->round_counter, 100);
                RoundPhaseToStep(&row->phase, g_combat_state->round_counter);
                return;
            }
        }
    }
    row->phase = 0;
}

/* Run one monster's committed combat action - the monster-side counterpart
   of ExecuteCharacterAction004EA5C0. A condition interrupt can rewrite the
   action first (berserk forces a fresh random choice); the action-kind
   switch executes it and a failed action loops back through the AI for a
   repick until something commits or the monster holds. */
// FUNCTION: WIZ8 0x004EAE20
void ExecuteMonsterAction004EAE20(W8MonsterInfo* monster_info, W8MonsterRecord* record)
{
    char tried[11] = {0};
    unsigned char result = 0;
    unsigned char move_flag = 0;
    char berserked = 0;
    int interrupt;
    int sight;
    W8RangeCategory range;
    float approach_distance;
    float engage_distance;
    srVector3T<float> position;
    W8MonsterInfo* target_info;
    W8TargetSource source;
    W8TargetSource monster_source;

    if (gXStatus.fCombatMode == '\0') {
        srAssertFail("gXStatus.fCombatMode", "C:\\Projects\\Wizardry 8\\Local Code\\Combat.cpp",
                     0xe04, 0);
    }
    if (g_combat_state->uiCurrentPartyActionStatus == 1) {
        EndPartyMovementPhase();
    }
    PointCameraAtMonster(monster_info, '\0', '\x01');
    SetTargetSourceToMonster(monster_info, &source);
    interrupt = GetConditionInterrupt004EC1E0(&source);
    if (interrupt != -1) {
        ShowNoticef(9, g_format_s_space_s_00617584, GetMonsterName(monster_info, NULL, '\0'),
                    gppStringList[g_condition_notices_0061E570[interrupt + 0x74]]);
        switch (interrupt) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 10:
            monster_info->action_kind = -1;
            break;
        case 7:
            monster_info->action_kind = -1;
            ClearMonsterCondition(monster_info->location_id, 0xe);
            break;
        case 8:
            if (monster_info->pCombat->unknown_015 == '\0') {
                monster_info->pCombat->unknown_015 = 1;
                berserked = 1;
            }
            break;
        case 0xb:
            monster_info->action_kind = 9;
            break;
        case 0xc:
            monster_info->action_kind = 4;
            break;
        }
        if (monster_info->pCombat->unknown_015 != '\0') {
            monster_info->action_kind = 0;
            if (ChooseRandomMonsterAction(monster_info, 1, 0, berserked) == '\0') {
                if (interrupt == 8) {
                    FormatDebugMessage(
                        1, "ERROR: %ls (ID %d) is attacking friends with nobody in range",
                        record->name_00, monster_info->location_id);
                }
                monster_info->action_kind = -1;
            }
        }
    }
    while (true) {
        if (monster_info->action_kind != -1) {
            tried[monster_info->action_kind] = 1;
        }
        if (MonsterActionTargetsEnemies(monster_info->action_kind, monster_info->action_detail,
                                        &monster_info->spell_power_level) != 0 &&
            interrupt == -1 && monster_info->pCombat->unknown_015 == '\0') {
            SetTargetSourceToMonster(monster_info, &monster_source);
            MakeTargetGroupHostile(&monster_source, &monster_info->Target);
        }
        switch (monster_info->action_kind) {
        case 0:
            result = StartMonsterAttack0053FEA0(monster_info, record);
            break;
        case 1:
            goto action_committed;
        case 2: {
            int spell_id = monster_info->action_detail;
            unsigned int power = ChooseMonsterSpellPowerLevel(monster_info, record, spell_id);
            if (CanMonsterAimSpell(monster_info, spell_id) == 0 ||
                MonsterOKToCastSpell(monster_info, spell_id, power) == 0) {
                result = 0;
                goto action_retry;
            }
            monster_info->spell_power_level = power;
            OrientMonsterTowardTarget(monster_info, 0);
            monster_info->fSpellReleased = 0;
            StartMonsterCycle(monster_info, 0x19, 1);
            goto action_committed;
        }
        case 3:
            result = MonsterFleeAction(monster_info, record);
            break;
        case 4:
            SetUpMonsterTurn(monster_info);
            result = MonsterLinkToStartupNavigator004C6030(monster_info->monster) != 0;
            if (result != 0) {
                monster_info->pCombat->advancing_14b = 1;
                if (interrupt != 0xc) {
                    ShowNoticef(
                        9, gppStringList[0x23b], GetMonsterName(monster_info, NULL, '\0'),
                        gppStringList[g_gender_name_message_rows_61e430[record->name_group_0cc]
                                                                       [2]]);
                }
            }
            break;
        case 5:
        case 7:
            if (monster_info->condition_turns[0xc] == 0 || record->kind_0cb == '\f') {
                if (record->prefer_ranged_actions_1b9 == '\0') {
                    range = GetBestMonsterAttackRange(record, '\x01');
                    sight = 0;
                } else {
                    range = GetMonsterBestRangeCategory(monster_info, '\0', &sight);
                }
                if (range != W8_RANGE_NONE) {
                    unsigned short move_result;
                    if (range < W8_RANGE_LONG) {
                        approach_distance = CalcRangeDistance(W8_RANGE_TOUCH) * g_float_005ebccc;
                    } else {
                        approach_distance =
                            CalcRangeDistance(W8_RANGE_SHORT) * g_prepath_link_height_5ed300;
                    }
                    GetMonsterAttackSourceOffset(monster_info->monster, sight, &position);
                    engage_distance = CalcRangeDistance(range);
                    MonsterChooseTarget(monster_info, &monster_info->Target, 0);
                    SetUpMonsterTurn(monster_info);
                    if (monster_info->Target.iType == W8_TARGET_KIND_MONSTER) {
                        unsigned int index = MonsterGetIndexByLocationID(
                            0xec1, "C:\\Projects\\Wizardry 8\\Local Code\\Combat.cpp",
                            monster_info->Target.iMonsterID, '\x01');
                        target_info = MonsterGetScriptPartByLocationIndex(index);
                        move_result = MonsterConfigureMovementToMonster004C60D0(
                            monster_info->monster, target_info->monster, approach_distance,
                            engage_distance, position, sight, &move_flag);
                    } else {
                        if (monster_info->ubDisposition != '\x01') {
                            break;
                        }
                        move_result = MonsterConfigureMovementToPlayer004C6070(
                            monster_info->monster, approach_distance, engage_distance, position,
                            sight, &move_flag);
                    }
                    if (move_result == 1 || (move_result == 3 && move_flag != '\0')) {
                        if (monster_info->action_kind == 5) {
                            if (g_settings_6850c8.verbose_combat_messages != '\0') {
                                if (monster_info->Target.iType == W8_TARGET_KIND_MONSTER) {
                                    PostMonsterNotice(monster_info, gppStringList[0x238],
                                                      GetMonsterName(target_info, NULL, '\0'));
                                } else {
                                    PostMonsterNotice(monster_info, gppStringList[0x236]);
                                }
                            }
                        } else if (monster_info->Target.iType == W8_TARGET_KIND_MONSTER) {
                            PostMonsterNotice(monster_info, gppStringList[0x239],
                                              GetMonsterName(target_info, NULL, '\0'));
                        } else {
                            PostMonsterNotice(monster_info, gppStringList[0x237]);
                        }
                        goto action_committed;
                    }
                    if (monster_info->pCombat->unknown_151[1] != '\0') {
                        monster_info->pCombat->unknown_151[1] = '\0';
                        continue;
                    }
                    if (move_flag != '\0') {
                        break;
                    }
                }
                monster_info->action_kind = 6;
                continue;
            }
            break;
        case 6:
            if (monster_info->summoned_2da != 0 ||
                (monster_info->ubDisposition == '\x02' && gXStatus.hostile_monster_count == 0)) {
                ShowNoticef(9, gppStringList[0x23c], GetMonsterName(monster_info, NULL, '\0'));
                monster_info->action_kind = 1;
                continue;
            }
            SetUpMonsterTurn(monster_info);
            result = MonsterLinkToStartupNavigator004C6030(monster_info->monster) != 0;
            if (result != 0) {
                ShowNoticef(9, gppStringList[0x23a], GetMonsterName(monster_info, NULL, '\0'));
            }
            break;
        case 8:
            result = CanMonsterAttackItsTarget(monster_info);
            break;
        case 9:
            SetUpMonsterTurn(monster_info);
            result = IsMonsterControlPointInRange(monster_info) != 0;
            break;
        }
        if (result != '\0') {
        action_committed:
            if (monster_info->action_kind != 1 && monster_info->action_kind != 8) {
                MonsterSetNavigatorFlag25(monster_info->monster, '\0');
            }
            goto action_done;
        }
    action_retry:
        if (interrupt != -1 || monster_info->pCombat->active != '\0' ||
            monster_info->action_kind == 9) {
            monster_info->action_kind = -1;
            goto action_done;
        }
        UpdateMonsterAI(monster_info);
        if (monster_info->action_kind == -1 || tried[monster_info->action_kind] != '\0') {
            monster_info->action_kind = 1;
        }
        if (monster_info->action_kind == 1) {
            ShowNoticef(9, gppStringList[0x23c], GetMonsterName(monster_info, NULL, '\0'));
        action_done:
            if (monster_info->action_kind != 1 && monster_info->action_kind != 8) {
                g_combat_state->flag_a55 = 0;
            }
            if (IsMonsterActionUsable(monster_info) != '\0') {
                monster_info->pCombat->value_14c = 0;
            }
            monster_info->pCombat->active = '\x01';
            RequestRedraw(0x100000);
            g_combat_state->eCombatActionStatus = 2;
            return;
        }
    }
}

/* The monster flee action: check the monster can and will run, aim it, play
   its flee cycle unless the special-attack mode table keeps it facing the
   target, then report the attempt. */
// FUNCTION: WIZ8 0x004EB980
char MonsterFleeAction(W8MonsterInfo* monster_info, W8MonsterRecord* record)
{
    if (!CanMonsterFlee(monster_info, record, '\0')) {
        return 0;
    }
    if (AimFleeingMonster(monster_info, record) == 0) {
        return 0;
    }
    if (g_special_attack_table[record->special_attack_kind_0e3][0] != 6) {
        OrientMonsterTowardTarget(monster_info, 0);
        AimMonsterBreathAtTarget(monster_info);
    }
    monster_info->pCombat->unknown_145[0] = '\0';
    StartMonsterCycle(monster_info, 0x12, 1);
    if (g_settings_6850c8.verbose_combat_messages != '\0') {
        ShowNoticef(
            9, L"%s %s!", GetMonsterName(monster_info, NULL, '\0'),
            gppStringList
                [g_monster_special_attack_name_ids_61ec14[record->special_attack_kind_0e3]]);
        return 1;
    }
    ShowNoticef(
        9, g_format_s_space_s_00617584, GetMonsterName(monster_info, NULL, '\0'),
        gppStringList[g_monster_special_attack_name_ids_61ec14[record->special_attack_kind_0e3]]);
    return 1;
}

/* Aim the monster's mode-three particle axis at its combat target: the target
   monster's center, a visible group member, the aimed point, or the camera.
   Self targets and empty slots leave the axis unchanged. */
// FUNCTION: WIZ8 0x004ECB40
void AimMonsterBreathAtTarget(W8MonsterInfo* monster_info)
{
    srVector3T<float> position;
    int location_id;
    W8Monster* target_monster;
    bool no_target;

    switch (monster_info->Target.iType) {
    case W8_TARGET_KIND_NONE:
    case W8_TARGET_KIND_FIVE:
        return;
    case W8_TARGET_KIND_MONSTER:
        location_id = monster_info->Target.iMonsterID;
        no_target = monster_info->location_id == location_id;
        break;
    case W8_TARGET_KIND_GROUP:
        location_id =
            FindNearestVisibleGroupMonster(monster_info, monster_info->Target.iGroupID, 3);
        no_target = location_id == -1;
        break;
    case W8_TARGET_KIND_PLACE:
        monster_info->monster->m_axis_1c0 = monster_info->Target.point;
        monster_info->monster->unknown_1bf = 1;
        return;
    default:
        GetCameraPosition(&position);
        monster_info->monster->m_axis_1c0 = position;
        monster_info->monster->unknown_1bf = 1;
        return;
    }
    if (!no_target) {
        target_monster = GetMonsterByLocationID(location_id);
        monster_info->monster->m_axis_1c0.x = target_monster->movement_0c0.position_040.x;
        monster_info->monster->m_axis_1c0.y = target_monster->movement_0c0.position_040.y +
                                              target_monster->movement_0c0.height_offset_0b8;
        monster_info->monster->m_axis_1c0.z = target_monster->movement_0c0.position_040.z;
        monster_info->monster->unknown_1bf = 1;
    }
}

/* Run the monster's committed special attack: for a non-summoning kind, point
   the camera at the target, rebuild the spell's character and monster marker
   lists, fold in every monster hostile to the source, then resolve the group
   attack. Either way the record's cooldown lands in its combat state and the
   attack costs a tenth of its stamina. */
// FUNCTION: WIZ8 0x004EBA70
int ExecuteMonsterSpecialAttack(W8MonsterInfo* monster_info, W8MonsterRecord* record)
{
    W8TargetSource source;
    W8GrowableVector<int> char_targets;
    W8GrowableVector<int> monster_targets;

    if (monster_info->pCombat->unknown_145[0] == '\0') {
        return 2;
    }
    SetTargetSourceToMonster(monster_info, &source);
    if (g_special_attack_table[record->special_attack_kind_0e3][0] != 6) {
        monster_info->monster->unknown_1bf = '\0';
        PointCameraAtCombatTarget(&source, &monster_info->Target);
        PopulateSpellTargetMarkers(0x77, 0, &source, &monster_info->Target, &monster_targets,
                                   &char_targets, 0);
        ProvokeListedMonsterGroups(&source, &monster_targets);
    }
    ResolveMonsterGroupAttack005560A0(record->special_attack_kind_0e3, &source,
                                      &monster_info->Target, char_targets, monster_targets);
    monster_info->pCombat->unknown_145[1] = record->special_attack_cooldown_15c;
    FatigueMonster(monster_info, static_cast<unsigned int>(monster_info->stamina_max) / 10, NULL);
    return 3;
}

/* The character's breath action: refuse while the previous breath visual is
   still playing, hand it to the world updater, rebuild the spell marker lists
   from the committed target, fold in every hostile monster, resolve the group
   attack as kind 0x1e and tax a fifth of the stamina ceiling. */
// FUNCTION: WIZ8 0x004EBFE0
int ExecuteCharacterSpecialAttack(int party_slot)
{
    W8TargetSource source;
    W8GrowableVector<int> char_targets;
    W8GrowableVector<int> monster_targets;

    if (g_combat_state->breath_visual_a4c->finished == '\0') {
        return 2;
    }
    g_combat_state->breath_visual_a4c->auto_release = '\x01';
    SetTargetSourceToCharacter(party_slot, &source);
    PopulateSpellTargetMarkers(0x77, 0, &source,
                               &g_status_685170.buffers.XChar[party_slot].target_out_of_combat,
                               &monster_targets, &char_targets, 0);
    ProvokeListedMonsterGroups(&source, &monster_targets);
    ResolveMonsterGroupAttack005560A0(
        0x1e, &source, &g_status_685170.buffers.XChar[party_slot].target_out_of_combat,
        char_targets, monster_targets);
    FatigueCharacter(
        party_slot,
        static_cast<int>(
            static_cast<unsigned int>(g_status_685170.buffers.Char[party_slot].uiStaminaMax) / 5),
        '\0', NULL);
    return 3;
}

/* The character's breathe step: face the committed target, rebuild the spell's
   marker lists from it, then build the breath visual aimed down the
   camera-to-target line. Fails when no monster marker survives, when the
   visual cannot be created, and otherwise reports the breath name and leaves
   the visual held for the attack. */
// FUNCTION: WIZ8 0x004EBCE0
char CreateCharacterBreathEffect(int party_slot)
{
    W8TargetSource source;
    srMatrix3T<float> rotation;
    srVector3T<float> camera;
    srVector3T<float> direction;
    W8GrowableVector<int> char_targets;
    W8GrowableVector<int> monster_targets;
    srVector3T<float> aim;
    W8CombatSlot* target;
    double yaw;
    double pitch;

    SetTargetSourceToCharacter(party_slot, &source);
    target = &g_status_685170.buffers.XChar[party_slot].target_out_of_combat;
    PopulateSpellTargetMarkers(0x77, 0, &source, target, &monster_targets, &char_targets, 0);
    if (monster_targets.GetCount() == 0) {
        PostCharacterNotice(
            party_slot, FormatWideString(gppStringList[0x1b7], g_spell_records[0x13].display_name));
        return 0;
    }
    FaceCharacterTowardCombatTarget(party_slot, target);
    GetCameraPosition(&camera);
    aim.x = target->point.x - camera.x;
    aim.y = target->point.y - camera.y;
    aim.z = target->point.z - camera.z;
    direction = aim;
    aim.SetLength(1.0);
    rotation.SetIdentity();
    yaw = atan2(aim.x, aim.z);
    if (yaw != g_zero_005ebb40) {
        rotation.RotateAboutY(sin(yaw), cos(yaw));
    }
    aim = rotation.Transform(direction);
    pitch = -atan2(aim.y, aim.z);
    rotation.RotateAboutX(pitch);
    g_combat_state->breath_visual_a4c =
        CreateAimedSpellEffect(g_spell_records[0x77].resource_name, 1, &camera, &rotation, 0, 0);
    if (g_combat_state->breath_visual_a4c == 0) {
        return 0;
    }
    if (g_settings_6850c8.verbose_combat_messages != '\0') {
        PostCharacterNotice(party_slot, g_format_s_bang_006175c0,
                            gppStringList[g_breath_notice_id_0061ec8c]);
    } else {
        PostCharacterNotice(party_slot, g_format_s_dash_dash_006175b0,
                            gppStringList[g_breath_notice_id_0061ec8c]);
    }
    g_combat_state->breath_visual_a4c->auto_release = '\0';
    PointCameraAtCombatTarget(&source, target);
    return 1;
}

/* Step the acting monster's committed action: attack swings, the spell wait,
   the special attack, or the movement kinds - which may retarget, re-decide
   through the AI and bank the next phase. The outcome code lands in
   eCombatActionStatus while this monster is the action owner. */
// FUNCTION: WIZ8 0x004EB5A0
void StepMonsterCombatAction(W8MonsterInfo* monster_info)
{
    int outcome;
    /* MonsterChooseTarget fills the target kind and the chosen monster's id. */
    W8CombatSlot chosen;
    int progress_pct;
    W8MonsterRecord* record;
    W8MonsterCombatState* combat;
    W8Monster* monster;

    switch (monster_info->action_kind) {
    case 0:
        record = GetMonsterDataForInfo(monster_info);
        outcome = ContinueMonsterAttack(monster_info, record);
        break;
    case 3:
        record = GetMonsterDataForInfo(monster_info);
        outcome = ExecuteMonsterSpecialAttack(monster_info, record);
        break;
    case 2:
        if (monster_info->fSpellReleased == '\0') {
            srAssertFail("pMonsterInfo->fSpellReleased",
                         "C:\\Projects\\Wizardry 8\\Local Code\\Combat.cpp", 0xff2, 0);
        }
        outcome = 2;
        break;
    case 4:
    case 5:
    case 6:
    case 7:
    case 9:
        PointCameraAtMonster(monster_info, '\0', '\x01');
        if (monster_info->monster->movement_complete_026 == '\0') {
            outcome = 2;
            break;
        }
        if ((monster_info->action_kind == 5 || monster_info->action_kind == 7) &&
            monster_info->hp_current != 0 && monster_info->highest_condition < 0xe &&
            monster_info->condition_turns[W8_CONDITION_BLIND] == 0) {
            MonsterChooseTarget(monster_info, &chosen, 3);
            if (chosen.iType == 2) {
                MonsterForwardReferencePosition(monster_info->monster, '\0');
            } else if (chosen.iType == 3) {
                MonsterAimAtMonster004C62C0(monster_info->monster,
                                            GetMonsterByLocationID(chosen.iMonsterID), '\0');
            }
        }
        RefreshAllSight();
        monster = monster_info->monster;
        combat = monster_info->pCombat;
        if (static_cast<int>((monster->movement_0c0.callback_threshold_058 <= g_float_005ebb34
                                  ? monster->movement_0c0.callback_progress_05c /
                                        monster->movement_0c0.callback_threshold_058
                                  : g_float_005ebb34) *
                             g_octree_cell_scale_005ebcd0) < 100) {
            progress_pct =
                static_cast<int>((monster->movement_0c0.callback_threshold_058 <= g_float_005ebb34
                                      ? monster->movement_0c0.callback_progress_05c /
                                            monster->movement_0c0.callback_threshold_058
                                      : g_float_005ebb34) *
                                 g_octree_cell_scale_005ebcd0);
        } else {
            progress_pct = 100;
        }
        if (static_cast<int>(Random(100)) < 100 - progress_pct) {
            if (monster_info->action_kind != 9) {
                UpdateMonsterAI(monster_info);
            }
            outcome = monster_info->action_kind;
            if (outcome < 4 || (outcome > 7 && outcome != 9)) {
                combat->active = '\0';
                combat->phase = combat->phase + g_combat_state->round_counter;
                if (combat->phase > 100) {
                    combat->phase = 100;
                }
                RoundPhaseToStep(&combat->phase, g_combat_state->round_counter);
            }
        }
        outcome = 3;
        break;
    default:
        outcome = 3;
        break;
    }
    if (g_combat_state->pActionMonsterInfo == monster_info &&
        g_combat_state->eCombatActionStatus != outcome) {
        g_combat_state->eCombatActionStatus = outcome;
    }
}

/* Condition interrupt rolled before an actor's committed action runs: a
   nausea fit, a webbed actor's break-out check, blindness, insanity, fear,
   control and turncoat. Returns the interrupt category the action scheduler
   substitutes, -1 when the action proceeds. */
// FUNCTION: WIZ8 0x004EC1E0
int GetConditionInterrupt004EC1E0(W8TargetSource* source)
{
    int party_slot;
    W8Character* character;
    W8MonsterInfo* monster_info;
    unsigned int* condition_turns;
    unsigned char secondary_flag;
    unsigned int attribute;
    bool moving;
    bool controlled;
    bool can_attack;
    bool second_hand_attack;

    monster_info = 0;
    second_hand_attack = false;
    character = 0;
    can_attack = false;
    if (source->iType == W8_TARGET_SOURCE_CHARACTER) {
        if (source->iChar == -1) {
            srAssertFail("pSource->iChar != BAD_INDEX",
                         "C:\\Projects\\Wizardry 8\\Local Code\\Combat.cpp", 0x1193, 0);
        }
        party_slot = source->iChar;
        character = &g_status_685170.buffers.Char[party_slot];
        condition_turns = character->uiCondition;
        moving = g_status_685170.buffers.XChar[party_slot].pending_action == W8_ACTION_RUN;
        if (CanAnyHandReachTarget(party_slot)) {
            can_attack = CanPartySlotAttackAnyTarget(party_slot, 8, 1, '\0') != 0;
            second_hand_attack = CanPartySlotAttackAnyTarget(party_slot, 8, 1, '\x01') != 0;
        }
        secondary_flag = g_combat_state->characters[party_slot].flag_80;
        attribute = character->attributes[0].effective;
        controlled = false;
    } else {
        if (source->iType != W8_TARGET_SOURCE_MONSTER) {
            srAssertFail("pSource->iType == SOURCE_TYPE_MONSTER",
                         "C:\\Projects\\Wizardry 8\\Local Code\\Combat.cpp", 0x11a8, 0);
        }
        if (source->iMonsterID == -1) {
            srAssertFail("pSource->iMonsterID != BAD_INDEX",
                         "C:\\Projects\\Wizardry 8\\Local Code\\Combat.cpp", 0x11a9, 0);
        }
        monster_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0x11aa, "C:\\Projects\\Wizardry 8\\Local Code\\Combat.cpp",
                                        source->iMonsterID, '\x01'));
        moving = monster_info->action_kind == 4;
        condition_turns = monster_info->condition_turns;
        can_attack =
            RateMonsterBestAttack(monster_info, GetMonsterDataForInfo(monster_info), 0) == 0;
        secondary_flag = monster_info->pCombat->unknown_015;
        attribute = monster_info->attributes[0];
        second_hand_attack =
            RateMonsterBestAttack(monster_info, GetMonsterDataForInfo(monster_info), 1) == 0;
        controlled = monster_info->control_state == 1;
    }
    if (condition_turns[W8_CONDITION_NAUSEATED] != 0 && Random(100) < 0x19) {
        return 3;
    }
    if (condition_turns[W8_CONDITION_WEBBED] != 0) {
        return (Random(100) < attribute / 3) ? 7 : 0;
    }
    if (condition_turns[W8_CONDITION_BLIND] != 0 && Random(100) < 0x14) {
        if (source->iType == W8_TARGET_SOURCE_CHARACTER &&
            CharacterHasTrait00547940(character, 7) == 0) {
            return 2;
        }
        if (source->iType == W8_TARGET_SOURCE_MONSTER &&
            GetMonsterDataForInfo(monster_info)->kind_0cb != '\x0c') {
            return 2;
        }
    }
    if (condition_turns[W8_CONDITION_INSANE] != 0 && Random(100) < 0x50) {
        if (second_hand_attack && secondary_flag == '\0' && Random(100) < 0x28) {
            return 8;
        }
        return Random(3) + 4;
    }
    if (condition_turns[W8_CONDITION_AFRAID] != 0 && Random(100) < 0x19 && !moving) {
        return 1;
    }
    if (source->iType == W8_TARGET_SOURCE_MONSTER && condition_turns[W8_CONDITION_AFRAID] != 0 &&
        Random(100) < 0x28) {
        return 0xc;
    }
    if (controlled) {
        return 0xb;
    }
    if (condition_turns[W8_CONDITION_TURNCOAT] == 0) {
        return -1;
    }
    if (source->iType == W8_TARGET_SOURCE_CHARACTER) {
        if (!can_attack) {
            if (AreAllHandSlotsEmpty(&g_status_685170.buffers.Char[party_slot]) == 0 &&
                CanUnequipSlotItem(character, 6) != 0 && CanUnequipSlotItem(character, 7) != 0) {
                SwapWeaponSetSlots0051D3B0(party_slot, '\0', '\x01');
                if (CanAnyHandReachTarget(party_slot) != 0 &&
                    CanPartySlotAttackAnyTarget(party_slot, 8, 1, '\0') != '\0') {
                    PostCharacterNotice(party_slot, gppStringList[0x23d]);
                    return 9;
                }
                SwapWeaponSetSlots0051D3B0(party_slot, '\0', '\x01');
            }
            return 10;
        }
    } else if (MonsterActionTargetsEnemies(monster_info->action_kind, monster_info->action_detail,
                                           &monster_info->spell_power_level) == '\0') {
        return -1;
    }
    return 9;
}

/* Turn the camera onto the acting combat slot's target so the player sees
   what is about to happen. Rotation mode 2 suppresses it entirely; mode 0
   only turns for the selected character's own actions. Monster and group
   targets resolve through the monster list; a place target aims the camera
   at the point. */
// FUNCTION: WIZ8 0x004ECC80
void PointCameraAtCombatTarget(W8TargetSource* source, W8CombatSlot* target)
{
    int location_id;
    int caller_line;
    W8MonsterInfo* monster_info;
    W8MonsterGroup* group;
    srVector3T<float> position;

    if (g_settings_6850c8.camera_rotation_mode == 2) {
        return;
    }
    if (IsTargetStillPresent(target) == 0) {
        return;
    }
    if (g_settings_6850c8.camera_rotation_mode == 0 &&
        (TargetSourceIsCharacter(source, 0) == 0 ||
         source->iChar != g_status_685170.selected_character)) {
        if (target->iType != W8_TARGET_KIND_CHARACTER) {
            return;
        }
        if (target->iChar != g_status_685170.selected_character) {
            return;
        }
    }
    if (target->iType == W8_TARGET_KIND_MONSTER) {
        if (target->iMonsterID == -1) {
            srAssertFail("pTarget->iMonsterID != -1",
                         "C:\\Projects\\Wizardry 8\\Local Code\\Combat.cpp", 0x13fe, 0);
        }
        location_id = target->iMonsterID;
        caller_line = 0x1400;
    } else if (target->iType == W8_TARGET_KIND_GROUP) {
        if (target->iGroupID == -1) {
            srAssertFail("pTarget->iGroupID != -1",
                         "C:\\Projects\\Wizardry 8\\Local Code\\Combat.cpp", 0x140c, 0);
        }
        group = GetMonsterGroupByListIndex(GetMonsterGroupIndexByID(
            0x140d, "C:\\Projects\\Wizardry 8\\Local Code\\Combat.cpp", target->iGroupID, '\x01'));
        location_id = group->value_9f;
        caller_line = 0x140d;
    } else {
        if (target->iType != W8_TARGET_KIND_PLACE) {
            return;
        }
        position = target->point;
        PointCameraAtTarget(&position, '\0', '\0');
        return;
    }
    monster_info = MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
        caller_line, "C:\\Projects\\Wizardry 8\\Local Code\\Combat.cpp", location_id, '\x01'));
    PointCameraAtMonster(monster_info, '\0', '\x01');
}

// FUNCTION: WIZ8 0x004E8EA0
void UpdateCombat004E8EA0(void)
{
    if (gXStatus.world_update_blocked != 0) {
        return;
    }
    if (gXStatus.fSurprisePossible != 0) {
        return;
    }
    if (g_combat_state->combat_evaluated_a48 == 0 && gXStatus.hostile_monster_count != 0) {
        EvaluateCombatDifficulty004E6CE0();
        g_combat_state->combat_evaluated_a48 = 1;
    }
    if (g_combat_state->engaged_missile != 0 &&
        g_combat_state->engaged_missile->BlocksEndingCombat004A5790() == 0) {
        g_combat_state->engaged_missile->flag_1e2 = 1;
        g_combat_state->engaged_missile = 0;
    }
    if (g_combat_state->uiCurrentPartyActionStatus == 1) {
        if (g_combat_state->uiCurrentPartyAction == 1 ||
            g_combat_state->uiCurrentPartyAction == 2) {
            UpdateActivePartyMovement();
        }
    } else {
        AutoAdvanceSelectedCharacter004E8DA0();
        if (IsCameraTransitionActive00420E10() != 0) {
            return;
        }
        if (AnyCombatMonsterBusy004E8C30() != 0) {
            return;
        }
        if (g_combat_state->hit_sound_active_7c0 != 0) {
            if (SoundIsPlaying(g_combat_state->hit_sound_7bc) != 0) {
                return;
            }
            g_combat_state->hit_sound_active_7c0 = 0;
        }
        if (AllSpellEffectsStillRunning() == 0) {
            return;
        }
        W8Missile* missile = NextMissile004A2760('\x01');
        while (missile != 0) {
            if ((missile == g_combat_state->engaged_missile ||
                 g_missile_table_65bde0[missile->missile_table_index_1d8].flag_154 != 0) &&
                missile->BlocksEndingCombat004A5790() != 0) {
                return;
            }
            missile = NextMissile004A2760('\0');
        }
        if (gXStatus.fCombatMode != 0) {
            for (unsigned int slot = 0; slot < 2; ++slot) {
                if (g_status_685170.buffers.XChar[slot].fOccupied != 0 &&
                    g_status_685170.buffers.Char[slot].hp_current != 0 &&
                    g_status_685170.buffers.Char[slot].highest_condition < 0xf &&
                    g_combat_state->npc_combat_script_pending[slot] != 0) {
                    return;
                }
            }
        }
    }
    for (unsigned int monster_index = 0; monster_index < PLLength(gXStatus.plsMonsterList);
         ++monster_index) {
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
        if (monster_info->fActive != 0 && monster_info->fInCombat != 0 &&
            monster_info->hp_current != 0 && monster_info->pCombat->unknown_151[0] != 0) {
            RefreshMonsterSight(monster_info);
            monster_info->pCombat->unknown_151[0] = 0;
        }
    }
    if (g_combat_state->flag_000 == 0) {
        if (QueueNpcCombatScript()) {
            return;
        }
        if (CheckCombatEnd004E9F90(g_combat_state->flag_000 == 0 &&
                                   g_combat_state->flag_a54 != 0) != 0) {
            return;
        }
        if ((g_settings_6850c8.continuous_combat == 0 ||
             ClockIsTicking(g_combat_state->combat_ui_timer_7a8) != 0 ||
             CombatMayAdvanceContinuously() == 0) &&
            g_combat_state->party_surprised_a52 == 0) {
            return;
        }
        BeginCombatExecution004E8370();
        return;
    }
    switch (g_combat_state->eCombatActionStatus) {
    case 0:
        if (g_combat_state->iActionChar != -1) {
            srAssertFail("gpCombat->iActionChar == BAD_INDEX",
                         "C:\\Projects\\Wizardry 8\\Local Code\\Combat.cpp", 0x7e1, 0);
        }
        if (g_combat_state->pActionMonsterInfo != 0) {
            srAssertFail("gpCombat->pActionMonsterInfo == NULL",
                         "C:\\Projects\\Wizardry 8\\Local Code\\Combat.cpp", 0x7e2, 0);
        }
        if (!QueueNpcCombatScript()) {
            ScheduleCombatActor004E9490();
            return;
        }
        break;
    case 1:
        if (ClockIsTicking(g_combat_state->action_clock_7ac) == 0) {
            if (g_settings_6850c8.autoscroll_combat_messages != 0) {
                g_combat_state->notice_scroll_pending_a57 = 1;
            }
            if (g_combat_state->iActionChar != -1) {
                ExecuteCharacterAction004EA5C0(g_combat_state->iActionChar);
                return;
            }
            if (g_combat_state->pActionMonsterInfo != 0) {
                ExecuteMonsterAction004EAE20(
                    g_combat_state->pActionMonsterInfo,
                    GetMonsterDataForInfo(g_combat_state->pActionMonsterInfo));
                return;
            }
            srAssertFail("FALSE", "C:\\Projects\\Wizardry 8\\Local Code\\Combat.cpp", 0x805, 0);
            return;
        }
        break;
    case 2: {
        int slot = g_combat_state->iActionChar;
        if (slot == -1) {
            if (g_combat_state->pActionMonsterInfo != 0) {
                StepMonsterCombatAction(g_combat_state->pActionMonsterInfo);
                return;
            }
            srAssertFail("FALSE", "C:\\Projects\\Wizardry 8\\Local Code\\Combat.cpp", 0x816, 0);
            return;
        }
        int pending_action = g_status_685170.buffers.XChar[slot].pending_action;
        int result;
        if (pending_action < 0) {
            result = 3;
        } else if (pending_action < 2) {
            result = ResolveCharacterAttack0053E250(slot);
        } else if (pending_action == 2) {
            result = ExecuteCharacterSpecialAttack(slot);
        } else {
            result = 3;
        }
        if (g_combat_state->iActionChar == slot && g_combat_state->eCombatActionStatus != result) {
            g_combat_state->eCombatActionStatus = result;
            return;
        }
        break;
    }
    case 3: {
        int slot = g_combat_state->iActionChar;
        if (slot != -1) {
            if (g_combat_state->characters[slot].action_changed_a7 == 0) {
                W8PartySlotRow* row = &g_status_685170.buffers.XChar[slot];
                switch (row->action_03d) {
                case 0:
                case 1:
                case 4:
                case 5:
                    break;
                default:
                    if (GetCurrentTargetingContext(slot) != W8_TARGETING_CONTEXT_SHARED) {
                        ClearTargetHighlights(slot, &row->target_in_combat);
                    }
                    ResetCombatSlot(&row->target_in_combat);
                    RequestPartySlotRedraw(slot);
                    if (slot == g_status_685170.selected_character) {
                        RequestRedrawParty();
                    }
                    SetCharacterCombatAction(slot, row->action_kind, row->action_detail, 0, 0);
                }
            }
            ComputeCharacterActionPhase004EAC90(slot);
            g_combat_state->eCombatActionStatus = 0;
            g_combat_state->iActionChar = -1;
            return;
        }
        W8MonsterInfo* monster_info = g_combat_state->pActionMonsterInfo;
        if (monster_info == 0) {
            srAssertFail("FALSE", "C:\\Projects\\Wizardry 8\\Local Code\\Combat.cpp", 0x826, 0);
            return;
        }
        GetMonsterDataForInfo(monster_info);
        FatigueMonster(monster_info, MonsterActionFatigueCost(monster_info), 0);
        MonsterSetNavigatorFlag25(monster_info->monster, '\x01');
        g_combat_state->eCombatActionStatus = 0;
        g_combat_state->pActionMonsterInfo = 0;
        W8MonsterCombatState* combat = monster_info->pCombat;
        if (combat->active != 0) {
            if (monster_info->action_kind == 0 && combat->attacks_per_round != 0 &&
                ChooseRandomMonsterAction(monster_info, 1, 0, '\0') != 0) {
                combat->phase =
                    combat->phase + (100U - g_combat_state->round_counter) /
                                        static_cast<unsigned int>(combat->attacks_per_round + 1);
                if (combat->phase <= 0x64) {
                    RoundPhaseToStep(&combat->phase, g_combat_state->round_counter);
                    return;
                }
            }
            combat->phase = 0;
            return;
        }
        break;
    }
    }
}

// FUNCTION: WIZ8 0x004E9490
void ScheduleCombatActor004E9490(void)
{
    if (g_combat_state->eCombatActionStatus != 0) {
        srAssertFail("gpCombat->eCombatActionStatus == COMBAT_ACTION_NONE",
                     "C:\\Projects\\Wizardry 8\\Local Code\\Combat.cpp", 0x84f, 0);
    }
    bool free_turn = false;
    for (;;) {
        if (g_combat_state->uiCurrentPartyAction == 0 ||
            g_combat_state->uiCurrentPartyActionStatus == 3) {
            for (int slot = 0; slot < 8; ++slot) {
                W8PartySlotRow* row = &g_status_685170.buffers.XChar[slot];
                if (row->fOccupied == 0) {
                    continue;
                }
                W8CombatCharacterRow* combat_row = &g_combat_state->characters[slot];
                if (combat_row->phase != g_combat_state->round_counter) {
                    continue;
                }
                W8Character* character = &g_status_685170.buffers.Char[slot];
                if (character->hp_current != 0 && character->highest_condition < 0xf) {
                    g_combat_state->eCombatActionStatus = 1;
                    g_combat_state->iActionChar = slot;
                    break;
                }
                row->pending_action = -1;
                combat_row->phase = 0;
                if (combat_row->flag_34 == 0) {
                    combat_row->flag_34 = 1;
                    RequestRedraw((1 << slot) | 0x100000);
                }
            }
        }
        if (g_combat_state->eCombatActionStatus == 0) {
            for (unsigned int monster_index = 0; monster_index < PLLength(gXStatus.plsMonsterList);
                 ++monster_index) {
                W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
                if (monster_info->fInCombat == 0) {
                    continue;
                }
                W8MonsterCombatState* combat = monster_info->pCombat;
                if (combat->phase != g_combat_state->round_counter) {
                    continue;
                }
                if (monster_info->hp_current != 0 && monster_info->highest_condition < 0xf &&
                    monster_info->fMotionless == 0) {
                    g_combat_state->eCombatActionStatus = 1;
                    g_combat_state->pActionMonsterInfo = monster_info;
                    break;
                }
                monster_info->action_kind = -1;
                combat->phase = 0;
                if (combat->active == 0) {
                    combat->active = 1;
                    RequestRedraw(0x100000);
                }
            }
        }
        if (g_combat_state->eCombatActionStatus == 0 && g_combat_state->uiCurrentPartyAction != 0 &&
            g_combat_state->uiCurrentPartyActionStatus == 0 &&
            g_combat_state->uiPartyActionPhase == g_combat_state->round_counter) {
            int slot = 0;
            for (; slot < 8; ++slot) {
                if (IsPartySlotEligible00524A10(slot) != 0) {
                    break;
                }
            }
            if (slot < 8) {
                StartPartyMovementAction004EFC00();
            } else {
                ShowNotice(0xc, gppStringList[0x22a], -1, -1, 0);
                BeginFreeTurnPhase();
            }
            free_turn = true;
        }
        if (g_combat_state->eCombatActionStatus != 0 || free_turn != 0) {
            bool interruptible = false;
            bool apply_delay = false;
            if (g_combat_state->eCombatActionStatus == 0) {
                apply_delay = true;
            } else if (g_combat_state->pActionMonsterInfo == 0) {
                int slot = g_combat_state->iActionChar;
                if (slot == -1) {
                    apply_delay = true;
                } else {
                    W8TargetingContext context;
                    int action;
                    if (g_combat_state->characters[slot].flag_34 == 0) {
                        action = g_status_685170.buffers.XChar[slot].action_03d;
                        context = W8_TARGETING_CONTEXT_IN_COMBAT;
                    } else {
                        action = g_status_685170.buffers.XChar[slot].pending_action;
                        context = W8_TARGETING_CONTEXT_OUT_OF_COMBAT;
                    }
                    if ((action == W8_ACTION_DEFEND || action == W8_ACTION_PROTECT ||
                         action == -1) &&
                        (g_settings_6850c8.continuous_combat == 0 ||
                         CharacterCanSwitchTo(slot, context, 0, 0) != 0)) {
                        interruptible = true;
                    }
                    if ((g_settings_6850c8.continuous_combat != 0 ||
                         CharacterCanSwitchTo(slot, context, 0, 0) != 0) &&
                        interruptible == 0) {
                        apply_delay = true;
                    }
                }
            } else {
                int action_kind = g_combat_state->pActionMonsterInfo->action_kind;
                if (action_kind == 1 || action_kind == 8 || action_kind == -1) {
                    interruptible = true;
                }
                if (g_combat_state->unknown_a60 == 0 && interruptible == 0) {
                    apply_delay = true;
                }
            }
            if (apply_delay != 0 &&
                (gXStatus.hostile_monster_count != 0 || gXStatus.hostile_group_count != 0) &&
                GetLevelDataFlag6() != 0) {
                unsigned int delay = g_settings_6850c8.combat_delay_ms;
                if (g_combat_state->unknown_a60 != 0 &&
                    (g_settings_6850c8.continuous_combat == 0 || free_turn == 0) && delay > 0x31f) {
                    delay = 0x320;
                }
                g_combat_state->action_clock_7ac = SetCountdownClock(delay);
            }
            g_combat_state->unknown_a60 = 0;
        } else {
            if (g_combat_state->uiCurrentPartyActionStatus == 2) {
                int slot = 0;
                for (; slot < 8; ++slot) {
                    if (IsPartySlotEligible00524A10(slot) != 0) {
                        break;
                    }
                }
                if (slot < 8) {
                    FinishPartyMovementAction004EFDA0();
                } else {
                    ShowNotice(0xc, gppStringList[0x22a], -1, -1, 0);
                    BeginFreeTurnPhase();
                }
            }
            if (gXStatus.fPartyMovementMode == 0 || PartyMovementReachedPhaseLimit004F00C0() != 0) {
                g_combat_state->round_counter += GetPhaseStep();
            }
        }
        if (g_combat_state->eCombatActionStatus != 0 || gXStatus.fPartyMovementMode != 0 ||
            free_turn != 0 || g_combat_state->round_counter > 0x64) {
            if (g_combat_state->round_counter > 0x64) {
                g_combat_state->flag_a50 = 0;
                g_combat_state->flag_a51 = 0;
                if (gXStatus.fCombatMode != 0) {
                    for (unsigned int slot = 0; slot < 8; ++slot) {
                        if (g_status_685170.buffers.XChar[slot].fOccupied != 0 &&
                            g_status_685170.buffers.Char[slot].hp_current != 0 &&
                            g_status_685170.buffers.Char[slot].highest_condition < 0xd &&
                            g_status_685170.buffers.XChar[slot].pending_action == W8_ACTION_EQUIP) {
                            ++g_combat_state->flag_a51;
                            if (g_combat_state->flag_a50 == 0) {
                                g_combat_state->flag_a50 = 1;
                                OpenCharacterScreenForPartySlot(slot, 0);
                            }
                        }
                    }
                }
                if (g_combat_state->flag_a50 == 0) {
                    AdvanceCombatRound004E9B20();
                    if (CheckCombatEnd004E9F90(1) == 0 &&
                        g_settings_6850c8.continuous_combat != 0) {
                        BeginCombatExecution004E8370();
                    }
                }
            }
            return;
        }
    }
}

// FUNCTION: WIZ8 0x004EC610
short GetCombatActionProgress004EC610(int* out_total)
{
    int total = 0;
    int completed = 0;
    if (gXStatus.fCombatMode == 0) {
        srAssertFail("gXStatus.fCombatMode", "C:\\Projects\\Wizardry 8\\Local Code\\Combat.cpp",
                     0x1244, 0);
    }
    if (g_combat_state->uiCurrentPartyAction == 0 ||
        g_combat_state->uiCurrentPartyActionStatus == 3) {
        for (int party_slot = 0; party_slot < 8; ++party_slot) {
            W8Character* character = &g_status_685170.buffers.Char[party_slot];
            if (g_status_685170.buffers.XChar[party_slot].fOccupied != 0 &&
                character->hp_current != 0 && character->highest_condition < 0xf &&
                TryCharacterAction(party_slot, W8_ACTION_DEFEND, 0) == 0 &&
                TryCharacterAction(party_slot, W8_ACTION_PROTECT, 0) == 0 &&
                TryCharacterAction(party_slot, -1, 0) == 0) {
                total += GetCharacterTurnValue(party_slot);
                W8CombatCharacterRow* row = &g_combat_state->characters[party_slot];
                if (row->flag_34 != 0) {
                    int swings = 0;
                    int kind;
                    ChooseCombatAction(party_slot, 0, &kind, 0, 0, 0);
                    if (kind == W8_ACTION_ATTACK || kind == W8_ACTION_BERSERK) {
                        for (int hand = 0; hand < 2; ++hand) {
                            swings +=
                                row->saved_attack_value[hand] - row->hand_attack_values_40[hand];
                        }
                    } else {
                        swings = 1;
                    }
                    completed += swings;
                }
            }
        }
    } else {
        total = 1;
        if (0 < g_combat_state->uiCurrentPartyActionStatus) {
            completed = 1;
        }
    }
    for (unsigned int index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(index);
        if (monster_info->fInCombat != 0 && monster_info->hp_current != 0 &&
            monster_info->highest_condition < 0xf && monster_info->fMotionless == 0 &&
            monster_info->action_kind != 1 && monster_info->action_kind != 8 &&
            monster_info->action_kind != -1) {
            total += monster_info->action_kind == 0 ? monster_info->pCombat->unknown_005 : 1;
            if (monster_info->pCombat->active != 0) {
                completed += monster_info->action_kind == 0
                                 ? monster_info->pCombat->unknown_005 -
                                       monster_info->pCombat->attacks_per_round
                                 : 1;
            }
        }
    }
    short percent =
        total == 0 ? 100 : static_cast<short>((completed * 100) / static_cast<unsigned int>(total));
    if (out_total != 0) {
        *out_total = total;
    }
    return percent;
}
