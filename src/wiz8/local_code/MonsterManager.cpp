#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

#include <new>
#include <stdlib.h>

#define MONSTER_MANAGER_CPP "C:\\Projects\\Wizardry 8\\Local Code\\MonsterManager.cpp"
#define MAX_MONSTERS_IN_DATABASE 1000

extern "C" char* String(const char* format, ...);
int Function4A87A0(const char* name);
unsigned char Function47B610(int index);

/* The global constructed at 0x006836b8 contains eight 0x118-byte records.
   Each record owns one instantiation of the polymorphic pointer-vector family
   at +0xd8, and the outer object owns the same vector at +0x9b7.  The element
   type and the vector's original template name are not yet proven, so those
   names deliberately remain address-neutral. */
#pragma pack(push, 1)
class W8MonsterManagerPtrVector {
public:
    __forceinline W8MonsterManagerPtrVector()
    {
        data = static_cast<void**>(::operator new(5 * sizeof(void*)));
        count = 0;
        if (data != 0) {
            capacity = 5;
        }
        else {
            capacity = 0;
        }
    }

    virtual __forceinline ~W8MonsterManagerPtrVector()
    {
        ::operator delete(data);
    }

    int count;                            /* 0x04 */
    int capacity;                         /* 0x08 */
    void** data;                          /* 0x0c */
};                                       /* 0x10 */

class W8MonsterManagerEntry {
public:
    W8MonsterManagerEntry();
    ~W8MonsterManagerEntry();

private:
    unsigned char unknown_000[0xd8];
    W8MonsterManagerPtrVector vector_d8;  /* 0x0d8 */
    unsigned char unknown_0e8[0x30];
};                                       /* 0x118 */

class W8MonsterManagerState {
public:
    W8MonsterManagerState();
    ~W8MonsterManagerState();

private:
    W8MonsterManagerEntry entries[8];     /* 0x000 .. 0x8c0 */
    unsigned char unknown_8c0[0xf7];
    W8MonsterManagerPtrVector vector_9b7; /* 0x9b7 */
};                                       /* 0x9c7 */
#pragma pack(pop)

typedef char W8MonsterManagerPtrVector_size_must_be_0x10[
    sizeof(W8MonsterManagerPtrVector) == 0x10 ? 1 : -1];
typedef char W8MonsterManagerEntry_size_must_be_0x118[
    sizeof(W8MonsterManagerEntry) == 0x118 ? 1 : -1];
typedef char W8MonsterManagerState_size_must_be_0x9c7[
    sizeof(W8MonsterManagerState) == 0x9c7 ? 1 : -1];

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
            MONSTER_MANAGER_CPP,
            0x5c1,
            String(
                "MonsterIndex: ID %d not found (%s line %d)",
                location_id,
                caller_file,
                caller_line));
    }
    return 0xffffffff;
}

// FUNCTION: WIZ8 0x004E5620
W8MonsterInfo* MonsterGetScriptPartByLocationIndex(unsigned int monster_list_index)
{
    W8MonsterInfo* result;

    if (monster_list_index < 10000 || monster_list_index >= 20000) {
        if (PListGetCount(g_monster_list) <= monster_list_index) {
            srAssertFail(
                "uiMonsterListIndex < (UINT32) PLLength(gXStatus.plsMonsterList)",
                MONSTER_MANAGER_CPP,
                0x5da,
                0);
        }
        result = (W8MonsterInfo*)PListGetAt(g_monster_list, monster_list_index);
        if (result != 0) {
            return result;
        }
        srAssertFail(
            "pMonsterInfo != NULL",
            MONSTER_MANAGER_CPP,
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
                MONSTER_MANAGER_CPP,
                0x5d1,
                0);
        }
        result = (W8MonsterInfo*)PListGetAt(g_unborn_monster_list, index);
        if (result != 0) {
            return result;
        }
        srAssertFail(
            "pMonsterInfo != NULL",
            MONSTER_MANAGER_CPP,
            0x5d5,
            String(
                "MonsterInfo: ERROR - PLGet failed, index %d, pList %d",
                monster_list_index,
                g_monster_list));
    }
    return 0;
}

static __inline W8MonsterRecord* GetMonsterDataByIDInline(unsigned int monster_species)
{
    W8MonsterRecord* record;

    if (monster_species >= MAX_MONSTERS_IN_DATABASE) {
        srAssertFail(
            "uiMonsterSpecies < MAX_MONSTERS_IN_DATABASE",
            MONSTER_MANAGER_CPP,
            0x5f3,
            0);
    }
    record = g_monster_record_cache[monster_species];
    if (record == 0) {
        record = (W8MonsterRecord*)malloc(sizeof(W8MonsterRecord));
        if (record == 0) {
            return 0;
        }
        if (!LoadMonsterDatabaseRecord(monster_species, record)) {
            free(record);
            return 0;
        }
        g_monster_record_cache[monster_species] = record;
    }
    return record;
}

// FUNCTION: WIZ8 0x004E57C0
W8MonsterRecord* GetMonsterDataByID(unsigned int monster_species)
{
    return GetMonsterDataByIDInline(monster_species);
}

static __inline W8MonsterInfo* MonsterInfoFromIDInline(
    int caller_line,
    const char* caller_file,
    int location_id,
    unsigned char assert_on_failure)
{
    W8MonsterInfo* monster = 0;
    unsigned int index;

    index = MonsterGetIndexByLocationID(0x61d, MONSTER_MANAGER_CPP, location_id, 0);
    if (index != 0xffffffff) {
        monster = MonsterGetScriptPartByLocationIndex(index);
    }
    if (monster == 0 && assert_on_failure != 0) {
        srAssertFail(
            "FALSE",
            MONSTER_MANAGER_CPP,
            0x626,
            String(
                "MonsterInfoFromID: ID %d not found (%s line %d)",
                location_id,
                caller_file,
                caller_line));
    }
    return monster;
}

// FUNCTION: WIZ8 0x004E5840
W8MonsterInfo* MonsterInfoFromID(
    int caller_line,
    const char* caller_file,
    int location_id,
    unsigned char assert_on_failure)
{
    return MonsterInfoFromIDInline(
        caller_line,
        caller_file,
        location_id,
        assert_on_failure);
}

// FUNCTION: WIZ8 0x004E58B0
W8MonsterRecord* GetMonsterDataByLocationID(int location_id)
{
    W8MonsterInfo* monster;
    unsigned int index;

    index = MonsterGetIndexByLocationID(0x632, MONSTER_MANAGER_CPP, location_id, 1);
    monster = MonsterGetScriptPartByLocationIndex(index);
    if (monster == 0) {
        return 0;
    }
    return GetMonsterDataByIDInline(monster->monster_species);
}

// FUNCTION: WIZ8 0x004E5950
W8Monster* GetMonsterByLocationID(int location_id)
{
    W8MonsterInfo* monster_info;
    unsigned int index;

    if (location_id != -1) {
        index = MonsterGetIndexByLocationID(0x64f, MONSTER_MANAGER_CPP, location_id, 0);
        if (index != 0xffffffff) {
            monster_info = MonsterGetScriptPartByLocationIndex(index);
            if (monster_info != 0) {
                return monster_info->monster;
            }
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x004E5990
float GetMonsterRecordScaledFloat1BA(W8MonsterInfo* monster_info)
{
    W8MonsterRecord* record;
    float result;

    if (monster_info == 0) {
        srAssertFail(
            "pMonsterInfo != NULL",
            MONSTER_MANAGER_CPP,
            0x5e9,
            0);
    }
    record = GetMonsterDataByIDInline(monster_info->monster_species);
    if (record == 0) {
        srAssertFail(
            "pMonsterDB",
            MONSTER_MANAGER_CPP,
            0x66a,
            0);
    }
    result = record->float_1ba * g_monster_record_float_scale;
    return result;
}

// FUNCTION: WIZ8 0x004E5AA0
W8MonsterInfo* GetNextMonsterInfo(unsigned char reset_iterator)
{
    W8MonsterInfo* result = 0;
    int index;

    if (reset_iterator != 0) {
        g_monster_info_iterator_index = 0;
    }
    if (g_monster_info_iterator_index < (int)PListGetCount(g_monster_list)) {
        index = g_monster_info_iterator_index++;
        result = (W8MonsterInfo*)PListGetAt(g_monster_list, index);
    }
    return result;
}

// FUNCTION: WIZ8 0x004E5B50
int Function4E5B50(unsigned int monster_species)
{
    W8MonsterRecord* record;

    record = GetMonsterDataByIDInline(monster_species);
    if (record == 0) {
        return -1;
    }
    if (Function4A87A0(record->cycle_name_189) != 0) {
        return 0;
    }
    if (Function47B610(0xe) != 0) {
        return record->value_257;
    }
    return record->value_253;
}

// FUNCTION: WIZ8 0x004E5E50
W8MonsterInfo* FindMonsterInfoBySpecies(unsigned int monster_species)
{
    unsigned int index;
    W8MonsterInfo* monster_info;

    for (index = 0; index < PListGetCount(g_monster_list); ++index) {
        monster_info = MonsterGetScriptPartByLocationIndex(index);
        if (monster_info->monster_species == monster_species) {
            return monster_info;
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x004E6780
unsigned int GetMonsterCombatValue(const W8MonsterRecord* record)
{
    unsigned int value = record->combat_value_override_26b;

    if (value <= 0) {
        value = record->combat_value_181;
    }
    return value;
}

// FUNCTION: WIZ8 0x004E68C0
unsigned char Function4E68C0(void)
{
    unsigned int index;
    W8MonsterInfo* monster_info;

    for (index = 0; index < PListGetCount(g_monster_list); ++index) {
        monster_info = MonsterGetScriptPartByLocationIndex(index);
        if (monster_info != 0 && monster_info->monster->Function4CA4C0() != 0) {
            return 1;
        }
    }
    return 0;
}

static W8MonsterManagerState g_monster_manager_state;

// FUNCTION: WIZ8 0x004E6940
W8MonsterManagerState::~W8MonsterManagerState()
{
}

// FUNCTION: WIZ8 0x004E6970
W8MonsterManagerState::W8MonsterManagerState()
{
}

// FUNCTION: WIZ8 0x004E6A10
W8MonsterManagerEntry::~W8MonsterManagerEntry()
{
}

// FUNCTION: WIZ8 0x004E6A30
W8MonsterManagerEntry::W8MonsterManagerEntry()
{
}
