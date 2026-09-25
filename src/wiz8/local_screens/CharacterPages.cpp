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
#include "wiz8/engine_code/Environment.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/local_code/ControlsRect.h"
#include "input.h"
#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/local_code/ControlSelection.h"
#include "wiz8/local_screens/CharacterScreen.h"

#include "wiz8/local_code/Strings.h"
#include "wiz8/character_event_queue.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/regions.h"
#include "wiz8/text_input.h"
#include "wiz8/utility.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/fonts.h"
#include "vsurface.h"

#include <new>
#include <string.h>

// GLOBAL: WIZ8 0x006483d0
W8PortraitDescriptor g_portrait_descriptors[80] = {
    {0, 0, 0, 1},  {0, 0, 1, 0},  {0, 0, 2, 2},  {0, 0, 3, 0},  {0, 0, 3, 1},  {0, 0, 3, 2},
    {0, 1, 0, 2},  {0, 1, 1, 0},  {0, 1, 2, 0},  {0, 1, 3, 1},  {0, 1, 3, 0},  {0, 1, 3, 0},
    {1, 0, 0, 1},  {1, 0, 1, 1},  {1, 0, 2, 2},  {1, 1, 0, 1},  {1, 1, 1, 1},  {1, 1, 2, 0},
    {2, 0, 0, 1},  {2, 0, 1, 2},  {2, 0, 2, 1},  {2, 1, 0, 1},  {2, 1, 1, 1},  {2, 1, 2, 2},
    {3, 0, 1, 1},  {3, 0, 2, 2},  {3, 1, 0, 0},  {3, 1, 1, 1},  {4, 0, 0, 1},  {4, 0, 1, 1},
    {4, 1, 0, 1},  {4, 1, 1, 0},  {5, 0, 0, 0},  {5, 0, 2, 1},  {5, 1, 0, 1},  {5, 1, 1, 0},
    {6, 0, 0, 1},  {6, 0, 1, 2},  {6, 1, 0, 0},  {6, 1, 1, 2},  {7, 0, 0, 0},  {7, 0, 1, 2},
    {7, 1, 0, 2},  {7, 1, 1, 2},  {8, 0, 0, 0},  {8, 0, 1, 2},  {8, 1, 0, 2},  {8, 1, 1, 1},
    {9, 0, 1, 2},  {9, 0, 2, 1},  {9, 1, 0, 1},  {9, 1, 1, 1},  {10, 0, 0, 2}, {10, 0, 1, 1},
    {10, 1, 0, 2}, {10, 1, 1, 1}, {11, 0, 4, 0}, {11, 1, 4, 2}, {10, 0, 3, 2}, {11, 0, 1, 1},
    {11, 0, 0, 0}, {11, 0, 1, 0}, {12, 0, 3, 2}, {13, 0, 3, 1}, {13, 0, 3, 1}, {13, 0, 3, 2},
    {13, 0, 3, 2}, {13, 0, 3, 1}, {13, 0, 3, 0}, {13, 0, 3, 1}, {13, 0, 3, 1}, {13, 0, 3, 2},
    {13, 0, 3, 0}, {13, 0, 3, 1}, {13, 0, 3, 0}, {13, 0, 3, 0}, {0, 0, 3, 0},  {0, 1, 3, 2},
    {1, 0, 3, 1},  {1, 1, 3, 0},
};
// VTABLE: WIZ8 0x005ef1d8 W8CharacterPageEntry
// class W8CharacterPageEntry

// SYNTHETIC: WIZ8 0x005af990
// W8CharacterPageEntry::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005af690
W8CharacterPageEntry::W8CharacterPageEntry(Controls* owner, int x, int y, bool compact)
    : m_listener_004(0), m_first_020(0), m_second_024(0), m_third_028(0), m_id_02c(-1),
      m_draw_background_038(compact), m_dirty_039(1), m_enabled_03a(0), m_increment_allowed_03b(1)
{
    m_x_030 = owner->origin_x + x;
    m_y_034 = owner->origin_y + y;
    int split = m_x_030 + (compact ? 0x6d : 0x78);
    W8ControlsRect bounds = {m_x_030 + 5, m_y_034 + 1, split, m_y_034 + 0xd};
    m_label_014 = new W8TextBuffer(&bounds, 0, 0, 0, 4);
    m_label_014->SetLayoutMode(g_W8TextBufferLayoutMask005ED548);
    bounds.left = split;
    bounds.right = split + 0x17;
    m_first_text_018 = new W8TextBuffer(&bounds, 0, 0, 0, 4);
    m_first_text_018->SetLayoutMode(g_W8TextBufferLayoutMask005ED550);
    bounds.left = split + 0x2a;
    bounds.right = split + 0x39;
    m_second_text_01c = new W8TextBuffer(&bounds, 0, 0, 0, 4);
    m_second_text_01c->SetLayoutMode(g_W8TextBufferLayoutMask005ED550);

    int relative_split = split - owner->origin_x;
    m_decrement_00c = new W8TextControl(owner, 0xffffffff, relative_split + 0x1b, y + 1, 0, 0,
                                        0x10a, 0, 0x19, 0x1b, 0x1a, 0x1d, 0x1c);
    m_decrement_00c->AddLayoutFlags(0x100);
    m_decrement_00c->SetEnabled(0);
    m_decrement_00c->SetActive(0);
    m_decrement_00c->m_listener = this;

    m_increment_008 = new W8TextControl(owner, 0xffffffff, relative_split + 0x3d, y + 1, 0, 0,
                                        0x10a, 0, 0x1e, 0x20, 0x1f, 0x22, 0x21);
    m_increment_008->AddLayoutFlags(0x100);
    m_increment_008->SetActive(0);
    m_increment_008->m_listener = this;

    m_help_010 = new W8TextControl(owner, 0xffffffff, x, y, relative_split, y + 0xc, -1, -1, -1, -1,
                                   -1, -1, -1);
    m_help_010->SetActive(0);
    m_help_010->m_listener = this;
}

// FUNCTION: WIZ8 0x005af9e0
void W8CharacterPageEntry::SetContent(unsigned int id, const wchar_t* label, unsigned int* first,
                                      int* second, int* third, int help_id)
{
    m_id_02c = id;
    m_first_020 = first;
    m_second_024 = second;
    m_third_028 = third;
    m_label_014->SetText(label, g_font_683660);
    if (help_id == -1)
        m_help_010->DisableRegionHelp();
    else
        m_help_010->EnableRegionHelp(help_id);
    m_enabled_03a = 1;
    m_increment_008->SetActive(1);
    m_decrement_00c->SetActive(1);
    m_help_010->SetActive(1);
    m_decrement_00c->Invalidate(0);
    m_increment_008->Invalidate(0);
    m_dirty_039 = 1;
    UpdateButtons();
    m_decrement_00c->Invalidate(0);
    m_increment_008->Invalidate(0);
    m_dirty_039 = 1;
}

// FUNCTION: WIZ8 0x005afc20
void W8CharacterPageEntry::SetIncrementAllowed(bool allowed)
{
    m_increment_allowed_03b = allowed;
    UpdateButtons();
    m_decrement_00c->Invalidate(0);
    m_increment_008->Invalidate(0);
    m_dirty_039 = 1;
}

// FUNCTION: WIZ8 0x005afa90
void W8CharacterPageEntry::SetEnabled(bool enabled)
{
    m_enabled_03a = enabled;
    m_increment_008->SetActive(enabled);
    m_decrement_00c->SetActive(enabled);
    m_help_010->SetActive(enabled);
    m_decrement_00c->Invalidate(0);
    m_increment_008->Invalidate(0);
    m_dirty_039 = 1;
}

// FUNCTION: WIZ8 0x005afae0
void W8CharacterPageEntry::SetHelpActive(bool active)
{
    m_help_010->SetActive(active);
}

// FUNCTION: WIZ8 0x005afaf0
void W8CharacterPageEntry::Redraw()
{
    if (m_enabled_03a && m_dirty_039) {
        if (m_draw_background_038) {
            DrawCatalogImageAndInvalidate(-14, 0x108, 0, 2, m_x_030, m_y_034, 2, 0);
        }
        m_first_text_018->SetText(FormatWideString(L"%d", *m_first_020), g_font_683660);
        m_second_text_01c->SetText(FormatWideString(L"%d", *m_second_024), g_font_683660);
        m_label_014->FillBounds(0x8000);
        m_first_text_018->FillBounds(0x8000);
        m_second_text_01c->FillBounds(0x8000);
        m_label_014->RenderToTarget(0, 1, -14);
        m_first_text_018->RenderToTarget(0, 1, -14);
        m_second_text_01c->RenderToTarget(0, 1, -14);
        m_dirty_039 = 0;
    } else if (!m_draw_background_038 && m_dirty_039) {
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
        bool enabled = *m_second_024 > 0;
        if (m_decrement_00c->m_enabled != enabled) {
            m_decrement_00c->SetEnabled(enabled);
            m_decrement_00c->Invalidate(0);
        }
        enabled = m_increment_allowed_03b && *m_second_024 < *m_third_028;
        if (m_increment_008->m_enabled != enabled) {
            m_increment_008->SetEnabled(enabled);
            m_increment_008->Invalidate(0);
        }
        m_help_010->SetEnabled(1);
    }
}

// FUNCTION: WIZ8 0x005afc50
void W8CharacterPageEntry::OnPrimary(W8TextControl* control)
{
    if (control == m_increment_008) {
        if (m_listener_004 != 0)
            m_listener_004->AdjustEntry(this, 1);
    } else if (control == m_decrement_00c) {
        if (m_listener_004 != 0)
            m_listener_004->AdjustEntry(this, -1);
    }
    UpdateButtons();
    m_decrement_00c->Invalidate(0);
    m_increment_008->Invalidate(0);
    m_dirty_039 = 1;
}

// FUNCTION: WIZ8 0x005afcb0
void W8CharacterPageEntry::OnSecondary(W8TextControl* control)
{
    if (control == m_increment_008) {
        if (m_listener_004 != 0)
            m_listener_004->AdjustEntry(this, 5);
        UpdateButtons();
        m_decrement_00c->Invalidate(0);
        m_increment_008->Invalidate(0);
        m_dirty_039 = 1;
    } else if (control == m_decrement_00c) {
        if (m_listener_004 != 0)
            m_listener_004->AdjustEntry(this, -5);
        UpdateButtons();
        m_decrement_00c->Invalidate(0);
        m_increment_008->Invalidate(0);
        m_dirty_039 = 1;
    } else if (m_listener_004 != 0) {
        m_listener_004->ShowEntryInfo(this);
    }
}

// VTABLE: WIZ8 0x005ef1e4 W8CharacterPage
// class W8CharacterPage

// VTABLE: WIZ8 0x005ef218 W8GrowableVector<W8CharacterPageEntry*>
// class W8GrowableVector<W8CharacterPageEntry*>

// FUNCTION: WIZ8 0x005afd90
W8CharacterPage::W8CharacterPage(int render_target)
    : Controls(0xc3, 0x2b, 0x280, 0x1c1, render_target, 0, 0), m_screen_05c(0)
{
}

// SYNTHETIC: WIZ8 0x005afe20
// W8CharacterPage::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005afe40
W8CharacterPage::~W8CharacterPage()
{
    DestroyAllControls();
    while (m_entries_04c.count > 0) {
        m_entries_04c.RemoveAtAndDelete(m_entries_04c.count - 1);
    }
}

// FUNCTION: WIZ8 0x005aff00
void W8CharacterPage::SetCharacter(W8Character* character, W8CharacterCreationState* creation_state,
                                   int mode)
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
        (*m_entries_04c.GetAt(index))->Redraw();
    }
}

// FUNCTION: WIZ8 0x005aff50
void W8CharacterPage::Invalidate(const W8ControlsRect* rect)
{
    Controls::Invalidate(rect);
    for (int index = 0; index < m_entries_04c.count; ++index) {
        W8CharacterPageEntry* entry = *m_entries_04c.GetAt(index);
        entry->m_decrement_00c->Invalidate(0);
        entry->m_increment_008->Invalidate(0);
        entry->m_dirty_039 = 1;
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
