#pragma once

#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/gameplay_databases.h"

bool LoadMonsterDatabase(W8MonsterRecord** records);
bool LoadMonsterDatabaseRange(unsigned int uiStartIndex, unsigned int uiEndIndex,
                              W8MonsterRecord* records);
bool LoadMonsterDatabaseRecord(unsigned int monster_species, W8MonsterRecord* record);

class W8GameTimer;
extern unsigned int g_starting_item_ids[];

bool InitializeItemDatabase(void);
bool InitializeItemTables(void);
void DestroyItemDatabase(void);
void DestroyItemTables(void);
void FreeIfNotNull(void* block);
