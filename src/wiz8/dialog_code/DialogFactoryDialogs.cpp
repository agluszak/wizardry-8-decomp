#include "wiz8/dialog_code/DialogBase.h"

// FUNCTION: WIZ8 0x005d97d0
W8Dialog005D97D0::W8Dialog005D97D0()
{
    int index;

    SetExtent(322, 111);
    SetBackground("Data\\Dialogs\\popup_splititem.sti", 2);
    for (index = 0; index < 6; ++index) {
        m_fields_54[index] = 0;
    }
    m_field_6c = 0;
    m_field_70 = 0;
    m_field_74 = 0;
    m_field_78 = 0;
    m_field_80 = 0;
    m_field_84 = 0;
    m_field_88 = 0;
    m_field_8c = 0;
    m_field_7c = 0;
}
