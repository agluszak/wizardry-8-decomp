#pragma once
#include "wiz8/layouts/screen_state.h"

#include "wiz8/local_code/ControlsRect.h"
#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/local_code/ControlSelection.h"
#include <stddef.h>

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
#include "wiz8/compat/compiler.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/local_code/Controls.h"
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
    W8CharacterPageEntry(Controls* owner, int x, int y, bool compact);
    virtual ~W8CharacterPageEntry()
    {
        delete m_label;
        delete m_first_text;
        delete m_second_text;
    }
    void SetContent(unsigned int id, const wchar_t* label, unsigned int* first, int* second,
                    int* third, int help_id);
    void SetEnabled(bool enabled);
    void SetIncrementAllowed(bool allowed);
    void Redraw();
    void SetLabelFontState(int state);
    void MarkDirty();
    void UpdateButtons();
    virtual void OnPrimary(W8TextControl* control) override;
    virtual void OnSecondary(W8TextControl* control) override;
    void SetHelpActive(bool active);

    W8CharacterPageEntryListener* m_listener;
    W8TextControl* m_increment;
    W8TextControl* m_decrement;
    W8TextControl* m_help;
    W8TextBuffer* m_label;
    W8TextBuffer* m_first_text;
    W8TextBuffer* m_second_text;
    unsigned int* m_first;
    int* m_second;
    int* m_third;
    unsigned int m_id;
    int m_x;
    int m_y;
    bool m_draw_background;
    bool m_dirty;
    bool m_enabled;
    bool m_increment_allowed;
};
W8_ABI_ASSERT(sizeof(W8CharacterPageEntry) == 0x3c, "W8CharacterPageEntry_size");

/* Common base of the four character screen pages. */
class W8CharacterPage : public Controls {
public:
    W8CharacterPage(int catalog_object);
    virtual ~W8CharacterPage();
    virtual void Invalidate(const W8ControlsRect* rect) override;
    virtual void Redraw() override;
    virtual void SetCharacter(W8Character* character, W8CharacterCreationState* creation_state,
                              int mode);
    virtual void Activate() = 0;
    virtual void Deactivate() = 0;
    virtual void Accept() = 0;
    virtual void GetNavigationState(bool* next_enabled, bool* exit_enabled) = 0;
    virtual void HandleInput(InputAtom* input);
    virtual void Refresh();
    virtual void Prepare();
    W8Vector<W8CharacterPageEntry*> m_entries;
    W8CharacterScreen* m_screen;
    W8Character* m_character;
    W8CharacterCreationState* m_creation_state;
    int m_mode;
    bool m_prepared;
    bool m_dirty;
    unsigned char pad_06e[2];

    void AddEntry(W8CharacterPageEntry* entry);
};
W8_ABI_ASSERT(sizeof(W8CharacterPage) == 0x70, "W8CharacterPage_size");

class W8CharacterStatsRow;

/* One record of the three stats-page row tables (profession, race, sex).
   0x00 is the catalogue object id, 0x04/0x08 its two images, 0x0C the name
   message id and 0x0E the selectable flag. */
struct W8CharacterStatsRecord {
    /* The video-object catalog id handed to DrawCatalogImage's
       `object` argument. */
    unsigned int object;
    /* Catalog image ids drawn while the control is enabled and
       disabled respectively. */
    int image_enabled;
    int image_disabled;
    unsigned short name_id;
    unsigned char enabled;
    unsigned char pad_0f;
};
static_assert(sizeof(W8CharacterStatsRecord) == 0x10, "W8CharacterStatsRecord_size");

/* The stats value control: it carries the record currently shown and the
   record to fall back to when the character has none. */
// VTABLE: WIZ8 0x005ef6b0 W8CharacterStatsValue
class W8CharacterStatsValue : public W8TextControl {
public:
    W8CharacterStatsValue(Controls* owner, int x, int y,
                          const W8CharacterStatsRecord* default_record);
    virtual void Redraw(bool full_redraw) override;
    virtual void OnRightButtonUp(int event) override;
    void SetRecord(const W8CharacterStatsRecord* record);

    const W8CharacterStatsRecord* m_record;
    const W8CharacterStatsRecord* m_default_record;
};
W8_ABI_ASSERT(sizeof(W8CharacterStatsValue) == 0xc0, "W8CharacterStatsValue_size");

/* Stats-row callbacks. The row itself raises slots 0 and 3; the page's input
   handling raises slots 1 and 2. */
class W8CharacterStatsRowListener {
public:
    virtual void OnRowValueChanged(W8CharacterStatsRow* row, int value) = 0;
    virtual void OnRowExpanded(W8CharacterStatsRow* row) = 0;
    virtual void OnRowCollapsed(W8CharacterStatsRow* row) = 0;
    virtual void OnRowInfoRequested(W8CharacterStatsRow* row, int value) = 0;
};
W8_ABI_ASSERT(sizeof(W8CharacterStatsRowListener) == 0x4, "W8CharacterStatsRowListener_size");

/* A stats-page value row: a decrement arrow, an increment arrow and a value
   control whose current index indexes the row's 0x10-byte record table. */
class W8CharacterStatsRow : public W8TextControl::Listener {
public:
    W8CharacterStatsRow();
    ~W8CharacterStatsRow()
    {
        delete m_subpanel;
        if (m_subpanel_entries != 0) {
            for (unsigned short index = 0; index < m_count; ++index) {
                delete m_subpanel_entries[index];
            }
            delete[] m_subpanel_entries;
        }
    }
    virtual void OnPrimary(W8TextControl* control) override;
    virtual void OnSecondary(W8TextControl* control) override;
    void Initialize(Controls* owner, unsigned int* region_set, int x, int y, int count,
                    const W8CharacterStatsRecord* table,
                    const W8CharacterStatsRecord* default_record, int help_first, int help_second,
                    int help_value);
    /* Descriptive names for the collapse operations expanded in row/page input. */
    void Collapse();
    void CollapseIfNotHovered();
    void Invalidate();
    void BuildSubpanel();
    void SetValue(int index);

    int m_index; /* current stat index */
    unsigned short m_count;
    unsigned char pad_00a[2];
    int m_x;
    int m_y;
    unsigned int* m_region_set;
    const W8CharacterStatsRecord* m_table;
    W8TextControl* m_decrement;
    W8TextControl* m_increment;
    W8CharacterStatsValue* m_value_control;
    Controls* m_subpanel;
    W8TextControl** m_subpanel_entries;
    W8CharacterStatsRowListener* m_listener;
};
W8_ABI_ASSERT(sizeof(W8CharacterStatsRow) == 0x34, "W8CharacterStatsRow_size");

// VTABLE: WIZ8 0x005ef778 W8CharacterPage
class W8CharacterStatsPage : public W8CharacterPage,
                             public W8CharacterStatsRowListener,
                             public W8CharacterPageEntryListener,
                             public W8TextControl::Listener {
public:
    W8CharacterStatsPage() : W8CharacterPage(0x104) {}
    virtual ~W8CharacterStatsPage() override;
    virtual void Redraw() override;
    virtual void SetCharacter(W8Character*, W8CharacterCreationState*, int) override;
    virtual void Activate() override;
    virtual void Deactivate() override;
    virtual void Accept() override;
    virtual void GetNavigationState(bool*, bool*) override;
    virtual void HandleInput(InputAtom*) override;
    virtual void Refresh() override;
    virtual void Prepare() override;
    virtual void OnRowValueChanged(W8CharacterStatsRow* row, int value) override;
    virtual void OnRowExpanded(W8CharacterStatsRow* row) override;
    virtual void OnRowCollapsed(W8CharacterStatsRow* row) override;
    virtual void OnRowInfoRequested(W8CharacterStatsRow* row, int value) override;
    virtual void AdjustEntry(W8CharacterPageEntry*, int) override;
    virtual void ShowEntryInfo(W8CharacterPageEntry*) override;
    virtual void OnPrimary(W8TextControl*) override;
    virtual void OnSecondary(W8TextControl*) override;

private:
    void UpdateRowValues();
    void SetRowControlsActive(W8CharacterStatsRow* row, bool active);

public:
    W8CharacterStatsRow* m_profession_row;
    W8CharacterStatsRow* m_race_row;
    W8CharacterStatsRow* m_gender_row;
    bool nav_next_state;
    bool m_rows_initialized;
    unsigned char pad_08a[2];
    W8TextControl* m_attribute_controls[5];
};
W8_ABI_ASSERT(sizeof(W8CharacterStatsPage) == 0xa0, "W8CharacterStatsPage_size");
W8_ASSERT_BASE_END(W8CharacterStatsPage, W8TextControl::Listener, m_profession_row, 0x78);

struct W8CharacterSpellEntry {
    W8SpellRealm realm;
    unsigned int spell;
    bool fSelectable;
    bool selected;
};
static_assert(sizeof(W8CharacterSpellEntry) == 0xc, "W8CharacterSpellEntry_size");

class W8CharacterSpellListListener {
public:
    virtual void SelectSpell(unsigned int entry) = 0;
    virtual void ShowSpellInfo(unsigned int entry) = 0;
};

class W8CharacterSpellList;

class W8CharacterSpellsPage : public W8CharacterPage, public W8CharacterSpellListListener {
public:
    W8CharacterSpellsPage() : W8CharacterPage(0x109), anim_timer(0.05f, 1)
    {
        for (int realm = 0; realm < 6; ++realm) {
            m_realms[realm] = 0;
        }
    }
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
    W8CharacterSpellList* m_realms[6];
    W8CharacterSpellEntry m_SpellData[114];
    W8GameTimer anim_timer;
    unsigned int m_animation_frames[6];
    unsigned int m_last_selected;
};
W8_ABI_ASSERT(sizeof(W8CharacterSpellsPage) == 0x624, "W8CharacterSpellsPage_size");

class W8CharacterSkillsPage : public W8CharacterPage, public W8CharacterPageEntryListener {
public:
    W8CharacterSkillsPage() : W8CharacterPage(0x108) {}
    virtual void Redraw() override;
    virtual void SetCharacter(W8Character*, W8CharacterCreationState*, int) override;
    virtual void Activate() override;
    virtual void Deactivate() override;
    virtual void Accept() override;
    virtual void GetNavigationState(bool*, bool*) override;
    virtual void AdjustEntry(W8CharacterPageEntry*, int) override;
    virtual void ShowEntryInfo(W8CharacterPageEntry*) override;
    virtual void Refresh() override;

private:
    void UpdateEntries();
    bool m_force_redraw;
    bool m_show_fifth_category;
    bool nav_next_state;
    unsigned char padding_077;
};
W8_ABI_ASSERT(sizeof(W8CharacterSkillsPage) == 0x78, "W8CharacterSkillsPage_size");

class W8CharacterPersonalityPage : public W8CharacterPage,
                                   public W8ControlSelectionListener,
                                   public W8TextControl::Listener {
public:
    W8CharacterPersonalityPage()
        : W8CharacterPage(0x105), anim_timer(0.4f, 1), m_animation_active(0)
    {
    }
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
    W8TextControl* m_control4;
    W8TextControl* m_control5;
    W8TextControl* m_control6;
    W8TextControl* m_control7;
    W8TextControl* m_randomize;
    W8ControlSelection m_personality_selection;
    W8ControlSelection m_voice_selection;
    W8GameTimer anim_timer;
    int m_animation_frame;
    bool m_animation_active;
    bool m_description_dirty;
    bool m_portrait_dirty;
    unsigned char pad_0ff;
};
W8_ABI_ASSERT(sizeof(W8CharacterPersonalityPage) == 0x100, "W8CharacterPersonalityPage_size");

W8CharacterStatsPage* CreateCharacterStatsPage();
W8CharacterSpellsPage* CreateCharacterSpellsPage();
W8CharacterSkillsPage* CreateCharacterSkillsPage();
W8CharacterPersonalityPage* CreateCharacterPersonalityPage();
extern unsigned int g_character_page4_region_set;

/* Realm animation records shared by the stats page's resistance icons and the
   spells page's realm list. The stats page reads only initial_frame, through
   the realm-1 offset the spells page's array lays out. */
struct W8SpellRealmAnimation {
    int image;
    unsigned int frame_count;
    unsigned int initial_frame;
};
extern W8SpellRealmAnimation g_spell_realm_animations[6];
extern wchar_t g_format_s_space_s[];
extern wchar_t g_format_s_colon[];

/* The six realm-icon object ids; the definition is the GLOBAL in
   CGSStatsPage.cpp. The camp screen's character block reuses them. */
extern int g_character_resistance_images[6];

/* One message id per character trait, indexed by trait id. */
extern unsigned short g_character_trait_name_ids[0x20];

/* Attribute-entry and skill-name message id tables shared by the stats and
   skills pages. */
extern unsigned short g_character_description_first_ids[22];
extern unsigned short g_character_skill_name_ids[84];
/* One row per gender, the third entry the possessive the item
   notices print. */
extern unsigned short g_gender_name_message_rows[4][4];
/* One message id per final-page personality slot; the array's terminating zero
   keeps its ten-entry extent. */
extern unsigned short g_personality_message_ids[10];

/* Page and per-row region sets. */
extern unsigned int g_character_stats_region_set;

/* Skill-availability bookkeeping raised by RefreshCharacterSkillAvailability while the character
   screen is open: adjust the named page-2 entry and refresh that page. */
void ResetCharacterScreenSkill(W8Skill skill_id);
void RefundCharacterScreenSkill(W8Skill skill_id);

/* Used by the pages to raise the screen-owned dialogs and to query the
   current character. */
class W8CharacterPageHost {
public:
    virtual void UpdateNavigation(W8CharacterPage* page) = 0;
    virtual void ShowSpellInfo(int value) = 0;
    virtual void ShowProfessionInfo(W8Profession profession) = 0;
    virtual void ShowRaceInfo(W8Race race) = 0;
    virtual void ShowPrimaryAttributeInfo(W8Attribute attribute) = 0;
    virtual void ShowSecondaryAttributeInfo(unsigned int attribute) = 0;
    virtual void ShowSkillInfo(W8Skill value) = 0;
    virtual void ShowDescription(int first, int second) = 0;
    virtual void ShowCharacterSummary() = 0;
    virtual bool HasDialog() = 0;
    virtual W8Character* GetOriginalCharacter() = 0;
};
W8_ABI_ASSERT(sizeof(W8CharacterPageHost) == 0x4, "W8CharacterPageHost_size");

class W8CharacterScreen : public W8CharacterPageHost, public W8TextControl::Listener {
public:
    W8CharacterScreen(int mode, W8Character* character);
    void BuildControls();
    void UpdateDialog();
    void AdvancePage(bool forward);
    void SelectPage(int index);
    void SyncCharacterForPage(int index);
    bool CommitCharacter();
    void DrawHeader();
    void ShowMessage(wchar_t* text, int confirmation, int response);
    void HandleDialogResult(int response, unsigned char accepted);
    bool ValidateName();

    virtual void UpdateNavigation(W8CharacterPage* page) override;
    virtual void ShowSpellInfo(int value) override;
    virtual void ShowProfessionInfo(W8Profession profession) override;
    virtual void ShowRaceInfo(W8Race race) override;
    virtual void ShowPrimaryAttributeInfo(W8Attribute attribute) override;
    virtual void ShowSecondaryAttributeInfo(unsigned int attribute) override;
    virtual void ShowSkillInfo(W8Skill value) override;
    virtual void ShowDescription(int first, int second) override;
    virtual void ShowCharacterSummary() override;
    virtual bool HasDialog() override;
    virtual W8Character* GetOriginalCharacter() override;
    virtual void OnPrimary(W8TextControl* control) override;
    virtual void OnSecondary(W8TextControl* control) override;

    int m_mode;
    int m_page_index;
    bool m_header_dirty;
    unsigned char pad_011[3];
    W8Character* m_original;
    W8Character m_character;
    unsigned char pad_187a[2];
    W8CharacterCreationState m_creation_state;
    bool m_block_advance;
    bool m_confirm_profession;
    bool m_force_transition;
    unsigned char pad_1aef;
    Controls* m_controls;
    W8TextControl* m_previous;
    W8TextControl* m_next;
    W8TextControl* m_exit;
    W8TextControl* m_accept;
    W8TextControl* m_reset;
    bool m_page_enabled[4];
    W8CharacterPage* m_pages[4];
    W8DialogBase* m_dialog;
    unsigned int m_dialog_response;
    bool m_capture_dialog_result;
    unsigned char pad_1b25[3];
};
W8_ABI_ASSERT(sizeof(W8CharacterScreen) == 0x1b28, "W8CharacterScreen_size");
W8_ASSERT_BASE_END(W8CharacterScreen, W8TextControl::Listener, m_mode, 0x4);

extern W8CharacterScreen* g_character_screen;

/* Per-profession message indexes. */
extern unsigned short g_profession_name_message_ids[32];
extern unsigned short g_race_name_message_ids[W8_RACE_COUNT];
/* Per-profession level-name message indexes, one row per profession for the
   level bands. */
extern unsigned short g_profession_level_name_message_ids[15][9];

/* Refresh the character-screen response when a party slot changes. */
void RefreshCharacterScreenPartySlot(unsigned int party_slot);
unsigned char CharacterScreenEnter(void);
void CharacterScreenFrame(void);
unsigned char CharacterScreenLeave(int leaving);
extern wchar_t g_dash[];
extern wchar_t g_format_d[];
extern wchar_t g_format_d_slash_d[];
extern wchar_t g_format_plus_d[];
