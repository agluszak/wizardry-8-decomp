#pragma once

#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/dialog_code/DialogButton.h"
#include "wiz8/dialog_code/DialogScrollBar.h"
#include "wiz8/dialog_code/DialogTextArea.h"
#include "wiz8/item_instance.h"
#include "wiz8/local_code/TextBuffer.h"

struct W8Character;

enum { W8_ASSAY_BUTTON_COUNT = 0x25, W8_ASSAY_TEXT_BUFFER_COUNT = 5 };

/* Dialog Code\AssayDialog.cpp. The constructor stores the inspected item and
   the character whose stats the requirement lines compare against. The live
   hull is PopulateText at 0x005D7310; CreateControls, Draw and the tab
   callbacks sit in the gaps around that hull. */
// VTABLE: WIZ8 0x005ef988
class W8AssayDialog : public W8DialogBase {
public:
    W8AssayDialog(W8ItemInstance* item, W8Character* character); /* 0x005D6FB0 */
    virtual ~W8AssayDialog() override;
    virtual int CreateControls() override;
    virtual void DestroyControls() override;
    virtual void Draw() override;
    virtual void OnRightButtonUp() override;
    virtual void OnMouseWheel(int delta) override;
    static void ScrollCallback(W8DialogScrollBar* scroll_bar, int first_visible_entry);

private:
    unsigned char PopulateText();             /* 0x005D7310 */
    unsigned char PopulateRequirements();     /* 0x005D8850 */
    unsigned char CreateTextBuffers();        /* 0x005D8FB0 */
    void SetProfessionIconsVisible(int show); /* 0x005D9330 */
    void SetRaceIconsVisible(int show);       /* 0x005D9460 */
    void ShowPrimaryTab();                    /* 0x005D9620 */
    void ShowSecondaryTab();                  /* 0x005D96A0 */
    static void PrimaryTabCallback(W8DialogButton* button);
    static void SecondaryTabCallback(W8DialogButton* button);

    W8DialogButton* m_buttons[W8_ASSAY_BUTTON_COUNT];         /* 0x54 */
    W8TextBuffer* m_text_buffers[W8_ASSAY_TEXT_BUFFER_COUNT]; /* 0xe8 */
    W8DialogScrollBar m_scroll_bar;                           /* 0xfc */
    W8ItemInstance* m_item;                                   /* 0x148 */
    unsigned char m_item_portrait_dirty;                      /* 0x14c */
    unsigned char unknown_14d[3];
    W8DialogTextArea m_text_area; /* 0x150 */
    W8Character* m_character;     /* 0x1a8 */
};
static_assert(sizeof(W8AssayDialog) == 0x1ac, "W8AssayDialog_size");
