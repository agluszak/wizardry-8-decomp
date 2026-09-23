#pragma once

#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/gameplay_databases.h"

unsigned char LoadMonsterDatabase(W8MonsterRecord** records);
unsigned char LoadMonsterDatabaseRange(unsigned int uiStartIndex, unsigned int uiEndIndex,
                                       unsigned int unused, W8MonsterRecord* records);
unsigned char LoadMonsterDatabaseRecord(unsigned int monster_species, W8MonsterRecord* record);

class W8GameTimer;
extern unsigned int g_starting_item_ids[];

unsigned char InitializeItemDatabase(void);
unsigned char InitializeItemTables(void);
void DestroyItemDatabase(void);
void DestroyItemTables(void);
void FreeIfNotNull(void* block);
