#pragma once
#include "wiz8/screen_state.h"

#include "wiz8/local_code/ControlsRect.h"
#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/local_code/ControlSelection.h"
#include <stddef.h>

#include "wiz8/character.h"
#include "wiz8/compat/compiler.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/vector.h"

#include "input.h"

class W8DialogBase;
class W8CharacterScreen;
class W8CharacterPageEntry;

class W8CharacterPageEntryListener {
public:
    virtual void AdjustEntry(W8CharacterPageEntry* entry, int delta) = 0;
    virtual void ShowEntryInfo(W8CharacterPageEntry* entry) = 0;
};

/* One value row shared by the skills and attributes pages. */
class W8CharacterPageEntry : public W8TextControl::Listener {
public:
    W8CharacterPageEntry(Controls* owner, int x, int y, bool compact); /* 0x005AF690 */
    virtual ~W8CharacterPageEntry()
    {
        delete m_label_014;
        delete m_first_text_018;
        delete m_second_text_01c;
    }
    void SetContent(unsigned int id, const wchar_t* label, unsigned int* first, int* second,
                    int* third, int help_id);                  /* 0x005AF9E0 */
    void SetEnabled(bool enabled);                             /* 0x005AFA90 */
    void SetIncrementAllowed(bool allowed);                    /* 0x005AFC20 */
    void Redraw();                                             /* 0x005AFAF0 */
    void SetLabelFontState(int state);                         /* 0x005AFBF0 */
    void MarkDirty();                                          /* 0x005AFC00 */
    void UpdateButtons();                                      /* 0x005AFD10 */
    virtual void OnPrimary(W8TextControl* control) override;   /* 0x005AFC50 */
    virtual void OnSecondary(W8TextControl* control) override; /* 0x005AFCB0 */
    void SetHelpActive005AFAE0(bool active);                   /* 0x005AFAE0 */

    W8CharacterPageEntryListener* m_listener_004;
    W8TextControl* m_increment_008;
    W8TextControl* m_decrement_00c;
    W8TextControl* m_help_010;
    W8TextBuffer* m_label_014;
    W8TextBuffer* m_first_text_018;
    W8TextBuffer* m_second_text_01c;
    unsigned int* m_first_020;
    int* m_second_024;
    int* m_third_028;
    unsigned int m_id_02c;
    int m_x_030;
    int m_y_034;
    bool m_draw_background_038;
    bool m_dirty_039;
    bool m_enabled_03a;
    bool m_increment_allowed_03b;
};
static_assert(sizeof(W8CharacterPageEntry) == 0x3c, "W8CharacterPageEntry_size");

/* Common 0x70-byte base constructed by 0x005AFD90.  The four page constructors
   below all call it and SelectPage dispatches these primary slots. */
class W8CharacterPage : public Controls {
public:
    W8CharacterPage(int render_target);                           /* 0x005AFD90 */
    virtual ~W8CharacterPage();                                   /* 0x005AFE40 */
    virtual void Invalidate(const W8ControlsRect* rect) override; /* 0x005AFF50 */
    virtual void Redraw() override;                               /* 0x005AFF20 */
    virtual void SetCharacter(W8Character* character, W8CharacterCreationState* creation_state,
                              int mode); /* 0x005AFF00 */
    virtual void Activate() = 0;
    virtual void Deactivate(); /* 0x005CA1F0 */
    virtual void Accept() = 0;
    virtual void GetNavigationState(bool* next_enabled, bool* exit_enabled) = 0;
    virtual void HandleInput(InputAtom* input); /* 0x005B1BE0 */
    virtual void Refresh();                     /* 0x005B1BF0 */
    virtual void Prepare();                     /* 0x005AFFA0 */
    W8GrowableVector<W8CharacterPageEntry*> m_entries_04c;
    W8CharacterScreen* m_screen_05c;
    W8Character* m_character_060;
    W8CharacterCreationState* m_creation_state_064;
    int m_mode_068;
    unsigned char m_prepared_06c;
    unsigned char m_dirty_06d;
    unsigned char pad_06e[2];

    void AddEntry(W8CharacterPageEntry* entry); /* 0x005AFFC0 */
};
static_assert(sizeof(W8CharacterPage) == 0x70, "W8CharacterPage_size");

class W8CharacterStatsRow005EF750;

/* One record of the three stats-page row tables (profession, race, sex).
   0x00 is the catalogue object id, 0x04/0x08 its two images, 0x0C the name
   message id and 0x0E the selectable flag. */
struct W8CharacterStatsRecord {
    unsigned int unknown_00;
    int unknown_04;
    int unknown_08;
    unsigned short name_id_0c;
    unsigned char enabled_0e;
    unsigned char pad_0f;
};
static_assert(sizeof(W8CharacterStatsRecord) == 0x10, "W8CharacterStatsRecord_size");

/* The 0xEF6B0 value control: it carries the record currently shown and the
   record to fall back to when the character has none. 0x005C8F60 is the
   hierarchy's compiler-emitted destructor, so the class leaves it defaulted. */
// VTABLE: WIZ8 0x005ef6b0 W8CharacterStatsValue005EF6B0
class W8CharacterStatsValue005EF6B0 : public W8TextControl {
public:
    W8CharacterStatsValue005EF6B0(Controls* owner, int x, int y,
                                  const W8CharacterStatsRecord* default_record);
    virtual void Redraw(int full_redraw) override;
    virtual void OnRightButtonUp(int event) override;
    void SetRecord(const W8CharacterStatsRecord* record);

    const W8CharacterStatsRecord* m_record_0b8;
    const W8CharacterStatsRecord* m_default_record_0bc;
};
static_assert(sizeof(W8CharacterStatsValue005EF6B0) == 0xc0, "W8CharacterStatsValue005EF6B0_size");

/* Stats-row callbacks installed at +0x70 of the page. The row itself raises
   slots 0 and 3; the page's input handling raises slots 1 and 2. */
class W8CharacterStatsRowListener005EF768 {
public:
    virtual void OnRowValueChanged(W8CharacterStatsRow005EF750* row, int value) = 0;
    virtual void OnRowExpanded(W8CharacterStatsRow005EF750* row) = 0;
    virtual void OnRowCollapsed(W8CharacterStatsRow005EF750* row) = 0;
    virtual void OnRowInfoRequested(W8CharacterStatsRow005EF750* row, int value) = 0;
};
static_assert(sizeof(W8CharacterStatsRowListener005EF768) == 0x4,
              "W8CharacterStatsRowListener005EF768_size");

/* A stats-page value row: a decrement arrow, an increment arrow and a value
   control whose current index indexes the row's 0x10-byte record table. */
class W8CharacterStatsRow005EF750 : public W8TextControl::Listener {
public:
    W8CharacterStatsRow005EF750();
    virtual void OnPrimary(W8TextControl* control) override;   /* 0x005c9760 */
    virtual void OnSecondary(W8TextControl* control) override; /* 0x005c9a50 */
    void Initialize(Controls* owner, unsigned int* region_set, int x, int y, int count,
                    const W8CharacterStatsRecord* table,
                    const W8CharacterStatsRecord* default_record, int help_first, int help_second,
                    int help_value); /* 0x005c9310 */
    void BuildSubpanel();            /* 0x005c94e0 */
    void SetValue(int index);        /* 0x005c96c0 */

    int m_value_004;
    unsigned short m_count_008;
    unsigned char pad_00a[2];
    int m_x_00c;
    int m_y_010;
    unsigned int* m_region_set_014;
    const W8CharacterStatsRecord* m_table_018;
    W8TextControl* m_decrement_01c;
    W8TextControl* m_increment_020;
    W8CharacterStatsValue005EF6B0* m_value_control_024;
    Controls* m_subpanel_028;
    W8TextControl** m_subpanel_entries_02c;
    W8CharacterStatsRowListener005EF768* m_listener_030;
};
static_assert(sizeof(W8CharacterStatsRow005EF750) == 0x34, "W8CharacterStatsRow005EF750_size");

class W8CharacterPage005EF778 : public W8CharacterPage,
                                public W8CharacterStatsRowListener005EF768,
                                public W8CharacterPageEntryListener,
                                public W8TextControl::Listener {
public:
    W8CharacterPage005EF778() : W8CharacterPage(0x104) {}
    virtual ~W8CharacterPage005EF778() override;
    virtual void Redraw() override;
    virtual void SetCharacter(W8Character*, W8CharacterCreationState*, int) override;
    virtual void Activate() override;
    virtual void Accept() override;
    virtual void GetNavigationState(bool*, bool*) override;
    virtual void HandleInput(InputAtom*) override;
    virtual void Refresh() override;
    virtual void Prepare() override;
    virtual void OnRowValueChanged(W8CharacterStatsRow005EF750* row, int value) override;
    virtual void OnRowExpanded(W8CharacterStatsRow005EF750* row) override;
    virtual void OnRowCollapsed(W8CharacterStatsRow005EF750* row) override;
    virtual void OnRowInfoRequested(W8CharacterStatsRow005EF750* row, int value) override;
    virtual void AdjustEntry(W8CharacterPageEntry*, int) override;
    virtual void ShowEntryInfo(W8CharacterPageEntry*) override;
    virtual void OnPrimary(W8TextControl*) override;
    virtual void OnSecondary(W8TextControl*) override;

private:
    void UpdateRowValues(); /* 0x005ca200 */

public:
    W8CharacterStatsRow005EF750* m_profession_row_07c;
    W8CharacterStatsRow005EF750* m_race_row_080;
    W8CharacterStatsRow005EF750* m_gender_row_084;
    bool m_navigation_state_088;
    unsigned char m_rows_initialized_089;
    unsigned char pad_08a[2];
    W8TextControl* m_attribute_controls_08c[5];
};
static_assert(sizeof(W8CharacterPage005EF778) == 0xa0, "W8CharacterPage005EF778_size");

struct W8CharacterSpellEntry {
    int realm;
    unsigned int spell;
    unsigned char selectable;
    unsigned char selected;
    unsigned char pad_00a[2];
};
static_assert(sizeof(W8CharacterSpellEntry) == 0xc, "W8CharacterSpellEntry_size");

class W8CharacterSpellListListener {
public:
    virtual void SelectSpell(unsigned int entry) = 0;
    virtual void ShowSpellInfo(unsigned int entry) = 0;
};

class W8CharacterSpellList;

/* CGSSpellsPage.cpp: the 0x558-byte middle is 114 spell entries, not an
   embedded framework object. The range-list callbacks use the +0x70 base. */
class W8CharacterPage005EF664 : public W8CharacterPage, public W8CharacterSpellListListener {
public:
    W8CharacterPage005EF664();
    virtual ~W8CharacterPage005EF664() override;
    virtual void SetCharacter(W8Character*, W8CharacterCreationState*, int) override;
    virtual void Redraw() override;
    virtual void Activate() override;
    virtual void Deactivate() override;
    virtual void Accept() override;
    virtual void GetNavigationState(bool*, bool*) override;
    virtual void Refresh() override;
    virtual void SelectSpell(unsigned int entry) override;
    virtual void ShowSpellInfo(unsigned int entry) override;

private:
    void UpdateSpellLists();
    W8CharacterSpellList* m_realms_074[6];
    W8CharacterSpellEntry m_spell_data_08c[114];
    W8GameTimer m_animation_timer_5e4;
    unsigned int m_animation_frames_608[6];
    unsigned int m_last_selected_620;
};
static_assert(sizeof(W8CharacterPage005EF664) == 0x624, "W8CharacterPage005EF664_size");

class W8CharacterPage005EF5C8 : public W8CharacterPage, public W8CharacterPageEntryListener {
public:
    W8CharacterPage005EF5C8() : W8CharacterPage(0x108) {}
    virtual ~W8CharacterPage005EF5C8() override {}
    virtual void Redraw() override;
    virtual void SetCharacter(W8Character*, W8CharacterCreationState*, int) override;
    virtual void Activate() override;
    virtual void Accept() override;
    virtual void GetNavigationState(bool*, bool*) override;
    virtual void AdjustEntry(W8CharacterPageEntry*, int) override;
    virtual void ShowEntryInfo(W8CharacterPageEntry*) override;
    virtual void Refresh() override;

private:
    void UpdateEntries(); /* 0x005C7B50 */
    unsigned char m_force_redraw_074;
    unsigned char m_show_fifth_category_075;
    bool m_navigation_state_076;
    unsigned char unknown_077;
};
static_assert(sizeof(W8CharacterPage005EF5C8) == 0x78, "W8CharacterPage005EF5C8_size");

class W8CharacterPage005EF57C : public W8CharacterPage,
                                public W8ControlSelectionListener,
                                public W8TextControl::Listener {
public:
    W8CharacterPage005EF57C()
        : W8CharacterPage(0x105), m_animation_timer_0d4(0.4f, 1), m_animation_active_0fc(0)
    {
    }
    virtual ~W8CharacterPage005EF57C() override;
    virtual void Redraw() override;
    virtual void SetCharacter(W8Character*, W8CharacterCreationState*, int) override;
    virtual void Activate() override;
    virtual void Deactivate() override;
    virtual void Accept() override;
    virtual void GetNavigationState(bool*, bool*) override;
    virtual void HandleInput(InputAtom*) override;
    virtual void Refresh() override;
    virtual void OnSelectionChanged(W8ControlSelection*, int) override;
    virtual void OnPrimary(W8TextControl*) override;

private:
    W8TextControl* m_control_078;
    W8TextControl* m_control_07c;
    W8TextControl* m_control_080;
    W8TextControl* m_control_084;
    W8TextControl* m_randomize_088;
    W8ControlSelection m_personality_selection_08c;
    W8ControlSelection m_voice_selection_0b0;
    W8GameTimer m_animation_timer_0d4;
    int m_animation_frame_0f8;
    unsigned char m_animation_active_0fc;
    unsigned char m_description_dirty_0fd;
    unsigned char m_portrait_dirty_0fe;
    unsigned char pad_0ff;
};
static_assert(sizeof(W8CharacterPage005EF57C) == 0x100, "W8CharacterPage005EF57C_size");

W8CharacterPage005EF778* CreateCharacterPage005CBA90();
W8CharacterPage005EF664* CreateCharacterPage005C8DE0();
W8CharacterPage005EF5C8* CreateCharacterPage005C7CC0();
W8CharacterPage005EF57C* CreateCharacterPage005C73F0();

/* Realm animation records shared by the stats page's resistance icons and the
   spells page's realm list. The stats page reads only initial_frame, through
   the realm-1 offset the spells page's array lays out. */
struct W8SpellRealmAnimation {
    int image;
    unsigned int frame_count;
    unsigned int initial_frame;
};
extern W8SpellRealmAnimation g_spell_realm_animations_00648c90[6];
extern const wchar_t g_format_s_parenthesized_s_00617584[];

/* One message id per character trait, indexed by trait id. */
extern unsigned short g_character_trait_name_ids_61e530[0x20];

/* Attribute-entry and skill-name message id tables shared by the stats and
   skills pages. */
extern unsigned short g_character_description_first_ids_61e3a4[22];
extern unsigned short g_character_skill_name_ids_61e454[84];
/* 0x0061E430: one row per gender, the third entry the possessive the item
   notices print. */
extern unsigned short g_gender_name_message_rows_61e430[4][4];
/* One message id per final-page personality slot; the array's terminating zero
   keeps its ten-entry extent. */
extern unsigned short g_personality_message_ids_61e674[10];

/* Page and per-row region sets. */
extern unsigned int g_character_stats_region_set_0069c550;

/* Skill-availability bookkeeping raised by RefreshCharacterSkillAvailability00553CD0 while the character
   screen is open: adjust the named page-2 entry and refresh that page. */
void ResetCharacterScreenSkill(int skill_id);
void RefundCharacterScreenSkill(int skill_id);

/* Primary interface at 0x005EF224, used by the pages to raise the screen-owned
   dialogs and to query the current character. */
class W8CharacterPageHost {
public:
    virtual void UpdateNavigation(W8CharacterPage* page) = 0;
    virtual void ShowDialog005B0610(int value) = 0;
    virtual void ShowProfessionInfo(unsigned int profession) = 0;
    virtual void ShowRaceInfo(unsigned int race) = 0;
    virtual void ShowAttributeInfo005B07C0(unsigned int attribute) = 0;
    virtual void ShowAttributeInfo005B0850(unsigned int attribute) = 0;
    virtual void ShowDialog005B08E0(int value) = 0;
    virtual void ShowDescription(int first, int second) = 0;
    virtual void ShowCharacterSummary() = 0;
    virtual unsigned char HasDialog() = 0;
    virtual W8Character* GetOriginalCharacter() = 0;
};
static_assert(sizeof(W8CharacterPageHost) == 0x4, "W8CharacterPageHost_size");

class W8CharacterScreen : public W8CharacterPageHost, public W8TextControl::Listener {
public:
    W8CharacterScreen(int mode, W8Character* character);             /* 0x005B0040 */
    void BuildControls();                                            /* 0x005B0140 */
    void UpdateDialog();                                             /* 0x005B04B0 */
    void AdvancePage(unsigned char forward);                         /* 0x005B0B50 */
    void SelectPage(int index);                                      /* 0x005B0D50 */
    void SyncCharacterForPage(int index);                            /* 0x005B0F30 */
    unsigned char CommitCharacter();                                 /* 0x005B0FD0 */
    void DrawHeader();                                               /* 0x005B1110 */
    void ShowMessage(wchar_t* text, int confirmation, int response); /* 0x005B1430 */
    void HandleDialogResult(int response, unsigned char accepted);   /* 0x005B1520 */
    unsigned char ValidateName();                                    /* 0x005B1670 */

    virtual void UpdateNavigation(W8CharacterPage* page) override;
    virtual void ShowDialog005B0610(int value) override;
    virtual void ShowProfessionInfo(unsigned int profession) override;
    virtual void ShowRaceInfo(unsigned int race) override;
    virtual void ShowAttributeInfo005B07C0(unsigned int attribute) override;
    virtual void ShowAttributeInfo005B0850(unsigned int attribute) override;
    virtual void ShowDialog005B08E0(int value) override;
    virtual void ShowDescription(int first, int second) override;
    virtual void ShowCharacterSummary() override;
    virtual unsigned char HasDialog() override;
    virtual W8Character* GetOriginalCharacter() override;
    virtual void OnPrimary(W8TextControl* control) override;
    virtual void OnSecondary(W8TextControl* control) override;

    int m_mode_008;
    int m_page_index_00c;
    unsigned char m_header_dirty_010;
    unsigned char pad_011[3];
    W8Character* m_original_014;
    W8Character m_character_018;
    unsigned char pad_187a[2];
    W8CharacterCreationState m_creation_state_187c;
    unsigned char m_block_advance_1aec;
    unsigned char m_confirm_profession_1aed;
    unsigned char m_force_transition_1aee;
    unsigned char pad_1aef;
    Controls* m_controls_1af0;
    W8TextControl* m_previous_1af4;
    W8TextControl* m_next_1af8;
    W8TextControl* m_exit_1afc;
    W8TextControl* m_accept_1b00;
    W8TextControl* m_reset_1b04;
    bool m_page_enabled_1b08[4];
    W8CharacterPage* m_pages_1b0c[4];
    W8DialogBase* m_dialog_1b1c;
    unsigned int m_dialog_response_1b20;
    unsigned char m_capture_dialog_result_1b24;
    unsigned char pad_1b25[3];
};
static_assert(sizeof(W8CharacterScreen) == 0x1b28, "W8CharacterScreen_size");

extern W8CharacterScreen* g_character_screen_0069c2e8;

/* CharacterScreen.cpp GLOBAL at 0x0061E3F0: per-profession message indexes. */
extern unsigned short g_profession_name_message_ids_61e3f0[32];

/* Refresh the character-screen response when a party slot changes. */
void RefreshCharacterScreenPartySlot(unsigned int party_slot); /* 0x005B1AD0 */
