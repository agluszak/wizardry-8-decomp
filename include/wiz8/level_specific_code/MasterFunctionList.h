#pragma once

#include "wiz8/vector.h"

struct W8MonsterInfo;

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

void Function4D6C50(int level);
unsigned char Function4D9080(W8MonsterInfo* monster_info, int arg_2, int arg_3);

