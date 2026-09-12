#include "wiz8/dialog_code/MonsterInfoDialog.h"
#include "wiz8/fonts.h"
#include "wiz8/local_code/MonsterManager.h"
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

// FUNCTION: WIZ8 0x005d6e60
void W8MonsterInfoDialog::OnRightButtonUp()
{
    if (m_right_button_down) {
        m_keep_open = 0;
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
