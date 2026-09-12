#pragma once

#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/dialog_code/DialogButton.h"
#include "wiz8/dialog_code/DialogScrollBar.h"
#include "wiz8/dialog_code/DialogTextArea.h"

/* Dialog Code\MonsterInfoDialog.cpp. The constructor stores a monster
   location id; Draw and PopulateText pass that id to
   MonsterGetIndexByLocationID. The live hull is Draw and PopulateText
   (0x005D6080-0x005D6160). Constructor, destructor, CreateControls,
   OnRightButtonUp, OnMouseWheel and DestroyControls sit in the gaps around
   that hull and remain in this class file because they are its methods. */
// VTABLE: WIZ8 0x005ef910
class W8MonsterInfoDialog : public W8DialogBase {
public:
    W8MonsterInfoDialog(int location_id); /* 0x005D5E30 */
    virtual ~W8MonsterInfoDialog() override;
    virtual int CreateControls() override;
    virtual void DestroyControls() override;
    virtual void Draw() override;
    virtual void OnRightButtonUp() override;
    virtual void OnMouseWheel(int delta) override;

private:
    unsigned char PopulateText(); /* 0x005D6160 */
    static void CloseButtonCallback(W8DialogButton* button);
    static void ScrollCallback(W8DialogScrollBar* scroll_bar, int first_visible_entry);

    int m_location_id;                 /* 0x54 */
    W8DialogScrollBar m_scroll_bar_58; /* 0x58 */
    W8DialogButton m_button_a4;        /* 0xa4 */
    W8DialogTextArea m_text_area_ec;   /* 0xec */
};
static_assert(sizeof(W8MonsterInfoDialog) == 0x144, "W8MonsterInfoDialog_size");
