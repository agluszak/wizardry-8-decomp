#include "wiz8/local_code/Widget.h"
#include "wiz8/local_code/RangeControl.h"
#include "wiz8/local_screens/CharacterScreen.h"

#include "wiz8/cursor.h"
#include "wiz8/dirty_tiles.h"
#include "wiz8/local_code/ButtonSound.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/magic.h"
#include "wiz8/screen_state.h"
#include "wiz8/fonts.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/video_object_catalog.h"
#include "Font.h"

#include <new>
#include <wchar.h>

// GLOBAL: WIZ8 0x0069c534
unsigned int g_character_spells_region_set_0069c534;
// GLOBAL: WIZ8 0x0069c538
unsigned int g_character_spell_list_region_sets_0069c538[6];

// GLOBAL: WIZ8 0x00648c90
W8SpellRealmAnimation g_spell_realm_animations_00648c90[6] = {
    {486, 22, 3}, {487, 18, 8}, {488, 14, 8},
    {489, 21, 1}, {490, 16, 14}, {491, 24, 11}
};

/* Each realm owns a range panel. Its range callback is the existing
   W8RangeListener subobject at +0x34, not a second widget base. */
// VTABLE: WIZ8 0x005ef614 W8Widget
// VTABLE: WIZ8 0x005ef610 W8RangeListener
class W8CharacterSpellList : public W8Widget,
                                     public W8RangeListener {
public:
    W8CharacterSpellList(Controls* owner, int x, int y,
                                W8CharacterSpellEntry* entries,
                                unsigned int* region_set);
    virtual ~W8CharacterSpellList() override;
    virtual void Redraw(int force) override;
    virtual void OnMouseEnter(int event) override;
    virtual void OnMouseLeave(int event) override;
    virtual void OnMouseMove(int event) override;
    virtual void AdjustValue(int steps) override;
    virtual void OnLeftButtonDown(int event) override;
    virtual void OnRightButtonDown(int event) override;
    virtual void OnLeftButtonUp(int event) override;
    virtual void OnRightButtonUp(int event) override;
    virtual void OnRangeChanged(W8RangeControl* range) override;
    void SetEntryCount(int count);

    W8RangeControl* m_range;       /* 0x38 */
    W8CharacterSpellEntry* m_entries;     /* 0x3c */
    int m_first_entry;                   /* 0x40 */
    int m_entry_count;                   /* 0x44 */
    int m_hovered_entry;                 /* 0x48 */
    int m_scroll_offset;                 /* 0x4c */
    W8CharacterSpellListListener* m_listener; /* 0x50 */
    unsigned int* m_region_set;          /* 0x54 */
    int m_x;                            /* 0x58 */
    int m_y;                            /* 0x5c */
};
static_assert(sizeof(W8CharacterSpellList) == 0x60,
              "W8CharacterSpellList_size");

W8CharacterSpellList::W8CharacterSpellList(
    Controls* owner, int x, int y, W8CharacterSpellEntry* entries,
    unsigned int* region_set)
    : W8Widget(owner, 0xffffffff, x + 0x1a, y + 0x19,
                           x + 0xb4, y + 0x79),
      m_range(0), m_entries(entries), m_hovered_entry(-1), m_scroll_offset(0),
      m_listener(0), m_region_set(region_set), m_x(x), m_y(y)
{
}

// SYNTHETIC: WIZ8 0x005c7d80
// W8CharacterSpellList::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005c7da0
W8CharacterSpellList::~W8CharacterSpellList()
{
    delete m_range;
}

// FUNCTION: WIZ8 0x005c7e10
void W8CharacterSpellList::SetEntryCount(int count)
{
    if (!m_range) {
        m_range = new W8RangeControl(
            m_pPanel->origin_x + m_x + 0xb8, m_pPanel->origin_y + m_y + 0x1a,
            m_pPanel->origin_x + m_x + 0xca, m_pPanel->origin_y + m_y + 0x78,
            m_region_set);
        m_range->m_listener = this;
    }
    m_range->Invalidate(0);
    m_range->SetEnabled(1);
    m_entry_count = count;
    if (count > 7) {
        m_range->SetRange(0, count - 7);
        m_range->SetRangeEnabled(1);
    } else {
        m_range->SetRangeEnabled(0);
    }
}

// FUNCTION: WIZ8 0x005c7ef0
void W8CharacterSpellList::Redraw(int force)
{
    if (m_active && (m_dirty || force)) {
        int left = m_left + m_pPanel->origin_x;
        int top = m_top + m_pPanel->origin_y;
        int right = m_right + m_pPanel->origin_x;
        int bottom = m_bottom + m_pPanel->origin_y;
        InvalidateRegion(left, top, right, bottom, 0);
        BlitCatalogSurfaceRectTo16BPP(-14, left, top, right, bottom, 0x1b6, 0, 0);
        int y = top + 1;
        SetFont(g_font_683660);
        SetFontObjectPalette16BPP(g_font_683660, g_colour_68ee08);
        SetObjectShade(g_wiz_text_font_secondary_object_683680, 4);
        for (int index = m_first_entry + m_scroll_offset;
             index < m_first_entry + m_scroll_offset + 7; ++index) {
            if (index >= m_first_entry + m_entry_count) break;
            if (!m_entries[index].selectable) {
                SetObjectShade(g_wiz_text_font_secondary_object_683680, 6);
            } else if (m_entries[index].selected) {
                SetFontObjectPalette16BPP(g_font_683660, g_font_state_palettes_68ee1c[3]);
            } else if (index == m_hovered_entry) {
                SetFontObjectPalette16BPP(g_font_683660, g_font_state_palettes_68ee1c[5]);
            }
            mprintf(left + 2, y, L"%s", g_spell_records[m_entries[index].spell].display_name);
            wchar_t cost[6];
            wcscpy(cost, FormatWideString(L"%d",
                g_spell_records[m_entries[index].spell].spell_point_cost));
            mprintf(right - StringPixLength(cost, g_font_683660) - 2, y, L"%s", cost);
            y += 13;
            SetFontObjectPalette16BPP(g_font_683660, g_colour_68ee08);
            SetObjectShade(g_wiz_text_font_secondary_object_683680, 4);
        }
        m_range->Invalidate(0);
        m_dirty = 0;
    }
}

/* Shared with other widget classes at 0x004F58C0. */
void W8CharacterSpellList::OnMouseEnter(int)
{
    PushButtonSoundScheme005587C0(0, 1);
}

// FUNCTION: WIZ8 0x005c8100
void W8CharacterSpellList::OnMouseLeave(int event)
{
    m_hovered_entry = -1;
    Invalidate(static_cast<unsigned char>(event));
}

// FUNCTION: WIZ8 0x005c8120
void W8CharacterSpellList::OnMouseMove(int)
{
    POINT mouse;
    SGPMouseGetPos(&mouse);
    int entry = (mouse.y - m_pPanel->origin_y - m_top) / 13 +
                m_scroll_offset + m_first_entry;
    if (entry > m_first_entry + m_entry_count) entry = -1;
    if (entry != m_hovered_entry) {
        m_hovered_entry = entry;
        Invalidate(0);
    }
}

// FUNCTION: WIZ8 0x005c8190
void W8CharacterSpellList::OnLeftButtonDown(int)
{
    POINT mouse;
    SGPMouseGetPos(&mouse);
    int entry = (mouse.y - m_pPanel->origin_y - m_top) / 13 +
                m_scroll_offset + m_first_entry;
    if (entry >= m_first_entry + m_entry_count) {
        PushButtonSoundScheme005587C0(0, 1);
    }
}

/* Folded with OnLeftButtonDown at 0x005C8190. */
void W8CharacterSpellList::OnRightButtonDown(int)
{
    POINT mouse;
    SGPMouseGetPos(&mouse);
    int entry = (mouse.y - m_pPanel->origin_y - m_top) / 13 +
                m_scroll_offset + m_first_entry;
    if (entry >= m_first_entry + m_entry_count) {
        PushButtonSoundScheme005587C0(0, 1);
    }
}

// FUNCTION: WIZ8 0x005c81f0
void W8CharacterSpellList::OnLeftButtonUp(int event)
{
    POINT mouse;
    SGPMouseGetPos(&mouse);
    int entry = (mouse.y - m_pPanel->origin_y - m_top) / 13 +
                m_scroll_offset + m_first_entry;
    if (entry >= m_first_entry + m_entry_count) {
        PushButtonSoundScheme005587C0(0, 1);
        return;
    }
    if (m_entries[entry].selectable) {
        Invalidate(static_cast<unsigned char>(event));
        if (m_listener) m_listener->SelectSpell(entry);
    }
}

// FUNCTION: WIZ8 0x005c8280
void W8CharacterSpellList::OnRightButtonUp(int)
{
    POINT mouse;
    SGPMouseGetPos(&mouse);
    int entry = (mouse.y - m_pPanel->origin_y - m_top) / 13 +
                m_scroll_offset + m_first_entry;
    if (entry >= m_first_entry + m_entry_count) {
        PushButtonSoundScheme005587C0(0, 1);
        return;
    }
    if (m_listener) m_listener->ShowSpellInfo(entry);
}

// FUNCTION: WIZ8 0x005c82f0
void W8CharacterSpellList::AdjustValue(int steps)
{
    int step;
    for (step = 0; step < steps; ++step) m_range->Decrement();
    for (step = 0; step < -steps; ++step) m_range->Increment();
}

// FUNCTION: WIZ8 0x005c8330
void W8CharacterSpellList::OnRangeChanged(W8RangeControl* range)
{
    m_scroll_offset = range->m_value;
    Invalidate(0);
}

// FUNCTION: WIZ8 0x005c83d0
void W8CharacterPage005EF664::SetCharacter(
    W8Character* character, W8CharacterCreationState* creation_state, int mode)
{
    W8CharacterPage::SetCharacter(character, creation_state, mode);
    AcquireRegionSet(&g_character_spells_region_set_0069c534);
    for (int realm = 0; realm < 6; ++realm) {
        m_realms_074[realm] = new W8CharacterSpellList(
            this, (realm % 2) * 215 + 13, (realm / 2) * 130 + 8,
            m_spell_data_08c, &g_character_spell_list_region_sets_0069c538[realm]);
        m_realms_074[realm]->m_listener = this;
    }
}

// FUNCTION: WIZ8 0x005c8570
void W8CharacterPage005EF664::Deactivate()
{
    EnableRegionSet(0);
    for (int realm = 0; realm < 6; ++realm) {
        m_realms_074[realm]->m_range->EnableRegionSet(0);
    }
}

// FUNCTION: WIZ8 0x005c8770
void W8CharacterPage005EF664::GetNavigationState(
    unsigned char* next_enabled, unsigned char* exit_enabled)
{
    *next_enabled = 1;
    *exit_enabled = m_creation_state_064->spell_points_remaining <
                    m_creation_state_064->spell_points_total;
}

// FUNCTION: WIZ8 0x005c88a0
void W8CharacterPage005EF664::ShowSpellInfo(unsigned int entry)
{
    m_screen_05c->ShowDialog005B0610(m_spell_data_08c[entry].spell);
}
