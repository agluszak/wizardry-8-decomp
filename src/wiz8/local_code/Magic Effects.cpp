#include "wiz8/engine_code/World.h"
#include "wiz8/combat_state.h"
#include "wiz8/game_state.h"
#include "wiz8/magic.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/sr_api.h"
#include "wiz8/ui_state.h"
#include "wiz8/utility.h"
#include "random.h"

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
typedef struct W8SpellEffectDefinition {
    unsigned char unknown_00[4];
    W8Dice magnitude;                     /* 0x04 */
    unsigned char unknown_08[0x18];
    int duration_scale;                   /* 0x20 */
    unsigned int percent;                 /* 0x24 */
    int duration_base;                    /* 0x28 */
    int duration_per_power;               /* 0x2c */
} W8SpellEffectDefinition;

/* One effect slot on a target. The leading flag says whether it is running,
   and the byte after selects the visual the renderer is told to drop. */
typedef struct W8EffectSlot {
    unsigned char active;                 /* 0x00 */
    int visual_index;                     /* 0x01, unaligned */
    unsigned char unknown_05[8];
    unsigned char unknown_0d[4];
} W8EffectSlot;                           /* 0x11 */

#pragma pack(pop)

/* The duration that means "for good". */
enum { W8_EFFECT_PERMANENT = 9999 };

/* 0x0060CFFC: eight bytes per visual, whose leading dword names it. */
extern const int g_effect_visual_table[][2];

extern void DropMonsterVisual(W8Monster* monster, int visual, int arg_3);  /* 0x004ACD80 */
extern void PostMonsterNotice(W8MonsterInfo* monster_info, void* notice);  /* 0x00590B40 */
extern void Function50E700(void);
extern void Function5AF2D0(void);
extern void RequestRedraw(int mask);
extern void Function50E8C0(int location_id);

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
                          g_effect_visual_table[slot->visual_index][0], 0);
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
    Function50E700();
    Function5AF2D0();
    RequestRedraw(0x800100);
}

extern void PostCharacterNotice(int party_slot, void* notice);           /* 0x00590950 */
extern void ApplyEffectToTarget(
    int* result, W8CombatSlot* target, int arg_3, int arg_4);            /* 0x00552250 */
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

extern void MoveWorldToPoint(W8World* destination, W8World* source, const srVector3T<float>* point);
/* 0x00450610 */
extern void PlacePartyAtPoint(const srVector3T<float>* point);                  /* 0x00421090 */
extern void BeginLevelTransition(void);                                  /* 0x005611A0 */

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
        if (caster->saved_level == g_current_level) {
            MoveWorldToPoint(GetWorld(), GetWorld659AB8(), &caster->saved_location.point);
            point = caster->saved_location.point;
            PlacePartyAtPoint(&point);
            MarkRendererReady();
            return;
        }
        g_pending_move_location_00687417 = caster->saved_location;
        g_level_block->pending_level = g_party_characters[pQueue->Source.iChar].saved_level;
        g_level_block->pending_entry_id = -1;
        BeginLevelTransition();
    }
}
