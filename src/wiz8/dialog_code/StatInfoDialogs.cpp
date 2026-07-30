#include "wiz8/dialog_code/StatInfoDialogs.h"
#include "wiz8/sr_api.h"

/* Dialog Code\StatInfoDialogs.cpp. Two attribute-info dialogs whose
   constructors are the same body twice over: the canonical pair differs only
   in its vtable, its two lookup tables and its assertion line. Both are named
   by their own assertion, which also supplies the parameter name uiIndex and
   pins ATTR_COUNT to seven - the same seven attributes W8Character carries. */

static const char STAT_INFO_DIALOGS_CPP[] =
    "C:\\Projects\\Wizardry 8\\Dialog Code\\StatInfoDialogs.cpp";

enum { ATTR_COUNT = 7 };

/* Per-attribute lookup tables. Each dialog reads its own pair, widening the
   16-bit entries into its 32-bit fields. */
extern unsigned short g_attr_table_61E3A4[];
extern unsigned short g_attr_table_61E4FC[];
extern unsigned short g_attr_table_61E3C4[];
extern unsigned short g_attr_table_61E50C[];

// FUNCTION: WIZ8 0x005dfc70
W8StatInfoDialog005DFC70::W8StatInfoDialog005DFC70(unsigned int uiIndex)
{
    if (uiIndex >= ATTR_COUNT) {
        srAssertFail("uiIndex < ATTR_COUNT", STAT_INFO_DIALOGS_CPP, 204, 0);
    }
    m_uiIndex = uiIndex;
    m_value_140 = g_attr_table_61E3A4[uiIndex];
    m_value_144 = g_attr_table_61E4FC[uiIndex];
}

// FUNCTION: WIZ8 0x005e0180
W8StatInfoDialog005E0180::W8StatInfoDialog005E0180(unsigned int uiIndex)
{
    if (uiIndex >= ATTR_COUNT) {
        srAssertFail("uiIndex < ATTR_COUNT", STAT_INFO_DIALOGS_CPP, 278, 0);
    }
    m_uiIndex = uiIndex;
    m_value_140 = g_attr_table_61E3C4[uiIndex];
    m_value_144 = g_attr_table_61E50C[uiIndex];
}
