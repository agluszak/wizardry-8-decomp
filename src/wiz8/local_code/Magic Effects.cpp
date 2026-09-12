#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/combat_state.h"
#include "wiz8/game_status.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/engine_code/Trigger.h"
#include "wiz8/screen_state.h"
#include "wiz8/magic.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/sr_api.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/utility.h"
#include "random.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_code/MonsterGroup.h"

/*
 * Local Code\Magic Effects.cpp.
 *
 * What a running spell effect is worth and how long it lasts, and the two
 * paths that take one off again.
 */

#pragma pack(push, 1)

/* One spell effect definition. The dice at 0x04 are rolled for the effect's
   size, the three values at 0x20 through 0x2c combine into its duration, and
   the percentage at 0x24 scales both. */
struct W8SpellEffectDefinition {
    unsigned char unknown_00[4];
    W8Dice magnitude;                     /* 0x04 */
    unsigned char unknown_08[0x18];
    int duration_scale;                   /* 0x20 */
    unsigned int percent;                 /* 0x24 */
    int duration_base;                    /* 0x28 */
    int duration_per_power;               /* 0x2c */
};

#pragma pack(pop)

/* The duration that means "for good". */
enum { W8_EFFECT_PERMANENT = 9999 };

/* 0x0060CFFC: eight bytes per effect id, whose leading dword names the
   visual resource; -1 means the effect has none. The table ends where
   the next unrelated data region begins. */
// GLOBAL: WIZ8 0x0060cffc
const int g_effect_visual_table[149][2] = {
    {-1, -1},
    {-1, 224},
    {34, -1},
    {38, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {1, 213},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {15, -1},
    {14, -1},
    {-1, -1},
    {-1, -1},
    {11, -1},
    {-1, 211},
    {-1, -1},
    {-1, -1},
    {-1, 212},
    {25, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {10, -1},
    {-1, 215},
    {27, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {4, -1},
    {13, 209},
    {24, 210},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, 214},
    {26, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, 216},
    {-1, -1},
    {7, -1},
    {-1, 219},
    {29, 218},
    {28, -1},
    {-1, -1},
    {-1, -1},
    {-1, 225},
    {35, -1},
    {-1, -1},
    {-1, -1},
    {21, -1},
    {-1, -1},
    {-1, 226},
    {36, -1},
    {39, -1},
    {22, 227},
    {37, -1},
    {-1, 217},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {8, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, 220},
    {30, -1},
    {-1, -1},
    {-1, -1},
    {-1, 223},
    {33, 221},
    {31, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {12, -1},
    {-1, -1},
    {-1, -1},
    {-1, 222},
    {32, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
    {-1, -1},
};

// FUNCTION: WIZ8 0x005af2d0
void Function5AF2D0(void)
{
    if (g_current_screen_state.id == 7) {
        ClearSurfaceRect(0x7f, 0x14, 0x201, 0x28);
        InvalidateRegion(0x7f, 0x14, 0x201, 0x28, 0);
    }
}

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
    int combined = definition->duration_per_power * definition->duration_scale +
                   definition->duration_base;
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
    }
    else if (roll == 1 && duration <= 2) {
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
        DropMonsterVisual(monster_info->monster, 0x26, 0);
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
        DropMonsterVisual(monster_info->monster,
                          g_effect_visual_table[slot->effect_id][0], 0);
    }
    for (index = 0; index < 9; ++index) {
        bytes[index] = 0;
    }
    for (index = 0xd; index < 0x11; ++index) {
        bytes[index] = 0;
    }
    Function50E8C0(monster_info->location_id);
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
    Function5AF2D0();
    RequestRedraw(0x800100);
}

#define MAGIC_EFFECTS_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Magic Effects.cpp"

/* Say that whoever was aimed at shrugged the effect off. A monster target is
   told through the monster notice path and anything else through the
   character one, which is what splits the two here. */
// FUNCTION: WIZ8 0x00552070
void AnnounceEffectResisted(W8CombatSlot* target)
{
    if (g_detailed_combat_messages_0068510c == 0) {
        return;
    }
    if (target->iType == W8_TARGET_KIND_MONSTER) {
        PostMonsterNotice(
            MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
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
void ApplyEffectAndAnnounce(int* result, W8CombatSlot* target, int arg_3, int arg_4)
{
    ApplyEffectToTarget(result, target, arg_3, arg_4);
    if (*result != 0 || g_detailed_combat_messages_0068510c == 0) {
        return;
    }
    if (target->iType == W8_TARGET_KIND_MONSTER) {
        PostMonsterNotice(
            MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
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
    W8TargetSource Source;               /* 0x5c */
};

/* 0x00450610 */

/* Return the casting character to the anchor they set earlier. Nothing happens
   unless the anchor was ever set. On the same level the party is moved there
   directly and the renderer is told to catch up; on any other level the anchor
   is staged into the pending-transition globals instead and the level change
   does the work. The whole 0x3c-byte anchor travels, not just its point. */
// FUNCTION: WIZ8 0x005507d0
void RecallCasterToSavedLocation(W8SpellQueueEntry* pQueue)
{
    W8Character* caster;
    srVector3T<float> point;

    if (!TargetSourceIsCharacter(&pQueue->Source, 0)) {
        srAssertFail("SourceIsCharacter(&(pQueue->Source))", MAGIC_EFFECTS_CPP, 2685, 0);
    }
    caster = &g_party_characters[pQueue->Source.iChar];
    if (caster->has_saved_location != 0) {
        if (caster->saved_level == g_status_685170.current_level) {
            MoveWorldToPoint(GetWorld(), GetWorld659AB8(), &caster->saved_location.point);
            point = caster->saved_location.point;
            PlacePartyAtPoint(&point);
            MarkRendererReady();
            return;
        }
        g_status_685170.pending_move_location = caster->saved_location;
        g_level_block->pending_level = g_party_characters[pQueue->Source.iChar].saved_level;
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
    if (g_in_combat_00683f94 != 0) {
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
        resistance->base =
            character->skills[W8_FIRST_RESISTANCE_SKILL + index].level / 10 + 25;
        if (character->skills[W8_RESISTANCE_BONUS_SKILL].flag_00 != 0) {
            resistance->base +=
                character->skills[W8_RESISTANCE_BONUS_SKILL].level / 5 + 5;
        }
        if (character->current_profession == 14) {
            resistance->base += 5;
        }
    }

    if (character->race != -1) {
        for (index = 0; index < W8_RESISTANCE_COUNT; ++index) {
            channel = g_race_resistance_profiles[character->race]
                          .adjustments[index].resistance_index;
            if (channel == -1) {
                break;
            }
            adjustment = g_race_resistance_profiles[character->race]
                             .adjustments[index].adjustment_or_attribute;
            if (static_cast<int>(adjustment) > W8_RACE_ADJUSTMENT_ATTRIBUTE_BIAS) {
                adjustment =
                    character->attributes[adjustment - W8_RACE_ADJUSTMENT_ATTRIBUTE_BIAS]
                        .value / 5;
            }
            character->resistances[channel].base += adjustment;
        }
    }

    if (character->attributes[1].effective > 0x50) {
        character->resistances[4].base +=
            (character->attributes[1].effective - 0x50) >> 1;
    }
    if (character->attributes[2].effective > 0x50) {
        character->resistances[5].base +=
            (character->attributes[2].effective - 0x50) >> 1;
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
