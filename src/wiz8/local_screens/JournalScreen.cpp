#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/NPCInteractionSubscreen.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/local_code/ControlsRect.h"
#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/xstatus.h"
#include "wiz8/local_screens/JournalScreen.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/local_screens/Screens.h"

#include "wiz8/cursor.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/fonts.h"
#include "wiz8/fact_state.h"
#include "wiz8/local_code/Factions.h"
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
#include "FileMan.h"

#include "input.h"
#include "Types.h"
#include "mousesystem.h"

#include <new>
#include <stdlib.h>
#include "wiz8/local_screens/OptionsScreen.h"

// GLOBAL: WIZ8 0x0064df38
int g_journal_page = -1;
// GLOBAL: WIZ8 0x0069c4e0
bool g_journal_show_all;
// GLOBAL: WIZ8 0x0064d7b8
wchar_t g_default_level[] = L"Default Level";
// GLOBAL: WIZ8 0x0064d7f0
wchar_t g_journal_page_format[] = L"%d / %d";
// GLOBAL: WIZ8 0x0064df40
signed char g_journal_factions[12] = {4, 5, 6, 7, 8, 9, 12, 11, 13, 15, 16, 0};
// GLOBAL: WIZ8 0x0064df4c
int g_journal_faction_name_indices[11] = {
    0x6e0, 0x6e1, 0x6e2, 0x6e3, 0x6e4, 0x6e5, 0x6e6, 0x6e7, 0x6e8, 0x6e9, 0x6ea,
};
// GLOBAL: WIZ8 0x0064df78
wchar_t g_journal_alternate_page[] = L"1 / 1";
// GLOBAL: WIZ8 0x0064df3c
int g_journal_page_count = -1;
// GLOBAL: WIZ8 0x0069C4CC
int g_journal_font;
// GLOBAL: WIZ8 0x0069C4D0
unsigned short* g_journal_font_palette;
// GLOBAL: WIZ8 0x0069C4D8
unsigned short* g_journal_font_original_palette;
// GLOBAL: WIZ8 0x0069C4D4
W8JournalPanel* g_journal_panel;
// GLOBAL: WIZ8 0x0069C4E4
W8GrowableVector<W8JournalEntry>* g_journal_entries;
// GLOBAL: WIZ8 0x0069C4DC
unsigned int g_journal_region_set;
// GLOBAL: WIZ8 0x0068de40
W8GrowableVector<W8JournalEntry>* g_fact_journal_entries;

// GLOBAL: WIZ8 0x0068de44
unsigned char g_fact_notifications_suppressed;

/* Create the fact journal on first use. An existing journal is only emptied,
   which is what a new game does to the entries left by the previous one. */
// FUNCTION: WIZ8 0x00558820
void InitializeFactJournal(void)
{
    if (g_fact_journal_entries == 0) {
        g_fact_journal_entries = new W8GrowableVector<W8JournalEntry>();
    } else {
        g_fact_journal_entries->Clear();
    }
}

// FUNCTION: WIZ8 0x005588e0
void SetFactNotificationsSuppressed(unsigned char suppressed)
{
    g_fact_notifications_suppressed = suppressed;
}

/* Append one changed fact to the journal and, when notices are not suppressed
   and the fact is visible at the current notice setting, post the fact's own
   description through the notice pane with its sound. */
// FUNCTION: WIZ8 0x005588f0
void RecordFactChangeForJournal(int fact_id)
{
    if (g_fact_journal_entries == 0) {
        InitializeFactJournal();
    }
    W8JournalEntry entry;
    entry.level = g_status.current_level;
    entry.fact = fact_id;
    entry.alternate_text = GetFact(fact_id);
    g_fact_journal_entries->Add(entry);

    if (g_fact_notifications_suppressed != 0) {
        return;
    }
    int visibility;
    if (g_settings.difficulty == W8_DIFFICULTY_NOVICE) {
        visibility = 2;
    } else if (g_settings.difficulty == W8_DIFFICULTY_NORMAL) {
        visibility = 1;
    } else {
        visibility = fact_id;
        if (g_settings.difficulty == W8_DIFFICULTY_EXPERT) {
            visibility = 0;
        }
    }
    const W8FactDatabaseRecord* record = &g_fact_records[fact_id];
    if (record->visibility_037 > visibility) {
        return;
    }
    const wchar_t* description =
        GetFact(fact_id) ? record->alternate_description_038 : record->description_100;
    if (*description == 0 || g_level_block == 0) {
        return;
    }
    if (gXStatus.fNpcDialogueMode != 0) {
        QueueNpcMessageLine(W8_NPC_MSG_JOURNAL_QUOTE, 0);
        return;
    }
    int range = GetTextBoxScrollRange();
    ShowNotice(3, gppStringList[0x1d28 / 4], 2, range, 0);
    SoundPlay("Data\\Sound\\Misc\\Journal Entry.wav", 0);
}

/* Write the fact journal into the open JRNL chunk: the entry count, a format
   dword, then each 0x0c-byte entry. */
// FUNCTION: WIZ8 0x00558A90
void SaveFactJournal(int file)
{
    int format = 1;
    int count;
    int index;

    if (g_fact_journal_entries == 0) {
        InitializeFactJournal();
    }
    count = g_fact_journal_entries->count;
    FileWrite(file, &format, 4, 0);
    FileWrite(file, &count, 4, 0);
    for (index = 0; index < count; ++index) {
        FileWrite(file, g_fact_journal_entries->GetAt(index), sizeof(W8JournalEntry), 0);
    }
}

/* Read the fact journal back from the open JRNL chunk: a format dword, the
   entry count, then each 0x0c-byte entry straight into the vector. The vector
   grows to the serialized count first; a failed grow leaves the count
   unstored, the same outcome a failed load leaves behind. */
// FUNCTION: WIZ8 0x00558B20
void LoadJournalEntries(unsigned int file)
{
    int format;
    int count;
    int index;

    InitializeFactJournal();
    FileRead(file, &format, 4, 0);
    FileRead(file, &count, 4, 0);
    if (g_fact_journal_entries->Grow(count) != 0) {
        g_fact_journal_entries->count = count;
    }
    for (index = 0; index < count; ++index) {
        FileRead(file, g_fact_journal_entries->GetAt(index), sizeof(W8JournalEntry), 0);
    }
}

// FUNCTION: WIZ8 0x005bdd00
void DrawJournalLine(const wchar_t* text, int column, int y, int palette, char centered)
{
    int left;
    int right;
    int x;

    if (column == 0) {
        left = 0x0b;
        right = 0xaf;
    } else {
        left = 0xc1;
        right = 0x1b3;
    }
    if (!centered) {
        x = 5;
    } else {
        x = (right - StringPixLength(const_cast<wchar_t*>(text), g_journal_font)) / 2;
        if (x < 0) {
            x = 0;
        }
    }
    SetFont(g_journal_font);
    SetFontDestBuffer(-14, left, y, left + right, y + 0x1e, 0);
    if (palette == 0) {
        SetFontObjectPalette16BPP(g_journal_font, g_journal_font_original_palette);
    } else if (palette == 1) {
        SetFontObjectPalette16BPP(g_journal_font, g_journal_font_palette);
    }
    gprintf(left + x, y, L"%s", text);
}

// FUNCTION: WIZ8 0x005bd860
void RefreshJournalPanel(void)
{
    W8JournalPanel* panel = g_journal_panel;

    if (!panel->m_alternate_mode_064) {
        int last_page = (g_journal_entries->count - 1) / 12;
        int page_count = last_page + 1;
        if (page_count != g_journal_page_count || g_journal_page < 0 ||
            g_journal_page >= page_count) {
            g_journal_page_count = page_count;
            g_journal_page = last_page;
        }
        panel->m_next_050->SetEnabled(g_journal_page < last_page);
        panel->m_previous_054->SetEnabled(g_journal_page > 0);

        wchar_t page_text[20];
        swprintf(page_text, g_journal_page_format, g_journal_page + 1, page_count);
        panel->m_page_text_060->SetText(page_text, g_options_detail_font_683614);
        DrawCatalogImageAndInvalidate(-14, 0x1b8, 0, 0, 0, 0, 2, 0);
        DrawJournalLine(gppStringList[0x1b6c / 4], 0, 0x19, 0, 1);
        DrawJournalLine(gppStringList[0x1b70 / 4], 1, 0x19, 0, 1);

        int first = g_journal_page * 12;
        int last = first + 11;
        if (last >= g_journal_entries->count) {
            last = g_journal_entries->count - 1;
        }
        int previous_level = -1;
        int y = 0x39;
        for (int index = first; index <= last; ++index, y += 0x1e) {
            const W8JournalEntry* entry = g_journal_entries->GetAt(index);
            const W8FactDatabaseRecord* fact = &g_fact_records[entry->fact];
            int active = fact->highlight_when_true_036 && GetFact(entry->fact);
            const wchar_t* description =
                entry->alternate_text ? fact->alternate_description_038 : fact->description_100;
            const wchar_t* level_name;
            if (entry->level == 0x38) {
                level_name = g_default_level;
            } else if (entry->level == -1) {
                level_name = gppStringList[0x1b74 / 4];
            } else {
                level_name = gppStringList[g_level_name_indices[entry->level]];
            }
            if (entry->level != previous_level) {
                DrawJournalLine(level_name, 0, y, 0, 1);
            }
            DrawJournalLine(description, 1, y, active, 0);
            previous_level = entry->level;
        }
    } else {
        panel->m_next_050->SetEnabled(0);
        panel->m_previous_054->SetEnabled(0);
        panel->m_page_text_060->SetText(g_journal_alternate_page, g_options_detail_font_683614);
        DrawCatalogImageAndInvalidate(-14, 0x1b8, 0, 0, 0, 0, 2, 0);
        DrawJournalLine(gppStringList[0x1b78 / 4], 0, 0x19, 0, 1);
        DrawJournalLine(gppStringList[0x1b7c / 4], 1, 0x19, 0, 1);

        int y = 0x39;
        for (int index = 0; index < 11; ++index) {
            signed char faction = g_journal_factions[index];
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
                    DrawJournalLine(disposition_name, 0, y, 0, 1);
                }
                DrawJournalLine(gppStringList[g_journal_faction_name_indices[index]], 1, y, 0, 0);
                y += 0x1e;
            }
        }
    }
    panel->Invalidate(0);
}

// VTABLE: WIZ8 0x005ef340 Controls
// VTABLE: WIZ8 0x005ef338 W8TextControl::Listener
// class W8JournalPanel

// FUNCTION: WIZ8 0x005bd530
W8JournalPanel::W8JournalPanel(unsigned int* region_set)
    : Controls(0x66, 0x1bb, 0, 0, 0xf3, 0, 0), m_next_050(0), m_previous_054(0), m_close_058(0),
      m_mode_05c(0), m_page_text_060(0), m_alternate_mode_064(0)
{
    short width;
    short height;

    AcquireRegionSet(region_set);
    GetCatalogImageSize(0xf3, 0, 0, &width, &height);
    right = origin_x + static_cast<unsigned short>(width);
    bottom = origin_y + static_cast<unsigned short>(height);

    m_previous_054 = new W8TextControl(this, 0xffffffff, 3, 3, 0, 0, 0xf4, 0, 0, 2, 1, -1, 3);
    m_previous_054->m_listener = this;

    m_next_050 = new W8TextControl(this, 0xffffffff, 0x11f, 3, 0, 0, 0xf4, 0, 4, 6, 5, -1, 7);
    m_next_050->m_listener = this;

    W8ControlsRect bounds = {origin_x, origin_y, right, bottom};
    m_page_text_060 =
        new W8TextBuffer(&bounds, &g_wchar_00689b34, g_options_detail_font_683614,
                         g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C, 4);

    m_mode_05c = new W8TextControl(this, 0xffffffff, 0x1b0, -2, 0, 0, 0x1bb, 0, 0, 2, 1, 2, 3);
    m_mode_05c->AddLayoutFlags(g_W8TextControlMask005ED578);
    m_mode_05c->m_listener = this;

    m_close_058 =
        new W8TextControl(this, 0xffffffff, 0x1ea, -2, 0, 0, 0x106, 0, 0x10, -1, 0x11, 0x12, 0x13);
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
W8JournalPanel::~W8JournalPanel()
{
    DestroyAllControls();
    delete m_page_text_060;
}

// FUNCTION: WIZ8 0x005bdbf0
void W8JournalPanel::Redraw()
{
    if (m_fEnabled && (m_fDirty || m_fLayoutDirty)) {
        Controls::Redraw();
        m_page_text_060->RenderToTarget(0, 1, -14);
    }
}

// FUNCTION: WIZ8 0x005bdc20
void W8JournalPanel::OnPrimary(W8TextControl* control)
{
    if (control == m_previous_054) {
        if (g_journal_page > 0) {
            --g_journal_page;
            RefreshJournalPanel();
        }
    } else if (control == m_next_050) {
        if (g_journal_page < (g_journal_entries->count - 1) / 12) {
            ++g_journal_page;
            RefreshJournalPanel();
        }
    } else if (control == m_close_058) {
        RequestScreenTransition();
    } else {
        m_alternate_mode_064 =
            static_cast<unsigned char>(m_mode_05c->m_stateFlags & g_W8TextControlMask005ED570);
        if (m_alternate_mode_064) {
            m_mode_05c->EnableRegionHelp(0x6ec);
            if ((m_mode_05c->m_stateFlags & g_W8TextControlMask005ED570) == 0) {
                m_mode_05c->ActivatePrimary(1);
            }
        } else {
            m_mode_05c->EnableRegionHelp(0x6eb);
            if ((m_mode_05c->m_stateFlags & g_W8TextControlMask005ED570) != 0) {
                m_mode_05c->ActivateSecondary(1);
            }
        }
        RefreshJournalPanel();
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
    g_journal_font =
        LoadFontFile(reinterpret_cast<UINT8*>( // reinterpret-ok: SGP API declared UINT8* for text
            const_cast<char*>("Data\\Journal\\journal_font.sti")));
    g_journal_font_original_palette = GetFontObjectPalette16BPP(g_journal_font);
    g_journal_font_palette = CopyCatalogImagePalette16BPP(0x1b9, 0);
    return 1;
}

// FUNCTION: WIZ8 0x005bde10
unsigned char JournalScreenFinalize(void)
{
    SetFontObjectPalette16BPP(g_journal_font, g_journal_font_original_palette);
    free(g_journal_font_palette);
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
    g_journal_entries = new W8GrowableVector<W8JournalEntry>();
    g_journal_panel = new W8JournalPanel(&g_journal_region_set);

    switch (g_settings.difficulty) {
    case W8_DIFFICULTY_NOVICE:
        maximum_visibility = 2;
        break;
    case W8_DIFFICULTY_NORMAL:
        maximum_visibility = 1;
        break;
    case W8_DIFFICULTY_EXPERT:
        maximum_visibility = 0;
        break;
    }

    g_journal_entries->Clear();
    for (index = 0; index < g_fact_journal_entries->count; ++index) {
        W8JournalEntry entry = *g_fact_journal_entries->GetAt(index);
        const W8FactDatabaseRecord* fact = &g_fact_records[entry.fact];
        const wchar_t* description =
            entry.alternate_text ? fact->alternate_description_038 : fact->description_100;
        if ((g_journal_show_all || fact->visibility_037 <= maximum_visibility) &&
            *description != 0) {
            g_journal_entries->Add(entry);
        }
    }
    RefreshJournalPanel();
    DrawCatalogImageAndInvalidate(-14, 0x1b7, 0, 1, 0, 0x1b4, 2, 0);
    return 1;
}

// FUNCTION: WIZ8 0x005be0b0
unsigned char JournalScreenLeave(int)
{
    MSYS_Shutdown();
    ResetRegions();
    delete g_journal_panel;
    g_journal_panel = 0;
    g_journal_entries->Clear();
    delete g_journal_entries;
    g_journal_entries = 0;
    if (gXStatus.fCampMode) {
        SyncDialogueNpcState00577260();
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
            } else if (input.usParam == 0x25 && g_journal_page > 0) {
                --g_journal_page;
                RefreshJournalPanel();
            } else if (input.usParam == 0x27 &&
                       g_journal_page < (g_journal_entries->count - 1) / 12) {
                ++g_journal_page;
                RefreshJournalPanel();
            }
        }
    }
    g_journal_panel->Redraw();
    RenderFrame();
}
