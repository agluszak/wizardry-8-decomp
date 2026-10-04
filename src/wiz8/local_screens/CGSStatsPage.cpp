#include "wiz8/local_code/Widget.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/local_screens/CharacterScreen.h"

#include "wiz8/layouts/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/fonts.h"
#include "wiz8/local_code/ControlsRect.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/video_object_catalog.h"

#include "input.h"

#include <new>

#define CGS_STATS_PAGE_CPP "C:\\Projects\\Wizardry 8\\Local Screens\\CGSStatsPage.cpp"

// GLOBAL: WIZ8 0x0069c550
unsigned int g_character_stats_region_set;
// GLOBAL: WIZ8 0x0069c554
static unsigned int g_character_stats_profession_region_set;
// GLOBAL: WIZ8 0x0069c558
static unsigned int g_character_stats_race_region_set;
// GLOBAL: WIZ8 0x0069c55c
static unsigned int g_character_stats_gender_region_set;

// GLOBAL: WIZ8 0x0064f028
static W8CharacterStatsRecord g_character_profession_records[15] = {
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
static W8CharacterStatsRecord g_character_race_records[11] = {
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
static W8CharacterStatsRecord g_character_gender_records[2] = {
    {0x0000010d, 0x00000000, 0x00000001, 0x02d1, 0, 0},
    {0x0000010d, 0x00000002, 0x00000003, 0x02d2, 0, 0},
};
// GLOBAL: WIZ8 0x0064f248
static W8CharacterStatsRecord g_character_profession_default_record = {
    0x0000010e, 0x00000002, 0x00000003, 0x008f, 1, 0,
};
// GLOBAL: WIZ8 0x0064f258
static W8CharacterStatsRecord g_character_race_default_record = {
    0x0000010e, 0x00000000, 0x00000001, 0x0090, 1, 0,
};
// GLOBAL: WIZ8 0x0064f268
static W8CharacterStatsRecord g_character_gender_default_record = {
    0x0000010e, 0x00000004, 0x00000005, 0x0091, 1, 0,
};

/* The six resistance icons drawn beside the stats page's resistance values. */
// GLOBAL: WIZ8 0x0064ce60
int g_character_resistance_images[6] = {
    0x193, 0x194, 0x195, 0x196, 0x197, 0x198,
};
// GLOBAL: WIZ8 0x0061e530
unsigned short g_character_trait_name_ids[0x20] = {
    0x328, 0x329, 0x32a, 0x32b, 0x32c, 0x32d, 0x32e, 0x32f, 0x330, 0x331, 0x332,
    0x333, 0x334, 0x335, 0x336, 0x337, 0x338, 0x339, 0x33a, 0x33b, 0x33c, 0x33d,
    0x33e, 0x33f, 0x340, 0x341, 0x342, 0x343, 0x344, 0x345, 0x346, 0x347,
};
// GLOBAL: WIZ8 0x0060aa20
wchar_t g_format_d[] = L"%d";
// GLOBAL: WIZ8 0x00614b58
wchar_t g_format_d_slash_d[] = L"%d/%d";
// GLOBAL: WIZ8 0x00617584
wchar_t g_format_s_space_s[] = L"%s %s";
// GLOBAL: WIZ8 0x0064789c
wchar_t g_dash[] = L"-";
// GLOBAL: WIZ8 0x0064dc24
wchar_t g_format_plus_d[] = L"%+d";
// GLOBAL: WIZ8 0x0064f2c0
static wchar_t g_zero_slash_zero[] = L"0/0";

/* Skill name message ids indexed by skill id are declared with the stats
   page's shared tables in CharacterScreen.h. */

/* The stats page's columns are laid out against these two origin-relative
   frames; the profession bonus block's line height depends on how many traits
   the character has. */

/* The 0xEF700 subpanel entry: one record of an expanded value row.
   0x005C91C0 is the hierarchy's compiler-emitted destructor, so the class
   leaves it defaulted. */
// VTABLE: WIZ8 0x005ef700 W8CharacterStatsRecordControl
class W8CharacterStatsRecordControl : public W8TextControl {
public:
    W8CharacterStatsRecordControl(Controls* owner, int top, int height,
                                  const W8CharacterStatsRecord* record, int variant);
    virtual void Redraw(unsigned char full_redraw) override;
    virtual void OnMouseEnter(int event) override;
    virtual void OnMouseLeave(int event) override;
    virtual void OnRightButtonUp(int event) override;

    const W8CharacterStatsRecord* m_record;
    unsigned char m_variant;
    unsigned char pad_0bd[3];
    int m_text_offset;
};
static_assert(sizeof(W8CharacterStatsRecordControl) == 0xc4, "W8CharacterStatsRecordControl_size");

// FUNCTION: WIZ8 0x005c8e70
W8CharacterStatsValue::W8CharacterStatsValue(Controls* owner, int x, int y,
                                             const W8CharacterStatsRecord* default_record)
    : W8TextControl(owner, 0xffffffff, x, y, 0, 0, 0x104, 0, 1, 1, 2, 2, 3),
      m_default_record(default_record)
{
    m_textBuffer.SetText(gppStringList[default_record->name_id], g_options_detail_font);
    m_textBuffer.SetLayoutMode(g_W8TextBufferAlignMiddle | g_W8TextBufferAlignLeft);
    m_pressedTextOffset = 0;
    UpdateTextBounds(x + 0x19, y, x + 0x7e, y + 0x16);
}

/* Store the record the value control displays. A null record selects the
   default the constructor was handed. */
// FUNCTION: WIZ8 0x005c8fc0
void W8CharacterStatsValue::SetRecord(const W8CharacterStatsRecord* record)
{
    if (record == 0) {
        record = m_default_record;
    }
    m_record = record;
    m_textBuffer.SetText(gppStringList[record->name_id], g_options_detail_font);
}

/* Redraw the current record's name over the value control's own text. */
// FUNCTION: WIZ8 0x005c9000
void W8CharacterStatsValue::Redraw(unsigned char full_redraw)
{
    W8TextControl::Redraw(full_redraw);
    if (!m_active || m_pPanel == 0) {
        return;
    }
    int left = m_pPanel->m_bounds.left + m_left + 2;
    int top = m_pPanel->m_bounds.top + m_top + 1;
    const W8CharacterStatsRecord* record = m_record;
    unsigned int image = m_enabled ? record->image_enabled : record->image_disabled;
    DrawCatalogImage(-14, record->object, 0, image, left, top, 2, 0);
}

/* Force the control enabled while the base handles the right-button release,
   so a disabled value can still raise its info callback. */
// FUNCTION: WIZ8 0x005c92f0
void W8CharacterStatsValue::OnRightButtonUp(int event)
{
    bool enabled = m_enabled;
    m_enabled = 1;
    W8TextControl::OnRightButtonUp(event);
    m_enabled = enabled;
}

// FUNCTION: WIZ8 0x005c9060
W8CharacterStatsRecordControl::W8CharacterStatsRecordControl(Controls* owner, int top, int height,
                                                             const W8CharacterStatsRecord* record,
                                                             int variant)
    : W8TextControl(owner, 0xffffffff, 0, top, 0x7e, top + height, 0x104, 0, 0, 0, 0, 0, 0)
{
    m_record = record;
    m_variant = static_cast<unsigned char>(variant);
    int text = 0;
    if (variant == 0) {
        text = 4;
        m_text_offset = 2;
    } else if (variant == 1) {
        text = 5;
        m_text_offset = 0;
    } else if (variant == 2) {
        text = 6;
        m_text_offset = 0;
    } else {
        text = variant;
    }
    AddLayoutFlags(g_W8TextControlLayoutImageAtOrigin);
    m_normalSprite = text;
    m_pressedSprite = text;
    m_alternatePressedSprite = text;
    m_alternateNormalSprite = text;
    m_disabledSprite = text;
    m_textBuffer.SetText(gppStringList[record->name_id], g_options_detail_font);
    m_textBuffer.SetLayoutMode(g_W8TextBufferAlignMiddle | g_W8TextBufferAlignLeft);
    m_pressedTextOffset = 0;
    UpdateTextBounds(0x19, m_text_offset + top, 0x7e, m_text_offset + top + 0x16);
}

/* Redraw the record's catalogue image over the entry's own background. */
// FUNCTION: WIZ8 0x005c9220
void W8CharacterStatsRecordControl::Redraw(unsigned char full_redraw)
{
    W8TextControl::Redraw(full_redraw);
    if (!m_active || m_pPanel == 0) {
        return;
    }
    int left = m_pPanel->m_bounds.left + m_left + 2;
    int top = m_pPanel->m_bounds.top + m_text_offset + m_top + 1;
    const W8CharacterStatsRecord* record = m_record;
    unsigned int image = m_enabled ? record->image_enabled : record->image_disabled;
    DrawCatalogImage(-14, record->object, 0, image, left, top, 2, 0);
}

// FUNCTION: WIZ8 0x005c9290
void W8CharacterStatsRecordControl::OnMouseEnter(int event)
{
    W8TextControl::OnMouseEnter(event);
    if (m_active && m_enabled) {
        m_textBuffer.SetFontStateIndex(0xd);
    }
}

// FUNCTION: WIZ8 0x005c92c0
void W8CharacterStatsRecordControl::OnMouseLeave(int event)
{
    W8TextControl::OnMouseLeave(event);
    if (m_active) {
        m_textBuffer.SetFontStateIndex(-1);
        Invalidate(0);
    }
}

/* Retail ICF shares this right-button release with the value control's
   retained body at 0x005c92f0; this source override stays unmarked. */
void W8CharacterStatsRecordControl::OnRightButtonUp(int event)
{
    bool enabled = m_enabled;
    m_enabled = 1;
    W8TextControl::OnRightButtonUp(event);
    m_enabled = enabled;
}

// FUNCTION: WIZ8 0x005c9310
void W8CharacterStatsRow::Initialize(Controls* owner, unsigned int* region_set, int x, int y,
                                     int count, const W8CharacterStatsRecord* table,
                                     const W8CharacterStatsRecord* default_record, int help_first,
                                     int help_second, int help_value)
{
    m_table = table;
    m_count = static_cast<unsigned short>(count);
    m_x = owner->m_bounds.left + x;
    m_y = owner->m_bounds.top + y;
    m_region_set = region_set;
    if (count == 0) {
        srAssertFail("usItemsInList", CGS_STATS_PAGE_CPP, 0x1d6, 0);
    }

    m_decrement =
        new W8TextControl(owner, 0xffffffff, x + 1, y + 4, 0, 0, 0x10a, 0, 0, 2, 1, 4, 3);
    m_decrement->m_listener = this;
    m_decrement->EnableRegionHelp(help_first);

    m_increment =
        new W8TextControl(owner, 0xffffffff, x + 0x9f, y + 4, 0, 0, 0x10a, 0, 5, 7, 6, 9, 8);
    m_increment->m_listener = this;
    m_increment->EnableRegionHelp(help_value);

    m_value_control = new W8CharacterStatsValue(owner, x + 0x1c, y + 4, default_record);
    m_value_control->AddLayoutFlags(g_W8TextControlLayoutToggle);
    m_value_control->m_listener = this;
    m_value_control->EnableRegionHelp(help_second);

    m_decrement->SetActive(1);
    m_increment->SetActive(1);
    m_value_control->SetActive(1);
}

/* Build the expanded record list once, then enable it and mirror each
   record's selectable flag onto its entry control. */
// FUNCTION: WIZ8 0x005c94e0
void W8CharacterStatsRow::BuildSubpanel()
{
    if (m_subpanel == 0) {
        m_subpanel =
            new Controls(m_x + 0x9e, m_y + 1, m_x + 0x11c,
                         m_y + 5 + static_cast<unsigned int>(m_count) * 0x16, -1, 0, -1);
        m_subpanel->AcquireRegionSet(m_region_set);
        m_subpanel_entries = new W8TextControl*[m_count];

        for (unsigned int index = 0; index < m_count; ++index) {
            int variant;
            int top;
            int height;
            if (index == 0) {
                variant = 0;
                top = 0;
                height = 0x18;
            } else if (index == m_count - 1) {
                variant = 2;
                top = index * 0x16 + 2;
                height = 0x18;
            } else {
                variant = 1;
                top = index * 0x16 + 2;
                height = 0x16;
            }
            m_subpanel_entries[index] = new W8CharacterStatsRecordControl(
                m_subpanel, top, height, &m_table[index], variant);
            m_subpanel_entries[index]->m_listener = this;
        }
    }
    m_subpanel->SetEnabled(1);
    m_subpanel->EnableRegionSet(1);
    for (unsigned int index = 0; index < m_count; ++index) {
        m_subpanel_entries[index]->SetEnabled(m_table[index].enabled);
    }
}

/* Select one record by index, updating the value control and telling the
   owning page when the selection actually moved. */
// FUNCTION: WIZ8 0x005c96c0
void W8CharacterStatsRow::SetValue(int index)
{
    if (index == -1) {
        m_value_control->SetRecord(0);
    } else {
        m_value_control->SetRecord(&m_table[index]);
    }
    int previous = m_index;
    m_index = index;
    m_value_control->Invalidate(1);
    if (m_listener != 0 && previous != index) {
        m_listener->OnRowValueChanged(this, index);
    }
}

// FUNCTION: WIZ8 0x005c9760
void W8CharacterStatsRow::OnPrimary(W8TextControl* control)
{
    if (control == m_decrement) {
        int previous = m_index;
        int index = previous - 1;
        if (index >= 0) {
            const W8CharacterStatsRecord* record = &m_table[index];
            do {
                if (record->enabled != 0) {
                    m_value_control->SetRecord(index == -1 ? 0 : &m_table[index]);
                    bool changed = previous != index;
                    m_index = index;
                    m_value_control->Invalidate(1);
                    if (m_listener == 0 || !changed) {
                        return;
                    }
                    m_listener->OnRowValueChanged(this, index);
                    return;
                }
                --index;
                --record;
            } while (index >= 0);
        }
        index = m_count - 1;
        if (previous < index) {
            const W8CharacterStatsRecord* record = &m_table[index];
            while (record->enabled == 0) {
                --index;
                --record;
                if (index <= previous) {
                    return;
                }
            }
            m_value_control->SetRecord(index == -1 ? 0 : &m_table[index]);
            bool changed = previous != index;
            m_index = index;
            m_value_control->Invalidate(1);
            if (m_listener != 0 && changed) {
                m_listener->OnRowValueChanged(this, index);
            }
        }
    } else if (control == m_increment) {
        int previous = m_index;
        int index = previous + 1;
        if (index < static_cast<int>(static_cast<unsigned int>(m_count))) {
            const W8CharacterStatsRecord* record = &m_table[index];
            do {
                if (record->enabled != 0) {
                    m_value_control->SetRecord(index == -1 ? 0 : &m_table[index]);
                    bool changed = previous != index;
                    m_index = index;
                    m_value_control->Invalidate(1);
                    if (m_listener == 0 || !changed) {
                        return;
                    }
                    m_listener->OnRowValueChanged(this, index);
                    return;
                }
                ++index;
                ++record;
            } while (index < static_cast<int>(static_cast<unsigned int>(m_count)));
        }
        index = 0;
        if (previous > 0) {
            const W8CharacterStatsRecord* record = &m_table[0];
            while (record->enabled == 0) {
                ++index;
                ++record;
                if (index >= previous) {
                    return;
                }
            }
            m_value_control->SetRecord(index == -1 ? 0 : &m_table[index]);
            bool changed = previous != index;
            m_index = index;
            m_value_control->Invalidate(1);
            if (m_listener != 0 && changed) {
                m_listener->OnRowValueChanged(this, index);
            }
        }
    } else if (control == m_value_control) {
        if ((m_value_control->m_stateFlags & g_W8TextControlStateSecondary) != 0) {
            BuildSubpanel();
            m_subpanel->Invalidate(0);
            if (m_listener != 0) {
                m_listener->OnRowExpanded(this);
            }
        } else {
            m_subpanel->SetEnabled(0);
            m_subpanel->EnableRegionSet(0);
            m_value_control->DisableSecondaryState(1);
            if (m_listener != 0) {
                m_listener->OnRowCollapsed(this);
            }
        }
    } else {
        for (int index = 0; index < m_count; ++index) {
            if (control == m_subpanel_entries[index]) {
                control->OnMouseLeave(0);
                SetValue(index);
                m_subpanel->SetEnabled(0);
                m_subpanel->EnableRegionSet(0);
                m_value_control->DisableSecondaryState(1);
                if (m_listener != 0) {
                    m_listener->OnRowCollapsed(this);
                }
            }
        }
    }
}

// FUNCTION: WIZ8 0x005c9a50
void W8CharacterStatsRow::OnSecondary(W8TextControl* control)
{
    if (control != m_decrement && control != m_increment) {
        if (control == m_value_control) {
            if (m_index != -1) {
                m_listener->OnRowInfoRequested(this, m_index);
            }
        } else {
            for (int index = 0; index < m_count; ++index) {
                if (control == m_subpanel_entries[index]) {
                    control->OnMouseLeave(0);
                    m_listener->OnRowInfoRequested(this, index);
                }
            }
        }
    }
}

/* The row's default state: no selection, no table, no child controls. */
W8CharacterStatsRow::W8CharacterStatsRow()
    : m_index(-1), m_count(0), m_table(0), m_decrement(0), m_increment(0),
      m_value_control(0), m_subpanel(0), m_subpanel_entries(0), m_listener(0)
{
}

/* Send every row control through its enabled state, redraw the values, and
   refresh the character-wide figures the rows depend on. */
// FUNCTION: WIZ8 0x005ca140
void W8CharacterStatsPage::Activate()
{
    EnableRegionSet(1);
    Refresh();
    for (int index = 0; index < 5; ++index) {
        m_attribute_controls[index]->SetActive(1);
        m_attribute_controls[index]->SetEnabled(1);
    }
    m_dirty = 1;
    m_prepared = 1;
}

/* The skills page's Deactivate is the same one-call body; the linker folded
   both onto this address, so only this definition carries the marker. */
// FUNCTION: WIZ8 0x005ca1f0
void W8CharacterStatsPage::Deactivate()
{
    EnableRegionSet(0);
}

/* Rebuild the three row displays from the character's current profession,
   race and sex. In creation mode the profession list is opened up
   entirely; in level-up mode only the eligible professions stay enabled. */
// FUNCTION: WIZ8 0x005ca200
void W8CharacterStatsPage::UpdateRowValues()
{
    unsigned char eligible[15];

    if (m_mode == 0) {
        if (m_character->iProfession == 2) {
            g_character_gender_records[0].enabled = 0;
        } else {
            g_character_gender_records[0].enabled = 1;
            g_character_gender_records[1].enabled = 1;
        }
    } else if (m_mode == 2) {
        DetermineEligibleProfessions(m_character, m_creation_state, eligible);
    }
    for (int index = 0; index < 15; ++index) {
        if (m_mode == 0) {
            g_character_profession_records[index].enabled = 1;
        } else if (m_mode == 2) {
            g_character_profession_records[index].enabled = eligible[index];
        }
    }

    W8Profession profession = m_character->iProfession;
    W8CharacterStatsRow* row = m_profession_row;
    if (profession == -1) {
        row->m_value_control->SetRecord(0);
    } else {
        row->m_value_control->SetRecord(&row->m_table[profession]);
    }
    int previous = row->m_index;
    row->m_index = profession;
    row->m_value_control->Invalidate(1);
    if (row->m_listener != 0 && previous != profession) {
        row->m_listener->OnRowValueChanged(row, profession);
    }
    row->m_decrement->Invalidate(0);
    row->m_increment->Invalidate(0);
    row->m_value_control->Invalidate(0);
    if (row->m_subpanel != 0) {
        row->m_subpanel->Invalidate(0);
    }

    int race = m_character->iRace;
    row = m_race_row;
    if (race == -1) {
        row->m_value_control->SetRecord(0);
    } else {
        row->m_value_control->SetRecord(&row->m_table[race]);
    }
    previous = row->m_index;
    row->m_index = race;
    row->m_value_control->Invalidate(1);
    if (row->m_listener != 0 && previous != race) {
        row->m_listener->OnRowValueChanged(row, race);
    }
    row->m_decrement->Invalidate(0);
    row->m_increment->Invalidate(0);
    row->m_value_control->Invalidate(0);
    if (row->m_subpanel != 0) {
        row->m_subpanel->Invalidate(0);
    }

    W8Gender gender = m_character->gender;
    row = m_gender_row;
    if (gender == -1) {
        row->m_value_control->SetRecord(0);
    } else {
        row->m_value_control->SetRecord(&row->m_table[gender]);
    }
    previous = row->m_index;
    row->m_index = gender;
    row->m_value_control->Invalidate(1);
    if (row->m_listener != 0 && previous != gender) {
        row->m_listener->OnRowValueChanged(row, gender);
    }
    row->m_decrement->Invalidate(0);
    row->m_increment->Invalidate(0);
    row->m_value_control->Invalidate(0);
    if (row->m_subpanel != 0) {
        row->m_subpanel->Invalidate(0);
    }
}

/* The entries only need enabling the first time the rows become usable. */
// FUNCTION: WIZ8 0x005ca480
void W8CharacterStatsPage::Refresh()
{
    UpdateRowValues();
    if (m_character->iRace != -1 || m_character->iProfession != -1) {
        for (int index = 0; index < m_entries.count; ++index) {
            if (!m_rows_initialized) {
                m_entries.data[index]->SetEnabled(1);
            }
            m_entries.data[index]->UpdateButtons();
        }
        m_rows_initialized = true;
    }
}

/* Accept the page: refund everything the editing state still owes, then let
   the screen recompute its navigation buttons. */
// FUNCTION: WIZ8 0x005ca4f0
void W8CharacterStatsPage::Accept()
{
    RefundAllocatedAttributes(m_character, m_creation_state);
    Invalidate(0);
    m_dirty = 1;
    m_screen->UpdateNavigation(this);
    for (int index = 0; index < m_entries.count; ++index) {
        m_entries.data[index]->UpdateButtons();
    }
}

/* The next button needs a complete attribute allocation; exit is available
   once any points are committed. */
// FUNCTION: WIZ8 0x005ca550
void W8CharacterStatsPage::GetNavigationState(bool* next_enabled, bool* exit_enabled)
{
    if (!m_creation_state->attributes_complete || m_character->iProfession == -1 ||
        m_character->iRace == -1 || m_character->gender == -1) {
        *next_enabled = false;
    } else {
        *next_enabled = true;
    }
    *exit_enabled = m_creation_state->attribute_points_remaining <
                    m_creation_state->attribute_points_total;
    if (*next_enabled != nav_next_state) {
        for (int index = 0; index < m_entries.count; ++index) {
            m_entries.data[index]->SetIncrementAllowed(!*next_enabled);
        }
        nav_next_state = *next_enabled;
    }
}

/* A click anywhere closes an expanded row's record list. */
// FUNCTION: WIZ8 0x005ca5e0
void W8CharacterStatsPage::HandleInput(InputAtom* input)
{
    if (input->usEvent != LEFT_BUTTON_DOWN && input->usEvent != RIGHT_BUTTON_DOWN) {
        return;
    }
    W8CharacterStatsRow* row = m_profession_row;
    if (row->m_subpanel != 0 && row->m_subpanel->m_fEnabled) {
        int index = 0;
        while (index < row->m_count) {
            if (row->m_subpanel_entries[index]->m_alternateTextEnabled) {
                goto next_profession;
            }
            ++index;
        }
        if (!row->m_value_control->m_alternateTextEnabled) {
            row->m_subpanel->SetEnabled(0);
            row->m_subpanel->EnableRegionSet(0);
            row->m_value_control->DisableSecondaryState(1);
            if (row->m_listener != 0) {
                row->m_listener->OnRowCollapsed(row);
            }
        }
    }
next_profession:
    row = m_race_row;
    if (row->m_subpanel != 0 && row->m_subpanel->m_fEnabled) {
        int index = 0;
        while (index < row->m_count) {
            if (row->m_subpanel_entries[index]->m_alternateTextEnabled) {
                goto next_race;
            }
            ++index;
        }
        if (!row->m_value_control->m_alternateTextEnabled) {
            row->m_subpanel->SetEnabled(0);
            row->m_subpanel->EnableRegionSet(0);
            row->m_value_control->DisableSecondaryState(1);
            if (row->m_listener != 0) {
                row->m_listener->OnRowCollapsed(row);
            }
        }
    }
next_race:
    row = m_gender_row;
    if (row->m_subpanel != 0 && row->m_subpanel->m_fEnabled) {
        int index = 0;
        while (index < row->m_count) {
            if (row->m_subpanel_entries[index]->m_alternateTextEnabled) {
                return;
            }
            ++index;
        }
        if (!row->m_value_control->m_alternateTextEnabled) {
            row->m_subpanel->SetEnabled(0);
            row->m_subpanel->EnableRegionSet(0);
            row->m_value_control->DisableSecondaryState(1);
            if (row->m_listener != 0) {
                row->m_listener->OnRowCollapsed(row);
            }
        }
    }
}

/* One attribute entry was adjusted: apply the point through the shared
   creation-state helper, then show every skill the maxed attribute gates. */
// FUNCTION: WIZ8 0x005ca730
void W8CharacterStatsPage::AdjustEntry(W8CharacterPageEntry* entry, int delta)
{
    entry->MarkDirty();
    m_dirty = 1;
    AdjustAllocatedAttribute(m_character, m_creation_state, entry->m_id, delta);
    Invalidate(0);
    m_screen->UpdateNavigation(this);
    if (m_character->attributes[entry->m_id].value >= 100) {
        for (int skill = 0x22; skill < 0x29; ++skill) {
            if (g_skill_attributes[skill].attribute_1 == static_cast<int>(entry->m_id) &&
                m_character->skills[skill].active != 0) {
                m_screen->ShowDescription(entry->m_id, skill);
            }
        }
    }
}

// FUNCTION: WIZ8 0x005ca7e0
void W8CharacterStatsPage::ShowEntryInfo(W8CharacterPageEntry* entry)
{
    m_screen->ShowPrimaryAttributeInfo(entry->m_id);
}

/* A value row moved: rerun the whole creation rebuild for the new
   profession, race or sex, then refresh the row controls. */
// FUNCTION: WIZ8 0x005ca800
void W8CharacterStatsPage::OnRowValueChanged(W8CharacterStatsRow* row, int value)
{
    if (row == m_profession_row) {
        RebuildLevelUpPoolsForProfession(m_character, m_creation_state,
                                         static_cast<W8Profession>(value));
    } else if (row == m_race_row) {
        SetCharacterRace(m_character, m_creation_state, value);
    } else {
        SetCharacterGender(m_character, m_creation_state, static_cast<W8Gender>(value));
    }
    m_dirty = 1;
    m_screen->UpdateNavigation(this);
    Refresh();
}

/* A row opened its record list: park the row and attribute controls so the
   list owns the input. */
// FUNCTION: WIZ8 0x005ca8d0
void W8CharacterStatsPage::OnRowExpanded(W8CharacterStatsRow* row)
{
    if (row == m_profession_row) {
        m_profession_row->m_increment->SetActive(0);
        m_race_row->m_increment->SetActive(0);
        m_gender_row->m_increment->SetActive(0);
    } else if (row == m_race_row) {
        m_race_row->m_increment->SetActive(0);
        m_gender_row->m_increment->SetActive(0);
    } else {
        m_gender_row->m_increment->SetActive(0);
    }
    for (int entry_index = 0; entry_index < m_entries.count; ++entry_index) {
        m_entries.data[entry_index]->SetHelpActive(0);
    }
    for (int control_index = 0; control_index < 5; ++control_index) {
        m_attribute_controls[control_index]->SetActive(0);
    }
}

/* The record list closed: restore the row and attribute controls. */
// FUNCTION: WIZ8 0x005ca970
void W8CharacterStatsPage::OnRowCollapsed(W8CharacterStatsRow* row)
{
    if (row == m_profession_row) {
        m_profession_row->m_increment->SetActive(1);
        m_race_row->m_increment->SetActive(1);
        m_gender_row->m_increment->SetActive(1);
    } else if (row == m_race_row) {
        m_race_row->m_increment->SetActive(1);
        m_gender_row->m_increment->SetActive(1);
    } else {
        m_gender_row->m_increment->SetActive(1);
    }
    for (int entry_index = 0; entry_index < m_entries.count; ++entry_index) {
        m_entries.data[entry_index]->SetHelpActive(1);
    }
    for (int control_index = 0; control_index < 5; ++control_index) {
        m_attribute_controls[control_index]->SetActive(1);
    }
    Invalidate(0);
}

/* Right-clicking a row's value asks the screen for that entry's info. */
// FUNCTION: WIZ8 0x005caa20
void W8CharacterStatsPage::OnRowInfoRequested(W8CharacterStatsRow* row, int value)
{
    if (row == m_profession_row) {
        m_screen->ShowProfessionInfo(value);
        return;
    }
    if (row == m_race_row) {
        m_screen->ShowRaceInfo(value);
    }
}

/* The primary listener slot shares the page's empty HandleInput emission.
   The retail fold is linked-image evidence, so this authored method has no
   separate FUNCTION address claim. */
void W8CharacterStatsPage::OnPrimary(W8TextControl*) {}

/* The five coloured attribute-corner controls name the attribute whose info
   dialog to raise. */
// FUNCTION: WIZ8 0x005caa50
void W8CharacterStatsPage::OnSecondary(W8TextControl* control)
{
    for (unsigned int index = 0; index < 5; ++index) {
        if (control == m_attribute_controls[index]) {
            m_screen->ShowSecondaryAttributeInfo(index);
        }
    }
}

// FUNCTION: WIZ8 0x005caa80
void W8CharacterStatsPage::Prepare()
{
    W8CharacterPage::Prepare();
    W8CharacterStatsRow* rows[3] = {
        m_profession_row,
        m_race_row,
        m_gender_row,
    };
    for (int index = 0; index < 3; ++index) {
        rows[index]->m_decrement->Invalidate(0);
        rows[index]->m_increment->Invalidate(0);
        rows[index]->m_value_control->Invalidate(0);
        if (rows[index]->m_subpanel != 0) {
            rows[index]->m_subpanel->Invalidate(0);
        }
    }
}

/* The whole page: three value rows, seven attribute entries, five attribute
   corner controls, then the profession/race/sex list state. */
// FUNCTION: WIZ8 0x005c9c80
void W8CharacterStatsPage::SetCharacter(W8Character* character,
                                        W8CharacterCreationState* creation_state, int mode)
{
    W8CharacterPage::SetCharacter(character, creation_state, mode);
    m_profession_row = new W8CharacterStatsRow;
    m_race_row = new W8CharacterStatsRow;
    m_gender_row = new W8CharacterStatsRow;
    AcquireRegionSet(&g_character_stats_region_set);
    m_profession_row->Initialize(this, &g_character_stats_profession_region_set, 0x16, 10, 0xf,
                                     g_character_profession_records,
                                     &g_character_profession_default_record, 0xf7, 0xf6, 0xf8);
    m_race_row->Initialize(this, &g_character_stats_race_region_set, 0x16, 0x3d, 0xb,
                               g_character_race_records, &g_character_race_default_record, 0xfa,
                               0xf9, 0xfb);
    m_gender_row->Initialize(this, &g_character_stats_gender_region_set, 0x16, 0x70, 2,
                                 g_character_gender_records, &g_character_gender_default_record,
                                 0xfd, 0xfc, 0xfe);
    m_profession_row->m_listener = this;
    m_race_row->m_listener = this;
    m_gender_row->m_listener = this;

    if (mode == 0) {
        W8CharacterStatsRow* rows[3] = {
            m_profession_row,
            m_race_row,
            m_gender_row,
        };
        for (int row_index = 0; row_index < 3; ++row_index) {
            rows[row_index]->m_decrement->SetEnabled(1);
            rows[row_index]->m_increment->SetEnabled(1);
            rows[row_index]->m_value_control->SetEnabled(1);
        }
    } else if (mode == 1) {
        W8CharacterStatsRow* rows[3] = {
            m_profession_row,
            m_race_row,
            m_gender_row,
        };
        for (int row_index = 0; row_index < 3; ++row_index) {
            rows[row_index]->m_decrement->SetEnabled(0);
            rows[row_index]->m_increment->SetEnabled(0);
            rows[row_index]->m_value_control->SetEnabled(0);
        }
    } else if (mode == 2) {
        if (character->iRace == 0xf) {
            m_profession_row->m_decrement->SetEnabled(0);
            m_profession_row->m_increment->SetEnabled(0);
            m_profession_row->m_value_control->SetEnabled(0);
        } else {
            m_profession_row->m_decrement->SetEnabled(1);
            m_profession_row->m_increment->SetEnabled(1);
            m_profession_row->m_value_control->SetEnabled(1);
        }
        m_race_row->m_decrement->SetEnabled(0);
        m_race_row->m_increment->SetEnabled(0);
        m_race_row->m_value_control->SetEnabled(0);
        m_gender_row->m_decrement->SetEnabled(0);
        m_gender_row->m_increment->SetEnabled(0);
        m_gender_row->m_value_control->SetEnabled(0);
    }

    for (int attribute_index = 0; attribute_index < 7; ++attribute_index) {
        W8CharacterPageEntry* entry =
            new W8CharacterPageEntry(this, 0xe5, 0x1b + attribute_index * 0xe, 0);
        AddEntry(entry);
        entry->m_listener = this;
        entry->SetContent(attribute_index,
                          gppStringList[g_character_description_first_ids[attribute_index]],
                          &character->attributes[attribute_index].value,
                          &creation_state->attribute_values[attribute_index],
                          &creation_state->attribute_limits[attribute_index], 0x101);
        entry->SetEnabled(0);
    }
    nav_next_state = false;
    m_rows_initialized = false;

    for (int control_index = 0; control_index < 5; ++control_index) {
        W8TextControl* control =
            new W8TextControl(this, 0xffffffff, 0xf3, 0xba + control_index * 0xe, 0x195,
                              0xba + control_index * 0xe + 0xc, -1, -1, -1, -1, -1, -1, -1);
        m_attribute_controls[control_index] = control;
        control->SetActive(0);
        control->m_listener = this;
        control->EnableRegionHelp(0x101);
    }
}

// FUNCTION: WIZ8 0x005c9ae0
W8CharacterStatsPage::~W8CharacterStatsPage()
{
    W8CharacterStatsRow* rows[3] = {
        m_profession_row,
        m_race_row,
        m_gender_row,
    };
    for (int index = 0; index < 3; ++index) {
        W8CharacterStatsRow* row = rows[index];
        if (row != 0) {
            delete row->m_subpanel;
            if (row->m_subpanel_entries != 0) {
                for (unsigned int entry = 0; entry < row->m_count; ++entry) {
                    delete row->m_subpanel_entries[entry];
                }
                delete[] row->m_subpanel_entries;
            }
            delete row;
        }
    }
}

/* The page redraw: header figures, the seven attribute entries' section,
   the three value rows' labels, then the resistance and trait blocks. */
// FUNCTION: WIZ8 0x005cab20
void W8CharacterStatsPage::Redraw()
{
    bool redraw = static_cast<unsigned char>(m_fEnabled && m_fDirty);
    W8TextBuffer text;
    int left = m_bounds.left;
    int top = m_bounds.top;
    W8CharacterPage::Redraw();
    if (redraw) {
        W8ControlsRect bounds;
        text.SetLayoutMode(g_W8TextBufferAlignMiddle | g_W8TextBufferAlignCenter);
        bounds.left = left + 0xe5;
        bounds.right = left + 0x1a8;
        bounds.top = top + 0xe;
        bounds.bottom = top + 0x1a;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0x96], g_wiz_text_font_secondary);
        text.RenderToTarget(0, 0, -14);

        if (m_character->iRace != -1) {
            bounds.top = top + 0x7e;
            bounds.bottom = top + 0x8a;
            bounds.left = left + 0xf9;
            bounds.right = left + 0x10f;
            text.SetLayoutBounds(&bounds, 1, 1);
            text.SetText(
                FormatWideString(g_format_d, m_creation_state->attribute_points_remaining),
                g_wiz_text_font_secondary);
            text.RenderToTarget(0, 0, -14);
        }

        text.SetLayoutMode(g_W8TextBufferAlignLeft | g_W8TextBufferAlignMiddle);
        bounds.right = left + 0x1a8;
        bounds.top = top + 0x7e;
        bounds.bottom = top + 0x8a;
        bounds.left = left + 0x116;
        int saved_right = bounds.right;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0x87], g_wiz_text_font_secondary);
        text.RenderToTarget(0, 0, -14);

        text.SetLayoutMode(g_W8TextBufferAlignMiddle | g_W8TextBufferAlignCenter);
        bounds.left = left + 0x1a;
        bounds.right = left + 0xe4;
        bounds.top = top + 0xac;
        bounds.bottom = top + 0xb8;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0xb1], g_wiz_text_font_secondary);
        text.RenderToTarget(0, 0, -14);

        text.SetLayoutMode(g_W8TextBufferAlignTop | g_W8TextBufferAlignLeft);
        bounds.left = left + 0x1d;
        bounds.bottom = top + 0x133;
        bounds.top = top + 0xbb;
        bounds.right = left + 0xe6;
        if (m_character->iProfession != -1 || m_character->iRace != -1) {
            unsigned char available[0x20];
            unsigned int available_count = 0;
            for (int trait = 0; trait < 0x20; ++trait) {
                if (CharacterHasTrait(m_character, trait)) {
                    available[trait] = 1;
                    ++available_count;
                } else {
                    available[trait] = 0;
                }
            }
            int line_height = (available_count < 8) + 0xd;
            if (m_character->iProfession != -1) {
                text.SetLayoutBounds(&bounds, 1, 1);
                text.SetText(
                    FormatWideString(
                        g_format_s_space_s,
                        gppStringList
                            [g_character_skill_name_ids
                                 [g_profession_bonus_skills[m_character->iProfession]]],
                        gppStringList[0xb2]),
                    g_wiz_text_font_secondary);
                text.RenderToTarget(0, 0, -14);
                bounds.top += line_height;
            }
            for (int trait_index = 0; trait_index < 0x20; ++trait_index) {
                if (available[trait_index] != 0) {
                    text.SetLayoutBounds(&bounds, 1, 1);
                    text.SetText(gppStringList[g_character_trait_name_ids[trait_index]],
                                 g_wiz_text_font_secondary);
                    text.RenderToTarget(0, 0, -14);
                    bounds.top += line_height;
                }
            }
        }

        text.SetLayoutMode(g_W8TextBufferAlignMiddle | g_W8TextBufferAlignCenter);
        bounds.left = left + 0xf3;
        bounds.right = left + 0x1a7;
        bounds.top = top + 0xac;
        bounds.bottom = top + 0xb8;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0xa6], g_wiz_text_font_secondary);
        text.RenderToTarget(0, 0, -14);

        bounds.left = left + 0xf8;
        bounds.top = top + 0xba;
        bounds.bottom = top + 0xc6;
        bounds.right = left + 0x195;
        text.SetLayoutMode(g_W8TextBufferAlignLeft | g_W8TextBufferAlignMiddle);
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0xa7], g_wiz_text_font_secondary);
        text.RenderToTarget(0, 0, -14);
        if (m_character->iRace != -1) {
            text.SetLayoutMode(g_W8TextBufferAlignRight | g_W8TextBufferAlignMiddle);
            text.SetText(FormatWideString(g_format_d, m_character->uiHPMax),
                         g_wiz_text_font_secondary);
            text.RenderToTarget(0, 0, -14);
        }

        bounds.top += 0xe;
        bounds.bottom += 0xe;
        text.SetLayoutMode(g_W8TextBufferAlignLeft | g_W8TextBufferAlignMiddle);
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0xa9], g_wiz_text_font_secondary);
        text.RenderToTarget(0, 0, -14);
        if (m_character->iRace != -1) {
            text.SetLayoutMode(g_W8TextBufferAlignRight | g_W8TextBufferAlignMiddle);
            text.SetText(FormatWideString(g_format_d, m_character->uiStaminaMax),
                         g_wiz_text_font_secondary);
            text.RenderToTarget(0, 0, -14);
        }

        bounds.bottom += 0xe;
        bounds.top += 0xe;
        text.SetLayoutMode(g_W8TextBufferAlignLeft | g_W8TextBufferAlignMiddle);
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0xed], g_wiz_text_font_secondary);
        text.RenderToTarget(0, 0, -14);
        if (m_character->iRace != -1) {
            text.SetLayoutMode(g_W8TextBufferAlignRight | g_W8TextBufferAlignMiddle);
            text.SetText(FormatWideString(g_format_d, ComputeLevelUpSpellPointAward(
                                                          m_character, m_creation_state)),
                         g_wiz_text_font_secondary);
            text.RenderToTarget(0, 0, -14);
        }

        bounds.top += 0xe;
        bounds.bottom += 0xe;
        text.SetLayoutMode(g_W8TextBufferAlignLeft | g_W8TextBufferAlignMiddle);
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0xad], g_wiz_text_font_secondary);
        text.RenderToTarget(0, 0, -14);
        if (m_character->iRace != -1) {
            text.SetLayoutMode(g_W8TextBufferAlignRight | g_W8TextBufferAlignMiddle);
            text.SetText(FormatWideString(g_format_d, m_character->armor_class_average),
                         g_wiz_text_font_secondary);
            text.RenderToTarget(0, 0, -14);
        }

        bounds.top += 0xe;
        bounds.bottom += 0xe;
        text.SetLayoutMode(g_W8TextBufferAlignLeft | g_W8TextBufferAlignMiddle);
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0xaf], g_wiz_text_font_secondary);
        text.RenderToTarget(0, 0, -14);
        if (m_character->iRace != -1) {
            text.SetLayoutMode(g_W8TextBufferAlignRight | g_W8TextBufferAlignMiddle);
            text.SetText(FormatWideString(g_format_d, m_character->carrying_capacity / 10),
                         g_wiz_text_font_secondary);
            text.RenderToTarget(0, 0, -14);
        }

        text.SetLayoutMode(g_W8TextBufferAlignMiddle | g_W8TextBufferAlignCenter);
        bounds.left = left + 0x1a;
        bounds.right = left + 0xe4;
        bounds.top = top + 0x140;
        bounds.bottom = top + 0x14c;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0xb9], g_wiz_text_font_secondary);
        text.RenderToTarget(0, 0, -14);
        if (m_character->iRace != -1 || m_character->iProfession != -1) {
            for (int realm = 0; realm < 6; ++realm) {
                bool first_column = (realm & 1) == 0;
                bounds.left = (realm / 2) * 0x3f + 0x39 + left;
                bounds.top = (first_column ? 0x157 : 0x171) + top;
                bounds.bottom = bounds.top + 0xe;
                bounds.right = bounds.left + 0x21;
                text.SetLayoutBounds(&bounds, 1, 1);
                int value = m_character->resistances[realm].total - 0x19;
                if (value == 0) {
                    text.SetText(FormatWideString(g_dash), g_wiz_text_font_secondary);
                } else {
                    text.SetText(FormatWideString(g_format_plus_d, value),
                                 g_wiz_text_font_secondary);
                }
                text.RenderToTarget(0, 0, -14);
            }
        }
        for (int realm_index = 0; realm_index < 6; ++realm_index) {
            bool first_column = (realm_index & 1) == 0;
            int frame_top = (first_column ? 0x155 : 0x16f) + top;
            DrawCatalogImageAndInvalidate(-14, g_character_resistance_images[realm_index], 0,
                                          g_spell_realm_animations[realm_index].initial_frame,
                                          (realm_index / 2) * 0x3f + 0x23 + left, frame_top, 2, 0);
        }

        text.SetLayoutMode(g_W8TextBufferAlignMiddle | g_W8TextBufferAlignCenter);
        bounds.left = left + 0xf3;
        bounds.right = left + 0x1a7;
        bounds.top = top + 0x120;
        bounds.bottom = top + 0x12c;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0xb3], g_wiz_text_font_secondary);
        text.RenderToTarget(0, 0, -14);
        if (m_character->iProfession != -1) {
            text.SetLayoutMode(g_W8TextBufferAlignTop | g_W8TextBufferAlignLeft);
            bounds.top = top + 0x12f;
            bounds.bottom = top + 0x13b;
            bounds.left = left + 0xf6;
            bounds.right = left + 0x1a9;
            text.SetLayoutBounds(&bounds, 1, 1);
            text.SetText(
                gppStringList[g_character_skill_name_ids
                                  [g_profession_bonus_skills[m_character->iProfession]]],
                g_wiz_text_font_secondary);
            text.RenderToTarget(0, 0, -14);
            for (int index = 0; index < 4; ++index) {
                if (g_profession_skills[m_character->iProfession][index] != -1) {
                    bounds.top += 0xe;
                    bounds.bottom += 0xe;
                    text.SetLayoutBounds(&bounds, 1, 1);
                    text.SetText(
                        gppStringList
                            [g_character_skill_name_ids
                                 [g_profession_skills[m_character->iProfession][index]]],
                        g_wiz_text_font_secondary);
                    text.RenderToTarget(0, 0, -14);
                }
            }
            text.SetLayoutMode(g_W8TextBufferAlignMiddle | g_W8TextBufferAlignCenter);
            bounds.left = left + 0x106;
            bounds.right = left + 0x11c;
            bounds.top = top + 0x179;
            bounds.bottom = top + 0x185;
            text.SetLayoutBounds(&bounds, 1, 1);
            text.SetText(FormatWideString(g_format_d, m_creation_state->skill_points_total),
                         g_wiz_text_font_secondary);
            text.RenderToTarget(0, 0, -14);
        }
        text.SetLayoutMode(g_W8TextBufferAlignLeft | g_W8TextBufferAlignMiddle);
        bounds.top = top + 0x179;
        bounds.left = left + 0x123;
        bounds.bottom = top + 0x185;
        bounds.right = saved_right;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0x87], g_wiz_text_font_secondary);
        text.RenderToTarget(0, 0, -14);
    }

    text.SetLayoutMode(g_W8TextBufferAlignMiddle | g_W8TextBufferAlignCenter);
    if (m_prepared) {
        W8ControlsRect bounds = {4, 0xec, 0xc2, 0x162};
        text.SetLayoutBounds(&bounds, 1, 1);
        if (m_mode == 0) {
            text.SetText(gppStringList[0xe5], g_wiz_text_font_secondary);
        } else if (m_character->iRace == 0xf) {
            text.SetText(gppStringList[0xe7], g_wiz_text_font_secondary);
        } else {
            text.SetText(gppStringList[0xe6], g_wiz_text_font_secondary);
        }
        text.RenderToTarget(0, 1, -14);
        bounds.top = 0x162;
        bounds.bottom = 0x179;
        bounds.right = 0x8f;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0xe2], g_wiz_text_font_secondary);
        text.RenderToTarget(0, 1, -14);
        bounds.top = 0x184;
        bounds.bottom = 0x19b;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0xe4], g_wiz_text_font_secondary);
        text.RenderToTarget(0, 1, -14);
        m_prepared = 0;
    }

    if (m_dirty) {
        W8ControlsRect bounds = {0x8f, 0x162, 0xbf, 0x179};
        DrawCatalogImage(-14, 0x107, 0, 5, 0x8f, 0x162, 2, 0);
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(FormatWideString(g_format_d, m_creation_state->attribute_step_limit),
                     g_options_detail_font);
        text.RenderToTarget(0, 1, -14);
        bounds.top = 0x184;
        bounds.bottom = 0x19b;
        DrawCatalogImage(-14, 0x107, 0, 5, 0x8f, 0x184, 2, 0);
        text.SetLayoutBounds(&bounds, 1, 1);
        int total = m_creation_state->attribute_points_total;
        if (total < 1) {
            text.SetText(const_cast<wchar_t*>(g_zero_slash_zero), g_options_detail_font);
        } else {
            text.SetText(FormatWideString(g_format_d_slash_d,
                                          m_creation_state->attribute_points_remaining, total),
                         g_options_detail_font);
        }
        text.RenderToTarget(0, 1, -14);
        m_dirty = 0;
    }

    W8CharacterStatsRow* rows[3] = {
        m_profession_row,
        m_race_row,
        m_gender_row,
    };
    for (int row_index = 0; row_index < 3; ++row_index) {
        W8CharacterStatsRow* row = rows[row_index];
        if (row->m_subpanel != 0 && row->m_subpanel->m_fEnabled) {
            row->m_subpanel->Redraw();
        }
    }
}

/* Factory for the stats page; the render target comes from the base
   constructor. */
// FUNCTION: WIZ8 0x005cba90
W8CharacterStatsPage* CreateCharacterStatsPage()
{
    return new W8CharacterStatsPage;
}
