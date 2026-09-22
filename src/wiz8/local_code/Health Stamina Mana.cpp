#include "wiz8/engine_code/Camera.h"
#include "wiz8/local_screens/Screens.h"
#include "soundman.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/character_event_queue.h"
#include "wiz8/xstatus.h"
#include "wiz8/layouts/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/float_constants.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/CombatHostility.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/layouts/npc_state.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/npc_script_file.h"
#include "wiz8/layouts/item_instance.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/local_code/SpellEffect.h"
#include "wiz8/sr_api.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/utility.h"
#include "wiz8/sound_man.h"
#include "random.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/local_screens/MGSTextBox.h"

#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/music_playlist.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/string_database.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/npc_interaction.h"
#include "timer.h"
#include "wiz8/local_screens/MGSPortraits.h"
#include "wiz8/dialog_code/PortraitQuote.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"
#include "wiz8/notices.h"
#include "wiz8/regions.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/local_screens/mipe.h"
#include "wiz8/engine_code/Video2.h"
#include "sgp.h"
#undef S32
#undef U32
#include "bink.h"
#include "wiz8/bink_video.h"
#include "wiz8/mouth_gap.h"
#include "FileMan.h"

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_screens/OptionsScreen.h"

/* Local Code\Health Stamina Mana.cpp, named by the assertions these bodies
   embed. The party sweeps in here all share one shape: walk the eight party
   slots, skip the empty ones, and hand each occupied slot to a per-character
   worker. */

#define HEALTH_STAMINA_MANA_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Health Stamina Mana.cpp"

// FUNCTION: WIZ8 0x0052A890
unsigned int ApplyDamageToCharacter(int party_slot, unsigned int amount, char arg_3, char arg_4,
                                    char arg_5, W8SpellEffectResult* result_stats, char arg_7)
{
    W8Character* character = &g_status_685170.buffers.Char[party_slot];
    unsigned int absorbed;
    unsigned int applied;

    if (g_status_685170.buffers.XChar[party_slot].fOccupied == 0) {
        srAssertFail("fCHAR_OCCUPIED(uiChar)", HEALTH_STAMINA_MANA_CPP, 403, 0);
    }
    if (character->hp_current == 0) {
        return 0;
    }
    if (g_status_685170.world_suspended_2390 != 0) {
        PostCharacterNotice(party_slot, gppStringList[0x94c / 4], amount);
        return 0;
    }

    if (character->enchantments[2].turns_08 != 0) {
        absorbed = character->enchantments[2].magnitude_06;
        if (amount <= absorbed) {
            PostCharacterNotice(party_slot, gppStringList[0x193 - (arg_3 != 0)], amount);
            character->enchantments[2].magnitude_06 =
                static_cast<unsigned short>(absorbed - amount);
            if (result_stats != 0) {
                ++result_stats->count;
            }
            return 0;
        }

        PostCharacterNotice(party_slot, gppStringList[0x193 - (arg_3 != 0)], absorbed);
        amount -= absorbed;
        ClearCharacterEnchantmentSlot(party_slot, 2);
        PostCharacterNotice(party_slot, gppStringList[0x650 / 4]);
    }

    FatigueCharacter(party_slot, (amount * 2) / 3, 0, result_stats);
    if (result_stats != 0) {
        result_stats->amount += amount;
        ++result_stats->count;
    }

    if (arg_5 != 0) {
        if (arg_7 != 0) {
            PostCharacterNotice(party_slot, gppStringList[0x9a0 / 4], amount);
        } else if (arg_4 != 0) {
            ShowNoticef(9, gppStringList[0x950 / 4], amount);
        } else {
            PostCharacterNotice(party_slot, gppStringList[0x954 / 4], amount,
                                arg_3 != 0 ? gppStringList[0x95c / 4] : &g_wchar_00689b34);
        }
    }

    applied = character->hp_current;
    if (applied <= amount) {
        if (CharacterHasTrait00547940(character, 2) != 0 &&
            character->uiCondition[W8_CONDITION_EXHAUSTED] < 7) {
            CheatDeathRevive00547A50(party_slot);
            RecordCharacterDamage(party_slot, amount);
            return applied;
        }
        if (applied < amount) {
            amount = applied;
        }
    }

    character->hp_current = applied - amount;
    RecordCharacterDamage(party_slot, amount);
    if (character->hp_current != 0) {
        if (gXStatus.fSurprisePossible == 0) {
            QueueDamageReactionEvents(character);
        }
    } else {
        if (result_stats != 0) {
            W8SpellDamageReport* report =
                static_cast<W8SpellDamageReport*>(malloc(sizeof(W8SpellDamageReport)));
            if (report != 0) {
                memset(report, 0, sizeof(W8SpellDamageReport));
                report->kind = 1;
                report->value = party_slot;
                result_stats->reports.Add(report);
            }
        }
        SetCharacterCondition(party_slot, W8_CONDITION_DEAD, W8_CONDITION_INDEFINITE, 0, 0,
                              result_stats == 0);
    }

    if (character->uiCondition[W8_CONDITION_ASLEEP] != 0 && arg_3 == 0 &&
        Random(100) < (character->attributes[6].effective >> 1) + 0x32) {
        RemoveCharacterCondition(party_slot, W8_CONDITION_ASLEEP, 1);
    }
    return amount;
}

/* Stamina and realm spell-point constants the encodings keep as addressable
   storage rather than immediates. */
// GLOBAL: WIZ8 0x005ed8b8
float g_float_005ed8b8 = 0.1f;
// GLOBAL: WIZ8 0x005ec3f8
float g_float_005ec3f8 = 125.0f;
// GLOBAL: WIZ8 0x005ecbb4
float g_float_005ecbb4 = 0.02f;

/* The eligibility window the party sweeps use, the same one GetRandomCharacter
   and AnyPartyMemberCanUseItem apply: highest_condition below death. */

/* A negative amount means "as much as they could possibly hold", which the
   restore computes by summing the whole spell-point ceiling. */
enum { W8_RESTORE_EVERYTHING = -1 };

/* Roll the dice once per eligible party member and apply the result to each of
   them. The roll is separate per character rather than shared. */
// FUNCTION: WIZ8 0x0052a820
void ApplyRolledHealthChangeToParty(const W8Dice* dice, W8SpellEffectResult* result, int arg_3)
{
    int party_slot;

    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_status_685170.buffers.XChar[party_slot].fOccupied != 0 &&
            g_status_685170.buffers.Char[party_slot].highest_condition < W8_CONDITION_DEAD) {
            ApplyDamageToCharacter(party_slot, RollDice(dice), 0, arg_3, 0, result, 0);
        }
    }
}

/* Heal every occupied slot by its own roll, announced. The dice are assembled
   from the caller's own arguments rather than passed as a record, which is why
   the two count bytes arrive separately from the base. */
// FUNCTION: WIZ8 0x0052ad70
void HealPartyByDice(unsigned char count, unsigned char sides, short base)
{
    W8Dice dice;
    int party_slot;

    dice.base = base;
    dice.count = count;
    dice.sides = sides;
    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_status_685170.buffers.XChar[party_slot].fOccupied != 0) {
            HealCharacter(party_slot, RollDice(&dice), 1);
        }
    }
}

/* The stamina form, which is the same sweep unannounced. */
// FUNCTION: WIZ8 0x0052b160
void RestorePartyStaminaByDice(unsigned char count, unsigned char sides, short base)
{
    W8Dice dice;
    int party_slot;

    dice.base = base;
    dice.count = count;
    dice.sides = sides;
    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_status_685170.buffers.XChar[party_slot].fOccupied != 0) {
            RestoreCharacterStamina(party_slot, RollDice(&dice), 0);
        }
    }
}

/* Spend spell points from one realm. Spending more than is left is a caller
   error rather than something to clamp. */
// FUNCTION: WIZ8 0x0052b480
void SpendCharacterSpellPoints(int party_slot, int realm, int amount)
{
    W8Character* character = &g_status_685170.buffers.Char[party_slot];

    if (amount != 0) {
        if (character->iSPLeft[realm] < amount) {
            srAssertFail("pPC->iSPLeft[uiRealm] >= (INT32) uiSPs", HEALTH_STAMINA_MANA_CPP, 1067,
                         0);
        }
        character->iSPLeft[realm] -= amount;
        RequestPartySlotRedraw(party_slot);
    }
}

/* Give spell points back to one realm, never past its ceiling. */
// FUNCTION: WIZ8 0x0052b4f0
void RestoreCharacterRealmSpellPoints(int party_slot, int realm, int amount)
{
    W8Character* character = &g_status_685170.buffers.Char[party_slot];

    character->iSPLeft[realm] += amount;
    if (character->sp_max[realm] < character->iSPLeft[realm]) {
        character->iSPLeft[realm] = character->sp_max[realm];
    }
    RequestPartySlotRedraw(party_slot);
}

/* Drain spell points across the party. Unlike its neighbours this does not
   filter on the eligibility window - an unconscious character still loses
   points. */
// FUNCTION: WIZ8 0x0052b550
void DrainPartySpellPoints(int arg_1, int arg_2)
{
    int party_slot;

    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_status_685170.buffers.XChar[party_slot].fOccupied != 0) {
            DrainCharacterSpellPoints(party_slot, arg_1, arg_2);
        }
    }
}

/* Restore spell points across the party. A negative amount means as much as
   the character could possibly hold, which is the sum of all six realm
   ceilings. */
// FUNCTION: WIZ8 0x0052ba00
void RestorePartySpellPoints(int amount)
{
    int party_slot;
    int realm;
    int granted;

    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_status_685170.buffers.XChar[party_slot].fOccupied != 0 &&
            g_status_685170.buffers.Char[party_slot].highest_condition < W8_CONDITION_DEAD &&
            g_status_685170.buffers.Char[party_slot].hp_current != 0) {
            granted = amount;
            if (amount < 0) {
                granted = 0;
                for (realm = 0; realm < W8_SPELL_REALM_COUNT; ++realm) {
                    granted += g_status_685170.buffers.Char[party_slot].sp_max[realm];
                }
            }
            RestoreCharacterSpellPointsEvenly(party_slot, granted);
        }
    }
}

/* Suffix the damage notice carries when a poison tick is what hurt the
   monster. */
// GLOBAL: WIZ8 0x0061C964
const wchar_t g_poison_suffix_0061c964[] = L"POISON ";

/* Roll the dice once for every live monster inside the radius of a point and
   apply each roll as damage through the monster-side effect pass. */
// FUNCTION: WIZ8 0x0052BA80
void DamageMonstersInRadius(const srVector3T<float>& center, float radius, const W8Dice* dice,
                            W8TargetSource* source, W8SpellEffectResult* result)
{
    unsigned int index;
    int damage;
    W8MonsterInfo* monster_info;
    srVector3T<float> location;
    srVector3T<float> offset;

    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        monster_info = MonsterGetScriptPartByLocationIndex(index);
        if (monster_info->fActive != 0) {
            MonsterGetLocation(monster_info->monster, &location);
            offset = srVector3T<float>(center.x - location.x, center.y - location.y,
                                       center.z - location.z);
            if (offset.Length() <= radius) {
                damage = RollDice(dice);
                ApplyDamageToMonster(monster_info, damage, source, 0, 1, 0, result, 0);
            }
        }
    }
}

/* Apply rolled damage to a monster: the slot-2 enchantment absorbs first,
   the result block collects the hit, combat and threat state decide whether
   the blow is announced and remembered, two thirds of it fatigue the monster,
   and the remainder comes off hit points - a lethal hit logs a kill record
   and starts the death sequence. Returns the amount actually applied. */
// FUNCTION: WIZ8 0x0052BB60
unsigned int ApplyDamageToMonster(W8MonsterInfo* monster_info, unsigned int amount,
                                  W8TargetSource* source, char quiet, unsigned char in_combat,
                                  char a, W8SpellEffectResult* result_stats, char c)
{
    unsigned int absorbed;
    unsigned int applied;
    unsigned int category;
    W8SpellDamageReport* report;

    if (monster_info->hp_current == 0) {
        return 0;
    }
    if (monster_info->enchantments[2].turns_08 != 0) {
        absorbed = monster_info->enchantments[2].magnitude_06;
        if (amount <= absorbed) {
            PostMonsterNotice(monster_info, gppStringList[0x193 - (quiet != 0)], amount);
            monster_info->enchantments[2].magnitude_06 =
                static_cast<unsigned short>(absorbed - amount);
            if (result_stats != 0) {
                ++result_stats->count;
            }
            return 0;
        }
        PostMonsterNotice(monster_info, gppStringList[0x193 - (quiet != 0)], absorbed);
        amount -= absorbed;
        ClearMonsterEnchantmentSlot(monster_info->location_id, 2);
    }
    if (result_stats != 0) {
        result_stats->amount += amount;
        ++result_stats->count;
    }
    if (amount != 0) {
        if (gXStatus.fCombatMode != 0 || monster_info->party_threat.visible_to_player_25 != 0) {
            PointCameraAtMonster(monster_info, 0, 1);
            category = 9;
            if (TargetSourceIsCharacter(source, 0) != 0 && source->iChar != -1) {
                category = 8;
            }
            if (in_combat != 0) {
                if (c != 0) {
                    ShowNoticef(category, FormatWideString(g_format_s_space_s_00617584,
                                                           GetMonsterName(monster_info, 0, 0),
                                                           gppStringList[0x9a0 / 4], amount));
                } else if (a != 0) {
                    ShowNoticef(category, gppStringList[0x950 / 4], amount);
                } else {
                    ShowNoticef(category, gppStringList[0x958 / 4],
                                GetMonsterName(monster_info, 0, 0), amount,
                                quiet != 0 ? g_poison_suffix_0061c964 : &g_wchar_00689b34);
                }
            }
        }
        if (source->fBackfire == 0 && source->fReflection == 0 && source->target_diverted == 0 &&
            quiet == 0) {
            monster_info->condition_target_304 = *source;
            if (monster_info->fInCombat != 0 && TargetSourceIsCharacter(source, 0) != 0 &&
                source->iChar != -1) {
                monster_info->pCombat->character_hate[source->iChar] += amount;
            }
        }
        applied = monster_info->hp_current;
        if (amount < applied) {
            monster_info->hp_current = applied - amount;
            FatigueMonster(monster_info, (amount * 2) / 3, result_stats);
            MonsterReactsToBeingStruck(monster_info, source, quiet);
        } else {
            monster_info->hp_current = 0;
            if (result_stats != 0) {
                report = static_cast<W8SpellDamageReport*>(malloc(sizeof(W8SpellDamageReport)));
                if (report != 0) {
                    memset(report, 0, sizeof(W8SpellDamageReport));
                    report->kind = 3;
                    wcscpy(report->text, GetMonsterName(monster_info, 0, 0));
                    result_stats->reports.Add(report);
                }
            }
            MonsterStartsDying(monster_info, result_stats == 0);
        }
        ClearHighlightIfItIs(&monster_info->location_id);
        UpdateMonsterDamageAppearance(monster_info);
        if (monster_info->monster != 0) {
            monster_info->monster->SpawnDamageNumber(amount);
        }
    }
    return amount;
}

/* Heal one monster. A monster that is already dead or already whole is left
   alone; healing it to full says so differently from healing it partway. */
// FUNCTION: WIZ8 0x0052bfd0
void HealMonster(W8MonsterInfo* monster_info, unsigned int amount, char announce)
{
    if (monster_info->hp_current == 0 ||
        monster_info->hp_current == static_cast<unsigned int>(monster_info->hp_max) ||
        amount == 0) {
        return;
    }

    monster_info->hp_current += amount;
    if (monster_info->hp_current > static_cast<unsigned int>(monster_info->hp_max)) {
        monster_info->hp_current = monster_info->hp_max;
    }
    ClearHighlightIfItIs(&monster_info->location_id);
    UpdateMonsterDamageAppearance(monster_info);

    if (announce) {
        if (monster_info->hp_current == static_cast<unsigned int>(monster_info->hp_max)) {
            ShowNoticef(9, gppStringList[0x964 / 4], GetMonsterName(monster_info, 0, 0));
        } else {
            ShowNoticef(9, gppStringList[0x96c / 4], GetMonsterName(monster_info, 0, 0), amount);
        }
    }
}

/* What one monster action costs in fatigue, before it is spent. Four of the
   ten actions are free; the plain attack costs markedly more when its detail
   is three. A monster that tires twice as fast pays double. */
// FUNCTION: WIZ8 0x0052c240
int MonsterActionFatigueCost(const W8MonsterInfo* monster_info)
{
    int cost = 0;

    switch (monster_info->action_kind) {
    case 0:
        if (monster_info->action_detail == 3) {
            cost = Random(8) + 5;
        } else {
            cost = Random(3) + 2;
        }
        break;
    case 1:
    case 9:
        cost = Random(2) + 1;
        break;
    case 4:
    case 7:
        cost = Random(5) + 5;
        break;
    case 5:
    case 6:
        cost = Random(3) + 3;
        break;
    case -1:
    case 2:
    case 3:
    case 8:
        break;
    default:
        srAssertFail("FALSE", HEALTH_STAMINA_MANA_CPP, 1774,
                     FormatString("MonsterActionFatigueCost: ERROR - Invalid action %d",
                                  monster_info->action_kind));
    }

    if (monster_info->condition_turns[W8_CONDITION_FATIGUE_DOUBLED] == 0) {
        return cost;
    }
    return cost * 2;
}

/* The two fatigue-band thresholds and the four bands they cut the stamina
   fraction into. Both characters and monsters use the same ladder. */
enum {
    W8_FATIGUE_BAND_1 = 0x32,
    W8_FATIGUE_BAND_2 = 0x46,
    W8_FATIGUE_BAND_3 = 0x55,
    W8_FATIGUE_BAND_4 = 0x5e
};

/* How much stamina shakes exhaustion off again. */
enum { W8_STAMINA_TO_SHAKE_OFF_EXHAUSTION = 9 };

// FUNCTION: WIZ8 0x0052a710
int GetCharacterRealmSpellPoints(const W8Character* character, int realm)
{
    int points = character->iSPLeft[realm];
    return points > 0 ? points : 0;
}

// FUNCTION: WIZ8 0x0052a730
int SumCharacterSpellPointsLeft(const W8Character* character)
{
    int total = 0;
    for (int realm = 0; realm < W8_SPELL_REALM_COUNT; ++realm) {
        total += character->iSPLeft[realm] > 0 ? character->iSPLeft[realm] : 0;
    }
    return total;
}

// FUNCTION: WIZ8 0x0052a7d0
unsigned int FatigueArmorPenalty(int fatigue_band)
{
    switch (fatigue_band) {
    case 0:
        return 0;
    case 1:
        return 5;
    case 2:
        return 10;
    case 3:
        return 20;
    case 4:
        return 40;
    default:
        return fatigue_band;
    }
}

/* Two effects the party is holding that a wounded character can no longer
   sustain, and the third that only the deeper threshold breaks. */
// GLOBAL: WIZ8 0x005ed904
unsigned int g_effect_threshold_005ed904 = 50;
// GLOBAL: WIZ8 0x005ed900
unsigned int g_effect_threshold_005ed900 = 70;
/* 0x0061E518: one notice index per spell realm, giving the realm's name. */
/* Turn a pool fraction into a band. The same ladder decides a character's
   fatigue band and a monster's, from the percentage of the pool that is
   missing rather than the part that is left. */
static int FatigueBandFromMissing(int missing_percent)
{
    if (missing_percent < W8_FATIGUE_BAND_1) {
        return 0;
    }
    if (missing_percent < W8_FATIGUE_BAND_2) {
        return 1;
    }
    if (missing_percent < W8_FATIGUE_BAND_3) {
        return 2;
    }
    return (missing_percent > W8_FATIGUE_BAND_4) + 3;
}

/* Heal one character, never past their maximum. Recovering enough of their
   hit points breaks the effects that only held while they were badly hurt -
   the deeper threshold breaks two more than the shallower one. */
// FUNCTION: WIZ8 0x0052add0
void HealCharacter(int party_slot, int amount, char announce)
{
    W8Character* character = &g_status_685170.buffers.Char[party_slot];
    unsigned int hp_max;
    unsigned int fraction;

    if (g_status_685170.buffers.XChar[party_slot].fOccupied == 0) {
        srAssertFail("fCHAR_OCCUPIED(uiChar)", HEALTH_STAMINA_MANA_CPP, 661, 0);
    }

    if (character->hp_current == 0) {
        return;
    }
    hp_max = character->uiHPMax;
    if (character->hp_current == hp_max) {
        return;
    }

    character->hp_current += amount;
    if (character->hp_current > hp_max) {
        character->hp_current = hp_max;
    }
    if (announce) {
        if (character->hp_current == hp_max) {
            PostCharacterNotice(party_slot, gppStringList[0x960 / 4]);
        } else {
            PostCharacterNotice(party_slot, gppStringList[0x968 / 4], amount);
        }
    }

    fraction = (character->hp_current * 100) / static_cast<unsigned int>(character->uiHPMax);
    if (fraction >= g_effect_threshold_005ed904) {
        if (gXStatus.character_event_queue->HasEventCharacter(g_effect_005ee594, party_slot)) {
            gXStatus.character_event_queue->SetEventCharacterMask(g_effect_005ee594, party_slot, 0);
        }
        if (fraction >= g_effect_threshold_005ed900) {
            if (gXStatus.character_event_queue->HasEventCharacter(g_effect_005ee590, party_slot)) {
                gXStatus.character_event_queue->SetEventCharacterMask(g_effect_005ee590, party_slot,
                                                                      0);
            }
            if (gXStatus.character_event_queue->HasEventCharacter(g_effect_005ee5f8, party_slot)) {
                gXStatus.character_event_queue->SetEventCharacterMask(g_effect_005ee5f8, party_slot,
                                                                      0);
            }
        }
    }
}

/* Give one character stamina back, never past their maximum. Any move re-bands
   their fatigue, and a change of band re-runs the armour class pass because
   fatigue feeds it. Enough stamina also shakes off exhaustion. */
// FUNCTION: WIZ8 0x0052b1c0
void RestoreCharacterStamina(int party_slot, int amount, char announce)
{
    W8Character* character = &g_status_685170.buffers.Char[party_slot];
    int stamina_max;
    int previous_band;
    int band;

    if (character->highest_condition >= W8_CONDITION_DEAD || character->hp_current == 0) {
        return;
    }
    stamina_max = character->uiStaminaMax;
    if (character->stamina == stamina_max) {
        return;
    }

    character->stamina += amount;
    if (character->stamina > stamina_max) {
        character->stamina = stamina_max;
    }
    if (announce) {
        if (character->stamina == stamina_max) {
            PostCharacterNotice(party_slot, gppStringList[0x970 / 4]);
        } else {
            PostCharacterNotice(party_slot, gppStringList[0x978 / 4], amount);
        }
    }

    previous_band = character->fatigue_band;
    band = FatigueBandFromMissing(
        100 - static_cast<int>((character->stamina * 100) /
                               static_cast<unsigned int>(character->uiStaminaMax)));
    character->fatigue_band = band;
    if (band != previous_band) {
        CalcArmorClasses(character);
    }
    if (character->uiCondition[W8_CONDITION_EXHAUSTED] == W8_CONDITION_INDEFINITE &&
        character->stamina > W8_STAMINA_TO_SHAKE_OFF_EXHAUSTION) {
        RemoveCharacterCondition(party_slot, W8_CONDITION_EXHAUSTED, 1);
    }
}

/* Drain spell points from one character, taking them from randomly chosen
   realms until the whole amount is gone or fifty attempts have been spent.
   Each realm only gives up what it has, and each withdrawal is announced with
   that realm's name. */
// FUNCTION: WIZ8 0x0052b590
void DrainCharacterSpellPoints(int party_slot, unsigned int amount, char announce)
{
    W8Character* character = &g_status_685170.buffers.Char[party_slot];
    unsigned int remaining = amount;
    unsigned int taken;
    int attempts;
    int realm;

    if (character->hp_current == 0) {
        return;
    }
    if (g_status_685170.world_suspended_2390 != 0) {
        PostCharacterNotice(party_slot, gppStringList[0x980 / 4], amount);
        return;
    }

    for (attempts = 0x32; remaining != 0 && attempts != 0; --attempts) {
        realm = Random(W8_SPELL_REALM_COUNT);
        if (character->iSPLeft[realm] > 0) {
            taken = remaining;
            if (static_cast<unsigned int>(character->iSPLeft[realm]) <= remaining) {
                taken = character->iSPLeft[realm];
            }
            SpendCharacterSpellPoints(party_slot, realm, taken);
            if (announce) {
                ShowNoticef(8, gppStringList[0x98c / 4], amount,
                            gppStringList[g_realm_message_offsets[realm]]);
            }
            remaining = amount - taken;
            amount = remaining;
        }
    }
}

/* Spread spell points across a character's six realms, always topping up the
   realm that is furthest from full. The realms are sorted by how far short
   they are, then handed a point each in turn; ties are given a point together
   so the deficits stay level. */
// FUNCTION: WIZ8 0x0052b910
void RestoreCharacterSpellPointsEvenly(int party_slot, int amount)
{
    W8Character* character = &g_status_685170.buffers.Char[party_slot];
    struct {
        unsigned int realm;
        unsigned int deficit;
    } order[W8_SPELL_REALM_COUNT];
    unsigned int index;
    int granted = 0;
    bool tied;

    for (index = 0; index < W8_SPELL_REALM_COUNT; ++index) {
        order[index].realm = index;
        order[index].deficit = character->sp_max[index] - character->iSPLeft[index];
    }
    qsort(order, W8_SPELL_REALM_COUNT, sizeof(order[0]), CompareSpellPointDeficits);

    for (;;) {
        if (amount == 0) {
            break;
        }
        for (index = 0; index < W8_SPELL_REALM_COUNT; ++index) {
            if (order[index].deficit == 0 || (index != W8_SPELL_REALM_COUNT - 1 &&
                                              order[index].deficit < order[index + 1].deficit)) {
                if (index == W8_SPELL_REALM_COUNT - 1) {
                    PostCharacterNotice(party_slot, gppStringList[0x68c / 4], granted);
                    return;
                }
                continue;
            }
            tied = index < W8_SPELL_REALM_COUNT && order[index].deficit == order[index + 1].deficit;
            ++character->iSPLeft[order[index].realm];
            --amount;
            ++granted;
            --order[index].deficit;
            if (!tied) {
                break;
            }
            if (amount == 0) {
                PostCharacterNotice(party_slot, gppStringList[0x68c / 4], granted);
                return;
            }
        }
    }
    PostCharacterNotice(party_slot, gppStringList[0x68c / 4], granted);
}

/* Wound one character. Two thirds of the damage also tires them, the damage
   itself is booked against the hit-point adjustment and the pools rebuilt from
   it, and a character with no protection against it is put under condition
   one. */
// FUNCTION: WIZ8 0x0052b7e0
void DamageCharacter(int party_slot, int damage, char announce)
{
    W8Character* character = &g_status_685170.buffers.Char[party_slot];

    if (g_status_685170.buffers.XChar[party_slot].fOccupied == 0) {
        srAssertFail("fCHAR_OCCUPIED(uiChar)", HEALTH_STAMINA_MANA_CPP, 1186, 0);
    }

    if (character->uiHPMax != 0 && character->hp_current != 0) {
        FatigueCharacter(party_slot, (damage * 2) / 3, 0, 0);
        if (announce) {
            ShowNoticef(8, gppStringList[0x710 / 4], damage);
        }
        character->hp_adjustment -= damage;
        RecalculateCharacterHitPoints(character);
        if (character->uiCondition[1] == 0) {
            SetCharacterCondition(party_slot, 1, W8_CONDITION_INDEFINITE, 0, 0, 0);
        }
        if (character->hp_current != 0) {
            QueueDamageReactionEvents(character);
        }
    }
}

/* Tire one monster. Running its pool down to nothing puts it under the
   exhausted condition indefinitely, and tells whoever asked for the fatigue
   that it landed. */
// FUNCTION: WIZ8 0x0052c070
void FatigueMonster(W8MonsterInfo* monster_info, unsigned int amount,
                    W8SpellEffectResult* report_to)
{
    W8TargetSource target_block;

    if (monster_info->hp_current == 0) {
        return;
    }
    if (amount != 0) {
        GetMonsterDataForInfo(monster_info);
        if (amount < static_cast<unsigned int>(monster_info->stamina)) {
            monster_info->stamina -= amount;
        } else {
            monster_info->stamina = 0;
        }
    }

    monster_info->fatigue_band = FatigueBandFromMissing(
        100 - static_cast<int>((monster_info->stamina * 100) /
                               static_cast<unsigned int>(monster_info->stamina_max)));

    if (monster_info->stamina == 0 &&
        monster_info->condition_turns[W8_CONDITION_EXHAUSTED] < W8_CONDITION_INDEFINITE) {
        ResetTargetSource(&target_block);
        SetMonsterCondition(monster_info->location_id, W8_CONDITION_EXHAUSTED,
                            W8_CONDITION_INDEFINITE, 0, &target_block, report_to == 0);
        if (report_to != 0) {
            ++report_to->condition_counts[W8_CONDITION_EXHAUSTED];
        }
    }
}

/* A completed spell costs its database level plus the cast result. Result
   eight is the one outcome that carries no stamina charge. MonsterCastsSpell
   deliberately returns this value to its caller. */
// FUNCTION: WIZ8 0x0052c320
int SpellCastFatigueCost(int spell_id, int result)
{
    if (result == 8) {
        return 0;
    }
    return g_spell_records[spell_id].spell_level + result;
}

/* Give one monster stamina back, the mirror of the character form down to the
   band ladder and the exhaustion it shakes off. */
// FUNCTION: WIZ8 0x0052c140
void RestoreMonsterStamina(W8MonsterInfo* monster_info, int amount, char announce)
{
    unsigned int stamina_max;

    if (monster_info->highest_condition >= W8_CONDITION_DEAD || monster_info->hp_current == 0) {
        return;
    }
    stamina_max = monster_info->stamina_max;
    if (static_cast<unsigned int>(monster_info->stamina) == stamina_max) {
        return;
    }

    monster_info->stamina += amount;
    if (static_cast<unsigned int>(monster_info->stamina) > stamina_max) {
        monster_info->stamina = stamina_max;
    }
    if (announce) {
        if (static_cast<unsigned int>(monster_info->stamina) == stamina_max) {
            ShowNoticef(9, gppStringList[0x974 / 4], GetMonsterName(monster_info, 0, 0));
        } else {
            ShowNoticef(9, gppStringList[0x97c / 4], GetMonsterName(monster_info, 0, 0), amount);
        }
    }

    monster_info->fatigue_band = FatigueBandFromMissing(
        100 - static_cast<int>((monster_info->stamina * 100) /
                               static_cast<unsigned int>(monster_info->stamina_max)));

    if (monster_info->condition_turns[W8_CONDITION_EXHAUSTED] == W8_CONDITION_INDEFINITE &&
        static_cast<unsigned int>(monster_info->stamina) > W8_STAMINA_TO_SHAKE_OFF_EXHAUSTION) {
        ClearMonsterCondition(monster_info->location_id, W8_CONDITION_EXHAUSTED);
    }
}

/* How a monster answers being struck. It plays the struck cycle; a monster
   with the right make-up may be knocked into another condition, its chance
   riding on half its fifth converted attribute; a monster the party had under
   control is handed back to itself; and if the attacker is not already an
   enemy, the disposition check decides whether being hit makes them one. */
// FUNCTION: WIZ8 0x0052beb0
void MonsterReactsToBeingStruck(W8MonsterInfo* monster_info, W8TargetSource* attacker, char quiet)
{
    StartMonsterCycle(monster_info, 0x14, 1);

    if (monster_info->condition_turns[15] != 0 && quiet == 0 &&
        Random(100) < static_cast<unsigned int>((monster_info->attributes[4] >> 1) + 0x32)) {
        ClearMonsterCondition(monster_info->location_id, W8_CONDITION_ASLEEP);
    }
    if (monster_info->control_state == 1) {
        SetMonsterControlState(monster_info, 0);
    }

    if (!TargetSourceIsCharacter(attacker, 0) && !TargetSourceIsMonster(attacker, 0)) {
        return;
    }
    if (attacker->fBackfire == 0 && attacker->fReflection == 0 && attacker->target_diverted == 0 &&
        quiet == 0 && monster_info->condition_turns[W8_CONDITION_HOSTILE] != 0) {
        if (TargetSourceIsCharacter(attacker, 0)) {
            if (MonsterVsCharDisposition(attacker->iChar, monster_info) == 2) {
                ApplyMonsterCondition(monster_info->location_id, 0xd, 1);
            }
        } else if (MonsterHostility00546F80(
                       MonsterInfoFromID(1570, HEALTH_STAMINA_MANA_CPP, attacker->iMonsterID, 1),
                       monster_info) == 2) {
            ApplyMonsterCondition(monster_info->location_id, 0xd, 1);
        }
    }
}

/* The five load categories and what each costs on top of an action's base
   fatigue, as a percentage. */
enum { W8_LOAD_CATEGORY_COUNT = 5 };
static const int kLoadFatiguePercent[W8_LOAD_CATEGORY_COUNT] = {0, 0x19, 0x32, 100, 200};

/* The band past which deep fatigue takes hold, and the band it lets go at. */
enum { W8_FATIGUE_BAND_DEEP = 2, W8_FATIGUE_BAND_RECOVERED = 2 };

/* Tire one character. The load they are carrying scales the cost - eased or
   worsened by the two load modifiers - and the result is taken out of their
   stamina without going below nothing. Running out puts them under the
   exhausted condition; merely dropping into the deep band applies the
   deep-fatigue effect once. */
// FUNCTION: WIZ8 0x0052af50
void FatigueCharacter(int party_slot, int amount, char scale_by_load,
                      W8SpellEffectResult* report_to)
{
    W8Character* character = &g_status_685170.buffers.Char[party_slot];
    int previous_band;
    int band;
    int load_percent;

    if (character->stamina <= 0 || character->hp_current == 0 || amount == 0) {
        return;
    }

    if (scale_by_load) {
        if ((unsigned int)character->load_category >= W8_LOAD_CATEGORY_COUNT) {
            srAssertFail("FALSE", HEALTH_STAMINA_MANA_CPP, 813,
                         "FatigueCharacter: ERROR - Invalid load category");
        }
        load_percent = kLoadFatiguePercent[character->load_category];
        if (character->uiCondition[W8_CONDITION_LOAD_EASED] == 0) {
            if (character->enchantments[5].turns_08 != 0) {
                load_percent += 0x19;
            }
        } else {
            load_percent -= 0x19;
        }
        amount += (load_percent * amount) / 100;
    }

    if (amount < 0) {
        amount = 0;
    } else if (amount > character->stamina) {
        amount = character->stamina;
    }
    character->stamina -= amount;

    previous_band = character->fatigue_band;
    band = FatigueBandFromMissing(
        100 - static_cast<int>((character->stamina * 100) /
                               static_cast<unsigned int>(character->uiStaminaMax)));
    character->fatigue_band = band;
    if (band != previous_band) {
        CalcArmorClasses(character);
    }

    if (character->stamina < 1) {
        if (character->uiCondition[W8_CONDITION_EXHAUSTED] < W8_CONDITION_INDEFINITE) {
            SetCharacterCondition(party_slot, W8_CONDITION_EXHAUSTED, W8_CONDITION_INDEFINITE, 0, 0,
                                  report_to == 0);
            if (report_to != 0) {
                ++report_to->condition_counts[W8_CONDITION_EXHAUSTED];
            }
        }
    } else if (band != previous_band && band > W8_FATIGUE_BAND_DEEP) {
        if (!character->deep_fatigue_applied) {
            QueueCharacterEvent(character, g_effect_005ee598, 0, g_effect_argument_005ed8c8,
                                g_effect_argument_005ed914);
            character->deep_fatigue_applied = 1;
        }
        if ((unsigned int)character->fatigue_band < W8_FATIGUE_BAND_RECOVERED) {
            character->deep_fatigue_applied = 0;
        }
    }
}

/* What one character action costs in fatigue before it is spent. An attack
   costs by the weight of what is swung - unarmed and the two special attack
   modes are flat rolls instead - a run costs a fifth of the character's whole
   stamina with a floor, and the rest are free. A character who tires twice as
   fast pays double. */
// FUNCTION: WIZ8 0x0052b2f0
unsigned int CharacterActionFatigueCost(int party_slot, int action_kind)
{
    unsigned int cost = 0;
    W8CombatCharacterRow* combat_row = &g_combat_state->characters[party_slot];
    int attack_mode;
    int item_id;
    int weight_bands;

    switch (action_kind) {
    case W8_ACTION_ATTACK:
    case W8_ACTION_BERSERK:
        attack_mode =
            g_status_685170.buffers.XChar[party_slot].attack_mode[combat_row->current_hand];
        if (attack_mode == 5) {
            cost = Random(3) + 2;
        } else if (attack_mode == 6) {
            cost = Random(3) + 3;
        } else {
            item_id = g_status_685170.buffers.Char[party_slot]
                          .EquippedItem[combat_row->current_equip_slot]
                          .iItemNo;
            if (item_id == -1) {
                cost = Random(4) + 3;
            } else {
                weight_bands = g_item_records[item_id].weight / 0x28 + 1;
                cost = Random(weight_bands) + 1 + weight_bands;
            }
        }
        if (action_kind == W8_ACTION_BERSERK) {
            cost *= 2;
        }
        break;
    case W8_ACTION_BREATHE:
    case W8_ACTION_DEFEND:
    case W8_ACTION_PROTECT:
    case W8_ACTION_CAST_SPELL:
        break;
    case W8_ACTION_TURN_UNDEAD:
    case W8_ACTION_PRAY:
        cost = g_status_685170.buffers.Char[party_slot].uiStaminaMax / 5;
        if (cost < 0x14) {
            cost = 0x14;
        }
        break;
    case W8_ACTION_USE_ITEM:
        cost = Random(2) + 1;
        break;
    case W8_ACTION_EQUIP:
        cost = Random(4) + 3;
        break;
    default:
        cost = 0;
    }

    if (g_status_685170.buffers.Char[party_slot].uiCondition[W8_CONDITION_FATIGUE_DOUBLED] != 0) {
        cost *= 2;
    }
    return cost;
}

/* Drain spell points from one named realm, taking no more than it holds.
   Announced with the realm's own name. */
// FUNCTION: WIZ8 0x0052b6d0
void DrainCharacterRealmSpellPoints(int party_slot, int realm, unsigned int amount, char announce)
{
    W8Character* character = &g_status_685170.buffers.Char[party_slot];
    unsigned int available;

    if (character->hp_current == 0) {
        return;
    }
    available = character->iSPLeft[realm];
    if (available == 0) {
        return;
    }

    if (g_status_685170.world_suspended_2390 != 0) {
        PostCharacterNotice(party_slot, gppStringList[0x988 / 4], amount,
                            gppStringList[g_realm_message_offsets[realm]]);
        return;
    }

    if ((int)available < 0) {
        return;
    }
    if (amount >= available) {
        amount = available;
    }
    if (amount == 0) {
        return;
    }
    SpendCharacterSpellPoints(party_slot, realm, amount);
    if (announce) {
        ShowNoticef(8, gppStringList[0x98c / 4], amount,
                    gppStringList[g_realm_message_offsets[realm]]);
    }
}

/* One character dies. Every condition but the tenth is lifted, both pools are
   emptied, the two targeting blocks on their slot row are cleared, combat
   forgets them as a participant, and whatever they were animating is told to
   stop. */
// FUNCTION: WIZ8 0x0052abf0
void CharacterDies(int party_slot)
{
    W8Character* character = &g_status_685170.buffers.Char[party_slot];
    W8PartySlotRow* row = &g_status_685170.buffers.XChar[party_slot];
    unsigned int condition;
    int animation;

    if (row->fOccupied == 0) {
        srAssertFail("fCHAR_OCCUPIED(uiChar)", HEALTH_STAMINA_MANA_CPP, 561, 0);
    }

    ++character->death_count_09fd;
    for (condition = 0; condition < W8_CONDITION_CLEARABLE_COUNT; ++condition) {
        if (condition != 10 && character->uiCondition[condition] != 0) {
            RemoveCharacterCondition(party_slot, condition, 0);
        }
    }
    character->hp_current = 0;
    character->stamina = 0;

    ResetCombatSlot(&row->target_out_of_combat);
    ResetCombatSlot(&row->target_in_combat);
    if (gXStatus.fCombatMode != 0) {
        RecordCharacterDeath(party_slot);
    }
    QueuePartyDeathReaction(party_slot);
    SoundPlay("Data\\Sound\\Misc\\CharacterDead.wav", 0);

    if (gXStatus.fCombatMode != 0) {
        if (g_combat_state->iActionChar == party_slot) {
            g_combat_state->eCombatActionStatus = 0;
            g_combat_state->iActionChar = -1;
        }
        row->pending_action = -1;
        g_combat_state->characters[party_slot].phase = 0;
        g_combat_state->characters[party_slot].dead_34 = 1;
        DropCharacterFromRound(party_slot);
    }

    animation = row->animation_0fa;
    if (animation != -1) {
        W8NpcState* npc = GetNpcState(animation);
        if (npc != 0) {
            npc->spawned_04 = 1;
        }
    }
}

/* Rebuilds the hit-point ceiling from scratch every time it is called: each
   profession the character has levels in contributes its own per-level factor,
   scaled by a figure derived from the fourth attribute record's effective
   value, and the profession the character started in counts one level more
   than it has taken. The running total lives in the x87 stack across the whole
   loop, which is why the zero it starts from is loaded before the profession
   guard and discarded by an `fstp` on the early return.
   The level is unsigned - the emitted test is `jbe`, not `jle` - and the
   attribute is widened through a zeroed high dword, which is the unsigned
   conversion rather than the signed one.
   Losing the last hit point applies condition 0x12 with the ceiling duration
   the party notice uses, which is the one place this writes anything beyond
   the two pools. */
// FUNCTION: WIZ8 0x0052A2F0
void RecalculateCharacterHitPoints(W8Character* character)
{
    double total = 0.0;
    int profession;
    int hit_points;
    int remaining;

    if (character->iProfession == W8_PROFESSION_NONE) {
        return;
    }

    for (profession = 0; profession < W8_PROFESSION_COUNT; profession++) {
        unsigned int levels = character->profession_levels[profession];
        if (profession == character->original_profession) {
            levels++;
        }
        if (levels > 0) {
            double vitality = character->attributes[3].effective * 0.4;
            total += (vitality * 0.02 + 0.6) * g_profession_hit_point_factors[profession] * levels;
        }
    }

    hit_points = (int)(total + 0.5) + character->hp_adjustment;
    if (hit_points < 1) {
        hit_points = 1;
    }
    if (hit_points != character->uiHPMax) {
        remaining = (hit_points - character->uiHPMax) + static_cast<int>(character->hp_current);
        if (remaining < 0) {
            remaining = 0;
        }
        character->uiHPMax = hit_points;
        character->hp_current = remaining;
        if (remaining == 0) {
            SetCharacterCondition(CharacterPointerToPartySlot(character), W8_CONDITION_DEAD,
                                  W8_CONDITION_INDEFINITE, 0, 0, 1);
        }
    }
}

/* Recompute the stamina ceiling from the three physical attributes and the
   level, subtract any outstanding penalty, carry the difference into the
   current pool, and derive the fatigue band from what is left.

   Retail compares the penalty with JNC and divides the scaled pool with DIV,
   so the ceiling and its delta are unsigned values even though the stored
   fields are ints. The divisor is unguarded in retail; a zero ceiling would
   trap there too. */
// FUNCTION: WIZ8 0x0052a3e0
void RecalculateCharacterStamina(W8Character* character)
{
    unsigned int previous = character->uiStaminaMax;
    unsigned int value =
        (unsigned int)(((character->attributes[0].effective + character->attributes[2].effective +
                         character->attributes[3].effective) *
                        (1.0f / 3.0f)) *
                           (character->uiExpLevel * g_float_005ed8b8 +
                            g_environment_near_scale_005ec0b0) +
                       g_double_005ebe80);
    character->uiStaminaMax = value;
    if (character->fatigue_penalty_0b21 < value) {
        character->uiStaminaMax = value - character->fatigue_penalty_0b21;
    } else {
        character->uiStaminaMax = 0;
    }
    value = character->uiStaminaMax;
    if (value != previous) {
        character->stamina += value - previous;
    }
    int fatigue = 100 - (int)((unsigned int)character->stamina * 100 / value);
    if (fatigue < 0x32) {
        character->fatigue_band = 0;
        return;
    }
    if (fatigue < 0x46) {
        character->fatigue_band = 1;
        return;
    }
    if (fatigue < 0x55) {
        character->fatigue_band = 2;
        return;
    }
    character->fatigue_band = (fatigue > 0x5e) + 3;
}

/* The resistance bonus skill (36) is derived only for the professions whose
   bodies can learn spells; a few fixed professions keep it at zero. */
// FUNCTION: WIZ8 0x0052a500
void RecalculateRealmSpellPoints(W8Character* character)
{
    int profession = character->iProfession;
    if (profession != 0 && (profession < 7 || profession > 9)) {
        character->skill_unlocks[0x24] = RebuildRealmSpellPointCeilings0052A540(character);
        return;
    }
    character->skill_unlocks[0x24] = 0;
}

/* Rebuild the six realm spell-point ceilings from the learned spells, the
   realm skill levels and the attributes. The weighted best four realm skills
   are capped at 125 and added to every realm's own skill and school
   attribute; the sum scales with the realm's learned-spell count, level and
   one. */
// FUNCTION: WIZ8 0x0052a540
int RebuildRealmSpellPointCeilings0052A540(W8Character* character)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
    /* Retail compiled this comparison with VC6's mixed-sign operands; the
   signedness is part of the recovered body and changing it would change
   the compare and branch. Suppress only this diagnostic here. */
    int max_spell_levels[6];
    int realm_skills[4];
    int index;
    int best = 0;

    RecountLearnedSpellsByRealm004F96A0(character);
    for (index = 0; index < 6; ++index) {
        max_spell_levels[index] = 0;
    }
    for (index = 0; index < 0x72; ++index) {
        if (character->spell_learned[index] == 1 || character->spell_learned[index] == 2) {
            int realm = g_spell_records[index].realm;
            int cost = g_spell_records[index].spell_point_cost;
            if (max_spell_levels[realm] < cost) {
                max_spell_levels[realm] = cost;
            }
        }
    }
    for (index = 0; index < 4; ++index) {
        realm_skills[index] = character->skills[0x18 + index].level;
    }
    qsort(realm_skills, 4, 4, CompareUnsignedDescending);

    float weighted = (float)(realm_skills[0] + (realm_skills[1] >> 1) + (realm_skills[2] >> 2) +
                             (realm_skills[3] >> 3));
    if (weighted > 125.0f) {
        weighted = 125.0f;
    }

    for (index = 0; index < 6; ++index) {
        unsigned int old = character->sp_max[index];
        unsigned int learned = character->skill_unlocks[0x1c + index];
        int computed = (int)(((weighted + character->skills[0x1c + index].level * 3 +
                               character->attributes[2].effective) *
                              g_float_005ecbb4) *
                                 (learned + character->uiExpLevel + 1) +
                             g_double_005ebe80);
        if (best < computed) {
            best = computed;
        }
        if (learned == 0) {
            computed = 0;
        }
        character->sp_max[index] = computed;
        if (computed < max_spell_levels[index]) {
            character->sp_max[index] = max_spell_levels[index];
        }
        if (character->sp_max[index] != old) {
            character->iSPLeft[index] += character->sp_max[index] - old;
        }
    }
    return best;
#pragma clang diagnostic pop
}

// FUNCTION: WIZ8 0x0052a760
int SumCharacterSpellPoints(const W8Character* character)
{
    int total = 0;
    for (int realm = 0; realm < W8_SPELL_REALM_COUNT; ++realm) {
        total += character->sp_max[realm];
    }
    return total;
}

/* Occupied slots below the dead condition compete on hit-point percent; the
   weakest one wins. */
// FUNCTION: WIZ8 0x0052C350
unsigned int FindPartySlotWithLowestHitPoints(void)
{
    unsigned int best_slot = 0;
    unsigned int best_percent = 100;

    for (unsigned int slot = 0; slot < 8; ++slot) {
        W8Character* character = &g_status_685170.buffers.Char[slot];
        if (g_status_685170.buffers.XChar[slot].fOccupied != 0 &&
            character->highest_condition < W8_CONDITION_DEAD) {
            unsigned int percent =
                (character->hp_current * 100) / static_cast<unsigned int>(character->uiHPMax);
            if (percent < best_percent) {
                best_slot = slot;
                best_percent = percent;
            }
        }
    }
    return best_slot;
}

/* Same sweep over the spell-point pools: the numerator counts only realms
   with points still remaining, so the member with the lowest remaining
   percentage wins. */
// FUNCTION: WIZ8 0x0052C3B0
unsigned int FindPartySlotWithLowestSpellPoints(void)
{
    unsigned int best_slot = 0;
    unsigned int best_percent = 100;

    for (unsigned int slot = 0; slot < 8; ++slot) {
        W8Character* character = &g_status_685170.buffers.Char[slot];
        if (g_status_685170.buffers.XChar[slot].fOccupied != 0 &&
            character->highest_condition < W8_CONDITION_DEAD) {
            unsigned int pool_max = 0;
            for (int realm = 0; realm < W8_SPELL_REALM_COUNT; ++realm) {
                pool_max += character->sp_max[realm];
            }
            if (pool_max > 0) {
                int pool_left = 0;
                for (int realm = 0; realm < W8_SPELL_REALM_COUNT; ++realm) {
                    int left = character->iSPLeft[realm];
                    pool_left += left & ((left <= 0) - 1);
                }
                unsigned int percent = (unsigned int)(pool_left * 100) / pool_max;
                if (percent < best_percent) {
                    best_percent = percent;
                    best_slot = slot;
                }
            }
        }
    }
    return best_slot;
}

/* Applies one queued fatigue op: a character target fatigues that party
   slot, a monster target fatigues the monster spawned from its location
   id. */
// FUNCTION: WIZ8 0x0052C500
void ApplyQueuedFatigue(W8CombatSlot* op, unsigned int amount, int arg_3)
{
    if (op->iType == W8_TARGET_KIND_CHARACTER) {
        FatigueCharacter(op->iChar, amount, 0, 0);
        return;
    }
    if (op->iType == W8_TARGET_KIND_MONSTER) {
        unsigned int location_index =
            MonsterGetIndexByLocationID(0x771, HEALTH_STAMINA_MANA_CPP, op->iMonsterID, 1);
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(location_index);
        FatigueMonster(monster_info, amount, 0);
    }
}

// FUNCTION: WIZ8 0x0052C480
W8Character* FindPartyMemberWithLowestResistance4(void)
{
    unsigned int lowest = 999;
    int selected = 0;
    for (int party_slot = 0; party_slot < 8; ++party_slot) {
        W8Character* character = &g_status_685170.buffers.Char[party_slot];
        if (g_status_685170.buffers.XChar[party_slot].fOccupied != 0 &&
            character->highest_condition < W8_CONDITION_DEAD &&
            character->resistances[4].total < lowest) {
            selected = party_slot;
            lowest = character->resistances[4].total;
        }
    }
    if (lowest == 999)
        return 0;
    return &g_status_685170.buffers.Char[selected];
}
