#include "wiz8/level_specific_code/SeaCaves.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/NPCManager.h"
#include "surrender/srMath.h"

#define SEACAVES_CPP "C:\\Projects\\Wizardry 8\\Level Specific Code\\SeaCaves.cpp"

/* Level Specific Code\SeaCaves.cpp (level 0x16).

   Attribution evidence: 0x004DAF70 passes this file's path string to
   GetMonsterGroupIndexByID. The level-0x16 block in
   InitializeLevelMasterFunctions004D6C50 registers the surrounding cluster
   (HigardiChest01-05, doortomb). */

/* "HigardiChest01": spawn one spirit monster (0x1f2, hostile) at the
   NP_Spirit01 entity. */
// FUNCTION: WIZ8 0x004DAE30
bool SeaCavesHigardiChest01004DAE30(Trigger* pTrigger)
{
    srVector3T<float> position;

    if (FindEntityByName("NP_Spirit01", &position, 0, 0)) {
        SpawnMonsters(0x1f2, 1, &position, 1, 0, 0, 0);
    }
    return true;
}

/* "HigardiChest02": spawn one spirit monster at NP_Spirit02. */
// FUNCTION: WIZ8 0x004DAE70
bool SeaCavesHigardiChest02004DAE70(Trigger* pTrigger)
{
    srVector3T<float> position;

    if (FindEntityByName("NP_Spirit02", &position, 0, 0)) {
        SpawnMonsters(0x1f2, 1, &position, 1, 0, 0, 0);
    }
    return true;
}

/* "HigardiChest03": spawn one spirit monster at NP_Spirit03. */
// FUNCTION: WIZ8 0x004DAEB0
bool SeaCavesHigardiChest03004DAEB0(Trigger* pTrigger)
{
    srVector3T<float> position;

    if (FindEntityByName("NP_Spirit03", &position, 0, 0)) {
        SpawnMonsters(0x1f2, 1, &position, 1, 0, 0, 0);
    }
    return true;
}

/* "HigardiChest04": spawn one spirit monster at NP_Spirit04. */
// FUNCTION: WIZ8 0x004DAEF0
bool SeaCavesHigardiChest04004DAEF0(Trigger* pTrigger)
{
    srVector3T<float> position;

    if (FindEntityByName("NP_Spirit04", &position, 0, 0)) {
        SpawnMonsters(0x1f2, 1, &position, 1, 0, 0, 0);
    }
    return true;
}

/* "HigardiChest05": spawn one spirit monster at NP_Spirit05. */
// FUNCTION: WIZ8 0x004DAF30
bool SeaCavesHigardiChest05004DAF30(Trigger* pTrigger)
{
    srVector3T<float> position;

    if (FindEntityByName("NP_Spirit05", &position, 0, 0)) {
        SpawnMonsters(0x1f2, 1, &position, 1, 0, 0, 0);
    }
    return true;
}

/* "doortomb": NPC kind 0x64's monster entry gets a fresh group member when
   its group still exists and the monster answers query 6 false. */
// FUNCTION: WIZ8 0x004DAF70
bool SeaCavesDoorTomb004DAF70(Trigger* pTrigger)
{
    W8NpcState* npc;
    W8MonsterInfo* monster_info;
    W8MonsterGroup* group;

    npc = GetNpcStateByKind(0x64);
    if (npc == 0) {
        return true;
    }
    monster_info = GetNpcMonsterInfo(npc);
    if (monster_info == 0) {
        return true;
    }
    if (monster_info->monster == 0) {
        return true;
    }
    if (MonsterQuery(monster_info->monster, 6) != 0) {
        return true;
    }
    group = GetMonsterGroupByListIndex(
        GetMonsterGroupIndexByID(0x4b, SEACAVES_CPP, monster_info->monster_group_id, 1));
    if (group != 0) {
        GiveBirthToMonster(group);
    }
    return true;
}
