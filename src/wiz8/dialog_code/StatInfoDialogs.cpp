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

/* The shared base at vtable 0x005EFC88, constructed by 0x005DF880 and
   destroyed by 0x005DFD20; opaque here because only the derived tail is
   established. Its destructor is virtual, which is what puts a vtable pointer
   at offset zero of both dialogs. */
class W8StatInfoDialogBase005DF880 {
public:
    W8StatInfoDialogBase005DF880();
    virtual ~W8StatInfoDialogBase005DF880();

protected:
    unsigned char unknown_004[0x13c];
};                                       /* 0x140 */

class W8StatInfoDialog005DFC70 : public W8StatInfoDialogBase005DF880 {
public:
    W8StatInfoDialog005DFC70(unsigned int uiIndex);
    virtual ~W8StatInfoDialog005DFC70();

private:
    unsigned int m_value_140;            /* 0x140: widened from the first table */
    unsigned int m_value_144;            /* 0x144: widened from the second table */
    unsigned int m_uiIndex;              /* 0x148 */
};                                       /* 0x14c */

class W8StatInfoDialog005E0180 : public W8StatInfoDialogBase005DF880 {
public:
    W8StatInfoDialog005E0180(unsigned int uiIndex);
    virtual ~W8StatInfoDialog005E0180();

private:
    unsigned int m_value_140;            /* 0x140 */
    unsigned int m_value_144;            /* 0x144 */
    unsigned int m_uiIndex;              /* 0x148 */
};                                       /* 0x14c */

// FUNCTION: WIZ8 0x005DFC70
W8StatInfoDialog005DFC70::W8StatInfoDialog005DFC70(unsigned int uiIndex)
{
    if (!(uiIndex < ATTR_COUNT)) {
        srAssertFail("uiIndex < ATTR_COUNT", STAT_INFO_DIALOGS_CPP, 204, 0);
    }
    m_uiIndex = uiIndex;
    m_value_140 = g_attr_table_61E3A4[uiIndex];
    m_value_144 = g_attr_table_61E4FC[uiIndex];
}

// FUNCTION: WIZ8 0x005E0180
W8StatInfoDialog005E0180::W8StatInfoDialog005E0180(unsigned int uiIndex)
{
    if (!(uiIndex < ATTR_COUNT)) {
        srAssertFail("uiIndex < ATTR_COUNT", STAT_INFO_DIALOGS_CPP, 278, 0);
    }
    m_uiIndex = uiIndex;
    m_value_140 = g_attr_table_61E3C4[uiIndex];
    m_value_144 = g_attr_table_61E50C[uiIndex];
}
