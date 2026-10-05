#include "wiz8/fonts.h"
#include <string.h>

#include "wiz8/local_screens/Screens.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/Sight.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/xstatus.h"
#include "wiz8/layouts/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/CombatHostility.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/utility.h"
#include "wiz8/sr_api.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/layouts/npc_state.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/npc_script_file.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/layouts/item_instance.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/layouts/game_status.h"

/* Condition-to-notice word table. Only the first word of each four-word
   stride is read, hence the multiplied index. */
// GLOBAL: WIZ8 0x0061e570
unsigned short g_condition_notices[128] = {
    0x348, 0x349, 0x34a, 0x34b, 0x34c, 0x34d, 0x34e, 0x34f, 0x350, 0x351, 0x352, 0x353, 0x354,
    0x355, 0x356, 0x357, 0x358, 0x359, 0x35a, 0x35b, 0x35c, 0x35d, 0x35e, 0x35f, 0x360, 0x361,
    0x362, 0x363, 0x364, 0x365, 0x366, 0x367, 0x368, 0x369, 0x36a, 0x36b, 0x36c, 0x36d, 0x36e,
    0x36f, 0x370, 0x371, 0x372, 0x373, 0x374, 0x375, 0x376, 0x377, 0x378, 0x379, 0x37a, 0x37b,
    0x37c, 0x37d, 0x37e, 0x37f, 0x380, 0x381, 0x382, 0x383, 0x384, 0x385, 0x386, 0x387, 0x38c,
    0x38d, 0x38e, 0x38f, 0x388, 0x389, 0x38a, 0x38b, 0x390, 0x391, 0x392, 0x393, 0x394, 0x395,
    0x396, 0x397, 0x273, 0x65d, 0x65e, 0x65f, 0x660, 0x661, 0x662, 0x663, 0x664, 0x665, 0x666,
    0x667, 0x668, 0x669, 0x66a, 0x66b, 0x66c, 0x66d, 0x66e, 0x66f, 0x398, 0x399, 0x39a, 0x39b,
    0x39c, 0x39d, 0x39e, 0x39f, 0x273, 0x670, 0x671, 0x672, 0x673, 0x674, 0x675, 0x676, 0x3a0,
    0x3a1, 0x3a2, 0x3a3, 0x3a4, 0x3a5, 0x3a6, 0x3a7, 0x3a8, 0x3a9, 0x3aa, 0x3ab,
};
// FUNCTION: WIZ8 0x005248a0
unsigned char GetConditionRecordFlag(int party_slot, int condition)
{
    return g_status.buffers.Char[party_slot].conditions[condition].active;
}

// FUNCTION: WIZ8 0x005248D0
void ReleaseMonsterConditionBindings(W8MonsterInfo* monster_info)
{
    for (unsigned int slot_kind = 0; slot_kind < 2; ++slot_kind) {
        bool cleared = false;
        if ((monster_info->condition_binding_mask & (1 << slot_kind)) != 0) {
            W8Condition condition;
            switch (slot_kind) {
            case 0:
                condition = W8_CONDITION_NONE;
                break;
            case 1:
                condition = W8_CONDITION_MISSING;
                break;
            }
            for (unsigned int party_slot = 0; party_slot < 8; ++party_slot) {
                W8Character* character = &g_status.buffers.Char[party_slot];
                W8CharacterConditionRecord* record = &character->conditions[slot_kind];
                if ((condition == 0 || character->uiCondition[condition] != 0) &&
                    record->level_acquired == g_status.current_level &&
                    record->source_monster == monster_info->location_id) {
                    cleared = true;
                    record->active = false;
                    record->level_acquired = 0;
                    record->source_monster = 0;
                    if (condition != 0 && character->fInParty) {
                        RemoveCharacterCondition(party_slot, condition, true);
                    }
                }
            }
            if (!cleared && slot_kind == 0) {
                for (unsigned int index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
                    W8MonsterInfo* bound = MonsterGetScriptPartByLocationIndex(index);
                    if (bound->insanity_summon == monster_info->location_id) {
                        bound->insanity_summon = -1;
                        break;
                    }
                }
            }
        }
    }
}

/* Lifting a character's condition clears its duration and any state that
   condition alone maintained, then notifies dependents. */
// FUNCTION: WIZ8 0x00523330
void RemoveCharacterCondition(int party_slot, W8Condition condition, bool announce)
{
    W8Character* character = &g_status.buffers.Char[party_slot];
    W8PartySlotRow* row = &g_status.buffers.XChar[party_slot];
    W8ItemInstance* found_item;
    W8Character* found_character;
    bool can_rest;

    if (character->uiCondition[condition] != 0 || !g_status.world_suspended) {
        if (!row->fOccupied) {
            srAssertFail("fCHAR_OCCUPIED(uiChar)",
                         "C:\\Projects\\Wizardry 8\\Local Code\\Conditions & Enchantments.cpp",
                         0xf4, 0);
        }
        if (character->uiCondition[condition] == 0) {
            srAssertFail("gStatus.Char[uiChar].uiCondition[uiCondition] > 0",
                         "C:\\Projects\\Wizardry 8\\Local Code\\Conditions & Enchantments.cpp",
                         0xf5, 0);
        }
        if (party_slot >= 0 && party_slot < 8 && row->fOccupied && character->hp_current != 0) {
            can_rest = character->highest_condition < W8_CONDITION_TURNCOAT;
        } else {
            can_rest = false;
        }
        if (condition == W8_CONDITION_HEXED) {
            if (character->uiCondition[W8_CONDITION_INFATUATED] != 0 &&
                GetLevelBand(g_status.current_level) != '\t' &&
                GetLevelBand(g_status.current_level) != '\n') {
                return;
            }
        } else if (condition == W8_CONDITION_INSANE &&
                   FindItemOnParty(0x243, &found_item, &found_character, 2, 0) &&
                   found_item != &g_status.item_in_hand) {
            if (found_character == 0) {
                found_character = FindPartyMemberWithLowestResistance4();
            }
            if (CharacterPointerToPartySlot(found_character) == static_cast<unsigned int>(party_slot) &&
                !FindItemOnCharacter(found_character, 0x239, 0, 0, 0)) {
                return;
            }
        }
        if (announce) {
            PostCharacterNotice(party_slot, gppStringList[0x243],
                                gppStringList[g_condition_notices[condition * 4]]);
        }
        character->uiCondition[condition] = 0;
        RecomputeCharacterHighestCondition(party_slot);
        switch (condition) {
        case W8_CONDITION_HEXED:
        case W8_CONDITION_BLIND:
            gXStatus.sight_refresh_pending = true;
            break;
        case W8_CONDITION_POISONED:
            character->condition_argument = 0;
            break;
        case W8_CONDITION_DRAINED:
            character->hp_adjustment = 0;
            character->fatigue_penalty = 0;
            break;
        case W8_CONDITION_INSANE:
            if (gXStatus.fCombatMode && g_combat_state->characters[party_slot].berserk) {
                row->target_out_of_combat = row->target_in_combat;
            }
            break;
        case W8_CONDITION_TURNCOAT:
            SetTargetToCharacter(party_slot, true);
            RepickActionTarget(party_slot, W8_TARGETING_CONTEXT_OUT_OF_COMBAT, 0);
            RepickActionTarget(party_slot, W8_TARGETING_CONTEXT_IN_COMBAT, 0);
            break;
        default:
            break;
        }
        RebuildConditionsAndDerivedStats(party_slot);
        QueueConditionClearedReaction(character, condition);
        if (!can_rest && party_slot > -1 && party_slot < 8 && row->fOccupied &&
            character->hp_current != 0 && character->highest_condition < W8_CONDITION_TURNCOAT &&
            gXStatus.fCombatMode &&
            CharacterCanSwitchTo(party_slot, W8_TARGETING_CONTEXT_IN_COMBAT, 0, 0)) {
            RefreshCombatTargetHighlights(party_slot, &row->target_in_combat);
        }
    }
}

/*
 * Original translation unit: Local Code\Conditions & Enchantments.cpp.
 *
 * Characters and monsters carry the same two arrays: twenty condition
 * durations and eight enchantment slots. The bodies here move conditions
 * between the two, lift them one at a time or all at once, and keep the
 * highest enchantment slot in use up to date as slots empty.
 */

#define CONDITIONS_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Conditions & Enchantments.cpp"

/* Keep the live quantity in the byte selected by the database record. Stack
   items use stack_count; charge and use-count items use uses_or_charges. */
// FUNCTION: WIZ8 0x00522e80
void NormalizeItemQuantityKind(W8ItemInstance* item)
{
    unsigned char quantity_kind;

    if (item->iItemNo == -1) {
        return;
    }

    quantity_kind = g_item_records[item->iItemNo].quantity_kind;
    if (quantity_kind != W8_ITEM_QUANTITY_STACK) {
        if (quantity_kind <= W8_ITEM_QUANTITY_STACK || quantity_kind > W8_ITEM_QUANTITY_SHOTS ||
            item->stack_count <= 0) {
            return;
        }
        if (item->uses_or_charges > 0) {
            item->stack_count = 0;
            return;
        }
        item->uses_or_charges = item->stack_count;
        item->stack_count = 0;
        return;
    }

    if (item->uses_or_charges > 0) {
        if (item->stack_count > 0) {
            item->uses_or_charges = 0;
            return;
        }
        item->stack_count = item->uses_or_charges;
        item->uses_or_charges = 0;
        return;
    }
    if (item->stack_count == 0) {
        item->stack_count = 1;
    }
}

/* The load tail: drop equipment the loaded character can no longer wear, then
   normalize the quantity byte on every worn and packed item and on the shared
   party pool. */
// FUNCTION: WIZ8 0x00522ef0
void SanitizeLoadedItems(void)
{
    unsigned int slot;
    unsigned int index;
    W8Character* character;

    for (slot = 0; slot < 8; ++slot) {
        if (g_status.buffers.XChar[slot].fOccupied) {
            UnequipUnusableItems(&g_status.buffers.Char[slot]);
        }
    }
    for (slot = 0; slot < 8; ++slot) {
        if (!g_status.buffers.XChar[slot].fOccupied) {
            continue;
        }
        character = &g_status.buffers.Char[slot];
        for (index = 0; index < 12; ++index) {
            if (character->EquippedItem[index].iItemNo != -1) {
                NormalizeItemQuantityKind(&character->EquippedItem[index]);
            }
        }
        for (index = 0; index < 8; ++index) {
            if (character->backpack[index].iItemNo != -1) {
                NormalizeItemQuantityKind(&character->backpack[index]);
            }
        }
    }
    for (index = 0; index < g_status.party_item_count; ++index) {
        if (g_status.party_item_pool[index].iItemNo != -1) {
            NormalizeItemQuantityKind(&g_status.party_item_pool[index]);
        }
    }
}

/* Condition immunity sets by monster kind. Each entry is a kind byte, one
   byte no recovered reader consumes, and twenty condition ids; a match means
   the condition never lands. The two-byte packing is load-bearing: padded to
   four the stride would be 0x54, but the table walks 0x52 per entry. Values
   are the retail table at 0x006171A8; the walkers address the ids at +2. */
// GLOBAL: WIZ8 0x006171A8
W8ConditionImmunity g_condition_immunities[3] = {
    {20, 1, {W8_CONDITION_DISEASED,  W8_CONDITION_POISONED,  W8_CONDITION_UNCONSCIOUS,
             W8_CONDITION_IRRITATED, W8_CONDITION_AFRAID,    W8_CONDITION_INSANE,
             W8_CONDITION_ASLEEP,    W8_CONDITION_NAUSEATED, W8_CONDITION_NONE,
             W8_CONDITION_NONE,      W8_CONDITION_NONE,      W8_CONDITION_NONE,
             W8_CONDITION_NONE,      W8_CONDITION_NONE,      W8_CONDITION_NONE,
             W8_CONDITION_NONE,      W8_CONDITION_NONE,      W8_CONDITION_NONE,
             W8_CONDITION_NONE,      W8_CONDITION_NONE}},
    {22,
     0,
     {W8_CONDITION_DISEASED, W8_CONDITION_POISONED, W8_CONDITION_NAUSEATED, W8_CONDITION_IRRITATED,
      W8_CONDITION_AFRAID,   W8_CONDITION_ASLEEP,   W8_CONDITION_NONE,      W8_CONDITION_NONE,
      W8_CONDITION_NONE,     W8_CONDITION_NONE,     W8_CONDITION_NONE,      W8_CONDITION_NONE,
      W8_CONDITION_NONE,     W8_CONDITION_NONE,     W8_CONDITION_NONE,      W8_CONDITION_NONE,
      W8_CONDITION_NONE,     W8_CONDITION_NONE,     W8_CONDITION_NONE,      W8_CONDITION_NONE}},
    {17, 1, {W8_CONDITION_INSANE, W8_CONDITION_AFRAID, W8_CONDITION_NAUSEATED, W8_CONDITION_NONE,
             W8_CONDITION_NONE,   W8_CONDITION_NONE,   W8_CONDITION_NONE,      W8_CONDITION_NONE,
             W8_CONDITION_NONE,   W8_CONDITION_NONE,   W8_CONDITION_NONE,      W8_CONDITION_NONE,
             W8_CONDITION_NONE,   W8_CONDITION_NONE,   W8_CONDITION_NONE,      W8_CONDITION_NONE,
             W8_CONDITION_NONE,   W8_CONDITION_NONE,   W8_CONDITION_NONE,      W8_CONDITION_NONE}},
};

/* Rescan the character's condition countdowns from the top and write
   highest_condition, then resync the three formation copies, encumbrance when
   the dead threshold was crossed either way, the portrait pose and the
   portrait page if it is up. */
// FUNCTION: WIZ8 0x005237e0
void RecomputeCharacterHighestCondition(int party_slot)
{
    if (!g_status.buffers.XChar[party_slot].fOccupied) {
        srAssertFail("fCHAR_OCCUPIED(uiChar)", CONDITIONS_CPP, 0x183, 0);
    }
    W8Character* character = &g_status.buffers.Char[party_slot];
    W8Condition previous = character->highest_condition;
    for (int index = 0x13; index >= 0; --index) {
        if (character->uiCondition[index] != 0 || index == 0) {
            character->highest_condition = static_cast<W8Condition>(index);
            break;
        }
    }
    UpdateFormationSlotState(&g_status.formation, party_slot);
    if (gXStatus.fCombatMode) {
        UpdateFormationSlotState(&gXStatus.edited_formation, party_slot);
        UpdateFormationSlotState(&g_combat_state->saved_formation, party_slot);
    }
    if ((previous < W8_CONDITION_DEAD) != (character->highest_condition < W8_CONDITION_DEAD)) {
        RedistributePartyEncumbrance();
    }
    RequestPartySlotRedraw(party_slot);
    int pose = gXStatus.monster_manager_entries[party_slot].target_portrait_pose;
    if (character->highest_condition < W8_CONDITION_ASLEEP) {
        if (pose != 2) {
            goto done;
        }
        pose = 1;
    } else {
        if (pose == 2) {
            goto done;
        }
        pose = 2;
    }
    SetPortraitTargetPose(&gXStatus.monster_manager_entries[party_slot], pose);
done:
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
        RequestRedraw(W8_MAIN_REDRAW_CHARACTER_ACTION);
        RequestRedraw(W8_MAIN_REDRAW_PORTRAIT_PANEL);
    }
}

/* Settle a condition's enchantment record on the character: a bigger argument
   replaces the stored one, slot two rolls its bonus dice, and the top scan,
   redraw and stats rebuild follow. */
// FUNCTION: WIZ8 0x00523940
void ApplyCharacterCondition(int party_slot, W8EnchantmentSlot slot, int argument,
                             unsigned int duration, unsigned int percent)
{
    W8Character* character = &g_status.buffers.Char[party_slot];
    W8Enchantment* enchantment = &character->enchantments[slot];
    if (enchantment->power < static_cast<unsigned int>(argument)) {
        enchantment->power = argument;
        enchantment->percent = static_cast<unsigned short>(percent);
        enchantment->turns = duration;
        if (slot == W8_ENCHANTMENT_GUARDIAN_ANGEL) {
            enchantment->magnitude =
                static_cast<short>(RollDice(&g_spell_records[0x15].effect_dice) * argument);
            enchantment->magnitude =
                static_cast<short>(
                    (static_cast<unsigned int>(enchantment->magnitude) * percent) / 100) +
                enchantment->magnitude;
        }
        for (int scan = 7; scan >= 0; --scan) {
            if (character->enchantments[scan].turns > 0 || scan == 0) {
                character->enchantment_top = static_cast<W8EnchantmentSlot>(scan);
                break;
            }
        }
        RequestPartySlotRedraw(party_slot);
        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
            RequestRedraw(W8_MAIN_REDRAW_CHARACTER_ACTION);
            RequestRedraw(W8_MAIN_REDRAW_PORTRAIT_PANEL);
        }
        if (slot == W8_ENCHANTMENT_SUPERMAN) {
            gXStatus.sight_refresh_pending = true;
        }
    }
    RebuildConditionsAndDerivedStats(party_slot);
}

/* Setting a monster's condition runs the sameCountdown rescan, group recount
   and motion update as clearing it, plus immunity, argument and combat-hate
   handling on the way in. */
/* One condition's share of the aging tick: run its remaining turns down by
   the elapsed minutes, lifting it once they run out. POISONED also bleeds its
   stored strength pro rata over the remaining duration, with a hundred-sided
   roll covering the fractional part when the quotient floors to zero. */
// FUNCTION: WIZ8 0x005236A0
void TickCharacterCondition(unsigned int party_slot, W8Condition condition, unsigned int minutes)
{
    W8PartySlotRow* row = &g_status.buffers.XChar[party_slot];

    if (!row->fOccupied) {
        srAssertFail("fCHAR_OCCUPIED(uiSlot)",
                     "C:\\Projects\\Wizardry 8\\Local Code\\Conditions & Enchantments.cpp", 0x158,
                     0);
    }
    if (g_status.buffers.Char[party_slot].uiCondition[condition] == 0) {
        srAssertFail("gStatus.Char[uiSlot].uiCondition[uiCondition] > 0",
                     "C:\\Projects\\Wizardry 8\\Local Code\\Conditions & Enchantments.cpp", 0x159,
                     0);
    }
    if (g_status.buffers.Char[party_slot].uiCondition[condition] <= minutes) {
        RemoveCharacterCondition(party_slot, condition, true);
        return;
    }
    if (condition == W8_CONDITION_POISONED) {
        int strength = g_status.buffers.Char[party_slot].condition_argument;
        unsigned int lost = strength * minutes /
                            g_status.buffers.Char[party_slot].uiCondition[W8_CONDITION_POISONED];

        if (lost == 0 &&
            Random(100) <
                g_status.buffers.Char[party_slot].condition_argument * minutes * 100 /
                    g_status.buffers.Char[party_slot].uiCondition[W8_CONDITION_POISONED]) {
            lost = 1;
        }
        g_status.buffers.Char[party_slot].condition_argument = strength - lost;
    }
    g_status.buffers.Char[party_slot].uiCondition[condition] -= minutes;
}

// FUNCTION: WIZ8 0x00523C00
void SetMonsterCondition(int location_id, W8Condition condition, int duration, int argument,
                         W8TargetSource* target, char announce)
{
    unsigned int list_index;
    W8MonsterInfo* monster_info;
    W8MonsterRecord* record;
    W8MonsterGroup* monster_group;
    W8ConditionImmunity* immunity;
    unsigned char kind;
    int index;
    int old_duration;
    int slot;
    bool handled;

    if (argument != 0 && condition != W8_CONDITION_POISONED) {
        srAssertFail("(uiPoisonStrength == 0) || (uiCondition == COND_POISONED)",
                     "C:\\Projects\\Wizardry 8\\Local Code\\Conditions & Enchantments.cpp", 0x220,
                     0);
    }
    if (condition == W8_CONDITION_POISONED && argument == 0) {
        return;
    }
    switch (condition) {
    case W8_CONDITION_DISEASED:
    case W8_CONDITION_INFATUATED:
    case W8_CONDITION_DEAD:
    case W8_CONDITION_MISSING:
        duration = W8_CONDITION_INDEFINITE;
        break;
    default:
        if (duration == 0) {
            return;
        }
        break;
    }
    list_index = MonsterGetIndexByLocationID(
        0x23c, "C:\\Projects\\Wizardry 8\\Local Code\\Conditions & Enchantments.cpp", location_id,
        true);
    monster_info = MonsterGetScriptPartByLocationIndex(list_index);
    record = GetMonsterDataForInfo(monster_info);
    if (monster_info->hp_current == 0) {
        return;
    }
    kind = record->kind;
    for (immunity = g_condition_immunities; immunity < g_condition_immunities + 3; ++immunity) {
        if (immunity->kind == kind) {
            for (index = 0; index < 0x14; ++index) {
                if (condition == immunity->conditions[index]) {
                    return;
                }
            }
        }
    }
    old_duration = monster_info->uiCondition[condition];
    if (old_duration < duration) {
        monster_info->uiCondition[condition] = duration;
        if (old_duration == 0) {
            if (condition == W8_CONDITION_HEXED || condition == W8_CONDITION_BLIND) {
                RefreshMonsterSight(monster_info);
            } else if (condition == W8_CONDITION_TURNCOAT) {
                if (monster_info->ubDisposition == W8_DISPOSITION_NEUTRAL) {
                    monster_info->uiCondition[W8_CONDITION_TURNCOAT] = 0;
                    return;
                }
                SetMonsterHostility(monster_info,
                                    (monster_info->ubDisposition == W8_DISPOSITION_HOSTILE) + 1);
            }
        }
        slot = 0x13;
        while (monster_info->uiCondition[slot] == 0) {
            if (slot == 0) {
                break;
            }
            --slot;
        }
        monster_info->highest_condition = static_cast<W8Condition>(slot);
        list_index = GetMonsterGroupIndexByID(
            0x34e, "C:\\Projects\\Wizardry 8\\Local Code\\Conditions & Enchantments.cpp",
            monster_info->monster_group_id, true);
        monster_group = GetMonsterGroupByListIndex(list_index);
        RecountActiveMonsterGroupMembers(monster_group);
        if (monster_info->fActive) {
            MonsterInfoSetMotionless(monster_info,
                                     monster_info->highest_condition < W8_CONDITION_WEBBED ? 0 : 1);
        }
        /* The retail's mangled signature keeps condition an int, but 0x00523D8A
           tests it and 0x00523D8F bounds it unsigned, which is the same cast the
           three later uses of condition in this function already carry. */
        unsigned int condition_index = static_cast<unsigned int>(condition);
        if (old_duration == 0 && condition_index != 0 && condition_index <= 0x12) {
            SetMonsterSpellIcon(monster_info->p3D, static_cast<W8MonsterSpellIconId>(condition - 1),
                                true);
        }
        if (monster_info->fInCombat && TargetSourceIsCharacter(target, 0) && target->iChar != -1) {
            int* hate = &monster_info->pCombat->character_hate[target->iChar];
            record = GetMonsterDataForInfo(monster_info);
            *hate += (record->effective_level * static_cast<unsigned int>(condition)) / 3;
        }
        handled = true;
    } else {
        handled = false;
    }
    if (TargetSourceIsCharacter(target, 0) || TargetSourceIsMonster(target, 0)) {
        if (!target->fBackfire && !target->fReflection && target->target_diverted == 0) {
            monster_info->condition_target = *target;
        }
    }
    if (static_cast<unsigned int>(argument) > static_cast<unsigned int>(monster_info->condition_argument)) {
        monster_info->condition_argument = argument;
        handled = true;
    }
    RebuildMonsterDerivedStats(location_id);
    if (!handled) {
        return;
    }
    if (static_cast<unsigned int>(condition) >= 0xD) {
        ResetCombatSlot(&monster_info->Target);
    }
    if (static_cast<unsigned int>(condition) >= 0x12) {
        MonsterStartsDying(monster_info, announce);
        return;
    }
    if (announce != 0 && (gXStatus.fCombatMode || monster_info->party_threat.visible_to_player)) {
        wchar_t* name = GetMonsterName(monster_info, 0, 0);
        ShowNoticef(W8_FONT_PALETTE_RUST, L"%s %s!", name,
                    gppStringList[g_condition_notices[condition * 4 + 1]]);
    }
    if (monster_info->p3D->IsCycleInterruptable(monster_info->p3D->m_pRep->pending_cycle)) {
        StartMonsterCycle(monster_info, 0x14, 1);
    }
}
/* Clearing a monster's condition also re-derives its highest set condition
   index, recounts its group, and possibly stops it moving. */
// FUNCTION: WIZ8 0x00523F40
void ClearMonsterCondition(int location_id, W8Condition condition)
{
    unsigned int list_index;
    W8MonsterInfo* monster_info;
    W8MonsterGroup* monster_group;
    int slot;

    list_index = MonsterGetIndexByLocationID(
        0x2d0, "C:\\Projects\\Wizardry 8\\Local Code\\Conditions & Enchantments.cpp", location_id,
        true);
    monster_info = MonsterGetScriptPartByLocationIndex(list_index);
    if (monster_info->hp_current != 0) {
        if (monster_info->uiCondition[condition] == 0) {
            srAssertFail("pMonsterInfo->uiCondition[uiCondition] > 0",
                         "C:\\Projects\\Wizardry 8\\Local Code\\Conditions & Enchantments.cpp",
                         0x2d8, 0);
        }
        if (condition == W8_CONDITION_TURNCOAT &&
            monster_info->uiCondition[W8_CONDITION_TURNCOAT] != 0) {
            list_index = GetMonsterGroupIndexByID(
                0x2e0, "C:\\Projects\\Wizardry 8\\Local Code\\Conditions & Enchantments.cpp",
                monster_info->monster_group_id, true);
            monster_group = GetMonsterGroupByListIndex(list_index);
            SetMonsterHostility(monster_info, monster_group->ubDisposition);
        }
        if (gXStatus.fCombatMode || monster_info->party_threat.visible_to_player) {
            ShowNoticef(W8_FONT_PALETTE_RUST, gppStringList[0x244],
                        GetMonsterName(monster_info, 0, 0),
                        gppStringList[g_condition_notices[condition * 4]]);
        }
        monster_info->uiCondition[condition] = 0;
        slot = 0x13;
        while (monster_info->uiCondition[slot] == 0) {
            if (slot == 0) {
                break;
            }
            --slot;
        }
        monster_info->highest_condition = static_cast<W8Condition>(slot);
        list_index = GetMonsterGroupIndexByID(
            0x34e, "C:\\Projects\\Wizardry 8\\Local Code\\Conditions & Enchantments.cpp",
            monster_info->monster_group_id, true);
        monster_group = GetMonsterGroupByListIndex(list_index);
        RecountActiveMonsterGroupMembers(monster_group);
        if (monster_info->fActive) {
            MonsterInfoSetMotionless(monster_info,
                                     monster_info->highest_condition < W8_CONDITION_WEBBED ? 0 : 1);
        }
        if (condition != W8_CONDITION_NONE && condition < W8_CONDITION_MISSING) {
            SetMonsterSpellIcon(monster_info->p3D, static_cast<W8MonsterSpellIconId>(condition - 1),
                                false);
        }
        switch (condition) {
        case W8_CONDITION_HEXED:
        case W8_CONDITION_BLIND:
            RefreshMonsterSight(monster_info);
            RebuildMonsterDerivedStats(location_id);
            return;
        case W8_CONDITION_POISONED:
            monster_info->condition_argument = 0;
            RebuildMonsterDerivedStats(location_id);
            return;
        case W8_CONDITION_AFRAID:
            if (monster_info->fInCombat) {
                monster_info->pCombat->advancing = false;
            }
            break;
        default:
            break;
        }
        RebuildMonsterDerivedStats(location_id);
    }
}

/* Age one live monster condition. Poison loses its carried strength in the
   same proportion as its remaining duration, with a probabilistic one-point
   correction when integer division rounds the loss to zero. */
// FUNCTION: WIZ8 0x00524110
void TickMonsterCondition(int location_id, W8Condition condition, unsigned int minutes)
{
    W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(
        MonsterGetIndexByLocationID(0x313, CONDITIONS_CPP, location_id, true));

    if (monster_info->hp_current == 0) {
        return;
    }
    if (monster_info->uiCondition[condition] == 0) {
        srAssertFail("pMonsterInfo->uiCondition[uiCondition] > 0", CONDITIONS_CPP, 0x31b, 0);
    }
    if (monster_info->uiCondition[condition] <= minutes) {
        ClearMonsterCondition(location_id, condition);
        return;
    }
    if (condition == W8_CONDITION_POISONED) {
        int strength = monster_info->condition_argument;
        unsigned int lost = strength * minutes / monster_info->uiCondition[condition];

        if (lost == 0 &&
            Random(100) < strength * minutes * 100 / monster_info->uiCondition[condition]) {
            lost = 1;
        }
        monster_info->condition_argument = strength - lost;
    }
    monster_info->uiCondition[condition] -= minutes;
}

/* Setting a character's condition runs poison/immunity gates, the duration
   switch, trait gates, and the same rescan/notify/tail handling as the other
   transitions. */
// FUNCTION: WIZ8 0x00522FE0
unsigned char SetCharacterCondition(int party_slot, W8Condition condition, int duration,
                                    int argument, char value_5, char value_6)
{
    W8Character* character = &g_status.buffers.Char[party_slot];
    W8PartySlotRow* row = &g_status.buffers.XChar[party_slot];
    W8Condition old_highest;
    unsigned int old_duration;
    bool handled;

    if (!row->fOccupied) {
        srAssertFail("fCHAR_OCCUPIED(uiChar)",
                     "C:\\Projects\\Wizardry 8\\Local Code\\Conditions & Enchantments.cpp", 0x2a,
                     0);
    }
    if (argument != 0 && condition != W8_CONDITION_POISONED) {
        srAssertFail("(uiPoisonStrength == 0) || (uiCondition == COND_POISONED)",
                     "C:\\Projects\\Wizardry 8\\Local Code\\Conditions & Enchantments.cpp", 0x2d,
                     0);
    }
    if (condition == W8_CONDITION_POISONED && argument == 0) {
        return 0;
    }
    if (condition == W8_CONDITION_DEAD && CharacterHasTrait(character, W8_TRAIT_CHEAT_DEATH) &&
        character->uiCondition[W8_CONDITION_UNCONSCIOUS] < 7) {
        CheatDeathRevive(party_slot);
        return 0;
    }
    switch (condition) {
    case W8_CONDITION_DISEASED:
    case W8_CONDITION_INFATUATED:
    case W8_CONDITION_DEAD:
    case W8_CONDITION_MISSING:
        duration = W8_CONDITION_INDEFINITE;
        break;
    default:
        if (duration == 0) {
            return 0;
        }
        break;
    }
    if (g_status.world_suspended) {
        PostCharacterNotice(party_slot, gppStringList[0x242],
                            gppStringList[g_condition_notices[condition * 4]]);
        return 0;
    }
    switch (condition) {
    case W8_CONDITION_AFRAID:
        if (CharacterHasTrait(character, W8_TRAIT_FEARLESS)) {
            PostCharacterNotice(party_slot, gppStringList[0x180]);
            return 0;
        }
        /* fall through */
    case W8_CONDITION_DISEASED:
    case W8_CONDITION_IRRITATED:
    case W8_CONDITION_NAUSEATED:
    case W8_CONDITION_POISONED:
    case W8_CONDITION_ASLEEP:
        if (CharacterHasTrait(character, static_cast<W8Trait>(0x1e))) {
            return 0;
        }
        break;
    case W8_CONDITION_INSANE:
        if (duration == 9999) {
            break;
        }
        /* fall through */
    case W8_CONDITION_TURNCOAT:
        if (CharacterHasTrait(character, W8_TRAIT_MENTAL_CONDITION_IMMUNITY)) {
            PostCharacterNotice(party_slot, gppStringList[0x181]);
            return 0;
        }
        break;
    default:
        break;
    }
    old_highest = character->highest_condition;
    old_duration = character->uiCondition[condition];
    if (old_duration < static_cast<unsigned int>(duration)) {
        if (old_duration == 0) {
            if (condition == W8_CONDITION_HEXED || condition == W8_CONDITION_BLIND) {
                gXStatus.sight_refresh_pending = true;
            } else if (condition == W8_CONDITION_TURNCOAT) {
                SetTargetToCharacter(party_slot, true);
            }
        }
        character->uiCondition[condition] = duration;
        RecomputeCharacterHighestCondition(party_slot);
        handled = true;
    } else {
        handled = false;
    }
    if (static_cast<unsigned int>(character->condition_argument) < static_cast<unsigned int>(argument)) {
        character->condition_argument = argument;
    }
    RebuildConditionsAndDerivedStats(party_slot);
    if (!handled) {
        return 0;
    }
    if (condition == W8_CONDITION_DEAD) {
        CharacterDies(party_slot);
    }
    if (condition >= W8_CONDITION_DEAD) {
        SetTargetToCharacter(party_slot, false);
    }
    if (old_highest != character->highest_condition) {
        QueueConditionChangeReaction(character);
    }
    if (value_6 != 0) {
        if (condition == W8_CONDITION_MISSING && value_5 != 0) {
            PostCharacterNotice(party_slot, gppStringList[0x1d5]);
        } else {
            PostCharacterNotice(party_slot, L"%s!",
                                gppStringList[g_condition_notices[condition * 4 + 1]]);
        }
    }
    if ((party_slot < 0 || party_slot > 7 || !row->fOccupied || character->hp_current == 0 ||
         character->highest_condition > W8_CONDITION_BLIND) &&
        gXStatus.fCombatMode) {
        ClearPartySlotMonsterHighlights(party_slot);
    }
    return 1;
}

/* Copy every condition a character is under onto something else in the world.
   Condition seven carries an argument alongside its duration, so it is the one
   entry that is not just a duration. */
// FUNCTION: WIZ8 0x005241e0
void CopyCharacterConditionsToTarget(const W8Character* character, const int* target)
{
    W8TargetSource target_block;
    unsigned int condition;
    int duration;
    int argument;

    ResetTargetSource(&target_block);
    for (condition = 0; condition < W8_CONDITION_COUNT; ++condition) {
        duration = character->uiCondition[condition];
        if (duration != 0) {
            if (condition == W8_CONDITION_POISONED) {
                argument = character->condition_argument;
                duration = character->uiCondition[W8_CONDITION_POISONED];
            } else {
                argument = 0;
            }
            SetMonsterCondition(*target, static_cast<W8Condition>(condition), duration, argument,
                                &target_block, 0);
        }
    }
}

/* The same copy the other way round, from a monster onto one character. Every
   condition carries the monster's argument here, not only the seventh. */
// FUNCTION: WIZ8 0x00524250
void CopyMonsterConditionsToCharacter(W8Character* character, const W8MonsterInfo* monster_info)
{
    unsigned int condition;
    int duration;
    int argument;

    for (condition = 0; condition < W8_CONDITION_COUNT; ++condition) {
        duration = monster_info->uiCondition[condition];
        if (duration != 0) {
            argument = monster_info->condition_argument;
            if (condition == W8_CONDITION_POISONED) {
                duration = monster_info->uiCondition[W8_CONDITION_POISONED];
            }
            SetCharacterCondition(CharacterPointerToPartySlot(character),
                                  static_cast<W8Condition>(condition), duration, argument, 0, 0);
        }
    }
}

/* Empty one of a character's enchantment slots and find the highest one still
   in use, scanning down from the last. Emptying the sixth also raises the flag
   the interface watches. */
// FUNCTION: WIZ8 0x00523a80
void ClearCharacterEnchantmentSlot(int party_slot, W8EnchantmentSlot slot)
{
    W8Character* character;
    int scan;

    memset(&g_status.buffers.Char[party_slot].enchantments[slot], 0, sizeof(W8Enchantment));

    character = &g_status.buffers.Char[party_slot];
    for (scan = 7; scan >= 0; --scan) {
        if (character->enchantments[scan].turns > 0 || scan == 0) {
            character->enchantment_top = static_cast<W8EnchantmentSlot>(scan);
            break;
        }
    }

    RequestPartySlotRedraw(party_slot);
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
        RequestRedraw(W8_MAIN_REDRAW_CHARACTER_ACTION);
        RequestRedraw(W8_MAIN_REDRAW_PORTRAIT_PANEL);
    }
    RebuildConditionsAndDerivedStats(party_slot);
    if (slot == W8_ENCHANTMENT_SUPERMAN) {
        gXStatus.sight_refresh_pending = true;
    }
}

/* Run one of a character's enchantment slots down by the given number of
   turns, emptying it the same way ClearCharacterEnchantmentSlot does when
   nothing is left. */
// FUNCTION: WIZ8 0x00523b30
void TickCharacterEnchantmentSlot(int party_slot, W8EnchantmentSlot slot, unsigned int turns)
{
    unsigned int remaining = g_status.buffers.Char[party_slot].enchantments[slot].turns;

    if (remaining <= turns) {
        ClearCharacterEnchantmentSlot(party_slot, slot);
    } else {
        g_status.buffers.Char[party_slot].enchantments[slot].turns = remaining - turns;
    }
}

/* The monster-side counterpart of ApplyCharacterCondition: a bigger
   argument replaces the stored one, slot two rolls its bonus dice, a freshly
   applied slot adds the spell icon, and the stats rebuild and special-slot
   sight refresh follow. */
// FUNCTION: WIZ8 0x005242b0
void ApplyMonsterCondition(int location_id, W8EnchantmentSlot slot, int argument,
                           unsigned int duration, unsigned int percent)
{
    unsigned int monster_list_index =
        MonsterGetIndexByLocationID(0x38b, CONDITIONS_CPP, location_id, true);
    W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
    W8Enchantment* enchantment = &monster_info->enchantments[slot];
    if (enchantment->power <= static_cast<unsigned int>(argument)) {
        unsigned int previous = enchantment->turns;
        enchantment->power = argument;
        enchantment->percent = static_cast<unsigned short>(percent);
        enchantment->turns = duration;
        if (slot == W8_ENCHANTMENT_GUARDIAN_ANGEL) {
            int roll = RollDice(&g_spell_records[0x15].effect_dice);
            enchantment->magnitude =
                static_cast<short>(
                    (static_cast<unsigned int>(static_cast<unsigned short>(roll * argument)) *
                     percent) /
                    100) +
                static_cast<short>(roll * argument);
        }
        if (previous == 0) {
            SetMonsterSpellIcon(monster_info->p3D, static_cast<W8MonsterSpellIconId>(slot + 0x10),
                                true);
        }
        RebuildMonsterDerivedStats(location_id);
        if (slot == W8_ENCHANTMENT_SUPERMAN) {
            RefreshMonsterSight(monster_info);
        }
    }
}

/* Empty one of a monster's enchantment slots and tell the live engine object
   that the matching effect is over. */
// FUNCTION: WIZ8 0x00524390
void ClearMonsterEnchantmentSlot(int location_id, W8EnchantmentSlot slot)
{
    W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(
        MonsterGetIndexByLocationID(948, CONDITIONS_CPP, location_id, true));

    memset(&monster_info->enchantments[slot], 0, sizeof(W8Enchantment));
    SetMonsterSpellIcon(monster_info->p3D, static_cast<W8MonsterSpellIconId>(slot + 0x10), false);
    RebuildMonsterDerivedStats(location_id);
    if (slot == W8_ENCHANTMENT_SUPERMAN) {
        RefreshMonsterSight(monster_info);
    }
}

/* Run one of a monster's enchantment slots down by the given number of turns,
   emptying it when nothing is left. The look-up is repeated rather than
   reused, which is what the two separate index calls show. */
// FUNCTION: WIZ8 0x00524400
void TickMonsterEnchantmentSlot(int location_id, W8EnchantmentSlot slot, unsigned int turns)
{
    W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(
        MonsterGetIndexByLocationID(969, CONDITIONS_CPP, location_id, true));
    unsigned int remaining = monster_info->enchantments[slot].turns;

    if (turns < remaining) {
        monster_info->enchantments[slot].turns = remaining - turns;
        return;
    }

    ClearMonsterEnchantmentSlot(location_id, slot);
}

/* Lift one condition from everybody in the party who is under it. */
// FUNCTION: WIZ8 0x005246c0
void RemoveConditionFromParty(W8Condition condition)
{
    unsigned int party_slot;

    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_status.buffers.XChar[party_slot].fOccupied &&
            g_status.buffers.Char[party_slot].uiCondition[condition] != 0) {
            RemoveCharacterCondition(party_slot, condition, true);
        }
    }
}

/* Lift one condition from everybody in the world - the party first, then every
   live monster. The monster count is re-read every iteration because lifting a
   condition can remove one. */
// FUNCTION: WIZ8 0x005244a0
void RemoveConditionFromEveryone(W8Condition condition)
{
    unsigned int party_slot;
    unsigned int monster_index;
    W8MonsterInfo* monster_info;

    for (party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_status.buffers.XChar[party_slot].fOccupied &&
            g_status.buffers.Char[party_slot].uiCondition[condition] != 0) {
            RemoveCharacterCondition(party_slot, condition, true);
        }
    }

    for (monster_index = 0; monster_index < PLLength(gXStatus.plsMonsterList); ++monster_index) {
        monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
        if (monster_info->uiCondition[condition] != 0) {
            ClearMonsterCondition(monster_info->location_id, condition);
        }
    }
}

/* Strip all eight enchantment slots from every occupied party member and every
   live monster, rescanning each character's top slot and flagging the special
   slot's refresh. The monster count is re-read every iteration because
   clearing one can remove a monster from the list. */
// FUNCTION: WIZ8 0x00524540
void RemoveAllEnchantments(void)
{
    for (unsigned int enchantment = 0; enchantment < 8; ++enchantment) {
        for (int party_slot = 0; party_slot < 8; ++party_slot) {
            W8Character* character = &g_status.buffers.Char[party_slot];

            if (g_status.buffers.XChar[party_slot].fOccupied &&
                character->enchantments[enchantment].turns != 0) {
                memset(&character->enchantments[enchantment], 0, sizeof(W8Enchantment));
                int top = 7;
                W8Enchantment* scan = &character->enchantments[W8_ENCHANTMENT_BODY_OF_STONE];

                do {
                    if (scan->turns != 0 || top == 0) {
                        character->enchantment_top = static_cast<W8EnchantmentSlot>(top);
                        break;
                    }
                    --top;
                    --scan;
                } while (top > -1);
                RequestPartySlotRedraw(party_slot);
                if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
                    RequestRedraw(W8_MAIN_REDRAW_CHARACTER_ACTION);
                    RequestRedraw(W8_MAIN_REDRAW_PORTRAIT_PANEL);
                }
                RebuildConditionsAndDerivedStats(party_slot);
                if (enchantment == W8_ENCHANTMENT_SUPERMAN) {
                    gXStatus.sight_refresh_pending = true;
                }
            }
        }
        for (unsigned int index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(index);

            if (monster_info->enchantments[enchantment].turns != 0) {
                int location_id = monster_info->location_id;

                monster_info = MonsterGetScriptPartByLocationIndex(
                    MonsterGetIndexByLocationID(0x3b4, CONDITIONS_CPP, location_id, true));
                memset(&monster_info->enchantments[enchantment], 0, sizeof(W8Enchantment));
                SetMonsterSpellIcon(monster_info->p3D,
                                    static_cast<W8MonsterSpellIconId>(enchantment + 0x10), false);
                RebuildMonsterDerivedStats(location_id);
                if (enchantment == W8_ENCHANTMENT_SUPERMAN) {
                    RefreshMonsterSight(monster_info);
                }
            }
        }
    }
}

/* Lift every condition from the whole party. The tenth is left alone, the same
   one death leaves alone, and the last two of the twenty are outside the sweep
   entirely. */
// FUNCTION: WIZ8 0x00524720
void RemoveAllConditionsFromParty(void)
{
    unsigned int condition;
    unsigned int party_slot;

    for (condition = 0; condition < W8_CONDITION_CLEARABLE_COUNT; ++condition) {
        if (condition == W8_CONDITION_INFATUATED) {
            continue;
        }
        for (party_slot = 0; party_slot < 8; ++party_slot) {
            if (g_status.buffers.XChar[party_slot].fOccupied &&
                g_status.buffers.Char[party_slot].uiCondition[condition] != 0) {
                RemoveCharacterCondition(party_slot, static_cast<W8Condition>(condition), true);
            }
        }
    }
}

/* Bind a monster to one of a character's two dependence slots: the monster
   remembers it is bound, a slot-one binding retires the monster's group from
   the encounter budget, and the character's condition record keeps the level
   it happened on, the monster's id and the live flag before the derived
   stats rebuild. */
// FUNCTION: WIZ8 0x00524780
void BindMonsterToCharacterDependence(unsigned int party_slot, unsigned int dependence_slot,
                                      int monster_id)
{
    W8MonsterInfo* monster_info;

    if (party_slot >= 8) {
        srAssertFail("uiChar < MAX_CHARS", CONDITIONS_CPP, 0x447, 0);
    }
    if (dependence_slot >= 2) {
        srAssertFail("uiDependence < DEPEND_COND_COUNT", CONDITIONS_CPP, 0x448, 0);
    }
    if (monster_id == -1) {
        srAssertFail("iMonsterID != -1", CONDITIONS_CPP, 0x449, 0);
    }

    monster_info = MonsterGetScriptPartByLocationIndex(
        MonsterGetIndexByLocationID(0x44b, CONDITIONS_CPP, monster_id, true));
    monster_info->condition_binding_mask = static_cast<unsigned char>(
        monster_info->condition_binding_mask | (1 << dependence_slot));
    if (dependence_slot == 1) {
        RetireMonsterGroupAndAllies(GetMonsterGroupByListIndex(
            GetMonsterGroupIndexByID(0x455, CONDITIONS_CPP, monster_info->monster_group_id, true)));
    }

    g_status.buffers.Char[party_slot].conditions[dependence_slot].level_acquired =
        g_status.current_level;
    g_status.buffers.Char[party_slot].conditions[dependence_slot].source_monster =
        monster_id;
    g_status.buffers.Char[party_slot].conditions[dependence_slot].active = true;
    RebuildConditionsAndDerivedStats(party_slot);
}
