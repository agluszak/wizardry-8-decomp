#pragma once

extern unsigned char g_flag_6109f0;

void Function4DFAE0(int value); /* 0x004DFAE0 */
void Function4DFB40(int value); /* 0x004DFB40 */
void Function4DFB80(int value); /* 0x004DFB80 */
void Function4E0430(void);      /* 0x004E0430 */

#include "wiz8/vector.h"

struct W8MonsterInfo;
struct W8MonsterGroup;
template <class T> class srVector3T;

/* One level-specific per-frame entry point, registered by the level's own
   master function and run every world update. */
typedef void (*W8MasterFunction)(int);

/* The registered master functions, and the flag a callback sets to ask the
   dispatcher to drop it after this run. */
extern W8GrowableVector<W8MasterFunction>* g_master_functions_006834d8;
extern unsigned char g_flag_006834dc;

void ClearValue6834D4(void);
int NormalizeMasterFunctionValue004D9700(int value);
/* Run every registered master function once with argument zero, dropping the
   ones that set the removal flag while it runs. */
void RunMasterFunctions004D8E40(void);

void InitializeLevelMasterFunctions004D6C50(int level);
unsigned char Function4D9080(W8MonsterInfo* monster_info, int arg_2, int arg_3);
W8MonsterGroup* SpawnMonsters(int monster_id, int count, srVector3T<float>* position, int hostility,
                              int settle, int a, int b); /* 0x004D8F00 */
