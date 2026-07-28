#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

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
enum { W8_SKILL_EXEMPTING_ALCHEMY = 26 };

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
    unsigned char* combat_slot);
extern unsigned char Function5369F0(W8MonsterInfo* monster_info);
extern unsigned char Function4D9080(W8MonsterInfo* monster_info, int arg_2, int arg_3);
extern unsigned char Function5327E0(
    W8MonsterInfo* monster_info, int spell_id, unsigned char* combat_slot);
extern unsigned char Function5330E0(
    W8MonsterInfo* monster_info, int spell_id, unsigned char* combat_slot);
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
        return character->skills[W8_SKILL_EXEMPTING_ALCHEMY].level == 0;
    }
    return false;
}

/* The same question for a monster. The block stops everything except alchemy
   from the three kinds that keep it. */
// FUNCTION: WIZ8 0x004FB1D0
bool IsSpellBlockedForMonster(W8MonsterInfo* monster_info, int spell_id)
{
    unsigned char kind;

    if (monster_info->spellcasting_blocked != 0) {
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
    unsigned char* combat_slot;
    unsigned char kind;

    if (g_spell_records[spell_id].monster_castable == 0) {
        srAssertFail(
            "FALSE", MAGIC_CPP, 1132,
            FormatString("MonsterOKToCastSpell: ERROR - spell not monster castable: %hs",
                         g_spell_records[spell_id].display_name));
        return false;
    }

    if (monster_info->spellcasting_blocked != 0) {
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

    combat_slot = monster_info->combat_slot_2ba;
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

/* Record what one party slot is about to cast at, from a target block the
   caller already holds. */
// FUNCTION: WIZ8 0x004F9AA0
void SetPartySlotSpellTarget(
    int party_slot, int target_kind, int target_value, const int* target_block)
{
    W8PartySlotRow* row = &g_party_slot_rows[party_slot];
    int index;

    row->spell_target_kind = target_kind;
    row->spell_target_value = target_value;
    row->spell_target_reset = 0;
    for (index = 0; index < 8; ++index) {
        row->spell_target_block[index] = target_block[index];
    }
}

/* The same, addressed by character rather than by slot, and taking the target
   block from the targeting code instead of the caller. */
// FUNCTION: WIZ8 0x004F9A20
void SetCharacterSpellTarget(const W8Character* character, int target_kind, int target_value)
{
    int party_slot = CharacterPointerToPartySlot(character);
    int notify[2];
    const int* target_block;
    W8PartySlotRow* row;
    int index;

    notify[0] = target_value;
    notify[1] = 0;
    Function4E7CC0(party_slot, 7, target_kind, notify, 0, 1);
    target_block = Function53B7F0(party_slot, 6);

    row = &g_party_slot_rows[party_slot];
    row->spell_target_value = target_value;
    row->spell_target_kind = target_kind;
    row->spell_target_reset = 0;
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
        g_party_slot_rows[party_slot].spell_target_kind, 0, 6);

    if (!Function537160(Function53B7F0(party_slot, 3), needed)) {
        return false;
    }
    return Function519180(party_slot, 0, 3) != 0;
}
