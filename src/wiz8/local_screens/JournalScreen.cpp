#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/targeting.h"
#include "wiz8/render_state.h"
#include "wiz8/local_code/ControlsRect.h"
#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/local_screens/JournalScreen.h"
#include "wiz8/xstatus.h"

#include "wiz8/cursor.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/fact_state.h"
#include "wiz8/factions.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/notices.h"
#include "wiz8/regions.h"
#include "wiz8/utility.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/vector.h"

#include "Font.h"
#include "soundman.h"

#include "input.h"
#include "Types.h"
#include "mousesystem.h"

#include <new>
#include <stdlib.h>

extern const wchar_t g_wchar_00689b34;
extern int g_font_00683614;
extern int g_journal_page_0064df38;
// GLOBAL: WIZ8 0x0064df38
int g_journal_page_0064df38 = -1;
extern W8GrowableVector<W8JournalEntry>* g_journal_entries_0069c4e4;
extern unsigned char g_journal_show_all_0069c4e0;
// GLOBAL: WIZ8 0x0069c4e0
unsigned char g_journal_show_all_0069c4e0;
// GLOBAL: WIZ8 0x006850d5
int g_value_006850d5;
// GLOBAL: WIZ8 0x0064d7b8
wchar_t g_default_level_0064d7b8[] = L"Default Level";
// GLOBAL: WIZ8 0x0064d7f0
wchar_t g_journal_page_format_0064d7f0[] = L"%d / %d";
// GLOBAL: WIZ8 0x0064df40
signed char g_journal_factions_0064df40[12] = {4, 5, 6, 7, 8, 9, 12, 11, 13, 15, 16, 0};
// GLOBAL: WIZ8 0x0064df4c
int g_journal_faction_name_indices_0064df4c[11] = {
    0x6e0, 0x6e1, 0x6e2, 0x6e3, 0x6e4, 0x6e5, 0x6e6, 0x6e7, 0x6e8, 0x6e9, 0x6ea,
};
// GLOBAL: WIZ8 0x0064df78
wchar_t g_journal_alternate_page_0064df78[] = L"1 / 1";
extern int g_journal_page_count_0064df3c;
// GLOBAL: WIZ8 0x0064df3c
int g_journal_page_count_0064df3c = -1;
// GLOBAL: WIZ8 0x0069C4CC
int g_journal_font_69c4cc;
// GLOBAL: WIZ8 0x0069C4D0
unsigned short* g_journal_font_palette_69c4d0;
// GLOBAL: WIZ8 0x0069C4D8
unsigned short* g_journal_font_original_palette_69c4d8;
W8JournalPanel005EF340* g_journal_panel_0069c4d4;
W8GrowableVector<W8JournalEntry>* g_journal_entries_0069c4e4;
unsigned int g_journal_region_set_0069c4dc;
// GLOBAL: WIZ8 0x0068de40
W8GrowableVector<W8JournalEntry>* g_fact_journal_entries_0068de40;

/* Create the fact journal on first use. An existing journal is only emptied,
   which is what a new game does to the entries left by the previous one. */
// FUNCTION: WIZ8 0x00558820
void InitializeFactJournal(void)
{
    if (g_fact_journal_entries_0068de40 == 0) {
        g_fact_journal_entries_0068de40 = new W8GrowableVector<W8JournalEntry>();
    }
    else {
        g_fact_journal_entries_0068de40->count = 0;
    }
}

/* Append one changed fact to the journal and, when notices are not suppressed
   and the fact is visible at the current notice setting, post the fact's own
   description through the notice pane with its sound. */
// FUNCTION: WIZ8 0x005588f0
void RecordFactChangeForJournal(int fact_id)
{
    if (g_fact_journal_entries_0068de40 == 0) {
        InitializeFactJournal();
    }
    W8JournalEntry entry;
    entry.level = g_status_685170.current_level;
    entry.fact = fact_id;
    entry.alternate_text = GetFact(fact_id);
    g_fact_journal_entries_0068de40->Add(entry);

    if (g_fact_notifications_suppressed != 0) {
        return;
    }
    int visibility;
    if (g_value_006850d5 == 0) {
        visibility = 2;
    }
    else if (g_value_006850d5 == 1) {
        visibility = 1;
    }
    else {
        visibility = fact_id;
        if (g_value_006850d5 == 2) {
            visibility = 0;
        }
    }
    const W8FactDatabaseRecord* record = &g_fact_records[fact_id];
    if (record->visibility_037 > visibility) {
        return;
    }
    const W8WideChar* description = GetFact(fact_id)
        ? record->alternate_description_038
        : record->description_100;
    if (*description == 0 || g_level_block == 0) {
        return;
    }
    if (gXStatus.field_01f != 0) {
        Function5289B0(8, 0);
        return;
    }
    int range = GetTextBoxScrollRange();
    ShowNotice(3, gppStringList[0x1d28 / 4], 2, range, 0);
    SoundPlay("Data\\Sound\\Misc\\Journal Entry.wav", 0);
}

// FUNCTION: WIZ8 0x005bdd00
void DrawJournalLine005BDD00(
    const wchar_t* text, int column, int y, int palette, char centered)
{
    int left;
    int right;
    int x;

    if (column == 0) {
        left = 0x0b;
        right = 0xaf;
    }
    else {
        left = 0xc1;
        right = 0x1b3;
    }
    if (!centered) {
        x = 5;
    }
    else {
        x = (right - StringPixLength(
                         const_cast<unsigned short*>(text),
                         g_journal_font_69c4cc)) / 2;
        if (x < 0) {
            x = 0;
        }
    }
    SetFont(g_journal_font_69c4cc);
    SetFontDestBuffer(-14, left, y, left + right, y + 0x1e, 0);
    if (palette == 0) {
        SetFontObjectPalette16BPP(
            g_journal_font_69c4cc, g_journal_font_original_palette_69c4d8);
    }
    else if (palette == 1) {
        SetFontObjectPalette16BPP(
            g_journal_font_69c4cc, g_journal_font_palette_69c4d0);
    }
    gprintf(left + x, y, (unsigned short*)L"%s", text);
}

// FUNCTION: WIZ8 0x005bd860
void RefreshJournalPanel005BD860(void)
{
    W8JournalPanel005EF340* panel = g_journal_panel_0069c4d4;

    if (!panel->m_alternate_mode_064) {
        int last_page = (g_journal_entries_0069c4e4->count - 1) / 12;
        int page_count = last_page + 1;
        if (page_count != g_journal_page_count_0064df3c ||
            g_journal_page_0064df38 < 0 ||
            g_journal_page_0064df38 >= page_count) {
            g_journal_page_count_0064df3c = page_count;
            g_journal_page_0064df38 = last_page;
        }
        panel->m_next_050->SetEnabled(g_journal_page_0064df38 < last_page);
        panel->m_previous_054->SetEnabled(g_journal_page_0064df38 > 0);

        wchar_t page_text[20];
        swprintf(page_text, g_journal_page_format_0064d7f0,
                 g_journal_page_0064df38 + 1, page_count);
        panel->m_page_text_060->SetText(page_text, g_font_00683614);
        DrawCatalogImageAndInvalidate(-14, 0x1b8, 0, 0, 0, 0, 2, 0);
        DrawJournalLine005BDD00(gppStringList[0x1b6c / 4], 0, 0x19, 0, 1);
        DrawJournalLine005BDD00(gppStringList[0x1b70 / 4], 1, 0x19, 0, 1);

        int first = g_journal_page_0064df38 * 12;
        int last = first + 11;
        if (last >= g_journal_entries_0069c4e4->count) {
            last = g_journal_entries_0069c4e4->count - 1;
        }
        int previous_level = -1;
        int y = 0x39;
        for (int index = first; index <= last; ++index, y += 0x1e) {
            const W8JournalEntry* entry = g_journal_entries_0069c4e4->GetAt(index);
            const W8FactDatabaseRecord* fact = &g_fact_records[entry->fact];
            int active = fact->highlight_when_true_036 && GetFact(entry->fact);
            const wchar_t* description = entry->alternate_text
                ? fact->alternate_description_038
                : fact->description_100;
            const wchar_t* level_name;
            if (entry->level == 0x38) {
                level_name = g_default_level_0064d7b8;
            }
            else if (entry->level == -1) {
                level_name = gppStringList[0x1b74 / 4];
            }
            else {
                level_name = gppStringList[g_level_name_indices_605820[entry->level]];
            }
            if (entry->level != previous_level) {
                DrawJournalLine005BDD00(level_name, 0, y, 0, 1);
            }
            DrawJournalLine005BDD00(description, 1, y, active, 0);
            previous_level = entry->level;
        }
    }
    else {
        panel->m_next_050->SetEnabled(0);
        panel->m_previous_054->SetEnabled(0);
        panel->m_page_text_060->SetText(
            g_journal_alternate_page_0064df78, g_font_00683614);
        DrawCatalogImageAndInvalidate(-14, 0x1b8, 0, 0, 0, 0, 2, 0);
        DrawJournalLine005BDD00(gppStringList[0x1b78 / 4], 0, 0x19, 0, 1);
        DrawJournalLine005BDD00(gppStringList[0x1b7c / 4], 1, 0x19, 0, 1);

        int y = 0x39;
        for (int index = 0; index < 11; ++index) {
            signed char faction = g_journal_factions_0064df40[index];
            if (GetFactionFlag(faction)) {
                W8FactionDisposition disposition = GetFactionDisposition(faction);
                const wchar_t* disposition_name = 0;
                switch (disposition) {
                case W8_FACTION_HOSTILE:
                    disposition_name = gppStringList[0x800 / 4];
                    break;
                case W8_FACTION_NEUTRAL:
                    disposition_name = gppStringList[0x7fc / 4];
                    break;
                case W8_FACTION_FRIENDLY:
                    disposition_name = gppStringList[0x804 / 4];
                    break;
                }
                if (disposition_name != 0) {
                    DrawJournalLine005BDD00(disposition_name, 0, y, 0, 1);
                }
                DrawJournalLine005BDD00(
                    gppStringList[g_journal_faction_name_indices_0064df4c[index]],
                    1, y, 0, 0);
                y += 0x1e;
            }
        }
    }
    panel->Invalidate(0);
}

// VTABLE: WIZ8 0x005ef340 Controls
// VTABLE: WIZ8 0x005ef338 W8TextControl::Listener
// class W8JournalPanel005EF340

// FUNCTION: WIZ8 0x005bd530
W8JournalPanel005EF340::W8JournalPanel005EF340(unsigned int* region_set)
    : Controls(0x66, 0x1bb, 0, 0, 0xf3, 0, 0),
      m_next_050(0),
      m_previous_054(0),
      m_close_058(0),
      m_mode_05c(0),
      m_page_text_060(0),
      m_alternate_mode_064(0)
{
    short width;
    short height;

    AcquireRegionSet(region_set);
    GetCatalogImageSize(0xf3, 0, 0, &width, &height);
    right = origin_x + static_cast<unsigned short>(width);
    bottom = origin_y + static_cast<unsigned short>(height);

    m_previous_054 = new W8TextControl(
        this, 0xffffffff, 3, 3, 0, 0, 0xf4, 0, 0, 2, 1, -1, 3);
    m_previous_054->m_listener = this;

    m_next_050 = new W8TextControl(
        this, 0xffffffff, 0x11f, 3, 0, 0, 0xf4, 0, 4, 6, 5, -1, 7);
    m_next_050->m_listener = this;

    W8ControlsRect bounds = { origin_x, origin_y, right, bottom };
    m_page_text_060 = new W8TextBuffer(
        &bounds, &g_wchar_00689b34, g_font_00683614,
        g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C,
        4);

    m_mode_05c = new W8TextControl(
        this, 0xffffffff, 0x1b0, -2, 0, 0, 0x1bb, 0, 0, 2, 1, 2, 3);
    m_mode_05c->AddLayoutFlags(g_W8TextControlMask005ED578);
    m_mode_05c->m_listener = this;

    m_close_058 = new W8TextControl(
        this, 0xffffffff, 0x1ea, -2, 0, 0, 0x106, 0, 0x10, -1, 0x11, 0x12, 0x13);
    m_close_058->m_listener = this;
    m_close_058->EnableRegionHelp(0x6ed);
    SetEnabled(1);
    EnableRegionSet(1);
    m_alternate_mode_064 = 0;
    m_mode_05c->EnableRegionHelp(0x6eb);
    if ((m_mode_05c->m_stateFlags & g_W8TextControlMask005ED570) != 0) {
        m_mode_05c->ActivateSecondary(1);
    }
}

// FUNCTION: WIZ8 0x005bd7f0
W8JournalPanel005EF340::~W8JournalPanel005EF340()
{
    DestroyAllControls();
    delete m_page_text_060;
}

// FUNCTION: WIZ8 0x005bdbf0
void W8JournalPanel005EF340::Redraw()
{
    if (m_fEnabled && (m_fDirty || m_fLayoutDirty)) {
        Controls::Redraw();
        m_page_text_060->RenderToTarget(0, 1, -14);
    }
}

// FUNCTION: WIZ8 0x005bdc20
void W8JournalPanel005EF340::OnPrimary(W8TextControl* control)
{
    if (control == m_previous_054) {
        if (g_journal_page_0064df38 > 0) {
            --g_journal_page_0064df38;
            RefreshJournalPanel005BD860();
        }
    }
    else if (control == m_next_050) {
        if (g_journal_page_0064df38 <
            (g_journal_entries_0069c4e4->count - 1) / 12) {
            ++g_journal_page_0064df38;
            RefreshJournalPanel005BD860();
        }
    }
    else if (control == m_close_058) {
        RequestScreenTransition();
    }
    else {
        m_alternate_mode_064 = static_cast<unsigned char>(
            m_mode_05c->m_stateFlags & g_W8TextControlMask005ED570);
        if (m_alternate_mode_064) {
            m_mode_05c->EnableRegionHelp(0x6ec);
            if ((m_mode_05c->m_stateFlags & g_W8TextControlMask005ED570) == 0) {
                m_mode_05c->ActivatePrimary(1);
            }
        }
        else {
            m_mode_05c->EnableRegionHelp(0x6eb);
            if ((m_mode_05c->m_stateFlags & g_W8TextControlMask005ED570) != 0) {
                m_mode_05c->ActivateSecondary(1);
            }
        }
        RefreshJournalPanel005BD860();
    }
}

/* Lifecycle record 11's initializer and finalizer. The initializer loads the
   journal font, saves the palette the font arrived with, and takes a second
   palette from frame 0 of video object 0x1B9; the finalizer puts the original
   palette back and releases the one it took. The two together are the second
   proof that a record's fifth slot is its finalizer rather than a second
   initializer - record 10's allocate/free pair is the first. */
// FUNCTION: WIZ8 0x005bddd0
unsigned char JournalScreenInitialize(void)
{
    g_journal_font_69c4cc = LoadFontFile((UINT8*)"Data\\Journal\\journal_font.sti");
    g_journal_font_original_palette_69c4d8 = GetFontObjectPalette16BPP(g_journal_font_69c4cc);
    g_journal_font_palette_69c4d0 = CopyCatalogImagePalette16BPP(0x1b9, 0);
    return 1;
}

// FUNCTION: WIZ8 0x005bde10
unsigned char JournalScreenFinalize(void)
{
    SetFontObjectPalette16BPP(g_journal_font_69c4cc, g_journal_font_original_palette_69c4d8);
    free(g_journal_font_palette_69c4d0);
    return 1;
}

// FUNCTION: WIZ8 0x005bde40
unsigned char JournalScreenEnter(void)
{
    int maximum_visibility;
    int index;

    SetViewport(0, 0, 0x280, 0x1e0);
    SetPrimarySurfaceTextureHint2Enabled(0);
    MSYS_Init();
    ResetRegions();
    UpdateHeldItemCursor();
    g_journal_entries_0069c4e4 = new W8GrowableVector<W8JournalEntry>();
    g_journal_panel_0069c4d4 =
        new W8JournalPanel005EF340(&g_journal_region_set_0069c4dc);

    switch (g_value_006850d5) {
    case 0:
        maximum_visibility = 2;
        break;
    case 1:
        maximum_visibility = 1;
        break;
    case 2:
        maximum_visibility = 0;
        break;
    }

    g_journal_entries_0069c4e4->count = 0;
    for (index = 0; index < g_fact_journal_entries_0068de40->count; ++index) {
        W8JournalEntry entry = *g_fact_journal_entries_0068de40->GetAt(index);
        const W8FactDatabaseRecord* fact = &g_fact_records[entry.fact];
        const W8WideChar* description = entry.alternate_text
            ? fact->alternate_description_038
            : fact->description_100;
        if ((g_journal_show_all_0069c4e0 || fact->visibility_037 <= maximum_visibility) &&
            *description != 0) {
            g_journal_entries_0069c4e4->Add(entry);
        }
    }
    RefreshJournalPanel005BD860();
    DrawCatalogImageAndInvalidate(-14, 0x1b7, 0, 1, 0, 0x1b4, 2, 0);
    return 1;
}

// FUNCTION: WIZ8 0x005be0b0
unsigned char JournalScreenLeave(int)
{
    MSYS_Shutdown();
    ResetRegions();
    delete g_journal_panel_0069c4d4;
    g_journal_panel_0069c4d4 = 0;
    g_journal_entries_0069c4e4->count = 0;
    delete g_journal_entries_0069c4e4;
    g_journal_entries_0069c4e4 = 0;
    if (gXStatus.fCampMode) {
        Function577260();
    }
    return 1;
}

// FUNCTION: WIZ8 0x005be110
void JournalScreenFrame(void)
{
    POINT point;
    InputAtom input;

    SGPMouseGetPos(&point);
    UpdateRegionMousePosition(point.x, point.y);
    while (DequeueEvent(&input) == 1) {
        if (!DispatchRegionInput(&input) && input.usEvent == KEY_DOWN) {
            if (input.usParam == 0x1b) {
                RequestScreenTransition();
            }
            else if (input.usParam == 0x25 && g_journal_page_0064df38 > 0) {
                --g_journal_page_0064df38;
                RefreshJournalPanel005BD860();
            }
            else if (input.usParam == 0x27 &&
                     g_journal_page_0064df38 <
                         (g_journal_entries_0069c4e4->count - 1) / 12) {
                ++g_journal_page_0064df38;
                RefreshJournalPanel005BD860();
            }
        }
    }
    g_journal_panel_0069c4d4->Redraw();
    RenderFrame();
}
