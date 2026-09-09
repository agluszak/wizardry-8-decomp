#include "wiz8/dialog_code/DialogFactoryDialogs.h"
#include "wiz8/dialog_code/DialogButton.h"
#include "wiz8/bringup_gates.h"

// FUNCTION: WIZ8 0x005cd700
int W8Dialog005CBB40::GetDialogType()
{
    return 3;
}

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

// FUNCTION: WIZ8 0x005d9ac0
void W8Dialog005D97D0::DestroyControls()
{
    int index;
    int* field;

    W8DialogBase::DestroyControls();
    for (index = 0; index < 6; ++index) {
        if (m_fields_54[index] != 0) {
            delete reinterpret_cast<W8DialogButton*>(m_fields_54[index]);
            m_fields_54[index] = 0;
        }
    }
    field = &m_field_6c;
    for (index = 0; index < 3; ++index) {
        if (*field != 0) {
            delete reinterpret_cast<W8DialogButton*>(*field);
            *field = 0;
        }
        ++field;
    }
    if (m_field_78 != 0) {
        NoOp();
        ::operator delete(reinterpret_cast<void*>(m_field_78));
        m_field_78 = 0;
    }
}

// FUNCTION: WIZ8 0x005d9930
W8Dialog005D97D0::~W8Dialog005D97D0()
{
    int index;
    int* field;

    W8DialogBase::DestroyControls();
    for (index = 0; index < 6; ++index) {
        if (m_fields_54[index] != 0) {
            delete reinterpret_cast<W8DialogButton*>(m_fields_54[index]);
            m_fields_54[index] = 0;
        }
    }
    field = &m_field_6c;
    for (index = 0; index < 3; ++index) {
        if (*field != 0) {
            delete reinterpret_cast<W8DialogButton*>(*field);
            *field = 0;
        }
        ++field;
    }
    if (m_field_78 != 0) {
        NoOp();
        ::operator delete(reinterpret_cast<void*>(m_field_78));
        m_field_78 = 0;
    }
}
