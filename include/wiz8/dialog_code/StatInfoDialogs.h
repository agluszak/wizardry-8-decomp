#pragma once

/* Shared storage for the two attribute-information dialogs. The base has no
   accepted original name yet, so its constructor address remains in the name. */
class W8StatInfoDialogBase005DF880 {
public:
    W8StatInfoDialogBase005DF880();
    virtual ~W8StatInfoDialogBase005DF880();

protected:
    unsigned char unknown_004[0x13c];
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

static_assert(sizeof(W8StatInfoDialogBase005DF880) == 0x140,
              "W8StatInfoDialogBase005DF880_must_be_0x140");
static_assert(sizeof(W8StatInfoDialog005DFC70) == 0x14c,
              "W8StatInfoDialog005DFC70_must_be_0x14c");
static_assert(sizeof(W8StatInfoDialog005E0180) == 0x14c,
              "W8StatInfoDialog005E0180_must_be_0x14c");
