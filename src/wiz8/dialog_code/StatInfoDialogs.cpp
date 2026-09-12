#include "wiz8/dialog_code/StatInfoDialogs.h"
#include "wiz8/screen_state.h"
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
   16-bit entries into its 32-bit  */
extern unsigned short g_character_description_first_ids_61e3a4[];
extern unsigned short g_character_skill_name_ids_61e454[];
// GLOBAL: WIZ8 0x0061e4fc
unsigned short g_attr_table_61E4FC[8] = {
    0x6a0, 0x6a1, 0x6a2, 0x6a3, 0x6a4, 0x6a5, 0x6a6, 0,
};
// GLOBAL: WIZ8 0x0061e50c
unsigned short g_attr_table_61E50C[50] = {
    0x6a7, 0x6a8, 0x6a9, 0x6aa, 0x6ab, 0,     0x30b, 0x30c, 0x30d, 0x30e, 0x30f, 0x310, 0x311,
    0x312, 0x313, 0x314, 0x315, 0x316, 0x328, 0x329, 0x32a, 0x32b, 0x32c, 0x32d, 0x32e, 0x32f,
    0x330, 0x331, 0x332, 0x333, 0x334, 0x335, 0x336, 0x337, 0x338, 0x339, 0x33a, 0x33b, 0x33c,
    0x33d, 0x33e, 0x33f, 0x340, 0x341, 0x342, 0x343, 0x344, 0x345, 0x346, 0x347,
};

// FUNCTION: WIZ8 0x005df880
W8StatInfoDialogBase005DF880::W8StatInfoDialogBase005DF880()
{
    SetOrigin(0x9c, 0x69);
    SetExtent(0x14a, 0x10e);
    SetBackground("Data\\Dialogs\\popup_monsterinfo.sti", 0);
}

// FUNCTION: WIZ8 0x005DF940
W8StatInfoDialogBase005DF880::~W8StatInfoDialogBase005DF880()
{
    scrollbar_054.DestroyControls();
    W8DialogBase::DestroyControls();
    NoOp();
}

// FUNCTION: WIZ8 0x005dfc70
W8StatInfoDialog005DFC70::W8StatInfoDialog005DFC70(unsigned int uiIndex)
{
    if (uiIndex >= ATTR_COUNT) {
        srAssertFail("uiIndex < ATTR_COUNT", STAT_INFO_DIALOGS_CPP, 204, 0);
    }
    m_uiIndex = uiIndex;
    m_value_140 = g_character_description_first_ids_61e3a4[uiIndex];
    m_value_144 = g_attr_table_61E4FC[uiIndex];
}

// FUNCTION: WIZ8 0x005e0180
W8StatInfoDialog005E0180::W8StatInfoDialog005E0180(unsigned int uiIndex)
{
    if (uiIndex >= ATTR_COUNT) {
        srAssertFail("uiIndex < ATTR_COUNT", STAT_INFO_DIALOGS_CPP, 278, 0);
    }
    m_uiIndex = uiIndex;
    m_value_140 = g_character_description_first_ids_61e3a4[16 + uiIndex];
    m_value_144 = g_attr_table_61E50C[uiIndex];
}
