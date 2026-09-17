#pragma once

#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/dialog_code/DialogButton.h"
#include "wiz8/dialog_code/DialogScrollBar.h"
#include "wiz8/dialog_code/DialogTextArea.h"
#include "wiz8/layouts/gameplay_databases.h"

enum { W8_DIALOG_ATTRIBUTE_COUNT = 7 };

struct W8AttributeMinimums {
    int values[W8_DIALOG_ATTRIBUTE_COUNT];
};

/* One row supplies the two string ids and the catalog image index copied into
   a profession or race information dialog. */
struct W8ProfRaceInfoRow {
    unsigned int name_id;
    unsigned int detail_id;
    unsigned int image_id;
};

extern W8AttributeMinimums g_profession_attribute_minimums[];
extern W8AttributeMinimums g_race_attribute_minimums[];
extern W8ProfRaceInfoRow g_profession_info_rows[W8_PROFESSION_COUNT];
extern W8ProfRaceInfoRow g_race_info_rows[16];

/* Dialog Code\ProfRaceInfoDialog.cpp shared shell: the constructor fixes the
   popup frame and the derived dialogs fill the string ids, the attribute
   minimums and the catalog image. DrawCatalogImageAndInvalidate proves that
   +0x54/+0x58 are the object and image operands of the header icon, not
   gppStringList indices. CreateControls dispatches the slot-14 PopulateText
   virtual, which the base leaves at text-area configuration and the derived
   dialogs extend with their entries. */
// VTABLE: WIZ8 0x005efbc0
class W8ProfRaceInfoDialogBase : public W8DialogBase {
public:
    W8ProfRaceInfoDialogBase();
    /* Header-visible: the derived destructors emit this body inline rather
       than tail-calling the standalone copy the base deleting destructor
       keeps at 0x005DEBB0. */
    // FUNCTION: WIZ8 0x005DEBB0
    virtual ~W8ProfRaceInfoDialogBase() override
    {
        m_scroll_bar_084.DestroyControls();
        W8DialogBase::DestroyControls();
    }
    virtual int CreateControls() override;
    virtual void DestroyControls() override;
    virtual void Draw() override;
    virtual void OnRightButtonUp() override;
    virtual void OnMouseWheel(int delta) override;

private:
    virtual unsigned char PopulateText();
    static void ScrollCallback(W8DialogScrollBar* scroll_bar, int first_visible_entry);
    /* Renders one text line at a dialog-relative rectangle through a scratch
       W8TextBuffer. */
    void DrawTextLine(unsigned int layout_mode, int left, int top, int width, int height,
                      const wchar_t* text, int font);

protected:
    unsigned int m_uiTitleId;                  /* 0x054: video object catalog id */
    unsigned int m_uiSummaryId;                /* 0x058: image inside the catalog object */
    unsigned int m_uiNameId;                   /* 0x05c: gppStringList index */
    unsigned int m_uiHeadingId;                /* 0x060: gppStringList index */
    unsigned int m_uiDetailId;                 /* 0x064: gppStringList index */
    int m_minimums[W8_DIALOG_ATTRIBUTE_COUNT]; /* 0x068 */
    W8DialogScrollBar m_scroll_bar_084;
    W8DialogButton m_button_0d0;
    W8DialogTextArea m_text_area_118;
};

// VTABLE: WIZ8 0x005efbfc
class W8ProfessionInfoDialog : public W8ProfRaceInfoDialogBase {
public:
    W8ProfessionInfoDialog(unsigned int uiIndex);
    virtual ~W8ProfessionInfoDialog() override;

private:
    virtual unsigned char PopulateText() override;

    unsigned int m_uiIndex; /* 0x170 */
};

// VTABLE: WIZ8 0x005efc38
class W8RaceInfoDialog : public W8ProfRaceInfoDialogBase {
public:
    W8RaceInfoDialog(unsigned int uiIndex);
    virtual ~W8RaceInfoDialog() override;

private:
    virtual unsigned char PopulateText() override;

    unsigned int m_uiIndex; /* 0x170 */
};

static_assert(sizeof(W8AttributeMinimums) == 0x1c, "W8AttributeMinimums_must_be_0x1c");
static_assert(sizeof(W8ProfRaceInfoRow) == 0x0c, "W8ProfRaceInfoRow_must_be_0x0c");
static_assert(sizeof(W8ProfRaceInfoDialogBase) == 0x170, "W8ProfRaceInfoDialogBase_must_be_0x170");
static_assert(sizeof(W8ProfessionInfoDialog) == 0x174, "W8ProfessionInfoDialog_must_be_0x174");
static_assert(sizeof(W8RaceInfoDialog) == 0x174, "W8RaceInfoDialog_must_be_0x174");
