#include "wiz8/dialog_code/StatInfoDialogs.h"
#include "Font.h"
#include "wiz8/character.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/fonts.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/screen_state.h"
#include "wiz8/utility.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/sr_api.h"
#include "wiz8/video_object_catalog.h"

/* Dialog Code\StatInfoDialogs.cpp. Two attribute-info dialogs whose
   constructors are the same body twice over: the canonical pair differs only
   in its vtable, its two lookup tables and its assertion line. Both are named
   by their own assertion, which also supplies the parameter name uiIndex and
   pins ATTR_COUNT to seven - the same seven attributes W8Character carries.
   The shared base holds the scroll bar, close button and text area every
   info dialog in this family reuses; its own vtable is emitted retail even
   though it is only ever a base subobject. */

static const char STAT_INFO_DIALOGS_CPP[] =
    "C:\\Projects\\Wizardry 8\\Dialog Code\\StatInfoDialogs.cpp";

enum { ATTR_COUNT = 7 };
enum { SKILL_COUNT = 0x29 };

// GLOBAL: WIZ8 0x0061e4fc
unsigned short g_attr_table_61E4FC[8] = {
    0x6a0, 0x6a1, 0x6a2, 0x6a3, 0x6a4, 0x6a5, 0x6a6, 0,
};
// GLOBAL: WIZ8 0x0061e50c
unsigned short g_attr_table_61E50C[18] = {
    0x6a7, 0x6a8, 0x6a9, 0x6aa, 0x6ab, 0,     0x30b, 0x30c, 0x30d,
    0x30e, 0x30f, 0x310, 0x311, 0x312, 0x313, 0x314, 0x315, 0x316,
};

// FUNCTION: WIZ8 0x005df880
W8StatInfoDialogBase005DF880::W8StatInfoDialogBase005DF880()
{
    SetOrigin(0x9c, 0x69);
    SetExtent(0x14a, 0x10e);
    SetBackground("Data\\Dialogs\\popup_monsterinfo.sti", 0);
}

// SYNTHETIC: WIZ8 0x005df920
// W8StatInfoDialogBase005DF880::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005DF940
W8StatInfoDialogBase005DF880::~W8StatInfoDialogBase005DF880()
{
    scrollbar_054.DestroyControls();
    W8DialogBase::DestroyControls();
}

// FUNCTION: WIZ8 0x005df9d0
int W8StatInfoDialogBase005DF880::CreateControls()
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
    scrollbar_054.CreateControls(&resources);
    scrollbar_054.SetLayout(m_x + 299, m_y + 0x26, textarea_0e8.GetTotalLineCount(), 0,
                            textarea_0e8.GetLineHeight(), 0xb9);
    scrollbar_054.m_owner = this;

    button_0a0.Configure("Data\\Dialogs\\popup_confirmationbuttons.sti", 3, 0, 1, 4, 2,
                         DialogCloseButtonCallback, 0, 0, 0x7f, -1, 0, 0);
    button_0a0.SetPosition(m_x + 0x11a, m_y + 0xe6);
    button_0a0.m_owner_040 = this;
    return 0;
}

// FUNCTION: WIZ8 0x005dfac0
void W8StatInfoDialogBase005DF880::DestroyControls()
{
    scrollbar_054.DestroyControls();
    W8DialogBase::DestroyControls();
}

// FUNCTION: WIZ8 0x005dfae0
void W8StatInfoDialogBase005DF880::Draw()
{
    if ((m_dirty_flags & 1) != 0) {
        if (m_initialized == 0) {
            CreateControls();
        }
        textarea_0e8.m_dirty = 1;
        scrollbar_054.m_dirty = 1;
        button_0a0.m_dirty = 1;
        W8DialogBase::Draw();
        DrawTitle();
    }
    textarea_0e8.Draw(0);
    scrollbar_054.Draw(0);
    button_0a0.Draw();
}

// FUNCTION: WIZ8 0x005dfb40
void W8StatInfoDialogBase005DF880::DrawTitle()
{
    SetFont(g_font_683660);
    SetFontObjectPalette16BPP(g_font_683660, g_colour_68ee08);
    wchar_t* title = gppStringList[m_title_id_140];
    INT16 width = StringPixLength(title, g_font_683660);
    gprintf(m_x + 0xe + (0x112 - width) / 2, m_y + 0x11, L"%s", title);
}

void W8StatInfoDialogBase005DF880::OnRightButtonUp()
{
    if (m_right_button_down) {
        m_keep_open = 0;
    }
}

// FUNCTION: WIZ8 0x005dfbb0
void W8StatInfoDialogBase005DF880::OnMouseWheel(int delta)
{
    if (delta > 0) {
        for (int step = 0; step < delta; ++step) {
            scrollbar_054.ScrollUp();
        }
    } else if (delta < 0) {
        for (int step = 0; step < -delta; ++step) {
            scrollbar_054.ScrollDown();
        }
    }
}

// FUNCTION: WIZ8 0x005dfbf0
void W8StatInfoDialogBase005DF880::ScrollCallback(W8DialogScrollBar* scroll_bar,
                                                  int first_visible_entry)
{
    int left;
    int top;
    int right;
    int bottom;
    W8StatInfoDialogBase005DF880* dialog =
        static_cast<W8StatInfoDialogBase005DF880*>(scroll_bar->m_owner);
    if (dialog != 0) {
        dialog->textarea_0e8.SetFirstVisibleLine(first_visible_entry);
        left = dialog->m_x + 0x11;
        top = dialog->m_y + 0x26;
        right = left + 0x10e;
        bottom = top + 0xb9;
        InvalidateRegion(left, top, right, bottom, 0);
        BlitCatalogSurfaceRectTo16BPP(-0xe, left, top, right, bottom, 0x1b6, 0, 0);
        dialog->textarea_0e8.m_dirty = 1;
    }
}

// FUNCTION: WIZ8 0x005dfdb0
unsigned char W8StatInfoDialogBase005DF880::PopulateText()
{
    W8ControlsRect bounds;
    bounds.left = m_x + 0x11;
    bounds.top = m_y + 0x26;
    bounds.right = m_x + 0x11f;
    bounds.bottom = m_y + 0xdf;
    textarea_0e8.Configure(&bounds, g_font_683660, 0);
    textarea_0e8.SetEntrySpacing(0);
    textarea_0e8.AddEntry(gppStringList[0x155], gppStringList[m_detail_id_144], 10, 0xf, 0);
    textarea_0e8.AddEntry(0, &g_wchar_00689b34, 10, 0xf, 0);
    return 1;
}

// FUNCTION: WIZ8 0x005dfc70
W8AttributeInfoDialog005DFC70::W8AttributeInfoDialog005DFC70(unsigned int uiIndex)
{
    if (uiIndex >= ATTR_COUNT) {
        srAssertFail("uiIndex < ATTR_COUNT", STAT_INFO_DIALOGS_CPP, 204, 0);
    }
    m_uiIndex = uiIndex;
    m_title_id_140 = g_character_description_first_ids_61e3a4[uiIndex];
    m_detail_id_144 = g_attr_table_61E4FC[uiIndex];
}

// SYNTHETIC: WIZ8 0x005dfd00
// W8AttributeInfoDialog005DFC70::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005dfd20
W8AttributeInfoDialog005DFC70::~W8AttributeInfoDialog005DFC70()
{
    scrollbar_054.DestroyControls();
    W8DialogBase::DestroyControls();
}

// FUNCTION: WIZ8 0x005dfe40
W8SkillInfoDialog005EFD08::W8SkillInfoDialog005EFD08(unsigned int skill, unsigned char first,
                                                     unsigned char second, unsigned char bonus)
{
    if (skill >= SKILL_COUNT) {
        srAssertFail("uiIndex < SKILL_COUNT", STAT_INFO_DIALOGS_CPP, 227, 0);
    }
    m_skill_148 = skill;
    m_title_id_140 = g_character_skill_name_ids_61e454[skill];
    m_detail_id_144 = g_character_skill_name_ids_61e454[skill + 0x2a];
    m_first_14c = first;
    m_second_14d = second;
    m_bonus_14e = bonus;
}

// SYNTHETIC: WIZ8 0x005dfef0
// W8SkillInfoDialog005EFD08::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005dff10
W8SkillInfoDialog005EFD08::~W8SkillInfoDialog005EFD08()
{
    scrollbar_054.DestroyControls();
    W8DialogBase::DestroyControls();
}

// FUNCTION: WIZ8 0x005dffa0
unsigned char W8SkillInfoDialog005EFD08::PopulateText()
{
    W8ControlsRect bounds;
    bounds.left = m_x + 0x11;
    bounds.top = m_y + 0x26;
    bounds.right = m_x + 0x11f;
    bounds.bottom = m_y + 0xdf;
    textarea_0e8.Configure(&bounds, g_font_683660, 0);
    textarea_0e8.SetEntrySpacing(0);
    textarea_0e8.AddEntry(gppStringList[0x155], gppStringList[m_detail_id_144], 10, 0xf, 0);
    textarea_0e8.AddEntry(0, &g_wchar_00689b34, 10, 0xf, 0);
    textarea_0e8.AddEntry(gppStringList[0x156], &g_wchar_00689b34, 10, 0xf, 0);
    W8SkillAttributes* skill = &g_skill_attributes[m_skill_148];
    textarea_0e8.AddEntry(
        0, gppStringList[g_character_description_first_ids_61e3a4[skill->unknown_08]], 10, 0xf, 0);
    if (skill->unknown_08 != skill->unknown_0c) {
        textarea_0e8.AddEntry(
            0, gppStringList[g_character_description_first_ids_61e3a4[skill->unknown_0c]], 10, 0xf,
            0);
    }
    if (m_first_14c != 0) {
        textarea_0e8.AddEntry(0, &g_wchar_00689b34, 10, 0xf, 0);
        textarea_0e8.AddEntry(0, gppStringList[0x157], 10, 5, 0);
    }
    if (m_second_14d != 0) {
        textarea_0e8.AddEntry(0, &g_wchar_00689b34, 10, 0xf, 0);
        textarea_0e8.AddEntry(0, gppStringList[0x158], 10, 0xb, 0);
    }
    if (m_bonus_14e != 0) {
        textarea_0e8.AddEntry(0, &g_wchar_00689b34, 10, 0xf, 0);
        textarea_0e8.AddEntry(0, FormatWideString(gppStringList[0x159], 0x19), 10, 3, 0);
    }
    return 1;
}

// FUNCTION: WIZ8 0x005e0180
W8SecondaryAttributeInfoDialog005E0180::W8SecondaryAttributeInfoDialog005E0180(unsigned int uiIndex)
{
    if (uiIndex >= ATTR_COUNT) {
        srAssertFail("uiIndex < ATTR_COUNT", STAT_INFO_DIALOGS_CPP, 278, 0);
    }
    m_uiIndex = uiIndex;
    m_title_id_140 = g_character_description_first_ids_61e3a4[16 + uiIndex];
    m_detail_id_144 = g_attr_table_61E50C[uiIndex];
}

// SYNTHETIC: WIZ8 0x005e0210
// W8SecondaryAttributeInfoDialog005E0180::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005e0230
W8SecondaryAttributeInfoDialog005E0180::~W8SecondaryAttributeInfoDialog005E0180()
{
    scrollbar_054.DestroyControls();
    W8DialogBase::DestroyControls();
}
