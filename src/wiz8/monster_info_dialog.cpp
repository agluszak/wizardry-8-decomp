#include "wiz8/monster_info_dialog.h"

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
