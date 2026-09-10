#include "wiz8/dialog_code/DialogFactoryDialogs.h"
#include "wiz8/dialog_code/DialogButton.h"
#include "wiz8/bringup_gates.h"
#include "wiz8/cursor.h"
#include "wiz8/utility.h"
#include "wiz8/fonts.h"
#include "wiz8/item_spawning.h"
#include "wiz8/item_video_object_vector.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/video_object_catalog.h"

#include "Font.h"
#include "input.h"
#include "mousesystem_macros.h"

extern unsigned char Function577A40(void);

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

/* Merge one item into the picker. Identification comes first, then the item is
   placed next to the existing entries with the same equipment class and, among
   those, the same unidentified name, keeping each name run ordered by item id.
   Both vectors grow five at a time on this insertion path. A nil result means
   the caller's group handed in nothing. */
// FUNCTION: WIZ8 0x005ce210
int W8Dialog005CD710::AddItem005CE210(W8WorldItem* item)
{
    W8ItemInstance* instance = &item->item;

    if (instance != 0) {
        int index;

        PartyAttemptsToIdentifyItem(instance, 0);
        for (index = 0; index < items_54.GetCount(); ++index) {
            W8WorldItem* other = *items_54.GetAt(index);
            if (ItemsShareEquipClass(instance, &other->item)) {
                break;
            }
        }
        if (index < items_54.GetCount()) {
            W8WorldItem* other = *items_54.GetAt(index);
            while (index < items_54.GetCount() &&
                   !ItemsShareUnidentifiedName(instance, &other->item)) {
                other = *items_54.GetAt(index);
                ++index;
            }
            if (index < items_54.GetCount()) {
                while (index < items_54.GetCount() &&
                       other->item.item_id != instance->item_id) {
                    other = *items_54.GetAt(index);
                    ++index;
                }
                if (index < items_54.GetCount()) {
                    items_54.InsertAt(index, item);
                    flags_64.InsertAt(index, 0);
                    return -1;
                }
            }
        }
        items_54.Add(item);
        flags_64.Add(0);
    }
    return -1;
}

/* Poll the input queue for this picker. Button presses over a party portrait
   belong to the party, not the picker, and the two button-down kinds first ask
   the screen helper whether it wants them. Every handled button is forwarded
   to the SGP mouse system; the wheel scrolls the picker's first visible row in
   four-row steps and sets the redraw bit. Anything else goes to the picker's
   own event handler. */
// FUNCTION: WIZ8 0x005cef00
unsigned char W8Dialog005CD710::ProcessInput()
{
    W8ScreenPoint mouse;
    InputAtom input;

    GetScreenPoint004284F0(&mouse);
    MSYS_SGP_Mouse_Handler_Hook(
        MOUSE_POS, mouse.x, mouse.y, gfLeftButtonState, gfRightButtonState);
    while (DequeueEvent(&input)) {
        if ((input.usEvent == LEFT_BUTTON_DOWN ||
             input.usEvent == RIGHT_BUTTON_DOWN) &&
            Function577A40() != 0) {
            continue;
        }
        if (HitTestPartyPortrait(&input) != 0) {
            continue;
        }
        switch (input.usEvent) {
        case LEFT_BUTTON_DOWN:
        case LEFT_BUTTON_REPEAT:
            MSYS_SGP_Mouse_Handler_Hook(
                LEFT_BUTTON_DOWN, mouse.x, mouse.y,
                gfLeftButtonState, gfRightButtonState);
            break;
        case LEFT_BUTTON_UP:
            MSYS_SGP_Mouse_Handler_Hook(
                LEFT_BUTTON_UP, mouse.x, mouse.y,
                gfLeftButtonState, gfRightButtonState);
            break;
        case RIGHT_BUTTON_DOWN:
            MSYS_SGP_Mouse_Handler_Hook(
                RIGHT_BUTTON_DOWN, mouse.x, mouse.y,
                gfLeftButtonState, gfRightButtonState);
            break;
        case RIGHT_BUTTON_UP:
            MSYS_SGP_Mouse_Handler_Hook(
                RIGHT_BUTTON_UP, mouse.x, mouse.y,
                gfLeftButtonState, gfRightButtonState);
            break;
        case MOUSE_WHEEL: {
            short delta = GetMouseWheelDeltaValue(input.uiParam);
            int first_item = m_first_item_0a8 - delta;
            if (items_54.GetCount() < 5) {
                m_first_item_0a8 = 0;
            }
            else if (first_item >= 0 &&
                     first_item <= items_54.GetCount() - 4) {
                m_first_item_0a8 = first_item;
                m_dirty_flags |= 1;
            }
            break;
        }
        default:
            HandleInputEvent005CEC20(&input);
            break;
        }
    }
    return m_field_41;
}

/* Draw the picker. Up to four item rows share the five buttons at the top of
   the slot array; the first dirty row draws the item's catalogue image, its
   display name and its weight. A short list hides the four scroll buttons,
   otherwise they are placed around the row area and the scroll bar tracks the
   first visible item. */
// FUNCTION: WIZ8 0x005cdc70
void W8Dialog005CD710::Draw()
{
    int count = items_54.GetCount();
    int visible_rows;
    if (count < 3) {
        visible_rows = 2;
    }
    else if (count > 3) {
        visible_rows = 4;
    }
    else {
        visible_rows = count;
    }

    if (m_initialized == 0) {
        CreateControls();
    }
    if (g_in_combat_00683f94 != 0) {
        m_buttons_74[2]->SetEnabled(0);
    }
    if ((m_dirty_flags & 1) != 0) {
        for (int index = 0; index < 13; ++index) {
            m_buttons_74[index]->m_dirty = 1;
        }
        SetExtent(m_buttons_74[4]->GetWidth() + 0xe,
                  m_buttons_74[5]->GetHeight() * visible_rows +
                      m_buttons_74[4]->GetHeight() + 0xe);
        W8DialogBase::Draw();
    }

    if (count > 4) {
        m_buttons_74[12]->SetVisible(1);
        m_buttons_74[9]->SetVisible(1);
        m_buttons_74[10]->SetVisible(1);
        m_buttons_74[11]->SetVisible(1);
        if (m_buttons_74[12]->m_dirty != 0) {
            m_buttons_74[9]->m_dirty = 1;
            m_buttons_74[10]->m_dirty = 1;
            m_buttons_74[11]->m_dirty = 1;
        }

        m_buttons_74[12]->SetPosition(
            m_x + m_width - m_buttons_74[12]->GetWidth() - 9, m_y + 7);
        m_buttons_74[12]->Draw();
        m_buttons_74[9]->SetPosition(m_buttons_74[12]->GetX() + 4,
                                     m_buttons_74[12]->GetY() + 3);
        m_buttons_74[9]->Draw();
        m_buttons_74[10]->SetPosition(
            m_buttons_74[9]->GetX(),
            m_buttons_74[12]->GetY() + m_buttons_74[12]->GetHeight() - 4 -
                m_buttons_74[10]->GetHeight());
        m_buttons_74[10]->Draw();

        int progress = 0;
        if (m_first_item_0a8 != 0) {
            progress = items_54.GetCount();
            if (m_first_item_0a8 + 4 < items_54.GetCount()) {
                progress = m_first_item_0a8 + 2;
            }
        }
        int travel = m_buttons_74[12]->GetHeight() -
                     m_buttons_74[9]->GetHeight() -
                     m_buttons_74[10]->GetHeight() -
                     m_buttons_74[11]->GetHeight();
        m_buttons_74[11]->SetPosition(
            m_buttons_74[9]->GetX(),
            m_buttons_74[9]->GetHeight() + m_buttons_74[12]->GetY() +
                travel * progress / items_54.GetCount() + 1);
        m_buttons_74[11]->Draw();
    }
    else {
        m_buttons_74[12]->SetVisible(0);
        m_buttons_74[9]->SetVisible(0);
        m_buttons_74[10]->SetVisible(0);
        m_buttons_74[11]->SetVisible(0);
    }

    m_buttons_74[4]->SetPosition(
        m_x + 7, m_y + m_height - m_buttons_74[4]->GetHeight() - 5);
    m_buttons_74[4]->Draw();
    m_buttons_74[3]->SetPosition(
        m_buttons_74[4]->GetX() - m_buttons_74[3]->GetWidth() +
            m_buttons_74[4]->GetWidth() - 1,
        m_buttons_74[4]->GetY());
    m_buttons_74[3]->Draw();
    m_buttons_74[2]->SetPosition(
        m_buttons_74[3]->GetX() - m_buttons_74[2]->GetWidth(),
        m_buttons_74[4]->GetY());
    m_buttons_74[2]->Draw();
    m_buttons_74[1]->SetPosition(
        m_buttons_74[2]->GetX() - m_buttons_74[1]->GetWidth(),
        m_buttons_74[4]->GetY());
    m_buttons_74[1]->Draw();
    m_buttons_74[0]->SetPosition(
        m_buttons_74[1]->GetX() - m_buttons_74[0]->GetWidth(),
        m_buttons_74[4]->GetY());
    m_buttons_74[0]->Draw();
    RefreshScrollButtons005CE420();

    for (int row = 0; row < visible_rows; ++row) {
        W8DialogButton* button = m_buttons_74[5 + row];

        button->SetVisible(1);
        button->SetPosition(m_x + 7,
                            m_y + button->GetHeight() * row + 7);
        if (button->m_dirty == 0) {
            continue;
        }
        button->Draw();
        if (items_54.GetCount() == 0) {
            continue;
        }
        int item_index = m_first_item_0a8 + row;
        if (item_index >= items_54.GetCount()) {
            continue;
        }
        W8WorldItem* world_item = *items_54.GetAt(item_index);
        W8ItemInstance* item = &world_item->item;
        int video_object =
            g_item_video_objects_68ec68.GetOrCreateVideoObject(item->item_id);
        DrawCatalogImage(-0xe, video_object, 0, 0, button->GetX() + 2,
                         button->GetY() + 2, 2, 0);
        SetFont(g_wiz_text_font_683640);
        if (item->stack_count > 1) {
            W8WideChar* name = GetItemDisplayName(item);
            gprintf(button->GetX() + 0x3c, button->GetY() + 6,
                    (unsigned short*)L"%s (%d)", name, item->stack_count);
        }
        else {
            W8WideChar* name = GetItemDisplayName(item);
            gprintf(button->GetX() + 0x3c, button->GetY() + 6,
                    (unsigned short*)name);
        }
        unsigned short weight = g_item_records[item->item_id].weight;
        gprintf(button->GetX() + 0x3c,
                button->GetY() + GetFontHeight(g_wiz_text_font_683640) + 6,
                (unsigned short*)L"%4.1f lbs",
                (double)((float)weight * 0.1f));
    }
}

/* Create the base controls first, then the thirteen button slots. A failed
   button allocation is reported as the dialog's own error 7. */
// FUNCTION: WIZ8 0x005cdc10
int W8Dialog005CD710::CreateControls()
{
    if (W8DialogBase::CreateControls() != 0) {
        return m_error;
    }
    if (CreateButtons005CD8D0() == 0) {
        m_error = 7;
        return 7;
    }
    return 0;
}

/* Release the base controls and every allocated button slot. */
// FUNCTION: WIZ8 0x005cdc40
void W8Dialog005CD710::DestroyControls()
{
    int index;

    W8DialogBase::DestroyControls();
    for (index = 0; index < 13; ++index) {
        if (m_buttons_74[index] != 0) {
            delete m_buttons_74[index];
            m_buttons_74[index] = 0;
        }
    }
}
