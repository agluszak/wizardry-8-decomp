#pragma once

#include "wiz8/character_skills.h"
#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/dialog_code/DialogButton.h"
#include "wiz8/dialog_code/DialogScrollBar.h"
#include "wiz8/dialog_code/DialogTextArea.h"

/* Shared storage for the attribute-information dialogs and the skill info
   dialog. */
// VTABLE: WIZ8 0x005efc88
class W8StatInfoDialogBase : public W8DialogBase {
public:
    W8StatInfoDialogBase();
    virtual ~W8StatInfoDialogBase() override;
    virtual int CreateControls() override;
    virtual void DestroyControls() override;
    virtual void Draw() override;
    virtual void OnRightButtonUp() override;
    virtual void OnMouseWheel(int delta) override;
    virtual void DrawTitle();
    virtual bool PopulateText();

protected:
    static void ScrollCallback(W8DialogScrollBar* scroll_bar, int first_visible_entry);

    W8DialogScrollBar scrollbar;
    W8DialogButton button;
    W8DialogTextArea textarea;
    unsigned int m_title_id;  /* gppStringList index, DrawTitle */
    unsigned int m_detail_id; /* gppStringList index, PopulateText */
};

// VTABLE: WIZ8 0x005efcc8
class W8AttributeInfoDialog : public W8StatInfoDialogBase {
public:
    W8AttributeInfoDialog(W8Attribute uiIndex);
    virtual ~W8AttributeInfoDialog() override;

private:
    W8Attribute m_uiIndex;
};

// VTABLE: WIZ8 0x005efd48
class W8SecondaryAttributeInfoDialog : public W8StatInfoDialogBase {
public:
    W8SecondaryAttributeInfoDialog(unsigned int uiIndex);
    virtual ~W8SecondaryAttributeInfoDialog() override;

private:
    unsigned int m_uiIndex;
};

// VTABLE: WIZ8 0x005efd08
class W8SkillInfoDialog : public W8StatInfoDialogBase {
public:
    W8SkillInfoDialog(W8Skill skill, bool first, bool second, bool bonus);
    virtual ~W8SkillInfoDialog() override;

protected:
    virtual bool PopulateText() override;

private:
    W8Skill m_skill;
    bool m_first;
    bool m_second;
    bool m_bonus;
    unsigned char pad_14f;
};

W8_ABI_ASSERT(sizeof(W8StatInfoDialogBase) == 0x148, "W8StatInfoDialogBase005DF880_must_be_0x148");
W8_ABI_ASSERT(sizeof(W8AttributeInfoDialog) == 0x14c,
              "W8AttributeInfoDialog005DFC70_must_be_0x14c");
W8_ABI_ASSERT(sizeof(W8SecondaryAttributeInfoDialog) == 0x14c,
              "W8SecondaryAttributeInfoDialog005E0180_must_be_0x14c");
W8_ABI_ASSERT(sizeof(W8SkillInfoDialog) == 0x150, "W8SkillInfoDialog005EFD08_must_be_0x150");

/* GppStringList indices naming each attribute/resistance row. */
extern unsigned short g_attr_table1[18];
