#pragma once

#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/gameplay_databases.h"

unsigned char LoadMonsterDatabase(W8MonsterRecord** records);
unsigned char LoadMonsterDatabaseRange(unsigned int uiStartIndex, unsigned int uiEndIndex,
                                       unsigned int unused, W8MonsterRecord* records);
unsigned char LoadMonsterDatabaseRecord(unsigned int monster_species, W8MonsterRecord* record);

extern unsigned char g_status_block_685078[56];
class W8GameTimer;
extern W8GameTimer* g_gameplay_timer_685067;
extern unsigned char g_party_moving_006850b5;
extern unsigned int g_starting_item_ids[];

unsigned char InitializeItemDatabase(void);
unsigned char InitializeItemTables(void);
void DestroyItemDatabase(void);
void DestroyItemTables(void);
void FreeIfNotNull(void* block);

void QueueGameplayEvent(int event_type, int party_slot);
