#include "wiz8/gameplay_boundaries.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/npc_state.h"
#include "wiz8/sr_api.h"

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
extern W8GrowableVector<W8NpcState*>* g_npc_states;
/* 0x006836B8: the monster manager's eight entries, 0x118 bytes each. */
// GLOBAL: WIZ8 0x006836B8
extern unsigned char g_monster_manager_entries[];

extern char GetNpcDisposition(W8NpcState* npc);                          /* 0x0050A280 */
extern unsigned int GetItemStackValue(const W8ItemInstance* item);       /* 0x0051B840 */
extern void UpdateNpcAt(W8NpcState* npc, int arg_2, void* scratch);      /* 0x0050B2F0 */

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
    unsigned char scratch[12];

    UpdateNpcAt(npc, 0, scratch);
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

/* The NPC state at one index, skipping deleted database entries. Out-of-range
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
    if (npc->record->deleted != 0) {
        return 0;
    }
    return npc;
}

/* The monster standing in the world for this NPC, if one is. */
// FUNCTION: WIZ8 0x0050a3c0
W8MonsterInfo* GetNpcMonsterInfo(W8NpcState* npc)
{
    if (npc->has_monster == 0 || npc->is_present == 0) {
        return 0;
    }
    return MonsterGetScriptPartByLocationIndex(
        MonsterGetIndexByLocationID(673, NPC_MANAGER_CPP, npc->location_id, 1));
}

/* The monster manager entry this NPC's group occupies, if its database entry
   says it has a group and it is in one. */
// FUNCTION: WIZ8 0x0050b870
unsigned char* GetNpcGroupEntry(W8NpcState* npc)
{
    if (npc->record->has_group == 0) {
        return 0;
    }
    if (npc->is_grouped == 0) {
        return 0;
    }
    return g_monster_manager_entries + npc->group_index * 0x118;
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
    if (g_party_slot_rows[0].flag_00 != 0) {
        if (g_party_slot_rows[1].flag_00 != 0) {
            return 2;
        }
        if (g_party_slot_rows[0].flag_00 != 0) {
            return 1;
        }
    }
    if (g_party_slot_rows[1].flag_00 != 0) {
        return 1;
    }
    return 0;
}

#include <string.h>

/* 0x00619DFC: one three-dword row per service - the service id, the bit that
   stands for it, and one more field nothing here reads. -1 ends the table. */
typedef struct W8NpcServiceRow {
    unsigned int service_id;
    unsigned int bit;
    unsigned int unknown_08;
} W8NpcServiceRow;
extern const W8NpcServiceRow g_npc_services[];

/* 0x00619F18: the name a fact substitutes, and 0x00689F60 the buffer it is
   copied into so the caller always gets a writable one. */
extern const char g_substituted_npc_name[];
extern char g_npc_name_buffer[];

/* The name style that admits a substituted name. */
enum { W8_NPC_NAME_STYLE_SUBSTITUTABLE = ' ' };
/* The fact that makes the substitution happen. */
enum { W8_FACT_NPC_NAME_KNOWN = 0x44 };

extern unsigned char GetFact(int fact);

/* The engine object standing in the world for this NPC. */
// FUNCTION: WIZ8 0x0050a400
W8Monster* GetNpcMonster(W8NpcState* npc)
{
    W8MonsterInfo* monster_info;

    if (npc->has_monster == 0 || npc->is_present == 0) {
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
    }
    else {
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
