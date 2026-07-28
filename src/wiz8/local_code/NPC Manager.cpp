#include "wiz8/gameplay_boundaries.h"
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

#pragma pack(push, 1)

/* One NPC's runtime state. The database pointer sits unaligned at 0x06, which
   is what the byte-offset loads through it show, and everything the bodies
   here reach is placed off it. */
typedef struct W8NpcState {
    unsigned char unknown_00[6];
    W8NpcDatabaseRecord* record;          /* 0x06 */
    unsigned char unknown_0a[0xc];
    int location_id;                      /* 0x16 */
    unsigned char has_monster;            /* 0x1a */
    unsigned char unknown_1b[0xa];
    unsigned char is_present;             /* 0x25 */
    unsigned char is_grouped;             /* 0x26 */
    unsigned char unknown_27[4];
    signed char group_index;              /* 0x2b */
    unsigned char unknown_2c[0x5d];
    /* 0x089: five slots holding the topics the NPC will talk about, stored
       one more than the topic they name so that zero can mean empty. */
    int topics[5];
    unsigned char unknown_9d[0x55];
    /* 0x0f2: the fourteen facts the NPC has been told, appended in order and
       terminated by the first zero. */
    short known_facts[14];
    unsigned char unknown_10e[0x1c];
} W8NpcState;                             /* 0x12a partitioned */

#pragma pack(pop)

enum { W8_NPC_TOPIC_SLOTS = 5, W8_NPC_FACT_SLOTS = 14 };

/* The NPC kind that will not trade at all. */
enum { W8_NPC_KIND_NO_TRADE = 0x14 };

/* The one item a trader always accepts regardless of what it is worth, and the
   value everything else has to clear. */
enum { W8_NPC_ALWAYS_TRADED_ITEM = 0x29f, W8_NPC_MINIMUM_TRADE_VALUE = 300 };

/* The two thresholds the disposition band is cut at. */
enum { W8_NPC_DISPOSITION_HOSTILE = 0x21, W8_NPC_DISPOSITION_FRIENDLY = 0x42 };

/* 0x00689F94: every NPC state, held in the shared growable vector. */
extern W8GrowableVector<W8NpcState*>* g_npc_states;
/* 0x006836B8: the monster manager's eight entries, 0x118 bytes each. */
extern unsigned char g_monster_manager_entries[];

extern char GetNpcDisposition(W8NpcState* npc);                          /* 0x0050A280 */
extern unsigned int GetItemStackValue(const W8ItemInstance* item);       /* 0x0051B840 */
extern void UpdateNpcAt(W8NpcState* npc, int arg_2, void* scratch);      /* 0x0050B2F0 */

/* Whether the NPC's database entry carries the value at 0x002 at all. */
// FUNCTION: WIZ8 0x0050AA00
bool NpcRecordHasValue002(W8NpcState* npc)
{
    return npc->record->value_002 != 0;
}

/* Which of the three disposition bands the NPC falls in. The bands are cut at
   0x21 and 0x42, and the hostile band answers two rather than zero. */
// FUNCTION: WIZ8 0x0050A500
unsigned char GetNpcDispositionBand(W8NpcState* npc)
{
    char disposition = GetNpcDisposition(npc);

    if (disposition < W8_NPC_DISPOSITION_HOSTILE) {
        return 2;
    }
    return disposition < W8_NPC_DISPOSITION_FRIENDLY;
}

/* Run the update with an empty scratch block the caller does not see. */
// FUNCTION: WIZ8 0x0050B2D0
void UpdateNpc(W8NpcState* npc)
{
    unsigned char scratch[12];

    UpdateNpcAt(npc, 0, scratch);
}

/* Whether the NPC will talk about one topic. Topics are stored one more than
   they name, so zero can mean an empty slot. */
// FUNCTION: WIZ8 0x0050C190
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
// FUNCTION: WIZ8 0x0050DD50
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
// FUNCTION: WIZ8 0x0050DD10
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
// FUNCTION: WIZ8 0x0050B800
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
// FUNCTION: WIZ8 0x0050A3C0
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
// FUNCTION: WIZ8 0x0050B870
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
// FUNCTION: WIZ8 0x0050A9C0
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
// FUNCTION: WIZ8 0x0050B9B0
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
