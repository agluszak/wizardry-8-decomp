#include "wiz8/dialog_code/ProfRaceInfoDialog.h"
#include "wiz8/bringup_gates.h"

// GLOBAL: WIZ8 0x00614cf0
W8AttributeMinimums g_race_attribute_minimums[11] = {
    {{45, 45, 45, 45, 45, 45, 45}}, {{35, 50, 50, 35, 50, 45, 40}},
    {{55, 30, 50, 60, 35, 35, 35}}, {{35, 50, 40, 50, 50, 35, 45}},
    {{40, 40, 30, 45, 55, 50, 50}}, {{25, 55, 35, 30, 50, 60, 45}},
    {{60, 25, 25, 70, 40, 50, 30}}, {{55, 35, 30, 60, 50, 40, 30}},
    {{40, 40, 30, 35, 50, 60, 50}}, {{40, 30, 55, 50, 40, 40, 50}},
    {{50, 50, 25, 50, 35, 35, 55}},
};

// GLOBAL: WIZ8 0x00614e24
W8AttributeMinimums g_profession_attribute_minimums[15] = {
    {{55, 0, 0, 50, 50, 0, 0}},  {{55, 0, 55, 55, 50, 50, 0}},
    {{50, 0, 55, 55, 50, 50, 0}}, {{50, 50, 0, 50, 55, 0, 55}},
    {{50, 55, 0, 50, 55, 55, 0}}, {{50, 50, 0, 50, 55, 55, 50}},
    {{0, 50, 50, 0, 50, 55, 55}}, {{0, 0, 0, 0, 55, 50, 50}},
    {{45, 55, 0, 0, 60, 0, 55}},  {{45, 50, 0, 0, 55, 0, 55}},
    {{0, 0, 60, 55, 0, 0, 0}},    {{0, 55, 0, 0, 60, 0, 0}},
    {{0, 55, 55, 0, 55, 0, 55}},  {{0, 55, 0, 0, 0, 0, 60}},
    {{0, 60, 0, 0, 55, 0, 0}},
};
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

// GLOBAL: WIZ8 0x0064ffb0
W8ProfRaceInfoRow g_profession_info_rows[15] = {
    {676, 691, 10}, {677, 692, 24}, {678, 693, 8},  {679, 694, 2},
    {680, 695, 16}, {681, 696, 20}, {682, 697, 12}, {683, 698, 28},
    {684, 699, 6},  {685, 700, 0},  {686, 701, 18}, {687, 702, 22},
    {688, 703, 4},  {689, 704, 14}, {690, 705, 26},
};

// GLOBAL: WIZ8 0x00650064
W8ProfRaceInfoRow g_race_info_rows[16] = {
    {644, 660, 8},  {645, 661, 4},  {646, 662, 12}, {647, 663, 16},
    {648, 664, 14}, {649, 665, 20}, {650, 666, 0},  {651, 667, 10},
    {652, 668, 2},  {653, 669, 18}, {654, 670, 6},  {655, 671, 30},
    {656, 672, 24}, {657, 673, 22}, {658, 674, 26}, {659, 675, 28},
};

/* Copies the row straight out of the table: every profession has minimums, so
   there is no per-entry test the way the race dialog needs one. */
// FUNCTION: WIZ8 0x005DEAF0
W8ProfRaceInfoDialogBase005DEAF0::W8ProfRaceInfoDialogBase005DEAF0()
{
    SetOrigin(0x85, 0x69);
    SetExtent(0x177, 0x10e);
    SetBackground("Data\\Dialogs\\popup_race_profession.sti", 0);
}

// FUNCTION: WIZ8 0x005DEBB0
W8ProfRaceInfoDialogBase005DEAF0::~W8ProfRaceInfoDialogBase005DEAF0()
{
    scrollbar_084.DestroyControls();
    W8DialogBase::DestroyControls();
    NoOp();
}

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
