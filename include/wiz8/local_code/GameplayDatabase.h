#pragma once

#include "wiz8/game_state.h"
#include "wiz8/layouts/gameplay_databases.h"

extern "C" {

extern unsigned char g_save_flag_00687599;
extern unsigned char g_status_block_685078[56];
extern void* g_object_685067;
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
void Function54B100(void);
void Function54B300(unsigned int slot);
void Function54B560(void);

}
