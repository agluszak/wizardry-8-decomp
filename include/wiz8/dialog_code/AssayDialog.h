#pragma once

#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/dialog_code/DialogButton.h"
#include "wiz8/dialog_code/DialogScrollBar.h"
#include "wiz8/dialog_code/DialogTextArea.h"
#include "wiz8/layouts/item_instance.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/TextBuffer.h"

struct W8Character;

enum { W8_ASSAY_BUTTON_COUNT = 0x25, W8_ASSAY_TEXT_BUFFER_COUNT = 5 };

/* The item information dialog. It keeps the inspected item and the character
   whose stats the requirement lines compare against. */
// VTABLE: WIZ8 0x005ef988
class W8AssayDialog : public W8DialogBase {
public:
    W8AssayDialog(W8ItemInstance* item, W8Character* character);
    virtual ~W8AssayDialog() override;
    virtual int CreateControls() override;
    virtual void DestroyControls() override;
    virtual void Draw() override;
    virtual void OnRightButtonUp() override;
    virtual void OnMouseWheel(int delta) override;
    static void ScrollCallback(W8DialogScrollBar* scroll_bar, int first_visible_entry);

private:
    bool PopulateText();
    bool PopulateRequirements();
    bool CreateTextBuffers();
    void SetProfessionIconsVisible(bool show);
    void SetRaceIconsVisible(bool show);
    void ShowPrimaryTab();
    void ShowSecondaryTab();
    static void PrimaryTabCallback(W8DialogButton* button);
    static void SecondaryTabCallback(W8DialogButton* button);

    W8DialogButton* m_buttons[W8_ASSAY_BUTTON_COUNT];
    W8TextBuffer* m_text_buffers[W8_ASSAY_TEXT_BUFFER_COUNT];
    W8DialogScrollBar m_scroll_bar;
    W8ItemInstance* m_item;
    bool m_item_portrait_dirty;
    W8DialogTextArea m_text_area;
    W8Character* m_character;
};
W8_ABI_ASSERT(sizeof(W8AssayDialog) == 0x1ac, "W8AssayDialog_size");
extern unsigned short g_equip_class_name_ids[32];
extern wchar_t g_assay_format[];
