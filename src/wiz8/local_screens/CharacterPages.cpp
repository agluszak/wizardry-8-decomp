#include "wiz8/local_screens/CharacterScreen.h"

#include "wiz8/local_code/Strings.h"
#include "wiz8/video_object_catalog.h"

#include <new>
#include <string.h>

extern int g_font_683660;
extern wchar_t* FormatWideString(const wchar_t*, ...);
extern unsigned int g_character_page2_region_set_0069c530;
extern int g_character_page2_category_geometry_64ef90[5][2];
extern void Function557F90(W8Character*, void*);
extern void Function557BC0(W8Character*, void*, unsigned int, int);

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
    unsigned int id, const wchar_t* label, int* first, int* second, int* third,
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
            Function549600(-14, 0x108, 0, 2, m_x_030, m_y_034, 2, 0);
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

// FUNCTION: WIZ8 0x005afd90
W8CharacterPage::W8CharacterPage(int render_target)
    : Controls(0xc3, 0x2b, 0x280, 0x1c1, render_target, 0, 0),
      m_entry_count_050(0), m_entry_capacity_054(0), m_entries_058(0),
      m_screen_05c(0), m_character_060(0), m_creation_state_064(0),
      m_mode_068(0), m_prepared_06c(0), m_dirty_06d(0)
{
    m_entries_058 = static_cast<W8CharacterPageEntry**>(::operator new(0x14));
    if (m_entries_058 != 0) {
        m_entry_capacity_054 = 5;
        memset(m_entries_058, 0, 0x14);
    }
}

// SYNTHETIC: WIZ8 0x005afe20
// W8CharacterPage::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005afe40
W8CharacterPage::~W8CharacterPage()
{
    DestroyAllControls();
    while (m_entry_count_050 > 0) {
        int index = m_entry_count_050 - 1;
        W8CharacterPageEntry* entry = m_entries_058[index];
        for (; index < m_entry_count_050 - 1; ++index) {
            m_entries_058[index] = m_entries_058[index + 1];
        }
        --m_entry_count_050;
        delete entry;
    }
    ::operator delete(m_entries_058);
}

// FUNCTION: WIZ8 0x005aff00
void W8CharacterPage::SetCharacter(
    W8Character* character, void* creation_state, int mode)
{
    m_character_060 = character;
    m_creation_state_064 = creation_state;
    m_mode_068 = mode;
}

// FUNCTION: WIZ8 0x005aff20
void W8CharacterPage::Redraw()
{
    Controls::Redraw();
    for (int index = 0; index < m_entry_count_050; ++index) {
        m_entries_058[index]->Redraw();
    }
}

// FUNCTION: WIZ8 0x005aff50
void W8CharacterPage::Invalidate(const W8ControlsRect* rect)
{
    Controls::Invalidate(rect);
    for (int index = 0; index < m_entry_count_050; ++index) {
        m_entries_058[index]->m_decrement_00c->Invalidate(0);
        m_entries_058[index]->m_increment_008->Invalidate(0);
        m_entries_058[index]->m_dirty_039 = 1;
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
    int new_count = m_entry_count_050 + 1;
    if (new_count > m_entry_capacity_054) {
        W8CharacterPageEntry** previous = m_entries_058;
        W8CharacterPageEntry** entries =
            static_cast<W8CharacterPageEntry**>(::operator new(new_count * 4));
        if (entries == 0) return;
        m_entries_058 = entries;
        m_entry_capacity_054 = new_count;
        for (int index = 0; index < m_entry_count_050; ++index) {
            entries[index] = previous[index];
        }
        ::operator delete(previous);
    }
    m_entries_058[m_entry_count_050++] = entry;
}

void W8CharacterPage::HandleInput(InputAtom*) {}
void W8CharacterPage::Refresh() {}

// VTABLE: WIZ8 0x005ef5c8 W8CharacterPage005EF5C8
// VTABLE: WIZ8 0x005ef5c0 W8CharacterPageEntryListener
// class W8CharacterPage005EF5C8

// FUNCTION: WIZ8 0x005c7580
void W8CharacterPage005EF5C8::SetCharacter(
    W8Character* character, void* creation_state, int mode)
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
    for (int index = 0; index < m_entry_count_050; ++index) {
        m_entries_058[index]->UpdateButtons();
    }
}

// FUNCTION: WIZ8 0x005c7720
void W8CharacterPage005EF5C8::GetNavigationState(
    unsigned char* next_enabled, unsigned char* exit_enabled)
{
    unsigned char* state = static_cast<unsigned char*>(m_creation_state_064);
    *next_enabled = state[0x1b8];
    *exit_enabled =
        *reinterpret_cast<int*>(state + 0x64) <
        *reinterpret_cast<int*>(state + 0x68);
    if (*next_enabled != m_navigation_state_076) {
        for (int index = 0; index < m_entry_count_050; ++index) {
            m_entries_058[index]->SetIncrementAllowed(
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
