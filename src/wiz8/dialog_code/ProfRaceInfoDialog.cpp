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

typedef struct W8AttributeMinimums {
    int values[ATTR_COUNT];
} W8AttributeMinimums;                   /* 0x1c */

extern W8AttributeMinimums g_profession_attribute_minimums[];  /* 0x0064FDD8 */
extern W8AttributeMinimums g_race_attribute_minimums[];        /* 0x0064FF14 */

/* One 0x0c-byte row per profession or race, holding the three string ids the
   dialog copies out. The three are separate fields rather than an array because
   the constructor stores them into three separate members, out of order. */
typedef struct W8ProfRaceInfoRow {
    unsigned int name_id;                /* 0x00 */
    unsigned int detail_id;              /* 0x04 */
    unsigned int summary_id;             /* 0x08 */
} W8ProfRaceInfoRow;                     /* 0x0c */

extern W8ProfRaceInfoRow g_profession_info_rows[];             /* 0x0064FFB0 */
extern W8ProfRaceInfoRow g_race_info_rows[];                   /* 0x00650064 */

/* The shared base at vtable 0x005EFC88's neighbour, constructed by 0x005DEAF0;
   opaque here because only the derived tail is established. Its destructor is
   virtual, which is what puts a vtable pointer at offset zero of both dialogs
   and what gives both constructors the unwind frame the originals carry. */
class W8ProfRaceInfoDialogBase005DEAF0 {
public:
    W8ProfRaceInfoDialogBase005DEAF0();
    virtual ~W8ProfRaceInfoDialogBase005DEAF0();

protected:
    unsigned char unknown_004[0x50];
};                                       /* 0x54 */

class W8ProfessionInfoDialog005EFBFC : public W8ProfRaceInfoDialogBase005DEAF0 {
public:
    W8ProfessionInfoDialog005EFBFC(unsigned int uiIndex);
    virtual ~W8ProfessionInfoDialog005EFBFC() override;

private:
    unsigned int m_uiTitleId;            /* 0x054 */
    unsigned int m_uiSummaryId;          /* 0x058 */
    unsigned int m_uiNameId;             /* 0x05c */
    unsigned int m_uiHeadingId;          /* 0x060 */
    unsigned int m_uiDetailId;           /* 0x064 */
    int m_minimums[ATTR_COUNT];          /* 0x068 */
    unsigned char unknown_084[0xec];
    unsigned int m_uiIndex;              /* 0x170 */
};                                       /* 0x174 */

class W8RaceInfoDialog005EFC38 : public W8ProfRaceInfoDialogBase005DEAF0 {
public:
    W8RaceInfoDialog005EFC38(unsigned int uiIndex);
    virtual ~W8RaceInfoDialog005EFC38() override;

private:
    unsigned int m_uiTitleId;            /* 0x054 */
    unsigned int m_uiSummaryId;          /* 0x058 */
    unsigned int m_uiNameId;             /* 0x05c */
    unsigned int m_uiHeadingId;          /* 0x060 */
    unsigned int m_uiDetailId;           /* 0x064 */
    int m_minimums[ATTR_COUNT];          /* 0x068 */
    unsigned char unknown_084[0xec];
    unsigned int m_uiIndex;              /* 0x170 */
};                                       /* 0x174 */

/* Copies the row straight out of the table: every profession has minimums, so
   there is no per-entry test the way the race dialog needs one. */
// FUNCTION: WIZ8 0x005DF0D0
W8ProfessionInfoDialog005EFBFC::W8ProfessionInfoDialog005EFBFC(unsigned int uiIndex)
{
    int* source;
    int remaining;

    if (uiIndex >= PROF_COUNT) {
        srAssertFail("uiIndex < PROF_COUNT", PROF_RACE_INFO_DIALOG_CPP, 0x123, 0);
    }
    m_uiIndex = uiIndex;
    m_uiTitleId = 0x10b;
    m_uiHeadingId = 0x14c;
    m_uiSummaryId = g_profession_info_rows[uiIndex].summary_id;
    m_uiNameId = g_profession_info_rows[uiIndex].name_id;
    m_uiDetailId = g_profession_info_rows[uiIndex].detail_id;
    source = g_profession_attribute_minimums[uiIndex].values;
    for (remaining = 0; remaining < ATTR_COUNT; ++remaining) {
        m_minimums[remaining] = source[remaining];
    }
}

/* The same shape, except that only the first eleven races have minimums and the
   rest show -1, so the copy is a counted loop with the test inside it rather
   than the profession dialog's straight block move. */
// FUNCTION: WIZ8 0x005DF570
W8RaceInfoDialog005EFC38::W8RaceInfoDialog005EFC38(unsigned int uiIndex)
{
    unsigned int index;
    int* destination;

    if (uiIndex >= RACE_COUNT) {
        srAssertFail("uiIndex < RACE_COUNT", PROF_RACE_INFO_DIALOG_CPP, 0x199, 0);
    }
    m_uiIndex = uiIndex;
    m_uiTitleId = 0x10c;
    m_uiHeadingId = 0x152;
    m_uiSummaryId = g_race_info_rows[uiIndex].summary_id;
    m_uiNameId = g_race_info_rows[uiIndex].name_id;
    m_uiDetailId = g_race_info_rows[uiIndex].detail_id;
    for (index = 0, destination = m_minimums; index < ATTR_COUNT; ++index) {
        if (uiIndex < RACE_MINIMUMS_COUNT) {
            *destination = g_race_attribute_minimums[uiIndex].values[index];
        } else {
            *destination = -1;
        }
        ++destination;
    }
}
