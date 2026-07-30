#pragma once

enum { W8_DIALOG_ATTRIBUTE_COUNT = 7 };

struct W8AttributeMinimums {
    int values[W8_DIALOG_ATTRIBUTE_COUNT];
};

/* One row supplies the three string ids copied into a profession or race
   information dialog. */
struct W8ProfRaceInfoRow {
    unsigned int name_id;
    unsigned int detail_id;
    unsigned int summary_id;
};

extern W8AttributeMinimums g_profession_attribute_minimums[];
extern W8AttributeMinimums g_race_attribute_minimums[];
extern W8ProfRaceInfoRow g_profession_info_rows[];
extern W8ProfRaceInfoRow g_race_info_rows[];

/* Shared dialog storage constructed by 0x005DEAF0. Only the derived tail is
   known, so the base remains address-qualified and opaque. */
class W8ProfRaceInfoDialogBase005DEAF0 {
public:
    W8ProfRaceInfoDialogBase005DEAF0();
    virtual ~W8ProfRaceInfoDialogBase005DEAF0();

protected:
    unsigned char unknown_004[0x50];
};

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
    int m_minimums[W8_DIALOG_ATTRIBUTE_COUNT]; /* 0x068 */
    unsigned char unknown_084[0xec];
    unsigned int m_uiIndex;              /* 0x170 */
};

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
    int m_minimums[W8_DIALOG_ATTRIBUTE_COUNT]; /* 0x068 */
    unsigned char unknown_084[0xec];
    unsigned int m_uiIndex;              /* 0x170 */
};

static_assert(sizeof(W8AttributeMinimums) == 0x1c,
              "W8AttributeMinimums_must_be_0x1c");
static_assert(sizeof(W8ProfRaceInfoRow) == 0x0c,
              "W8ProfRaceInfoRow_must_be_0x0c");
static_assert(sizeof(W8ProfRaceInfoDialogBase005DEAF0) == 0x54,
              "W8ProfRaceInfoDialogBase005DEAF0_must_be_0x54");
static_assert(sizeof(W8ProfessionInfoDialog005EFBFC) == 0x174,
              "W8ProfessionInfoDialog005EFBFC_must_be_0x174");
static_assert(sizeof(W8RaceInfoDialog005EFC38) == 0x174,
              "W8RaceInfoDialog005EFC38_must_be_0x174");
