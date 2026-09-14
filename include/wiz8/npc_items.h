#ifndef WIZ8_NPC_ITEMS_H
#define WIZ8_NPC_ITEMS_H

#include "wiz8/layouts/npc_state.h"

/* NPC stock and trade interface. The implementing source, src/wiz8/
   npc_items.cpp, sits in an unresolved-fragment gap, so original-TU ownership
   of these declarations is provisional until the fragment is attributed. */

int AddNpcItem(W8NpcState* npc, int item_id, unsigned int quantity);
int AddNpcItemFromInstance(W8NpcState* npc, const W8ItemInstance* item, char quantity);
int AddNpcItemWithDelay(W8NpcState* npc, int item_id, unsigned int quantity, int delay);
char RateItemIdentifyDifficulty(W8NpcState* npc, int item_id);
int RestockNpcItems(W8NpcState* npc);
void ClearNpcItems(W8NpcState* npc);
void SortNpcItems(W8NpcState* npc);
unsigned char PopulateNpcStock(W8NpcState* npc);
unsigned char MaintainNpcStock(W8NpcState* npc, char force);
W8NpcItemEntry* GetNpcItemAt(W8NpcState* npc, int index);
unsigned int GetNpcItemCount(W8NpcState* npc);
unsigned char ConsumeNpcItemQuantity(W8NpcState* npc, int index, unsigned char quantity);
void DecayNpcInventory(W8NpcState* npc);

#endif
