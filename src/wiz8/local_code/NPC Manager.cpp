#include "wiz8/local_code/PC_Item.h"
#include "wiz8/layouts/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/Factions.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/item_spawning.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/CombatHostility.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/engine_code/Spells.h"
#include "random.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/fact_state.h"
#include "wiz8/location_variables.h"
#include "wiz8/layouts/npc_state.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/npc_items.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/npc_script_file.h"
#include "wiz8/layouts/item_instance.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/xstatus.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/sr_api.h"
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/local_code/Factions.h"
#include "wiz8/local_code/Sight.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/chunk.h"
#include "FileMan.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_screens/NPCInteractionSubscreen.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/utility.h"
#include "wiz8/chunk.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"
#include "FileMan.h"

#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include "wiz8/layouts/game_status.h"
#include "wiz8/engine_code/GameData.h"

/*
 * Local Code\NPC Manager.cpp.
 *
 * The runtime side of an NPC: the state record that pairs a database entry
 * with the monster standing in the world for it, the short list of facts it
 * has been told, and the small predicates the dialogue and trading code asks
 * about it.
 */

#define NPC_MANAGER_CPP "C:\\Projects\\Wizardry 8\\Local Code\\NPC Manager.cpp"

enum { W8_NPC_TOPIC_SLOTS = 5, W8_NPC_FACT_SLOTS = 14 };

/* The NPC kind that will not trade at all. */
enum { W8_NPC_KIND_NO_TRADE = 0x14 };

/* The one item a trader always accepts regardless of what it is worth, and the
   value everything else has to clear. */
enum { W8_NPC_ALWAYS_TRADED_ITEM = 0x29f, W8_NPC_MINIMUM_TRADE_VALUE = 300 };

/* The two thresholds the disposition band is cut at. */
enum { W8_NPC_DISPOSITION_HOSTILE = 0x21, W8_NPC_DISPOSITION_FRIENDLY = 0x42 };

/* 0x00689F94: every NPC state, held in the shared growable vector. */
// GLOBAL: WIZ8 0x00689F94
W8GrowableVector<W8NpcState*>* g_npc_states;

/* Whether the NPC's database entry carries the value at 0x002 at all. */
// FUNCTION: WIZ8 0x0050aa00
bool NpcRecordHasValue002(W8NpcState* npc)
{
    return npc->record->trade_pool_002 != 0;
}

/* The NPC's effective disposition: the state's byte plus its bound monster's
   modifier, forced fully hostile while that monster is turned. A factioned
   NPC instead reports its faction score unless the record keeps the answer
   static or the faction holds nothing toward the party; a factioned NPC whose
   allied faction a living front-rank party RPC shares answers fifty. */
// FUNCTION: WIZ8 0x0050A280
char GetNpcDisposition(W8NpcState* npc)
{
    char disposition = npc->disposition;
    char faction_disposition;
    W8MonsterInfo* monster_info;
    W8NpcState* bound;
    int bound_index;
    unsigned int slot;

    if (npc->has_monster != 0 && npc->is_present != 0) {
        monster_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0x2a1, NPC_MANAGER_CPP, npc->location_id, 1));
        if (monster_info != 0) {
            disposition += monster_info->effect_2de;
            if (monster_info->uiCondition[W8_CONDITION_TURNCOAT] > 0) {
                disposition = 0x64;
            }
        }
    }
    if (npc->record->faction_5f == 0) {
        return disposition;
    }
    faction_disposition = GetFactionDispositionScore(npc->record->faction_5f);
    if (npc->record->monster_bound_054 != 0) {
        return faction_disposition;
    }
    if (GetFactionDispositionToward(npc->record->faction_5f, W8_FACTION_PARTY) == 0) {
        return faction_disposition;
    }
    if (npc->record->allied_faction_6e != 0) {
        for (slot = 0; slot < 2; ++slot) {
            if (g_status_685170.buffers.XChar[slot].fOccupied != 0 &&
                g_status_685170.buffers.Char[slot].hp_current > 0) {
                bound_index = g_status_685170.buffers.XChar[slot].npc_index;
                if (g_npc_states != 0) {
                    bound = *g_npc_states->GetAt(bound_index);
                    if (bound != 0 && bound->binding_unavailable == 0 &&
                        bound->record->faction_5f == npc->record->allied_faction_6e) {
                        return 0x32;
                    }
                }
            }
        }
    }
    return disposition;
}

/* Which of the three disposition bands the NPC falls in. The bands are cut at
   0x21 and 0x42, and the hostile band answers two rather than zero. */
/* The NPC's effective disposition: the stored byte adjusted by its monster's
   effect, pinned to 'd' while the monster carries condition 0x0d, then replaced
   by the faction score when the record names a faction - and finally pinned
   friendly while a bound lead NPC belongs to the record's ally faction. */

// FUNCTION: WIZ8 0x0050a500
unsigned char GetNpcDispositionBand(W8NpcState* npc)
{
    char disposition = GetNpcDisposition(npc);

    if (disposition < W8_NPC_DISPOSITION_HOSTILE) {
        return 2;
    }
    return disposition < W8_NPC_DISPOSITION_FRIENDLY;
}

/* Test a placement near the party without retaining the position. */
// FUNCTION: WIZ8 0x0050b2d0
bool CanPlaceNpcNearParty(int party_slot)
{
    srVector3T<float> position;

    return ProbeNpcPlacementNearParty(party_slot, 0, &position);
}

/* Whether the NPC will talk about one topic. Topics are stored one more than
   they name, so zero can mean an empty slot. */
// FUNCTION: WIZ8 0x0050c190
bool NpcHasTopic(W8NpcState* npc, int topic)
{
    int slot;

    for (slot = 0; slot < W8_NPC_TOPIC_SLOTS; ++slot) {
        if (npc->topics[slot] == topic + 1) {
            return true;
        }
    }
    return false;
}

/* Tell the NPC one fact, in the first empty slot. A full list silently drops
   it. */
// FUNCTION: WIZ8 0x0050dd50
void TellNpcFact(W8NpcState* npc, short fact)
{
    int slot;

    for (slot = 0; slot < W8_NPC_FACT_SLOTS; ++slot) {
        if (npc->known_facts[slot] == 0) {
            npc->known_facts[slot] = fact;
            return;
        }
    }
}

/* Whether the NPC has already been told a fact. The scan stops at the first
   empty slot, so the list is packed from the front. */
// FUNCTION: WIZ8 0x0050dd10
bool NpcKnowsFact(W8NpcState* npc, unsigned int fact)
{
    int slot;

    for (slot = 0; slot < W8_NPC_FACT_SLOTS; ++slot) {
        if (npc->known_facts[slot] == 0) {
            return false;
        }
        if ((unsigned int)npc->known_facts[slot] == fact) {
            return true;
        }
    }
    return false;
}

/* The NPC state at one index, skipping a released binding. Out-of-range
   indices are clamped to the front by the shared vector rather than refused. */
// FUNCTION: WIZ8 0x0050b800
W8NpcState* GetNpcState(int index)
{
    W8NpcState* npc;

    if (g_npc_states == 0) {
        return 0;
    }
    npc = *g_npc_states->GetAt(index);
    if (npc == 0) {
        return 0;
    }
    if (npc->binding_unavailable != 0) {
        return 0;
    }
    return npc;
}

// FUNCTION: WIZ8 0x0050b830
W8NpcState* GetNpcStateByKind(int kind)
{
    int count = g_npc_states->count;
    W8GrowableVector<W8NpcState*>* npc_states = g_npc_states;
    int index = 0;

    if (count > 0) {
        do {
            W8NpcState** element;
            W8NpcState* npc;

            if (index < count) {
                element = &npc_states->data[index];
            } else {
                element = npc_states->data;
            }
            npc = *element;

            if (npc->record->kind == kind) {
                return npc;
            }
            ++index;
        } while (index < count);
    }
    return 0;
}

/* Whether the NPC attached to either of the first two party rows carries the
   given name style with an unreleased binding, while that row's lead stays
   under level fifteen. */
// FUNCTION: WIZ8 0x0050B8F0
bool NpcLeadHasNameStyle(unsigned int kind)
{
    if (g_status_685170.buffers.XChar[0].fOccupied != 0) {
        W8NpcState* npc = 0;
        if (g_npc_states != 0) {
            int index = g_status_685170.buffers.XChar[0].npc_index;
            W8NpcState** slot = g_npc_states->data;
            if (index < g_npc_states->count) {
                slot += index;
            }
            npc = *slot;
            if (npc != 0 && npc->binding_unavailable != 0) {
                npc = 0;
            }
        }
        if (npc->name_style == kind && g_status_685170.buffers.Char[0].highest_condition < 0xf) {
            return 1;
        }
    }
    if (g_status_685170.buffers.XChar[1].fOccupied != 0) {
        W8NpcState* npc = 0;
        if (g_npc_states != 0) {
            int index = g_status_685170.buffers.XChar[1].npc_index;
            W8NpcState** slot = g_npc_states->data;
            if (index < g_npc_states->count) {
                slot += index;
            }
            npc = *slot;
            if (npc != 0 && npc->binding_unavailable != 0) {
                npc = 0;
            }
        }
        if (npc->name_style == kind && g_status_685170.buffers.Char[1].highest_condition < 0xf) {
            return 1;
        }
    }
    return 0;
}

/* The monster standing in the world for this NPC, if one is. */
// FUNCTION: WIZ8 0x0050a3c0
W8MonsterInfo* GetNpcMonsterInfo(W8NpcState* npc)
{
    if (!npc->has_monster || !npc->is_present) {
        return 0;
    }
    return MonsterGetScriptPartByLocationIndex(
        MonsterGetIndexByLocationID(673, NPC_MANAGER_CPP, npc->location_id, 1));
}

/* The monster manager entry this NPC's group occupies, if its database entry
   says it has a group and it is in one. */
// FUNCTION: WIZ8 0x0050b870
W8MonsterManagerEntry* GetNpcGroupEntry(W8NpcState* npc)
{
    if (npc->record->has_group == 0) {
        return 0;
    }
    if (!npc->is_grouped) {
        return 0;
    }
    return &gXStatus.monster_manager_entries[npc->group_index];
}

/* The party character occupying this NPC's group slot. */
// FUNCTION: WIZ8 0x0050b8b0
W8Character* GetNpcGroupCharacter(W8NpcState* npc)
{
    if (npc->record->has_group == 0) {
        return 0;
    }
    if (!npc->is_grouped) {
        return 0;
    }
    return &g_status_685170.buffers.Char[npc->group_index];
}

/* Whether an NPC would take one item in trade. The kind that trades in nothing
   refuses outright, one particular item is always taken, and everything else
   has to be worth enough. */
// FUNCTION: WIZ8 0x0050a9c0
char WillNpcTradeForItem(W8NpcState* npc, W8ItemInstance* item)
{
    if (npc->record->kind == W8_NPC_KIND_NO_TRADE) {
        return 0;
    }
    if (item->iItemNo == W8_NPC_ALWAYS_TRADED_ITEM) {
        return 1;
    }
    return GetItemStackValue(item) >= W8_NPC_MINIMUM_TRADE_VALUE;
}

/* How many of the two leading party slots are occupied. Written as nested
   tests rather than a count, which is why the first slot is read twice. */
// FUNCTION: WIZ8 0x0050b9b0
unsigned char CountLeadingPartySlots(void)
{
    if (g_status_685170.buffers.XChar[0].fOccupied != 0) {
        if (g_status_685170.buffers.XChar[1].fOccupied != 0) {
            return 2;
        }
        if (g_status_685170.buffers.XChar[0].fOccupied != 0) {
            return 1;
        }
    }
    if (g_status_685170.buffers.XChar[1].fOccupied != 0) {
        return 1;
    }
    return 0;
}

/* The NPC-index consumers at 0x0050CC14/0x0050E025 address the first
   word at 0x00619DF8. Service lookup starts one word into each row.
   The five mutable race bytes at 0x00619EAC are a separate object. */
struct W8NpcServiceRow {
    unsigned int npc_id;
    unsigned int service_id;
    unsigned int bit;
};
// GLOBAL: WIZ8 0x00619DF8
const W8NpcServiceRow g_npc_services[] = {
    {0x46, 2, W8_NPC_SERVICE_ARNIKA},
    {0x47, 3, W8_NPC_SERVICE_TRYNTON},
    {0x50, 4, W8_NPC_SERVICE_SWAMP},
    {0x48, 5, W8_NPC_SERVICE_MARTEN_BLUFF},
    {0x49, 7, W8_NPC_SERVICE_SEA_CAVES},
    {0x4d, 8, W8_NPC_SERVICE_BAYJIN},
    {0x4c, 9, W8_NPC_SERVICE_RAPAX},
    {0x4b, 10, W8_NPC_SERVICE_RIFT},
    {0x4a, 11, W8_NPC_SERVICE_MT_GIGAS},
    {0x4e, 12, W8_NPC_SERVICE_ASCENSION},
    {0x51, 13, W8_NPC_SERVICE_RAPAX_CAMP},
    {0x4f, 14, W8_NPC_SERVICE_CIRCLE},
    {0x4d, 15, W8_NPC_SERVICE_GIGAS_CAVES},
    {0x52, 6, W8_NPC_SERVICE_MTN_PASS},
    {0, 0xffffffff, 0},
};

// GLOBAL: WIZ8 0x00619EAC
unsigned char g_npc_join_races[5] = {13, 12, 14, 15, 11};

/* Whether the NPC wants the offered item: it matches one of the record's
   three wanted entries by id or by the shared 0x83 name kind, and a grouped
   NPC whose member already carries more than one declines. */
// FUNCTION: WIZ8 0x0050DC50
bool NpcWantsItem0050DC50(W8NpcState* npc, W8ItemInstance* item)
{
    int index;
    const short* wanted = npc->record->character.wanted_item_ids_1f7;

    for (index = 0; index < 3; ++index) {
        int wanted_id = wanted[index] - 1;
        if (wanted_id < 0) {
            continue;
        }
        if (wanted_id == item->iItemNo) {
            break;
        }
        if (g_item_records[wanted_id].unidentified_name_index != 0x83) {
            continue;
        }
        if (g_item_records[item->iItemNo].unidentified_name_index == 0x83) {
            break;
        }
    }
    if (index == 3) {
        return 0;
    }
    if (npc->is_grouped != 0 &&
        CountItemOnCharacter(&g_status_685170.buffers.Char[npc->group_index], item->iItemNo, 0, 2) >
            1) {
        return 0;
    }
    return 1;
}

// FUNCTION: WIZ8 0x0050ddc0
void ReturnDismissedNpcItems(W8NpcState* npc, W8Character* character)
{
    bool returned = false;
    bool dropped = false;
    int slot;
    for (slot = 0; slot < 12; ++slot) {
        W8ItemInstance* item = &character->EquippedItem[slot];
        if (item->iItemNo != -1 && CanUnequipSlotItem(character, slot) &&
            !NpcWantsItem0050DC50(npc, item)) {
            if (AddItemToPartyOrDrop(item, 0)) {
                returned = true;
            } else {
                dropped = true;
            }
        }
    }
    for (slot = 0; slot < 8; ++slot) {
        W8ItemInstance* item = &character->backpack[slot];
        if (item->iItemNo != -1 && !NpcWantsItem0050DC50(npc, item)) {
            if (AddItemToPartyOrDrop(item, 0)) {
                returned = true;
            } else {
                dropped = true;
            }
        }
    }
    if (returned) {
        ShowNoticef(0, gppStringList[0x7f4 / 4], npc->record->source_name_004);
    }
    if (dropped) {
        ShowNoticef(0, gppStringList[0x7f8 / 4], npc->record->source_name_004);
    }
}

// FUNCTION: WIZ8 0x0050b590
int DismissNpcFromParty(int party_slot, int /*unused*/, bool skip_spawn, bool neutral)
{
    W8PartySlotRow* row = &g_status_685170.buffers.XChar[party_slot];
    if (row->npc_index == -1) {
        return 0;
    }
    srVector3T<float> position;
    if (!skip_spawn) {
        srVector3T<float> placement;
        ProbeNpcPlacementNearParty(party_slot, 1, &placement);
        position = placement;
    }
    W8NpcState* npc = GetNpcState(row->npc_index);
    if (npc == 0 || npc->binding_unavailable) {
        return 0;
    }
    W8Character* character = &g_status_685170.buffers.Char[party_slot];
    *npc->character = *character;
    npc->is_grouped = false;
    ReleaseNpcScriptFile0055A0A0(npc->script_file);
    npc->script_file = 0;
    RemoveCharacterFromParty(party_slot, 0);
    memset(character, 0, sizeof(*character));
    memset(row, 0, sizeof(*row));
    /* Retail clears all 0x118 bytes, including the embedded vector's vfptr. */
    memset(static_cast<void*>(&gXStatus.monster_manager_entries[party_slot]), 0,
           sizeof(W8MonsterManagerEntry));
    row->npc_index = -1;
    if (npc->character->highest_condition != 18 && !skip_spawn) {
        W8MonsterRecord* records;
        LoadMonsterDatabase(&records);
        unsigned int species;
        for (species = 0; species < gXStatus.uiMonstersInDatabase; ++species) {
            if ((records[species].flags_0d0 & 1) != 0 &&
                records[species].npc_kind_0cd == npc->name_style) {
                break;
            }
        }
        FreeIfNotNull(records);
        if (species == gXStatus.uiMonstersInDatabase) {
            return 0;
        }
        W8MonsterGroup* group = CreateGroup(species, 1, &position, 1, 0, 1);
        W8MonsterInfo* monster = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0x6c7, NPC_MANAGER_CPP, group->leader_location_id, 1));
        if (monster != 0) {
            CopyCharacterConditionsToTarget(npc->character, &monster->location_id);
            if (monster->uiCondition[17] == 9999) {
                unsigned int stamina = static_cast<unsigned int>(npc->character->uiStaminaMax);
                if (static_cast<unsigned int>(npc->character->stamina) < stamina) {
                    stamina = static_cast<unsigned int>(npc->character->stamina);
                }
                monster->stamina = stamina;
            }
            if (neutral) {
                SetMonsterGroupHostility(group, 0, 0);
                group->forced_neutral = 1;
            }
        }
    }
    RequestRedraw(~0U);
    ReturnDismissedNpcItems(npc, npc->character);
    npc->dismissed_flag = 1;
    memset(&npc->dismissed_timer, 0, sizeof(npc->dismissed_timer));
    npc->marked_e9 = 1;
    return 1;
}

/* The per-frame pass over a bound NPC's party slot: a dying member is handed
   its held item back and removed, a flagged one removed outright, then the
   NPC's placement is probed - failure complains, success runs the level's
   service check and queues either the scripted group action or the slot's
   ambient event. */
// FUNCTION: WIZ8 0x0050B3B0
void UpdateNpcPartyMember0050B3B0(int party_slot)
{
    W8NpcState* npc = GetNpcState(g_status_685170.buffers.XChar[party_slot].npc_index);
    W8Character* character = &g_status_685170.buffers.Char[party_slot];
    srVector3T<float> position;

    if (character->highest_condition == 0x12) {
        ShowNoticef(0, gppStringList[0x7d4], character->name);
        StashDepartingCharacterItems005223A0(character);
        RemoveCharacterFromParty(party_slot, 0);
        return;
    }
    if (character->highest_condition == 0x13) {
        RemoveCharacterFromParty(party_slot, 0);
        return;
    }
    if (ProbeNpcPlacementNearParty(party_slot, 0, &position) == 0) {
        ShowNoticef(0, gppStringList[0x7d5]);
        return;
    }
    int band = GetLevelBand(g_status_685170.current_level);
    int service = 0;
    if (g_npc_services[0].service_id != 0xffffffff) {
        while (g_npc_services[service].service_id != 0xffffffff) {
            if (g_npc_services[service].service_id == static_cast<unsigned int>(band)) {
                if ((npc->record->service_flags & g_npc_services[service].bit) != 0) {
                    if (character->highest_condition > 0xe) {
                        npc->marked_e9 = 1;
                        BeginScriptedWorldAction();
                        QueueNpcMessageLine(W8_NPC_MSG_GROUP_ACTION, party_slot);
                        return;
                    }
                    QueueCharacterEvent(character, 0x53, 0, g_effect_argument_005ed8c8,
                                        g_effect_argument_005ed914);
                    return;
                }
                break;
            }
            ++service;
        }
    }
    if (character->highest_condition < 0xf) {
        QueueCharacterEvent(character, g_effect_005ee69c, 0, g_effect_argument_005ed8c8,
                            g_effect_argument_005ed914);
    }
}

// FUNCTION: WIZ8 0x0050b160
bool RecruitNpcIntoParty(W8NpcState* npc)
{
    if (npc->record->has_group == 0) {
        return false;
    }
    int party_slot = AddCharacterToParty(npc->character, npc->partner_index_2c);
    if (party_slot == -1) {
        return false;
    }
    int index;
    for (index = 0; index < 0x29; ++index) {
        if (g_status_685170.buffers.Char[party_slot].skills[index].points_02 > 0) {
            g_status_685170.buffers.Char[party_slot].skills[index].available_13 = true;
        }
    }
    RefreshCharacterSkillAvailability00553CD0(&g_status_685170.buffers.Char[party_slot]);
    if (npc->has_monster && npc->is_present) {
        W8MonsterInfo* monster = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0x2a1, NPC_MANAGER_CPP, npc->location_id, 1));
        if (monster != 0) {
            CopyMonsterConditionsToCharacter(&g_status_685170.buffers.Char[party_slot], monster);
            RemoveMonster(
                MonsterGetIndexByLocationID(0x5bf, NPC_MANAGER_CPP, monster->location_id, 1), 1);
        }
    }
    npc->group_index = static_cast<signed char>(party_slot);
    npc->is_grouped = true;
    npc->is_present = false;
    npc->marked_e9 = 0;
    npc->pending_restore = 0;
    ReleaseNpcScriptFile0055A0A0(npc->script_file);
    npc->script_file = 0;
    ReloadNpcScriptResources(npc);

    /* Retail writes the race into this initially populated list, then tests
       the value just written. Preserve that assignment and early exit. */
    unsigned char race = 0;
    for (index = 0; index < 5; ++index) {
        race = static_cast<unsigned char>(npc->record->character.race);
        g_npc_join_races[index] = race;
        if (race != 0) {
            break;
        }
    }
    if (index == 5) {
        return true;
    }
    for (index = 0; index < 5; ++index) {
        if (g_status_685170.rpc_races_243a[index] == race) {
            return true;
        }
    }
    for (index = 0; index < 5; ++index) {
        if (g_status_685170.rpc_races_243a[index] == 0) {
            g_status_685170.rpc_races_243a[index] = race;
            break;
        }
    }
    return true;
}

/* 0x00619F18: the name a fact substitutes, and 0x00689F60 the buffer it is
   copied into so the caller always gets a writable one. */
// GLOBAL: WIZ8 0x00619F18
const char g_substituted_npc_name[] = "RFS81B";
// GLOBAL: WIZ8 0x00689F60
char g_npc_name_buffer[52];

/* The name style that admits a substituted name: the unfixed RFS-81 record,
   renamed to RFS81B once the fix fact is set. */

/* The engine object standing in the world for this NPC. */
// FUNCTION: WIZ8 0x0050a400
W8Monster* GetNpcMonster(W8NpcState* npc)
{
    W8MonsterInfo* monster_info;

    if (!npc->has_monster || !npc->is_present) {
        return 0;
    }
    monster_info = MonsterGetScriptPartByLocationIndex(
        MonsterGetIndexByLocationID(673, NPC_MANAGER_CPP, npc->location_id, 1));
    if (monster_info == 0) {
        return 0;
    }
    return monster_info->p3D;
}

/* Move the NPC into one disposition band. Each band is written as one
   representative value rather than a range, and a band it is already in is
   left alone - which is why the current band is computed twice. */
// FUNCTION: WIZ8 0x0050a520
void SetNpcDispositionBand(W8NpcState* npc, char band)
{
    char current;

    GetNpcDisposition(npc);
    current = GetNpcDisposition(npc);
    if (current < W8_NPC_DISPOSITION_HOSTILE) {
        current = 2;
    } else {
        current = current < W8_NPC_DISPOSITION_FRIENDLY;
    }
    if (current == band) {
        return;
    }
    if (band == 2) {
        npc->disposition = 0x19;
        return;
    }
    npc->disposition = band == 1 ? 0x32 : 0x4b;
}

/* Resume (or leave hostile) the NPC after its scripted pause: clamp the
   disposition into the friendly band, then re-flag its monster group's
   hostility from `enabled`. */
// FUNCTION: WIZ8 0x0050AE40
void ResumeNpc(W8NpcState* npc, int enabled)
{
    W8MonsterInfo* monster_info = GetNpcMonsterInfo(npc);

    GetNpcDisposition(npc);
    if (GetNpcDisposition(npc) > ' ') {
        npc->disposition = 0x19;
    }
    if (monster_info != 0) {
        if (static_cast<char>(enabled) != 0) {
            SetMonsterGroupHostilityByID(monster_info->monster_group_id, 1, 0);
            return;
        }
        SetMonsterGroupHostilityByID(monster_info->monster_group_id, 0, 0);
    }
}

/* Whether any NPC of one kind is in the world, and what its own byte at 0x04
   says - the two answers are the same value, so a kind that is not there is
   indistinguishable from one whose byte is zero. */
// FUNCTION: WIZ8 0x0050dd80
unsigned char FindNpcOfKind(int kind)
{
    int index;
    W8NpcState* npc;

    for (index = 0; index < g_npc_states->GetCount(); ++index) {
        npc = *g_npc_states->GetAt(index);
        if (npc->record->kind == kind) {
            if (npc == 0) {
                break;
            }
            return static_cast<unsigned char>(npc->spawned_04);
        }
    }
    return 0;
}

/* Mark the NPC of one kind, raising both of the two flags that go together. */
// FUNCTION: WIZ8 0x0050ca30
void MarkNpcOfKind(int kind)
{
    int index;
    W8NpcState* npc;

    for (index = 0; index < g_npc_states->GetCount(); ++index) {
        npc = *g_npc_states->GetAt(index);
        if (npc->record->kind == kind) {
            if (npc != 0) {
                npc->marked_e9 = 1;
                npc->marked_114 = 1;
            }
            return;
        }
    }
}

// FUNCTION: WIZ8 0x0050C870
bool CanNpcJoinParty(W8NpcState* npc)
{
    int band;
    int row;
    int index;
    unsigned int count;
    unsigned int total;
    unsigned int average;

    if (npc->record->has_group == 0) {
        return 0;
    }
    /* The service ids are the GetLevelBand region numbering: an NPC who offers
       the current region's service stays on duty and refuses to join. */
    band = GetLevelBand(g_status_685170.current_level);
    row = 0;
    while (g_npc_services[row].service_id != 0xffffffff) {
        if (g_npc_services[row].service_id == static_cast<unsigned int>(band)) {
            if ((npc->record->service_flags & g_npc_services[row].bit) != 0) {
                return 0;
            }
            break;
        }
        ++row;
    }
    if (npc->name_style == W8_NPC_GLUMPH && GetFact(W8_FACT_UMISSION_SCUBA_DONE) != 0) {
        return 0;
    }
    if (npc->name_style == W8_NPC_SEXUS && GetFact(W8_FACT_SEXUS_PAID) == 0) {
        return 0;
    }
    /* Retail performs the identical Madras check twice in a row - the second
       test is dead but genuinely present, kept faithful. */
    if (npc->name_style == W8_NPC_MADRAS && GetFact(W8_FACT_TRYNNIE_MADRAS_WILL_JOIN) == 0) {
        return 0;
    }
    if (npc->name_style == W8_NPC_MADRAS && GetFact(W8_FACT_TRYNNIE_MADRAS_WILL_JOIN) == 0) {
        return 0;
    }
    if ((npc->name_style == W8_NPC_DRAZIC || npc->name_style == W8_NPC_RODAN) &&
        GetFact(W8_FACT_PEACE_ACHIEVED) != 0) {
        return 0;
    }
    if (npc->record->min_party_level_6f > 0) {
        total = 0;
        count = 0;
        average = 0;
        for (index = 0; index < 8; ++index) {
            if (g_status_685170.buffers.XChar[index].fOccupied != 0 &&
                g_status_685170.buffers.Char[index].hp_current > 0 &&
                g_status_685170.buffers.Char[index].highest_condition < 0xf) {
                ++count;
                total += g_status_685170.buffers.Char[index].uiExpLevel;
            }
        }
        if (count > 0) {
            average = total / count;
        }
        if (average < npc->record->min_party_level_6f) {
            return 0;
        }
    }
    return 1;
}

/* Whether the NPC offers one service. The service id is looked up in a table
   that pairs it with its bit, so the ids need not be contiguous. */
// FUNCTION: WIZ8 0x0050c9e0
bool NpcOffersService(W8NpcState* npc, unsigned int service_id)
{
    int row = 0;

    if (g_npc_services[0].service_id == 0xffffffff) {
        return false;
    }
    while (g_npc_services[row].service_id != 0xffffffff) {
        if (g_npc_services[row].service_id == service_id) {
            return (npc->record->service_flags & g_npc_services[row].bit) != 0;
        }
        ++row;
    }
    return false;
}

/* Add a topic to the front of the NPC's five, pushing the oldest off the end
   when they are full. An empty slot is filled in place instead. */
// FUNCTION: WIZ8 0x0050c140
void AddNpcTopic(W8NpcState* npc, int topic)
{
    int slot;

    for (slot = 0; slot < W8_NPC_TOPIC_SLOTS; ++slot) {
        if (npc->topics[slot] == 0) {
            npc->topics[slot] = topic + 1;
            return;
        }
    }
    for (slot = W8_NPC_TOPIC_SLOTS - 1; slot > 0; --slot) {
        npc->topics[slot] = npc->topics[slot - 1];
    }
    npc->topics[0] = topic + 1;
}

/* What to call the NPC. One naming style takes a substituted name once the
   party has learned it, copied into a shared buffer so the caller always gets
   a writable string; everything else is named by its record. */
// FUNCTION: WIZ8 0x0050c770
const char* GetNpcDisplayName(W8NpcState* npc)
{
    if (npc->name_style == W8_NPC_RFS81_A && GetFact(W8_FACT_RFS81_HAS_BEEN_FIXED) != 0) {
        strcpy(g_npc_name_buffer, g_substituted_npc_name);
        return g_npc_name_buffer;
    }
    return npc->record->display_name;
}

/* The live NPC state whose display (or fact-substituted) name matches
   case-insensitively; released bindings and non-matches are skipped. */
// FUNCTION: WIZ8 0x0050ADA0
W8NpcState* FindNpcStateByName(const char* name)
{
    int index;
    const char* candidate;

    for (index = 0; index < g_npc_states->count; ++index) {
        W8NpcState* npc = *g_npc_states->GetAt(index);

        if (npc->binding_unavailable == 0) {
            if (npc->name_style == W8_NPC_RFS81_A && GetFact(W8_FACT_RFS81_HAS_BEEN_FIXED) != 0) {
                strcpy(g_npc_name_buffer, g_substituted_npc_name);
                candidate = g_npc_name_buffer;
            } else {
                candidate = npc->record->display_name;
            }
            if (_stricmp(candidate, name) == 0) {
                return npc;
            }
        }
    }
    return 0;
}

/* Age every bound NPC's timeout state once per game-time pass. The caller
   feeds elapsed minutes times ten: a dismissed NPC's flag clears once its
   accumulator passes 0x3c of those units, and each dialogue cooldown flag
   drops once its world-clock stamp is more than 0xa8c0 old. */
// FUNCTION: WIZ8 0x0050C7D0
void AdvanceNpcTimers0050C7D0(unsigned int elapsed)
{
    for (unsigned int index = 0; index < static_cast<unsigned int>(g_npc_states->count); ++index) {
        W8NpcState* npc = *g_npc_states->GetAt(index);
        if (npc->binding_unavailable == 0) {
            if (npc->dismissed_flag != 0) {
                npc->dismissed_timer += elapsed;
                if (npc->dismissed_timer > 0x3c) {
                    npc->dismissed_flag = 0;
                }
            }
            if (npc->talk_cooldown_active != 0 &&
                static_cast<unsigned int>(g_status_685170.world_clock - npc->talk_cooldown_clock) >
                    0xa8c0) {
                npc->talk_cooldown_active = 0;
            }
            if (npc->trade_cooldown_active != 0 &&
                static_cast<unsigned int>(g_status_685170.world_clock - npc->trade_cooldown_clock) >
                    0xa8c0) {
                npc->trade_cooldown_active = 0;
            }
        }
    }
}

/* Consume the two pending NPC event marks once combat and surprise are both
   quiet. The 0x2446 mark resolves the kind-0x42 NPC: when a bound, flagged
   record is present the mark just clears; otherwise the named party slot
   becomes infatuated and fact 0x2a6 is raised unless the level band is 9 or
   10. The 0x242e mark replays the bound NPCs of the first two party rows
   after its 0x3c delay: a row whose character carries a highest condition of
   0xf or worse keeps its NPC bound and the whole mark stays pending, while
   clear rows fire the name-style group events or mark the NPC's service. */
// FUNCTION: WIZ8 0x0050CA80
void ProcessNpcPendingEvents0050CA80(void)
{
    unsigned char all_clear = 1; // bool-byte-ok: retail byte flag

    if (gXStatus.fCombatMode == 0 && gXStatus.fSurprisePossible == 0) {
        if (g_status_685170.infatuation_pending_2446 != 0) {
            unsigned char flagged = 0;
            for (int index = 0; index < g_npc_states->count; ++index) {
                W8NpcState* candidate = *g_npc_states->GetAt(index);
                if (candidate->record->kind == 0x42) {
                    if (candidate != 0 && static_cast<unsigned char>(candidate->spawned_04) != 0) {
                        flagged = 1;
                    }
                    break;
                }
            }
            if (flagged != 0) {
                g_status_685170.infatuation_pending_2446 = 0;
            } else {
                char band = GetLevelBand(g_status_685170.current_level);
                if (band != 9 && band != 0xa) {
                    W8Character* character =
                        &g_status_685170.buffers.Char[g_status_685170.sedexus_party_slot_247f];
                    if (character->gender == W8_GENDER_MALE) {
                        QueueCharacterEvent(character, g_effect_005ee638, 0,
                                            g_effect_argument_005ed8c8, g_effect_argument_005ed914);
                    }
                    SetCharacterCondition(g_status_685170.sedexus_party_slot_247f,
                                          W8_CONDITION_INFATUATED, 9999, 0, 0, 1);
                    g_status_685170.infatuation_pending_2446 = 0;
                    SetFact(0x2a6, 1, 0);
                }
            }
        }
        if (g_status_685170.binding_reset_pending_242e != 0 &&
            (g_status_685170.world_clock - g_status_685170.binding_reset_clock_242a) > 0x3c) {
            int service = 0;
            if (g_npc_services[0].service_id != 0xffffffff) {
                while (g_npc_services[service].service_id !=
                       static_cast<unsigned int>(GetLevelBand(g_status_685170.current_level))) {
                    ++service;
                    if (g_npc_services[service].service_id == 0xffffffff) {
                        g_status_685170.binding_reset_pending_242e = 0;
                        return;
                    }
                }
                for (int slot = 0; slot < 2; ++slot) {
                    W8PartySlotRow* row = &g_status_685170.buffers.XChar[slot];
                    W8Character* character = &g_status_685170.buffers.Char[slot];
                    if (row->fOccupied == 0 || character->hp_current == 0) {
                        continue;
                    }
                    W8NpcState* npc = GetNpcState(row->npc_index);
                    if (character->highest_condition < 0xf) {
                        int event = 0;
                        if (GetLevelBand(g_status_685170.current_level) != 0xd) {
                            if (npc->name_style == 0x11 &&
                                (g_status_685170.buffers.XChar[0].fOccupied == 0 ||
                                 GetNpcState(g_status_685170.buffers.XChar[0].npc_index)
                                         ->name_style != 0x10 ||
                                 g_status_685170.buffers.Char[0].highest_condition >= 0xf) &&
                                (g_status_685170.buffers.XChar[1].fOccupied == 0 ||
                                 GetNpcState(g_status_685170.buffers.XChar[1].npc_index)
                                         ->name_style != 0x10 ||
                                 g_status_685170.buffers.Char[1].highest_condition >= 0xf)) {
                                event = 0x6c;
                            } else if (npc->name_style == 0x10 && !NpcLeadHasNameStyle(0x11)) {
                                event = 0x67;
                            }
                        }
                        if (event != 0) {
                            QueueCharacterEvent(character, event, 0, g_effect_argument_005ed8c8,
                                                g_effect_argument_005ed914);
                            BeginScriptedWorldAction();
                            QueueNpcMessageLine(W8_NPC_MSG_GROUP_ACTION, slot);
                            continue;
                        }
                        W8NpcState* bound = GetNpcState(row->npc_index);
                        if (bound != 0 && bound->service_flags[service] == 0) {
                            bound->service_flags[service] = 1;
                            if (NpcOffersService(
                                    bound, GetLevelBand(g_status_685170.current_level)) == 0 &&
                                service != 0xc && service != 0xd) {
                                QueueCharacterEvent(
                                    character,
                                    static_cast<unsigned char>(g_npc_services[service].npc_id), 0,
                                    g_effect_argument_005ed8c8, g_effect_argument_005ed914);
                            }
                        }
                        bound = GetNpcState(row->npc_index);
                        if (bound != 0) {
                            char band = GetLevelBand(g_status_685170.current_level);
                            for (int index = 0; g_npc_services[index].service_id != 0xffffffff;
                                 ++index) {
                                if (g_npc_services[index].service_id ==
                                    static_cast<unsigned int>(band)) {
                                    if ((bound->record->service_flags &
                                         g_npc_services[index].bit) != 0) {
                                        row->npc_bound_fe = 1;
                                        bound->event_clock_eb = g_status_685170.world_clock;
                                        RebuildConditionsAndDerivedStats(slot);
                                        QueueCharacterEvent(character, 0x56, 0,
                                                            g_effect_argument_005ed8c8,
                                                            g_effect_argument_005ed914);
                                    }
                                    break;
                                }
                            }
                        }
                        npc->incapacitated_e8 = 0;
                    } else if (character->highest_condition == 0x11 ||
                               character->highest_condition == 0x10 ||
                               character->highest_condition == 0xf) {
                        all_clear = 0;
                        npc->incapacitated_e8 = 1;
                    }
                }
                if (all_clear == 0) {
                    return;
                }
            }
            g_status_685170.binding_reset_pending_242e = 0;
        }
    }
}

/* Choose the new-game start level and entrance, then bind the intro NPCs that
   belong to that campaign path. Import 0x4c is Gigas, 0x4b is the bluff, and
   0x4e or neither is the monastery. */
// FUNCTION: WIZ8 0x005092f0
void ChooseNewGameStartLocation(int* level, int* entrance)
{
    unsigned char value;
    wchar_t display_value[10];
    int start_level;

    value = EvaluateFact(0x4e);
    if (g_status_685170.log_fact_checks_3120) {
        if (value) {
            wcscpy(display_value, L"TRUE");
        } else {
            wcscpy(display_value, L"FALSE");
        }
        ShowNoticef(5, L"Checking fact %S which is %s", g_fact_records[0x4e].symbolic_name,
                    display_value);
    }
    if (value != 0) {
        start_level = 8;
    } else {
        value = EvaluateFact(0x4c);
        if (g_status_685170.log_fact_checks_3120) {
            if (value) {
                wcscpy(display_value, L"TRUE");
            } else {
                wcscpy(display_value, L"FALSE");
            }
            ShowNoticef(5, L"Checking fact %S which is %s", g_fact_records[0x4c].symbolic_name,
                        display_value);
        }
        if (value != 0) {
            start_level = 0xe;
        } else {
            start_level = GetFact(W8_FACT_IMPORT_TRANG) != 0 ? 6 : 8;
        }
    }
    *level = start_level;
    *entrance = 0;
    g_status_685170.greeting_pending_2497 = 1;

    value = EvaluateFact(0x4e);
    if (g_status_685170.log_fact_checks_3120) {
        if (value) {
            wcscpy(display_value, L"TRUE");
        } else {
            wcscpy(display_value, L"FALSE");
        }
        ShowNoticef(5, L"Checking fact %S which is %s", g_fact_records[0x4e].symbolic_name,
                    display_value);
    }
    if (value != 0) {
        return;
    }

    value = EvaluateFact(0x4c);
    if (g_status_685170.log_fact_checks_3120) {
        if (value) {
            wcscpy(display_value, L"TRUE");
        } else {
            wcscpy(display_value, L"FALSE");
        }
        ShowNoticef(5, L"Checking fact %S which is %s", g_fact_records[0x4c].symbolic_name,
                    display_value);
    }
    if (value != 0) {
        RestoreNamedNpcAtLevel0050C1C0(0x18, 0xe, "NP_ViGigas");
        RestoreNamedNpcAtLevel0050C1C0(0xc, 0xe, "NP_BalbrakIntro");
        return;
    }

    value = EvaluateFact(0x4b);
    if (g_status_685170.log_fact_checks_3120) {
        if (value) {
            wcscpy(display_value, L"TRUE");
        } else {
            wcscpy(display_value, L"FALSE");
        }
        ShowNoticef(5, L"Checking fact %S which is %s", g_fact_records[0x4b].symbolic_name,
                    display_value);
    }
    if (value != 0) {
        RestoreNamedNpcAtLevel0050C1C0(0x18, 6, "NP_ViBluff");
        RestoreNamedNpcAtLevel0050C1C0(0x8c, 6, "NP_GuardBluff");
        return;
    }
    RestoreNamedNpcAtLevel0050C1C0(0x18, 8, "NP_ViMon");
}

/* Runs when the pending greeting_pending_2497 transition times out: new-game parties get
   the opening scripted effect, imported parties instead pull focus to the NPC
   their ending selected — ending 0x4c the kind-0x0c greeter, ending 0x4b the
   kind-0x8c greeter with the camera swung onto its head — and anything else
   falls back to the kind-0x18 greeter. */
// FUNCTION: WIZ8 0x00509560
void SelectStartNpcGreeting(void)
{
    W8Monster* monster;
    W8NpcState* npc;
    W8MonsterInfo* monster_info;
    unsigned char value;
    wchar_t display_value[10];
    srVector3T<float> head;

    value = EvaluateFact(0x4e);
    if (g_status_685170.log_fact_checks_3120) {
        if (value) {
            wcscpy(display_value, L"TRUE");
        } else {
            wcscpy(display_value, L"FALSE");
        }
        ShowNoticef(5, L"Checking fact %S which is %s", g_fact_records[0x4e].symbolic_name,
                    display_value);
    }
    if (value != 0) {
        ApplyItemEffectToRandomCharacter(g_fact_check_event_005ee6f0, -1, 0,
                                         g_effect_argument_005ed8c8);
        return;
    }

    value = EvaluateFact(0x4c);
    if (g_status_685170.log_fact_checks_3120) {
        if (value) {
            wcscpy(display_value, L"TRUE");
        } else {
            wcscpy(display_value, L"FALSE");
        }
        ShowNoticef(5, L"Checking fact %S which is %s", g_fact_records[0x4c].symbolic_name,
                    display_value);
    }
    if (value != 0) {
        npc = GetNpcStateByKind(0xc);
    } else {
        value = EvaluateFact(0x4b);
        if (g_status_685170.log_fact_checks_3120) {
            if (value) {
                wcscpy(display_value, L"TRUE");
            } else {
                wcscpy(display_value, L"FALSE");
            }
            ShowNoticef(5, L"Checking fact %S which is %s", g_fact_records[0x4b].symbolic_name,
                        display_value);
        }
        if (value != 0) {
            npc = GetNpcStateByKind(0x8c);
            if (npc == 0) {
                return;
            }
            QueueNpcScriptNotice(npc, 0, 0, 0, 0);
            monster_info = GetNpcMonsterInfo(npc);
            if (monster_info == 0) {
                return;
            }
            monster = monster_info->p3D;
            head.x = monster->movement_0c0.position_040.x;
            head.y = monster->movement_0c0.position_040.y + monster->movement_0c0.height_offset_0b8;
            head.z = monster->movement_0c0.position_040.z;
            g_gd_camera_65a0f8->LookAt(&head, 0);
            return;
        }
        npc = GetNpcStateByKind(0x18);
    }
    if (npc != 0) {
        QueueNpcScriptNotice(npc, 0, -1, 0, 0);
    }
}

/* New-game start level from the campaign facts InitializeFactState planted.
   Import path 0x4c is level 14, 0x4b is level 6, and 0x4e or neither is 8. */
// FUNCTION: WIZ8 0x00509750
int SelectNewGameStartLevel(void)
{
    unsigned char value;
    wchar_t display_value[10];

    value = EvaluateFact(0x4e);
    if (g_status_685170.log_fact_checks_3120) {
        if (value) {
            wcscpy(display_value, L"TRUE");
        } else {
            wcscpy(display_value, L"FALSE");
        }
        ShowNoticef(5, L"Checking fact %S which is %s", g_fact_records[0x4e].symbolic_name,
                    display_value);
    }
    if (value != 0) {
        return 8;
    }

    value = EvaluateFact(0x4c);
    if (g_status_685170.log_fact_checks_3120) {
        if (value) {
            wcscpy(display_value, L"TRUE");
        } else {
            wcscpy(display_value, L"FALSE");
        }
        ShowNoticef(5, L"Checking fact %S which is %s", g_fact_records[0x4c].symbolic_name,
                    display_value);
    }
    if (value != 0) {
        return 0xe;
    }

    value = EvaluateFact(0x4b);
    if (g_status_685170.log_fact_checks_3120) {
        if (value) {
            wcscpy(display_value, L"TRUE");
        } else {
            wcscpy(display_value, L"FALSE");
        }
        ShowNoticef(5, L"Checking fact %S which is %s", g_fact_records[0x4b].symbolic_name,
                    display_value);
    }
    if (value != 0) {
        return 6;
    }
    return 8;
}

/* Create the shared NPC-state vector the first time anything needs it. */
// FUNCTION: WIZ8 0x00509890
void InitializeNpcStates(void)
{
    if (g_npc_states == 0) {
        g_npc_states = new W8GrowableVector<W8NpcState*>();
    }
}

/* Empty the shared vector: release every state's monster binding and character,
   hand its stock to the item-list teardown, and delete the state itself. Then
   rebuild a runtime node for every database record that is not flagged at
   0x054, which is the state a fresh game starts from. */
// FUNCTION: WIZ8 0x00509920
void ResetNpcStates(void)
{
    int index;
    unsigned int npc_id;

    if (g_npc_states != 0) {
        for (index = 0; index < g_npc_states->count; ++index) {
            W8NpcState* npc = *g_npc_states->GetAt(index);

            ReleaseNpcScriptFile0055A0A0(npc->script_file);
            npc->script_file = 0;
            if (npc->record != 0 && npc->record->owns_stock_055 != 0) {
                ClearNpcItems(npc);
            }
            delete npc->character;
            delete npc;
        }
        g_npc_states->count = 0;
    }
    for (npc_id = 0; npc_id < gXStatus.uiNpcsInDatabase; ++npc_id) {
        if (g_npc_records[npc_id].monster_bound_054 == 0) {
            CreateNpcRuntimeNode(npc_id);
        }
    }
}

/* ShutdownGameData teardown: release every NPC state the same way
   ResetNpcStates does, then destroy the state vector itself. */
// FUNCTION: WIZ8 0x005099D0
void ReleaseNpcStates005099D0(void)
{
    int index;

    if (g_npc_states != 0) {
        for (index = 0; index < g_npc_states->count; ++index) {
            W8NpcState* npc = *g_npc_states->GetAt(index);
            if (g_npc_states != 0) {
                ReleaseNpcScriptFile0055A0A0(npc->script_file);
                npc->script_file = 0;
                if (npc->record != 0 && npc->record->owns_stock_055 != 0) {
                    ClearNpcItems(npc);
                }
                delete npc->character;
                delete npc;
            }
        }
        g_npc_states->count = 0;
    }
    delete g_npc_states;
    g_npc_states = 0;
}

/* The g_npc_states teardown's own vector emission. */
// TEMPLATE: WIZ8 0x00509A80
// W8GrowableVector<W8NpcState*>::~W8GrowableVector<W8NpcState*>

/* The NPCT section writer: a version byte, the state count, then each 0x13d
   state block followed by its 0x1862 character block when the NPC carries one.
   The per-state stock lists trail through the section's file handle. */
// FUNCTION: WIZ8 0x00509F00
unsigned char SaveNpcStates00509F00(W8Chunk* chunks)
{
    unsigned char version = 3;
    W8NpcState* npc;
    unsigned int count;
    unsigned int index;
    int size;

    chunks->Write(&version, 1, 0);
    count = g_npc_states->count;
    chunks->Write(&count, 4, 0);
    for (index = 0; index < count; ++index) {
        npc = *g_npc_states->GetAt(index);
        chunks->Write(npc, sizeof(*npc), 0);
        if (npc->character != 0) {
            size = sizeof(*npc->character);
            chunks->Write(&size, 4, 0);
            chunks->Write(npc->character, size, 0);
        }
    }
    return SaveNpcItemLists0050AA10(chunks->m_hFile);
}

/* The NPCT section reader: release the live states, then rebuild them from the
   saved blocks. Character blocks carry their size from version 3 on and older
   saves read as the legacy 0x185c length. Grouped NPCs re-bind to the loaded
   level; afterwards the missing runtime nodes for unflagged records are
   created. */
/* The stock-list tail of the NPCT section: for every NPC state the entry
   count, then each 0x14-byte stock entry in list order. */
// FUNCTION: WIZ8 0x0050AA10
unsigned char SaveNpcItemLists0050AA10(int file)
{
    unsigned int written = 0;
    unsigned int item_count = 0;
    unsigned int index;
    unsigned int npc_index;
    unsigned int count;
    W8NpcState* npc;
    W8NpcItemEntry* entry;

    count = g_npc_states->count;
    for (npc_index = 0; npc_index < count; ++npc_index) {
        npc = *g_npc_states->GetAt(npc_index);
        if (npc->items != 0) {
            item_count = PLLength(npc->items);
        } else {
            item_count = 0;
        }
        if (FileWrite(file, &item_count, 4, &written) == 0 || written != 4) {
            return 0;
        }
        for (index = 0; index < item_count; ++index) {
            entry = static_cast<W8NpcItemEntry*>(PLGet(npc->items, index));
            if (FileWrite(file, entry, sizeof(*entry), &written) == 0 ||
                written != sizeof(*entry)) {
                return 0;
            }
        }
    }
    return 1;
}

/* The matching read side of the stock-list tail: each saved count clears the
   state's list, then that many 0x14-byte entries are appended to a fresh
   list. */
/* Build one runtime state from its database record: clear the 0x13d-byte
   block, bind the record, build the group-member character and stock, copy the
   item table, and insert the node into the shared vector. A node whose binding
   was released keeps its slot for the newcomer, which is placed at the same
   index and then removed; otherwise the node is appended. Either way the node
   records its own slot at 0x2c, and a failed append leaves the not-found
   value. */
// FUNCTION: WIZ8 0x00509aa0
W8NpcState* CreateNpcRuntimeNode(int npc_id)
{
    W8NpcState* npc;
    W8NpcState* released;
    int index;

    npc = new W8NpcState;
    memset(npc, 0, sizeof(*npc));
    npc->name_style = npc_id;
    npc->record = &g_npc_records[npc_id];
    if (npc->record->has_group != 0) {
        npc->character = new W8Character;
        InitializeNpcCharacter(npc, npc->character);
    }
    if (npc->record->owns_stock_055 != 0) {
        PopulateNpcStock(npc);
    }
    InitializeNpcItemTable(npc);
    npc->location_id = 0;
    npc->is_present = 0;
    npc->disposition = g_npc_records[npc_id].disposition;
    npc->gold_80 = g_npc_records[npc_id].gold;
    npc->greeting_pending = 1;
    npc->trade_pool_ca = g_npc_records[npc_id].trade_pool_002;

    for (index = 0; index < g_npc_states->count; ++index) {
        released = *g_npc_states->GetAt(index);
        if (released != 0 && released->binding_unavailable != 0) {
            g_npc_states->InsertAt(index, npc);
            g_npc_states->Remove(released);
            npc->partner_index_2c = (unsigned char)index;
            return npc;
        }
    }
    if (g_npc_states->Add(npc) == -1) {
        npc->partner_index_2c = 0xff;
        return npc;
    }
    npc->partner_index_2c = (unsigned char)(g_npc_states->count - 1);
    return npc;
}

/* Bind an NPC runtime state to a monster location, creating the state when the
   database entry allows it. The dialogue path instead finds the existing state
   by its database kind and only refreshes its presence fields. */
// FUNCTION: WIZ8 0x00509cd0
void BindNpcToMonster(unsigned char npc_id, int has_monster, int location_id)
{
    W8NpcState* npc = 0;
    W8MonsterInfo* monster_info = 0;
    int index;

    if (g_npc_states == 0) {
        InitializeNpcStates();
    }
    if (has_monster != 0) {
        unsigned int monster_list_index =
            MonsterGetIndexByLocationID(0x150, NPC_MANAGER_CPP, location_id, 1);
        monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
        W8MonsterRecord* monster_record = GetMonsterDataForInfo(monster_info);
        if (monster_record == 0 || (monster_record->flags_0d0 & 1) == 0) {
            monster_info->bound_npc_index = -1;
            return;
        }

        if (g_npc_records[npc_id].monster_bound_054 == 0) {
            for (index = 0; index < g_npc_states->count; ++index) {
                W8NpcState* candidate = *g_npc_states->GetAt(index);
                if (candidate->record->kind == npc_id) {
                    npc = candidate;
                    break;
                }
            }
        } else {
            if (monster_info->bound_npc_index > 0) {
                for (index = 0; index < g_npc_states->count; ++index) {
                    W8NpcState* candidate = *g_npc_states->GetAt(index);
                    if (candidate->binding_unavailable == 0 &&
                        monster_info->bound_npc_index == index &&
                        candidate->location_id == location_id) {
                        if (candidate->name_style == npc_id) {
                            npc = candidate;
                        }
                        break;
                    }
                }
            }
            if (npc == 0) {
                npc = CreateNpcRuntimeNode(npc_id);
            }
        }
        monster_info->bound_npc_index = npc->partner_index_2c;
    } else {
        for (index = 0; index < g_npc_states->count; ++index) {
            W8NpcState* candidate = *g_npc_states->GetAt(index);
            if (candidate->record->kind == npc_id) {
                npc = candidate;
                break;
            }
        }
    }

    if (npc == 0) {
        srAssertFail("pNode", NPC_MANAGER_CPP, 0x194, 0);
    }
    npc->location_id = location_id;
    npc->is_present = has_monster;
    npc->has_monster = true;
    npc->level_band = GetLevelBand(g_status_685170.current_level);
    npc->bound_level = static_cast<unsigned char>(g_status_685170.current_level);
    ReloadNpcScriptResources(npc);
}

/* Expand the record's character block into a fresh group-member character:
   the name, profession and starting level, attributes, skills, known spells
   and worn/carried items, then the derived passes a level advance settles. */
// FUNCTION: WIZ8 0x0050aed0
unsigned char InitializeNpcCharacter(W8NpcState* npc, W8Character* character)
{
    W8NpcDatabaseRecord* record = npc->record;
    W8NpcCharacterTemplate* source;
    W8ItemInstance item;
    int index;

    if (record->has_group == 0) {
        return 0;
    }
    source = &record->character;
    memset(character, 0, sizeof(*character));
    character->level_band_base = 0;
    character->attribute_point_deficit_0199 = 0;
    character->highest_condition = 0;
    character->enchantment_top = 0;
    character->unknown_007d = -1;
    character->personality_0081 = -1;
    for (index = 0; index < 12; ++index) {
        EmptyItemRecord(&character->EquippedItem[index], 0, 1);
    }
    for (index = 0; index < 8; ++index) {
        EmptyItemRecord(&character->backpack[index], 0, 1);
    }
    wcscpy(character->name, source->name);
    wcscpy(character->name_part_2, source->name_part_2);
    character->iProfession = source->profession;
    character->original_profession = source->profession;
    character->profession_levels[source->profession] = source->level;
    character->iRace = source->race;
    character->gender = static_cast<W8Gender>(source->gender);
    character->portrait_index = source->table_value;
    for (index = 0; index < 7; ++index) {
        character->attributes[index].value = source->attributes[index];
    }
    for (index = 0; index < 0x29; ++index) {
        character->skills[index].points_02 = source->skills[index];
    }
    for (index = 0; index < 12; ++index) {
        if (source->equipment_present[index] != 0 && source->equipment_ids[index] != 0xffff) {
            ReplaceOrCreateItem(&item, (short)source->equipment_ids[index], 1, 1, 0);
            character->EquippedItem[index] = item;
        }
    }
    for (index = 0; index < 8; ++index) {
        if (source->backpack_present[index] != 0 && source->backpack_ids[index] != 0xffff) {
            ReplaceOrCreateItem(&item, (short)source->backpack_ids[index], 1, 1, 0);
            character->backpack[index] = item;
        }
    }
    AdvanceCharacterToLevel(character, source->level);
    AccumulateEquipmentModifiers(character, &character->equipment_bonus_1709);
    RebuildCharacterModifierBlock(character);
    CalcCharacterLevelBand(character);
    RefreshCharacterSkillAvailability00553CD0(character);
    RecalculateCharacterDerivedStats(character);
    for (index = 1; index < 0x73; ++index) {
        if (source->spells[index - 1] != 0 && CanCharacterLearnSpell(character, index)) {
            LearnSpell(character, index, 0);
        } else {
            character->spell_learned[index] = 0;
        }
    }
    character->hp_current = character->uiHPMax;
    character->stamina = character->uiStaminaMax;
    for (index = 0; index < W8_SPELL_REALM_COUNT; ++index) {
        character->iSPLeft[index] = character->sp_max[index];
    }
    return 1;
}

/* Copy the record's one-based item table into the state's runtime arrays: the
   forty entry item ids and weights, and the table's item-count dice. A record
   whose table id is zero or past the table database keeps the -1 ids and
   leaves the dice untouched. */
// FUNCTION: WIZ8 0x0050b9e0
void InitializeNpcItemTable(W8NpcState* npc)
{
    unsigned int index;

    memset(npc->item_ids_30, 0xff, sizeof(npc->item_ids_30));
    if (npc->record->item_table_id >= (int)gXStatus.uiItemTablesInDatabase) {
        return;
    }
    if (npc->record->item_table_id == 0) {
        return;
    }
    for (index = 0; index < 40; ++index) {
        if (g_item_tables[npc->record->item_table_id - 1]->entries[index].selector_00 != 0) {
            npc->item_ids_30[index] =
                g_item_tables[npc->record->item_table_id - 1]->entries[index].item_id;
            npc->item_weights_115[index] =
                g_item_tables[npc->record->item_table_id - 1]->entries[index].weight;
        }
    }
    npc->item_count_dice_10e = g_item_tables[npc->record->item_table_id - 1]->item_count_dice;
}

/* Release the NPC binding held at the given index: clear its monster link and
   handle, then hand the handle to the owned item-list teardown. An index past
   the end reads slot zero instead of stopping. */
// FUNCTION: WIZ8 0x00509EA0
void ReleaseNpcBinding(int value)
{
    W8NpcState* npc;
    W8NpcDatabaseRecord* record;
    unsigned char flag;
    W8NpcScriptFile* file;

    if (value == -1) {
        return;
    }
    if (value < 0) {
        return;
    }
    if (value > g_npc_states->count) {
        return;
    }
    if (value < g_npc_states->count) {
        npc = g_npc_states->data[value];
    } else {
        npc = g_npc_states->data[0];
    }
    file = npc->script_file;
    npc->has_monster = 0;
    record = npc->record;
    npc->script_file = 0;
    flag = record->monster_bound_054;
    ReleaseNpcScriptFile0055A0A0(file);
    if (flag != 0) {
        npc->binding_unavailable = 1;
    }
}

/* Serialize every NPC state record into the open NPCT chunk: a version byte,
   the live count, then each 0x13d-byte state. A state whose embedded group
   character exists is followed by a sizeof(W8Character) marker and the
   character record itself. The item lists are a separate FileWrite pass. */

/* Read every NPC state record back from the open NPCT chunk: a version byte,
   the serialized count, then each 0x13d-byte state. A state whose serialized
   group-character pointer is non-null carries a size-prefixed character
   record behind it (format 3 and later; older saves store a raw 0x185c-byte
   block). After the pass every state rebinds to its database record through
   name_style, grouped states reload their script resources, and format two
   and later append the item lists through the second pass. The walk ends by
   backfilling runtime nodes for database records no loaded state claims. */
// FUNCTION: WIZ8 0x00509FC0
void LoadNpcStates00509FC0(W8Chunk* chunks)
{
    unsigned char version = 3;
    int index;
    unsigned int count;
    unsigned int loaded;
    unsigned int npc_id;
    unsigned int size;
    W8NpcState* npc;

    InitializeNpcStates();
    if (g_npc_states != 0) {
        for (index = 0; index < g_npc_states->count; ++index) {
            npc = *g_npc_states->GetAt(index);

            ReleaseNpcScriptFile0055A0A0(npc->script_file);
            npc->script_file = 0;
            if (npc->record != 0 && npc->record->owns_stock_055 != 0) {
                ClearNpcItems(npc);
            }
            delete npc->character;
            delete npc;
        }
        g_npc_states->count = 0;
    }
    chunks->Read(&version, 1, 0);
    chunks->Read(&count, 4, 0);
    for (loaded = 0; loaded < count; ++loaded) {
        npc = new W8NpcState;
        chunks->Read(npc, sizeof(*npc), 0);
        npc->items = 0;
        if (npc->character != 0) {
            npc->character = new W8Character;
            if (version < 3) {
                memset(npc->character, 0, sizeof(*npc->character));
                chunks->Read(npc->character, 0x185c, 0);
            } else {
                memset(npc->character, 0, sizeof(*npc->character));
                chunks->Read(&size, 4, 0);
                if (size > sizeof(*npc->character)) {
                    srAssertFail("uiSize <= sizeof(*pNode->pPCData)", NPC_MANAGER_CPP, 0x217, 0);
                }
                chunks->Read(npc->character, size, 0);
            }
        }
        if (npc->name_style == 0) {
            npc->name_style = loaded;
        }
        npc->partner_index_2c = g_npc_states->Add(npc);
    }
    for (loaded = 0; loaded < count; ++loaded) {
        npc = *g_npc_states->GetAt(loaded);

        npc->script_file = 0;
        npc->has_monster = 0;
        npc->record = &g_npc_records[npc->name_style];
        if (npc->is_grouped != 0) {
            npc->is_present = 0;
            npc->has_monster = 1;
            npc->level_band = GetLevelBand(g_status_685170.current_level);
            npc->bound_level = g_status_685170.current_level;
            ReloadNpcScriptResources(npc);
        }
        if (npc->spawned_04 == 0xffff) {
            npc->spawned_04 = 0;
        }
    }
    if (version > 1) {
        LoadNpcItemLists0050AAF0(chunks->m_hFile);
    }
    for (npc_id = 0; npc_id < gXStatus.uiNpcsInDatabase; ++npc_id) {
        if (g_npc_records[npc_id].monster_bound_054 == 0 && GetNpcStateByKind(npc_id) == 0) {
            CreateNpcRuntimeNode(npc_id);
        }
    }
}

/* Append every NPC's stock item list behind the NPCT state records: the
   entry count followed by each 0x14-byte entry. A short FileWrite fails the
   whole pass. */

/* Read every NPC's stock item list back from the open NPCT chunk tail: the
   entry count followed by each 0x14-byte entry appended to a fresh plist. A
   short FileRead or a failed allocation fails the whole pass. */
// FUNCTION: WIZ8 0x0050AAF0
unsigned char LoadNpcItemLists0050AAF0(unsigned int file)
{
    unsigned int transferred = 0;
    unsigned int item_count = 0;
    unsigned int index;
    unsigned int npc_index;
    unsigned int count;
    W8NpcState* npc;
    W8NpcItemEntry* entry;

    count = g_npc_states->count;
    for (npc_index = 0; npc_index < count; ++npc_index) {
        npc = *g_npc_states->GetAt(npc_index);
        if (FileRead(file, &item_count, 4, &transferred) == 0 || transferred != 4) {
            return 0;
        }
        if (npc->items != 0) {
            ClearNpcItems(npc);
        }
        if (item_count == 0) {
            npc->items = 0;
        } else {
            npc->items = PLCreate();
            for (index = 0; index < item_count; ++index) {
                entry = new W8NpcItemEntry;
                if (entry == 0) {
                    return 0;
                }
                if (FileRead(file, entry, sizeof(*entry), &transferred) == 0 ||
                    transferred != sizeof(*entry)) {
                    return 0;
                }
                PLAdoptAppend(npc->items, entry);
            }
        }
    }
    return 1;
}

/* Hand back the NPC binding selected by a monster-list index, or null when
   the monster's record is missing, is not NPC-routed, binds no NPC, or the
   binding has been released. */
// FUNCTION: WIZ8 0x0050A440
W8NpcState* FindNpcBindingForMonster(unsigned int monster_list_index)
{
    W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
    W8MonsterRecord* record = GetMonsterDataForInfo(monster_info);
    W8NpcState* npc;

    if (record == 0 || (record->flags_0d0 & 1) == 0 || record->npc_kind_0cd == 0xfa) {
        return 0;
    }
    npc = *g_npc_states->GetAt(monster_info->bound_npc_index);
    if (npc->binding_unavailable == 0) {
        return npc;
    }
    return 0;
}

/* The NPC bound to a monster's script part while its binding is still
   available; `allow_unavailable` also hands back a released binding. */
// FUNCTION: WIZ8 0x0050A4A0
W8NpcState* GetNpcStateForMonsterInfo(W8MonsterInfo* monster_info, unsigned char allow_unavailable)
{
    W8MonsterRecord* record = GetMonsterDataForInfo(monster_info);
    W8NpcState* npc;

    if (record == 0 || (record->flags_0d0 & 1) == 0 || record->npc_kind_0cd == 0xfa) {
        return 0;
    }
    npc = *g_npc_states->GetAt(monster_info->bound_npc_index);
    if (npc->binding_unavailable == 0 || allow_unavailable != 0) {
        return npc;
    }
    return 0;
}

/* One dialogue interaction against an NPC. The action kind selects the path:
   talking and the level-scaled charm shift disposition through the record's
   signed scale bytes, paying gold and selling an item draw the record's trade
   pool down by the party's Communication-adjusted price, and the scripted
   kind adds its operand straight to disposition. Kinds 0 and 1 reuse an
   argument's stack slot as the best-skill out-parameter; kind 2 and 3 fall
   into the shared disposition refresh at the tail. */
// FUNCTION: WIZ8 0x0050A570
void ApplyNpcInteraction0050A570(W8NpcState* npc, int kind, int value, W8ItemInstance* item,
                                 unsigned int gold)
{
    switch (static_cast<char>(kind)) {
    case 0: {
        int scale = npc->record->talk_scale_5e;
        int quotient;
        int level;
        int delta;
        int sum;

        npc->talk_cooldown_active = 1;
        npc->talk_cooldown_clock = g_status_685170.world_clock;
        if (scale < 1) {
            level = static_cast<int>(GetBestPartySkillLevel(0x16, &kind));
            quotient = -scale / 5;
        } else {
            level = static_cast<int>(GetBestPartySkillLevel(0x16, &kind));
            quotient = scale / 5;
        }
        delta = scale + level * quotient / 100;
        sum = npc->disposition + static_cast<char>(delta);
        if (sum > 99) {
            npc->disposition = 99;
        } else if (sum < 0) {
            npc->disposition = 0;
        } else {
            npc->disposition += static_cast<char>(delta);
        }
        PracticeCharacterSkill(&g_status_685170.buffers.Char[kind], 0x16, 8, 0);
        GetNpcDisposition(npc);
        return;
    }
    case 1: {
        W8MonsterInfo* monster_info;
        W8MonsterRecord* monster_record;
        unsigned int monster_level;
        unsigned int total_level = 0;
        unsigned int count = 0;
        unsigned int average_level;
        int scale;
        int quotient;
        int level;
        int delta;
        int sum;
        int index;

        npc->trade_cooldown_active = 1;
        npc->trade_cooldown_clock = g_status_685170.world_clock;
        if (npc->has_monster == 0 || npc->is_present == 0) {
            monster_info = 0;
        } else {
            monster_info = MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(0x2a1, NPC_MANAGER_CPP, npc->location_id, 1));
        }
        monster_record = GetMonsterDataForInfo(monster_info);
        monster_level = monster_record->effective_level_24f;
        if (monster_level < 1) {
            monster_level = 1;
        }
        for (index = 0; index < 8; ++index) {
            W8Character* character = &g_status_685170.buffers.Char[index];
            if (g_status_685170.buffers.XChar[index].fOccupied != 0 && character->hp_current != 0 &&
                character->highest_condition < 0xf) {
                ++count;
                total_level += character->uiExpLevel;
            }
        }
        if (count == 0) {
            average_level = 1;
        } else {
            average_level = total_level / count;
        }
        scale = npc->record->charm_scale_5d;
        if (scale < 1) {
            level = static_cast<int>(GetBestPartySkillLevel(0x16, &value));
            quotient = -scale / 5;
        } else {
            level = static_cast<int>(GetBestPartySkillLevel(0x16, &value));
            quotient = scale / 5;
        }
        delta = scale + level * quotient / 100;
        if (delta > 0) {
            delta = static_cast<int>(average_level * delta / monster_level);
        }
        sum = npc->disposition + static_cast<char>(delta);
        if (sum > 99) {
            npc->disposition = 99;
        } else if (sum < 0) {
            npc->disposition = 0;
        } else {
            npc->disposition += static_cast<char>(delta);
        }
        PracticeCharacterSkill(&g_status_685170.buffers.Char[value], 0x16, 5, 0);
        GetNpcDisposition(npc);
        return;
    }
    case 2: {
        int level;
        unsigned int adjusted;

        SpendPartyGold(gold);
        level = static_cast<int>(GetBestPartySkillLevel(0x16, 0));
        adjusted = gold + level * (static_cast<int>(gold & 0xffff) / 5) / 100;
        if (static_cast<int>(npc->trade_pool_ca - (adjusted & 0xffff)) < 0) {
            npc->trade_pool_ca = 0;
        } else {
            npc->trade_pool_ca = static_cast<unsigned short>(npc->trade_pool_ca - adjusted);
        }
        if (npc->trade_pool_ca != 0) {
            break;
        }
        GetNpcDisposition(npc);
        level = GetNpcDisposition(npc);
        if (level < W8_NPC_DISPOSITION_HOSTILE) {
            level = 2;
        } else {
            level = level < W8_NPC_DISPOSITION_FRIENDLY;
        }
        if (level != 0) {
            npc->disposition = 0x4b;
        }
        npc->trade_pool_ca = g_npc_records[npc->name_style].trade_pool_002;
        break;
    }
    case 3: {
        unsigned int price = GetItemStackValue(item);
        int level;
        unsigned int adjusted;

        level = static_cast<int>(GetBestPartySkillLevel(0x16, 0));
        adjusted = price + level * (static_cast<int>(price & 0xffff) / 5) / 100;
        if (static_cast<int>(npc->trade_pool_ca - (adjusted & 0xffff)) < 0) {
            npc->trade_pool_ca = 0;
        } else {
            npc->trade_pool_ca = static_cast<unsigned short>(npc->trade_pool_ca - adjusted);
        }
        if (npc->trade_pool_ca != 0) {
            break;
        }
        GetNpcDisposition(npc);
        level = GetNpcDisposition(npc);
        if (level < W8_NPC_DISPOSITION_HOSTILE) {
            level = 2;
        } else {
            level = level < W8_NPC_DISPOSITION_FRIENDLY;
        }
        if (level != 0) {
            npc->disposition = 0x4b;
        }
        npc->trade_pool_ca = g_npc_records[npc->name_style].trade_pool_002;
        break;
    }
    case 4: {
        int sum = npc->disposition + static_cast<char>(gold);
        if (sum > 99) {
            npc->disposition = 99;
        } else if (sum < 0) {
            npc->disposition = 0;
        } else {
            npc->disposition += static_cast<char>(gold);
        }
        GetNpcDisposition(npc);
        return;
    }
    default:
        break;
    }
    GetNpcDisposition(npc);
}

/* The 0..127 theft score the pickpocket resolutions compare against a d100
   roll: the character's stealth skill (rogues add a dexterity share), a
   quarter of the NPC's disposition swing, a random penalty scaled by the
   NPC's suspicion byte, the level gap to the NPC's monster, and - when an
   item is offered - its weight and price penalties, all scaled by the
   character's difficulty. */
// FUNCTION: WIZ8 0x0050BAF0
char ScoreNpcTheft0050BAF0(W8Character* character, W8NpcState* npc, int item_id, int count)
{
    W8ItemInstance item;
    int score;

    unsigned int skill = character->skills[0xd].level;
    if (character->iProfession == W8_PROFESSION_ROGUE) {
        skill += character->attributes[4].effective / 10;
    } else if (static_cast<int>(skill) >= 1) {
        skill += character->attributes[4].effective / 0x14;
    }
    score = skill + (GetNpcDisposition(npc) - 0x32) / 4;
    score -= (Random(5) + 10) * static_cast<signed char>(npc->suspicion_84);
    W8MonsterInfo* monster_info = GetNpcMonsterInfo(npc);
    W8MonsterRecord* record = GetMonsterDataForInfo(monster_info);
    unsigned int penalty = character->skills[0xd].level >> 2;
    score += (character->uiExpLevel - record->effective_level_24f) * 5;
    if (item_id != -1) {
        unsigned int weight = g_item_records[item_id].weight * count;
        ReplaceOrCreateItem(&item, item_id, 0, 0, 0);
        item.stack_count = static_cast<unsigned char>(count);
        penalty = penalty - weight / 10 - GetItemStackValue(&item) / 100;
    }
    if (static_cast<int>(penalty) > 0) {
        penalty = 0;
    }
    score += penalty;
    unsigned int slot = CharacterPointerToPartySlot(character);
    ScaleValueForCharacterDifficulty(slot, &score);
    unsigned int result = score < 0 ? 0 : score;
    if (static_cast<int>(result) > 0x7e) {
        result = 0x7f;
    }
    return result;
}

/* Resolve one pickpocket attempt against the NPC's stock: the PRNG is reseeded
   from the living party's experience and spun by the NPC's naming style and
   suspicion, then either the purse or a random eligible item is scored.
   Results: 0 item taken, 1 gold taken, 2 refused, 3 caught, 4 nothing left. */
// FUNCTION: WIZ8 0x0050BC90
int AttemptNpcPickpocket0050BC90(W8Character* character, W8NpcState* npc, W8ItemInstance* item_out,
                                 unsigned int* gold_out)
{
    bool empty_pick = false;
    W8GrowableVector<int> candidates;
    unsigned int picked = 0xffffffff;
    int index;

    int spins = CharacterPointerToPartySlot(character) + npc->name_style * 0xb +
                static_cast<signed char>(npc->suspicion_84) * 7;
    unsigned int seed = 0;
    for (index = 2; index < 8; ++index) {
        if (g_status_685170.buffers.XChar[index].fOccupied != 0) {
            seed += g_status_685170.buffers.Char[index].experience;
        }
    }
    srand(seed);
    for (; spins != 0; --spins) {
        Random(100);
    }
    for (index = 0; index < 40; ++index) {
        short item_id = npc->item_ids_30[index];
        if (item_id != -1 && item_id != 0x242 && item_id != 0x244 && item_id != 0x243) {
            candidates.Add(index);
        }
    }
    char score;
    if (candidates.count == 0 || Random(100) < 0x21) {
        empty_pick = true;
        score = ScoreNpcTheft0050BAF0(character, npc, -1, 0);
    } else {
        W8ItemInstance item;
        picked = *candidates.GetAt(Random(candidates.count));
        ReplaceOrCreateItem(&item, npc->item_ids_30[picked], 1, 1, 0);
        score = ScoreNpcTheft0050BAF0(character, npc, item.iItemNo, 1);
    }
    if (static_cast<signed char>(npc->suspicion_84) < 'd') {
        ++npc->suspicion_84;
    }
    char roll = static_cast<char>(Random(100));
    if (roll > '_' || score * 2 < roll) {
        return 3;
    }
    if (score <= roll) {
        return 2;
    }
    if (empty_pick) {
        if (npc->gold_80 != 0) {
            PracticeCharacterSkill(character, 0xd, 5, 0);
            unsigned int taken = Random(100) * 7;
            if (static_cast<unsigned int>(npc->gold_80) < taken) {
                taken = npc->gold_80;
            }
            npc->gold_80 -= taken;
            if (npc->gold_80 < 0) {
                npc->gold_80 = 0;
            }
            *gold_out = taken;
            return 1;
        }
        if (candidates.count == 0) {
            return 4;
        }
        return 2;
    }
    if (npc == 0) {
        srAssertFail("pNPC", NPC_MANAGER_CPP, 0x7a4, 0);
    }
    short item_id = npc->item_ids_30[picked & 0xff];
    if (item_id != -1) {
        if (item_out != 0) {
            ReplaceOrCreateItem(item_out, item_id, 1, 1, 0);
        }
        npc->item_ids_30[picked & 0xff] = -1;
        PracticeCharacterSkill(character, 0xd, 5, 0);
    }
    return 0;
}

/* The trade-screen steal of one offered item, scored by the same pickpocket
   roll: 0 takes it (and practices the skill), 1 is refused, 2 is caught. */
// FUNCTION: WIZ8 0x0050C040
char AttemptNpcItemTheft0050C040(W8Character* character, W8NpcState* npc, int item_id, int count)
{
    int index;

    int spins = CharacterPointerToPartySlot(character) + npc->name_style * 0xb +
                static_cast<signed char>(npc->suspicion_84) * 7;
    unsigned int seed = 0;
    for (index = 2; index < 8; ++index) {
        if (g_status_685170.buffers.XChar[index].fOccupied != 0) {
            seed += g_status_685170.buffers.Char[index].experience;
        }
    }
    srand(seed);
    for (; spins != 0; --spins) {
        Random(100);
    }
    char score = ScoreNpcTheft0050BAF0(character, npc, item_id, count);
    if (static_cast<signed char>(npc->suspicion_84) < 'd') {
        ++npc->suspicion_84;
    }
    char roll = static_cast<char>(Random(100));
    if (roll < '`' && roll <= score * 2) {
        if (roll < score) {
            PracticeCharacterSkill(character, 0xd, 5, 0);
            return 0;
        }
        return 1;
    }
    return 2;
}

// GLOBAL: WIZ8 0x005EC29C
const float g_float_005ec29c = 0.7853981256484985f;

/* Probe the navigator from the party eye at three height bands, reporting
   whether any band reaches. */
// FUNCTION: WIZ8 0x0050B2F0
bool ProbeNpcPlacementNearParty(int /*party_slot*/, int /*mode*/, srVector3T<float>* position_out)
{
    srVector3T<float> party_position;
    float yaw;

    GetCameraPosition(&party_position);
    party_position.y -= g_default_world_height_00603ac8;
    yaw = GetCameraYawRadians() + g_float_005ec29c;
    if (g_octree_6598a4->FindNavigatorPosition(&party_position, yaw, 1000.0f, 1, position_out, 1, 0,
                                               1, 10, 0) > 0) {
        return 1;
    }
    if (g_octree_6598a4->FindNavigatorPosition(&party_position, yaw, 1000.0f, 1, position_out, 1, 0,
                                               1, 20, 0) > 0) {
        return 1;
    }
    g_octree_6598a4->FindNavigatorPosition(&party_position, yaw, 1000.0f, 1, position_out, 1, 0, 1,
                                           30, 0);
    return 0;
}

/* The frame-0x10 callback the 0x1b6 NPC cycle installs: mark the monster,
   reset its navigator to the origin, and fire the VOC_BELA_CC voice event on
   the 0x89 NPC kind sharing the queue. */
// FUNCTION: WIZ8 0x0050D480
void TriggerBelaVoice0050D480(W8Monster* monster)
{
    srVector3T<float> position;

    monster->flags_1dc |= 0x40;
    position.SetZero();
    monster->SetPosition(&position);

    W8NpcState* npc = 0;

    for (int index = 0; index < g_npc_states->GetCount(); ++index) {
        W8NpcState* candidate = *g_npc_states->GetAt(index);
        if (candidate->record->kind == 0x89) {
            npc = candidate;
            break;
        }
    }
    if (npc != 0) {
        QueueNpcScriptNotice(npc, 0, -1, 0, 0);
        return;
    }
    srAssertFail("pNPC", NPC_MANAGER_CPP, 0xbec, "Cannot find VOC_BELA_CC");
}

/* The NPC side of the global frame. Two timed world events first: one retires
   NPC monster 0x1b3 through its dying state, the next starts the 0x1b6 cycle
   with a callback. Then a one-shot pass over the NPC states releases the
   monster binding of the partner each candidate names. Afterwards the group
   event counter can consume a fact and run the NPC 0x8d teardown, the party's
   portrait rows can trigger their NPC's spoken event, and two long reward
   timers set their facts. */
// FUNCTION: WIZ8 0x0050D530
void UpdateNpcEvents0050D530(void)
{
    W8MonsterGroup* group;
    W8MonsterInfo* monster_info;
    W8NpcState* npc;
    unsigned int index;

    if (g_status_685170.trang_check_pending_49bb != 0 &&
        static_cast<unsigned int>(g_status_685170.world_clock -
                                  g_status_685170.trang_check_clock_49b7) > 0x2a30) {
        if (GetFact(W8_FACT_ALIGNMENT_UMPANI) != 0 && Random(100) < 6) {
            SetFact(W8_FACT_TRANG_YOU_ARE_BUSTED, 1, 0);
        }
        g_status_685170.trang_check_pending_49bb = 0;
    }
    if (g_status_685170.savant_hack_tick != 0 &&
        static_cast<unsigned int>(GetTickCount() - g_status_685170.savant_hack_tick) > 0x32) {
        group = FindFirstMonsterByID(0x1b3);
        if (group != 0) {
            index =
                MonsterGetIndexByLocationID(0xc17, NPC_MANAGER_CPP, group->leader_location_id, 1);
            monster_info = MonsterGetScriptPartByLocationIndex(index);
            MonsterStartsDying(monster_info, 1);
        }
        g_status_685170.savant_hack_tick = 0;
        g_status_685170.bela_cycle_tick = GetTickCount();
    }
    if (g_status_685170.bela_cycle_tick != 0 &&
        static_cast<unsigned int>(GetTickCount() - g_status_685170.bela_cycle_tick) > 0x1388) {
        group = FindFirstMonsterByID(0x1b6);
        if (group != 0) {
            index =
                MonsterGetIndexByLocationID(0xc2f, NPC_MANAGER_CPP, group->leader_location_id, 1);
            monster_info = MonsterGetScriptPartByLocationIndex(index);
            StartMonsterCycle(monster_info, 0x10, 1);
            monster_info->p3D->SetCycleCallback004CA340(0x10, TriggerBelaVoice0050D480);
        }
        g_status_685170.bela_cycle_tick = 0;
    }

    if (g_status_685170.npc_restore_pending_2430 != 0) {
        W8NpcState* partner = 0;

        for (int slot = 0; slot < g_npc_states->GetCount(); ++slot) {
            npc = *g_npc_states->GetAt(slot);
            if (npc->binding_unavailable != 0 || npc->restored_ea == 0) {
                g_status_685170.npc_restore_pending_2430 = 0;
                continue;
            }
            unsigned int kind = npc->name_style;

            partner = 0;
            for (int search = 0; search < g_npc_states->GetCount(); ++search) {
                W8NpcState* candidate = *g_npc_states->GetAt(search);
                if ((unsigned int)candidate->record->kind == kind) {
                    partner = candidate;
                    break;
                }
            }
            if (!partner->has_monster) {
                g_status_685170.npc_restore_pending_2430 = 0;
                continue;
            }
            if (partner->is_present) {
                index =
                    MonsterGetIndexByLocationID(0x2a1, NPC_MANAGER_CPP, partner->location_id, 1);
                monster_info = MonsterGetScriptPartByLocationIndex(index);
                if (monster_info != 0) {
                    index = MonsterGetIndexByLocationID(0x9bb, NPC_MANAGER_CPP,
                                                        monster_info->location_id, 1);
                    RemoveMonster(index, 1);
                }
            }
            int partner_index = partner->partner_index_2c;
            if (partner_index != -1 && partner_index <= g_npc_states->GetCount()) {
                W8NpcState* released = *g_npc_states->GetAt(partner_index);
                released->has_monster = 0;
                ReleaseNpcScriptFile0055A0A0(released->script_file);
                released->script_file = 0;
                if (released->record->monster_bound_054 != 0) {
                    released->binding_unavailable = 1;
                }
            }
            g_status_685170.npc_restore_pending_2430 = 0;
        }
    }

    if (gXStatus.fCombatMode == 0 && g_status_685170.vi_event_stage_498b > 1) {
        bool run_event = GetFact(W8_FACT_VI_IS_DEAD) != 0;

        if (!run_event) {
            for (int search = 0; search < g_npc_states->GetCount(); ++search) {
                W8NpcState* candidate = *g_npc_states->GetAt(search);
                if (candidate->record->kind == 99) {
                    if (candidate != 0 && static_cast<unsigned char>(candidate->spawned_04) != 0) {
                        run_event = true;
                    }
                    break;
                }
            }
        }
        if (run_event) {
            g_status_685170.vi_event_stage_498b = 0;
            for (int search = 0; search < g_npc_states->GetCount(); ++search) {
                W8NpcState* candidate = *g_npc_states->GetAt(search);
                if (candidate->record->kind == 0x8d) {
                    if (candidate != 0) {
                        BeginNpcDialogue(candidate, 0, 0, 0, 0);
                    }
                    break;
                }
            }
            QueueNpcMessageLine(W8_NPC_MSG_ENDGAME_SCREEN, 0);
        }
    }

    if (gXStatus.fSurprisePossible == 0) {
        for (int slot = 0; slot < 2; ++slot) {
            W8PartySlotRow* row = &g_status_685170.buffers.XChar[slot];
            W8Character* character = &g_status_685170.buffers.Char[slot];

            if (row->fOccupied != 0 && character->hp_current != 0) {
                W8NpcState* npc_state = 0;
                if (g_npc_states != 0) {
                    npc_state = *g_npc_states->GetAt(row->npc_index);
                    if (npc_state != 0 && npc_state->binding_unavailable != 0) {
                        npc_state = 0;
                    }
                }
                if (row->npc_bound_fe != 0 &&
                    (g_status_685170.world_clock - npc_state->event_clock_eb) > 0x168) {
                    if (Random(2) == 0) {
                        npc_state->event_clock_eb = g_status_685170.world_clock + Random(6) * 0x3c;
                    } else {
                        int event = Random(2) == 0 ? 0x57 : 0x58;
                        QueueCharacterEvent(character, event, 0, g_effect_argument_005ed8c8,
                                            g_effect_argument_005ed914);
                        npc_state->event_clock_eb = g_status_685170.world_clock;
                    }
                }
            }
        }
    }

    if (g_status_685170.fact_b8_pending_248a != 0 &&
        static_cast<unsigned int>(g_status_685170.world_clock -
                                  g_status_685170.fact_b8_clock_2493) > 0x2a300) {
        g_status_685170.fact_b8_pending_248a = 0;
        SetFact(0xb8, 1, 0);
    }
    if (g_status_685170.greeting_pending_2497 != 0 &&
        (g_status_685170.world_clock - g_status_685170.binding_reset_clock_242a) > 0x3c) {
        g_status_685170.greeting_pending_2497 = 0;
        SelectStartNpcGreeting();
    }
}

/* Reset the NPC bindings of the first party slots at level entry: stamp the
   sight clock mark, then for each occupied, living slot clear the bound NPC's
   release flag and the slot's own flag and run the per-slot companion reset.
   The lookup is GetNpcState's; the binary's own null test only zeroes the
   result before the store still writes through it, so retail leaves that
   invariant-violating path as a null write. */
// FUNCTION: WIZ8 0x0050db50
void ResetNpcBindingsForParty0050DB50(void)
{
    g_status_685170.binding_reset_clock_242a = g_status_685170.world_clock;
    g_status_685170.binding_reset_pending_242e = 1;
    for (int party_slot = 0; party_slot < 2; ++party_slot) {
        W8PartySlotRow* row = &g_status_685170.buffers.XChar[party_slot];
        W8Character* character = &g_status_685170.buffers.Char[party_slot];

        if (row->fOccupied != 0 && character->hp_current != 0) {
            GetNpcState(row->npc_index)->incapacitated_e8 = 0;
            row->npc_bound_fe = 0;
            RebuildConditionsAndDerivedStats(party_slot);
        }
    }
}

/* Subtract 0x14 from every attribute adjustment and the 0x13 byte run of the
   character's modifier block while the slot's bound-NPC flag is set. The
   condition/enchantment rebuild at 0x0050E650 runs it as the last source. */
// FUNCTION: WIZ8 0x0050dbf0
void ApplyBoundNpcPenalty0050DBF0(W8Character* character, W8GameplayModifierBlock* target)
{
    unsigned int index;

    if (g_status_685170.buffers.XChar[CharacterPointerToPartySlot(character)].npc_bound_fe == 0) {
        return;
    }
    for (index = 0; index < 7; ++index) {
        target->attribute_adjustments[index] -= 0x14;
    }
    for (index = 0; index < 0x29; ++index) {
        target->skill_bonus_13[index] -= 0x14;
    }
}

/* Restore the named entity's NPC on its level, or stamp the pending restore
   for the level-entry pass. The runtime node is created when no state for the
   record kind exists. */
// FUNCTION: WIZ8 0x0050C1C0
void RestoreNamedNpcAtLevel0050C1C0(int kind, char level, const char* entity_name)
{
    int count = g_npc_states->count;
    W8NpcState* npc = 0;

    for (int index = 0; index < count; ++index) {
        W8NpcState** slot = g_npc_states->data;
        if (index < count) {
            slot += index;
        }
        W8NpcState* candidate = *slot;
        if (candidate->record->kind == kind) {
            npc = candidate;
            break;
        }
    }
    if (npc == 0) {
        npc = CreateNpcRuntimeNode(kind);
    }
    if (level == g_status_685170.current_level) {
        RestoreNpcMonster0050C560(npc, entity_name);
        npc->restored_ea = 0;
        return;
    }
    npc->pending_restore = 1;
    strcpy(npc->restore_entity_name, entity_name);
    npc->pending_restore_level = level;
}

/* Drop the pending-restore flag from every NPC bound to the loaded level whose
   restore check passes. The state vector is re-read after the check because it
   can remove an entry. */
// FUNCTION: WIZ8 0x0050c270
void ClearPendingNpcLevelFlags0050C270(void)
{
    unsigned int count = g_npc_states->count;
    unsigned int npc_index = 0;

    if (count != 0) {
        do {
            W8NpcState** slot = g_npc_states->data;
            if (npc_index < count) {
                slot += npc_index;
            }
            W8NpcState* npc = *slot;

            if (npc->pending_restore != 0 && npc->binding_unavailable == 0 &&
                npc->pending_restore_level == g_status_685170.current_level) {
                if (RestoreNpcMonster0050C560(npc, npc->restore_entity_name) != 0) {
                    npc->pending_restore = 0;
                }
            }
            count = g_npc_states->count;
            ++npc_index;
        } while (npc_index < count);
    }
}

/* Release the monster binding of every NPC whose stamped release flag matches
   the loaded level. The companion NPC is found by the record kind matching the
   NPC's naming style; its live monster is destroyed and its own binding is
   handed back. */
// FUNCTION: WIZ8 0x0050c2e0
void ReleaseNpcMonsterBindings0050C2E0(void)
{
    unsigned int count = g_npc_states->count;
    unsigned int npc_index = 0;

    if (count == 0) {
        return;
    }
    do {
        W8NpcState** slot = g_npc_states->data;
        if (npc_index < count) {
            slot += npc_index;
        }
        W8NpcState* npc = *slot;

        if (npc->pending_release != 0 && npc->binding_unavailable == 0 &&
            npc->pending_release_level == g_status_685170.current_level) {
            W8NpcState* companion = 0;
            bool found = false;

            for (unsigned int index = 0; index < count; ++index) {
                W8NpcState** candidate_slot = g_npc_states->data;

                if (index < count) {
                    candidate_slot += index;
                }
                companion = *candidate_slot;
                if (static_cast<unsigned int>(companion->record->kind) == npc->name_style) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                companion = 0;
            }
            if (companion->has_monster) {
                if (companion->is_present) {
                    unsigned int monster_index = MonsterGetIndexByLocationID(
                        0x2a1, NPC_MANAGER_CPP, companion->location_id, 1);
                    W8MonsterInfo* monster_info =
                        MonsterGetScriptPartByLocationIndex(monster_index);

                    if (monster_info != 0) {
                        unsigned char destroy = 1;

                        monster_index = MonsterGetIndexByLocationID(0x9bb, NPC_MANAGER_CPP,
                                                                    monster_info->location_id, 1);
                        RemoveMonster(monster_index, destroy);
                    }
                }
                {
                    int partner_index = companion->partner_index_2c;

                    if (partner_index != -1 && partner_index >= 0 &&
                        partner_index <= g_npc_states->GetCount()) {
                        W8NpcState* target = *g_npc_states->GetAt(partner_index);

                        target->has_monster = 0;
                        ReleaseNpcScriptFile0055A0A0(target->script_file);
                        target->script_file = 0;
                        if (target->record->monster_bound_054 != 0) {
                            target->binding_unavailable = 1;
                        }
                    }
                }
            }
        }
        count = g_npc_states->count;
        ++npc_index;
    } while (npc_index < count);
}

/* Release the monster binding held under this NPC's naming style, now or when
   the stamped level is loaded: the same companion lookup the level-entry
   release pass runs - the NPC whose record kind is this state's name_style. */
// FUNCTION: WIZ8 0x0050C440
void ReleaseNpcMonsterBinding0050C440(W8NpcState* npc, char level)
{
    if (level == g_status_685170.current_level) {
        unsigned int count = g_npc_states->count;
        W8NpcState* companion = 0;
        for (unsigned int index = 0; index < count; ++index) {
            W8NpcState** slot = g_npc_states->data;
            if (index < count) {
                slot += index;
            }
            W8NpcState* candidate = *slot;
            if (static_cast<unsigned int>(candidate->record->kind) == npc->name_style) {
                companion = candidate;
                break;
            }
        }
        if (companion->has_monster) {
            if (companion->is_present) {
                unsigned int monster_index =
                    MonsterGetIndexByLocationID(0x2a1, NPC_MANAGER_CPP, companion->location_id, 1);
                W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);

                if (monster_info != 0) {
                    RemoveMonster(MonsterGetIndexByLocationID(0x9bb, NPC_MANAGER_CPP,
                                                              monster_info->location_id, 1),
                                  1);
                }
            }
            int partner_index = companion->partner_index_2c;

            if (partner_index != -1 && partner_index >= 0 &&
                partner_index <= g_npc_states->GetCount()) {
                W8NpcState* target = *g_npc_states->GetAt(partner_index);

                target->has_monster = 0;
                ReleaseNpcScriptFile0055A0A0(target->script_file);
                target->script_file = 0;
                if (target->record->monster_bound_054 != 0) {
                    target->binding_unavailable = 1;
                }
            }
        }
    } else {
        npc->pending_release = 1;
        npc->pending_release_level = level;
    }
}

/* Release the monster binding of the first NPC whose record kind matches:
   its live monster is destroyed and the partner node its index names gets its
   binding handed back. */
// FUNCTION: WIZ8 0x0050C680
void ReleaseNpcMonsterByKind(int kind)
{
    W8NpcState* npc = 0;

    for (int index = 0; index < g_npc_states->count; ++index) {
        W8NpcState* candidate = *g_npc_states->GetAt(index);
        if (candidate->record->kind == kind) {
            npc = candidate;
            break;
        }
    }
    if (npc->has_monster) {
        if (npc->is_present) {
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(0x2a1, NPC_MANAGER_CPP, npc->location_id, 1));
            if (monster_info != 0) {
                RemoveMonster(MonsterGetIndexByLocationID(0x9bb, NPC_MANAGER_CPP,
                                                          monster_info->location_id, 1),
                              1);
            }
        }
        unsigned int partner_index = npc->partner_index_2c;
        if (partner_index != 0xffffffff && static_cast<int>(partner_index) <= g_npc_states->count) {
            W8NpcState* target = *g_npc_states->GetAt(partner_index);

            target->has_monster = 0;
            ReleaseNpcScriptFile0055A0A0(target->script_file);
            target->script_file = 0;
            if (target->record->monster_bound_054 != 0) {
                target->binding_unavailable = 1;
            }
        }
    }
}

/* Place or move this NPC's monster at the named world entity. Without a live
   monster it loads MONSTERS.DBS, finds the NPC-linked species whose name-style
   byte matches, and asks CreateGroup to create it; with a live monster it
   repositions the Navigator subobject. */
// FUNCTION: WIZ8 0x0050c560
unsigned char RestoreNpcMonster0050C560(W8NpcState* npc, const char* entity_name)
{
    srVector3T<float> position;
    srVector3T<float> copied;

    if (npc->has_monster == 0) {
        W8MonsterRecord* records = 0;
        unsigned int index = 0;

        LoadMonsterDatabase(&records);
        if (gXStatus.uiMonstersInDatabase != 0) {
            for (; index < gXStatus.uiMonstersInDatabase; ++index) {
                if ((records[index].flags_0d0 & 1) != 0 &&
                    records[index].npc_kind_0cd == npc->name_style) {
                    break;
                }
            }
        }
        FreeIfNotNull(records);
        if (index == gXStatus.uiMonstersInDatabase) {
            return 0;
        }
        if (FindEntityByName(entity_name, &position, 0, 0) == 0) {
            return 0;
        }
        copied = position;
        CreateGroup(index, 1, &copied, 1, 0, 1);
        return 1;
    }
    if (npc->is_present == 0) {
        return 0;
    }
    {
        unsigned int monster_index =
            MonsterGetIndexByLocationID(0x2a1, NPC_MANAGER_CPP, npc->location_id, 1);
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);

        if (monster_info == 0) {
            return 0;
        }
        if (FindEntityByName(entity_name, &position, 0, 0) == 0) {
            return 0;
        }
        static_cast<W8Navigator*>(monster_info->p3D)->SetPosition(&position);
        return 1;
    }
}

/* Run one marked NPC's scripted event step. The special naming styles run
   first - Vi Domina's fact, Sgt Rubble's two-stage teleport on level 13 and
   Glumph's mission item - then mode 0 arms the one-shot event pass while any
   other mode releases the companion monster binding, and the NPC's own
   restore is scheduled last. */
// FUNCTION: WIZ8 0x0050CF70
void HandleMarkedNpcEvent0050CF70(W8NpcState* npc, char mode)
{
    W8MonsterInfo* monster_info = GetNpcMonsterInfo(npc);

    if (monster_info != 0 && monster_info->highest_condition > 0xe) {
        return;
    }
    if (npc->name_style == ' ' || npc->name_style == '!') {
        npc->marked_e9 = 0;
        npc->marked_114 = 0;
        return;
    }
    if (npc->name_style == W8_NPC_VI_DOMINA) {
        SetFact(0x22b, 0, 0);
    }
    if (npc->name_style == ')' && g_status_685170.current_level == 0xd) {
        srVector3T<float> position;

        if (GetLocationVarIDByName("CODESgtRubbleTeleport") == -1 ||
            GetLocationVarValueByName("CODESgtRubbleTeleport") == 0) {
            if (FindEntityByName("RubbleCovert", &position, 0, 0)) {
                static_cast<W8Navigator*>(monster_info->p3D)->SetPosition(&position);
                if (GetLocationVarIDByName("CODESgtRubbleTeleport") == -1) {
                    CreateLocationVar("CODESgtRubbleTeleport", 1);
                } else {
                    SetTriggerVariableByName00444030("CODESgtRubbleTeleport", 1);
                }
                Trigger* trigger = FindTriggerByName("door08");

                if (trigger != 0) {
                    trigger->Run(-1);
                }
            }
            SetFact(0x21e, 0, 0);
            npc->marked_e9 = 0;
            npc->marked_114 = 0;
            return;
        }
        if (GetLocationVarValueByName("CODESgtRubbleTeleport") == 1) {
            if (FindEntityByName("rubbleUnderWater", &position, 0, 0)) {
                static_cast<W8Navigator*>(monster_info->p3D)->SetPosition(&position);
                SetTriggerVariableByName00444030("CODESgtRubbleTeleport", 2);
            }
            npc->marked_e9 = 0;
            npc->marked_114 = 0;
            return;
        }
    }
    if (npc->name_style == W8_NPC_GLUMPH && GetFact(W8_FACT_UMISSION_SCUBA_DONE) == 0) {
        srVector3T<float> position;

        monster_info = GetNpcMonsterInfo(npc);
        if (monster_info != 0) {
            position = monster_info->p3D->GetPosition();
            W8WorldItem* item = SpawnItem(0x1e6, &position, 3, 1);

            if (item != 0) {
                ActivateItem(item);
            }
        } else if (FindEntityByName("NP_Glumph", &position, 0, 0)) {
            W8WorldItem* item = SpawnItem(0x1e6, &position, 3, 1);

            if (item != 0) {
                ActivateItem(item);
            }
        }
    }
    if (mode != 0) {
        unsigned int count = g_npc_states->count;
        W8NpcState* companion = 0;

        for (unsigned int index = 0; index < count; ++index) {
            W8NpcState** slot = g_npc_states->data;

            if (index < count) {
                slot += index;
            }
            W8NpcState* candidate = *slot;
            if (static_cast<unsigned int>(candidate->record->kind) == npc->name_style) {
                companion = candidate;
                break;
            }
        }
        if (companion->has_monster) {
            if (companion->is_present) {
                unsigned int monster_index =
                    MonsterGetIndexByLocationID(0x2a1, NPC_MANAGER_CPP, companion->location_id, 1);
                W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(monster_index);

                if (info != 0) {
                    RemoveMonster(
                        MonsterGetIndexByLocationID(0x9bb, NPC_MANAGER_CPP, info->location_id, 1),
                        1);
                }
            }
            int partner_index = companion->partner_index_2c;

            if (partner_index != -1 && partner_index >= 0 && partner_index <= g_npc_states->count) {
                W8NpcState* target = *g_npc_states->GetAt(partner_index);

                target->has_monster = 0;
                ReleaseNpcScriptFile0055A0A0(target->script_file);
                target->script_file = 0;
                if (target->record->monster_bound_054 != 0) {
                    target->binding_unavailable = 1;
                }
            }
        }
    } else {
        npc->restored_ea = 1;
        g_status_685170.npc_restore_pending_2430 = 1;
    }
    if (npc->marked_114 == 0) {
        if (npc->name_style == 7 && GetFact(0x1f) != 0) {
            RestoreNamedNpcAtLevel0050C1C0(npc->name_style, 0x11, "NP_MylesCell");
        } else {
            RestoreNamedNpcAtLevel0050C1C0(npc->name_style, npc->record->restore_level,
                                           npc->record->restore_entity_name);
        }
    }
    npc->marked_e9 = 0;
    npc->marked_114 = 0;
}

/* Hand back the monster binding of every marked NPC, then find the companion
   whose record kind matches the NPC's naming style and release its live
   monster and its own binding. The state vector is re-read after every
   callback. */
// FUNCTION: WIZ8 0x0050da00
void ReleaseMarkedNpcBindings0050DA00(void)
{
    unsigned int count = g_npc_states->count;
    unsigned int npc_index = 0;

    if (count == 0) {
        return;
    }
    do {
        W8NpcState** slot = g_npc_states->data;
        if (npc_index < count) {
            slot += npc_index;
        }
        W8NpcState* npc = *slot;

        if (npc->binding_unavailable == 0) {
            if (npc->marked_e9 != 0) {
                HandleMarkedNpcEvent0050CF70(npc, 1);
            }
            if (npc->pending_restore != 0) {
                W8NpcState* companion = 0;
                bool found = false;

                for (unsigned int index = 0; index < count; ++index) {
                    W8NpcState** candidate_slot = g_npc_states->data;

                    if (index < count) {
                        candidate_slot += index;
                    }
                    companion = *candidate_slot;
                    if (static_cast<unsigned int>(companion->record->kind) == npc->name_style) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    companion = 0;
                }
                if (companion->has_monster) {
                    if (companion->is_present) {
                        unsigned int monster_index = MonsterGetIndexByLocationID(
                            0x2a1, NPC_MANAGER_CPP, companion->location_id, 1);
                        W8MonsterInfo* monster_info =
                            MonsterGetScriptPartByLocationIndex(monster_index);

                        if (monster_info != 0) {
                            unsigned char destroy = 1;

                            monster_index = MonsterGetIndexByLocationID(
                                0x9bb, NPC_MANAGER_CPP, monster_info->location_id, 1);
                            RemoveMonster(monster_index, destroy);
                        }
                    }
                    {
                        int partner_index = companion->partner_index_2c;

                        if (partner_index != -1 && partner_index >= 0 &&
                            partner_index <= g_npc_states->GetCount()) {
                            W8NpcState* target = *g_npc_states->GetAt(partner_index);

                            target->has_monster = 0;
                            ReleaseNpcScriptFile0055A0A0(target->script_file);
                            target->script_file = 0;
                            if (target->record->monster_bound_054 != 0) {
                                target->binding_unavailable = 1;
                            }
                        }
                    }
                }
            }
        }
        count = g_npc_states->count;
        ++npc_index;
    } while (npc_index < count);
}

/* The activation callback RebindNpcLevelTriggers0050AC60 installs on every NPC
   trigger: queue the NPC's script notice, handing it the item on the cursor
   when the trigger's 0x100 flag or the NPC's '{' naming style asks for it. */
// FUNCTION: WIZ8 0x0050abf0
bool NotifyNpcTriggerActivation0050ABF0(Trigger* trigger)
{
    W8ItemInstance* item = 0;

    if (gXStatus.fNpcDialogueMode == 0) {
        W8NpcState* npc = *g_npc_states->GetAt(trigger->m_lData1);

        if ((trigger->flags_0a0 & W8_TRIGGER_ENABLED) != 0 || npc->name_style == '{') {
            if (g_status_685170.item_in_cursor != 0) {
                item = &g_status_685170.item_in_hand_235b;
            }
        }
        QueueNpcScriptNotice(npc, item, -1, 0, 0);
    }
    return false;
}

/* Rebuild the level's NPC bindings: first drop the followers whose record or
   presence rules changed, then re-install each NPC trigger's activation
   callback and re-stamp the NPC from the loaded level. */
// FUNCTION: WIZ8 0x0050ac60
void RebindNpcLevelTriggers0050AC60(void)
{
    unsigned int count = g_npc_states->count;
    unsigned int npc_index = 0;

    if (count != 0) {
        do {
            W8NpcState** slot = g_npc_states->data;
            if (npc_index < count) {
                slot += npc_index;
            }
            W8NpcState* npc = *slot;

            if (npc->has_monster && (npc->record->merchant_056 != 0 ||
                                     (npc->record->voice_script_2ea != 0 && !npc->is_present))) {
                npc->has_monster = 0;
                ReleaseNpcScriptFile0055A0A0(npc->script_file);
                npc->script_file = 0;
                if (npc->record->monster_bound_054 != 0) {
                    npc->binding_unavailable = 1;
                }
            }
            count = g_npc_states->count;
            ++npc_index;
        } while (npc_index < count);
    }

    count = g_npc_states->count;
    for (npc_index = 0; npc_index < count; ++npc_index) {
        W8NpcState** slot = g_npc_states->data;
        char trigger_name[40];

        if (npc_index < count) {
            slot += npc_index;
        }
        W8NpcState* npc = *slot;

        if (npc->record->merchant_056 != 0 || npc->record->voice_script_2ea != 0) {
            sprintf(trigger_name, "_%S", npc->record->source_name_004);
            Trigger* trigger = FindTriggerByName(trigger_name);

            if (trigger != 0) {
                trigger->activation_callback_360 = NotifyNpcTriggerActivation0050ABF0;
                trigger->m_lData1 = static_cast<int>(npc_index);
                npc->has_monster = 1;
                npc->level_band =
                    static_cast<unsigned char>(GetLevelBand(g_status_685170.current_level));
                npc->bound_level = static_cast<unsigned char>(g_status_685170.current_level);
                ReloadNpcScriptResources(npc);
                npc->is_present = 0;
            }
        }
        count = g_npc_states->count;
    }
}

/* Clear the first item_ids_30 slot matching `item_id`, optionally building the
   item into `out` first - the scheduled-stock handoff the pickpocket and trade
   resolutions run. */
// FUNCTION: WIZ8 0x0050BA80
unsigned char ClearNpcScheduledItem(W8NpcState* npc, int item_id, W8ItemInstance* out)
{
    if (npc == 0) {
        srAssertFail("pNPC", NPC_MANAGER_CPP, 0x7b7, 0);
    }
    for (int index = 0; index < 40; ++index) {
        if (npc->item_ids_30[index] == item_id) {
            if (out != 0) {
                ReplaceOrCreateItem(out, npc->item_ids_30[index], 1, 1, 0);
            }
            npc->item_ids_30[index] = -1;
            return 1;
        }
    }
    return 0;
}

/* Scan the two bound lead NPCs for ones that refuse the destination level: an
   NPC that serves the destination region but not the current one queues its
   departure event, and on level 13 a Rodan or Drazic travelling without its
   healthy partner does the same. Any queued event raises the travel-confirm
   message and resets the level data vectors. */
// FUNCTION: WIZ8 0x0050DEC0
char QueueNpcDepartureEvents0050DEC0(int destination_level)
{
    char queued = 0;

    for (int slot = 0; slot < 2; ++slot) {
        W8PartySlotRow* row = &g_status_685170.buffers.XChar[slot];
        W8Character* character = &g_status_685170.buffers.Char[slot];

        if (row->fOccupied == 0 || character->hp_current == 0) {
            continue;
        }
        W8NpcState* npc = GetNpcState(row->npc_index);
        if (character->highest_condition >= 0xf) {
            continue;
        }
        int service = 0;
        if (g_npc_services[0].service_id != 0xffffffff) {
            while (g_npc_services[service].service_id != 0xffffffff) {
                if (g_npc_services[service].service_id ==
                    static_cast<unsigned int>(GetLevelBand(destination_level))) {
                    if ((npc->record->service_flags & g_npc_services[service].bit) != 0) {
                        int other = 0;
                        bool serves_here = false;
                        if (g_npc_services[0].service_id != 0xffffffff) {
                            while (g_npc_services[other].service_id != 0xffffffff) {
                                if (g_npc_services[other].service_id ==
                                    static_cast<unsigned int>(
                                        GetLevelBand(g_status_685170.current_level))) {
                                    if ((npc->record->service_flags & g_npc_services[other].bit) !=
                                        0) {
                                        serves_here = true;
                                    }
                                    break;
                                }
                                ++other;
                            }
                        }
                        if (!serves_here) {
                            int departure = 0;
                            if (g_npc_services[0].service_id != 0xffffffff) {
                                while (g_npc_services[departure].service_id != 0xffffffff) {
                                    if (g_npc_services[departure].service_id ==
                                        static_cast<unsigned int>(
                                            GetLevelBand(destination_level))) {
                                        QueueCharacterEvent(
                                            character, g_npc_services[departure].npc_id, 0,
                                            g_effect_argument_005ed8c8, g_effect_argument_005ed914);
                                        queued = 1;
                                        break;
                                    }
                                    ++departure;
                                }
                            }
                        }
                    }
                    break;
                }
                ++service;
            }
        }
        if (GetLevelBand(g_status_685170.current_level) == 0xd) {
            int event = 0;
            if (npc->name_style == W8_NPC_RODAN) {
                bool paired = false;
                if (g_status_685170.buffers.XChar[0].fOccupied != 0) {
                    W8NpcState* lead = GetNpcState(g_status_685170.buffers.XChar[0].npc_index);
                    if (lead->name_style == W8_NPC_DRAZIC &&
                        g_status_685170.buffers.Char[0].highest_condition < 0xf) {
                        paired = true;
                    }
                }
                if (!paired && g_status_685170.buffers.XChar[1].fOccupied != 0) {
                    W8NpcState* lead = GetNpcState(g_status_685170.buffers.XChar[1].npc_index);
                    if (lead->name_style == W8_NPC_DRAZIC &&
                        g_status_685170.buffers.Char[1].highest_condition < 0xf) {
                        paired = true;
                    }
                }
                if (!paired) {
                    event = 0x6c;
                }
            } else if (npc->name_style == W8_NPC_DRAZIC) {
                bool paired = false;
                if (g_status_685170.buffers.XChar[0].fOccupied != 0) {
                    W8NpcState* lead = GetNpcState(g_status_685170.buffers.XChar[0].npc_index);
                    if (lead->name_style == W8_NPC_RODAN &&
                        g_status_685170.buffers.Char[0].highest_condition < 0xf) {
                        paired = true;
                    }
                }
                if (!paired && g_status_685170.buffers.XChar[1].fOccupied != 0) {
                    W8NpcState* lead = GetNpcState(g_status_685170.buffers.XChar[1].npc_index);
                    if (lead->name_style == W8_NPC_RODAN &&
                        g_status_685170.buffers.Char[1].highest_condition < 0xf) {
                        paired = true;
                    }
                }
                if (!paired) {
                    event = 0x67;
                }
            }
            if (event != 0) {
                QueueCharacterEvent(character, event, 0, g_effect_argument_005ed8c8,
                                    g_effect_argument_005ed914);
                queued = 1;
            }
        }
    }
    if (queued != 0) {
        QueueNpcMessageLine(W8_NPC_MSG_TRAVEL_CONFIRM, destination_level);
        ResetLevelDataVectors0041F0D0();
    }
    return queued;
}

/* Refuse the destination level on behalf of each bound lead NPC: an NPC that
   serves the destination region speaks the group-action line, and a Rodan or
   Drazic whose healthy partner is not also in the party does the same. */
// FUNCTION: WIZ8 0x0050E230
void QueueNpcTravelRefusals(int destination_level)
{
    ClearLevelDataFlag6();
    for (int slot = 0; slot < 2; ++slot) {
        W8PartySlotRow* row = &g_status_685170.buffers.XChar[slot];
        W8Character* character = &g_status_685170.buffers.Char[slot];

        if (row->fOccupied == 0 || character->hp_current == 0) {
            continue;
        }
        W8NpcState* npc = GetNpcState(row->npc_index);
        if (character->highest_condition >= 0xf) {
            continue;
        }
        int service = 0;
        if (g_npc_services[0].service_id != 0xffffffff) {
            while (g_npc_services[service].service_id != 0xffffffff) {
                if (g_npc_services[service].service_id ==
                    static_cast<unsigned int>(GetLevelBand(destination_level))) {
                    if ((npc->record->service_flags & g_npc_services[service].bit) != 0) {
                        BeginScriptedWorldAction();
                        QueueNpcMessageLine(W8_NPC_MSG_GROUP_ACTION, slot);
                    }
                    break;
                }
                ++service;
            }
        }
        if (npc->name_style == W8_NPC_RODAN) {
            bool paired = false;
            if (g_status_685170.buffers.XChar[0].fOccupied != 0) {
                W8NpcState* lead = GetNpcState(g_status_685170.buffers.XChar[0].npc_index);
                if (lead->name_style == W8_NPC_DRAZIC &&
                    g_status_685170.buffers.Char[0].highest_condition < 0xf) {
                    paired = true;
                }
            }
            if (!paired) {
                if (g_status_685170.buffers.XChar[1].fOccupied == 0) {
                    BeginScriptedWorldAction();
                    QueueNpcMessageLine(W8_NPC_MSG_GROUP_ACTION, slot);
                    return;
                }
                W8NpcState* lead = GetNpcState(g_status_685170.buffers.XChar[1].npc_index);
                if (lead->name_style != W8_NPC_DRAZIC ||
                    g_status_685170.buffers.Char[1].highest_condition >= 0xf) {
                    BeginScriptedWorldAction();
                    QueueNpcMessageLine(W8_NPC_MSG_GROUP_ACTION, slot);
                    return;
                }
            }
        }
        if (npc->name_style == W8_NPC_DRAZIC) {
            bool paired = false;
            if (g_status_685170.buffers.XChar[0].fOccupied != 0) {
                W8NpcState* lead = GetNpcState(g_status_685170.buffers.XChar[0].npc_index);
                if (lead->name_style == W8_NPC_RODAN &&
                    g_status_685170.buffers.Char[0].highest_condition < 0xf) {
                    paired = true;
                }
            }
            if (!paired) {
                if (g_status_685170.buffers.XChar[1].fOccupied == 0) {
                    BeginScriptedWorldAction();
                    QueueNpcMessageLine(W8_NPC_MSG_GROUP_ACTION, slot);
                    return;
                }
                W8NpcState* lead = GetNpcState(g_status_685170.buffers.XChar[1].npc_index);
                if (lead->name_style != W8_NPC_RODAN ||
                    g_status_685170.buffers.Char[1].highest_condition >= 0xf) {
                    BeginScriptedWorldAction();
                    QueueNpcMessageLine(W8_NPC_MSG_GROUP_ACTION, slot);
                    return;
                }
            }
        }
    }
}

/* Clear every item_ids_30 slot that still holds `item_id` - quote entries
   call this after the item leaves the npc's stock so it cannot be sold
   twice. The npc null check is the original's own dead assert: the pointer
   was already dereferenced by the scan. */
// FUNCTION: WIZ8 0x0050E4B0
void ClearNpcItemId(W8NpcState* npc, int item_id)
{
    int index;
    unsigned char slot;

    for (index = 0; index < 40; ++index) {
        if (npc->item_ids_30[index] == item_id) {
            if (npc == 0) {
                srAssertFail("pNPC", NPC_MANAGER_CPP, 0x7a4, 0);
            }
            slot = index;
            if (npc->item_ids_30[slot] != -1) {
                npc->item_ids_30[slot] = -1;
            }
        }
    }
}
