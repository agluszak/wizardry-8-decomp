#include "wiz8/local_code/PC_Item.h"
#include "wiz8/character.h"
#include "wiz8/item_spawning.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/magic.h"
#include "random.h"
#include "wiz8/combat_state.h"
#include "wiz8/fact_state.h"
#include "wiz8/location_variables.h"
#include "wiz8/npc_state.h"
#include "wiz8/xstatus.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/Trigger.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/sr_api.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/local_code/Sight.h"
#include "wiz8/local_screens/MainGameScreen.h"

#include <stdio.h>

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
    return npc->record->value_002 != 0;
}

/* Which of the three disposition bands the NPC falls in. The bands are cut at
   0x21 and 0x42, and the hostile band answers two rather than zero. */
// FUNCTION: WIZ8 0x0050a500
unsigned char GetNpcDispositionBand(W8NpcState* npc)
{
    char disposition = GetNpcDisposition(npc);

    if (disposition < W8_NPC_DISPOSITION_HOSTILE) {
        return 2;
    }
    return disposition < W8_NPC_DISPOSITION_FRIENDLY;
}

/* Run the update with an empty scratch block the caller does not see. */
// FUNCTION: WIZ8 0x0050b2d0
void UpdateNpc(W8NpcState* npc)
{
    srVector3T<float> scratch;

    UpdateNpcAt(npc, 0, &scratch);
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
    if (npc->unknown_c7 != 0) {
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
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
    /* Retail compiled this comparison with VC6's mixed-sign operands; the
   signedness is part of the recovered body and changing it would change
   the compare and branch. Suppress only this diagnostic here. */
    if (g_status_685170.buffers.party_rows[0].occupied != 0) {
        W8NpcState* npc = 0;
        if (g_npc_states != 0) {
            int index = g_status_685170.buffers.party_rows[0].animation_0fa;
            W8NpcState** slot = g_npc_states->data;
            if (index < g_npc_states->count) {
                slot += index;
            }
            npc = *slot;
            if (npc != 0 && npc->unknown_c7 != 0) {
                npc = 0;
            }
        }
        if (npc->name_style == kind && g_status_685170.buffers.characters[0].unknown_0b01 < 0xf) {
            return 1;
        }
    }
    if (g_status_685170.buffers.party_rows[1].occupied != 0) {
        W8NpcState* npc = 0;
        if (g_npc_states != 0) {
            int index = g_status_685170.buffers.party_rows[1].animation_0fa;
            W8NpcState** slot = g_npc_states->data;
            if (index < g_npc_states->count) {
                slot += index;
            }
            npc = *slot;
            if (npc != 0 && npc->unknown_c7 != 0) {
                npc = 0;
            }
        }
        if (npc->name_style == kind && g_status_685170.buffers.characters[1].unknown_0b01 < 0xf) {
            return 1;
        }
    }
    return 0;
#pragma clang diagnostic pop
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
    return &g_monster_manager_entries[npc->group_index];
}

/* Whether an NPC would take one item in trade. The kind that trades in nothing
   refuses outright, one particular item is always taken, and everything else
   has to be worth enough. */
// FUNCTION: WIZ8 0x0050a9c0
char WillNpcTradeForItem(W8NpcState* npc, const W8ItemInstance* item)
{
    if (npc->record->kind == W8_NPC_KIND_NO_TRADE) {
        return 0;
    }
    if (item->item_id == W8_NPC_ALWAYS_TRADED_ITEM) {
        return 1;
    }
    return GetItemStackValue(item) >= W8_NPC_MINIMUM_TRADE_VALUE;
}

/* How many of the two leading party slots are occupied. Written as nested
   tests rather than a count, which is why the first slot is read twice. */
// FUNCTION: WIZ8 0x0050b9b0
unsigned char CountLeadingPartySlots(void)
{
    if (g_status_685170.buffers.party_rows[0].occupied != 0) {
        if (g_status_685170.buffers.party_rows[1].occupied != 0) {
            return 2;
        }
        if (g_status_685170.buffers.party_rows[0].occupied != 0) {
            return 1;
        }
    }
    if (g_status_685170.buffers.party_rows[1].occupied != 0) {
        return 1;
    }
    return 0;
}

#include <string.h>

/* 0x00619DFC: one three-dword row per service - the service id, the bit that
   stands for it, and one more field nothing here reads. -1 ends the table. */
struct W8NpcServiceRow {
    unsigned int service_id;
    unsigned int bit;
    unsigned int unknown_08;
};
// GLOBAL: WIZ8 0x00619DFC
const W8NpcServiceRow g_npc_services[] = {
    {2, 1, 0x47},       {3, 2, 0x50},      {4, 0x400, 0x48},
    {5, 4, 0x49},       {7, 8, 0x4d},      {8, 0x80, 0x4c},
    {9, 0x40, 0x4b},    {10, 0x20, 0x4a},  {11, 0x10, 0x4e},
    {12, 0x100, 0x51},  {13, 0x800, 0x4f}, {14, 0x200, 0x4d},
    {15, 0x1000, 0x52}, {6, 0x2000, 0},    {0xffffffff, 0, 0x0f0e0c0d},
};

/* 0x00619F18: the name a fact substitutes, and 0x00689F60 the buffer it is
   copied into so the caller always gets a writable one. */
// GLOBAL: WIZ8 0x00619F18
const char g_substituted_npc_name[] = "RFS81B";
// GLOBAL: WIZ8 0x00689F60
char g_npc_name_buffer[52];

/* The name style that admits a substituted name. */
enum { W8_NPC_NAME_STYLE_SUBSTITUTABLE = ' ' };
/* The fact that makes the substitution happen. */
enum { W8_FACT_NPC_NAME_KNOWN = 0x44 };

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
    return monster_info->monster;
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
            return *((unsigned char*)npc + 4);
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
    if (npc->name_style == W8_NPC_NAME_STYLE_SUBSTITUTABLE &&
        GetFact(W8_FACT_NPC_NAME_KNOWN) != 0) {
        strcpy(g_npc_name_buffer, g_substituted_npc_name);
        return g_npc_name_buffer;
    }
    return npc->record->display_name;
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

            Function55A0A0(npc->unknown_00);
            npc->unknown_00 = 0;
            if (npc->record != 0 && npc->record->flag_055 != 0) {
                ClearNpcItems(npc);
            }
            delete npc->character;
            delete npc;
        }
        g_npc_states->count = 0;
    }
    for (npc_id = 0; npc_id < gXStatus.uiNpcsInDatabase; ++npc_id) {
        if (g_npc_records[npc_id].unknown_054 == 0) {
            CreateNpcRuntimeNode(npc_id);
        }
    }
}

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
    npc->name_style = (char)npc_id;
    npc->record = &g_npc_records[npc_id];
    if (npc->record->has_group != 0) {
        npc->character = new W8Character;
        InitializeNpcCharacter(npc, npc->character);
    }
    if (npc->record->flag_055 != 0) {
        PopulateNpcStock(npc);
    }
    InitializeNpcItemTable(npc);
    npc->location_id = 0;
    npc->is_present = 0;
    npc->disposition = g_npc_records[npc_id].disposition;
    npc->gold_80 = g_npc_records[npc_id].gold;
    npc->unknown_1d = 1;
    npc->unknown_ca = g_npc_records[npc_id].value_002;

    for (index = 0; index < g_npc_states->count; ++index) {
        released = *g_npc_states->GetAt(index);
        if (released != 0 && released->unknown_c7 != 0) {
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
    character->unknown_0b01 = 0;
    character->enchantment_top = 0;
    character->unknown_007d = -1;
    character->personality_0081 = -1;
    for (index = 0; index < 12; ++index) {
        EmptyItemRecord(&character->equipment[index], 0, 1);
    }
    for (index = 0; index < 8; ++index) {
        EmptyItemRecord(&character->backpack[index], 0, 1);
    }
    wcscpy(character->name, source->name);
    wcscpy(character->name_part_2, source->name_part_2);
    character->current_profession = source->profession;
    character->original_profession = source->profession;
    character->profession_levels[source->profession] = source->level;
    character->race = source->race;
    character->gender = static_cast<W8Gender>(source->gender);
    character->table_value_0079 = source->table_value;
    for (index = 0; index < 7; ++index) {
        character->attributes[index].value = source->attributes[index];
    }
    for (index = 0; index < 0x29; ++index) {
        character->skills[index].value_02 = source->skills[index];
    }
    for (index = 0; index < 12; ++index) {
        if (source->equipment_present[index] != 0 && source->equipment_ids[index] != 0xffff) {
            ReplaceOrCreateItem(&item, (short)source->equipment_ids[index], 1, 1, 0);
            character->equipment[index] = item;
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
    character->hp_current = character->hp_max;
    character->stamina = character->stamina_max;
    for (index = 0; index < W8_SPELL_REALM_COUNT; ++index) {
        character->sp_left[index] = character->sp_max[index];
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
    int handle;

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
    handle = npc->unknown_00;
    npc->has_monster = 0;
    record = npc->record;
    npc->unknown_00 = 0;
    flag = record->unknown_054;
    Function55A0A0(handle);
    if (flag != 0) {
        npc->unknown_c7 = 1;
    }
}

/* Hand back the NPC binding selected by a monster-list index, or null when
   the monster carries no matching enchantment mark or the binding is not
   released. */
// FUNCTION: WIZ8 0x0050A440
W8NpcState* FindNpcBindingForMonster(unsigned int monster_list_index)
{
    W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
    W8MonsterRecord* record = GetMonsterDataForInfo(monster_info);
    const unsigned char* enchant;
    W8NpcState* npc;

    if (record == 0) {
        return 0;
    }
    /* The fifth and third bytes of enchantment slot three; the slot itself is
       three dwords, as the effect-slot clearing shows. */
    enchant = (const unsigned char*)&monster_info->enchantments[3];
    if ((enchant[5] & 1) == 0) {
        return 0;
    }
    if (enchant[2] != 0xfa) {
        return 0;
    }
    if (monster_info->runtime_value_2f1 >= g_npc_states->count) {
        npc = g_npc_states->data[0];
    } else {
        npc = g_npc_states->data[monster_info->runtime_value_2f1];
    }
    if (npc->unknown_c7 != 0) {
        return npc;
    }
    return 0;
}

// GLOBAL: WIZ8 0x005EC29C
const float g_float_005ec29c = 0.7853981256484985f;

/* Probe the navigator from the party eye at three height bands, reporting
   whether any band reaches. */
// FUNCTION: WIZ8 0x0050B2F0
unsigned char UpdateNpcAt(W8NpcState* /*npc*/, int /*arg_2*/, srVector3T<float>* scratch)
{
    srVector3T<float> party_position;
    float yaw;

    GetCameraPosition(&party_position);
    party_position.y = party_position.y - g_default_world_height_00603ac8;
    yaw = GetCameraYawRadians() + g_float_005ec29c;
    if (FindNavigatorPosition00437F30(&party_position, yaw, 1000.0f, 1, scratch, 1, 0, 1, 10, 0) >
        0) {
        return 1;
    }
    if (FindNavigatorPosition00437F30(&party_position, yaw, 1000.0f, 1, scratch, 1, 0, 1, 20, 0) >
        0) {
        return 1;
    }
    FindNavigatorPosition00437F30(&party_position, yaw, 1000.0f, 1, scratch, 1, 0, 1, 30, 0);
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
        Function56C5E0(npc, 0, -1, 0, 0);
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

    if (g_status_685170.flag_49bb != 0 &&
        (unsigned int)(g_status_685170.world_clock - g_status_685170.value_49b7) > 0x2a30) {
        if (GetFact(0x3c) != 0 && Random(100) < 6) {
            SetFact(0x2f1, 1, 0);
        }
        g_status_685170.flag_49bb = 0;
    }
    if (g_status_685170.value_4973 != 0 &&
        (unsigned int)(GetTickCount() - g_status_685170.value_4973) > 0x32) {
        group = FindFirstMonsterByID(0x1b3);
        if (group != 0) {
            index = MonsterGetIndexByLocationID(0xc17, NPC_MANAGER_CPP, group->value_9f, 1);
            monster_info = MonsterGetScriptPartByLocationIndex(index);
            MonsterStartsDying(monster_info, 1);
        }
        g_status_685170.value_4973 = 0;
        g_status_685170.value_4977 = GetTickCount();
    }
    if (g_status_685170.value_4977 != 0 &&
        (unsigned int)(GetTickCount() - g_status_685170.value_4977) > 0x1388) {
        group = FindFirstMonsterByID(0x1b6);
        if (group != 0) {
            index = MonsterGetIndexByLocationID(0xc2f, NPC_MANAGER_CPP, group->value_9f, 1);
            monster_info = MonsterGetScriptPartByLocationIndex(index);
            StartMonsterCycle(monster_info, 0x10, 1);
            monster_info->monster->SetCycleCallback004CA340(0x10, TriggerBelaVoice0050D480);
        }
        g_status_685170.value_4977 = 0;
    }

    if (g_status_685170.flag_2430 != 0) {
        W8NpcState* partner = 0;

        for (int slot = 0; slot < g_npc_states->GetCount(); ++slot) {
            npc = *g_npc_states->GetAt(slot);
            if (npc->unknown_c7 != 0 || npc->flag_ea == 0) {
                g_status_685170.flag_2430 = 0;
                continue;
            }
            unsigned int kind = (unsigned char)npc->name_style;

            partner = 0;
            for (int search = 0; search < g_npc_states->GetCount(); ++search) {
                W8NpcState* candidate = *g_npc_states->GetAt(search);
                if ((unsigned int)candidate->record->kind == kind) {
                    partner = candidate;
                    break;
                }
            }
            if (!partner->has_monster) {
                g_status_685170.flag_2430 = 0;
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
                Function55A0A0(released->unknown_00);
                released->unknown_00 = 0;
                if (released->record->unknown_054 != 0) {
                    released->unknown_c7 = 1;
                }
            }
            g_status_685170.flag_2430 = 0;
        }
    }

    if (gXStatus.fCombatMode == 0 && g_status_685170.value_498b > 1) {
        bool run_event = GetFact(0x216) != 0;

        if (!run_event) {
            for (int search = 0; search < g_npc_states->GetCount(); ++search) {
                W8NpcState* candidate = *g_npc_states->GetAt(search);
                if (candidate->record->kind == 99) {
                    if (candidate != 0 && *(unsigned char*)&candidate->unknown_04 != 0) {
                        run_event = true;
                    }
                    break;
                }
            }
        }
        if (run_event) {
            g_status_685170.value_498b = 0;
            for (int search = 0; search < g_npc_states->GetCount(); ++search) {
                W8NpcState* candidate = *g_npc_states->GetAt(search);
                if (candidate->record->kind == 0x8d) {
                    if (candidate != 0) {
                        Function56CA60(candidate, 0, 0, 0, 0);
                    }
                    break;
                }
            }
            Function5289B0(0x42, 0);
        }
    }

    if (gXStatus.fSurprisePossible == 0) {
        for (int slot = 0; slot < 2; ++slot) {
            W8PartySlotRow* row = &g_status_685170.buffers.party_rows[slot];
            W8Character* character = &g_status_685170.buffers.characters[slot];

            if (row->occupied != 0 && character->hp_current != 0) {
                W8NpcState* npc_state = 0;
                if (g_npc_states != 0) {
                    npc_state = *g_npc_states->GetAt(row->animation_0fa);
                    if (npc_state != 0 && npc_state->unknown_c7 != 0) {
                        npc_state = 0;
                    }
                }
                if (row->flag_fe != 0 &&
                    (g_status_685170.world_clock - npc_state->event_clock_eb) > 0x168) {
                    if (Random(2) == 0) {
                        npc_state->event_clock_eb = g_status_685170.world_clock + Random(6) * 0x3c;
                    } else {
                        int event = Random(2) == 0 ? 0x57 : 0x58;
                        Function52E690(character, event, 0, g_effect_argument_005ed8c8,
                                       g_effect_argument_005ed914);
                        npc_state->event_clock_eb = g_status_685170.world_clock;
                    }
                }
            }
        }
    }

    if (g_status_685170.flag_248a != 0 &&
        (unsigned int)(g_status_685170.world_clock - g_status_685170.value_2493) > 0x2a300) {
        g_status_685170.flag_248a = 0;
        SetFact(0xb8, 1, 0);
    }
    if (g_status_685170.flag_2497 != 0 &&
        (g_status_685170.world_clock - g_status_685170.value_242a) > 0x3c) {
        g_status_685170.flag_2497 = 0;
        Function509560();
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
    g_status_685170.value_242a = g_status_685170.world_clock;
    g_status_685170.flag_242e = 1;
    for (int party_slot = 0; party_slot < 2; ++party_slot) {
        W8PartySlotRow* row = &g_status_685170.buffers.party_rows[party_slot];
        W8Character* character = &g_status_685170.buffers.characters[party_slot];

        if (row->occupied != 0 && character->hp_current != 0) {
            GetNpcState(row->animation_0fa)->flag_e8 = 0;
            row->flag_fe = 0;
            Function50E650(party_slot);
        }
    }
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

            if (npc->flag_c5 != 0 && npc->unknown_c7 == 0 &&
                npc->flag_c6 == g_status_685170.current_level) {
                if (Function50C560(npc, npc->unknown_9d) != 0) {
                    npc->flag_c5 = 0;
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
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
    /* Retail compiled this comparison with VC6's mixed-sign operands; the
   signedness is part of the recovered body and changing it would change
   the compare and branch. Suppress only this diagnostic here. */
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

        if (npc->flag_112 != 0 && npc->unknown_c7 == 0 &&
            npc->flag_113 == g_status_685170.current_level) {
            W8NpcState* companion = 0;
            bool found = false;

            for (unsigned int index = 0; index < count; ++index) {
                W8NpcState** candidate_slot = g_npc_states->data;

                if (index < count) {
                    candidate_slot += index;
                }
                companion = *candidate_slot;
                if (static_cast<unsigned int>(companion->record->kind) ==
                    static_cast<unsigned char>(npc->name_style)) {
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
                    unsigned int partner_index = companion->partner_index_2c;

                    if (partner_index != 0xffffffff && static_cast<int>(partner_index) <= count) {
                        W8NpcState** target_slot = g_npc_states->data;

                        if (static_cast<int>(partner_index) < count) {
                            target_slot += partner_index;
                        }
                        W8NpcState* target = *target_slot;

                        target->has_monster = 0;
                        Function55A0A0(target->unknown_00);
                        target->unknown_00 = 0;
                        if (target->record->unknown_054 != 0) {
                            target->unknown_c7 = 1;
                        }
                    }
                }
            }
        }
        count = g_npc_states->count;
        ++npc_index;
    } while (npc_index < count);
#pragma clang diagnostic pop
}

/* Hand back the monster binding of every marked NPC, then find the companion
   whose record kind matches the NPC's naming style and release its live
   monster and its own binding. The state vector is re-read after every
   callback. */
// FUNCTION: WIZ8 0x0050da00
void ReleaseMarkedNpcBindings0050DA00(void)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
    /* Retail compiled this comparison with VC6's mixed-sign operands; the
   signedness is part of the recovered body and changing it would change
   the compare and branch. Suppress only this diagnostic here. */
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

        if (npc->unknown_c7 == 0) {
            if (npc->marked_e9 != 0) {
                Function50CF70(npc, 1);
            }
            if (npc->flag_c5 != 0) {
                W8NpcState* companion = 0;
                bool found = false;

                for (unsigned int index = 0; index < count; ++index) {
                    W8NpcState** candidate_slot = g_npc_states->data;

                    if (index < count) {
                        candidate_slot += index;
                    }
                    companion = *candidate_slot;
                    if (static_cast<unsigned int>(companion->record->kind) ==
                        static_cast<unsigned char>(npc->name_style)) {
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
                        unsigned int partner_index = companion->partner_index_2c;

                        if (partner_index != 0xffffffff &&
                            static_cast<int>(partner_index) <= count) {
                            W8NpcState** target_slot = g_npc_states->data;

                            if (static_cast<int>(partner_index) < count) {
                                target_slot += partner_index;
                            }
                            W8NpcState* target = *target_slot;

                            target->has_monster = 0;
                            Function55A0A0(target->unknown_00);
                            target->unknown_00 = 0;
                            if (target->record->unknown_054 != 0) {
                                target->unknown_c7 = 1;
                            }
                        }
                    }
                }
            }
        }
        count = g_npc_states->count;
        ++npc_index;
    } while (npc_index < count);
#pragma clang diagnostic pop
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

            if (npc->has_monster && (npc->record->unknown_056 != 0 ||
                                     (npc->record->flag_2ea != 0 && !npc->is_present))) {
                npc->has_monster = 0;
                Function55A0A0(npc->unknown_00);
                npc->unknown_00 = 0;
                if (npc->record->unknown_054 != 0) {
                    npc->unknown_c7 = 1;
                }
            }
            count = g_npc_states->count;
            ++npc_index;
        } while (npc_index < count);
    }

    count = g_npc_states->count;
    npc_index = 0;
    if (count != 0) {
        do {
            W8NpcState** slot = g_npc_states->data;
            char trigger_name[40];

            if (npc_index < count) {
                slot += npc_index;
            }
            W8NpcState* npc = *slot;

            if (npc->record->unknown_056 != 0 || npc->record->flag_2ea != 0) {
                sprintf(trigger_name, "_%S", npc->record->source_name_004);
                Trigger* trigger = FindTriggerByName(trigger_name);

                if (trigger != 0) {
                    trigger->activation_callback_360 = Function50ABF0;
                    trigger->m_lData1 = static_cast<int>(npc_index);
                    npc->has_monster = 1;
                    npc->value_24 =
                        static_cast<unsigned char>(Function42B740(g_status_685170.current_level));
                    npc->value_2f = static_cast<unsigned char>(g_status_685170.current_level);
                    Function524CA0(npc);
                    npc->is_present = 0;
                }
            }
            count = g_npc_states->count;
            ++npc_index;
        } while (npc_index < count);
    }
}
