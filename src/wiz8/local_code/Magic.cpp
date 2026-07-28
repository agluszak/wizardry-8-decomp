#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

#include <wchar.h>

/* Local Code\Magic.cpp, named by the assertion this body embeds. */

// FUNCTION: WIZ8 0x004FF3B0
int GetProfessionCasterLevel(W8Character* character, int profession_id)
{
    int magic_level_offset;

    if (profession_id == -1) {
        profession_id = character->current_profession;
        if (profession_id == -1) {
            srAssertFail(
                "iProfession != -1",
                "C:\\Projects\\Wizardry 8\\Local Code\\Magic.cpp",
                0xe13,
                0);
        }
    }

    magic_level_offset = g_profession_magic_level_offsets[profession_id];
    if (magic_level_offset == -255) {
        return -1;
    }
    return character->profession_levels[profession_id] + magic_level_offset;
}

extern unsigned char Function51D610(int caster, int item_id);   /* 0x0051D610 */

/* The target type that costs three off the difficulty: whatever
   GetSpellTargetType answers seven for. */
enum { W8_SPELL_TARGET_DISCOUNTED = 7 };

/* How hard one spell is to bring off. Half the caster's own figure, plus the
   caller's bonus, plus half of the spell's level and half its point cost taken
   together - so the point cost counts a quarter and the level a half. One
   target type is three easier than the rest, and the answer never goes below
   zero. */
// FUNCTION: WIZ8 0x004FF790
int GetSpellDifficulty(unsigned int caster_figure, int spell_id, int bonus)
{
    int difficulty = (caster_figure >> 1) + bonus +
                     (g_spell_records[spell_id].spell_point_cost / 2 +
                      g_spell_records[spell_id].spell_level) / 2;

    if (GetSpellTargetType(spell_id, 0) == W8_SPELL_TARGET_DISCOUNTED) {
        difficulty -= 3;
    }
    if (difficulty < 0) {
        return 0;
    }
    return difficulty;
}

/* Whether one carried item can be cast from. It has to be of the spell-source
   kind, it has to be identified, and the caster has to be able to use it. */
// FUNCTION: WIZ8 0x00500010
bool CanCastFromItem(int caster, const W8ItemInstance* item)
{
    if (g_item_records[item->item_id].category != 3) {
        return false;
    }
    if (item->identified == 0) {
        return false;
    }
    return Function51D610(caster, item->item_id) != 0;
}

#define MAGIC_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Magic.cpp"

/* The three monster kinds whose alchemy survives a spellcasting block. */
enum {
    W8_MONSTER_KIND_ALCHEMY_LOW = 4,
    W8_MONSTER_KIND_ALCHEMY_HIGH = 5,
    W8_MONSTER_KIND_ALCHEMY_OTHER = 0xd
};

/* The skill whose presence exempts a character's alchemy from the same block.
   Only its index is established. */
/* The four spellbook skills sit in the order the spellbook mask numbers them,
   which is what makes the alchemy one 26 - the third bit, the third skill. The
   spellcasting block spares alchemy in the hands of someone who has it. */
enum {
    W8_SKILL_WIZARDRY = 0x18,
    W8_SKILL_DIVINITY = 0x19,
    W8_SKILL_ALCHEMY = 0x1a,
    W8_SKILL_PSIONICS = 0x1b
};

/* SPELL_COUNT and SPELL_USAGE_COUNT, both named by the SpellUsableNow
   assertions that bound their arguments. */
enum {
    W8_SPELL_COUNT = 0x96,
    W8_SPELL_USAGE_COUNT = 5
};

/* The five uiSpellUsableWhen values, numbered by the switch that consumes
   them. Only the conditions each one imposes are established, not a name the
   original gave them. */
enum {
    W8_SPELL_USABLE_ANY_TIME = 0,
    W8_SPELL_USABLE_IN_COMBAT = 1,
    W8_SPELL_USABLE_OUT_OF_COMBAT = 2,
    W8_SPELL_USABLE_WHILE_CAMPED = 3,
    W8_SPELL_USABLE_WHILE_SHOPPING = 4
};

extern unsigned char g_in_combat_00683f94;
extern unsigned char g_camp_open_00683f9b;
extern unsigned char g_flag_00683f9c;
extern int g_flag_00683f9d;
extern int g_screen_state_0068ec78;
extern void* g_item_selection_owner_0068edcc;

extern int Function53A2C0(W8MonsterInfo* monster_info, int location_id);
extern unsigned char Function53A300(W8MonsterInfo* monster_info, int spell_id);
extern unsigned char Function519F80(
    W8MonsterInfo* monster_info,
    W8MonsterRecord* record,
    int arg_3,
    W8CombatSlot* combat_slot);
extern unsigned char Function5369F0(W8MonsterInfo* monster_info);
extern unsigned char Function4D9080(W8MonsterInfo* monster_info, int arg_2, int arg_3);
extern unsigned char Function5327E0(
    W8MonsterInfo* monster_info, int spell_id, W8CombatSlot* combat_slot);
extern unsigned char Function5330E0(
    W8MonsterInfo* monster_info, int spell_id, W8CombatSlot* combat_slot);
extern unsigned char Function4EBC80(int spell_id);

/* Whether a spellcasting block stops this character casting this spell. The
   block stops everything except alchemy in the hands of someone who has the
   skill for it. */
// FUNCTION: WIZ8 0x004FAE70
bool IsSpellBlockedForCharacter(const W8Character* character, int spell_id)
{
    if (character->condition_turns[W8_CONDITION_SPELLCASTING_BLOCKED] != 0) {
        if (g_spell_records[spell_id].alchemy_spell == 0) {
            return true;
        }
        return character->skills[W8_SKILL_ALCHEMY].level == 0;
    }
    return false;
}

/* The same question for a monster. The block stops everything except alchemy
   from the three kinds that keep it. */
// FUNCTION: WIZ8 0x004FB1D0
bool IsSpellBlockedForMonster(W8MonsterInfo* monster_info, int spell_id)
{
    unsigned char kind;

    if (monster_info->condition_turns[W8_CONDITION_SPELLCASTING_BLOCKED] != 0) {
        if (g_spell_records[spell_id].alchemy_spell == 0) {
            return true;
        }
        kind = GetMonsterDataForInfo(monster_info)->kind_0cb;
        if (kind < W8_MONSTER_KIND_ALCHEMY_LOW ||
            (kind > W8_MONSTER_KIND_ALCHEMY_HIGH && kind != W8_MONSTER_KIND_ALCHEMY_OTHER)) {
            return true;
        }
    }
    return false;
}

/* Everything that has to hold before a monster may start casting: the spell
   has to be one monsters can cast at all, the monster has to be in a state to
   cast it, it has to have somewhere to aim, it has to be able to act, and the
   two combat gates have to agree. */
// FUNCTION: WIZ8 0x004FB0A0
bool MonsterOKToCastSpell(W8MonsterInfo* monster_info, int spell_id)
{
    W8MonsterRecord* record = GetMonsterDataForInfo(monster_info);
    W8CombatSlot* combat_slot;
    unsigned char kind;

    if (g_spell_records[spell_id].monster_castable == 0) {
        srAssertFail(
            "FALSE", MAGIC_CPP, 1132,
            FormatString("MonsterOKToCastSpell: ERROR - spell not monster castable: %hs",
                         g_spell_records[spell_id].display_name));
        return false;
    }

    if (monster_info->condition_turns[W8_CONDITION_SPELLCASTING_BLOCKED] != 0) {
        if (g_spell_records[spell_id].alchemy_spell == 0) {
            return false;
        }
        kind = GetMonsterDataForInfo(monster_info)->kind_0cb;
        if (kind < W8_MONSTER_KIND_ALCHEMY_LOW ||
            (kind > W8_MONSTER_KIND_ALCHEMY_HIGH && kind != W8_MONSTER_KIND_ALCHEMY_OTHER)) {
            return false;
        }
    }

    if (GetSpellTargetType(spell_id, 0) == 0) {
        Function53A2C0(monster_info, monster_info->location_id);
    }
    else if (!Function53A300(monster_info, spell_id)) {
        return false;
    }

    combat_slot = &monster_info->combat_slot_2ba;
    if (!Function519F80(monster_info, record, 0, combat_slot) &&
        !Function5369F0(monster_info)) {
        return false;
    }
    if (Function4D9080(monster_info, 4, 0)) {
        return false;
    }
    if (!Function5327E0(monster_info, spell_id, combat_slot)) {
        return false;
    }
    return !Function5330E0(monster_info, spell_id, combat_slot);
}

/* Whether the spell may be cast in the situation the party is in now. Two
   spells are always allowed on the shop screen out of combat; otherwise the
   record's usable-when value picks which of camp, combat and the shop admit
   it. */
// FUNCTION: WIZ8 0x005001E0
unsigned char SpellUsableNow(
    int spell_id, int unused, char allow_out_of_combat, unsigned char fallback)
{
    int usable_when;
    bool shopping;

    if (spell_id >= W8_SPELL_COUNT) {
        srAssertFail("uiSpell < SPELL_COUNT", MAGIC_CPP, 4179, 0);
    }
    usable_when = g_spell_records[spell_id].usable_when;
    if (usable_when >= W8_SPELL_USAGE_COUNT) {
        srAssertFail("uiSpellUsableWhen < SPELL_USAGE_COUNT", MAGIC_CPP, 4182, 0);
    }

    if (g_screen_state_0068ec78 == 6 && g_in_combat_00683f94 == 0 &&
        g_camp_open_00683f9b != 0 && (spell_id == 0x17 || spell_id == 0x3a)) {
        return 1;
    }

    shopping = g_flag_00683f9c != 0 || g_flag_00683f9d != 0;

    switch (usable_when) {
    case W8_SPELL_USABLE_ANY_TIME:
        break;
    case W8_SPELL_USABLE_IN_COMBAT:
        if (g_in_combat_00683f94 != 1 && allow_out_of_combat == 0) {
            return 0;
        }
        break;
    case W8_SPELL_USABLE_OUT_OF_COMBAT:
        if (g_in_combat_00683f94 != 0) {
            return 0;
        }
        break;
    case W8_SPELL_USABLE_WHILE_CAMPED:
        if (g_camp_open_00683f9b == 0) {
            return 0;
        }
        return !shopping;
    case W8_SPELL_USABLE_WHILE_SHOPPING:
        if (spell_id != 0x27) {
            return spell_id == 0x12 ? (unsigned char)g_flag_00683f9d : 0;
        }
        if (g_flag_00683f9c != 0) {
            return 1;
        }
        return g_flag_00683f9d != 0;
    default:
        srAssertFail("FALSE", MAGIC_CPP, 4228,
                     "SpellUsableNow: ERROR - Invalid uiSpellUsableWhen");
        return fallback;
    }

    if (g_camp_open_00683f9b == 0) {
        return !shopping;
    }
    return 0;
}

/* What the interface has to ask the player to pick before a friendly spell can
   be cast. Nine target types map onto seven answers; the one that depends on
   the selection owner only needs a pick when there is nothing selected already
   and the caller is not in one of the two contexts that supply it. */
// FUNCTION: WIZ8 0x005010F0
char GetTargetNeededForSpellFriendly(int spell_id, unsigned char normalize, int context)
{
    if (spell_id != 0) {
        switch (GetSpellTargetType(spell_id, normalize)) {
        case 0:
            return 8;
        case 1:
            return spell_id != 0x58 ? 1 : 7;
        case 2:
        case 7:
        case 10:
            break;
        case 3:
            return 2;
        case 4:
            return 5;
        case 5:
            return 4;
        case 6:
        case 8:
            return 3;
        case 9:
            if (g_item_selection_owner_0068edcc == 0 || context == 3 || context == 4) {
                return 6;
            }
            break;
        default:
            srAssertFail(
                "FALSE", MAGIC_CPP, 4851,
                FormatString(
                    "GetTargetNeededForSpellFriendly: ERROR - Invalid spell target for %d",
                    spell_id));
        }
    }
    return 0;
}

/* The same question for a hostile spell, which has fewer answers because a
   hostile spell never targets the party's own belongings. */
// FUNCTION: WIZ8 0x005011C0
char GetTargetNeededForSpellHostile(int spell_id)
{
    switch (GetSpellTargetType(spell_id, 0)) {
    case 0:
        return 8;
    case 1:
        return spell_id != 0x58 ? 1 : 7;
    case 2:
    case 5:
    case 6:
    case 7:
    case 8:
        break;
    case 3:
        return 2;
    case 4:
        return 5;
    default:
        srAssertFail(
            "FALSE", MAGIC_CPP, 4889,
            FormatString("GetTargetNeededForSpellHostile: ERROR - Invalid spell target for %d",
                         spell_id));
    }
    return 0;
}

/* One-line forwarder that narrows the underlying answer to a flag. */
// FUNCTION: WIZ8 0x00501860
bool IsSpellFlagged4EBC80(int spell_id)
{
    return Function4EBC80(spell_id) != 0;
}

/* One queued spell effect. Each entry counts down a turn at a time and is
   distinguished only by its kind; the effect body itself lives elsewhere. */
typedef struct W8SpellEffectEntry {
    int kind;                            /* 0x00 */
    int turns_remaining;                 /* 0x04 */
} W8SpellEffectEntry;

/* The kind whose expiry hands every monster back its own control. */
enum { W8_SPELL_EFFECT_KIND_MONSTER_CONTROL = 0x26 };

/* One condition slot, as both the party-wide table and the two per-side combat
   tables lay it out: an occupied flag ahead of the condition it names. */
typedef struct W8ConditionSlot {
    unsigned char occupied;              /* 0x00 */
    int condition_id;                    /* 0x01 */
    unsigned char unknown_05[0xc];
} W8ConditionSlot;                       /* 0x11 */

enum {
    W8_PARTY_CONDITION_SLOTS = 12,
    W8_COMBAT_CONDITION_SLOTS = 9
};

/* 0x00689B58: the queued spell effects, the shared growable vector again. */
extern W8GrowableVector<W8SpellEffectEntry*> g_spell_effects;
/* The same vector reached field by field, which is how the append below writes
   it: count at 0x00689B5C, capacity at 0x00689B60, data at 0x00689B64. */
extern int g_spell_effects_count;
extern int g_spell_effects_capacity;
extern W8SpellEffectEntry** g_spell_effects_data;
/* 0x0068691F */
extern W8ConditionSlot g_party_conditions[W8_PARTY_CONDITION_SLOTS];
extern W8PList* g_active_monster_list_00683fad;

/* Whether every queued effect still has time left on it. */
// FUNCTION: WIZ8 0x00500E50
bool AllSpellEffectsStillRunning(void)
{
    int index;

    for (index = 0; index < g_spell_effects.GetCount(); ++index) {
        if ((*g_spell_effects.GetAt(index))->turns_remaining == 0) {
            return false;
        }
    }
    return true;
}

/* The queued effect that holds monsters under the party's control, if one is
   running. */
// FUNCTION: WIZ8 0x00500F30
W8SpellEffectEntry* FindMonsterControlSpellEffect(void)
{
    int index;
    W8SpellEffectEntry* effect;

    for (index = 0; index < g_spell_effects.GetCount(); ++index) {
        effect = *g_spell_effects.GetAt(index);
        if (effect->kind == W8_SPELL_EFFECT_KIND_MONSTER_CONTROL) {
            return effect;
        }
    }
    return 0;
}

/* Count every queued effect down by one turn. The monster-control effect
   running out is the one with a consequence: every monster that is still alive
   goes back to controlling itself. */
// FUNCTION: WIZ8 0x00500E90
void TickSpellEffects(void)
{
    int count = g_spell_effects.GetCount();
    int index;
    unsigned int monster_index;
    W8SpellEffectEntry* effect;
    W8MonsterInfo* monster_info;

    for (index = 0; index < count; ++index) {
        effect = *g_spell_effects.GetAt(index);
        if (effect->turns_remaining != 0) {
            --effect->turns_remaining;
            if (effect->turns_remaining == 0 &&
                effect->kind == W8_SPELL_EFFECT_KIND_MONSTER_CONTROL) {
                for (monster_index = 0;
                     monster_index < PListGetCount(g_active_monster_list_00683fad);
                     ++monster_index) {
                    monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
                    if (monster_info != 0 && !monster_info->monster->IsDying()) {
                        SetMonsterControlState(monster_info, 0);
                    }
                }
            }
        }
    }
}

/* Whether the party as a whole is under one particular condition. */
// FUNCTION: WIZ8 0x005012B0
bool PartyHasCondition(int condition_id)
{
    W8ConditionSlot* slot = g_party_conditions;

    while (slot->occupied == 0 || slot->condition_id != condition_id) {
        ++slot;
        if (slot > &g_party_conditions[W8_PARTY_CONDITION_SLOTS - 1]) {
            return false;
        }
    }
    return true;
}

/* Whether either side of the current fight is under one particular condition.
   Both tables are searched, the party's first. */
// FUNCTION: WIZ8 0x00501250
bool CombatHasCondition(int condition_id)
{
    W8ConditionSlot* slot;
    unsigned int index;

    if (g_combat_state != 0) {
        slot = (W8ConditionSlot*)((char*)g_combat_state + 0x7c1);
        for (index = 0; index < W8_COMBAT_CONDITION_SLOTS; ++index, ++slot) {
            if (slot->occupied != 0 && slot->condition_id == condition_id) {
                return true;
            }
        }
        slot = (W8ConditionSlot*)((char*)g_combat_state + 0x85a);
        for (index = 0; index < W8_COMBAT_CONDITION_SLOTS; ++index, ++slot) {
            if (slot->occupied != 0 && slot->condition_id == condition_id) {
                return true;
            }
        }
    }
    return false;
}

extern int CharacterPointerToPartySlot(const W8Character* character);
extern void Function4E7CC0(
    int party_slot, int arg_2, int arg_3, void* arg_4, int arg_5, int arg_6);
extern int* Function53B7F0(int party_slot, int selector);
extern unsigned char Function537160(int* target, char needed_target);
extern unsigned char Function519180(int party_slot, int arg_2, int arg_3);

/* Record the spell one party slot is about to cast, at what strength, and at
   what, from a target block the caller already holds. */
// FUNCTION: WIZ8 0x004F9AA0
void SetPartySlotSpell(
    int party_slot, int spell_id, int power_level, const int* target_block)
{
    W8PartySlotRow* row = &g_party_slot_rows[party_slot];
    int index;

    row->spell_id = spell_id;
    row->spell_power_level = power_level;
    row->spell_power_extra = 0;
    for (index = 0; index < 8; ++index) {
        row->spell_target_block[index] = target_block[index];
    }
}

/* The same, addressed by character rather than by slot, and taking the target
   block from the targeting code instead of the caller. */
// FUNCTION: WIZ8 0x004F9A20
void SetCharacterSpell(const W8Character* character, int spell_id, int power_level)
{
    int party_slot = CharacterPointerToPartySlot(character);
    int notify[2];
    const int* target_block;
    W8PartySlotRow* row;
    int index;

    notify[0] = power_level;
    notify[1] = 0;
    Function4E7CC0(party_slot, 7, spell_id, notify, 0, 1);
    target_block = Function53B7F0(party_slot, 6);

    row = &g_party_slot_rows[party_slot];
    row->spell_power_level = power_level;
    row->spell_id = spell_id;
    row->spell_power_extra = 0;
    for (index = 0; index < 8; ++index) {
        row->spell_target_block[index] = target_block[index];
    }
}

/* Whether one party slot's recorded spell target is still a target it could
   reach: the target has to be of the kind the spell needs, and the slot has to
   be in range of it. */
// FUNCTION: WIZ8 0x00501530
bool PartySlotSpellTargetStillValid(int party_slot)
{
    char needed = GetTargetNeededForSpellFriendly(
        g_party_slot_rows[party_slot].spell_id, 0, 6);

    if (!Function537160(Function53B7F0(party_slot, 3), needed)) {
        return false;
    }
    return Function519180(party_slot, 0, 3) != 0;
}

extern void ChooseAction(int party_slot, int action, int detail, int a, int b, int c);
/* 0x004E7CC0 */
extern void AimAtTarget(int actor, W8CombatSlot* target, int context);   /* 0x005387F0 */
extern void StartBreathCycle(int party_slot, int arg_2);                 /* 0x0052FE80 */
extern void ReportBreathFailed(int party_slot);                          /* 0x0056A770 */
extern bool CanCharReBreathe(int party_slot);                            /* 0x004EBC80 */
extern bool IsSpellTargetStillValidIn(int party_slot, int spell_id, int context);
extern int g_combat_difficulty_006850d5;
extern W8CombatCharacterRow* g_combat_character_rows;
/* 0x00616DF0: seventeen entries, indexed by the spell's own cost band. The
   monster power-level chooser reads the same table as a spell-point budget
   cost, so the one table serves both. */
extern const int g_spell_failure_table[];

/* Start one character's breath attack. The assertion names the predicate it
   depends on outright - CanCharReBreathe - so a character who cannot is a
   caller error rather than a refusal. */
// FUNCTION: WIZ8 0x00501880
void StartCharacterBreathAttack(int party_slot)
{
    W8PartySlotRow* row = &g_party_slot_rows[party_slot];

    if (!CanCharReBreathe(party_slot)) {
        srAssertFail("CanCharReBreathe(uiChar)", MAGIC_CPP, 5320, 0);
    }
    ChooseAction(party_slot, 2, -1, 0, 0, 1);
    AimAtTarget(party_slot, (W8CombatSlot*)&row->unknown_0d1, 6);
    if (IsSpellTargetStillValidIn(party_slot, 0x77, 5)) {
        StartBreathCycle(party_slot, 0);
        return;
    }
    ReportBreathFailed(party_slot);
}

/* Append one effect to the queue, growing it by exactly one slot when it is
   full - the growth is inlined here rather than going through the shared Grow,
   and a failed allocation leaves the old array in place. */
// FUNCTION: WIZ8 0x005008A0
void AddSpellEffect(W8SpellEffectEntry* effect)
{
    W8SpellEffectEntry** previous = g_spell_effects_data;
    int wanted = g_spell_effects_count + 1;
    int index;

    if (g_spell_effects_capacity < wanted) {
        g_spell_effects_data = (W8SpellEffectEntry**)operator new(wanted * 4);
        if (g_spell_effects_data == 0) {
            g_spell_effects_data = previous;
            return;
        }
        g_spell_effects_capacity = wanted;
        for (index = 0; index < g_spell_effects_count; ++index) {
            g_spell_effects_data[index] = previous[index];
        }
        operator delete(previous);
    }
    g_spell_effects_data[g_spell_effects_count] = effect;
    ++g_spell_effects_count;
}

/* Scale a value by how far ahead of the difficulty's own pace one combatant
   is. Out of combat nothing is scaled; in combat a combatant slower than the
   pace is left alone too. */
// FUNCTION: WIZ8 0x00501910
unsigned int ScaleByCombatPace(int party_slot, unsigned int* value)
{
    unsigned int pace;
    unsigned int speed;
    int scaled;

    if (g_in_combat_00683f94 == 0) {
        return g_in_combat_00683f94;
    }

    if (g_combat_difficulty_006850d5 == 0) {
        pace = 0x50;
    }
    else if (g_combat_difficulty_006850d5 == 1) {
        pace = 0x3c;
    }
    else {
        if (g_combat_difficulty_006850d5 != 2) {
            srAssertFail("FALSE", MAGIC_CPP, 5352, 0);
        }
        pace = 0x28;
    }

    speed = *(unsigned int*)((char*)&g_combat_character_rows[party_slot] + 0xb4);
    if (pace <= speed) {
        scaled = ((0x32 - pace) + speed) * *value;
        *value = scaled / 50;
    }
    return speed;
}

/* How likely a spell is to fail outright. The spell's own cost band picks a
   difficulty off a seventeen-entry table, scaled by the caller's factor and
   divided by seven; a caster already at or past that has no chance of failing
   at all, and the rest is the shortfall as a percentage capped at a hundred. */
// FUNCTION: WIZ8 0x004FF410
unsigned int GetSpellFailureChance(unsigned int skill, int spell_id, int factor)
{
    int band = g_spell_records[spell_id].spell_point_cost / 2 +
               g_spell_records[spell_id].spell_level;
    unsigned int needed;
    unsigned int chance;

    if (band > 0x10) {
        band = 0x10;
    }
    needed = (unsigned int)(g_spell_failure_table[band] * factor) / 7;
    if (needed <= skill) {
        return 0;
    }
    chance = (needed * 70 - skill * 70) / needed;
    if ((int)chance < 0) {
        return 0;
    }
    if ((int)chance > 100) {
        return 100;
    }
    return chance;
}

extern void ReportActionFailed(int party_slot);                          /* 0x0056A770 */
extern unsigned char GetItemSpell(const W8ItemInstance* item);           /* 0x00520880 */
extern void RecordItemOrigin(int party_slot, unsigned char origin, unsigned short slot);
/* 0x00522180 */
extern unsigned char g_profession_spellbooks[];

/* Begin one character's spell. The recorded target is copied to the stack
   first because choosing the action overwrites it, and out of combat the
   original target is re-aimed at before that happens. The spell is always the
   slot's own; the argument overrides only the strength it goes off at, and
   zero means keep the recorded one. */
// FUNCTION: WIZ8 0x00501590
void StartCharacterSpellCast(int party_slot, int power_level)
{
    W8PartySlotRow* row = &g_party_slot_rows[party_slot];
    int saved_target[8];
    int named[2];
    const int* target;
    int index;

    for (index = 0; index < 8; ++index) {
        saved_target[index] = row->spell_target_block[index];
    }
    if (g_in_combat_00683f94 == 0) {
        AimAtTarget(party_slot, (W8CombatSlot*)row->spell_target_block, 6);
    }

    if (power_level == 0) {
        target = &row->spell_power_level;
    }
    else {
        named[0] = power_level;
        named[1] = 0;
        target = named;
    }
    ChooseAction(party_slot, 7, row->spell_id, (int)target, 0, 1);
    AimAtTarget(party_slot, (W8CombatSlot*)saved_target, 6);

    if (IsSpellTargetStillValidIn(party_slot, row->spell_id, 3)) {
        StartBreathCycle(party_slot, 0);
        return;
    }
    ReportActionFailed(party_slot);
}

/* The same for using an item. The item's own spell decides whether the target
   still holds, and where the item came from is recorded before the check so a
   failed use can put it back. */
// FUNCTION: WIZ8 0x00501790
void StartCharacterItemUse(int party_slot)
{
    W8PartySlotRow* row = &g_party_slot_rows[party_slot];
    int saved_target[8];
    int index;

    for (index = 0; index < 8; ++index) {
        saved_target[index] = row->item_target_block[index];
    }
    if (g_in_combat_00683f94 == 0) {
        AimAtTarget(party_slot, (W8CombatSlot*)row->item_target_block, 6);
    }
    ChooseAction(party_slot, 8, -1, (int)&row->item_use_kind, 0, 1);
    AimAtTarget(party_slot, (W8CombatSlot*)saved_target, 6);
    RecordItemOrigin(party_slot, row->item_origin, row->item_slot);

    if (IsSpellTargetStillValidIn(party_slot, GetItemSpell(row->item_in_use), 4)) {
        StartBreathCycle(party_slot, 0);
        return;
    }
    ReportActionFailed(party_slot);
}

/* A character's whole casting strength in one spellbook: their current
   profession's caster level, plus every other profession they have ever held
   that shares the book. Only positive contributions count, and a caster level
   of nothing at all short-circuits unless the caller asks for the sum
   anyway. */
// FUNCTION: WIZ8 0x004FFBD0
int GetTotalCasterLevel(
    const W8Character* character, int unused, int spellbook, char include_all)
{
    int profession = character->current_profession;
    int total;
    int other;
    int level;

    if (profession == -1) {
        srAssertFail("iProfession != -1", MAGIC_CPP, 3603, 0);
    }
    if (g_profession_magic_level_offsets[profession] == -255) {
        total = -1;
    }
    else {
        total = character->profession_levels[profession] +
                g_profession_magic_level_offsets[profession];
    }
    if (total < 1 && include_all == 0) {
        return total;
    }

    for (other = 0; other < 15; ++other) {
        if (character->profession_levels[other] != 0 &&
            other != character->current_profession &&
            (g_profession_spellbooks[other] & spellbook) != 0) {
            if (g_profession_magic_level_offsets[other] != -255) {
                level = character->profession_levels[other] +
                        g_profession_magic_level_offsets[other];
                if (level > 0) {
                    total += level;
                }
            }
        }
    }
    return total;
}

extern unsigned char Function547940(const W8Character* character, int trait);
/* 0x00547940 */
extern void PracticeCharacterSkill(
    W8Character* character, unsigned int skill_id, int usage_points, int arg_4);
extern int GetSpellbookForSpell(
    const W8Character* character, int spell_id, int a, int b, int c);   /* 0x004FF7F0 */

/* The trait that stops a character learning anything at all. */
enum { W8_TRAIT_CANNOT_LEARN = 0x1f };

/* The first of the six realm skills. A spell's realm names its skill by
   sitting this far along, which is what LearnSpellFromItem's practice call
   establishes. */
enum { W8_SKILL_FIRST_REALM = 0x1c };

/* The four spellbook skills, one per book, practised together for a spell that
   belongs to more than one. */
enum { W8_SKILL_FIRST_SPELLBOOK = W8_SKILL_WIZARDRY, W8_SKILL_AFTER_SPELLBOOK = 0x1c };

/* The skill every learned spell practises regardless of its book. */
enum { W8_SKILL_SPELL_LEARNING = 0x14 };

/* Which spellbooks a spell belongs to, as the mask the profession table is
   tested against. A spell in no book at all answers nothing, which is what
   makes the test below a membership test rather than a comparison. */
static unsigned char SpellbookMaskForSpell(int spell_id)
{
    return (unsigned char)((g_spell_records[spell_id].wizardry_spell != 0) |
                           (g_spell_records[spell_id].psionics_spell != 0
                                ? W8_SPELLBOOK_PSIONICS
                                : W8_SPELLBOOK_NONE) |
                           (g_spell_records[spell_id].divinity_spell != 0
                                ? W8_SPELLBOOK_DIVINITY
                                : W8_SPELLBOOK_NONE) |
                           (g_spell_records[spell_id].alchemy_spell != 0
                                ? W8_SPELLBOOK_ALCHEMY
                                : W8_SPELLBOOK_NONE));
}

/* Whether a character is far enough along to take one spell on. Their whole
   caster level - the current profession's plus every other one that shares the
   spell's book - fixes the highest spell level they could ever hold, and their
   realm skill and spellbook skill together fix a second, lower ceiling. The
   spell has to be at or under both, the character's profession has to be in
   one of the spell's books, and the blocking trait has to be down.

   The two ceilings are computed even when the first alone would settle it,
   which is what shows them as one rule rather than two guards. */
// FUNCTION: WIZ8 0x004FFCB0
char CanCharacterLearnSpell(W8Character* character, int spell_id)
{
    unsigned char book = SpellbookMaskForSpell(spell_id);
    int caster_level;
    int other;
    int other_level;
    unsigned int level_ceiling = 0;
    unsigned int skill_ceiling;
    unsigned int ceiling;
    unsigned int spell_level;
    int spellbook_skill;

    if ((g_profession_spellbooks[character->current_profession] & book) == W8_SPELLBOOK_NONE) {
        return 0;
    }
    if (Function547940(character, W8_TRAIT_CANNOT_LEARN)) {
        return 0;
    }

    caster_level = GetProfessionCasterLevel(character, -1);
    if (caster_level > 0) {
        for (other = 0; other < 15; ++other) {
            if (character->profession_levels[other] != 0 &&
                other != character->current_profession &&
                (g_profession_spellbooks[other] & book) != W8_SPELLBOOK_NONE) {
                other_level = GetProfessionCasterLevel(character, other);
                if (other_level > 0) {
                    caster_level += other_level;
                }
            }
        }
    }

    /* The highest spell level that caster level reaches, searched down from
       the top rather than up, so a caster who reaches nothing keeps zero. */
    for (spell_level = 7;; --spell_level) {
        if (MinimumCasterLevelForSpellLevel(spell_level) <= caster_level) {
            level_ceiling = spell_level;
            break;
        }
        if ((int)(spell_level - 1) < 0) {
            break;
        }
    }

    spellbook_skill = GetSpellbookForSpell(character, spell_id, 0, 0, 0);
    skill_ceiling =
        (character->skills[W8_SKILL_FIRST_REALM + g_spell_records[spell_id].realm].value_02 / 10 +
         character->skills[spellbook_skill].level) /
            15 +
        1;
    ceiling = skill_ceiling < level_ceiling ? skill_ceiling : level_ceiling;

    return (char)(1 - (ceiling < (unsigned int)g_spell_records[spell_id].spell_level));
}

extern int Function52A540(W8Character* character);                       /* 0x0052A540 */
extern void ShowNoticeLine(const wchar_t* line, int a, int b, int c);    /* 0x0055F260 */
extern wchar_t* FormatWideString(const wchar_t* format, ...);
/* 0x0068C09C: the loaded message table, one wide string per entry. Bodies
   name entries by their byte offset into it, which is why the index is
   spelled as one. */
extern wchar_t* g_message_strings[];
/* 0x0061E518: one message-table byte offset per realm, for the realm's name. */
extern const unsigned short g_realm_message_offsets[];

/* Take one spell on. The spell is marked known, its realm's known count goes
   up, the spell-point pools are recomputed, and - when the caller asks for it -
   the character says so in a line built from the character's name, the spell's
   name and the realm's remaining points.

   The line is assembled twice over: once only to measure the three pieces so
   the buffer can be allocated, and once into it. */
// FUNCTION: WIZ8 0x004FFE70
void LearnSpell(W8Character* character, int spell_id, char announce)
{
    W8SpellRealm realm;
    wchar_t realm_name[0x62];
    wchar_t* piece;
    size_t name_length;
    size_t spell_length;
    size_t points_length;
    wchar_t* line;

    character->spell_learned[spell_id] = 1;
    realm = g_spell_records[spell_id].realm;
    ++character->skill_unlocks[W8_SKILL_FIRST_REALM + realm];
    character->skill_unlocks[W8_RESISTANCE_BONUS_SKILL] = Function52A540(character);

    if (announce == 0) {
        return;
    }

    piece = FormatWideString(g_message_strings[0x6e4 / 4], character->name);
    name_length = wcslen(piece);
    spell_length = wcslen(g_spell_records[spell_id].display_name);
    wcscpy(realm_name, g_message_strings[g_realm_message_offsets[realm]]);
    piece = FormatWideString(g_message_strings[0x6e8 / 4], realm_name, character->sp_max[realm]);
    points_length = wcslen(piece);

    line = (wchar_t*)operator new((name_length + spell_length + 8 + points_length) * 2);
    if (line == 0) {
        srAssertFail("wTempMsg", MAGIC_CPP, 0xfdc, 0);
    }
    wcscpy(line, FormatWideString(g_message_strings[0x6e4 / 4], realm_name));
    wcscat(line, L" -- ");
    wcscat(line, g_spell_records[spell_id].display_name);
    wcscat(line, L", ");
    wcscat(line, FormatWideString(g_message_strings[0x6e8 / 4], realm_name, character->sp_max[realm]));
    ShowNoticeLine(line, 0, 1, 0);
}

extern void Function520070(void* arg_1, W8Character* character, int arg_3);  /* 0x00520070 */
extern void Function52E690(
    W8Character* character, int sound, int arg_3, float arg_4, float arg_5); /* 0x0052E690 */
extern int g_learn_sound_0068c510;
extern float g_learn_volume_005ed8c8;
extern float g_learn_pan_005ed914;

/* Learn the spell a scroll or book teaches, and consume it. The item has to
   carry a spell - the assertion names the field ubSpellNumber - and the
   character has to be able to take it on; failing that the item is left alone
   and the refusal is shown.

   Learning practises three things at once: the learning skill at twice the
   spell's level, the spell's own realm skill at its level, and every spellbook
   skill the spell belongs to at the same. */
// FUNCTION: WIZ8 0x00500060
void LearnSpellFromItem(void* origin, W8Character* character, const W8ItemInstance* item)
{
    unsigned int spell_id;
    int usage_points;
    unsigned int skill_id;

    if (g_item_records[item->item_id].spell_id == W8_SPELL_NONE) {
        srAssertFail(
            "gpItemDB[pPCItem->iItemNo].ubSpellNumber != SPELL_NONE", MAGIC_CPP, 0x101f, 0);
    }
    spell_id = g_item_records[item->item_id].spell_id;

    if (!CanCharacterLearnSpell(character, spell_id)) {
        ShowNoticeLine(
            FormatWideString(g_message_strings[0x6ec / 4], character->name), 0, 1, 0);
        return;
    }

    LearnSpell(character, spell_id, 1);
    usage_points = g_spell_records[spell_id].spell_level;
    PracticeCharacterSkill(character, W8_SKILL_SPELL_LEARNING, usage_points * 2, 0);
    PracticeCharacterSkill(
        character, W8_SKILL_FIRST_REALM + g_spell_records[spell_id].realm, usage_points, 0);
    for (skill_id = W8_SKILL_FIRST_SPELLBOOK; skill_id < W8_SKILL_AFTER_SPELLBOOK; ++skill_id) {
        if (g_spell_records[spell_id].wizardry_spell != 0) {
            PracticeCharacterSkill(character, skill_id, usage_points, 0);
        }
    }
    Function520070(origin, character, 1);
    Function52E690(
        character, g_learn_sound_0068c510, 0, g_learn_volume_005ed8c8, g_learn_pan_005ed914);
}

extern unsigned char Function5248A0(int party_slot, int arg_2);          /* 0x005248A0 */

/* Eight is not a power level but the request to cast at the highest one the
   caster can pay for; the walk below resolves it. */
enum { W8_SPELL_POWER_AS_AFFORDABLE = 8, W8_SPELL_POWER_MAX = 7 };

/* The one spell whose availability is decided by a further check rather than
   by the character's spellbook. */
enum { W8_SPELL_CONDITIONAL = 0x3c };

/* Whether the slot could go off with the spell it has recorded. The spell has
   to exist, to be one the character knows, to be usable now, and - out of
   combat - still to have a valid target; and the slot has to be able to pay
   for it at the power level it asked for.

   A spell asking for the affordable power level is priced at one here, so this
   answers whether the cast is possible at all rather than at the level the
   player picked. */
// FUNCTION: WIZ8 0x005012E0
bool CanPartySlotCastRecordedSpell(int party_slot)
{
    const W8PartySlotRow* row = &g_party_slot_rows[party_slot];
    int spell_id = row->spell_id;
    int power_level = row->spell_power_level;

    if (spell_id == 0) {
        return false;
    }
    if (spell_id == W8_SPELL_CONDITIONAL && Function5248A0(party_slot, 0)) {
        return false;
    }
    if (g_party_characters[party_slot].spell_learned[spell_id] != 1) {
        return false;
    }
    if (row->spell_power_level == W8_SPELL_POWER_AS_AFFORDABLE &&
        g_spell_records[spell_id].unknown_125 == 1 && g_in_combat_00683f94 != 0) {
        return false;
    }

    if (power_level == W8_SPELL_POWER_AS_AFFORDABLE) {
        power_level = 1;
    }
    if (g_spell_records[spell_id].spell_point_cost * power_level >
        g_party_characters[party_slot].sp_left[g_spell_records[spell_id].realm]) {
        return false;
    }
    if (!SpellUsableNow(spell_id, 0, 0, 0)) {
        return false;
    }
    if (g_in_combat_00683f94 == 0 && !IsSpellTargetStillValidIn(party_slot, spell_id, 3)) {
        return false;
    }
    return true;
}

/* The highest power level the slot can actually pay for, counting down from
   the one it asked for. Zero means the cast cannot happen at all - either
   because the spell fails the same checks as above, or because even one level
   is more than the pool holds. */
// FUNCTION: WIZ8 0x00501400
int GetAffordableSpellPowerLevel(int party_slot)
{
    const W8PartySlotRow* row = &g_party_slot_rows[party_slot];
    int spell_id = row->spell_id;
    int power_level = row->spell_power_level;
    int cost;

    if (spell_id == 0) {
        return 0;
    }
    if (spell_id == W8_SPELL_CONDITIONAL && Function5248A0(party_slot, 0)) {
        return 0;
    }
    if (g_party_characters[party_slot].spell_learned[spell_id] != 1) {
        return 0;
    }
    if (row->spell_power_level == W8_SPELL_POWER_AS_AFFORDABLE &&
        g_spell_records[spell_id].unknown_125 == 1 && g_in_combat_00683f94 != 0) {
        return 0;
    }
    if (!SpellUsableNow(spell_id, 0, 0, 0)) {
        return 0;
    }
    if (g_in_combat_00683f94 == 0 && !IsSpellTargetStillValidIn(party_slot, spell_id, 3)) {
        return 0;
    }

    if (power_level == W8_SPELL_POWER_AS_AFFORDABLE) {
        power_level = 1;
    }
    cost = g_spell_records[spell_id].spell_point_cost * power_level;
    while (power_level != 0) {
        if (cost <= g_party_characters[party_slot].sp_left[g_spell_records[spell_id].realm]) {
            return power_level;
        }
        --power_level;
        cost -= g_spell_records[spell_id].spell_point_cost;
    }
    return 0;
}

extern W8ItemInstance* FindCharacterItemAt(
    int party_slot, unsigned char origin, unsigned short slot);          /* 0x00522180 */
extern unsigned char Function522A30(int party_slot, const W8ItemInstance* item);
/* 0x00522A30 */
extern unsigned char CanCharacterUseItem(
    W8Character* character, const W8ItemInstance* item);                 /* 0x0051D800 */
extern unsigned char ItemClassNormalizesTarget(const W8ItemDatabaseRecord* record);

/* The origin that means the item is worn or held rather than carried; in
   combat an equipped item is not re-fetched. */
enum { W8_ITEM_ORIGIN_EQUIPPED = 2 };

/* Whether the slot could go through with the item use it has recorded. The
   item is looked up again from where it was taken rather than trusted, and the
   re-read pointer is stored back, so a stale record is caught here and not at
   the point of use.

   A spell whose target type is not the self-only one may be aimed anywhere; the
   self-only one has to be aimed at the user. */
// FUNCTION: WIZ8 0x00501660
bool CanPartySlotUseRecordedItem(int party_slot)
{
    W8PartySlotRow* row = &g_party_slot_rows[party_slot];
    W8ItemInstance* item;
    int spell_id;
    unsigned char normalize;

    if ((signed char)row->item_origin < 0 || (short)row->item_slot < 0) {
        return false;
    }

    item = FindCharacterItemAt(party_slot, row->item_origin, row->item_slot);
    row->item_in_use = item;

    if (row->item_origin == W8_ITEM_ORIGIN_EQUIPPED && g_in_combat_00683f94 != 0) {
        return false;
    }
    if (item == 0 || item->item_id == -1 || item->item_id != row->item_id_0c9) {
        return false;
    }
    if (!Function522A30(party_slot, item)) {
        return false;
    }
    if (!CanCharacterUseItem(&g_party_characters[party_slot], item)) {
        return false;
    }

    spell_id = GetItemSpell(item);
    normalize = ItemClassNormalizesTarget(&g_item_records[item->item_id]);
    if (GetSpellTargetType(spell_id, normalize) == 0 && row->item_target_block[1] != party_slot) {
        return false;
    }
    if (!SpellUsableNow(spell_id, 0, 0, 0)) {
        return false;
    }
    if (g_in_combat_00683f94 == 0 && !IsSpellTargetStillValidIn(party_slot, spell_id, 4)) {
        return false;
    }
    return true;
}

extern W8MonsterRecord* GetMonsterDataForInfo(W8MonsterInfo* monster_info);
extern wchar_t* GetMonsterName(
    W8MonsterInfo* monster_info, W8MonsterRecord* record, char arg_3);
extern void FormatDebugMessage(int channel, const char* format, ...);
extern int Random(int bound);

/* The spell id that stands for no monster spell. */
enum { W8_MONSTER_SPELL_NONE = 0x77 };

/* How hard a monster casts. It walks the power levels up from one until the
   cost of the next would leave it far enough short of its spell-point budget
   to matter - nine per cent of the cost - and then steps back to the last one
   it could comfortably pay for. A quarter of the time that answer is nudged
   one level down and a quarter one level up, so identical monsters do not all
   cast identically.

   The budget is the monster's database base plus its own runtime bonus; a base
   of zero is a data error the monster is named in. */
// FUNCTION: WIZ8 0x00500330
unsigned int ChooseMonsterSpellPowerLevel(W8MonsterInfo* monster_info, int unused, int spell_id)
{
    unsigned int power_level;
    unsigned int budget;
    unsigned int cost;
    int band;
    int roll;

    if (spell_id == W8_MONSTER_SPELL_NONE) {
        return 0;
    }

    power_level = 1;
    for (;;) {
        W8MonsterRecord* record = GetMonsterDataForInfo(monster_info);

        budget = monster_info->sp_budget_bonus + record->sp_budget;
        if (record->sp_budget == 0) {
            FormatDebugMessage(
                0, "DATA ERROR: Monster %ls casting spells with SP Budget of 0",
                GetMonsterName(monster_info, 0, 0));
        }
        if ((int)budget < 0) {
            budget = 0;
        }

        band = g_spell_records[spell_id].spell_point_cost / 2 +
               g_spell_records[spell_id].spell_level;
        if (band > 16) {
            band = 16;
        }
        cost = (g_spell_failure_table[band] * power_level) / 7;

        if (cost > budget) {
            unsigned int shortfall = (cost * 70 - budget * 70) / cost;
            if ((int)shortfall >= 0 && ((int)shortfall >= 0x65 || shortfall >= 9)) {
                break;
            }
        }
        ++power_level;
        if (power_level >= 8) {
            break;
        }
    }
    if (power_level > 1) {
        --power_level;
    }

    roll = Random(4);
    if (roll == 0) {
        if (power_level > 2) {
            --power_level;
        }
    }
    else if (roll == 1 && power_level < W8_SPELL_POWER_MAX) {
        ++power_level;
    }

    if (power_level == 0 || power_level > W8_SPELL_POWER_MAX) {
        srAssertFail("( uiPowerLevel >= 1 ) && ( uiPowerLevel <= 7 )", MAGIC_CPP, 0x10bf, 0);
    }
    return power_level;
}

extern void ResetTargetSource(W8TargetSource* target_block);               /* 0x00536150 */
extern void ResetCombatSlot(W8CombatSlot* combat_slot);                  /* 0x00536170 */
extern int GetRandomCharacter(
    int require_primary, int require_secondary, int excluded_slot, int excluded_slot_2);
extern void AimCombatSlotAtParty(W8CombatSlot* combat_slot, int hostile);
/* 0x0053C630 */
extern int CastSpellFromSource(
    int spell_id, W8TargetSource* source, W8CombatSlot* target, unsigned int power_level,
    int a, int b, int c, int d, int e, int f, int g);                    /* 0x004FB4C0 */

/* The target-block kind that means the cast comes from a point in the world
   rather than from a character or a monster. */
enum { W8_SOURCE_TYPE_POINT = 3 };

/* Where the cast lands, by the spell's own target type. */
enum {
    W8_TARGET_KIND_ONE_CHARACTER = 1,
    W8_TARGET_KIND_WHOLE_PARTY = 2,
    W8_TARGET_KIND_PARTY_SIDE = 6
};

/* Cast one spell at the party from a point in the world - a trap, a rune, a
   scripted effect. The caller gives the point and the power level; who it hits
   comes from the spell's own target type, so this decides the target rather
   than taking one.

   A power level outside one through seven is silently taken as one, which is
   what makes a caller passing zero cast nothing at all rather than cast weakly.
   Its error text names the function. */
// FUNCTION: WIZ8 0x004FB220
int PointCastSpell(float x, float y, float z, int spell_id, unsigned int power_level)
{
    W8TargetSource source;
    W8CombatSlot target;
    int hostile;

    if (power_level == 0) {
        return 0;
    }
    if (power_level > W8_SPELL_POWER_MAX) {
        power_level = 1;
    }

    ResetTargetSource(&source);
    source.iType = W8_SOURCE_TYPE_POINT;
    source.point.x = x;
    source.point.y = y;
    source.point.z = z;

    ResetCombatSlot(&target);
    switch (GetSpellTargetType(spell_id, 0)) {
    case 0:
    case 1:
    case 3:
        target.kind = W8_TARGET_KIND_ONE_CHARACTER;
        target.character_slot = GetRandomCharacter(0, 1, -1, -1);
        break;
    case 2:
    case 4:
        target.kind = W8_TARGET_KIND_WHOLE_PARTY;
        break;
    case 5:
        hostile = 0;
        target.kind = W8_TARGET_KIND_WHOLE_PARTY;
        AimCombatSlotAtParty(&target, hostile);
        target.kind = W8_TARGET_KIND_PARTY_SIDE;
        break;
    case 6:
    case 8:
        hostile = 1;
        target.kind = W8_TARGET_KIND_WHOLE_PARTY;
        AimCombatSlotAtParty(&target, hostile);
        target.kind = W8_TARGET_KIND_PARTY_SIDE;
        break;
    default:
        srAssertFail(
            "FALSE", MAGIC_CPP, 0x56d,
            FormatString("PointCastSpell: ERROR - Invalid spell target for %d", spell_id));
    }

    CastSpellFromSource(spell_id, &source, &target, power_level, 0, 0, 0, 0, 0, 0, 0);
    return 1;
}

/* Which spellbook skill of the spell's own books the character is best at, and
   - when the caller asks - which of those they have actually unlocked. The
   unlocked one wins only when it is not already the best; otherwise the plain
   best stands.

   The choice matters because the two answers price the cast differently, so
   picking the unlocked one is followed by working out what the cast would cost
   at that skill and abandoning it when the answer comes to nothing. A spell
   with no skill behind it at all is an error that names the spell.

   The alchemy shortcut ahead of all of it: a character the alchemy-exempting
   field marks is answered with the fixed skill outright. */
// FUNCTION: WIZ8 0x004FF7F0
unsigned int GetBestSpellbookSkillForSpell(
    W8Character* character, int spell_id, char pricing, char prefer_unlocked,
    unsigned int power_level, int level_bonus)
{
    unsigned char book = SpellbookMaskForSpell(spell_id);
    unsigned char probe;
    unsigned int skill_id;
    unsigned int best_skill = 0xffffffff;
    unsigned int best_level = 0xffffffff;
    unsigned int unlocked_skill = 0xffffffff;
    unsigned int unlocked_level = 0xffffffff;
    unsigned int chosen;
    unsigned int level;
    int party_slot = book;

    if (pricing != 0 &&
        character->condition_turns[W8_CONDITION_SPELLCASTING_BLOCKED] != 0 &&
        g_spell_records[spell_id].alchemy_spell != 0) {
        return W8_SKILL_ALCHEMY;
    }

    probe = 1;
    for (skill_id = W8_SKILL_FIRST_SPELLBOOK; skill_id < W8_SKILL_AFTER_SPELLBOOK; ++skill_id) {
        if ((probe & book) != 0) {
            level = character->skills[skill_id].level;
            if ((int)best_level < (int)level) {
                best_skill = skill_id;
                best_level = level;
            }
            if (prefer_unlocked != 0 && character->skills[skill_id].flag_00 != 0 &&
                (int)unlocked_level < (int)level) {
                unlocked_skill = skill_id;
                unlocked_level = level;
            }
        }
        probe = (unsigned char)(probe << 1);
    }

    chosen = unlocked_skill;
    if (prefer_unlocked != 0 && unlocked_skill != 0xffffffff && best_skill != unlocked_skill) {
        if (pricing != 0) {
            unsigned int failure;
            int shortfall;
            int band;
            unsigned int skill_figure;
            unsigned int needed;

            party_slot = CharacterPointerToPartySlot(character);
            band = g_spell_records[spell_id].spell_point_cost / 2 +
                   g_spell_records[spell_id].spell_level;
            skill_figure =
                (unsigned int)(character->skills[unlocked_skill].level +
                               character
                                       ->skills[W8_SKILL_FIRST_REALM +
                                                g_spell_records[spell_id].realm]
                                       .level *
                                   4) /
                5;
            if (band > 16) {
                band = 16;
            }
            needed = (g_spell_failure_table[band] * power_level) / 7;
            if (skill_figure < needed) {
                failure = (needed * 70 - skill_figure * 70) / needed;
                if ((int)failure < 0) {
                    failure = 0;
                }
                else if ((int)failure > 100) {
                    failure = 100;
                }
            }
            else {
                failure = 0;
            }

            best_level = failure;
            chosen = GetMinimumCasterLevelForSpell(spell_id);
            shortfall = (int)chosen -
                        GetTotalCasterLevel(character, 0, book, 1) - 1 + level_bonus;
            if (shortfall > 0) {
                unlocked_skill = g_spell_records[spell_id].spell_level * shortfall + power_level;
            }
            ScaleByCombatPace(party_slot, &unlocked_skill);
            if (unlocked_skill != 0) {
                goto done;
            }
        }
        best_level = chosen;
    }

done:
    if (best_level == 0xffffffff) {
        srAssertFail(
            "(iHighestSkill != SKILL_NONE)", MAGIC_CPP, 0xf29,
            FormatString("Failed on spell %ld, usability being %d", spell_id, party_slot));
    }
    return best_level;
}

/* How likely one whole cast is to come apart, as a percentage. Two things
   spoil it, and this is where the two meet: a spellbook skill short of what
   the spell's cost band asks for at that power level, which the plain
   failure-chance body above answers, and a caster level short of what the
   spell asks for, charged flat at the spell's own level per level missing. The
   sum is then scaled by how far ahead of the combat pace the caster is.

   The skill this is measured against is the spell's own best spellbook skill
   weighted four to one against the realm skill, which is what makes the realm
   the larger part of it.

   Power level eight is the request to cast as high as affordable rather than a
   level, so it has no failure chance of its own. The power-level choosers
   carry this whole body inline rather than calling it. */
// FUNCTION: WIZ8 0x004FF4B0
unsigned int GetSpellFailureChanceForCast(
    W8Character* character, int spell_id, unsigned int power_level, int level_bonus)
{
    int skill;
    int party_slot;
    unsigned int skill_figure;
    unsigned int chance;
    int shortfall;

    if (power_level == W8_SPELL_POWER_AS_AFFORDABLE) {
        return 0;
    }

    skill = GetBestSpellbookSkillForSpell(character, spell_id, 1, 1, power_level, 0);
    party_slot = CharacterPointerToPartySlot(character);
    skill_figure =
        (unsigned int)(character->skills[skill].level +
                       character->skills[W8_SKILL_FIRST_REALM + g_spell_records[spell_id].realm]
                               .level *
                           4) /
        5;
    chance = GetSpellFailureChance(skill_figure, spell_id, (int)power_level);

    shortfall = GetMinimumCasterLevelForSpell(spell_id) -
                GetTotalCasterLevel(character, 0, SpellbookMaskForSpell(spell_id), 1) - 1 +
                level_bonus;
    if (shortfall > 0) {
        chance = g_spell_records[spell_id].spell_level * shortfall + power_level;
    }
    ScaleByCombatPace(party_slot, &chance);
    return chance;
}

/* The average of one dice expression, taken as the midpoint of what it can
   roll: base plus the dice at one each, and base plus the dice at their
   faces. The die count is multiplied by the power level first, in a byte, so a
   high power level on a many-dice spell wraps rather than growing. */
static __forceinline int AverageEffectAtPower(W8Dice dice, unsigned int power_level)
{
    unsigned char count = (unsigned char)(dice.count * (unsigned char)power_level);

    return (int)(((float)(dice.base + count * dice.sides) + (float)(dice.base + count)) * 0.5f);
}

/* The failure chance past which casting harder is not worth it. */
enum { W8_SPELL_FAILURE_ACCEPTABLE = 10 };

/* The three spells whose power level is decided by how much of a pool the
   target is missing. The third takes hit points first and falls back to
   stamina only when they are already full, which is what separates it from the
   other two rather than making it a combination of them. */
enum {
    W8_SPELL_RESTORE_HP = 6,
    W8_SPELL_RESTORE_STAMINA = 0xd,
    W8_SPELL_RESTORE_HP_THEN_STAMINA = 100
};

/* How hard to cast a spell that has to last a given number of turns. Each
   power level is priced at what it would really cost - the spell points for
   one cast, times how many casts the failure chance implies, times the level -
   and the cheapest wins. A power level the caster cannot pay for at all ends
   the walk, so the answer is zero when even the first is out of reach.

   A condition that never runs out cannot be out-waited, so it is answered with
   the lowest power level rather than the cheapest. */
// FUNCTION: WIZ8 0x004FDF30
unsigned int ChoosePowerLevelForDuration(
    W8Character* character, int spell_id, unsigned int turns_needed)
{
    W8SpellRealm realm = g_spell_records[spell_id].realm;
    unsigned int power_level;
    unsigned int best_cost = 0;
    unsigned int best_power = 0;
    unsigned int failure;
    unsigned int per_turn;
    unsigned int casts;
    unsigned int cost;

    if (turns_needed == W8_CONDITION_INDEFINITE) {
        return 1;
    }

    for (power_level = 1; power_level < 8; ++power_level) {
        if (character->sp_left[realm] <
            (int)(g_spell_records[spell_id].spell_point_cost * power_level)) {
            return best_power;
        }

        failure = GetSpellFailureChanceForCast(character, spell_id, power_level, (int)power_level);

        /* What one cast at this level really delivers: the square of the power
           level, less the share of it the failure chance takes away. */
        per_turn = power_level * power_level - (failure * power_level * power_level) / 100;
        casts = turns_needed / per_turn;
        if (turns_needed % per_turn != 0) {
            ++casts;
        }
        cost = g_spell_records[spell_id].spell_point_cost * casts * power_level;

        if (cost < best_cost || power_level == 1) {
            best_power = power_level;
            best_cost = cost;
        }
    }
    return best_power;
}

/* How hard to cast a spell that has to restore a given amount. The power level
   climbs until either the failure chance stops being worth it - in which case
   the previous level is taken - or the average roll at that level covers what
   is missing. Nothing missing takes the lowest level.

   Which pool is missing comes from the spell: one restores hit points, one
   stamina, and one takes hit points first and falls back to stamina when they
   are already full. */
// FUNCTION: WIZ8 0x004FE1C0
unsigned int ChoosePowerLevelToRestore(
    W8Character* character, int spell_id, const W8Character* target)
{
    int missing;
    unsigned int power_level;
    unsigned int failure;

    if (target == 0) {
        return 1;
    }

    if (spell_id == W8_SPELL_RESTORE_HP) {
        missing = target->hp_max - (int)target->hp_current;
    }
    else if (spell_id == W8_SPELL_RESTORE_HP_THEN_STAMINA) {
        missing = target->hp_max - (int)target->hp_current;
        if (missing == 0) {
            missing = target->stamina_max - target->stamina;
        }
    }
    else if (spell_id == W8_SPELL_RESTORE_STAMINA) {
        missing = target->stamina_max - target->stamina;
    }
    else {
        return 1;
    }

    if (missing < 1) {
        return 1;
    }

    for (power_level = 1; power_level < 8; ++power_level) {
        failure = GetSpellFailureChanceForCast(character, spell_id, power_level, (int)power_level);
        if (failure > W8_SPELL_FAILURE_ACCEPTABLE) {
            if (power_level > 1) {
                --power_level;
            }
            return power_level;
        }
        if (missing < AverageEffectAtPower(g_spell_records[spell_id].effect_dice, power_level)) {
            return power_level;
        }
    }
    return power_level;
}

extern unsigned int CountIdentifyAttemptsNeeded(int item, int arg_2);
extern unsigned int Function520C70(int item);                            /* 0x00520C70 */

/* The spells whose power level is decided by how bad the target's condition
   is, and which condition each of them lifts. A spell that lifts more than one
   is decided by the worst of them. */
enum {
    W8_SPELL_CURE_GROUP_A = 0x10,
    W8_SPELL_CURE_16 = 0x22,
    W8_SPELL_CURE_7 = 0x23,
    W8_SPELL_CURE_2 = 0x33,
    W8_SPELL_CURE_9 = 0x3a,
    W8_SPELL_CURE_GROUP_B = 0x4a,
    W8_SPELL_IDENTIFY = 0x17
};

/* The target kinds this chooser knows what to do with. */
enum {
    W8_TARGET_TYPE_ONE = 1,
    W8_TARGET_TYPE_PARTY = 2,
    W8_TARGET_TYPE_ITEM = 9
};

/* How hard the slot should cast the spell it has picked, from what its target
   actually needs. Everything it reads is the target's condition array - a
   character's at 0x0a01 or a monster's at 0x57, the same twenty entries with
   the same meanings - so the two targets are handled by one pointer.

   Zero from any of the sub-decisions means "no reason to cast harder", which
   comes back as the lowest power level rather than as nothing. */
// FUNCTION: WIZ8 0x004FE480
unsigned int ChooseSpellPowerLevelForTarget(int party_slot, int spell_id, int identify_context)
{
    W8Character* caster = &g_party_characters[party_slot];
    W8PartySlotRow* row = &g_party_slot_rows[party_slot];
    const int* conditions = 0;
    const W8Character* target_character = 0;
    unsigned int power_level;
    unsigned int worst;

    if (row->spell_target_block[0] == W8_TARGET_KIND_ONE_CHARACTER) {
        target_character = &g_party_characters[row->spell_target_block[1]];
        conditions = target_character->condition_turns;
    }
    else if (row->spell_target_block[0] == W8_SOURCE_TYPE_POINT) {
        W8MonsterInfo* monster_info =
            MonsterInfoFromID(0xb83, MAGIC_CPP, row->spell_target_block[2], 1);
        conditions = monster_info->condition_turns;
    }

    switch (GetSpellTargetType(spell_id, 0)) {
    case W8_TARGET_TYPE_ONE:
        switch (spell_id) {
        case W8_SPELL_RESTORE_HP:
        case W8_SPELL_RESTORE_STAMINA:
        case W8_SPELL_RESTORE_HP_THEN_STAMINA:
            power_level = ChoosePowerLevelToRestore(caster, spell_id, target_character);
            break;
        case W8_SPELL_CURE_GROUP_A:
            worst = conditions[4];
            if (worst <= (unsigned int)conditions[6]) {
                worst = conditions[6];
            }
            if (worst <= (unsigned int)conditions[15]) {
                worst = conditions[15];
            }
            if (worst <= (unsigned int)conditions[12]) {
                worst = conditions[12];
            }
            power_level = ChoosePowerLevelForDuration(caster, spell_id, worst);
            break;
        case W8_SPELL_CURE_16:
            power_level = ChoosePowerLevelForDuration(caster, spell_id, conditions[16]);
            break;
        case W8_SPELL_CURE_7:
            power_level = ChoosePowerLevelForDuration(caster, spell_id, conditions[7]);
            break;
        case W8_SPELL_CURE_2:
            power_level = ChoosePowerLevelForDuration(caster, spell_id, conditions[2]);
            break;
        case W8_SPELL_CURE_9:
            power_level = ChoosePowerLevelForDuration(caster, spell_id, conditions[9]);
            /* The one case where the target being a character says something
               the condition does not: the item they are carrying asks for more
               than the condition does. */
            if (row->spell_target_block[0] == W8_TARGET_KIND_ONE_CHARACTER &&
                power_level <= Function520C70(row->spell_target_block[1])) {
                power_level = Function520C70(row->spell_target_block[1]);
            }
            break;
        case W8_SPELL_CURE_GROUP_B:
            worst = conditions[11];
            if (worst <= (unsigned int)conditions[13]) {
                worst = conditions[13];
            }
            if (worst <= (unsigned int)conditions[15]) {
                worst = conditions[15];
            }
            power_level = ChoosePowerLevelForDuration(caster, spell_id, worst);
            break;
        default:
            return 1;
        }
        break;

    case W8_TARGET_TYPE_PARTY:
        if (spell_id != 0x2c && spell_id != 0x44) {
            return 1;
        }
        power_level = ChoosePowerLevelToRestore(caster, spell_id, 0);
        break;

    case W8_TARGET_TYPE_ITEM:
        if (spell_id != W8_SPELL_IDENTIFY) {
            return 1;
        }
        power_level = CountIdentifyAttemptsNeeded(row->spell_target_block[7], identify_context);
        break;

    default:
        return 1;
    }

    if (power_level != 0) {
        return power_level;
    }
    return 1;
}


/* The target block and the source block are the same struct, so the two
   predicates Targeting.cpp declares over a source answer for a target too. */
extern unsigned char TargetSourceIsCharacter(const W8TargetSource* source, int allow_indirect);
extern unsigned char TargetSourceIsMonster(const W8TargetSource* source, int allow_indirect);
extern wchar_t* GetMonsterGroupName(W8MonsterGroup* group);              /* 0x00510280 */
extern W8WideChar* FormatItemDisplayName(const W8ItemInstance* item, int arg_2);
/* 0x0068C09C is indexed here by byte offset; 0x610 is the "at %s" wrapper every
   named target goes through and the rest are the fixed words. */
enum {
    W8_MESSAGE_TARGET_AT = 0x610,
    W8_MESSAGE_TARGET_PARTY = 0x614,
    W8_MESSAGE_TARGET_PLACE = 0x618,
    W8_MESSAGE_TARGET_DIRECTION = 0x61c,
    W8_MESSAGE_TARGET_ITEM = 0x620,
    W8_MESSAGE_TARGET_UNKNOWN = 0x624
};
/* 0x0061E436: the name-prefix table, eight-byte rows, holding a message-table
   offset rather than a string. A character indexes it by faction and a monster
   by its own name group at record+0x0cc, which is what makes the two one
   table. */
extern const unsigned short g_name_prefix_messages[];
/* 0x00689B34: the empty string every no-target kind is described by. */
extern wchar_t g_no_target_text[];

/* Say in words what a spell is aimed at. Each target kind reads its own field,
   which is what makes the two assertions here - on iChar and on iMonsterID -
   name two different fields of one block rather than one field twice.

   A character or a monster whose name the party does not have is described by
   its name-prefix instead, looked up in the table at 0x0061E436 - by faction
   for a character and by name group for a monster, which is what makes the two
   one table. The entry is a message-table offset rather than a string, so it
   is resolved twice. Everything else is a fixed word. Its error
   text names the function. */
// FUNCTION: WIZ8 0x004F97A0
wchar_t* SpellTargetString(int unused, const W8CombatSlot* target)
{
    unsigned short name_prefix;

    if (target == 0) {
        srAssertFail("pTarget != NULL", MAGIC_CPP, 0xa4, 0);
    }

    switch (target->kind) {
    case 0:
    case 6:
        return g_no_target_text;

    case 1:
    case 9:
        if (target->character_slot == -1) {
            srAssertFail("pTarget->iChar != BAD_INDEX", MAGIC_CPP, 0xad, 0);
        }
        if (!TargetSourceIsCharacter((const W8TargetSource*)target, 0) || target->described.name_known != 0) {
            return FormatWideString(
                g_message_strings[W8_MESSAGE_TARGET_AT / 4],
                g_party_characters[target->character_slot].name);
        }
        name_prefix =
            g_name_prefix_messages[g_party_characters[target->character_slot].faction * 4];
        break;

    case 2:
        return g_message_strings[W8_MESSAGE_TARGET_PARTY / 4];

    case 3: {
        W8MonsterInfo* monster_info;
        W8MonsterRecord* record;

        if (target->monster_id == -1) {
            srAssertFail("pTarget->iMonsterID != BAD_INDEX", MAGIC_CPP, 0xbd, 0);
        }
        monster_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0xc0, MAGIC_CPP, target->monster_id, 1));
        record = GetMonsterDataForInfo(monster_info);
        if (!TargetSourceIsMonster((const W8TargetSource*)target, 0)) {
            return FormatWideString(
                g_message_strings[W8_MESSAGE_TARGET_AT / 4],
                GetMonsterName(monster_info, record, 0));
        }
        name_prefix = g_name_prefix_messages[record->name_group_0cc * 4];
        break;
    }

    case 4:
        return FormatWideString(
            g_message_strings[W8_MESSAGE_TARGET_AT / 4],
            GetMonsterGroupName(GetMonsterGroupByListIndex(
                GetMonsterGroupIndexByID(0xcf, MAGIC_CPP, target->group_id, 1))));

    case 5:
        return g_message_strings[W8_MESSAGE_TARGET_PLACE / 4];

    case 7:
        return FormatWideString(
            g_message_strings[W8_MESSAGE_TARGET_ITEM / 4],
            FormatItemDisplayName(target->item, 0));

    case 8:
        return g_message_strings[W8_MESSAGE_TARGET_DIRECTION / 4];

    default:
        srAssertFail(
            "FALSE", MAGIC_CPP, 0xde,
            FormatString("SpellTargetString: ERROR - Invalid spell target for %d", target->kind));
        return g_message_strings[W8_MESSAGE_TARGET_UNKNOWN / 4];
    }

    return FormatWideString(
        g_message_strings[W8_MESSAGE_TARGET_AT / 4], g_message_strings[name_prefix]);
}

extern void SetTargetSourceToMonster(const W8MonsterInfo* monster_info, W8TargetSource* source);
/* 0x0053BE50 */
extern void WriteGameLog(int channel, const wchar_t* format, ...);
extern void NoteSpellCast(int spell_id, int result);                     /* 0x0052C320 */
extern unsigned char g_log_verbose_0068510c;
extern void SetTextBoxMode(unsigned char mode, int value);   /* 0x005905C0 */

/* The two log lines a monster's cast is announced with: one that names the
   power level and one that does not. */
enum { W8_MESSAGE_MONSTER_CAST_VERBOSE = 0x638, W8_MESSAGE_MONSTER_CAST = 0x63c };

/* A monster casts. The line it is announced with names the caster, the spell
   and what it is aimed at; the quiet form leaves the power level out and puts
   the text box into its message mode, and the verbose form keeps the power
   level and does not.

   Its failure chance is the same rule the party's is, with the monster's
   spell-point budget standing in for a caster's spellbook skill, which is what
   makes the two one rule rather than a monster-specific one. */
// FUNCTION: WIZ8 0x004FAEC0
void MonsterCastsSpell(W8MonsterInfo* monster_info, int spell_id, unsigned int power_level)
{
    W8MonsterRecord* record = GetMonsterDataForInfo(monster_info);
    W8TargetSource source;
    unsigned int budget;
    unsigned int failure;
    int result;

    SetTargetSourceToMonster(monster_info, &source);

    if (g_log_verbose_0068510c == 0) {
        WriteGameLog(
            9, g_message_strings[W8_MESSAGE_MONSTER_CAST / 4],
            GetMonsterName(monster_info, record, 0),
            g_spell_records[spell_id].display_name,
            SpellTargetString((int)&source, &monster_info->combat_slot_2ba));
        SetTextBoxMode(1, 9);
    }
    else {
        WriteGameLog(
            9, g_message_strings[W8_MESSAGE_MONSTER_CAST_VERBOSE / 4],
            GetMonsterName(monster_info, record, 0),
            g_spell_records[spell_id].display_name, power_level,
            SpellTargetString((int)&source, &monster_info->combat_slot_2ba));
    }

    record = GetMonsterDataForInfo(monster_info);
    budget = monster_info->sp_budget_bonus + record->sp_budget;
    if (record->sp_budget == 0) {
        FormatDebugMessage(
            0, "DATA ERROR: Monster %ls casting spells with SP Budget of 0",
            GetMonsterName(monster_info, 0, 0));
    }
    if ((int)budget < 0) {
        budget = 0;
    }
    failure = GetSpellFailureChance(budget, spell_id, (int)power_level);

    CastSpellFromSource(
        spell_id, &source, &monster_info->combat_slot_2ba, power_level, 0, failure, 0,
        (int)&result, 0, 0, 0);
    NoteSpellCast(spell_id, result);
}
