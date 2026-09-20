#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/PolyPick.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/quad.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/engine_code/3d.h"
#include "wiz8/local_code/Sight.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/CombatHostility.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/xstatus.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/SpellEffect.h"
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
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/gameplay_modifiers.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/startup_world.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/engine_code/quad.h"
#include "wiz8/engine_code/3d.h"
#include "wiz8/engine_code/GrCycle.h"
#include "wiz8/fact_state.h"
#include "wiz8/float_constants.h"
#include "wiz8/geometry.h"
#include "wiz8/local_code/Noise.h"
#include "wiz8/local_code/Sight.h"
#include "soundman.h"

/*
 * Local Code\Magic Effects.cpp.
 *
 * What a running spell effect is worth and how long it lasts, and the two
 * paths that take one off again.
 */

#define MAGIC_EFFECTS_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Magic Effects.cpp"

/* The duration that means "for good". */
enum { W8_EFFECT_PERMANENT = 9999 };

// GLOBAL: WIZ8 0x005ee838
float g_float_005ee838 = 0.7900000214576721f;

/* 0x0060CFF8: eight bytes per effect id. The leading dword is the icon the
   party and combat HUD strips show for the effect (-1 means none); the second
   dword is the visual resource SetMonsterSpellIcon attaches to the monster.
   The array fills the region between g_spellbook_name_ids_60cff0 and
   g_spell_usage_name_ids_60d4a8 exactly. */
// GLOBAL: WIZ8 0x0060cff8
const int g_effect_visual_table[150][2] = {
    {-1, -1},  {-1, -1},  {224, 34}, {-1, 38},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, 1},
    {213, -1}, {-1, -1},  {-1, -1},  {-1, 15},  {-1, 14},  {-1, -1},  {-1, -1},  {-1, 11},
    {-1, -1},  {211, -1}, {-1, -1},  {-1, -1},  {212, 25}, {-1, -1},  {-1, -1},  {-1, -1},
    {-1, 10},  {-1, -1},  {215, 27}, {-1, -1},  {-1, -1},  {-1, -1},  {-1, 4},   {-1, 13},
    {209, 24}, {210, -1}, {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},
    {214, 26}, {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {216, -1}, {-1, 7},   {-1, -1},
    {219, 29}, {218, 28}, {-1, -1},  {-1, -1},  {-1, -1},  {225, 35}, {-1, -1},  {-1, -1},
    {-1, 21},  {-1, -1},  {-1, -1},  {226, 36}, {-1, 39},  {-1, 22},  {227, 37}, {-1, -1},
    {217, -1}, {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, 8},   {-1, -1},  {-1, -1},
    {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {220, 30}, {-1, -1},  {-1, -1},  {-1, -1},
    {223, 33}, {221, 31}, {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},
    {-1, -1},  {-1, 12},  {-1, -1},  {-1, -1},  {-1, -1},  {222, 32}, {-1, -1},  {-1, -1},
    {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},
    {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},
    {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},
    {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},
    {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},
    {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},
    {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},  {-1, -1},
};

/* 0x00616F4C: the monster group the Insanity effect summons - one row of four
   per power level, the column the elemental realm the caster's realm skills
   weighted the roll toward. */
// GLOBAL: WIZ8 0x00616f4c
const int g_insanity_group_ids_00616f4c[8][4] = {
    {24, 23, 21, 22}, {24, 23, 21, 22}, {28, 27, 25, 26}, {32, 31, 29, 30},
    {36, 35, 33, 34}, {40, 39, 37, 38}, {44, 43, 41, 42}, {48, 47, 45, 46},
};

/* 0x0061E208: the affliction spell's weighted pick - the index is the
   condition id, the value its share of a fifty-point roll. */
// GLOBAL: WIZ8 0x0061e208
const int g_affliction_condition_weights_0061e208[W8_CONDITION_COUNT] = {
    0, 0, 0, 0, 10, 0, 10, 0, 0, 5, 0, 5, 5, 1, 0, 10, 3, 0, 1, 0,
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

/* Whether the monster shrugs off an effect of the given power: effect kind
   0x14 and a monster already past the last condition tier always resist,
   otherwise the roll pits effective level against the incoming power, scaled
   by three and boosted by the magic resistance bonus and the stored
   resistance, clamped to the 5..95 band. */
// FUNCTION: WIZ8 0x00552410
bool MonsterResistsSpellEffect(const W8CombatSlot* target, int power)
{
    W8MonsterInfo* monster_info;
    W8MonsterRecord* monster;
    int score;

    monster_info = MonsterGetScriptPartByLocationIndex(
        MonsterGetIndexByLocationID(0xf93, MAGIC_EFFECTS_CPP, target->iMonsterID, 1));
    monster = GetMonsterDataForInfo(monster_info);
    if (monster->kind_0cb == 0x14) {
        return true;
    }
    if (monster_info->highest_condition >= 0x12) {
        return true;
    }
    score = (monster->effective_level_24f - power) * 3 +
            monster_info->modifiers_1db.resistance_bonus[4] + monster->resistances[4];
    ClampInteger(&score, 5, 0x5f);
    return static_cast<int>(Random(100)) < score;
}

/* Empty one effect slot, dropping its visual first if it was running. The
   clear is written field by field rather than as a block, which is what leaves
   the four bytes at 0x09 untouched. */
// FUNCTION: WIZ8 0x005524e0
void ClearEffectSlot(W8MonsterInfo* monster_info, W8EffectSlot* slot)
{
    if (slot->active != 0) {
        SetMonsterSpellIcon(monster_info->monster, g_effect_visual_table[slot->effect_id][1], 0);
    }
    slot->active = false;
    slot->effect_id = 0;
    slot->amount = 0;
    slot->duration_0d = 0;
    RebuildMonsterDerivedStats(monster_info->location_id);
}

/* Clear every live combat effect at battle end: the party blocks get the
   field-by-field wipe plus the rebuild/HUD/redraw trio, and each active
   in-combat monster loses the matching mirrored slot, its visual and a
   derived-stat rebuild. */
// FUNCTION: WIZ8 0x00552530
void ResetCombatEffects(void)
{
    W8MonsterInfo* monster_info;
    W8EffectSlot* slot;
    unsigned int index;
    int i;

    for (i = 0; i < 9; ++i) {
        if (g_combat_state->effect_slots[i].active != 0) {
            g_combat_state->effect_slots[i].active = 0;
            g_combat_state->effect_slots[i].effect_id = 0;
            g_combat_state->effect_slots[i].amount = 0;
            g_combat_state->effect_slots[i].duration_0d = 0;
            RebuildPartyEffectBlock0050E700();
            InvalidateMainGameEffectHud();
            RequestRedraw(0x800100);
        }
        for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
            monster_info = MonsterGetScriptPartByLocationIndex(index);
            if (monster_info->fActive != 0 && monster_info->fInCombat != 0) {
                slot = &monster_info->pCombat->effect_slots_3e[i];
                if (slot->active != 0) {
                    if (slot->active != 0) {
                        SetMonsterSpellIcon(monster_info->monster,
                                            g_effect_visual_table[slot->effect_id][1], 0);
                    }
                    slot->active = 0;
                    slot->effect_id = 0;
                    slot->amount = 0;
                    slot->duration_0d = 0;
                    RebuildMonsterDerivedStats(monster_info->location_id);
                }
            }
        }
    }
    for (i = 0; i < 6; ++i) {
        if (g_combat_state->effect_slots_85a[i].active != 0) {
            g_combat_state->effect_slots_85a[i].active = 0;
            g_combat_state->effect_slots_85a[i].effect_id = 0;
            g_combat_state->effect_slots_85a[i].amount = 0;
            g_combat_state->effect_slots_85a[i].duration_0d = 0;
            RebuildPartyEffectBlock0050E700();
            InvalidateMainGameEffectHud();
            RequestRedraw(0x800100);
        }
        for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
            monster_info = MonsterGetScriptPartByLocationIndex(index);
            if (monster_info->fActive != 0 && monster_info->fInCombat != 0) {
                slot = &monster_info->pCombat->effect_slots_d7[i];
                if (slot->active != 0) {
                    if (slot->active != 0) {
                        SetMonsterSpellIcon(monster_info->monster,
                                            g_effect_visual_table[slot->effect_id][1], 0);
                    }
                    slot->active = 0;
                    slot->effect_id = 0;
                    slot->amount = 0;
                    slot->duration_0d = 0;
                    RebuildMonsterDerivedStats(monster_info->location_id);
                }
            }
        }
    }
}

/* Wipe the party-wide effect block and tell the three displays that read it. */
// FUNCTION: WIZ8 0x005524b0
void ResetPartyEffectBlock(W8EffectSlot* slot)
{
    slot->active = false;
    slot->effect_id = 0;
    slot->amount = 0;
    slot->duration_0d = 0;
    RebuildPartyEffectBlock0050E700();
    InvalidateMainGameEffectHud();
    RequestRedraw(0x800100);
}

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
        level = monster->effective_level_24f;
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
        level = monster->effective_level_24f;
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
                            ShowNoticef(9, L"%s %s!", GetMonsterName(monster_info, 0, 0),
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

/* A heading from a world point toward the nearest live monster, or the
   camera-facing yaw for a hostile disposition - which is also the fallback
   when the extreme-range box query turns up no monster at all. */
// FUNCTION: WIZ8 0x0054ff20
float HeadingTowardNearestMonster(srVector3T<float> point, char disposition, int exclusion)
{
    W8Monster* nearest;
    W8Monster* monster;
    srVector3T<float> lower;
    srVector3T<float> upper;
    srVector3T<float> position;
    unsigned long* location_ids;
    double nearest_distance;
    float distance;
    float range;
    unsigned int count;
    unsigned int index;

    nearest = 0;
    nearest_distance = 0.0;
    lower = point;
    upper = point;
    range = CalcRangeDistance(W8_RANGE_EXTREME);
    if (disposition == W8_DISPOSITION_HOSTILE) {
        return HeadingToTargetCPP(&point);
    }
    lower.x -= range;
    lower.y -= range;
    lower.z -= range;
    upper.x += range;
    upper.y += range;
    upper.z += range;
    location_ids = static_cast<unsigned long*>(operator new(0x400));
    count = g_octree_6598a4->QueryLocationsInBox(&location_ids, &lower, &upper,
                                                 static_cast<unsigned short>(exclusion));
    if (count == 0) {
        operator delete(location_ids);
        return HeadingToTargetCPP(&point);
    }
    for (index = 0; index < count; ++index) {
        monster = GetMonsterByLocationID(location_ids[index]);
        position = monster->GetPosition();
        distance = sqrtf((position.x - point.x) * (position.x - point.x) +
                         (position.y - point.y) * (position.y - point.y) +
                         (position.z - point.z) * (position.z - point.z));
        if (distance < nearest_distance || index == 0) {
            nearest_distance = distance;
            nearest = monster;
        }
    }
    operator delete(location_ids);
    position = nearest->GetPosition();
    return GetHeadingAngle(&point, &position);
}

/* The Insanity effect, spell 0x3c: a hostile monster's phantom double is
   summoned to draw attacks away from the party. When the target already has
   the bound-monster record set the effect only reports; otherwise the caster's
   four elemental realm skills (plus a base twenty each) weight which of the
   four phantom kinds appears, a turncoated or backfired source flips the
   phantom's disposition, and on a secondary roll the new group also picks up
   the matching summon enchantment. */
// FUNCTION: WIZ8 0x005500c0
void ApplyInsanityEffect(W8SpellEffectEntry* effect)
{
    W8MonsterInfo* target_info;
    W8MonsterInfo* summon;
    W8MonsterInfo* member;
    W8MonsterGroup* group;
    W8Character* caster;
    W8SpellEffectDefinition attack_block;
    W8EffectSlot* effect_slot;
    unsigned int weights[4];
    unsigned int duration;
    unsigned int roll;
    W8Disposition disposition;
    int spell_id;
    int slot;
    int tier;
    unsigned int index;

    if (TargetSourceIsCharacter(&effect->Source, 0) &&
        GetConditionRecordFlag(effect->Source.iChar, 0) != 0) {
        if (g_settings_6850c8.verbose_combat_messages != 0) {
            PostCharacterNotice(effect->Source.iChar, gppStringList[0x6a0 / 4]);
        } else {
            AppendToLastTextLine(L" -- ", -1);
            SetTextBoxMode(1, -1);
            AppendToLastTextLine(gppStringList[0x6a0 / 4], -1);
        }
        effect->result_124.reported = 1;
        return;
    }
    if (TargetSourceIsMonster(&effect->Source, 0)) {
        target_info = MonsterInfoFromID(0x9b0, MAGIC_EFFECTS_CPP, effect->Source.iMonsterID, 1);
        if (target_info->insanity_summon_344 != -1) {
            return;
        }
    }
    weights[0] = 20;
    weights[1] = 20;
    weights[2] = 20;
    weights[3] = 20;
    if (TargetSourceIsCharacter(&effect->Source, 0)) {
        caster = &g_status_685170.buffers.characters[effect->Source.iChar];
        weights[0] = caster->skills[28].level + 20;
        weights[1] = caster->skills[29].level + 20;
        weights[2] = caster->skills[30].level + 20;
        weights[3] = caster->skills[31].level + 20;
    }
    if (g_camera_sway_active_652da4) {
        weights[0] = 0;
    }
    roll = Random(weights[0] + weights[1] + weights[2] + weights[3]);
    tier = 0;
    for (index = 0; index < 4; ++index) {
        if (roll < weights[index]) {
            tier = index;
            break;
        }
        roll -= weights[index];
    }
    if (TargetSourceIsCharacter(&effect->Source, 0)) {
        disposition =
            (g_status_685170.buffers.characters[effect->Source.iChar].condition_turns[13] == 0) + 1;
    } else if (TargetSourceIsMonster(&effect->Source, 0)) {
        disposition = target_info->ubDisposition;
    } else {
        disposition = W8_DISPOSITION_HOSTILE;
    }
    if (effect->Source.fBackfire != 0) {
        disposition = (disposition == W8_DISPOSITION_HOSTILE) + 1;
    }
    group = CreateGroup(g_insanity_group_ids_00616f4c[effect->definition.duration_scale][tier], 1,
                        &effect->target.point, 1, 0, 1);
    if (group == 0) {
        srAssertFail("pGroup", MAGIC_EFFECTS_CPP, 0xa0c, 0);
    }
    summon = MonsterInfoFromID(0xa0e, MAGIC_EFFECTS_CPP, group->value_9f, 1);
    summon->monster->SetAngles004538F0(
        HeadingTowardNearestMonster(effect->target.point, disposition, summon->location_id));
    SetMonsterGroupHostility(group, disposition, 0);
    if (disposition == W8_DISPOSITION_FRIENDLY) {
        for (index = 0; index < group->member_count; ++index) {
            member =
                MonsterInfoFromID(0xfdb, MAGIC_EFFECTS_CPP, IListGetAt(group->monsters, index), 1);
            if (member == 0) {
                srAssertFail("pMonsterInfo", MAGIC_EFFECTS_CPP, 0xfdc, 0);
            }
            if (member->summoned_2da == 0) {
                member->summoned_2da = 1;
                SetMonsterSpellIcon(member->monster, 0x27, 1);
            }
        }
    } else {
        for (index = 0; index < group->member_count; ++index) {
            member =
                MonsterInfoFromID(0xfdb, MAGIC_EFFECTS_CPP, IListGetAt(group->monsters, index), 1);
            if (member == 0) {
                srAssertFail("pMonsterInfo", MAGIC_EFFECTS_CPP, 0xfdc, 0);
            }
            if (member->summoned_2da == 0) {
                member->summoned_2da = 2;
                SetMonsterSpellIcon(member->monster, 0x27, 1);
            }
        }
    }
    RefreshAllSight();
    if (TargetSourceIsCharacter(&effect->Source, 0)) {
        BindMonsterToCharacterDependence(effect->Source.iChar, 0, summon->location_id);
    } else if (TargetSourceIsMonster(&effect->Source, 0)) {
        target_info->insanity_summon_344 = summon->location_id;
    }
    if (effect->definition.percent != 0 && Random(100) < effect->definition.percent) {
        ClearAttackBlock(&attack_block);
        attack_block.duration_scale = effect->definition.duration_scale;
        switch (tier) {
        case 0:
        case 1:
            spell_id = tier == 1 ? 0x3d : 0x38;
            attack_block.duration_base = g_spell_records[spell_id].duration_per_level_04d;
            attack_block.duration_per_power = g_spell_records[spell_id].duration_044;
            duration = attack_block.duration_per_power * attack_block.duration_scale +
                       attack_block.duration_base;
            if (duration != 9999) {
                duration = duration + 1;
                {
                    unsigned int roll = Random(4);
                    if (roll == 0) {
                        if (1 < duration) {
                            duration = duration + 1;
                        }
                    } else if (roll == 1 && duration < 3) {
                        duration = duration + 1;
                    }
                }
                AdjustIntegerByPercent(&duration, attack_block.percent);
            }
            switch (spell_id) {
            case 0x13:
                slot = 1;
                break;
            case 0x15:
                slot = 2;
                break;
            case 0x1b:
                slot = 3;
                break;
            case 0x36:
                slot = 4;
                break;
            case 0x38:
                slot = 5;
                break;
            case 0x3d:
                slot = 6;
                break;
            case 0x41:
                slot = 7;
                break;
            default:
                srAssertFail("FALSE", MAGIC_EFFECTS_CPP, 0xd0b, 0);
                slot = 0;
            }
            ApplyMonsterCondition005242B0(summon->location_id, slot, attack_block.duration_scale,
                                          duration, 0);
            break;
        case 2:
        case 3:
            spell_id = tier == 2 ? 0x1a : 0x20;
            attack_block.duration_base = g_spell_records[spell_id].duration_per_level_04d;
            attack_block.duration_per_power = g_spell_records[spell_id].duration_044;
            for (index = 0; index < 12; ++index) {
                if (g_being_effect_slot_spells_00616d84[index] == spell_id) {
                    duration = attack_block.duration_per_power * attack_block.duration_scale +
                               attack_block.duration_base;
                    if (duration != 9999) {
                        duration = duration + 1;
                        {
                            unsigned int roll = Random(4);
                            if (roll == 0) {
                                if (1 < duration) {
                                    duration = duration + 1;
                                }
                            } else if (roll == 1 && duration < 3) {
                                duration = duration + 1;
                            }
                        }
                        AdjustIntegerByPercent(&duration, attack_block.percent);
                    }
                    effect_slot = &summon->effect_slots_10f[index];
                    if (effect_slot->active == 0 || effect_slot->effect_id != spell_id) {
                        SetMonsterSpellIcon(summon->monster, g_effect_visual_table[spell_id][1], 1);
                    }
                    effect_slot->active = 1;
                    effect_slot->effect_id = spell_id;
                    effect_slot->amount = attack_block.duration_scale;
                    effect_slot->duration_0d = duration;
                    RebuildMonsterDerivedStats(summon->location_id);
                    break;
                }
            }
            break;
        }
    }
    SetMonsterGroupNavigatorDirty(group, 0);
    MonsterGroupEnterCombat(group);
    effect->result_124.applied = 1;
}

/* Return the casting character to the CamPos spell 0x4b stored. Nothing
   happens unless the anchor was ever set. On the same level RestoreWorldCameraState
   applies the full pose and the renderer is told to catch up; on any other
   level the 0x3c-byte record is staged into pending_move_location for LoadLevel. */
// FUNCTION: WIZ8 0x005507d0
void RecallCasterToSavedLocation(W8SpellEffectEntry* pQueue)
{
    W8Character* caster;
    srVector3T<float> point;

    if (!TargetSourceIsCharacter(&pQueue->Source, 0)) {
        srAssertFail("SourceIsCharacter(&(pQueue->Source))", MAGIC_EFFECTS_CPP, 2685, 0);
    }
    caster = &g_status_685170.buffers.characters[pQueue->Source.iChar];
    if (caster->has_saved_location) {
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
bool IsScreenBusy(void)
{
    if (gXStatus.fCombatMode != 0) {
        return true;
    }
    if (IsModalOpen()) {
        return true;
    }
    if (g_flag_0068506e != 0) {
        return true;
    }
    if (g_current_screen_state.id != 7) {
        return true;
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
        if (monster->instant_death_immune_1be != 0 && condition_id == W8_CONDITION_DEAD) {
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

/* Give every listed target the condition for a rolled duration. A target
   already carrying condition five first spends the new duration against the
   turns it has left, and only the remainder is applied again. */
// FUNCTION: WIZ8 0x0054e3f0
void ApplyConditionToTargets(W8SpellEffectEntry* effect, int condition)
{
    W8MonsterInfo* monster_info;
    unsigned int duration;
    int character_index;
    int location_id;
    int remaining;
    int index;

    duration = RollEffectDuration(&effect->definition);
    for (index = 0; index < effect->target_indices_0f0.GetCount(); ++index) {
        character_index = *effect->target_indices_0f0.GetAt(index);
        if (character_index == -1) {
            srAssertFail("iChar != -1", MAGIC_EFFECTS_CPP, 0x5b2, 0);
        }
        if (condition == 5 &&
            g_status_685170.buffers.characters[character_index].condition_turns[5] != 0) {
            remaining =
                duration - g_status_685170.buffers.characters[character_index].condition_turns[5];
            TickCharacterCondition(character_index, 5, duration);
            if (remaining > 0) {
                ApplyCharacterCondition00523940(character_index, 5,
                                                effect->definition.duration_scale, remaining,
                                                effect->definition.percent);
            }
        } else {
            ApplyCharacterCondition00523940(character_index, condition,
                                            effect->definition.duration_scale, duration,
                                            effect->definition.percent);
        }
    }
    for (index = 0; index < effect->monster_ids_0e0.GetCount(); ++index) {
        location_id = MonsterGetIndexByLocationID(0x5c8, MAGIC_EFFECTS_CPP,
                                                  *effect->monster_ids_0e0.GetAt(index), 1);
        monster_info = MonsterGetScriptPartByLocationIndex(location_id);
        if (monster_info == 0) {
            srAssertFail("pMonsterInfo", MAGIC_EFFECTS_CPP, 0x5c9, 0);
        }
        if (condition == 5 && monster_info->condition_turns[5] != 0) {
            remaining = duration - monster_info->condition_turns[5];
            TickMonsterCondition(monster_info->location_id, 5, duration);
            if (remaining > 0) {
                ApplyMonsterCondition005242B0(monster_info->location_id, 5,
                                              effect->definition.duration_scale, remaining,
                                              effect->definition.percent);
            }
        } else {
            ApplyMonsterCondition005242B0(monster_info->location_id, condition,
                                          effect->definition.duration_scale, duration,
                                          effect->definition.percent);
        }
    }
}

/* Roll the duration, then pick one of the afflictions at random: mostly a
   guaranteed condition attack, sometimes a stamina refill or a settled
   condition. Only a single character target ever reaches here. */
// FUNCTION: WIZ8 0x0054e610
void ApplyRandomAfflictionToTarget(W8SpellEffectEntry* effect)
{
    unsigned int duration;
    unsigned int roll;

    duration = effect->definition.duration_per_power * effect->definition.duration_scale +
               effect->definition.duration_base;
    if (duration != 9999) {
        unsigned int jitter;
        ++duration;
        jitter = Random(4);
        if (jitter == 0) {
            if (duration > 1) {
                ++duration;
            }
        } else if (jitter == 1 && duration < 3) {
            ++duration;
        }
        AdjustIntegerByPercent(&duration, effect->definition.percent);
    }
    if (effect->target.iType != W8_TARGET_KIND_CHARACTER) {
        srAssertFail("(pQueue->Target.iType == TARGET_TYPE_CHAR)", MAGIC_EFFECTS_CPP, 0x5ee, 0);
    }
    roll = Random(100);
    if (roll < 0x28) {
        InflictConditionAttack0054D5C0(effect, 4, 100, 0);
        return;
    }
    if (roll < 0x50) {
        RestoreTargetsStamina(effect);
        return;
    }
    if (roll < 0x5a) {
        InflictConditionAttack0054D5C0(effect, 9, 100, 0);
        return;
    }
    if (roll < 0x5f) {
        InflictConditionAttack0054D5C0(effect, 0xf, 100, 0);
        return;
    }
    ApplyConditionToTargets(effect, 6);
}

/* Post what a non-verbose effect accumulated. The total goes out as one
   "<amount>" line for a lone hit or "<count> <average>" for several, each
   nonzero condition adds its own "<count> <name>", and the queued report
   records are then drained the same way the verbose pass drains them. */
// FUNCTION: WIZ8 0x0054e710
void ReportSpellEffectResult(W8SpellEffectEntry* effect)
{
    unsigned char text_box_mode;
    const unsigned short* condition_text;
    unsigned int* condition_count;
    W8SpellDamageReport* report;
    const wchar_t* condition_name;

    if (g_settings_6850c8.verbose_combat_messages != 0) {
        return;
    }
    if (effect->result_124.result.count != 0) {
        text_box_mode = GetTextBoxMode();
        if (text_box_mode != 0) {
            AppendToLastTextLine(effect->result_124.reported == 0 ? L" -- " : L", ", -1);
            SetTextBoxMode(1, -1);
        }
        if (effect->target_indices_0f0.GetCount() + effect->monster_ids_0e0.GetCount() == 1) {
            AppendToLastTextLine(
                FormatWideString(gppStringList[0x19a], effect->result_124.result.amount, -1), -1);
        } else {
            AppendToLastTextLine(
                FormatWideString(gppStringList[0x199], effect->result_124.result.count,
                                 effect->result_124.result.amount / effect->result_124.result.count,
                                 -1),
                -1);
        }
        SetTextBoxMode(1, -1);
        effect->result_124.reported = 1;
        effect->result_124.applied = 1;
    }
    condition_text = g_condition_notices_0061E570 + 2;
    condition_count = effect->result_124.result.condition_counts;
    do {
        if (*condition_count != 0) {
            text_box_mode = GetTextBoxMode();
            if (text_box_mode != 0) {
                AppendToLastTextLine(effect->result_124.reported == 0 ? L" -- " : L", ", -1);
                SetTextBoxMode(1, -1);
            }
            if (*condition_count == 1) {
                condition_name = gppStringList[condition_text[-1]];
            } else {
                condition_name = gppStringList[*condition_text];
            }
            AppendToLastTextLine(FormatWideString(L"%ld %s", *condition_count, condition_name, -1),
                                 -1);
            SetTextBoxMode(1, -1);
        }
        condition_text += 4;
        ++condition_count;
    } while (condition_text < g_condition_notices_0061E570 + 0x4a);

    while (effect->result_124.result.reports.GetCount() > 0) {
        report = *effect->result_124.result.reports.GetAt(0);
        effect->result_124.result.reports.RemoveAt(0);
        if (report != 0) {
            if (report->kind == 1) {
                PostCharacterNotice(report->value, L"%s!",
                                    gppStringList[g_condition_notices_0061E570[0x49]]);
            } else if (report->kind == 3) {
                ShowNoticef(9, L"%s %s!", report->text,
                            gppStringList[g_condition_notices_0061E570[0x49]]);
            }
            free(report);
        }
    }
}

/* How long the target's own copy of the condition still runs; condition
   seven also hands back the argument it was set with. Any other target kind
   is a caller error. */
// FUNCTION: WIZ8 0x00553910
unsigned int GetTargetConditionTurns(W8SpellEffectEntry* effect, int condition, int* argument)
{
    W8MonsterInfo* monster_info;

    if (effect->target.iType == W8_TARGET_KIND_CHARACTER) {
        if (condition == 7 && argument != 0) {
            *argument = g_status_685170.buffers.characters[effect->target.iChar].condition_argument;
        }
        return g_status_685170.buffers.characters[effect->target.iChar].condition_turns[condition];
    }
    if (effect->target.iType == W8_TARGET_KIND_MONSTER) {
        monster_info = MonsterInfoFromID(0x1280, MAGIC_EFFECTS_CPP, effect->target.iMonsterID, 1);
        if (condition == 7 && argument != 0) {
            *argument = monster_info->condition_argument;
        }
        return monster_info->condition_turns[condition];
    }
    srAssertFail("0", MAGIC_EFFECTS_CPP, 0x128a, 0);
    return 0;
}

/* Wear the listed targets down: each one with any stamina left takes a fresh
   rolled magnitude of fatigue. */
// FUNCTION: WIZ8 0x0054f8c0
void FatigueTargets(W8SpellEffectEntry* effect)
{
    W8MonsterInfo* monster_info;
    unsigned int magnitude;
    int character_index;
    int location_id;
    int index;

    for (index = 0; index < effect->target_indices_0f0.GetCount(); ++index) {
        character_index = *effect->target_indices_0f0.GetAt(index);
        if (character_index == -1) {
            srAssertFail("iChar != -1", MAGIC_EFFECTS_CPP, 0x863, 0);
        }
        if (g_status_685170.buffers.characters[character_index].stamina != 0) {
            magnitude = RollEffectMagnitude(&effect->definition);
            FatigueCharacter(character_index, magnitude, 0, 0);
        }
    }
    for (index = 0; index < effect->monster_ids_0e0.GetCount(); ++index) {
        location_id = MonsterGetIndexByLocationID(0x871, MAGIC_EFFECTS_CPP,
                                                  *effect->monster_ids_0e0.GetAt(index), 1);
        monster_info = MonsterGetScriptPartByLocationIndex(location_id);
        if (monster_info == 0) {
            srAssertFail("pMonsterInfo", MAGIC_EFFECTS_CPP, 0x872, 0);
        }
        if (monster_info->stamina != 0) {
            magnitude = RollEffectMagnitude(&effect->definition);
            FatigueMonster(monster_info, magnitude, 0);
        }
    }
}

/* Put stamina back into the listed targets, capped at what each one is
   missing. In quiet mode the totals are gathered and reported at the end;
   the answer is whether every affected target is now full. */
// FUNCTION: WIZ8 0x0054f520
char RestoreTargetsStamina(W8SpellEffectEntry* effect)
{
    W8MonsterInfo* monster_info;
    unsigned char verbose;
    unsigned int restored;
    unsigned int missing;
    unsigned int total;
    unsigned int count;
    int character_index;
    int target_id;
    bool target_is_character;
    bool all_full;
    int index;

    verbose = g_settings_6850c8.verbose_combat_messages;
    all_full = 1;
    total = 0;
    count = 0;
    for (index = 0; index < effect->target_indices_0f0.GetCount(); ++index) {
        character_index = *effect->target_indices_0f0.GetAt(index);
        if (character_index == -1) {
            srAssertFail("iChar != -1", MAGIC_EFFECTS_CPP, 0x7e1, 0);
        }
        W8Character* character = &g_status_685170.buffers.characters[character_index];
        if (character->stamina != character->stamina_max) {
            restored = RollEffectMagnitude(&effect->definition);
            if (restored != 0) {
                missing = character->stamina_max - character->stamina;
                if (missing <= restored) {
                    restored = missing;
                }
                RestoreCharacterStamina(character_index, restored, verbose);
                if (character->stamina < character->stamina_max) {
                    all_full = 0;
                }
                if (verbose == 0) {
                    total += restored;
                    ++count;
                    target_is_character = 1;
                    target_id = character_index;
                }
            }
        }
    }
    for (index = 0; index < effect->monster_ids_0e0.GetCount(); ++index) {
        unsigned int monster_index = MonsterGetIndexByLocationID(
            0x800, MAGIC_EFFECTS_CPP, *effect->monster_ids_0e0.GetAt(index), 1);
        monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
        if (monster_info == 0) {
            srAssertFail("pMonsterInfo", MAGIC_EFFECTS_CPP, 0x801, 0);
        }
        if (monster_info->stamina != monster_info->stamina_max) {
            restored = RollEffectMagnitude(&effect->definition);
            if (restored != 0) {
                missing = monster_info->stamina_max - monster_info->stamina;
                if (missing <= restored) {
                    restored = missing;
                }
                RestoreMonsterStamina(monster_info, restored, 1);
                if (static_cast<unsigned int>(monster_info->stamina) <
                    static_cast<unsigned int>(monster_info->stamina_max)) {
                    all_full = 0;
                }
                if (verbose == 0) {
                    total += restored;
                    ++count;
                    target_id = monster_info->location_id;
                    target_is_character = 0;
                }
            }
        }
    }
    if (verbose == 0 && count != 0) {
        effect->result_124.applied = 1;
        if (GetTextBoxMode() != 0) {
            AppendToLastTextLine(L" -- ", -1);
            SetTextBoxMode(1, -1);
        }
        if (count == 1) {
            const wchar_t* name;
            const wchar_t* format;
            if (target_is_character == 0) {
                monster_info = MonsterInfoFromID(0x83b, MAGIC_EFFECTS_CPP, target_id, 1);
                name = GetMonsterName(monster_info, 0, 0);
                format = gppStringList[0x1a2];
            } else {
                name = g_status_685170.buffers.characters[target_id].name;
                format = gppStringList[(total != 1) + 0x1a1];
            }
            AppendToLastTextLine(FormatWideString(format, name, total, -1), -1);
        } else {
            AppendToLastTextLine(
                FormatWideString(gppStringList[(total != 1) + 0x19f], count, total / count, -1),
                -1);
        }
        SetTextBoxMode(1, -1);
        effect->result_124.reported = 1;
    }
    return all_full;
}

/* Heal the listed targets, capped at what each one is missing, with the same
   quiet-mode tally and "everyone full" answer as the stamina version. */
// FUNCTION: WIZ8 0x0054f190
char HealTargets(W8SpellEffectEntry* effect)
{
    W8MonsterInfo* monster_info;
    unsigned char verbose;
    unsigned int healed;
    unsigned int missing;
    unsigned int total;
    unsigned int count;
    int character_index;
    int target_id;
    bool target_is_character;
    bool all_full;
    int index;

    verbose = g_settings_6850c8.verbose_combat_messages;
    all_full = 1;
    count = 0;
    total = 0;
    for (index = 0; index < effect->target_indices_0f0.GetCount(); ++index) {
        character_index = *effect->target_indices_0f0.GetAt(index);
        if (character_index == -1) {
            srAssertFail("iChar != -1", MAGIC_EFFECTS_CPP, 0x75e, 0);
        }
        W8Character* character = &g_status_685170.buffers.characters[character_index];
        missing = character->hp_max - character->hp_current;
        if (missing != 0) {
            healed = RollEffectMagnitude(&effect->definition);
            if (missing <= healed) {
                healed = missing;
            }
            HealCharacter(character_index, healed, verbose);
            if (character->hp_current < static_cast<unsigned int>(character->hp_max)) {
                all_full = 0;
            }
            if (verbose == 0) {
                total += healed;
                ++count;
                target_is_character = 1;
                target_id = character_index;
            }
        }
    }
    for (index = 0; index < effect->monster_ids_0e0.GetCount(); ++index) {
        unsigned int monster_index = MonsterGetIndexByLocationID(
            0x77d, MAGIC_EFFECTS_CPP, *effect->monster_ids_0e0.GetAt(index), 1);
        monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
        if (monster_info == 0) {
            srAssertFail("pMonsterInfo", MAGIC_EFFECTS_CPP, 0x77e, 0);
        }
        missing = monster_info->hp_max - monster_info->hp_current;
        if (missing != 0) {
            healed = RollEffectMagnitude(&effect->definition);
            if (missing <= healed) {
                healed = missing;
            }
            HealMonster(monster_info, healed, verbose);
            if (monster_info->hp_current < static_cast<unsigned int>(monster_info->hp_max)) {
                all_full = 0;
            }
            if (verbose == 0) {
                total += healed;
                ++count;
                target_id = monster_info->location_id;
                target_is_character = 0;
            }
        }
    }
    if (verbose == 0 && count != 0) {
        if (GetTextBoxMode() != 0) {
            AppendToLastTextLine(L" -- ", -1);
            SetTextBoxMode(1, -1);
        }
        if (count == 1) {
            const wchar_t* name;
            if (target_is_character == 0) {
                monster_info = MonsterInfoFromID(0x7b2, MAGIC_EFFECTS_CPP, target_id, 1);
                name = GetMonsterName(monster_info, 0, 0);
            } else {
                name = g_status_685170.buffers.characters[target_id].name;
            }
            AppendToLastTextLine(
                FormatWideString(gppStringList[(total != 1) + 0x19d], name, total, -1), -1);
        } else {
            AppendToLastTextLine(FormatWideString(gppStringList[(total / count != 1) + 0x19b],
                                                  count, total / count, -1),
                                 -1);
        }
        SetTextBoxMode(1, -1);
        effect->result_124.reported = 1;
        effect->result_124.applied = 1;
    }
    return all_full;
}

/* Roll a magnitude per listed target, cut it by the target's resistance in
   the spell's realm, and apply what is left as damage. A fully resisted hit
   is only announced; a monster's soak bookkeeping still runs on it. */
// FUNCTION: WIZ8 0x0054e950
void ApplyDamageToTargets(W8SpellEffectEntry* effect)
{
    W8MonsterInfo* monster_info;
    W8CombatSlot target;
    W8SpellEffectResult* result;
    unsigned char verbose;
    unsigned int magnitude;
    int character_index;
    int realm;
    int power_level;
    int index;

    verbose = g_settings_6850c8.verbose_combat_messages;
    realm = g_spell_records[effect->kind].realm;
    power_level = effect->definition.power_level;
    TargetSourceIsCharacter(&effect->Source, 1);
    for (index = 0; index < effect->target_indices_0f0.GetCount(); ++index) {
        character_index = *effect->target_indices_0f0.GetAt(index);
        if (character_index == -1) {
            srAssertFail("iChar != -1", MAGIC_EFFECTS_CPP, 0x694, 0);
        }
        magnitude = RollEffectMagnitude(&effect->definition);
        target.iType = W8_TARGET_KIND_CHARACTER;
        target.iChar = character_index;
        ReduceMagnitudeByResistance(&magnitude, &target, realm, power_level);
        if (magnitude == 0) {
            if (g_settings_6850c8.verbose_combat_messages != 0) {
                if (target.iType == W8_TARGET_KIND_MONSTER) {
                    unsigned int monster_index =
                        MonsterGetIndexByLocationID(0xeae, MAGIC_EFFECTS_CPP, target.iMonsterID, 1);
                    monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
                    PostMonsterNotice(monster_info, gppStringList[0x1b3]);
                } else {
                    PostCharacterNotice(target.iChar, gppStringList[0x1b3]);
                }
            }
        } else {
            if (verbose == 0) {
                result = &effect->result_124.result;
            } else {
                result = 0;
            }
            ApplyDamageToCharacter(character_index, magnitude, 0, verbose != 0, 0, result, 0);
        }
    }
    for (index = 0; index < effect->monster_ids_0e0.GetCount(); ++index) {
        unsigned int monster_index = MonsterGetIndexByLocationID(
            0x6ac, MAGIC_EFFECTS_CPP, *effect->monster_ids_0e0.GetAt(index), 1);
        monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
        if (monster_info == 0) {
            srAssertFail("pMonsterInfo", MAGIC_EFFECTS_CPP, 0x6ad, 0);
        }
        magnitude = RollEffectMagnitude(&effect->definition);
        target.iType = W8_TARGET_KIND_MONSTER;
        target.iMonsterID = monster_info->location_id;
        ReduceMagnitudeByResistance(&magnitude, &target, realm, power_level);
        if (magnitude == 0) {
            if (g_settings_6850c8.verbose_combat_messages != 0) {
                if (target.iType == W8_TARGET_KIND_MONSTER) {
                    monster_index =
                        MonsterGetIndexByLocationID(0xeae, MAGIC_EFFECTS_CPP, target.iMonsterID, 1);
                    monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
                    PostMonsterNotice(monster_info, gppStringList[0x1b3]);
                } else {
                    PostCharacterNotice(target.iChar, gppStringList[0x1b3]);
                }
            }
            monster_info->monster->SpawnDamageNumber(magnitude);
        } else {
            if (verbose == 0) {
                result = &effect->result_124.result;
            } else {
                result = 0;
            }
            ApplyDamageToMonster(monster_info, magnitude, &effect->Source, 0, verbose != 0, 0,
                                 result, 0);
        }
    }
    if (verbose == 0) {
        ReportSpellEffectResult(effect);
    }
}

/* Drain the listed targets: the resisted remainder of a rolled magnitude is
   taken as damage, never more than the target has left, and the total dealt
   is handed back to the caster - as healing for spell 0x52, as spell points
   for spell 0x54. */
// FUNCTION: WIZ8 0x0054ec80
void DrainTargetsLife(W8SpellEffectEntry* effect)
{
    W8MonsterInfo* monster_info;
    W8CombatSlot target;
    W8SpellEffectResult* result;
    unsigned char announce;
    unsigned int magnitude;
    unsigned int drained;
    int character_index;
    int realm;
    int power_level;
    int index;

    announce = g_settings_6850c8.verbose_combat_messages;
    realm = g_spell_records[effect->kind].realm;
    power_level = effect->definition.power_level;
    drained = 0;
    TargetSourceIsCharacter(&effect->Source, 1);
    for (index = 0; index < effect->target_indices_0f0.GetCount(); ++index) {
        character_index = *effect->target_indices_0f0.GetAt(index);
        if (character_index == -1) {
            srAssertFail("iChar != -1", MAGIC_EFFECTS_CPP, 0x6e8, 0);
        }
        magnitude = RollEffectMagnitude(&effect->definition);
        target.iType = W8_TARGET_KIND_CHARACTER;
        target.iChar = character_index;
        ReduceMagnitudeByResistance(&magnitude, &target, realm, power_level);
        if (magnitude == 0 && g_settings_6850c8.verbose_combat_messages != 0) {
            if (target.iType == W8_TARGET_KIND_MONSTER) {
                unsigned int monster_index =
                    MonsterGetIndexByLocationID(0xeae, MAGIC_EFFECTS_CPP, target.iMonsterID, 1);
                monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
                PostMonsterNotice(monster_info, gppStringList[0x1b3]);
            } else {
                PostCharacterNotice(target.iChar, gppStringList[0x1b3]);
            }
        }
        if (g_status_685170.buffers.characters[character_index].hp_current <= magnitude) {
            magnitude = g_status_685170.buffers.characters[character_index].hp_current;
        }
        if (magnitude != 0) {
            if (announce == 0) {
                result = &effect->result_124.result;
            } else {
                result = 0;
            }
            ApplyDamageToCharacter(character_index, magnitude, 0, announce != 0, 0, result, 0);
            drained += magnitude;
        }
    }
    for (index = 0; index < effect->monster_ids_0e0.GetCount(); ++index) {
        unsigned int monster_index = MonsterGetIndexByLocationID(
            0x704, MAGIC_EFFECTS_CPP, *effect->monster_ids_0e0.GetAt(index), 1);
        monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
        if (monster_info == 0) {
            srAssertFail("pMonsterInfo", MAGIC_EFFECTS_CPP, 0x705, 0);
        }
        magnitude = RollEffectMagnitude(&effect->definition);
        target.iType = W8_TARGET_KIND_MONSTER;
        target.iMonsterID = monster_info->location_id;
        ReduceMagnitudeByResistance(&magnitude, &target, realm, power_level);
        if (magnitude == 0 && g_settings_6850c8.verbose_combat_messages != 0) {
            if (target.iType == W8_TARGET_KIND_MONSTER) {
                monster_index =
                    MonsterGetIndexByLocationID(0xeae, MAGIC_EFFECTS_CPP, target.iMonsterID, 1);
                monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
                PostMonsterNotice(monster_info, gppStringList[0x1b3]);
            } else {
                PostCharacterNotice(target.iChar, gppStringList[0x1b3]);
            }
        }
        if (monster_info->hp_current <= magnitude) {
            magnitude = monster_info->hp_current;
        }
        if (magnitude != 0) {
            if (announce == 0) {
                result = &effect->result_124.result;
            } else {
                result = 0;
            }
            ApplyDamageToMonster(monster_info, magnitude, &effect->Source, 0, announce != 0, 0,
                                 result, 0);
            drained += magnitude;
        }
    }
    if (announce == 0) {
        ReportSpellEffectResult(effect);
    }
    if (TargetSourceIsCharacter(&effect->Source, 0) == 0) {
        if (TargetSourceIsMonster(&effect->Source, 0) != 0) {
            monster_info =
                MonsterInfoFromID(0x738, MAGIC_EFFECTS_CPP, effect->Source.iMonsterID, 1);
            if (monster_info == 0) {
                srAssertFail("pMonsterInfo", MAGIC_EFFECTS_CPP, 0x739, 0);
            }
            if (effect->kind == 0x52) {
                HealMonster(monster_info, (drained * 7) / 10, announce);
            } else if (effect->kind == 0x54) {
                monster_info->spell_points_2f9 += drained >> 1;
            }
        }
    } else if (effect->kind == 0x52) {
        unsigned int slot = FindPartySlotWithLowestHitPoints();
        if (g_status_685170.buffers.party_rows[slot].occupied != 0 &&
            g_status_685170.buffers.characters[slot].hp_current <
                static_cast<unsigned int>(g_status_685170.buffers.characters[slot].hp_max)) {
            HealCharacter(slot, (drained * 7) / 10, announce);
        }
    } else if (effect->kind == 0x54) {
        unsigned int slot = FindPartySlotWithLowestSpellPoints();
        if (g_status_685170.buffers.party_rows[slot].occupied != 0) {
            W8Character* character = &g_status_685170.buffers.characters[slot];
            if (SumCharacterSpellPointsLeft(character) < SumCharacterSpellPoints(character)) {
                RestoreCharacterSpellPointsEvenly(slot, drained);
            }
        }
    }
}

/* Wear down or lift a condition on every listed target. A permanent
   condition, or a forced cure, only comes off on a removal-chance roll - a
   hundred percent for conditions one and eighteen and for anything the cure
   power already covers, argument times fifteen otherwise. Lifting death
   leaves the character barely alive and may queue the revive event. A
   condition the roll fails simply ticks down by the squared power and counts
   toward the resisted report. */
// FUNCTION: WIZ8 0x0054df00
char TryCureConditionOnTargets(W8SpellEffectEntry* effect, int condition, char force)
{
    W8NpcState* npc_state;
    W8MonsterInfo* monster_info;
    unsigned int power;
    unsigned int turns;
    unsigned int restored;
    unsigned int chance;
    int character_index;
    int remaining_count;
    bool all_cured;
    int index;

    power = effect->definition.duration_scale * effect->definition.duration_scale;
    all_cured = 1;
    remaining_count = 0;
    AdjustIntegerByPercent(&power, effect->definition.percent);
    for (index = 0; index < effect->target_indices_0f0.GetCount(); ++index) {
        character_index = *effect->target_indices_0f0.GetAt(index);
        if (character_index == -1) {
            srAssertFail("iChar != -1", MAGIC_EFFECTS_CPP, 0x501, 0);
        }
        W8Character* character = &g_status_685170.buffers.characters[character_index];
        if (character->condition_turns[condition] != 0) {
            if (character->condition_turns[condition] == W8_EFFECT_PERMANENT || force != 0) {
                if (condition == 0x12 || (condition == 1 || force != 0)) {
                    chance = 100;
                } else {
                    chance = effect->definition.duration_scale * 0xf;
                }
                if (Random(100) < chance) {
                    RemoveCharacterCondition(character_index, condition, 1);
                    effect->result_124.applied = 1;
                    effect->result_124.reported = 1;
                    if (condition == 0x12) {
                        restored = (character->attributes[1].effective +
                                    character->attributes[0].effective) >>
                                   2;
                        turns = (character->hp_max * restored) / 100;
                        if (turns < 2) {
                            turns = 1;
                        }
                        character->hp_current = turns;
                        turns = (character->stamina_max * restored) / 100;
                        if (turns < 2) {
                            turns = 1;
                        }
                        character->stamina = turns;
                        int npc_index =
                            g_status_685170.buffers.party_rows[character_index].animation_0fa;
                        if (npc_index != -1 &&
                            (npc_state = GetNpcState(npc_index), npc_state != 0)) {
                            npc_state->unknown_04 = 0;
                        }
                    }
                    if (Random(100) < 0x32) {
                        QueueCharacterEvent(character, g_special_event_0068c55c, 0,
                                            g_effect_argument_005ed8c8, g_effect_argument_005ed914);
                    }
                } else {
                    all_cured = 0;
                }
            } else {
                TickCharacterCondition(character_index, condition, power);
                if (character->condition_turns[condition] == 0) {
                    effect->result_124.reported = 1;
                } else {
                    ++remaining_count;
                    all_cured = 0;
                    if (g_settings_6850c8.verbose_combat_messages != 0) {
                        PostCharacterNotice(
                            character_index, gppStringList[0x1b1],
                            gppStringList[g_condition_notices_0061E570[condition * 4 + 3]]);
                        effect->result_124.applied = 1;
                        continue;
                    }
                }
                effect->result_124.applied = 1;
            }
        }
    }
    for (index = 0; index < effect->monster_ids_0e0.GetCount(); ++index) {
        unsigned int monster_index = MonsterGetIndexByLocationID(
            0x558, MAGIC_EFFECTS_CPP, *effect->monster_ids_0e0.GetAt(index), 1);
        monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
        if (monster_info == 0) {
            srAssertFail("pMonsterInfo", MAGIC_EFFECTS_CPP, 0x559, 0);
        }
        turns = monster_info->condition_turns[condition];
        if (turns != 0) {
            if (turns == W8_EFFECT_PERMANENT || turns <= power || force != 0) {
                if (condition == 0x12 || condition == 1 || turns <= power || force != 0) {
                    chance = 100;
                } else {
                    chance = effect->definition.duration_scale * 0xf;
                }
                if (Random(100) < chance) {
                    ClearMonsterCondition(monster_info->location_id, condition);
                    effect->result_124.applied = 1;
                } else {
                    all_cured = 0;
                }
            } else {
                monster_info->condition_turns[condition] = turns - power;
                ++remaining_count;
                all_cured = 0;
                effect->result_124.applied = 1;
            }
        }
    }
    if (g_settings_6850c8.verbose_combat_messages == 0 && remaining_count != 0) {
        if (GetTextBoxMode() != 0) {
            AppendToLastTextLine(effect->result_124.reported == 0 ? L" -- " : L", ", -1);
            SetTextBoxMode(1, -1);
        }
        if (remaining_count == 1) {
            if (effect->target.iType == W8_TARGET_KIND_CHARACTER) {
                PostCharacterNotice(effect->target.iChar, gppStringList[0x1b1],
                                    gppStringList[g_condition_notices_0061E570[condition * 4 + 3]]);
                effect->result_124.reported = 1;
                return all_cured;
            }
            if (effect->target.iType == W8_TARGET_KIND_MONSTER) {
                unsigned int monster_index = MonsterGetIndexByLocationID(
                    0x597, MAGIC_EFFECTS_CPP, effect->target.iMonsterID, 1);
                monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
                PostMonsterNotice(monster_info, gppStringList[0x1b1],
                                  gppStringList[g_condition_notices_0061E570[condition * 4 + 3]]);
                effect->result_124.reported = 1;
                return all_cured;
            }
        }
        AppendToLastTextLine(
            FormatWideString(gppStringList[0x1b2], remaining_count,
                             gppStringList[g_condition_notices_0061E570[condition * 4 + 3]], -1),
            -1);
        effect->result_124.reported = 1;
    }
    return all_cured;
}

/* Attack every listed target with a condition on a chance roll each. The
   rolled duration is what the condition runs for; spell 0x1d first spends it
   against enchantment slot five, and the elemental spells 0xe and 0x1e strike
   a monster a second time at half strength. Quiet mode counts the targets the
   attack landed on and reports the tally. */
// FUNCTION: WIZ8 0x0054d5c0
void InflictConditionAttack0054D5C0(W8SpellEffectEntry* effect, int condition, int chance,
                                    int argument)
{
    W8MonsterInfo* monster_info;
    W8CombatSlot target;
    unsigned char verbose;
    unsigned int duration;
    int remaining;
    int affected;
    int character_index;
    int index;
    const wchar_t* notice;

    verbose = g_settings_6850c8.verbose_combat_messages;
    affected = 0;
    for (index = 0; index < effect->target_indices_0f0.GetCount(); ++index) {
        if (Random(100) >= static_cast<unsigned int>(chance)) {
            continue;
        }
        duration = RollEffectDuration(&effect->definition);
        character_index = *effect->target_indices_0f0.GetAt(index);
        if (character_index == -1) {
            srAssertFail("iChar != -1", MAGIC_EFFECTS_CPP, 0x40c, 0);
        }
        target.iType = W8_TARGET_KIND_CHARACTER;
        target.iChar = character_index;
        if (effect->kind == 0x1d &&
            g_status_685170.buffers.characters[character_index].enchantments[5].value_08 != 0) {
            /* Retail subtracts the shield with signed JLE on the leftover. */
            remaining =
                static_cast<int>(duration) -
                static_cast<int>(
                    g_status_685170.buffers.characters[character_index].enchantments[5].value_08);
            TickCharacterEnchantmentSlot(character_index, 5, duration);
            if (remaining > 0 &&
                ResolveAttackOnTarget00551BA0(&effect->Source, &target, condition,
                                              g_spell_records[effect->kind].realm,
                                              effect->definition.power_level, argument, remaining,
                                              verbose, verbose, 0) == 0) {
                if (verbose == 0) {
                    ++affected;
                }
            }
        } else {
            if (ResolveAttackOnTarget00551BA0(
                    &effect->Source, &target, condition, g_spell_records[effect->kind].realm,
                    effect->definition.power_level, argument, duration, verbose, verbose, 0) != 0) {
                continue;
            }
            if (effect->kind == 0xe) {
                ResolveAttackOnTarget00551BA0(
                    &effect->Source, &target, 0x11, g_spell_records[0xe].realm,
                    static_cast<unsigned int>(effect->definition.power_level) >> 1, argument,
                    duration >> 1, 0, verbose, 0);
            }
            if (verbose == 0) {
                ++affected;
            }
        }
    }
    for (index = 0; index < effect->monster_ids_0e0.GetCount(); ++index) {
        if (Random(100) >= static_cast<unsigned int>(chance)) {
            continue;
        }
        duration = RollEffectDuration(&effect->definition);
        unsigned int monster_index = MonsterGetIndexByLocationID(
            0x43d, MAGIC_EFFECTS_CPP, *effect->monster_ids_0e0.GetAt(index), 1);
        monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
        if (monster_info == 0) {
            srAssertFail("pMonsterInfo", MAGIC_EFFECTS_CPP, 0x43e, 0);
        }
        target.iType = W8_TARGET_KIND_MONSTER;
        target.iMonsterID = monster_info->location_id;
        if (effect->kind == 0x1d && monster_info->enchantments[5].value_08 != 0) {
            remaining = static_cast<int>(duration) -
                        static_cast<int>(monster_info->enchantments[5].value_08);
            TickMonsterEnchantmentSlot(monster_info->location_id, 5, duration);
            if (remaining > 0 &&
                ResolveAttackOnTarget00551BA0(&effect->Source, &target, condition,
                                              g_spell_records[effect->kind].realm,
                                              effect->definition.power_level, argument, remaining,
                                              verbose, verbose, 0) == 0) {
                if (verbose == 0) {
                    ++affected;
                }
            }
        } else {
            if (ResolveAttackOnTarget00551BA0(
                    &effect->Source, &target, condition, g_spell_records[effect->kind].realm,
                    effect->definition.power_level, argument, duration, verbose, verbose, 0) != 0) {
                continue;
            }
            if (effect->kind == 0xe || effect->kind == 0x1e) {
                ResolveAttackOnTarget00551BA0(
                    &effect->Source, &target, condition, g_spell_records[effect->kind].realm,
                    static_cast<unsigned int>(effect->definition.power_level) >> 1, argument,
                    duration >> 1, 0, verbose, 0);
            }
            if (verbose == 0) {
                ++affected;
            }
        }
    }
    if (verbose != 0 || affected == 0) {
        return;
    }
    if (GetTextBoxMode() != 0) {
        AppendToLastTextLine(effect->result_124.reported == 0 ? L" -- " : L", ", -1);
        SetTextBoxMode(1, -1);
    }
    if (effect->monster_ids_0e0.GetCount() + effect->target_indices_0f0.GetCount() != 1) {
        if (affected == 1) {
            notice = gppStringList[g_condition_notices_0061E570[condition * 4 + 1]];
        } else {
            notice = gppStringList[g_condition_notices_0061E570[condition * 4 + 2]];
        }
        AppendToLastTextLine(FormatWideString(L"%ld %s", affected, notice, -1), -1);
        SetTextBoxMode(1, -1);
    } else {
        notice = gppStringList[g_condition_notices_0061E570[condition * 4 + 1]];
        if (target.iType == W8_TARGET_KIND_CHARACTER) {
            AppendToLastTextLine(
                FormatWideString(L"%s %s", g_status_685170.buffers.characters[character_index].name,
                                 notice, -1),
                -1);
        } else if (target.iType == W8_TARGET_KIND_MONSTER) {
            monster_info = MonsterInfoFromID(0x486, MAGIC_EFFECTS_CPP, target.iMonsterID, 1);
            AppendToLastTextLine(
                FormatWideString(L"%s %s", GetMonsterName(monster_info, 0, 0), notice, -1), -1);
        } else {
            AppendToLastTextLine(FormatWideString(L"%ld %s", affected, notice, -1), -1);
        }
        SetTextBoxMode(1, -1);
    }
    effect->result_124.reported = 1;
    effect->result_124.applied = 1;
}

/* Install the spell's being-effect slot on the party and on every listed
   monster. The slot index is the spell's place in the being table; a slot
   that was not already running this spell drops the old visual first. */
// FUNCTION: WIZ8 0x0054cbf0
void ApplyBeingEffectSlot(W8SpellEffectEntry* effect)
{
    W8MonsterInfo* monster_info;
    W8EffectSlot* slot;
    unsigned int duration;
    const int* table;
    int slot_index;
    int index;

    duration = RollEffectDuration(&effect->definition);
    slot_index = 0;
    table = g_being_effect_slot_spells_00616d84;
    while (effect->kind != *table) {
        ++table;
        ++slot_index;
        if (g_combat_effect_slot_spells_00616db4 <= table) {
            srAssertFail("fFound", MAGIC_EFFECTS_CPP, 0x319, 0);
            return;
        }
    }
    if (effect->target_indices_0f0.GetCount() != 0) {
        slot = &g_status_685170.effect_slots_17af[slot_index];
        slot->active = 1;
        slot->effect_id = effect->kind;
        slot->amount = effect->definition.duration_scale;
        slot->duration_0d = duration;
        RebuildPartyEffectBlock0050E700();
        RequestRedraw(0x800100);
        effect->result_124.applied = 1;
    }
    for (index = 0; index < effect->monster_ids_0e0.GetCount(); ++index) {
        unsigned int monster_index = MonsterGetIndexByLocationID(
            0x30d, MAGIC_EFFECTS_CPP, *effect->monster_ids_0e0.GetAt(index), 1);
        monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
        if (monster_info == 0) {
            srAssertFail("pMonsterInfo", MAGIC_EFFECTS_CPP, 0x30e, 0);
        }
        slot = &monster_info->effect_slots_10f[slot_index];
        if (slot->active == 0 || slot->effect_id != effect->kind) {
            SetMonsterSpellIcon(monster_info->monster, g_effect_visual_table[effect->kind][1], 1);
        }
        slot->active = 1;
        slot->effect_id = effect->kind;
        slot->amount = effect->definition.duration_scale;
        slot->duration_0d = duration;
        RebuildMonsterDerivedStats(monster_info->location_id);
        effect->result_124.applied = 1;
    }
}

/* Roll for each listed target: the attack condition goes out against a
   chance, and a target that shakes it off still takes the second condition
   the effect carries. */
// FUNCTION: WIZ8 0x0054dbe0
void AttackThenInflictCondition(W8SpellEffectEntry* effect, int attack_condition, int condition,
                                int chance, int attack_argument, int condition_argument)
{
    W8MonsterInfo* monster_info;
    W8CombatSlot target;
    unsigned int duration;
    int source_character;
    int character_index;
    int index;

    if (TargetSourceIsCharacter(&effect->Source, 0) == 0) {
        source_character = -1;
    } else {
        source_character = effect->Source.iChar;
    }
    for (index = 0; index < effect->target_indices_0f0.GetCount(); ++index) {
        if (Random(100) >= static_cast<unsigned int>(chance)) {
            continue;
        }
        duration = RollEffectDuration(&effect->definition);
        character_index = *effect->target_indices_0f0.GetAt(index);
        if (character_index == -1) {
            srAssertFail("iChar != -1", MAGIC_EFFECTS_CPP, 0x4bf, 0);
        }
        target.iType = W8_TARGET_KIND_CHARACTER;
        target.iChar = character_index;
        if (ResolveAttackOnTarget00551BA0(
                &effect->Source, &target, attack_condition, g_spell_records[effect->kind].realm,
                effect->definition.power_level, attack_argument, duration, 0, 1, 0) != 0) {
            InflictConditionOnTarget(&target, condition, g_spell_records[effect->kind].realm,
                                     effect->definition.power_level, condition_argument, duration,
                                     source_character, 0, 1);
        }
    }
    for (index = 0; index < effect->monster_ids_0e0.GetCount(); ++index) {
        if (Random(100) >= static_cast<unsigned int>(chance)) {
            continue;
        }
        duration = RollEffectDuration(&effect->definition);
        unsigned int monster_index = MonsterGetIndexByLocationID(
            0x4d8, MAGIC_EFFECTS_CPP, *effect->monster_ids_0e0.GetAt(index), 1);
        monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
        if (monster_info == 0) {
            srAssertFail("pMonsterInfo", MAGIC_EFFECTS_CPP, 0x4d9, 0);
        }
        target.iType = W8_TARGET_KIND_MONSTER;
        target.iMonsterID = monster_info->location_id;
        if (ResolveAttackOnTarget00551BA0(
                &effect->Source, &target, attack_condition, g_spell_records[effect->kind].realm,
                effect->definition.power_level, attack_argument, duration, 0, 1, 0) != 0) {
            InflictConditionOnTarget(&target, condition, g_spell_records[effect->kind].realm,
                                     effect->definition.power_level, condition_argument, duration,
                                     source_character, 0, 1);
        }
    }
}

/* Fill the spell's slot in the first combat-effect block on the party and on
   every listed monster. A monster not yet in combat has its group pulled in
   first; one that already fights takes the slot straight away and, when the
   source is a character, hates them a little more for it. */
// FUNCTION: WIZ8 0x0054cdd0
void ApplyCombatEffectSlot(W8SpellEffectEntry* effect)
{
    W8MonsterInfo* monster_info;
    W8EffectSlot* slot;
    unsigned int duration;
    const int* table;
    int source_character;
    int spell_id;
    int slot_index;
    int index;

    spell_id = effect->kind;
    if (gXStatus.fCombatMode == 0) {
        srAssertFail("gXStatus.fCombatMode", MAGIC_EFFECTS_CPP, 0x329, 0);
    }
    duration = effect->definition.duration_per_power * effect->definition.duration_scale +
               effect->definition.duration_base;
    if (duration != 9999) {
        unsigned int roll;
        ++duration;
        roll = Random(4);
        if (roll == 0) {
            if (duration > 1) {
                ++duration;
            }
        } else if (roll == 1 && duration < 3) {
            ++duration;
        }
        AdjustIntegerByPercent(&duration, effect->definition.percent);
    }
    if (TargetSourceIsCharacter(&effect->Source, 1) != 0) {
        source_character = effect->Source.iChar;
    }
    slot_index = 0;
    table = g_combat_effect_slot_spells_00616db4;
    while (spell_id != *table) {
        ++table;
        ++slot_index;
        if (table >= g_combat_effect_slot_spells_00616dd8) {
            break;
        }
    }
    if (spell_id != *table) {
        srAssertFail("fFound", MAGIC_EFFECTS_CPP, 0x35f, 0);
    } else {
        if (effect->target_indices_0f0.GetCount() != 0) {
            slot = &g_combat_state->effect_slots[slot_index];
            slot->active = 1;
            slot->effect_id = spell_id;
            slot->amount = effect->definition.duration_scale;
            slot->duration_0d = duration;
            RebuildPartyEffectBlock0050E700();
            RequestRedraw(0x800100);
            effect->result_124.applied = 1;
            effect->result_124.result.count += CountActiveCharacters();
        }
        for (index = 0; index < effect->monster_ids_0e0.GetCount(); ++index) {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0x342, MAGIC_EFFECTS_CPP, *effect->monster_ids_0e0.GetAt(index), 1);
            monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
            if (monster_info == 0) {
                srAssertFail("pMonsterInfo", MAGIC_EFFECTS_CPP, 0x343, 0);
            }
            if (monster_info->fInCombat == 0) {
                MonsterGroupEnterCombat(GetMonsterGroupByListIndex(GetMonsterGroupIndexByID(
                    0x347, MAGIC_EFFECTS_CPP, monster_info->monster_group_id, 1)));
                if (monster_info->fInCombat == 0) {
                    continue;
                }
            }
            slot = &monster_info->pCombat->effect_slots_3e[slot_index];
            if (slot->active == 0 || slot->effect_id != spell_id) {
                SetMonsterSpellIcon(monster_info->monster, g_effect_visual_table[spell_id][1], 1);
            }
            slot->active = 1;
            slot->effect_id = spell_id;
            slot->amount = effect->definition.duration_scale;
            slot->duration_0d = duration;
            RebuildMonsterDerivedStats(monster_info->location_id);
            if (source_character != -1) {
                W8MonsterRecord* monster = GetMonsterDataForInfo(monster_info);
                monster_info->pCombat->character_hate[source_character] +=
                    monster->effective_level_24f * effect->definition.duration_scale;
            }
            effect->result_124.applied = 1;
            ++effect->result_124.result.count;
        }
    }
    if (effect->result_124.result.count != 0) {
        if (GetTextBoxMode() != 0) {
            AppendToLastTextLine(effect->result_124.reported == 0 ? L" -- " : L", ", -1);
            SetTextBoxMode(1, -1);
        }
        AppendToLastTextLine(
            FormatWideString(gppStringList[0x1a4], effect->result_124.result.count, -1), -1);
        effect->result_124.reported = 1;
    }
}

/* The second combat-effect block: the six defensive slots past the offensive
   nine. Same dance as ApplyCombatEffectSlot, minus the hate and the report. */
// FUNCTION: WIZ8 0x0054d380
void ApplyDefenseEffectSlot(W8SpellEffectEntry* effect)
{
    W8MonsterInfo* monster_info;
    W8EffectSlot* slot;
    unsigned int duration;
    const int* table;
    int spell_id;
    int slot_index;
    int index;

    spell_id = effect->kind;
    if (gXStatus.fCombatMode == 0) {
        srAssertFail("gXStatus.fCombatMode", MAGIC_EFFECTS_CPP, 0x3c5, 0);
    }
    duration = effect->definition.duration_per_power * effect->definition.duration_scale +
               effect->definition.duration_base;
    if (duration != 9999) {
        unsigned int roll;
        ++duration;
        roll = Random(4);
        if (roll == 0) {
            if (duration > 1) {
                ++duration;
            }
        } else if (roll == 1 && duration < 3) {
            ++duration;
        }
        AdjustIntegerByPercent(&duration, effect->definition.percent);
    }
    slot_index = 0;
    table = g_combat_effect_slot_spells_00616dd8;
    while (spell_id != *table) {
        ++table;
        ++slot_index;
        if (table >= g_combat_effect_slot_spells_00616dd8 + 9) {
            srAssertFail("fFound", MAGIC_EFFECTS_CPP, 0x3f1, 0);
            return;
        }
    }
    if (effect->target_indices_0f0.GetCount() != 0) {
        slot = &g_combat_state->effect_slots_85a[slot_index];
        slot->active = 1;
        slot->effect_id = spell_id;
        slot->amount = effect->definition.duration_scale;
        slot->duration_0d = duration;
        RebuildPartyEffectBlock0050E700();
        RequestRedraw(0x800100);
        effect->result_124.applied = 1;
    }
    for (index = 0; index < effect->monster_ids_0e0.GetCount(); ++index) {
        unsigned int monster_index = MonsterGetIndexByLocationID(
            0x3d8, MAGIC_EFFECTS_CPP, *effect->monster_ids_0e0.GetAt(index), 1);
        monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
        if (monster_info == 0) {
            srAssertFail("pMonsterInfo", MAGIC_EFFECTS_CPP, 0x3d9, 0);
        }
        if (monster_info->fInCombat == 0) {
            MonsterGroupEnterCombat(GetMonsterGroupByListIndex(GetMonsterGroupIndexByID(
                0x3e0, MAGIC_EFFECTS_CPP, monster_info->monster_group_id, 1)));
            if (monster_info->fInCombat == 0) {
                continue;
            }
        }
        slot = &monster_info->pCombat->effect_slots_d7[slot_index];
        if (slot->active == 0 || slot->effect_id != spell_id) {
            SetMonsterSpellIcon(monster_info->monster, g_effect_visual_table[spell_id][1], 1);
        }
        slot->active = 1;
        slot->effect_id = spell_id;
        slot->amount = effect->definition.duration_scale;
        slot->duration_0d = duration;
        RebuildMonsterDerivedStats(monster_info->location_id);
        effect->result_124.applied = 1;
    }
}

/* The affliction spells: each listed target takes either a weighted-random
   condition attack or a straight damage roll, about a coin flip each. With
   verbose reporting off the accumulated result is flushed at the end. */
// FUNCTION: WIZ8 0x0054fa30
void ResolveAfflictionAgainstTargets(W8SpellEffectEntry* effect)
{
    unsigned int magnitude;
    unsigned int roll;
    int realm;
    int power_level;
    unsigned char verbose;
    W8CombatSlot target;
    W8MonsterInfo* monster_info;
    W8SpellEffectResult* result;
    int index;
    int weight_index;
    int party_slot;
    char landed;

    realm = g_spell_records[effect->kind].realm;
    power_level = effect->definition.power_level;
    verbose = g_settings_6850c8.verbose_combat_messages;
    TargetSourceIsCharacter(&effect->Source, 1);
    for (index = 0; index < effect->target_indices_0f0.GetCount(); ++index) {
        unsigned int duration =
            effect->definition.duration_per_power * effect->definition.duration_scale +
            effect->definition.duration_base;
        if (duration != 9999) {
            unsigned int jitter;
            ++duration;
            jitter = Random(4);
            if (jitter == 0) {
                if (duration > 1) {
                    ++duration;
                }
            } else if (jitter == 1 && duration < 3) {
                ++duration;
            }
            AdjustIntegerByPercent(&duration, effect->definition.percent);
        }
        party_slot = *effect->target_indices_0f0.GetAt(index);
        if (party_slot == -1) {
            srAssertFail("iChar != -1", MAGIC_EFFECTS_CPP, 0x8ca, 0);
        }
        target.iType = W8_TARGET_KIND_CHARACTER;
        target.iChar = party_slot;
        roll = Random(100);
        if (roll < 0x32) {
            for (weight_index = 0; weight_index < W8_CONDITION_COUNT; ++weight_index) {
                int weight = g_affliction_condition_weights_0061e208[weight_index];
                if (weight != 0) {
                    if (roll < static_cast<unsigned int>(weight)) {
                        landed = ResolveAttackOnTarget00551BA0(&effect->Source, &target,
                                                               weight_index, realm, power_level, 0,
                                                               duration, verbose, verbose, 0);
                        if (landed == 0) {
                            ++effect->result_124.result.condition_counts[weight_index];
                        }
                        break;
                    }
                    roll -= weight;
                }
            }
        } else {
            char resisted;
            magnitude = RollDice(&effect->definition.magnitude);
            if (magnitude != 9999) {
                AdjustIntegerByPercent(&magnitude, effect->definition.percent);
            }
            ReduceMagnitudeByResistance(&magnitude, &target, realm, power_level);
            resisted = 0;
            if (magnitude == 0) {
                if (verbose == 0) {
                    continue;
                }
                if (target.iType == W8_TARGET_KIND_MONSTER) {
                    PostMonsterNotice(
                        MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                            0xeae, MAGIC_EFFECTS_CPP, target.iMonsterID, 1)),
                        gppStringList[0x1b3]);
                } else {
                    PostCharacterNotice(target.iChar, gppStringList[0x1b3]);
                }
                resisted = 1;
            }
            if (resisted == 0) {
                result = verbose == 0 ? &effect->result_124.result : 0;
                ApplyDamageToCharacter(party_slot, magnitude, 0, verbose != 0, 0, result, 0);
            }
        }
    }
    for (index = 0; index < effect->monster_ids_0e0.GetCount(); ++index) {
        unsigned int duration =
            effect->definition.duration_per_power * effect->definition.duration_scale +
            effect->definition.duration_base;
        if (duration != 9999) {
            unsigned int jitter;
            ++duration;
            jitter = Random(4);
            if (jitter == 0) {
                if (duration > 1) {
                    ++duration;
                }
            } else if (jitter == 1 && duration < 3) {
                ++duration;
            }
            AdjustIntegerByPercent(&duration, effect->definition.percent);
        }
        monster_info = MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
            0x907, MAGIC_EFFECTS_CPP, *effect->monster_ids_0e0.GetAt(index), 1));
        if (monster_info == 0) {
            srAssertFail("pMonsterInfo", MAGIC_EFFECTS_CPP, 0x908, 0);
        }
        target.iType = W8_TARGET_KIND_MONSTER;
        target.iMonsterID = monster_info->location_id;
        roll = Random(100);
        if (roll < 0x32) {
            for (weight_index = 0; weight_index < W8_CONDITION_COUNT; ++weight_index) {
                int weight = g_affliction_condition_weights_0061e208[weight_index];
                if (weight != 0) {
                    if (roll < static_cast<unsigned int>(weight)) {
                        landed = ResolveAttackOnTarget00551BA0(&effect->Source, &target,
                                                               weight_index, realm, power_level, 0,
                                                               duration, verbose, verbose, 0);
                        if (landed == 0) {
                            ++effect->result_124.result.condition_counts[weight_index];
                        }
                        break;
                    }
                    roll -= weight;
                }
            }
        } else {
            char resisted;
            magnitude = RollDice(&effect->definition.magnitude);
            if (magnitude != 9999) {
                AdjustIntegerByPercent(&magnitude, effect->definition.percent);
            }
            ReduceMagnitudeByResistance(&magnitude, &target, realm, power_level);
            resisted = 0;
            if (magnitude == 0) {
                if (verbose == 0) {
                    continue;
                }
                if (target.iType == W8_TARGET_KIND_MONSTER) {
                    PostMonsterNotice(
                        MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                            0xeae, MAGIC_EFFECTS_CPP, target.iMonsterID, 1)),
                        gppStringList[0x1b3]);
                } else {
                    PostCharacterNotice(target.iChar, gppStringList[0x1b3]);
                }
                resisted = 1;
            }
            if (resisted == 0) {
                result = verbose == 0 ? &effect->result_124.result : 0;
                ApplyDamageToMonster(monster_info, magnitude, &effect->Source, 0, verbose != 0, 0,
                                     result, 0);
            }
        }
    }
    if (verbose == 0) {
        ReportSpellEffectResult(effect);
    }
}

/* The mass-damage spells: every listed character takes the raw dice roll,
   every listed monster the percentage-adjusted one, each shrunk by realm
   resistance. The monster pass reuses the character target slot, so a
   resisted monster reports under the last character's name. With verbose
   reporting off the totals go out as one line and the report queue drains. */
// FUNCTION: WIZ8 0x00550c10
void DamageTargetsAndReport(W8SpellEffectEntry* effect)
{
    W8SpellEffectResult* result;
    W8MonsterInfo* monster_info;
    unsigned char verbose;
    unsigned int magnitude;
    unsigned int total;
    unsigned int hits;
    int party_slot;
    int index;
    W8CombatSlot target;
    W8SpellDamageReport* report;

    result = &effect->result_124.result;
    verbose = g_settings_6850c8.verbose_combat_messages;
    total = 0;
    hits = 0;
    for (index = 0; index < effect->target_indices_0f0.GetCount(); ++index) {
        party_slot = *effect->target_indices_0f0.GetAt(index);
        if (party_slot == -1) {
            srAssertFail("iChar != -1", MAGIC_EFFECTS_CPP, 0xb1f, 0);
        }
        ResetCombatSlot(&target);
        target.iType = W8_TARGET_KIND_CHARACTER;
        target.iChar = party_slot;
        magnitude = RollDice(&effect->definition.magnitude);
        ReduceMagnitudeByResistance(&magnitude, &target, g_spell_records[effect->kind].realm,
                                    effect->definition.power_level);
        if (magnitude == 0) {
            if (g_settings_6850c8.verbose_combat_messages != 0) {
                if (target.iType == W8_TARGET_KIND_MONSTER) {
                    PostMonsterNotice(
                        MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                            0xeae, MAGIC_EFFECTS_CPP, target.iMonsterID, 1)),
                        gppStringList[0x1b3]);
                } else {
                    PostCharacterNotice(target.iChar, gppStringList[0x1b3]);
                }
            }
        } else {
            DamageCharacter(party_slot, magnitude, verbose);
            total += magnitude;
            ++hits;
        }
    }
    TargetSourceIsCharacter(&effect->Source, 1);
    for (index = 0; index < effect->monster_ids_0e0.GetCount(); ++index) {
        monster_info = MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
            0xb39, MAGIC_EFFECTS_CPP, *effect->monster_ids_0e0.GetAt(index), 1));
        if (monster_info == 0) {
            srAssertFail("pMonsterInfo", MAGIC_EFFECTS_CPP, 0xb3a, 0);
        }
        magnitude = RollDice(&effect->definition.magnitude);
        if (magnitude != 9999) {
            AdjustIntegerByPercent(&magnitude, effect->definition.percent);
        }
        ReduceMagnitudeByResistance(&magnitude, &target, g_spell_records[effect->kind].realm,
                                    effect->definition.power_level);
        if (magnitude == 0) {
            if (g_settings_6850c8.verbose_combat_messages != 0) {
                if (target.iType == W8_TARGET_KIND_MONSTER) {
                    PostMonsterNotice(
                        MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                            0xeae, MAGIC_EFFECTS_CPP, target.iMonsterID, 1)),
                        gppStringList[0x1b3]);
                } else {
                    PostCharacterNotice(target.iChar, gppStringList[0x1b3]);
                }
            }
        } else {
            ApplyDamageToMonster(monster_info, magnitude, &effect->Source, 0, verbose != 0, 0,
                                 verbose != 0 ? 0 : result, 0);
            total += magnitude;
            ++hits;
        }
    }
    if (verbose == 0) {
        if (hits != 0) {
            if (GetTextBoxMode() != 0) {
                AppendToLastTextLine(effect->result_124.reported == 0 ? L" -- " : L", ", -1);
                SetTextBoxMode(1, -1);
            }
            if (effect->monster_ids_0e0.GetCount() + effect->target_indices_0f0.GetCount() == 1) {
                AppendToLastTextLine(FormatWideString(gppStringList[0x1a6], total, -1), -1);
            } else {
                AppendToLastTextLine(FormatWideString(gppStringList[0x1a7], hits, total / hits, -1),
                                     -1);
            }
            SetTextBoxMode(1, -1);
            effect->result_124.reported = 1;
            effect->result_124.applied = 1;
        }
        while (effect->result_124.result.reports.GetCount() > 0) {
            report = *effect->result_124.result.reports.GetAt(0);
            effect->result_124.result.reports.RemoveAt(0);
            if (report != 0) {
                if (report->kind == 1) {
                    PostCharacterNotice(report->value, L"%s!",
                                        gppStringList[g_condition_notices_0061E570[0x49]]);
                } else if (report->kind == 3) {
                    ShowNoticef(9, L"%s %s!", report->text,
                                gppStringList[g_condition_notices_0061E570[0x49]]);
                }
                free(report);
            }
        }
    }
}

/* The missile-destroying spell: every arrow, bolt and bullet stack held by
   the listed characters or sitting in the party pool rolls `argument * 10`
   percent per unit and sheds what fails. The three class totals each get
   their own summary line. */
// FUNCTION: WIZ8 0x005510b0
void DestroyMissilesOnTargets(W8SpellEffectEntry* effect)
{
    W8Character* character;
    W8ItemInstance* item;
    unsigned char verbose;
    unsigned int chance;
    unsigned int destroyed;
    unsigned int unit;
    int totals[3];
    int index;
    int slot;

    verbose = g_settings_6850c8.verbose_combat_messages;
    chance = effect->definition.duration_scale * 10;
    totals[2] = 0;
    totals[1] = 0;
    totals[0] = 0;
    for (index = 0; index < effect->target_indices_0f0.GetCount(); ++index) {
        int party_slot = *effect->target_indices_0f0.GetAt(index);
        if (party_slot == -1) {
            srAssertFail("iChar != -1", MAGIC_EFFECTS_CPP, 0xbb6, 0);
        }
        character = &g_status_685170.buffers.characters[party_slot];
        for (slot = 0; slot < 12; ++slot) {
            item = &character->equipment[slot];
            if (item->item_id != -1 && (g_item_records[item->item_id].equip_class == 0x10 ||
                                        (g_item_records[item->item_id].equip_class > 0x12 &&
                                         g_item_records[item->item_id].equip_class < 0x15))) {
                destroyed = 0;
                for (unit = 0; unit < item->stack_count; ++unit) {
                    if (Random(100) < chance) {
                        ++destroyed;
                    }
                }
                if (destroyed != 0) {
                    if (destroyed == item->stack_count) {
                        EmptyItemRecord(item, character, 1);
                    } else {
                        item->stack_count -= static_cast<char>(destroyed);
                    }
                }
                if (g_item_records[item->item_id].equip_class == 0x10) {
                    totals[0] += destroyed;
                } else if (g_item_records[item->item_id].equip_class == 0x13) {
                    totals[2] += destroyed;
                } else if (g_item_records[item->item_id].equip_class == 0x14) {
                    totals[1] += destroyed;
                }
            }
        }
        for (slot = 0; slot < 8; ++slot) {
            item = &character->backpack[slot];
            if (item->item_id != -1 && (g_item_records[item->item_id].equip_class == 0x10 ||
                                        (g_item_records[item->item_id].equip_class > 0x12 &&
                                         g_item_records[item->item_id].equip_class < 0x15))) {
                destroyed = 0;
                for (unit = 0; unit < item->stack_count; ++unit) {
                    if (Random(100) < chance) {
                        ++destroyed;
                    }
                }
                if (destroyed != 0) {
                    if (destroyed == item->stack_count) {
                        EmptyItemRecord(item, character, 1);
                    } else {
                        item->stack_count -= static_cast<char>(destroyed);
                    }
                }
                if (g_item_records[item->item_id].equip_class == 0x10) {
                    totals[0] += destroyed;
                } else if (g_item_records[item->item_id].equip_class == 0x13) {
                    totals[2] += destroyed;
                } else if (g_item_records[item->item_id].equip_class == 0x14) {
                    totals[1] += destroyed;
                }
            }
        }
    }
    if (effect->target_indices_0f0.GetCount() > 0) {
        for (index = 0; index < g_status_685170.party_item_count_1791; ++index) {
            item = &g_status_685170.party_item_pool_0021[index];
            if (item->item_id != -1 && (g_item_records[item->item_id].equip_class == 0x10 ||
                                        (g_item_records[item->item_id].equip_class > 0x12 &&
                                         g_item_records[item->item_id].equip_class < 0x15))) {
                destroyed = 0;
                for (unit = 0; unit < item->stack_count; ++unit) {
                    if (Random(100) < chance) {
                        ++destroyed;
                    }
                }
                if (destroyed != 0) {
                    if (destroyed == item->stack_count) {
                        EmptyItemRecord(item, 0, 1);
                    } else {
                        item->stack_count -= static_cast<char>(destroyed);
                    }
                }
                if (g_item_records[item->item_id].equip_class == 0x10) {
                    totals[0] += destroyed;
                } else if (g_item_records[item->item_id].equip_class == 0x13) {
                    totals[2] += destroyed;
                } else if (g_item_records[item->item_id].equip_class == 0x14) {
                    totals[1] += destroyed;
                }
            }
        }
    }
    if (totals[2] + totals[1] + totals[0] != 0) {
        for (index = 0; index < 3; ++index) {
            if (totals[index] != 0) {
                if (verbose == 0) {
                    AppendToLastTextLine(effect->result_124.reported == 0 ? L" -- " : L", ", -1);
                }
                SetTextBoxMode(verbose == 0, -1);
                if (index == 0) {
                    AppendToLastTextLine(FormatWideString(gppStringList[0x1fa], totals[0], -1), -1);
                } else if (index == 1) {
                    AppendToLastTextLine(FormatWideString(gppStringList[0x1fb], totals[1], -1), -1);
                } else if (index == 2) {
                    AppendToLastTextLine(FormatWideString(gppStringList[0x1fc], totals[2], -1), -1);
                }
                if (verbose == 0) {
                    SetTextBoxMode(1, -1);
                }
                effect->result_124.reported = 1;
            }
        }
        effect->result_124.applied = 1;
    }
}

/* The lure effect, spell 0x26: every hostile monster in the world-far-clip
   box around the target point that the effect can see and that is not already
   spoken for gets a resistance roll. A monster that fails is switched to the
   controlled state - its whole group when the party is out of combat, just
   itself in combat - and one that resists is switched the other way. The
   0x400-byte query buffer is not freed, matching retail. */
// FUNCTION: WIZ8 0x00551500
void ApplyMonsterControlToNearbyMonsters(W8SpellEffectEntry* effect)
{
    W8MonsterInfo* monster_info;
    W8CombatSlot target;
    srVector3T<float> center;
    srVector3T<float> lower;
    srVector3T<float> upper;
    srVector3T<float> eye;
    unsigned long* location_ids;
    unsigned int count;
    unsigned int index;
    float far_clip;

    far_clip = static_cast<float>(WorldGetFarClip(GetWorld()));
    center = effect->target.point;
    center.y += g_default_world_height_00603ac8 * g_float_005ebc7c;
    lower.x = center.x - far_clip;
    lower.y = center.y - far_clip;
    lower.z = center.z - far_clip;
    upper.x = center.x + far_clip;
    upper.y = center.y + far_clip;
    upper.z = center.z + far_clip;
    location_ids = static_cast<unsigned long*>(operator new(0x400));
    count = g_octree_6598a4->QueryLocationsInBox(&location_ids, &lower, &upper, 0);
    for (index = 0; index < count; ++index) {
        monster_info = MonsterInfoFromID(0xc7c, MAGIC_EFFECTS_CPP, location_ids[index], 1);
        if (monster_info->monster->IsDying() != 0) {
            continue;
        }
        if ((gXStatus.fCombatMode == 0 && monster_info->monster->linked_navigator_05c != 0) ||
            monster_info->ubDisposition != W8_DISPOSITION_HOSTILE) {
            continue;
        }
        eye = monster_info->monster->movement_0c0.position_040;
        eye.y += monster_info->monster->movement_0c0.height_offset_0b8;
        if (g_octree_6598a4->HasLineOfSight(&eye, &center, 1) == 0) {
            continue;
        }
        if (monster_info->control_state < 0 || monster_info->control_state >= 2) {
            continue;
        }
        if (monster_info->condition_turns[0xc] != 0 || monster_info->highest_condition >= 0xf) {
            SetMonsterControlState(monster_info, 0);
            continue;
        }
        ResetCombatSlot(&target);
        target.iType = W8_TARGET_KIND_MONSTER;
        target.iMonsterID = monster_info->location_id;
        if (MonsterResistsSpellEffect(&target, effect->definition.power_level) == 0) {
            if (gXStatus.fCombatMode != 0) {
                SetMonsterControlState(monster_info, 1);
            } else {
                SetMonsterGroupControlState(
                    GetMonsterGroupByListIndex(GetMonsterGroupIndexByID(
                        0xcbb, MAGIC_EFFECTS_CPP, monster_info->monster_group_id, 1)),
                    1);
            }
            effect->result_124.applied = 1;
        } else {
            if (gXStatus.fCombatMode != 0) {
                SetMonsterControlState(monster_info, 2);
            } else {
                SetMonsterGroupControlState(
                    GetMonsterGroupByListIndex(GetMonsterGroupIndexByID(
                        0xcaf, MAGIC_EFFECTS_CPP, monster_info->monster_group_id, 1)),
                    2);
            }
        }
    }
}

/* The reveal-item effect on a character target: an ordinary condition-nine
   cure pass first, then the binding reveal. Only a character target produces
   bindings to report; a result of one or two appends the matching line and,
   on the partial answer, clears the applied flag. */
// FUNCTION: WIZ8 0x005517c0
char RevealItemBindingsToTarget(W8SpellEffectEntry* effect)
{
    unsigned char verbose;
    char applied;
    int result;

    verbose = g_settings_6850c8.verbose_combat_messages;
    applied = TryCureConditionOnTargets(effect, 9, 0);
    if (effect->target.iType == W8_TARGET_KIND_CHARACTER) {
        result = RevealCharacterItemBindings(
            effect->target.iChar, effect->definition.duration_scale, effect->definition.percent);
        if (result == 0) {
            return 0;
        }
        if (result > 0 && result < 3) {
            if (verbose == 0) {
                effect->result_124.applied = 1;
                AppendToLastTextLine(effect->result_124.reported == 0 ? L" -- " : L", ", -1);
                SetTextBoxMode(1, -1);
            }
            if (result == 2) {
                AppendToLastTextLine(
                    FormatWideString(gppStringList[0x1bc],
                                     g_status_685170.buffers.characters[effect->target.iChar].name,
                                     -1),
                    -1);
                effect->result_124.reported = 1;
                return applied;
            }
            AppendToLastTextLine(
                FormatWideString(gppStringList[0x1bd],
                                 g_status_685170.buffers.characters[effect->target.iChar].name, -1),
                -1);
            applied = 0;
            effect->result_124.reported = 1;
        }
    }
    return applied;
}

/* Charm the target monster: when the condition save fails, the rolled amount
   beats the stored effect strength and raises it. A charm that lands also
   gets its "<name> <effect>" line into the NPC quote bubble. A resisting or
   backfiring monster answers with a refusal quote instead. */
// FUNCTION: WIZ8 0x00550a00
void ApplyCharmToMonsterTarget(W8SpellEffectEntry* effect)
{
    W8CombatSlot* target;
    W8MonsterInfo* monster_info;
    W8MonsterRecord* monster;
    unsigned int magnitude;
    const wchar_t* notice;
    const unsigned short* name;
    wchar_t* formatted;

    target = &effect->target;
    if (target->iType != W8_TARGET_KIND_MONSTER) {
        srAssertFail("pQueue->Target.iType == TARGET_TYPE_MONSTER", MAGIC_EFFECTS_CPP, 0xace, 0);
    }
    if (target->iMonsterID == -1) {
        srAssertFail("pQueue->Target.iMonsterID != -1", MAGIC_EFFECTS_CPP, 0xacf, 0);
    }
    monster_info = MonsterInfoFromID(0xad1, MAGIC_EFFECTS_CPP, target->iMonsterID, 1);
    if (effect->Source.fBackfire != 0) {
        monster = GetMonsterDataForInfo(monster_info);
        if (Random(100) < (monster->attribute_values_d1[4] + monster->attribute_values_d1[1]) / 2) {
            QueueNpcScriptLine(0x16, 0, 0, 0);
        }
        return;
    }
    if (TargetResistsCondition(target, 4, effect->definition.power_level, 0) != 0) {
        if (g_settings_6850c8.verbose_combat_messages != 0) {
            if (target->iType == W8_TARGET_KIND_MONSTER) {
                PostMonsterNotice(MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                                      0xeae, MAGIC_EFFECTS_CPP, target->iMonsterID, 1)),
                                  gppStringList[0x1b3]);
            } else {
                PostCharacterNotice(target->iChar, gppStringList[0x1b3]);
            }
        }
        return;
    }
    magnitude = effect->definition.duration_scale * 7;
    AdjustIntegerByPercent(&magnitude, effect->definition.percent);
    ReduceMagnitudeByResistance(&magnitude, target, 4, effect->definition.power_level);
    if (static_cast<char>(magnitude) <= static_cast<char>(monster_info->effect_2de)) {
        return;
    }
    if (monster_info->effect_2de == 0) {
        SetMonsterSpellIcon(monster_info->monster, 0x26, 1);
    }
    monster_info->effect_2de = static_cast<char>(magnitude);
    if (g_settings_6850c8.verbose_combat_messages != 0) {
        PostMonsterNotice(monster_info, gppStringList[0x1ac]);
    } else {
        effect->result_124.applied = 1;
    }
    notice = gppStringList[0x1ac];
    name = GetMonsterName(monster_info, 0, 0);
    formatted = FormatWideString(g_format_s_space_s_00617584, name, notice);
    DisplayNpcQuote00529570(formatted, 0);
}

/* The tame/refuse spell: a monster target that fails its resistance check
   answers with a refusal quote, and a backfired cast asks for one outright.
   Otherwise the dialogue NPC's staged refusal runs. */
// FUNCTION: WIZ8 0x005508e0
void ResolveCharmRefusal(W8SpellEffectEntry* effect)
{
    W8MonsterInfo* monster_info;
    W8MonsterRecord* monster;
    int realm;

    realm = g_spell_records[effect->kind].realm;
    if (TargetSourceIsCharacter(&effect->Source, 0) == 0) {
        srAssertFail("SourceIsCharacter(&(pQueue->Source))", MAGIC_EFFECTS_CPP, 0xaa9, 0);
    }
    if (effect->target.iType != W8_TARGET_KIND_MONSTER) {
        srAssertFail("pQueue->Target.iType == TARGET_TYPE_MONSTER", MAGIC_EFFECTS_CPP, 0xaaa, 0);
    }
    monster_info = MonsterInfoFromID(0xaac, MAGIC_EFFECTS_CPP, effect->target.iMonsterID, 1);
    if (effect->Source.fBackfire == 0) {
        if (TargetResistsCondition(&effect->target, realm, effect->definition.power_level, 0) !=
            0) {
            QueueNpcScriptLine(0x69, 0, 0, 0);
            return;
        }
        QueueDialogueNpcRefusal00576DA0();
    } else {
        monster = GetMonsterDataForInfo(monster_info);
        if (Random(100) < (monster->attribute_values_d1[4] + monster->attribute_values_d1[1]) / 2) {
            QueueNpcScriptLine(0x16, 0, 0, 0);
            return;
        }
    }
}

/* Weaken the running combat effects: each of the dispel-able spells in the
   first combat-effect block loses `argument` ticks, jittered and scaled the
   same way a fresh application would be, plus the level gap between the
   reference spell and the spell in the slot. A slot that runs out is
   dropped and rebuilt. */
// FUNCTION: WIZ8 0x0054d100
void ReduceCombatEffectDurations(W8SpellEffectEntry* effect)
{
    W8MonsterInfo* monster_info;
    W8EffectSlot* slot;
    unsigned int duration;
    int reduce;
    const int* table;
    int slot_index;
    int index;

    if (gXStatus.fCombatMode == 0) {
        srAssertFail("gXStatus.fCombatMode", MAGIC_EFFECTS_CPP, 900, 0);
    }
    duration = effect->definition.duration_scale;
    if (duration != 9999) {
        unsigned int roll;
        ++duration;
        roll = Random(4);
        if (roll == 0) {
            if (duration > 1) {
                ++duration;
            }
        } else if (roll == 1 && duration < 3) {
            ++duration;
        }
        AdjustIntegerByPercent(&duration, effect->definition.percent);
    }
    slot_index = 0;
    for (table = g_combat_effect_slot_spells_00616db4; table < g_combat_effect_slot_spells_00616dd8;
         ++table, ++slot_index) {
        switch (*table) {
        case 0x30:
        case 0x4c:
        case 0x50:
        case 0x51:
        case 0x5d:
            reduce =
                g_spell_records[72].spell_level - g_spell_records[*table].spell_level + duration;
            if (reduce > 0) {
                slot = &g_combat_state->effect_slots[slot_index];
                if (effect->target_indices_0f0.GetCount() != 0 && slot->active != 0) {
                    if (static_cast<unsigned int>(reduce) < slot->duration_0d) {
                        slot->duration_0d -= reduce;
                    } else {
                        slot->active = 0;
                        slot->effect_id = 0;
                        slot->amount = 0;
                        slot->duration_0d = 0;
                        RebuildPartyEffectBlock0050E700();
                        InvalidateMainGameEffectHud();
                        RequestRedraw(0x800100);
                    }
                    effect->result_124.applied = 1;
                }
                for (index = 0; index < effect->monster_ids_0e0.GetCount(); ++index) {
                    unsigned int monster_index = MonsterGetIndexByLocationID(
                        0x3a7, MAGIC_EFFECTS_CPP, *effect->monster_ids_0e0.GetAt(index), 1);
                    monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
                    if (monster_info == 0) {
                        srAssertFail("pMonsterInfo", MAGIC_EFFECTS_CPP, 0x3a8, 0);
                    }
                    slot = &monster_info->pCombat->effect_slots_3e[slot_index];
                    if (slot->active != 0) {
                        if (static_cast<unsigned int>(reduce) < slot->duration_0d) {
                            slot->duration_0d -= reduce;
                        } else {
                            if (slot->active != 0) {
                                SetMonsterSpellIcon(monster_info->monster,
                                                    g_effect_visual_table[slot->effect_id][1], 0);
                            }
                            slot->active = 0;
                            slot->effect_id = 0;
                            slot->amount = 0;
                            slot->duration_0d = 0;
                            RebuildMonsterDerivedStats(monster_info->location_id);
                        }
                        effect->result_124.applied = 1;
                    }
                }
            }
        }
    }
}

/* Reflection handling at the end of a cast: when the spell can be aimed and
   the source was not already a bounce, the strongest reflection enchantment
   among the targets soaks the cast - the effect re-targets at a clamped
   power, the reflector is charged one use, and the notice goes out. */
// FUNCTION: WIZ8 0x0054c930
void FinishSpellEffectTargets(W8SpellEffectEntry* effect)
{
    W8Character* character;
    W8MonsterInfo* monster_info;
    W8MonsterInfo* info;
    unsigned int best;
    unsigned int amount;
    W8TargetSource source_copy;
    W8CombatSlot target_copy;
    int index;
    unsigned int monster_index;

    character = 0;
    monster_info = 0;
    best = 0;
    if (MonsterCanAimSpell005474B0(effect->kind) != 0 && effect->Source.unknown_18[2] == 0 &&
        effect->Source.fBackfire == 0 && effect->Source.fReflection == 0) {
        for (index = 0; index < effect->target_indices_0f0.GetCount(); ++index) {
            W8Character* member =
                &g_status_685170.buffers.characters[*effect->target_indices_0f0.GetAt(index)];
            if (member->highest_condition < 0x12 && member->enchantments[4].value_08 != 0 &&
                best < member->enchantments[4].value_00) {
                best = member->enchantments[4].value_00;
                character = member;
            }
        }
        for (index = 0; index < effect->monster_ids_0e0.GetCount(); ++index) {
            monster_index = MonsterGetIndexByLocationID(0x2b1, MAGIC_EFFECTS_CPP,
                                                        *effect->monster_ids_0e0.GetAt(index), 0);
            if (monster_index == 0xffffffff) {
                effect->monster_ids_0e0.RemoveAt(index);
                --index;
                continue;
            }
            info = MonsterGetScriptPartByLocationIndex(monster_index);
            if (info == 0) {
                srAssertFail("pMonsterInfo", MAGIC_EFFECTS_CPP, 700, 0);
            }
            if (info->highest_condition < 0x12 && info->enchantments[4].value_08 != 0 &&
                best < info->enchantments[4].value_00) {
                character = 0;
                best = info->enchantments[4].value_00;
                monster_info = info;
            }
        }
        if (best != 0) {
            amount = effect->definition.duration_scale;
            if (best <= static_cast<unsigned int>(effect->definition.duration_scale)) {
                amount = best;
            }
            source_copy = effect->Source;
            effect->definition.duration_scale = amount;
            target_copy = effect->target;
            PrepareSpellTarget004FEA50(effect->kind, &source_copy, &target_copy);
            /* The bounced cast carries the reflection flag so it cannot
               reflect a second time. */
            source_copy.fReflection = 1;
            CastSpellFromSource(effect->kind, &source_copy, &target_copy,
                                effect->definition.duration_scale, effect->definition.percent, 0, 0,
                                0, 0, 0, 0);
            if (character == 0) {
                if (monster_info != 0) {
                    PostMonsterNotice(monster_info, gppStringList[0x198]);
                    if (--monster_info->enchantments[4].value_00 == 0) {
                        ClearMonsterEnchantmentSlot(monster_info->location_id, 4);
                    }
                }
            } else {
                PostCharacterNotice(CharacterPointerToPartySlot(character), gppStringList[0x198]);
                if (--character->enchantments[4].value_00 == 0) {
                    ClearCharacterEnchantmentSlot(CharacterPointerToPartySlot(character), 4);
                }
            }
        }
    }
}

/* Per-tick handler for the active 0x3e party combat effect: each live slot
   detonates a radius blast around the party - every monster in range takes a
   resistance-checked dice roll scaled by the slot's stored magnitude.  In
   verbose mode each damage line goes straight out; otherwise the results
   accumulate and print as one summary plus per-target condition reports. */
// FUNCTION: WIZ8 0x00552ef0
void TickRadiusBlastEffectSlots(W8EffectSlot* effect_slots)
{
    W8GrowableVector<int> found;
    W8SpellEffectResult local_result;
    W8TargetSource source;
    W8CombatSlot target;
    W8Dice dice;
    W8MonsterInfo* monster_info;
    unsigned int amount;
    unsigned int monster_index;
    unsigned char verbose;
    int index;
    int remaining;

    verbose = g_settings_6850c8.verbose_combat_messages;
    ResetTargetSource(&source);
    memset(static_cast<void*>(&local_result), 0, sizeof(local_result));
    memset(&target, 0, sizeof(target));
    remaining = 9;
    do {
        if (effect_slots->active != 0 && effect_slots->effect_id == 0x3e) {
            ShowNoticef(8, gppStringList[0x1ae], g_spell_records[62].display_name);
            SetTextBoxMode(1, -1);
            {
                float reach = CalcRangeDistance(W8_RANGE_SHORT);
                float radius = g_startup_world_659c0c->radius_084;
                srVector3T<float> centre = g_startup_world_659c0c->GetPosition();
                CollectMonstersWithinRadius(&centre, &centre, &found, radius + reach, 1, 0);
            }
            for (index = 0; index < found.GetCount(); ++index) {
                monster_index =
                    MonsterGetIndexByLocationID(0x11a6, MAGIC_EFFECTS_CPP, *found.GetAt(index), 1);
                monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
                if (monster_info == 0) {
                    srAssertFail("pMonsterInfo", MAGIC_EFFECTS_CPP, 0x11a7, 0);
                }
                dice = g_spell_records[62].effect_dice;
                dice.count = static_cast<char>(effect_slots->amount) * dice.count;
                amount = RollDice(&dice);
                target.iType = W8_TARGET_KIND_MONSTER;
                target.iMonsterID = monster_info->location_id;
                ReduceMagnitudeByResistance(&amount, &target, g_spell_records[62].realm,
                                            effect_slots->amount * 3);
                if (amount == 0) {
                    if (g_settings_6850c8.verbose_combat_messages != 0) {
                        if (target.iType == W8_TARGET_KIND_MONSTER) {
                            monster_index = MonsterGetIndexByLocationID(0xeae, MAGIC_EFFECTS_CPP,
                                                                        target.iMonsterID, 1);
                            monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
                            PostMonsterNotice(monster_info, gppStringList[0x1b3]);
                        } else {
                            PostCharacterNotice(target.iChar, gppStringList[0x1b3]);
                        }
                    }
                } else {
                    ApplyDamageToMonster(monster_info, amount, &source, 0, verbose != 0, 0,
                                         verbose == 0 ? &local_result : 0, 0);
                }
            }
            if (verbose == 0) {
                if (GetTextBoxMode() != 0) {
                    AppendToLastTextLine(L" -- ", -1);
                    SetTextBoxMode(1, -1);
                }
                if (local_result.count != 0) {
                    const wchar_t* line;
                    if (found.GetCount() == 1) {
                        line = FormatWideString(gppStringList[0x19a], local_result.amount, -1);
                    } else {
                        line = FormatWideString(gppStringList[0x199], local_result.count,
                                                local_result.amount / local_result.count, -1);
                    }
                    AppendToLastTextLine(line, -1);
                } else {
                    AppendToLastTextLine(gppStringList[0x1a5], -1);
                }
                index = local_result.reports.GetCount();
                while (0 < index) {
                    W8SpellDamageReport* report = *local_result.reports.GetAt(0);
                    local_result.reports.RemoveAt(0);
                    if (report != 0) {
                        if (report->kind == 1) {
                            PostCharacterNotice(report->value, L"%s!",
                                                gppStringList[g_condition_notices_0061E570[0x49]]);
                        } else if (report->kind == 3) {
                            ShowNoticef(9, L"%s %s!", report->text,
                                        gppStringList[g_condition_notices_0061E570[0x49]]);
                        }
                        free(report);
                        index = local_result.reports.GetCount();
                    }
                }
            }
        }
        ++effect_slots;
        --remaining;
        if (remaining == 0) {
            return;
        }
    } while (true);
}

/* Per-tick upkeep for the first party combat-effect block (the nine slots at
   W8CombatState+0x7c1).  Each live slot re-applies its spell to the block's
   target: the damage spells roll fresh dice every tick, the 0x5d sleep
   effect retries the condition instead, and 0x4c additionally rolls the
   shared 4/7/0x11 condition bundle on whatever survived the first checks. */
// FUNCTION: WIZ8 0x005526f0
void TickCombatEffectSlots(W8EffectSlot* effect_slots, W8CombatSlot* target)
{
    W8TargetSource source;
    W8CombatSlot char_target;
    W8Dice dice;
    W8MonsterInfo* monster_info;
    W8MonsterInfo* notice_info;
    W8MonsterRecord* monster_record;
    unsigned int difficulty;
    unsigned int amount;
    unsigned int duration;
    unsigned int condition_turns;
    unsigned int resisted_total;
    unsigned int hit_count;
    unsigned int monster_index;
    int power;
    int percent;
    int duration_base;
    int duration_scale;
    int spell_id;
    int party_slot;
    int realm;
    int index;
    char rolls_damage;
    bool sleep_type;
    unsigned char verbose;

    verbose = g_settings_6850c8.verbose_combat_messages;
    ResetTargetSource(&source);
    index = 0;
    do {
        if (effect_slots->active != 0) {
            spell_id = effect_slots->effect_id;
            power = effect_slots->amount;
            percent = effect_slots->percent;
            duration_base = g_spell_records[spell_id].duration_per_level_04d;
            duration_scale = g_spell_records[spell_id].duration_044;
            party_slot = 0;
            difficulty = GetSpellDifficulty(0, spell_id, power);
            AdjustIntegerByPercent(&difficulty, static_cast<unsigned int>(percent) >> 1);
            switch (spell_id) {
            case 0x30:
            case 0x4c:
            case 0x50:
            case 0x51:
                dice = g_spell_records[spell_id].effect_dice;
                dice.count = static_cast<char>(power) * dice.count;
                rolls_damage = 1;
                sleep_type = false;
                break;
            case 0x5d:
                rolls_damage = 0;
                sleep_type = true;
                break;
            default:
                goto next_slot;
            }
            if (target->iType == W8_TARGET_KIND_PARTY) {
                hit_count = 0;
                resisted_total = 0;
                if (verbose != 0 && !sleep_type) {
                    ShowNoticef(0xc, gppStringList[0x1ae], g_spell_records[spell_id].display_name);
                }
                do {
                    if (g_status_685170.buffers.party_rows[party_slot].occupied != 0 &&
                        g_status_685170.buffers.characters[party_slot].highest_condition < 0x12) {
                        char_target.iType = W8_TARGET_KIND_CHARACTER;
                        char_target.iChar = party_slot;
                        if (rolls_damage != 0) {
                            amount = RollDice(&dice);
                            AdjustIntegerByPercent(&amount, percent);
                            ReduceMagnitudeByResistance(
                                &amount, &char_target, g_spell_records[spell_id].realm, difficulty);
                            if (amount == 0) {
                                if (g_settings_6850c8.verbose_combat_messages != 0) {
                                    if (char_target.iType == W8_TARGET_KIND_MONSTER) {
                                        monster_index = MonsterGetIndexByLocationID(
                                            0xeae, MAGIC_EFFECTS_CPP, char_target.iMonsterID, 1);
                                        notice_info =
                                            MonsterGetScriptPartByLocationIndex(monster_index);
                                        PostMonsterNotice(notice_info, gppStringList[0x1b3]);
                                    } else {
                                        PostCharacterNotice(char_target.iChar,
                                                            gppStringList[0x1b3]);
                                    }
                                }
                            }
                            if (amount != 0) {
                                ApplyDamageToCharacter(party_slot, amount, 0, 1, 0, 0, 0);
                                resisted_total = resisted_total + amount;
                                ++hit_count;
                                if (spell_id == 0x50) {
                                    for (realm = 0; realm < 6; ++realm) {
                                        DrainCharacterRealmSpellPoints(party_slot, realm, amount,
                                                                       1);
                                    }
                                    FatigueCharacter(party_slot, amount, 0, 0);
                                }
                            }
                        }
                        if (sleep_type &&
                            TargetResistsCondition(&char_target, g_spell_records[spell_id].realm,
                                                   difficulty, 0x12) == 0) {
                            SetCharacterCondition(party_slot, 0x12, 9999, 0, 0, 1);
                        }
                        if (spell_id == 0x4c) {
                            duration = duration_scale * power + duration_base;
                            if (duration != 9999) {
                                duration = duration + 1;
                                {
                                    unsigned int roll = Random(4);
                                    if (roll == 0) {
                                        if (1 < duration) {
                                            duration = duration + 1;
                                        }
                                    } else if (roll == 1 && duration < 3) {
                                        duration = duration + 1;
                                    }
                                }
                                AdjustIntegerByPercent(&duration, percent);
                            }
                            condition_turns = duration;
                            ReduceMagnitudeByResistance(&condition_turns, target,
                                                        g_spell_records[0x4c].realm, difficulty);
                            if (condition_turns != 0) {
                                if (condition_turns == 1) {
                                    condition_turns = 2;
                                }
                                if (TargetResistsCondition(&char_target,
                                                           g_spell_records[0x4c].realm, difficulty,
                                                           4) == 0) {
                                    SetCharacterCondition(party_slot, 4, condition_turns, 0, 0, 1);
                                }
                                if (TargetResistsCondition(&char_target,
                                                           g_spell_records[0x4c].realm, difficulty,
                                                           7) == 0) {
                                    SetCharacterCondition(party_slot, 7, condition_turns,
                                                          condition_turns, 0, 1);
                                }
                                if (TargetResistsCondition(&char_target,
                                                           g_spell_records[0x4c].realm, difficulty,
                                                           0x11) == 0) {
                                    SetCharacterCondition(party_slot, 0x11, condition_turns, 0, 0,
                                                          1);
                                }
                            }
                        }
                    }
                    ++party_slot;
                } while (party_slot < 8);
                if (verbose == 0 && hit_count != 0) {
                    ShowNoticef(0xc, gppStringList[0x1af], g_spell_records[spell_id].display_name,
                                hit_count, resisted_total / hit_count);
                }
            } else if (target->iType == W8_TARGET_KIND_MONSTER) {
                monster_info = MonsterInfoFromID(0x1124, MAGIC_EFFECTS_CPP, target->iMonsterID, 1);
                if (rolls_damage != 0) {
                    amount = RollDice(&dice);
                    AdjustIntegerByPercent(&amount, percent);
                    ReduceMagnitudeByResistance(&amount, target, g_spell_records[spell_id].realm,
                                                difficulty);
                    if (amount == 0) {
                        if (g_settings_6850c8.verbose_combat_messages != 0) {
                            if (target->iType == W8_TARGET_KIND_MONSTER) {
                                monster_index = MonsterGetIndexByLocationID(
                                    0xeae, MAGIC_EFFECTS_CPP, target->iMonsterID, 1);
                                notice_info = MonsterGetScriptPartByLocationIndex(monster_index);
                                PostMonsterNotice(notice_info, gppStringList[0x1b3]);
                            } else {
                                PostCharacterNotice(target->iChar, gppStringList[0x1b3]);
                            }
                        }
                    }
                    if (amount != 0) {
                        ApplyDamageToMonster(monster_info, amount, &source, 0, 1, 0, 0, 0);
                        if (spell_id == 0x50) {
                            FatigueMonster(monster_info, amount, 0);
                            monster_info->spell_points_2f9 =
                                monster_info->spell_points_2f9 - (amount >> 1);
                        }
                    }
                }
                monster_record = GetMonsterDataForInfo(monster_info);
                if (sleep_type && monster_record->instant_death_immune_1be == 0 &&
                    TargetResistsCondition(target, g_spell_records[0x5d].realm, difficulty, 0x12) ==
                        0) {
                    SetMonsterCondition(target->iMonsterID, 0x12, 9999, 0, &source, 1);
                }
                if (monster_info->hp_current == 0) {
                    return;
                }
                if (spell_id == 0x4c) {
                    duration = duration_scale * power + duration_base;
                    if (duration != 9999) {
                        duration = duration + 1;
                        {
                            unsigned int roll = Random(4);
                            if (roll == 0) {
                                if (1 < duration) {
                                    duration = duration + 1;
                                }
                            } else if (roll == 1 && duration < 3) {
                                duration = duration + 1;
                            }
                        }
                        AdjustIntegerByPercent(&duration, percent);
                    }
                    condition_turns = duration;
                    ReduceMagnitudeByResistance(&condition_turns, target,
                                                g_spell_records[0x4c].realm, difficulty);
                    if (condition_turns != 0) {
                        if (condition_turns == 1) {
                            condition_turns = 2;
                        }
                        if (TargetResistsCondition(target, g_spell_records[0x4c].realm, difficulty,
                                                   4) == 0) {
                            SetMonsterCondition(target->iMonsterID, 4, condition_turns, 0, &source,
                                                1);
                        }
                        if (TargetResistsCondition(target, g_spell_records[0x4c].realm, difficulty,
                                                   7) == 0) {
                            SetMonsterCondition(target->iMonsterID, 7, condition_turns,
                                                condition_turns, &source, 1);
                        }
                        if (TargetResistsCondition(target, g_spell_records[0x4c].realm, difficulty,
                                                   0x11) == 0) {
                            SetMonsterCondition(target->iMonsterID, 0x11, condition_turns, 0,
                                                &source, 1);
                        }
                    }
                }
            } else {
                srAssertFail("0", MAGIC_EFFECTS_CPP, 0x116f, 0);
            }
        }
    next_slot:
        ++index;
        ++effect_slots;
        if (8 < index) {
            return;
        }
    } while (true);
}

/* The per-frame spell effect dispatcher. A queued effect that needed aiming
   and was never armed is dropped up front; everything else routes on the
   spell id. Backfired effects invert the helpful spells - cures and buffs
   become attacks on the caster's side. After the dispatch the unreported
   effect gets its summary line, the caster is charged (repeatedly, while
   stamina and realm spell points hold out), and the reflection pass runs. */
// FUNCTION: WIZ8 0x0054ba00
void ProcessSpellEffectTargets(W8SpellEffectEntry* effect)
{
    W8CameraShakeEffect* shake;
    W8WorldCameraState* camera_state;
    W8World* world;
    SOUNDPARMS* parms;
    srVector3T<float> point;
    char* sound_name;
    char* path;
    unsigned char applied;
    unsigned int level;
    unsigned int amount;
    int count;
    unsigned int percent_roll;
    int queued_spell_id;
    int spell_id;
    int condition;
    int target_type;
    int index;
    int cost;
    W8Character* character;
    W8MonsterInfo* monster_info;
    W8MonsterInfo* notice_info;
    unsigned int monster_index;
    W8ItemInstance* item;
    W8EffectSlot* slot;

    spell_id = effect->kind;
    applied = 1;
    queued_spell_id = spell_id;
    if (g_spell_records[spell_id].needs_aim_13f != 0 && IsCombatEffectSlotSpell(spell_id) == 0 &&
        (target_type = GetSpellTargetType(spell_id, 0)) != W8_TARGET_TYPE_RADIUS &&
        target_type != W8_TARGET_TYPE_PARTY) {
        return;
    }
    if (effect->Source.unknown_18[0] != 0 && spell_id != 9) {
        effect->result_124.reported = 1;
    }
    switch (spell_id) {
    case 1:
    case 5:
    case 10:
    case 0xb:
    case 0x19:
    case 0x4f:
        return;
    case 2:
    case 0x35:
    case 0x3b:
    case 0x3e:
        ApplyDefenseEffectSlot(effect);
        break;
    case 3:
        ApplyCharmToMonsterTarget(effect);
        break;
    case 6:
    case 0x44:
        if (effect->Source.fBackfire == 0) {
            applied &= HealTargets(effect);
        } else {
            ApplyDamageToTargets(effect);
        }
        break;
    case 7:
        InflictConditionAttack0054D5C0(effect, 3, 100, 0);
        break;
    case 8:
    case 0x11:
    case 0x14:
    case 0x1a:
    case 0x20:
    case 0x21:
    case 0x28:
    case 0x2d:
    case 0x40:
        ApplyBeingEffectSlot(effect);
        break;
    case 0xc:
        InflictConditionAttack0054D5C0(effect, 0xf, 100, 0);
        break;
    case 0xd:
    case 0x2c:
        if (effect->Source.fBackfire == 0) {
            applied &= RestoreTargetsStamina(effect);
        } else {
            FatigueTargets(effect);
        }
        break;
    case 0xe:
    case 0x75:
        InflictConditionAttack0054D5C0(effect, 6, 100, 0);
        break;
    case 0xf:
        InflictConditionAttack0054D5C0(effect, 0xc, 100, 0);
        break;
    case 0x10:
        if (effect->Source.fBackfire == 0) {
            applied &= TryCureConditionOnTargets(effect, 3, 0) &
                       TryCureConditionOnTargets(effect, 4, 0) &
                       TryCureConditionOnTargets(effect, 6, 0) &
                       TryCureConditionOnTargets(effect, 0xf, 0) &
                       TryCureConditionOnTargets(effect, 0xc, 0);
        } else {
            switch (Random(5)) {
            case 0:
                InflictConditionAttack0054D5C0(effect, 3, 100, 0);
                break;
            case 1:
                InflictConditionAttack0054D5C0(effect, 4, 100, 0);
                break;
            case 2:
                InflictConditionAttack0054D5C0(effect, 6, 100, 0);
                break;
            case 3:
                InflictConditionAttack0054D5C0(effect, 0xf, 100, 0);
                break;
            default:
                InflictConditionAttack0054D5C0(effect, 0xc, 100, 0);
                break;
            }
        }
        break;
    case 0x12:
        level = effect->definition.duration_scale;
        AdjustIntegerByPercent(&level, effect->definition.percent);
        if (6 < level) {
            level = 7;
        }
        SetKnockKnockTarget(level, effect->Source.unknown_18[1], effect->Source.fBackfire);
        effect->result_124.reported = 1;
        break;
    case 0x13:
    case 0x15:
    case 0x1b:
    case 0x36:
    case 0x38:
    case 0x3d:
    case 0x41:
        if (effect->Source.fBackfire == 0) {
            switch (spell_id) {
            case 0x13:
                condition = 1;
                break;
            default:
                srAssertFail("FALSE", MAGIC_EFFECTS_CPP, 0xd0b, 0);
                condition = 0;
                break;
            case 0x15:
                condition = 2;
                break;
            case 0x1b:
                condition = 3;
                break;
            case 0x36:
                condition = 4;
                break;
            case 0x38:
                condition = 5;
                break;
            case 0x3d:
                condition = 6;
                break;
            case 0x41:
                condition = 7;
                break;
            }
            ApplyConditionToTargets(effect, condition);
            effect->result_124.applied = 1;
        } else {
            if (effect->kind == 0x13) {
                InflictConditionAttack0054D5C0(effect, 4, 100, 0);
            }
            if (effect->kind == 0x1b) {
                ApplyDamageToTargets(effect);
            }
            if (effect->kind == 0x38) {
                InflictConditionAttack0054D5C0(effect, 5, 100, 0);
            }
        }
        break;
    case 0x17:
        if (TargetSourceIsCharacter(&effect->Source, 0) == 0) {
            srAssertFail("SourceIsCharacter(&(pQueue->Source))", MAGIC_EFFECTS_CPP, 0x1c2, 0);
        }
        ApplyIdentifyAttempt(effect->target.pPCItem, effect->definition.duration_scale,
                             effect->definition.percent);
        item = effect->target.pPCItem;
        if (item->identified != 0 && item->bound != 0) {
            effect->result_124.applied = 1;
        }
        break;
    case 0x18:
        InflictConditionAttack0054D5C0(effect, 0xb, 100, 0);
        break;
    case 0x1d:
        InflictConditionAttack0054D5C0(effect, 5, 100, 0);
        break;
    case 0x1e:
    case 0x7b:
        InflictConditionAttack0054D5C0(effect, 6, 100, 0);
        AlertMonsterGroupsToNoise004F0E80(&effect->target.point,
                                          effect->definition.duration_scale * 15000 + 25000, 0);
        if (gXStatus.fCombatMode == 0) {
            effect->result_124.applied = 1;
        }
        break;
    case 0x1f:
        InflictConditionAttack0054D5C0(effect, 0xe, 100, 0);
        break;
    case 0x22:
        if (effect->Source.fBackfire == 0) {
            applied &= TryCureConditionOnTargets(effect, 0x10, 0) &
                       TryCureConditionOnTargets(effect, 0xe, 0);
        } else {
            InflictConditionAttack0054D5C0(effect, 0x10, 100, 0);
        }
        break;
    case 0x23:
        if (effect->Source.fBackfire == 0) {
            applied &= TryCureConditionOnTargets(effect, 7, 0);
        } else {
            int turns = GetTargetConditionTurns(effect, 7, &condition);
            unsigned int roll = Random(effect->definition.duration_scale);
            effect->definition.duration_base = roll + 1 + turns;
            roll = Random(effect->definition.duration_scale / 3);
            InflictConditionAttack0054D5C0(effect, 7, 100, roll + 1 + condition);
        }
        break;
    case 0x25:
    case 0x43:
        InflictConditionAttack0054D5C0(effect, 0x10, 100, 0);
        break;
    case 0x26:
        ApplyMonsterControlToNearbyMonsters(effect);
        break;
    case 0x27:
        level = effect->definition.duration_scale;
        AdjustIntegerByPercent(&level, effect->definition.percent);
        if (6 < level) {
            level = 7;
        }
        CastSpellAtLockInteraction00587C80(level, effect->Source.unknown_18[1],
                                           effect->Source.fBackfire);
        effect->result_124.reported = 1;
        break;
    case 0x29:
        ResolveCharmRefusal(effect);
        effect->result_124.reported = 1;
        break;
    case 0x2a:
        ApplyDamageToTargets(effect);
        InflictConditionAttack0054D5C0(effect, 4, 0x5a, 0);
        InflictConditionAttack0054D5C0(effect, 0x11, 0x19, 0);
        break;
    case 0x2e:
        InflictConditionAttack0054D5C0(effect, 8, 100, 0);
        break;
    case 0x30:
    case 0x31:
    case 0x4c:
    case 0x50:
    case 0x51:
    case 0x5d:
        ApplyCombatEffectSlot(effect);
        break;
    case 0x33:
        applied &= TryCureConditionOnTargets(effect, 2, 0);
        break;
    case 0x3a:
        if (effect->Source.fBackfire != 0) {
            InflictConditionAttack0054D5C0(effect, 9, 100, 0);
        } else {
            RevealItemBindingsToTarget(effect);
        }
        break;
    case 0x3c:
        ApplyInsanityEffect(effect);
        break;
    case 0x45:
        InflictConditionAttack0054D5C0(effect, 9, 100, 0);
        break;
    case 0x46:
    case 0x57:
    case 0x5a:
    case 0x5e:
        InflictConditionAttack0054D5C0(effect, 0x12, 100, 0);
        break;
    case 0x47:
        ApplyDamageToTargets(effect);
        InflictConditionAttack0054D5C0(effect, 0xb, 0x19, 0);
        break;
    case 0x48:
        ReduceCombatEffectDurations(effect);
        effect->result_124.applied = 1;
        break;
    case 0x49:
        RecallCasterToSavedLocation(effect);
        effect->result_124.applied = 1;
        break;
    case 0x4a:
        if (effect->Source.fBackfire == 0) {
            applied &= TryCureConditionOnTargets(effect, 0xb, 0) &
                       TryCureConditionOnTargets(effect, 0xd, 0);
        } else {
            InflictConditionAttack0054D5C0(effect, 0xb, 100, 0);
        }
        break;
    case 0x4b:
        if (TargetSourceIsCharacter(&effect->Source, 0) == 0) {
            srAssertFail("SourceIsCharacter(&(pQueue->Source))", MAGIC_EFFECTS_CPP, 0xa6f, 0);
        }
        camera_state = &g_status_685170.buffers.characters[effect->Source.iChar].saved_location;
        world = GetWorld();
        GetWorldCameraState(world, camera_state);
        g_status_685170.buffers.characters[effect->Source.iChar].saved_level =
            g_status_685170.current_level;
        g_status_685170.buffers.characters[effect->Source.iChar].has_saved_location = 1;
        effect->result_124.applied = 1;
        break;
    case 0x4e:
        ApplyDamageToTargets(effect);
        InflictConditionAttack0054D5C0(effect, 0xc, 0x19, 0);
        break;
    case 0x52:
    case 0x54:
        DrainTargetsLife(effect);
        break;
    case 0x55:
        AttackThenInflictCondition(effect, 0xb, 6, 100, 0, 0);
        break;
    case 0x56:
    case 0x63:
        ResolveAfflictionAgainstTargets(effect);
        effect->result_124.reported = 1;
        break;
    case 0x58:
        applied &= TryCureConditionOnTargets(effect, 0x12, 0);
        break;
    case 0x59:
        InflictConditionAttack0054D5C0(effect, 0xd, 100, 0);
        break;
    case 0x5b:
        ApplyDamageToTargets(effect);
        InflictConditionAttack0054D5C0(effect, 0xb, 0x4b, 0);
        break;
    case 0x5c:
        ApplyDamageToTargets(effect);
        InflictConditionAttack0054D5C0(effect, 0x11, 0x32, 0);
        break;
    case 0x5f:
        point = effect->Source.point;
        level = effect->definition.duration_scale;
        shake = CreateCameraShakeEffect004AE080(
            level * g_navigator_vertical_phase_step_005ebcc8 + g_float_005ebc7c, 1,
            level * g_navigator_snap_angle_005ec2f0 + g_float_005ee838, 0x47435000, &point);
        shake->flags_00 = shake->flags_00 & 0xffffffe7;
        sound_name = g_spell_records[spell_id].sound_name;
        if (sound_name[0] != 0) {
            parms = 0;
            path = FormatString("Data\\Spells\\Sounds\\%s.wav", sound_name);
            SoundPlay(path, parms);
        }
        /* fall through */
    case 4:
    case 9:
    case 0x16:
    case 0x1c:
    case 0x24:
    case 0x2b:
    case 0x2f:
    case 0x32:
    case 0x34:
    case 0x37:
    case 0x39:
    case 0x3f:
    case 0x42:
    case 0x4d:
    case 0x53:
    case 0x60:
    case 0x61:
    case 0x62:
    case 0x65:
    case 0x76:
        ApplyDamageToTargets(effect);
        break;
    case 0x64:
        applied &= HealTargets(effect) & RestoreTargetsStamina(effect);
        for (condition = 2; condition < 0x12; ++condition) {
            if (condition != 10) {
                applied &= TryCureConditionOnTargets(effect, condition, 1);
            }
        }
        break;
    case 0x72:
        applied &= HealTargets(effect);
        effect->definition.magnitude.count <<= 1;
        applied &= RestoreTargetsStamina(effect);
        effect->result_124.reported = 1;
        break;
    case 0x73:
        count = effect->target_indices_0f0.GetCount();
        if (0 < count) {
            for (index = 0; index < count; ++index) {
                int iChar = *effect->target_indices_0f0.GetAt(index);
                if (iChar == -1) {
                    srAssertFail("iChar != -1", MAGIC_EFFECTS_CPP, 0x88a, 0);
                }
                amount = RollDice(&effect->definition.magnitude);
                if (amount != 9999) {
                    AdjustIntegerByPercent(&amount, effect->definition.percent);
                }
                RestoreCharacterSpellPointsEvenly(iChar, amount);
            }
        }
        effect->result_124.applied = 1;
        break;
    case 0x74:
        applied &=
            TryCureConditionOnTargets(effect, 0x11, 0) & TryCureConditionOnTargets(effect, 0xf, 0);
        break;
    case 0x78:
        TryCureConditionOnTargets(effect, 1, 1);
        break;
    case 0x7c:
        if (0 < effect->target_indices_0f0.GetCount()) {
            percent_roll = RollDice(&effect->definition.magnitude);
            if (percent_roll < 100) {
                percent_roll = RollDice(&effect->definition.magnitude);
            } else {
                percent_roll = 100;
            }
            g_status_685170.party_gold =
                g_status_685170.party_gold - (percent_roll * g_status_685170.party_gold) / 100;
            if (g_settings_6850c8.verbose_combat_messages == 0 && GetTextBoxMode() != 0) {
                AppendToLastTextLine(L" -- ", -1);
                SetTextBoxMode(1, -1);
            }
            if (percent_roll < 100) {
                AppendToLastTextLine(gppStringList[0x1d5], -1);
                effect->result_124.reported = 1;
            } else {
                AppendToLastTextLine(gppStringList[0x1d6], -1);
                effect->result_124.reported = 1;
            }
        }
        break;
    case 0x7d:
        DamageTargetsAndReport(effect);
        break;
    case 0x7e:
        DestroyMissilesOnTargets(effect);
        break;
    case 0x7f:
        amount = effect->definition.duration_scale * 0x32;
        AdjustIntegerByPercent(&amount, effect->definition.percent);
        if (0 < effect->target_indices_0f0.GetCount()) {
            DrainPartySpellPoints(amount, 0);
            slot = g_status_685170.effect_slots_17af;
            do {
                if (slot->active != 0) {
                    slot->active = 0;
                    slot->effect_id = 0;
                    slot->amount = 0;
                    slot->duration_0d = 0;
                    RebuildPartyEffectBlock0050E700();
                    InvalidateMainGameEffectHud();
                    RequestRedraw(0x800100);
                }
                ++slot;
            } while (slot < g_status_685170.effect_slots_17af + 12);
        }
        effect->result_124.reported = 1;
        break;
    case 0x81:
        ApplyDamageToTargets(effect);
        InflictConditionAttack0054D5C0(effect, 0x10, 0x32, 0);
        break;
    case 0x82:
        ApplyRandomAfflictionToTarget(effect);
        break;
    case 0x83:
        GetFact(0x83);
        ApplyDamageToTargets(effect);
        break;
    default:
        ShowNoticef(7, L"%s - spell effect not implemented",
                    g_spell_records[spell_id].display_name);
        break;
    }
    if (g_settings_6850c8.verbose_combat_messages == 0 && effect->result_124.reported == 0 &&
        effect->Source.fBackfire == 0) {
        if (GetTextBoxMode() != 0) {
            AppendToLastTextLine(L" -- ", -1);
            SetTextBoxMode(1, -1);
        }
        AppendToLastTextLine(
            effect->result_124.applied != 0 ? gppStringList[0x1b0] : gppStringList[0x1a5], -1);
        effect->result_124.reported = 1;
    }
    if (effect->flag_120 != 0 && applied == 0) {
        if (effect->Source.unknown_18[0] != 0) {
            CastSpellFromSource(spell_id, &effect->Source, &effect->target,
                                effect->definition.duration_scale, 0, 0, 1, &cost, 0, 0, 0);
        } else if (TargetSourceIsCharacter(&effect->Source, 0) != 0) {
            character = &g_status_685170.buffers.characters[effect->Source.iChar];
            cost = SpellCastFatigueCost(spell_id, effect->definition.duration_scale);
            FatigueCharacter(effect->Source.iChar, cost, 1, 0);
            while (SpellCastFatigueCost(spell_id, 1) <= character->stamina) {
                if (character->sp_left[g_spell_records[spell_id].realm] <
                        g_spell_records[spell_id].spell_point_cost ||
                    ExecuteCharacterSpellCast(effect->Source.iChar, spell_id, 8, &cost, 1) != 2) {
                    break;
                }
                FatigueCharacter(effect->Source.iChar, cost, 1, 0);
            }
        }
    }
    FinishSpellEffectTargets(effect);
    if (queued_spell_id != 0x17 && g_current_screen_state.id == 6) {
        RefreshTextBoxMode00590BD0(0xffff);
    }
}

/* Damage from the target-side enchantment: the enchantment's power scales the
   spell record's dice, the reduced roll is applied to the character, the
   result's amount feeds the running combat total at +0xa1a, and the reports
   queue on the combat state for the message pass. */
// FUNCTION: WIZ8 0x00553350
void ApplyDiceDamageToCharacter00553350(int party_slot, W8TargetSource* source,
                                        W8Enchantment* enchantment)
{
    unsigned char verbose = g_settings_6850c8.verbose_combat_messages;
    W8SpellEffectResult result;
    W8SpellDamageReport* report;
    W8Dice dice;
    unsigned int amount;

    if (enchantment->value_00 == 0) {
        return;
    }
    memset(static_cast<void*>(&result), 0, sizeof(result));
    dice = g_spell_records[0x1b].effect_dice;
    dice.count = static_cast<unsigned char>(enchantment->value_00) * dice.count;
    amount = ApplyCharacterDamageReduction(&g_status_685170.buffers.characters[party_slot],
                                           RollDice(&dice));
    if (amount > 0) {
        ApplyDamageToCharacter(party_slot, amount, 0, verbose, verbose, &result, 0);
        g_combat_state->attack_report.notice_values[3] += result.amount;
        while (result.reports.GetCount() > 0) {
            report = *result.reports.GetAt(0);
            result.reports.RemoveAt(0);
            g_combat_state->attack_report.reports.Add(report);
        }
    }
}

/* The monster-side counterpart: the same enchantment-scaled dice roll feeds
   ApplyDamageToMonster and the running total, with no report queueing. */
// FUNCTION: WIZ8 0x00553540
void ApplyDiceDamageToMonster00553540(W8MonsterInfo* monster_info, W8TargetSource* source,
                                      W8Enchantment* enchantment)
{
    unsigned char verbose = g_settings_6850c8.verbose_combat_messages;
    W8MonsterRecord* record;
    W8Dice dice;
    int damage;
    unsigned int amount;

    if (enchantment->value_00 == 0) {
        return;
    }
    dice = g_spell_records[0x1b].effect_dice;
    dice.count = static_cast<unsigned char>(enchantment->value_00) * dice.count;
    damage = RollDice(&dice);
    record = GetMonsterDataForInfo(monster_info);
    amount = ApplyDamageReduction(monster_info, record, damage);
    if (amount > 0) {
        ApplyDamageToMonster(monster_info, amount, source, 0, verbose, verbose, 0, 0);
        g_combat_state->attack_report.notice_values[3] += amount;
    }
}

/* Flat-amount damage to a character: when the combat log is quiet the applied
   amount feeds the running total at +0xa1e and the reports queue up; when it
   is verbose the damage is applied with the announced flags instead. */
// FUNCTION: WIZ8 0x005535D0
void ApplyDirectDamageToCharacter005535D0(int party_slot, W8TargetSource* source, int damage)
{
    unsigned char verbose = g_settings_6850c8.verbose_combat_messages;
    W8SpellEffectResult result;
    W8SpellDamageReport* report;
    unsigned int amount;

    amount = ApplyCharacterDamageReduction(&g_status_685170.buffers.characters[party_slot], damage);
    if (amount > 0) {
        if (verbose != 0) {
            ApplyDamageToCharacter(party_slot, amount, 0, 1, 1, 0, 1);
        } else {
            amount = ApplyDamageToCharacter(party_slot, amount, 0, 0, 0, &result, 0);
            g_combat_state->attack_report.notice_values[4] += amount;
            while (result.reports.GetCount() > 0) {
                report = *result.reports.GetAt(0);
                result.reports.RemoveAt(0);
                g_combat_state->attack_report.reports.Add(report);
            }
        }
    }
}

/* The monster-side counterpart: the flat amount reduced by the monster's own
   reduction is applied, feeding the running total and report queue in quiet
   mode or the announced apply in verbose mode. */
// FUNCTION: WIZ8 0x00553770
void ApplyDirectDamageToMonster00553770(W8MonsterInfo* monster_info, W8TargetSource* source,
                                        int damage)
{
    unsigned char verbose = g_settings_6850c8.verbose_combat_messages;
    W8SpellEffectResult result;
    W8SpellDamageReport* report;
    W8MonsterRecord* record;
    unsigned int amount;

    record = GetMonsterDataForInfo(monster_info);
    amount = ApplyDamageReduction(monster_info, record, damage);
    if (amount > 0) {
        if (verbose != 0) {
            ApplyDamageToMonster(monster_info, amount, source, 0, 1, 1, 0, 1);
        } else {
            amount = ApplyDamageToMonster(monster_info, amount, source, 0, 0, 0, &result, 0);
            g_combat_state->attack_report.notice_values[4] += amount;
            while (result.reports.GetCount() > 0) {
                report = *result.reports.GetAt(0);
                result.reports.RemoveAt(0);
                g_combat_state->attack_report.reports.Add(report);
            }
        }
    }
}
