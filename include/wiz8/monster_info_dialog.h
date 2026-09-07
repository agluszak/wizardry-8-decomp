#pragma once

#include "wiz8/dialog_code/DialogBase.h"

class W8MonsterInfoDialog : public W8DialogBase005DC7A0 {
public:
    virtual ~W8MonsterInfoDialog() override;
    virtual int CreateControls() override;
    virtual void DestroyControls() override;
    virtual void Draw() override;
    virtual void OnRightButtonUp() override;
    virtual void OnMouseWheel(int delta) override;

private:
    void* m_constructor_argument_54;      /* 0x54 */
    W8DialogScrollBar005E0C40 m_scroll_bar_58; /* 0x58 */
    W8DialogButton005DB1B0 m_button_a4;       /* 0xa4 */
    W8DialogTextArea005D14D0 m_text_area_ec;  /* 0xec */
};                                      /* modeled minimum 0x144 */
