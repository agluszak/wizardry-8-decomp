#include "Types.h"
#include "mousesystem.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/LoadSaveGame.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/local_code/ControlsRect.h"
#include "input.h"
#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/local_code/Widget.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/local_code/RangeControl.h"
#include "wiz8/local_code/ControlSelection.h"
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/local_code/Sight.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/layouts/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/local_code/PartyImport.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/xstatus.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/cursor.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/local_screens/IntroScreen.h"
#include "wiz8/dialog_code/MessageDialogBase.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/geometry.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/local_code/GameplayInit.h"
#include "wiz8/local_code/PartyImport.h"
#include "wiz8/local_screens/PartySelectionScreen.h"
#include "wiz8/local_code/PartyImport.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/music_playlist.h"
#include "wiz8/regions.h"
#include "wiz8/fonts.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/text_input.h"
#include "wiz8/vector.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/virtual_file.h"
#include "wiz8/utility.h"
#include "FileMan.h"
#include "Font.h"
#include "vsurface.h"

#include <stdio.h>
#include <string.h>

/* Party-selection and imported-character UI. The original translation-unit
   spelling is not established; this descriptive name is provisional. */

void MSYS_SGP_Mouse_Handler_Hook(unsigned short event, unsigned short x, unsigned short y,
                                 char right_button, char left_button);

/* Two ordinary growable vectors and the scroll origin account for all 0x24
   bytes allocated at party-selection entry. The second vector supplies the names this
   control renders; the first owns the corresponding party records. */
struct W8PartySelectionCharacterCollection {
    W8PartySelectionCharacterCollection() : characters(5), names(), first_visible(0) {}
    ~W8PartySelectionCharacterCollection();
    W8Character* GetCharacter(int index);
    int FindPartySlot(int index);
    void DetachFromParty(int index);
    void DeleteAt(int index);
    void ClearCharacters();
    void ReloadCharacters();
    void LoadExternalCharacters();
    void SortCharactersByWriteTime();

    /* W8Vector's own vtable 0x5EF4F0 is stored over the base's 0x5EF360 after
       the 0x5C37B0 base-ctor emission inside PartySelectionScreenEnter. */
    W8Vector<W8Character*> characters;
    W8GrowableVector<char*> names;
    int first_visible;
};
W8_ABI_ASSERT(sizeof(W8PartySelectionCharacterCollection) == 0x24,
              "W8PartySelectionCharacterCollection_size");

// GLOBAL: WIZ8 0x0069C4EC
W8PartySelectionCharacterCollection* g_party_selection_character_collection;

// GLOBAL: WIZ8 0x0069C4F0
unsigned int g_party_selection_character_region_set;

// GLOBAL: WIZ8 0x0069C4F4
unsigned int g_party_selection_party_slot_region_set;

// GLOBAL: WIZ8 0x0069C4F8
unsigned int g_party_selection_character_grid_region_set;

/* Imported characters not installed in the active party are owned here. Name
   strings are separately owned by the second vector. Clearing the counts
   before the two ordinary vector destructors preserves their storage teardown
   without asking the vector template to own its pointer elements. */
// FUNCTION: WIZ8 0x005be270
W8PartySelectionCharacterCollection::~W8PartySelectionCharacterCollection()
{
    int index;
    ClearCharacters();
    for (index = 0; index < names.GetCount(); ++index) {
        delete names[index];
    }
    names.Clear();
}

void W8PartySelectionCharacterCollection::ClearCharacters()
{
    int index;
    for (index = 0; index < characters.GetCount(); ++index) {
        W8Character* character = GetCharacter(index);
        if (!character->fInParty) {
            delete character;
        }
    }
    characters.Clear();
}

void W8PartySelectionCharacterCollection::ReloadCharacters()
{
    ClearCharacters();
    W8PartySlotRow* rows = g_status.buffers.XChar;
    for (int slot = 2; slot < 8; ++slot) {
        if (rows[slot].fOccupied) {
            characters.Add(&g_status.buffers.Char[slot]);
        }
    }
    LoadExternalCharacters();
    SortCharactersByWriteTime();
}

// FUNCTION: WIZ8 0x005be4b0
W8Character* W8PartySelectionCharacterCollection::GetCharacter(int index)
{
    if (index >= 0 && index < characters.GetCount()) {
        return characters[index];
    }
    return 0;
}

// FUNCTION: WIZ8 0x005be4d0
void W8PartySelectionCharacterCollection::DetachFromParty(int index)
{
    W8Character* previous = GetCharacter(index);
    int slot = FindPartySlot(index);
    W8Character* replacement = new W8Character;
    char path[128];
    BuildCharacterPath(path, previous->name, -1);
    if (!LoadCharacter(path, replacement, -1, false)) {
        memcpy(replacement, previous, sizeof(W8Character));
    }
    RemoveCharacterFromParty(slot + 2, false);
    replacement->fInParty = false;
    characters.SetAt(index, replacement);
}

// FUNCTION: WIZ8 0x005be5f0
int W8PartySelectionCharacterCollection::FindPartySlot(int index)
{
    W8Character* character = GetCharacter(index);
    if (character && character->fInParty) {
        W8PartySlotRow* rows = g_status.buffers.XChar;
        for (int slot = 0; slot < 6; ++slot) {
            if (rows[slot + 2].fOccupied && &g_status.buffers.Char[slot + 2] == character) {
                return slot;
            }
        }
    }
    return -1;
}

// FUNCTION: WIZ8 0x005c34d0
void W8PartySelectionCharacterCollection::DeleteAt(int index)
{
    characters.RemoveAtAndDelete(index);
}

/* Enumerate loose CHR files, loading only characters that are not already in
   an occupied party slot.  The collection owns every record accepted here;
   records rejected by loading or duplicate-name detection are destroyed
   immediately. */
// FUNCTION: WIZ8 0x005be340
void W8PartySelectionCharacterCollection::LoadExternalCharacters()
{
    char search_path[128];
    GETFILESTRUCT find;

    BuildCharacterFilePath(search_path, FormatString("*.%s", "CHR", -1), -1);
    BOOLEAN found = GetFileFirst(search_path, &find);
    for (;;) {
        if (!found) {
            return;
        }
        W8Character* character = new W8Character;
        if (!LoadCharacter(find.zFileName, character, -1, false)) {
            delete character;
        } else {
            int slot;
            for (slot = 2; slot < 8; ++slot) {
                if (g_status.buffers.XChar[slot].fOccupied &&
                    wcscmp(g_status.buffers.Char[slot].name, character->name) == 0) {
                    break;
                }
            }
            if (slot < 8) {
                delete character;
            } else {
                characters.Add(character);
            }
        }
        found = GetFileNext(&find);
    }
}

/* Sort newest files first while carrying each character pointer with its file
   time. Small partitions use insertion sort; larger partitions use the last
   time as the quicksort pivot, matching the retail split at ten elements. */
// FUNCTION: WIZ8 0x005c3520
static void SortPartySelectionCharactersByTime(W8Character** characters, SGP_FILETIME* times,
                                               int first, int last)
{
    if (last - first < 9) {
        for (int next = first + 1; next <= last; ++next) {
            SGP_FILETIME time = times[next];
            W8Character* character = characters[next];
            int insert = next;
            while (insert > first && CompareSGPFileTimes(&times[insert - 1], &time) < 0) {
                times[insert] = times[insert - 1];
                characters[insert] = characters[insert - 1];
                --insert;
            }
            times[insert] = time;
            characters[insert] = character;
        }
        return;
    }

    SGP_FILETIME pivot = times[last];
    int left = first - 1;
    int right = last;
    for (;;) {
        do {
            ++left;
        } while (left < last && CompareSGPFileTimes(&times[left], &pivot) > 0);
        do {
            --right;
        } while (right > first && CompareSGPFileTimes(&times[right], &pivot) < 0);
        if (left >= right) {
            break;
        }
        SGP_FILETIME time = times[left];
        times[left] = times[right];
        times[right] = time;
        W8Character* character = characters[left];
        characters[left] = characters[right];
        characters[right] = character;
    }
    times[last] = times[left];
    times[left] = pivot;
    W8Character* character = characters[left];
    characters[left] = characters[last];
    characters[last] = character;
    if (first < left - 1) {
        SortPartySelectionCharactersByTime(characters, times, first, left - 1);
    }
    if (left + 1 < last) {
        SortPartySelectionCharactersByTime(characters, times, left + 1, last);
    }
}

// FUNCTION: WIZ8 0x005be650
void W8PartySelectionCharacterCollection::SortCharactersByWriteTime()
{
    if (characters.GetCount() <= 1) {
        return;
    }

    SGP_FILETIME* times = new SGP_FILETIME[characters.GetCount()];
    memset(times, 0, characters.GetCount() * sizeof(SGP_FILETIME));
    for (int index = 0; index < characters.GetCount(); ++index) {
        char path[128];
        BuildCharacterPath(path, characters[index]->name, -1);
        int handle = FileOpen(path, FILE_ACCESS_READ, 0);
        if (handle) {
            SGP_FILETIME creation;
            SGP_FILETIME access;
            GetFileManFileTime(handle, &creation, &access, &times[index]);
            FileClose(handle);
        }
    }
    SortPartySelectionCharactersByTime(characters.data, times, 0, characters.GetCount() - 1);
    delete[] times;
}

class W8PartySelectionListControl;

class W8PartySelectionListSelectionListener {
public:
    virtual void OnSelectionChanged(W8PartySelectionListControl* control, int selection) = 0;
};

/* State 5's scrolling party-name list. Its secondary vtable is the existing
   range callback; its own listener is the controller's independently observed
   +4 callback subobject. */
// VTABLE: WIZ8 0x005ef464
class W8PartySelectionListControl : public W8Widget, public W8RangeListener {
public:
    W8PartySelectionListControl(Controls* panel, unsigned int region, int left, int top, int right,
                                int bottom)
        : W8Widget(panel, region, left, top, right, bottom), m_visible_rows(0x11), m_selection(0),
          m_hovered(-1), m_first_visible(0), m_listener(0)
    {
    }

    virtual void Redraw(bool full_redraw) override;
    virtual void OnMouseLeave(int event) override;
    virtual void OnMouseMove(int event) override;
    virtual void OnLeftButtonUp(int event) override;
    virtual void OnRangeChanged(W8RangeControl* control) override;

    int m_visible_rows;
    int m_selection;
    int m_hovered;
    int m_first_visible;
    W8PartySelectionListSelectionListener* m_listener;
};
W8_ABI_ASSERT(sizeof(W8PartySelectionListControl) == 0x4c, "W8PartySelectionListControl_size");
W8_ASSERT_BASE_END(W8PartySelectionListControl, W8RangeListener, m_visible_rows, 0x34);

// FUNCTION: WIZ8 0x005bff60
void W8PartySelectionListControl::Redraw(bool full_redraw)
{
    if (m_active && (m_dirty || full_redraw)) {
        int left = m_pPanel->m_bounds.left + m_left;
        int top = m_pPanel->m_bounds.top + m_top;
        int right = m_pPanel->m_bounds.left + m_right;
        int bottom = m_pPanel->m_bounds.top + m_bottom;

        InvalidateRegion(left, top, right, bottom, 0);
        BlitCatalogSurfaceRectTo16BPP(FRAME_BUFFER, left, top, right, bottom, 0x1b6, 0, 0);
        SetFontDestBuffer(FRAME_BUFFER, left, top, right, bottom, 0);

        int end = m_first_visible + m_visible_rows;
        if (g_party_selection_character_collection->names.GetCount() <= end) {
            end = g_party_selection_character_collection->names.GetCount();
        }
        top += 1;
        SetFont(g_wiz_text_font_secondary);
        for (int row = m_first_visible; row < end; ++row) {
            unsigned short* colour = g_font_state_palettes[W8_FONT_PALETTE_BLUE];
            if (row != m_selection) {
                colour = g_wiz_text_font_secondary_palette;
                if (row == m_hovered) {
                    colour = g_font_state_palettes[W8_FONT_PALETTE_YELLOW];
                }
            }
            SetFontObjectPalette16BPP(g_wiz_text_font_secondary, colour);
            gprintf(left + 2, top, L"%S",
                    *g_party_selection_character_collection->names.GetAt(row));
            top += 0x0e;
        }
        SetFontObjectPalette16BPP(g_wiz_text_font_secondary, g_wiz_text_font_secondary_palette);
        SetFontDestBuffer(FRAME_BUFFER, 0, 0, 0x280, 0x1e0, 0);
        m_dirty = false;
    }
}

// FUNCTION: WIZ8 0x005c00c0
void W8PartySelectionListControl::OnMouseLeave(int event)
{
    m_hovered = -1;
    Invalidate(static_cast<unsigned char>(event));
}

// FUNCTION: WIZ8 0x005c00e0
void W8PartySelectionListControl::OnMouseMove(int)
{
    POINT point;
    SGPMouseGetPos(&point);
    int hovered = (point.x - m_pPanel->m_bounds.top - m_top) / 0x0e + m_first_visible;
    if (hovered != m_hovered) {
        m_hovered = hovered;
        Invalidate(false);
    }
}

// FUNCTION: WIZ8 0x005c0140
void W8PartySelectionListControl::OnLeftButtonUp(int event)
{
    POINT point;
    SGPMouseGetPos(&point);
    int selection = (point.y - m_pPanel->m_bounds.top - m_top) / 0x0e + m_first_visible;
    if (selection != m_selection) {
        m_selection = selection;
        Invalidate(static_cast<unsigned char>(event));
        if (m_listener) {
            m_listener->OnSelectionChanged(this, m_selection);
        }
    }
}

// FUNCTION: WIZ8 0x005c01b0
void W8PartySelectionListControl::OnRangeChanged(W8RangeControl* control)
{
    m_first_visible = control->m_value;
    Invalidate(false);
}

class W8PartySelectionInputHandler;

class W8PartySelectionDecisionListener {
public:
    virtual void OnDecision(W8PartySelectionInputHandler* handler, unsigned char accepted) = 0;
    virtual void OnToggle(int value) = 0;
};

class W8PartySelectionPanelSelectionListener {
public:
    virtual void SelectPartyMemberRow(int row) = 0;
    virtual void OpenCampForSelectedMember(int row) = 0;
    virtual void AdjustPartyMemberRange(int amount) = 0;
};

/* Each visible character row is the same text control that W8Control stores
   and selects. The three added dwords are the visible row, absolute character
   index, and the panel callback used for row/scroll actions. */
class W8PartySelectionCharacterRow : public W8TextControl {
public:
    W8PartySelectionCharacterRow(Controls* panel, int top, int row);
    virtual void Redraw(bool full_redraw) override;
    virtual void AdjustValue(int amount) override;
    virtual void OnRightButtonUp(int event) override;
    virtual void OnLeftButtonDoubleClick(int event) override;

    int m_row;
    int m_character_index;
    W8PartySelectionPanelSelectionListener* m_selection_listener;
};
W8_ABI_ASSERT(sizeof(W8PartySelectionCharacterRow) == 0xc4, "W8PartySelectionCharacterRow_size");

// VTABLE: WIZ8 0x005EF4BC W8PartySelectionInputHandler
// class W8PartySelectionInputHandler
class W8PartySelectionInputHandler {
public:
    W8PartySelectionInputHandler() {}
    virtual ~W8PartySelectionInputHandler()
    {
        RemoveTextInputField(0);
        KillTextInputMode();
    }
    unsigned char HandleInput(const InputAtom* input);

    W8PartySelectionDecisionListener* m_listener;
};
W8_ABI_ASSERT(sizeof(W8PartySelectionInputHandler) == 8, "W8PartySelectionInputHandler_size");

class W8PartySelectionController;

/* Controls owns the six row widgets. The three callback bases occupy +0x4c,
   +0x50 and +0x54; the complete W8Control member at +0x58 owns their selection
   vector. The selected absolute row at +0x80 is read directly by the party-selection
   keyboard handler. */
// VTABLE: WIZ8 0x005ef3b4 W8PartySelectionPanelSelectionListener
class W8PartySelectionCharacterPanel : public Controls,
                                       public W8ControlSelectionListener,
                                       public W8RangeListener,
                                       public W8PartySelectionPanelSelectionListener {
public:
    W8PartySelectionCharacterPanel();
    virtual ~W8PartySelectionCharacterPanel();
    virtual void OnSelectionChanged(W8ControlSelection* control, int selected) override;
    virtual void OnRangeChanged(W8RangeControl* control) override;
    virtual void SelectPartyMemberRow(int row) override;
    virtual void OpenCampForSelectedMember(int row) override;
    virtual void AdjustPartyMemberRange(int amount) override;
    /* Descriptive name for the visible character-row refresh operation. */
    void RefreshVisibleRows();
    void SynchronizeVisibleSelection();
    void SetSelectedRow(int selection);

    W8ControlSelection m_control;
    W8RangeControl* m_range;
    int m_selected_row;
};
W8_ABI_ASSERT(sizeof(W8PartySelectionCharacterPanel) == 0x84,
              "W8PartySelectionCharacterPanel_size");
W8_ASSERT_BASE_END(W8PartySelectionCharacterPanel, W8PartySelectionPanelSelectionListener,
                   m_control, 0x54);

// VTABLE: WIZ8 0x005ef448 W8TextControl::Listener
class W8PartySelectionCharacterGridPanel : public Controls, public W8TextControl::Listener {
public:
    W8PartySelectionCharacterGridPanel();
    virtual ~W8PartySelectionCharacterGridPanel();
    virtual void OnPrimary(W8TextControl* control) override;
    virtual void OnSecondary(W8TextControl*) override {}
};
W8_ABI_ASSERT(sizeof(W8PartySelectionCharacterGridPanel) == 0x50,
              "W8PartySelectionCharacterGridPanel_size");
W8_ASSERT_BASE_TAIL(W8PartySelectionCharacterGridPanel, W8TextControl::Listener, 0x4c);

class W8PartySelectionPartySlotRow : public W8TextControl {
public:
    W8PartySelectionPartySlotRow(Controls* panel, int row);
    virtual void Redraw(bool full_redraw) override;
    virtual void OnRightButtonUp(int event) override;
    virtual void OnLeftButtonDoubleClick(int event) override;

    int m_row;
    W8Widget* m_redraw_partner;
};
W8_ABI_ASSERT(sizeof(W8PartySelectionPartySlotRow) == 0xc0, "W8PartySelectionPartySlotRow_size");

class W8PartySelectionPartySlotPanel : public Controls, public W8ControlSelectionListener {
public:
    friend class W8PartySelectionController;

    W8PartySelectionPartySlotPanel();
    virtual ~W8PartySelectionPartySlotPanel();
    virtual void OnSelectionChanged(W8ControlSelection* control, int selected) override;

    W8ControlSelection m_control;
};
W8_ABI_ASSERT(sizeof(W8PartySelectionPartySlotPanel) == 0x74,
              "W8PartySelectionPartySlotPanel_size");
W8_ASSERT_BASE_END(W8PartySelectionPartySlotPanel, W8ControlSelectionListener, m_control, 0x4c);

class W8PartySelectionCharacterSummaryPanel : public Controls {
public:
    W8PartySelectionCharacterSummaryPanel() : Controls(), m_character(0) {}
    virtual void Redraw() override;

    W8Character* m_character;
};
W8_ABI_ASSERT(sizeof(W8PartySelectionCharacterSummaryPanel) == 0x50,
              "W8PartySelectionCharacterSummaryPanel_size");

/* Unlike the two selection panels above, the option panel does not inherit
   W8Control.  Its constructor writes a mode word at +0x4c and constructs a
   complete W8Control member at +0x50.  The two toggle controls and the owned
   option-entry vector follow that member at the observed offsets. */
class W8PartySelectionOptionPanel : public Controls {
public:
    W8PartySelectionOptionPanel();
    virtual ~W8PartySelectionOptionPanel();
    virtual void Redraw() override;
    void SetOptionPanelMode(W8PartyCreationPage page);

    W8PartyCreationPage m_page;
    W8ControlSelection m_options;
    W8TextControl* m_toggle;
    W8TextControl* npc_interact_toggle;
    W8GrowableVector<W8TextBuffer*> m_entries;
    int m_render_left;
    int m_render_top;
    short m_image_width;
    short m_image_height;
};
W8_ABI_ASSERT(sizeof(W8PartySelectionOptionPanel) == 0x98, "W8PartySelectionOptionPanel_size");

/* The party-selection owner is established by three construction-phase abstract
   vtables, seven registered text controls, the list-selection callback above,
   and independently observed owned state through +0x6c. The address-qualified
   name does not claim a source-era screen name. */
// VTABLE: WIZ8 0x005ef4cc
class W8PartySelectionController : public W8TextControl::Listener,
                                   public W8PartySelectionListSelectionListener,
                                   public W8PartySelectionDecisionListener {
public:
    W8PartySelectionController() : m_character(0), m_input_handler(0), m_dialog(0) {}
    ~W8PartySelectionController();

    virtual void OnPrimary(W8TextControl* control) override;
    virtual void OnSecondary(W8TextControl*) override {}
    virtual void OnSelectionChanged(W8PartySelectionListControl* control, int selection) override;
    virtual void OnDecision(W8PartySelectionInputHandler* handler, unsigned char accepted) override;
    virtual void OnToggle(int value) override;

    void SetMode(W8PartySelectionMode mode);
    void SetSelection(int selection, bool highlighted, bool refresh);
    void OpenNotification(const wchar_t* message, bool allow_cancel,
                          W8PartyConfirmationAction action);
    void Setup();
    void InvalidatePartySelectionComposition();
    void DrawPartySelectionComposition();
    void ApplyPartySelectionConfirmation(W8PartyConfirmationAction action, bool accepted);
    void LoadImportedPartyFile(int selection);
    void TogglePartyMemberSelection();

    W8PartySelectionMode m_mode;
    W8PartySelectionMode m_previous_mode;
    bool m_redraw_backdrop;
    unsigned char pad_15[3];
    W8Character* m_character;
    W8RangeControl* m_range;
    W8PartySelectionCharacterPanel* m_character_panel;
    W8PartySelectionCharacterGridPanel* m_control0;
    W8PartySelectionPartySlotPanel* m_control1;
    W8PartySelectionCharacterSummaryPanel* m_control2;
    W8PartySelectionOptionPanel* m_control3;
    Controls* m_left_panel;
    Controls* m_import_panel;
    Controls* m_bottom_panel;
    W8TextControl* m_create_button;
    W8TextControl* m_add_remove_button;
    W8TextControl* m_delete_button;
    W8TextControl* m_review_button;
    W8TextControl* m_reset_button;
    W8TextControl* m_confirm_button;
    W8TextControl* m_back_button;
    W8PartySelectionListControl* m_list;
    W8TextBuffer* m_text_buffer;
    W8PartySelectionInputHandler* m_input_handler;
    W8MessageDialogBase* m_dialog;
    W8PartyConfirmationAction m_confirmation_action;
};
W8_ABI_ASSERT(sizeof(W8PartySelectionController) == 0x70, "W8PartySelectionController_size");
W8_ASSERT_BASE_END(W8PartySelectionController, W8PartySelectionDecisionListener, m_mode, 0x8);

// GLOBAL: WIZ8 0x0069C4E8
W8PartySelectionController* g_party_selection_controller;

// FUNCTION: WIZ8 0x005C33C0
void RefreshPartySelectionPortrait(unsigned int party_slot)
{
    W8TextControl** buttons = g_party_selection_controller->m_control1->m_control.m_lsButtons.data;
    if (static_cast<int>(party_slot - 2) <
        g_party_selection_controller->m_control1->m_control.m_lsButtons.GetCount()) {
        buttons += party_slot - 2;
    }
    W8PartySelectionPartySlotRow* row = static_cast<W8PartySelectionPartySlotRow*>(*buttons);
    Controls* panel = row->m_pPanel;
    int portrait = g_status.buffers.Char[row->m_row + 2].portrait_index;
    unsigned int flags = VO_BLT_SRCTRANSPARENCY;
    if ((g_portrait_descriptors[portrait].render_mode == 1 && row->m_row % 2 == 0) ||
        (g_portrait_descriptors[portrait].render_mode == 2 && row->m_row % 2 != 0)) {
        flags = VO_BLT_SRCTRANSPARENCY | VO_BLT_MIRROR_Y;
    }
    BlitPartyPortraitAnimation(portrait, panel->m_bounds.left + row->m_left,
                               panel->m_bounds.top + row->m_top, flags, row->m_row + 2, false);
}

/* Whether the party selector is in its review-existing-character mode; the
   camp screen consults it when deciding if the level-up panel applies. */
// FUNCTION: WIZ8 0x005c3470
bool PartySelectionInReviewMode(void)
{
    return g_party_selection_controller->m_mode == W8_PARTY_SELECT_IMPORT;
}

W8PartySelectionCharacterRow::W8PartySelectionCharacterRow(Controls* panel, int top, int row)
    : W8TextControl(panel, 0xffffffff, 0, top, 0, 0, 0xfb, 0, 0, 1, 2, 1, -1), m_row(row),
      m_character_index(0), m_selection_listener(0)
{
    AddLayoutFlags(0x11);
    m_character_index = g_party_selection_character_collection->first_visible + m_row;
    SetActive(m_character_index < g_party_selection_character_collection->characters.GetCount());
    Invalidate(false);
}

// FUNCTION: WIZ8 0x005be9b0
void W8PartySelectionCharacterRow::Redraw(bool full_redraw)
{
    if (!m_active || (!full_redraw && !m_dirty)) {
        return;
    }

    W8TextControl::Redraw(full_redraw);
    W8Character* character =
        g_party_selection_character_collection->GetCharacter(m_character_index);
    int left = m_pPanel->m_bounds.left + m_left;
    int top = m_pPanel->m_bounds.top + m_top;
    DrawCatalogImage(FRAME_BUFFER, 0x13, character->portrait_index, 0, left + 2, top + 2,
                     VO_BLT_SRCTRANSPARENCY, 0);
    if (character->fInParty) {
        ShadowVideoSurfaceRect(FRAME_BUFFER, left + 2, top + 2, left + 0x2e, top + 0x25);
        SetObjectShade(g_wiz_text_font_secondary_object, 6);
    }

    left += 0x36;
    SetFont(g_wiz_text_font_secondary);
    gprintf(left, top + 4, g_format_s, character->name);
    gprintf(left, top + 0x0e, L"%s %d %s", gppStringList[0x6b9], character->uiExpLevel,
            gppStringList[g_profession_name_message_ids[character->iProfession]]);
    gprintf(left, top + 0x18, g_format_s_space_s,
            gppStringList[g_gender_name_message_rows[character->gender][0]],
            gppStringList[g_race_name_message_ids[character->iRace]]);
    SetObjectShade(g_wiz_text_font_secondary_object, 4);
}

// FUNCTION: WIZ8 0x005beb90
void W8PartySelectionCharacterRow::AdjustValue(int amount)
{
    if (m_active && m_enabled && m_selection_listener) {
        m_selection_listener->AdjustPartyMemberRange(amount);
    }
}

// FUNCTION: WIZ8 0x005beb10
void W8PartySelectionCharacterRow::OnRightButtonUp(int event)
{
    if (m_active && m_enabled) {
        SetAlternateTextEnabled(false);
        if (m_selection_listener) {
            m_selection_listener->OpenCampForSelectedMember(m_row);
        }
    }
    W8TextControl::OnRightButtonUp(event);
}

// FUNCTION: WIZ8 0x005beb50
void W8PartySelectionCharacterRow::OnLeftButtonDoubleClick(int event)
{
    if (m_active && m_enabled && m_selection_listener) {
        m_selection_listener->SelectPartyMemberRow(m_row);
    }
    W8TextControl::OnLeftButtonDoubleClick(event);
}

void W8PartySelectionCharacterPanel::RefreshVisibleRows()
{
    for (int index = 0; index < m_controls.GetCount(); ++index) {
        W8PartySelectionCharacterRow* row =
            static_cast<W8PartySelectionCharacterRow*>(ControlAt(index));
        row->m_character_index = g_party_selection_character_collection->first_visible + row->m_row;
        row->SetActive(row->m_character_index <
                       g_party_selection_character_collection->characters.GetCount());
        row->Invalidate(false);
    }
}

void W8PartySelectionCharacterPanel::SynchronizeVisibleSelection()
{
    int selection = m_selected_row - g_party_selection_character_collection->first_visible;
    if (selection < 0 || selection > 5) {
        selection = -1;
    }
    m_control.SetSelected(selection);
    m_control.m_selectionListener = this;
    RefreshVisibleRows();
}

// FUNCTION: WIZ8 0x005bebc0
W8PartySelectionCharacterPanel::W8PartySelectionCharacterPanel()
    : Controls(), m_range(0), m_selected_row(0)
{
    AcquireRegionSet(&g_party_selection_character_region_set);
    m_bounds.left = 0x148;
    m_bounds.top = 0x31;

    int top = 0;
    for (int row = 0; row < 6; ++row) {
        W8PartySelectionCharacterRow* control = new W8PartySelectionCharacterRow(this, top, row);
        control->m_selection_listener = this;
        m_control.AddEntry(control);
        top += 0x2a;
    }

    SynchronizeVisibleSelection();
}

// FUNCTION: WIZ8 0x005bedf0
W8PartySelectionCharacterPanel::~W8PartySelectionCharacterPanel()
{
    DestroyAllControls();
}

// FUNCTION: WIZ8 0x005bee70
void W8PartySelectionCharacterPanel::OnSelectionChanged(W8ControlSelection*, int selected)
{
    m_selected_row = selected + g_party_selection_character_collection->first_visible;
    g_party_selection_controller->SetSelection(m_selected_row, false, false);
}

// FUNCTION: WIZ8 0x005beea0
void W8PartySelectionCharacterPanel::OnRangeChanged(W8RangeControl* control)
{
    if (!m_fEnabled) {
        return;
    }
    g_party_selection_character_collection->first_visible = control->m_value;
    m_control.m_selectionListener = 0;
    SynchronizeVisibleSelection();
}

// FUNCTION: WIZ8 0x005bef60
void W8PartySelectionCharacterPanel::SetSelectedRow(int selection)
{
    m_selected_row = selection;
    int visible = selection - g_party_selection_character_collection->first_visible;
    if (visible < 0 || visible > 5 ||
        g_party_selection_character_collection->characters.GetCount() <
            g_party_selection_character_collection->first_visible + 6) {
        int maximum = g_party_selection_character_collection->characters.GetCount() - 6;
        int first = selection < maximum ? selection : maximum;
        if (first < 0) {
            first = 0;
        }
        g_party_selection_character_collection->first_visible = first;
        m_range->SetValue(first);
    }

    m_control.m_selectionListener = 0;
    SynchronizeVisibleSelection();
}

// FUNCTION: WIZ8 0x005bf0c0
void W8PartySelectionCharacterPanel::SelectPartyMemberRow(int row)
{
    m_control.SetSelected(row);
    g_party_selection_controller->TogglePartyMemberSelection();
}

// FUNCTION: WIZ8 0x005bf050
void W8PartySelectionCharacterPanel::OpenCampForSelectedMember(int row)
{
    m_control.SetSelected(row);
    int slot;
    for (slot = 0; slot < 6; ++slot) {
        if (g_status.buffers.XChar[slot + 2].fOccupied &&
            &g_status.buffers.Char[slot + 2] == g_party_selection_controller->m_character) {
            break;
        }
    }
    g_pending_screen_state.parameter_2 = slot < 6 ? slot + 2 : -1;
    g_pending_screen_state.parameter_3 = g_party_selection_controller->m_character;
    SetPendingScreenState(W8_SCREEN_CAMP);
}

// FUNCTION: WIZ8 0x005bf0e0
void W8PartySelectionCharacterPanel::AdjustPartyMemberRange(int amount)
{
    m_range->AdjustValue(amount);
}

// FUNCTION: WIZ8 0x005bf120
W8PartySelectionPartySlotRow::W8PartySelectionPartySlotRow(Controls* panel, int row)
    : W8TextControl(panel, 0xffffffff, (row & 1) * 0x20b + 0x0e, (row / 2) * 0x82 + 0x3a, 0, 0,
                    0x101, 0, -1, 1, 0, 0, -1),
      m_row(row), m_redraw_partner(0)
{
    AddLayoutFlags(0x11);
    UpdateTextBounds(m_left - 8, m_top + 0x51, m_left + 0x61, m_top + 0x60);
}

// FUNCTION: WIZ8 0x005bf280
void W8PartySelectionPartySlotRow::Redraw(bool full_redraw)
{
    if (!m_active || (!full_redraw && !m_dirty)) {
        return;
    }

    int left = m_pPanel->m_bounds.left + m_left;
    int top = m_pPanel->m_bounds.top + m_top;
    W8Character* character = 0;
    if (!g_status.buffers.XChar[m_row + 2].fOccupied) {
        ColorFillVideoSurfaceArea(FRAME_BUFFER, left, top, m_pPanel->m_bounds.left + m_right,
                                  m_pPanel->m_bounds.top + m_bottom, 0x8000);
    } else {
        character = &g_status.buffers.Char[m_row + 2];
        int portrait = character->portrait_index;
        unsigned int flags = VO_BLT_SRCTRANSPARENCY;
        if ((g_portrait_descriptors[portrait].render_mode == 1 && !(m_row & 1)) ||
            (g_portrait_descriptors[portrait].render_mode == 2 && (m_row & 1))) {
            flags = VO_BLT_SRCTRANSPARENCY | VO_BLT_MIRROR_Y;
        }
        RenderPartyPortrait(portrait, left, top, flags, 1, m_row + 2);
    }
    m_textBuffer.SetText(character ? character->name : 0, g_wiz_text_font_secondary);
    DrawCatalogImageAndInvalidate(FRAME_BUFFER, 0x100, 0, 0, left - 0x0d, top - 0x0b,
                                  VO_BLT_SRCTRANSPARENCY, 0);
    W8TextControl::Redraw(full_redraw);
    if (m_redraw_partner) {
        m_redraw_partner->Invalidate(false);
    }
}

// FUNCTION: WIZ8 0x005bf410
void W8PartySelectionPartySlotRow::OnRightButtonUp(int event)
{
    W8TextControl::OnRightButtonUp(event);
    if (!m_enabled || !m_active) {
        return;
    }
    SetAlternateTextEnabled(false);
    if (m_row == -1) {
        int slot;
        for (slot = 0; slot < 6; ++slot) {
            if (g_status.buffers.XChar[slot + 2].fOccupied &&
                &g_status.buffers.Char[slot + 2] == g_party_selection_controller->m_character) {
                break;
            }
        }
        g_pending_screen_state.parameter_2 = slot < 6 ? slot + 2 : -1;
        g_pending_screen_state.parameter_3 = g_party_selection_controller->m_character;
    } else {
        g_pending_screen_state.parameter_2 = m_row + 2;
        g_pending_screen_state.parameter_3 = &g_status.buffers.Char[m_row + 2];
    }
    SetPendingScreenState(W8_SCREEN_CAMP);
}

// FUNCTION: WIZ8 0x005bf4e0
void W8PartySelectionPartySlotRow::OnLeftButtonDoubleClick(int event)
{
    W8TextControl::OnLeftButtonDoubleClick(event);
    if (m_enabled && m_active) {
        g_party_selection_controller->SetSelection(m_row, true, false);
        g_party_selection_controller->TogglePartyMemberSelection();
    }
}

// FUNCTION: WIZ8 0x005bf6b0
void W8PartySelectionPartySlotPanel::OnSelectionChanged(W8ControlSelection*, int)
{
    g_party_selection_controller->SetSelection(m_control.m_selectedIndex, true, false);
}

// FUNCTION: WIZ8 0x005bf520
W8PartySelectionPartySlotPanel::W8PartySelectionPartySlotPanel() : Controls()
{
    AcquireRegionSet(&g_party_selection_party_slot_region_set);
    for (int row = 0; row < 6; ++row) {
        m_control.AddEntry(new W8PartySelectionPartySlotRow(this, row));
    }
    m_control.m_selectionListener = this;
    for (int slot = 0; slot < 6; ++slot) {
        W8TextControl* control = m_control.m_lsButtons[slot];
        control->SetEnabled(g_status.buffers.XChar[slot + 2].fOccupied);
    }
    Invalidate(0);
}

// FUNCTION: WIZ8 0x005bf6d0
W8PartySelectionCharacterGridPanel::W8PartySelectionCharacterGridPanel() : Controls()
{
    AcquireRegionSet(&g_party_selection_character_grid_region_set);
    for (int index = 0; index < 6; ++index) {
        int left = (index & 1) ? 0x21d : 0x52;
        int top = (index / 2) * 0x82 + 0x6c;
        W8TextControl* control =
            new W8TextControl(this, 0xffffffff, left, top, 0, 0, 0xa7, 0, 0, 2, 1, 4, 3);
        control->m_listener = this;
    }
}

// FUNCTION: WIZ8 0x005bf840
void W8PartySelectionCharacterGridPanel::OnPrimary(W8TextControl* control)
{
    int index;
    for (index = 0; index < 6; ++index) {
        if (ControlAt(index) == control) {
            break;
        }
    }
    control->SetAlternateTextEnabled(false);
    g_pending_screen_state.parameter_3 = &g_status.buffers.Char[index + 2];
    g_pending_screen_state.mode = 2;
    SetPendingScreenState(W8_SCREEN_CHARACTER);
}

// FUNCTION: WIZ8 0x005bf7e0
W8PartySelectionCharacterGridPanel::~W8PartySelectionCharacterGridPanel()
{
    DestroyAllControls();
}

// FUNCTION: WIZ8 0x005bf640
W8PartySelectionPartySlotPanel::~W8PartySelectionPartySlotPanel()
{
    DestroyAllControls();
}

/* Draw the selected character's portrait and complete summary card.  The
   seven attribute values and the four right-column derived values come from
   the canonical character record; no parallel presentation snapshot exists. */
// FUNCTION: WIZ8 0x005bf8b0
void W8PartySelectionCharacterSummaryPanel::Redraw()
{
    if (!m_fEnabled || !m_fDirty) {
        return;
    }
    m_fDirty = false;

    DrawCatalogImageAndInvalidate(FRAME_BUFFER, 0xfc, 0, 0, 0x7f, 0xc5, VO_BLT_SRCTRANSPARENCY, 0);
    if (!m_character) {
        BlitCatalogSurfaceRectTo16BPP(FRAME_BUFFER, 0x85, 0x30, 0x139, 0xbf, 0x1b6, 0, 0);
        InvalidateRegion(0x85, 0x30, 0x139, 0xbf, 0);
        return;
    }

    W8Character* character = m_character;
    DrawCatalogImageAndInvalidate(FRAME_BUFFER, 0x11, character->portrait_index, 0, 0x85, 0x30,
                                  VO_BLT_SRCTRANSPARENCY, 0);
    if (character->fInParty) {
        W8ControlsRect bounds = {0x85, 0x30, 0x139, 0xba};
        W8TextBuffer overlay(&bounds, gppStringList[0x6b8], g_options_title_font,
                             g_W8TextBufferAlignBottom | g_W8TextBufferAlignCenter, 4);
        overlay.RenderToTarget(0, false, FRAME_BUFFER);
    }

    {
        W8ControlsRect name_bounds = {0x84, 0xc8, 0x139, 0xed};
        W8TextBuffer name(
            &name_bounds, FormatWideString(L"%s (%s)", character->name_part_2, character->name),
            g_wiz_text_bold_font, g_W8TextBufferAlignMiddle | g_W8TextBufferAlignCenter, 4);
        name.RenderToTarget(0, false, FRAME_BUFFER);
    }

    SetFont(g_wiz_text_font_secondary);
    SetObjectShade(g_wiz_text_font_secondary_object, 4);

    const wchar_t* level_text = gppStringList[0x6b9];
    const wchar_t* profession =
        gppStringList[g_profession_name_message_ids[character->iProfession]];
    wchar_t* level_line =
        FormatWideString(L"%s %d %s", level_text, character->uiExpLevel, profession);
    int width = StringPixLength(level_line, g_wiz_text_font_secondary);
    gprintf((0xbf - width) / 2 + 0x78, 0xef, L"%s %d %s", level_text, character->uiExpLevel,
            profession);

    const wchar_t* gender = gppStringList[g_gender_name_message_rows[character->gender][0]];
    const wchar_t* race = gppStringList[g_race_name_message_ids[character->iRace]];
    wchar_t* race_line = FormatWideString(g_format_s_space_s, gender, race);
    width = StringPixLength(race_line, g_wiz_text_font_secondary);
    gprintf((0xbf - width) / 2 + 0x7f, 0xfd, g_format_s_space_s, gender, race);

    const wchar_t* personality = gppStringList[g_personality_message_ids[character->personality]];
    width = StringPixLength(const_cast<wchar_t*>(personality), g_wiz_text_font_secondary);
    gprintf((0xbf - width) / 2 + 0x82, 0x10b, g_format_s, personality);

    gprintf(0x96, 0x127, gppStringList[0x6ba]);
    gprintf(0xbf, 0x127, g_format_d, character->attributes[W8_ATTRIBUTE_STRENGTH].base);
    gprintf(0x96, 0x135, gppStringList[0x6bb]);
    gprintf(0xbf, 0x135, g_format_d, character->attributes[W8_ATTRIBUTE_INTELLIGENCE].base);
    gprintf(0x96, 0x143, gppStringList[0x6bc]);
    gprintf(0xbf, 0x143, g_format_d, character->attributes[W8_ATTRIBUTE_PIETY].base);
    gprintf(0x96, 0x151, gppStringList[0x6bd]);
    gprintf(0xbf, 0x151, g_format_d, character->attributes[W8_ATTRIBUTE_VITALITY].base);
    gprintf(0x96, 0x15f, gppStringList[0x6be]);
    gprintf(0xbf, 0x15f, g_format_d, character->attributes[W8_ATTRIBUTE_DEXTERITY].base);
    gprintf(0x96, 0x16d, gppStringList[0x6bf]);
    gprintf(0xbf, 0x16d, g_format_d, character->attributes[W8_ATTRIBUTE_SPEED].base);
    gprintf(0x96, 0x17b, gppStringList[0x6c0]);
    gprintf(0xbf, 0x17b, g_format_d, character->attributes[W8_ATTRIBUTE_SENSES].base);

    gprintf(0xe3, 0x127, gppStringList[0x6c1]);
    gprintf(0x115, 0x127, g_format_d, character->uiHPMax);
    gprintf(0xe3, 0x135, gppStringList[0x6c2]);
    gprintf(0x115, 0x135, g_format_d, SumCharacterSpellPoints(character));
    gprintf(0xe3, 0x143, gppStringList[0x6c3]);
    gprintf(0x115, 0x143, g_format_d, character->uiStaminaMax);
    gprintf(0xe3, 0x151, gppStringList[0x6c4]);
    gprintf(0x115, 0x151, g_format_d, character->carrying_capacity / 10);
}

// GLOBAL: WIZ8 0x0069C4FC
unsigned int g_party_selection_option_region_set;

// GLOBAL: WIZ8 0x0069C500
unsigned int g_party_selection_range_region_set;
// GLOBAL: WIZ8 0x0069C504
unsigned int g_party_selection_left_action_region_set;
// GLOBAL: WIZ8 0x0069C508
unsigned int g_party_selection_bottom_action_region_set;
// GLOBAL: WIZ8 0x0069C50C
unsigned int g_party_selection_import_list_region_set;

/* Build the three mutually selected difficulty controls and the two
   independent toggles used by the later creation modes.  The entry vector is
   separate: redraw walks it for the repeated option-detail render pass. */
// FUNCTION: WIZ8 0x005c01d0
W8PartySelectionOptionPanel::W8PartySelectionOptionPanel()
    : Controls(0x78, 0x28, 0, 0, 0x102, 0, 0), m_options(), m_entries()
{
    AcquireRegionSet(&g_party_selection_option_region_set);

    short width;
    short height;
    GetCatalogImageSize(0x102, 0, 0, &width, &height);
    m_bounds.right = m_bounds.left + static_cast<unsigned short>(width);
    m_bounds.bottom = m_bounds.top + static_cast<unsigned short>(height);

    int top = 0x13;
    for (int index = 0; index < 3; ++index) {
        m_options.AddEntry(
            new W8TextControl(this, 0xffffffff, 0x15f, top, 0, 0, 0xf1, 0, 4, 6, 5, 7, -1));
        top += 0x16;
    }
    m_options.SetSelected(g_settings.difficulty);

    top += 0x16;
    npc_interact_toggle =
        new W8TextControl(this, 0xffffffff, 0x15b, top, 0, 0, 0xf1, 0, 2, 0, 3, 1, -1);
    npc_interact_toggle->AddLayoutFlags(g_W8TextControlLayoutLatchedImage |
                                        g_W8TextControlLayoutToggle);
    if (g_settings.simplified_npc_interaction) {
        npc_interact_toggle->EnableSecondaryState(false);
    }

    m_toggle = new W8TextControl(this, 0xffffffff, 0x15b, 0xd6, 0, 0, 0xf1, 0, 2, 0, 3, 1, -1);
    m_toggle->AddLayoutFlags(g_W8TextControlLayoutLatchedImage | g_W8TextControlLayoutToggle);

    GetCatalogImageSize(0x102, 0, 1, &m_image_width, &m_image_height);
    m_render_left = m_bounds.left + 0x18 + (0x160 - static_cast<unsigned short>(m_image_width)) / 2;
    m_render_top = m_bounds.top + 0x80;
}

// FUNCTION: WIZ8 0x005c0560
void W8PartySelectionOptionPanel::Redraw()
{
    bool redraw = m_fEnabled && m_fDirty;
    Controls::Redraw();
    if (!redraw) {
        return;
    }

    for (int index = 0; index < m_entries.GetCount(); ++index) {
        m_entries[index]->RenderToTarget(0, true, FRAME_BUFFER);
    }
    if (m_page == W8_PARTY_CREATION_SAVE_NAME) {
        DrawCatalogImage(FRAME_BUFFER, 0x102, 0, 1, m_render_left, m_render_top,
                         VO_BLT_SRCTRANSPARENCY, 0);
    }
}

/* Give the active string editor first refusal on keyboard events, reject the
   filename characters retail excludes, and report both edit-state and final
   Escape/Return decisions to the controller's decision-listener subobject. */
// FUNCTION: WIZ8 0x005c0e50
unsigned char W8PartySelectionInputHandler::HandleInput(const InputAtom* input)
{
    if (input->usEvent != KEY_DOWN && input->usEvent != KEY_REPEAT) {
        DispatchMainGameMouseButtons(input);
        return 0;
    }

    if (input->usParam != VK_ESCAPE) {
        if (input->usParam != VK_RETURN) {
            unsigned short character = TranslateKeyToCharacter(
                static_cast<unsigned short>(input->usParam), input->usKeyState);
            if (character != 0 && strchr("\\/:*?\"<>|", static_cast<unsigned char>(character))) {
                return 1;
            }
            HandleTextInput(input);
            if (m_listener) {
                m_listener->OnToggle(GetTextInputFieldLength(0) != 0);
            }
            return 1;
        }
        if (!GetTextInputFieldLength(0)) {
            return 1;
        }
    }

    SetTargetCursor(W8_CURSOR_NONE);
    if (m_listener) {
        m_listener->OnDecision(this, input->usParam == VK_ESCAPE);
    }
    return 1;
}

/* Replace the option panel's complete detail composition. Mode zero owns the
   eight creation-summary buffers, mode one shows its single explanatory
   paragraph, and mode two owns the name prompt plus the real string-input
   session. The panel's three difficulty controls are interactive only in
   mode zero. */
// FUNCTION: WIZ8 0x005c05f0
void W8PartySelectionOptionPanel::SetOptionPanelMode(W8PartyCreationPage page)
{
    m_page = page;
    while (m_entries.GetCount() > 0) {
        m_entries.RemoveAtAndDelete(m_entries.GetCount() - 1);
    }
    for (int index = 0; index < m_controls.GetCount(); ++index) {
        ControlAt(index)->SetActive(page == W8_PARTY_CREATION_OPTIONS);
    }

    W8ControlsRect bounds = {m_bounds.left + 0x22, m_bounds.top + 0x12, m_bounds.left + 0x16e,
                             m_bounds.top + 0x171};

    if (page == W8_PARTY_CREATION_OPTIONS) {
        m_entries.Add(new W8TextBuffer(&bounds, gppStringList[0x7f7], g_options_detail_font,
                                       g_W8TextBufferAlignTop | g_W8TextBufferAlignLeft, 4));

        bounds.right = m_bounds.left + 0x155;
        m_entries.Add(new W8TextBuffer(&bounds, gppStringList[0x7f8], g_options_detail_font,
                                       g_W8TextBufferAlignRight | g_W8TextBufferAlignTop, 4));
        bounds.top += 0x16;
        m_entries.Add(new W8TextBuffer(&bounds, gppStringList[0x7f9], g_options_detail_font,
                                       g_W8TextBufferAlignRight | g_W8TextBufferAlignTop, 4));
        bounds.top += 0x16;
        m_entries.Add(new W8TextBuffer(&bounds, gppStringList[0x7fa], g_options_detail_font,
                                       g_W8TextBufferAlignRight | g_W8TextBufferAlignTop, 4));

        bounds.right = m_bounds.left + 0x16e;
        bounds.top += 0x2c;
        m_entries.Add(new W8TextBuffer(&bounds, gppStringList[0x80b], g_options_detail_font,
                                       g_W8TextBufferAlignTop | g_W8TextBufferAlignLeft, 4));

        bounds.top += 0x2c;
        W8TextBuffer* text =
            new W8TextBuffer(&bounds, gppStringList[0x6cd], g_options_detail_font,
                             g_W8TextBufferAlignCenter | g_W8TextBufferAlignTop, 4);
        text->SetLineHeight(0x16);
        m_entries.Add(text);

        bounds.top += 0x42;
        m_entries.Add(new W8TextBuffer(&bounds, gppStringList[0x6ce], g_options_detail_font,
                                       g_W8TextBufferAlignTop | g_W8TextBufferAlignLeft, 4));

        bounds.top += 0x2c;
        text = new W8TextBuffer(&bounds, gppStringList[0x6cf], g_options_detail_font,
                                g_W8TextBufferAlignCenter | g_W8TextBufferAlignTop, 4);
        text->SetLineHeight(0x16);
        m_entries.Add(text);
        return;
    }

    if (page == W8_PARTY_CREATION_NOTICE) {
        bounds.top += 0x2c;
        W8TextBuffer* text =
            new W8TextBuffer(&bounds, gppStringList[0x6d0], g_options_detail_font,
                             g_W8TextBufferAlignCenter | g_W8TextBufferAlignTop, 4);
        text->SetLineHeight(0x16);
        m_entries.Add(text);
        return;
    }

    if (page != W8_PARTY_CREATION_SAVE_NAME) {
        return;
    }

    bounds.left = m_bounds.left + 0x2c;
    bounds.right = m_bounds.left + 0x164;
    W8TextBuffer* text = new W8TextBuffer(&bounds, gppStringList[0x6d1], g_options_detail_font,
                                          g_W8TextBufferAlignCenter | g_W8TextBufferAlignTop, 4);
    text->SetLineHeight(0x16);
    m_entries.Add(text);

    W8PartySelectionController* controller = g_party_selection_controller;
    if (controller->m_input_handler) {
        return;
    }
    W8PartySelectionInputHandler* input_handler = new W8PartySelectionInputHandler;
    InitTextInputModeWithScheme(1);
    AddTextInputField(m_render_left + 2, m_render_top + 2,
                      static_cast<unsigned short>(m_image_width) - 4,
                      static_cast<unsigned short>(m_image_height) - 4, 0x7f, L"", 0x28, 0x0f, 1);
    SetActiveField(0);
    controller->m_input_handler = input_handler;
    input_handler->m_listener = controller;
    input_handler->m_listener->OnToggle(0);
}

/* Construct the complete party-selection panel graph and attach every callback before
   the first mode transition.  The shared range is rebound by SetMode to either
   the character rows or import list; the six portrait rows invalidate their
   paired advance controls after drawing. */
// FUNCTION: WIZ8 0x005c0f30
void W8PartySelectionController::Setup()
{
    m_range = new W8RangeControl(0x1e9, 0x31, 0x1fb, 0x12b, &g_party_selection_range_region_set);
    m_character_panel = new W8PartySelectionCharacterPanel;
    m_control0 = new W8PartySelectionCharacterGridPanel;
    m_control1 = new W8PartySelectionPartySlotPanel;
    m_control2 = new W8PartySelectionCharacterSummaryPanel;
    m_control3 = new W8PartySelectionOptionPanel;

    for (int slot = 0; slot < 6; ++slot) {
        W8PartySelectionPartySlotRow* row =
            static_cast<W8PartySelectionPartySlotRow*>(m_control1->m_control.m_lsButtons[slot]);
        row->m_redraw_partner = m_control0->ControlAt(slot);
    }

    m_left_panel = new Controls(0x145, 0x137, 0, 0, -1, -1, -1);
    m_left_panel->AcquireRegionSet(&g_party_selection_left_action_region_set);
    m_create_button =
        new W8TextControl(m_left_panel, 0xffffffff, 0, 0, 0, 0, 0xfe, 0, 0, 2, 1, 2, 3);
    m_create_button->m_textBuffer.SetText(gppStringList[0x6c5], g_wiz_text_bold_font);
    m_create_button->m_listener = this;

    m_add_remove_button =
        new W8TextControl(m_left_panel, 0xffffffff, 0, 0x1a, 0, 0, 0xfe, 0, 0, 2, 1, 2, 3);
    m_add_remove_button->m_listener = this;

    m_delete_button =
        new W8TextControl(m_left_panel, 0xffffffff, 0, 0x34, 0, 0, 0xfe, 0, 0, 2, 1, 2, 3);
    m_delete_button->m_textBuffer.SetText(gppStringList[0x6c8], g_wiz_text_bold_font);
    m_delete_button->m_listener = this;

    m_review_button =
        new W8TextControl(m_left_panel, 0xffffffff, 0, 0x4e, 0, 0, 0xfe, 0, 0, 2, 1, 2, 3);
    m_review_button->m_textBuffer.SetText(gppStringList[0x6c9], g_wiz_text_bold_font);
    m_review_button->m_listener = this;

    m_import_panel = new Controls(0x148, 0x31, 0, 0, -1, -1, -1);
    m_import_panel->AcquireRegionSet(&g_party_selection_import_list_region_set);
    m_list = new W8PartySelectionListControl(m_import_panel, 0xffffffff, 0, 0, 0x9d, 0xfa);
    m_list->m_listener = this;

    m_bottom_panel = new Controls(0x84, 0x1b5, 0, 0, -1, -1, -1);
    m_bottom_panel->AcquireRegionSet(&g_party_selection_bottom_action_region_set);
    m_back_button =
        new W8TextControl(m_bottom_panel, 0xffffffff, 0x14c, 0, 0, 0, 0x106, 0, 4, 6, 5, 6, 7);
    m_back_button->EnableRegionHelp(0x6ca);
    m_back_button->m_listener = this;

    m_confirm_button =
        new W8TextControl(m_bottom_panel, 0xffffffff, 0x120, 0, 0, 0, 0x106, 0, 0, 2, 1, 2, 3);
    m_confirm_button->EnableRegionHelp(0x6cb);
    m_confirm_button->m_listener = this;

    m_reset_button = new W8TextControl(m_bottom_panel, 0xffffffff, 0xf4, 0, 0, 0, 0x106, 0, 0x18,
                                       0x1a, 0x19, 0x1c, 0x1b);
    m_reset_button->AddLayoutFlags(g_W8TextControlLayoutLatchedImage | g_W8TextControlLayoutToggle);
    m_reset_button->EnableRegionHelp(0x6cc);
    m_reset_button->m_listener = this;

    W8ControlsRect bounds = {0x7b, 6, 0x205, 0x23};
    m_text_buffer = new W8TextBuffer;
    m_text_buffer->SetLayoutBounds(&bounds, true, true);

    SetMode(W8_PARTY_SELECT_CHARACTERS);
    SetSelection(0, false, true);
}

// FUNCTION: WIZ8 0x005c0480
W8PartySelectionOptionPanel::~W8PartySelectionOptionPanel()
{
    DestroyAllControls();
    while (m_entries.GetCount() > 0) {
        m_entries.RemoveAtAndDelete(m_entries.GetCount() - 1);
    }
}

// FUNCTION: WIZ8 0x005c1590
W8PartySelectionController::~W8PartySelectionController()
{
    delete m_text_buffer;

    if (m_bottom_panel) {
        m_bottom_panel->DestroyAllControls();
        delete m_bottom_panel;
    }
    if (m_left_panel) {
        m_left_panel->DestroyAllControls();
        delete m_left_panel;
    }
    if (m_import_panel) {
        m_import_panel->DestroyAllControls();
        delete m_import_panel;
    }

    delete m_control0;
    delete m_control1;
    delete m_character_panel;
    delete m_range;
    delete m_control2;
    delete m_control3;
}

/* Switch the party-builder as a complete screen mode.  Mode zero presents the
   loose-character collection, mode one presents the six active slots and the
   import list, and modes two through four hand the screen to the option panel.
   Every panel is invalidated before its enable/region state changes so the
   next frame redraws the new composition. */
// FUNCTION: WIZ8 0x005c2010
void W8PartySelectionController::SetMode(W8PartySelectionMode mode)
{
    m_mode = mode;
    InvalidatePartySelectionComposition();
    m_bottom_panel->SetEnabled(true);
    m_bottom_panel->EnableRegionSet(true);
    m_control1->SetEnabled(true);

    const wchar_t* label = 0;
    switch (m_mode) {
    case W8_PARTY_SELECT_CHARACTERS: {
        m_range->SetEnabled(true);
        m_range->EnableRegionSet(true);
        m_left_panel->SetEnabled(true);
        m_left_panel->EnableRegionSet(true);
        m_character_panel->SetEnabled(true);
        m_character_panel->EnableRegionSet(true);

        m_character_panel->m_range = m_range;
        m_range->m_listener = m_character_panel;
        int maximum = g_party_selection_character_collection->characters.GetCount() - 6;
        if (maximum < 0) {
            maximum = 0;
        }
        m_range->SetRange(0, maximum);
        m_range->SetRangeEnabled(maximum > 0);
        m_range->Invalidate(0);

        m_character_panel->m_control.m_selectionListener = 0;
        int selected = m_character_panel->m_selected_row -
                       g_party_selection_character_collection->first_visible;
        if (selected < 0 || selected > 5) {
            selected = -1;
        }
        m_character_panel->m_control.SetSelected(selected);
        m_character_panel->m_control.m_selectionListener = m_character_panel;
        for (int index = 0; index < m_character_panel->m_controls.GetCount(); ++index) {
            W8PartySelectionCharacterRow* row =
                static_cast<W8PartySelectionCharacterRow*>(m_character_panel->ControlAt(index));
            row->m_character_index =
                g_party_selection_character_collection->first_visible + row->m_row;
            row->SetActive(row->m_character_index <
                           g_party_selection_character_collection->characters.GetCount());
            row->Invalidate(false);
        }

        m_control0->SetEnabled(false);
        m_control0->EnableRegionSet(false);
        m_control1->EnableRegionSet(true);
        m_import_panel->SetEnabled(false);
        m_import_panel->EnableRegionSet(false);
        m_control2->SetEnabled(true);
        m_control3->SetEnabled(false);
        m_control3->EnableRegionSet(false);
        m_confirm_button->SetEnabled(CountActiveCharacters() != 0);
        label = gppStringList[0x6b4];
        break;
    }
    case W8_PARTY_SELECT_IMPORT: {
        if (g_party_selection_character_collection->names.GetCount() == 0) {
            char search[128];
            GETFILESTRUCT find;
            sprintf(search, "%s\\*.*", "Saves\\Import");
            BOOLEAN found = GetFileFirst(search, &find);
            while (found) {
                if ((find.uiFileAttribs & FILE_IS_DIRECTORY) == 0) {
                    size_t length = strlen(find.zFileName) + 1;
                    char* name = new char[length];
                    memcpy(name, find.zFileName, length);
                    g_party_selection_character_collection->names.Add(name);
                }
                found = GetFileNext(&find);
            }
        }

        m_range->SetEnabled(true);
        m_range->EnableRegionSet(true);
        m_left_panel->SetEnabled(true);
        m_left_panel->EnableRegionSet(true);
        m_create_button->SetActive(false);
        m_add_remove_button->SetActive(false);
        m_delete_button->SetActive(false);
        m_character_panel->SetEnabled(false);
        m_character_panel->EnableRegionSet(false);
        m_control0->EnableRegionSet(true);
        m_control0->SetEnabled(true);
        for (int slot = 0; slot < 6; ++slot) {
            bool occupied = g_status.buffers.XChar[slot + 2].fOccupied;
            m_control0->ControlAt(slot)->SetActive(m_mode == W8_PARTY_SELECT_IMPORT && occupied &&
                                                   IsCharacterReadyToAdvance(slot + 2));
        }
        m_control0->Invalidate(0);
        m_control1->EnableRegionSet(true);
        m_import_panel->SetEnabled(true);
        m_import_panel->EnableRegionSet(true);

        m_range->m_listener = m_list;
        int maximum =
            g_party_selection_character_collection->names.GetCount() - m_list->m_visible_rows;
        if (maximum < 0) {
            maximum = 0;
        }
        m_range->SetRange(0, maximum);
        m_range->SetValue(m_list->m_selection);
        m_range->SetRangeEnabled(maximum > 0);
        m_range->Invalidate(0);

        m_control2->SetEnabled(true);
        m_control3->SetEnabled(false);
        m_control3->EnableRegionSet(false);
        label = gppStringList[0x6b5];
        break;
    }
    case W8_PARTY_SELECT_OPTIONS:
        m_range->SetEnabled(false);
        m_range->EnableRegionSet(false);
        m_left_panel->SetEnabled(false);
        m_left_panel->EnableRegionSet(false);
        m_control0->SetEnabled(false);
        m_control0->EnableRegionSet(false);
        m_control1->EnableRegionSet(false);
        m_character_panel->SetEnabled(false);
        m_character_panel->EnableRegionSet(false);
        m_import_panel->SetEnabled(false);
        m_import_panel->EnableRegionSet(false);
        m_control2->SetEnabled(false);
        m_reset_button->SetActive(false);
        m_control3->SetEnabled(true);
        m_control3->EnableRegionSet(true);
        m_control3->SetOptionPanelMode(W8_PARTY_CREATION_OPTIONS);
        label = gppStringList[0x6b6];
        break;
    case W8_PARTY_SELECT_CREATION_NOTICE:
        m_reset_button->SetActive(false);
        m_control3->SetOptionPanelMode(W8_PARTY_CREATION_NOTICE);
        label = gppStringList[0x6b7];
        break;
    case W8_PARTY_SELECT_SAVE_NAME:
        m_reset_button->SetActive(false);
        m_control3->SetOptionPanelMode(W8_PARTY_CREATION_SAVE_NAME);
        label = gppStringList[0x6b7];
        break;
    default:
        return;
    }
    m_text_buffer->SetText(label, g_wiz_text_bold_font);
}

// FUNCTION: WIZ8 0x005c1680
void W8PartySelectionController::SetSelection(int selection, bool party_slot, bool refresh_other)
{
    if (!party_slot) {
        if (m_mode == W8_PARTY_SELECT_CHARACTERS) {
            m_character = g_party_selection_character_collection->GetCharacter(selection);
            int selected_slot = -1;
            if (m_character && m_character->fInParty) {
                for (int slot = 0; slot < 6; ++slot) {
                    if (g_status.buffers.XChar[slot + 2].fOccupied &&
                        &g_status.buffers.Char[slot + 2] == m_character) {
                        selected_slot = slot;
                        break;
                    }
                }
            }
            m_control1->m_control.m_selectionListener = 0;
            m_control1->m_control.SetSelected(selected_slot);
            m_control1->m_control.m_selectionListener = m_control1;
            if (refresh_other) {
                m_character_panel->SetSelectedRow(selection);
            }
        }
    } else {
        m_character = &g_status.buffers.Char[selection + 2];
        if (!m_character->fInParty) {
            m_character = 0;
            selection = -1;
        }
        if (m_mode == W8_PARTY_SELECT_CHARACTERS) {
            int character_index = -1;
            if (selection >= 0 && g_status.buffers.XChar[selection + 2].fOccupied) {
                for (int index = 0;
                     index < g_party_selection_character_collection->characters.GetCount();
                     ++index) {
                    if (g_party_selection_character_collection->GetCharacter(index) ==
                        m_character) {
                        character_index = index;
                        break;
                    }
                }
            }
            m_character_panel->SetSelectedRow(character_index);
        }
        if (refresh_other) {
            m_control1->m_control.m_selectionListener = 0;
            m_control1->m_control.SetSelected(selection);
            m_control1->m_control.m_selectionListener = m_control1;
        }
    }

    m_control2->m_character = m_character;
    m_control2->Invalidate(0);
    wchar_t* text = gppStringList[((!m_character || !m_character->fInParty) ? 0x1b18 : 0x1b1c) / 4];
    m_add_remove_button->m_textBuffer.SetText(text, g_wiz_text_font_secondary);
    bool have_character = m_character != 0;
    m_add_remove_button->SetEnabled(have_character);
    m_add_remove_button->Invalidate(false);
    m_delete_button->SetEnabled(have_character);
    m_delete_button->Invalidate(false);
    m_review_button->SetEnabled(have_character);
    m_review_button->Invalidate(false);
    if (m_character && !m_character->fInParty && FindFreePartySlot(2, 8) == 0xffffffff) {
        m_add_remove_button->SetEnabled(false);
    }
}

/* Dispatch the seven visible party-builder actions.  This is the screen's
   product decision layer: it owns back/finish behavior, edit/create/delete,
   the option-mode progression, and converting an active party back into loose
   character records before an imported party is selected. */
// FUNCTION: WIZ8 0x005c1920
void W8PartySelectionController::OnPrimary(W8TextControl* control)
{
    if (control == m_back_button) {
        if (m_input_handler) {
            OnDecision(m_input_handler, 1);
            return;
        }
        switch (m_mode) {
        case W8_PARTY_SELECT_CHARACTERS:
        case W8_PARTY_SELECT_IMPORT:
            if (CountActiveCharacters() != 0) {
                OpenNotification(gppStringList[0x6d3], true, W8_PARTY_CONFIRM_LEAVE);
                return;
            }
            RequestScreenTransition();
            return;
        case W8_PARTY_SELECT_OPTIONS:
        case W8_PARTY_SELECT_CREATION_NOTICE:
        case W8_PARTY_SELECT_SAVE_NAME:
            SetMode(m_previous_mode);
            SetSelection(0, m_previous_mode == W8_PARTY_SELECT_IMPORT, true);
            return;
        default:
            return;
        }
    }

    if (control == m_add_remove_button) {
        TogglePartyMemberSelection();
        return;
    }
    if (control == m_delete_button) {
        OpenNotification(FormatWideString(L"%s %s %s?", gppStringList[0x6d4], m_character->name,
                                          gppStringList[0x6d5]),
                         true, W8_PARTY_CONFIRM_DELETE_CHARACTER);
        return;
    }
    if (control == m_review_button) {
        int slot;
        for (slot = 0; slot < 6; ++slot) {
            if (g_status.buffers.XChar[slot + 2].fOccupied &&
                &g_status.buffers.Char[slot + 2] == m_character) {
                break;
            }
        }
        g_pending_screen_state.parameter_2 = slot < 6 ? slot + 2 : -1;
        g_pending_screen_state.parameter_3 = m_character;
        SetPendingScreenState(W8_SCREEN_CAMP);
        return;
    }
    if (control == m_create_button) {
        g_pending_screen_state.mode = 0;
        g_pending_screen_state.parameter_3 = 0;
        SetPendingScreenState(W8_SCREEN_CHARACTER);
        return;
    }

    if (control == m_confirm_button) {
        switch (m_mode) {
        case W8_PARTY_SELECT_CHARACTERS:
            if (static_cast<unsigned int>(CountActiveCharacters()) < 6) {
                OpenNotification(FormatWideString(gppStringList[0x6d9], CountActiveCharacters()),
                                 true, W8_PARTY_CONFIRM_PROCEED_TO_OPTIONS);
                return;
            }
            /* fall through */
        case W8_PARTY_SELECT_IMPORT:
            m_previous_mode = m_mode;
            SetSelection(-1, true, true);
            SetMode(W8_PARTY_SELECT_OPTIONS);
            return;
        case W8_PARTY_SELECT_OPTIONS:
            g_settings.difficulty =
                static_cast<W8Difficulty>(m_control3->m_options.m_selectedIndex);
            g_settings.simplified_npc_interaction = static_cast<unsigned char>(
                m_control3->npc_interact_toggle->m_stateFlags & g_W8TextControlStateSecondary);
            if ((m_control3->m_toggle->m_stateFlags & g_W8TextControlStateSecondary) != 0) {
                SetMode(W8_PARTY_SELECT_CREATION_NOTICE);
            } else {
                RunNewGameOpeningSequence(true, 0);
            }
            return;
        case W8_PARTY_SELECT_CREATION_NOTICE:
            SetMode(W8_PARTY_SELECT_SAVE_NAME);
            return;
        case W8_PARTY_SELECT_SAVE_NAME:
            OnDecision(m_input_handler, 0);
            return;
        default:
            return;
        }
    }

    if (control != m_reset_button) {
        return;
    }
    if ((m_reset_button->m_stateFlags & g_W8TextControlStateSecondary) == 0) {
        ResetForNewGame();
        SetMode(W8_PARTY_SELECT_CHARACTERS);
        SetSelection(0, false, true);
        return;
    }
    if (CountActiveCharacters() != 0) {
        OpenNotification(gppStringList[0x6d6], true, W8_PARTY_CONFIRM_IMPORT);
        return;
    }

    W8PartySelectionCharacterCollection* collection = g_party_selection_character_collection;
    for (int index = 0; index < collection->characters.GetCount(); ++index) {
        W8Character* previous = collection->GetCharacter(index);
        if (!previous->fInParty) {
            continue;
        }

        collection->DetachFromParty(index);
    }
    SetMode(W8_PARTY_SELECT_IMPORT);
    LoadImportedPartyFile(m_list->m_selection);
}

// FUNCTION: WIZ8 0x005c1d60
void W8PartySelectionController::OnSelectionChanged(W8PartySelectionListControl*, int selection)
{
    LoadImportedPartyFile(selection);
}

// FUNCTION: WIZ8 0x005c1d70
void W8PartySelectionController::OnDecision(W8PartySelectionInputHandler*, unsigned char accepted)
{
    if (!accepted) {
        wchar_t slot_name[64];
        Get16BitStringFromField(0, slot_name);
        if (SaveSlotFileExists(ConvertWideStringToString(slot_name))) {
            OpenNotification(gppStringList[0x829], true, W8_PARTY_CONFIRM_START_WITH_SAVE_NAME);
            return;
        }
        if (m_input_handler) {
            delete m_input_handler;
        }
        m_input_handler = 0;
        RunNewGameOpeningSequence(true, slot_name);
    } else {
        if (m_input_handler) {
            delete m_input_handler;
        }
        m_input_handler = 0;
        switch (m_mode) {
        case W8_PARTY_SELECT_CHARACTERS:
        case W8_PARTY_SELECT_IMPORT:
            if (CountActiveCharacters() == 0) {
                RequestScreenTransition();
                return;
            }
            OpenNotification(gppStringList[0x6d3], true, W8_PARTY_CONFIRM_LEAVE);
            return;
        case W8_PARTY_SELECT_OPTIONS:
        case W8_PARTY_SELECT_CREATION_NOTICE:
        case W8_PARTY_SELECT_SAVE_NAME:
            SetMode(m_previous_mode);
            SetSelection(0, m_previous_mode == W8_PARTY_SELECT_IMPORT, true);
            return;
        }
    }
}

// FUNCTION: WIZ8 0x005c1ea0
void W8PartySelectionController::OnToggle(int value)
{
    if (static_cast<char>(m_confirm_button->m_enabled) != static_cast<char>(value)) {
        m_confirm_button->SetEnabled(static_cast<unsigned char>(value));
        m_confirm_button->Invalidate(false);
    }
}

// FUNCTION: WIZ8 0x005c1ed0
void W8PartySelectionController::InvalidatePartySelectionComposition()
{
    m_redraw_backdrop = true;
    m_range->Invalidate(0);
    m_control2->Invalidate(0);
    m_control3->Invalidate(0);
    m_character_panel->Invalidate(0);
    m_left_panel->Invalidate(0);
    m_control0->Invalidate(0);
    m_control1->Invalidate(0);
    m_import_panel->Invalidate(0);
    m_bottom_panel->Invalidate(0);
    m_text_buffer->SetGeometryDirty();
}

/* Draw the party-selection composition in owner order.  The one-shot backdrop is
   refreshed only after a mode/dialog change; panel redraws and modal overlay
   dispatch still run every frame. */
// FUNCTION: WIZ8 0x005c1f40
void W8PartySelectionController::DrawPartySelectionComposition()
{
    if (m_redraw_backdrop) {
        DrawCatalogImageAndInvalidate(FRAME_BUFFER, 0xfa, 0, 0, 0, 0, VO_BLT_SRCTRANSPARENCY, 0);
        if (m_mode == W8_PARTY_SELECT_IMPORT) {
            DrawCatalogImage(FRAME_BUFFER, 0x103, 0, 0, 0x140, 0x12e, VO_BLT_SRCTRANSPARENCY, 0);
        }
        m_redraw_backdrop = false;
    }

    m_range->Redraw();
    m_control2->Redraw();
    m_control3->Redraw();
    m_character_panel->Redraw();
    m_left_panel->Redraw();
    m_control1->Redraw();
    m_control0->Redraw();
    m_import_panel->Redraw();
    m_bottom_panel->Redraw();
    m_text_buffer->RenderToTarget(0, false, FRAME_BUFFER);
    if (m_dialog) {
        m_dialog->Draw();
    }
    if (m_input_handler) {
        RenderActiveTextField();
    }
}

/* Install the party-selection confirmation/notification dialog and retain the value
   consumed by ApplyPartySelectionConfirmation after the modal closes. */
// FUNCTION: WIZ8 0x005c25e0
void W8PartySelectionController::OpenNotification(const wchar_t* message, bool allow_cancel,
                                                  W8PartyConfirmationAction action)
{
    m_confirmation_action = action;
    m_dialog = new W8MessageDialogBase;
    if (!m_dialog) {
        return;
    }
    m_dialog->SetOrigin(0xf0, 0xbe);
    m_dialog->SetExtent(0xa0, 100);
    m_dialog->SetBackground("Data\\Dialogs\\DialogBackground.sti", 0);
    m_dialog->SetClientExtent(0xfa, 200);
    m_dialog->SetMessage(const_cast<wchar_t*>(message), 1, 0x32, true, allow_cancel, true, true, 0,
                         0x15e);
    ActivateDialogRegion(0x138);
}

/* Apply the result of a party-selection confirmation dialog.  The dialog value selects
   delete, save-slot creation, option entry, imported-party replacement, or
   leaving the screen; cancellation only restores the mode-4 toggle. */
// FUNCTION: WIZ8 0x005c26c0
void W8PartySelectionController::ApplyPartySelectionConfirmation(W8PartyConfirmationAction,
                                                                 bool accepted)
{
    if (!accepted) {
        if (m_confirmation_action == W8_PARTY_CONFIRM_IMPORT) {
            m_reset_button->DisableSecondaryState(false);
        }
        return;
    }

    W8PartySelectionCharacterCollection* collection = g_party_selection_character_collection;
    switch (m_confirmation_action) {
    case W8_PARTY_CONFIRM_DELETE_CHARACTER: {
        int selected = m_character_panel->m_selected_row;
        W8Character* character = collection->GetCharacter(selected);
        if (character->fInParty) {
            collection->DetachFromParty(selected);
            character = collection->GetCharacter(selected);
        }
        char path[128];
        BuildCharacterPath(path, character->name, -1);
        FileDelete(path);
        collection->DeleteAt(selected);

        m_character_panel->m_range = m_range;
        m_range->m_listener = m_character_panel;
        int maximum = collection->characters.GetCount() - 6;
        if (maximum < 0) {
            maximum = 0;
        }
        m_range->SetRange(0, maximum);
        m_range->SetRangeEnabled(maximum > 0);
        m_range->Invalidate(0);
        if (selected > 0 || collection->characters.GetCount() == 0) {
            --selected;
        }
        SetSelection(selected, false, true);
        return;
    }
    case W8_PARTY_CONFIRM_START_WITH_SAVE_NAME: {
        wchar_t slot_name[64];
        Get16BitStringFromField(0, slot_name);
        if (m_input_handler) {
            delete m_input_handler;
        }
        m_input_handler = 0;
        RunNewGameOpeningSequence(true, slot_name);
        return;
    }
    case W8_PARTY_CONFIRM_PROCEED_TO_OPTIONS:
        m_previous_mode = m_mode;
        SetSelection(-1, true, true);
        SetMode(W8_PARTY_SELECT_OPTIONS);
        return;
    case W8_PARTY_CONFIRM_IMPORT: {
        for (int index = 0; index < collection->characters.GetCount(); ++index) {
            W8Character* previous = collection->GetCharacter(index);
            if (!previous->fInParty) {
                continue;
            }
            collection->DetachFromParty(index);
        }
        SetMode(W8_PARTY_SELECT_IMPORT);
        LoadImportedPartyFile(m_list->m_selection);
        return;
    }
    case W8_PARTY_CONFIRM_LEAVE:
        RequestScreenTransition();
        return;
    default:
        break;
    }
}

/* Load the selected imported-party file, report its two failure classes, and
   refresh every control whose state depends on the resulting six party slots. */
// FUNCTION: WIZ8 0x005c2c60
void W8PartySelectionController::LoadImportedPartyFile(int selection)
{
    ResetForNewGame();
    W8PartySelectionCharacterCollection* collection = g_party_selection_character_collection;
    if (selection >= 0 && selection < collection->names.GetCount()) {
        char path[128];
        sprintf(path, "%s\\%s", "Saves\\Import", *collection->names.GetAt(selection));
        int result = ImportWizardry7Party(path);
        if (result != 0) {
            ResetForNewGame();
            OpenNotification(gppStringList[(result == 2 ? 0x1b60 : 0x1b5c) / 4], false,
                             W8_PARTY_CONFIRM_NONE);
        }
    }

    m_confirm_button->SetEnabled(CountActiveCharacters() != 0);
    m_confirm_button->Invalidate(false);
    int slot;
    for (slot = 0; slot < 6; ++slot) {
        bool occupied = g_status.buffers.XChar[slot + 2].fOccupied;
        m_control1->m_control.m_lsButtons[slot]->SetEnabled(occupied);
    }
    m_control1->Invalidate(0);
    for (slot = 0; slot < 6; ++slot) {
        bool occupied = g_status.buffers.XChar[slot + 2].fOccupied;
        m_control0->ControlAt(slot)->SetActive(m_mode == W8_PARTY_SELECT_IMPORT && occupied &&
                                               IsCharacterReadyToAdvance(slot + 2));
    }
    m_control0->Invalidate(0);
    SetSelection(0, true, true);
}

/* Toggle the selected loose character into the active party, or detach an
   active member back into an owned loose record.  The selection then follows
   the nearest remaining active slot and both six-row panels are refreshed. */
// FUNCTION: WIZ8 0x005c2970
void W8PartySelectionController::TogglePartyMemberSelection()
{
    int selected = m_character_panel->m_selected_row;
    if (selected == -1) {
        return;
    }

    W8PartySelectionCharacterCollection* collection = g_party_selection_character_collection;
    W8Character* character = collection->GetCharacter(selected);
    if (!character->fInParty) {
        int slot = AddCharacterToParty(character, -1);
        if (slot != -1) {
            delete character;
            collection->characters.SetAt(selected, &g_status.buffers.Char[slot]);
        }
        if (selected < collection->characters.GetCount() - 1) {
            ++selected;
        }
        SetSelection(selected, false, true);
    } else {
        int previous_slot = collection->FindPartySlot(selected);
        collection->DetachFromParty(selected);

        int next_slot = previous_slot + 1;
        bool found = false;
        while (next_slot != previous_slot) {
            if (next_slot > 5) {
                next_slot = 0;
                if (previous_slot == 0) {
                    break;
                }
            }
            if (g_status.buffers.XChar[next_slot + 2].fOccupied) {
                found = true;
                break;
            }
            ++next_slot;
        }
        if (found) {
            SetSelection(next_slot, true, true);
        } else {
            SetSelection(selected, false, false);
        }
    }

    for (int slot = 0; slot < 6; ++slot) {
        bool occupied = g_status.buffers.XChar[slot + 2].fOccupied;
        m_control1->m_control.m_lsButtons[slot]->SetEnabled(occupied);
    }
    m_control1->Invalidate(0);
    m_character_panel->Invalidate(0);
    m_confirm_button->SetEnabled(CountActiveCharacters() != 0);
    m_confirm_button->Invalidate(false);
}

/* Enter the party-selection party builder.  The persistent collection is rebuilt from
   the six occupied player slots whenever this is a fresh entry or a return
   from state 3 that is not preserving mode 1.  Loose CHR files are then merged
   and ordered before the controller presents them. */
// FUNCTION: WIZ8 0x005c2de0
unsigned char PartySelectionScreenEnter(void)
{
    SetViewport(0, 0, 0x280, 0x1e0);
    SetPrimarySurfaceTextureHint2Enabled(false);
    MSYS_Init();
    ResetRegions();
    UpdateHeldItemCursor();
    SetFontObjectPalette16BPP(g_wiz_text_font_secondary, g_wiz_text_font_secondary_palette);
    SetFontObjectPalette16BPP(g_wiz_text_bold_font, g_font_palette_wiz_text_bold);

    W8PartySelectionCharacterCollection* collection = g_party_selection_character_collection;
    if (!collection) {
        ResetForNewGame();
        collection = new W8PartySelectionCharacterCollection;
        g_party_selection_character_collection = collection;

        collection->ReloadCharacters();

        g_party_selection_controller = new W8PartySelectionController;
        g_party_selection_controller->Setup();
    } else {
        if (g_previous_screen_id == W8_SCREEN_CHARACTER &&
            g_party_selection_controller->m_mode != W8_PARTY_SELECT_IMPORT) {
            collection->ReloadCharacters();
            g_party_selection_controller->SetSelection(0, false, true);
        }
        g_party_selection_controller->SetMode(g_party_selection_controller->m_mode);
    }
    StartMusicResource("MainMenu.MPL", 1, 1);
    return 1;
}

/* The state persists its controller and imported-character collection while
   another screen is temporarily stacked over it.  A leaving tick destroys
   both; an ordinary tick only performs the common display/region reset. */
// FUNCTION: WIZ8 0x005c30b0
unsigned char PartySelectionScreenLeave(int leaving)
{
    if (leaving) {
        W8PartySelectionCharacterCollection* collection = g_party_selection_character_collection;
        if (collection) {
            delete collection;
        }
        W8PartySelectionController* controller = g_party_selection_controller;
        g_party_selection_character_collection = 0;
        if (controller) {
            delete controller;
        }
        g_party_selection_controller = 0;
    }
    NoOp();
    MSYS_Shutdown();
    ResetRegions();
    return 1;
}

/* Run modal close-out, cursor/region dispatch and the party-selection keyboard layer.
   Region handling gets first refusal.  The optional party-selection input handler gets
   second refusal, after which the screen owns Return, Escape, left/right row
   movement and Delete. */
// FUNCTION: WIZ8 0x005c3120
void PartySelectionScreenFrame(void)
{
    POINT point;
    POINT current;
    InputAtom input;

    if (g_dev_mode) {
        RequestExitScreen();
    }
    SGPMouseGetPos(&point);
    W8PartySelectionController* controller = g_party_selection_controller;
    if (controller->m_dialog) {
        controller->m_dialog->ProcessInput();
        if (!controller->m_dialog->is_open) {
            bool result = controller->m_dialog->accepted;
            delete controller->m_dialog;
            controller->m_dialog = 0;
            ClearActiveRegionIfMatches(0x138);
            controller->InvalidatePartySelectionComposition();
            controller->ApplyPartySelectionConfirmation(controller->m_confirmation_action, result);
        }
    }
    if (controller->m_input_handler) {
        SGPMouseGetPos(&current);
        MSYS_SGP_Mouse_Handler_Hook(MOUSE_POS, static_cast<unsigned short>(current.x),
                                    static_cast<unsigned short>(current.y), gfLeftButtonState,
                                    gfRightButtonState);
    }
    UpdateRegionMousePosition(point.x, point.y);
    while (DequeueEvent(&input) == 1) {
        if (!DispatchRegionInput(&input) &&
            (!controller->m_input_handler || !controller->m_input_handler->HandleInput(&input)) &&
            (input.usEvent == KEY_DOWN || input.usEvent == KEY_REPEAT)) {
            switch (input.usParam) {
            case VK_RETURN:
                controller->TogglePartyMemberSelection();
                break;
            case VK_ESCAPE:
                switch (controller->m_mode) {
                case W8_PARTY_SELECT_CHARACTERS:
                case W8_PARTY_SELECT_IMPORT:
                    if (CountActiveCharacters() == 0) {
                        RequestScreenTransition();
                    } else {
                        controller->OpenNotification(gppStringList[0x6d3], true,
                                                     W8_PARTY_CONFIRM_LEAVE);
                    }
                    break;
                case W8_PARTY_SELECT_OPTIONS:
                case W8_PARTY_SELECT_CREATION_NOTICE:
                case W8_PARTY_SELECT_SAVE_NAME:
                    controller->SetMode(controller->m_previous_mode);
                    controller->SetSelection(
                        0, controller->m_previous_mode == W8_PARTY_SELECT_IMPORT, true);
                    break;
                }
                break;
            case VK_UP:
                if (controller->m_mode == W8_PARTY_SELECT_CHARACTERS &&
                    controller->m_character_panel->m_selected_row > 0) {
                    controller->SetSelection(controller->m_character_panel->m_selected_row - 1,
                                             false, true);
                }
                break;
            case VK_DOWN:
                if (controller->m_mode == W8_PARTY_SELECT_CHARACTERS &&
                    controller->m_character_panel->m_selected_row <
                        g_party_selection_character_collection->characters.GetCount() - 1) {
                    controller->SetSelection(controller->m_character_panel->m_selected_row + 1,
                                             false, true);
                }
                break;
            case VK_DELETE:
                if (controller->m_mode == W8_PARTY_SELECT_CHARACTERS && controller->m_character) {
                    controller->OnPrimary(controller->m_delete_button);
                }
                break;
            }
        }
    }
    UpdateCharacterEventState();
    controller->DrawPartySelectionComposition();
    RenderFrame();
}

/* Lifecycle record 2's frame close-out. The record's other slots only report
   success. Every path asks for a screen
   transition, records one of four codes and queues screen 0, so the record is a
   pure router - it selects which of the four the transition reports and then
   leaves. Nothing here names the four codes or the two globals that pick them. */
// FUNCTION: WIZ8 0x005c3800
void GameStartRouterFrame(void)
{
    w8_ulong code;

    RequestScreenTransition();
    if (!g_status.skip_loose_character_check) {
        code = 4;
    } else {
        switch (g_wiz7_ending) {
        case 1:
            code = 2;
            break;
        case 2:
            code = 3;
            break;
        default:
            code = 1;
            break;
        }
    }
    SetIntroVideoIndex(code);
    SetPendingScreenState(W8_SCREEN_INTRO);
}

/* Lifecycle record 2 has nothing to release. Retail's linker folded this body
   into ScreenLifecycleSuccess, which compiles to the same bytes. */
unsigned char GameStartRouterLeave(int leaving)
{
    return 1;
}

/* One byte per portrait; the retail table is one for every portrait. */
// GLOBAL: WIZ8 0x0061cbc0
unsigned char g_portrait_frame_flags[0x50] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};
