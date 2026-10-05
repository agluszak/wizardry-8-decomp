#include "wiz8/fonts.h"
#include "wiz8/local_code/GroupAttacks.h"
#include "wiz8/layouts/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/MonsterAI.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/CombatHostility.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/Sight.h"
#include "wiz8/notices.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/sr_api.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/utility.h"

#include "random.h"
#include "wiz8/engine_code/PolyPick.h"

#define GROUP_ATTACKS_CPP "C:\\Projects\\Wizardry 8\\Local Code\\GroupAttacks.cpp"

/* The per-special-attack condition each attack slot tries to inflict, indexing the same
   rows as g_condition_notices. */
// GLOBAL: WIZ8 0x0061EFFC
static W8Condition g_special_attack_condition_table[32][2] = {
    {W8_CONDITION_NONE, W8_CONDITION_NONE},        {W8_CONDITION_BLIND, W8_CONDITION_NONE},
    {W8_CONDITION_BLIND, W8_CONDITION_NONE},       {W8_CONDITION_NONE, W8_CONDITION_NONE},
    {W8_CONDITION_NONE, W8_CONDITION_NONE},        {W8_CONDITION_AFRAID, W8_CONDITION_INSANE},
    {W8_CONDITION_AFRAID, W8_CONDITION_TURNCOAT},  {W8_CONDITION_AFRAID, W8_CONDITION_NONE},
    {W8_CONDITION_NONE, W8_CONDITION_NONE},        {W8_CONDITION_NONE, W8_CONDITION_NONE},
    {W8_CONDITION_INSANE, W8_CONDITION_NONE},      {W8_CONDITION_INSANE, W8_CONDITION_TURNCOAT},
    {W8_CONDITION_INSANE, W8_CONDITION_NONE},      {W8_CONDITION_IRRITATED, W8_CONDITION_NONE},
    {W8_CONDITION_IRRITATED, W8_CONDITION_NONE},   {W8_CONDITION_NAUSEATED, W8_CONDITION_NONE},
    {W8_CONDITION_NAUSEATED, W8_CONDITION_NONE},   {W8_CONDITION_PARALYZED, W8_CONDITION_NONE},
    {W8_CONDITION_PARALYZED, W8_CONDITION_NONE},   {W8_CONDITION_POISONED, W8_CONDITION_IRRITATED},
    {W8_CONDITION_POISONED, W8_CONDITION_NONE},    {W8_CONDITION_ASLEEP, W8_CONDITION_NONE},
    {W8_CONDITION_SLOWED, W8_CONDITION_PARALYZED}, {W8_CONDITION_SLOWED, W8_CONDITION_NONE},
    {W8_CONDITION_NONE, W8_CONDITION_NONE},        {W8_CONDITION_NONE, W8_CONDITION_NONE},
    {W8_CONDITION_NONE, W8_CONDITION_NONE},        {W8_CONDITION_NONE, W8_CONDITION_NONE},
    {W8_CONDITION_NONE, W8_CONDITION_NONE},        {W8_CONDITION_NONE, W8_CONDITION_NONE},
    {W8_CONDITION_NONE, W8_CONDITION_NONE},        {W8_CONDITION_NONE, W8_CONDITION_NONE},
};

/* The per-special-attack realm/effect id handed to ApplyEffectAndAnnounce and the
   realm drain. */
// GLOBAL: WIZ8 0x0061F0FC
static W8SpellRealm g_special_attack_realm_table[32] = {
    W8_SPELL_REALM_FIRE,   W8_SPELL_REALM_WATER,  W8_SPELL_REALM_WATER,  W8_SPELL_REALM_DIVINE,
    W8_SPELL_REALM_WATER,  W8_SPELL_REALM_MENTAL, W8_SPELL_REALM_DIVINE, W8_SPELL_REALM_MENTAL,
    W8_SPELL_REALM_FIRE,   W8_SPELL_REALM_FIRE,   W8_SPELL_REALM_MENTAL, W8_SPELL_REALM_DIVINE,
    W8_SPELL_REALM_MENTAL, W8_SPELL_REALM_EARTH,  W8_SPELL_REALM_AIR,    W8_SPELL_REALM_EARTH,
    W8_SPELL_REALM_AIR,    W8_SPELL_REALM_MENTAL, W8_SPELL_REALM_EARTH,  W8_SPELL_REALM_WATER,
    W8_SPELL_REALM_WATER,  W8_SPELL_REALM_AIR,    W8_SPELL_REALM_WATER,  W8_SPELL_REALM_EARTH,
    W8_SPELL_REALM_FIRE,   W8_SPELL_REALM_FIRE,   W8_SPELL_REALM_FIRE,   W8_SPELL_REALM_FIRE,
    W8_SPELL_REALM_FIRE,   W8_SPELL_REALM_FIRE,   W8_SPELL_REALM_WATER,  W8_SPELL_REALM_FIRE,
};

/* Whether a special attack kind still fires while the attacker's spellcasting
   is blocked: kinds 5-7 and 0x19-0x1d minus 0x1b ignore the block. */
// FUNCTION: WIZ8 0x00556050
bool MonsterSpecialAttackHonorsCastingBlock(int special_attack_kind)
{
    switch (special_attack_kind) {
    case 5:
    case 6:
    case 7:
    case 0x19:
    case 0x1a:
    case 0x1c:
    case 0x1d:
        return true;
    }
    return false;
}

static unsigned int RollSpecialAttackSum(unsigned int bound)
{
    unsigned int first = Random(bound);
    unsigned int second = Random(bound);
    unsigned int third = Random(bound);
    return first + second + third;
}

/* Resolve one kind-table entry against every character and monster target in
   the group's attack. The per-slot bound comes from the attacker's current
   stamina fraction - a character source scales off level and stamina, a
   monster source off its missile value and current runtime stat - and a kind
   five table doubles it. Kind six delegates to the summon. */
// FUNCTION: WIZ8 0x005560A0
void ResolveMonsterGroupAttack(int special_attack_kind, W8TargetSource* pSource,
                               W8CombatSlot* pAttackerSlot, W8GrowableVector<int> char_targets,
                               W8GrowableVector<int> monster_targets)
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
    int extra;
    bool resolved;
    wchar_t* text;

    announce = static_cast<char>(g_settings.verbose_combat_messages);
    if (pSource->iType == W8_TARGET_SOURCE_CHARACTER) {
        unsigned int level;
        unsigned int bound;
        unsigned int stamina;
        unsigned int stamina_max;

        iChar = pSource->iChar;
        level = g_status.buffers.Char[iChar].uiExpLevel;
        bound = 0xf;
        if (level < 0x10) {
            bound = level;
        }
        stamina = g_status.buffers.Char[iChar].stamina;
        stamina_max = g_status.buffers.Char[iChar].uiStaminaMax;
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

        monster_info = MonsterInfoFromID(0xb0, GROUP_ATTACKS_CPP, pSource->iMonsterID, true);
        record = GetMonsterDataForInfo(monster_info);
        if (record->effective_level < 0x10) {
            value = record->effective_level;
        } else {
            value = 0xf;
        }
        uiMinRoll = (static_cast<unsigned int>(monster_info->stamina) *
                     (record->effective_level + value)) /
                    static_cast<unsigned int>(monster_info->stamina_max);
        if (uiMinRoll == 0) {
            uiMinRoll = 1;
        }
        uiBounds[0] = (static_cast<unsigned int>(monster_info->stamina) *
                       (record->effective_level + value + 5)) /
                      static_cast<unsigned int>(monster_info->stamina_max);
        if (uiBounds[0] == 0) {
            uiBounds[0] = 1;
        }
        uiBounds[1] = uiBounds[0];
        if (g_special_attack_table[0][0] == 5) {
            uiBounds[0] *= 2;
        } else if (g_special_attack_table[1][0] == 5) {
            uiBounds[1] = uiBounds[0] * 2;
        }
    }

    if (g_special_attack_table[special_attack_kind][0] == 6) {
        SpawnSummonedMonsterGroup(special_attack_kind, pSource, pAttackerSlot);
    } else {
        uiHits[0] = 0;
        uiTotals[0] = 0;
        uiHits[1] = 0;
        uiTotals[1] = 0;
        for (iTarget = 0; iTarget < char_targets.GetCount(); ++iTarget) {
            iChar = *char_targets.GetAt(iTarget);
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
                switch (g_special_attack_table[special_attack_kind][i]) {
                case 1:
                    uiBound = uiBounds[i];
                    uiDamage = Random(uiBound);
                    if (g_special_attack_condition_table[special_attack_kind][i] == 7) {
                        extra = Random(uiBound) + 1;
                    } else {
                        extra = 0;
                    }
                    resolved = ResolveAttackOnTarget(
                        pSource, &target, g_special_attack_condition_table[special_attack_kind][i],
                        g_special_attack_realm_table[special_attack_kind], uiMinRoll, extra,
                        uiDamage, announce, announce, 0);
                    if (!resolved && announce == 0) {
                        ++uiHits[i];
                    }
                    break;
                case 2:
                    uiBound = uiBounds[i];
                    uiDamage = RollSpecialAttackSum(uiBound) / 3 + 1;
                    ApplyEffectAndAnnounce(&uiDamage, &target,
                                           g_special_attack_realm_table[special_attack_kind],
                                           uiMinRoll);
                    if (uiDamage == 0) {
                        AnnounceEffectResisted(&target);
                    } else {
                        DamageCharacter(iChar, uiDamage, 1);
                        if (announce == 0) {
                            uiTotals[i] += uiDamage;
                            ++uiHits[i];
                        }
                    }
                    break;
                case 3:
                    uiBound = uiBounds[i];
                    uiDamage = RollSpecialAttackSum(uiBound) / 3 + 1;
                    ApplyEffectAndAnnounce(&uiDamage, &target,
                                           g_special_attack_realm_table[special_attack_kind],
                                           uiMinRoll);
                    if (uiDamage == 0) {
                        AnnounceEffectResisted(&target);
                    } else {
                        for (drain = 6; drain != 0; --drain) {
                            DrainCharacterRealmSpellPoints(
                                iChar, g_special_attack_realm_table[special_attack_kind], uiDamage,
                                true);
                        }
                        if (announce == 0) {
                            uiTotals[i] += uiDamage;
                            ++uiHits[i];
                        }
                    }
                    break;
                case 4:
                    uiBound = uiBounds[i];
                    uiDamage = RollSpecialAttackSum(uiBound) + 1;
                    ApplyEffectAndAnnounce(&uiDamage, &target,
                                           g_special_attack_realm_table[special_attack_kind],
                                           uiMinRoll);
                    if (uiDamage == 0) {
                        AnnounceEffectResisted(&target);
                    } else {
                        FatigueCharacter(iChar, uiDamage, false, 0);
                        if (announce == 0) {
                            uiTotals[i] += uiDamage;
                            ++uiHits[i];
                        }
                    }
                    break;
                case 5:
                    uiBound = uiBounds[i];
                    uiDamage = RollSpecialAttackSum(uiBound) / 3 + 1;
                    if (special_attack_kind != 10) {
                        ApplyEffectAndAnnounce(&uiDamage, &target,
                                               g_special_attack_realm_table[special_attack_kind],
                                               uiMinRoll);
                    }
                    if (uiDamage == 0) {
                        AnnounceEffectResisted(&target);
                    } else {
                        ApplyDamageToCharacter(iChar, uiDamage, false, announce, false, 0, false);
                        if (announce == 0) {
                            uiTotals[i] += uiDamage;
                            ++uiHits[i];
                        }
                    }
                    break;
                }
            }
        }
        for (iTarget = 0; iTarget < monster_targets.GetCount(); ++iTarget) {
            iMonsterID = *monster_targets.GetAt(iTarget);
            if (iMonsterID == -1) {
                srAssertFail("iTargetMonsterID != BAD_INDEX", GROUP_ATTACKS_CPP, 0x169, 0);
            }
            ResetCombatSlot(&target);
            target.iType = W8_TARGET_KIND_MONSTER;
            target.iMonsterID = iMonsterID;
            monster_info = MonsterInfoFromID(0x170, GROUP_ATTACKS_CPP, iMonsterID, true);
            for (i = 0; i < 2; ++i) {
                if (i == 0) {
                    if (Random(100) <= 9) {
                        continue;
                    }
                } else if (Random(100) <= 0x31) {
                    continue;
                }
                switch (g_special_attack_table[special_attack_kind][i]) {
                case 1:
                    uiBound = 1;
                    if (uiBounds[i] / 3 != 0) {
                        uiBound = uiBounds[i] / 3;
                    }
                    if (g_special_attack_condition_table[special_attack_kind][i] == 7) {
                        extra = Random(2) + 1;
                    } else {
                        extra = 0;
                    }
                    resolved = ResolveAttackOnTarget(
                        pSource, &target, g_special_attack_condition_table[special_attack_kind][i],
                        g_special_attack_realm_table[special_attack_kind], uiMinRoll, extra,
                        uiBound, announce, announce, 0);
                    if (!resolved && announce == 0) {
                        ++uiHits[i];
                    }
                    break;
                case 2:
                    uiBound = uiBounds[i];
                    uiDamage = RollSpecialAttackSum(uiBound) / 3 + 1;
                    ApplyEffectAndAnnounce(&uiDamage, &target,
                                           g_special_attack_realm_table[special_attack_kind],
                                           uiMinRoll);
                    if (uiDamage == 0) {
                        AnnounceEffectResisted(&target);
                    } else {
                        ApplyDamageToMonster(monster_info, uiDamage, pSource, false, 1, 0, 0,
                                             false);
                        if (announce == 0) {
                            uiTotals[i] += uiDamage;
                            ++uiHits[i];
                        }
                    }
                    break;
                case 3:
                    uiBound = uiBounds[i];
                    uiDamage = RollSpecialAttackSum(uiBound) / 6 + 1;
                    ApplyEffectAndAnnounce(&uiDamage, &target,
                                           g_special_attack_realm_table[special_attack_kind],
                                           uiMinRoll);
                    if (uiDamage != 0) {
                        monster_info->spell_points =
                            monster_info->spell_points - (uiDamage >> 1);
                        if (announce == 0) {
                            uiTotals[i] += uiDamage;
                            ++uiHits[i];
                        }
                    } else {
                        AnnounceEffectResisted(&target);
                    }
                    break;
                case 4:
                    uiBound = uiBounds[i];
                    uiDamage = RollSpecialAttackSum(uiBound);
                    ApplyEffectAndAnnounce(&uiDamage, &target,
                                           g_special_attack_realm_table[special_attack_kind],
                                           uiMinRoll);
                    if (uiDamage == 0) {
                        AnnounceEffectResisted(&target);
                    } else {
                        FatigueMonster(monster_info, uiDamage, 0);
                        if (announce == 0) {
                            uiTotals[i] += uiDamage;
                            ++uiHits[i];
                        }
                    }
                    break;
                case 5:
                    uiBound = uiBounds[i];
                    uiDamage = RollSpecialAttackSum(uiBound) / 3 + 1;
                    if (special_attack_kind != 10) {
                        ApplyEffectAndAnnounce(&uiDamage, &target,
                                               g_special_attack_realm_table[special_attack_kind],
                                               uiMinRoll);
                    }
                    if (uiDamage == 0) {
                        AnnounceEffectResisted(&target);
                    } else {
                        ApplyDamageToMonster(monster_info, uiDamage, pSource, false, announce, 0, 0,
                                             false);
                        if (announce == 0) {
                            uiTotals[i] += uiDamage;
                            ++uiHits[i];
                        }
                    }
                    break;
                }
            }
        }
        if (announce == 0) {
            if (uiHits[0] == 0) {
                AppendToLastTextLine(gppStringList[0x1a5], -1);
            } else {
                for (i = 0; i < 2; ++i) {
                    if (uiHits[i] == 0) {
                        continue;
                    }
                    if (i == 1 && uiHits[0] != 0) {
                        AppendToLastTextLine(L",", -1);
                    }
                    switch (g_special_attack_table[special_attack_kind][i]) {
                    case 1:
                        text = FormatWideString(
                            L"%ld %s", uiHits[i],
                            gppStringList
                                [g_condition_notices
                                     [g_special_attack_condition_table[special_attack_kind][i] * 4 +
                                      2]],
                            -1);
                        break;
                    case 2:
                        text = FormatWideString(gppStringList[0x1c1], uiHits[i],
                                                uiTotals[i] / uiHits[i], -1);
                        break;
                    case 3:
                        text = FormatWideString(gppStringList[0x1c2], uiHits[i],
                                                uiTotals[i] / uiHits[i], -1);
                        break;
                    case 4:
                        text = FormatWideString(gppStringList[0x1c3], uiHits[i],
                                                uiTotals[i] / uiHits[i], -1);
                        break;
                    case 5:
                        text = FormatWideString(gppStringList[0x199], uiHits[i],
                                                uiTotals[i] / uiHits[i], -1);
                        break;
                    default:
                        SetTextBoxMode(1, -1);
                        continue;
                    }
                    AppendToLastTextLine(text, -1);
                    SetTextBoxMode(1, -1);
                }
            }
        }
    }
}

/* Kind six: the kind selects the summoned species, the record's group-size
   dice sizes it and the new group is created at the attacker's position,
   then walked in toward where the camera can see it - straight to the slot
   when the attacker is unseen, otherwise up to three camera-near placements.
   Kinds 0x18..0x1d and 0x1f map to fixed species; anything else keeps the
   kind id. */
// FUNCTION: WIZ8 0x00556B10
void SpawnSummonedMonsterGroup(int special_attack_kind, W8TargetSource* pSource,
                               W8CombatSlot* pAttackerSlot)
{
    W8MonsterRecord* record;
    int count;
    srVector3T<float>* position;
    W8MonsterGroup* group;
    W8MonsterInfo* monster_info;
    int disposition;
    bool placed;
    float yaw;

    switch (special_attack_kind) {
    case 0x18:
        special_attack_kind = 0x99;
        break;
    case 0x19:
        special_attack_kind = 0x95;
        break;
    case 0x1a:
        special_attack_kind = 0x96;
        break;
    case 0x1b:
        special_attack_kind = 0x97;
        break;
    case 0x1c:
        special_attack_kind = 0x98;
        break;
    case 0x1d:
        special_attack_kind = 0x94;
        break;
    default:
        break;
    case 0x1f:
        special_attack_kind = 0xc;
        break;
    }

    record = MonsterDBFromSpecies(special_attack_kind);
    count = RollDice(&record->group_size_dice);
    position = &pAttackerSlot->point;
    group = CreateGroup(special_attack_kind, count, position, false, false, true);
    if (group == 0) {
        srAssertFail("pGroup", GROUP_ATTACKS_CPP, 600, 0);
    }
    MonsterInfoFromID(0x25a, GROUP_ATTACKS_CPP, group->leader_location_id, true);
    monster_info = MonsterInfoFromID(0x25b, GROUP_ATTACKS_CPP, pAttackerSlot->iMonsterID, true);
    if (monster_info->ubDisposition == W8_DISPOSITION_FRIENDLY) {
        disposition = 2;
    } else {
        disposition = 1;
    }
    SetMonsterGroupHostility(group, disposition, false);
    if (monster_info->player_visibility.sight_state == W8_SIGHT_SEEN) {
        placed = PositionMonsterGroupNearCamera(group, 0.0f, 0.0f, true);
        if (!placed) {
            placed = PositionMonsterGroupNearCamera(group, 1500.0f, 0.0f, true);
        }
        if (!placed) {
            placed = PositionMonsterGroupNearCamera(group, 3000.0f, 0.0f, true);
        }
    } else {
        yaw = GetCameraFacingYaw(position);
        placed = MoveMonsterGroupToPosition(group, position, yaw, true, false, false, false);
    }
    if (!placed) {
        RemoveAllGroupMembers(group);
        ShowNotice(W8_FONT_PALETTE_RUST, gppStringList[0x26a]);
        return;
    }
    RefreshAllSight();
    SetMonsterGroupNavigatorDirty(group, false);
    MonsterGroupEnterCombat(group);
}
