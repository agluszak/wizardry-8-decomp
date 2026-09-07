#include "wiz8/local_screens/CharacterScreen.h"

#include "wiz8/local_code/Strings.h"
#include "wiz8/text_input.h"
#include "wiz8/utility.h"
#include "wiz8/video_object_catalog.h"
#include "vsurface.h"

#include <new>
#include <string.h>

extern int g_font_683660;
extern wchar_t* FormatWideString(const wchar_t*, ...);
extern unsigned int g_character_page2_region_set_0069c530;
extern unsigned int g_character_page4_region_set_0069c52c;
extern int g_character_page2_category_geometry_64ef90[5][2];
extern int g_character_page2_category_frames_64efb8[5];
extern unsigned short g_character_skill_name_ids_61e454[0x29];
extern int g_options_detail_font_683614;
extern void Function557F90(W8Character*, W8CharacterCreationState*);
extern void Function557BC0(W8Character*, W8CharacterCreationState*, unsigned int, int);
extern void Function558610(W8Character*);
extern void Function422F10(void);

struct W8PortraitDescriptor {
    int group;
    unsigned char unknown_004[0xc];
};
struct W8PortraitGroup {
    int count;
    int portraits[14];
};
static_assert(sizeof(W8PortraitDescriptor) == 0x10, "W8PortraitDescriptor_size");
static_assert(sizeof(W8PortraitGroup) == 0x3c, "W8PortraitGroup_size");
extern W8PortraitDescriptor g_portrait_descriptors_6483d0[];
extern W8PortraitGroup g_portrait_groups_648950[12];

// VTABLE: WIZ8 0x005ef1d8 W8CharacterPageEntry
// class W8CharacterPageEntry

// SYNTHETIC: WIZ8 0x005af990
// W8CharacterPageEntry::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005af690
W8CharacterPageEntry::W8CharacterPageEntry(
    Controls* owner, int x, int y, unsigned char compact)
    : m_listener_004(0), m_first_020(0), m_second_024(0), m_third_028(0),
      m_id_02c(-1), m_draw_background_038(compact), m_dirty_039(1),
      m_enabled_03a(0), m_flag_03b(1)
{
    m_x_030 = owner->origin_x + x;
    m_y_034 = owner->origin_y + y;
    int split = m_x_030 + (compact ? 0x6d : 0x78);
    W8ControlsRect bounds = {m_x_030 + 5, m_y_034 + 1,
                             split, m_y_034 + 0xd};
    m_label_014 = new W8TextBuffer005ED5B8(&bounds, 0, 0, 0, 4);
    m_label_014->SetLayoutMode(g_W8TextBufferLayoutMask005ED548);
    bounds.left = split;
    bounds.right = split + 0x17;
    m_first_text_018 = new W8TextBuffer005ED5B8(&bounds, 0, 0, 0, 4);
    m_first_text_018->SetLayoutMode(g_W8TextBufferLayoutMask005ED550);
    bounds.left = split + 0x2a;
    bounds.right = split + 0x39;
    m_second_text_01c = new W8TextBuffer005ED5B8(&bounds, 0, 0, 0, 4);
    m_second_text_01c->SetLayoutMode(g_W8TextBufferLayoutMask005ED550);

    int relative_split = split - owner->origin_x;
    m_decrement_00c = new W8TextControl005ED604(
        owner, 0xffffffff, relative_split + 0x1b, y + 1, 0, 0,
        0x10a, 0, 0x19, 0x1b, 0x1a, 0x1d, 0x1c);
    m_decrement_00c->AddLayoutFlags(0x100);
    m_decrement_00c->SetVisible(0);
    m_decrement_00c->SetEnabled(0);
    m_decrement_00c->m_listener = this;

    m_increment_008 = new W8TextControl005ED604(
        owner, 0xffffffff, relative_split + 0x3d, y + 1, 0, 0,
        0x10a, 0, 0x1e, 0x20, 0x1f, 0x22, 0x21);
    m_increment_008->AddLayoutFlags(0x100);
    m_increment_008->SetEnabled(0);
    m_increment_008->m_listener = this;

    m_help_010 = new W8TextControl005ED604(
        owner, 0xffffffff, x, y, relative_split, y + 0xc,
        -1, -1, -1, -1, -1, -1, -1);
    m_help_010->SetEnabled(0);
    m_help_010->m_listener = this;
}

// FUNCTION: WIZ8 0x005af9e0
void W8CharacterPageEntry::SetContent(
    unsigned int id, const wchar_t* label, unsigned int* first, int* second, int* third,
    int help_id)
{
    m_id_02c = id;
    m_first_020 = first;
    m_second_024 = second;
    m_third_028 = third;
    m_label_014->SetText(label, g_font_683660);
    if (help_id == -1) m_help_010->DisableRegionHelp();
    else m_help_010->EnableRegionHelp(help_id);
    m_enabled_03a = 1;
    m_increment_008->SetEnabled(1);
    m_decrement_00c->SetEnabled(1);
    m_help_010->SetEnabled(1);
    m_decrement_00c->Invalidate(0);
    m_increment_008->Invalidate(0);
    m_dirty_039 = 1;
    UpdateButtons();
    m_decrement_00c->Invalidate(0);
    m_increment_008->Invalidate(0);
    m_dirty_039 = 1;
}

// FUNCTION: WIZ8 0x005afc20
void W8CharacterPageEntry::SetIncrementAllowed(unsigned char allowed)
{
    m_flag_03b = allowed;
    UpdateButtons();
    m_decrement_00c->Invalidate(0);
    m_increment_008->Invalidate(0);
    m_dirty_039 = 1;
}

// FUNCTION: WIZ8 0x005afa90
void W8CharacterPageEntry::SetEnabled(unsigned char enabled)
{
    m_enabled_03a = enabled;
    m_increment_008->SetEnabled(enabled);
    m_decrement_00c->SetEnabled(enabled);
    m_help_010->SetEnabled(enabled);
    m_decrement_00c->Invalidate(0);
    m_increment_008->Invalidate(0);
    m_dirty_039 = 1;
}

// FUNCTION: WIZ8 0x005afaf0
void W8CharacterPageEntry::Redraw()
{
    if (m_enabled_03a && m_dirty_039) {
        if (m_draw_background_038) {
            DrawCatalogImageAndInvalidate(-14, 0x108, 0, 2, m_x_030, m_y_034, 2, 0);
        }
        m_first_text_018->SetText(
            FormatWideString(L"%d", *m_first_020), g_font_683660);
        m_second_text_01c->SetText(
            FormatWideString(L"%d", *m_second_024), g_font_683660);
        m_label_014->FillBounds(0x8000);
        m_first_text_018->FillBounds(0x8000);
        m_second_text_01c->FillBounds(0x8000);
        m_label_014->RenderToTarget(0, 1, -14);
        m_first_text_018->RenderToTarget(0, 1, -14);
        m_second_text_01c->RenderToTarget(0, 1, -14);
        m_dirty_039 = 0;
    }
    else if (!m_draw_background_038 && m_dirty_039) {
        m_label_014->RenderToTarget(0, 1, -14);
        m_dirty_039 = 0;
    }
}

// FUNCTION: WIZ8 0x005afbf0
void W8CharacterPageEntry::SetLabelFontState(int state)
{
    m_label_014->m_fontStateIndex = state;
}

// FUNCTION: WIZ8 0x005afc00
void W8CharacterPageEntry::MarkDirty()
{
    m_decrement_00c->Invalidate(0);
    m_increment_008->Invalidate(0);
    m_dirty_039 = 1;
}

// FUNCTION: WIZ8 0x005afd10
void W8CharacterPageEntry::UpdateButtons()
{
    if (m_enabled_03a) {
        unsigned char enabled = static_cast<unsigned char>(*m_second_024 > 0);
        if (m_decrement_00c->m_flag_4 != enabled) {
            m_decrement_00c->SetVisible(enabled);
            m_decrement_00c->Invalidate(0);
        }
        enabled = static_cast<unsigned char>(
            m_flag_03b && *m_second_024 < *m_third_028);
        if (m_increment_008->m_flag_4 != enabled) {
            m_increment_008->SetVisible(enabled);
            m_increment_008->Invalidate(0);
        }
        m_help_010->SetVisible(1);
    }
}

// FUNCTION: WIZ8 0x005afc50
void W8CharacterPageEntry::OnPrimary(W8TextControl005ED604* control)
{
    if (control == m_increment_008) {
        if (m_listener_004 != 0) m_listener_004->AdjustEntry(this, 1);
    }
    else if (control == m_decrement_00c) {
        if (m_listener_004 != 0) m_listener_004->AdjustEntry(this, -1);
    }
    UpdateButtons();
    m_decrement_00c->Invalidate(0);
    m_increment_008->Invalidate(0);
    m_dirty_039 = 1;
}

// FUNCTION: WIZ8 0x005afcb0
void W8CharacterPageEntry::OnSecondary(W8TextControl005ED604* control)
{
    if (control == m_increment_008) {
        if (m_listener_004 != 0) m_listener_004->AdjustEntry(this, 5);
        UpdateButtons();
        m_decrement_00c->Invalidate(0);
        m_increment_008->Invalidate(0);
        m_dirty_039 = 1;
    }
    else if (control == m_decrement_00c) {
        if (m_listener_004 != 0) m_listener_004->AdjustEntry(this, -5);
        UpdateButtons();
        m_decrement_00c->Invalidate(0);
        m_increment_008->Invalidate(0);
        m_dirty_039 = 1;
    }
    else if (m_listener_004 != 0) {
        m_listener_004->ShowEntryInfo(this);
    }
}

// VTABLE: WIZ8 0x005ef1e4 W8CharacterPage
// class W8CharacterPage

// VTABLE: WIZ8 0x005ef214 W8CharacterPageEntries005EF214
// class W8CharacterPageEntries005EF214

// VTABLE: WIZ8 0x005ef218 W8GrowableVector<W8CharacterPageEntry*>
// class W8GrowableVector<W8CharacterPageEntry*>

// TEMPLATE: WIZ8 0x005b1b70
// W8GrowableVector<W8CharacterPageEntry*>::~W8GrowableVector<W8CharacterPageEntry*>

// SYNTHETIC: WIZ8 0x005b1b90
// W8GrowableVector<W8CharacterPageEntry*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x005b1bc0
// W8CharacterPageEntries005EF214::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005afd90
W8CharacterPage::W8CharacterPage(int render_target)
    : Controls(0xc3, 0x2b, 0x280, 0x1c1, render_target, 0, 0),
      m_screen_05c(0), m_character_060(0), m_creation_state_064(0),
      m_mode_068(0), m_prepared_06c(0), m_dirty_06d(0)
{
}

// SYNTHETIC: WIZ8 0x005afe20
// W8CharacterPage::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005afe40
W8CharacterPage::~W8CharacterPage()
{
    DestroyAllControls();
    while (m_entries_04c.count > 0) {
        delete m_entries_04c.RemoveAt(m_entries_04c.count - 1);
    }
}

// FUNCTION: WIZ8 0x005aff00
void W8CharacterPage::SetCharacter(
    W8Character* character, W8CharacterCreationState* creation_state, int mode)
{
    m_character_060 = character;
    m_creation_state_064 = creation_state;
    m_mode_068 = mode;
}

// FUNCTION: WIZ8 0x005aff20
void W8CharacterPage::Redraw()
{
    Controls::Redraw();
    for (int index = 0; index < m_entries_04c.count; ++index) {
        m_entries_04c.data[index]->Redraw();
    }
}

// FUNCTION: WIZ8 0x005aff50
void W8CharacterPage::Invalidate(const W8ControlsRect* rect)
{
    Controls::Invalidate(rect);
    for (int index = 0; index < m_entries_04c.count; ++index) {
        m_entries_04c.data[index]->m_decrement_00c->Invalidate(0);
        m_entries_04c.data[index]->m_increment_008->Invalidate(0);
        m_entries_04c.data[index]->m_dirty_039 = 1;
    }
}

// FUNCTION: WIZ8 0x005affa0
void W8CharacterPage::Prepare()
{
    Invalidate(0);
    m_dirty_06d = 1;
    m_prepared_06c = 1;
}

// FUNCTION: WIZ8 0x005affc0
void W8CharacterPage::AddEntry(W8CharacterPageEntry* entry)
{
    m_entries_04c.Add(entry);
}

void W8CharacterPage::HandleInput(InputAtom*) {}
void W8CharacterPage::Refresh() {}

// FUNCTION: WIZ8 0x005ca1f0
void W8CharacterPage::Deactivate()
{
    EnableRegionSet(0);
}

// VTABLE: WIZ8 0x005ef5c8 W8CharacterPage005EF5C8
// VTABLE: WIZ8 0x005ef5c0 W8CharacterPageEntryListener
// class W8CharacterPage005EF5C8

// FUNCTION: WIZ8 0x005c7580
void W8CharacterPage005EF5C8::SetCharacter(
    W8Character* character, W8CharacterCreationState* creation_state, int mode)
{
    AcquireRegionSet(&g_character_page2_region_set_0069c530);
    W8CharacterPage::SetCharacter(character, creation_state, mode);
    int category_count[5] = {0, 0, 0, 0, 0};
    for (int skill = 0; skill < 0x29; ++skill) {
        int category = g_skill_attributes[skill].category;
        W8CharacterPageEntry* entry = new W8CharacterPageEntry(
            this, g_character_page2_category_geometry_64ef90[category][0],
            g_character_page2_category_geometry_64ef90[category][1] +
                category_count[category] * 0xe,
            1);
        AddEntry(entry);
        entry->m_listener_004 = this;
        ++category_count[category];
    }
    m_navigation_state_076 = 0;
}

// FUNCTION: WIZ8 0x005c76a0
void W8CharacterPage005EF5C8::Activate()
{
    EnableRegionSet(1);
    Refresh();
    m_dirty_06d = 1;
    m_prepared_06c = 1;
}

// FUNCTION: WIZ8 0x005c76c0
void W8CharacterPage005EF5C8::Accept()
{
    Function557F90(m_character_060, m_creation_state_064);
    Invalidate(0);
    m_dirty_06d = 1;
    m_screen_05c->UpdateNavigation(this);
    for (int index = 0; index < m_entries_04c.count; ++index) {
        m_entries_04c.data[index]->UpdateButtons();
    }
}

// FUNCTION: WIZ8 0x005c7720
void W8CharacterPage005EF5C8::GetNavigationState(
    unsigned char* next_enabled, unsigned char* exit_enabled)
{
    *next_enabled = m_creation_state_064->skills_complete;
    *exit_enabled = m_creation_state_064->skill_points_remaining <
                    m_creation_state_064->skill_points_total;
    if (*next_enabled != m_navigation_state_076) {
        for (int index = 0; index < m_entries_04c.count; ++index) {
            m_entries_04c.data[index]->SetIncrementAllowed(
                static_cast<unsigned char>(*next_enabled == 0));
        }
        m_navigation_state_076 = *next_enabled;
    }
}

// FUNCTION: WIZ8 0x005c7790
void W8CharacterPage005EF5C8::AdjustEntry(
    W8CharacterPageEntry* entry, int delta)
{
    entry->MarkDirty();
    m_dirty_06d = 1;
    Function557BC0(m_character_060, m_creation_state_064, entry->m_id_02c, delta);
    m_screen_05c->UpdateNavigation(this);
}

// FUNCTION: WIZ8 0x005c77d0
void W8CharacterPage005EF5C8::ShowEntryInfo(W8CharacterPageEntry* entry)
{
    m_screen_05c->ShowDialog005B08E0(entry->m_id_02c);
}

// FUNCTION: WIZ8 0x005c7b50
void W8CharacterPage005EF5C8::UpdateEntries()
{
    int index;
    for (index = 0; index < 0x29; ++index) {
        m_entries_04c.data[index]->SetEnabled(0);
    }

    int category_count[5] = {0, 0, 0, 0, 0};
    m_show_fifth_category_075 = 0;
    for (int skill = 0; skill < 0x29; ++skill) {
        W8CharacterSkill* value = &m_character_060->skills[skill];
        if (value->flag_00 || value->value_02 != 0) {
            int category = g_skill_attributes[skill].category;
            int entry_index = 0;
            int occurrence = 0;
            for (; entry_index < 0x29; ++entry_index) {
                if (g_skill_attributes[entry_index].category == category &&
                    occurrence++ == category_count[category]) {
                    break;
                }
            }
            W8CharacterPageEntry* entry = m_entries_04c.data[entry_index];
            ++category_count[category];
            if (category == 4) m_show_fifth_category_075 = 1;
            entry->SetContent(
                skill, gppStringList[g_character_skill_name_ids_61e454[skill]],
                &value->value_02,
                &m_creation_state_064->skill_points_spent[skill],
                &m_creation_state_064->skill_limits[skill], 0x101);
            entry->SetLabelFontState(
                skill == g_profession_bonus_skills[m_character_060->current_profession]
                    ? 3 : -1);
        }
    }
}

// FUNCTION: WIZ8 0x005c77f0
void W8CharacterPage005EF5C8::Redraw()
{
    unsigned char redraw = static_cast<unsigned char>(m_fEnabled && m_fDirty);
    if (m_force_redraw_074) {
        UpdateEntries();
        Invalidate(0);
        redraw = 1;
        m_force_redraw_074 = 0;
    }
    W8CharacterPage::Redraw();

    if (redraw) {
        for (int category = 0; category < 5; ++category) {
            if (category != 4 || m_show_fifth_category_075) {
                DrawCatalogImage(
                    -14, 0x144, 0,
                    static_cast<short>(g_character_page2_category_frames_64efb8[category]),
                    origin_x + g_character_page2_category_geometry_64ef90[category][0] - 0x16,
                    origin_y + g_character_page2_category_geometry_64ef90[category][1] - 3,
                    2, 0);
            }
        }
        if (!m_show_fifth_category_075) {
            DrawCatalogImage(-14, 0x108, 0, 1, origin_x, origin_y + 0x118, 2, 0);
        }
    }

    if (m_prepared_06c) {
        W8TextBuffer005ED5B8 text;
        W8ControlsRect bounds = {4, 0xec, 0xc2, 0x162};
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0xe8], g_font_683660);
        text.RenderToTarget(0, 1, -14);
        bounds.top = 0x162;
        bounds.right = 0x8f;
        bounds.bottom = 0x179;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0xe3], g_font_683660);
        text.RenderToTarget(0, 1, -14);
        bounds.top = 0x184;
        bounds.bottom = 0x19b;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0xe4], g_font_683660);
        text.RenderToTarget(0, 1, -14);
        bounds.left = 0x8f;
        bounds.top = 0x162;
        bounds.right = 0xbf;
        bounds.bottom = 0x179;
        DrawCatalogImage(-14, 0x107, 0, 5, 0x8f, 0x162, 2, 0);
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(FormatWideString(L"%d",
            m_creation_state_064->skill_step_limit),
            g_options_detail_font_683614);
        text.RenderToTarget(0, 1, -14);
        m_prepared_06c = 0;
    }

    if (m_dirty_06d) {
        W8TextBuffer005ED5B8 text;
        W8ControlsRect bounds = {0x8f, 0x184, 0xbf, 0x19b};
        DrawCatalogImage(-14, 0x107, 0, 5, 0x8f, 0x184, 2, 0);
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(FormatWideString(L"%d/%d",
            m_creation_state_064->skill_points_remaining,
            m_creation_state_064->skill_points_total),
            g_options_detail_font_683614);
        text.RenderToTarget(0, 1, -14);
        m_dirty_06d = 0;
    }
}

// FUNCTION: WIZ8 0x005c7d30
void W8CharacterPage005EF5C8::Refresh()
{
    m_force_redraw_074 = 1;
}

// FUNCTION: WIZ8 0x005c7cc0
W8CharacterPage005EF5C8* CreateCharacterPage005C7CC0()
{
    return new W8CharacterPage005EF5C8;
}

// VTABLE: WIZ8 0x005ef57c W8CharacterPage005EF57C
// VTABLE: WIZ8 0x005ef578 W8ControlSelectionListener
// VTABLE: WIZ8 0x005ef570 W8TextControlActionListener005ED664
// class W8CharacterPage005EF57C

// FUNCTION: WIZ8 0x005c6460
void W8CharacterPage005EF57C::SetCharacter(
    W8Character* character, W8CharacterCreationState* creation_state, int mode)
{
    AcquireRegionSet(&g_character_page4_region_set_0069c52c);
    W8CharacterPage::SetCharacter(character, creation_state, mode);
    W8TextControl005ED604::Listener* action_listener =
        reinterpret_cast<W8TextControl005ED604::Listener*>(
            static_cast<W8TextControlActionListener005ED664*>(this));

    m_control_07c = new W8TextControl005ED604(
        this, 0xffffffff, 100, 0x19, 0, 0, 0x10a, 0,
        10, 0xc, 0xb, 0xe, 0xd);
    m_control_07c->m_listener = action_listener;
    m_control_078 = new W8TextControl005ED604(
        this, 0xffffffff, 0x144, 0x19, 0, 0, 0x10a, 0,
        0xf, 0x11, 0x10, 0x13, 0x12);
    m_control_078->m_listener = action_listener;
    m_control_084 = new W8TextControl005ED604(
        this, 0xffffffff, 100, 0x67, 0, 0, 0x10a, 0,
        10, 0xc, 0xb, 0xe, 0xd);
    m_control_084->m_listener = action_listener;
    m_control_080 = new W8TextControl005ED604(
        this, 0xffffffff, 0x144, 0x67, 0, 0, 0x10a, 0,
        0xf, 0x11, 0x10, 0x13, 0x12);
    m_control_080->m_listener = action_listener;
    m_randomize_088 = new W8TextControl005ED604(
        this, 0xffffffff, 0x16d, 0x155, 0, 0, 0x10a, 0,
        0x14, 0x16, 0x15, 0x18, 0x17);
    m_randomize_088->m_listener = action_listener;
    m_randomize_088->EnableRegionHelp(0xf5);

    int index;
    for (index = 0; index < 9; ++index) {
        int column = index % 3;
        int row = index / 3;
        W8TextControl005ED604* entry = new W8TextControl005ED604(
            this, 0xffffffff, column * 0x80 + 0x24, row * 0xe + 0x107,
            column * 0x80 + 0xa3, row * 0xe + 0x114,
            0x105, 0, 5, 7, 6, 8, -1);
        entry->AddLayoutFlags(g_W8TextControlMask005ED594);
        m_personality_selection_08c.AddEntry(entry);
    }
    m_personality_selection_08c.SetSelected(character->personality_0081);
    m_personality_selection_08c.m_selectionListener = this;

    for (index = 0; index < 2; ++index) {
        int top = index == 0 ? 0x140 : 0x15d;
        W8TextControl005ED604* entry = new W8TextControl005ED604(
            this, 0xffffffff, 0x21, top, 0x69, top + 0xe,
            0x105, 0, 5, 7, 6, 8, -1);
        entry->AddLayoutFlags(g_W8TextControlMask005ED594);
        m_voice_selection_0b0.AddEntry(entry);
    }
    ClampInteger(&character->voice_0085, 0, 1);
    m_voice_selection_0b0.SetSelected(character->voice_0085);
    m_voice_selection_0b0.m_selectionListener = this;
}

// FUNCTION: WIZ8 0x005c6820
void W8CharacterPage005EF57C::Activate()
{
    EnableRegionSet(1);
    m_prepared_06c = 1;
    InitTextInputModeWithScheme(1);
    AddTextInputField(origin_x + 0x97, origin_y + 0xab, 0x106, 0x10,
                      0x7f, m_character_060->name_part_2, 0x27, 0xf, 1);
    AddTextInputField(origin_x + 0x97, origin_y + 0xc7, 0x106, 0x10,
                      0x7f, m_character_060->name, 9, 0xf, 1);
    if (GetTextInputFieldLength(0) == 0) SetActiveField(0);
    else if (GetTextInputFieldLength(1) == 0) SetActiveField(1);
    m_personality_selection_08c.SetSelected(m_character_060->personality_0081);
    m_voice_selection_0b0.SetSelected(m_character_060->voice_0085);
}

// FUNCTION: WIZ8 0x005c68f0
void W8CharacterPage005EF57C::Deactivate()
{
    EnableRegionSet(0);
    RemoveTextInputField(1);
    RemoveTextInputField(0);
    KillTextInputMode();
}

// FUNCTION: WIZ8 0x005c6910
void W8CharacterPage005EF57C::Accept()
{
    if (m_mode_068 == 0) {
        Function558610(m_character_060);
    }
    else {
        W8Character* original = m_screen_05c->GetOriginalCharacter();
        m_character_060->personality_0081 = original->personality_0081;
        m_character_060->table_value_0079 = original->table_value_0079;
        m_character_060->voice_0085 = original->voice_0085;
        wcscpy(m_character_060->name_part_2, original->name_part_2);
        wcscpy(m_character_060->name, original->name);
    }
    Refresh();
    Invalidate(0);
    m_screen_05c->UpdateNavigation(this);
}

// FUNCTION: WIZ8 0x005c69a0
void W8CharacterPage005EF57C::GetNavigationState(
    unsigned char* next_enabled, unsigned char* exit_enabled)
{
    *next_enabled = static_cast<unsigned char>(
        GetTextInputFieldLength(0) != 0 && GetTextInputFieldLength(1) != 0);
    if (m_mode_068 != 0) {
        W8Character* original = m_screen_05c->GetOriginalCharacter();
        *exit_enabled = 0;
        if (m_character_060->personality_0081 == original->personality_0081 &&
            m_character_060->table_value_0079 == original->table_value_0079 &&
            m_character_060->voice_0085 == original->voice_0085 &&
            wcscmp(m_character_060->name_part_2, original->name_part_2) == 0 &&
            wcscmp(m_character_060->name, original->name) == 0) {
            return;
        }
        *exit_enabled = 1;
    }
    else {
        *exit_enabled = 1;
    }
}

// FUNCTION: WIZ8 0x005c6b20
void W8CharacterPage005EF57C::Refresh()
{
    SetInputFieldStringWith16BitString(0, m_character_060->name_part_2);
    SetInputFieldStringWith16BitString(1, m_character_060->name);
    m_personality_selection_08c.SetSelected(m_character_060->personality_0081);
    m_voice_selection_0b0.SetSelected(m_character_060->voice_0085);
}

// FUNCTION: WIZ8 0x005c73b0
void W8CharacterPage005EF57C::OnSelectionChanged(
    W8Control005ED654* control, int selected)
{
    if (control == &m_voice_selection_0b0) {
        m_character_060->voice_0085 = selected;
    }
    else {
        m_character_060->personality_0081 = selected;
    }
    m_description_dirty_0fd = 1;
    m_screen_05c->UpdateNavigation(this);
}

// FUNCTION: WIZ8 0x005c7220
void W8CharacterPage005EF57C::OnControlAction(
    W8TextControl005ED604* control)
{
    int portrait = m_character_060->table_value_0079;
    if (control == m_control_078) {
        int group = g_portrait_descriptors_6483d0[portrait].group + 1;
        if (group > 11) group = 0;
        m_character_060->table_value_0079 =
            g_portrait_groups_648950[group].portraits[0];
        m_portrait_dirty_0fe = 1;
    }
    else if (control == m_control_07c) {
        int group = g_portrait_descriptors_6483d0[portrait].group - 1;
        if (group < 0) group = 11;
        m_character_060->table_value_0079 =
            g_portrait_groups_648950[group].portraits[0];
        m_portrait_dirty_0fe = 1;
    }
    else if (control == m_control_080 || control == m_control_084) {
        int group = g_portrait_descriptors_6483d0[portrait].group;
        W8PortraitGroup* portraits = &g_portrait_groups_648950[group];
        int index = 0;
        while (index < portraits->count && portraits->portraits[index] != portrait) {
            ++index;
        }
        if (control == m_control_080) {
            ++index;
            if (index >= portraits->count) index = 0;
        }
        else {
            --index;
            if (index < 0) index = portraits->count - 1;
        }
        m_character_060->table_value_0079 = portraits->portraits[index];
        m_portrait_dirty_0fe = 1;
    }
    else if (control == m_randomize_088) {
        m_animation_active_0fc = 1;
        m_animation_frame_0f8 = 2;
        m_animation_timer_0d4.Restart();
        ShadowVideoSurfaceRect(-14, 0, 0, 0x280, 0x1e0);
        Function422F10();
        m_screen_05c->ShowCharacterSummary();
    }

    if (control != m_randomize_088) {
        m_screen_05c->UpdateNavigation(this);
    }
}
