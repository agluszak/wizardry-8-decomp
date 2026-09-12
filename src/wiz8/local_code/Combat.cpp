#include "wiz8/local_screens/Screens.h"
#include "wiz8/character.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/targeting.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/notices.h"
#include "wiz8/xstatus.h"
#include "wiz8/combat_state.h"
#include "wiz8/npc_interaction.h"
#include "wiz8/magic.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "random.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/local_code/CombatPartyMovement.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/FormationAndFacing.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/LoadSaveGame.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/engine_code/Cursor3d.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/local_screens/MGSPortraits.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_screens/MGSPartyMovement.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/screen_state.h"
#include "timer.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/float_constants.h"

#include <stdarg.h>
#include <stdio.h>

// GLOBAL: WIZ8 0x006836a8
W8CombatState* g_combat_state;
// GLOBAL: WIZ8 0x006850b0
unsigned int g_combat_countdown_6850b0;

/*
 * Local Code\Combat.cpp.
 *
 * The round bookkeeping: who is engaged, what each slot has chosen to do, and
 * the running list of characters who died this round.
 */

/* The action the round reset lifts, and the action the cached flag beside it
   watches for. */
enum { W8_ACTION_LIFTED_AT_ROUND_END = 9, W8_ACTION_KIND_ONE = 1 };

/* 0x00524A10 */

/* 0x00547940 */
/* The per-character combat rows live at +0x18 of the combat state and run
   0xd4 bytes apart; the state's leading 0x18 bytes are its own header. */
// GLOBAL: WIZ8 0x0068d810
unsigned char g_combat_log_enabled_0068d810;
// GLOBAL: WIZ8 0x00617664
extern const wchar_t g_combat_log_format_00617664[] = L"%hs";
/* 0x0053AC30 */

/* Whether anybody in the party is engaged with something. */
// FUNCTION: WIZ8 0x004e7ca0
unsigned char AnyCharacterEngaged(void)
{
    unsigned int party_slot;

    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (IsPartySlotEligible00524A10(party_slot)) {
            return 1;
        }
    }
    return 0;
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
            if (g_status_685170.buffers.party_rows[party_slot].occupied != 0 &&
                g_status_685170.buffers.characters[party_slot].hp_current != 0 &&
                g_status_685170.buffers.characters[party_slot].highest_condition < 0xf &&
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
    W8PartySlotRow* row = &g_status_685170.buffers.party_rows[party_slot];

    row->action_kind = action_kind;
    row->action_detail = action_detail;
    row->action_is_kind_one = action_kind == W8_ACTION_KIND_ONE;
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
        WriteGameLog(7, g_combat_log_format_00617664, line);
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
        if (g_status_685170.buffers.party_rows[party_slot].pending_action ==
            W8_ACTION_LIFTED_AT_ROUND_END) {
            g_status_685170.buffers.party_rows[party_slot].pending_action = -1;
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
unsigned char CanCharReBreathe(int party_slot)
{
    W8Character* character = &g_status_685170.buffers.characters[party_slot];

    if (!CharacterHasCondition(character, 0x1c)) {
        return 0;
    }
    return character->stamina_max / 5 <= character->stamina;
}

/* Take one character out of the round: hand back whatever they were aiming at,
   clear the slot, and re-choose an action of the same kind. */
// FUNCTION: WIZ8 0x004e82e0
void DropCharacterFromRound(int party_slot)
{
    W8PartySlotRow* row = &g_status_685170.buffers.party_rows[party_slot];

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
            (unsigned int)monster_info->highest_condition < 0xe &&
            monster_info->condition_turns[12] == 0 && monster_info->flag_16 == 1) {
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
    while (g_status_685170.buffers.party_rows[party_slot].occupied == 0 ||
           g_status_685170.buffers.characters[party_slot].hp_current == 0 ||
           g_status_685170.buffers.characters[party_slot].highest_condition > 10) {
        ++party_slot;
        if (party_slot > 7) {
            return 1;
        }
    }
    Random(1);
    if (g_status_685170.buffers.characters[party_slot].attributes[6].effective >> 1 <=
        Random(100)) {
        return 1;
    }
    return 0;
}

/* 0x004C62C0 */
extern int g_effect_005ee610;
extern unsigned int g_flee_hp_fraction_005ed8f8;
// GLOBAL: WIZ8 0x005ed908
unsigned int g_flee_chance_005ed908 = 15;
// GLOBAL: WIZ8 0x005ed490
float g_movement_speed_step_005ed490 = 0.009999999776482582f;
/* 0x00683FE7-adjacent: the per-character per-hand attack values combat saved
   when the round began, 0x35 dwords per character. */
// GLOBAL
int g_saved_attack_values[8 * 0x35];

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
            value = g_saved_attack_values[party_slot * 0x35 + hand];
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
    character = &g_status_685170.buffers.characters[target->iChar];
    if ((character->hp_current * 100) / (unsigned int)character->hp_max >=
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
    int chosen[2];
    int target_location;

    monster_info->action_kind = -1;
    monster_info->action_detail = 0;
    monster_info->pCombat->phase = 0;
    monster_info->pCombat->active = 1;
    monster_info->pCombat->value_14c = 0;
    RequestRedraw(0x100000);

    if (monster_info->hp_current != 0 && (unsigned int)monster_info->highest_condition < 0xe &&
        monster_info->condition_turns[12] == 0) {
        MonsterChooseTarget(monster_info, chosen, 3);
        if (chosen[0] == 2) {
            NotifyMonsterIdle(monster_info->monster, 0);
            monster_info->flag_253 = 0;
            return;
        }
        if (chosen[0] == 3) {
            target_location = chosen[1];
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

    scale = GetMonsterRecordScaledFloat1BA(monster_info);
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

    SetMonsterTurnSpeed(speed * scale * g_movement_speed_step_005ed490);
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
        if (Function5323F0(monster_info, 1, 0, 0)) {
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
    W8Character* character = &g_status_685170.buffers.characters[party_slot];

    if (character->hp_current == 0 || character->highest_condition >= 0xf) {
        return 0;
    }
    if (g_combat_state->characters[party_slot].flag_34 != 0) {
        return g_status_685170.buffers.party_rows[party_slot].pending_action == action;
    }
    if (g_status_685170.buffers.party_rows[party_slot].action_03d != action) {
        if (action != 4) {
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
    if (action != 4 && action != 5) {
        srAssertFail("(iCharAction == CHAR_ACTION_DEFEND) || (iCharAction == CHAR_ACTION_PROTECT)",
                     "C:\\Projects\\Wizardry 8\\Local Code\\Combat.cpp", 0x1686, 0);
    }
    W8PartySlotRow* row = &g_status_685170.buffers.party_rows[party_slot];

    row->pending_action = action;
    row->attack_mode[0] = -1;
    row->attack_mode[1] = -1;
    if (action == 5) {
        row->target_out_of_combat = row->target_in_combat;
    }
    g_combat_state->characters[party_slot].flag_34 = 1;
    RequestRedraw(1 << party_slot | 0x100000);
    if (action == 4 && row->action_03d != action) {
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
    Function53AE00();
    SetTargetingMode(0);
    RemoveConditionFromEveryone(5);
    RemoveConditionFromParty(0xd);
    Function524540();
    Function552530();
    ProcessMonstersAtCombatEnd(mode);
    unsigned int group_count = PLLength(gXStatus.plsMonsterGroupList);
    for (unsigned int group_index = 0; group_index < group_count; ++group_index) {
        W8MonsterGroup* group = GetMonsterGroupByListIndex(group_index);
        if (group->flag_29 != 0) {
            MonsterGroupLeaveCombat(group);
        }
        group_count = PLLength(gXStatus.plsMonsterGroupList);
    }
    if (g_combat_state->flag_a54 != 0) {
        const wchar_t* message = reinterpret_cast<const wchar_t*>(
            g_string_table[0x233]); /* reinterpret-ok: heterogeneous string table */
        ShowNotice(0xc, message, 1, -1, 0);
    }
    unsigned int active = CountActiveCharacters();
    if (active != 0 && g_combat_state->value_010 != 0) {
        if (g_status_685170.current_level < 0x2f) {
            g_status_685170.level_progress[g_status_685170.current_level].combat_end_count_01 += 1;
        }
        g_combat_state->value_010 /= active;
        Function4EEF10(g_combat_state->value_014 + g_combat_state->value_010, 1);
        int* entry = g_status_685170.status_ints_3121;
        int* end = entry + 1000;
        while (entry < end) {
            if (*entry == 1) {
                *entry = 2;
            }
            ++entry;
        }
    }
    g_combat_countdown_6850b0 = SetCountdownClock(120000);
    if (g_combat_state->uiNextPartyAction != 0) {
        Function4F0560(-1);
    }
    UpdateScreenOverlays(0);
    RestoreCombatFormation();
    Function53CD60();
    gXStatus.fCombatMode = 0;
    if (g_combat_state->unknown_a55[0xc] != 0) {
        Function517780();
    }
    Function5A3470();
    EnablePortraitAdvanceRegions0059BB70();
    DisableMainRegionSet();
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block->flag_327 == 0) {
        ClearSurfaceRect(0x17, 0x34, 0x2d, 0x159);
        ClearSurfaceRect(0x253, 0x34, 0x269, 0x159);
        RequestRedraw(0x810ff);
    }
    if (gXStatus.fNpcDialogueMode == 0 && gXStatus.fSpellCastMode == 0 &&
        gXStatus.fItemSelectMode == 0) {
        Function58F6B0(0);
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
        if (g_status_685170.buffers.party_rows[slot].occupied != 0) {
            CalcArmorClasses(&g_status_685170.buffers.characters[slot]);
        }
    }
    if (g_flag_006840bc != 0) {
        ResumeMainGameWorld();
    }
    ClearLevelDataFlags5To7();
    RequestRedrawParty();
    SetFloat60AB48();
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
        Function4EA1F0();
        ReportSaveFailed(1);
    }
}

/* Record a chosen action on the slot row: outside combat it fills the
   pending-action block, inside combat it fills the chosen-action record and
   dispatches the action's follow-up. */
// FUNCTION: WIZ8 0x004e7cc0
void ChooseAction(int party_slot, int action, int detail, const void* data, int arg_5, int arg_6)
{
    W8PartySlotRow* row = &g_status_685170.buffers.party_rows[party_slot];

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
            Function4EA5C0(party_slot);
        }
        ClearPartySlotMonsterHighlights(party_slot);
    } else {
        Function4E7EE0(party_slot, action, detail, data, arg_6);
        switch (action) {
        case 0:
        case 1:
        case 4:
        case 5:
            row->action_kind = action;
            row->action_detail = detail;
            row->action_is_kind_one = (action == 1);
            break;
        default:
            if (action != 10 && action != 0xb) {
                row->flag_0d0 = (unsigned char)action;
            }
        }
    }
    switch (action) {
    case 3:
    case 4:
    case 6:
    case 9:
        StartBreathCycle(party_slot, 0);
        return;
    case 10:
    case 0xb:
        Function52E5C0(g_special_event_0068c50c, -1, 0, g_effect_argument_005ed8c8);
        break;
    }
}

/* Record the chosen in-combat action on the slot row, copy its detail block,
   then aim and validate that choice for a still-active character. */
// FUNCTION: WIZ8 0x004e8000
void SetCharacterCombatAction(int party_slot, int action_kind, int action_detail, int arg_4,
                              void* data)
{
    if (gXStatus.fSpellCastMode == 0 && gXStatus.fItemSelectMode == 0) {
        g_status_685170.buffers.party_rows[party_slot].action_03d = -1;
        AimByKind(party_slot, W8_TARGET_KIND_NONE, W8_TARGETING_CONTEXT_CURRENT);
    }
    W8TargetSource source;
    W8CombatCharacterRow* row = &g_combat_state->characters[party_slot];
    SetTargetSourceToCharacter(party_slot, &source);
    if (action_kind > 0xb) {
        srAssertFail("iAction < CHAR_ACTION_COUNT",
                     "C:\\Projects\\Wizardry 8\\Local Code\\Combat.cpp", 0x484, 0);
    }
    g_status_685170.buffers.party_rows[party_slot].action_03d = action_kind;
    g_status_685170.buffers.party_rows[party_slot].action_detail_041 = arg_4;
    if (data == 0) {
        memset(&g_status_685170.buffers.party_rows[party_slot].action_detail_045, 0,
               sizeof(g_status_685170.buffers.party_rows[party_slot].action_detail_045));
    } else {
        memcpy(&g_status_685170.buffers.party_rows[party_slot].action_detail_045, data,
               sizeof(g_status_685170.buffers.party_rows[party_slot].action_detail_045));
    }
    row->unknown_a5[2] = 1;
    W8Character* character = &g_status_685170.buffers.characters[party_slot];
    if (character->hp_current != 0 && character->highest_condition < 0xd && action_detail != -1) {
        if (CharacterCanSwitchTo(party_slot, W8_TARGETING_CONTEXT_IN_COMBAT, 1, (char)(int)data) ==
            0) {
            AimByKind(party_slot, W8_TARGET_KIND_NONE, W8_TARGETING_CONTEXT_IN_COMBAT);
        } else if (Function536F60(party_slot, 2) == 0 &&
                   Function536570(party_slot, W8_TARGETING_CONTEXT_IN_COMBAT, (int)data) == 1) {
            Function4ECC80(&source,
                           &g_status_685170.buffers.party_rows[party_slot].target_in_combat);
        }
        if ((char)(int)data != 0) {
            Function537540(party_slot);
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
        PostCharacterNotice(party_slot, reinterpret_cast<const wchar_t*>(g_string_table[0x225]));
    } else if (action_kind == 9 &&
               !(g_combat_state->iActionChar == party_slot &&
                 g_status_685170.buffers.party_rows[party_slot].pending_action == 9)) {
        PostCharacterNotice(party_slot, reinterpret_cast<const wchar_t*>(g_string_table[0x226]));
    }
    CalcArmorClasses(character);
}

/* Ask the slot's currently selected action which context its outputs hold:
   the chosen action kind plus three context-dependent words. */
// FUNCTION: WIZ8 0x004e77b0
void ChooseCombatAction(int party_slot, int context, int* out_kind, int* out_action,
                        W8CombatSlot** out_target, W8ActionDetailBlock** out_detail)
{
    W8PartySlotRow* row = &g_status_685170.buffers.party_rows[party_slot];
    int kind;
    int value_a;
    W8CombatSlot* target;
    W8ActionDetailBlock* detail;

    if (context == W8_TARGETING_CONTEXT_CURRENT) {
        context = Function53BC90(party_slot);
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
            g_shared_action_detail_006840ab.item_use.item = reinterpret_cast<W8ItemInstance*>(
                GetSelectedOrFallbackValue0059E0D0()); // reinterpret-ok: the selected use-item value arrives as an int
            kind = 8;
            value_a = -1;
            target = &g_shared_target_0068408b;
            detail = &g_shared_action_detail_006840ab;
        } else {
            kind = 7;
            value_a = Function5A1350();
            target = &g_shared_target_0068408b;
            detail = &g_shared_action_detail_006840ab;
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
    if (CanPartySlotParticipate(context) == 0 && kind != 10 && kind != 0xb) {
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
unsigned char CharacterCanSwitchTo(int party_slot, W8TargetingContext context, int arg_3, int arg_4)
{
    W8Character* character = &g_status_685170.buffers.characters[party_slot];

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
        context = Function53BC90(party_slot);
    }
    int chosen;
    int value_a;
    W8ActionDetailBlock* detail;
    ChooseCombatAction(party_slot, context, &chosen, &value_a, 0, &detail);
    if (chosen == -1) {
        return 0;
    }
    switch (chosen) {
    case 0:
        if (CanAnyHandReachTarget(party_slot) == 0) {
            if (arg_4 == 0) {
                return 0;
            }
            if (character->equipment[6].item_id != -1) {
                if (character->equipment[7].item_id == -1) {
                    Function51EB90(character, &character->equipment[6], -1, 7);
                }
                if (character->equipment[6].uses_or_charges == 0) {
                    Function51EA90(character, &character->equipment[6]);
                }
            }
            if (CanAnyHandReachTarget(party_slot) == 0) {
                return 0;
            }
        }
        break;
    case 1:
        if (Function5458A0(party_slot) == 0) {
            if (arg_4 == 0) {
                return 0;
            }
            if (character->equipment[6].item_id != -1) {
                if (character->equipment[7].item_id == -1) {
                    Function51EB90(character, &character->equipment[6], -1, 7);
                }
                if (character->equipment[6].uses_or_charges == 0) {
                    Function51EA90(character, &character->equipment[6]);
                }
            }
            if (Function5458A0(party_slot) == 0) {
                return 0;
            }
        }
        break;
    case 2:
        if (CharacterHasTrait00547940(character, 0x1c) == 0 ||
            character->stamina <
                static_cast<int>(static_cast<unsigned int>(character->stamina_max) / 5)) {
            return 0;
        }
        break;
    case 4:
        return 1;
    case 5:
        if (CanCharacterAttack(party_slot) == 0) {
            return 0;
        }
        break;
    case 7:
        if (context == W8_TARGETING_CONTEXT_DIALOGUE && Function4F96F0(character) != 0) {
            return 1;
        }
        if (value_a == 0) {
            return 0;
        }
        if (Function4F9750(character, value_a) == 0) {
            return 0;
        }
        break;
    case 8:
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
        if (Function536F60(party_slot, 2, validated) == 0) {
            return 0;
        }
    }
    return 1;
}
