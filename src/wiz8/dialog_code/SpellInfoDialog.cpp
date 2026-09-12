#include "wiz8/dialog_code/SpellInfoDialog.h"
#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/fonts.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/magic.h"
#include "wiz8/screen_state.h"
#include "wiz8/utility.h"
#include "wiz8/video_object_catalog.h"
#include "Font.h"

// GLOBAL: WIZ8 0x0064fccc
const char* g_spell_info_background_path = "Data\\Dialogs\\popup_spellinfo.sti";
// GLOBAL: WIZ8 0x0061a128
const wchar_t g_format_d_s_0061a128[] = L"%d %s";

// SYNTHETIC: WIZ8 0x005dbc30
// W8SpellInfoDialog::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005dbb60
W8SpellInfoDialog::W8SpellInfoDialog(unsigned int spell) : m_spell_054(spell), m_timer_144(0.05f, 1)
{
    m_animation_frame = 0;
    SetOrigin(0x9c, 0x5a);
    SetExtent(0x14a, 0x12c);
    SetBackground(g_spell_info_background_path, 0);
}

// FUNCTION: WIZ8 0x005dbc50
W8SpellInfoDialog::~W8SpellInfoDialog()
{
    m_scroll_bar_058.DestroyControls();
    W8DialogBase::DestroyControls();
    NoOp();
}

// FUNCTION: WIZ8 0x005dbcf0
int W8SpellInfoDialog::CreateControls()
{
    W8DialogBase::CreateControls();
    if (PopulateText() == 0) {
        m_error = 7;
        return 7;
    }

    W8DialogScrollBar::Resources resources;
    resources.arrows_path = "Data\\Main Interface\\main_scroll.sti";
    resources.track_path = g_spell_info_background_path;
    resources.track_frame = 1;
    resources.on_scroll = ScrollCallback;
    m_scroll_bar_058.CreateControls(&resources);
    int x = m_x;
    m_scroll_bar_058.SetLayout(x + 0x12b, m_y + 0x43, m_text_area_0ec.GetTotalLineCount(), 0,
                               m_text_area_0ec.GetLineHeight(), 0xb9);
    m_scroll_bar_058.m_owner = this;

    m_button_0a4.Configure("Data\\Dialogs\\popup_confirmationbuttons.sti", 3, 0, 1, 4, 2,
                           DialogCloseButtonCallback, 0, 0, 0x7f, -1, 0, 0);
    m_button_0a4.SetPosition(m_x + 0x11a, m_y + 0x104);
    m_button_0a4.m_owner_040 = this;
    return 0;
}

/* Same folded body as W8MonsterInfoDialog::DestroyControls at 0x005DBDE0;
   /OPT:NOICF emits this copy. */
void W8SpellInfoDialog::DestroyControls()
{
    m_scroll_bar_058.DestroyControls();
    W8DialogBase::DestroyControls();
}

// FUNCTION: WIZ8 0x005dbe00
void W8SpellInfoDialog::Draw()
{
    int steps;
    int realm;
    W8SpellRealmAnimation* animation;

    if ((m_dirty_flags & 1) != 0) {
        if (m_initialized == 0) {
            CreateControls();
        }
        m_text_area_0ec.m_dirty = 1;
        m_scroll_bar_058.m_dirty = 1;
        m_button_0a4.m_dirty = 1;
        W8DialogBase::Draw();
        DrawLabels();
    }
    m_text_area_0ec.Draw(0);
    m_scroll_bar_058.Draw(0);
    m_button_0a4.Draw();
    steps = (int)m_timer_144.GetProgress();
    if (steps > 0) {
        realm = g_spell_records[m_spell_054].realm;
        animation = &g_spell_realm_animations_00648c90[realm];
        m_animation_frame += steps;
        m_animation_frame %= animation->frame_count;
        DrawCatalogImageAndInvalidate(-0xe, animation->image, 0, m_animation_frame, m_x + 0xe,
                                      m_y + 0xe, 2, 0);
    }
}

// FUNCTION: WIZ8 0x005dc490
void W8SpellInfoDialog::DrawLabels()
{
    W8WideChar* text;
    INT16 width;
    W8SpellRuntimeRecord* record = &g_spell_records[m_spell_054];

    SetFont(g_font_683660);
    SetFontObjectPalette16BPP(g_font_683660, g_colour_68ee08);
    text = record->display_name;
    width = StringPixLength(text, g_font_683660);
    gprintf(m_x + 0x25 + (0xe7 - width) / 2, m_y + 0x12, (unsigned short*)L"%s", text);
    text = gppStringList[0x460 / 4];
    width = StringPixLength(text, g_font_683660);
    gprintf(m_x + 0x25 + (0x53 - width) / 2, m_y + 0x24, (unsigned short*)L"%s", text);
    if (record->field_12b == 3) {
        text = gppStringList[0x468 / 4];
    } else {
        text = gppStringList[0x464 / 4];
    }
    width = StringPixLength(text, g_font_683660);
    gprintf(m_x + 0x25 + (0x7b - width) / 2, m_y + 0x33, (unsigned short*)L"%s", text);
    text = FormatWideString(g_format_d_0060aa20, record->spell_level);
    width = StringPixLength(text, g_font_683660);
    gprintf(m_x + 0x7c + (0x16 - width) / 2, m_y + 0x24, (unsigned short*)L"%s", text);
    text =
        FormatWideString(g_format_d_s_0061a128, record->spell_point_cost, gppStringList[0x46c / 4]);
    width = StringPixLength(text, g_font_683660);
    gprintf(m_x + 0xa4 + (0x25 - width) / 2, m_y + 0x33, (unsigned short*)L"%s", text);
}

// FUNCTION: WIZ8 0x005dc6c0
void W8SpellInfoDialog::ScrollCallback(W8DialogScrollBar* scroll_bar, int first_visible_entry)
{
    int left;
    int top;
    int right;
    int bottom;
    W8SpellInfoDialog* dialog = static_cast<W8SpellInfoDialog*>(scroll_bar->m_owner);
    if (dialog != 0) {
        dialog->m_text_area_0ec.SetFirstVisibleLine(first_visible_entry);
        left = dialog->m_x + 0x11;
        top = dialog->m_y + 0x43;
        right = left + 0x10e;
        bottom = top + 0xb9;
        InvalidateRegion(left, top, right, bottom, 0);
        BlitCatalogSurfaceRectTo16BPP(-0xe, left, top, right, bottom, 0x1b6, 0, 0);
        dialog->m_text_area_0ec.m_dirty = 1;
    }
}

/* Same folded body as W8MonsterInfoDialog::OnRightButtonUp at 0x005D6E60;
   /OPT:NOICF emits this copy. */
void W8SpellInfoDialog::OnRightButtonUp()
{
    if (m_right_button_down) {
        m_keep_open = 0;
    }
}

/* Same folded body as W8MonsterInfoDialog::OnMouseWheel at 0x005D6E70;
   /OPT:NOICF emits this copy. */
void W8SpellInfoDialog::OnMouseWheel(int delta)
{
    if (delta > 0) {
        for (int step = 0; step < delta; ++step) {
            m_scroll_bar_058.ScrollUp();
        }
    } else if (delta < 0) {
        for (int step = 0; step < -delta; ++step) {
            m_scroll_bar_058.ScrollDown();
        }
    }
}
