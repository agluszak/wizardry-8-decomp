#include "gameplay_boundaries.h"
#include "sr_api.h"

extern char* String(const char* format, ...);

// FUNCTION: WIZ8 0x004E5620
W8MonsterInfo* MonsterGetScriptPartByLocationIndex(unsigned int monster_list_index)
{
    W8MonsterInfo* result;

    if (monster_list_index < 10000 || monster_list_index >= 20000) {
        if (PListGetCount(g_monster_list) <= monster_list_index) {
            srAssertFail(
                "uiMonsterListIndex < (UINT32) PLLength(gXStatus.plsMonsterList)",
                "C:\\Projects\\Wizardry 8\\Local Code\\MonsterManager.cpp",
                0x5da,
                0);
        }
        result = PListGetAt(g_monster_list, monster_list_index);
        if (result != 0) {
            return result;
        }
        srAssertFail(
            "pMonsterInfo != NULL",
            "C:\\Projects\\Wizardry 8\\Local Code\\MonsterManager.cpp",
            0x5de,
            String(
                "MonsterInfo: ERROR - PLGet failed, index %d, pList %d",
                monster_list_index,
                g_monster_list));
    }
    else {
        unsigned int index = monster_list_index - 10000;
        if (PListGetCount(g_unborn_monster_list) <= index) {
            srAssertFail(
                "(uiMonsterListIndex-10000) < (UINT32) PLLength(gXStatus.plsUnbornMonsterList)",
                "C:\\Projects\\Wizardry 8\\Local Code\\MonsterManager.cpp",
                0x5d1,
                0);
        }
        result = PListGetAt(g_unborn_monster_list, index);
        if (result != 0) {
            return result;
        }
        srAssertFail(
            "pMonsterInfo != NULL",
            "C:\\Projects\\Wizardry 8\\Local Code\\MonsterManager.cpp",
            0x5d5,
            String(
                "MonsterInfo: ERROR - PLGet failed, index %d, pList %d",
                monster_list_index,
                g_monster_list));
    }
    return 0;
}

// FUNCTION: WIZ8 0x004E5550
unsigned int MonsterGetIndexByLocationID(
    int caller_line,
    const char* caller_file,
    int location_id,
    unsigned char assert_on_failure)
{
    unsigned int index;
    W8MonsterInfo* monster;

    for (index = 0; index < PListGetCount(g_monster_list); ++index) {
        monster = MonsterGetScriptPartByLocationIndex(index);
        if (monster->location_id == location_id) {
            return index;
        }
    }

    for (index = 0; index < PListGetCount(g_unborn_monster_list); ++index) {
        monster = (W8MonsterInfo*)PListGetAt(g_unborn_monster_list, index);
        if (monster->location_id == location_id) {
            return index + 10000;
        }
    }

    if (assert_on_failure != 0) {
        srAssertFail(
            "FALSE",
            "C:\\Projects\\Wizardry 8\\Local Code\\MonsterManager.cpp",
            0x5c1,
            String(
                "MonsterIndex: ID %d not found (%s line %d)",
                location_id,
                caller_file,
                caller_line));
    }
    return 0xffffffff;
}
