#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_screens/MGSPortraits.h"
#include "wiz8/local_screens/RCSItemsPage.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/xstatus.h"
#include "wiz8/layouts/character.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/PolyPick.h"
#include "wiz8/startup_world.h"
#include "surrender/srMath.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/float_constants.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/engine_code/Missile.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/local_code/SpellEffect.h"
#include "wiz8/engine_code/SpellVisual.h"
#include "wiz8/local_code/CombatHostility.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/notices.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "random.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/local_code/MonsterAI.h"
#include "wiz8/engine_code/GameData.h"

#include <cstdlib>
#include <wchar.h>
#include "wiz8/local_screens/OptionsScreen.h"
#include "wiz8/local_screens/Screens.h"

/* Local Code\Magic.cpp, named by the assertion this body embeds. */

// FUNCTION: WIZ8 0x004ff3b0
int GetProfessionCasterLevel(W8Character* character, int profession_id)
{
    int magic_level_offset;

    if (profession_id == -1) {
        profession_id = character->iProfession;
        if (profession_id == -1) {
            srAssertFail("iProfession != -1", "C:\\Projects\\Wizardry 8\\Local Code\\Magic.cpp",
                         0xe13, 0);
        }
    }

    magic_level_offset = g_profession_magic_level_offsets[profession_id];
    if (magic_level_offset == -255) {
        return -1;
    }
    return character->profession_levels[profession_id] + magic_level_offset;
}

/* The all-enemies target type costs three off the difficulty - hitting
   everything is priced easier than picking targets. */

/* How hard one spell is to bring off. Half the caster's own figure, plus the
   caller's bonus, plus half of the spell's level and half its point cost taken
   together - so the point cost counts a quarter and the level a half. One
   target type is three easier than the rest, and the answer never goes below
   zero. */
// FUNCTION: WIZ8 0x004ff790
int GetSpellDifficulty(unsigned int caster_figure, int spell_id, int bonus)
{
    int difficulty =
        (caster_figure >> 1) + bonus +
        (g_spell_records[spell_id].spell_point_cost / 2 + g_spell_records[spell_id].spell_level) /
            2;

    if (GetSpellTargetType(spell_id, 0) == W8_TARGET_TYPE_ALL_ENEMIES) {
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
bool CanCastFromItem(const W8Character* caster, const W8ItemInstance* item)
{
    if (g_item_records[item->iItemNo].category != 3) {
        return false;
    }
    if (item->identified == 0) {
        return false;
    }
    return CanCharacterUseItem(caster, item->iItemNo) != 0;
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

/* SPELL_COUNT, named by the SpellUsableNow assertion that bounds its
   argument. The usable-when domain is W8SpellUsage, declared with the spell
   record. */
enum { W8_SPELL_COUNT = 0x96 };

/* Whether a spellcasting block stops this character casting this spell. The
   block stops everything except alchemy in the hands of someone who has the
   skill for it. */
// FUNCTION: WIZ8 0x004fae70
bool IsSpellBlockedForCharacter(const W8Character* character, int spell_id)
{
    if (character->uiCondition[W8_CONDITION_SPELLCASTING_BLOCKED] != 0) {
        if (g_spell_records[spell_id].alchemy_spell == 0) {
            return true;
        }
        return character->skills[W8_SKILL_ALCHEMY].level == 0;
    }
    return false;
}

/* The same question for a monster. The block stops everything except alchemy
   from the three kinds that keep it. */
// FUNCTION: WIZ8 0x004fb1d0
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
// FUNCTION: WIZ8 0x004fb0a0
bool MonsterOKToCastSpell(W8MonsterInfo* monster_info, int spell_id, int power_level)
{
    W8MonsterRecord* record = GetMonsterDataForInfo(monster_info);
    W8CombatSlot* combat_slot;
    unsigned char kind;

    if (g_spell_records[spell_id].monster_castable == 0) {
        srAssertFail("FALSE", MAGIC_CPP, 1132,
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

    if (GetSpellTargetType(spell_id, 0) == W8_TARGET_TYPE_CASTER) {
        SetMonsterCombatTarget(monster_info, monster_info->location_id);
    } else if (!MonsterTargetMatchesSpell(monster_info, spell_id)) {
        return false;
    }

    combat_slot = &monster_info->Target;
    if (!MonsterActionReachesTarget(monster_info, record, 0, combat_slot) &&
        !ClearMonsterCombatSlot(monster_info)) {
        return false;
    }
    if (DispatchWorldCursorNodeCommand004D9080(monster_info, 4, 0)) {
        return false;
    }
    if (!MonsterSpellTargetOK(monster_info, spell_id, combat_slot)) {
        return false;
    }
    return !SpellAreaHitsNeutralMonster(monster_info, spell_id, combat_slot);
}

/* Whether the spell may be cast in the situation the party is in now. Two
   spells are always allowed on the shop screen out of combat; otherwise the
   record's usable-when value picks which of camp, combat and the shop admit
   it. Every retail call site pushes only the two parameters, so the second
   doubles as the out-of-combat override and the default-case return. */
// FUNCTION: WIZ8 0x005001e0
bool SpellUsableNow(int spell_id, unsigned char allow_out_of_combat)
{
    W8SpellUsage usable_when;
    bool lock_or_trap;

    if (spell_id >= W8_SPELL_COUNT) {
        srAssertFail("uiSpell < SPELL_COUNT", MAGIC_CPP, 4179, 0);
    }
    usable_when = g_spell_records[spell_id].usable_when;
    if (usable_when >= W8_SPELL_USAGE_COUNT) {
        srAssertFail("uiSpellUsableWhen < SPELL_USAGE_COUNT", MAGIC_CPP, 4182, 0);
    }

    if (g_current_screen_state.id == W8_SCREEN_CAMP && gXStatus.fCombatMode == 0 &&
        gXStatus.fCampMode != 0 && (spell_id == 0x17 || spell_id == 0x3a)) {
        return 1;
    }

    lock_or_trap = gXStatus.fLockInteract != 0 || gXStatus.fTrapInteract != 0;

    switch (usable_when) {
    case W8_SPELL_USABLE_ANY_TIME:
        break;
    case W8_SPELL_USABLE_IN_COMBAT:
        if (gXStatus.fCombatMode != 1 && allow_out_of_combat == 0) {
            return 0;
        }
        break;
    case W8_SPELL_USABLE_OUT_OF_COMBAT:
        if (gXStatus.fCombatMode != 0) {
            return 0;
        }
        break;
    case W8_SPELL_USABLE_WHILE_CAMPED:
        if (gXStatus.fCampMode == 0) {
            return 0;
        }
        return !lock_or_trap;
    case W8_SPELL_USABLE_ON_LOCK_OR_TRAP:
        if (spell_id != 0x27) {
            return spell_id == 0x12 ? gXStatus.fTrapInteract : 0;
        }
        if (gXStatus.fLockInteract != 0) {
            return 1;
        }
        return gXStatus.fTrapInteract != 0;
    default:
        srAssertFail("FALSE", MAGIC_CPP, 4228, "SpellUsableNow: ERROR - Invalid uiSpellUsableWhen");
        return allow_out_of_combat;
    }

    if (gXStatus.fCampMode == 0) {
        return !lock_or_trap;
    }
    return 0;
}

/* What the interface has to ask the player to pick before a friendly spell can
   be cast. Nine target types map onto seven answers; the one that depends on
   the selection owner only needs a pick when there is nothing selected already
   and the caller is not in one of the two contexts that supply it. */
// FUNCTION: WIZ8 0x005010f0
int GetTargetNeededForSpellFriendly(int spell_id, unsigned char normalize,
                                    W8TargetingContext context)
{
    if (spell_id != 0) {
        switch (GetSpellTargetType(spell_id, normalize)) {
        case W8_TARGET_TYPE_CASTER:
            return 8;
        case W8_TARGET_TYPE_ALLY:
            return spell_id != 0x58 ? 1 : 7;
        case W8_TARGET_TYPE_PARTY:
        case W8_TARGET_TYPE_ALL_ENEMIES:
        case W8_TARGET_TYPE_LOCK_OR_TRAP:
            break;
        case W8_TARGET_TYPE_ENEMY:
            return 2;
        case W8_TARGET_TYPE_ENEMY_GROUP:
            return 5;
        case W8_TARGET_TYPE_CONE:
            return 4;
        case W8_TARGET_TYPE_RADIUS:
        case W8_TARGET_TYPE_POINT:
            return 3;
        case W8_TARGET_TYPE_ITEM:
            if (g_level_block == 0 || context == W8_TARGETING_CONTEXT_SPELL ||
                context == W8_TARGETING_CONTEXT_ITEM) {
                return 6;
            }
            break;
        default:
            srAssertFail(
                "FALSE", MAGIC_CPP, 4851,
                FormatString("GetTargetNeededForSpellFriendly: ERROR - Invalid spell target for %d",
                             spell_id));
        }
    }
    return 0;
}

/* The same question for a hostile spell, which has fewer answers because a
   hostile spell never targets the party's own belongings. */
// FUNCTION: WIZ8 0x005011c0
int GetTargetNeededForSpellHostile(int spell_id)
{
    switch (GetSpellTargetType(spell_id, 0)) {
    case W8_TARGET_TYPE_CASTER:
        return 8;
    case W8_TARGET_TYPE_ALLY:
        return spell_id != 0x58 ? 1 : 7;
    case W8_TARGET_TYPE_PARTY:
    case W8_TARGET_TYPE_CONE:
    case W8_TARGET_TYPE_RADIUS:
    case W8_TARGET_TYPE_ALL_ENEMIES:
    case W8_TARGET_TYPE_POINT:
        break;
    case W8_TARGET_TYPE_ENEMY:
        return 2;
    case W8_TARGET_TYPE_ENEMY_GROUP:
        return 5;
    default:
        srAssertFail(
            "FALSE", MAGIC_CPP, 4889,
            FormatString("GetTargetNeededForSpellHostile: ERROR - Invalid spell target for %d",
                         spell_id));
    }
    return 0;
}

/* Forwarder that narrows CanCharReBreathe's answer to a flag. Its argument is
   a party slot, not a spell - which is only visible once the underlying
   predicate is named. */
// FUNCTION: WIZ8 0x00501860
bool CanPartySlotReBreathe(int party_slot)
{
    if (CanCharReBreathe(party_slot) == 0) {
        return 0;
    }
    return 1;
}

/* One queued spell effect. Each entry counts down a turn at a time and is
   distinguished only by its kind; the effect body itself lives elsewhere. */
/* The kind whose expiry hands every monster back its own control. */
enum { W8_SPELL_EFFECT_KIND_MONSTER_CONTROL = 0x26 };

enum { W8_PARTY_CONDITION_SLOTS = 12, W8_COMBAT_CONDITION_SLOTS = 9 };

/* 0x00689b58 */
W8GrowableVector<W8SpellEffectEntry*> g_spell_effects;
// GLOBAL: WIZ8 0x005ED7D0
const float g_ground_settle_fail_005ed7d0 = -1000000.0f;
/* Debug switch: when set, every cast except 0x76 fizzles on a forced 100
   percent failure chance. */
// GLOBAL: WIZ8 0x00689b68
unsigned char g_flag_00689b68;
/* Whether every queued effect still has time left on it. */
// FUNCTION: WIZ8 0x00500e50
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
// FUNCTION: WIZ8 0x00500f30
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
// FUNCTION: WIZ8 0x00500e90
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
                for (monster_index = 0; monster_index < PLLength(gXStatus.plsMonsterList);
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
// FUNCTION: WIZ8 0x005012b0
bool PartyHasCondition(int condition_id)
{
    W8EffectSlot* slot = g_status_685170.effect_slots_17af;

    while (slot->active == 0 || slot->effect_id != condition_id) {
        ++slot;
        if (slot > &g_status_685170.effect_slots_17af[W8_PARTY_CONDITION_SLOTS - 1]) {
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
    W8EffectSlot* slot;
    unsigned int index;

    if (g_combat_state != 0) {
        slot = g_combat_state->effect_slots;
        for (index = 0; index < W8_COMBAT_CONDITION_SLOTS; ++index, ++slot) {
            if (slot->active != 0 && slot->effect_id == condition_id) {
                return true;
            }
        }
        /* 0x00501250: nine 0x11-byte strides from g_combat_state+0x85a.
           The first six occupy effect_slots_85a; the rest overlap
           engaged_missile and TargetHit. Retail does that overlapping
           walk; it is a raw stride, not a typed array of nine. */
        // clang-format off
        for (index = 0; index < W8_COMBAT_CONDITION_SLOTS; ++index) {
            slot = g_combat_state->effect_slots_85a + index;
            if (slot->active != 0 && slot->effect_id == condition_id) {
                return true;
            }
        }
        // clang-format on
    }
    return false;
}

/* Record the spell one party slot is about to cast, at what strength, and at
   what, from a target block the caller already holds. */
// FUNCTION: WIZ8 0x004f9aa0
void SetPartySlotSpell(int party_slot, int spell_id, int power_level, const W8CombatSlot* target)
{
    W8PartySlotRow* row = &g_status_685170.buffers.XChar[party_slot];

    row->spell_id = spell_id;
    row->spell_detail.spell.power_level = power_level;
    row->spell_detail.spell.unused = 0;
    row->spell_target = *target;
}

/* The same, addressed by character rather than by slot, and taking the target
   block from the targeting code instead of the caller. */
// FUNCTION: WIZ8 0x004f9a20
void SetCharacterSpell(const W8Character* character, int spell_id, int power_level)
{
    int party_slot = CharacterPointerToPartySlot(character);
    W8ActionDetailBlock notify;
    W8PartySlotRow* row;

    notify.spell.power_level = power_level;
    notify.spell.unused = 0;
    ChooseAction(party_slot, 7, spell_id, &notify, 0, 1);

    row = &g_status_685170.buffers.XChar[party_slot];
    row->spell_detail.spell.power_level = power_level;
    row->spell_id = spell_id;
    row->spell_detail.spell.unused = 0;
    row->spell_target = *GetTargetBlockForContext(party_slot, W8_TARGETING_CONTEXT_CURRENT);
}

/* Whether one party slot's recorded spell target is still a target it could
   reach: the target has to be of the kind the spell needs, and the slot has to
   be in range of it. */
// FUNCTION: WIZ8 0x00501530
bool PartySlotSpellTargetStillValid(int party_slot)
{
    char needed = GetTargetNeededForSpellFriendly(
        g_status_685170.buffers.XChar[party_slot].spell_id, 0, W8_TARGETING_CONTEXT_CURRENT);

    if (!TargetMatchesNeeded(GetTargetBlockForContext(party_slot, W8_TARGETING_CONTEXT_SPELL),
                             needed)) {
        return false;
    }
    return IsCurrentTargetInRange(party_slot, 0, W8_TARGETING_CONTEXT_SPELL) != 0;
}

/* Start one character's breath attack. The assertion names the predicate it
   depends on outright - CanCharReBreathe - so a character who cannot is a
   caller error rather than a refusal. */
// FUNCTION: WIZ8 0x00501880
void StartCharacterBreathAttack(int party_slot)
{
    W8PartySlotRow* row = &g_status_685170.buffers.XChar[party_slot];

    if (!CanCharReBreathe(party_slot)) {
        srAssertFail("CanCharReBreathe(uiChar)", MAGIC_CPP, 5320, 0);
    }
    ChooseAction(party_slot, 2, -1, 0, 0, 1);
    AimAtTarget(party_slot, &row->target_context_5, W8_TARGETING_CONTEXT_CURRENT);
    if (IsSpellTargetStillValidIn(party_slot, 0x77, W8_TARGETING_CONTEXT_FIVE)) {
        StartBreathCycle(party_slot, 0);
        return;
    }
    FallbackFromUnreachableAction(party_slot);
}

/* Fold one missile's accumulated damage and reports into the queued effect
   that owns it. The owning effect is the one whose missile list still names
   the missile; nothing happens if it has already been detached. */
// FUNCTION: WIZ8 0x00500460
void AbsorbMissileDamage00500460(W8Missile* missile)
{
    for (int effect_index = 0; effect_index < g_spell_effects.GetCount(); ++effect_index) {
        W8SpellEffectEntry* effect = *g_spell_effects.GetAt(effect_index);

        for (int missile_index = 0; missile_index < effect->missiles.GetCount(); ++missile_index) {
            if (*effect->missiles.GetAt(missile_index) == missile) {
                effect->result_126.count += missile->result_280.count;
                effect->result_126.amount += missile->result_280.amount;
                for (int condition = 0; condition < W8_CONDITION_COUNT; ++condition) {
                    effect->result_126.condition_counts[condition] +=
                        missile->result_280.condition_counts[condition];
                }
                while (missile->result_280.reports.GetCount() >= 1) {
                    effect->result_126.reports.Add(missile->result_280.reports.RemoveAt(0));
                }
                return;
            }
        }
    }
}

/* The queued effect's compiler-generated teardown: the five embedded lists
   release their storage in reverse declaration order. */
// FUNCTION: WIZ8 0x0042bac0
W8SpellEffectEntry::~W8SpellEffectEntry() {}

/* Append one effect to the shared queue. */
// FUNCTION: WIZ8 0x005008a0
void AddSpellEffect(W8SpellEffectEntry* effect)
{
    g_spell_effects.Add(effect);
}

void FinishSpellEffect00500F70(W8SpellEffectEntry* effect); /* 0x00500F70 */

/* Advance every queued spell effect one frame. An effect first checks that
   everything it owns is still live: its visuals have started, its missiles
   carry their launch flag, and its monster list still points at live manager
   entries. An effect whose activation byte is set turns its missiles into the
   spell's own visuals and clears that byte; an already-active effect applies
   its monster-control consequence to the combat selection. An effect that has
   run out is finalized: a spell that needed aiming and is not in the singled
   out set reports itself, its visuals and missiles are released, and the entry
   is removed from the queue and deleted. */
// FUNCTION: WIZ8 0x00500930
void UpdateSpellEffects00500930(void)
{
    bool handled = false;
    int index;

    if (g_spell_effects.GetCount() <= 0) {
        return;
    }
    for (index = 0; index < g_spell_effects.GetCount(); ++index) {
        W8SpellEffectEntry* effect = *g_spell_effects.GetAt(index);
        bool alive = true;

        for (int visual_index = 0; visual_index < effect->spell_visuals.GetCount() && alive;
             ++visual_index) {
            W8SpellVisual* visual = *effect->spell_visuals.GetAt(visual_index);
            if (visual->finished == 0) {
                alive = false;
            }
        }
        for (int missile_index = 0; missile_index < effect->missiles.GetCount() && alive;
             ++missile_index) {
            W8Missile* missile = *effect->missiles.GetAt(missile_index);
            if (missile->flag_1e0 == 0) {
                alive = false;
            }
        }
        for (int monster_index = 0; monster_index < effect->target_indices_0f0.GetCount() && alive;
             ++monster_index) {
            int entry = *effect->target_indices_0f0.GetAt(monster_index);
            if (gXStatus.monster_manager_entries[entry].effect_icon_active != 0) {
                alive = false;
            }
        }

        if ((g_spell_records[effect->kind].missile_delivered != 0 || effect->flag_122 != 0) &&
            effect->flag_121 == 0 && !alive) {
            continue;
        }
        if (effect->flag_123 != 0) {
            continue;
        }

        if (effect->flag_122 != 0) {
            W8Missile* missile = 0;

            for (int missile_index = 0; missile_index < effect->missiles.GetCount();
                 ++missile_index) {
                missile = *effect->missiles.GetAt(missile_index);
                missile->flag_1e2 = 1;
                effect->missiles.RemoveAt(missile_index);
            }
            if (missile != 0) {
                srVector3T<float> position = missile->GetPosition();
                position.y -= g_float_005ebc64;
                W8SpellVisual* visual =
                    SpawnSpellEffect(&position, g_spell_records[effect->kind].resource_name,
                                     missile->definition_1fc.duration_scale, 0, 0);
                if (visual != 0) {
                    visual->auto_release = 0;
                    effect->spell_visuals.Add(visual);
                    alive = false;
                }
            }
            effect->flag_122 = 0;
            handled = true;
        } else {
            effect->flag_123 = 1;
            if (MonsterCanAimSpell005474B0(effect->kind) != 0 && effect->Source.fBackfire == 0 &&
                effect->Source.fReflection == 0) {
                ProvokeListedMonsterGroups(&effect->Source, &effect->monster_ids_0e0);
            }
            ProcessSpellEffectTargets(effect);
            if (TargetSourceIsMonster(&effect->OrigSource, 0) != 0) {
                if (effect->OrigSource.iMonsterID == -1) {
                    srAssertFail("pOrigSource->iMonsterID != -1", MAGIC_CPP, 0x1504, 0);
                }
                unsigned int monster_list_index = MonsterGetIndexByLocationID(
                    0x1505, MAGIC_CPP, effect->OrigSource.iMonsterID, 1);
                W8MonsterInfo* monster_info =
                    MonsterGetScriptPartByLocationIndex(monster_list_index);
                if (gXStatus.fCombatMode != 0 && g_combat_state->eCombatActionStatus != 0 &&
                    g_combat_state->pActionMonsterInfo != 0 &&
                    *(int*)g_combat_state->pActionMonsterInfo == monster_info->location_id) {
                    g_combat_state->eCombatActionStatus = 3;
                }
            }
        }

        if (effect->flag_121 != 0) {
            if (effect->turns_remaining != 0) {
                continue;
            }
        } else if (!alive || effect->flag_123 == 0) {
            continue;
        }

        if (!handled && g_spell_records[effect->kind].needs_aim_13f != 0 &&
            !IsCombatEffectSlotSpell(effect->kind)) {
            W8SpellTargetType target_type = GetSpellTargetType(effect->kind, 0);
            if (target_type != W8_TARGET_TYPE_RADIUS && target_type != W8_TARGET_TYPE_PARTY) {
                if (g_settings_6850c8.verbose_combat_messages == 0) {
                    ReportSpellResult005005C0(effect);
                }
                FinishSpellEffectTargets(effect);
            }
        }
        if (effect->kind == 0x4f) {
            FinishSpellEffect00500F70(effect);
        }
        for (int release_visual = 0; release_visual < effect->spell_visuals.GetCount();
             ++release_visual) {
            W8SpellVisual* visual = *effect->spell_visuals.GetAt(release_visual);
            visual->auto_release = 1;
            if (effect->flag_121 != 0) {
                visual->finished = 1;
            }
        }
        for (int release_missile = 0; release_missile < effect->missiles.GetCount();
             ++release_missile) {
            W8Missile* missile = *effect->missiles.GetAt(release_missile);
            missile->flag_1e2 = 1;
        }
        g_spell_effects.RemoveAt(index);
        if (effect != 0) {
            delete effect;
        }
        --index;
    }
}

/* Take one missile back off the spell effect that owns it. The first effect
   whose missile list contains it drops the entry and stops the walk. */
// FUNCTION: WIZ8 0x005019a0
void DetachMissileReferences005019A0(W8Missile* missile)
{
    for (int index = 0; index < g_spell_effects.GetCount(); ++index) {
        W8SpellEffectEntry* effect = *g_spell_effects.GetAt(index);
        int missile_index = effect->missiles.IndexOf(missile);

        if (missile_index != -1) {
            effect->missiles.RemoveAt(missile_index);
            return;
        }
    }
}

/* Which spell a missile type carries, for the missile kinds that are spells
   at all. Anything else carries no spell. */
// FUNCTION: WIZ8 0x00501a60
int MissileSpellId(int missile_type)
{
    switch (missile_type) {
    case 1:
        return 1;
    case 2:
        return 5;
    case 3:
        return 10;
    case 4:
        return 11;
    case 5:
        return 25;
    case 6:
        return 79;
    case 7:
        return 47;
    case 8:
        return 93;
    case 9:
        return 48;
    case 10:
        return 80;
    case 11:
        return 55;
    case 12:
        return 36;
    case 13:
        return 81;
    case 14:
        return 57;
    case 15:
        return 76;
    case 30:
        return 87;
    case 31:
        return 31;
    case 32:
        return 122;
    case 34:
        return 121;
    default:
        return W8_SPELL_NONE;
    }
}

/* The 0x49 teleport lands on the character's saved anchor, so without an
   anchor set it cannot run. Every other spell id clears this block. */
// FUNCTION: WIZ8 0x00501D00
bool IsTeleportCastMissingAnchor00501D00(W8Character* character, int spell_id)
{
    if (spell_id != 0x49) {
        return false;
    }
    return !character->has_saved_location;
}

/* The 0x4f spell's finalizer. Once its target is gone, the impact spell 0x76
   is cast at the target's last position and the matching notice is posted:
   the monster's own notice for a monster that has died, or the targeted
   character's notice for a character whose row is empty. A text box is only
   opened when the detailed combat messages are off, matching the ordinary
   damage path. */
// FUNCTION: WIZ8 0x00500F70
void FinishSpellEffect00500F70(W8SpellEffectEntry* effect)
{
    W8MonsterInfo* monster_info = 0;
    W8CombatSlot target;
    srVector3T<float> position;

    if (effect->target.iType == W8_TARGET_KIND_MONSTER) {
        monster_info = MonsterInfoFromID(0x1279, MAGIC_CPP, effect->target.iMonsterID, 1);
    }
    if ((effect->target.iType == W8_TARGET_KIND_MONSTER && monster_info->hp_current == 0) ||
        (effect->target.iType == W8_TARGET_KIND_CHARACTER &&
         g_status_685170.buffers.Char[effect->target.iChar].hp_current == 0)) {
        target.iType = W8_TARGET_KIND_PLACE;
        if (effect->target.iType == W8_TARGET_KIND_MONSTER) {
            W8NavigatorMovementState* movement = &monster_info->monster->movement_0c0;
            position = movement->position_040;
            position.y += movement->height_offset_0b8;
            position.y = SettlePositionToGround00420BD0(&position, 0);
            PostMonsterNotice(monster_info, gppStringList[0x654 / 4]);
        } else {
            GetCameraPosition(&position);
            position.y -= g_default_world_height_00603ac8;
            PostCharacterNotice(effect->target.iChar, gppStringList[0x654 / 4]);
        }
        if (g_settings_6850c8.verbose_combat_messages == 0) {
            SetTextBoxMode(1, -1);
        }
        target.point = position;
        CastSpellFromSource(0x76, &effect->Source, &target, effect->definition.duration_scale,
                            effect->definition.percent, 0, 0, 0, 0, 0, 0);
    }
}

/* Scale a value by how far ahead of the difficulty's own pace one combatant
   is. Out of combat nothing is scaled; in combat a combatant slower than the
   pace is left alone too. */
// FUNCTION: WIZ8 0x00501910
unsigned int ScaleByCombatPace(int party_slot, unsigned int* value)
{
    unsigned int pace;
    unsigned int phase_clock;
    int scaled;

    if (gXStatus.fCombatMode == 0) {
        return gXStatus.fCombatMode;
    }

    if (g_settings_6850c8.difficulty == W8_DIFFICULTY_NOVICE) {
        pace = 0x50;
    } else if (g_settings_6850c8.difficulty == W8_DIFFICULTY_NORMAL) {
        pace = 0x3c;
    } else {
        if (g_settings_6850c8.difficulty != W8_DIFFICULTY_EXPERT) {
            srAssertFail("FALSE", MAGIC_CPP, 5352, 0);
        }
        pace = 0x28;
    }

    phase_clock = g_combat_state->characters[party_slot].phase_clock_stamp;
    if (pace <= phase_clock) {
        scaled = ((0x32 - pace) + phase_clock) * *value;
        *value = scaled / 50;
    }
    return phase_clock;
}

/* How likely a spell is to fail outright. The spell's own cost band picks a
   difficulty off a seventeen-entry table, scaled by the caller's factor and
   divided by seven; a caster already at or past that has no chance of failing
   at all, and the rest is the shortfall as a percentage capped at a hundred. */
// FUNCTION: WIZ8 0x004ff410
unsigned int GetSpellFailureChance(unsigned int skill, int spell_id, int factor)
{
    int band =
        g_spell_records[spell_id].spell_point_cost / 2 + g_spell_records[spell_id].spell_level;
    unsigned int needed;
    unsigned int chance;

    if (band > 0x10) {
        band = 0x10;
    }
    needed = static_cast<unsigned int>(
                 g_combat_effect_slot_spells_and_cast_success_00616dd8[6 + band] * factor) /
             7;
    if (needed <= skill) {
        return 0;
    }
    chance = (needed * 70 - skill * 70) / needed;
    if (static_cast<int>(chance) < 0) {
        return 0;
    }
    if (static_cast<int>(chance) > 100) {
        return 100;
    }
    return chance;
}

// GLOBAL: WIZ8 0x0061634c
unsigned char g_profession_spellbooks[15] = {
    0, 2, 2, 4, 1, 4, 8, 0, 0, 0, 2, 4, 15, 8, 1,
};

/* Begin one character's spell. The recorded target is copied to the stack
   first because choosing the action overwrites it, and out of combat the
   original target is re-aimed at before that happens. The spell is always the
   slot's own; the argument overrides only the strength it goes off at, and
   zero means keep the recorded one. */
// FUNCTION: WIZ8 0x00501590
void StartCharacterSpellCast(int party_slot, int power_level)
{
    W8PartySlotRow* row = &g_status_685170.buffers.XChar[party_slot];
    W8CombatSlot saved_target = row->spell_target;
    W8ActionDetailBlock named;
    const W8ActionDetailBlock* target;

    if (gXStatus.fCombatMode == 0) {
        AimAtTarget(party_slot, &row->spell_target, W8_TARGETING_CONTEXT_CURRENT);
    }

    if (power_level == 0) {
        target = &row->spell_detail;
    } else {
        named.spell.power_level = power_level;
        named.spell.unused = 0;
        target = &named;
    }
    ChooseAction(party_slot, 7, row->spell_id, target, 0, 1);
    AimAtTarget(party_slot, &saved_target, W8_TARGETING_CONTEXT_CURRENT);

    if (IsSpellTargetStillValidIn(party_slot, row->spell_id, W8_TARGETING_CONTEXT_SPELL)) {
        StartBreathCycle(party_slot, 0);
        return;
    }
    FallbackFromUnreachableAction(party_slot);
}

/* The same for using an item. The item's own spell decides whether the target
   still holds, and where the item came from is recorded before the check so a
   failed use can put it back. */
// FUNCTION: WIZ8 0x00501790
void StartCharacterItemUse(int party_slot)
{
    W8PartySlotRow* row = &g_status_685170.buffers.XChar[party_slot];
    W8CombatSlot saved_target = row->item_target;

    if (gXStatus.fCombatMode == 0) {
        AimAtTarget(party_slot, &row->item_target, W8_TARGETING_CONTEXT_CURRENT);
    }
    ChooseAction(party_slot, 8, -1, &row->item_detail, 0, 1);
    AimAtTarget(party_slot, &saved_target, W8_TARGETING_CONTEXT_CURRENT);
    RecordItemOrigin(party_slot, row->item_origin, row->item_slot);

    if (IsSpellTargetStillValidIn(party_slot, GetItemSpell(row->item_detail.item_use.item),
                                  W8_TARGETING_CONTEXT_ITEM)) {
        StartBreathCycle(party_slot, 0);
        return;
    }
    FallbackFromUnreachableAction(party_slot);
}

/* A character's whole casting strength in one spellbook: their current
   profession's caster level, plus every other profession they have ever held
   that shares the book. Only positive contributions count, and a caster level
   of nothing at all short-circuits unless the caller asks for the sum
   anyway. */
// FUNCTION: WIZ8 0x004ffbd0
int GetTotalCasterLevel(const W8Character* character, int unused, int spellbook, char include_all)
{
    int profession = character->iProfession;
    int total;
    int other;
    int level;

    if (profession == -1) {
        srAssertFail("iProfession != -1", MAGIC_CPP, 3603, 0);
    }
    if (g_profession_magic_level_offsets[profession] == -255) {
        total = -1;
    } else {
        total =
            character->profession_levels[profession] + g_profession_magic_level_offsets[profession];
    }
    if (total < 1 && include_all == 0) {
        return total;
    }

    for (other = 0; other < 15; ++other) {
        if (character->profession_levels[other] != 0 && other != character->iProfession &&
            (g_profession_spellbooks[other] & spellbook) != 0) {
            if (g_profession_magic_level_offsets[other] != -255) {
                level =
                    character->profession_levels[other] + g_profession_magic_level_offsets[other];
                if (level > 0) {
                    total += level;
                }
            }
        }
    }
    return total;
}

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
                           (g_spell_records[spell_id].psionics_spell != 0 ? W8_SPELLBOOK_PSIONICS
                                                                          : W8_SPELLBOOK_NONE) |
                           (g_spell_records[spell_id].divinity_spell != 0 ? W8_SPELLBOOK_DIVINITY
                                                                          : W8_SPELLBOOK_NONE) |
                           (g_spell_records[spell_id].alchemy_spell != 0 ? W8_SPELLBOOK_ALCHEMY
                                                                         : W8_SPELLBOOK_NONE));
}

/* Recount the learned spells into the six per-realm slots (0x1c..0x21 of
   skill_unlocks), where the expert-skill gate and the spell-point ceiling
   both read them. */
// FUNCTION: WIZ8 0x004f96a0
void RecountLearnedSpellsByRealm004F96A0(W8Character* character)
{
    for (int realm = 0; realm < 6; ++realm) {
        character->skill_unlocks[0x1c + realm] = 0;
    }
    for (int index = 0; index < 0x72; ++index) {
        if (character->spell_learned[index] == 1 || character->spell_learned[index] == 2) {
            ++character->skill_unlocks[0x1c + g_spell_records[index].realm];
        }
    }
}

/* Whether the character knows any spell whose realm's remaining points still
   cover that spell's cost. */
// FUNCTION: WIZ8 0x004f96f0
bool CharacterHasCastableSpell(W8Character* character)
{
    int spell_id;

    for (spell_id = 0; spell_id < 0x72; ++spell_id) {
        if (spell_id != 0 && character->spell_learned[spell_id] == 1 &&
            g_spell_records[spell_id].spell_point_cost <=
                character->iSPLeft[g_spell_records[spell_id].realm]) {
            return 1;
        }
    }
    return 0;
}

/* Learned, and the remaining points in the spell's realm cover its cost. */
// FUNCTION: WIZ8 0x004f9750
bool CanCharacterCastSpell(W8Character* character, int spell_id)
{
    if (spell_id != 0 && character->spell_learned[spell_id] == 1 &&
        g_spell_records[spell_id].spell_point_cost <=
            character->iSPLeft[g_spell_records[spell_id].realm]) {
        return 1;
    }
    return 0;
}

/* Whether a character is far enough along to take one spell on. Their whole
   caster level - the current profession's plus every other one that shares the
   spell's book - fixes the highest spell level they could ever hold, and their
   realm skill and spellbook skill together fix a second, lower ceiling. The
   spell has to be at or under both, the character's profession has to be in
   one of the spell's books, and the blocking trait has to be down.

   The two ceilings are computed even when the first alone would settle it,
   which is what shows them as one rule rather than two guards. */
// FUNCTION: WIZ8 0x004ffcb0
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

    if ((g_profession_spellbooks[character->iProfession] & book) == W8_SPELLBOOK_NONE) {
        return 0;
    }
    if (CharacterHasTrait00547940(character, W8_TRAIT_CANNOT_LEARN)) {
        return 0;
    }

    caster_level = GetProfessionCasterLevel(character, -1);
    if (caster_level > 0) {
        for (other = 0; other < 15; ++other) {
            if (character->profession_levels[other] != 0 && other != character->iProfession &&
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

/* 0x0068C09C: the loaded message table, one wide string per entry. Bodies
   name entries by their byte offset into it, which is why the index is
   spelled as one. */
/* One message-table index per realm, for the realm's name. */
// GLOBAL: WIZ8 0x0061E518
// offset alias of the tail of g_attr_table_61E50C; shared retail storage.
extern const unsigned short g_realm_message_offsets[W8_SPELL_REALM_COUNT] = {
    0x30b, 0x30c, 0x30d, 0x30e, 0x30f, 0x310,
};

/* Take one spell on. The spell is marked known, its realm's known count goes
   up, the spell-point pools are recomputed, and - when the caller asks for it -
   the character says so in a line built from the character's name, the spell's
   name and the realm's remaining points.

   The line is assembled twice over: once only to measure the three pieces so
   the buffer can be allocated, and once into it. */
// FUNCTION: WIZ8 0x004ffe70
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
    character->skill_unlocks[W8_RESISTANCE_BONUS_SKILL] =
        RebuildRealmSpellPointCeilings0052A540(character);

    if (announce == 0) {
        return;
    }

    piece = FormatWideString(gppStringList[0x6e4 / 4], character->name);
    name_length = wcslen(piece);
    spell_length = wcslen(g_spell_records[spell_id].display_name);
    wcscpy(realm_name, gppStringList[g_realm_message_offsets[realm]]);
    piece = FormatWideString(gppStringList[0x6e8 / 4], realm_name, character->sp_max[realm]);
    points_length = wcslen(piece);

    line = new wchar_t[name_length + spell_length + 8 + points_length];
    if (line == 0) {
        srAssertFail("wTempMsg", MAGIC_CPP, 0xfdc, 0);
    }
    wcscpy(line, FormatWideString(gppStringList[0x6e4 / 4], realm_name));
    wcscat(line, L" -- ");
    wcscat(line, g_spell_records[spell_id].display_name);
    wcscat(line, L", ");
    wcscat(line, FormatWideString(gppStringList[0x6e8 / 4], realm_name, character->sp_max[realm]));
    ShowNoticeLine(line, 0, 1, 0);
}

// GLOBAL: WIZ8 0x0068c510
int g_learn_sound_0068c510;
/* Learn the spell a scroll or book teaches, and consume it. The item has to
   carry a spell - the assertion names the field ubSpellNumber - and the
   character has to be able to take it on; failing that the item is left alone
   and the refusal is shown.

   Learning practises three things at once: the learning skill at twice the
   spell's level, the spell's own realm skill at its level, and every spellbook
   skill the spell belongs to at the same. */
// FUNCTION: WIZ8 0x00500060
void LearnSpellFromItem(W8Character* character, W8ItemInstance* item)
{
    unsigned int spell_id;
    int usage_points;
    unsigned int skill_id;

    if (g_item_records[item->iItemNo].spell_id == W8_SPELL_NONE) {
        srAssertFail("gpItemDB[pPCItem->iItemNo].ubSpellNumber != SPELL_NONE", MAGIC_CPP, 0x101f,
                     0);
    }
    spell_id = g_item_records[item->iItemNo].spell_id;

    if (!CanCharacterLearnSpell(character, spell_id)) {
        ShowNoticeLine(FormatWideString(gppStringList[0x6ec / 4], character->name), 0, 1, 0);
        return;
    }

    LearnSpell(character, spell_id, 1);
    usage_points = g_spell_records[spell_id].spell_level;
    PracticeCharacterSkill(character, W8_SKILL_SPELL_LEARNING, usage_points * 2, 0);
    PracticeCharacterSkill(character, W8_SKILL_FIRST_REALM + g_spell_records[spell_id].realm,
                           usage_points, 0);
    for (skill_id = W8_SKILL_FIRST_SPELLBOOK; skill_id < W8_SKILL_AFTER_SPELLBOOK; ++skill_id) {
        if (g_spell_records[spell_id].wizardry_spell != 0) {
            PracticeCharacterSkill(character, skill_id, usage_points, 0);
        }
    }
    EmptyItemRecord(item, character, 1);
    QueueCharacterEvent(character, g_learn_sound_0068c510, 0, g_effect_argument_005ed8c8,
                        g_effect_argument_005ed914);
}

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
// FUNCTION: WIZ8 0x005012e0
bool CanPartySlotCastRecordedSpell(int party_slot)
{
    const W8PartySlotRow* row = &g_status_685170.buffers.XChar[party_slot];
    int spell_id = row->spell_id;
    int power_level = row->spell_detail.spell.power_level;

    if (spell_id == 0) {
        return false;
    }
    if (spell_id == W8_SPELL_CONDITIONAL && GetConditionRecordFlag(party_slot, 0)) {
        return false;
    }
    if (g_status_685170.buffers.Char[party_slot].spell_learned[spell_id] != 1) {
        return false;
    }
    if (row->spell_detail.spell.power_level == W8_SPELL_POWER_AS_AFFORDABLE &&
        g_spell_records[spell_id].blocks_auto_power_in_combat == 1 && gXStatus.fCombatMode != 0) {
        return false;
    }

    if (power_level == W8_SPELL_POWER_AS_AFFORDABLE) {
        power_level = 1;
    }
    if (g_spell_records[spell_id].spell_point_cost * power_level >
        g_status_685170.buffers.Char[party_slot].iSPLeft[g_spell_records[spell_id].realm]) {
        return false;
    }
    if (!SpellUsableNow(spell_id, 0)) {
        return false;
    }
    if (gXStatus.fCombatMode == 0 &&
        !IsSpellTargetStillValidIn(party_slot, spell_id, W8_TARGETING_CONTEXT_SPELL)) {
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
    const W8PartySlotRow* row = &g_status_685170.buffers.XChar[party_slot];
    int spell_id = row->spell_id;
    int power_level = row->spell_detail.spell.power_level;
    int cost;

    if (spell_id == 0) {
        return 0;
    }
    if (spell_id == W8_SPELL_CONDITIONAL && GetConditionRecordFlag(party_slot, 0)) {
        return 0;
    }
    if (g_status_685170.buffers.Char[party_slot].spell_learned[spell_id] != 1) {
        return 0;
    }
    if (row->spell_detail.spell.power_level == W8_SPELL_POWER_AS_AFFORDABLE &&
        g_spell_records[spell_id].blocks_auto_power_in_combat == 1 && gXStatus.fCombatMode != 0) {
        return 0;
    }
    if (!SpellUsableNow(spell_id, 0)) {
        return 0;
    }
    if (gXStatus.fCombatMode == 0 &&
        !IsSpellTargetStillValidIn(party_slot, spell_id, W8_TARGETING_CONTEXT_SPELL)) {
        return 0;
    }

    if (power_level == W8_SPELL_POWER_AS_AFFORDABLE) {
        power_level = 1;
    }
    cost = g_spell_records[spell_id].spell_point_cost * power_level;
    while (power_level != 0) {
        if (cost <=
            g_status_685170.buffers.Char[party_slot].iSPLeft[g_spell_records[spell_id].realm]) {
            return power_level;
        }
        --power_level;
        cost -= g_spell_records[spell_id].spell_point_cost;
    }
    return 0;
}

/* Whether the slot could go through with the item use it has recorded. The
   item is looked up again from where it was taken rather than trusted, and the
   re-read pointer is stored back, so a stale record is caught here and not at
   the point of use. A record pointing into the shared party pool is refused
   in combat, where only what a character personally carries is reachable.

   A spell whose target type is not the self-only one may be aimed anywhere; the
   self-only one has to be aimed at the user. */
// FUNCTION: WIZ8 0x00501660
bool CanPartySlotUseRecordedItem(int party_slot)
{
    W8PartySlotRow* row = &g_status_685170.buffers.XChar[party_slot];
    W8ItemInstance* item;
    int spell_id;
    unsigned char normalize;

    if ((signed char)row->item_origin < 0 || (short)row->item_slot < 0) {
        return false;
    }

    item = FindCharacterItemAt(party_slot, row->item_origin, row->item_slot);
    row->item_detail.item_use.item = item;

    if (row->item_origin == W8_ITEM_ORIGIN_PARTY_POOL && gXStatus.fCombatMode != 0) {
        return false;
    }
    if (item == 0 || item->iItemNo == -1 || item->iItemNo != row->item_id_0c9) {
        return false;
    }
    if (!CanUseItemForAction(party_slot, item)) {
        return false;
    }
    if (!CanCharacterActivateItem(&g_status_685170.buffers.Char[party_slot], item)) {
        return false;
    }

    spell_id = GetItemSpell(item);
    normalize = ItemClassNormalizesTarget(&g_item_records[item->iItemNo]);
    if (GetSpellTargetType(spell_id, normalize) == W8_TARGET_TYPE_CASTER &&
        row->item_target.iChar != party_slot) {
        return false;
    }
    if (!SpellUsableNow(spell_id, 0)) {
        return false;
    }
    if (gXStatus.fCombatMode == 0 &&
        !IsSpellTargetStillValidIn(party_slot, spell_id, W8_TARGETING_CONTEXT_ITEM)) {
        return false;
    }
    return true;
}

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
unsigned int ChooseMonsterSpellPowerLevel(W8MonsterInfo* monster_info, W8MonsterRecord*,
                                          int spell_id)
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
            FormatDebugMessage(0, "DATA ERROR: Monster %ls casting spells with SP Budget of 0",
                               GetMonsterName(monster_info, 0, 0));
        }
        if ((int)budget < 0) {
            budget = 0;
        }

        band =
            g_spell_records[spell_id].spell_point_cost / 2 + g_spell_records[spell_id].spell_level;
        if (band > 16) {
            band = 16;
        }
        cost = (g_combat_effect_slot_spells_and_cast_success_00616dd8[6 + band] * power_level) / 7;

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
    } else if (roll == 1 && power_level < W8_SPELL_POWER_MAX) {
        ++power_level;
    }

    if (power_level == 0 || power_level > W8_SPELL_POWER_MAX) {
        srAssertFail("( uiPowerLevel >= 1 ) && ( uiPowerLevel <= 7 )", MAGIC_CPP, 0x10bf, 0);
    }
    return power_level;
}

/* Where the cast lands, by the spell's own target type. The kind values are
   W8TargetKind's; the source-kind values are W8TargetSourceKind's. */

/* Cast one spell at the party from a point in the world - a trap, a rune, a
   scripted effect. The caller gives the point and the power level; who it hits
   comes from the spell's own target type, so this decides the target rather
   than taking one.

   A power level outside one through seven is silently taken as one, which is
   what makes a caller passing zero cast nothing at all rather than cast weakly.
   Its error text names the function. */
// FUNCTION: WIZ8 0x004fb220
int PointCastSpell(srVector3T<float> position, int spell_id, unsigned int power_level)
{
    W8TargetSource source;
    W8CombatSlot target;
    char sight_probe;

    if (power_level == 0) {
        return 0;
    }
    if (power_level > W8_SPELL_POWER_MAX) {
        power_level = 1;
    }

    ResetTargetSource(&source);
    source.iType = W8_TARGET_SOURCE_INDIRECT;
    source.point = position;

    ResetCombatSlot(&target);
    switch (GetSpellTargetType(spell_id, 0)) {
    case W8_TARGET_TYPE_CASTER:
    case W8_TARGET_TYPE_ALLY:
    case W8_TARGET_TYPE_ENEMY:
        target.iType = W8_TARGET_KIND_CHARACTER;
        target.iChar = GetRandomCharacter(0, 1, -1, -1);
        break;
    case W8_TARGET_TYPE_PARTY:
    case W8_TARGET_TYPE_ENEMY_GROUP:
        target.iType = W8_TARGET_KIND_PARTY;
        break;
    case W8_TARGET_TYPE_CONE:
        sight_probe = 0;
        target.iType = W8_TARGET_KIND_PARTY;
        ResolveTargetPoint(&target, sight_probe);
        target.iType = W8_TARGET_KIND_PLACE;
        break;
    case W8_TARGET_TYPE_RADIUS:
    case W8_TARGET_TYPE_POINT:
        sight_probe = 1;
        target.iType = W8_TARGET_KIND_PARTY;
        ResolveTargetPoint(&target, sight_probe);
        target.iType = W8_TARGET_KIND_PLACE;
        break;
    default:
        srAssertFail("FALSE", MAGIC_CPP, 0x56d,
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
// FUNCTION: WIZ8 0x004ff7f0
unsigned int GetBestSpellbookSkillForSpell(W8Character* character, int spell_id, char pricing,
                                           char prefer_unlocked, unsigned int power_level)
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

    if (pricing != 0 && character->uiCondition[W8_CONDITION_SPELLCASTING_BLOCKED] != 0 &&
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
                (character->skills[unlocked_skill].level +
                 character->skills[W8_SKILL_FIRST_REALM + g_spell_records[spell_id].realm].level *
                     4) /
                5;
            if (band > 16) {
                band = 16;
            }
            needed =
                (g_combat_effect_slot_spells_and_cast_success_00616dd8[6 + band] * power_level) / 7;
            if (skill_figure < needed) {
                failure = (needed * 70 - skill_figure * 70) / needed;
                if ((int)failure < 0) {
                    failure = 0;
                } else if ((int)failure > 100) {
                    failure = 100;
                }
            } else {
                failure = 0;
            }

            best_level = failure;
            chosen = GetMinimumCasterLevelForSpell(spell_id);
            shortfall = static_cast<int>(chosen) - GetTotalCasterLevel(character, 0, book, 1) - 1 +
                        power_level;
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
        srAssertFail("(iHighestSkill != SKILL_NONE)", MAGIC_CPP, 0xf29,
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
// FUNCTION: WIZ8 0x004ff4b0
unsigned int GetSpellFailureChanceForCast(W8Character* character, int spell_id,
                                          unsigned int power_level)
{
    int skill;
    int party_slot;
    unsigned int skill_figure;
    unsigned int chance;
    int shortfall;

    if (power_level == W8_SPELL_POWER_AS_AFFORDABLE) {
        return 0;
    }

    skill = GetBestSpellbookSkillForSpell(character, spell_id, 1, 1, power_level);
    party_slot = CharacterPointerToPartySlot(character);
    skill_figure =
        (character->skills[skill].level +
         character->skills[W8_SKILL_FIRST_REALM + g_spell_records[spell_id].realm].level * 4) /
        5;
    chance = GetSpellFailureChance(skill_figure, spell_id, (int)power_level);

    shortfall = GetMinimumCasterLevelForSpell(spell_id) -
                GetTotalCasterLevel(character, 0, SpellbookMaskForSpell(spell_id), 1) - 1 +
                power_level;
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
// FUNCTION: WIZ8 0x004fdf30
unsigned int ChoosePowerLevelForDuration(W8Character* character, int spell_id,
                                         unsigned int turns_needed)
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
        if (character->iSPLeft[realm] <
            (int)(g_spell_records[spell_id].spell_point_cost * power_level)) {
            return best_power;
        }

        failure = GetSpellFailureChanceForCast(character, spell_id, power_level);

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
// FUNCTION: WIZ8 0x004fe1c0
unsigned int ChoosePowerLevelToRestore(W8Character* character, int spell_id,
                                       const W8Character* target)
{
    int missing;
    unsigned int power_level;
    unsigned int failure;

    if (target == 0) {
        return 1;
    }

    if (spell_id == W8_SPELL_RESTORE_HP) {
        missing = target->uiHPMax - static_cast<int>(target->hp_current);
    } else if (spell_id == W8_SPELL_RESTORE_HP_THEN_STAMINA) {
        missing = target->uiHPMax - static_cast<int>(target->hp_current);
        if (missing == 0) {
            missing = target->uiStaminaMax - target->stamina;
        }
    } else if (spell_id == W8_SPELL_RESTORE_STAMINA) {
        missing = target->uiStaminaMax - target->stamina;
    } else {
        return 1;
    }

    if (missing < 1) {
        return 1;
    }

    for (power_level = 1; power_level < 8; ++power_level) {
        failure = GetSpellFailureChanceForCast(character, spell_id, power_level);
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

/* How hard the slot should cast the spell it has picked, from what its target
   actually needs. Everything it reads is the target's condition array - a
   character's at 0x0a01 or a monster's at 0x57, the same twenty entries with
   the same meanings - so the two targets are handled by one pointer.

   Zero from any of the sub-decisions means "no reason to cast harder", which
   comes back as the lowest power level rather than as nothing. */
// FUNCTION: WIZ8 0x004fe480
unsigned int ChooseSpellPowerLevelForTarget(int party_slot, int spell_id, int identify_context)
{
    W8Character* caster = &g_status_685170.buffers.Char[party_slot];
    W8PartySlotRow* row = &g_status_685170.buffers.XChar[party_slot];
    const unsigned int* conditions = 0;
    const W8Character* target_character = 0;
    unsigned int power_level;
    unsigned int worst;

    if (row->spell_target.iType == W8_TARGET_KIND_CHARACTER) {
        target_character = &g_status_685170.buffers.Char[row->spell_target.iChar];
        conditions = target_character->uiCondition;
    } else if (row->spell_target.iType == W8_TARGET_KIND_MONSTER) {
        W8MonsterInfo* monster_info =
            MonsterInfoFromID(0xb83, MAGIC_CPP, row->spell_target.iMonsterID, 1);
        conditions = monster_info->condition_turns;
    }

    switch (GetSpellTargetType(spell_id, 0)) {
    case W8_TARGET_TYPE_ALLY:
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
            if (row->spell_target.iType == W8_TARGET_KIND_CHARACTER &&
                power_level <= GetEquipmentBindingDifficulty(row->spell_target.iChar)) {
                power_level = GetEquipmentBindingDifficulty(row->spell_target.iChar);
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
        power_level = CountIdentifyAttemptsNeeded(row->spell_target.pPCItem, identify_context);
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
   offset rather than a string. A character indexes it by sex and a monster
   by its own name group at record+0x0cc, which is what makes the two one
   table. */
// GLOBAL: WIZ8 0x0061E436
// offset alias of g_gender_name_message_rows_61e430; shared retail storage.
extern const unsigned short g_name_prefix_messages[15] = {
    0x2da, 0x2d2, 0x2d5, 0x2d8, 0x2db, 0x2d3, 0x2d6, 0x2d9,
    0x2dc, 0x2dd, 0x2de, 0x2df, 0x2e0, 0x2e1, 0,
};
/* 0x00689B34: the empty string every no-target kind is described by. */

/* Say in words what a spell is aimed at. Each target kind reads its own field,
   which is what makes the two assertions here - on iChar and on iMonsterID -
   name two different fields of one block rather than one field twice.

   A character or a monster whose name the party does not have is described by
   its name-prefix instead, looked up in the table at 0x0061E436 - by sex
   for a character and by name group for a monster, which is what makes the two
   one table. The entry is a message-table offset rather than a string, so it
   is resolved twice. Everything else is a fixed word. Its error
   text names the function. */
// FUNCTION: WIZ8 0x004f97a0
wchar_t* SpellTargetString(const W8TargetSource* source, const W8CombatSlot* target)
{
    unsigned short name_prefix;

    if (target == 0) {
        srAssertFail("pTarget != NULL", MAGIC_CPP, 0xa4, 0);
    }

    switch (target->iType) {
    case 0:
    case 6:
        return &g_wchar_00689b34;

    case 1:
    case 9:
        if (target->iChar == -1) {
            srAssertFail("pTarget->iChar != BAD_INDEX", MAGIC_CPP, 0xad, 0);
        }
        if (!TargetSourceIsCharacter(source, 0) || source->unknown_18[1] != 0 ||
            source->iChar != target->iChar) {
            return FormatWideString(gppStringList[W8_MESSAGE_TARGET_AT / 4],
                                    g_status_685170.buffers.Char[target->iChar].name);
        }
        name_prefix =
            g_name_prefix_messages[g_status_685170.buffers.Char[target->iChar].gender * 4];
        break;

    case 2:
        return gppStringList[W8_MESSAGE_TARGET_PARTY / 4];

    case 3: {
        W8MonsterInfo* monster_info;
        W8MonsterRecord* record;

        if (target->iMonsterID == -1) {
            srAssertFail("pTarget->iMonsterID != BAD_INDEX", MAGIC_CPP, 0xbd, 0);
        }
        monster_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0xc0, MAGIC_CPP, target->iMonsterID, 1));
        record = GetMonsterDataForInfo(monster_info);
        if (!TargetSourceIsMonster(source, 0) || source->iMonsterID != target->iMonsterID) {
            return FormatWideString(gppStringList[W8_MESSAGE_TARGET_AT / 4],
                                    GetMonsterName(monster_info, record, 0));
        }
        name_prefix = g_name_prefix_messages[record->name_group_0cc * 4];
        break;
    }

    case 4:
        return FormatWideString(
            gppStringList[W8_MESSAGE_TARGET_AT / 4],
            GetMonsterGroupName(GetMonsterGroupByListIndex(
                GetMonsterGroupIndexByID(0xcf, MAGIC_CPP, target->iGroupID, 1))));

    case W8_TARGET_KIND_FIVE:
        return gppStringList[W8_MESSAGE_TARGET_PLACE / 4];

    case 7:
        return FormatWideString(gppStringList[W8_MESSAGE_TARGET_ITEM / 4],
                                FormatItemDisplayName(target->pPCItem, 0));

    case W8_TARGET_KIND_EIGHT:
        return gppStringList[W8_MESSAGE_TARGET_DIRECTION / 4];

    default:
        srAssertFail(
            "FALSE", MAGIC_CPP, 0xde,
            FormatString("SpellTargetString: ERROR - Invalid spell target for %d", target->iType));
        return gppStringList[W8_MESSAGE_TARGET_UNKNOWN / 4];
    }

    return FormatWideString(gppStringList[W8_MESSAGE_TARGET_AT / 4], gppStringList[name_prefix]);
}

/* 0x0053BE50 */

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
// FUNCTION: WIZ8 0x004faec0
unsigned int MonsterCastsSpell(W8MonsterInfo* monster_info, int spell_id, unsigned int power_level)
{
    W8MonsterRecord* record = GetMonsterDataForInfo(monster_info);
    W8TargetSource source;
    unsigned int budget;
    unsigned int failure;
    int result;

    SetTargetSourceToMonster(monster_info, &source);

    if (g_settings_6850c8.verbose_combat_messages == 0) {
        ShowNoticef(9, gppStringList[W8_MESSAGE_MONSTER_CAST / 4],
                    GetMonsterName(monster_info, record, 0), g_spell_records[spell_id].display_name,
                    SpellTargetString(&source, &monster_info->Target));
        SetTextBoxMode(1, 9);
    } else {
        ShowNoticef(9, gppStringList[W8_MESSAGE_MONSTER_CAST_VERBOSE / 4],
                    GetMonsterName(monster_info, record, 0), g_spell_records[spell_id].display_name,
                    power_level, SpellTargetString(&source, &monster_info->Target));
    }

    record = GetMonsterDataForInfo(monster_info);
    budget = monster_info->sp_budget_bonus + record->sp_budget;
    if (record->sp_budget == 0) {
        FormatDebugMessage(0, "DATA ERROR: Monster %ls casting spells with SP Budget of 0",
                           GetMonsterName(monster_info, 0, 0));
    }
    if ((int)budget < 0) {
        budget = 0;
    }
    failure = GetSpellFailureChance(budget, spell_id, (int)power_level);

    CastSpellFromSource(spell_id, &source, &monster_info->Target, power_level, 0, failure, 0,
                        &result, 0, 0, 0);
    return SpellCastFatigueCost(spell_id, result);
}

/* The lure spell, and how far under the target the first of its two effects is
   placed. */
enum { W8_SPELL_LURE = 0x26 };

/* Put the lure's two effects into the world. The first is the spell's own
   resource a thousand units below where the spell landed; the second is the
   named companion effect at the landing point itself, and only that one has
   its mode set. Both are appended to whatever the cast hangs its effects off,
   and an effect that failed to spawn is simply not appended. */
// FUNCTION: WIZ8 0x004fb360
void SpawnLureEffects(W8SpellEffectEntry* owner, int argument, const W8CombatSlot* target)
{
    srVector3T<float> position;
    W8SpellVisual* effect;

    position.x = target->point.x;
    position.y = target->point.y - 1000.0f;
    position.z = target->point.z;

    effect =
        SpawnSpellEffect(&position, g_spell_records[W8_SPELL_LURE].resource_name, argument, 0, 0);
    if (effect != 0) {
        effect->auto_release = 0;
        owner->spell_visuals.Add(effect);
    }

    position = target->point;
    effect = SpawnSpellEffect(&position, "hyp_lure2", argument, 0, 0);
    if (effect != 0) {
        effect->auto_release = 0;
        effect->host->pending_behaviour_071 = 3;
        owner->spell_visuals.Add(effect);
    }
}

/* Condition names are the condition-notice table from +5, not a second
   initialized object at 0x0061E57A. */
static const unsigned short* const g_spell_condition_text_0061e57a =
    g_condition_notices_0061E570 + 5;

/* Queued report records use the exhausted-condition notice. */

/* Post what an effect accumulated. The opening separator only appears once
   the text box already has something in it; the total is reported as
   "<count> <unit>" or, for a single hit, "<amount> points", and each nonzero
   condition appends its own affected-target count and condition name. Records still queued on
   the result are then drained: a character-slot record posts the named
   character's notice, a text record formats its own "%s %s" line, and the box
   is reset around each. With nothing reported at all the effect reports that
   its target took no damage. */
// FUNCTION: WIZ8 0x005005c0
void ReportSpellResult005005C0(W8SpellEffectEntry* effect)
{
    const unsigned short* condition_text = g_spell_condition_text_0061e57a;

    if (GetTextBoxMode() != 0) {
        AppendToLastTextLine(effect->reported_124 == 0 ? L" -- " : L", ", -1);
        SetTextBoxMode(1, -1);
    }
    if (effect->result_126.amount != 0) {
        if (effect->result_126.count == 1) {
            AppendToLastTextLine(
                FormatWideString(gppStringList[0x668 / 4], effect->result_126.amount), -1);
        } else {
            AppendToLastTextLine(
                FormatWideString(gppStringList[0x664 / 4], effect->result_126.count,
                                 effect->result_126.amount / effect->result_126.count, -1),
                -1);
            SetTextBoxMode(1, -1);
        }
        SetTextBoxMode(1, -1);
        effect->reported_124 = 1;
    }
    unsigned int* condition_count = &effect->result_126.condition_counts[1];

    do {
        if (*condition_count != 0) {
            if (effect->reported_124 != 0 && GetTextBoxMode() != 0) {
                AppendToLastTextLine(L", ", -1);
                SetTextBoxMode(1, -1);
            }
            if (*condition_count == 1) {
                if (effect->target.iType == W8_TARGET_KIND_CHARACTER) {
                    AppendToLastTextLine(
                        FormatWideString(L"%s %s",
                                         g_status_685170.buffers.Char[effect->target.iChar].name,
                                         gppStringList[condition_text[0]]),
                        -1);
                } else if (effect->target.iType == W8_TARGET_KIND_MONSTER) {
                    W8MonsterInfo* monster_info =
                        MonsterInfoFromID(0x112a, MAGIC_CPP, effect->target.iMonsterID, 1);
                    if (monster_info != 0) {
                        AppendToLastTextLine(FormatWideString(L"%s %s",
                                                              GetMonsterName(monster_info, 0, 0),
                                                              gppStringList[condition_text[0]]),
                                             -1);
                    }
                }
            } else {
                AppendToLastTextLine(
                    FormatWideString(L"%ld %s", *condition_count, gppStringList[condition_text[1]]),
                    -1);
            }
            SetTextBoxMode(1, -1);
            effect->reported_124 = 1;
        }
        condition_text += 4;
        ++condition_count;
    } while (condition_text < g_spell_condition_text_0061e57a + 76);

    while (effect->result_126.reports.GetCount() > 0) {
        W8SpellDamageReport* report = *effect->result_126.reports.GetAt(0);
        effect->result_126.reports.RemoveAt(0);
        if (report != 0) {
            if (report->kind == 1) {
                SetTextBoxMode(0, -1);
                PostCharacterNotice(
                    report->value, L"%s",
                    gppStringList[g_spell_condition_text_0061e57a[W8_CONDITION_EXHAUSTED * 4]]);
                effect->reported_124 = 1;
            } else if (report->kind == 3) {
                SetTextBoxMode(0, -1);
                ShowNoticef(
                    9, L"%s %s", report->text,
                    gppStringList[g_spell_condition_text_0061e57a[W8_CONDITION_EXHAUSTED * 4]]);
                effect->reported_124 = 1;
            }
            free(report);
        }
    }
    if (effect->reported_124 == 0) {
        AppendToLastTextLine(gppStringList[0x694 / 4], -1);
    }
}

// FUNCTION: WIZ8 0x004fac40
bool ValidateSpellTarget004FAC40(int party_slot, int spell_id, unsigned int power, bool item_cast,
                                 bool skip_world_cursor)
{
    W8GrowableVector<int> monsters;
    W8GrowableVector<int> party;
    W8TargetSource source;
    SetTargetSourceToCharacter(party_slot, &source);
    PopulateSpellTargetMarkers(spell_id, power, &source,
                               &g_status_685170.buffers.XChar[party_slot].target_out_of_combat,
                               &monsters, &party, 0);

    bool valid = true;
    bool has_targets = spell_id == 0x1e || monsters.count != 0 || party.count != 0;
    if (!has_targets) {
        W8SpellTargetType target_type = GetSpellTargetType(spell_id, 0);
        has_targets =
            target_type > W8_TARGET_TYPE_ALL_ENEMIES && target_type < W8_TARGET_TYPE_COUNT;
    }
    if (!has_targets) {
        if (!item_cast) {
            PostCharacterNotice(
                party_slot,
                FormatWideString(gppStringList[0x1b7], g_spell_records[spell_id].display_name));
        } else {
            PostCharacterNotice(party_slot, FormatWideString(gppStringList[0x1b8]));
        }
        valid = false;
    } else if (!skip_world_cursor && DispatchWorldCursorNodeCommand004D9080(0, 4, 1)) {
        valid = false;
    }

    if (spell_id == 0x4b &&
        ((g_level_data_00652dac->flags & 1) != 0 || !GetLevelDataFlag4() || GetLevelDataFlag9())) {
        valid = false;
    }
    if (!valid && (!gXStatus.fCombatMode || !MonsterCanAimSpell005474B0(spell_id) ||
                   gXStatus.hostile_monster_count != 0 || !g_combat_state->flag_a54)) {
        QueueCharacterEvent(&g_status_685170.buffers.Char[party_slot],
                            g_character_event_kind_005ee65c, 0, g_effect_argument_005ed8c8,
                            g_effect_argument_005ed914);
    }
    return valid;
}

/* The spells whose casts share one three-minute cooldown slot each; the
   shared slot lives in the status block's clock array. */
// GLOBAL: WIZ8 0x00616E34
const int g_cooldown_gated_spells_00616e34[14] = {30, 38, 75, 73, 32, 33, 17,
                                                  20, 8,  40, 26, 45, 64, 58};

/* Whether the spell's current target would actually be affected by it - the
   per-spell rules the cast path checks before it spends the points. */
// FUNCTION: WIZ8 0x004F9AE0
unsigned char SpellAffectedTarget004F9AE0(W8Character* character, int spell_id, W8CombatSlot* aim,
                                          unsigned int power)
{
    int duration;
    int index;
    int slot;
    const int* table;
    const unsigned int* conditions;
    const W8Enchantment* enchantments;
    W8MonsterInfo* monster_info;
    const W8Character* walker;
    const unsigned int* stats;
    const W8PartySlotRow* row;
    float share;
    bool affected;

    duration = g_spell_records[spell_id].duration_044 * power +
               g_spell_records[spell_id].duration_per_level_04d;
    affected = true;
    if (duration != 9999) {
        duration += 1;
    }
    switch (spell_id) {
    case 0x2:
    case 0x35:
    case 0x3b:
    case 0x3e:
        if (gXStatus.fCombatMode != 0 && gXStatus.hostile_monster_count != 0) {
            index = 0;
            table = g_combat_effect_slot_spells_and_cast_success_00616dd8;
            while (spell_id != *table) {
                ++table;
                ++index;
                if (index > 8) {
                    return true;
                }
            }
            share =
                g_combat_state->effect_slots_85a[index].duration_0d / static_cast<float>(duration);
            goto LAB_004f9bc2;
        }
        goto LAB_004fa3f4;
    case 0x6:
        if (aim->iType == W8_TARGET_KIND_CHARACTER) {
            if (g_status_685170.buffers.Char[aim->iChar].hp_current ==
                static_cast<unsigned int>(g_status_685170.buffers.Char[aim->iChar].uiHPMax)) {
                return false;
            }
        } else {
            monster_info = MonsterInfoFromID(0x1d2, MAGIC_CPP, aim->iMonsterID, '\x01');
            if (monster_info->hp_current == static_cast<unsigned int>(monster_info->hp_max)) {
                return false;
            }
        }
        break;
    case 0x8:
    case 0x11:
    case 0x21:
    case 0x2d:
    case 0x40:
    case 0x49:
    case 0x4b:
        goto switchD_004f9b2d_caseD_8;
    case 0xd:
        if (gXStatus.fCombatMode != 0) {
            if (aim->iType != W8_TARGET_KIND_CHARACTER) {
                monster_info = MonsterInfoFromID(0x1fc, MAGIC_CPP, aim->iMonsterID, '\x01');
                if (monster_info->stamina != monster_info->stamina_max) {
                    return true;
                }
                return false;
            }
            if (g_status_685170.buffers.Char[aim->iChar].stamina !=
                g_status_685170.buffers.Char[aim->iChar].uiStaminaMax) {
                return true;
            }
            return false;
        }
        goto LAB_004fa3f4;
    case 0x10:
        if (aim->iType == W8_TARGET_KIND_CHARACTER) {
            conditions = g_status_685170.buffers.Char[aim->iChar].uiCondition;
        } else {
            monster_info = MonsterInfoFromID(0x2b6, MAGIC_CPP, aim->iMonsterID, '\x01');
            conditions = monster_info->condition_turns;
        }
        if (conditions[3] == 0 && conditions[4] == 0 && conditions[6] == 0 &&
            conditions[0xf] == 0 && conditions[0xc] == 0) {
            return false;
        }
        break;
    case 0x13:
    case 0x15:
    case 0x1b:
    case 0x36:
    case 0x3d:
    case 0x41:
        if (gXStatus.fCombatMode != 0 && gXStatus.hostile_monster_count != 0) {
            if (aim->iType == W8_TARGET_KIND_CHARACTER) {
                enchantments = g_status_685170.buffers.Char[aim->iChar].enchantments;
            } else {
                monster_info = MonsterInfoFromID(0x18a, MAGIC_CPP, aim->iMonsterID, '\x01');
                enchantments = monster_info->enchantments;
            }
            index = GetConditionDisplaySlot(spell_id);
            share = enchantments[index].value_08 / static_cast<float>(duration);
            goto LAB_004f9bc2;
        }
        goto LAB_004fa3f4;
    case 0x14:
    case 0x1a:
    case 0x20:
    case 0x28:
        if (gXStatus.fCombatMode != 0) {
            index = 0;
            table = g_being_effect_slot_spells_00616d84;
            while (spell_id != *table) {
                ++table;
                ++index;
                if (index > 11) {
                    return true;
                }
            }
            share =
                g_status_685170.effect_slots_17af[index].duration_0d / static_cast<float>(duration);
            goto LAB_004f9bc2;
        }
        affected = true;
        index = 0;
        table = g_cooldown_gated_spells_00616e34;
        while (*table != spell_id) {
            ++table;
            ++index;
            if (index > 13) {
                return true;
            }
        }
        // reinterpret-ok: one dword clock slot inside the status block
        if (ClockIsTicking(gXStatus.spell_cooldown_clocks[index]) == 0) {
            goto LAB_004fa307;
        }
        goto LAB_004fa305;
    case 0x17:
        if (aim->pPCItem->identified != '\0') {
            return false;
        }
        break;
    case 0x1e:
    case 0x26:
        if (g_combat_state != 0 && gXStatus.hostile_monster_count != 0) {
            return true;
        }
        goto switchD_004f9b2d_caseD_8;
    case 0x22:
        if (aim->iType == W8_TARGET_KIND_CHARACTER) {
            conditions = g_status_685170.buffers.Char[aim->iChar].uiCondition;
        } else {
            monster_info = MonsterInfoFromID(0x250, MAGIC_CPP, aim->iMonsterID, '\x01');
            conditions = monster_info->condition_turns;
        }
        if (conditions[0x10] == 0) {
            return false;
        }
        break;
    case 0x23:
        if (aim->iType == W8_TARGET_KIND_CHARACTER) {
            conditions = g_status_685170.buffers.Char[aim->iChar].uiCondition;
        } else {
            monster_info = MonsterInfoFromID(0x261, MAGIC_CPP, aim->iMonsterID, '\x01');
            conditions = monster_info->condition_turns;
        }
        if (conditions[7] == 0) {
            return false;
        }
        break;
    case 0x2c:
        affected = false;
        if (gXStatus.fCombatMode != 0) {
            index = 0;
            walker = g_status_685170.buffers.Char;
            row = g_status_685170.buffers.XChar;
            while (!row->fOccupied || 0x12 < walker->highest_condition ||
                   walker->uiStaminaMax <= walker->stamina) {
                ++index;
                ++row;
                ++walker;
                if (7 < index) {
                    return false;
                }
            }
            return true;
        }
        break;
    case 0x33:
        if (aim->iType == W8_TARGET_KIND_CHARACTER) {
            conditions = g_status_685170.buffers.Char[aim->iChar].uiCondition;
        } else {
            monster_info = MonsterInfoFromID(0x272, MAGIC_CPP, aim->iMonsterID, '\x01');
            conditions = monster_info->condition_turns;
        }
        if (conditions[2] == 0) {
            return false;
        }
        break;
    case 0x38:
        if (gXStatus.fCombatMode != 0 && gXStatus.hostile_monster_count != 0) {
            if (aim->iType != W8_TARGET_KIND_PARTY) {
                return true;
            }
            affected = false;
            index = 8;
            row = g_status_685170.buffers.XChar;
            do {
                if (row->fOccupied &&
                    g_status_685170.buffers.Char[8 - index].enchantments[3].value_08 /
                            static_cast<float>(duration) <=
                        g_navigator_vertical_phase_step_005ebcc8) {
                    affected = true;
                }
                ++row;
                --index;
            } while (index != 0);
            return affected;
        }
        goto LAB_004fa3f4;
    case 0x3a:
        if (gXStatus.fCombatMode == 0) {
            affected = true;
            index = 0;
            table = g_cooldown_gated_spells_00616e34;
            do {
                if (*table == spell_id) {
                    // reinterpret-ok: one dword clock slot inside the status block
                    affected = ClockIsTicking(gXStatus.spell_cooldown_clocks[index]) == 0;
                    gXStatus.spell_cooldown_clocks[index] = SetCountdownClock(180000);
                    break;
                }
                ++table;
                ++index;
            } while (table < g_cooldown_gated_spells_00616e34 + 14);
            if (affected == false) {
                return false;
            }
        }
        if (aim->iType == W8_TARGET_KIND_CHARACTER) {
            conditions = g_status_685170.buffers.Char[aim->iChar].uiCondition;
        } else {
            monster_info = MonsterInfoFromID(0x2f9, MAGIC_CPP, aim->iMonsterID, '\x01');
            conditions = monster_info->condition_turns;
        }
        if (conditions[9] != 0) {
            return affected;
        }
        if (aim->iType == W8_TARGET_KIND_CHARACTER &&
            GetEquipmentBindingDifficulty(aim->iChar) != 0) {
            return affected;
        }
        goto LAB_004fa3f4;
    case 0x44:
        index = 0;
        while (g_status_685170.buffers.XChar[index].fOccupied == 0 ||
               0x12 < g_status_685170.buffers.Char[index].highest_condition ||
               g_status_685170.buffers.Char[index].uiHPMax <=
                   static_cast<int>(g_status_685170.buffers.Char[index].hp_current)) {
            ++index;
            if (7 < index) {
                return false;
            }
        }
        return true;
    case 0x48:
        affected = false;
        if (g_combat_state != 0) {
            index = 0;
            do {
                if (g_combat_state->effect_slots[index].active != 0) {
                    return true;
                }
                ++index;
            } while (index < 9);
            return false;
        }
        break;
    case 0x4a:
        if (aim->iType == W8_TARGET_KIND_CHARACTER) {
            conditions = g_status_685170.buffers.Char[aim->iChar].uiCondition;
        } else {
            monster_info = MonsterInfoFromID(0x283, MAGIC_CPP, aim->iMonsterID, '\x01');
            conditions = monster_info->condition_turns;
        }
        if (conditions[0xb] == 0 && conditions[0xd] == 0) {
            return false;
        }
        break;
    case 0x58:
        if (aim->iType == W8_TARGET_KIND_CHARACTER_INDIRECT) {
            conditions = g_status_685170.buffers.Char[aim->iChar].uiCondition;
        } else {
            monster_info = MonsterInfoFromID(0x2a5, MAGIC_CPP, aim->iMonsterID, '\x01');
            conditions = monster_info->condition_turns;
        }
        if (conditions[0x12] == 0) {
            return false;
        }
        break;
    case 0x64:
        if (aim->iType == W8_TARGET_KIND_CHARACTER) {
            slot = aim->iChar;
            index = 1;
            conditions = g_status_685170.buffers.Char[slot].uiCondition + 1;
            while (*conditions == 0) {
                ++index;
                ++conditions;
                if (0x11 < index) {
                    return g_status_685170.buffers.Char[slot].stamina <
                               g_status_685170.buffers.Char[slot].uiStaminaMax ||
                           g_status_685170.buffers.Char[slot].hp_current <
                               static_cast<unsigned int>(
                                   g_status_685170.buffers.Char[slot].uiHPMax);
                }
            }
        } else {
            monster_info = MonsterInfoFromID(0x232, MAGIC_CPP, aim->iMonsterID, '\x01');
            index = 1;
            stats = monster_info->condition_turns;
            while (stats = stats + 1, *stats == 0) {
                ++index;
                if (0x11 < index) {
                    return static_cast<unsigned int>(monster_info->stamina) <
                               static_cast<unsigned int>(monster_info->stamina_max) ||
                           monster_info->hp_current <
                               static_cast<unsigned int>(monster_info->hp_max);
                }
            }
        }
        return true;
    case 0x78:
        if (aim->iType == W8_TARGET_KIND_CHARACTER) {
            conditions = g_status_685170.buffers.Char[aim->iChar].uiCondition;
        } else {
            monster_info = MonsterInfoFromID(0x294, MAGIC_CPP, aim->iMonsterID, '\x01');
            conditions = monster_info->condition_turns;
        }
        if (conditions[1] == 0) {
            return false;
        }
    LAB_004fa3f4:
        affected = false;
    }
    return affected;
LAB_004f9bc2:
    if (share <= g_navigator_vertical_phase_step_005ebcc8) {
        return true;
    }
    return false;
switchD_004f9b2d_caseD_8:
    affected = true;
    index = 0;
    table = g_cooldown_gated_spells_00616e34;
    while (*table != spell_id) {
        ++table;
        ++index;
        if (index > 13) {
            return true;
        }
    }
    // reinterpret-ok: one dword clock slot inside the status block
    if (ClockIsTicking(gXStatus.spell_cooldown_clocks[index]) != 0) {
    LAB_004fa305:
        affected = false;
    }
LAB_004fa307:
    gXStatus.spell_cooldown_clocks[index] = SetCountdownClock(180000);
    return affected;
}

/* Charge the realm spell points one cast costs and run the cast itself:
   notices, the success-chance figure, the point spend and the skill practice
   all happen here. Answers the cast step's status; out_points gets the
   fatigue cost. */
// FUNCTION: WIZ8 0x004FA4D0
int ExecuteCharacterSpellCast(int party_slot, int spell_id, unsigned int power_level,
                              int* out_points, char continue_cast)
{
    W8Character* character;
    const W8SpellRuntimeRecord* record;
    W8TargetSource source;
    W8CombatSlot* aim;
    unsigned int sp_left;
    unsigned int chance;
    unsigned int best_skill;
    unsigned int skill_score;
    int realm_skill;
    int slot;
    int realm;
    int level_index;
    int caster_level;
    int minimum_level;
    int identify_context;
    int spellbook;
    int profession;
    int index;
    unsigned int threshold;
    unsigned int morale;
    unsigned char affected;
    int result;
    int cast_result;
    unsigned int quiet;
    const int* profession_level;
    char flag_byte;

    character = &g_status_685170.buffers.Char[party_slot];
    record = &g_spell_records[spell_id];
    realm = record->realm;
    sp_left = record->spell_point_cost;
    *out_points = 0;
    flag_byte = continue_cast;
    switch (record->field_12b) {
    case 1:
        if (gXStatus.fCombatMode == '\0' && power_level == 8) {
            quiet = 1;
            goto LAB_004fa575;
        }
    case 0:
        quiet = 0;
    LAB_004fa5d7:
        threshold = character->iSPLeft[realm];
        if (sp_left * power_level - threshold != 0 &&
            static_cast<int>(threshold) <= static_cast<int>(sp_left * power_level)) {
            power_level = threshold / sp_left;
        }
        break;
    case 2:
        quiet = 0;
        flag_byte = power_level != 8;
    default:
        if (flag_byte != '\0') {
            goto LAB_004fa5d7;
        }
    LAB_004fa575:
        if (character->iSPLeft[realm] < static_cast<int>(sp_left)) {
            goto LAB_004fa58d;
        }
        break;
    case 3:
        power_level = 1;
        quiet = 0;
        goto LAB_004fa575;
    }
    if (power_level == 0) {
    LAB_004fa58d:
        PostCharacterNotice(party_slot, gppStringList[0x18a], record->display_name);
        return 0;
    }
    if (ValidateSpellTarget004FAC40(party_slot, spell_id, power_level, false, false) == '\0') {
        return 0;
    }
    if (character->uiCondition[8] != 0 &&
        (record->alchemy_spell == '\0' || character->skills[0x1a].level == 0)) {
        return 0;
    }
    SetTargetSourceToCharacter(party_slot, &source);
    if (character->skills[0x23].flag_00 == '\0') {
        identify_context = 0;
    } else {
        identify_context = (character->skills[0x23].level >> 2) + 1;
    }
    if (power_level == 8) {
        power_level = ChooseSpellPowerLevelForTarget(party_slot, spell_id, identify_context);
        if (power_level == 0) {
            return 0;
        }
        threshold = character->iSPLeft[realm];
        if (sp_left * power_level - threshold != 0 &&
            static_cast<int>(threshold) <= static_cast<int>(sp_left * power_level)) {
            power_level = threshold / sp_left;
        }
    }
    aim = &g_status_685170.buffers.XChar[party_slot].target_out_of_combat;
    if (g_settings_6850c8.verbose_combat_messages == '\0') {
        if (continue_cast == '\0') {
            PostCharacterNotice(party_slot, gppStringList[0x18c], record->display_name,
                                SpellTargetString(&source, aim));
            SetTextBoxMode('\x01', 8);
        } else {
            PostCharacterNotice(party_slot, gppStringList[0x18d]);
            SetTextBoxMode('\x01', 8);
        }
    } else {
        PostCharacterNotice(party_slot, gppStringList[0x18b], record->display_name, power_level,
                            SpellTargetString(&source, aim));
    }
    best_skill = GetBestSpellbookSkillForSpell(character, spell_id, '\x01', '\x01', power_level);
    realm_skill = realm + 0x1c;
    slot = CharacterPointerToPartySlot(character);
    level_index = record->spell_point_cost / 2 + record->spell_level;
    skill_score =
        (character->skills[best_skill].level + character->skills[realm + 0x1c].level * 4) / 5;
    if (0x10 < level_index) {
        level_index = 0x10;
    }
    threshold =
        (g_combat_effect_slot_spells_and_cast_success_00616dd8[level_index + 6] * power_level) / 7;
    if (skill_score < threshold) {
        chance = ((threshold - skill_score) * 70) / threshold;
        if (static_cast<int>(chance) < 0) {
            chance = 0;
        } else if (100 < static_cast<int>(chance)) {
            chance = 100;
        }
    } else {
        chance = 0;
    }
    minimum_level = GetMinimumCasterLevelForSpell(spell_id);
    caster_level = GetProfessionCasterLevel(character, -1);
    spellbook = (-(record->psionics_spell != '\0') & 8U) |
                (-(record->divinity_spell != '\0') & 2U) | (record->wizardry_spell != '\0') |
                (-(record->alchemy_spell != '\0') & 4U);
    profession_level = character->profession_levels;
    profession = W8_PROFESSION_FIGHTER;
    do {
        if (*profession_level != 0 && profession != character->iProfession &&
            (g_profession_spellbooks[profession] & spellbook) != 0 &&
            (index = GetProfessionCasterLevel(character, profession), 0 < index)) {
            caster_level += index;
        }
        ++profession;
        ++profession_level;
    } while (profession < W8_PROFESSION_COUNT);
    index = (minimum_level - caster_level) - 1 + power_level;
    if (0 < index) {
        chance += record->spell_level * index;
    }
    if (gXStatus.fCombatMode != '\0') {
        if (g_settings_6850c8.difficulty == 0) {
            morale = 0x50;
        } else if (g_settings_6850c8.difficulty == 1) {
            morale = 0x3c;
        } else {
            if (g_settings_6850c8.difficulty != 2) {
                srAssertFail("FALSE", MAGIC_CPP, 0x14e8, 0);
                goto LAB_004faa0f;
            }
            morale = 0x28;
        }
        threshold = g_combat_state->characters[slot].phase_clock_stamp;
        if (morale <= threshold) {
            chance = (((0x32 - morale) + threshold) * chance * 2) / 100;
        }
    }
LAB_004faa0f:
    if (spell_id == 0x4a && aim->iType == W8_TARGET_KIND_CHARACTER && aim->iChar == party_slot) {
        chance += 0x32;
    }
    index = 0;
    do {
        if (index != 0 && character->spell_learned[index] == 1 &&
            g_spell_records[index].spell_point_cost <=
                character->iSPLeft[g_spell_records[index].realm] &&
            SpellUsableNow(index, '\0')) {
            ++g_status_685170.tail_3121.spell_usage.records[index].usable_cast_count;
        }
        ++index;
    } while (index < 0x72);
    ++g_status_685170.tail_3121.spell_usage.records[spell_id].cast_count;
    affected = SpellAffectedTarget004F9AE0(character, spell_id, aim, power_level);
    if (continue_cast == '\0') {
        if (Random(2) != 0) {
            index = g_special_event_0068c528;
        } else {
            index = g_special_event_0068c524;
        }
        QueueCharacterEvent(character, index, 0, g_effect_argument_005ed8c8,
                            g_effect_argument_005ed914);
    }
    result = CastSpellFromSource(spell_id, &source, aim, power_level, identify_context, chance,
                                 quiet, &cast_result, 0, 0, 0);
    if (cast_result != 0) {
        SpendCharacterSpellPoints(party_slot, realm, cast_result * sp_left);
        if (affected != '\0') {
            index = record->spell_level + cast_result;
            if (index < 2) {
                srAssertFail("uiUsagePoints >= 2", MAGIC_CPP, 0x3d5, 0);
            }
            PracticeCharacterSkill(character, best_skill, (index + 2) >> 2, '\0');
            PracticeCharacterSkill(character, realm_skill, index, '\0');
            if (character->skills[0x23].flag_00 != '\0') {
                PracticeCharacterSkill(character, 0x23, (index + 2) >> 2, '\0');
            }
        }
    }
    *out_points = SpellCastFatigueCost(spell_id, cast_result);
    return result;
}

/* Resolve one cast into a queued spell effect: roll the fizzle and backfire
   chances, collect the target markers, spawn the visuals or fire the
   missiles that carry the effect, and either queue it for later or apply it
   immediately. The last two vectors let callers supply their own target
   lists instead of the aimed collection. Returns 0 when the effect could
   not be allocated, 1 on a normal cast, 2 when a fizzle consumed the cast
   and 3 when it backfired. */
// FUNCTION: WIZ8 0x004FB4C0
int CastSpellFromSource(int spell_id, W8TargetSource* source, W8CombatSlot* target,
                        unsigned int power_level, int a, int b, int c, int* d, int e,
                        W8GrowableVector<int>* f, W8GrowableVector<int>* g)
{
    srVector3T<float> point;
    bool fizzled;
    bool quiet;
    bool forced;
    unsigned char icon_flag;
    W8SpellEffectEntry* owner;
    W8MonsterRecord* record_data;
    W8Missile* missile;
    W8Monster* monster;
    W8SpellVisual* visual;
    W8MonsterInfo* monster_info;
    int result;
    unsigned int chance;
    unsigned int floor_power;
    unsigned int roll;
    int local_a0;
    int index;
    int target_type;
    int missile_index;
    W8GrowableVector<int> local_d8;
    W8GrowableVector<int> local_c8;
    W8SpellEffectDefinition block;
    W8CombatSlot point_target;
    srVector3T<float> local_b8;
    srVector3T<float> local_ac;
    srVector3T<float> local_9c;
    srMatrix3T<float> local_90;
    double yaw;
    double pitch;
    float heading;
    unsigned int minimum;
    unsigned int caster_figure;
    unsigned int monster_index;
    unsigned int adjusted_power;

    fizzled = false;
    quiet = e == 4;
    forced = false;
    if (d != 0) {
        *d = 0;
    }
    owner = new W8SpellEffectEntry;
    if (owner == 0) {
        return 0;
    }
    owner->Source = *source;
    memcpy(owner->unknown_03c, target, sizeof(owner->unknown_03c));
    if (TargetSourceIsCharacter(source, 0)) {
        local_a0 = source->iChar;
    } else {
        local_a0 = -1;
    }
    source->fBackfire = '\0';
    if (g_flag_00689b68 != '\0' && spell_id != 0x76) {
        forced = true;
        b = 100;
    }
    if (g_spell_records[spell_id].realm == 0 && g_camera_sway_active_652da4 != false) {
        b = 100;
    }
    if (quiet && g_flag_00689b68 == '\0') {
        b = static_cast<unsigned int>(b) >> 1;
    }
    CombatLog("Chance of FAILURE: %d", b);
    CombatLog("");
    if (b != 0 && (roll = Random(100), roll < static_cast<unsigned int>(b))) {
        if (g_spell_records[spell_id].realm == 0 && g_camera_sway_active_652da4 != false) {
            b = 0;
        } else if (!quiet && !forced) {
            if (static_cast<unsigned int>(b) < 6) {
                b = 0;
            } else {
                b = static_cast<unsigned int>(b) / 3;
            }
        }
        if (!CanSpellBackfire(spell_id)) {
            b = 0;
        }
        CombatLog("Chance of BACKFIRE: %d (roll %d)", b, roll);
        CombatLog("");
        if (static_cast<unsigned int>(b) <= roll) {
            fizzled = true;
            SoundPlay("Data\\Sound\\Misc\\Spell Fizzle 01.", 0);
            if (local_a0 != -1 && (roll = Random(100), roll < 0x46)) {
                QueueCharacterEvent(&g_status_685170.buffers.Char[local_a0],
                                    g_special_event_0068c558, 0, g_effect_argument_005ed8c8,
                                    g_effect_argument_005ed914);
            }
            if (e == 2) {
                if (source->iType != W8_TARGET_SOURCE_CHARACTER) {
                    srAssertFail("pSource->iType == SOURCE_TYPE_CHARACTER", MAGIC_CPP, 0x8c2, 0);
                }
                index = 0x1aa;
                goto LAB_004fcdb3;
            }
            if (e == 3) {
                goto LAB_004fcdb3;
            }
            if (g_settings_6850c8.verbose_combat_messages != '\0') {
                ShowNotice(0xc, FormatWideString(gppStringList[0x196]), (short)0xffff, -1);
            } else {
                AppendToLastTextLine(FormatWideString(L" -- %s", gppStringList[0x196]), -1);
            }
            goto LAB_004fce23;
        }
        PrepareSpellTarget004FEA50(spell_id, source, target);
        source->fBackfire = '\x01';
        c = 0;
        SoundPlay("Data\\Sound\\Misc\\Spell Backfire.wav", 0);
    }
    if (source->fBackfire == '\0' && source->unknown_18[0] == '\0' && source->fReflection == '\0' &&
        g_spell_records[spell_id].realm != 4 && spell_id != 0x83) {
        CheckSpellBackfire004FF220(spell_id, source, target);
    }
    if (f == 0) {
        if (g == 0) {
            PopulateSpellTargetMarkers(spell_id, power_level, source, target, &local_d8, &local_c8,
                                       0);
            if (spell_id == 0x3c) {
                local_b8.y = target->point.y - g_float_005ebc64;
                target->point.y = local_b8.y;
                local_b8.x = target->point.x;
                local_b8.z = target->point.z;
                local_9c = local_b8;
                if ((TargetSourceIsCharacter(source, 0) && source->fBackfire == '\0') ||
                    (!TargetSourceIsCharacter(source, 0) && source->fBackfire != '\0')) {
                    heading = HeadingTowardNearestMonster(local_b8, 2, 0);
                } else {
                    heading = HeadingTowardNearestMonster(local_b8, 1, 0);
                }
                if (g_octree_6598a4->FindNavigatorPosition(
                        &local_b8, heading,
                        static_cast<float>(
                            g_world_cursor_extent_table_00616eb0
                                [g_spell_power_extent_index_00616f41[power_level - 1] * 6 + 3]) +
                            g_world_scale_005ebc40,
                        1, &local_ac, 1, 0, 0, 5, 1) != 0) {
                    target->point = local_ac;
                }
            }
        } else {
            local_d8 = *g;
        }
    } else {
        local_c8 = *f;
        if (g != 0) {
            local_d8 = *g;
        }
    }
    ClearAttackBlock(&block);
    if (TargetSourceIsCharacter(source, 1)) {
        if (source->unknown_18[1] == '\0') {
            caster_figure =
                GetTotalCasterLevel(&g_status_685170.buffers.Char[source->iChar], 0,
                                    (g_spell_records[spell_id].psionics_spell != '\0' ? 8 : 0) |
                                        (g_spell_records[spell_id].divinity_spell != '\0' ? 2 : 0) |
                                        (g_spell_records[spell_id].wizardry_spell != '\0' ? 1 : 0) |
                                        (g_spell_records[spell_id].alchemy_spell != '\0' ? 4 : 0),
                                    1);
            caster_figure = GetSpellDifficulty(caster_figure, spell_id, power_level);
        } else {
            caster_figure = source->unknown_1f[0];
        }
    } else if (TargetSourceIsMonster(source, 1)) {
        monster_index = MonsterGetIndexByLocationID(0x6ad, MAGIC_CPP, source->iMonsterID, 1);
        monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
        record_data = GetMonsterDataForInfo(monster_info);
        caster_figure = GetSpellDifficulty(record_data->effective_level_24f, spell_id, power_level);
    } else {
        if (source->iType != W8_TARGET_SOURCE_INDIRECT) {
            srAssertFail("pSource->iType == SOURCE_TYPE_3D", MAGIC_CPP, 0x6b2, 0);
        }
        caster_figure = (g_spell_records[spell_id].spell_point_cost / 2 +
                         g_spell_records[spell_id].spell_level) /
                            2 +
                        power_level;
        if (GetSpellTargetType(spell_id, 0) == 7) {
            caster_figure -= 3;
        }
        if (static_cast<int>(caster_figure) < 0) {
            caster_figure = 0;
        }
    }
    switch (spell_id) {
    case 0x46:
        floor_power = 2;
        break;
    case 0x57:
        floor_power = 4;
        break;
    case 0x5a:
    case 0x5d:
    case 0x5e:
        floor_power = 6;
        break;
    default:
        floor_power = 0;
        break;
    }
    if (caster_figure < floor_power) {
        block.power_level = 0;
    } else {
        block.power_level = caster_figure - floor_power;
    }
    adjusted_power = block.power_level;
    AdjustIntegerByPercent(&adjusted_power, static_cast<unsigned int>(a) >> 1);
    block.power_level = adjusted_power;
    if (IsCombatEffectSlotSpell(spell_id)) {
        SetDice(&block.magnitude, 0, 0, 0);
    } else {
        block.magnitude = g_spell_records[spell_id].effect_dice;
        block.magnitude.count = static_cast<char>(power_level) * block.magnitude.count;
    }
    block.duration_scale = power_level;
    block.percent = a;
    block.duration_base = g_spell_records[spell_id].duration_per_level_04d;
    block.duration_per_power = g_spell_records[spell_id].duration_044;
    if (g_spell_records[spell_id].needs_aim_13f == '\0') {
        target_type = GetSpellTargetType(spell_id, 0);
        if (spell_id == 0x31 || target_type < 0xb) {
            switch (target_type) {
            default:
                if (spell_id == 0x4b || spell_id == 0x49) {
                    visual = SpawnCameraSpellEffect(g_spell_records[spell_id].resource_name,
                                                    power_level, 0, 0);
                    goto LAB_004fc61a;
                }
                for (index = 0; index < local_d8.GetCount(); ++index) {
                    monster = GetMonsterByLocationID(*local_d8.GetAt(index));
                    visual = CreateMonsterSpellEffect(g_spell_records[spell_id].resource_name,
                                                      power_level, monster, 0, 0);
                    if (visual != 0) {
                        visual->auto_release = '\0';
                        owner->spell_visuals.Add(visual);
                    }
                }
                if (local_c8.GetCount() != 0) {
                    for (index = 0; index < local_c8.GetCount(); ++index) {
                        if (MonsterCanAimSpell005474B0(spell_id)) {
                            icon_flag = source->fBackfire != '\0';
                        } else {
                            icon_flag = source->fBackfire == '\0';
                        }
                        StageMonsterCastIcon0059AF40(*local_c8.GetAt(index),
                                                     g_spell_records[spell_id].realm, icon_flag,
                                                     spell_id);
                    }
                }
                break;
            case 2:
            case 4:
            case 7:
                if (spell_id != 0x5f) {
                    if (spell_id == 0x62) {
                        point.y = target->point.y - g_float_005ebc64;
                        target->point.y = point.y;
                        point.x = target->point.x;
                        point.z = target->point.z;
                        visual = SpawnSpellEffect(&point, g_spell_records[0x62].resource_name,
                                                  power_level, 0, 0);
                        goto LAB_004fc61a;
                    }
                    for (index = 0; index < local_d8.GetCount(); ++index) {
                        monster = GetMonsterByLocationID(*local_d8.GetAt(index));
                        visual = CreateMonsterSpellEffect(g_spell_records[spell_id].resource_name,
                                                          power_level, monster, 0, 0);
                        if (visual != 0) {
                            visual->auto_release = '\0';
                            owner->spell_visuals.Add(visual);
                        }
                    }
                    if (local_c8.GetCount() != 0) {
                        visual = SpawnCameraSpellEffect(g_spell_records[spell_id].resource_name,
                                                        power_level, 0, 0);
                        goto LAB_004fc61a;
                    }
                }
                break;
            case 5:
                if (TargetSourceIsCharacter(source, 0)) {
                    visual = CreateAttachedSpellEffect(g_spell_records[spell_id].resource_name,
                                                       power_level, (W8Monster*)0x0, 0, 0);
                    if (visual != 0) {
                        visual->auto_release = '\0';
                        visual->fixed_transform = '\x01';
                        owner->spell_visuals.Add(visual);
                    }
                } else {
                    if (TargetSourceIsMonster(source, 0)) {
                        visual = CreateAttachedSpellEffect(
                            g_spell_records[spell_id].resource_name, power_level,
                            GetMonsterByLocationID(source->iMonsterID), 0, 0);
                    } else {
                        local_9c = source->point;
                        local_b8.x = target->point.x - local_9c.x;
                        local_b8.y = target->point.y - local_9c.y;
                        local_b8.z = target->point.z - local_9c.z;
                        local_ac = local_b8;
                        local_b8.SetLength(1.0);
                        local_90.vectors[0] = *local_ac.Set(1.0, 0.0, 0.0);
                        local_90.vectors[1] = *local_ac.Set(0.0, 1.0, 0.0);
                        local_90.vectors[2] = *local_ac.Set(0.0, 0.0, 1.0);
                        yaw = atan2(local_b8.x, local_b8.z);
                        local_90.RotateAboutY(yaw);
                        local_b8 = local_90.Transform(local_b8);
                        pitch = -atan2(local_b8.y, local_b8.z);
                        local_90.RotateAboutX(pitch);
                        visual = CreateAimedSpellEffect(g_spell_records[spell_id].resource_name,
                                                        power_level, &local_9c, &local_90, 0, 0);
                    }
                    if (visual != 0) {
                        visual->auto_release = '\0';
                        owner->spell_visuals.Add(visual);
                    }
                }
                break;
            case 6:
                if (spell_id == 0x76) {
                    if (local_c8.GetCount() == 0) {
                        point.y = target->point.y - g_float_005ebc64;
                        target->point.y = point.y;
                        point.x = target->point.x;
                        point.z = target->point.z;
                        visual =
                            SpawnSpellEffect(&point, g_spell_records[0x76].resource_name, 1, 0, 0);
                    } else {
                        GetCameraForwardPoint00421150(1.0, &local_ac);
                        local_ac.y = local_ac.y - g_default_world_height_00603ac8;
                        visual = SpawnSpellEffect(&local_ac, g_spell_records[0x76].resource_name, 2,
                                                  0, 0);
                    }
                } else {
                    point.y = target->point.y - g_float_005ebc64;
                    target->point.y = point.y;
                    point.x = target->point.x;
                    point.z = target->point.z;
                    visual = SpawnSpellEffect(&point, g_spell_records[spell_id].resource_name,
                                              power_level, 0, 0);
                }
                goto LAB_004fc61a;
            case 8:
                if (spell_id != 0x26) {
                    if (spell_id != 0x3c) {
                        target->point.y = target->point.y - g_float_005ebc64;
                    }
                    point = target->point;
                    visual = SpawnSpellEffect(&point, g_spell_records[spell_id].resource_name,
                                              power_level, 0, 0);
                    goto LAB_004fc61a;
                }
                SpawnLureEffects(owner, power_level, target);
                break;
            case 9:
                break;
            case 10:
                visual = SpawnCameraSpellEffect(g_spell_records[spell_id].resource_name,
                                                power_level, 0, 0);
            LAB_004fc61a:
                if (visual != 0) {
                    visual->auto_release = '\0';
                    owner->spell_visuals.Add(visual);
                }
                break;
            }
        }
    } else {
        missile_index = g_spell_records[spell_id].missile_index_140;
        if (GetSpellTargetType(spell_id, 0) == 6) {
            ResetCombatSlot(&point_target);
            point_target.point.x = target->point.x;
            point_target.point.z = target->point.z;
            point_target.point.y =
                g_default_world_height_00603ac8 * g_float_005ebc7c + target->point.y;
            point_target.iType = W8_TARGET_KIND_PLACE;
            missile = FireMissileSourceToTarget(missile_index, source, &point_target, &block, 1,
                                                0xffffffff, 9999);
            if (missile != 0) {
                owner->missiles.Add(missile);
            }
            owner->flag_122 = '\x01';
        } else {
            memcpy(block.condition_chances,
                   g_missile_table_65bde0[missile_index].condition_chances_155, 0x10);
            block.value_1c = g_missile_table_65bde0[missile_index].value_150;
            for (index = 0; index < local_d8.GetCount(); ++index) {
                ResetCombatSlot(&point_target);
                point_target.iType = W8_TARGET_KIND_MONSTER;
                point_target.iMonsterID = *local_d8.GetAt(index);
                if (source->fBackfire == '\0' && source->fReflection == '\0') {
                    MakeTargetGroupHostile(source, &point_target);
                }
                missile = FireMissileSourceToTarget(missile_index, source, &point_target, &block, 1,
                                                    g_spell_records[spell_id].range_category, 9999);
                if (missile != 0) {
                    owner->missiles.Add(missile);
                }
            }
            for (index = 0; index < local_c8.GetCount(); ++index) {
                ResetCombatSlot(&point_target);
                point_target.iType = W8_TARGET_KIND_CHARACTER;
                point_target.iChar = *local_c8.GetAt(index);
                missile = FireMissileSourceToTarget(missile_index, source, &point_target, &block, 1,
                                                    g_spell_records[spell_id].range_category, 9999);
                if (missile != 0) {
                    owner->missiles.Add(missile);
                }
            }
        }
    }
    owner->kind = spell_id;
    owner->Source = *source;
    owner->target = *target;
    owner->definition = block;
    owner->monster_ids_0e0 = local_d8;
    owner->target_indices_0f0 = local_c8;
    owner->flag_120 = static_cast<unsigned char>(c);
    if (spell_id == 0x26) {
        owner->flag_121 = '\x01';
        owner->turns_remaining = RollEffectDuration(&owner->definition);
        for (index = 0; index < g_spell_effects.GetCount(); ++index) {
            W8SpellEffectEntry* previous = *g_spell_effects.GetAt(index);
            if (previous->kind == 0x26) {
                if (previous != 0) {
                    previous->turns_remaining = 0;
                    for (monster_index = 0; monster_index < PLLength(gXStatus.plsMonsterList);
                         ++monster_index) {
                        monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
                        if (monster_info != 0 && !monster_info->monster->IsDying()) {
                            SetMonsterControlState(monster_info, 0);
                        }
                    }
                }
                break;
            }
        }
    }
    if (g_current_screen_state.id == 7 && gXStatus.fNpcDialogueMode == '\0' &&
        gXStatus.fCampMode == '\0') {
        g_spell_effects.Add(owner);
    } else {
        ProcessSpellEffectTargets(owner);
        if (gXStatus.fNpcDialogueMode != '\0' || gXStatus.fCampMode != '\0') {
            for (index = 0; index < owner->spell_visuals.GetCount(); ++index) {
                (*owner->spell_visuals.GetAt(index))->auto_release = '\x01';
            }
        }
    }
    PointCameraAtCombatTarget(source, target);
    if (source->fBackfire != '\0') {
        if (quiet) {
        LAB_004fccea:
            owner->reported_124 = '\x01';
        } else {
            if (g_settings_6850c8.verbose_combat_messages == '\0') {
                AppendToLastTextLine(FormatWideString(L" -- %s", gppStringList[0x197]), -1);
                goto LAB_004fccea;
            }
            ShowNotice(0xc, FormatWideString(gppStringList[0x197]), (short)0xffff, -1);
        }
        if (local_a0 != -1) {
            if (Random(2) == 0) {
                QueueCharacterEvent(&g_status_685170.buffers.Char[local_a0],
                                    g_item_message_005ee5c8, 0, g_effect_argument_005ed8c8,
                                    g_effect_argument_005ed914);
            } else {
                ApplyItemEffectToRandomCharacter(g_item_message_005ee5cc, local_a0, 0,
                                                 g_effect_argument_005ed8c8);
            }
        }
    }
LAB_004fce23:
    if (d != 0) {
        *d = power_level;
    }
    if (fizzled) {
        if (TargetSourceIsMonster(&owner->Source, 0)) {
            if (owner->Source.iMonsterID == -1) {
                srAssertFail("pOrigSource->iMonsterID != -1", MAGIC_CPP, 0x1504, 0);
            }
            monster_index =
                MonsterGetIndexByLocationID(0x1505, MAGIC_CPP, owner->Source.iMonsterID, 1);
            monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
            if (gXStatus.fCombatMode != '\0' && g_combat_state->eCombatActionStatus != 0 &&
                g_combat_state->pActionMonsterInfo != 0 &&
                g_combat_state->pActionMonsterInfo->location_id == monster_info->location_id) {
                g_combat_state->eCombatActionStatus = 3;
            }
        }
        delete owner;
        return 2;
    }
    if (source->fBackfire == '\0') {
        return 1;
    }
    return 3;

LAB_004fcdb3:
    if (source->iType != W8_TARGET_SOURCE_CHARACTER) {
        srAssertFail("pSource->iType == SOURCE_TYPE_CHARACTER", MAGIC_CPP, 0x8c6, 0);
    }
    if (e == 3) {
        index = 0x1ab;
    }
    if (g_settings_6850c8.verbose_combat_messages != '\0') {
        PostCharacterNotice(source->iChar, FormatWideString(gppStringList[index]));
    } else {
        AppendToLastTextLine(FormatWideString(L" -- %s", gppStringList[index]), -1);
    }
    goto LAB_004fce23;
}

/* A backfiring spell swaps roles: the intended target becomes the source and
   the concrete thing to aim back at is resolved here - the caster for a
   character target, the first live party member for a party target, and so
   on. */
// FUNCTION: WIZ8 0x004FE740
void RedirectBackfiredSpellTarget004FE740(W8TargetSource* source, W8CombatSlot* target)
{
    W8TargetSource source_copy;
    W8CombatSlot target_copy;
    W8MonsterInfo* monster_info;
    W8MonsterGroup* group;
    unsigned int list_index;
    int slot;

    if (source->fBackfire != '\0') {
        srAssertFail("!pSource->fBackfire", MAGIC_CPP, 0xbf7, 0);
    }
    source_copy = *source;
    target_copy = *target;
    switch (target_copy.iType) {
    case W8_TARGET_KIND_CHARACTER:
    case W8_TARGET_KIND_CHARACTER_INDIRECT:
        SetTargetSourceToCharacter(target_copy.iChar, source);
        if (TargetSourceIsCharacter(&source_copy, 0)) {
            target->iType = W8_TARGET_KIND_CHARACTER;
            target->iChar = source_copy.iChar;
            source->unknown_1d = '\x01';
            return;
        }
        if (TargetSourceIsMonster(&source_copy, 0)) {
            target->iType = W8_TARGET_KIND_MONSTER;
            target->iMonsterID = source_copy.iMonsterID;
            source->unknown_1d = '\x01';
            return;
        }
        break;
    case W8_TARGET_KIND_PARTY:
        slot = 0;
        while (g_status_685170.buffers.XChar[slot].fOccupied == '\0' ||
               g_status_685170.buffers.Char[slot].hp_current == 0 ||
               0x12 <= g_status_685170.buffers.Char[slot].highest_condition) {
            ++slot;
            if (8 <= slot) {
                srAssertFail("FALSE", MAGIC_CPP, 0xc1b, 0);
                return;
            }
        }
        if (7 < slot) {
            srAssertFail("FALSE", MAGIC_CPP, 0xc1b, 0);
            return;
        }
        SetTargetSourceToCharacter(slot, source);
        ResetCombatSlot(target);
        if (source_copy.iType == W8_TARGET_SOURCE_CHARACTER) {
            goto LAB_004fe9ec;
        }
        if (source_copy.iType == W8_TARGET_SOURCE_MONSTER) {
            target->iType = W8_TARGET_KIND_GROUP;
            monster_info = MonsterInfoFromID(0xc2a, MAGIC_CPP, source_copy.iMonsterID, '\x01');
            target->iGroupID = monster_info->monster_group_id;
            source->unknown_1d = '\x01';
            return;
        }
        srAssertFail("FALSE", MAGIC_CPP, 0xc2d, 0);
        source->unknown_1d = '\x01';
        return;
    case W8_TARGET_KIND_MONSTER:
        list_index = MonsterGetIndexByLocationID(0xc09, MAGIC_CPP, target_copy.iMonsterID, '\x01');
        monster_info = MonsterGetScriptPartByLocationIndex(list_index);
        SetTargetSourceToMonster(monster_info, source);
        if (TargetSourceIsCharacter(&source_copy, 0)) {
            target->iType = W8_TARGET_KIND_CHARACTER;
            target->iChar = source_copy.iChar;
            source->unknown_1d = '\x01';
            return;
        }
        if (TargetSourceIsMonster(&source_copy, 0)) {
            target->iType = W8_TARGET_KIND_MONSTER;
            target->iMonsterID = source_copy.iMonsterID;
            source->unknown_1d = '\x01';
            return;
        }
        break;
    case W8_TARGET_KIND_GROUP:
        list_index = GetMonsterGroupIndexByID(0xc33, MAGIC_CPP, target_copy.iGroupID, '\x01');
        group = GetMonsterGroupByListIndex(list_index);
        list_index = MonsterGetIndexByLocationID(0xc33, MAGIC_CPP, group->value_9f, '\x01');
        monster_info = MonsterGetScriptPartByLocationIndex(list_index);
        SetTargetSourceToMonster(monster_info, source);
        ResetCombatSlot(target);
        if (source_copy.iType == W8_TARGET_SOURCE_CHARACTER) {
        LAB_004fe9ec:
            target->iType = W8_TARGET_KIND_PARTY;
            source->unknown_1d = '\x01';
            return;
        }
        if (source_copy.iType == W8_TARGET_SOURCE_MONSTER) {
            target->iType = W8_TARGET_KIND_GROUP;
            monster_info = MonsterInfoFromID(0xc40, MAGIC_CPP, source_copy.iMonsterID, '\x01');
            target->iGroupID = monster_info->monster_group_id;
            source->unknown_1d = '\x01';
            return;
        }
        srAssertFail("FALSE", MAGIC_CPP, 0xc43, 0);
        source->unknown_1d = '\x01';
        return;
    default:
        srAssertFail("FALSE", MAGIC_CPP, 0xc52, 0);
        return;
    }
    srAssertFail("FALSE", MAGIC_CPP, 0xc6b, 0);
    source->unknown_1d = '\x01';
}

/* Assert and route the source/target pair a cast is about to use. Target
   kinds that aim at a single combatant re-resolve both ends through
   RedirectBackfiredSpellTarget; the point kinds trace the aim to where the
   spell actually lands and turn the source into a point. */
// FUNCTION: WIZ8 0x004FEA50
void PrepareSpellTarget004FEA50(int spell_id, W8TargetSource* source, W8CombatSlot* target)
{
    srVector3T<float> from;
    srVector3T<float> trace;
    srVector3T<float> saved;
    W8MonsterInfo* monster_info;
    W8Navigator* navigator;
    int target_type;

    if (source->iType < 1 || 3 < source->iType) {
        srAssertFail("(pSource->iType > SOURCE_TYPE_NONE) && (pSource->iType < SOURCE_TYPE_COUNT)",
                     MAGIC_CPP, 0xc78, 0);
    }
    if (target->iType < 1 || 9 < target->iType) {
        srAssertFail("(pTarget->iType > TARGET_TYPE_NONE) && (pTarget->iType < TARGET_TYPE_COUNT)",
                     MAGIC_CPP, 0xc79, 0);
    }
    if (source->fBackfire != '\0') {
        srAssertFail("!pSource->fBackfire", MAGIC_CPP, 0xc7c, 0);
    }
    monster_info = 0;
    if (TargetSourceIsMonster(source, 0)) {
        monster_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0xc81, MAGIC_CPP, source->iMonsterID, '\x01'));
    }
    target_type = GetSpellTargetType(spell_id, '\0');
    switch (target_type) {
    case 2:
    case 7:
    case 10:
        return;
    case 3:
        if (spell_id == 3) {
            return;
        }
        if (spell_id == 0x29) {
            return;
        }
    case 1:
    case 4:
        RedirectBackfiredSpellTarget004FE740(source, target);
        return;
    case 5:
        if (TargetSourceIsCharacter(source, 0)) {
            GetCameraPosition(&from);
        } else if (TargetSourceIsMonster(source, 0)) {
            if (monster_info->monster->GetSpellPosition004C78E0(&from) == '\0') {
                monster_info->monster->GetMappedPosition004C72A0(&from);
            }
        } else {
            from = source->point;
        }
        trace.x = target->point.x;
        trace.y = target->point.y;
        trace.z = target->point.z;
        g_octree_6598a4->TraceLineOfSight(&from, &trace, '\x01', -3, -3, '\x01', 0);
        target->point = trace;
        saved = source->point;
        source->point = target->point;
        if (TargetSourceIsCharacter(source, 0)) {
            GetCameraPosition(&from);
            target->point = from;
            source->iType = W8_TARGET_SOURCE_INDIRECT;
            return;
        }
        if (TargetSourceIsMonster(source, 0)) {
            navigator = monster_info->monster;
            target->point.x = navigator->movement_0c0.position_040.x;
            target->point.y =
                navigator->movement_0c0.position_040.y + navigator->movement_0c0.height_offset_0b8;
            target->point.z = navigator->movement_0c0.position_040.z;
            source->iType = W8_TARGET_SOURCE_INDIRECT;
            return;
        }
        goto LAB_004fed4b;
    case 6:
        break;
    default:
        srAssertFail("FALSE", MAGIC_CPP, 0xd04,
                     FormatString("SpellBackfires: ERROR - Invalid spell target type for spell %d",
                                  spell_id));
        return;
    }
    saved = source->point;
    source->point = target->point;
    if (TargetSourceIsCharacter(source, 0)) {
        navigator = g_startup_world_659c0c;
    } else {
        if (!TargetSourceIsMonster(source, 0)) {
        LAB_004fed4b:
            target->point = saved;
            source->iType = W8_TARGET_SOURCE_INDIRECT;
            return;
        }
        navigator = monster_info->monster;
    }
    trace = navigator->GetPosition();
    target->point = trace;
    source->iType = W8_TARGET_SOURCE_INDIRECT;
}

/* Pick one random in-combat participant other than the source and write it as
   the backfired spell's new target. A single-target spell keeps it a single
   slot; a group spell takes the whole monster group or the party instead. */
// FUNCTION: WIZ8 0x004FEDC0
int PickBackfireTarget004FEDC0(int spell_id, W8TargetSource* source, W8CombatSlot* target)
{
    W8MonsterInfo* monster_info;
    unsigned int candidates;
    unsigned int pick;
    unsigned int index;
    int slot;

    candidates = 0;
    for (slot = 0; slot < 8; ++slot) {
        if (g_status_685170.buffers.XChar[slot].fOccupied != '\0' &&
            (!TargetSourceIsCharacter(source, 0) || source->iChar != slot)) {
            ++candidates;
        }
    }
    index = 0;
    while (PLLength(gXStatus.plsMonsterList) != 0 && index < PLLength(gXStatus.plsMonsterList)) {
        monster_info = MonsterGetScriptPartByLocationIndex(index);
        if (monster_info->fActive != '\0' && monster_info->fInCombat != '\0' &&
            monster_info->hp_current != 0 &&
            (!TargetSourceIsMonster(source, 0) ||
             source->iMonsterID != monster_info->location_id)) {
            ++candidates;
        }
        ++index;
    }
    if (candidates == 0) {
        return 0;
    }
    pick = Random(candidates) + 1;
    for (slot = 0; slot < 8; ++slot) {
        if (g_status_685170.buffers.XChar[slot].fOccupied != '\0' &&
            (!TargetSourceIsCharacter(source, 0) || source->iChar != slot) && (--pick == 0)) {
            if (GetSpellTargetType(spell_id, '\0') == 4) {
                target->iType = W8_TARGET_KIND_PARTY;
                return 1;
            }
            target->iChar = slot;
            target->iType = W8_TARGET_KIND_CHARACTER;
            return 1;
        }
    }
    index = 0;
    while (PLLength(gXStatus.plsMonsterList) != 0 && index < PLLength(gXStatus.plsMonsterList)) {
        monster_info = MonsterGetScriptPartByLocationIndex(index);
        if (monster_info->fActive != '\0' && monster_info->fInCombat != '\0' &&
            monster_info->hp_current != 0 &&
            (!TargetSourceIsMonster(source, 0) ||
             source->iMonsterID != monster_info->location_id) &&
            (--pick == 0)) {
            if (GetSpellTargetType(spell_id, '\0') == 4) {
                target->iType = W8_TARGET_KIND_GROUP;
                target->iGroupID = monster_info->monster_group_id;
                return 1;
            }
            target->iType = W8_TARGET_KIND_MONSTER;
            target->iMonsterID = monster_info->location_id;
            return 1;
        }
        ++index;
    }
    return 0;
}

/* Scatter a backfired point target to a random reachable spot inside the
   spell's range around its source: random offsets off the source's position,
   line of sight traced, the result settled onto the ground and clamped back
   inside range. */
// FUNCTION: WIZ8 0x004FEF90
void ScatterSpellPointTarget004FEF90(int spell_id, W8TargetSource* source, W8CombatSlot* target)
{
    srVector3T<float> origin;
    srVector3T<float> point;
    srVector3T<float> delta;
    W8MonsterInfo* monster_info;
    W8Navigator* navigator;
    float range;
    float distance;
    int attempt;

    range = CalcRangeDistance(g_spell_records[spell_id].range_category);
    navigator = g_startup_world_659c0c;
    if (source->iType != W8_TARGET_SOURCE_CHARACTER) {
        if (source->iType != W8_TARGET_SOURCE_MONSTER) {
            srAssertFail("pSource->iType == SOURCE_TYPE_MONSTER", MAGIC_CPP, 0xd80, 0);
        }
        monster_info = MonsterInfoFromID(0xd81, MAGIC_CPP, source->iMonsterID, '\x01');
        navigator = monster_info->monster;
    }
    origin = navigator->GetPosition();
    attempt = 0;
    do {
        point.x = static_cast<float>((Random(0x7d1) - g_monster_poster_max_distance_005ec3d8) *
                                         range * g_double_005ec8d0 +
                                     origin.x);
        point.z = static_cast<float>((Random(0x7d1) - g_monster_poster_max_distance_005ec3d8) *
                                         range * g_double_005ec8d0 +
                                     origin.z);
        point.y = Random(0x3e9) * range * g_double_005ec8d0 + origin.y;
        g_octree_6598a4->TraceLineOfSight(&origin, &point, '\x01', -3, -3, '\x01', 0);
        point.y = SettlePositionToGround00420BD0(&point, (unsigned char*)0x0);
        if (point.y != g_ground_settle_fail_005ed7d0) {
            break;
        }
        ++attempt;
    } while (attempt < 5);
    delta.x = origin.x - point.x;
    delta.y = origin.y - point.y;
    delta.z = origin.z - point.z;
    distance = delta.y * delta.y + delta.x * delta.x + delta.z * delta.z;
    if (range < sqrtf(distance)) {
        if (distance != static_cast<float>(g_zero_005ebb40)) {
            range = range / sqrtf(distance);
            delta.x = delta.x * range;
            delta.y = delta.y * range;
            delta.z = range * delta.z;
        }
        point.x = delta.x + origin.x;
        point.y = delta.y + origin.y;
        point.z = delta.z + origin.z;
    }
    target->point = point;
}

/* The once-per-cast backfire roll for a spell whose source is under the
   confusion condition: a trait save can skip it, then roughly one cast in
   five has its target re-picked at random or scattered to a random point,
   with a notice to whoever the source is. */
// FUNCTION: WIZ8 0x004FF220
void CheckSpellBackfire004FF220(int spell_id, W8TargetSource* source, W8CombatSlot* target)
{
    W8Character* character;
    W8MonsterInfo* monster_info;
    int target_type;

    monster_info = 0;
    if (source->iType == W8_TARGET_SOURCE_CHARACTER) {
        character = &g_status_685170.buffers.Char[source->iChar];
        if (character->uiCondition[0xc] == 0) {
            return;
        }
        if (target->iType == W8_TARGET_KIND_CHARACTER && target->iChar == source->iChar) {
            return;
        }
        if (CharacterHasTrait00547940(character, 7)) {
            if (Random(100) < static_cast<unsigned int>(static_cast<int>(
                                  ScaleValueByProfessionLevel005479B0(character, 7, 50.0)))) {
                return;
            }
        }
    } else {
        if (source->iType != W8_TARGET_SOURCE_MONSTER) {
            return;
        }
        monster_info = MonsterInfoFromID(0xdcf, MAGIC_CPP, source->iMonsterID, '\x01');
        if (monster_info->condition_turns[0xc] == 0) {
            return;
        }
        if (target->iType == W8_TARGET_KIND_MONSTER && target->iMonsterID == source->iMonsterID) {
            return;
        }
    }
    if (0x13 < Random(100)) {
        return;
    }
    target_type = GetSpellTargetType(spell_id, '\0');
    switch (target_type) {
    case 1:
    case 3:
    case 4:
        if (PickBackfireTarget004FEDC0(spell_id, source, target) == '\0') {
            return;
        }
        break;
    case 5:
    case 6:
    case 8:
        ScatterSpellPointTarget004FEF90(spell_id, source, target);
        break;
    default:
        return;
    }
    if (TargetSourceIsCharacter(source, 0)) {
        PostCharacterNotice(source->iChar, gppStringList[0x191]);
        return;
    }
    if (TargetSourceIsMonster(source, 0)) {
        PostMonsterNotice(monster_info, gppStringList[0x191]);
    }
}

/* Resolve valid spell targets for one cast and append location ids to the
   caller's vectors. The source's position and eye are picked per source kind,
   then the spell's target type decides whether the single aimed slot, a
   radius, a cone or a whole side goes into the vectors. */
// FUNCTION: WIZ8 0x004FD030
void PopulateSpellTargetMarkers(int spell_id, int power_level, W8TargetSource* source,
                                W8CombatSlot* target, W8GrowableVector<int>* monster_markers,
                                W8GrowableVector<int>* party_markers, int highlighting)
{
    srVector3T<float> player_pos;
    srVector3T<float> camera;
    srVector3T<float> centre;
    srVector3T<float> eye;
    srVector3T<float> target_point;
    srVector3T<float> trace;
    W8MonsterInfo* monster_info;
    W8MonsterInfo* member;
    W8MonsterGroup* group;
    W8Monster* monster;
    float radius;
    float heading;
    float elevation;
    float distance;
    int target_type;
    int side;
    int sight_flag;
    int slot;
    unsigned int index;
    bool marked;

    monster = 0;
    marked = false;
    monster_info = 0;
    sight_flag = 0;
    if (source->fBackfire == '\0' && source->fReflection == '\0' &&
        SourceActionReachesTarget(source, target) == '\0') {
        return;
    }
    player_pos = g_startup_world_659c0c->GetPosition();
    GetCameraPosition(&camera);
    if (TargetSourceIsMonster(source, 1)) {
        if (source->iMonsterID == -1) {
            FormatDebugMessage(1,
                               "Invalid Magic Source! Spell %d '%ls' Target(%d,%d,%d,%d) "
                               "Source(%d,%d,%d)",
                               spell_id, g_spell_records[spell_id].display_name, target->iType,
                               target->iChar, target->iMonsterID, target->iGroupID, source->iType,
                               source->iChar, -1);
            return;
        }
        monster_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0x937, MAGIC_CPP, source->iMonsterID, '\x01'));
        GetMonsterDataForInfo(monster_info);
        monster = monster_info->monster;
    }
    if (TargetSourceIsCharacter(source, 0)) {
        centre = player_pos;
        eye = camera;
    } else if (TargetSourceIsMonster(source, 0)) {
        centre = monster->GetPosition();
        if (source->unknown_1d == '\0' && spell_id != 0x77 && monster_info->has_spell_37c != '\0') {
            if (monster->GetSpellPosition004C78E0(&eye) != '\0') {
                sight_flag = 3;
                goto LAB_004fd26b;
            }
            srAssertFail("fVertextAvail", MAGIC_CPP, 0x951, 0);
        }
        eye = monster->movement_0c0.position_040;
        eye.y += monster->movement_0c0.height_offset_0b8;
        sight_flag = 2;
    } else {
        eye = source->point;
        centre = source->point;
    }
LAB_004fd26b:
    target_point = target->point;
    target_type = GetSpellTargetType(spell_id, '\0');
    switch (target_type) {
    case 0:
    case 3:
        if (target->iType == W8_TARGET_KIND_CHARACTER) {
            party_markers->Add(target->iChar);
            goto switchD_004fd2b7_default;
        }
        if (target->iType != W8_TARGET_KIND_MONSTER) {
            if (target_type == 3) {
                goto LAB_004fd3b6;
            }
            goto switchD_004fd2b7_default;
        }
        goto LAB_004fd396;
    case 1:
        if (target->iType == W8_TARGET_KIND_CHARACTER ||
            target->iType == W8_TARGET_KIND_CHARACTER_INDIRECT) {
            party_markers->Add(target->iChar);
            goto switchD_004fd2b7_default;
        }
        if (target->iType != W8_TARGET_KIND_MONSTER) {
            goto LAB_004fd3b6;
        }
    LAB_004fd396:
        monster_markers->Add(target->iMonsterID);
        goto switchD_004fd2b7_default;
    case 2:
        radius = (g_spell_records[spell_id].radius_per_level_127 * power_level +
                  g_spell_records[spell_id].effect_radius) *
                 g_world_scale_005ebc40;
        if (TargetSourceIsCharacter(source, 0)) {
            marked = true;
            side = 2;
            if (g_float_005ebb34 < radius) {
                radius += g_startup_world_659c0c->radius_084;
            }
        } else if (TargetSourceIsMonster(source, 0)) {
            if (monster_info->ubDisposition == '\x02') {
                distance = (centre.x - player_pos.x) * (centre.x - player_pos.x) +
                           (centre.y - player_pos.y) * (centre.y - player_pos.y) +
                           (centre.z - player_pos.z) * (centre.z - player_pos.z);
                if (sqrtf(distance) <= radius &&
                    monster_info->player_visibility.los_flags_05[sight_flag] != '\0') {
                    marked = true;
                }
            } else if (monster_info->ubDisposition != '\x01') {
                goto LAB_004fdaf6;
            }
            side = monster_info->ubDisposition;
            radius += monster->radius_084;
        } else {
            side = 2;
            distance = (centre.x - player_pos.x) * (centre.x - player_pos.x) +
                       (centre.y - player_pos.y) * (centre.y - player_pos.y) +
                       (centre.z - player_pos.z) * (centre.z - player_pos.z);
            if (sqrtf(distance) <= radius &&
                g_octree_6598a4->TraceLineOfSight(&eye, &camera, '\x01', -3, -3, '\x01', 0) == 0) {
                marked = true;
            }
        }
        CollectMonstersWithinRadius(&centre, &eye, monster_markers, radius, static_cast<char>(side),
                                    static_cast<char>(highlighting));
        break;
    case 4:
        if (target->iType == W8_TARGET_KIND_GROUP) {
            group = GetMonsterGroupByListIndex(
                GetMonsterGroupIndexByID(0x99f, MAGIC_CPP, target->iGroupID, '\x01'));
            if (group == 0) {
                goto LAB_004fd3b6;
            }
            index = 0;
            while (ILLength(group->monsters) != 0 && index < ILLength(group->monsters)) {
                slot = IListGetAt(group->monsters, index);
                member = MonsterInfoFromID(0x9aa, MAGIC_CPP, slot, '\x01');
                if (member->ubDisposition == group->ubDisposition) {
                    monster_markers->Add(slot);
                }
                ++index;
            }
            goto switchD_004fd2b7_default;
        }
        if (target->iType != W8_TARGET_KIND_PARTY) {
            goto LAB_004fd3b6;
        }
        goto LAB_004fddcd;
    case 5:
        heading = GetHeadingAngle(&eye, &target_point);
        elevation = GetElevationAngle(&eye, &target_point);
        if (TargetSourceIsCharacter(source, 0)) {
            side = 1;
        } else if (TargetSourceIsMonster(source, 0)) {
            if (monster_info->ubDisposition == '\x02') {
            LAB_004fd5d9:
                side = 1;
            } else {
                if (monster_info->ubDisposition != '\x01') {
                    goto LAB_004fdaf6;
                }
                side = 2;
                if (TargetInRangeAndArcs00539B70(&camera, g_startup_world_659c0c->radius_084, &eye,
                                                 monster->radius_084, heading, elevation) != 0 &&
                    monster_info->player_visibility.los_flags_05[sight_flag] != '\0') {
                    goto LAB_004fd749;
                }
            }
        } else {
            side = 3;
            if (source->fBackfire != '\0' || source->fReflection != '\0') {
                if (source->iChar == -1) {
                    if (source->iMonsterID == -1) {
                        goto LAB_004fd6e2;
                    }
                    if (MonsterInfoFromID(0x9f3, MAGIC_CPP, source->iMonsterID, '\x01')
                            ->ubDisposition != '\x02') {
                        goto LAB_004fd5d9;
                    }
                }
                side = 2;
            }
        }
    LAB_004fd6e2:
        if (TargetInRangeAndArcs00539B70(&camera, g_startup_world_659c0c->radius_084, &eye, 0,
                                         heading, elevation) != 0 &&
            g_octree_6598a4->TraceLineOfSight(&eye, &camera, '\x01', -3, -3, '\x01', 0) == 0) {
        LAB_004fd749:
            marked = true;
        }
        CollectConeMonsterTargets00539CA0(source, &eye, heading, elevation, monster_markers,
                                          static_cast<unsigned char>(side), sight_flag);
        if (monster_markers->count == 0 &&
            (g_combat_state == 0 || g_combat_state->flag_a54 == '\0') &&
            TargetSourceIsCharacter(source, 0) && static_cast<char>(side) == '\x01' &&
            !AnyMonsterEngaged()) {
            CollectConeMonsterTargets00539CA0(source, &eye, heading, elevation, monster_markers, 3,
                                              sight_flag);
        }
        break;
    case 6:
        trace.x = target_point.x;
        trace.y = target_point.y - g_float_005ebc64;
        trace.z = target_point.z;
        radius = (g_spell_records[spell_id].radius_per_level_127 * power_level +
                  g_spell_records[spell_id].effect_radius) *
                 g_world_scale_005ebc40;
        if (TargetSourceIsCharacter(source, 1)) {
        LAB_004fda9d:
            if (source->fBackfire != '\0' || source->fReflection != '\0') {
            LAB_004fdaab:
                side = 1;
            } else {
            LAB_004fdac7:
                side = 2;
            }
        } else {
            if (!TargetSourceIsMonster(source, 1)) {
                side = 3;
            } else if (monster_info->ubDisposition == '\x02') {
                goto LAB_004fda9d;
            } else {
                if (monster_info->ubDisposition != '\x01') {
                    goto LAB_004fdaf6;
                }
                if (source->fBackfire == '\0' && source->fReflection == '\0') {
                    goto LAB_004fdac7;
                }
                goto LAB_004fdaab;
            }
        }
        if (side != 1) {
            distance = (trace.x - player_pos.x) * (trace.x - player_pos.x) +
                       (trace.y - player_pos.y) * (trace.y - player_pos.y) +
                       (trace.z - player_pos.z) * (trace.z - player_pos.z);
            if (sqrtf(distance) <= radius &&
                g_octree_6598a4->TraceLineOfSight(&target_point, &camera, '\x01', -3, -3, '\x01',
                                                  0) == 0) {
                marked = true;
            }
        }
        CollectMonstersWithinRadius(&trace, &target_point, monster_markers, radius,
                                    static_cast<char>(side), static_cast<char>(highlighting));
        if (monster_markers->count == 0 &&
            (g_combat_state == 0 || g_combat_state->flag_a54 == '\0') &&
            TargetSourceIsCharacter(source, 0) && static_cast<char>(side) == '\x01' &&
            !AnyMonsterEngaged()) {
            CollectMonstersWithinRadius(&trace, &target_point, monster_markers, radius, '\x03',
                                        static_cast<char>(highlighting));
        }
        break;
    case 7:
        target->point = eye;
        radius = CalcRangeDistance(g_spell_records[spell_id].range_category, source);
        if (TargetSourceIsCharacter(source, 1) ||
            (TargetSourceIsMonster(source, 1) && monster_info->ubDisposition == '\x02')) {
            if (source->fBackfire != '\0' || source->fReflection != '\0') {
            LAB_004fdcc6:
                side = 2;
                goto LAB_004fdccb;
            }
            side = 1;
        } else {
            if (TargetSourceIsMonster(source, 1)) {
                if (source->fBackfire == '\0' && source->fReflection == '\0') {
                    goto LAB_004fdcc6;
                }
                side = 1;
            } else {
                side = 3;
            LAB_004fdccb:
                distance = (centre.x - player_pos.x) * (centre.x - player_pos.x) +
                           (centre.y - player_pos.y) * (centre.y - player_pos.y) +
                           (centre.z - player_pos.z) * (centre.z - player_pos.z);
                if (sqrtf(distance) <= radius &&
                    g_octree_6598a4->TraceLineOfSight(&eye, &camera, '\x01', -3, -3, '\x01', 0) ==
                        0) {
                    marked = true;
                }
            }
        }
        CollectMonstersWithinRadius(&centre, &eye, monster_markers, radius, static_cast<char>(side),
                                    static_cast<char>(highlighting));
        if (monster_markers->count == 0 &&
            (g_combat_state == 0 || g_combat_state->flag_a54 == '\0') &&
            TargetSourceIsCharacter(source, 0) && static_cast<char>(side) == '\x01' &&
            !AnyMonsterEngaged()) {
            CollectMonstersWithinRadius(&centre, &eye, monster_markers, radius, '\x03',
                                        static_cast<char>(highlighting));
        }
        break;
    default:
        goto switchD_004fd2b7_default;
    }
    if (marked) {
    LAB_004fddcd:
        if (spell_id != 0x16 && spell_id != 0x4d) {
            slot = 0;
            do {
                if (g_status_685170.buffers.XChar[slot].fOccupied != '\0' &&
                    g_status_685170.buffers.Char[slot].hp_current != 0 &&
                    g_status_685170.buffers.Char[slot].highest_condition < 0x12 &&
                    (g_status_685170.buffers.Char[slot].uiCondition[0xd] == 0 ||
                     !MonsterCanAimSpell005474B0(spell_id) || static_cast<char>(side) == '\x03')) {
                    party_markers->Add(slot);
                }
                ++slot;
            } while (slot < 8);
        }
    }
switchD_004fd2b7_default:
    PruneSpellTargetMarkers00501B70(spell_id, monster_markers);
    return;
LAB_004fd3b6:
    FormatDebugMessage(1,
                       "Invalid Magic Target! Spell %d '%ls' Target(%d,%d,%d,%d) "
                       "Source(%d,%d,%d)",
                       spell_id, g_spell_records[spell_id].display_name, target->iType,
                       target->iChar, target->iMonsterID, target->iGroupID, source->iType,
                       source->iChar, source->iMonsterID);
    return;
LAB_004fdaf6:
    FormatDebugMessage(1,
                       "Invalid Magic Source! Spell %d '%ls' Target(%d,%d,%d,%d) "
                       "Source(%d,%d,%d)",
                       spell_id, g_spell_records[spell_id].display_name, target->iType,
                       target->iChar, target->iMonsterID, target->iGroupID, source->iType,
                       source->iChar, source->iMonsterID);
}

/* Drop every marker that no longer names a live, targetable monster; a few
   spell ids prune on extra monster-record rules. */
// FUNCTION: WIZ8 0x00501B70
void PruneSpellTargetMarkers00501B70(int spell_id, W8GrowableVector<int>* monster_markers)
{
    W8MonsterInfo* monster_info;
    W8MonsterRecord* record;
    int index;
    int shift;
    int location_id;

    index = monster_markers->count;
joined_r0x00501b7a:
    --index;
    if (index < 0) {
        return;
    }
    if (index < monster_markers->count) {
        location_id = monster_markers->data[index];
    } else {
        location_id = monster_markers->data[0];
    }
    monster_info = MonsterGetScriptPartByLocationIndex(
        MonsterGetIndexByLocationID(0x1595, MAGIC_CPP, location_id, '\x01'));
    record = GetMonsterDataForInfo(monster_info);
    if (monster_info->fActive == '\0' || monster_info->hp_current == 0 ||
        monster_info->condition_turns[0x12] != 0 || record->untargetable_24a != '\0') {
        goto LAB_00501cc9;
    }
    if (spell_id == 0x16) {
        if (record->kind_0cb == '\x14' || record->kind_0cb == '\x15') {
            goto joined_r0x00501b7a;
        }
    } else if (spell_id == 0x4d) {
        if (record->kind_0cb == '\x14' || record->kind_0cb == '\x15' ||
            record->kind_0cb == '\x1c' || monster_info->summoned_2da != 0) {
            goto joined_r0x00501b7a;
        }
    } else {
        if (spell_id != 0x81 || record->kind_0cb == '\x14') {
            goto joined_r0x00501b7a;
        }
    }
    if (monster_markers->count <= index || index < 0) {
        goto joined_r0x00501b7a;
    }
    shift = index;
    while (shift < monster_markers->count - 1) {
        monster_markers->data[shift] = monster_markers->data[shift + 1];
        ++shift;
    }
    goto LAB_00501cee;
LAB_00501cc9:
    if (monster_markers->count <= index || index < 0) {
        goto joined_r0x00501b7a;
    }
    shift = index;
    while (shift < monster_markers->count - 1) {
        monster_markers->data[shift] = monster_markers->data[shift + 1];
        ++shift;
    }
LAB_00501cee:
    --monster_markers->count;
    goto joined_r0x00501b7a;
}
