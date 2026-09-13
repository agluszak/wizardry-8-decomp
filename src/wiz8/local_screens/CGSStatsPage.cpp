#include "wiz8/local_code/Widget.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/local_screens/CharacterScreen.h"

#include "wiz8/character.h"
#include "wiz8/fonts.h"
#include "wiz8/local_code/ControlsRect.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/screen_state.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/video_object_catalog.h"

#include "input.h"

#include <new>

#define CGS_STATS_PAGE_CPP "C:\\Projects\\Wizardry 8\\Local Screens\\CGSStatsPage.cpp"

// GLOBAL: WIZ8 0x0069c550
unsigned int g_character_stats_region_set_0069c550;
// GLOBAL: WIZ8 0x0069c554
unsigned int g_character_stats_profession_region_set_0069c554;
// GLOBAL: WIZ8 0x0069c558
unsigned int g_character_stats_race_region_set_0069c558;
// GLOBAL: WIZ8 0x0069c55c
unsigned int g_character_stats_gender_region_set_0069c55c;

// GLOBAL: WIZ8 0x0064f028
W8CharacterStatsRecord g_character_profession_records_0064f028[15] = {
    {0x0000010b, 0x0000000a, 0x0000000b, 0x02a4, 0, 0},
    {0x0000010b, 0x00000018, 0x00000019, 0x02a5, 0, 0},
    {0x0000010b, 0x00000008, 0x00000009, 0x02a6, 0, 0},
    {0x0000010b, 0x00000002, 0x00000003, 0x02a7, 0, 0},
    {0x0000010b, 0x00000010, 0x00000011, 0x02a8, 0, 0},
    {0x0000010b, 0x00000014, 0x00000015, 0x02a9, 0, 0},
    {0x0000010b, 0x0000000c, 0x0000000d, 0x02aa, 0, 0},
    {0x0000010b, 0x0000001c, 0x0000001d, 0x02ab, 0, 0},
    {0x0000010b, 0x00000006, 0x00000007, 0x02ac, 0, 0},
    {0x0000010b, 0x00000000, 0x00000001, 0x02ad, 0, 0},
    {0x0000010b, 0x00000012, 0x00000013, 0x02ae, 0, 0},
    {0x0000010b, 0x00000016, 0x00000017, 0x02af, 0, 0},
    {0x0000010b, 0x00000004, 0x00000005, 0x02b0, 0, 0},
    {0x0000010b, 0x0000000e, 0x0000000f, 0x02b1, 0, 0},
    {0x0000010b, 0x0000001a, 0x0000001b, 0x02b2, 0, 0},
};
// GLOBAL: WIZ8 0x0064f118
W8CharacterStatsRecord g_character_race_records_0064f118[11] = {
    {0x0000010c, 0x00000008, 0x00000009, 0x0284, 1, 0},
    {0x0000010c, 0x00000004, 0x00000005, 0x0285, 1, 0},
    {0x0000010c, 0x0000000c, 0x0000000d, 0x0286, 1, 0},
    {0x0000010c, 0x00000010, 0x00000011, 0x0287, 1, 0},
    {0x0000010c, 0x0000000e, 0x0000000f, 0x0288, 1, 0},
    {0x0000010c, 0x00000014, 0x00000015, 0x0289, 1, 0},
    {0x0000010c, 0x00000000, 0x00000001, 0x028a, 1, 0},
    {0x0000010c, 0x0000000a, 0x0000000b, 0x028b, 1, 0},
    {0x0000010c, 0x00000002, 0x00000003, 0x028c, 1, 0},
    {0x0000010c, 0x00000012, 0x00000013, 0x028d, 1, 0},
    {0x0000010c, 0x00000006, 0x00000007, 0x028e, 1, 0},
};
// GLOBAL: WIZ8 0x0064f218
W8CharacterStatsRecord g_character_gender_records_0064f218[2] = {
    {0x0000010d, 0x00000000, 0x00000001, 0x02d1, 0, 0},
    {0x0000010d, 0x00000002, 0x00000003, 0x02d2, 0, 0},
};
// GLOBAL: WIZ8 0x0064f248
W8CharacterStatsRecord g_character_profession_default_record_0064f248 = {
    0x0000010e, 0x00000002, 0x00000003, 0x008f, 1, 0,
};
// GLOBAL: WIZ8 0x0064f258
W8CharacterStatsRecord g_character_race_default_record_0064f258 = {
    0x0000010e, 0x00000000, 0x00000001, 0x0090, 1, 0,
};
// GLOBAL: WIZ8 0x0064f268
W8CharacterStatsRecord g_character_gender_default_record_0064f268 = {
    0x0000010e, 0x00000004, 0x00000005, 0x0091, 1, 0,
};

/* The six resistance icons drawn beside the stats page's resistance values. */
// GLOBAL: WIZ8 0x0064ce60
int g_character_resistance_images_0064ce60[6] = {
    0x193, 0x194, 0x195, 0x196, 0x197, 0x198,
};
// GLOBAL: WIZ8 0x0061e530
unsigned short g_character_trait_name_ids_61e530[0x20] = {
    0x328, 0x329, 0x32a, 0x32b, 0x32c, 0x32d, 0x32e, 0x32f, 0x330, 0x331, 0x332,
    0x333, 0x334, 0x335, 0x336, 0x337, 0x338, 0x339, 0x33a, 0x33b, 0x33c, 0x33d,
    0x33e, 0x33f, 0x340, 0x341, 0x342, 0x343, 0x344, 0x345, 0x346, 0x347,
};
// GLOBAL: WIZ8 0x0060aa20
const wchar_t g_format_d_0060aa20[] = L"%d";
// GLOBAL: WIZ8 0x00614b58
const wchar_t g_format_d_slash_d_00614b58[] = L"%d/%d";
// GLOBAL: WIZ8 0x00617584
const wchar_t g_format_s_space_s_00617584[] = L"%s %s";
// GLOBAL: WIZ8 0x0064789c
const wchar_t g_dash_0064789c[] = L"-";
// GLOBAL: WIZ8 0x0064dc24
const wchar_t g_format_plus_d_0064dc24[] = L"%+d";
// GLOBAL: WIZ8 0x0064f2c0
const wchar_t g_zero_slash_zero_0064f2c0[] = L"0/0";

/* Skill name message ids indexed by skill id are declared with the stats
   page's shared tables in CharacterScreen.h. */

/* The stats page's columns are laid out against these two origin-relative
   frames; the profession bonus block's line height depends on how many traits
   the character has. */

/* The 0xEF700 subpanel entry: one record of an expanded value row.
   0x005C91C0 is the hierarchy's compiler-emitted destructor, so the class
   leaves it defaulted. */
// VTABLE: WIZ8 0x005ef700 W8CharacterStatsRecordControl005EF700
class W8CharacterStatsRecordControl005EF700 : public W8TextControl {
public:
    W8CharacterStatsRecordControl005EF700(Controls* owner, int top, int height,
                                          const W8CharacterStatsRecord* record, int variant);
    virtual void Redraw(int full_redraw) override;
    virtual void OnMouseEnter(int event) override;
    virtual void OnMouseLeave(int event) override;
    virtual void OnRightButtonUp(int event) override;

    const W8CharacterStatsRecord* m_record_0b8;
    unsigned char m_variant_0bc;
    unsigned char pad_0bd[3];
    int m_text_offset_0c0;
};
static_assert(sizeof(W8CharacterStatsRecordControl005EF700) == 0xc4,
              "W8CharacterStatsRecordControl005EF700_size");

// FUNCTION: WIZ8 0x005c8e70
W8CharacterStatsValue005EF6B0::W8CharacterStatsValue005EF6B0(
    Controls* owner, int x, int y, const W8CharacterStatsRecord* default_record)
    : W8TextControl(owner, 0xffffffff, x, y, 0, 0, 0x104, 0, 1, 1, 2, 2, 3)
{
    m_default_record_0bc = default_record;
    m_textBuffer.SetText(gppStringList[default_record->name_id_0c], g_options_detail_font_683614);
    m_textBuffer.SetLayoutMode(g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED548);
    m_pressedTextOffset = 0;
    UpdateTextBounds(x + 0x19, y, x + 0x7e, y + 0x16);
}

// SYNTHETIC: WIZ8 0x005c8f40
// W8CharacterStatsValue005EF6B0::`scalar deleting destructor'

/* Store the record the value control displays. A null record selects the
   default the constructor was handed. */
// FUNCTION: WIZ8 0x005c8fc0
void W8CharacterStatsValue005EF6B0::SetRecord(const W8CharacterStatsRecord* record)
{
    if (record == 0) {
        record = m_default_record_0bc;
    }
    m_record_0b8 = record;
    m_textBuffer.SetText(gppStringList[record->name_id_0c], g_options_detail_font_683614);
}

/* Redraw the current record's name over the value control's own text. */
// FUNCTION: WIZ8 0x005c9000
void W8CharacterStatsValue005EF6B0::Redraw(int full_redraw)
{
    W8TextControl::Redraw(full_redraw);
    if (!m_active || m_pPanel == 0) {
        return;
    }
    int left = m_pPanel->origin_x + m_left + 2;
    int top = m_pPanel->origin_y + m_top + 1;
    const W8CharacterStatsRecord* record = m_record_0b8;
    unsigned int image = m_enabled ? record->unknown_04 : record->unknown_08;
    DrawCatalogImage(-14, record->unknown_00, 0, image, left, top, 2, 0);
}

/* Force the control enabled while the base handles the right-button release,
   so a disabled value can still raise its info callback. */
// FUNCTION: WIZ8 0x005c92f0
void W8CharacterStatsValue005EF6B0::OnRightButtonUp(int event)
{
    unsigned char enabled = m_enabled;
    m_enabled = 1;
    W8TextControl::OnRightButtonUp(event);
    m_enabled = enabled;
}

// FUNCTION: WIZ8 0x005c9060
W8CharacterStatsRecordControl005EF700::W8CharacterStatsRecordControl005EF700(
    Controls* owner, int top, int height, const W8CharacterStatsRecord* record, int variant)
    : W8TextControl(owner, 0xffffffff, 0, top, 0x7e, top + height, 0x104, 0, 0, 0, 0, 0, 0)
{
    m_record_0b8 = record;
    m_variant_0bc = static_cast<unsigned char>(variant);
    int text = 0;
    if (variant == 0) {
        text = 4;
        m_text_offset_0c0 = 2;
    } else if (variant == 1) {
        text = 5;
        m_text_offset_0c0 = 0;
    } else if (variant == 2) {
        text = 6;
        m_text_offset_0c0 = 0;
    } else {
        text = variant;
    }
    AddLayoutFlags(g_W8TextControlMask005ED594);
    m_normalSprite = text;
    m_pressedSprite = text;
    m_alternatePressedSprite = text;
    m_alternateNormalSprite = text;
    m_disabledSprite = text;
    m_textBuffer.SetText(gppStringList[record->name_id_0c], g_options_detail_font_683614);
    m_textBuffer.SetLayoutMode(g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED548);
    m_pressedTextOffset = 0;
    UpdateTextBounds(0x19, m_text_offset_0c0 + top, 0x7e, m_text_offset_0c0 + top + 0x16);
}

// SYNTHETIC: WIZ8 0x005c91a0
// W8CharacterStatsRecordControl005EF700::`scalar deleting destructor'

/* Redraw the record's catalogue image over the entry's own background. */
// FUNCTION: WIZ8 0x005c9220
void W8CharacterStatsRecordControl005EF700::Redraw(int full_redraw)
{
    W8TextControl::Redraw(full_redraw);
    if (!m_active || m_pPanel == 0) {
        return;
    }
    int left = m_pPanel->origin_x + m_left + 2;
    int top = m_pPanel->origin_y + m_text_offset_0c0 + m_top + 1;
    const W8CharacterStatsRecord* record = m_record_0b8;
    unsigned int image = m_enabled ? record->unknown_04 : record->unknown_08;
    DrawCatalogImage(-14, record->unknown_00, 0, image, left, top, 2, 0);
}

// FUNCTION: WIZ8 0x005c9290
void W8CharacterStatsRecordControl005EF700::OnMouseEnter(int event)
{
    W8TextControl::OnMouseEnter(event);
    if (m_active && m_enabled) {
        m_textBuffer.m_fontStateIndex = 0xd;
    }
}

// FUNCTION: WIZ8 0x005c92c0
void W8CharacterStatsRecordControl005EF700::OnMouseLeave(int event)
{
    W8TextControl::OnMouseLeave(event);
    if (m_active) {
        m_textBuffer.m_fontStateIndex = -1;
        Invalidate(0);
    }
}

/* The record control's right-button release is the same folded body as the
   value control's; /OPT:NOICF emits its own copy. */
void W8CharacterStatsRecordControl005EF700::OnRightButtonUp(int event)
{
    unsigned char enabled = m_enabled;
    m_enabled = 1;
    W8TextControl::OnRightButtonUp(event);
    m_enabled = enabled;
}

// FUNCTION: WIZ8 0x005c9310
void W8CharacterStatsRow005EF750::Initialize(Controls* owner, unsigned int* region_set, int x,
                                             int y, int count, const W8CharacterStatsRecord* table,
                                             const W8CharacterStatsRecord* default_record,
                                             int help_first, int help_second, int help_value)
{
    m_table_018 = table;
    m_count_008 = static_cast<unsigned short>(count);
    m_x_00c = owner->origin_x + x;
    m_y_010 = owner->origin_y + y;
    m_region_set_014 = region_set;
    if (count == 0) {
        srAssertFail("usItemsInList", CGS_STATS_PAGE_CPP, 0x1d6, 0);
    }

    m_decrement_01c =
        new W8TextControl(owner, 0xffffffff, x + 1, y + 4, 0, 0, 0x10a, 0, 0, 2, 1, 4, 3);
    m_decrement_01c->m_listener = this;
    m_decrement_01c->EnableRegionHelp(help_first);

    m_increment_020 =
        new W8TextControl(owner, 0xffffffff, x + 0x9f, y + 4, 0, 0, 0x10a, 0, 5, 7, 6, 9, 8);
    m_increment_020->m_listener = this;
    m_increment_020->EnableRegionHelp(help_value);

    m_value_control_024 = new W8CharacterStatsValue005EF6B0(owner, x + 0x1c, y + 4, default_record);
    m_value_control_024->AddLayoutFlags(g_W8TextControlMask005ED578);
    m_value_control_024->m_listener = this;
    m_value_control_024->EnableRegionHelp(help_second);

    m_decrement_01c->SetActive(1);
    m_increment_020->SetActive(1);
    m_value_control_024->SetActive(1);
}

/* Build the expanded record list once, then enable it and mirror each
   record's selectable flag onto its entry control. */
// FUNCTION: WIZ8 0x005c94e0
void W8CharacterStatsRow005EF750::BuildSubpanel()
{
    if (m_subpanel_028 == 0) {
        m_subpanel_028 = new Controls(m_x_00c + 0x9e, m_y_010 + 1, m_x_00c + 0x11c,
                                      m_y_010 + 5 + (unsigned int)m_count_008 * 0x16, -1, 0, -1);
        m_subpanel_028->AcquireRegionSet(m_region_set_014);
        m_subpanel_entries_02c = new W8TextControl*[m_count_008];

        for (unsigned int index = 0; index < m_count_008; ++index) {
            int variant;
            int top;
            int height;
            if (index == 0) {
                variant = 0;
                top = 0;
                height = 0x18;
            } else if (index == m_count_008 - 1) {
                variant = 2;
                top = index * 0x16 + 2;
                height = 0x18;
            } else {
                variant = 1;
                top = index * 0x16 + 2;
                height = 0x16;
            }
            m_subpanel_entries_02c[index] = new W8CharacterStatsRecordControl005EF700(
                m_subpanel_028, top, height, &m_table_018[index], variant);
            m_subpanel_entries_02c[index]->m_listener = this;
        }
    }
    m_subpanel_028->SetEnabled(1);
    m_subpanel_028->EnableRegionSet(1);
    for (unsigned int index = 0; index < m_count_008; ++index) {
        m_subpanel_entries_02c[index]->SetEnabled(m_table_018[index].enabled_0e);
    }
}

/* Select one record by index, updating the value control and telling the
   owning page when the selection actually moved. */
// FUNCTION: WIZ8 0x005c96c0
void W8CharacterStatsRow005EF750::SetValue(int index)
{
    if (index == -1) {
        m_value_control_024->SetRecord(0);
    } else {
        m_value_control_024->SetRecord(&m_table_018[index]);
    }
    int previous = m_value_004;
    m_value_004 = index;
    m_value_control_024->Invalidate(1);
    if (m_listener_030 != 0 && previous != index) {
        m_listener_030->OnRowValueChanged(this, index);
    }
}

// FUNCTION: WIZ8 0x005c9760
void W8CharacterStatsRow005EF750::OnPrimary(W8TextControl* control)
{
    if (control == m_decrement_01c) {
        int previous = m_value_004;
        int index = previous - 1;
        if (index >= 0) {
            const W8CharacterStatsRecord* record = &m_table_018[index];
            do {
                if (record->enabled_0e != 0) {
                    m_value_control_024->SetRecord(index == -1 ? 0 : &m_table_018[index]);
                    unsigned char changed = previous != index;
                    m_value_004 = index;
                    m_value_control_024->Invalidate(1);
                    if (m_listener_030 == 0 || !changed) {
                        return;
                    }
                    m_listener_030->OnRowValueChanged(this, index);
                    return;
                }
                --index;
                --record;
            } while (index >= 0);
        }
        index = m_count_008 - 1;
        if (previous < index) {
            const W8CharacterStatsRecord* record = &m_table_018[index];
            while (record->enabled_0e == 0) {
                --index;
                --record;
                if (index <= previous) {
                    return;
                }
            }
            m_value_control_024->SetRecord(index == -1 ? 0 : &m_table_018[index]);
            unsigned char changed = previous != index;
            m_value_004 = index;
            m_value_control_024->Invalidate(1);
            if (m_listener_030 != 0 && changed) {
                m_listener_030->OnRowValueChanged(this, index);
            }
        }
    } else if (control == m_increment_020) {
        int previous = m_value_004;
        int index = previous + 1;
        if (index < (int)(unsigned int)m_count_008) {
            const W8CharacterStatsRecord* record = &m_table_018[index];
            do {
                if (record->enabled_0e != 0) {
                    m_value_control_024->SetRecord(index == -1 ? 0 : &m_table_018[index]);
                    unsigned char changed = previous != index;
                    m_value_004 = index;
                    m_value_control_024->Invalidate(1);
                    if (m_listener_030 == 0 || !changed) {
                        return;
                    }
                    m_listener_030->OnRowValueChanged(this, index);
                    return;
                }
                ++index;
                ++record;
            } while (index < (int)(unsigned int)m_count_008);
        }
        index = 0;
        if (previous > 0) {
            const W8CharacterStatsRecord* record = &m_table_018[0];
            while (record->enabled_0e == 0) {
                ++index;
                ++record;
                if (index >= previous) {
                    return;
                }
            }
            m_value_control_024->SetRecord(index == -1 ? 0 : &m_table_018[index]);
            unsigned char changed = previous != index;
            m_value_004 = index;
            m_value_control_024->Invalidate(1);
            if (m_listener_030 != 0 && changed) {
                m_listener_030->OnRowValueChanged(this, index);
            }
        }
    } else if (control == m_value_control_024) {
        if ((m_value_control_024->m_stateFlags & g_W8TextControlMask005ED570) != 0) {
            BuildSubpanel();
            m_subpanel_028->Invalidate(0);
            if (m_listener_030 != 0) {
                m_listener_030->OnRowExpanded(this);
            }
        } else {
            m_subpanel_028->SetEnabled(0);
            m_subpanel_028->EnableRegionSet(0);
            m_value_control_024->DisableSecondaryState(1);
            if (m_listener_030 != 0) {
                m_listener_030->OnRowCollapsed(this);
            }
        }
    } else {
        for (unsigned int index = 0; index < m_count_008; ++index) {
            if (control == m_subpanel_entries_02c[index]) {
                control->OnMouseLeave(0);
                SetValue(index);
                m_subpanel_028->SetEnabled(0);
                m_subpanel_028->EnableRegionSet(0);
                m_value_control_024->DisableSecondaryState(1);
                if (m_listener_030 != 0) {
                    m_listener_030->OnRowCollapsed(this);
                }
            }
        }
    }
}

// FUNCTION: WIZ8 0x005c9a50
void W8CharacterStatsRow005EF750::OnSecondary(W8TextControl* control)
{
    if (control != m_decrement_01c && control != m_increment_020) {
        if (control == m_value_control_024) {
            if (m_value_004 != -1) {
                m_listener_030->OnRowInfoRequested(this, m_value_004);
            }
        } else {
            for (unsigned int index = 0; index < m_count_008; ++index) {
                if (control == m_subpanel_entries_02c[index]) {
                    control->OnMouseLeave(0);
                    m_listener_030->OnRowInfoRequested(this, index);
                }
            }
        }
    }
}

/* The row's default state: no selection, no table, no child controls. */
W8CharacterStatsRow005EF750::W8CharacterStatsRow005EF750()
    : m_value_004(-1), m_count_008(0), m_table_018(0), m_decrement_01c(0), m_increment_020(0),
      m_value_control_024(0), m_subpanel_028(0), m_subpanel_entries_02c(0), m_listener_030(0)
{
}

/* Send every row control through its enabled state, redraw the values, and
   refresh the character-wide figures the rows depend on. */
// FUNCTION: WIZ8 0x005ca140
void W8CharacterPage005EF778::Activate()
{
    EnableRegionSet(1);
    UpdateRowValues();
    if (m_character_060->race != -1 || m_character_060->current_profession != -1) {
        for (int index = 0; index < m_entries_04c.count; ++index) {
            if (!m_rows_initialized_089) {
                m_entries_04c.data[index]->SetEnabled(1);
            }
            m_entries_04c.data[index]->UpdateButtons();
        }
        m_rows_initialized_089 = 1;
    }
    for (int index = 0; index < 5; ++index) {
        m_attribute_controls_08c[index]->SetActive(1);
        m_attribute_controls_08c[index]->SetEnabled(1);
    }
    m_dirty_06d = 1;
    m_prepared_06c = 1;
}

/* The skills page's Deactivate is the same one-call body; the linker folded
   both onto this address, so only this definition carries the marker. */
// FUNCTION: WIZ8 0x005ca1f0
void W8CharacterPage005EF778::Deactivate()
{
    EnableRegionSet(0);
}

/* Rebuild the three row displays from the character's current profession,
   race and sex. In creation mode the profession list is opened up
   entirely; in level-up mode only the eligible professions stay enabled. */
// FUNCTION: WIZ8 0x005ca200
void W8CharacterPage005EF778::UpdateRowValues()
{
    unsigned char eligible[15];

    if (m_mode_068 == 0) {
        if (m_character_060->current_profession == 2) {
            g_character_gender_records_0064f218[0].enabled_0e = 0;
        } else {
            g_character_gender_records_0064f218[0].enabled_0e = 1;
            g_character_gender_records_0064f218[1].enabled_0e = 1;
        }
    } else if (m_mode_068 == 2) {
        DetermineEligibleProfessions(m_character_060, m_creation_state_064, eligible);
    }
    for (int index = 0; index < 15; ++index) {
        if (m_mode_068 == 0) {
            g_character_profession_records_0064f028[index].enabled_0e = 1;
        } else if (m_mode_068 == 2) {
            g_character_profession_records_0064f028[index].enabled_0e = eligible[index];
        }
    }

    int profession = m_character_060->current_profession;
    W8CharacterStatsRow005EF750* row = m_profession_row_07c;
    if (profession == -1) {
        row->m_value_control_024->SetRecord(0);
    } else {
        row->m_value_control_024->SetRecord(&row->m_table_018[profession]);
    }
    int previous = row->m_value_004;
    row->m_value_004 = profession;
    row->m_value_control_024->Invalidate(1);
    if (row->m_listener_030 != 0 && previous != profession) {
        row->m_listener_030->OnRowValueChanged(row, profession);
    }
    row->m_decrement_01c->Invalidate(0);
    row->m_increment_020->Invalidate(0);
    row->m_value_control_024->Invalidate(0);
    if (row->m_subpanel_028 != 0) {
        row->m_subpanel_028->Invalidate(0);
    }

    int race = m_character_060->race;
    row = m_race_row_080;
    if (race == -1) {
        row->m_value_control_024->SetRecord(0);
    } else {
        row->m_value_control_024->SetRecord(&row->m_table_018[race]);
    }
    previous = row->m_value_004;
    row->m_value_004 = race;
    row->m_value_control_024->Invalidate(1);
    if (row->m_listener_030 != 0 && previous != race) {
        row->m_listener_030->OnRowValueChanged(row, race);
    }
    row->m_decrement_01c->Invalidate(0);
    row->m_increment_020->Invalidate(0);
    row->m_value_control_024->Invalidate(0);
    if (row->m_subpanel_028 != 0) {
        row->m_subpanel_028->Invalidate(0);
    }

    int gender = m_character_060->gender;
    row = m_gender_row_084;
    if (gender == -1) {
        row->m_value_control_024->SetRecord(0);
    } else {
        row->m_value_control_024->SetRecord(&row->m_table_018[gender]);
    }
    previous = row->m_value_004;
    row->m_value_004 = gender;
    row->m_value_control_024->Invalidate(1);
    if (row->m_listener_030 != 0 && previous != gender) {
        row->m_listener_030->OnRowValueChanged(row, gender);
    }
    row->m_decrement_01c->Invalidate(0);
    row->m_increment_020->Invalidate(0);
    row->m_value_control_024->Invalidate(0);
    if (row->m_subpanel_028 != 0) {
        row->m_subpanel_028->Invalidate(0);
    }
}

/* The entries only need enabling the first time the rows become usable. */
// FUNCTION: WIZ8 0x005ca480
void W8CharacterPage005EF778::Refresh()
{
    UpdateRowValues();
    if (m_character_060->race != -1 || m_character_060->current_profession != -1) {
        for (int index = 0; index < m_entries_04c.count; ++index) {
            if (!m_rows_initialized_089) {
                m_entries_04c.data[index]->SetEnabled(1);
            }
            m_entries_04c.data[index]->UpdateButtons();
        }
        m_rows_initialized_089 = 1;
    }
}

/* Accept the page: refund everything the editing state still owes, then let
   the screen recompute its navigation buttons. */
// FUNCTION: WIZ8 0x005ca4f0
void W8CharacterPage005EF778::Accept()
{
    RefundAllocatedAttributes(m_character_060, m_creation_state_064);
    Invalidate(0);
    m_dirty_06d = 1;
    m_screen_05c->UpdateNavigation(this);
    for (int index = 0; index < m_entries_04c.count; ++index) {
        m_entries_04c.data[index]->UpdateButtons();
    }
}

/* The next button needs a complete attribute allocation; exit is available
   once any points are committed. */
// FUNCTION: WIZ8 0x005ca550
void W8CharacterPage005EF778::GetNavigationState(bool* next_enabled, bool* exit_enabled)
{
    if (!m_creation_state_064->attributes_complete || m_character_060->current_profession == -1 ||
        m_character_060->race == -1 || m_character_060->gender == -1) {
        *next_enabled = false;
    } else {
        *next_enabled = true;
    }
    *exit_enabled = m_creation_state_064->attribute_points_remaining <
                    m_creation_state_064->attribute_points_total;
    if (*next_enabled != m_navigation_state_088) {
        for (int index = 0; index < m_entries_04c.count; ++index) {
            m_entries_04c.data[index]->SetIncrementAllowed(!*next_enabled);
        }
        m_navigation_state_088 = *next_enabled;
    }
}

/* A click anywhere closes an expanded row's record list. */
// FUNCTION: WIZ8 0x005ca5e0
void W8CharacterPage005EF778::HandleInput(InputAtom* input)
{
    if (input->usEvent != LEFT_BUTTON_DOWN && input->usEvent != RIGHT_BUTTON_DOWN) {
        return;
    }
    W8CharacterStatsRow005EF750* row = m_profession_row_07c;
    if (row->m_subpanel_028 != 0 && row->m_subpanel_028->m_fEnabled) {
        unsigned int index = 0;
        while (index < row->m_count_008) {
            if (row->m_subpanel_entries_02c[index]->m_active) {
                goto next_profession;
            }
            ++index;
        }
        if (!row->m_value_control_024->m_active) {
            row->m_subpanel_028->SetEnabled(0);
            row->m_subpanel_028->EnableRegionSet(0);
            row->m_value_control_024->DisableSecondaryState(1);
            if (row->m_listener_030 != 0) {
                row->m_listener_030->OnRowCollapsed(row);
            }
        }
    }
next_profession:
    row = m_race_row_080;
    if (row->m_subpanel_028 != 0 && row->m_subpanel_028->m_fEnabled) {
        unsigned int index = 0;
        while (index < row->m_count_008) {
            if (row->m_subpanel_entries_02c[index]->m_active) {
                goto next_race;
            }
            ++index;
        }
        if (!row->m_value_control_024->m_active) {
            row->m_subpanel_028->SetEnabled(0);
            row->m_subpanel_028->EnableRegionSet(0);
            row->m_value_control_024->DisableSecondaryState(1);
            if (row->m_listener_030 != 0) {
                row->m_listener_030->OnRowCollapsed(row);
            }
        }
    }
next_race:
    row = m_gender_row_084;
    if (row->m_subpanel_028 != 0 && row->m_subpanel_028->m_fEnabled) {
        unsigned int index = 0;
        while (index < row->m_count_008) {
            if (row->m_subpanel_entries_02c[index]->m_active) {
                return;
            }
            ++index;
        }
        if (!row->m_value_control_024->m_active) {
            row->m_subpanel_028->SetEnabled(0);
            row->m_subpanel_028->EnableRegionSet(0);
            row->m_value_control_024->DisableSecondaryState(1);
            if (row->m_listener_030 != 0) {
                row->m_listener_030->OnRowCollapsed(row);
            }
        }
    }
}

/* One attribute entry was adjusted: apply the point through the shared
   creation-state helper, then show every skill the maxed attribute gates. */
// FUNCTION: WIZ8 0x005ca730
void W8CharacterPage005EF778::AdjustEntry(W8CharacterPageEntry* entry, int delta)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
    /* Retail compiled this comparison with VC6's mixed-sign operands; the
   signedness is part of the recovered body and changing it would change
   the compare and branch. Suppress only this diagnostic here. */
    entry->MarkDirty();
    m_dirty_06d = 1;
    AdjustAllocatedAttribute(m_character_060, m_creation_state_064, entry->m_id_02c, delta);
    Invalidate(0);
    m_screen_05c->UpdateNavigation(this);
    if (m_character_060->attributes[entry->m_id_02c].value >= 100) {
        for (int skill = 0x22; skill < 0x29; ++skill) {
            if (g_skill_attributes[skill].category == entry->m_id_02c &&
                m_character_060->skills[skill].flag_00 != 0) {
                m_screen_05c->ShowDescription(entry->m_id_02c, skill);
            }
        }
    }
#pragma clang diagnostic pop
}

// FUNCTION: WIZ8 0x005ca7e0
void W8CharacterPage005EF778::ShowEntryInfo(W8CharacterPageEntry* entry)
{
    m_screen_05c->ShowAttributeInfo005B07C0(entry->m_id_02c);
}

/* A value row moved: rerun the whole creation rebuild for the new
   profession, race or sex, then refresh the row controls. */
// FUNCTION: WIZ8 0x005ca800
void W8CharacterPage005EF778::OnRowValueChanged(W8CharacterStatsRow005EF750* row, int value)
{
    if (row == m_profession_row_07c) {
        RebuildLevelUpPoolsForProfession(m_character_060, m_creation_state_064,
                                         static_cast<W8Profession>(value));
    } else if (row == m_race_row_080) {
        SetCharacterRace(m_character_060, m_creation_state_064, value);
    } else {
        SetCharacterGender(m_character_060, m_creation_state_064, static_cast<W8Gender>(value));
    }
    m_dirty_06d = 1;
    m_screen_05c->UpdateNavigation(this);
    UpdateRowValues();
    if (m_character_060->race != -1 || m_character_060->current_profession != -1) {
        for (int index = 0; index < m_entries_04c.count; ++index) {
            if (!m_rows_initialized_089) {
                m_entries_04c.data[index]->SetEnabled(1);
            }
            m_entries_04c.data[index]->UpdateButtons();
        }
        m_rows_initialized_089 = 1;
    }
}

/* A row opened its record list: park the row and attribute controls so the
   list owns the input. */
// FUNCTION: WIZ8 0x005ca8d0
void W8CharacterPage005EF778::OnRowExpanded(W8CharacterStatsRow005EF750* row)
{
    if (row == m_profession_row_07c) {
        m_profession_row_07c->m_increment_020->SetActive(0);
        m_race_row_080->m_increment_020->SetActive(0);
        m_gender_row_084->m_increment_020->SetActive(0);
    } else if (row == m_race_row_080) {
        m_race_row_080->m_increment_020->SetActive(0);
        m_gender_row_084->m_increment_020->SetActive(0);
    } else {
        m_gender_row_084->m_increment_020->SetActive(0);
    }
    for (int entry_index = 0; entry_index < m_entries_04c.count; ++entry_index) {
        m_entries_04c.data[entry_index]->SetHelpActive005AFAE0(0);
    }
    for (int control_index = 0; control_index < 5; ++control_index) {
        m_attribute_controls_08c[control_index]->SetActive(0);
    }
}

/* The record list closed: restore the row and attribute controls. */
// FUNCTION: WIZ8 0x005ca970
void W8CharacterPage005EF778::OnRowCollapsed(W8CharacterStatsRow005EF750* row)
{
    if (row == m_profession_row_07c) {
        m_profession_row_07c->m_increment_020->SetActive(1);
        m_race_row_080->m_increment_020->SetActive(1);
        m_gender_row_084->m_increment_020->SetActive(1);
    } else if (row == m_race_row_080) {
        m_race_row_080->m_increment_020->SetActive(1);
        m_gender_row_084->m_increment_020->SetActive(1);
    } else {
        m_gender_row_084->m_increment_020->SetActive(1);
    }
    for (int entry_index = 0; entry_index < m_entries_04c.count; ++entry_index) {
        m_entries_04c.data[entry_index]->SetHelpActive005AFAE0(1);
    }
    for (int control_index = 0; control_index < 5; ++control_index) {
        m_attribute_controls_08c[control_index]->SetActive(1);
    }
    Invalidate(0);
}

/* Right-clicking a row's value asks the screen for that entry's info. */
// FUNCTION: WIZ8 0x005caa20
void W8CharacterPage005EF778::OnRowInfoRequested(W8CharacterStatsRow005EF750* row, int value)
{
    if (row == m_profession_row_07c) {
        m_screen_05c->ShowProfessionInfo(value);
        return;
    }
    if (row == m_race_row_080) {
        m_screen_05c->ShowRaceInfo(value);
    }
}

/* The five coloured attribute-corner controls name the attribute whose info
   dialog to raise. */
// FUNCTION: WIZ8 0x005caa50
void W8CharacterPage005EF778::OnSecondary(W8TextControl* control)
{
    for (unsigned int index = 0; index < 5; ++index) {
        if (control == m_attribute_controls_08c[index]) {
            m_screen_05c->ShowAttributeInfo005B0850(index);
        }
    }
}

// FUNCTION: WIZ8 0x005caa80
void W8CharacterPage005EF778::Prepare()
{
    W8CharacterPage::Prepare();
    W8CharacterStatsRow005EF750* rows[3] = {
        m_profession_row_07c,
        m_race_row_080,
        m_gender_row_084,
    };
    for (int index = 0; index < 3; ++index) {
        rows[index]->m_decrement_01c->Invalidate(0);
        rows[index]->m_increment_020->Invalidate(0);
        rows[index]->m_value_control_024->Invalidate(0);
        if (rows[index]->m_subpanel_028 != 0) {
            rows[index]->m_subpanel_028->Invalidate(0);
        }
    }
}

/* The whole page: three value rows, seven attribute entries, five attribute
   corner controls, then the profession/race/sex list state. */
// FUNCTION: WIZ8 0x005c9c80
void W8CharacterPage005EF778::SetCharacter(W8Character* character,
                                           W8CharacterCreationState* creation_state, int mode)
{
    W8CharacterPage::SetCharacter(character, creation_state, mode);
    m_profession_row_07c = new W8CharacterStatsRow005EF750;
    m_race_row_080 = new W8CharacterStatsRow005EF750;
    m_gender_row_084 = new W8CharacterStatsRow005EF750;
    AcquireRegionSet(&g_character_stats_region_set_0069c550);
    m_profession_row_07c->Initialize(this, &g_character_stats_profession_region_set_0069c554, 0x16,
                                     10, 0xf, g_character_profession_records_0064f028,
                                     &g_character_profession_default_record_0064f248, 0xf7, 0xf6,
                                     0xf8);
    m_race_row_080->Initialize(this, &g_character_stats_race_region_set_0069c558, 0x16, 0x3d, 0xb,
                               g_character_race_records_0064f118,
                               &g_character_race_default_record_0064f258, 0xfa, 0xf9, 0xfb);
    m_gender_row_084->Initialize(this, &g_character_stats_gender_region_set_0069c55c, 0x16, 0x70, 2,
                                 g_character_gender_records_0064f218,
                                 &g_character_gender_default_record_0064f268, 0xfd, 0xfc, 0xfe);
    m_profession_row_07c->m_listener_030 = this;
    m_race_row_080->m_listener_030 = this;
    m_gender_row_084->m_listener_030 = this;

    if (mode == 0) {
        W8CharacterStatsRow005EF750* rows[3] = {
            m_profession_row_07c,
            m_race_row_080,
            m_gender_row_084,
        };
        for (int row_index = 0; row_index < 3; ++row_index) {
            rows[row_index]->m_decrement_01c->SetEnabled(1);
            rows[row_index]->m_increment_020->SetEnabled(1);
            rows[row_index]->m_value_control_024->SetEnabled(1);
        }
    } else if (mode == 1) {
        W8CharacterStatsRow005EF750* rows[3] = {
            m_profession_row_07c,
            m_race_row_080,
            m_gender_row_084,
        };
        for (int row_index = 0; row_index < 3; ++row_index) {
            rows[row_index]->m_decrement_01c->SetEnabled(0);
            rows[row_index]->m_increment_020->SetEnabled(0);
            rows[row_index]->m_value_control_024->SetEnabled(0);
        }
    } else if (mode == 2) {
        if (character->race == 0xf) {
            m_profession_row_07c->m_decrement_01c->SetEnabled(0);
            m_profession_row_07c->m_increment_020->SetEnabled(0);
            m_profession_row_07c->m_value_control_024->SetEnabled(0);
        } else {
            m_profession_row_07c->m_decrement_01c->SetEnabled(1);
            m_profession_row_07c->m_increment_020->SetEnabled(1);
            m_profession_row_07c->m_value_control_024->SetEnabled(1);
        }
        m_race_row_080->m_decrement_01c->SetEnabled(0);
        m_race_row_080->m_increment_020->SetEnabled(0);
        m_race_row_080->m_value_control_024->SetEnabled(0);
        m_gender_row_084->m_decrement_01c->SetEnabled(0);
        m_gender_row_084->m_increment_020->SetEnabled(0);
        m_gender_row_084->m_value_control_024->SetEnabled(0);
    }

    for (int attribute_index = 0; attribute_index < 7; ++attribute_index) {
        W8CharacterPageEntry* entry =
            new W8CharacterPageEntry(this, 0xe5, 0x1b + attribute_index * 0xe, 0);
        AddEntry(entry);
        entry->m_listener_004 = this;
        entry->SetContent(attribute_index,
                          gppStringList[g_character_description_first_ids_61e3a4[attribute_index]],
                          &character->attributes[attribute_index].value,
                          &creation_state->attribute_values_008[attribute_index],
                          &creation_state->attribute_limits_028[attribute_index], 0x101);
        entry->SetEnabled(0);
    }
    m_navigation_state_088 = false;
    m_rows_initialized_089 = 0;

    for (int control_index = 0; control_index < 5; ++control_index) {
        W8TextControl* control =
            new W8TextControl(this, 0xffffffff, 0xf3, 0xba + control_index * 0xe, 0x195,
                              0xba + control_index * 0xe + 0xc, -1, -1, -1, -1, -1, -1, -1);
        m_attribute_controls_08c[control_index] = control;
        control->SetActive(0);
        control->m_listener = this;
        control->EnableRegionHelp(0x101);
    }
}

// SYNTHETIC: WIZ8 0x005c9ac0
// W8CharacterPage005EF778::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005c9ae0
W8CharacterPage005EF778::~W8CharacterPage005EF778()
{
    W8CharacterStatsRow005EF750* rows[3] = {
        m_profession_row_07c,
        m_race_row_080,
        m_gender_row_084,
    };
    for (int index = 0; index < 3; ++index) {
        W8CharacterStatsRow005EF750* row = rows[index];
        if (row != 0) {
            delete row->m_subpanel_028;
            if (row->m_subpanel_entries_02c != 0) {
                for (unsigned int entry = 0; entry < row->m_count_008; ++entry) {
                    delete row->m_subpanel_entries_02c[entry];
                }
                delete[] row->m_subpanel_entries_02c;
            }
            delete row;
        }
    }
    W8CharacterPage::~W8CharacterPage();
}

/* The page redraw: header figures, the seven attribute entries' section,
   the three value rows' labels, then the resistance and trait blocks. */
// FUNCTION: WIZ8 0x005cab20
void W8CharacterPage005EF778::Redraw()
{
    unsigned char redraw = static_cast<unsigned char>(m_fEnabled && m_fDirty);
    W8TextBuffer text;
    int left = origin_x;
    int top = origin_y;
    W8CharacterPage::Redraw();
    if (redraw) {
        W8ControlsRect bounds;
        text.SetLayoutMode(g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C);
        bounds.left = left + 0xe5;
        bounds.right = left + 0x1a8;
        bounds.top = top + 0xe;
        bounds.bottom = top + 0x1a;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0x96], g_font_683660);
        text.RenderToTarget(0, 0, -14);

        if (m_character_060->race != -1) {
            bounds.top = top + 0x7e;
            bounds.bottom = top + 0x8a;
            bounds.left = left + 0xf9;
            bounds.right = left + 0x10f;
            text.SetLayoutBounds(&bounds, 1, 1);
            text.SetText(FormatWideString(g_format_d_0060aa20,
                                          m_creation_state_064->attribute_points_remaining),
                         g_font_683660);
            text.RenderToTarget(0, 0, -14);
        }

        text.SetLayoutMode(g_W8TextBufferLayoutMask005ED548 | g_W8TextBufferLayoutMask005ED554);
        bounds.right = left + 0x1a8;
        bounds.top = top + 0x7e;
        bounds.bottom = top + 0x8a;
        bounds.left = left + 0x116;
        int saved_right = bounds.right;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0x21c], g_font_683660);
        text.RenderToTarget(0, 0, -14);

        text.SetLayoutMode(g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C);
        bounds.left = left + 0x1a;
        bounds.right = left + 0xe4;
        bounds.top = top + 0xac;
        bounds.bottom = top + 0xb8;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0x2c4], g_font_683660);
        text.RenderToTarget(0, 0, -14);

        text.SetLayoutMode(g_W8TextBufferLayoutMask005ED558 | g_W8TextBufferLayoutMask005ED548);
        bounds.left = left + 0x1d;
        bounds.bottom = top + 0x133;
        bounds.top = top + 0xbb;
        bounds.right = left + 0xe6;
        if (m_character_060->current_profession != -1 || m_character_060->race != -1) {
            unsigned char available[0x20];
            unsigned int available_count = 0;
            for (int trait = 0; trait < 0x20; ++trait) {
                if (CharacterHasTrait00547940(m_character_060, trait)) {
                    available[trait] = 1;
                    ++available_count;
                } else {
                    available[trait] = 0;
                }
            }
            int line_height = (available_count < 8) + 0xd;
            if (m_character_060->current_profession != -1) {
                text.SetLayoutBounds(&bounds, 1, 1);
                text.SetText(
                    FormatWideString(
                        g_format_s_space_s_00617584,
                        gppStringList
                            [g_character_skill_name_ids_61e454
                                 [g_profession_bonus_skills[m_character_060->current_profession]]],
                        gppStringList[0xb2]),
                    g_font_683660);
                text.RenderToTarget(0, 0, -14);
                bounds.top = bounds.top + line_height;
            }
            for (int trait_index = 0; trait_index < 0x20; ++trait_index) {
                if (available[trait_index] != 0) {
                    text.SetLayoutBounds(&bounds, 1, 1);
                    text.SetText(gppStringList[g_character_trait_name_ids_61e530[trait_index]],
                                 g_font_683660);
                    text.RenderToTarget(0, 0, -14);
                    bounds.top = bounds.top + line_height;
                }
            }
        }

        text.SetLayoutMode(g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C);
        bounds.left = left + 0xf3;
        bounds.right = left + 0x1a7;
        bounds.top = top + 0xac;
        bounds.bottom = top + 0xb8;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0x298], g_font_683660);
        text.RenderToTarget(0, 0, -14);

        bounds.left = left + 0xf8;
        bounds.top = top + 0xba;
        bounds.bottom = top + 0xc6;
        bounds.right = left + 0x195;
        text.SetLayoutMode(g_W8TextBufferLayoutMask005ED548 | g_W8TextBufferLayoutMask005ED554);
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0x29c], g_font_683660);
        text.RenderToTarget(0, 0, -14);
        if (m_character_060->race != -1) {
            text.SetLayoutMode(g_W8TextBufferLayoutMask005ED550 | g_W8TextBufferLayoutMask005ED554);
            text.SetText(FormatWideString(g_format_d_0060aa20, m_character_060->hp_max),
                         g_font_683660);
            text.RenderToTarget(0, 0, -14);
        }

        bounds.top = bounds.top + 0xe;
        bounds.bottom = bounds.bottom + 0xe;
        text.SetLayoutMode(g_W8TextBufferLayoutMask005ED548 | g_W8TextBufferLayoutMask005ED554);
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0x2a4], g_font_683660);
        text.RenderToTarget(0, 0, -14);
        if (m_character_060->race != -1) {
            text.SetLayoutMode(g_W8TextBufferLayoutMask005ED550 | g_W8TextBufferLayoutMask005ED554);
            text.SetText(FormatWideString(g_format_d_0060aa20, m_character_060->stamina_max),
                         g_font_683660);
            text.RenderToTarget(0, 0, -14);
        }

        bounds.bottom = bounds.bottom + 0xe;
        bounds.top = bounds.top + 0xe;
        text.SetLayoutMode(g_W8TextBufferLayoutMask005ED548 | g_W8TextBufferLayoutMask005ED554);
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0x3b4], g_font_683660);
        text.RenderToTarget(0, 0, -14);
        if (m_character_060->race != -1) {
            text.SetLayoutMode(g_W8TextBufferLayoutMask005ED550 | g_W8TextBufferLayoutMask005ED554);
            text.SetText(FormatWideString(
                             g_format_d_0060aa20,
                             ComputeLevelUpSpellPointAward(m_character_060, m_creation_state_064)),
                         g_font_683660);
            text.RenderToTarget(0, 0, -14);
        }

        bounds.top = bounds.top + 0xe;
        bounds.bottom = bounds.bottom + 0xe;
        text.SetLayoutMode(g_W8TextBufferLayoutMask005ED548 | g_W8TextBufferLayoutMask005ED554);
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0x2b4], g_font_683660);
        text.RenderToTarget(0, 0, -14);
        if (m_character_060->race != -1) {
            text.SetLayoutMode(g_W8TextBufferLayoutMask005ED550 | g_W8TextBufferLayoutMask005ED554);
            text.SetText(
                FormatWideString(g_format_d_0060aa20, m_character_060->armor_class_average),
                g_font_683660);
            text.RenderToTarget(0, 0, -14);
        }

        bounds.top = bounds.top + 0xe;
        bounds.bottom = bounds.bottom + 0xe;
        text.SetLayoutMode(g_W8TextBufferLayoutMask005ED548 | g_W8TextBufferLayoutMask005ED554);
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0x2bc], g_font_683660);
        text.RenderToTarget(0, 0, -14);
        if (m_character_060->race != -1) {
            text.SetLayoutMode(g_W8TextBufferLayoutMask005ED550 | g_W8TextBufferLayoutMask005ED554);
            text.SetText(
                FormatWideString(g_format_d_0060aa20, m_character_060->carrying_capacity / 10),
                g_font_683660);
            text.RenderToTarget(0, 0, -14);
        }

        text.SetLayoutMode(g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C);
        bounds.left = left + 0x1a;
        bounds.right = left + 0xe4;
        bounds.top = top + 0x140;
        bounds.bottom = top + 0x14c;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0x2e4], g_font_683660);
        text.RenderToTarget(0, 0, -14);
        if (m_character_060->race != -1 || m_character_060->current_profession != -1) {
            for (int realm = 0; realm < 6; ++realm) {
                bool first_column = (realm & 1) == 0;
                bounds.left = (realm / 2) * 0x3f + 0x39 + left;
                bounds.top = (first_column ? 0x157 : 0x171) + top;
                bounds.bottom = bounds.top + 0xe;
                bounds.right = bounds.left + 0x21;
                text.SetLayoutBounds(&bounds, 1, 1);
                int value = m_character_060->resistances[realm].total - 0x19;
                if (value == 0) {
                    text.SetText(FormatWideString(g_dash_0064789c), g_font_683660);
                } else {
                    text.SetText(FormatWideString(g_format_plus_d_0064dc24, value), g_font_683660);
                }
                text.RenderToTarget(0, 0, -14);
            }
        }
        for (int realm_index = 0; realm_index < 6; ++realm_index) {
            bool first_column = (realm_index & 1) == 0;
            int frame_top = (first_column ? 0x155 : 0x16f) + top;
            DrawCatalogImageAndInvalidate(
                -14, g_character_resistance_images_0064ce60[realm_index], 0,
                g_spell_realm_animations_00648c90[realm_index].initial_frame,
                (realm_index / 2) * 0x3f + 0x23 + left, frame_top, 2, 0);
        }

        text.SetLayoutMode(g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C);
        bounds.left = left + 0xf3;
        bounds.right = left + 0x1a7;
        bounds.top = top + 0x120;
        bounds.bottom = top + 0x12c;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0x2cc], g_font_683660);
        text.RenderToTarget(0, 0, -14);
        if (m_character_060->current_profession != -1) {
            text.SetLayoutMode(g_W8TextBufferLayoutMask005ED558 | g_W8TextBufferLayoutMask005ED548);
            bounds.top = top + 0x12f;
            bounds.bottom = top + 0x13b;
            bounds.left = left + 0xf6;
            bounds.right = left + 0x1a9;
            text.SetLayoutBounds(&bounds, 1, 1);
            text.SetText(
                gppStringList[g_character_skill_name_ids_61e454
                                  [g_profession_bonus_skills[m_character_060->current_profession]]],
                g_font_683660);
            text.RenderToTarget(0, 0, -14);
            for (int index = 0; index < 4; ++index) {
                if (g_profession_skills[m_character_060->current_profession][index] != -1) {
                    bounds.top = bounds.top + 0xe;
                    bounds.bottom = bounds.bottom + 0xe;
                    text.SetLayoutBounds(&bounds, 1, 1);
                    text.SetText(
                        gppStringList
                            [g_character_skill_name_ids_61e454
                                 [g_profession_skills[m_character_060->current_profession][index]]],
                        g_font_683660);
                    text.RenderToTarget(0, 0, -14);
                }
            }
            text.SetLayoutMode(g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C);
            bounds.left = left + 0x106;
            bounds.right = left + 0x11c;
            bounds.top = top + 0x179;
            bounds.bottom = top + 0x185;
            text.SetLayoutBounds(&bounds, 1, 1);
            text.SetText(
                FormatWideString(g_format_d_0060aa20, m_creation_state_064->skill_points_total),
                g_font_683660);
            text.RenderToTarget(0, 0, -14);
        }
        text.SetLayoutMode(g_W8TextBufferLayoutMask005ED548 | g_W8TextBufferLayoutMask005ED554);
        bounds.top = top + 0x179;
        bounds.left = left + 0x123;
        bounds.bottom = top + 0x185;
        bounds.right = saved_right;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0x21c], g_font_683660);
        text.RenderToTarget(0, 0, -14);
    }

    text.SetLayoutMode(g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C);
    if (m_prepared_06c) {
        W8ControlsRect bounds = {4, 0xec, 0xc2, 0x162};
        text.SetLayoutBounds(&bounds, 1, 1);
        if (m_mode_068 == 0) {
            text.SetText(gppStringList[0xe5], g_font_683660);
        } else if (m_character_060->race == 0xf) {
            text.SetText(gppStringList[0xe7], g_font_683660);
        } else {
            text.SetText(gppStringList[0xe6], g_font_683660);
        }
        text.RenderToTarget(0, 1, -14);
        bounds.top = 0x162;
        bounds.bottom = 0x179;
        bounds.right = 0x8f;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0xe2], g_font_683660);
        text.RenderToTarget(0, 1, -14);
        bounds.top = 0x184;
        bounds.bottom = 0x19b;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0xe4], g_font_683660);
        text.RenderToTarget(0, 1, -14);
        m_prepared_06c = 0;
    }

    if (m_dirty_06d) {
        W8ControlsRect bounds = {0x8f, 0x162, 0xbf, 0x179};
        DrawCatalogImage(-14, 0x107, 0, 5, 0x8f, 0x162, 2, 0);
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(
            FormatWideString(g_format_d_0060aa20, m_creation_state_064->attribute_step_limit),
            g_options_detail_font_683614);
        text.RenderToTarget(0, 1, -14);
        bounds.top = 0x184;
        bounds.bottom = 0x19b;
        DrawCatalogImage(-14, 0x107, 0, 5, 0x8f, 0x184, 2, 0);
        text.SetLayoutBounds(&bounds, 1, 1);
        int total = m_creation_state_064->attribute_points_total;
        if (total < 1) {
            text.SetText(const_cast<wchar_t*>(g_zero_slash_zero_0064f2c0),
                         g_options_detail_font_683614);
        } else {
            text.SetText(FormatWideString(g_format_d_slash_d_00614b58,
                                          m_creation_state_064->attribute_points_remaining, total),
                         g_options_detail_font_683614);
        }
        text.RenderToTarget(0, 1, -14);
        m_dirty_06d = 0;
    }

    W8CharacterStatsRow005EF750* rows[3] = {
        m_profession_row_07c,
        m_race_row_080,
        m_gender_row_084,
    };
    for (int row_index = 0; row_index < 3; ++row_index) {
        W8CharacterStatsRow005EF750* row = rows[row_index];
        if (row->m_subpanel_028 != 0 && row->m_subpanel_028->m_fEnabled) {
            row->m_subpanel_028->Redraw();
        }
    }
}

/* Factory for the stats page; the render target comes from the base
   constructor. */
// FUNCTION: WIZ8 0x005cba90
W8CharacterPage005EF778* CreateCharacterPage005CBA90()
{
    return new W8CharacterPage005EF778;
}
