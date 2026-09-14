#pragma once

#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/dialog_code/DialogButton.h"
#include "wiz8/dialog_code/DialogScrollBar.h"
#include "wiz8/dialog_code/DialogTextArea.h"

/* Shared storage for the attribute-information dialogs and the skill info
   dialog. The base has no accepted original name yet, so its constructor
   address remains in the name. It is never instantiated on its own, but
   retail still emits its vtable: the derived constructors briefly store
   0x005efc88 before overwriting it, and the derived destructors restore it. */
// VTABLE: WIZ8 0x005efc88
class W8StatInfoDialogBase005DF880 : public W8DialogBase {
public:
    W8StatInfoDialogBase005DF880();
    virtual ~W8StatInfoDialogBase005DF880() override;
    virtual int CreateControls() override;
    virtual void DestroyControls() override;
    virtual void Draw() override;
    virtual void OnRightButtonUp() override;
    virtual void OnMouseWheel(int delta) override;
    virtual void DrawTitle();
    virtual unsigned char PopulateText();

protected:
    static void ScrollCallback(W8DialogScrollBar* scroll_bar, int first_visible_entry);

    W8DialogScrollBar scrollbar_054;
    W8DialogButton button_0a0;
    W8DialogTextArea textarea_0e8;
    unsigned int m_title_id_140;  /* 0x140: gppStringList index, DrawTitle */
    unsigned int m_detail_id_144; /* 0x144: gppStringList index, PopulateText */
};

// VTABLE: WIZ8 0x005efcc8
class W8AttributeInfoDialog005DFC70 : public W8StatInfoDialogBase005DF880 {
public:
    W8AttributeInfoDialog005DFC70(unsigned int uiIndex);
    virtual ~W8AttributeInfoDialog005DFC70() override;

private:
    unsigned int m_uiIndex; /* 0x148 */
};

// VTABLE: WIZ8 0x005efd48
class W8SecondaryAttributeInfoDialog005E0180 : public W8StatInfoDialogBase005DF880 {
public:
    W8SecondaryAttributeInfoDialog005E0180(unsigned int uiIndex);
    virtual ~W8SecondaryAttributeInfoDialog005E0180() override;

private:
    unsigned int m_uiIndex; /* 0x148 */
};

// VTABLE: WIZ8 0x005efd08
class W8SkillInfoDialog005EFD08 : public W8StatInfoDialogBase005DF880 {
public:
    W8SkillInfoDialog005EFD08(unsigned int skill, unsigned char first, unsigned char second,
                              unsigned char bonus);
    virtual ~W8SkillInfoDialog005EFD08() override;

protected:
    virtual unsigned char PopulateText() override;

private:
    unsigned int m_skill_148;
    unsigned char m_first_14c;
    unsigned char m_second_14d;
    unsigned char m_bonus_14e;
    unsigned char pad_14f;
};

static_assert(sizeof(W8StatInfoDialogBase005DF880) == 0x148,
              "W8StatInfoDialogBase005DF880_must_be_0x148");
static_assert(sizeof(W8AttributeInfoDialog005DFC70) == 0x14c,
              "W8AttributeInfoDialog005DFC70_must_be_0x14c");
static_assert(sizeof(W8SecondaryAttributeInfoDialog005E0180) == 0x14c,
              "W8SecondaryAttributeInfoDialog005E0180_must_be_0x14c");
static_assert(sizeof(W8SkillInfoDialog005EFD08) == 0x150,
              "W8SkillInfoDialog005EFD08_must_be_0x150");
