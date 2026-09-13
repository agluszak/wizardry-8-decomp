#include "wiz8/local_code/GroupAttacks.h"
#include "wiz8/character.h"
#include "wiz8/game_status.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/MonsterAI.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/Sight.h"
#include "wiz8/notices.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/sr_api.h"
#include "wiz8/targeting.h"
#include "wiz8/utility.h"

#include "random.h"

#define GROUP_ATTACKS_CPP "C:\\Projects\\Wizardry 8\\Local Code\\GroupAttacks.cpp"

/* The per-kind condition each attack slot tries to inflict, indexing the same
   rows as g_condition_notices_0061E570. */
// GLOBAL: WIZ8 0x0061EFFC
const int g_ai_kind_condition_table_61effc[32][2] = {
    {0, 0},  {0, 0}, {0, 0}, {12, 6}, {11, 13}, {0, 0}, {0, 3}, {0, 0}, {0, 0}, {0, 0},  {0, 0},
    {0, 0},  {0, 0}, {0, 0}, {0, 0},  {0, 0},   {0, 0}, {0, 0}, {0, 0}, {0, 4}, {0, 16}, {0, 7},
    {0, 15}, {0, 5}, {0, 0}, {0, 0},  {0, 0},   {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
};

/* The per-kind realm/effect id handed to ApplyEffectAndAnnounce and the
   realm drain. */
// GLOBAL: WIZ8 0x0061F0FC
const int g_ai_kind_realm_table_61f0fc[32] = {
    0, 0, 0, 0, 5, 4, 1, 3, 1, 0, 0, 2, 4, 1, 0, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

/* Resolve one kind-table entry against every character and monster target in
   the group's attack. The per-slot bound comes from the attacker's current
   stamina fraction - a character source scales off level and stamina, a
   monster source off its missile value and current runtime stat - and a kind
   five table doubles it. Kind six delegates to the summon. */
// FUNCTION: WIZ8 0x005560A0
void ResolveMonsterGroupAttack005560A0(int iAIKind, W8TargetSource* pSource,
                                       W8CombatSlot* pAttackerSlot, int arg_4, int iNumCharTargets,
                                       int arg_6, int* piCharTargets, int arg_8,
                                       int iNumMonsterTargets, int arg_10, int* piMonsterTargets)
{
    char announce;
    int iTarget;
    int iChar;
    int iMonsterID;
    int i;
    int drain;
    W8MonsterInfo* monster_info;
    W8MonsterRecord* record;
    W8CombatSlot target;
    unsigned int uiHits[2];
    unsigned int uiTotals[2];
    unsigned int uiBounds[2];
    unsigned int uiMinRoll;
    unsigned int uiBound;
    unsigned int uiDamage;
    unsigned int uiRollA;
    unsigned int uiRollB;
    unsigned int uiRollC;
    int extra;
    char resolved;
    wchar_t* text;

    announce = (char)g_settings_6850c8.verbose_combat_messages;
    if (pSource->iType == W8_TARGET_SOURCE_CHARACTER) {
        unsigned int level;
        unsigned int bound;
        unsigned int stamina;
        unsigned int stamina_max;

        iChar = pSource->iChar;
        level = g_status_685170.buffers.characters[iChar].level;
        bound = 0xf;
        if (level < 0x10) {
            bound = level;
        }
        stamina = g_status_685170.buffers.characters[iChar].stamina;
        stamina_max = g_status_685170.buffers.characters[iChar].stamina_max;
        uiMinRoll = (stamina * (level + bound)) / stamina_max;
        if (uiMinRoll == 0) {
            uiMinRoll = 1;
        }
        uiBounds[1] = (stamina * (level + bound + 5)) / stamina_max;
        if (uiBounds[1] == 0) {
            uiBounds[1] = 1;
        }
        uiBounds[0] = uiBounds[1];
    } else if (pSource->iType == W8_TARGET_SOURCE_MONSTER) {
        unsigned int value;

        monster_info = MonsterInfoFromID(0xb0, GROUP_ATTACKS_CPP, pSource->iMonsterID, 1);
        record = GetMonsterDataForInfo(monster_info);
        if (record->missile_value_24f < 0x10) {
            value = record->missile_value_24f;
        } else {
            value = 0xf;
        }
        uiMinRoll = ((unsigned int)monster_info->runtime_stat_current_33 *
                     (record->missile_value_24f + value)) /
                    (unsigned int)monster_info->runtime_stat_max_2f;
        if (uiMinRoll == 0) {
            uiMinRoll = 1;
        }
        uiBounds[0] = ((unsigned int)monster_info->runtime_stat_current_33 *
                       (record->missile_value_24f + value + 5)) /
                      (unsigned int)monster_info->runtime_stat_max_2f;
        if (uiBounds[0] == 0) {
            uiBounds[0] = 1;
        }
        uiBounds[1] = uiBounds[0];
        if (g_ai_kind_table[0][0] == 5) {
            uiBounds[0] = uiBounds[0] * 2;
        } else if (g_ai_kind_table[1][0] == 5) {
            uiBounds[1] = uiBounds[0] * 2;
        }
    }

    if (g_ai_kind_table[iAIKind][0] == 6) {
        SpawnSummonedMonsterGroup00556B10(iAIKind, pSource, pAttackerSlot);
    } else {
        uiHits[0] = 0;
        uiTotals[0] = 0;
        uiHits[1] = 0;
        uiTotals[1] = 0;
        for (iTarget = 0; iTarget < iNumCharTargets; ++iTarget) {
            iChar = piCharTargets[iTarget];
            if (iChar == -1) {
                srAssertFail("iTargetChar != BAD_INDEX", GROUP_ATTACKS_CPP, 0xdc, 0);
            }
            ResetCombatSlot(&target);
            target.iType = W8_TARGET_KIND_CHARACTER;
            target.iChar = iChar;
            for (i = 0; i < 2; ++i) {
                if (i == 0) {
                    if (pSource->iType == W8_TARGET_SOURCE_MONSTER && Random(100) <= 9) {
                        continue;
                    }
                } else if (Random(100) <= 0x31) {
                    continue;
                }
                switch (g_ai_kind_table[iAIKind][i]) {
                case 1:
                    uiBound = uiBounds[i];
                    uiDamage = Random(uiBound);
                    if (g_ai_kind_condition_table_61effc[iAIKind][i] == 7) {
                        extra = Random(uiBound) + 1;
                    } else {
                        extra = 0;
                    }
                    resolved = ResolveAttackOnTarget00551BA0(
                        pSource, &target, g_ai_kind_condition_table_61effc[iAIKind][i],
                        g_ai_kind_realm_table_61f0fc[iAIKind], uiMinRoll, extra, uiDamage, announce,
                        announce, 0);
                    if (resolved == 0 && announce == 0) {
                        uiHits[i] = uiHits[i] + 1;
                    }
                    break;
                case 2:
                    uiBound = uiBounds[i];
                    uiRollA = Random(uiBound);
                    uiRollB = Random(uiBound);
                    uiRollC = Random(uiBound);
                    uiDamage = (uiRollA + uiRollB + uiRollC) / 3 + 1;
                    ApplyEffectAndAnnounce((int*)&uiDamage, &target,
                                           g_ai_kind_realm_table_61f0fc[iAIKind], uiMinRoll);
                    if (uiDamage == 0) {
                        AnnounceEffectResisted(&target);
                    } else {
                        DamageCharacter(iChar, uiDamage, 1);
                        if (announce == 0) {
                            uiTotals[i] = uiTotals[i] + uiDamage;
                            uiHits[i] = uiHits[i] + 1;
                        }
                    }
                    break;
                case 3:
                    uiBound = uiBounds[i];
                    uiRollA = Random(uiBound);
                    uiRollB = Random(uiBound);
                    uiRollC = Random(uiBound);
                    uiDamage = (uiRollA + uiRollB + uiRollC) / 3 + 1;
                    ApplyEffectAndAnnounce((int*)&uiDamage, &target,
                                           g_ai_kind_realm_table_61f0fc[iAIKind], uiMinRoll);
                    if (uiDamage == 0) {
                        AnnounceEffectResisted(&target);
                    } else {
                        for (drain = 6; drain != 0; --drain) {
                            DrainCharacterRealmSpellPoints(
                                iChar, g_ai_kind_realm_table_61f0fc[iAIKind], uiDamage, 1);
                        }
                        if (announce == 0) {
                            uiTotals[i] = uiTotals[i] + uiDamage;
                            uiHits[i] = uiHits[i] + 1;
                        }
                    }
                    break;
                case 4:
                    uiBound = uiBounds[i];
                    uiRollA = Random(uiBound);
                    uiRollB = Random(uiBound);
                    uiRollC = Random(uiBound);
                    uiDamage = uiRollA + uiRollB + 1 + uiRollC;
                    ApplyEffectAndAnnounce((int*)&uiDamage, &target,
                                           g_ai_kind_realm_table_61f0fc[iAIKind], uiMinRoll);
                    if (uiDamage == 0) {
                        AnnounceEffectResisted(&target);
                    } else {
                        FatigueCharacter(iChar, uiDamage, 0, 0);
                        if (announce == 0) {
                            uiTotals[i] = uiTotals[i] + uiDamage;
                            uiHits[i] = uiHits[i] + 1;
                        }
                    }
                    break;
                case 5:
                    uiBound = uiBounds[i];
                    uiRollA = Random(uiBound);
                    uiRollB = Random(uiBound);
                    uiRollC = Random(uiBound);
                    uiDamage = (uiRollA + uiRollB + uiRollC) / 3 + 1;
                    if (iAIKind != 10) {
                        ApplyEffectAndAnnounce((int*)&uiDamage, &target,
                                               g_ai_kind_realm_table_61f0fc[iAIKind], uiMinRoll);
                    }
                    if (uiDamage == 0) {
                        AnnounceEffectResisted(&target);
                    } else {
                        ApplyDamageToCharacter0052A890(iChar, uiDamage, 0, announce, 0, 0, 0);
                        if (announce == 0) {
                            uiTotals[i] = uiTotals[i] + uiDamage;
                            uiHits[i] = uiHits[i] + 1;
                        }
                    }
                    break;
                }
            }
        }
        for (iTarget = 0; iTarget < iNumMonsterTargets; ++iTarget) {
            iMonsterID = piMonsterTargets[iTarget];
            if (iMonsterID == -1) {
                srAssertFail("iTargetMonsterID != BAD_INDEX", GROUP_ATTACKS_CPP, 0x169, 0);
            }
            ResetCombatSlot(&target);
            target.iType = W8_TARGET_KIND_MONSTER;
            target.iMonsterID = iMonsterID;
            monster_info = MonsterInfoFromID(0x170, GROUP_ATTACKS_CPP, iMonsterID, 1);
            for (i = 0; i < 2; ++i) {
                if (i == 0) {
                    if (Random(100) <= 9) {
                        continue;
                    }
                } else if (Random(100) <= 0x31) {
                    continue;
                }
                switch (g_ai_kind_table[iAIKind][i]) {
                case 1:
                    uiBound = 1;
                    if (uiBounds[i] / 3 != 0) {
                        uiBound = uiBounds[i] / 3;
                    }
                    if (g_ai_kind_condition_table_61effc[iAIKind][i] == 7) {
                        extra = Random(2) + 1;
                    } else {
                        extra = 0;
                    }
                    resolved = ResolveAttackOnTarget00551BA0(
                        pSource, &target, g_ai_kind_condition_table_61effc[iAIKind][i],
                        g_ai_kind_realm_table_61f0fc[iAIKind], uiMinRoll, extra, uiBound, announce,
                        announce, 0);
                    if (resolved == 0 && announce == 0) {
                        uiHits[i] = uiHits[i] + 1;
                    }
                    break;
                case 2:
                    uiBound = uiBounds[i];
                    uiRollA = Random(uiBound);
                    uiRollB = Random(uiBound);
                    uiRollC = Random(uiBound);
                    uiDamage = (uiRollA + uiRollB + uiRollC) / 3 + 1;
                    ApplyEffectAndAnnounce((int*)&uiDamage, &target,
                                           g_ai_kind_realm_table_61f0fc[iAIKind], uiMinRoll);
                    if (uiDamage == 0) {
                        AnnounceEffectResisted(&target);
                    } else {
                        Function52BB60(monster_info, uiDamage, pSource, 0, 1, 0, 0, 0);
                        if (announce == 0) {
                            uiTotals[i] = uiTotals[i] + uiDamage;
                            uiHits[i] = uiHits[i] + 1;
                        }
                    }
                    break;
                case 3:
                    uiBound = uiBounds[i];
                    uiRollA = Random(uiBound);
                    uiRollB = Random(uiBound);
                    uiRollC = Random(uiBound);
                    uiDamage = (uiRollA + uiRollB + uiRollC) / 6 + 1;
                    ApplyEffectAndAnnounce((int*)&uiDamage, &target,
                                           g_ai_kind_realm_table_61f0fc[iAIKind], uiMinRoll);
                    if (uiDamage != 0) {
                        monster_info->spell_points_2f9 =
                            monster_info->spell_points_2f9 - (uiDamage >> 1);
                        if (announce == 0) {
                            uiTotals[i] = uiTotals[i] + uiDamage;
                            uiHits[i] = uiHits[i] + 1;
                        }
                    } else {
                        AnnounceEffectResisted(&target);
                    }
                    break;
                case 4:
                    uiBound = uiBounds[i];
                    uiRollA = Random(uiBound);
                    uiRollB = Random(uiBound);
                    uiRollC = Random(uiBound);
                    uiDamage = uiRollA + uiRollB + uiRollC;
                    ApplyEffectAndAnnounce((int*)&uiDamage, &target,
                                           g_ai_kind_realm_table_61f0fc[iAIKind], uiMinRoll);
                    if (uiDamage == 0) {
                        AnnounceEffectResisted(&target);
                    } else {
                        FatigueMonster(monster_info, uiDamage, 0);
                        if (announce == 0) {
                            uiTotals[i] = uiTotals[i] + uiDamage;
                            uiHits[i] = uiHits[i] + 1;
                        }
                    }
                    break;
                case 5:
                    uiBound = uiBounds[i];
                    uiRollA = Random(uiBound);
                    uiRollB = Random(uiBound);
                    uiRollC = Random(uiBound);
                    uiDamage = (uiRollA + uiRollB + uiRollC) / 3 + 1;
                    if (iAIKind != 10) {
                        ApplyEffectAndAnnounce((int*)&uiDamage, &target,
                                               g_ai_kind_realm_table_61f0fc[iAIKind], uiMinRoll);
                    }
                    if (uiDamage == 0) {
                        AnnounceEffectResisted(&target);
                    } else {
                        Function52BB60(monster_info, uiDamage, pSource, 0, announce, 0, 0, 0);
                        if (announce == 0) {
                            uiTotals[i] = uiTotals[i] + uiDamage;
                            uiHits[i] = uiHits[i] + 1;
                        }
                    }
                    break;
                }
            }
        }
        if (announce == 0) {
            if (uiHits[0] == 0) {
                Function5905F0(gppStringList[0x694 / 4], -1);
            } else {
                for (i = 0; i < 2; ++i) {
                    if (uiHits[i] == 0) {
                        continue;
                    }
                    if (i == 1 && uiHits[0] != 0) {
                        Function5905F0(L",", -1);
                    }
                    switch (g_ai_kind_table[iAIKind][i]) {
                    case 1:
                        text = FormatWideString(
                            L"%ld %s", uiHits[i],
                            gppStringList[g_condition_notices_0061E570
                                              [g_ai_kind_condition_table_61effc[iAIKind][i] * 4 +
                                               2]],
                            -1);
                        break;
                    case 2:
                        text = FormatWideString(gppStringList[0x704 / 4], uiHits[i],
                                                uiTotals[i] / uiHits[i], -1);
                        break;
                    case 3:
                        text = FormatWideString(gppStringList[0x708 / 4], uiHits[i],
                                                uiTotals[i] / uiHits[i], -1);
                        break;
                    case 4:
                        text = FormatWideString(gppStringList[0x70c / 4], uiHits[i],
                                                uiTotals[i] / uiHits[i], -1);
                        break;
                    case 5:
                        text = FormatWideString(gppStringList[0x664 / 4], uiHits[i],
                                                uiTotals[i] / uiHits[i], -1);
                        break;
                    default:
                        SetTextBoxMode(1, -1);
                        continue;
                    }
                    Function5905F0(text, -1);
                    SetTextBoxMode(1, -1);
                }
            }
        }
    }
    delete piCharTargets;
    delete piMonsterTargets;
}

/* Kind six: the kind selects the summoned species, the record's group-size
   dice sizes it and the new group is created at the attacker's position,
   then walked in toward where the camera can see it - straight to the slot
   when the attacker is unseen, otherwise up to three camera-near placements.
   Kinds 0x18..0x1d and 0x1f map to fixed species; anything else keeps the
   kind id. */
// FUNCTION: WIZ8 0x00556B10
void SpawnSummonedMonsterGroup00556B10(int iAIKind, W8TargetSource* pSource,
                                       W8CombatSlot* pAttackerSlot)
{
    W8MonsterRecord* record;
    int count;
    srVector3T<float>* position;
    W8MonsterGroup* group;
    W8MonsterInfo* monster_info;
    int disposition;
    char placed;
    float yaw;

    switch (iAIKind) {
    case 0x18:
        iAIKind = 0x99;
        break;
    case 0x19:
        iAIKind = 0x95;
        break;
    case 0x1a:
        iAIKind = 0x96;
        break;
    case 0x1b:
        iAIKind = 0x97;
        break;
    case 0x1c:
        iAIKind = 0x98;
        break;
    case 0x1d:
        iAIKind = 0x94;
        break;
    default:
        break;
    case 0x1f:
        iAIKind = 0xc;
        break;
    }

    record = MonsterDBFromSpecies(iAIKind);
    count = RollDice(&record->group_size_dice_0c1);
    position = &pAttackerSlot->point;
    group = CreateMonsterGroupAt0050F1A0(iAIKind, count, position, 0, 0, 1);
    if (group == 0) {
        srAssertFail("pGroup", GROUP_ATTACKS_CPP, 600, 0);
    }
    MonsterInfoFromID(0x25a, GROUP_ATTACKS_CPP, group->value_9f, 1);
    monster_info = MonsterInfoFromID(0x25b, GROUP_ATTACKS_CPP, pAttackerSlot->iMonsterID, 1);
    if (monster_info->flag_16 == 2) {
        disposition = 2;
    } else {
        disposition = 1;
    }
    Function547570(group, disposition, 0);
    if (monster_info->player_visibility.state_04 == 1) {
        placed = PositionMonsterGroupNearCamera00511050(group, 0.0f, 0.0f, 1);
        if (placed == 0) {
            placed = PositionMonsterGroupNearCamera00511050(group, 1500.0f, 0.0f, 1);
        }
        if (placed == 0) {
            placed = PositionMonsterGroupNearCamera00511050(group, 3000.0f, 0.0f, 1);
        }
    } else {
        yaw = Function4BE5C0(position);
        placed = Function510CC0(group, position, yaw, 1, 0, 0, 0);
    }
    if (placed == 0) {
        RemoveAllGroupMembers(group);
        ShowNotice(9, gppStringList[0x9a8 / 4], -1, -1, 0);
        return;
    }
    RefreshAllSight();
    Function511CE0(group, 0);
    Function50F720(group);
}
