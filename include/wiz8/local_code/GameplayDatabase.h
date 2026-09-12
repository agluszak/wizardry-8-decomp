#pragma once

#include "wiz8/game_status.h"
#include "wiz8/layouts/gameplay_databases.h"

void ResetGameStatus(unsigned char release);
void InitializeGameplayRuntimeObjects(void);
unsigned char LoadMonsterDatabase(W8MonsterRecord** records);
unsigned char LoadMonsterDatabaseRange(unsigned int uiStartIndex, unsigned int uiEndIndex,
                                       unsigned int unused, W8MonsterRecord* records);
unsigned char InitializeSpellDatabase(void);

extern unsigned char g_status_block_685078[56];
class W8GameTimer;
extern W8GameTimer* g_gameplay_timer_685067;
extern unsigned char g_party_moving_006850b5;
extern unsigned int g_starting_item_ids[];

unsigned char InitializeFactDatabase(void);
unsigned char InitializeItemDatabase(void);
unsigned char InitializeLevelDatabase(void);
unsigned char InitializeItemTables(void);
unsigned char InitializeNpcDatabase(void);
void DestroyFactDatabase(void);
void DestroyItemDatabase(void);
void DestroyLevelDatabase(void);
void DestroyItemTables(void);
void DestroyNpcDatabase(void);
void FreeIfNotNull(void* block);
unsigned char AllocateStatusBuffers(W8StatusBuffers* status);
void FreeStatusBuffers(W8StatusBuffers* status);
void ResetPartySlotRow(int slot);
void ResetGameplayStatusBlock(void);
void DestroyGameplayObjects(void);
void ResetForNewGame(void);
void ResetGameplaySlot(unsigned int slot);
void ResetGameplaySettings(void);

void RunNewGameOpeningSequence(unsigned char notify, const wchar_t* target);
