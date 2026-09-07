#pragma once

#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/dialog_code/DialogButton.h"
#include "wiz8/dialog_code/DialogScrollBar.h"
#include "wiz8/dialog_code/DialogTextArea.h"

class W8MonsterInfoDialog : public W8DialogBase {
public:
    virtual ~W8MonsterInfoDialog() override;
    virtual int CreateControls() override;
    virtual void DestroyControls() override;
    virtual void Draw() override;
    virtual void OnRightButtonUp() override;
    virtual void OnMouseWheel(int delta) override;

private:
    void* m_constructor_argument_54;      /* 0x54 */
    W8DialogScrollBar m_scroll_bar_58; /* 0x58 */
    W8DialogButton m_button_a4;       /* 0xa4 */
    W8DialogTextArea m_text_area_ec;  /* 0xec */
};                                      /* modeled minimum 0x144 */
