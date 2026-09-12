#include "wiz8/dialog_code/MonsterInfoDialog.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/fonts.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/video_object_catalog.h"
#include "Font.h"

static const char MONSTER_INFO_DIALOG_CPP[] =
    "C:\\Projects\\Wizardry 8\\Dialog Code\\MonsterInfoDialog.cpp";

// SYNTHETIC: WIZ8 0x005d5ee0
// W8MonsterInfoDialog::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005d5e30
W8MonsterInfoDialog::W8MonsterInfoDialog(int location_id) : m_location_id(location_id)
{
    SetOrigin(0x9c, 0x31);
    SetExtent(0x14a, 0x10e);
    SetBackground("Data\\Dialogs\\popup_monsterinfo.sti", 0);
}

// FUNCTION: WIZ8 0x005d5f00
W8MonsterInfoDialog::~W8MonsterInfoDialog()
{
    m_scroll_bar_58.DestroyControls();
    W8DialogBase::DestroyControls();
}

// FUNCTION: WIZ8 0x005d5f90
int W8MonsterInfoDialog::CreateControls()
{
    W8DialogBase::CreateControls();
    if (PopulateText() == 0) {
        m_error = 7;
        return 7;
    }

    W8DialogScrollBar::Resources resources;
    resources.arrows_path = "Data\\Main Interface\\main_scroll.sti";
    resources.track_path = "Data\\Dialogs\\popup_monsterinfo.sti";
    resources.track_frame = 1;
    resources.on_scroll = ScrollCallback;
    m_scroll_bar_58.CreateControls(&resources);
    int x = m_x;
    m_scroll_bar_58.SetLayout(x + 0x12b, m_y + 0x26, m_text_area_ec.GetTotalLineCount(), 0,
                              m_text_area_ec.GetLineHeight(), 0xb9);
    m_scroll_bar_58.m_owner = this;

    m_button_a4.Configure("Data\\Dialogs\\popup_confirmationbuttons.sti", 3, 0, 1, 4, 2,
                          DialogCloseButtonCallback, 0, 0, 0x7f, -1, 0, 0);
    m_button_a4.SetPosition(m_x + 0x11a, m_y + 0xe6);
    m_button_a4.m_owner_040 = this;
    return 0;
}

// FUNCTION: WIZ8 0x005d6e60
void W8MonsterInfoDialog::OnRightButtonUp()
{
    if (m_right_button_down) {
        m_keep_open = 0;
    }
}

// FUNCTION: WIZ8 0x005d6e70
void W8MonsterInfoDialog::OnMouseWheel(int delta)
{
    if (delta > 0) {
        for (int step = 0; step < delta; ++step) {
            m_scroll_bar_58.ScrollUp();
        }
    } else if (delta < 0) {
        for (int step = 0; step < -delta; ++step) {
            m_scroll_bar_58.ScrollDown();
        }
    }
}

// FUNCTION: WIZ8 0x005d6ec0
void W8MonsterInfoDialog::ScrollCallback(W8DialogScrollBar* scroll_bar, int first_visible_entry)
{
    int left;
    int top;
    int right;
    int bottom;
    W8MonsterInfoDialog* dialog = static_cast<W8MonsterInfoDialog*>(scroll_bar->m_owner);
    if (dialog != 0) {
        dialog->m_text_area_ec.SetFirstVisibleLine(first_visible_entry);
        left = dialog->m_x + 0x11;
        top = dialog->m_y + 0x26;
        right = left + 0x10e;
        bottom = top + 0xb9;
        InvalidateRegion(left, top, right, bottom, 0);
        BlitCatalogSurfaceRectTo16BPP(-0xe, left, top, right, bottom, 0x1b6, 0, 0);
        dialog->m_text_area_ec.m_dirty = 1;
    }
}

// FUNCTION: WIZ8 0x005dbde0
void W8MonsterInfoDialog::DestroyControls()
{
    m_scroll_bar_58.DestroyControls();
    W8DialogBase::DestroyControls();
}

// FUNCTION: WIZ8 0x005d6080
void W8MonsterInfoDialog::Draw()
{
    if ((m_dirty_flags & 1) != 0) {
        if (m_initialized == 0) {
            CreateControls();
        }
        m_text_area_ec.m_dirty = 1;
        m_scroll_bar_58.m_dirty = 1;
        m_button_a4.m_dirty = 1;
        W8DialogBase::Draw();
        SetFont(g_font_683660);
        SetFontObjectPalette16BPP(g_font_683660, g_colour_68ee08);
        unsigned int monster_list_index =
            MonsterGetIndexByLocationID(0x1f1, MONSTER_INFO_DIALOG_CPP, m_location_id, 1);
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
        W8WideChar* name = GetMonsterName(monster_info, 0, 0);
        INT16 width = StringPixLength(name, g_font_683660);
        gprintf(m_x + 0xe + (0x112 - width) / 2, m_y + 0x11, (unsigned short*)L"%s", name);
    }
    m_text_area_ec.Draw(0);
    m_scroll_bar_58.Draw(0);
    m_button_a4.Draw();
}
