#include "wiz8/monster_info_dialog.h"

#include <new>

extern void Function40C710(int resource);
extern void Function40D150(int resource);
extern int g_dword_69ca28;

/* Dialog Code\MonsterInfoDialog.cpp defines no assertions, so unlike Octree or
   Monster this class yields no member names. Only offsets are established here,
   by byte-exact ports; the fields keep positional names. The 0x58 subobject is
   the first of the three the reviewed complete destructor tears down. */

// FUNCTION: WIZ8 0x005E0C40
W8DialogMember005E0C40::W8DialogMember005E0C40()
{
    unknown_000 = 0;
    unknown_001 = 0;
    unknown_024 = 0;
    unknown_004 = 1;
    unknown_008 = 0;
    unknown_00c = -1;
    unknown_010 = -1;
    unknown_002 = 0;
    unknown_014[0] = 0;
    unknown_014[1] = 0;
    unknown_014[2] = 0;
    unknown_014[3] = 0;
    unknown_028 = -1;
    unknown_02c = -1;
    unknown_030 = -1;
    unknown_034 = -1;
    unknown_038 = -1;
    unknown_03c = -1;
    unknown_040 = -1;
    unknown_044 = -1;
    unknown_048 = 0;
}

// FUNCTION: WIZ8 0x005DB1B0
W8DialogMember005DB1B0::W8DialogMember005DB1B0()
{
    m_resource_018 = -1;
    m_resource_01c = -1;
    unknown_024 = 0;
    unknown_028 = 0;
    unknown_02c = 0;
    unknown_030 = 0;
    unknown_034 = 0;
    unknown_035 = 1;
    unknown_036 = 0;
    unknown_037 = 0;
    unknown_020 = -1;
    unknown_038 = 1;
    unknown_039 = 0;
    unknown_03a = 0;
    unknown_03b = 0;
    unknown_03c = 0;
    unknown_040 = 0;
    unknown_004 = -1;
    unknown_008 = -1;
    unknown_00c = -1;
    unknown_010 = -1;
    unknown_014 = -1;
    unknown_044 = g_dword_69ca28;
}

// FUNCTION: WIZ8 0x005DB260
W8DialogMember005DB1B0::~W8DialogMember005DB1B0()
{
    if (m_resource_018 != -1) {
        Function40C710(m_resource_018);
        m_resource_018 = -1;
    }
    if (m_resource_01c != -1) {
        Function40D150(m_resource_01c);
        m_resource_01c = -1;
    }
}

__forceinline W8DialogPtrVectorBase005EF89C::W8DialogPtrVectorBase005EF89C()
{
    m_data = static_cast<W8DialogOwned005D14D0**>(::operator new(5 * sizeof(void*)));
    m_count = 0;
    if (m_data != 0) {
        m_capacity = 5;
    }
    else {
        m_capacity = 0;
    }
}

__forceinline W8DialogPtrVectorBase005EF89C::~W8DialogPtrVectorBase005EF89C()
{
    ::operator delete(m_data);
}

__forceinline W8DialogPtrVector005EF898::W8DialogPtrVector005EF898()
{
}

// FUNCTION: WIZ8 0x005D2540
__forceinline W8DialogPtrVector005EF898::~W8DialogPtrVector005EF898()
{
}

__forceinline int W8DialogPtrVector005EF898::GetCount() const
{
    return m_count;
}

__forceinline W8DialogOwned005D14D0* W8DialogPtrVector005EF898::RemoveAt(int position)
{
    int index;
    W8DialogOwned005D14D0* result;

    if (position >= m_count || position < 0) {
        return 0;
    }
    result = m_data[position];
    for (index = position; index < m_count - 1; ++index) {
        m_data[index] = m_data[index + 1];
    }
    --m_count;
    return result;
}

// FUNCTION: WIZ8 0x005D14D0
W8DialogMember005D14D0::W8DialogMember005D14D0()
{
    int invalid;

    invalid = -1;
    unknown_048 = invalid;
    unknown_04c = invalid;
    unknown_050 = invalid;
    unknown_055 = invalid;
    unknown_014 = 0;
    unknown_018 = 0;
    unknown_010 = 0;
    unknown_03c = 0;
    unknown_03d = 0;
    unknown_03e = 0;
    unknown_040 = 0;
    unknown_044 = 0;
    unknown_054 = 0;
    unknown_056 = 0;
}

// FUNCTION: WIZ8 0x005D1590
W8DialogMember005D14D0::~W8DialogMember005D14D0()
{
    int index;

    if (m_vector_01c.GetCount() > 0) {
        for (index = m_vector_01c.GetCount() - 1; index >= 0; --index) {
            delete m_vector_01c.RemoveAt(index);
        }
    }
}

// Primary vtable slot 12.
// FUNCTION: WIZ8 0x005D6E60
void W8MonsterInfoDialog::ClearField41IfEnabled()
{
    if (m_field_50) {
        m_field_41 = 0;
    }
}

// Primary vtable slot 2.
// FUNCTION: WIZ8 0x005DBDE0
void W8MonsterInfoDialog::ResetSubobjectAndRefresh()
{
    m_member_58.Reset();
    W8DialogBase005DC7A0::ResetSubobjectAndRefresh();
}
