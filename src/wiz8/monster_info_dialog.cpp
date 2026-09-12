#include "wiz8/monster_info_dialog.h"

// Primary vtable slot 12.
// FUNCTION: WIZ8 0x005d6e60
void W8MonsterInfoDialog::OnRightButtonUp()
{
    if (m_right_button_down) {
        m_keep_open = 0;
    }
}

// Primary vtable slot 2.
// FUNCTION: WIZ8 0x005dbde0
void W8MonsterInfoDialog::DestroyControls()
{
    m_scroll_bar_58.DestroyControls();
    W8DialogBase::DestroyControls();
}
