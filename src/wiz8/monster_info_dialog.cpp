#include "wiz8/gameplay_boundaries.h"

/* Dialog Code\MonsterInfoDialog.cpp defines no assertions, so unlike Octree or
   Monster this class yields no member names. Only offsets are established here,
   by byte-exact ports; the fields keep positional names. The 0x58 subobject is
   the first of the three the reviewed complete destructor tears down. */

struct W8DialogSubobject {
    void Reset();                           /* 0x005E0E00 */
};

struct W8MonsterInfoDialog {
    unsigned char unknown_000[0x41];
    unsigned char m_field_41;               /* 0x41 */
    unsigned char unknown_042[0xe];
    unsigned char m_field_50;               /* 0x50 */
    unsigned char unknown_051[7];
    W8DialogSubobject m_sub58;              /* 0x58 */

    void ClearField41IfEnabled();
    void Refresh();                         /* 0x005DCC30 */
    void ResetSubobjectAndRefresh();
};

// FUNCTION: WIZ8 0x005D6E60
// Primary vtable slot 12.
void W8MonsterInfoDialog::ClearField41IfEnabled()
{
    if (m_field_50) {
        m_field_41 = 0;
    }
}

// FUNCTION: WIZ8 0x005DBDE0
// Primary vtable slot 2.
void W8MonsterInfoDialog::ResetSubobjectAndRefresh()
{
    m_sub58.Reset();
    Refresh();
}
