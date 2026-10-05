#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/integer_constants.h"
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
int GetProfessionCasterLevel(const W8Character* character, W8Profession profession_id)
{
    int magic_level_offset;

    if (profession_id == W8_PROFESSION_NONE) {
        profession_id = character->iProfession;
        if (profession_id == W8_PROFESSION_NONE) {
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

    if (GetSpellTargetType(spell_id, false) == W8_TARGET_TYPE_ALL_ENEMIES) {
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
    if (g_item_records[item->iItemNo].category != W8_ITEM_CATEGORY_SPELL_SOURCE) {
        return false;
    }
    if (!item->identified) {
        return false;
    }
    return CanCharacterUseItem(caster, item->iItemNo);
}

#define MAGIC_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Magic.cpp"

/* The three monster kinds whose alchemy survives a spellcasting block. */
enum {
    W8_MONSTER_KIND_ALCHEMY_LOW = 4,
    W8_MONSTER_KIND_ALCHEMY_HIGH = 5,
    W8_MONSTER_KIND_ALCHEMY_OTHER = 0xd
};

/* Whether a spellcasting block stops this character casting this spell. The
   block stops everything except alchemy in the hands of someone who has the
   skill for it. */
// FUNCTION: WIZ8 0x004fae70
bool IsSpellBlockedForCharacter(const W8Character* character, int spell_id)
{
    if (character->uiCondition[W8_CONDITION_SILENCED] != 0) {
        if (g_spell_records[spell_id].alchemy_spell == 0) {
            return true;
        }
        return character->skills[W8_SKILL_SPELLBOOK_ALCHEMY].level == 0;
    }
    return false;
}

/* The same question for a monster. The block stops everything except alchemy
   from the three kinds that keep it. */
// FUNCTION: WIZ8 0x004fb1d0
bool IsSpellBlockedForMonster(W8MonsterInfo* monster_info, int spell_id)
{
    unsigned char kind;

    if (monster_info->uiCondition[W8_CONDITION_SILENCED] != 0) {
        if (g_spell_records[spell_id].alchemy_spell == 0) {
            return true;
        }
        kind = GetMonsterDataForInfo(monster_info)->kind;
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

    if (monster_info->uiCondition[W8_CONDITION_SILENCED] != 0) {
        if (g_spell_records[spell_id].alchemy_spell == 0) {
            return false;
        }
        kind = GetMonsterDataForInfo(monster_info)->kind;
        if (kind < W8_MONSTER_KIND_ALCHEMY_LOW ||
            (kind > W8_MONSTER_KIND_ALCHEMY_HIGH && kind != W8_MONSTER_KIND_ALCHEMY_OTHER)) {
            return false;
        }
    }

    if (GetSpellTargetType(spell_id, false) == W8_TARGET_TYPE_CASTER) {
        SetMonsterCombatTarget(monster_info, monster_info->location_id);
    } else if (!MonsterTargetMatchesSpell(monster_info, spell_id)) {
        return false;
    }

    combat_slot = &monster_info->Target;
    if (!MonsterActionReachesTarget(monster_info, record, 0, combat_slot) &&
        !ClearMonsterCombatSlot(monster_info)) {
        return false;
    }
    if (DispatchWorldCursorNodeCommand(monster_info, 4, 0)) {
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
bool SpellUsableNow(int spell_id, bool allow_out_of_combat)
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

    if (g_current_screen_state.id == W8_SCREEN_CAMP && !gXStatus.fCombatMode &&
        gXStatus.fCampMode && (spell_id == 0x17 || spell_id == 0x3a)) {
        return true;
    }

    lock_or_trap = gXStatus.fLockInteract || gXStatus.fTrapInteract;

    switch (usable_when) {
    case W8_SPELL_USABLE_ANY_TIME:
        break;
    case W8_SPELL_USABLE_IN_COMBAT:
        if (!gXStatus.fCombatMode && !allow_out_of_combat) {
            return false;
        }
        break;
    case W8_SPELL_USABLE_OUT_OF_COMBAT:
        if (gXStatus.fCombatMode) {
            return false;
        }
        break;
    case W8_SPELL_USABLE_WHILE_CAMPED:
        if (!gXStatus.fCampMode) {
            return false;
        }
        return !lock_or_trap;
    case W8_SPELL_USABLE_ON_LOCK_OR_TRAP:
        if (spell_id != 0x27) {
            return spell_id == 0x12 ? gXStatus.fTrapInteract : 0;
        }
        if (gXStatus.fLockInteract) {
            return true;
        }
        return gXStatus.fTrapInteract;
    default:
        srAssertFail("FALSE", MAGIC_CPP, 4228, "SpellUsableNow: ERROR - Invalid uiSpellUsableWhen");
        return allow_out_of_combat;
    }

    if (!gXStatus.fCampMode) {
        return !lock_or_trap;
    }
    return false;
}

/* What the interface has to ask the player to pick before a friendly spell can
   be cast. Nine target types map onto seven answers; the one that depends on
   the selection owner only needs a pick when there is nothing selected already
   and the caller is not in one of the two contexts that supply it. */
// FUNCTION: WIZ8 0x005010f0
W8TargetNeed GetTargetNeededForSpellFriendly(int spell_id, bool normalize,
                                             W8TargetingContext context)
{
    if (spell_id != 0) {
        switch (GetSpellTargetType(spell_id, normalize)) {
        case W8_TARGET_TYPE_ALLY:
            return spell_id != 0x58 ? W8_TARGET_NEED_ALLY : W8_TARGET_NEED_CHARACTER_INDIRECT;
        case W8_TARGET_TYPE_CASTER:
            return W8_TARGET_NEED_CASTER;
        case W8_TARGET_TYPE_ENEMY:
            return W8_TARGET_NEED_ENEMY;
        case W8_TARGET_TYPE_ENEMY_GROUP:
            return W8_TARGET_NEED_GROUP;
        case W8_TARGET_TYPE_CONE:
            return W8_TARGET_NEED_CONE;
        case W8_TARGET_TYPE_RADIUS:
        case W8_TARGET_TYPE_POINT:
            return W8_TARGET_NEED_PLACE;
        case W8_TARGET_TYPE_ITEM:
            if (g_level_block == 0 || context == W8_TARGETING_CONTEXT_SPELL ||
                context == W8_TARGETING_CONTEXT_ITEM) {
                return W8_TARGET_NEED_ITEM;
            }
            break;
        case W8_TARGET_TYPE_PARTY:
        case W8_TARGET_TYPE_ALL_ENEMIES:
        case W8_TARGET_TYPE_LOCK_OR_TRAP:
            break;
        default:
            srAssertFail(
                "FALSE", MAGIC_CPP, 4851,
                FormatString("GetTargetNeededForSpellFriendly: ERROR - Invalid spell target for %d",
                             spell_id));
            break;
        }
    }
    return W8_TARGET_NEED_NONE;
}

/* The same question for a hostile spell, which has fewer answers because a
   hostile spell never targets the party's own belongings. */
// FUNCTION: WIZ8 0x005011c0
W8TargetNeed GetTargetNeededForSpellHostile(int spell_id)
{
    switch (GetSpellTargetType(spell_id, false)) {
    case W8_TARGET_TYPE_ALLY:
        return spell_id != 0x58 ? W8_TARGET_NEED_ALLY : W8_TARGET_NEED_CHARACTER_INDIRECT;
    case W8_TARGET_TYPE_ENEMY:
        return W8_TARGET_NEED_ENEMY;
    case W8_TARGET_TYPE_CASTER:
        return W8_TARGET_NEED_CASTER;
    case W8_TARGET_TYPE_ENEMY_GROUP:
        return W8_TARGET_NEED_GROUP;
    case W8_TARGET_TYPE_PARTY:
    case W8_TARGET_TYPE_CONE:
    case W8_TARGET_TYPE_RADIUS:
    case W8_TARGET_TYPE_ALL_ENEMIES:
    case W8_TARGET_TYPE_POINT:
        break;
    default:
        srAssertFail(
            "FALSE", MAGIC_CPP, 4889,
            FormatString("GetTargetNeededForSpellHostile: ERROR - Invalid spell target for %d",
                         spell_id));
        break;
    }
    return W8_TARGET_NEED_NONE;
}

/* Forwarder that narrows CanCharReBreathe's answer to a flag. Its argument is
   a party slot, not a spell - which is only visible once the underlying
   predicate is named. */
// FUNCTION: WIZ8 0x00501860
bool CanPartySlotReBreathe(int party_slot)
{
    if (!CanCharReBreathe(party_slot)) {
        return false;
    }
    return true;
}

/* One queued spell effect. Each entry counts down a turn at a time and is
   distinguished only by its kind; the effect body itself lives elsewhere. */
/* The kind whose expiry hands every monster back its own control. */
enum { W8_SPELL_EFFECT_KIND_MONSTER_CONTROL = 0x26 };

enum { W8_PARTY_CONDITION_SLOTS = 12, W8_COMBAT_CONDITION_SLOTS = 9 };

// GLOBAL: WIZ8 0x00689B58
W8GrowableVector<W8SpellEffectEntry*> g_spell_effects;
// GLOBAL: WIZ8 0x005ED7D0
const float g_ground_settle_fail = -1000000.0f;
/* Debug switch: when set, every cast except 0x76 fizzles on a forced 100
   percent failure chance. */
// GLOBAL: WIZ8 0x00689b68
bool g_force_spell_failure;
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
                    if (monster_info != 0 && !monster_info->p3D->IsDying()) {
                        SetMonsterControlState(monster_info, W8_MONSTER_CONTROL_NONE);
                    }
                }
            }
        }
    }
}

// FUNCTION: WIZ8 0x005012b0
bool PartyHasCondition(int effect_id)
{
    W8EffectSlot* slot = g_status.effect_slots;

    while (!slot->active || slot->effect_id != effect_id) {
        ++slot;
        if (slot > &g_status.effect_slots[W8_PARTY_CONDITION_SLOTS - 1]) {
            return false;
        }
    }
    return true;
}

/* Whether either side of the current fight is under one particular condition.
   Both tables are searched, the party's first. */
// FUNCTION: WIZ8 0x00501250
bool CombatHasCondition(int effect_id)
{
    W8EffectSlot* slot;
    unsigned int index;

    if (g_combat_state != 0) {
        slot = g_combat_state->effect_slots;
        for (index = 0; index < W8_COMBAT_CONDITION_SLOTS; ++index, ++slot) {
            if (slot->active && slot->effect_id == effect_id) {
                return true;
            }
        }
        /* 0x00501250: nine 0x11-byte strides from g_combat_state+0x85a.
           The first six occupy effect_slots0; the rest overlap
           engaged_missile and TargetHit. Retail does that overlapping
           walk; it is a raw stride, not a typed array of nine. */
        // clang-format off
        for (index = 0; index < W8_COMBAT_CONDITION_SLOTS; ++index) {
            slot = g_combat_state->effect_slots0 + index;
            if (slot->active && slot->effect_id == effect_id) {
                return true;
            }
        }
        // clang-format on
    }
    return false;
}

// FUNCTION: WIZ8 0x004f9aa0
void SetPartySlotSpell(int party_slot, int spell_id, int power_level, const W8CombatSlot* target)
{
    W8PartySlotRow* row = &g_status.buffers.XChar[party_slot];

    row->spell_id = spell_id;
    row->spell_detail.spell.power_level = power_level;
    row->spell_detail.spell.unused = 0;
    row->spell_target = *target;
}

// FUNCTION: WIZ8 0x004f9a20
void SetCharacterSpell(const W8Character* character, int spell_id, int power_level)
{
    int party_slot = CharacterPointerToPartySlot(character);
    W8ActionDetailBlock notify;

    notify.spell.power_level = power_level;
    notify.spell.unused = 0;
    ChooseAction(party_slot, W8_ACTION_CAST_SPELL, spell_id, &notify, false, 1);

    SetPartySlotSpell(party_slot, spell_id, power_level,
                      GetTargetBlockForContext(party_slot, W8_TARGETING_CONTEXT_CURRENT));
}

/* Whether one party slot's recorded spell target is still a target it could
   reach: the target has to be of the kind the spell needs, and the slot has to
   be in range of it. */
// FUNCTION: WIZ8 0x00501530
bool PartySlotSpellTargetStillValid(int party_slot)
{
    W8TargetNeed needed = GetTargetNeededForSpellFriendly(
        g_status.buffers.XChar[party_slot].spell_id, false, W8_TARGETING_CONTEXT_CURRENT);

    if (!TargetMatchesNeeded(GetTargetBlockForContext(party_slot, W8_TARGETING_CONTEXT_SPELL),
                             needed)) {
        return false;
    }
    return CharacterActionReachesTarget(party_slot, 0, W8_TARGETING_CONTEXT_SPELL);
}

/* Start one character's breath attack. The assertion names the predicate it
   depends on outright - CanCharReBreathe - so a character who cannot is a
   caller error rather than a refusal. */
// FUNCTION: WIZ8 0x00501880
void StartCharacterBreathAttack(int party_slot)
{
    W8PartySlotRow* row = &g_status.buffers.XChar[party_slot];

    if (!CanCharReBreathe(party_slot)) {
        srAssertFail("CanCharReBreathe(uiChar)", MAGIC_CPP, 5320, 0);
    }
    ChooseAction(party_slot, W8_ACTION_BREATHE, -1, 0, false, 1);
    AimAtTarget(party_slot, &row->breath_target, W8_TARGETING_CONTEXT_CURRENT);
    if (IsSpellTargetStillValidIn(party_slot, 0x77, W8_TARGETING_CONTEXT_BREATH)) {
        StartBreathCycle(party_slot, false);
        return;
    }
    FallbackFromUnreachableAction(party_slot);
}

/* Fold one missile's accumulated damage and reports into the queued effect
   that owns it. The owning effect is the one whose missile list still names
   the missile; nothing happens if it has already been detached. */
// FUNCTION: WIZ8 0x00500460
void AbsorbMissileDamage(W8Missile* missile)
{
    for (int effect_index = 0; effect_index < g_spell_effects.GetCount(); ++effect_index) {
        W8SpellEffectEntry* effect = *g_spell_effects.GetAt(effect_index);

        for (int missile_index = 0; missile_index < effect->missiles.GetCount(); ++missile_index) {
            if (*effect->missiles.GetAt(missile_index) == missile) {
                effect->result.count += missile->result.count;
                effect->result.amount += missile->result.amount;
                for (int condition = 0; condition < W8_CONDITION_COUNT; ++condition) {
                    effect->result.condition_counts[condition] +=
                        missile->result.condition_counts[condition];
                }
                while (missile->result.reports.GetCount() >= 1) {
                    effect->result.reports.Add(missile->result.reports.RemoveAt(0));
                }
                return;
            }
        }
    }
}

// FUNCTION: WIZ8 0x005008a0
void AddSpellEffect(W8SpellEffectEntry* effect)
{
    g_spell_effects.Add(effect);
}

void FinishSpellEffect(W8SpellEffectEntry* effect); /* 0x00500F70 */

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
void UpdateSpellEffects(void)
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
            if (!visual->finished) {
                alive = false;
            }
        }
        for (int missile_index = 0; missile_index < effect->missiles.GetCount() && alive;
             ++missile_index) {
            W8Missile* missile = *effect->missiles.GetAt(missile_index);
            if (!missile->flight_done) {
                alive = false;
            }
        }
        for (int monster_index = 0; monster_index < effect->target_indices.GetCount() && alive;
             ++monster_index) {
            int entry = *effect->target_indices.GetAt(monster_index);
            if (gXStatus.monster_manager_entries[entry].effect_icon_active) {
                alive = false;
            }
        }

        bool process =
            ((g_spell_records[effect->kind].missile_delivered == 0 && !effect->missiles_pending) ||
             effect->sustained || alive) &&
            !effect->targets_resolved;
        if (process) {

            if (effect->missiles_pending) {
                W8Missile* missile = 0;

                for (int missile_index = 0; missile_index < effect->missiles.GetCount();
                     ++missile_index) {
                    missile = *effect->missiles.GetAt(missile_index);
                    missile->block_released = 1;
                    effect->missiles.RemoveAt(missile_index);
                }
                if (missile != 0) {
                    srVector3T<float> position = missile->GetPosition();
                    position.y -= g_float_005ebc64;
                    W8SpellVisual* visual =
                        SpawnSpellEffect(&position, g_spell_records[effect->kind].resource_name,
                                         missile->definition.duration_scale, 0, 0);
                    if (visual != 0) {
                        visual->auto_release = false;
                        effect->spell_visuals.Add(visual);
                        alive = false;
                    }
                }
                effect->missiles_pending = false;
                handled = true;
            } else {
                effect->targets_resolved = true;
                if (MonsterCanAimSpell(effect->kind) && !effect->Source.fBackfire &&
                    !effect->Source.fReflection) {
                    ProvokeListedMonsterGroups(&effect->Source, &effect->monster_ids);
                }
                ProcessSpellEffectTargets(effect);
                if (TargetSourceIsMonster(&effect->OrigSource, 0)) {
                    if (effect->OrigSource.iMonsterID == -1) {
                        srAssertFail("pOrigSource->iMonsterID != -1", MAGIC_CPP, 0x1504, 0);
                    }
                    unsigned int monster_list_index = MonsterGetIndexByLocationID(
                        0x1505, MAGIC_CPP, effect->OrigSource.iMonsterID, true);
                    W8MonsterInfo* monster_info =
                        MonsterGetScriptPartByLocationIndex(monster_list_index);
                    if (gXStatus.fCombatMode && g_combat_state->eCombatActionStatus != 0 &&
                        g_combat_state->pActionMonsterInfo != 0 &&
                        g_combat_state->pActionMonsterInfo->location_id ==
                            monster_info->location_id) {
                        g_combat_state->eCombatActionStatus = 3;
                    }
                }
            }
        }

        if (effect->sustained) {
            if (effect->turns_remaining != 0) {
                continue;
            }
        } else if (!alive || !effect->targets_resolved) {
            continue;
        }

        if (!handled && g_spell_records[effect->kind].needs_aim != 0 &&
            !IsCombatEffectSlotSpell(effect->kind)) {
            W8SpellTargetType target_type = GetSpellTargetType(effect->kind, false);
            if (target_type != W8_TARGET_TYPE_RADIUS && target_type != W8_TARGET_TYPE_PARTY) {
                if (g_settings.verbose_combat_messages == 0) {
                    ReportSpellResult(effect);
                }
                FinishSpellEffectTargets(effect);
            }
        }
        if (effect->kind == 0x4f) {
            FinishSpellEffect(effect);
        }
        for (int release_visual = 0; release_visual < effect->spell_visuals.GetCount();
             ++release_visual) {
            W8SpellVisual* visual = *effect->spell_visuals.GetAt(release_visual);
            visual->auto_release = true;
            if (effect->sustained) {
                visual->finished = true;
            }
        }
        for (int release_missile = 0; release_missile < effect->missiles.GetCount();
             ++release_missile) {
            W8Missile* missile = *effect->missiles.GetAt(release_missile);
            missile->block_released = 1;
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
void DetachMissileReferences(W8Missile* missile)
{
    for (int index = 0; index < g_spell_effects.GetCount(); ++index) {
        W8SpellEffectEntry* effect = *g_spell_effects.GetAt(index);
        if (effect->missiles.Remove(missile)) {
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
bool IsTeleportCastMissingAnchor(W8Character* character, int spell_id)
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
void FinishSpellEffect(W8SpellEffectEntry* effect)
{
    W8MonsterInfo* monster_info = 0;
    W8CombatSlot target;
    srVector3T<float> position;

    if (effect->target.iType == W8_TARGET_KIND_MONSTER) {
        monster_info = MonsterInfoFromID(0x1279, MAGIC_CPP, effect->target.iMonsterID, true);
    }
    if ((effect->target.iType == W8_TARGET_KIND_MONSTER && monster_info->hp_current == 0) ||
        (effect->target.iType == W8_TARGET_KIND_CHARACTER &&
         g_status.buffers.Char[effect->target.iChar].hp_current == 0)) {
        target.iType = W8_TARGET_KIND_PLACE;
        if (effect->target.iType == W8_TARGET_KIND_MONSTER) {
            W8NavigatorMovementState* movement = &monster_info->p3D->movement;
            position = movement->position;
            position.y += movement->height_offset;
            position.y = SettlePositionToGround(&position, 0);
            PostMonsterNotice(monster_info, gppStringList[0x195]);
        } else {
            GetCameraPosition(&position);
            position.y -= g_default_world_height;
            PostCharacterNotice(effect->target.iChar, gppStringList[0x195]);
        }
        if (g_settings.verbose_combat_messages == 0) {
            SetTextBoxMode(1, -1);
        }
        target.point = position;
        CastSpellFromSource(0x76, &effect->Source, &target, effect->definition.duration_scale,
                            effect->definition.percent, 0, false, 0, 0, 0, 0);
    }
}

/* Scale a value by how far ahead of the difficulty's own pace one combatant
   is. Out of combat nothing is scaled; in combat a combatant slower than the
   pace is left alone too, and so is every value under an unknown difficulty. */
// FUNCTION: WIZ8 0x00501910
void ScaleByCombatPace(int party_slot, unsigned int* value)
{
    unsigned int pace;
    unsigned int phase_clock;

    if (!gXStatus.fCombatMode) {
        return;
    }

    switch (g_settings.difficulty) {
    case W8_DIFFICULTY_NOVICE:
        pace = 0x50;
        break;
    case W8_DIFFICULTY_NORMAL:
        pace = 0x3c;
        break;
    case W8_DIFFICULTY_EXPERT:
        pace = 0x28;
        break;
    default:
        srAssertFail("FALSE", MAGIC_CPP, 5352, 0);
        return;
    }

    phase_clock = g_combat_state->characters[party_slot].phase_clock_stamp;
    if (pace <= phase_clock) {
        *value = ((0x32 - pace) + phase_clock) * *value * 2 / 100;
    }
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
    needed =
        static_cast<unsigned int>(g_combat_effect_slot_spells_and_cast_success[6 + band] * factor) /
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
    W8PartySlotRow* row = &g_status.buffers.XChar[party_slot];
    W8CombatSlot saved_target = row->spell_target;
    W8ActionDetailBlock named;
    const W8ActionDetailBlock* target;

    if (!gXStatus.fCombatMode) {
        AimAtTarget(party_slot, &row->spell_target, W8_TARGETING_CONTEXT_CURRENT);
    }

    if (power_level == 0) {
        target = &row->spell_detail;
    } else {
        named.spell.power_level = power_level;
        named.spell.unused = 0;
        target = &named;
    }
    ChooseAction(party_slot, W8_ACTION_CAST_SPELL, row->spell_id, target, false, 1);
    AimAtTarget(party_slot, &saved_target, W8_TARGETING_CONTEXT_CURRENT);

    if (IsSpellTargetStillValidIn(party_slot, row->spell_id, W8_TARGETING_CONTEXT_SPELL)) {
        StartBreathCycle(party_slot, false);
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
    W8PartySlotRow* row = &g_status.buffers.XChar[party_slot];
    W8CombatSlot saved_target = row->item_target;

    if (!gXStatus.fCombatMode) {
        AimAtTarget(party_slot, &row->item_target, W8_TARGETING_CONTEXT_CURRENT);
    }
    ChooseAction(party_slot, W8_ACTION_USE_ITEM, -1, &row->item_detail, false, 1);
    AimAtTarget(party_slot, &saved_target, W8_TARGETING_CONTEXT_CURRENT);
    FindCharacterItemAt(party_slot, row->item_origin, row->item_slot);

    if (IsSpellTargetStillValidIn(party_slot, GetItemSpell(row->item_detail.item_use.item),
                                  W8_TARGETING_CONTEXT_ITEM)) {
        StartBreathCycle(party_slot, false);
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
int GetTotalCasterLevel(const W8Character* character, int spellbook, bool include_all)
{
    W8Profession profession = character->iProfession;
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
    if (total < 1 && !include_all) {
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

/* Which spellbooks a spell belongs to, as the mask the profession table is
   tested against. A spell in no book at all answers nothing, which is what
   makes the test below a membership test rather than a comparison. */
static unsigned char SpellbookMaskForSpell(int spell_id)
{
    return static_cast<unsigned char>(
        (g_spell_records[spell_id].wizardry_spell != 0) |
        (g_spell_records[spell_id].psionics_spell != 0 ? W8_SPELLBOOK_PSIONICS
                                                       : W8_SPELLBOOK_NONE) |
        (g_spell_records[spell_id].divinity_spell != 0 ? W8_SPELLBOOK_DIVINITY
                                                       : W8_SPELLBOOK_NONE) |
        (g_spell_records[spell_id].alchemy_spell != 0 ? W8_SPELLBOOK_ALCHEMY : W8_SPELLBOOK_NONE));
}

/* Recount the learned spells into the six per-realm slots (0x1c..0x21 of
   skill_unlocks), where the expert-skill gate and the spell-point ceiling
   both read them. */
// FUNCTION: WIZ8 0x004f96a0
void RecountLearnedSpellsByRealm(W8Character* character)
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
            return true;
        }
    }
    return false;
}

/* Learned, and the remaining points in the spell's realm cover its cost. */
// FUNCTION: WIZ8 0x004f9750
bool CanCharacterCastSpell(W8Character* character, int spell_id)
{
    if (spell_id != 0 && character->spell_learned[spell_id] == 1 &&
        g_spell_records[spell_id].spell_point_cost <=
            character->iSPLeft[g_spell_records[spell_id].realm]) {
        return true;
    }
    return false;
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
bool CanCharacterLearnSpell(W8Character* character, int spell_id)
{
    unsigned char book = SpellbookMaskForSpell(spell_id);
    int caster_level;
    int other;
    int other_level;
    unsigned int level_ceiling = 0;
    unsigned int skill_ceiling;
    unsigned int ceiling;
    unsigned int spell_level;
    W8Skill spellbook_skill;

    if ((g_profession_spellbooks[character->iProfession] & book) == W8_SPELLBOOK_NONE) {
        return false;
    }
    if (CharacterHasTrait(character, W8_TRAIT_CANNOT_LEARN)) {
        return false;
    }

    caster_level = GetProfessionCasterLevel(character, W8_PROFESSION_NONE);
    if (caster_level > 0) {
        for (other = 0; other < 15; ++other) {
            if (character->profession_levels[other] != 0 && other != character->iProfession &&
                (g_profession_spellbooks[other] & book) != W8_SPELLBOOK_NONE) {
                other_level = GetProfessionCasterLevel(character, static_cast<W8Profession>(other));
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
        if (static_cast<int>(spell_level - 1) < 0) {
            break;
        }
    }

    spellbook_skill = GetBestSpellbookSkillForSpell(character, spell_id, false, false, 0);
    skill_ceiling =
        (character->skills[W8_SKILL_FIRE_MAGIC + g_spell_records[spell_id].realm].points / 10 +
         character->skills[spellbook_skill].level) /
            15 +
        1;
    ceiling = skill_ceiling < level_ceiling ? skill_ceiling : level_ceiling;

    return ceiling >= static_cast<unsigned int>(g_spell_records[spell_id].spell_level);
}

/* 0x0068C09C: the loaded message table, one wide string per entry. Bodies
   name entries by their byte offset into it, which is why the index is
   spelled as one. */
/* One message-table index per realm, for the realm's name. */
// GLOBAL: WIZ8 0x0061E518
// offset alias of the tail of g_attr_table1; shared retail storage.
unsigned short g_realm_message_offsets[W8_SPELL_REALM_COUNT] = {
    0x30b, 0x30c, 0x30d, 0x30e, 0x30f, 0x310,
};

/* Take one spell on. The spell is marked known, its realm's known count goes
   up, the spell-point pools are recomputed, and - when the caller asks for it -
   the character says so in a line built from the character's name, the spell's
   name and the realm's remaining points.

   The line is assembled twice over: once only to measure the three pieces so
   the buffer can be allocated, and once into it. */
// FUNCTION: WIZ8 0x004ffe70
void LearnSpell(W8Character* character, int spell_id, bool announce)
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
    ++character->skill_unlocks[W8_SKILL_FIRE_MAGIC + realm];
    character->skill_unlocks[W8_SKILL_IRON_WILL] = RebuildRealmSpellPointCeilings(character);

    if (!announce) {
        return;
    }

    piece = FormatWideString(gppStringList[0x1b9], character->name);
    name_length = wcslen(piece);
    spell_length = wcslen(g_spell_records[spell_id].display_name);
    wcscpy(realm_name, gppStringList[g_realm_message_offsets[realm]]);
    piece = FormatWideString(gppStringList[0x1ba], realm_name, character->sp_max[realm]);
    points_length = wcslen(piece);

    line = new wchar_t[name_length + spell_length + 8 + points_length];
    if (line == 0) {
        srAssertFail("wTempMsg", MAGIC_CPP, 0xfdc, 0);
    }
    wcscpy(line, FormatWideString(gppStringList[0x1b9], character->name));
    wcscat(line, L": \"");
    wcscat(line, g_spell_records[spell_id].display_name);
    wcscat(line, L"\" - ");
    wcscat(line, FormatWideString(gppStringList[0x1ba], realm_name, character->sp_max[realm]));
    ShowNoticeLine(line, 0, true, false);
}

// GLOBAL: WIZ8 0x0068c510
int g_learn_sound = g_first_remapped_event + 16;
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
        ShowNoticeLine(FormatWideString(gppStringList[0x1bb], character->name), 0, true, false);
        return;
    }

    LearnSpell(character, spell_id, true);
    usage_points = g_spell_records[spell_id].spell_level;
    PracticeCharacterSkill(character, W8_SKILL_ARTIFACTS, usage_points * 2, false);
    PracticeCharacterSkill(
        character, static_cast<W8Skill>(W8_SKILL_FIRE_MAGIC + g_spell_records[spell_id].realm),
        usage_points, false);
    for (skill_id = W8_SKILL_SPELLBOOK_WIZARDRY; skill_id < W8_SKILL_FIRE_MAGIC; ++skill_id) {
        if (g_spell_records[spell_id].wizardry_spell != 0) {
            PracticeCharacterSkill(character, static_cast<W8Skill>(skill_id), usage_points, false);
        }
    }
    EmptyItemRecord(item, character, true);
    QueueCharacterEvent(character, g_learn_sound, 0, g_character_event_no_flags,
                        g_character_event_full_volume);
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
    const W8PartySlotRow* row = &g_status.buffers.XChar[party_slot];
    int spell_id = row->spell_id;
    int power_level = row->spell_detail.spell.power_level;

    if (spell_id == 0) {
        return false;
    }
    if (spell_id == W8_SPELL_CONDITIONAL && GetConditionRecordFlag(party_slot, 0)) {
        return false;
    }
    if (g_status.buffers.Char[party_slot].spell_learned[spell_id] != 1) {
        return false;
    }
    if (row->spell_detail.spell.power_level == W8_SPELL_POWER_AS_AFFORDABLE &&
        g_spell_records[spell_id].power_class == W8_SPELL_POWER_REPEAT_OUT_OF_COMBAT &&
        gXStatus.fCombatMode) {
        return false;
    }

    if (power_level == W8_SPELL_POWER_AS_AFFORDABLE) {
        power_level = 1;
    }
    if (g_spell_records[spell_id].spell_point_cost * power_level >
        g_status.buffers.Char[party_slot].iSPLeft[g_spell_records[spell_id].realm]) {
        return false;
    }
    if (!SpellUsableNow(spell_id, false)) {
        return false;
    }
    if (!gXStatus.fCombatMode &&
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
    const W8PartySlotRow* row = &g_status.buffers.XChar[party_slot];
    int spell_id = row->spell_id;
    int power_level = row->spell_detail.spell.power_level;
    int cost;

    if (spell_id == 0) {
        return 0;
    }
    if (spell_id == W8_SPELL_CONDITIONAL && GetConditionRecordFlag(party_slot, 0)) {
        return 0;
    }
    if (g_status.buffers.Char[party_slot].spell_learned[spell_id] != 1) {
        return 0;
    }
    if (row->spell_detail.spell.power_level == W8_SPELL_POWER_AS_AFFORDABLE &&
        g_spell_records[spell_id].power_class == W8_SPELL_POWER_REPEAT_OUT_OF_COMBAT &&
        gXStatus.fCombatMode) {
        return 0;
    }
    if (!SpellUsableNow(spell_id, false)) {
        return 0;
    }
    if (!gXStatus.fCombatMode &&
        !IsSpellTargetStillValidIn(party_slot, spell_id, W8_TARGETING_CONTEXT_SPELL)) {
        return 0;
    }

    if (power_level == W8_SPELL_POWER_AS_AFFORDABLE) {
        power_level = 1;
    }
    cost = g_spell_records[spell_id].spell_point_cost * power_level;
    while (power_level != 0) {
        if (cost <= g_status.buffers.Char[party_slot].iSPLeft[g_spell_records[spell_id].realm]) {
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
    W8PartySlotRow* row = &g_status.buffers.XChar[party_slot];
    W8ItemInstance* item;
    int spell_id;
    bool normalize;

    if (static_cast<signed char>(row->item_origin) < 0 || static_cast<short>(row->item_slot) < 0) {
        return false;
    }

    item = FindCharacterItemAt(party_slot, row->item_origin, row->item_slot);
    row->item_detail.item_use.item = item;

    if (row->item_origin == W8_ITEM_ORIGIN_PARTY_POOL && gXStatus.fCombatMode) {
        return false;
    }
    if (item == 0 || item->iItemNo == -1 || item->iItemNo != row->item_id) {
        return false;
    }
    if (!CanUseItemForAction(party_slot, item)) {
        return false;
    }
    if (!CanCharacterActivateItem(&g_status.buffers.Char[party_slot], item)) {
        return false;
    }

    spell_id = GetItemSpell(item);
    normalize = ItemClassNormalizesTarget(&g_item_records[item->iItemNo]);
    if (GetSpellTargetType(spell_id, normalize) == W8_TARGET_TYPE_CASTER &&
        row->item_target.iChar != party_slot) {
        return false;
    }
    if (!SpellUsableNow(spell_id, false)) {
        return false;
    }
    if (!gXStatus.fCombatMode &&
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

    if (spell_id == W8_MONSTER_SPELL_NONE) {
        return 0;
    }

    power_level = 1;
    for (;;) {
        W8MonsterRecord* record = GetMonsterDataForInfo(monster_info);

        budget = monster_info->spell_points + record->sp_budget;
        if (record->sp_budget == 0) {
            FormatDebugMessage(0, "DATA ERROR: Monster %ls casting spells with SP Budget of 0",
                               GetMonsterName(monster_info, 0, 0));
        }
        if (static_cast<int>(budget) < 0) {
            budget = 0;
        }

        band =
            g_spell_records[spell_id].spell_point_cost / 2 + g_spell_records[spell_id].spell_level;
        if (band > 16) {
            band = 16;
        }
        cost = (g_combat_effect_slot_spells_and_cast_success[6 + band] * power_level) / 7;

        if (cost > budget) {
            unsigned int shortfall = (cost * 70 - budget * 70) / cost;
            if (static_cast<int>(shortfall) >= 0 &&
                (static_cast<int>(shortfall) >= 0x65 || shortfall >= 9)) {
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

    switch (Random(4)) {
    case 0:
        if (power_level > 2) {
            --power_level;
        }
        break;
    case 1:
        if (power_level < W8_SPELL_POWER_MAX) {
            ++power_level;
        }
        break;
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
    bool sight_probe;

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
    switch (GetSpellTargetType(spell_id, false)) {
    case W8_TARGET_TYPE_CASTER:
    case W8_TARGET_TYPE_ALLY:
    case W8_TARGET_TYPE_ENEMY:
        target.iType = W8_TARGET_KIND_CHARACTER;
        target.iChar = GetRandomCharacter(0, 1, -1, -1);
        break;
    case W8_TARGET_TYPE_RADIUS:
    case W8_TARGET_TYPE_POINT:
        sight_probe = true;
        target.iType = W8_TARGET_KIND_PARTY;
        ResolveTargetPoint(&target, sight_probe);
        target.iType = W8_TARGET_KIND_PLACE;
        break;
    case W8_TARGET_TYPE_CONE:
        sight_probe = false;
        target.iType = W8_TARGET_KIND_PARTY;
        ResolveTargetPoint(&target, sight_probe);
        target.iType = W8_TARGET_KIND_PLACE;
        break;
    case W8_TARGET_TYPE_PARTY:
    case W8_TARGET_TYPE_ENEMY_GROUP:
        target.iType = W8_TARGET_KIND_PARTY;
        break;
    default:
        srAssertFail("FALSE", MAGIC_CPP, 0x56d,
                     FormatString("PointCastSpell: ERROR - Invalid spell target for %d", spell_id));
        break;
    }

    CastSpellFromSource(spell_id, &source, &target, power_level, 0, 0, false, 0, 0, 0, 0);
    return 1;
}

unsigned int GetSpellCastingSkillLevel(const W8Character* character, W8Skill spellbook_skill,
                                       W8SpellRealm realm)
{
    return (character->skills[spellbook_skill].level +
            character->skills[W8_SKILL_FIRE_MAGIC + realm].level * 4) /
           5;
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
W8Skill GetBestSpellbookSkillForSpell(W8Character* character, int spell_id, bool pricing,
                                      bool prefer_unlocked, unsigned int power_level)
{
    unsigned char book = SpellbookMaskForSpell(spell_id);
    unsigned char probe;
    unsigned int skill_id;
    W8Skill best_skill = W8_SKILL_NONE;
    unsigned int best_level = 0xffffffff;
    W8Skill unlocked_skill = W8_SKILL_NONE;
    unsigned int unlocked_level = 0xffffffff;
    unsigned int level;
    int party_slot;

    if (pricing && character->uiCondition[W8_CONDITION_SILENCED] != 0 &&
        g_spell_records[spell_id].alchemy_spell != 0) {
        return W8_SKILL_SPELLBOOK_ALCHEMY;
    }

    probe = 1;
    for (skill_id = W8_SKILL_SPELLBOOK_WIZARDRY; skill_id < W8_SKILL_FIRE_MAGIC; ++skill_id) {
        if ((probe & book) != 0) {
            level = character->skills[skill_id].points;
            if (static_cast<int>(best_level) < static_cast<int>(level)) {
                best_skill = static_cast<W8Skill>(skill_id);
                best_level = level;
            }
            if (prefer_unlocked && character->skills[skill_id].active &&
                static_cast<int>(unlocked_level) < static_cast<int>(level)) {
                unlocked_skill = static_cast<W8Skill>(skill_id);
                unlocked_level = level;
            }
        }
        probe = static_cast<unsigned char>(probe << 1);
    }

    if (prefer_unlocked && unlocked_skill != W8_SKILL_NONE && best_skill != unlocked_skill) {
        if (pricing) {
            unsigned int failure;
            int shortfall;
            unsigned int skill_figure;

            party_slot = CharacterPointerToPartySlot(character);
            skill_figure = GetSpellCastingSkillLevel(character, unlocked_skill,
                                                     g_spell_records[spell_id].realm);
            failure = GetSpellFailureChance(skill_figure, spell_id, static_cast<int>(power_level));

            shortfall = GetMinimumCasterLevelForSpell(spell_id) -
                        GetTotalCasterLevel(character, book, true) - 1 + power_level;
            if (shortfall > 0) {
                failure += g_spell_records[spell_id].spell_level * shortfall;
            }
            ScaleByCombatPace(party_slot, &failure);
            if (failure == 0) {
                best_skill = unlocked_skill;
            }
        } else {
            best_skill = unlocked_skill;
        }
    }

    if (best_skill == W8_SKILL_NONE) {
        srAssertFail("(iHighestSkill != SKILL_NONE)", MAGIC_CPP, 0xf29,
                     FormatString("Failed on spell %ld, usability byte %ld", spell_id, book));
    }
    return best_skill;
}

/* How safe one whole cast is, as the spell screen's one-to-five rating (five
   for a sure cast). Two things spoil it: a spellbook skill short of what the
   spell's cost band asks for at that power level, which the plain
   failure-chance body above answers, and a caster level short of what the
   spell asks for, charged flat at the spell's own level per level missing. The
   sum is scaled by how far ahead of the combat pace the caster is and banded
   at 0, 5, 15 and 40 percent.

   The skill this is measured against is the spell's own best spellbook skill
   weighted four to one against the realm skill, which is what makes the realm
   the larger part of it.

   Power level eight is the request to cast as high as affordable rather than a
   level, so it has no rating of its own. The power-level choosers carry the
   unbanded percentage inline (GetCastFailureChance) rather than calling this. */
// FUNCTION: WIZ8 0x004ff4b0
W8SpellCastRating GetSpellCastRating(W8Character* character, int spell_id, unsigned int power_level)
{
    W8Skill skill;
    int party_slot;
    unsigned int skill_figure;
    unsigned int chance;
    int shortfall;

    if (power_level == W8_SPELL_POWER_AS_AFFORDABLE) {
        return W8_CAST_RATING_AUTOMATIC;
    }

    skill = GetBestSpellbookSkillForSpell(character, spell_id, true, true, power_level);
    party_slot = CharacterPointerToPartySlot(character);
    skill_figure = GetSpellCastingSkillLevel(character, skill, g_spell_records[spell_id].realm);
    chance = GetSpellFailureChance(skill_figure, spell_id, static_cast<int>(power_level));

    shortfall = GetMinimumCasterLevelForSpell(spell_id) -
                GetTotalCasterLevel(character, SpellbookMaskForSpell(spell_id), true) - 1 +
                power_level;
    if (shortfall > 0) {
        chance += g_spell_records[spell_id].spell_level * shortfall;
    }
    ScaleByCombatPace(party_slot, &chance);
    if (chance == 0) {
        return W8_CAST_RATING_NO_FAILURE;
    }
    if (chance <= 5) {
        return W8_CAST_RATING_MINIMAL_RISK;
    }
    if (chance <= 15) {
        return W8_CAST_RATING_LOW_RISK;
    }
    if (chance <= 40) {
        return W8_CAST_RATING_MODERATE_RISK;
    }
    return W8_CAST_RATING_HIGH_RISK;
}

/* The average of one dice expression, taken as the midpoint of what it can
   roll: base plus the dice at one each, and base plus the dice at their
   faces. The die count is multiplied by the power level first, in a byte, so a
   high power level on a many-dice spell wraps rather than growing. */
static int AverageEffectAtPower(W8Dice dice, unsigned int power_level)
{
    unsigned char count =
        static_cast<unsigned char>(dice.count * static_cast<unsigned char>(power_level));

    return static_cast<int>(
        ((dice.base + count * dice.sides) + static_cast<float>(dice.base + count)) * 0.5f);
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

/* One cast's failure chance as the power-level choosers work it out: the
   chosen spellbook skill weighted four to one against the realm skill, priced
   against the spell's cost band, plus the spell's level for every caster level
   short of what it asks for, scaled by the caster's combat pace. Retail
   carries this sequence in both choosers and has no out-of-line copy. */
static unsigned int GetCastFailureChance(W8Character* character, int spell_id,
                                         unsigned int power_level)
{
    const W8SpellRuntimeRecord* record = &g_spell_records[spell_id];
    W8Skill skill = GetBestSpellbookSkillForSpell(character, spell_id, true, true, power_level);
    int party_slot = CharacterPointerToPartySlot(character);
    unsigned int skill_figure = GetSpellCastingSkillLevel(character, skill, record->realm);
    unsigned int chance;
    unsigned char book;
    int caster_level;
    int shortfall;

    chance = GetSpellFailureChance(skill_figure, spell_id, static_cast<int>(power_level));
    book = SpellbookMaskForSpell(spell_id);
    caster_level = GetTotalCasterLevel(character, book, true);
    shortfall = GetMinimumCasterLevelForSpell(spell_id) - caster_level - 1 + power_level;
    if (shortfall > 0) {
        chance += record->spell_level * shortfall;
    }
    ScaleByCombatPace(party_slot, &chance);
    return chance;
}

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
            static_cast<int>(g_spell_records[spell_id].spell_point_cost * power_level)) {
            return best_power;
        }

        failure = GetCastFailureChance(character, spell_id, power_level);

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
        failure = GetCastFailureChance(character, spell_id, power_level);
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
    W8Character* caster = &g_status.buffers.Char[party_slot];
    W8PartySlotRow* row = &g_status.buffers.XChar[party_slot];
    const unsigned int* conditions = 0;
    const W8Character* target_character = 0;
    unsigned int power_level;
    unsigned int worst;

    if (row->spell_target.iType == W8_TARGET_KIND_CHARACTER) {
        target_character = &g_status.buffers.Char[row->spell_target.iChar];
        conditions = target_character->uiCondition;
    } else if (row->spell_target.iType == W8_TARGET_KIND_MONSTER) {
        W8MonsterInfo* monster_info =
            MonsterInfoFromID(0xb83, MAGIC_CPP, row->spell_target.iMonsterID, true);
        conditions = monster_info->uiCondition;
    }

    switch (GetSpellTargetType(spell_id, false)) {
    case W8_TARGET_TYPE_ALLY:
        switch (spell_id) {
        case W8_SPELL_RESTORE_HP:
        case W8_SPELL_RESTORE_STAMINA:
        case W8_SPELL_RESTORE_HP_THEN_STAMINA:
            power_level = ChoosePowerLevelToRestore(caster, spell_id, target_character);
            break;
        case W8_SPELL_CURE_GROUP_A:
            worst = conditions[4];
            if (worst <= static_cast<unsigned int>(conditions[6])) {
                worst = conditions[6];
            }
            if (worst <= static_cast<unsigned int>(conditions[15])) {
                worst = conditions[15];
            }
            if (worst <= static_cast<unsigned int>(conditions[12])) {
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
        case W8_SPELL_CURE_GROUP_B:
            worst = conditions[11];
            if (worst <= static_cast<unsigned int>(conditions[13])) {
                worst = conditions[13];
            }
            if (worst <= static_cast<unsigned int>(conditions[15])) {
                worst = conditions[15];
            }
            power_level = ChoosePowerLevelForDuration(caster, spell_id, worst);
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
// offset alias of g_gender_name_message_rows; shared retail storage.
unsigned short g_name_prefix_messages[15] = {
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
        return &g_empty_wide_string;

    case 1:
    case 9:
        if (target->iChar == -1) {
            srAssertFail("pTarget->iChar != BAD_INDEX", MAGIC_CPP, 0xad, 0);
        }
        if (!TargetSourceIsCharacter(source, 0) || source->name_known != 0 ||
            source->iChar != target->iChar) {
            return FormatWideString(gppStringList[W8_MESSAGE_TARGET_AT / 4],
                                    g_status.buffers.Char[target->iChar].name);
        }
        name_prefix = g_name_prefix_messages[g_status.buffers.Char[target->iChar].gender * 4];
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
            MonsterGetIndexByLocationID(0xc0, MAGIC_CPP, target->iMonsterID, true));
        record = GetMonsterDataForInfo(monster_info);
        if (!TargetSourceIsMonster(source, 0) || source->iMonsterID != target->iMonsterID) {
            return FormatWideString(gppStringList[W8_MESSAGE_TARGET_AT / 4],
                                    GetMonsterName(monster_info, record, 0));
        }
        name_prefix = g_name_prefix_messages[record->name_group * 4];
        break;
    }

    case 4:
        return FormatWideString(
            gppStringList[W8_MESSAGE_TARGET_AT / 4],
            GetMonsterGroupName(GetMonsterGroupByListIndex(
                GetMonsterGroupIndexByID(0xcf, MAGIC_CPP, target->iGroupID, true))));

    case W8_TARGET_KIND_ALL_ENEMIES:
        return gppStringList[W8_MESSAGE_TARGET_PLACE / 4];

    case 7:
        return FormatWideString(gppStringList[W8_MESSAGE_TARGET_ITEM / 4],
                                FormatItemDisplayName(target->pPCItem, false));

    case W8_TARGET_KIND_EIGHT:
        return gppStringList[W8_MESSAGE_TARGET_DIRECTION / 4];

    default:
        srAssertFail(
            "FALSE", MAGIC_CPP, 0xde,
            FormatString("SpellTargetString: ERROR - Invalid target type %d", target->iType));
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

    if (g_settings.verbose_combat_messages == 0) {
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
    budget = monster_info->spell_points + record->sp_budget;
    if (record->sp_budget == 0) {
        FormatDebugMessage(0, "DATA ERROR: Monster %ls casting spells with SP Budget of 0",
                           GetMonsterName(monster_info, 0, 0));
    }
    if (static_cast<int>(budget) < 0) {
        budget = 0;
    }
    failure = GetSpellFailureChance(budget, spell_id, static_cast<int>(power_level));

    CastSpellFromSource(spell_id, &source, &monster_info->Target, power_level, 0, failure, false,
                        &result, 0, 0, 0);
    return SpellCastFatigueCost(spell_id, result);
}

/* The lure spell, and how far under the target the first of its two effects is
   placed. */
enum { W8_SPELL_LURE = 0x26 };

/* Put the lure's two effects a thousand units below the target. The target
   point itself moves down, so subsequent users see the lowered position.
   Only the named companion effect has its mode set. */
// FUNCTION: WIZ8 0x004fb360
void SpawnLureEffects(W8SpellEffectEntry* owner, int argument, W8CombatSlot* target)
{
    srVector3T<float> position;
    W8SpellVisual* effect;

    target->point.y -= 1000.0f;
    position = target->point;

    effect =
        SpawnSpellEffect(&position, g_spell_records[W8_SPELL_LURE].resource_name, argument, 0, 0);
    if (effect != 0) {
        effect->auto_release = false;
        owner->spell_visuals.Add(effect);
    }

    position = target->point;
    effect = SpawnSpellEffect(&position, "hyp_lure2", argument, 0, 0);
    if (effect != 0) {
        effect->auto_release = false;
        effect->host->pending_behaviour = 3;
        owner->spell_visuals.Add(effect);
    }
}

/* Condition names are the condition-notice table from +5, not a second
   initialized object at 0x0061E57A. */
static const unsigned short* const g_spell_condition_text = g_condition_notices + 5;

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
void ReportSpellResult(W8SpellEffectEntry* effect)
{
    const unsigned short* condition_text = g_spell_condition_text;

    if (GetTextBoxMode() != 0) {
        AppendToLastTextLine(!effect->reported ? L" -- " : L", ", -1);
        SetTextBoxMode(1, -1);
    }
    if (effect->result.amount != 0) {
        if (effect->result.count == 1) {
            AppendToLastTextLine(FormatWideString(gppStringList[0x19a], effect->result.amount),
                                 -1);
        } else {
            AppendToLastTextLine(
                FormatWideString(gppStringList[0x199], effect->result.count,
                                 effect->result.amount / effect->result.count, -1),
                -1);
            SetTextBoxMode(1, -1);
        }
        SetTextBoxMode(1, -1);
        effect->reported = true;
    }
    unsigned int* condition_count = &effect->result.condition_counts[1];

    do {
        if (*condition_count != 0) {
            if (effect->reported && GetTextBoxMode() != 0) {
                AppendToLastTextLine(L", ", -1);
                SetTextBoxMode(1, -1);
            }
            if (*condition_count == 1) {
                if (effect->target.iType == W8_TARGET_KIND_CHARACTER) {
                    AppendToLastTextLine(
                        FormatWideString(L"%s %s", g_status.buffers.Char[effect->target.iChar].name,
                                         gppStringList[condition_text[0]]),
                        -1);
                } else if (effect->target.iType == W8_TARGET_KIND_MONSTER) {
                    W8MonsterInfo* monster_info =
                        MonsterInfoFromID(0x112a, MAGIC_CPP, effect->target.iMonsterID, true);
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
            effect->reported = true;
        }
        condition_text += 4;
        ++condition_count;
    } while (condition_text < g_spell_condition_text + 76);

    while (effect->result.reports.GetCount() > 0) {
        W8SpellDamageReport* report = *effect->result.reports.GetAt(0);
        effect->result.reports.RemoveAt(0);
        if (report != 0) {
            if (report->kind == 1) {
                SetTextBoxMode(0, -1);
                PostCharacterNotice(
                    report->value, g_format_s_bang,
                    gppStringList[g_spell_condition_text[W8_CONDITION_UNCONSCIOUS * 4]]);
                effect->reported = true;
            } else if (report->kind == 3) {
                SetTextBoxMode(0, -1);
                ShowNoticef(9, L"%s %s!", report->text,
                            gppStringList[g_spell_condition_text[W8_CONDITION_UNCONSCIOUS * 4]]);
                effect->reported = true;
            }
            free(report);
        }
    }
    if (!effect->reported) {
        AppendToLastTextLine(gppStringList[0x1a5], -1);
    }
}

// FUNCTION: WIZ8 0x004fac40
bool ValidateSpellTarget(int party_slot, int spell_id, unsigned int power, bool item_cast,
                         bool skip_world_cursor)
{
    W8GrowableVector<int> monsters;
    W8GrowableVector<int> party;
    W8TargetSource source;
    SetTargetSourceToCharacter(party_slot, &source);
    PopulateSpellTargetMarkers(spell_id, power, &source,
                               &g_status.buffers.XChar[party_slot].target_out_of_combat, &monsters,
                               &party, 0);

    bool valid = true;
    bool has_targets = spell_id == 0x1e || monsters.count != 0 || party.count != 0;
    if (!has_targets) {
        W8SpellTargetType target_type = GetSpellTargetType(spell_id, false);
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
    } else if (!skip_world_cursor && DispatchWorldCursorNodeCommand(0, 4, 1)) {
        valid = false;
    }

    if (spell_id == 0x4b &&
        ((g_level_data->flags & 1) != 0 || !HasLevelWalkableContact() || LevelMovedThisUpdate())) {
        valid = false;
    }
    if (!valid && (!gXStatus.fCombatMode || !MonsterCanAimSpell(spell_id) ||
                   gXStatus.hostile_monster_count != 0 || !g_combat_state->enemies_engaged)) {
        QueueCharacterEvent(&g_status.buffers.Char[party_slot], g_character_event_kind2, 0,
                            g_character_event_no_flags, g_character_event_full_volume);
    }
    return valid;
}

/* The spells whose casts share one three-minute cooldown slot each; the
   shared slot lives in the status block's clock array. */
// GLOBAL: WIZ8 0x00616E34
int g_cooldown_gated_spells[14] = {30, 38, 75, 73, 32, 33, 17, 20, 8, 40, 26, 45, 64, 58};

/* Descriptive name for the cooldown operation expanded in SpellAffectedTarget.
   A failed check still restarts the slot, as in retail. */
static bool CheckAndRestartSpellCooldown(int spell_id)
{
    bool affected = true;
    for (unsigned int index = 0; index < 14; ++index) {
        if (g_cooldown_gated_spells[index] == spell_id) {
            if (ClockIsTicking(gXStatus.spell_cooldown_clocks[index]) != 0) {
                affected = false;
            }
            gXStatus.spell_cooldown_clocks[index] = SetCountdownClock(180000);
            break;
        }
    }
    return affected;
}

/* Whether the spell's current target would actually be affected by it - the
   per-spell rules the cast path checks before it spends the points. */
// FUNCTION: WIZ8 0x004F9AE0
bool SpellAffectedTarget(W8Character* character, int spell_id, W8CombatSlot* aim,
                                  unsigned int power)
{
    unsigned int duration;
    unsigned int index;
    const unsigned int* conditions;
    const W8Enchantment* enchantments;
    W8MonsterInfo* monster_info;
    const W8Character* target;
    bool affected;

    duration = g_spell_records[spell_id].duration * power +
               g_spell_records[spell_id].duration_per_level;
    affected = true;
    if (duration != 9999) {
        duration += 1;
    }
    switch (spell_id) {
    case 0x14:
    case 0x1a:
    case 0x20:
    case 0x28:
        if (!gXStatus.fCombatMode) {
            affected = CheckAndRestartSpellCooldown(spell_id);
        } else {
            for (index = 0; index < 12; ++index) {
                if (g_being_effect_slot_spells[index] == spell_id) {
                    affected = g_status.effect_slots[index].duration /
                                   static_cast<float>(duration) <=
                               g_navigator_vertical_phase_step;
                    break;
                }
            }
        }
        break;
    case 0x2:
    case 0x35:
    case 0x3b:
    case 0x3e:
        if (gXStatus.fCombatMode && gXStatus.hostile_monster_count > 0) {
            for (index = 0; index < 9; ++index) {
                if (g_combat_effect_slot_spells_and_cast_success[index] == spell_id) {
                    affected = g_combat_state->effect_slots0[index].duration /
                                   static_cast<float>(duration) <=
                               g_navigator_vertical_phase_step;
                    break;
                }
            }
            break;
        }
        affected = false;
        break;
    case 0x13:
    case 0x15:
    case 0x1b:
    case 0x36:
    case 0x3d:
    case 0x41:
        if (gXStatus.fCombatMode && gXStatus.hostile_monster_count > 0) {
            if (aim->iType == W8_TARGET_KIND_CHARACTER) {
                enchantments = g_status.buffers.Char[aim->iChar].enchantments;
            } else {
                monster_info = MonsterInfoFromID(0x18a, MAGIC_CPP, aim->iMonsterID, true);
                enchantments = monster_info->enchantments;
            }
            W8EnchantmentSlot enchantment = GetConditionDisplaySlot(spell_id);
            affected = enchantments[enchantment].turns / static_cast<float>(duration) <=
                       g_navigator_vertical_phase_step;
            break;
        }
        affected = false;
        break;
    case 0x38:
        if (gXStatus.fCombatMode && gXStatus.hostile_monster_count > 0) {
            if (aim->iType != W8_TARGET_KIND_PARTY) {
                return true;
            }
            affected = false;
            for (index = 0; index < 8; ++index) {
                if (g_status.buffers.XChar[index].fOccupied &&
                    g_status.buffers.Char[index].enchantments[W8_ENCHANTMENT_RAZOR_CLOAK].turns /
                            static_cast<float>(duration) <=
                        g_navigator_vertical_phase_step) {
                    affected = true;
                }
            }
            return affected;
        }
        affected = false;
        break;
    case 0x6:
        if (aim->iType == W8_TARGET_KIND_CHARACTER) {
            if (g_status.buffers.Char[aim->iChar].hp_current ==
                static_cast<unsigned int>(g_status.buffers.Char[aim->iChar].uiHPMax)) {
                return false;
            }
        } else {
            monster_info = MonsterInfoFromID(0x1d2, MAGIC_CPP, aim->iMonsterID, true);
            if (monster_info->hp_current == static_cast<unsigned int>(monster_info->uiHPMax)) {
                return false;
            }
        }
        break;
    case 0x44:
        for (index = 0; index < 8; ++index) {
            target = &g_status.buffers.Char[index];
            if (g_status.buffers.XChar[index].fOccupied &&
                target->highest_condition <= W8_CONDITION_DEAD &&
                target->hp_current < static_cast<unsigned int>(target->uiHPMax)) {
                return true;
            }
        }
        return false;
    case 0xd:
        if (gXStatus.fCombatMode) {
            if (aim->iType != W8_TARGET_KIND_CHARACTER) {
                monster_info = MonsterInfoFromID(0x1fc, MAGIC_CPP, aim->iMonsterID, true);
                return monster_info->stamina != monster_info->stamina_max;
            }
            return g_status.buffers.Char[aim->iChar].stamina !=
                   g_status.buffers.Char[aim->iChar].uiStaminaMax;
        }
        affected = false;
        break;
    case 0x2c:
        affected = false;
        if (gXStatus.fCombatMode) {
            for (index = 0; index < 8; ++index) {
                target = &g_status.buffers.Char[index];
                if (g_status.buffers.XChar[index].fOccupied &&
                    target->highest_condition <= W8_CONDITION_DEAD &&
                    static_cast<unsigned int>(target->stamina) <
                        static_cast<unsigned int>(target->uiStaminaMax)) {
                    return true;
                }
            }
            return false;
        }
        break;
    case 0x64:
        if (aim->iType == W8_TARGET_KIND_CHARACTER) {
            target = &g_status.buffers.Char[aim->iChar];
            for (index = 1; index < 0x12; ++index) {
                if (target->uiCondition[index] != 0) {
                    return true;
                }
            }
            return static_cast<unsigned int>(target->stamina) <
                       static_cast<unsigned int>(target->uiStaminaMax) ||
                   target->hp_current < static_cast<unsigned int>(target->uiHPMax);
        }
        monster_info = MonsterInfoFromID(0x232, MAGIC_CPP, aim->iMonsterID, true);
        for (index = 1; index < 0x12; ++index) {
            if (monster_info->uiCondition[index] != 0) {
                return true;
            }
        }
        return static_cast<unsigned int>(monster_info->stamina) <
                   static_cast<unsigned int>(monster_info->stamina_max) ||
               monster_info->hp_current < static_cast<unsigned int>(monster_info->uiHPMax);
    case 0x22:
        if (aim->iType == W8_TARGET_KIND_CHARACTER) {
            conditions = g_status.buffers.Char[aim->iChar].uiCondition;
        } else {
            monster_info = MonsterInfoFromID(0x250, MAGIC_CPP, aim->iMonsterID, true);
            conditions = monster_info->uiCondition;
        }
        if (conditions[0x10] == 0) {
            return false;
        }
        break;
    case 0x23:
        if (aim->iType == W8_TARGET_KIND_CHARACTER) {
            conditions = g_status.buffers.Char[aim->iChar].uiCondition;
        } else {
            monster_info = MonsterInfoFromID(0x261, MAGIC_CPP, aim->iMonsterID, true);
            conditions = monster_info->uiCondition;
        }
        if (conditions[7] == 0) {
            return false;
        }
        break;
    case 0x33:
        if (aim->iType == W8_TARGET_KIND_CHARACTER) {
            conditions = g_status.buffers.Char[aim->iChar].uiCondition;
        } else {
            monster_info = MonsterInfoFromID(0x272, MAGIC_CPP, aim->iMonsterID, true);
            conditions = monster_info->uiCondition;
        }
        if (conditions[2] == 0) {
            return false;
        }
        break;
    case 0x4a:
        if (aim->iType == W8_TARGET_KIND_CHARACTER) {
            conditions = g_status.buffers.Char[aim->iChar].uiCondition;
        } else {
            monster_info = MonsterInfoFromID(0x283, MAGIC_CPP, aim->iMonsterID, true);
            conditions = monster_info->uiCondition;
        }
        if (conditions[0xb] == 0 && conditions[0xd] == 0) {
            return false;
        }
        break;
    case 0x78:
        if (aim->iType == W8_TARGET_KIND_CHARACTER) {
            conditions = g_status.buffers.Char[aim->iChar].uiCondition;
        } else {
            monster_info = MonsterInfoFromID(0x294, MAGIC_CPP, aim->iMonsterID, true);
            conditions = monster_info->uiCondition;
        }
        if (conditions[1] == 0) {
            return false;
        }
        break;
    case 0x58:
        if (aim->iType == W8_TARGET_KIND_CHARACTER_INDIRECT) {
            conditions = g_status.buffers.Char[aim->iChar].uiCondition;
        } else {
            monster_info = MonsterInfoFromID(0x2a5, MAGIC_CPP, aim->iMonsterID, true);
            conditions = monster_info->uiCondition;
        }
        if (conditions[0x12] == 0) {
            return false;
        }
        break;
    case 0x10:
        if (aim->iType == W8_TARGET_KIND_CHARACTER) {
            conditions = g_status.buffers.Char[aim->iChar].uiCondition;
        } else {
            monster_info = MonsterInfoFromID(0x2b6, MAGIC_CPP, aim->iMonsterID, true);
            conditions = monster_info->uiCondition;
        }
        if (conditions[3] == 0 && conditions[4] == 0 && conditions[6] == 0 &&
            conditions[0xf] == 0 && conditions[0xc] == 0) {
            return false;
        }
        break;
    case 0x48:
        affected = false;
        if (g_combat_state != 0) {
            for (index = 0; index < 9; ++index) {
                if (g_combat_state->effect_slots[index].active) {
                    return true;
                }
            }
            return false;
        }
        break;
    case 0x1e:
    case 0x26:
        if (g_combat_state != 0 && gXStatus.hostile_monster_count > 0) {
            return true;
        }
        // fall through
    case 0x8:
    case 0x11:
    case 0x21:
    case 0x2d:
    case 0x40:
    case 0x49:
    case 0x4b:
        affected = CheckAndRestartSpellCooldown(spell_id);
        break;
    case 0x17:
        if (aim->pPCItem->identified) {
            return false;
        }
        break;
    case 0x3a:
        if (!gXStatus.fCombatMode) {
            affected = CheckAndRestartSpellCooldown(spell_id);
            if (!affected) {
                return false;
            }
        }
        if (aim->iType == W8_TARGET_KIND_CHARACTER) {
            conditions = g_status.buffers.Char[aim->iChar].uiCondition;
        } else {
            monster_info = MonsterInfoFromID(0x2f9, MAGIC_CPP, aim->iMonsterID, true);
            conditions = monster_info->uiCondition;
        }
        if (conditions[9] == 0 && (aim->iType != W8_TARGET_KIND_CHARACTER ||
                                   GetEquipmentBindingDifficulty(aim->iChar) == 0)) {
            affected = false;
        }
        break;
    }
    return affected;
}

/* Charge the realm spell points one cast costs and run the cast itself:
   notices, the success-chance figure, the point spend and the skill practice
   all happen here. Answers the cast step's status; out_points gets the
   fatigue cost. */
// FUNCTION: WIZ8 0x004FA4D0
int ExecuteCharacterSpellCast(int party_slot, int spell_id, unsigned int power_level,
                              int* out_points, bool continue_cast)
{
    W8Character* character;
    const W8SpellRuntimeRecord* record;
    W8TargetSource source;
    W8CombatSlot* aim;
    unsigned int sp_cost;
    int sp_needed;
    unsigned int chance;
    W8Skill best_skill;
    unsigned int skill_score;
    W8Skill realm_skill;
    int slot;
    W8SpellRealm realm;
    int level_index;
    int caster_level;
    int minimum_level;
    int power_cast_bonus;
    int spellbook;
    int profession;
    int index;
    unsigned int threshold;
    unsigned int morale;
    bool affected;
    int result;
    int cast_result;
    bool recast;
    const int* profession_level;
    bool clamp_power;

    character = &g_status.buffers.Char[party_slot];
    record = &g_spell_records[spell_id];
    realm = record->realm;
    sp_cost = record->spell_point_cost;
    *out_points = 0;
    switch (record->power_class) {
    case W8_SPELL_POWER_REPEAT_OUT_OF_COMBAT:
        if (!gXStatus.fCombatMode && power_level == 8) {
            recast = true;
            clamp_power = false;
            break;
        }
        // fall through
    case W8_SPELL_POWER_SELECTABLE:
        recast = false;
        clamp_power = true;
        break;
    case W8_SPELL_POWER_MAXIMUM:
        recast = false;
        clamp_power = power_level != 8;
        break;
    case W8_SPELL_POWER_FIXED:
        power_level = 1;
        recast = false;
        clamp_power = false;
        break;
    default:
        clamp_power = continue_cast;
        break;
    }
    if (clamp_power) {
        sp_needed = sp_cost * power_level;
        if (sp_needed > character->iSPLeft[realm]) {
            power_level = character->iSPLeft[realm] / sp_cost;
        }
    } else if (character->iSPLeft[realm] < static_cast<int>(sp_cost)) {
        PostCharacterNotice(party_slot, gppStringList[0x18a], record->display_name);
        return 0;
    }
    if (power_level == 0) {
        PostCharacterNotice(party_slot, gppStringList[0x18a], record->display_name);
        return 0;
    }
    if (!ValidateSpellTarget(party_slot, spell_id, power_level, false, false)) {
        return 0;
    }
    if (character->uiCondition[W8_CONDITION_SILENCED] != 0 &&
        (record->alchemy_spell == 0 || character->skills[W8_SKILL_SPELLBOOK_ALCHEMY].level == 0)) {
        return 0;
    }
    SetTargetSourceToCharacter(party_slot, &source);
    if (!character->skills[W8_SKILL_POWER_CAST].active) {
        power_cast_bonus = 0;
    } else {
        power_cast_bonus = (character->skills[W8_SKILL_POWER_CAST].level >> 2) + 1;
    }
    if (power_level == 8) {
        power_level = ChooseSpellPowerLevelForTarget(party_slot, spell_id, power_cast_bonus);
        if (power_level == 0) {
            return 0;
        }
        sp_needed = sp_cost * power_level;
        if (sp_needed > character->iSPLeft[realm]) {
            power_level = character->iSPLeft[realm] / sp_cost;
        }
    }
    aim = &g_status.buffers.XChar[party_slot].target_out_of_combat;
    if (g_settings.verbose_combat_messages == 0) {
        if (!continue_cast) {
            PostCharacterNotice(party_slot, gppStringList[0x18c], record->display_name,
                                SpellTargetString(&source, aim));
            SetTextBoxMode(1, 8);
        } else {
            PostCharacterNotice(party_slot, gppStringList[0x18d]);
            SetTextBoxMode(1, 8);
        }
    } else {
        PostCharacterNotice(party_slot, gppStringList[0x18b], record->display_name, power_level,
                            SpellTargetString(&source, aim));
    }
    best_skill = GetBestSpellbookSkillForSpell(character, spell_id, true, true, power_level);
    realm_skill = static_cast<W8Skill>(realm + 0x1c);
    slot = CharacterPointerToPartySlot(character);
    level_index = record->spell_point_cost / 2 + record->spell_level;
    skill_score = GetSpellCastingSkillLevel(character, best_skill, realm);
    if (0x10 < level_index) {
        level_index = 0x10;
    }
    threshold = (g_combat_effect_slot_spells_and_cast_success[level_index + 6] * power_level) / 7;
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
    caster_level = GetProfessionCasterLevel(character, W8_PROFESSION_NONE);
    spellbook = (record->psionics_spell != 0 ? 8U : 0U) | (record->divinity_spell != 0 ? 2U : 0U) |
                (record->wizardry_spell != 0 ? 1U : 0U) | (record->alchemy_spell != 0 ? 4U : 0U);
    profession_level = character->profession_levels;
    profession = W8_PROFESSION_FIGHTER;
    do {
        if (*profession_level != 0 && profession != character->iProfession &&
            (g_profession_spellbooks[profession] & spellbook) != 0 &&
            (index = GetProfessionCasterLevel(character, static_cast<W8Profession>(profession)),
             0 < index)) {
            caster_level += index;
        }
        ++profession;
        ++profession_level;
    } while (profession < W8_PROFESSION_COUNT);
    index = (minimum_level - caster_level) - 1 + power_level;
    if (0 < index) {
        chance += record->spell_level * index;
    }
    if (gXStatus.fCombatMode) {
        if (g_settings.difficulty == W8_DIFFICULTY_NOVICE) {
            morale = 0x50;
        } else if (g_settings.difficulty == W8_DIFFICULTY_NORMAL) {
            morale = 0x3c;
        } else {
            if (g_settings.difficulty != W8_DIFFICULTY_EXPERT) {
                srAssertFail("FALSE", MAGIC_CPP, 0x14e8, 0);
                goto finish_difficulty_adjustment;
            }
            morale = 0x28;
        }
        threshold = g_combat_state->characters[slot].phase_clock_stamp;
        if (morale <= threshold) {
            chance = (((0x32 - morale) + threshold) * chance * 2) / 100;
        }
    }
finish_difficulty_adjustment:
    if (spell_id == 0x4a && aim->iType == W8_TARGET_KIND_CHARACTER && aim->iChar == party_slot) {
        chance += 0x32;
    }
    index = 0;
    do {
        if (index != 0 && character->spell_learned[index] == 1 &&
            g_spell_records[index].spell_point_cost <=
                character->iSPLeft[g_spell_records[index].realm] &&
            SpellUsableNow(index, false)) {
            ++g_status.spell_usage[index - 1].usable_cast_count;
        }
        ++index;
    } while (index < 0x72);
    ++g_status.spell_usage[spell_id - 1].cast_count;
    affected = SpellAffectedTarget(character, spell_id, aim, power_level);
    if (!continue_cast) {
        if (Random(2) != 0) {
            index = g_special_event6;
        } else {
            index = g_special_event5;
        }
        QueueCharacterEvent(character, index, 0, g_character_event_no_flags,
                            g_character_event_full_volume);
    }
    result = CastSpellFromSource(spell_id, &source, aim, power_level, power_cast_bonus, chance,
                                 recast, &cast_result, 0, 0, 0);
    if (cast_result != 0) {
        SpendCharacterSpellPoints(party_slot, realm, cast_result * sp_cost);
        if (affected) {
            index = record->spell_level + cast_result;
            if (index < 2) {
                srAssertFail("uiUsagePoints >= 2", MAGIC_CPP, 0x3d5, 0);
            }
            PracticeCharacterSkill(character, best_skill, (index + 2) >> 2, false);
            PracticeCharacterSkill(character, realm_skill, index, false);
            if (character->skills[W8_SKILL_POWER_CAST].active) {
                PracticeCharacterSkill(character, W8_SKILL_POWER_CAST, (index + 2) >> 2, false);
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
                        unsigned int power_level, int power_cast_bonus, unsigned int failure_chance,
                        bool recast, int* out_power, int cast_kind,
                        W8GrowableVector<int>* party_targets,
                        W8GrowableVector<int>* monster_targets)
{
    srVector3T<float> point;
    bool fizzled;
    bool quiet;
    bool forced;
    bool icon_flag;
    W8SpellEffectEntry* owner;
    W8MonsterRecord* record_data;
    W8Missile* missile;
    W8Monster* monster;
    W8SpellVisual* visual;
    W8MonsterInfo* monster_info;
    unsigned int backfire_chance;
    int message;
    unsigned int floor_power;
    unsigned int roll;
    int caster_slot;
    int index;
    W8SpellTargetType target_type;
    int missile_index;
    W8GrowableVector<int> monster_markers;
    W8GrowableVector<int> party_markers;
    W8SpellEffectDefinition block;
    W8CombatSlot point_target;
    srVector3T<float> ground_point;
    srVector3T<float> found_point;
    srVector3T<float> forward_point;
    srVector3T<float> origin;
    srVector3T<float> direction;
    srVector3T<float> axis;
    srMatrix3T<float> local_90;
    double yaw;
    double pitch;
    char disposition;
    float heading;
    unsigned int caster_figure;
    unsigned int monster_index;
    unsigned int adjusted_power;

    fizzled = false;
    quiet = cast_kind == 4;
    forced = false;
    if (out_power != 0) {
        *out_power = 0;
    }
    owner = new W8SpellEffectEntry;
    if (owner == 0) {
        return 0;
    }
    owner->OrigSource = *source;
    owner->OrigTarget = *target;
    if (TargetSourceIsCharacter(source, 0)) {
        caster_slot = source->iChar;
    } else {
        caster_slot = -1;
    }
    source->fBackfire = false;
    if (g_force_spell_failure && spell_id != 0x76) {
        forced = true;
        failure_chance = 100;
    }
    if (g_spell_records[spell_id].realm == W8_SPELL_REALM_FIRE && g_camera_sway_active) {
        failure_chance = 100;
    }
    if (quiet && !g_force_spell_failure) {
        failure_chance /= 2;
    }
    CombatLog("Chance of FAILURE: %d", failure_chance);
    CombatLog("");
    if (failure_chance != 0 && (roll = Random(100)) < failure_chance) {
        backfire_chance = failure_chance;
        if (g_spell_records[spell_id].realm == W8_SPELL_REALM_FIRE && g_camera_sway_active) {
            backfire_chance = 0;
        } else if (!quiet && !forced) {
            if (backfire_chance < 6) {
                backfire_chance = 0;
            } else {
                backfire_chance /= 3;
            }
        }
        if (!CanSpellBackfire(spell_id)) {
            backfire_chance = 0;
        }
        CombatLog("Chance of BACKFIRE: %d (roll %d)", backfire_chance, roll);
        CombatLog("");
        if (roll < backfire_chance) {
            PrepareSpellTarget(spell_id, source, target);
            source->fBackfire = true;
            recast = false;
            SoundPlay("Data\\Sound\\Misc\\Spell Backfire.wav", 0);
        } else {
            fizzled = true;
            SoundPlay("Data\\Sound\\Misc\\Spell Fizzle 01.wav", 0);
            if (caster_slot != -1 && Random(100) < 0x46) {
                QueueCharacterEvent(&g_status.buffers.Char[caster_slot], g_special_event13,
                                    0, g_character_event_no_flags, g_character_event_full_volume);
            }
        }
    }
    if (!fizzled) {
        if (!source->fBackfire && source->auto_cast == 0 && !source->fReflection &&
            g_spell_records[spell_id].realm != W8_SPELL_REALM_MENTAL && spell_id != 0x83) {
            CheckSpellBackfire(spell_id, source, target);
        }
        if (party_targets == 0) {
            if (monster_targets == 0) {
                PopulateSpellTargetMarkers(spell_id, power_level, source, target, &monster_markers,
                                           &party_markers, 0);
                if (spell_id == 0x3c) {
                    ground_point.y = target->point.y - g_float_005ebc64;
                    target->point.y = ground_point.y;
                    ground_point.x = target->point.x;
                    ground_point.z = target->point.z;
                    if ((TargetSourceIsCharacter(source, 0) && !source->fBackfire) ||
                        (!TargetSourceIsCharacter(source, 0) && source->fBackfire)) {
                        disposition = 2;
                    } else {
                        disposition = 1;
                    }
                    heading = HeadingTowardNearestMonster(ground_point, disposition, 0);
                    if (g_octree->FindNavigatorPosition(
                            &ground_point, heading,
                            static_cast<float>(
                                g_world_cursor_extent_table
                                    [g_spell_power_extent_index[power_level - 1] * 6 + 3]) +
                                g_world_scale,
                            1, &found_point, true, false, false, 5, true) != 0) {
                        target->point = found_point;
                    }
                }
            } else {
                monster_markers = *monster_targets;
            }
        } else {
            party_markers = *party_targets;
            if (monster_targets != 0) {
                monster_markers = *monster_targets;
            }
        }
        ClearAttackBlock(&block);
        if (TargetSourceIsCharacter(source, 1)) {
            if (source->name_known == 0) {
                caster_figure = GetTotalCasterLevel(
                    &g_status.buffers.Char[source->iChar],
                    (g_spell_records[spell_id].psionics_spell != 0 ? 8 : 0) |
                        (g_spell_records[spell_id].divinity_spell != 0 ? 2 : 0) |
                        (g_spell_records[spell_id].wizardry_spell != 0 ? 1 : 0) |
                        (g_spell_records[spell_id].alchemy_spell != 0 ? 4 : 0),
                    true);
                caster_figure = GetSpellDifficulty(caster_figure, spell_id, power_level);
            } else {
                caster_figure = source->spell_difficulty;
            }
        } else if (TargetSourceIsMonster(source, 1)) {
            monster_index = MonsterGetIndexByLocationID(0x6ad, MAGIC_CPP, source->iMonsterID, true);
            monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
            record_data = GetMonsterDataForInfo(monster_info);
            caster_figure =
                GetSpellDifficulty(record_data->effective_level, spell_id, power_level);
        } else {
            if (source->iType != W8_TARGET_SOURCE_INDIRECT) {
                srAssertFail("pSource->iType == SOURCE_TYPE_3D_POINT", MAGIC_CPP, 0x6b2, 0);
            }
            caster_figure = (g_spell_records[spell_id].spell_point_cost / 2 +
                             g_spell_records[spell_id].spell_level) /
                                2 +
                            power_level;
            if (GetSpellTargetType(spell_id, false) == 7) {
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
        AdjustIntegerByPercent(&adjusted_power, static_cast<unsigned int>(power_cast_bonus) >> 1);
        block.power_level = adjusted_power;
        if (IsCombatEffectSlotSpell(spell_id)) {
            SetDice(&block.magnitude, 0, 0, 0);
        } else {
            block.magnitude = g_spell_records[spell_id].effect_dice;
            block.magnitude.count = static_cast<char>(power_level) * block.magnitude.count;
        }
        block.duration_scale = power_level;
        block.percent = power_cast_bonus;
        block.duration_base = g_spell_records[spell_id].duration_per_level;
        block.duration_per_power = g_spell_records[spell_id].duration;
        if (g_spell_records[spell_id].needs_aim != 0) {
            missile_index = g_spell_records[spell_id].missile_index;
            if (GetSpellTargetType(spell_id, false) == 6) {
                ResetCombatSlot(&point_target);
                point_target.point.Set(target->point.x,
                                       g_default_world_height * g_float_005ebc7c + target->point.y,
                                       target->point.z);
                point_target.iType = W8_TARGET_KIND_PLACE;
                missile = FireMissileSourceToTarget(missile_index, source, &point_target, &block,
                                                    true, W8_RANGE_NONE, 9999);
                if (missile != 0) {
                    owner->missiles.Add(missile);
                }
                owner->missiles_pending = true;
            } else {
                memcpy(block.condition_chances,
                       g_missile_table[missile_index].condition_chances, 0x10);
                block.magnitude_base = g_missile_table[missile_index].magnitude_base;
                for (index = 0; index < monster_markers.GetCount(); ++index) {
                    ResetCombatSlot(&point_target);
                    point_target.iType = W8_TARGET_KIND_MONSTER;
                    point_target.iMonsterID = *monster_markers.GetAt(index);
                    if (!source->fBackfire && !source->fReflection) {
                        MakeTargetGroupHostile(source, &point_target);
                    }
                    missile = FireMissileSourceToTarget(
                        missile_index, source, &point_target, &block, true,
                        g_spell_records[spell_id].range_category, 9999);
                    if (missile != 0) {
                        owner->missiles.Add(missile);
                    }
                }
                for (index = 0; index < party_markers.GetCount(); ++index) {
                    ResetCombatSlot(&point_target);
                    point_target.iType = W8_TARGET_KIND_CHARACTER;
                    point_target.iChar = *party_markers.GetAt(index);
                    missile = FireMissileSourceToTarget(
                        missile_index, source, &point_target, &block, true,
                        g_spell_records[spell_id].range_category, 9999);
                    if (missile != 0) {
                        owner->missiles.Add(missile);
                    }
                }
            }
        } else {
            target_type = GetSpellTargetType(spell_id, false);
            if (spell_id == 0x31) {
                target_type = W8_TARGET_TYPE_ENEMY;
            }
            switch (target_type) {
            case W8_TARGET_TYPE_RADIUS:
                if (spell_id == 0x76) {
                    if (party_markers.GetCount() == 0) {
                        point.y = target->point.y - g_float_005ebc64;
                        target->point.y = point.y;
                        point.x = target->point.x;
                        point.z = target->point.z;
                        visual =
                            SpawnSpellEffect(&point, g_spell_records[0x76].resource_name, 1, 0, 0);
                    } else {
                        GetCameraForwardPoint(1.0, &forward_point);
                        forward_point.y -= g_default_world_height;
                        visual = SpawnSpellEffect(&forward_point,
                                                  g_spell_records[0x76].resource_name, 2, 0, 0);
                    }
                } else {
                    point.y = target->point.y - g_float_005ebc64;
                    target->point.y = point.y;
                    point.x = target->point.x;
                    point.z = target->point.z;
                    visual = SpawnSpellEffect(&point, g_spell_records[spell_id].resource_name,
                                              power_level, 0, 0);
                }
                if (visual != 0) {
                    visual->auto_release = false;
                    owner->spell_visuals.Add(visual);
                }
                break;
            case W8_TARGET_TYPE_CONE:
                if (TargetSourceIsCharacter(source, 0)) {
                    visual = CreateAttachedSpellEffect(g_spell_records[spell_id].resource_name,
                                                       power_level, 0, 0, 0);
                    if (visual != 0) {
                        visual->auto_release = false;
                        visual->fixed_transform = true;
                        owner->spell_visuals.Add(visual);
                    }
                } else {
                    if (TargetSourceIsMonster(source, 0)) {
                        visual = CreateAttachedSpellEffect(
                            g_spell_records[spell_id].resource_name, power_level,
                            GetMonsterByLocationID(source->iMonsterID), 0, 0);
                    } else {
                        origin = source->point;
                        direction = target->point - origin;
                        direction.SetLength(1.0);
                        local_90.SetIdentity();
                        yaw = atan2(direction.x, direction.z);
                        local_90.RotateAboutY(yaw);
                        direction = local_90.TransformTransposed(direction);
                        pitch = -atan2(direction.y, direction.z);
                        local_90.RotateAboutX(pitch);
                        visual = CreateAimedSpellEffect(g_spell_records[spell_id].resource_name,
                                                        power_level, &origin, &local_90, 0, 0);
                    }
                    if (visual != 0) {
                        visual->auto_release = false;
                        owner->spell_visuals.Add(visual);
                    }
                }
                break;
            case W8_TARGET_TYPE_POINT:
                if (spell_id == 0x26) {
                    SpawnLureEffects(owner, power_level, target);
                } else {
                    if (spell_id != 0x3c) {
                        target->point.y -= g_float_005ebc64;
                    }
                    point = target->point;
                    visual = SpawnSpellEffect(&point, g_spell_records[spell_id].resource_name,
                                              power_level, 0, 0);
                    if (visual != 0) {
                        visual->auto_release = false;
                        owner->spell_visuals.Add(visual);
                    }
                }
                break;
            case W8_TARGET_TYPE_CASTER:
            case W8_TARGET_TYPE_ALLY:
            case W8_TARGET_TYPE_ENEMY:
                if (spell_id != 0x4b && spell_id != 0x49) {
                    for (index = 0; index < monster_markers.GetCount(); ++index) {
                        monster = GetMonsterByLocationID(*monster_markers.GetAt(index));
                        visual = CreateMonsterSpellEffect(g_spell_records[spell_id].resource_name,
                                                          power_level, monster, 0, 0);
                        if (visual != 0) {
                            visual->auto_release = false;
                            owner->spell_visuals.Add(visual);
                        }
                    }
                    if (party_markers.GetCount() != 0) {
                        for (index = 0; index < party_markers.GetCount(); ++index) {
                            if (MonsterCanAimSpell(spell_id)) {
                                icon_flag = source->fBackfire;
                            } else {
                                icon_flag = !source->fBackfire;
                            }
                            StageMonsterCastIcon(*party_markers.GetAt(index),
                                                 g_spell_records[spell_id].realm, icon_flag,
                                                 spell_id);
                        }
                    }
                } else {
                    visual = SpawnCameraSpellEffect(g_spell_records[spell_id].resource_name,
                                                    power_level, 0, 0);
                    if (visual != 0) {
                        visual->auto_release = false;
                        owner->spell_visuals.Add(visual);
                    }
                }
                break;
            case W8_TARGET_TYPE_PARTY:
            case W8_TARGET_TYPE_ENEMY_GROUP:
            case W8_TARGET_TYPE_ALL_ENEMIES:
                if (spell_id == 0x5f) {
                    break;
                }
                if (spell_id == 0x62) {
                    point.y = target->point.y - g_float_005ebc64;
                    target->point.y = point.y;
                    point.x = target->point.x;
                    point.z = target->point.z;
                    visual = SpawnSpellEffect(&point, g_spell_records[0x62].resource_name,
                                              power_level, 0, 0);
                    if (visual != 0) {
                        visual->auto_release = false;
                        owner->spell_visuals.Add(visual);
                    }
                    break;
                }
                for (index = 0; index < monster_markers.GetCount(); ++index) {
                    monster = GetMonsterByLocationID(*monster_markers.GetAt(index));
                    visual = CreateMonsterSpellEffect(g_spell_records[spell_id].resource_name,
                                                      power_level, monster, 0, 0);
                    if (visual != 0) {
                        visual->auto_release = false;
                        owner->spell_visuals.Add(visual);
                    }
                }
                if (party_markers.GetCount() != 0) {
                    visual = SpawnCameraSpellEffect(g_spell_records[spell_id].resource_name,
                                                    power_level, 0, 0);
                    if (visual != 0) {
                        visual->auto_release = false;
                        owner->spell_visuals.Add(visual);
                    }
                }
                break;
            case W8_TARGET_TYPE_LOCK_OR_TRAP:
                visual = SpawnCameraSpellEffect(g_spell_records[spell_id].resource_name,
                                                power_level, 0, 0);
                if (visual != 0) {
                    visual->auto_release = false;
                    owner->spell_visuals.Add(visual);
                }
                break;
            default:
                SpawnCameraSpellEffect("Default", 0, 0, 0);
                SoundPlay("Data\\Sound\\Misc\\GeneralMagic.wav", 0);
                break;
            case W8_TARGET_TYPE_ITEM:
            case W8_TARGET_TYPE_NONE:
            case W8_TARGET_TYPE_COUNT:
                break;
            }
        }
        owner->kind = spell_id;
        owner->Source = *source;
        owner->target = *target;
        owner->definition = block;
        owner->monster_ids = monster_markers;
        owner->target_indices = party_markers;
        owner->recast = recast;
        if (spell_id == 0x26) {
            owner->sustained = true;
            owner->turns_remaining = RollEffectDuration(&owner->definition);
            for (index = 0; index < g_spell_effects.GetCount(); ++index) {
                W8SpellEffectEntry* previous = *g_spell_effects.GetAt(index);
                if (previous->kind == 0x26) {
                    if (previous != 0) {
                        previous->turns_remaining = 0;
                        for (monster_index = 0; monster_index < PLLength(gXStatus.plsMonsterList);
                             ++monster_index) {
                            monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
                            if (monster_info != 0 && !monster_info->p3D->IsDying()) {
                                SetMonsterControlState(monster_info, W8_MONSTER_CONTROL_NONE);
                            }
                        }
                    }
                    break;
                }
            }
        }
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && !gXStatus.fNpcDialogueMode &&
            !gXStatus.fCampMode) {
            g_spell_effects.Add(owner);
        } else {
            ProcessSpellEffectTargets(owner);
            if (gXStatus.fNpcDialogueMode || gXStatus.fCampMode) {
                for (index = 0; index < owner->spell_visuals.GetCount(); ++index) {
                    (*owner->spell_visuals.GetAt(index))->auto_release = '\x01';
                }
            }
        }
        PointCameraAtCombatTarget(source, target);
        if (source->fBackfire) {
            if (quiet) {
                owner->reported = true;
            } else if (g_settings.verbose_combat_messages != 0) {
                ShowNotice(0xc, FormatWideString(gppStringList[0x197]));
            } else {
                AppendToLastTextLine(FormatWideString(L" -- %s", gppStringList[0x197]), -1);
                owner->reported = true;
            }
            if (caster_slot != -1) {
                if (Random(2) == 0) {
                    QueueCharacterEvent(&g_status.buffers.Char[caster_slot],
                                        g_item_message0, 0, g_character_event_no_flags,
                                        g_character_event_full_volume);
                } else {
                    ApplyItemEffectToRandomCharacter(g_item_message1, caster_slot, 0,
                                                     g_character_event_no_flags);
                }
            }
        }
    } else {
        message = 0;
        switch (cast_kind) {
        case 2:
            if (source->iType != W8_TARGET_SOURCE_CHARACTER) {
                srAssertFail("pSource->iType == SOURCE_TYPE_CHAR", MAGIC_CPP, 0x8c2, 0);
            }
            message = 0x1aa;
            // fall through
        case 3:
            if (source->iType != W8_TARGET_SOURCE_CHARACTER) {
                srAssertFail("pSource->iType == SOURCE_TYPE_CHAR", MAGIC_CPP, 0x8c6, 0);
            }
            if (cast_kind == 3) {
                message = 0x1ab;
            }
            if (g_settings.verbose_combat_messages != 0) {
                PostCharacterNotice(source->iChar, FormatWideString(gppStringList[message]));
            } else {
                AppendToLastTextLine(FormatWideString(L" -- %s", gppStringList[message]), -1);
            }
            break;
        default:
            if (g_settings.verbose_combat_messages != 0) {
                ShowNotice(0xc, FormatWideString(gppStringList[0x196]));
            } else {
                AppendToLastTextLine(FormatWideString(L" -- %s", gppStringList[0x196]), -1);
            }
            break;
        }
    }
    if (out_power != 0) {
        *out_power = power_level;
    }
    if (fizzled) {
        if (TargetSourceIsMonster(&owner->Source, 0)) {
            if (owner->Source.iMonsterID == -1) {
                srAssertFail("pOrigSource->iMonsterID != -1", MAGIC_CPP, 0x1504, 0);
            }
            monster_index =
                MonsterGetIndexByLocationID(0x1505, MAGIC_CPP, owner->Source.iMonsterID, true);
            monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
            if (gXStatus.fCombatMode && g_combat_state->eCombatActionStatus != 0 &&
                g_combat_state->pActionMonsterInfo != 0 &&
                g_combat_state->pActionMonsterInfo->location_id == monster_info->location_id) {
                g_combat_state->eCombatActionStatus = 3;
            }
        }
        delete owner;
        return 2;
    }
    if (!source->fBackfire) {
        return 1;
    }
    return 3;
}

/* A backfiring spell swaps roles: the intended target becomes the source and
   the concrete thing to aim back at is resolved here - the caster for a
   character target, the first live party member for a party target, and so
   on. */
// FUNCTION: WIZ8 0x004FE740
void RedirectBackfiredSpellTarget(W8TargetSource* source, W8CombatSlot* target)
{
    W8TargetSource source_copy;
    W8CombatSlot target_copy;
    W8MonsterInfo* monster_info;
    W8MonsterGroup* group;
    unsigned int list_index;
    int slot;

    if (source->fBackfire) {
        srAssertFail("!pSource->fBackfire", MAGIC_CPP, 0xbf7, 0);
    }
    source_copy = *source;
    target_copy = *target;
    switch (target_copy.iType) {
    case W8_TARGET_KIND_CHARACTER:
    case W8_TARGET_KIND_CHARACTER_INDIRECT:
        SetTargetSourceToCharacter(target_copy.iChar, source);
        break;
    case W8_TARGET_KIND_MONSTER:
        list_index = MonsterGetIndexByLocationID(0xc09, MAGIC_CPP, target_copy.iMonsterID, true);
        monster_info = MonsterGetScriptPartByLocationIndex(list_index);
        SetTargetSourceToMonster(monster_info, source);
        break;
    case W8_TARGET_KIND_PARTY:
        for (slot = 0; slot < 8; ++slot) {
            if (g_status.buffers.XChar[slot].fOccupied &&
                g_status.buffers.Char[slot].hp_current != 0 &&
                g_status.buffers.Char[slot].highest_condition < W8_CONDITION_DEAD) {
                break;
            }
        }
        if (slot >= 8) {
            srAssertFail("FALSE", MAGIC_CPP, 0xc1b, 0);
            return;
        }
        SetTargetSourceToCharacter(slot, source);
        ResetCombatSlot(target);
        if (source_copy.iType == W8_TARGET_SOURCE_CHARACTER) {
            target->iType = W8_TARGET_KIND_PARTY;
        } else if (source_copy.iType == W8_TARGET_SOURCE_MONSTER) {
            target->iType = W8_TARGET_KIND_GROUP;
            monster_info = MonsterInfoFromID(0xc2a, MAGIC_CPP, source_copy.iMonsterID, true);
            target->iGroupID = monster_info->monster_group_id;
        } else {
            srAssertFail("FALSE", MAGIC_CPP, 0xc2d, 0);
        }
        source->point_source = 1;
        return;
    case W8_TARGET_KIND_GROUP:
        list_index = GetMonsterGroupIndexByID(0xc33, MAGIC_CPP, target_copy.iGroupID, true);
        group = GetMonsterGroupByListIndex(list_index);
        list_index = MonsterGetIndexByLocationID(0xc33, MAGIC_CPP, group->leader_location_id, true);
        monster_info = MonsterGetScriptPartByLocationIndex(list_index);
        SetTargetSourceToMonster(monster_info, source);
        ResetCombatSlot(target);
        if (source_copy.iType == W8_TARGET_SOURCE_CHARACTER) {
            target->iType = W8_TARGET_KIND_PARTY;
        } else if (source_copy.iType == W8_TARGET_SOURCE_MONSTER) {
            target->iType = W8_TARGET_KIND_GROUP;
            monster_info = MonsterInfoFromID(0xc40, MAGIC_CPP, source_copy.iMonsterID, true);
            target->iGroupID = monster_info->monster_group_id;
        } else {
            srAssertFail("FALSE", MAGIC_CPP, 0xc43, 0);
        }
        source->point_source = 1;
        return;
    default:
        srAssertFail("FALSE", MAGIC_CPP, 0xc52, 0);
        return;
    }
    if (TargetSourceIsCharacter(&source_copy, 0)) {
        target->iType = W8_TARGET_KIND_CHARACTER;
        target->iChar = source_copy.iChar;
    } else if (TargetSourceIsMonster(&source_copy, 0)) {
        target->iType = W8_TARGET_KIND_MONSTER;
        target->iMonsterID = source_copy.iMonsterID;
    } else {
        srAssertFail("FALSE", MAGIC_CPP, 0xc6b, 0);
    }
    source->point_source = 1;
}

/* Assert and route the source/target pair a cast is about to use. Target
   kinds that aim at a single combatant re-resolve both ends through
   RedirectBackfiredSpellTarget; the point kinds trace the aim to where the
   spell actually lands and turn the source into a point. */
// FUNCTION: WIZ8 0x004FEA50
void PrepareSpellTarget(int spell_id, W8TargetSource* source, W8CombatSlot* target)
{
    srVector3T<float> from;
    srVector3T<float> trace;
    srVector3T<float> saved;
    W8MonsterInfo* monster_info;
    W8Navigator* navigator;
    W8SpellTargetType target_type;

    if (source->iType <= W8_TARGET_SOURCE_NONE || source->iType >= W8_TARGET_SOURCE_COUNT) {
        srAssertFail("(pSource->iType > SOURCE_TYPE_NONE) && (pSource->iType < SOURCE_TYPE_COUNT)",
                     MAGIC_CPP, 0xc78, 0);
    }
    if (target->iType <= W8_TARGET_KIND_NONE || target->iType >= W8_TARGET_KIND_COUNT) {
        srAssertFail("(pTarget->iType > TARGET_TYPE_NONE) && (pTarget->iType < TARGET_TYPE_COUNT)",
                     MAGIC_CPP, 0xc79, 0);
    }
    if (source->fBackfire) {
        srAssertFail("!pSource->fBackfire", MAGIC_CPP, 0xc7c, 0);
    }
    monster_info = 0;
    if (TargetSourceIsMonster(source, 0)) {
        monster_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0xc81, MAGIC_CPP, source->iMonsterID, true));
    }
    target_type = GetSpellTargetType(spell_id, false);
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
        RedirectBackfiredSpellTarget(source, target);
        return;
    case 5:
        if (TargetSourceIsCharacter(source, 0)) {
            GetCameraPosition(&from);
        } else if (TargetSourceIsMonster(source, 0)) {
            if (monster_info->p3D->GetSpellPosition(&from) == 0) {
                monster_info->p3D->GetMappedPosition(&from);
            }
        } else {
            from = source->point;
        }
        trace = target->point;
        g_octree->TraceLineOfSight(&from, &trace, true, -3, -3, true, 0);
        target->point = trace;
        saved = source->point;
        source->point = target->point;
        if (TargetSourceIsCharacter(source, 0)) {
            GetCameraPosition(&from);
            target->point = from;
        } else if (TargetSourceIsMonster(source, 0)) {
            navigator = monster_info->p3D;
            target->point = navigator->movement.position;
            target->point.y += navigator->movement.height_offset;
        } else {
            target->point = saved;
        }
        source->iType = W8_TARGET_SOURCE_INDIRECT;
        return;
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
        navigator = g_startup_world;
    } else if (TargetSourceIsMonster(source, 0)) {
        navigator = monster_info->p3D;
    } else {
        target->point = saved;
        source->iType = W8_TARGET_SOURCE_INDIRECT;
        return;
    }
    trace = navigator->GetPosition();
    target->point = trace;
    source->iType = W8_TARGET_SOURCE_INDIRECT;
}

/* Pick one random in-combat participant other than the source and write it as
   the backfired spell's new target. A single-target spell keeps it a single
   slot; a group spell takes the whole monster group or the party instead. */
// FUNCTION: WIZ8 0x004FEDC0
int PickBackfireTarget(int spell_id, W8TargetSource* source, W8CombatSlot* target)
{
    W8MonsterInfo* monster_info;
    unsigned int candidates;
    unsigned int pick;
    unsigned int index;
    int slot;

    candidates = 0;
    for (slot = 0; slot < 8; ++slot) {
        if (g_status.buffers.XChar[slot].fOccupied &&
            (!TargetSourceIsCharacter(source, 0) || source->iChar != slot)) {
            ++candidates;
        }
    }
    index = 0;
    while (PLLength(gXStatus.plsMonsterList) != 0 && index < PLLength(gXStatus.plsMonsterList)) {
        monster_info = MonsterGetScriptPartByLocationIndex(index);
        if (monster_info->fActive && monster_info->fInCombat && monster_info->hp_current != 0 &&
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
        if (g_status.buffers.XChar[slot].fOccupied &&
            (!TargetSourceIsCharacter(source, 0) || source->iChar != slot) && (--pick == 0)) {
            if (GetSpellTargetType(spell_id, false) == 4) {
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
        if (monster_info->fActive && monster_info->fInCombat && monster_info->hp_current != 0 &&
            (!TargetSourceIsMonster(source, 0) ||
             source->iMonsterID != monster_info->location_id) &&
            (--pick == 0)) {
            if (GetSpellTargetType(spell_id, false) == 4) {
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
void ScatterSpellPointTarget(int spell_id, W8TargetSource* source, W8CombatSlot* target)
{
    srVector3T<float> origin;
    srVector3T<float> point;
    srVector3T<float> delta;
    W8MonsterInfo* monster_info;
    W8Navigator* navigator;
    double range;
    int attempt;

    range = CalcRangeDistance(g_spell_records[spell_id].range_category);
    navigator = g_startup_world;
    if (source->iType != W8_TARGET_SOURCE_CHARACTER) {
        if (source->iType != W8_TARGET_SOURCE_MONSTER) {
            srAssertFail("pSource->iType == SOURCE_TYPE_MONSTER", MAGIC_CPP, 0xd80, 0);
        }
        monster_info = MonsterInfoFromID(0xd81, MAGIC_CPP, source->iMonsterID, true);
        navigator = monster_info->p3D;
    }
    origin = navigator->GetPosition();
    attempt = 0;
    do {
        point.x = static_cast<float>(
            (Random(0x7d1) - g_monster_poster_max_distance) * range * g_double_005ec8d0 + origin.x);
        point.z = static_cast<float>(
            (Random(0x7d1) - g_monster_poster_max_distance) * range * g_double_005ec8d0 + origin.z);
        point.y = Random(0x3e9) * range * g_double_005ec8d0 + origin.y;
        g_octree->TraceLineOfSight(&origin, &point, true, -3, -3, true, 0);
        point.y = SettlePositionToGround(&point, 0);
        if (point.y != g_ground_settle_fail) {
            break;
        }
        ++attempt;
    } while (attempt < 5);
    delta = origin - point;
    if (range < delta.Length()) {
        delta.SetLength(range);
        point = delta + origin;
    }
    target->point = point;
}

/* The once-per-cast backfire roll for a spell whose source is under the
   confusion condition: a trait save can skip it, then roughly one cast in
   five has its target re-picked at random or scattered to a random point,
   with a notice to whoever the source is. */
// FUNCTION: WIZ8 0x004FF220
void CheckSpellBackfire(int spell_id, W8TargetSource* source, W8CombatSlot* target)
{
    W8Character* character;
    W8MonsterInfo* monster_info;
    W8SpellTargetType target_type;

    monster_info = 0;
    if (source->iType == W8_TARGET_SOURCE_CHARACTER) {
        character = &g_status.buffers.Char[source->iChar];
        if (character->uiCondition[W8_CONDITION_BLIND] == 0) {
            return;
        }
        if (target->iType == W8_TARGET_KIND_CHARACTER && target->iChar == source->iChar) {
            return;
        }
        if (CharacterHasTrait(character, W8_TRAIT_EFFECTIVE_WHILE_BLIND)) {
            if (Random(100) <
                static_cast<unsigned int>(static_cast<int>(ScaleValueByProfessionLevel(
                    character, W8_TRAIT_EFFECTIVE_WHILE_BLIND, 50.0)))) {
                return;
            }
        }
    } else {
        if (source->iType != W8_TARGET_SOURCE_MONSTER) {
            return;
        }
        monster_info = MonsterInfoFromID(0xdcf, MAGIC_CPP, source->iMonsterID, true);
        if (monster_info->uiCondition[W8_CONDITION_BLIND] == 0) {
            return;
        }
        if (target->iType == W8_TARGET_KIND_MONSTER && target->iMonsterID == source->iMonsterID) {
            return;
        }
    }
    if (0x13 < Random(100)) {
        return;
    }
    target_type = GetSpellTargetType(spell_id, false);
    switch (target_type) {
    case 1:
    case 3:
    case 4:
        if (PickBackfireTarget(spell_id, source, target) == 0) {
            return;
        }
        break;
    case 5:
    case 6:
    case 8:
        ScatterSpellPointTarget(spell_id, source, target);
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
    W8SpellTargetType target_type;
    int side;
    int sight_flag;
    int slot;
    unsigned int index;
    bool marked;
    unsigned char fVertextAvail;

    monster = 0;
    marked = false;
    monster_info = 0;
    sight_flag = 0;
    if (!source->fBackfire && !source->fReflection && !SourceActionReachesTarget(source, target)) {
        return;
    }
    player_pos = g_startup_world->GetPosition();
    GetCameraPosition(&camera);
    if (TargetSourceIsMonster(source, 1)) {
        if (source->iMonsterID == -1) {
            FormatDebugMessage(
                1,
                "InvalidMagicSource: Spell %d(%ls), Target Type %d(char %d, monster ID %d, group "
                "ID %d), Source Type %d(char %d,ID %d)",
                spell_id, g_spell_records[spell_id].display_name, target->iType, target->iChar,
                target->iMonsterID, target->iGroupID, source->iType, source->iChar,
                source->iMonsterID);
            return;
        }
        monster_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0x937, MAGIC_CPP, source->iMonsterID, true));
        GetMonsterDataForInfo(monster_info);
        monster = monster_info->p3D;
    }
    if (TargetSourceIsCharacter(source, 0)) {
        centre = player_pos;
        eye = camera;
    } else if (TargetSourceIsMonster(source, 0)) {
        centre = monster->GetPosition();
        fVertextAvail = 0;
        if (source->point_source == 0 && spell_id != 0x77 && monster_info->has_spell_origin) {
            fVertextAvail = monster->GetSpellPosition(&eye);
            if (fVertextAvail == 0) {
                srAssertFail("fVertextAvail", MAGIC_CPP, 0x951, 0);
            }
        }
        if (fVertextAvail != 0) {
            sight_flag = 3;
        } else {
            eye = monster->movement.position;
            eye.y += monster->movement.height_offset;
            sight_flag = 2;
        }
    } else {
        eye = source->point;
        centre = source->point;
    }
    target_point = target->point;
    target_type = GetSpellTargetType(spell_id, false);
    switch (target_type) {
    case W8_TARGET_TYPE_CASTER:
    case W8_TARGET_TYPE_ENEMY:
        if (target->iType == W8_TARGET_KIND_CHARACTER) {
            party_markers->Add(target->iChar);
        } else if (target->iType == W8_TARGET_KIND_MONSTER) {
            monster_markers->Add(target->iMonsterID);
        } else if (target_type == W8_TARGET_TYPE_ENEMY) {
            FormatDebugMessage(1,
                               "InvalidMagicTarget: Spell %d(%ls), Target Type %d(char %d, monster "
                               "ID %d, group ID %d), Source Type %d(char %d,ID %d)",
                               spell_id, g_spell_records[spell_id].display_name, target->iType,
                               target->iChar, target->iMonsterID, target->iGroupID, source->iType,
                               source->iChar, source->iMonsterID);
            return;
        }
        break;
    case W8_TARGET_TYPE_ALLY:
        if (target->iType == W8_TARGET_KIND_CHARACTER ||
            target->iType == W8_TARGET_KIND_CHARACTER_INDIRECT) {
            party_markers->Add(target->iChar);
        } else if (target->iType == W8_TARGET_KIND_MONSTER) {
            monster_markers->Add(target->iMonsterID);
        } else {
            FormatDebugMessage(1,
                               "InvalidMagicTarget: Spell %d(%ls), Target Type %d(char %d, monster "
                               "ID %d, group ID %d), Source Type %d(char %d,ID %d)",
                               spell_id, g_spell_records[spell_id].display_name, target->iType,
                               target->iChar, target->iMonsterID, target->iGroupID, source->iType,
                               source->iChar, source->iMonsterID);
            return;
        }
        break;
    case W8_TARGET_TYPE_PARTY:
        radius = (g_spell_records[spell_id].radius_per_level * power_level +
                  g_spell_records[spell_id].effect_radius) *
                 g_world_scale;
        if (TargetSourceIsCharacter(source, 0)) {
            marked = true;
            side = 2;
            if (g_float_zero < radius) {
                radius += g_startup_world->radius;
            }
        } else if (TargetSourceIsMonster(source, 0)) {
            if (monster_info->ubDisposition == W8_DISPOSITION_FRIENDLY) {
                distance = (centre.x - player_pos.x) * (centre.x - player_pos.x) +
                           (centre.y - player_pos.y) * (centre.y - player_pos.y) +
                           (centre.z - player_pos.z) * (centre.z - player_pos.z);
                if (sqrtf(distance) <= radius &&
                    monster_info->player_visibility.los_flags[sight_flag] != '\0') {
                    marked = true;
                }
            } else if (monster_info->ubDisposition != W8_DISPOSITION_HOSTILE) {
                FormatDebugMessage(1,
                                   "InvalidMagicSource: Spell %d(%ls), Target Type %d(char %d, "
                                   "monster ID %d, group ID %d), Source Type %d(char %d,ID %d)",
                                   spell_id, g_spell_records[spell_id].display_name, target->iType,
                                   target->iChar, target->iMonsterID, target->iGroupID,
                                   source->iType, source->iChar, source->iMonsterID);
                return;
            }
            side = monster_info->ubDisposition;
            radius += monster->radius;
        } else {
            side = 2;
            distance = (centre.x - player_pos.x) * (centre.x - player_pos.x) +
                       (centre.y - player_pos.y) * (centre.y - player_pos.y) +
                       (centre.z - player_pos.z) * (centre.z - player_pos.z);
            if (sqrtf(distance) <= radius &&
                g_octree->TraceLineOfSight(&eye, &camera, true, -3, -3, true, 0) == 0) {
                marked = true;
            }
        }
        CollectMonstersWithinRadius(&centre, &eye, monster_markers, radius, static_cast<char>(side),
                                    static_cast<char>(highlighting));
        break;
    case W8_TARGET_TYPE_ENEMY_GROUP:
        if (target->iType == W8_TARGET_KIND_GROUP) {
            group = GetMonsterGroupByListIndex(
                GetMonsterGroupIndexByID(0x99f, MAGIC_CPP, target->iGroupID, true));
            if (group == 0) {
                FormatDebugMessage(1,
                                   "InvalidMagicTarget: Spell %d(%ls), Target Type %d(char %d, "
                                   "monster ID %d, group ID %d), Source Type %d(char %d,ID %d)",
                                   spell_id, g_spell_records[spell_id].display_name, target->iType,
                                   target->iChar, target->iMonsterID, target->iGroupID,
                                   source->iType, source->iChar, source->iMonsterID);
                return;
            }
            index = 0;
            while (ILLength(group->monsters) != 0 && index < ILLength(group->monsters)) {
                slot = IListGetAt(group->monsters, index);
                member = MonsterInfoFromID(0x9aa, MAGIC_CPP, slot, true);
                if (member->ubDisposition == group->ubDisposition) {
                    monster_markers->Add(slot);
                }
                ++index;
            }
        } else if (target->iType == W8_TARGET_KIND_PARTY) {
            marked = true;
        } else {
            FormatDebugMessage(1,
                               "InvalidMagicTarget: Spell %d(%ls), Target Type %d(char %d, monster "
                               "ID %d, group ID %d), Source Type %d(char %d,ID %d)",
                               spell_id, g_spell_records[spell_id].display_name, target->iType,
                               target->iChar, target->iMonsterID, target->iGroupID, source->iType,
                               source->iChar, source->iMonsterID);
            return;
        }
        break;
    case W8_TARGET_TYPE_CONE:
        heading = GetHeadingAngle(&eye, &target_point);
        elevation = GetElevationAngle(&eye, &target_point);
        if (TargetSourceIsCharacter(source, 0)) {
            side = 1;
        } else if (TargetSourceIsMonster(source, 0)) {
            if (monster_info->ubDisposition == W8_DISPOSITION_FRIENDLY) {
                side = 1;
            } else if (monster_info->ubDisposition == W8_DISPOSITION_HOSTILE) {
                side = 2;
                if (TargetInRangeAndArcs(&camera, g_startup_world->radius, &eye, monster->radius,
                                         heading, elevation) &&
                    monster_info->player_visibility.los_flags[sight_flag] != '\0') {
                    marked = true;
                }
            } else {
                FormatDebugMessage(1,
                                   "InvalidMagicSource: Spell %d(%ls), Target Type %d(char %d, "
                                   "monster ID %d, group ID %d), Source Type %d(char %d,ID %d)",
                                   spell_id, g_spell_records[spell_id].display_name, target->iType,
                                   target->iChar, target->iMonsterID, target->iGroupID,
                                   source->iType, source->iChar, source->iMonsterID);
                return;
            }
        } else {
            side = 3;
            if (source->fBackfire || source->fReflection) {
                if (source->iChar != -1) {
                    side = 2;
                } else if (source->iMonsterID != -1) {
                    if (MonsterInfoFromID(0x9f3, MAGIC_CPP, source->iMonsterID, true)
                            ->ubDisposition != W8_DISPOSITION_FRIENDLY) {
                        side = 1;
                    } else {
                        side = 2;
                    }
                }
            }
        }
        if (!marked &&
            TargetInRangeAndArcs(&camera, g_startup_world->radius, &eye, 0, heading, elevation) &&
            g_octree->TraceLineOfSight(&eye, &camera, true, -3, -3, true, 0) == 0) {
            marked = true;
        }
        CollectConeMonsterTargets(source, &eye, heading, elevation, monster_markers,
                                  static_cast<unsigned char>(side), sight_flag);
        if (monster_markers->count == 0 &&
            (g_combat_state == 0 || !g_combat_state->enemies_engaged) &&
            TargetSourceIsCharacter(source, 0) && static_cast<char>(side) == 1 &&
            !AnyMonsterEngaged()) {
            CollectConeMonsterTargets(source, &eye, heading, elevation, monster_markers, 3,
                                      sight_flag);
        }
        break;
    case W8_TARGET_TYPE_RADIUS:
        trace.Set(target_point.x, target_point.y - g_float_005ebc64, target_point.z);
        radius = (g_spell_records[spell_id].radius_per_level * power_level +
                  g_spell_records[spell_id].effect_radius) *
                 g_world_scale;
        if (TargetSourceIsCharacter(source, 1)) {
            side = (source->fBackfire || source->fReflection) ? 1 : 2;
        } else if (TargetSourceIsMonster(source, 1)) {
            if (monster_info->ubDisposition != W8_DISPOSITION_FRIENDLY &&
                monster_info->ubDisposition != W8_DISPOSITION_HOSTILE) {
                FormatDebugMessage(1,
                                   "InvalidMagicSource: Spell %d(%ls), Target Type %d(char %d, "
                                   "monster ID %d, group ID %d), Source Type %d(char %d,ID %d)",
                                   spell_id, g_spell_records[spell_id].display_name, target->iType,
                                   target->iChar, target->iMonsterID, target->iGroupID,
                                   source->iType, source->iChar, source->iMonsterID);
                return;
            }
            side = (source->fBackfire || source->fReflection) ? 1 : 2;
        } else {
            side = 3;
        }
        if (side != 1) {
            distance = (trace.x - player_pos.x) * (trace.x - player_pos.x) +
                       (trace.y - player_pos.y) * (trace.y - player_pos.y) +
                       (trace.z - player_pos.z) * (trace.z - player_pos.z);
            if (sqrtf(distance) <= radius &&
                g_octree->TraceLineOfSight(&target_point, &camera, true, -3, -3, true, 0) == 0) {
                marked = true;
            }
        }
        CollectMonstersWithinRadius(&trace, &target_point, monster_markers, radius,
                                    static_cast<char>(side), static_cast<char>(highlighting));
        if (monster_markers->count == 0 &&
            (g_combat_state == 0 || !g_combat_state->enemies_engaged) &&
            TargetSourceIsCharacter(source, 0) && static_cast<char>(side) == 1 &&
            !AnyMonsterEngaged()) {
            CollectMonstersWithinRadius(&trace, &target_point, monster_markers, radius, 3,
                                        static_cast<char>(highlighting));
        }
        break;
    case W8_TARGET_TYPE_ALL_ENEMIES:
        target->point = eye;
        radius = CalcRangeDistance(g_spell_records[spell_id].range_category, source);
        if (TargetSourceIsCharacter(source, 1) ||
            (TargetSourceIsMonster(source, 1) &&
             monster_info->ubDisposition == W8_DISPOSITION_FRIENDLY)) {
            side = (source->fBackfire || source->fReflection) ? 2 : 1;
        } else if (TargetSourceIsMonster(source, 1)) {
            side = (source->fBackfire || source->fReflection) ? 1 : 2;
        } else {
            side = 3;
        }
        if (side != 1) {
            distance = (centre.x - player_pos.x) * (centre.x - player_pos.x) +
                       (centre.y - player_pos.y) * (centre.y - player_pos.y) +
                       (centre.z - player_pos.z) * (centre.z - player_pos.z);
            if (sqrtf(distance) <= radius &&
                g_octree->TraceLineOfSight(&eye, &camera, true, -3, -3, true, 0) == 0) {
                marked = true;
            }
        }
        CollectMonstersWithinRadius(&centre, &eye, monster_markers, radius, static_cast<char>(side),
                                    static_cast<char>(highlighting));
        if (monster_markers->count == 0 &&
            (g_combat_state == 0 || !g_combat_state->enemies_engaged) &&
            TargetSourceIsCharacter(source, 0) && static_cast<char>(side) == 1 &&
            !AnyMonsterEngaged()) {
            CollectMonstersWithinRadius(&centre, &eye, monster_markers, radius, 3,
                                        static_cast<char>(highlighting));
        }
        break;
    default:
        break;
    }
    if (marked && spell_id != 0x16 && spell_id != 0x4d) {
        for (slot = 0; slot < 8; ++slot) {
            if (g_status.buffers.XChar[slot].fOccupied &&
                g_status.buffers.Char[slot].hp_current != 0 &&
                g_status.buffers.Char[slot].highest_condition < W8_CONDITION_DEAD &&
                (g_status.buffers.Char[slot].uiCondition[W8_CONDITION_TURNCOAT] == 0 ||
                 !MonsterCanAimSpell(spell_id) || static_cast<char>(side) == 3)) {
                party_markers->Add(slot);
            }
        }
    }
    PruneSpellTargetMarkers(spell_id, monster_markers);
}

/* Drop every marker that no longer names a live, targetable monster. Spells
   0x16, 0x4d and 0x81 also drop markers whose monster kind they do not affect. */
// FUNCTION: WIZ8 0x00501B70
void PruneSpellTargetMarkers(int spell_id, W8GrowableVector<int>* monster_markers)
{
    W8MonsterInfo* monster_info;
    W8MonsterRecord* record;
    int index;

    for (index = monster_markers->count - 1; index >= 0; --index) {
        monster_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0x1595, MAGIC_CPP, *monster_markers->GetAt(index), true));
        record = GetMonsterDataForInfo(monster_info);
        if (!monster_info->fActive || monster_info->hp_current == 0 ||
            monster_info->uiCondition[W8_CONDITION_DEAD] > 0 || record->untargetable != 0) {
            monster_markers->RemoveAt(index);
            continue;
        }
        switch (spell_id) {
        case 0x16:
            if (record->kind != 0x14 && record->kind != 0x15) {
                monster_markers->RemoveAt(index);
            }
            break;
        case 0x4d:
            if (record->kind != 0x14 && record->kind != 0x15 && record->kind != 0x1c &&
                monster_info->summoned == 0) {
                monster_markers->RemoveAt(index);
            }
            break;
        case 0x81:
            if (record->kind != 0x14) {
                monster_markers->RemoveAt(index);
            }
            break;
        }
    }
}

// FUNCTION: WIZ8 0x00501D20
void TrackItemSpellSource(W8Character* character, int spell_id)
{
    bool has_spell_storage[0x96] = {false};
    bool* has_spell = has_spell_storage + 1;
    W8ItemInstance* item;
    int count;

    item = character->EquippedItem;
    for (count = 0xc; count != 0; --count) {
        int item_id = item->iItemNo;
        if (item_id != -1 && g_item_records[item_id].spell_id != 0 &&
            CanCharacterActivateItem(character, item) &&
            ((g_item_records[item_id].quantity_kind != W8_ITEM_QUANTITY_SHOTS &&
              g_item_records[item_id].quantity_kind != W8_ITEM_QUANTITY_CHARGES) ||
             item->uses_or_charges != 0)) {
            has_spell[g_item_records[item_id].spell_id - 1] = 1;
        }
        ++item;
    }
    item = character->backpack;
    for (count = 8; count != 0; --count) {
        int item_id = item->iItemNo;
        if (item_id != -1 && g_item_records[item_id].spell_id != 0 &&
            CanCharacterActivateItem(character, item) &&
            ((g_item_records[item_id].quantity_kind != W8_ITEM_QUANTITY_SHOTS &&
              g_item_records[item_id].quantity_kind != W8_ITEM_QUANTITY_CHARGES) ||
             item->uses_or_charges != 0)) {
            has_spell[g_item_records[item_id].spell_id - 1] = 1;
        }
        ++item;
    }
    /* Retail maps spell ids to records directly: record r is gated by
       storage[r] (the walk reads has_spell[index - 1] with the 0-based
       record index), so record 0 reads the always-zero leading byte and
       spell id s marks record s - the same record the cast_count access
       below increments. */
    W8ItemSpellUsageRecord* record = g_status.item_spell_usage;
    int index = 0;
    while (record < g_status.item_spell_usage + 150) {
        if (has_spell[index - 1]) {
            ++record->usable_cast_count;
        }
        ++index;
        ++record;
    }
    ++g_status.item_spell_usage[spell_id].cast_count;
}
