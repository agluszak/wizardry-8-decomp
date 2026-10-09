#pragma once

#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/dialog_code/DialogButton.h"
#include "wiz8/dialog_code/DialogScrollBar.h"
#include "wiz8/dialog_code/DialogTextArea.h"

/* The monster information dialog for one monster location id. */
// VTABLE: WIZ8 0x005ef910
class W8MonsterInfoDialog : public W8DialogBase {
public:
    W8MonsterInfoDialog(int location_id);
    virtual ~W8MonsterInfoDialog() override;
    virtual int CreateControls() override;
    virtual void DestroyControls() override;
    virtual void Draw() override;
    virtual void OnRightButtonUp() override;
    virtual void OnMouseWheel(int delta) override;

private:
    bool PopulateText();
    static void ScrollCallback(W8DialogScrollBar* scroll_bar, int first_visible_entry);

    int m_location_id;
    W8DialogScrollBar m_scroll_bar;
    W8DialogButton m_button;
    W8DialogTextArea m_text_area;
};
W8_ABI_ASSERT(sizeof(W8MonsterInfoDialog) == 0x144, "W8MonsterInfoDialog_size");

/* The monster and statistic info dialogs' background. */
extern char* g_info_dialog_background;
