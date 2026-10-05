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

// FUNCTION: WIZ8 0x005af690
W8CharacterPageEntry::W8CharacterPageEntry(Controls* owner, int x, int y, bool compact)
    : m_listener(0), m_first(0), m_second(0), m_third(0), m_id(-1),
      m_draw_background(compact), m_dirty(1), m_enabled(0), m_increment_allowed(1)
{
    m_x = owner->m_bounds.left + x;
    m_y = owner->m_bounds.top + y;
    int split = m_x + (compact ? 0x6d : 0x78);
    W8ControlsRect bounds = {m_x + 5, m_y + 1, split, m_y + 0xd};
    m_label = new W8TextBuffer(&bounds, 0, 0, 0, 4);
    m_label->SetLayoutMode(g_W8TextBufferAlignLeft);
    bounds.left = split;
    bounds.right = split + 0x17;
    m_first_text = new W8TextBuffer(&bounds, 0, 0, 0, 4);
    m_first_text->SetLayoutMode(g_W8TextBufferAlignRight);
    bounds.left = split + 0x2a;
    bounds.right = split + 0x39;
    m_second_text = new W8TextBuffer(&bounds, 0, 0, 0, 4);
    m_second_text->SetLayoutMode(g_W8TextBufferAlignRight);

    int relative_split = split - owner->m_bounds.left;
    m_decrement = new W8TextControl(owner, 0xffffffff, relative_split + 0x1b, y + 1, 0, 0,
                                        0x10a, 0, 0x19, 0x1b, 0x1a, 0x1d, 0x1c);
    m_decrement->AddLayoutFlags(0x100);
    m_decrement->SetEnabled(false);
    m_decrement->SetActive(false);
    m_decrement->m_listener = this;

    m_increment = new W8TextControl(owner, 0xffffffff, relative_split + 0x3d, y + 1, 0, 0,
                                        0x10a, 0, 0x1e, 0x20, 0x1f, 0x22, 0x21);
    m_increment->AddLayoutFlags(0x100);
    m_increment->SetActive(false);
    m_increment->m_listener = this;

    m_help = new W8TextControl(owner, 0xffffffff, x, y, relative_split, y + 0xc, -1, -1, -1, -1,
                                   -1, -1, -1);
    m_help->SetActive(false);
    m_help->m_listener = this;
}

// FUNCTION: WIZ8 0x005af9e0
void W8CharacterPageEntry::SetContent(unsigned int id, const wchar_t* label, unsigned int* first,
                                      int* second, int* third, int help_id)
{
    m_id = id;
    m_first = first;
    m_second = second;
    m_third = third;
    m_label->SetText(label, g_wiz_text_font_secondary);
    if (help_id == -1)
        m_help->DisableRegionHelp();
    else
        m_help->EnableRegionHelp(help_id);
    SetEnabled(true);
    UpdateButtons();
    MarkDirty();
}

// FUNCTION: WIZ8 0x005afc20
void W8CharacterPageEntry::SetIncrementAllowed(bool allowed)
{
    m_increment_allowed = allowed;
    UpdateButtons();
    MarkDirty();
}

// FUNCTION: WIZ8 0x005afa90
void W8CharacterPageEntry::SetEnabled(bool enabled)
{
    m_enabled = enabled;
    m_increment->SetActive(enabled);
    m_decrement->SetActive(enabled);
    m_help->SetActive(enabled);
    MarkDirty();
}

// FUNCTION: WIZ8 0x005afae0
void W8CharacterPageEntry::SetHelpActive(bool active)
{
    m_help->SetActive(active);
}

// FUNCTION: WIZ8 0x005afaf0
void W8CharacterPageEntry::Redraw()
{
    if (m_enabled && m_dirty) {
        if (m_draw_background) {
            DrawCatalogImageAndInvalidate(-14, 0x108, 0, 2, m_x, m_y, 2, 0);
        }
        m_first_text->SetText(FormatWideString(g_format_d, *m_first),
                                  g_wiz_text_font_secondary);
        m_second_text->SetText(FormatWideString(g_format_d, *m_second),
                                   g_wiz_text_font_secondary);
        m_first_text->FillBounds(0x8000);
        m_second_text->FillBounds(0x8000);
        m_label->RenderToTarget(0, true, -14);
        m_first_text->RenderToTarget(0, true, -14);
        m_second_text->RenderToTarget(0, true, -14);
        m_dirty = false;
    } else if (!m_draw_background && m_dirty) {
        m_label->RenderToTarget(0, true, -14);
        m_dirty = false;
    }
}

// FUNCTION: WIZ8 0x005afbf0
void W8CharacterPageEntry::SetLabelFontState(int state)
{
    m_label->SetFontStateIndex(state);
}

// FUNCTION: WIZ8 0x005afc00
void W8CharacterPageEntry::MarkDirty()
{
    m_decrement->Invalidate(false);
    m_increment->Invalidate(false);
    m_dirty = true;
}

// FUNCTION: WIZ8 0x005afd10
void W8CharacterPageEntry::UpdateButtons()
{
    if (m_enabled) {
        bool enabled = *m_second > 0;
        if (m_decrement->m_enabled != enabled) {
            m_decrement->SetEnabled(enabled);
            m_decrement->Invalidate(false);
        }
        enabled = m_increment_allowed && *m_second < *m_third;
        if (m_increment->m_enabled != enabled) {
            m_increment->SetEnabled(enabled);
            m_increment->Invalidate(false);
        }
        m_help->SetEnabled(true);
    }
}

// FUNCTION: WIZ8 0x005afc50
void W8CharacterPageEntry::OnPrimary(W8TextControl* control)
{
    if (control == m_increment) {
        if (m_listener != 0)
            m_listener->AdjustEntry(this, 1);
    } else if (control == m_decrement) {
        if (m_listener != 0)
            m_listener->AdjustEntry(this, -1);
    }
    UpdateButtons();
    MarkDirty();
}

// FUNCTION: WIZ8 0x005afcb0
void W8CharacterPageEntry::OnSecondary(W8TextControl* control)
{
    if (control == m_increment) {
        if (m_listener != 0)
            m_listener->AdjustEntry(this, 5);
        UpdateButtons();
        MarkDirty();
    } else if (control == m_decrement) {
        if (m_listener != 0)
            m_listener->AdjustEntry(this, -5);
        UpdateButtons();
        MarkDirty();
    } else if (m_listener != 0) {
        m_listener->ShowEntryInfo(this);
    }
}

// VTABLE: WIZ8 0x005ef1e4 W8CharacterPage
// class W8CharacterPage

// VTABLE: WIZ8 0x005ef218 W8GrowableVector<W8CharacterPageEntry*>
// class W8GrowableVector<W8CharacterPageEntry*>

// FUNCTION: WIZ8 0x005afd90
W8CharacterPage::W8CharacterPage(int render_target)
    : Controls(0xc3, 0x2b, 0x280, 0x1c1, render_target, 0, 0), m_screen(0)
{
}

// FUNCTION: WIZ8 0x005afe40
W8CharacterPage::~W8CharacterPage()
{
    DestroyAllControls();
    while (m_entries.count > 0) {
        m_entries.RemoveAtAndDelete(m_entries.count - 1);
    }
}

// FUNCTION: WIZ8 0x005aff00
void W8CharacterPage::SetCharacter(W8Character* character, W8CharacterCreationState* creation_state,
                                   int mode)
{
    m_character = character;
    m_creation_state = creation_state;
    m_mode = mode;
}

// FUNCTION: WIZ8 0x005aff20
void W8CharacterPage::Redraw()
{
    Controls::Redraw();
    for (int index = 0; index < m_entries.count; ++index) {
        (*m_entries.GetAt(index))->Redraw();
    }
}

// FUNCTION: WIZ8 0x005aff50
void W8CharacterPage::Invalidate(const W8ControlsRect* rect)
{
    Controls::Invalidate(rect);
    for (int index = 0; index < m_entries.count; ++index) {
        W8CharacterPageEntry* entry = *m_entries.GetAt(index);
        entry->MarkDirty();
    }
}

// FUNCTION: WIZ8 0x005affa0
void W8CharacterPage::Prepare()
{
    Invalidate(0);
    m_dirty = true;
    m_prepared = true;
}

// FUNCTION: WIZ8 0x005affc0
void W8CharacterPage::AddEntry(W8CharacterPageEntry* entry)
{
    m_entries.Add(entry);
}

void W8CharacterPage::HandleInput(InputAtom*) {}
void W8CharacterPage::Refresh() {}
