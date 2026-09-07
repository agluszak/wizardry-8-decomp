#pragma once

#include "wiz8/dialog_code/DialogBase.h"

/* Shared storage for the two attribute-information dialogs. The base has no
   accepted original name yet, so its constructor address remains in the name. */
class W8StatInfoDialogBase005DF880 : public W8DialogBase {
public:
    W8StatInfoDialogBase005DF880();
    virtual ~W8StatInfoDialogBase005DF880() override;

protected:
    unsigned char unknown_054[0xec];
};

class W8StatInfoDialog005DFC70 : public W8StatInfoDialogBase005DF880 {
public:
    W8StatInfoDialog005DFC70(unsigned int uiIndex);
    virtual ~W8StatInfoDialog005DFC70() override;

private:
    unsigned int m_value_140;            /* 0x140 */
    unsigned int m_value_144;            /* 0x144 */
    unsigned int m_uiIndex;              /* 0x148 */
};

class W8StatInfoDialog005E0180 : public W8StatInfoDialogBase005DF880 {
public:
    W8StatInfoDialog005E0180(unsigned int uiIndex);
    virtual ~W8StatInfoDialog005E0180() override;

private:
    unsigned int m_value_140;            /* 0x140 */
    unsigned int m_value_144;            /* 0x144 */
    unsigned int m_uiIndex;              /* 0x148 */
};

// VTABLE: WIZ8 0x005efd08
class W8SkillInfoDialog005EFD08 : public W8StatInfoDialogBase005DF880 {
public:
    W8SkillInfoDialog005EFD08(unsigned int skill, unsigned char first,
                              unsigned char second, unsigned char bonus);
    virtual ~W8SkillInfoDialog005EFD08() override;

private:
    unsigned int m_title_id_140;
    unsigned int m_detail_id_144;
    unsigned int m_skill_148;
    unsigned char m_first_14c;
    unsigned char m_second_14d;
    unsigned char m_bonus_14e;
    unsigned char pad_14f;
};

static_assert(sizeof(W8StatInfoDialogBase005DF880) == 0x140,
              "W8StatInfoDialogBase005DF880_must_be_0x140");
static_assert(sizeof(W8StatInfoDialog005DFC70) == 0x14c,
              "W8StatInfoDialog005DFC70_must_be_0x14c");
static_assert(sizeof(W8StatInfoDialog005E0180) == 0x14c,
              "W8StatInfoDialog005E0180_must_be_0x14c");
static_assert(sizeof(W8SkillInfoDialog005EFD08) == 0x150,
              "W8SkillInfoDialog005EFD08_must_be_0x150");
