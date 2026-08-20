#include "wiz8/gameplay_boundaries.h"
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/combat_state.h"
#include "wiz8/dialog_base.h"
#include "wiz8/dirty_tiles.h"
#include "wiz8/game_state.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/music_playlist.h"
#include "wiz8/regions.h"
#include "wiz8/screen_state.h"
#include "wiz8/vector.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/virtual_file.h"
#include "FileMan.h"
#include "Font.h"
#include "input.h"
#include "vsurface.h"

#include <stdio.h>
#include <string.h>

/* Address quarantine 005bc811-005c433f; bounds come from adjacent
   assertion-backed original translation-unit intervals.

   This unit declares the three functions it calls itself rather than reaching
   for quarantine_common.h: nothing here needs a type, and that header is a
   mechanism the repository drains rather than grows. */

void RequestScreenTransition(void);
void SetPendingScreenState(int value);
void SetValue64D8AC(unsigned long value);
void Function54B250(unsigned char notify, void* target);
void GetSaveSlotName005D3CC0(int slot, wchar_t* name);
unsigned char SaveSlotFileExists(const char* slot_name);
int CountActiveCharacters(void);
bool IsCharacterReadyToAdvance(int party_slot);
unsigned int FindFreePartySlot(unsigned int first, unsigned int last);
char* ConvertWideStringToString(const wchar_t* string);
void GetScreenPoint004284F0(W8ScreenPoint* point);
unsigned char SetValue5FF5F0(int font);
unsigned char LoadCharacter(const char* name, W8Character* character, int slot,
                            char report_failure);
void BuildCharacterFilePath00514FA0(char* destination, const char* filename,
                                    int slot);
void BuildCharacterPath00514EC0(char* destination, const wchar_t* name,
                                int slot);
extern "C" void SetViewport(int left, int top, int right, int bottom);
extern "C" int Function40B290(void);
extern "C" void NoOp(void);
extern "C" void ShutdownDisplayList(void);
void ResetRegions(void);
extern "C" void UpdateHeldItemCursor(void);
void Function591780(void);
void Function40B510(unsigned short event, unsigned short x, unsigned short y,
                    char right_button, char left_button);
void Function4F1360(int x, int y);
unsigned char DispatchScreenInput004F1910(const void* event);
int Function52E750(void);
void Function426790(void);
void Function5D5390(void);
unsigned int Function568950(const InputAtom* input);
unsigned short Function402780(unsigned short key, unsigned char modifiers);
unsigned int Function5D3F50(const InputAtom* input);
unsigned char Function5D3D00(int index);
void Function55EE70(int value);
void Function5D3520(int value);
char Function5D39B0(int left, int top, int width, int height, int colour,
                    const wchar_t* text, unsigned char capacity,
                    short input_type, unsigned char enabled);
void Function5D3D20(char index);
void Function5D3B40(int index);
void Function5D3800(void);
void RenderPartyPortrait0052EB00(int portrait, int left, int top,
                                int flags, int value, int party_slot);
void Function4EF610(int party_slot, int value);
int Function4EF4A0(W8Character* character, int slot);
void DeleteFileByName(const char* path);
int Function558C40(const char* path);

extern "C" int g_font_683660;
extern "C" int g_wiz_text_bold_font_683664;
extern "C" unsigned short* g_colour_68ee08;
extern "C" unsigned short* g_font_palette_wiz_text_bold_68ee0c;
extern "C" unsigned short* g_font_state_palettes_68ee1c[15];
extern "C" int g_dword_647bc0;
extern unsigned char g_flag_689b32;
extern unsigned char g_flag_6f04e8;
extern unsigned char g_flag_6f04ed;
extern "C" HVOBJECT g_wiz_text_font_secondary_object_683680;
extern "C" int g_options_title_font_68368c;
extern "C" int g_options_detail_font_683614;
extern unsigned short g_profession_name_message_ids_61e3f0[];
extern unsigned short g_race_name_message_ids_61e3d0[];
extern unsigned short g_faction_name_message_rows_61e430[][4];
extern unsigned short g_personality_message_ids_61e674[];
extern int g_portrait_render_modes_6483dc[][4];

/* Two ordinary growable vectors and the scroll origin account for all 0x24
   bytes allocated at state-5 entry. The second vector supplies the names this
   control renders; the first owns the corresponding party records. */
struct W8State5PartyCollection {
    __forceinline W8State5PartyCollection()
        : characters(5), names(), first_visible(0)
    {
    }
    ~W8State5PartyCollection();
    W8Character* GetCharacter(int index);
    int FindPartySlot(int index);
    void DetachFromParty(int index);
    void DeleteAt(int index);
    void LoadExternalCharacters();
    void SortCharactersByWriteTime();

    W8GrowableVector<W8Character*> characters;
    W8GrowableVector<char*> names;
    int first_visible;
};
WIZ8_ASSERT_SIZE(W8State5PartyCollection, 0x24);

// GLOBAL: WIZ8 0x0069C4EC
W8State5PartyCollection* g_state5_party_collection_69c4ec;

// GLOBAL: WIZ8 0x0069C4F0
unsigned int g_state5_character_region_set_69c4f0;

// GLOBAL: WIZ8 0x0069C4F4
unsigned int g_state5_party_slot_region_set_69c4f4;

// GLOBAL: WIZ8 0x0069C4F8
unsigned int g_state5_six_text_region_set_69c4f8;

/* Imported characters not installed in the active party are owned here. Name
   strings are separately owned by the second vector. Clearing the counts
   before the two ordinary vector destructors preserves their storage teardown
   without asking the vector template to own its pointer elements. */
// FUNCTION: WIZ8 0x005be270
W8State5PartyCollection::~W8State5PartyCollection()
{
    int index;
    for (index = 0; index < characters.count; ++index) {
        W8Character* character = GetCharacter(index);
        if (!character->in_party) {
            delete character;
        }
    }
    characters.count = 0;
    for (index = 0; index < names.count; ++index) {
        delete names.data[index];
    }
    names.count = 0;
}

// FUNCTION: WIZ8 0x005be4b0
W8Character* W8State5PartyCollection::GetCharacter(int index)
{
    if (index >= 0 && index < characters.count) {
        return characters.data[index];
    }
    return 0;
}

// FUNCTION: WIZ8 0x005be4d0
void W8State5PartyCollection::DetachFromParty(int index)
{
    W8Character* previous = GetCharacter(index);
    int slot = FindPartySlot(index);
    W8Character* replacement = new W8Character;
    char path[128];
    BuildCharacterPath00514EC0(path, previous->name, -1);
    if (!LoadCharacter(path, replacement, -1, 0)) {
        memcpy(replacement, previous, sizeof(W8Character));
    }
    Function4EF610(slot + 2, 0);
    replacement->in_party = 0;
    characters.SetAt(index, replacement);
}

// FUNCTION: WIZ8 0x005be5f0
int W8State5PartyCollection::FindPartySlot(int index)
{
    W8Character* character = GetCharacter(index);
    if (character && character->in_party) {
        unsigned char* occupied =
            static_cast<unsigned char*>(g_status_685170.buffers.buffer_08);
        for (int slot = 0; slot < 6; ++slot) {
            if (occupied[slot * 0x106 + 0x20c] &&
                &g_party_characters[slot + 2] == character) {
                return slot;
            }
        }
    }
    return -1;
}

// FUNCTION: WIZ8 0x005c34d0
void W8State5PartyCollection::DeleteAt(int index)
{
    if (index >= 0 && index < characters.count) {
        delete characters.RemoveAt(index);
    }
    else {
        delete static_cast<W8Character*>(0);
    }
}

/* Enumerate loose CHR files, loading only characters that are not already in
   an occupied party slot.  The collection owns every record accepted here;
   records rejected by loading or duplicate-name detection are destroyed
   immediately. */
// FUNCTION: WIZ8 0x005be340
void W8State5PartyCollection::LoadExternalCharacters()
{
    char search_path[128];
    GETFILESTRUCT find;

    BuildCharacterFilePath00514FA0(search_path, FormatString("*.%s", "CHR", -1),
                                   -1);
    BOOLEAN found = GetFileFirst(search_path, &find);
    for (;;) {
        if (!found) {
            return;
        }
        W8Character* character = new W8Character;
        if (!LoadCharacter(find.zFileName, character, -1, 0)) {
            delete character;
        }
        else {
            int slot;
            for (slot = 2; slot < 8; ++slot) {
                unsigned char* party_state =
                    static_cast<unsigned char*>(g_status_685170.buffers.buffer_08);
                if (party_state[slot * 0x106] != 0 &&
                    wcscmp(g_party_characters[slot].name, character->name) == 0) {
                    break;
                }
            }
            if (slot < 8) {
                delete character;
            }
            else {
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
static void SortState5CharactersByTime005C3520(
    W8Character** characters, SGP_FILETIME* times, int first, int last)
{
    if (last - first < 9) {
        for (int next = first + 1; next <= last; ++next) {
            SGP_FILETIME time = times[next];
            W8Character* character = characters[next];
            int insert = next;
            while (insert > first &&
                   CompareSGPFileTimes(&times[insert - 1], &time) < 0) {
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
        } while (right > first &&
                 CompareSGPFileTimes(&times[right], &pivot) < 0);
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
        SortState5CharactersByTime005C3520(
            characters, times, first, left - 1);
    }
    if (left + 1 < last) {
        SortState5CharactersByTime005C3520(
            characters, times, left + 1, last);
    }
}

// FUNCTION: WIZ8 0x005be650
void W8State5PartyCollection::SortCharactersByWriteTime()
{
    if (characters.count <= 1) {
        return;
    }

    SGP_FILETIME* times = new SGP_FILETIME[characters.count];
    memset(times, 0, characters.count * sizeof(SGP_FILETIME));
    for (int index = 0; index < characters.count; ++index) {
        char path[128];
        BuildCharacterPath00514EC0(path, characters.data[index]->name, -1);
        int handle = FileOpen(path, FILE_ACCESS_READ, 0);
        if (handle) {
            SGP_FILETIME creation;
            SGP_FILETIME access;
            GetFileManFileTime(handle, &creation, &access, &times[index]);
            CloseVirtualFile(handle);
        }
    }
    SortState5CharactersByTime005C3520(
        characters.data, times, 0, characters.count - 1);
    delete[] times;
}

class W8State5ListControl005EF464;

class W8State5ListSelectionListener005EF4C8 {
public:
    virtual void OnSelectionChanged(W8State5ListControl005EF464* control,
                                    int selection) = 0;
};

/* State 5's scrolling party-name list. Its secondary vtable is the existing
   range callback; its own listener is the controller's independently observed
   +4 callback subobject. */
// VTABLE: WIZ8 0x005ef464
class W8State5ListControl005EF464 : public W8WidgetBase005ED5BC,
                                    public W8RangeListener {
public:
    __forceinline W8State5ListControl005EF464(
        Controls* panel, unsigned int region,
        int left, int top, int right, int bottom)
        : W8WidgetBase005ED5BC(panel, region, left, top, right, bottom),
          m_visible_rows(0x11), m_selection(0), m_hovered(-1),
          m_first_visible(0), m_listener(0)
    {
    }

    virtual ~W8State5ListControl005EF464() override;
    virtual void Redraw(int full_redraw) override;
    virtual void Function4E00(int event) override;
    virtual void FunctionSlot09(int event) override;
    virtual void Function50C0(int event) override;
    virtual void OnRangeChanged(W8RangeControl005ED74C* control) override;

    int m_visible_rows;                  /* 0x38 */
    int m_selection;                     /* 0x3c */
    int m_hovered;                       /* 0x40 */
    int m_first_visible;                 /* 0x44 */
    W8State5ListSelectionListener005EF4C8* m_listener; /* 0x48 */
};
WIZ8_ASSERT_SIZE(W8State5ListControl005EF464, 0x4c);

// SYNTHETIC: WIZ8 0x005bff20
// W8State5ListControl005EF464::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005bff40
W8State5ListControl005EF464::~W8State5ListControl005EF464()
{
}

// FUNCTION: WIZ8 0x005bff60
void W8State5ListControl005EF464::Redraw(int full_redraw)
{
    if (m_flag_5 && (m_flag_6 || full_redraw)) {
        int left = m_pPanel->origin_x + m_left;
        int top = m_pPanel->origin_y + m_top;
        int right = m_pPanel->origin_x + m_right;
        int bottom = m_pPanel->origin_y + m_bottom;

        MarkScreenRectDirty(left, top, right, bottom, 0);
        Function5497C0(-14, left, top, right, bottom, 0x1b6, 0, 0);
        SetFontDestBuffer(-14, left, top, right, bottom, 0);

        int end = m_first_visible + m_visible_rows;
        if (g_state5_party_collection_69c4ec->names.count <= end) {
            end = g_state5_party_collection_69c4ec->names.count;
        }
        top += 1;
        SetValue5FF5F0(g_font_683660);
        for (int row = m_first_visible; row < end; ++row) {
            unsigned short* colour = g_font_state_palettes_68ee1c[3];
            if (row != m_selection) {
                colour = g_colour_68ee08;
                if (row == m_hovered) {
                    colour = g_font_state_palettes_68ee1c[5];
                }
            }
            SetFontObjectPalette16BPP(g_font_683660, colour);
            mprintf(left + 2, top, L"%S",
                    *g_state5_party_collection_69c4ec->names.GetAt(row));
            top += 0x0e;
        }
        SetFontObjectPalette16BPP(g_font_683660, g_colour_68ee08);
        SetFontDestBuffer(-14, 0, 0, 0x280, 0x1e0, 0);
        m_flag_6 = 0;
    }
}

// FUNCTION: WIZ8 0x005c00c0
void W8State5ListControl005EF464::Function4E00(int event)
{
    m_hovered = -1;
    Invalidate((unsigned char)event);
}

// FUNCTION: WIZ8 0x005c00e0
void W8State5ListControl005EF464::FunctionSlot09(int)
{
    W8ScreenPoint point;
    GetScreenPoint004284F0(&point);
    int hovered = (point.x - m_pPanel->origin_y - m_top) / 0x0e
                  + m_first_visible;
    if (hovered != m_hovered) {
        m_hovered = hovered;
        Invalidate(0);
    }
}

// FUNCTION: WIZ8 0x005c0140
void W8State5ListControl005EF464::Function50C0(int event)
{
    W8ScreenPoint point;
    GetScreenPoint004284F0(&point);
    int selection = (point.y - m_pPanel->origin_y - m_top) / 0x0e
                    + m_first_visible;
    if (selection != m_selection) {
        m_selection = selection;
        Invalidate((unsigned char)event);
        if (m_listener) {
            m_listener->OnSelectionChanged(this, m_selection);
        }
    }
}

// FUNCTION: WIZ8 0x005c01b0
void W8State5ListControl005EF464::OnRangeChanged(
    W8RangeControl005ED74C* control)
{
    m_first_visible = control->m_value;
    Invalidate(0);
}

class W8State5DecisionListener005EF4C0 {
public:
    virtual void OnDecision(int value, unsigned char accepted) = 0;
    virtual void OnToggle(int value) = 0;
};

class W8State5PanelSelectionListener005EF3B4 {
public:
    virtual void Function5BF0C0(W8TextControl005ED604* entry) = 0;
    virtual void Function5BF050(W8TextControl005ED604* entry) = 0;
    virtual void Function5BF0E0(int amount) = 0;
};

/* Each visible character row is the same text control that W8Control stores
   and selects. The three added dwords are the visible row, absolute character
   index, and the panel callback used for row/scroll actions. */
class W8State5CharacterRow005EF364 : public W8TextControl005ED604 {
public:
    W8State5CharacterRow005EF364(Controls* panel, int top, int row);
    virtual ~W8State5CharacterRow005EF364() override;
    virtual void Redraw(int full_redraw) override;
    virtual void AdjustValue(int amount) override;
    virtual void Function5290(int event) override;
    virtual void InvokeBlurCallback(int event) override;

    int m_row;
    int m_character_index;
    W8State5PanelSelectionListener005EF3B4* m_selection_listener;
};
WIZ8_ASSERT_SIZE(W8State5CharacterRow005EF364, 0xc4);

class W8State5InputHandler005C0E50 {
public:
    __forceinline W8State5InputHandler005C0E50() {}
    virtual ~W8State5InputHandler005C0E50();
    unsigned char HandleInput(const InputAtom* input);

    W8State5DecisionListener005EF4C0* m_listener;
};
WIZ8_ASSERT_SIZE(W8State5InputHandler005C0E50, 8);

class W8State5Controller005EF4CC;

/* The 0x84-byte character panel is an ordinary multiple-inheritance object:
   Controls owns its six row widgets, W8Control owns their selection vector,
   and the three intervening callback bases account exactly for +0x4c, +0x50
   and +0x54.  The selected absolute row at +0x80 is read directly by the
   state-5 keyboard handler. */
class W8State5CharacterPanel005EF3C8
    : public Controls,
      public W8ControlSelectionListener,
      public W8RangeListener,
      public W8State5PanelSelectionListener005EF3B4,
      public W8Control005ED654 {
public:
    W8State5CharacterPanel005EF3C8();
    virtual ~W8State5CharacterPanel005EF3C8();
    virtual void vslot00(W8Control005ED654* control, int selected) override;
    virtual void OnRangeChanged(W8RangeControl005ED74C* control) override;
    virtual void Function5BF0C0(W8TextControl005ED604* entry) override;
    virtual void Function5BF050(W8TextControl005ED604* entry) override;
    virtual void Function5BF0E0(int amount) override;
    void SetSelectedRow(int selection);

    W8RangeControl005ED74C* m_range_7c;
    int m_selected_row;
};
WIZ8_ASSERT_SIZE(W8State5CharacterPanel005EF3C8, 0x84);

class W8State5SixTextPanel005EF450
    : public Controls,
      public W8TextControl005ED604::Listener {
public:
    W8State5SixTextPanel005EF450();
    virtual ~W8State5SixTextPanel005EF450();
    virtual void OnPrimary(W8TextControl005ED604* control) override;
    virtual void OnSecondary(W8TextControl005ED604*) override {}
};
WIZ8_ASSERT_SIZE(W8State5SixTextPanel005EF450, 0x50);

class W8State5PartySlotRow005EF3E4 : public W8TextControl005ED604 {
public:
    W8State5PartySlotRow005EF3E4(Controls* panel, int row);
    virtual ~W8State5PartySlotRow005EF3E4() override;
    virtual void Redraw(int full_redraw) override;
    virtual void Function5290(int event) override;
    virtual void InvokeBlurCallback(int event) override;

    int m_row;
    W8WidgetBase005ED5BC* m_redraw_partner;
};
WIZ8_ASSERT_SIZE(W8State5PartySlotRow005EF3E4, 0xc0);

class W8State5PartySlotPanel005EF438
    : public Controls,
      public W8ControlSelectionListener,
      public W8Control005ED654 {
public:
    friend class W8State5Controller005EF4CC;

    W8State5PartySlotPanel005EF438();
    virtual ~W8State5PartySlotPanel005EF438();
    virtual void vslot00(W8Control005ED654* control, int selected) override;
};
WIZ8_ASSERT_SIZE(W8State5PartySlotPanel005EF438, 0x74);

class W8State5PlainPanel005EF4E0 : public Controls {
public:
    __forceinline W8State5PlainPanel005EF4E0() : Controls(), m_character_4c(0) {}
    virtual ~W8State5PlainPanel005EF4E0();
    virtual void Redraw() override;

    W8Character* m_character_4c;
};
WIZ8_ASSERT_SIZE(W8State5PlainPanel005EF4E0, 0x50);

/* Unlike the two selection panels above, the option panel does not inherit
   W8Control.  Its constructor writes a mode word at +0x4c and constructs a
   complete W8Control member at +0x50.  The two toggle controls and the owned
   option-entry vector follow that member at the observed offsets. */
class W8State5OptionPanel005EF4AC : public Controls {
public:
    W8State5OptionPanel005EF4AC();
    virtual ~W8State5OptionPanel005EF4AC();
    virtual void Redraw() override;
    void Function5C05F0(int mode);

    int m_mode_4c;
    W8Control005ED654 m_options_50;
    W8TextControl005ED604* m_toggle_74;
    W8TextControl005ED604* m_toggle_78;
    W8GrowableVector<W8TextBuffer005ED5B8*> m_entries_7c;
    int m_render_left_8c;
    int m_render_top_90;
    short m_image_width_94;
    short m_image_height_96;
};
WIZ8_ASSERT_SIZE(W8State5OptionPanel005EF4AC, 0x98);

/* The state-5 owner is established by three construction-phase abstract
   vtables, seven registered text controls, the list-selection callback above,
   and independently observed owned state through +0x6c. The address-qualified
   name does not claim a source-era screen name. */
// VTABLE: WIZ8 0x005ef4cc
class W8State5Controller005EF4CC
    : public W8TextControl005ED604::Listener,
      public W8State5ListSelectionListener005EF4C8,
      public W8State5DecisionListener005EF4C0 {
public:
    __forceinline W8State5Controller005EF4CC()
        : m_character_18(0), m_input_handler_64(0), m_dialog_68(0)
    {
    }
    ~W8State5Controller005EF4CC();

    virtual void OnPrimary(W8TextControl005ED604* control) override;
    virtual void OnSecondary(W8TextControl005ED604*) override {}
    virtual void OnSelectionChanged(W8State5ListControl005EF464* control,
                                    int selection) override;
    virtual void OnDecision(int value, unsigned char accepted) override;
    virtual void OnToggle(int value) override;

    void SetMode(int mode);
    void SetSelection(int selection, int highlighted, int refresh);
    void OpenNotification(const wchar_t* message, int kind, int value);
    void Setup();
    void Function5C1ED0();
    void Function5C1F40();
    void Function5C26C0(int value, unsigned char result);
    void Function5C2C60(int selection);
    void Function5C2970();

    int m_mode;                          /* 0x0c */
    int m_previous_mode;                 /* 0x10 */
    unsigned char m_redraw_backdrop_14;
    unsigned char pad_15[3];
    W8Character* m_character_18;
    W8RangeControl005ED74C* m_range;     /* 0x1c */
    W8State5CharacterPanel005EF3C8* m_character_panel_20;
    W8State5SixTextPanel005EF450* m_control_24;
    W8State5PartySlotPanel005EF438* m_control_28;
    W8State5PlainPanel005EF4E0* m_control_2c;
    W8State5OptionPanel005EF4AC* m_control_30;
    Controls* m_panel_34;
    Controls* m_panel_38;
    Controls* m_panel_3c;
    W8TextControl005ED604* m_text_40;
    W8TextControl005ED604* m_text_44;
    W8TextControl005ED604* m_text_48;
    W8TextControl005ED604* m_text_4c;
    W8TextControl005ED604* m_text_50;
    W8TextControl005ED604* m_text_54;
    W8TextControl005ED604* m_text_58;
    W8State5ListControl005EF464* m_list_5c;
    W8TextBuffer005ED5B8* m_text_buffer_60;
    W8State5InputHandler005C0E50* m_input_handler_64;
    W8ModalDialogBase* m_dialog_68;
    int m_dialog_value_6c;
};
WIZ8_ASSERT_SIZE(W8State5Controller005EF4CC, 0x70);

// GLOBAL: WIZ8 0x0069C4E8
W8State5Controller005EF4CC* g_state5_controller_69c4e8;

W8State5CharacterRow005EF364::W8State5CharacterRow005EF364(
    Controls* panel, int top, int row)
    : W8TextControl005ED604(panel, 0xffffffff, 0, top, 0, 0,
                           0xfb, 0, 0, 1, 2, 1, -1),
      m_row(row),
      m_character_index(0),
      m_selection_listener(0)
{
    AddLayoutFlags(0x11);
    m_character_index = g_state5_party_collection_69c4ec->first_visible + m_row;
    SetEnabled(m_character_index <
               g_state5_party_collection_69c4ec->characters.count);
    Invalidate(0);
}

// FUNCTION: WIZ8 0x005be950
W8State5CharacterRow005EF364::~W8State5CharacterRow005EF364()
{
}

// FUNCTION: WIZ8 0x005be9b0
void W8State5CharacterRow005EF364::Redraw(int full_redraw)
{
    if (!m_flag_5 || (!(unsigned char)full_redraw && !m_flag_6)) {
        return;
    }

    W8TextControl005ED604::Redraw(full_redraw);
    W8Character* character =
        g_state5_party_collection_69c4ec->GetCharacter(m_character_index);
    int left = m_pPanel->origin_x + m_left;
    int top = m_pPanel->origin_y + m_top;
    Function548F90(-14, 0x13, character->table_value_0079, 0,
                   left + 2, top + 2, 2, 0);
    if (character->in_party) {
        ShadowVideoSurfaceRect(-14, left + 2, top + 2,
                               left + 0x2e, top + 0x25);
        SetObjectShade(g_wiz_text_font_secondary_object_683680, 6);
    }

    left += 0x36;
    SetValue5FF5F0(g_font_683660);
    mprintf(left, top + 4,
            const_cast<wchar_t*>(L"%s"), character->name);
    mprintf(left, top + 0x0e, L"%s %d %s",
            gppStringList[0x1ae4 / 4],
            character->level,
            gppStringList[g_profession_name_message_ids_61e3f0[
                character->current_profession]]);
    mprintf(left, top + 0x18, L"%s %s",
            gppStringList[
                g_faction_name_message_rows_61e430[character->faction][0]],
            gppStringList[g_race_name_message_ids_61e3d0[character->race]]);
    SetObjectShade(g_wiz_text_font_secondary_object_683680, 4);
}

// FUNCTION: WIZ8 0x005beb90
void W8State5CharacterRow005EF364::AdjustValue(int amount)
{
    if (m_flag_5 && m_flag_4 && m_selection_listener) {
        m_selection_listener->Function5BF0E0(amount);
    }
}

// FUNCTION: WIZ8 0x005beb10
void W8State5CharacterRow005EF364::Function5290(int event)
{
    if (m_flag_5 && m_flag_4) {
        EnableSecondaryState(0);
        if (m_selection_listener) {
            m_selection_listener->Function5BF050(this);
        }
    }
    W8TextControl005ED604::Function5290(event);
}

// FUNCTION: WIZ8 0x005beb50
void W8State5CharacterRow005EF364::InvokeBlurCallback(int event)
{
    if (m_flag_5 && m_flag_4 && m_selection_listener) {
        m_selection_listener->Function5BF0C0(this);
    }
    W8TextControl005ED604::InvokeBlurCallback(event);
}

// FUNCTION: WIZ8 0x005bebc0
W8State5CharacterPanel005EF3C8::W8State5CharacterPanel005EF3C8()
    : Controls(), W8Control005ED654(), m_range_7c(0), m_selected_row(0)
{
    AcquireRegionSet(&g_state5_character_region_set_69c4f0);
    origin_x = 0x148;
    origin_y = 0x31;

    int top = 0;
    for (int row = 0; row < 6; ++row) {
        W8State5CharacterRow005EF364* control =
            new W8State5CharacterRow005EF364(this, top, row);
        control->m_selection_listener = this;
        AddEntry(control);
        top += 0x2a;
    }

    int selection = m_selected_row
                    - g_state5_party_collection_69c4ec->first_visible;
    if (selection < 0 || selection > 5) {
        selection = -1;
    }
    SetSelected(selection);
    m_value_20 = this;
    for (int index = 0; index < m_controls.count; ++index) {
        W8State5CharacterRow005EF364* row =
            static_cast<W8State5CharacterRow005EF364*>(ControlAt(index));
        row->m_character_index =
            g_state5_party_collection_69c4ec->first_visible + row->m_row;
        row->SetEnabled(row->m_character_index <
                        g_state5_party_collection_69c4ec->characters.count);
        row->Invalidate(0);
    }
}

// FUNCTION: WIZ8 0x005bedf0
W8State5CharacterPanel005EF3C8::~W8State5CharacterPanel005EF3C8()
{
    DestroyAllControls();
}

// FUNCTION: WIZ8 0x005bee70
void W8State5CharacterPanel005EF3C8::vslot00(
    W8Control005ED654*, int selected)
{
    m_selected_row = selected
                     + g_state5_party_collection_69c4ec->first_visible;
    g_state5_controller_69c4e8->SetSelection(m_selected_row, 0, 0);
}

// FUNCTION: WIZ8 0x005beea0
void W8State5CharacterPanel005EF3C8::OnRangeChanged(
    W8RangeControl005ED74C* control)
{
    if (!m_fEnabled) {
        return;
    }
    g_state5_party_collection_69c4ec->first_visible = control->m_value;
    m_value_20 = 0;
    int selection = m_selected_row
                    - g_state5_party_collection_69c4ec->first_visible;
    if (selection < 0 || selection > 5) {
        selection = -1;
    }
    SetSelected(selection);
    m_value_20 = this;
    for (int index = 0; index < m_controls.count; ++index) {
        W8State5CharacterRow005EF364* row =
            static_cast<W8State5CharacterRow005EF364*>(ControlAt(index));
        row->m_character_index =
            g_state5_party_collection_69c4ec->first_visible + row->m_row;
        row->SetEnabled(row->m_character_index <
                        g_state5_party_collection_69c4ec->characters.count);
        row->Invalidate(0);
    }
}

// FUNCTION: WIZ8 0x005bef60
void W8State5CharacterPanel005EF3C8::SetSelectedRow(int selection)
{
    m_selected_row = selection;
    int visible = selection
                  - g_state5_party_collection_69c4ec->first_visible;
    if (visible < 0 || visible > 5 ||
        g_state5_party_collection_69c4ec->characters.count <
            g_state5_party_collection_69c4ec->first_visible + 6) {
        int maximum =
            g_state5_party_collection_69c4ec->characters.count - 6;
        int first = selection < maximum ? selection : maximum;
        if (first < 0) {
            first = 0;
        }
        g_state5_party_collection_69c4ec->first_visible = first;
        m_range_7c->SetValue(first);
    }

    m_value_20 = 0;
    visible = m_selected_row
              - g_state5_party_collection_69c4ec->first_visible;
    if (visible < 0 || visible > 5) {
        visible = -1;
    }
    SetSelected(visible);
    m_value_20 = this;
    for (int index = 0; index < m_controls.count; ++index) {
        W8State5CharacterRow005EF364* row =
            static_cast<W8State5CharacterRow005EF364*>(ControlAt(index));
        row->m_character_index =
            g_state5_party_collection_69c4ec->first_visible + row->m_row;
        row->SetEnabled(row->m_character_index <
                        g_state5_party_collection_69c4ec->characters.count);
        row->Invalidate(0);
    }
}

// FUNCTION: WIZ8 0x005bf0c0
void W8State5CharacterPanel005EF3C8::Function5BF0C0(
    W8TextControl005ED604* entry)
{
    OnPrimary(entry);
    g_state5_controller_69c4e8->Function5C2970();
}

// FUNCTION: WIZ8 0x005bf050
void W8State5CharacterPanel005EF3C8::Function5BF050(
    W8TextControl005ED604* entry)
{
    OnPrimary(entry);
    int slot;
    for (slot = 0; slot < 6; ++slot) {
        if (static_cast<unsigned char*>(g_status_685170.buffers.buffer_08)
                [slot * 0x106 + 0x20c] &&
            &g_party_characters[slot + 2]
                == g_state5_controller_69c4e8->m_character_18) {
            break;
        }
    }
    g_dword_68ed10.parameter_2 = slot < 6 ? slot + 2 : -1;
    g_dword_68ed10.parameter_3 = g_state5_controller_69c4e8->m_character_18;
    SetPendingScreenState(6);
}

// FUNCTION: WIZ8 0x005bf0e0
void W8State5CharacterPanel005EF3C8::Function5BF0E0(int amount)
{
    while (amount > 0) {
        m_range_7c->Decrement();
        --amount;
    }
    while (amount < 0) {
        m_range_7c->Increment();
        ++amount;
    }
}

// FUNCTION: WIZ8 0x005bf120
W8State5PartySlotRow005EF3E4::W8State5PartySlotRow005EF3E4(
    Controls* panel, int row)
    : W8TextControl005ED604(panel, 0xffffffff,
                           (row & 1) * 0x20b + 0x0e,
                           (row / 2) * 0x82 + 0x3a,
                           0, 0, 0x101, 0, -1, 1, 0, 0, -1),
      m_row(row),
      m_redraw_partner(0)
{
    AddLayoutFlags(0x11);
    UpdateTextBounds(m_left - 8, m_top + 0x51,
                     m_left + 0x61, m_top + 0x60);
}

// FUNCTION: WIZ8 0x005bf220
W8State5PartySlotRow005EF3E4::~W8State5PartySlotRow005EF3E4()
{
}

// FUNCTION: WIZ8 0x005bf280
void W8State5PartySlotRow005EF3E4::Redraw(int full_redraw)
{
    if (!m_flag_5 || (!(unsigned char)full_redraw && !m_flag_6)) {
        return;
    }

    int left = m_pPanel->origin_x + m_left;
    int top = m_pPanel->origin_y + m_top;
    W8Character* character = 0;
    unsigned char* party_state =
        static_cast<unsigned char*>(g_status_685170.buffers.buffer_08);
    if (!party_state[(m_row + 2) * 0x106]) {
        ColorFillVideoSurfaceArea(-14, left, top,
                                  m_pPanel->origin_x + m_right,
                                  m_pPanel->origin_y + m_bottom, 0x8000);
    }
    else {
        character = &g_party_characters[m_row + 2];
        int portrait = character->table_value_0079;
        int flags = 2;
        if ((g_portrait_render_modes_6483dc[portrait][0] == 1 &&
             !(m_row & 1)) ||
            (g_portrait_render_modes_6483dc[portrait][0] == 2 &&
             (m_row & 1))) {
            flags = 0x1002;
        }
        RenderPartyPortrait0052EB00(portrait, left, top, flags, 1, m_row + 2);
    }
    m_textBuffer.SetText(character ? character->name : 0, g_font_683660);
    Function549600(-14, 0x100, 0, 0, left - 0x0d, top - 0x0b, 2, 0);
    W8TextControl005ED604::Redraw(full_redraw);
    if (m_redraw_partner) {
        m_redraw_partner->Invalidate(0);
    }
}

// FUNCTION: WIZ8 0x005bf410
void W8State5PartySlotRow005EF3E4::Function5290(int event)
{
    W8TextControl005ED604::Function5290(event);
    if (!m_flag_4 || !m_flag_5) {
        return;
    }
    SetAlternateTextEnabled(0);
    if (m_row == -1) {
        int slot;
        for (slot = 0; slot < 6; ++slot) {
            if (static_cast<unsigned char*>(g_status_685170.buffers.buffer_08)
                    [slot * 0x106 + 0x20c] &&
                &g_party_characters[slot + 2]
                    == g_state5_controller_69c4e8->m_character_18) {
                break;
            }
        }
        g_dword_68ed10.parameter_2 = slot < 6 ? slot + 2 : -1;
        g_dword_68ed10.parameter_3 =
            g_state5_controller_69c4e8->m_character_18;
    }
    else {
        g_dword_68ed10.parameter_2 = m_row + 2;
        g_dword_68ed10.parameter_3 = &g_party_characters[m_row + 2];
    }
    SetPendingScreenState(6);
}

// FUNCTION: WIZ8 0x005bf4e0
void W8State5PartySlotRow005EF3E4::InvokeBlurCallback(int event)
{
    W8TextControl005ED604::InvokeBlurCallback(event);
    if (m_flag_4 && m_flag_5) {
        g_state5_controller_69c4e8->SetSelection(m_row, 1, 0);
        g_state5_controller_69c4e8->Function5C2970();
    }
}

// FUNCTION: WIZ8 0x005bf6b0
void W8State5PartySlotPanel005EF438::vslot00(
    W8Control005ED654*, int)
{
    g_state5_controller_69c4e8->SetSelection(m_index_c, 1, 0);
}

// FUNCTION: WIZ8 0x005bf520
W8State5PartySlotPanel005EF438::W8State5PartySlotPanel005EF438()
    : Controls(), W8Control005ED654()
{
    AcquireRegionSet(&g_state5_party_slot_region_set_69c4f4);
    for (int row = 0; row < 6; ++row) {
        AddEntry(new W8State5PartySlotRow005EF3E4(this, row));
    }
    m_value_20 = this;
    for (int slot = 0; slot < 6; ++slot) {
        W8TextControl005ED604* control = m_lsButtons.data[slot];
        control->SetVisible(
            static_cast<unsigned char*>(g_status_685170.buffers.buffer_08)
                [slot * 0x106 + 0x20c]);
    }
    Invalidate(0);
}

// FUNCTION: WIZ8 0x005bf6d0
W8State5SixTextPanel005EF450::W8State5SixTextPanel005EF450()
    : Controls()
{
    AcquireRegionSet(&g_state5_six_text_region_set_69c4f8);
    for (int index = 0; index < 6; ++index) {
        int left = (index & 1) ? 0x21d : 0x52;
        int top = (index / 2) * 0x82 + 0x6c;
        W8TextControl005ED604* control =
            new W8TextControl005ED604(this, 0xffffffff,
                                      left, top, 0, 0,
                                      0xa7, 0, 0, 2, 1, 4, 3);
        control->m_listener = this;
    }
}

// FUNCTION: WIZ8 0x005bf840
void W8State5SixTextPanel005EF450::OnPrimary(
    W8TextControl005ED604* control)
{
    int index;
    for (index = 0; index < 6; ++index) {
        if (ControlAt(index) == control) {
            break;
        }
    }
    control->SetAlternateTextEnabled(0);
    g_dword_68ed10.parameter_3 = &g_party_characters[index + 2];
    g_dword_68ed10.mode = 2;
    SetPendingScreenState(3);
}

// FUNCTION: WIZ8 0x005bf7e0
W8State5SixTextPanel005EF450::~W8State5SixTextPanel005EF450()
{
    DestroyAllControls();
}

// FUNCTION: WIZ8 0x005bf640
W8State5PartySlotPanel005EF438::~W8State5PartySlotPanel005EF438()
{
    DestroyAllControls();
}

/* Draw the selected character's portrait and complete summary card.  The
   seven attribute values and the four right-column derived values come from
   the canonical character record; no parallel presentation snapshot exists. */
// FUNCTION: WIZ8 0x005bf8b0
void W8State5PlainPanel005EF4E0::Redraw()
{
    if (!m_fEnabled || !m_fDirty) {
        return;
    }
    m_fDirty = 0;

    Function549600(-14, 0xfc, 0, 0, 0x7f, 0xc5, 2, 0);
    if (!m_character_4c) {
        Function5497C0(-14, 0x85, 0x30, 0x139, 0xbf, 0x1b6, 0, 0);
        MarkScreenRectDirty(0x85, 0x30, 0x139, 0xbf, 0);
        return;
    }

    W8Character* character = m_character_4c;
    Function549600(-14, 0x11, character->table_value_0079,
                   0, 0x85, 0x30, 2, 0);
    if (character->in_party) {
        W8ControlsRect bounds = { 0x85, 0x30, 0x139, 0xba };
        W8TextBuffer005ED5B8 overlay(
            &bounds,
            gppStringList[0x1ae0 / 4],
            g_options_title_font_68368c,
            g_W8TextBufferLayoutMask005ED55C |
                g_W8TextBufferLayoutMask005ED54C,
            4);
        overlay.Function4F39B0(0, 0, -14);
    }

    W8ControlsRect name_bounds = { 0x84, 0xc8, 0x139, 0xed };
    W8TextBuffer005ED5B8 name(
        &name_bounds,
        FormatWideString(L"%s (%s)", character->name_part_2, character->name),
        g_wiz_text_bold_font_683664,
        g_W8TextBufferLayoutMask005ED554 |
            g_W8TextBufferLayoutMask005ED54C,
        4);
    name.Function4F39B0(0, 0, -14);

    SetValue5FF5F0(g_font_683660);
    SetObjectShade(g_wiz_text_font_secondary_object_683680, 4);

    const wchar_t* level_text = gppStringList[0x1ae4 / 4];
    const wchar_t* profession = gppStringList[
        g_profession_name_message_ids_61e3f0[character->current_profession]];
    wchar_t* level_line = FormatWideString(
        L"%s %d %s", level_text, character->level, profession);
    int width = StringPixLength(
        reinterpret_cast<unsigned short*>(level_line), g_font_683660);
    mprintf((0xbf - width) / 2 + 0x78, 0xef,
            L"%s %d %s", level_text, character->level, profession);

    const wchar_t* faction = gppStringList[
        g_faction_name_message_rows_61e430[character->faction][0]];
    const wchar_t* race =
        gppStringList[g_race_name_message_ids_61e3d0[character->race]];
    wchar_t* race_line = FormatWideString(L"%s %s", faction, race);
    width = StringPixLength(
        reinterpret_cast<unsigned short*>(race_line), g_font_683660);
    mprintf((0xbf - width) / 2 + 0x7f, 0xfd, L"%s %s", faction, race);

    const wchar_t* personality = gppStringList[
        g_personality_message_ids_61e674[character->personality_0081]];
    width = StringPixLength(
        const_cast<unsigned short*>(personality), g_font_683660);
    mprintf((0xbf - width) / 2 + 0x82, 0x10b,
            const_cast<wchar_t*>(L"%s"), personality);

    mprintf(0x96, 0x127,
            gppStringList[0x1ae8 / 4]);
    mprintf(0xbf, 0x127, L"%d", character->attributes[0].value);
    mprintf(0x96, 0x135,
            gppStringList[0x1aec / 4]);
    mprintf(0xbf, 0x135, L"%d", character->attributes[1].value);
    mprintf(0x96, 0x143,
            gppStringList[0x1af0 / 4]);
    mprintf(0xbf, 0x143, L"%d", character->attributes[2].value);
    mprintf(0x96, 0x151,
            gppStringList[0x1af4 / 4]);
    mprintf(0xbf, 0x151, L"%d", character->attributes[3].value);
    mprintf(0x96, 0x15f,
            gppStringList[0x1af8 / 4]);
    mprintf(0xbf, 0x15f, L"%d", character->attributes[4].value);
    mprintf(0x96, 0x16d,
            gppStringList[0x1afc / 4]);
    mprintf(0xbf, 0x16d, L"%d", character->attributes[5].value);
    mprintf(0x96, 0x17b,
            gppStringList[0x1b00 / 4]);
    mprintf(0xbf, 0x17b, L"%d", character->attributes[6].value);

    mprintf(0xe3, 0x127,
            gppStringList[0x1b04 / 4]);
    mprintf(0x115, 0x127, L"%d", character->hp_max);
    mprintf(0xe3, 0x135,
            gppStringList[0x1b08 / 4]);
    mprintf(0x115, 0x135, L"%d", SumCharacterSpellPoints(character));
    mprintf(0xe3, 0x143,
            gppStringList[0x1b0c / 4]);
    mprintf(0x115, 0x143, L"%d", character->stamina_max);
    mprintf(0xe3, 0x151,
            gppStringList[0x1b10 / 4]);
    mprintf(0x115, 0x151, L"%d", character->value_0bc5 / 10);
}

// GLOBAL: WIZ8 0x0069C4FC
unsigned int g_state5_option_region_set_69c4fc;

// GLOBAL: WIZ8 0x0069C500
unsigned int g_state5_range_region_set_69c500;
// GLOBAL: WIZ8 0x0069C504
unsigned int g_state5_left_action_region_set_69c504;
// GLOBAL: WIZ8 0x0069C508
unsigned int g_state5_bottom_action_region_set_69c508;
// GLOBAL: WIZ8 0x0069C50C
unsigned int g_state5_import_list_region_set_69c50c;

/* Build the three mutually selected difficulty controls and the two
   independent toggles used by the later creation modes.  The entry vector is
   separate: redraw walks it for the repeated option-detail render pass. */
// FUNCTION: WIZ8 0x005c01d0
W8State5OptionPanel005EF4AC::W8State5OptionPanel005EF4AC()
    : Controls(0x78, 0x28, 0, 0, 0x102, 0, 0),
      m_options_50(),
      m_entries_7c()
{
    AcquireRegionSet(&g_state5_option_region_set_69c4fc);

    short width;
    short height;
    Function549660(0x102, 0, 0, &width, &height);
    right = origin_x + (unsigned short)width;
    bottom = origin_y + (unsigned short)height;

    int top = 0x13;
    for (int index = 0; index < 3; ++index) {
        m_options_50.AddEntry(
            new W8TextControl005ED604(this, 0xffffffff,
                                      0x15f, top, 0, 0,
                                      0xf1, 0, 4, 6, 5, 7, -1));
        top += 0x16;
    }
    m_options_50.SetSelected(g_settings_6850c8.field_00d);

    top += 0x16;
    m_toggle_78 =
        new W8TextControl005ED604(this, 0xffffffff,
                                  0x15b, top, 0, 0,
                                  0xf1, 0, 2, 0, 3, 1, -1);
    m_toggle_78->AddLayoutFlags(
        g_W8TextControlMask005ED588 | g_W8TextControlMask005ED578);
    if (g_settings_6850c8.field_050) {
        m_toggle_78->ActivatePrimary(0);
    }

    m_toggle_74 =
        new W8TextControl005ED604(this, 0xffffffff,
                                  0x15b, 0xd6, 0, 0,
                                  0xf1, 0, 2, 0, 3, 1, -1);
    m_toggle_74->AddLayoutFlags(
        g_W8TextControlMask005ED588 | g_W8TextControlMask005ED578);

    Function549660(0x102, 0, 1, &m_image_width_94, &m_image_height_96);
    m_render_left_8c =
        origin_x + 0x18 + (0x160 - (unsigned short)m_image_width_94) / 2;
    m_render_top_90 = origin_y + 0x80;
}

// FUNCTION: WIZ8 0x005c0560
void W8State5OptionPanel005EF4AC::Redraw()
{
    bool redraw = m_fEnabled && m_fDirty;
    Controls::Redraw();
    if (!redraw) {
        return;
    }

    for (int index = 0; index < m_entries_7c.count; ++index) {
        m_entries_7c.data[index]->Function4F39B0(0, 1, -14);
    }
    if (m_mode_4c == 2) {
        Function548F90(-14, 0x102, 0, 1,
                       m_render_left_8c, m_render_top_90, 2, 0);
    }
}

/* Tear down the single-field text editor installed by option mode two.  The
   vtable at 0x005EF4BC contains only this deleting-destructor family; input
   completion is delivered through the separate decision-listener pointer. */
// SYNTHETIC: WIZ8 0x005c0e20
// W8State5InputHandler005C0E50::`scalar deleting destructor'
W8State5InputHandler005C0E50::~W8State5InputHandler005C0E50()
{
    Function5D3B40(0);
    Function5D3800();
}

/* Give the active string editor first refusal on keyboard events, reject the
   filename characters retail excludes, and report both edit-state and final
   Escape/Return decisions to the controller's decision-listener subobject. */
// FUNCTION: WIZ8 0x005c0e50
unsigned char W8State5InputHandler005C0E50::HandleInput(
    const InputAtom* input)
{
    if (input->usEvent != KEY_DOWN && input->usEvent != KEY_REPEAT) {
        Function568950(input);
        return 0;
    }

    if (input->usParam != VK_ESCAPE) {
        if (input->usParam != VK_RETURN) {
            unsigned short character = Function402780(
                static_cast<unsigned short>(input->usParam),
                input->usKeyState);
            if (character != 0 &&
                strchr("\\/:*?\"<>|", static_cast<unsigned char>(character))) {
                return 1;
            }
            Function5D3F50(input);
            if (m_listener) {
                m_listener->OnToggle(Function5D3D00(0) != 0);
            }
            return 1;
        }
        if (!Function5D3D00(0)) {
            return 1;
        }
    }

    Function55EE70(-1);
    if (m_listener) {
        m_listener->OnDecision(
            reinterpret_cast<int>(this), input->usParam == VK_ESCAPE);
    }
    return 1;
}

/* Replace the option panel's complete detail composition. Mode zero owns the
   eight creation-summary buffers, mode one shows its single explanatory
   paragraph, and mode two owns the name prompt plus the real string-input
   session. The panel's three difficulty controls are interactive only in
   mode zero. */
// FUNCTION: WIZ8 0x005c05f0
void W8State5OptionPanel005EF4AC::Function5C05F0(int mode)
{
    m_mode_4c = mode;
    while (m_entries_7c.count > 0) {
        delete m_entries_7c.RemoveAt(m_entries_7c.count - 1);
    }
    for (int index = 0; index < m_controls.count; ++index) {
        ControlAt(index)->SetEnabled(mode == 0);
    }

    W8ControlsRect bounds = {
        origin_x + 0x22, origin_y + 0x12,
        origin_x + 0x16e, origin_y + 0x171
    };

    if (mode == 0) {
        m_entries_7c.Add(new W8TextBuffer005ED5B8(
            &bounds,
            gppStringList[0x1fdc / 4],
            g_options_detail_font_683614,
            g_W8TextBufferLayoutMask005ED558 |
                g_W8TextBufferLayoutMask005ED548,
            4));

        bounds.right = origin_x + 0x155;
        m_entries_7c.Add(new W8TextBuffer005ED5B8(
            &bounds,
            gppStringList[0x1fe0 / 4],
            g_options_detail_font_683614,
            g_W8TextBufferLayoutMask005ED550 |
                g_W8TextBufferLayoutMask005ED558,
            4));
        bounds.top += 0x16;
        m_entries_7c.Add(new W8TextBuffer005ED5B8(
            &bounds,
            gppStringList[0x1fe4 / 4],
            g_options_detail_font_683614,
            g_W8TextBufferLayoutMask005ED550 |
                g_W8TextBufferLayoutMask005ED558,
            4));
        bounds.top += 0x16;
        m_entries_7c.Add(new W8TextBuffer005ED5B8(
            &bounds,
            gppStringList[0x1fe8 / 4],
            g_options_detail_font_683614,
            g_W8TextBufferLayoutMask005ED550 |
                g_W8TextBufferLayoutMask005ED558,
            4));

        bounds.right = origin_x + 0x16e;
        bounds.top += 0x2c;
        m_entries_7c.Add(new W8TextBuffer005ED5B8(
            &bounds,
            gppStringList[0x202c / 4],
            g_options_detail_font_683614,
            g_W8TextBufferLayoutMask005ED558 |
                g_W8TextBufferLayoutMask005ED548,
            4));

        bounds.top += 0x2c;
        W8TextBuffer005ED5B8* text = new W8TextBuffer005ED5B8(
            &bounds,
            gppStringList[0x1b34 / 4],
            g_options_detail_font_683614,
            g_W8TextBufferLayoutMask005ED54C |
                g_W8TextBufferLayoutMask005ED558,
            4);
        text->SetLineHeight(0x16);
        m_entries_7c.Add(text);

        bounds.top += 0x42;
        m_entries_7c.Add(new W8TextBuffer005ED5B8(
            &bounds,
            gppStringList[0x1b38 / 4],
            g_options_detail_font_683614,
            g_W8TextBufferLayoutMask005ED558 |
                g_W8TextBufferLayoutMask005ED548,
            4));

        bounds.top += 0x2c;
        text = new W8TextBuffer005ED5B8(
            &bounds,
            gppStringList[0x1b3c / 4],
            g_options_detail_font_683614,
            g_W8TextBufferLayoutMask005ED54C |
                g_W8TextBufferLayoutMask005ED558,
            4);
        text->SetLineHeight(0x16);
        m_entries_7c.Add(text);
        return;
    }

    if (mode == 1) {
        bounds.top += 0x2c;
        W8TextBuffer005ED5B8* text = new W8TextBuffer005ED5B8(
            &bounds,
            gppStringList[0x1b40 / 4],
            g_options_detail_font_683614,
            g_W8TextBufferLayoutMask005ED54C |
                g_W8TextBufferLayoutMask005ED558,
            4);
        text->SetLineHeight(0x16);
        m_entries_7c.Add(text);
        return;
    }

    if (mode != 2) {
        return;
    }

    bounds.left = origin_x + 0x2c;
    bounds.right = origin_x + 0x164;
    W8TextBuffer005ED5B8* text = new W8TextBuffer005ED5B8(
        &bounds,
        gppStringList[0x1b44 / 4],
        g_options_detail_font_683614,
        g_W8TextBufferLayoutMask005ED54C |
            g_W8TextBufferLayoutMask005ED558,
        4);
    text->SetLineHeight(0x16);
    m_entries_7c.Add(text);

    W8State5Controller005EF4CC* controller = g_state5_controller_69c4e8;
    if (controller->m_input_handler_64) {
        return;
    }
    W8State5InputHandler005C0E50* input_handler =
        new W8State5InputHandler005C0E50;
    Function5D3520(1);
    Function5D39B0(
        m_render_left_8c + 2, m_render_top_90 + 2,
        static_cast<unsigned short>(m_image_width_94) - 4,
        static_cast<unsigned short>(m_image_height_96) - 4,
        0x7f, L"", 0x28, 0x0f, 1);
    Function5D3D20(0);
    controller->m_input_handler_64 = input_handler;
    input_handler->m_listener = controller;
    input_handler->m_listener->OnToggle(0);
}

/* Construct the complete state-5 panel graph and attach every callback before
   the first mode transition.  The shared range is rebound by SetMode to either
   the character rows or import list; the six portrait rows invalidate their
   paired advance controls after drawing. */
// FUNCTION: WIZ8 0x005c0f30
void W8State5Controller005EF4CC::Setup()
{
    m_range = new W8RangeControl005ED74C(
        0x1e9, 0x31, 0x1fb, 0x12b, &g_state5_range_region_set_69c500);
    m_character_panel_20 = new W8State5CharacterPanel005EF3C8;
    m_control_24 = new W8State5SixTextPanel005EF450;
    m_control_28 = new W8State5PartySlotPanel005EF438;
    m_control_2c = new W8State5PlainPanel005EF4E0;
    m_control_30 = new W8State5OptionPanel005EF4AC;

    for (int slot = 0; slot < 6; ++slot) {
        W8State5PartySlotRow005EF3E4* row =
            static_cast<W8State5PartySlotRow005EF3E4*>(
                m_control_28->m_lsButtons.data[slot]);
        row->m_redraw_partner = m_control_24->ControlAt(slot);
    }

    m_panel_34 = new Controls(0x145, 0x137, 0, 0, -1, -1, -1);
    m_panel_34->AcquireRegionSet(&g_state5_left_action_region_set_69c504);
    m_text_40 =
        new W8TextControl005ED604(m_panel_34, 0xffffffff,
                                  0, 0, 0, 0,
                                  0xfe, 0, 0, 2, 1, 2, 3);
    m_text_40->m_textBuffer.SetText(
        gppStringList[0x1b14 / 4],
        g_wiz_text_bold_font_683664);
    m_text_40->m_listener = this;

    m_text_44 =
        new W8TextControl005ED604(m_panel_34, 0xffffffff,
                                  0, 0x1a, 0, 0,
                                  0xfe, 0, 0, 2, 1, 2, 3);
    m_text_44->m_listener = this;

    m_text_48 =
        new W8TextControl005ED604(m_panel_34, 0xffffffff,
                                  0, 0x34, 0, 0,
                                  0xfe, 0, 0, 2, 1, 2, 3);
    m_text_48->m_textBuffer.SetText(
        gppStringList[0x1b20 / 4],
        g_wiz_text_bold_font_683664);
    m_text_48->m_listener = this;

    m_text_4c =
        new W8TextControl005ED604(m_panel_34, 0xffffffff,
                                  0, 0x4e, 0, 0,
                                  0xfe, 0, 0, 2, 1, 2, 3);
    m_text_4c->m_textBuffer.SetText(
        gppStringList[0x1b24 / 4],
        g_wiz_text_bold_font_683664);
    m_text_4c->m_listener = this;

    m_panel_38 = new Controls(0x148, 0x31, 0, 0, -1, -1, -1);
    m_panel_38->AcquireRegionSet(&g_state5_import_list_region_set_69c50c);
    m_list_5c = new W8State5ListControl005EF464(
        m_panel_38, 0xffffffff, 0, 0, 0x9d, 0xfa);
    m_list_5c->m_listener = this;

    m_panel_3c = new Controls(0x84, 0x1b5, 0, 0, -1, -1, -1);
    m_panel_3c->AcquireRegionSet(&g_state5_bottom_action_region_set_69c508);
    m_text_58 =
        new W8TextControl005ED604(m_panel_3c, 0xffffffff,
                                  0x14c, 0, 0, 0,
                                  0x106, 0, 4, 6, 5, 6, 7);
    m_text_58->EnableRegionHelp(0x6ca);
    m_text_58->m_listener = this;

    m_text_54 =
        new W8TextControl005ED604(m_panel_3c, 0xffffffff,
                                  0x120, 0, 0, 0,
                                  0x106, 0, 0, 2, 1, 2, 3);
    m_text_54->EnableRegionHelp(0x6cb);
    m_text_54->m_listener = this;

    m_text_50 =
        new W8TextControl005ED604(m_panel_3c, 0xffffffff,
                                  0xf4, 0, 0, 0,
                                  0x106, 0, 0x18, 0x1a, 0x19, 0x1c, 0x1b);
    m_text_50->AddLayoutFlags(
        g_W8TextControlMask005ED588 | g_W8TextControlMask005ED578);
    m_text_50->EnableRegionHelp(0x6cc);
    m_text_50->m_listener = this;

    W8ControlsRect bounds = { 0x7b, 6, 0x205, 0x23 };
    m_text_buffer_60 = new W8TextBuffer005ED5B8;
    m_text_buffer_60->SetLayoutBounds(&bounds, 1, 1);

    SetMode(0);
    SetSelection(0, 0, 1);
}

// FUNCTION: WIZ8 0x005c1580
W8State5PlainPanel005EF4E0::~W8State5PlainPanel005EF4E0()
{
}

// FUNCTION: WIZ8 0x005c0480
W8State5OptionPanel005EF4AC::~W8State5OptionPanel005EF4AC()
{
    DestroyAllControls();
    while (m_entries_7c.count > 0) {
        delete m_entries_7c.RemoveAt(m_entries_7c.count - 1);
    }
}

// FUNCTION: WIZ8 0x005c1590
W8State5Controller005EF4CC::~W8State5Controller005EF4CC()
{
    delete m_text_buffer_60;

    if (m_panel_3c) {
        m_panel_3c->DestroyAllControls();
        delete m_panel_3c;
    }
    if (m_panel_34) {
        m_panel_34->DestroyAllControls();
        delete m_panel_34;
    }
    if (m_panel_38) {
        m_panel_38->DestroyAllControls();
        delete m_panel_38;
    }

    delete m_control_24;
    delete m_control_28;
    delete m_character_panel_20;
    delete m_range;
    delete m_control_2c;
    delete m_control_30;
}

/* Switch the party-builder as a complete screen mode.  Mode zero presents the
   loose-character collection, mode one presents the six active slots and the
   import list, and modes two through four hand the screen to the option panel.
   Every panel is invalidated before its enable/region state changes so the
   next frame redraws the new composition. */
// FUNCTION: WIZ8 0x005c2010
void W8State5Controller005EF4CC::SetMode(int mode)
{
    m_mode = mode;
    m_redraw_backdrop_14 = 1;

    m_range->Invalidate(0);
    m_control_2c->Invalidate(0);
    m_control_30->Invalidate(0);
    m_character_panel_20->Invalidate(0);
    m_panel_34->Invalidate(0);
    m_control_24->Invalidate(0);
    m_control_28->Invalidate(0);
    m_panel_38->Invalidate(0);
    m_panel_3c->Invalidate(0);

    m_text_buffer_60->m_alternateRenderer = 1;
    m_panel_3c->SetEnabled(1);
    m_panel_3c->EnableRegionSet(1);
    m_control_28->SetEnabled(1);

    const wchar_t* label = 0;
    switch (m_mode) {
    case 0: {
        m_range->SetEnabled(1);
        m_range->EnableRegionSet(1);
        m_panel_34->SetEnabled(1);
        m_panel_34->EnableRegionSet(1);
        m_character_panel_20->SetEnabled(1);
        m_character_panel_20->EnableRegionSet(1);

        m_character_panel_20->m_range_7c = m_range;
        m_range->m_listener = m_character_panel_20;
        int maximum = g_state5_party_collection_69c4ec->characters.count - 6;
        if (maximum < 0) {
            maximum = 0;
        }
        m_range->SetRange(0, maximum);
        m_range->SetEnabled(maximum > 0);
        m_range->Invalidate(0);

        m_character_panel_20->m_value_20 = 0;
        int selected = m_character_panel_20->m_selected_row
                       - g_state5_party_collection_69c4ec->first_visible;
        if (selected < 0 || selected > 5) {
            selected = -1;
        }
        m_character_panel_20->SetSelected(selected);
        m_character_panel_20->m_value_20 = m_character_panel_20;
        for (int index = 0;
             index < m_character_panel_20->m_controls.count; ++index) {
            W8State5CharacterRow005EF364* row =
                static_cast<W8State5CharacterRow005EF364*>(
                    m_character_panel_20->ControlAt(index));
            row->m_character_index =
                g_state5_party_collection_69c4ec->first_visible + row->m_row;
            row->SetEnabled(
                row->m_character_index <
                g_state5_party_collection_69c4ec->characters.count);
            row->Invalidate(0);
        }

        m_control_24->SetEnabled(0);
        m_control_24->EnableRegionSet(0);
        m_control_28->EnableRegionSet(1);
        m_panel_38->SetEnabled(0);
        m_panel_38->EnableRegionSet(0);
        m_control_2c->SetEnabled(1);
        m_control_30->SetEnabled(0);
        m_control_30->EnableRegionSet(0);
        m_text_54->SetVisible(CountActiveCharacters() != 0);
        label = gppStringList[0x1ad0 / 4];
        break;
    }
    case 1: {
        if (g_state5_party_collection_69c4ec->names.count == 0) {
            char search[128];
            GETFILESTRUCT find;
            sprintf(search, "%s\\*.*", "Saves\\Import");
            BOOLEAN found = GetFileFirst(search, &find);
            while (found) {
                if ((find.uiFileAttribs & FILE_IS_DIRECTORY) == 0) {
                    size_t length = strlen(find.zFileName) + 1;
                    char* name = new char[length];
                    memcpy(name, find.zFileName, length);
                    g_state5_party_collection_69c4ec->names.Add(name);
                }
                found = GetFileNext(&find);
            }
        }

        m_range->SetEnabled(1);
        m_range->EnableRegionSet(1);
        m_panel_34->SetEnabled(1);
        m_panel_34->EnableRegionSet(1);
        m_text_40->SetEnabled(0);
        m_text_44->SetEnabled(0);
        m_text_48->SetEnabled(0);
        m_character_panel_20->SetEnabled(0);
        m_character_panel_20->EnableRegionSet(0);
        m_control_24->EnableRegionSet(1);
        m_control_24->SetEnabled(1);
        for (int slot = 0; slot < 6; ++slot) {
            unsigned char occupied =
                static_cast<unsigned char*>(g_status_685170.buffers.buffer_08)
                    [slot * 0x106 + 0x20c];
            m_control_24->ControlAt(slot)->SetEnabled(
                m_mode == 1 && occupied &&
                IsCharacterReadyToAdvance(slot + 2));
        }
        m_control_24->Invalidate(0);
        m_control_28->EnableRegionSet(1);
        m_panel_38->SetEnabled(1);
        m_panel_38->EnableRegionSet(1);

        m_range->m_listener = m_list_5c;
        int maximum = g_state5_party_collection_69c4ec->names.count
                      - m_list_5c->m_visible_rows;
        if (maximum < 0) {
            maximum = 0;
        }
        m_range->SetRange(0, maximum);
        m_range->SetValue(m_list_5c->m_selection);
        m_range->SetEnabled(maximum > 0);
        m_range->Invalidate(0);

        m_control_2c->SetEnabled(1);
        m_control_30->SetEnabled(0);
        m_control_30->EnableRegionSet(0);
        label = gppStringList[0x1ad4 / 4];
        break;
    }
    case 2:
        m_range->SetEnabled(0);
        m_range->EnableRegionSet(0);
        m_panel_34->SetEnabled(0);
        m_panel_34->EnableRegionSet(0);
        m_control_24->SetEnabled(0);
        m_control_24->EnableRegionSet(0);
        m_control_28->EnableRegionSet(0);
        m_character_panel_20->SetEnabled(0);
        m_character_panel_20->EnableRegionSet(0);
        m_panel_38->SetEnabled(0);
        m_panel_38->EnableRegionSet(0);
        m_control_2c->SetEnabled(0);
        m_text_50->SetEnabled(0);
        m_control_30->SetEnabled(1);
        m_control_30->EnableRegionSet(1);
        m_control_30->Function5C05F0(0);
        label = gppStringList[0x1ad8 / 4];
        break;
    case 3:
        m_text_50->SetEnabled(0);
        m_control_30->Function5C05F0(1);
        label = gppStringList[0x1adc / 4];
        break;
    case 4:
        m_text_50->SetEnabled(0);
        m_control_30->Function5C05F0(2);
        label = gppStringList[0x1adc / 4];
        break;
    default:
        return;
    }
    m_text_buffer_60->SetText(label, g_wiz_text_bold_font_683664);
}

// FUNCTION: WIZ8 0x005c1680
void W8State5Controller005EF4CC::SetSelection(
    int selection, int party_slot, int refresh_other)
{
    if (!party_slot) {
        if (m_mode == 0) {
            m_character_18 =
                g_state5_party_collection_69c4ec->GetCharacter(selection);
            int selected_slot = -1;
            if (m_character_18 && m_character_18->in_party) {
                for (int slot = 0; slot < 6; ++slot) {
                    if (static_cast<unsigned char*>(
                            g_status_685170.buffers.buffer_08)
                            [slot * 0x106 + 0x20c] &&
                        &g_party_characters[slot + 2] == m_character_18) {
                        selected_slot = slot;
                        break;
                    }
                }
            }
            m_control_28->m_value_20 = 0;
            m_control_28->SetSelected(selected_slot);
            m_control_28->m_value_20 = m_control_28;
            if (refresh_other) {
                m_character_panel_20->SetSelectedRow(selection);
            }
        }
    }
    else {
        m_character_18 = &g_party_characters[selection + 2];
        if (!m_character_18->in_party) {
            m_character_18 = 0;
            selection = -1;
        }
        if (m_mode == 0) {
            int character_index = -1;
            if (selection >= 0 &&
                static_cast<unsigned char*>(g_status_685170.buffers.buffer_08)
                    [(selection + 2) * 0x106]) {
                for (int index = 0;
                     index < g_state5_party_collection_69c4ec->characters.count;
                     ++index) {
                    if (g_state5_party_collection_69c4ec->GetCharacter(index)
                        == m_character_18) {
                        character_index = index;
                        break;
                    }
                }
            }
            m_character_panel_20->SetSelectedRow(character_index);
        }
        if (refresh_other) {
            m_control_28->m_value_20 = 0;
            m_control_28->SetSelected(selection);
            m_control_28->m_value_20 = m_control_28;
        }
    }

    m_control_2c->m_character_4c = m_character_18;
    m_control_2c->Invalidate(0);
    wchar_t* text = gppStringList[
        ((!m_character_18 || !m_character_18->in_party)
             ? 0x1b18
             : 0x1b1c) / 4];
    m_text_44->m_textBuffer.SetText(text, g_font_683660);
    unsigned char have_character = m_character_18 != 0;
    m_text_44->SetVisible(have_character);
    m_text_44->Invalidate(0);
    m_text_48->SetVisible(have_character);
    m_text_48->Invalidate(0);
    m_text_4c->SetVisible(have_character);
    m_text_4c->Invalidate(0);
    if (m_character_18 && !m_character_18->in_party &&
        FindFreePartySlot(2, 8) == 0xffffffff) {
        m_text_44->SetVisible(0);
    }
}

/* Dispatch the seven visible party-builder actions.  This is the screen's
   product decision layer: it owns back/finish behavior, edit/create/delete,
   the option-mode progression, and converting an active party back into loose
   character records before an imported party is selected. */
// FUNCTION: WIZ8 0x005c1920
void W8State5Controller005EF4CC::OnPrimary(
    W8TextControl005ED604* control)
{
    if (control == m_text_58) {
        if (m_input_handler_64) {
            OnDecision(reinterpret_cast<int>(m_input_handler_64), 1);
            return;
        }
        switch (m_mode) {
        case 0:
        case 1:
            if (CountActiveCharacters() != 0) {
                OpenNotification(
                    gppStringList[0x1b4c / 4],
                    1, 5);
                return;
            }
            RequestScreenTransition();
            return;
        case 2:
        case 3:
        case 4:
            SetMode(m_previous_mode);
            SetSelection(0, m_previous_mode == 1, 1);
            return;
        default:
            return;
        }
    }

    if (control == m_text_44) {
        Function5C2970();
        return;
    }
    if (control == m_text_48) {
        OpenNotification(
            FormatWideString(
                L"%s %s %s",
                gppStringList[0x1b50 / 4],
                m_character_18->name,
                gppStringList[0x1b54 / 4]),
            1, 1);
        return;
    }
    if (control == m_text_4c) {
        int slot;
        for (slot = 0; slot < 6; ++slot) {
            if (static_cast<unsigned char*>(g_status_685170.buffers.buffer_08)
                    [slot * 0x106 + 0x20c] &&
                &g_party_characters[slot + 2] == m_character_18) {
                break;
            }
        }
        g_dword_68ed10.parameter_2 = slot < 6 ? slot + 2 : -1;
        g_dword_68ed10.parameter_3 = m_character_18;
        SetPendingScreenState(6);
        return;
    }
    if (control == m_text_40) {
        g_dword_68ed10.mode = 0;
        g_dword_68ed10.parameter_3 = 0;
        SetPendingScreenState(3);
        return;
    }

    if (control == m_text_54) {
        switch (m_mode) {
        case 0:
            if ((unsigned int)CountActiveCharacters() < 6) {
                OpenNotification(
                    FormatWideString(
                        gppStringList[0x1b64 / 4],
                        CountActiveCharacters()),
                    1, 3);
                return;
            }
            /* fall through */
        case 1:
            m_previous_mode = m_mode;
            SetSelection(-1, 1, 1);
            SetMode(2);
            return;
        case 2:
            g_settings_6850c8.field_00d = m_control_30->m_options_50.m_index_c;
            g_settings_6850c8.field_050 =
                (unsigned char)(m_control_30->m_toggle_78->m_stateFlags &
                                g_W8TextControlMask005ED570);
            if ((m_control_30->m_toggle_74->m_stateFlags &
                 g_W8TextControlMask005ED570) != 0) {
                SetMode(3);
            }
            else {
                Function54B250(1, 0);
            }
            return;
        case 3:
            SetMode(4);
            return;
        case 4:
            OnDecision(reinterpret_cast<int>(m_input_handler_64), 0);
            return;
        default:
            return;
        }
    }

    if (control != m_text_50) {
        return;
    }
    if ((m_text_50->m_stateFlags & g_W8TextControlMask005ED570) == 0) {
        Function54B100();
        SetMode(0);
        SetSelection(0, 0, 1);
        return;
    }
    if (CountActiveCharacters() != 0) {
        OpenNotification(
            gppStringList[0x1b58 / 4],
            1, 4);
        return;
    }

    W8State5PartyCollection* collection =
        g_state5_party_collection_69c4ec;
    for (int index = 0; index < collection->characters.count; ++index) {
        W8Character* previous = collection->GetCharacter(index);
        if (!previous->in_party) {
            continue;
        }

        int slot = collection->FindPartySlot(index);
        W8Character* replacement = new W8Character;
        char path[128];
        BuildCharacterPath00514EC0(path, previous->name, -1);
        if (!LoadCharacter(path, replacement, -1, 0)) {
            memcpy(replacement, previous, sizeof(W8Character));
        }
        Function4EF610(slot + 2, 0);
        replacement->in_party = 0;
        collection->characters.SetAt(index, replacement);
    }
    SetMode(1);
    Function5C2C60(m_list_5c->m_selection);
}

// FUNCTION: WIZ8 0x005c1d60
void W8State5Controller005EF4CC::OnSelectionChanged(
    W8State5ListControl005EF464*, int selection)
{
    Function5C2C60(selection);
}

// FUNCTION: WIZ8 0x005c1d70
void W8State5Controller005EF4CC::OnDecision(
    int, unsigned char accepted)
{
    if (!accepted) {
        wchar_t slot_name[64];
        GetSaveSlotName005D3CC0(0, slot_name);
        if (SaveSlotFileExists(ConvertWideStringToString(slot_name))) {
            OpenNotification(
                gppStringList[0x20a4 / 4],
                1, 2);
            return;
        }
        if (m_input_handler_64) {
            delete m_input_handler_64;
        }
        m_input_handler_64 = 0;
        Function54B250(1, slot_name);
    } else {
        if (m_input_handler_64) {
            delete m_input_handler_64;
        }
        m_input_handler_64 = 0;
        switch (m_mode) {
        case 0:
        case 1:
            if (CountActiveCharacters() == 0) {
                RequestScreenTransition();
                return;
            }
            OpenNotification(
                gppStringList[0x1b4c / 4],
                1, 5);
            return;
        case 2:
        case 3:
        case 4:
            SetMode(m_previous_mode);
            SetSelection(0, m_previous_mode == 1, 1);
            return;
        }
    }
}

// FUNCTION: WIZ8 0x005c1ea0
void W8State5Controller005EF4CC::OnToggle(int value)
{
    if ((char)m_text_54->m_flag_4 != (char)value) {
        m_text_54->SetVisible((unsigned char)value);
        m_text_54->Invalidate(0);
    }
}

// FUNCTION: WIZ8 0x005c1ed0
void W8State5Controller005EF4CC::Function5C1ED0()
{
    m_redraw_backdrop_14 = 1;
    m_range->Invalidate(0);
    m_control_2c->Invalidate(0);
    m_control_30->Invalidate(0);
    m_character_panel_20->Invalidate(0);
    m_panel_34->Invalidate(0);
    m_control_24->Invalidate(0);
    m_control_28->Invalidate(0);
    m_panel_38->Invalidate(0);
    m_panel_3c->Invalidate(0);
    m_text_buffer_60->m_alternateRenderer = 1;
}

/* Draw the state-5 composition in owner order.  The one-shot backdrop is
   refreshed only after a mode/dialog change; panel redraws and modal overlay
   dispatch still run every frame. */
// FUNCTION: WIZ8 0x005c1f40
void W8State5Controller005EF4CC::Function5C1F40()
{
    if (m_redraw_backdrop_14) {
        Function549600(-14, 0xfa, 0, 0, 0, 0, 2, 0);
        if (m_mode == 1) {
            Function548F90(-14, 0x103, 0, 0, 0x140, 0x12e, 2, 0);
        }
        m_redraw_backdrop_14 = 0;
    }

    m_range->Redraw();
    m_control_2c->Redraw();
    m_control_30->Redraw();
    m_character_panel_20->Redraw();
    m_panel_34->Redraw();
    m_control_28->Redraw();
    m_control_24->Redraw();
    m_panel_38->Redraw();
    m_panel_3c->Redraw();
    m_text_buffer_60->Function4F39B0(0, 0, -14);
    if (m_dialog_68) {
        m_dialog_68->vslot3();
    }
    if (m_input_handler_64) {
        Function5D5390();
    }
}

/* Install the state-5 confirmation/notification dialog and retain the value
   consumed by Function5C26C0 after the modal closes. */
// FUNCTION: WIZ8 0x005c25e0
void W8State5Controller005EF4CC::OpenNotification(
    const wchar_t* message, int kind, int value)
{
    m_dialog_value_6c = value;
    m_dialog_68 = new W8ModalDialogBase;
    if (!m_dialog_68) {
        return;
    }
    m_dialog_68->SetExtent(0xf0, 0xbe);
    m_dialog_68->SetOrigin(0xa0, 100);
    m_dialog_68->SetBackground("Data\\Dialogs\\DialogBackground.sti", 0);
    m_dialog_68->SetClientExtent(0xfa, 200);
    m_dialog_68->SetMessage(
        const_cast<wchar_t*>(message), 1, 0x32, 1, kind, 1, 1, 0, 0x15e);
    ActivateDialogRegion(0x138);
}

/* Apply the result of a state-5 confirmation dialog.  The dialog value selects
   delete, save-slot creation, option entry, imported-party replacement, or
   leaving the screen; cancellation only restores the mode-4 toggle. */
// FUNCTION: WIZ8 0x005c26c0
void W8State5Controller005EF4CC::Function5C26C0(
    int, unsigned char accepted)
{
    if (!accepted) {
        if (m_dialog_value_6c == 4) {
            m_text_50->ActivateSecondary(0);
        }
        return;
    }

    W8State5PartyCollection* collection =
        g_state5_party_collection_69c4ec;
    switch (m_dialog_value_6c) {
    case 1: {
        int selected = m_character_panel_20->m_selected_row;
        W8Character* character = collection->GetCharacter(selected);
        if (character->in_party) {
            collection->DetachFromParty(selected);
            character = collection->GetCharacter(selected);
        }
        char path[128];
        BuildCharacterPath00514EC0(path, character->name, -1);
        DeleteFileByName(path);
        collection->DeleteAt(selected);

        m_character_panel_20->m_range_7c = m_range;
        m_range->m_listener = m_character_panel_20;
        int maximum = collection->characters.count - 6;
        if (maximum < 0) {
            maximum = 0;
        }
        m_range->SetRange(0, maximum);
        m_range->SetEnabled(maximum > 0);
        m_range->Invalidate(0);
        if (selected > 0 || collection->characters.count == 0) {
            --selected;
        }
        SetSelection(selected, 0, 1);
        return;
    }
    case 2: {
        wchar_t slot_name[64];
        GetSaveSlotName005D3CC0(0, slot_name);
        if (m_input_handler_64) {
            delete m_input_handler_64;
        }
        m_input_handler_64 = 0;
        Function54B250(1, slot_name);
        return;
    }
    case 3:
        m_previous_mode = m_mode;
        SetSelection(-1, 1, 1);
        SetMode(2);
        return;
    case 4: {
        for (int index = 0; index < collection->characters.count; ++index) {
            W8Character* previous = collection->GetCharacter(index);
            if (!previous->in_party) {
                continue;
            }
            int slot = collection->FindPartySlot(index);
            W8Character* replacement = new W8Character;
            char path[128];
            BuildCharacterPath00514EC0(path, previous->name, -1);
            if (!LoadCharacter(path, replacement, -1, 0)) {
                memcpy(replacement, previous, sizeof(W8Character));
            }
            Function4EF610(slot + 2, 0);
            replacement->in_party = 0;
            collection->characters.SetAt(index, replacement);
        }
        SetMode(1);
        Function5C2C60(m_list_5c->m_selection);
        return;
    }
    case 5:
        RequestScreenTransition();
        return;
    }
}

/* Load the selected imported-party file, report its two failure classes, and
   refresh every control whose state depends on the resulting six party slots. */
// FUNCTION: WIZ8 0x005c2c60
void W8State5Controller005EF4CC::Function5C2C60(int selection)
{
    Function54B100();
    W8State5PartyCollection* collection =
        g_state5_party_collection_69c4ec;
    if (selection >= 0 && selection < collection->names.count) {
        char path[128];
        sprintf(path, "%s\\%s", "Saves\\Import",
                *collection->names.GetAt(selection));
        int result = Function558C40(path);
        if (result != 0) {
            Function54B100();
            OpenNotification(
                gppStringList[(result == 2 ? 0x1b60 : 0x1b5c) / 4],
                0, 0);
        }
    }

    m_text_54->SetVisible(CountActiveCharacters() != 0);
    m_text_54->Invalidate(0);
    int slot;
    for (slot = 0; slot < 6; ++slot) {
        unsigned char occupied =
            static_cast<unsigned char*>(g_status_685170.buffers.buffer_08)
                [slot * 0x106 + 0x20c];
        m_control_28->m_lsButtons.data[slot]->SetVisible(occupied);
    }
    m_control_28->Invalidate(0);
    for (slot = 0; slot < 6; ++slot) {
        unsigned char occupied =
            static_cast<unsigned char*>(g_status_685170.buffers.buffer_08)
                [slot * 0x106 + 0x20c];
        m_control_24->ControlAt(slot)->SetEnabled(
            m_mode == 1 && occupied && IsCharacterReadyToAdvance(slot + 2));
    }
    m_control_24->Invalidate(0);
    SetSelection(0, 1, 1);
}

/* Toggle the selected loose character into the active party, or detach an
   active member back into an owned loose record.  The selection then follows
   the nearest remaining active slot and both six-row panels are refreshed. */
// FUNCTION: WIZ8 0x005c2970
void W8State5Controller005EF4CC::Function5C2970()
{
    int selected = m_character_panel_20->m_selected_row;
    if (selected == -1) {
        return;
    }

    W8State5PartyCollection* collection =
        g_state5_party_collection_69c4ec;
    W8Character* character = collection->GetCharacter(selected);
    if (!character->in_party) {
        int slot = Function4EF4A0(character, -1);
        if (slot != -1) {
            delete character;
            collection->characters.SetAt(selected, &g_party_characters[slot]);
        }
        if (selected < collection->characters.count - 1) {
            ++selected;
        }
        SetSelection(selected, 0, 1);
    }
    else {
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
            if (static_cast<unsigned char*>(g_status_685170.buffers.buffer_08)
                    [(next_slot + 2) * 0x106]) {
                found = true;
                break;
            }
            ++next_slot;
        }
        if (found) {
            SetSelection(next_slot, 1, 1);
        }
        else {
            SetSelection(selected, 0, 0);
        }
    }

    for (int slot = 0; slot < 6; ++slot) {
        unsigned char occupied =
            static_cast<unsigned char*>(g_status_685170.buffers.buffer_08)
                [slot * 0x106 + 0x20c];
        m_control_28->m_lsButtons.data[slot]->SetVisible(occupied);
    }
    m_control_28->Invalidate(0);
    m_character_panel_20->Invalidate(0);
    m_text_54->SetVisible(CountActiveCharacters() != 0);
    m_text_54->Invalidate(0);
}

/* Enter the state-5 party builder.  The persistent collection is rebuilt from
   the six occupied player slots whenever this is a fresh entry or a return
   from state 3 that is not preserving mode 1.  Loose CHR files are then merged
   and ordered before the controller presents them. */
// FUNCTION: WIZ8 0x005c2de0
unsigned char State5Enter005C2DE0(void)
{
    SetViewport(0, 0, 0x280, 0x1e0);
    NoOp();
    Function40B290();
    ResetRegions();
    UpdateHeldItemCursor();
    SetFontObjectPalette16BPP(g_font_683660, g_colour_68ee08);
    SetFontObjectPalette16BPP(g_wiz_text_bold_font_683664,
                              g_font_palette_wiz_text_bold_68ee0c);

    W8State5PartyCollection* collection = g_state5_party_collection_69c4ec;
    if (!collection) {
        Function54B100();
        collection = new W8State5PartyCollection;
        g_state5_party_collection_69c4ec = collection;

        for (int index = 0; index < collection->characters.count; ++index) {
            W8Character* character = collection->GetCharacter(index);
            if (!character->in_party) {
                delete character;
            }
        }
        collection->characters.count = 0;
        unsigned char* party_state =
            static_cast<unsigned char*>(g_status_685170.buffers.buffer_08);
        for (int slot = 2; slot < 8; ++slot) {
            if (party_state[slot * 0x106]) {
                collection->characters.Add(&g_party_characters[slot]);
            }
        }
        collection->LoadExternalCharacters();
        collection->SortCharactersByWriteTime();

        g_state5_controller_69c4e8 = new W8State5Controller005EF4CC;
        g_state5_controller_69c4e8->Setup();
    }
    else {
        if (g_dword_647bc0 == 3 && g_state5_controller_69c4e8->m_mode != 1) {
            for (int index = 0; index < collection->characters.count; ++index) {
                W8Character* character = collection->GetCharacter(index);
                if (!character->in_party) {
                    delete character;
                }
            }
            collection->characters.count = 0;
            unsigned char* party_state =
                static_cast<unsigned char*>(g_status_685170.buffers.buffer_08);
            for (int slot = 2; slot < 8; ++slot) {
                if (party_state[slot * 0x106]) {
                    collection->characters.Add(&g_party_characters[slot]);
                }
            }
            collection->LoadExternalCharacters();
            collection->SortCharactersByWriteTime();
            g_state5_controller_69c4e8->SetSelection(0, 0, 1);
        }
        g_state5_controller_69c4e8->SetMode(g_state5_controller_69c4e8->m_mode);
    }
    Function48FC10("MainMenu.MPL", 1, 1);
    return 1;
}

/* The state persists its controller and imported-character collection while
   another screen is temporarily stacked over it.  A leaving tick destroys
   both; an ordinary tick only performs the common display/region reset. */
// FUNCTION: WIZ8 0x005c30b0
unsigned char State5Tick005C30B0(int leaving)
{
    if (leaving) {
        W8State5PartyCollection* collection =
            g_state5_party_collection_69c4ec;
        if (collection) {
            delete collection;
        }
        W8State5Controller005EF4CC* controller =
            g_state5_controller_69c4e8;
        g_state5_party_collection_69c4ec = 0;
        if (controller) {
            delete controller;
        }
        g_state5_controller_69c4e8 = 0;
    }
    NoOp();
    ShutdownDisplayList();
    ResetRegions();
    return 1;
}

/* Run modal close-out, cursor/region dispatch and the state-5 keyboard layer.
   Region handling gets first refusal.  The optional state-5 input handler gets
   second refusal, after which the screen owns Return, Escape, left/right row
   movement and Delete. */
// FUNCTION: WIZ8 0x005c3120
void State5Frame005C3120(void)
{
    W8ScreenPoint point;
    W8ScreenPoint current;
    InputAtom input;

    if (g_flag_689b32) {
        Function591780();
    }
    GetScreenPoint004284F0(&point);
    W8State5Controller005EF4CC* controller = g_state5_controller_69c4e8;
    if (controller->m_dialog_68) {
        controller->m_dialog_68->Close();
        if (!controller->m_dialog_68->is_open) {
            unsigned char result = controller->m_dialog_68->close_result;
            delete controller->m_dialog_68;
            controller->m_dialog_68 = 0;
            ClearActiveRegionIfMatches(0x138);
            controller->Function5C1ED0();
            controller->Function5C26C0(controller->m_dialog_value_6c, result);
        }
    }
    if (controller->m_input_handler_64) {
        GetScreenPoint004284F0(&current);
        Function40B510(MOUSE_POS, static_cast<unsigned short>(current.x),
                       static_cast<unsigned short>(current.y),
                       g_flag_6f04ed, g_flag_6f04e8);
    }
    Function4F1360(point.x, point.y);
    while (DequeueEvent(&input) == 1) {
        if (!DispatchScreenInput004F1910(&input) &&
            (!controller->m_input_handler_64 ||
             !controller->m_input_handler_64->HandleInput(&input)) &&
            (input.usEvent == KEY_DOWN || input.usEvent == KEY_REPEAT)) {
            switch (input.usParam) {
            case VK_RETURN:
                controller->Function5C2970();
                break;
            case VK_ESCAPE:
                switch (controller->m_mode) {
                case 0:
                case 1:
                    if (CountActiveCharacters() == 0) {
                        RequestScreenTransition();
                    }
                    else {
                        controller->OpenNotification(
                            gppStringList[0x1b4c / 4],
                            1, 5);
                    }
                    break;
                case 2:
                case 3:
                case 4:
                    controller->SetMode(controller->m_previous_mode);
                    controller->SetSelection(
                        0, controller->m_previous_mode == 1, 1);
                    break;
                }
                break;
            case VK_UP:
                if (controller->m_mode == 0 &&
                    controller->m_character_panel_20->m_selected_row > 0) {
                    controller->SetSelection(
                        controller->m_character_panel_20->m_selected_row - 1,
                        0, 1);
                }
                break;
            case VK_DOWN:
                if (controller->m_mode == 0 &&
                    controller->m_character_panel_20->m_selected_row <
                        g_state5_party_collection_69c4ec->characters.count - 1) {
                    controller->SetSelection(
                        controller->m_character_panel_20->m_selected_row + 1,
                        0, 1);
                }
                break;
            case VK_DELETE:
                if (controller->m_mode == 0 && controller->m_character_18) {
                    controller->OnPrimary(controller->m_text_48);
                }
                break;
            }
        }
    }
    Function52E750();
    controller->Function5C1F40();
    Function426790();
}

// GLOBAL: WIZ8 0x006875B4
unsigned char g_flag_6875b4;
// GLOBAL: WIZ8 0x0068DE50
int g_value_68de50;

/* Lifecycle record 2's frame close-out, and the only slot that record fills:
   its other four are the shared do-nothing filler. Every path asks for a screen
   transition, records one of four codes and queues screen 0, so the record is a
   pure router - it selects which of the four the transition reports and then
   leaves. Nothing here names the four codes or the two globals that pick them.

   The retail body carries four copies of the call tail, each loading its code
   through EAX rather than pushing it. That is VC6 duplicating one tail, not the
   original writing four: the tail is written once here and the four copies come
   back, where writing the four calls out literally emits direct pushes and four
   instructions too few. */
// FUNCTION: WIZ8 0x005c3800
void Screen2Finish(void)
{
    unsigned long code;

    RequestScreenTransition();
    if (!g_flag_6875b4) {
        code = 4;
    }
    else {
        switch (g_value_68de50) {
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
    SetValue64D8AC(code);
    SetPendingScreenState(0);
}
