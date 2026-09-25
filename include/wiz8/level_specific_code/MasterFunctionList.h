#pragma once

extern bool g_flag_6109f0;
extern bool g_flag_652da5;

#include "wiz8/vector.h"

struct W8MonsterInfo;
struct W8MonsterGroup;
class stSound3D;
template <class T> class srVector3T;

/* One level-specific per-frame entry point, registered by the level's own
   master function and run every world update. */
typedef void (*W8MasterFunction)(int);

/* The registered master functions, and the flag a callback sets to ask the
   dispatcher to drop it after this run. */
extern W8Vector<W8MasterFunction>* g_master_functions;
extern bool g_flag_006834dc;
extern bool g_flag_006834dd;

void ClearValue6834D4(void);
int NormalizeMasterFunctionValue(int value);
/* Run every registered master function once with argument zero, dropping the
   ones that set the removal flag while it runs. */
void RunMasterFunctions(void);
/* Run every registered master function once with argument -1, the persist
   command the level masters answer by writing their live state into the
   location variables. */
void SaveMasterFunctions(void);
/* Dispatch one world-cursor-node command against the nodes covering `info`'s
   ground-settled position (the camera's when info is null). */
unsigned char DispatchWorldCursorNodeCommand004D9080(W8MonsterInfo* info, int command, ...);
/* Format the current level's message-database path, fetch the indexed string
   and show it. Answers whether the string existed. */
unsigned char ShowLevelMessage(int message_id);

void InitializeLevelMasterFunctions(int level);
W8MonsterGroup* SpawnMonsters(int monster_id, int count, srVector3T<float>* position, int hostility,
                              unsigned char settle, unsigned char a,
                              unsigned char b); /* 0x004D8F00 */
stSound3D* CreateAndPlaySoundNode(char* sound_name, srVector3T<float> position, float volume,
                                  float scale, unsigned char play_flag); /* 0x004D8F80 */
