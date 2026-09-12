#include "wiz8/dialog_code/AssayDialog.h"
#include "wiz8/character.h"
#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/item_video_object_vector.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/screen_state.h"
#include "wiz8/video_object_catalog.h"

// GLOBAL: WIZ8 0x0064f908
W8DialogScrollBar::Resources g_assay_scroll_resources = {
    "Data\\Main Interface\\main_scroll.sti",
    "Data\\Dialogs\\popup_iteminfo.sti",
    4,
    W8AssayDialog::ScrollCallback,
};
// GLOBAL: WIZ8 0x0064f918
int g_assay_scroll_x_offset = 0x160;
// GLOBAL: WIZ8 0x0064f91c
int g_assay_scroll_y_offset = 0x48;
// GLOBAL: WIZ8 0x0064fa98
W8ControlsRect g_assay_text_area_offsets = {0x48, 0x4a, 0x156, 0xef};

// SYNTHETIC: WIZ8 0x005d7070
// W8AssayDialog::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005d6fb0
W8AssayDialog::W8AssayDialog(W8ItemInstance* item, W8Character* character)
{
    int index;

    SetExtent(0x180, 0x11d);
    SetBackground("Data\\Dialogs\\popup_iteminfo.sti", 0);
    for (index = 0; index < W8_ASSAY_BUTTON_COUNT; ++index) {
        m_buttons[index] = 0;
    }
    for (index = 0; index < W8_ASSAY_TEXT_BUFFER_COUNT; ++index) {
        m_text_buffers[index] = 0;
    }
    m_item_portrait_dirty = 0;
    m_character = character;
    m_item = item;
}

// FUNCTION: WIZ8 0x005d7090
W8AssayDialog::~W8AssayDialog()
{
    int index;

    W8DialogBase::DestroyControls();
    m_scroll_bar.DestroyControls();
    for (index = 0; index < W8_ASSAY_BUTTON_COUNT; ++index) {
        if (m_buttons[index] != 0) {
            delete m_buttons[index];
            m_buttons[index] = 0;
        }
    }
    for (index = 0; index < W8_ASSAY_TEXT_BUFFER_COUNT; ++index) {
        if (m_text_buffers[index] != 0) {
            delete m_text_buffers[index];
            m_text_buffers[index] = 0;
        }
    }
    NoOp();
}

// FUNCTION: WIZ8 0x005d7160
int W8AssayDialog::CreateControls()
{
    int index;

    W8DialogBase::CreateControls();
    m_item_portrait_dirty = 1;
    if (PopulateText() == 0) {
        m_error = 7;
        return 7;
    }
    if (m_scroll_bar.CreateControls(&g_assay_scroll_resources) != 0) {
        m_scroll_bar.SetLayout(g_assay_scroll_x_offset + m_x, g_assay_scroll_y_offset + m_y,
                               m_text_area.GetTotalLineCount(), 0, m_text_area.GetLineHeight(),
                               g_assay_text_area_offsets.bottom - g_assay_text_area_offsets.top);
        m_scroll_bar.m_owner = this;
        if (PopulateRequirements() == 0) {
            m_error = 7;
            return 7;
        }
        if (CreateTextBuffers() != 0) {
            m_buttons[0]->SetVisible(g_flag_00685070 != 0);
            SetProfessionIconsVisible(g_flag_00685070);
            m_buttons[1]->SetVisible(g_flag_00685070 == 0);
            SetRaceIconsVisible(g_flag_00685070 == 0);
            if (g_flag_00685070 != 0) {
                m_buttons[2]->SetPressed(true);
                return 0;
            }
            m_buttons[3]->SetPressed(true);
            return 0;
        }
        for (index = 0; index < W8_ASSAY_BUTTON_COUNT; ++index) {
            if (m_buttons[index] != 0) {
                delete m_buttons[index];
                m_buttons[index] = 0;
            }
        }
    }
    m_error = 7;
    return 7;
}

// FUNCTION: WIZ8 0x005d72b0
void W8AssayDialog::DestroyControls()
{
    int index;

    W8DialogBase::DestroyControls();
    m_scroll_bar.DestroyControls();
    for (index = 0; index < W8_ASSAY_BUTTON_COUNT; ++index) {
        if (m_buttons[index] != 0) {
            delete m_buttons[index];
            m_buttons[index] = 0;
        }
    }
    for (index = 0; index < W8_ASSAY_TEXT_BUFFER_COUNT; ++index) {
        if (m_text_buffers[index] != 0) {
            delete m_text_buffers[index];
            m_text_buffers[index] = 0;
        }
    }
}

// FUNCTION: WIZ8 0x005d9200
void W8AssayDialog::Draw()
{
    int index;

    if ((m_dirty_flags & 1) != 0) {
        if (m_initialized == 0) {
            CreateControls();
        }
        m_item_portrait_dirty = 1;
        m_text_area.m_dirty = 1;
        m_scroll_bar.m_dirty = 1;
        for (index = 0; index < W8_ASSAY_BUTTON_COUNT; ++index) {
            m_buttons[index]->m_dirty = 1;
        }
        for (index = 0; index < W8_ASSAY_TEXT_BUFFER_COUNT; ++index) {
            m_text_buffers[index]->SetGeometryDirty();
        }
        W8DialogBase::Draw();
    }
    if (m_item_portrait_dirty != 0) {
        DrawCatalogImageAndInvalidate(
            -0xe, g_item_video_objects_68ec68.GetOrCreateVideoObject(m_item->item_id), 0, 0,
            m_x + 0x45, m_y + 0xe, 2, 0);
        if (m_item->identified == 0) {
            DrawCatalogImageAndInvalidate(-0xe, 0x11b, 0, 0, m_x + 0x45, m_y + 0xe, 2, 0);
        }
        m_item_portrait_dirty = 0;
    }
    for (index = 0; index < W8_ASSAY_BUTTON_COUNT; ++index) {
        if (m_buttons[index] != 0) {
            m_buttons[index]->Draw();
        }
    }
    for (index = 0; index < W8_ASSAY_TEXT_BUFFER_COUNT; ++index) {
        if (m_text_buffers[index] != 0) {
            m_text_buffers[index]->RenderToTarget(0, 0, -0xe);
        }
    }
    m_text_area.Draw(0);
    m_scroll_bar.Draw(0);
}

/* Same folded body as W8MonsterInfoDialog::OnRightButtonUp at 0x005D6E60;
   /OPT:NOICF emits this copy. */
void W8AssayDialog::OnRightButtonUp()
{
    if (m_right_button_down) {
        m_keep_open = 0;
    }
}

// FUNCTION: WIZ8 0x005d9720
void W8AssayDialog::OnMouseWheel(int delta)
{
    if (delta > 0) {
        for (int step = 0; step < delta; ++step) {
            m_scroll_bar.ScrollUp();
        }
    } else if (delta < 0) {
        for (int step = 0; step < -delta; ++step) {
            m_scroll_bar.ScrollDown();
        }
    }
}

// FUNCTION: WIZ8 0x005d9620
void W8AssayDialog::ShowPrimaryTab()
{
    if (m_buttons[2]->IsPressed() == 0) {
        m_buttons[2]->SetPressed(true);
        m_buttons[2]->m_dirty = 1;
    }
    if (m_buttons[3]->IsPressed() != 0) {
        m_buttons[3]->SetPressed(false);
        m_buttons[3]->m_dirty = 1;
    }
    g_flag_00685070 = 1;
    m_buttons[0]->SetVisible(true);
    SetProfessionIconsVisible(1);
    m_buttons[1]->SetVisible(false);
    SetRaceIconsVisible(0);
    m_buttons[0]->m_dirty = 1;
    m_buttons[1]->m_dirty = 1;
    m_buttons[3]->m_dirty = 1;
}

// FUNCTION: WIZ8 0x005d96a0
void W8AssayDialog::ShowSecondaryTab()
{
    if (m_buttons[3]->IsPressed() == 0) {
        m_buttons[3]->SetPressed(true);
        m_buttons[3]->m_dirty = 1;
    }
    if (m_buttons[2]->IsPressed() != 0) {
        m_buttons[2]->SetPressed(false);
        m_buttons[2]->m_dirty = 1;
    }
    g_flag_00685070 = 0;
    m_buttons[0]->SetVisible(false);
    SetProfessionIconsVisible(0);
    m_buttons[1]->SetVisible(true);
    SetRaceIconsVisible(1);
    m_buttons[0]->m_dirty = 1;
    m_buttons[1]->m_dirty = 1;
    m_buttons[2]->m_dirty = 1;
}

// FUNCTION: WIZ8 0x005d9760
void W8AssayDialog::PrimaryTabCallback(W8DialogButton* button)
{
    if (button != 0) {
        static_cast<W8AssayDialog*>(button->m_owner_040)->ShowPrimaryTab();
    }
}

// FUNCTION: WIZ8 0x005d9780
void W8AssayDialog::SecondaryTabCallback(W8DialogButton* button)
{
    if (button != 0) {
        static_cast<W8AssayDialog*>(button->m_owner_040)->ShowSecondaryTab();
    }
}

// FUNCTION: WIZ8 0x005d97a0
void W8AssayDialog::ScrollCallback(W8DialogScrollBar* scroll_bar, int first_visible_entry)
{
    W8AssayDialog* dialog;
    if (scroll_bar != 0) {
        dialog = static_cast<W8AssayDialog*>(scroll_bar->m_owner);
        dialog->m_text_area.SetFirstVisibleLine(first_visible_entry);
        dialog->m_buttons[4]->m_dirty = 1;
        dialog->m_text_area.m_dirty = 1;
    }
}
