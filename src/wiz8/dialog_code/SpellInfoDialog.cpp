#include "wiz8/dialog_code/SpellInfoDialog.h"
#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/fonts.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/magic.h"
#include "wiz8/screen_state.h"
#include "wiz8/sr_api.h"
#include "wiz8/string_database.h"
#include "wiz8/utility.h"
#include "wiz8/video_object_catalog.h"
#include "Font.h"

#include <wchar.h>

// GLOBAL: WIZ8 0x0064fccc
const char* g_spell_info_background_path = "Data\\Dialogs\\popup_spellinfo.sti";
// GLOBAL: WIZ8 0x0064fcd0
const char* g_spell_effect_database_path = "Data\\Databases\\SpellEffect.dbs";
// GLOBAL: WIZ8 0x0064fcd4
const char* g_spell_desc_database_path = "Data\\Databases\\SpellDesc.dbs";
// GLOBAL: WIZ8 0x0061a128
const wchar_t g_format_d_s_0061a128[] = L"%d %s";
// GLOBAL: WIZ8 0x00619794
const wchar_t g_comma_space_00619794[] = L", ";
// GLOBAL: WIZ8 0x0060cff0
const unsigned short g_spellbook_name_ids_60cff0[4] = {791, 792, 793, 794};
// GLOBAL: WIZ8 0x0060d4a8
const unsigned short g_spell_usage_name_ids_60d4a8[5] = {795, 797, 796, 796, 796};
// GLOBAL: WIZ8 0x0060d4b4
const unsigned short g_spell_target_type_name_ids_60d4b4[11] = {798, 799, 800, 801, 802, 803,
                                                                804, 805, 806, 807, 807};
// GLOBAL: WIZ8 0x0060d4cc
const wchar_t g_spell_target_mark_fff4[] = {0xfff4, 0};
// GLOBAL: WIZ8 0x0060d4d0
const wchar_t g_spell_target_mark_fff0[] = {0xfff0, 0};
// GLOBAL: WIZ8 0x0060d4d4
const wchar_t g_spell_target_mark_fff1[] = {0xfff1, 0};
// GLOBAL: WIZ8 0x0060d4d8
const wchar_t g_spell_target_mark_fff2[] = {0xfff2, 0};
// GLOBAL: WIZ8 0x0060d4dc
const wchar_t g_spell_target_mark_fff3[] = {0xfff3, 0};
// GLOBAL: WIZ8 0x0060d4e0
const wchar_t* g_spell_target_parentheticals_60d4e0[11] = {
    g_spell_target_mark_fff4, g_spell_target_mark_fff2, g_spell_target_mark_fff4,
    g_spell_target_mark_fff2, g_spell_target_mark_fff3, g_spell_target_mark_fff1,
    g_spell_target_mark_fff0, g_spell_target_mark_fff4, g_spell_target_mark_fff0,
    g_spell_target_mark_fff4, g_spell_target_mark_fff4,
};
// GLOBAL: WIZ8 0x0061e9a0
const unsigned short g_spell_range_name_ids_61e9a0[8] = {1307, 1308, 1309, 1310,
                                                         1311, 1312, 1313, 1314};
// GLOBAL: WIZ8 0x0064fdcc
const wchar_t g_format_d_space_0064fdcc[] = L"%d ";
// GLOBAL: WIZ8 0x0064fdc4
const wchar_t g_plus_space_0064fdc4[] = L"+ ";
// GLOBAL: WIZ8 0x0064fdd4
const wchar_t g_format_d_d_s_0064fdd4[] = L"%d-%d %s";

static const char SPELL_INFO_DIALOG_CPP[] =
    "C:\\Projects\\Wizardry 8\\Dialog Code\\SpellInfoDialog.cpp";

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

// FUNCTION: WIZ8 0x005dbee0
unsigned char W8SpellInfoDialog::PopulateText()
{
    W8ControlsRect bounds;
    W8SpellRuntimeRecord* record;
    wchar_t text[2000];
    unsigned int spellbook_mask;
    unsigned int book;
    int count;
    int target_type;
    int duration_per_level;
    int duration_base;
    int ui_units;
    int ui_units_lvl;
    int display;
    int display_base;

    bounds.left = m_x + 0x11;
    bounds.top = m_y + 0x43;
    bounds.right = m_x + 0x11f;
    bounds.bottom = m_y + 0xfc;
    m_text_area_0ec.Configure(&bounds, g_font_683660, 0);
    m_text_area_0ec.SetEntrySpacing(1);

    record = &g_spell_records[m_spell_054];
    text[0] = L'\0';
    spellbook_mask = 0;
    if (record->wizardry_spell != 0) {
        spellbook_mask |= W8_SPELLBOOK_WIZARDRY;
    }
    if (record->divinity_spell != 0) {
        spellbook_mask |= W8_SPELLBOOK_DIVINITY;
    }
    if (record->alchemy_spell != 0) {
        spellbook_mask |= W8_SPELLBOOK_ALCHEMY;
    }
    if (record->psionics_spell != 0) {
        spellbook_mask |= W8_SPELLBOOK_PSIONICS;
    }
    count = 0;
    for (book = 0; book < 4; ++book) {
        if ((spellbook_mask & (1 << book)) != 0) {
            if (count > 0) {
                wcscat(text, g_comma_space_00619794);
            }
            wcscat(text, gppStringList[g_spellbook_name_ids_60cff0[book]]);
            ++count;
        }
    }
    m_text_area_0ec.AddEntry(gppStringList[0x470 / 4], text, 10, 0xf, 0);
    m_text_area_0ec.AddEntry(gppStringList[0x474 / 4],
                             gppStringList[g_spell_usage_name_ids_60d4a8[record->usable_when]], 10,
                             0xf, 0);

    target_type = GetSpellTargetType(m_spell_054, 0);
    m_text_area_0ec.AddEntry(
        gppStringList[0x478 / 4],
        FormatWideString(g_format_s_parenthesized_s_00617584,
                         gppStringList[g_spell_target_type_name_ids_60d4b4[target_type]],
                         g_spell_target_parentheticals_60d4e0[target_type]),
        10, 0xf, 0);
    m_text_area_0ec.AddEntry(gppStringList[0x47c / 4],
                             gppStringList[g_spell_range_name_ids_61e9a0[record->range_category]],
                             10, 0xf, 0);

    if (record->field_147 != 0 &&
        (record->effect_dice.base != 0 || record->effect_dice.count != 0)) {
        if (record->effect_dice.count == 0) {
            m_text_area_0ec.AddEntry(gppStringList[0x480 / 4],
                                     FormatWideString(g_format_d_s_0061a128,
                                                      (int)record->effect_dice.base,
                                                      gppStringList[0x484 / 4]),
                                     10, 0xf, 0);
        } else {
            m_text_area_0ec.AddEntry(
                gppStringList[0x480 / 4],
                FormatWideString(g_format_d_d_s_0064fdd4,
                                 record->effect_dice.count + (int)record->effect_dice.base,
                                 record->effect_dice.sides * record->effect_dice.count +
                                     (int)record->effect_dice.base,
                                 gppStringList[0x484 / 4]),
                10, 0xf, 0);
        }
    }

    duration_per_level = record->duration_per_level_04d;
    duration_base = record->duration_044;
    if (duration_per_level != 0 || duration_base != 0) {
        text[0] = L'\0';
        ui_units = 0;
        display = 0;
        if (duration_per_level > 0) {
            if (duration_per_level == 9999) {
                ui_units = 0x129;
                display = 0;
            } else if (duration_per_level * 10 < 300) {
                ui_units = 0x123;
                display = duration_per_level;
            } else if (duration_per_level * 0x78 < 0x3840) {
                display = (duration_per_level * 10) / 0x3c;
                ui_units = 0x125;
            } else {
                display = (duration_per_level * 0x78) / 0xe10;
                ui_units = 0x127;
            }
            if (display > 0) {
                wcscat(text, FormatWideString(g_format_d_space_0064fdcc, display));
            }
            if (duration_base > 0) {
                wcscat(text, g_plus_space_0064fdc4);
            }
        }
        ui_units_lvl = ui_units;
        display_base = 0;
        if (duration_base > 0) {
            if (duration_base == 9999) {
                ui_units_lvl = 0x129;
                display_base = 0;
            } else if (duration_base * 10 < 300) {
                ui_units_lvl = 0x123;
                display_base = duration_base;
            } else if (duration_base * 0x78 < 0x3840) {
                display_base = (duration_base * 10) / 0x3c;
                ui_units_lvl = 0x125;
            } else {
                display_base = (duration_base * 0x78) / 0xe10;
                ui_units_lvl = 0x127;
            }
            if (duration_per_level != 0 && ui_units != ui_units_lvl) {
                srAssertFail("uiUnits == uiUnitsLvl", SPELL_INFO_DIALOG_CPP, 0xc5,
                             FormatString("Spell %S duration has mismatched base & per-lvl units",
                                          record->display_name));
            }
            wcscat(text, FormatWideString(g_format_d_space_0064fdcc, display_base));
        }
        if ((duration_per_level == 1 && display_base == 0) ||
            (duration_per_level == 0 && display_base == 1)) {
            if (ui_units_lvl == 0x123) {
                ui_units_lvl = 0x124;
            } else if (ui_units_lvl == 0x125) {
                ui_units_lvl = 0x126;
            } else if (ui_units_lvl == 0x127) {
                ui_units_lvl = 0x128;
            }
        }
        wcscat(text, gppStringList[ui_units_lvl]);
        if (display_base > 0) {
            wcscat(text, gppStringList[0x4a8 / 4]);
        }
        m_text_area_0ec.AddEntry(gppStringList[0x488 / 4], text, 10, 0xf, 0);
    }

    text[0] = L'\0';
    if (GetStringFromStringDatabase(g_spell_effect_database_path, m_spell_054, text, 0, 0) != 0 &&
        text[0] != L'\0') {
        m_text_area_0ec.AddEntry(gppStringList[0x4ac / 4], text, 10, 0xf, 0);
    }
    text[0] = L'\0';
    if (GetStringFromStringDatabase(g_spell_desc_database_path, m_spell_054, text, 0, 0) != 0 &&
        text[0] != L'\0') {
        m_text_area_0ec.AddEntry(gppStringList[0x4b0 / 4], text, 10, 0xf, 0);
    }
    return 1;
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
