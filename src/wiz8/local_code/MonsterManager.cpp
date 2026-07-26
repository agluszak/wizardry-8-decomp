#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"
#include <new>
#include <stdlib.h>

#define MONSTER_MANAGER_CPP "C:\\Projects\\Wizardry 8\\Local Code\\MonsterManager.cpp"
#define MAX_MONSTERS_IN_DATABASE 1000

extern "C" char* String(const char* format, ...);
int Function4A87A0(const char* name);
unsigned char Function47B610(int index);
void Function4C5A00(W8Monster* monster, int value);
unsigned char Function4C5A80(W8Monster* monster, int value);
void Function4C5AA0(W8Monster* monster, int value);
void Function4C5B10(W8Monster* monster, int value);
int Function4C5B40(W8Monster* monster, int value);
void Function4C6140(W8Monster* monster);
void Function4C6180(W8Monster* monster, int value);
void Function4C61A0(W8Monster* monster, int value);
void Function4C61C0(W8Monster* monster, int value);
void Function4E3AF0(unsigned int monster_list_index, int value);
void Function4E4280(W8MonsterInfo* monster_info);
void Function4E4DB0(W8MonsterInfo* monster_info, int value, int reason);
void Function4E46F0(W8MonsterInfo* monster_info, int value);
void __fastcall Function452C90(W8MonsterMember18* member);
void __fastcall Function4537E0(W8MonsterMember18* member);
unsigned int Function5100B0(
    int caller_line,
    const char* caller_file,
    int monster_group_id,
    unsigned char assert_on_failure);
W8MonsterGroup* Function5101B0(unsigned int monster_group_index);
void Function510350(W8MonsterGroup* monster_group);
int Function555F30(srVector3T<float> position);
int Function4E5150(W8MonsterInfo* monster_info, int value_1, int value_2);
void Function5248D0(W8MonsterInfo* monster_info);
void Function58AB60(int value_1, int value_2, int value_3, int value_4);
extern unsigned char* g_object_68c09c;

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
    if (monster_info->flag_24e == 0) {
        result = Function4C5B40(monster_info->monster, 6);
        if (result != 1 && result != 2 &&
            monster_info->monster->m_cycles[18].unknown_0c[0xa7] == -1) {
            Function4E4DB0(monster_info, 1, 3);
        }
    }
}

// FUNCTION: WIZ8 0x004E4690
void Function4E4690(W8MonsterInfo* monster_info, int value)
{
    if (monster_info->monster->Function4CA4C0() == 0) {
        Function4E4DB0(monster_info, 0x15, 1);
        Function4E4280(monster_info);
        Function4E46F0(monster_info, value);
        Function4E3AF0(
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

// FUNCTION: WIZ8 0x004E5720
W8MonsterRecord* Function4E5720(W8MonsterInfo* monster_info)
{
    if (monster_info == 0) {
        srAssertFail(
            "pMonsterInfo != NULL",
            MONSTER_MANAGER_CPP,
            0x5e9,
            0);
    }
    return GetMonsterDataByIDInline(monster_info->monster_species);
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

// FUNCTION: WIZ8 0x004E5A50
void Function4E5A50(W8MonsterInfo* monster_info)
{
    W8Monster* monster = monster_info->monster;

    if (monster != 0) {
        int count = monster->Function4C6A50();
        if (count > 1) {
            int value = ((monster_info->value_27 - monster_info->value_2b) * count) /
                        monster_info->value_27;
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
int Function4E5AF0(W8MonsterInfo* monster_info)
{
    if (monster_info == 0) {
        srAssertFail("pMonsterInfo", MONSTER_MANAGER_CPP, 0x6b9, 0);
    }
    return Function555F30(monster_info->monster->member_18.Function4534C0());
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

// FUNCTION: WIZ8 0x004E5C00
void Function4E5C00(unsigned char value)
{
    unsigned int index;

    for (index = 0; index < PListGetCount(g_monster_list); ++index) {
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(index);

        if (monster_info->flag_14 != 0 &&
            static_cast<unsigned int>(monster_info->value_2b) > 0 &&
            monster_info->value_9f == 0 &&
            monster_info->value_2da != 0) {
            if (value == 0) {
                Function58AB60(
                    9,
                    0,
                    *(int*)(g_object_68c09c + 0x74c),
                    Function4E5150(monster_info, 0, 0));
            }
            Function5248D0(monster_info);
            if (value == 0) {
                monster_info->flag_253 = 1;
                if (monster_info->monster->Function4CA4C0() == 0) {
                    Function4E4DB0(monster_info, 0x15, 1);
                    Function4E4280(monster_info);
                    Function4E46F0(monster_info, 1);
                    Function4E3AF0(
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
void Function4E5D00(W8MonsterInfo* monster_info)
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
        record = GetMonsterDataByIDInline(monster_info->monster_species);
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
void Function4E5EA0(void)
{
    unsigned int index;

    for (index = 0; index < PListGetCount(g_monster_list); ++index) {
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(index);

        if (static_cast<unsigned int>(monster_info->value_2b) > 0) {
            Function452C90(&monster_info->monster->member_18);
            if (monster_info->flag_255 > 0 && monster_info->flag_255 <= 3) {
                monster_info->flag_255 = 0;
            }
        }
    }
}

// FUNCTION: WIZ8 0x004E6020
void Function4E6020(W8MonsterInfo* monster_info, int value)
{
    if (monster_info == 0) {
        srAssertFail("pMonsterInfo", MONSTER_MANAGER_CPP, 0x85b, 0);
    }
    switch (value) {
    case 0:
    case 2:
        if (monster_info->value_2fd == 1 &&
            monster_info->monster->member_18.value_5c == 0) {
            Function4537E0(&monster_info->monster->member_18);
            monster_info->flag_255 = 0;
        }
        break;
    }
    monster_info->value_2fd = value;
    Function510350(
        Function5101B0(
            Function5100B0(
                0x872,
                MONSTER_MANAGER_CPP,
                monster_info->monster_group_id,
                1)));
}

// FUNCTION: WIZ8 0x004E60B0
void Function4E60B0(W8MonsterInfo* monster_info, unsigned char value)
{
    unsigned char previous = monster_info->flag_24e;
    W8Monster* monster = monster_info->monster;

    monster_info->flag_24e = value;
    if (value == 0) {
        if (previous != 0) {
            Function4C5A00(monster, 1);
            if (monster_info->monster->m_cycles[18].unknown_0c[0xa7] == -1) {
                Function4E4DB0(monster_info, 1, 3);
            }
        }
    }
    else if (previous == 0) {
        signed char cycle_value = monster->m_cycles[18].unknown_0c[0xa7];

        if (cycle_value != -1) {
            if (cycle_value == 0x14) {
                return;
            }
            Function4C5AA0(monster, -1);
        }
        Function4C5A00(monster, 0);
    }
}

// FUNCTION: WIZ8 0x004E6130
void Function4E6130(W8MonsterInfo* monster_info)
{
    if (monster_info == 0) {
        srAssertFail("pMonsterInfo", MONSTER_MANAGER_CPP, 0x8b8, 0);
    }
    if (PListRemove(g_unborn_monster_list, monster_info) == 0) {
        return;
    }

    PListAdd(g_monster_list, monster_info);
    if (Function4C5A80(monster_info->monster, 0) != 0) {
        Function4C6180(monster_info->monster, 0);
        Function4C61A0(monster_info->monster, 1);
    }
    else {
        Function4C6180(monster_info->monster, 1);
        Function4C61A0(monster_info->monster, 3);
    }
    Function4C61C0(monster_info->monster, 0);
    Function4C5A00(monster_info->monster, 1);
    monster_info->monster->member_18.unknown_88 = 1;
    monster_info->monster->m_cycles[22].unknown_09 = 0;
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
