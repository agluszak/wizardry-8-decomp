#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"
#include <math.h>
#include <new>
#include <stdlib.h>

#define MONSTER_MANAGER_CPP "C:\\Projects\\Wizardry 8\\Local Code\\MonsterManager.cpp"
#define MAX_MONSTERS_IN_DATABASE 1000

extern "C" char* String(const char* format, ...);
int Function4A87A0(const char* name);
unsigned char Function47B610(int index);
unsigned char MonsterSetAnimating(W8Monster* monster, unsigned char animating);
unsigned char MonsterIsCycleSupported(W8Monster* monster, int cycle);
void MonsterSetPendingCycle(W8Monster* monster, int cycle);
void Function4C5B10(W8Monster* monster, int value);
int MonsterQuery(W8Monster* monster, int query);
void Function4C6140(W8Monster* monster);
void MonsterSetCycle(W8Monster* monster, int cycle);
void MonsterSetBehaviour(W8Monster* monster, int behavior);
void MonsterSetSubCycle(W8Monster* monster, int subcycle);
unsigned char RemoveMonster(
    unsigned int monster_list_index,
    unsigned char destroy_monster);
void DeactivateMonster(W8MonsterInfo* monster_info);
void StartMonsterCycle(W8MonsterInfo* monster_info, int cycle, int behavior);
void MonsterDies(W8MonsterInfo* monster_info, int display_message);
void __fastcall Function452C90(W8MonsterMember18* member);
void __fastcall Function4537E0(W8MonsterMember18* member);
int GetMonsterName(W8MonsterInfo* monster_info, int monster_record, int name_form);
void Function5248D0(W8MonsterInfo* monster_info);
void Function58AB60(int value_1, int value_2, int value_3, int value_4);
void Function4C59C0(W8Monster* monster, W8World* world);
W8World* Function451280(W8Monster* monster);
void Function46E5A0(W8World* world);
void Function4C5860(W8Monster* monster);
void Function42E650(unsigned short location_id);
void Function509EA0(int value);
void MonsterGetScaleRange(W8Monster* monster, float* minimum, float* maximum);
float MonsterGetScale(W8Monster* monster);
void MonsterSetScale(W8Monster* monster, float scale);
void Function4C5810(W8Monster* monster);
void Function4C5ED0(W8Monster* monster);
int Function52A780(int first, int second);
extern unsigned char* g_object_68c09c;
extern unsigned char* g_object_6836a8;
extern unsigned char g_flag_683f94;
extern unsigned char g_flag_683f97;
extern volatile int g_dword_6598a4;

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

// FUNCTION: WIZ8 0x004E4600
void Function4E4600(W8MonsterInfo* monster_info)
{
    int result;

    if (monster_info == 0) {
        srAssertFail("pMonsterInfo != NULL", MONSTER_MANAGER_CPP, 0x2f7, 0);
    }
    Function4C5B10(monster_info->monster, 0);
    monster_info->monster->member_18.flags_0c &= 0xdfffffff;
    Function4C6140(monster_info->monster);
    if (monster_info->motionless == 0) {
        result = MonsterQuery(monster_info->monster, 6);
        if (result != 1 && result != 2 &&
            monster_info->monster->m_cycles[18].unknown_0c[0xa7] == -1) {
            StartMonsterCycle(monster_info, 1, 3);
        }
    }
}

// FUNCTION: WIZ8 0x004E4690
void MonsterStartsDying(W8MonsterInfo* monster_info, int display_message)
{
    if (monster_info->monster->IsDying() == 0) {
        StartMonsterCycle(monster_info, 0x15, 1);
        DeactivateMonster(monster_info);
        MonsterDies(monster_info, display_message);
        RemoveMonster(
            MonsterGetIndexByLocationID(
                0x31f,
                MONSTER_MANAGER_CPP,
                monster_info->location_id,
                1),
            0);
    }
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
    const char* detail;
    int line;

    if (monster_list_index < 10000 || monster_list_index >= 20000) {
        if (monster_list_index >= PListGetCount(g_monster_list)) {
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
        detail = String(
            "MonsterInfo: ERROR - PLGet failed, index %d, pList %d",
            monster_list_index,
            g_monster_list);
        line = 0x5de;
    }
    else {
        if (monster_list_index - 10000 >= PListGetCount(g_unborn_monster_list)) {
            srAssertFail(
                "(uiMonsterListIndex-10000) < (UINT32) PLLength(gXStatus.plsUnbornMonsterList)",
                MONSTER_MANAGER_CPP,
                0x5d1,
                0);
        }
        result = (W8MonsterInfo*)PListGetAt(g_unborn_monster_list, monster_list_index - 10000);
        if (result != 0) {
            return result;
        }
        detail = String(
            "MonsterInfo: ERROR - PLGet failed, index %d, pList %d",
            monster_list_index,
            g_monster_list);
        line = 0x5d5;
    }
    /* One tail, reached from both branches with only the line number differing.
       The original carries it as a variable and calls srAssertFail through a
       register it loads before the branch; writing the call out in each branch
       instead duplicates it. */
    srAssertFail("pMonsterInfo != NULL", MONSTER_MANAGER_CPP, line, detail);
    return 0;
}

static __inline W8MonsterRecord* MonsterDBFromSpeciesInline(unsigned int monster_species)
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

// FUNCTION: WIZ8 0x004E5720
W8MonsterRecord* GetMonsterDataForInfo(W8MonsterInfo* monster_info)
{
    if (monster_info == 0) {
        srAssertFail(
            "pMonsterInfo != NULL",
            MONSTER_MANAGER_CPP,
            0x5e9,
            0);
    }
    return MonsterDBFromSpeciesInline(monster_info->monster_species);
}

// FUNCTION: WIZ8 0x004E57C0
W8MonsterRecord* MonsterDBFromSpecies(unsigned int monster_species)
{
    return MonsterDBFromSpeciesInline(monster_species);
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
    return MonsterDBFromSpeciesInline(monster->monster_species);
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
    record = MonsterDBFromSpeciesInline(monster_info->monster_species);
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

// FUNCTION: WIZ8 0x004E5A50
void UpdateMonsterDamageAppearance(W8MonsterInfo* monster_info)
{
    W8Monster* monster = monster_info->monster;

    if (monster != 0) {
        int count = monster->Function4C6A50();
        if (count > 1) {
            int value = ((monster_info->hp_max - monster_info->hp_current) * count) /
                        monster_info->hp_max;
            if (value >= count - 1) {
                value = count - 1;
            }
            monster->Function4C6990(value);
        }
    }
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

// FUNCTION: WIZ8 0x004E5AF0
int GetMonsterQuadrant(W8MonsterInfo* monster_info)
{
    if (monster_info == 0) {
        srAssertFail("pMonsterInfo", MONSTER_MANAGER_CPP, 0x6b9, 0);
    }
    return GetQuadrantForPosition(monster_info->monster->member_18.GetPosition());
}

// FUNCTION: WIZ8 0x004E5B50
int Function4E5B50(unsigned int monster_species)
{
    W8MonsterRecord* record;

    record = MonsterDBFromSpeciesInline(monster_species);
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

// FUNCTION: WIZ8 0x004E5C00
void ProcessMonstersAtCombatEnd(unsigned char forced_cleanup)
{
    unsigned int index;

    for (index = 0; index < PListGetCount(g_monster_list); ++index) {
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(index);

        if (monster_info->flag_14 != 0 &&
            static_cast<unsigned int>(monster_info->hp_current) > 0 &&
            monster_info->value_9f == 0 &&
            monster_info->value_2da != 0) {
            if (forced_cleanup == 0) {
                Function58AB60(
                    9,
                    0,
                    *(int*)(g_object_68c09c + 0x74c),
                    GetMonsterName(monster_info, 0, 0));
            }
            Function5248D0(monster_info);
            if (forced_cleanup == 0) {
                monster_info->flag_253 = 1;
                if (monster_info->monster->IsDying() == 0) {
                    StartMonsterCycle(monster_info, 0x15, 1);
                    DeactivateMonster(monster_info);
                    MonsterDies(monster_info, 1);
                    RemoveMonster(
                        MonsterGetIndexByLocationID(
                            0x31f,
                            MONSTER_MANAGER_CPP,
                            monster_info->location_id,
                            1),
                        0);
                }
            }
        }
    }
}

// FUNCTION: WIZ8 0x004E5D00
void ConvertMonsterAttributes(W8MonsterInfo* monster_info)
{
    unsigned int monster_attribute = 0;

    do {
        W8MonsterRecord* record;
        int attribute_index;
        int value;

        if (monster_info == 0) {
            srAssertFail(
                "pMonsterInfo != NULL",
                MONSTER_MANAGER_CPP,
                0x5e9,
                0);
        }
        record = MonsterDBFromSpeciesInline(monster_info->monster_species);
        value = record->attribute_values_d1[monster_attribute];
        attribute_index = 0;

        if (monster_attribute >= 5) {
            srAssertFail(
                "uiMonsterAttribute < MONSTER_ATTR_COUNT",
                MONSTER_MANAGER_CPP,
                0x78d,
                0);
        }
        switch (monster_attribute) {
        case 0:
            attribute_index = 0;
            break;
        case 1:
            attribute_index = 1;
            break;
        case 2:
            attribute_index = 4;
            break;
        case 3:
            attribute_index = 5;
            break;
        case 4:
            attribute_index = 6;
            break;
        default:
            srAssertFail(
                "FALSE",
                MONSTER_MANAGER_CPP,
                0x798,
                "ConvertMonsterAttribute: ERROR - Invalid monster attribute");
        }

        value += monster_info->attribute_adjustments_1e7[attribute_index];
        if (value > 125) {
            value = 125;
        }
        else if (value < 1) {
            value = 1;
        }
        monster_info->converted_attributes_247[monster_attribute] =
            static_cast<unsigned char>(value);
        ++monster_attribute;
    } while (monster_attribute < 5);
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

// FUNCTION: WIZ8 0x004E5EA0
void ResetLivingMonstersAfterCombat(void)
{
    unsigned int index;

    for (index = 0; index < PListGetCount(g_monster_list); ++index) {
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(index);

        if (static_cast<unsigned int>(monster_info->hp_current) > 0) {
            Function452C90(&monster_info->monster->member_18);
            if (monster_info->flag_255 > 0 && monster_info->flag_255 <= 3) {
                monster_info->flag_255 = 0;
            }
        }
    }
}

// FUNCTION: WIZ8 0x004E5F00
void DestroyUngroupedMonsters(void)
{
    unsigned int index;

    for (index = 0; index < PListGetCount(g_monster_list); ++index) {
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(index);

        if (monster_info->monster_group_id == 0) {
            W8Monster* monster = monster_info->monster;

            if (monster != 0) {
                unsigned int flags = monster->m_cycles[19].flags_00;
                flags >>= 8;
                if ((flags & 1) != 0 &&
                    *((unsigned char*)&monster->m_cycles[24].flags_00 + 2) != 0) {
                    monster->Function4C50F0();
                }
            }
            monster_info = MonsterGetScriptPartByLocationIndex(index);
            DeactivateMonster(monster_info);
            if (monster_info == 0) {
                srAssertFail("pMonsterInfo", MONSTER_MANAGER_CPP, 0x282, 0);
            }
            if (monster_info->monster != 0) {
                Function4C59C0(monster_info->monster, GetWorld());
                Function46E5A0(Function451280(monster_info->monster));
                Function4C5860(monster_info->monster);
                monster_info->monster = 0;
            }
            (void)g_dword_6598a4;
            Function42E650(static_cast<unsigned short>(monster_info->location_id));
            Function509EA0(monster_info->runtime_value_2f1);
            void* removed = PListRemoveAt(g_monster_list, index);
            if (removed != 0) {
                free(removed);
            }
            --index;
        }
    }
}

// FUNCTION: WIZ8 0x004E6020
void SetMonsterControlState(W8MonsterInfo* monster_info, int control_state)
{
    if (monster_info == 0) {
        srAssertFail("pMonsterInfo", MONSTER_MANAGER_CPP, 0x85b, 0);
    }
    switch (control_state) {
    case 0:
    case 2:
        if (monster_info->control_state == 1 &&
            monster_info->monster->member_18.value_5c == 0) {
            Function4537E0(&monster_info->monster->member_18);
            monster_info->flag_255 = 0;
        }
        break;
    }
    monster_info->control_state = control_state;
    RecountActiveMonsterGroupMembers(
        GetMonsterGroupByListIndex(
            GetMonsterGroupIndexByID(
                0x872,
                MONSTER_MANAGER_CPP,
                monster_info->monster_group_id,
                1)));
}

// FUNCTION: WIZ8 0x004E60B0
void MonsterInfoSetMotionless(W8MonsterInfo* monster_info, unsigned char motionless)
{
    unsigned char previous = monster_info->motionless;
    W8Monster* monster = monster_info->monster;

    monster_info->motionless = motionless;
    if (motionless == 0) {
        if (previous != 0) {
            MonsterSetAnimating(monster, 1);
            if (monster_info->monster->m_cycles[18].unknown_0c[0xa7] == -1) {
                StartMonsterCycle(monster_info, 1, 3);
            }
        }
    }
    else if (previous == 0) {
        signed char cycle_value = monster->m_cycles[18].unknown_0c[0xa7];

        if (cycle_value != -1) {
            if (cycle_value == 0x14) {
                return;
            }
            MonsterSetPendingCycle(monster, -1);
        }
        MonsterSetAnimating(monster, 0);
    }
}

// FUNCTION: WIZ8 0x004E6130
void MoveMonsterToLiveList(W8MonsterInfo* monster_info)
{
    if (monster_info == 0) {
        srAssertFail("pMonsterInfo", MONSTER_MANAGER_CPP, 0x8b8, 0);
    }
    if (PListRemove(g_unborn_monster_list, monster_info) == 0) {
        return;
    }

    PListAdd(g_monster_list, monster_info);
    if (MonsterIsCycleSupported(monster_info->monster, 0) != 0) {
        MonsterSetCycle(monster_info->monster, 0);
        MonsterSetBehaviour(monster_info->monster, 1);
    }
    else {
        MonsterSetCycle(monster_info->monster, 1);
        MonsterSetBehaviour(monster_info->monster, 3);
    }
    MonsterSetSubCycle(monster_info->monster, 0);
    MonsterSetAnimating(monster_info->monster, 1);
    monster_info->monster->member_18.unknown_88 = 1;
    monster_info->monster->m_cycles[22].unknown_09 = 0;
}

static __forceinline double DistanceBetweenPositions(
    const srVector3T<float>* first,
    const srVector3T<float>& second)
{
    float x = first->x - second.x;
    float y = first->y - second.y;
    float z = first->z - second.z;
    return sqrt(x * x + y * y + z * z);
}

// FUNCTION: WIZ8 0x004E61E0
W8MonsterInfo* FindNearestMonsterInfo(
    const srVector3T<float>* position,
    double maximum_distance)
{
    W8MonsterInfo* nearest = 0;
    double nearest_distance = 1.0e11;
    unsigned int index;

    for (index = 0; index < PListGetCount(g_monster_list); ++index) {
        W8MonsterInfo* monster_info = (W8MonsterInfo*)PListGetAt(g_monster_list, index);
        double distance = DistanceBetweenPositions(
            position,
            monster_info->monster->member_18.GetPosition());

        if (distance < nearest_distance &&
            (maximum_distance == 0.0 || distance < maximum_distance)) {
            nearest = monster_info;
            nearest_distance = distance;
        }
    }

    for (index = 0; index < PListGetCount(g_unborn_monster_list); ++index) {
        W8MonsterInfo* monster_info = (W8MonsterInfo*)PListGetAt(g_unborn_monster_list, index);
        double distance = DistanceBetweenPositions(
            position,
            monster_info->monster->member_18.GetPosition());

        if (distance < nearest_distance &&
            (maximum_distance == 0.0 || distance < maximum_distance)) {
            nearest = monster_info;
            nearest_distance = distance;
        }
    }
    return nearest;
}

// FUNCTION: WIZ8 0x004E6370
void InitializeMonsterRuntimeStats(void)
{
    unsigned int index;

    for (index = 0; index < PListGetCount(g_monster_list); ++index) {
        W8MonsterInfo* monster_info = (W8MonsterInfo*)PListGetAt(g_monster_list, index);
        W8MonsterRecord* record;
        int value;

        if (monster_info == 0) {
            srAssertFail(
                "pMonsterInfo != NULL",
                MONSTER_MANAGER_CPP,
                0x5e9,
                0);
        }
        record = MonsterDBFromSpeciesInline(monster_info->monster_species);
        value = RollDice(&record->hit_points_d6);
        monster_info->hp_max = value;
        monster_info->hp_current = value;
        value = RollDice(&record->runtime_stat_da);
        monster_info->runtime_stat_max_2f = value;
        monster_info->runtime_stat_current_33 = value;
        monster_info->runtime_value_242 = Function52A780(value, value);
        monster_info->scale_24f = CalculateMonsterScale(monster_info);
        MonsterSetScale(monster_info->monster, monster_info->scale_24f);
        Function4C5810(monster_info->monster);
        Function4C5ED0(monster_info->monster);
    }

    for (index = 0; index < PListGetCount(g_unborn_monster_list); ++index) {
        W8MonsterInfo* monster_info = (W8MonsterInfo*)PListGetAt(g_unborn_monster_list, index);
        W8MonsterRecord* record;
        int value;

        if (monster_info == 0) {
            srAssertFail(
                "pMonsterInfo != NULL",
                MONSTER_MANAGER_CPP,
                0x5e9,
                0);
        }
        record = MonsterDBFromSpeciesInline(monster_info->monster_species);
        value = RollDice(&record->hit_points_d6);
        monster_info->hp_max = value;
        monster_info->hp_current = value;
        value = RollDice(&record->runtime_stat_da);
        monster_info->runtime_stat_max_2f = value;
        monster_info->runtime_stat_current_33 = value;
        monster_info->runtime_value_242 = Function52A780(value, value);
        monster_info->scale_24f = CalculateMonsterScale(monster_info);
        MonsterSetScale(monster_info->monster, monster_info->scale_24f);
        Function4C5810(monster_info->monster);
        Function4C5ED0(monster_info->monster);
    }
}

// FUNCTION: WIZ8 0x004E65D0
float CalculateMonsterScale(W8MonsterInfo* monster_info)
{
    float minimum;
    float maximum;

    MonsterGetScaleRange(monster_info->monster, &minimum, &maximum);
    if (minimum == 0.0f || maximum == 0.0f) {
        return MonsterGetScale(monster_info->monster);
    }

    if (monster_info == 0) {
        srAssertFail(
            "pMonsterInfo != NULL",
            MONSTER_MANAGER_CPP,
            0x5e9,
            0);
    }
    W8MonsterRecord* record = MonsterDBFromSpeciesInline(monster_info->monster_species);
    int minimum_hp = record->hit_points_d6.base + record->hit_points_d6.count;
    int maximum_hp = record->hit_points_d6.base +
                     record->hit_points_d6.count * record->hit_points_d6.sides;
    float scale = ((maximum - minimum) *
                   ((float)(unsigned int)monster_info->hp_max - (float)minimum_hp)) /
                  ((float)maximum_hp - (float)minimum_hp) + minimum;
    float result = (maximum - minimum) * (Random(1000) * 0.0004f - 0.2f) + scale;

    if (result < minimum) {
        return minimum;
    }
    if (result > maximum) {
        return maximum;
    }
    return result;
}

// FUNCTION: WIZ8 0x004E67A0
void TryStartMonsterCycle2(
    W8MonsterInfo* monster_info,
    W8Monster* monster,
    int query_state)
{
    if (monster_info->flag_14 != 0 &&
        static_cast<unsigned int>(monster_info->hp_current) > 0 &&
        monster_info->value_9f == 0 &&
        monster_info->flag_24d != 0 &&
        query_state == 1) {
        int result = MonsterQuery(monster, 2);

        if (result != 0 && monster_info->motionless == 0) {
            monster->m_cycles[19].flags_00 |= 0x80;
            if (MonsterIsCycleSupported(monster, 2) != 0) {
                signed char cycle = monster->m_cycles[18].unknown_0c[0xa7];

                if (cycle == 2 ||
                    monster->Function4C2CF0(cycle) == 0 ||
                    (g_flag_683f94 != 0 &&
                     *(int*)(g_object_6836a8 + 0x7b0) != 0 &&
                     *(W8MonsterInfo**)(g_object_6836a8 + 0x7b8) == monster_info)) {
                    return;
                }
                W8MonsterGroup* group = GetMonsterGroupByListIndex(
                    GetMonsterGroupIndexByID(
                        0x987,
                        MONSTER_MANAGER_CPP,
                        monster_info->monster_group_id,
                        1));
                unsigned int chance = group->member_count * 20;

                if (g_flag_683f97 == 0 && Random(chance) == 0) {
                    StartMonsterCycle(monster_info, 2, 1);
                }
            }
        }
    }
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
unsigned char AnyMonsterDying(void)
{
    unsigned int index;
    W8MonsterInfo* monster_info;

    for (index = 0; index < PListGetCount(g_monster_list); ++index) {
        monster_info = MonsterGetScriptPartByLocationIndex(index);
        if (monster_info != 0 && monster_info->monster->IsDying() != 0) {
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
