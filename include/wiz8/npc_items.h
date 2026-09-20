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
void MatureNpcDelayedItems0055BB10(W8NpcState* npc);
void RestockNpcInventory(W8NpcState* npc); /* 0x0055BCC0 */
W8NpcItemEntry* GetNpcItemAt(W8NpcState* npc, int index);
unsigned int GetNpcItemCount(W8NpcState* npc);
bool NpcAcceptsTradeItem(W8NpcState* npc, W8ItemInstance* item);
int CalculateNpcTradeStackPrice(W8NpcState* npc, int item_id, int mode, unsigned char stack_count,
                                unsigned char identified);
unsigned char ConsumeNpcItemQuantity(W8NpcState* npc, int index, unsigned char quantity);
/* 0x0055B7E0: the stock mutation a completed trade applies for one slot. */
unsigned char Function55B7E0(W8NpcState* npc, int index, unsigned char quantity, char mode,
                             int* moved_out);
void DecayNpcInventory(W8NpcState* npc);

#endif
