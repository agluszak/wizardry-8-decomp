#include "wiz8/dialog_code/ProfRaceInfoDialog.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/fonts.h"
#include "wiz8/layouts/character.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_screens/OptionsScreen.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/video_object_catalog.h"

// GLOBAL: WIZ8 0x00614cf0
W8AttributeMinimums g_race_attribute_minimums[11] = {
    {{45, 45, 45, 45, 45, 45, 45}}, {{35, 50, 50, 35, 50, 45, 40}}, {{55, 30, 50, 60, 35, 35, 35}},
    {{35, 50, 40, 50, 50, 35, 45}}, {{40, 40, 30, 45, 55, 50, 50}}, {{25, 55, 35, 30, 50, 60, 45}},
    {{60, 25, 25, 70, 40, 50, 30}}, {{55, 35, 30, 60, 50, 40, 30}}, {{40, 40, 30, 35, 50, 60, 50}},
    {{40, 30, 55, 50, 40, 40, 50}}, {{50, 50, 25, 50, 35, 35, 55}},
};

// GLOBAL: WIZ8 0x00614e24
W8AttributeMinimums g_profession_attribute_minimums[W8_PROFESSION_COUNT] = {
    {{55, 0, 0, 50, 50, 0, 0}},   {{55, 0, 55, 55, 50, 50, 0}}, {{50, 0, 55, 55, 50, 50, 0}},
    {{50, 50, 0, 50, 55, 0, 55}}, {{50, 55, 0, 50, 55, 55, 0}}, {{50, 50, 0, 50, 55, 55, 50}},
    {{0, 50, 50, 0, 50, 55, 55}}, {{0, 0, 0, 0, 55, 50, 50}},   {{45, 55, 0, 0, 60, 0, 55}},
    {{45, 50, 0, 0, 55, 0, 55}},  {{0, 0, 60, 55, 0, 0, 0}},    {{0, 55, 0, 0, 60, 0, 0}},
    {{0, 55, 55, 0, 55, 0, 55}},  {{0, 55, 0, 0, 0, 0, 60}},    {{0, 60, 0, 0, 55, 0, 0}},
};

/* Dialog Code\ProfRaceInfoDialog.cpp. The profession and race information
   dialogs, whose constructors are the same body twice over: they differ in
   their vtable, their row table, their two string ids, their assertion line and
   how they fill the attribute minimums. Both are named by their own assertion,
   which also supplies the parameter name uiIndex and pins PROF_COUNT to fifteen
   and RACE_COUNT to sixteen. */

static const char PROF_RACE_INFO_DIALOG_CPP[] =
    "C:\\Projects\\Wizardry 8\\Dialog Code\\ProfRaceInfoDialog.cpp";

// GLOBAL: WIZ8 0x00650124
static const char* g_popup_race_profession_path = "Data\\Dialogs\\popup_race_profession.sti";

enum { ATTR_COUNT = 7, RACE_COUNT = 16 };

/* Only the first eleven races have their own minimums; the rest are shown as
   -1, which is the dialog's "no requirement" marker. */
enum { RACE_MINIMUMS_COUNT = 11 };

// GLOBAL: WIZ8 0x0064ffb0
W8ProfRaceInfoRow g_profession_info_rows[W8_PROFESSION_COUNT] = {
    {676, 691, 10}, {677, 692, 24}, {678, 693, 8},  {679, 694, 2},  {680, 695, 16},
    {681, 696, 20}, {682, 697, 12}, {683, 698, 28}, {684, 699, 6},  {685, 700, 0},
    {686, 701, 18}, {687, 702, 22}, {688, 703, 4},  {689, 704, 14}, {690, 705, 26},
};

// GLOBAL: WIZ8 0x00650064
W8ProfRaceInfoRow g_race_info_rows[16] = {
    {644, 660, 8},  {645, 661, 4},  {646, 662, 12}, {647, 663, 16}, {648, 664, 14}, {649, 665, 20},
    {650, 666, 0},  {651, 667, 10}, {652, 668, 2},  {653, 669, 18}, {654, 670, 6},  {655, 671, 30},
    {656, 672, 24}, {657, 673, 22}, {658, 674, 26}, {659, 675, 28},
};

// FUNCTION: WIZ8 0x005DEAF0
W8ProfRaceInfoDialogBase::W8ProfRaceInfoDialogBase()
{
    SetOrigin(0x85, 0x69);
    SetExtent(0x177, 0x10e);
    SetBackground(g_popup_race_profession_path, 0);
}

// SYNTHETIC: WIZ8 0x005deb90
// W8ProfRaceInfoDialogBase::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005DEC40
int W8ProfRaceInfoDialogBase::CreateControls()
{
    W8DialogBase::CreateControls();
    if (PopulateText() == 0) {
        m_error = 7;
        return 7;
    }

    W8DialogScrollBar::Resources resources;
    resources.track_path = g_popup_race_profession_path;
    resources.arrows_path = "Data\\Main Interface\\main_scroll.sti";
    resources.track_frame = 1;
    resources.on_scroll = ScrollCallback;
    m_scroll_bar_084.CreateControls(&resources);
    int x = m_x;
    m_scroll_bar_084.SetLayout(x + 0x159, m_y + 0x29, m_text_area_118.GetTotalLineCount(), 0,
                               m_text_area_118.GetLineHeight(), 0xb8);
    m_scroll_bar_084.m_owner = this;

    m_button_0d0.Configure("Data\\Dialogs\\popup_confirmationbuttons.sti", 3, 0, 1, 4, 2,
                           DialogCloseButtonCallback, 0, 0, 0x7f, -1, 0, 0);
    m_button_0d0.SetPosition(m_x + 0x14f, m_y + 0xe8);
    m_button_0d0.m_owner_040 = this;
    return 0;
}

// FUNCTION: WIZ8 0x005DED40
void W8ProfRaceInfoDialogBase::DestroyControls()
{
    m_scroll_bar_084.DestroyControls();
    W8DialogBase::DestroyControls();
}

// FUNCTION: WIZ8 0x005DED60
void W8ProfRaceInfoDialogBase::Draw()
{
    if ((m_dirty_flags & 1) != 0) {
        if (m_initialized == 0) {
            CreateControls();
        }
        m_text_area_118.m_dirty = 1;
        m_scroll_bar_084.m_dirty = 1;
        m_button_0d0.m_dirty = 1;
        W8DialogBase::Draw();
        DrawCatalogImageAndInvalidate(-0xe, m_uiTitleId, 0, m_uiSummaryId, m_x + 0xd, m_y + 0xd, 2,
                                      0);
        DrawTextLine(g_W8TextBufferLayoutMask005ED548, 0x25, 0xb, 0x147, 0x18,
                     gppStringList[m_uiNameId], g_options_detail_font_683614);
        DrawTextLine(g_W8TextBufferLayoutMask005ED54C, 0xd, 0x29, 0x8e, 0xc,
                     gppStringList[m_uiHeadingId], g_font_683660);
        int top = 0x37;
        for (int attribute = 0; attribute < ATTR_COUNT; ++attribute) {
            DrawTextLine(g_W8TextBufferLayoutMask005ED548, 0x10, top, 0x74, 0xc,
                         gppStringList[g_character_description_first_ids_61e3a4[attribute]],
                         g_font_683660);
            const wchar_t* minimum;
            if (m_minimums[attribute] == -1) {
                minimum = L"?";
            } else {
                minimum =
                    FormatWideString(g_format_d_0060aa20, m_minimums[attribute], g_font_683660);
            }
            DrawTextLine(g_W8TextBufferLayoutMask005ED54C, 0x85, top, 0x16, 0xc, minimum,
                         g_font_683660);
            top += 0xe;
        }
    }
    m_text_area_118.Draw(0);
    m_scroll_bar_084.Draw(0);
    m_button_0d0.Draw();
}

// FUNCTION: WIZ8 0x005DEEE0
void W8ProfRaceInfoDialogBase::DrawTextLine(unsigned int layout_mode, int left, int top, int width,
                                            int height, const wchar_t* text, int font)
{
    W8TextBuffer buffer;
    buffer.SetLayoutMode(g_W8TextBufferLayoutMask005ED554 | layout_mode);
    W8ControlsRect bounds;
    bounds.left = m_x + left;
    bounds.right = bounds.left + width;
    bounds.top = m_y + top;
    bounds.bottom = bounds.top + height;
    buffer.SetLayoutBounds(&bounds, 1, 1);
    buffer.SetText(text, font);
    buffer.RenderToTarget(0, 0, -0xe);
}

// FUNCTION: WIZ8 0x005DEFB0
unsigned char W8ProfRaceInfoDialogBase::PopulateText()
{
    W8ControlsRect bounds;
    bounds.left = m_x + 0xa2;
    bounds.top = m_y + 0x29;
    bounds.right = m_x + 0x151;
    bounds.bottom = m_y + 0xe1;
    m_text_area_118.Configure(&bounds, g_font_683660, 0);
    m_text_area_118.SetEntrySpacing(0);
    return 1;
}

/* Same folded body as W8MonsterInfoDialog::OnRightButtonUp at 0x005D6E60;
   /OPT:NOICF emits this copy. */
void W8ProfRaceInfoDialogBase::OnRightButtonUp()
{
    if (m_right_button_down) {
        m_keep_open = 0;
    }
}

// FUNCTION: WIZ8 0x005DF010
void W8ProfRaceInfoDialogBase::OnMouseWheel(int delta)
{
    if (delta > 0) {
        for (int step = 0; step < delta; ++step) {
            m_scroll_bar_084.ScrollUp();
        }
    } else if (delta < 0) {
        for (int step = 0; step < -delta; ++step) {
            m_scroll_bar_084.ScrollDown();
        }
    }
}

// FUNCTION: WIZ8 0x005DF050
void W8ProfRaceInfoDialogBase::ScrollCallback(W8DialogScrollBar* scroll_bar,
                                              int first_visible_entry)
{
    int left;
    int top;
    int right;
    int bottom;
    W8ProfRaceInfoDialogBase* dialog = static_cast<W8ProfRaceInfoDialogBase*>(scroll_bar->m_owner);
    if (dialog != 0) {
        dialog->m_text_area_118.SetFirstVisibleLine(first_visible_entry);
        left = dialog->m_x + 0xa2;
        top = dialog->m_y + 0x29;
        right = left + 0xaf;
        bottom = top + 0xb8;
        InvalidateRegion(left, top, right, bottom, 0);
        BlitCatalogSurfaceRectTo16BPP(-0xe, left, top, right, bottom, 0x1b6, 0, 0);
        dialog->m_text_area_118.m_dirty = 1;
    }
}

/* Copies the row straight out of the table: every profession has minimums, so
   there is no per-entry test the way the race dialog needs one. */
// FUNCTION: WIZ8 0x005df0d0
W8ProfessionInfoDialog::W8ProfessionInfoDialog(unsigned int uiIndex)
{
    if (uiIndex >= W8_PROFESSION_COUNT) {
        srAssertFail("uiIndex < PROF_COUNT", PROF_RACE_INFO_DIALOG_CPP, 0x123, 0);
    }
    m_uiIndex = uiIndex;
    m_uiTitleId = 0x10b;
    m_uiHeadingId = 0x14c;
    m_uiSummaryId = g_profession_info_rows[uiIndex].image_id;
    m_uiNameId = g_profession_info_rows[uiIndex].name_id;
    m_uiDetailId = g_profession_info_rows[uiIndex].detail_id;
    for (int attribute = 0; attribute < ATTR_COUNT; ++attribute) {
        m_minimums[attribute] = g_profession_attribute_minimums[uiIndex].values[attribute];
    }
}

// SYNTHETIC: WIZ8 0x005df1a0
// W8ProfessionInfoDialog::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005df1c0
W8ProfessionInfoDialog::~W8ProfessionInfoDialog() {}

// FUNCTION: WIZ8 0x005DF250
unsigned char W8ProfessionInfoDialog::PopulateText()
{
    W8ControlsRect bounds;
    bounds.left = m_x + 0xa2;
    bounds.top = m_y + 0x29;
    bounds.right = m_x + 0x151;
    bounds.bottom = m_y + 0xe1;
    m_text_area_118.Configure(&bounds, g_font_683660, 0);
    m_text_area_118.SetEntrySpacing(0);
    m_text_area_118.AddEntry(gppStringList[0x153], gppStringList[m_uiDetailId], 10, 0xf, 0);
    m_text_area_118.AddEntry(0, &g_wchar_00689b34, 10, 0xf, 0);
    m_text_area_118.AddEntry(gppStringList[0x14d], &g_wchar_00689b34, 10, 0xf, 0);
    unsigned int index;
    for (index = 0; index < 3; ++index) {
        int ability = g_profession_abilities[m_uiIndex].ability_ids[index];
        if (ability == -1) {
            break;
        }
        m_text_area_118.AddEntry(0, gppStringList[g_character_trait_name_ids_61e530[ability]], 10,
                                 0xf, 0);
    }
    m_text_area_118.AddEntry(0, &g_wchar_00689b34, 10, 0xf, 0);
    m_text_area_118.AddEntry(gppStringList[0x14e], &g_wchar_00689b34, 10, 0xf, 0);
    m_text_area_118.AddEntry(
        0, gppStringList[g_character_skill_name_ids_61e454[g_profession_bonus_skills[m_uiIndex]]],
        10, 0xf, 0);
    m_text_area_118.AddEntry(0, &g_wchar_00689b34, 10, 0xf, 0);
    m_text_area_118.AddEntry(gppStringList[0x14f], &g_wchar_00689b34, 10, 0xf, 0);
    for (index = 0; index < 4; ++index) {
        int skill = g_profession_skills[m_uiIndex][index];
        if (skill == -1) {
            break;
        }
        m_text_area_118.AddEntry(0, gppStringList[g_character_skill_name_ids_61e454[skill]], 10,
                                 0xf, 0);
    }
    m_text_area_118.AddEntry(0, &g_wchar_00689b34, 10, 0xf, 0);
    m_text_area_118.AddEntry(gppStringList[0x150], &g_wchar_00689b34, 10, 0xf, 0);
    for (index = 0; index < 6; ++index) {
        int item = g_starting_equipment_61635c[m_uiIndex][index];
        if (item != -1) {
            m_text_area_118.AddEntry(0, g_item_records[item].display_name, 10, 0xf, 0);
        }
    }
    const wchar_t* first;
    const wchar_t* second;
    switch (m_uiIndex) {
    case 10:
        second = g_item_records[82].display_name;
        first = g_item_records[22].display_name;
        break;
    case 0:
        second = g_item_records[7].display_name;
        first = g_item_records[18].display_name;
        break;
    case 8:
        m_text_area_118.AddEntry(0, g_item_records[599].display_name, 10, 0xf, 0);
        return 1;
    case 9:
        m_text_area_118.AddEntry(0, g_item_records[324].display_name, 10, 0xf, 0);
        return 1;
    default:
        return 1;
    }
    m_text_area_118.AddEntry(0, FormatWideString(gppStringList[0x151], first, second), 10, 0xf, 0);
    return 1;
}

/* The same shape, except that only the first eleven races have minimums and the
   rest show -1, so the copy is a counted loop with the test inside it rather
   than the profession dialog's straight block move. */
// FUNCTION: WIZ8 0x005df570
W8RaceInfoDialog::W8RaceInfoDialog(unsigned int uiIndex)
{
    if (uiIndex >= RACE_COUNT) {
        srAssertFail("uiIndex < RACE_COUNT", PROF_RACE_INFO_DIALOG_CPP, 0x199, 0);
    }
    m_uiIndex = uiIndex;
    m_uiTitleId = 0x10c;
    m_uiHeadingId = 0x152;
    m_uiSummaryId = g_race_info_rows[uiIndex].image_id;
    m_uiNameId = g_race_info_rows[uiIndex].name_id;
    m_uiDetailId = g_race_info_rows[uiIndex].detail_id;
    for (unsigned int attribute = 0; attribute < ATTR_COUNT; ++attribute) {
        if (uiIndex < RACE_MINIMUMS_COUNT) {
            m_minimums[attribute] = g_race_attribute_minimums[uiIndex].values[attribute];
        } else {
            m_minimums[attribute] = -1;
        }
    }
}

// SYNTHETIC: WIZ8 0x005df640
// W8RaceInfoDialog::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005df660
W8RaceInfoDialog::~W8RaceInfoDialog() {}

// FUNCTION: WIZ8 0x005DF6F0
unsigned char W8RaceInfoDialog::PopulateText()
{
    W8ControlsRect bounds;
    bounds.left = m_x + 0xa2;
    bounds.top = m_y + 0x29;
    bounds.right = m_x + 0x151;
    bounds.bottom = m_y + 0xe1;
    m_text_area_118.Configure(&bounds, g_font_683660, 0);
    m_text_area_118.SetEntrySpacing(0);
    m_text_area_118.AddEntry(gppStringList[0x153], gppStringList[m_uiDetailId], 10, 0xf, 0);
    m_text_area_118.AddEntry(0, &g_wchar_00689b34, 10, 0xf, 0);
    m_text_area_118.AddEntry(gppStringList[0x14d], &g_wchar_00689b34, 10, 0xf, 0);
    unsigned char listed = 0;
    for (unsigned int index = 0; index < 5; ++index) {
        int ability = g_race_abilities[m_uiIndex].ability_ids[index];
        if (ability == -1) {
            if (!listed) {
                m_text_area_118.AddEntry(0, gppStringList[0x154], 10, 0xf, 0);
            }
            return 1;
        }
        m_text_area_118.AddEntry(0, gppStringList[g_character_trait_name_ids_61e530[ability]], 10,
                                 0xf, 0);
        listed = 1;
    }
    return 1;
}
