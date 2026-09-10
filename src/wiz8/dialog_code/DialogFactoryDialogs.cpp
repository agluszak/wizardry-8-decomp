#include "wiz8/dialog_code/DialogFactoryDialogs.h"
#include "wiz8/dialog_code/DialogButton.h"
#include "wiz8/bringup_gates.h"
#include "wiz8/item_spawning.h"

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

/* The trigger-owned item picker. The thirteen same-sized button slots are
   allocated by CreateControls; the constructor only clears them. */
// FUNCTION: WIZ8 0x005cd710
W8Dialog005CD710::W8Dialog005CD710()
{
    int index;

    for (index = 0; index < 13; ++index) {
        m_buttons_74[index] = 0;
    }
    SetExtent(200, 100);
    SetOrigin(0x84, 0x50);
    SetBackground("Data\\Dialogs\\DialogBackground.sti", 0);
    m_dirty_flags |= 1;
    m_first_item_0a8 = 0;
}

// SYNTHETIC: WIZ8 0x005cd800
// W8Dialog005CD710::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005cd820
W8Dialog005CD710::~W8Dialog005CD710()
{
    int index;

    if (m_destroy_callback != 0) {
        m_destroy_callback(this);
        m_destroy_callback = 0;
    }
    W8DialogBase::DestroyControls();
    for (index = 0; index < 13; ++index) {
        if (m_buttons_74[index] != 0) {
            delete m_buttons_74[index];
            m_buttons_74[index] = 0;
        }
    }
}

/* The picker reports the fourth factory kind. */
// FUNCTION: WIZ8 0x005cf240
int W8Dialog005CD710::GetDialogType()
{
    return 4;
}

/* Move the trigger's whole item group into this picker, merging each item
   through the add path. */
// FUNCTION: WIZ8 0x005cf0c0
void W8Dialog005CD710::SetItemGroup005CF0C0(W8WorldItem* group)
{
    W8WorldItem* item;

    m_item_group_0ac = group;
    item = ItemInfoGroupGetNext(group);
    while (item != 0) {
        ItemInfoRemoveFromGroup(group, item);
        AddItem005CE210(item);
        item = ItemInfoGroupGetNext(group);
    }
}

/* Hand every picker item back to the trigger's group, keeping the order the
   picker displayed them in. */
// FUNCTION: WIZ8 0x005cf110
W8WorldItem* W8Dialog005CD710::ReturnItemsToGroup005CF110()
{
    if (items_54.GetCount() == 0) {
        return m_item_group_0ac;
    }
    do {
        W8WorldItem* item = *items_54.GetAt(0);
        items_54.RemoveAt(0);
        flags_64.RemoveAt(0);
        ItemInfoAddToGroup(m_item_group_0ac, item);
    } while (items_54.GetCount() != 0);
    return m_item_group_0ac;
}
