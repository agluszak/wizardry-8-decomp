#include "wiz8/dialog_code/ProfRaceInfoDialog.h"
#include "wiz8/sr_api.h"

/* Dialog Code\ProfRaceInfoDialog.cpp. The profession and race information
   dialogs, whose constructors are the same body twice over: they differ in
   their vtable, their row table, their two string ids, their assertion line and
   how they fill the attribute minimums. Both are named by their own assertion,
   which also supplies the parameter name uiIndex and pins PROF_COUNT to fifteen
   and RACE_COUNT to sixteen. */

static const char PROF_RACE_INFO_DIALOG_CPP[] =
    "C:\\Projects\\Wizardry 8\\Dialog Code\\ProfRaceInfoDialog.cpp";

enum { ATTR_COUNT = 7, PROF_COUNT = 15, RACE_COUNT = 16 };

/* Only the first eleven races have their own minimums; the rest are shown as
   -1, which is the dialog's "no requirement" marker. */
enum { RACE_MINIMUMS_COUNT = 11 };

/* Copies the row straight out of the table: every profession has minimums, so
   there is no per-entry test the way the race dialog needs one. */
// FUNCTION: WIZ8 0x005df0d0
W8ProfessionInfoDialog005EFBFC::W8ProfessionInfoDialog005EFBFC(unsigned int uiIndex)
{
    if (uiIndex >= PROF_COUNT) {
        srAssertFail("uiIndex < PROF_COUNT", PROF_RACE_INFO_DIALOG_CPP, 0x123, 0);
    }
    m_uiIndex = uiIndex;
    m_uiTitleId = 0x10b;
    m_uiHeadingId = 0x14c;
    m_uiSummaryId = g_profession_info_rows[uiIndex].summary_id;
    m_uiNameId = g_profession_info_rows[uiIndex].name_id;
    m_uiDetailId = g_profession_info_rows[uiIndex].detail_id;
    for (int attribute = 0; attribute < ATTR_COUNT; ++attribute) {
        m_minimums[attribute] =
            g_profession_attribute_minimums[uiIndex].values[attribute];
    }
}

/* The same shape, except that only the first eleven races have minimums and the
   rest show -1, so the copy is a counted loop with the test inside it rather
   than the profession dialog's straight block move. */
// FUNCTION: WIZ8 0x005df570
W8RaceInfoDialog005EFC38::W8RaceInfoDialog005EFC38(unsigned int uiIndex)
{
    if (uiIndex >= RACE_COUNT) {
        srAssertFail("uiIndex < RACE_COUNT", PROF_RACE_INFO_DIALOG_CPP, 0x199, 0);
    }
    m_uiIndex = uiIndex;
    m_uiTitleId = 0x10c;
    m_uiHeadingId = 0x152;
    m_uiSummaryId = g_race_info_rows[uiIndex].summary_id;
    m_uiNameId = g_race_info_rows[uiIndex].name_id;
    m_uiDetailId = g_race_info_rows[uiIndex].detail_id;
    for (int attribute = 0; attribute < ATTR_COUNT; ++attribute) {
        if (uiIndex < RACE_MINIMUMS_COUNT) {
            m_minimums[attribute] =
                g_race_attribute_minimums[uiIndex].values[attribute];
        } else {
            m_minimums[attribute] = -1;
        }
    }
}
