#pragma once

#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/dialog_code/DialogButton.h"
#include "wiz8/dialog_code/DialogScrollBar.h"
#include "wiz8/dialog_code/DialogTextArea.h"
#include "wiz8/engine_code/game_timer.h"

// VTABLE: WIZ8 0x005efab0
class W8SpellInfoDialog005EFAB0 : public W8DialogBase {
public:
    W8SpellInfoDialog005EFAB0(unsigned int spell); /* 0x005DBB60 */
    virtual ~W8SpellInfoDialog005EFAB0() override;

private:
    unsigned int m_spell_054;
    W8DialogScrollBar m_scroll_bar_058;
    W8DialogButton m_button_0a4;
    W8DialogTextArea m_text_area_0ec;
    W8GameTimer m_timer_144;
    unsigned int m_value_168;
};
static_assert(sizeof(W8SpellInfoDialog005EFAB0) == 0x16c,
              "W8SpellInfoDialog005EFAB0_size");
