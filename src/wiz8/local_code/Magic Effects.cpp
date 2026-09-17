#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/xstatus.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/sr_api.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/utility.h"
#include "random.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_screens/MGSSpellIcons.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/engine_code/GameData.h"

/*
 * Local Code\Magic Effects.cpp.
 *
 * What a running spell effect is worth and how long it lasts, and the two
 * paths that take one off again.
 */

/* The duration that means "for good". */
enum { W8_EFFECT_PERMANENT = 9999 };

/* 0x0060CFFC: eight bytes per effect id, whose leading dword names the
   visual resource; -1 means the effect has none. The table ends where
   the next unrelated data region begins. */
// GLOBAL: WIZ8 0x0060cffc
const int g_effect_visual_table[149][2] = {
    {-1, -1},  {-1, 224}, {34, -1},  {38, -1},  {-1, -1},  {-1, -1},  {-1, -1}, {1, 213},
    {-1, -1},  {-1, -1},  {-1, -1},  {15, -1},  {14, -1},  {-1, -1},  {-1, -1}, {11, -1},
    {-1, 211}, {-1, -1},  {-1, -1},  {-1, 212}, {25, -1},  {-1, -1},  {-1, -1}, {-1, -1},
    {10, -1},  {-1, 215}, {27, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {4, -1},  {13, 209},
    {24, 210}, {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1}, {-1, 214},
    {26, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, 216}, {-1, -1},  {7, -1},  {-1, 219},
    {29, 218}, {28, -1},  {-1, -1},  {-1, -1},  {-1, 225}, {35, -1},  {-1, -1}, {-1, -1},
    {21, -1},  {-1, -1},  {-1, 226}, {36, -1},  {39, -1},  {22, 227}, {37, -1}, {-1, 217},
    {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {8, -1},   {-1, -1}, {-1, -1},
    {-1, -1},  {-1, -1},  {-1, -1},  {-1, 220}, {30, -1},  {-1, -1},  {-1, -1}, {-1, 223},
    {33, 221}, {31, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1}, {-1, -1},
    {-1, -1},  {12, -1},  {-1, -1},  {-1, -1},  {-1, 222}, {32, -1},  {-1, -1}, {-1, -1},
    {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1}, {-1, -1},
    {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1}, {-1, -1},
    {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1}, {-1, -1},
    {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1}, {-1, -1},
    {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1}, {-1, -1},
    {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1}, {-1, -1},
    {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},
};

/* How big the effect lands. A permanent magnitude is taken as it is; anything
   else is scaled by the definition's percentage. */
// FUNCTION: WIZ8 0x00551a20
unsigned int RollEffectMagnitude(W8SpellEffectDefinition* definition)
{
    unsigned int magnitude = RollDice(&definition->magnitude);

    if (magnitude != W8_EFFECT_PERMANENT) {
        AdjustIntegerByPercent(&magnitude, definition->percent);
    }
    return magnitude;
}

/* How long it lasts. The three duration values combine, one is added for the
   turn it starts on, and a quarter of the time one more is added - but only
   at the two shortest durations, so a long effect never gains the extra
   turn. */
// FUNCTION: WIZ8 0x005519c0
unsigned int RollEffectDuration(W8SpellEffectDefinition* definition)
{
    int combined =
        definition->duration_per_power * definition->duration_scale + definition->duration_base;
    unsigned int duration;
    int roll;

    if (combined == W8_EFFECT_PERMANENT) {
        return W8_EFFECT_PERMANENT;
    }
    duration = combined + 1;

    roll = Random(4);
    if (roll == 0) {
        if (duration >= 2) {
            ++duration;
        }
    } else if (roll == 1 && duration <= 2) {
        ++duration;
    }

    AdjustIntegerByPercent(&duration, definition->percent);
    return duration;
}

/* Take the named effect off a monster: say so, lower the flag, and drop the
   visual. */
// FUNCTION: WIZ8 0x005523d0
void ClearMonsterEffect2DE(W8MonsterInfo* monster_info)
{
    if (monster_info->effect_2de != 0) {
        PostMonsterNotice(monster_info, gppStringList[0x6b4 / 4]);
        monster_info->effect_2de = 0;
        SetMonsterSpellIcon(monster_info->monster, SPELL_ICON_CHARMED, 0);
    }
}

/* Empty one effect slot, dropping its visual first if it was running. The
   clear is written field by field rather than as a block, which is what leaves
   the four bytes at 0x09 untouched. */
// FUNCTION: WIZ8 0x005524e0
void ClearEffectSlot(W8MonsterInfo* monster_info, W8EffectSlot* slot)
{
    unsigned char* bytes = (unsigned char*)slot;
    int index;

    if (slot->active != 0) {
        SetMonsterSpellIcon(monster_info->monster, g_effect_visual_table[slot->effect_id][0], 0);
    }
    for (index = 0; index < 9; ++index) {
        bytes[index] = 0;
    }
    for (index = 0xd; index < 0x11; ++index) {
        bytes[index] = 0;
    }
    RebuildMonsterDerivedStats(monster_info->location_id);
}

/* Wipe the party-wide effect block and tell the three displays that read it. */
// FUNCTION: WIZ8 0x005524b0
void ResetPartyEffectBlock(W8EffectSlot* slot)
{
    unsigned char* bytes = (unsigned char*)slot;

    bytes[0] = 0;
    *(int*)(bytes + 1) = 0;
    *(int*)(bytes + 5) = 0;
    *(int*)(bytes + 0xd) = 0;
    RebuildPartyEffectBlock0050E700();
    InvalidateMainGameEffectHud();
    RequestRedraw(0x800100);
}

#define MAGIC_EFFECTS_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Magic Effects.cpp"

/* 0x006172A0: how much harder each condition is to shrug off, added to the
   saving throw. */
// GLOBAL: WIZ8 0x006172a0
const int g_condition_resist_base[W8_CONDITION_COUNT] = {0, 1,  2,  3,  4,  4,  5,  6,  7,  8,
                                                         9, 10, 12, 14, 16, 20, 18, 22, 24, 22};

/* The target's standing against one realm: a character's clamped total, a
   monster's own figure plus its gameplay-modifier bonus. The saving throw and
   the magnitude reduction both start from it, with the target's level set
   against the effect's power level three points a level. */
// FUNCTION: WIZ8 0x00552250
void ReduceMagnitudeByResistance(unsigned int* magnitude, W8CombatSlot* target, int realm,
                                 int power_level)
{
    W8MonsterInfo* monster_info;
    W8MonsterRecord* monster;
    W8Character* character;
    int resistance;
    unsigned int level;
    int chance;
    unsigned int half;
    unsigned int percent;

    if (*magnitude == W8_EFFECT_PERMANENT) {
        return;
    }
    if (target->iType == W8_TARGET_KIND_MONSTER) {
        monster_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0xf18, MAGIC_EFFECTS_CPP, target->iMonsterID, 1));
        monster = GetMonsterDataForInfo(monster_info);
        resistance =
            monster_info->modifiers_1db.resistance_bonus[realm] + monster->resistances[realm];
        level = monster->missile_value_24f;
    } else {
        character = &g_status_685170.buffers.characters[target->iChar];
        resistance = character->resistances[realm].total;
        level = character->level;
    }
    chance = resistance + (level - power_level) * 3;
    if (chance > 0) {
        ++chance;
        half = chance / 2;
        percent = Random(chance - half) + half + Random(half);
        if (percent > 100) {
            percent = 100;
        }
        *magnitude -= (*magnitude * percent) / 100;
    }
}

/* The saving throw. The chance to resist is the realm standing plus the
   condition's own figure plus three a level over the effect's power level,
   scaled for difficulty and held between 5 and 95. A character who knows the
   resistance skill practises it on a success. */
// FUNCTION: WIZ8 0x005520d0
char TargetResistsCondition(W8CombatSlot* target, int realm, unsigned int power_level,
                            int condition_id)
{
    W8MonsterInfo* monster_info;
    W8MonsterRecord* monster;
    W8Character* character;
    int resistance;
    unsigned int level;
    unsigned int highest_condition;
    int chance;

    if (target->iType == W8_TARGET_KIND_MONSTER) {
        monster_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0xec6, MAGIC_EFFECTS_CPP, target->iMonsterID, 1));
        monster = GetMonsterDataForInfo(monster_info);
        resistance =
            monster_info->modifiers_1db.resistance_bonus[realm] + monster->resistances[realm];
        level = monster->missile_value_24f;
        highest_condition = monster_info->highest_condition;
    } else {
        if (target->iType != W8_TARGET_KIND_CHARACTER) {
            srAssertFail("pTarget->iType == TARGET_TYPE_CHAR", MAGIC_EFFECTS_CPP, 0xecf, 0);
        }
        if (target->iChar == -1) {
            srAssertFail("pTarget->iChar != BAD_INDEX", MAGIC_EFFECTS_CPP, 0xed0, 0);
        }
        character = &g_status_685170.buffers.characters[target->iChar];
        resistance = character->resistances[realm].total;
        level = character->level;
        highest_condition = character->highest_condition;
    }
    if (highest_condition >= W8_CONDITION_DEAD) {
        return 1;
    }

    chance = (level - power_level) * 3 + g_condition_resist_base[condition_id] + resistance;
    if (target->iType == W8_TARGET_KIND_MONSTER) {
        ScaleValueForMonsterDifficulty(monster_info, &chance);
    } else {
        ScaleValueForCharacterDifficulty(target->iChar, &chance);
    }
    ClampInteger(&chance, 5, 95);
    if (static_cast<int>(Random(100)) < chance) {
        if (target->iType == W8_TARGET_KIND_CHARACTER) {
            character = &g_status_685170.buffers.characters[target->iChar];
            if (character->skills[W8_RESISTANCE_BONUS_SKILL].flag_00 != 0) {
                PracticeCharacterSkill(character, W8_RESISTANCE_BONUS_SKILL, 2, 0);
            }
        }
        return 1;
    }
    return 0;
}

/* Land a condition the target failed to resist. The magnitude, cut down by
   resistance, becomes how long the condition runs; nothing lands from zero.
   Condition five first spends itself against enchantment slot five, and is
   announced as absorbed if the slot swallows all of it. A single turn is
   raised to two. Monsters always count as hit; a character's answer comes from
   SetCharacterCondition. */
// FUNCTION: WIZ8 0x00551eb0
char InflictConditionOnTarget(W8CombatSlot* target, int condition_id, int realm,
                              unsigned int power_level, int argument, unsigned int magnitude,
                              int source_character, int duration, char announce)
{
    W8TargetSource source;
    W8MonsterInfo* monster_info;
    unsigned int absorbed;
    int remaining;
    char result = 0;

    ResetTargetSource(&source);
    ReduceMagnitudeByResistance(&magnitude, target, realm, power_level);
    if (magnitude > 0) {
        result = 1;
        if (condition_id == 5) {
            if (target->iType == W8_TARGET_KIND_CHARACTER) {
                absorbed =
                    g_status_685170.buffers.characters[target->iChar].enchantments[5].value_08;
                if (absorbed > 0) {
                    remaining = magnitude - absorbed;
                    TickCharacterEnchantmentSlot(target->iChar, 5, magnitude);
                    if (remaining > 0) {
                        magnitude = remaining;
                    } else {
                        if (announce != 0) {
                            PostCharacterNotice(
                                target->iChar, L"%s!",
                                gppStringList[g_condition_notices_0061E570[5 * 4 + 1]]);
                        }
                        return 1;
                    }
                }
            } else {
                monster_info = MonsterInfoFromID(0xe76, MAGIC_EFFECTS_CPP, target->iMonsterID, 1);
                absorbed = monster_info->enchantments[5].value_08;
                if (absorbed > 0) {
                    remaining = magnitude - absorbed;
                    TickMonsterEnchantmentSlot(target->iMonsterID, 5, magnitude);
                    if (remaining > 0) {
                        magnitude = remaining;
                    } else {
                        if (announce != 0) {
                            WriteGameLogAmount(
                                9, L"%s %s!", GetMonsterName(monster_info, 0, 0),
                                gppStringList[g_condition_notices_0061E570[5 * 4 + 1]]);
                        }
                        return 1;
                    }
                }
            }
        }
        if (magnitude == 1) {
            magnitude = 2;
        }
        if (target->iType == W8_TARGET_KIND_MONSTER) {
            SetMonsterCondition(target->iMonsterID, condition_id, magnitude, argument, &source,
                                announce);
        } else {
            result = SetCharacterCondition(target->iChar, condition_id, magnitude, argument,
                                           duration, announce);
        }
    }
    return result;
}

/* Say that whoever was aimed at shrugged the effect off. A monster target is
   told through the monster notice path and anything else through the
   character one, which is what splits the two here. */
// FUNCTION: WIZ8 0x00552070
void AnnounceEffectResisted(W8CombatSlot* target)
{
    if (g_settings_6850c8.verbose_combat_messages == 0) {
        return;
    }
    if (target->iType == W8_TARGET_KIND_MONSTER) {
        PostMonsterNotice(MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                              3758, MAGIC_EFFECTS_CPP, target->iMonsterID, 1)),
                          gppStringList[0x6cc / 4]);
        return;
    }
    PostCharacterNotice(target->iChar, gppStringList[0x6cc / 4]);
}

/* Apply an effect and say so if it did not take. Only a zero result counts as
   shrugged off; the announcement is the same body as its neighbour written out
   again rather than called. */
// FUNCTION: WIZ8 0x00552340
void ApplyEffectAndAnnounce(unsigned int* result, W8CombatSlot* target, int realm, int power_level)
{
    ReduceMagnitudeByResistance(result, target, realm, power_level);
    if (*result != 0 || g_settings_6850c8.verbose_combat_messages == 0) {
        return;
    }
    if (target->iType == W8_TARGET_KIND_MONSTER) {
        PostMonsterNotice(MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                              3758, MAGIC_EFFECTS_CPP, target->iMonsterID, 1)),
                          gppStringList[0x6cc / 4]);
        return;
    }
    PostCharacterNotice(target->iChar, gppStringList[0x6cc / 4]);
}

/* Which of the seven display slots one condition owns. Anything not among the
   seven is a caller error rather than a missing slot. */
// FUNCTION: WIZ8 0x00551900
int GetConditionDisplaySlot(int condition)
{
    switch (condition) {
    case 0x13:
        return 1;
    case 0x15:
        return 2;
    case 0x1b:
        return 3;
    case 0x36:
        return 4;
    case 0x38:
        return 5;
    case 0x3d:
        return 6;
    case 0x41:
        return 7;
    default:
        srAssertFail("FALSE", MAGIC_EFFECTS_CPP, 3339, 0);
        return 0;
    }
}

/* One queued spell effect. Only the source block matters here; the assertion
   at Magic Effects.cpp:2685 names it pQueue->Source, and the recall reads the
   caster out of it. */
struct W8SpellQueueEntry {
    unsigned char unknown_00[0x5c];
    W8TargetSource Source; /* 0x5c */
};

/* Return the casting character to the CamPos spell 0x4b stored. Nothing
   happens unless the anchor was ever set. On the same level RestoreWorldCameraState
   applies the full pose and the renderer is told to catch up; on any other
   level the 0x3c-byte record is staged into pending_move_location for LoadLevel. */
// FUNCTION: WIZ8 0x005507d0
void RecallCasterToSavedLocation(W8SpellQueueEntry* pQueue)
{
    W8Character* caster;
    srVector3T<float> point;

    if (!TargetSourceIsCharacter(&pQueue->Source, 0)) {
        srAssertFail("SourceIsCharacter(&(pQueue->Source))", MAGIC_EFFECTS_CPP, 2685, 0);
    }
    caster = &g_status_685170.buffers.characters[pQueue->Source.iChar];
    if (caster->has_saved_location != 0) {
        if (caster->saved_level == g_status_685170.current_level) {
            RestoreWorldCameraState(GetWorld(), GetWorld659AB8(), &caster->saved_location);
            point = caster->saved_location.position;
            PlacePartyAtPoint(&point);
            MarkRendererReady();
            return;
        }
        g_status_685170.pending_move_location = caster->saved_location;
        g_level_block->pending_level =
            g_status_685170.buffers.characters[pQueue->Source.iChar].saved_level;
        g_level_block->pending_entry_id = -1;
        BeginLevelTransition();
    }
}

/* Whether anything holds the screen busy: combat, a modal, the trigger flag,
   or a current state past the idle slot all answer yes; otherwise the idle
   check decides. */
// FUNCTION: WIZ8 0x00554540
unsigned char IsScreenBusy(void)
{
    if (gXStatus.fCombatMode != 0) {
        return 1;
    }
    if (IsModalOpen()) {
        return 1;
    }
    if (g_flag_0068506e != 0) {
        return 1;
    }
    if (g_current_screen_state.id != 7) {
        return 1;
    }
    return !IsScreenIdle();
}

// FUNCTION: WIZ8 0x00551a60
void RecalculateCharacterResistances(W8Character* character)
{
    unsigned int index;
    int channel;
    unsigned int adjustment;

    for (index = 0; index < W8_RESISTANCE_COUNT; ++index) {
        W8CharacterResistance* resistance = &character->resistances[index];

        resistance->base = 25;
        resistance->base = character->skills[W8_FIRST_RESISTANCE_SKILL + index].level / 10 + 25;
        if (character->skills[W8_RESISTANCE_BONUS_SKILL].flag_00 != 0) {
            resistance->base += character->skills[W8_RESISTANCE_BONUS_SKILL].level / 5 + 5;
        }
        if (character->current_profession == 14) {
            resistance->base += 5;
        }
    }

    if (character->race != -1) {
        for (index = 0; index < W8_RESISTANCE_COUNT; ++index) {
            channel =
                g_race_resistance_profiles[character->race].adjustments[index].resistance_index;
            if (channel == -1) {
                break;
            }
            adjustment = g_race_resistance_profiles[character->race]
                             .adjustments[index]
                             .adjustment_or_attribute;
            if (static_cast<int>(adjustment) > W8_RACE_ADJUSTMENT_ATTRIBUTE_BIAS) {
                adjustment =
                    character->attributes[adjustment - W8_RACE_ADJUSTMENT_ATTRIBUTE_BIAS].value / 5;
            }
            character->resistances[channel].base += adjustment;
        }
    }

    if (character->attributes[1].effective > 0x50) {
        character->resistances[4].base += (character->attributes[1].effective - 0x50) >> 1;
    }
    if (character->attributes[2].effective > 0x50) {
        character->resistances[5].base += (character->attributes[2].effective - 0x50) >> 1;
    }

    for (index = 0; index < W8_RESISTANCE_COUNT; ++index) {
        W8CharacterResistance* resistance = &character->resistances[index];

        resistance->total = resistance->base;
        resistance->total = resistance->base + character->bonus_1770.resistance_bonus_all;
        resistance->total += character->bonus_1770.resistance_bonus[index];
    }
    for (index = 0; index < W8_RESISTANCE_COUNT; ++index) {
        if (character->resistances[index].total > 100) {
            character->resistances[index].total = 100;
        }
    }
}

// FUNCTION: WIZ8 0x00551BA0
char ResolveAttackOnTarget00551BA0(const W8TargetSource* source, W8CombatSlot* target,
                                   int condition_id, int realm, unsigned int power_level,
                                   int argument, int magnitude, char announce_resistance,
                                   char announce_condition, int duration)
{
    W8Character* character;
    W8MonsterInfo* monster_info;
    W8MonsterRecord* monster;
    unsigned int highest_condition;
    unsigned int index;
    int source_character;
    char resolved;

    if (target->iType == W8_TARGET_KIND_CHARACTER) {
        character = &g_status_685170.buffers.characters[target->iChar];
        highest_condition = character->highest_condition;
    } else {
        monster_info = MonsterInfoFromID(0xdc9, MAGIC_EFFECTS_CPP, target->iMonsterID, 1);
        monster = GetMonsterDataForInfo(monster_info);
        if (monster->unknown_1be[0] != 0 && condition_id == W8_CONDITION_DEAD) {
            return 1;
        }
        W8ConditionImmunity* immunity = g_condition_immunities_006171A8;
        for (; immunity < g_condition_immunities_006171A8 + 3; ++immunity) {
            if (monster->kind_0cb != immunity->kind) {
                continue;
            }
            for (index = 0; index < W8_CONDITION_COUNT; ++index) {
                if (condition_id == immunity->conditions[index]) {
                    return 1;
                }
            }
        }
        highest_condition = monster_info->highest_condition;
    }

    if (highest_condition >= W8_CONDITION_DEAD) {
        return 1;
    }
    if (condition_id == 0x13) {
        if (target->iType == W8_TARGET_KIND_MONSTER) {
            return 1;
        }
        if (g_status_685170.buffers.characters[target->iChar].condition_turns[0x13] != 0) {
            return 1;
        }
    }

    if (TargetResistsCondition(target, realm, power_level, condition_id) != 0) {
        resolved = 1;
    } else {
        resolved = 0;
        if (target->iType == W8_TARGET_KIND_CHARACTER) {
            switch (condition_id) {
            case 6:
                if (CharacterHasTrait00547940(character, 3) != 0) {
                    if (announce_resistance != 0) {
                        PostCharacterNotice(target->iChar, gppStringList[0x600 / 4]);
                    }
                    return 1;
                }
            case 2:
            case 3:
            case 4:
            case 7:
            case W8_CONDITION_ASLEEP:
                if (CharacterHasTrait00547940(character, 0x1e) != 0) {
                    return 1;
                }
                break;
            case 0xb:
                if (duration == W8_EFFECT_PERMANENT) {
                    break;
                }
            case 0xd:
                if (CharacterHasTrait00547940(character, 0xe) != 0) {
                    if (announce_resistance != 0) {
                        PostCharacterNotice(
                            target->iChar, gppStringList[0x604 / 4],
                            gppStringList[g_condition_notices_0061E570[condition_id * 4] * 4]);
                    }
                    return 1;
                }
                break;
            }
        }

        source_character = -1;
        if (target->iType == W8_TARGET_KIND_MONSTER && TargetSourceIsCharacter(source, 0) != 0) {
            source_character = source->iChar;
        }
        resolved =
            InflictConditionOnTarget(target, condition_id, realm, power_level, argument, magnitude,
                                     source_character, duration, announce_condition) == 0;
    }

    if (resolved != 0 && announce_resistance != 0 &&
        g_settings_6850c8.verbose_combat_messages != 0) {
        if (target->iType == W8_TARGET_KIND_MONSTER) {
            unsigned int monster_index =
                MonsterGetIndexByLocationID(0xeae, MAGIC_EFFECTS_CPP, target->iMonsterID, 1);
            PostMonsterNotice(MonsterGetScriptPartByLocationIndex(monster_index),
                              gppStringList[0x6cc / 4]);
        } else {
            PostCharacterNotice(target->iChar, gppStringList[0x6cc / 4]);
        }
    }
    return resolved;
}
