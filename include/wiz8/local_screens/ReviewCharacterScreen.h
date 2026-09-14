#pragma once

#include "wiz8/local_code/RangeControl.h"
#include "wiz8/dialog_code/DialogBase.h"

class W8DialogBase;
class W8HelpTextControl;
class W8TextControl;
class W8Widget;
struct W8Character;
struct W8CombatSlot;
struct W8ItemInstance;
struct W8Region;
struct W8RegionEvent;

/* The three listeners own different range controls. Their callbacks update
   the item, spell-realm and skill scroll positions respectively. */
// VTABLE: WIZ8 0x005eed08
class W8CampItemRange : public W8RangeListener {
public:
    W8CampItemRange();
    ~W8CampItemRange()
    {
        delete m_range;
    }
    virtual void OnRangeChanged(W8RangeControl* control) override;
    /* 0x005C4510: re-invalidates the scrollbar when the visible pool changed
       and always repaints it. */
    void UpdateItems(unsigned char items_changed);
    W8RangeControl* m_range;
};

// VTABLE: WIZ8 0x005ef298
class W8CampSpellRange : public W8RangeListener {
public:
    explicit W8CampSpellRange(int realm);
    ~W8CampSpellRange();
    virtual void OnRangeChanged(W8RangeControl* control) override;
    W8RangeControl* m_range;
    int m_realm;
};

// VTABLE: WIZ8 0x005ef530
class W8CampSkillRange : public W8RangeListener {
public:
    W8CampSkillRange();
    ~W8CampSkillRange();
    virtual void OnRangeChanged(W8RangeControl* control) override;
    W8RangeControl* m_range;
};

// VTABLE: WIZ8 0x005ef53c Controls
// VTABLE: WIZ8 0x005ef534 W8TextControl::Listener
class W8CampSkillControls : public Controls, public W8TextControl::Listener {
public:
    W8CampSkillControls();
    virtual ~W8CampSkillControls();
    virtual void OnPrimary(W8TextControl* control) override;
    W8TextControl* m_buttons[3];
};

// VTABLE: WIZ8 0x005ef278 Controls
// VTABLE: WIZ8 0x005ef270 W8TextControl::Listener
class W8CampCharacterInfo : public Controls, public W8TextControl::Listener {
public:
    W8CampCharacterInfo();
    virtual void SetEnabled(bool enabled) override;
    virtual void Redraw() override;
    virtual void OnPrimary(W8TextControl* control) override;
    void SetCombatView(bool enabled);
    bool m_combat_view;
    unsigned char m_pad_051[3];
    W8TextControl* m_button_054;
    W8TextControl* m_button_058;
    W8HelpTextControl* m_values[4];
};

static_assert(sizeof(W8CampItemRange) == 8, "W8CampItemRange_size");
static_assert(sizeof(W8CampSpellRange) == 12, "W8CampSpellRange_size");
static_assert(sizeof(W8CampSkillRange) == 8, "W8CampSkillRange_size");
static_assert(sizeof(W8CampSkillControls) == 0x5c, "W8CampSkillControls_size");
static_assert(sizeof(W8CampCharacterInfo) == 0x6c, "W8CampCharacterInfo_size");

/* malloc(0xd54) in Camp entry owns the record. Suspension destroys this UI;
   the screen-state stack retains the arguments needed to recreate it. */
struct W8CampScreenState0069C0F4 {
    wchar_t caption[120];
    int page; /* 0x0f0 */
    unsigned int hover_region;
    unsigned int redraw_flags;
    unsigned int item_redraw_flags;
    /* 0x100: the learned-spell lists, handed to BuildLearnedSpellState004F9600
       as its W8LearnedSpellScratch; the scratch's trailing counters land on
       spell_scroll and learned_spell_total. */
    int spell_ids_by_realm[6][40];
    int spell_scroll[6];          /* 0x4c0 */
    int learned_spell_total;      /* 0x4d8 */
    unsigned char realm_flags[6]; /* 0x4dc */
    unsigned char unknown_4e2[2];
    unsigned int item_scroll;
    /* 0x4e8: the displayed item-pool indices - the count and the list of pool
       slots RCSItemsPage.cpp renders. RebuildCampItemList005A4A00 rebuilds it; the pool
       handler reads it through item_scroll. */
    unsigned int item_list_count;
    int item_list_4ec[500];
    W8CampItemRange* item_range; /* 0xcbc */
    W8CampSpellRange* spell_ranges[6];
    W8CampSkillRange* skill_range; /* 0xcd8 */
    W8CampSkillControls* skill_controls;
    unsigned int item_timer; /* 0xce0 */
    unsigned char item_timer_active;
    unsigned char item_timer_expired;
    unsigned char unknown_ce6[2];
    unsigned int animation_timer;
    unsigned int animation_frames[6];
    int input_mode; /* 0xd04 */
    unsigned char skill_flag;
    unsigned char unknown_d09[3];
    int skill_scroll;
    unsigned char unknown_d10[8];
    int skill_selection; /* 0xd18: cleared when the reviewed character changes */
    unsigned char unknown_d1c[0x10];
    int skill_list_scroll;
    void* skill_stack;
    int selected_spell_row; /* 0xd34 */
    unsigned char unknown_d38[7];
    unsigned char entry_mode;
    unsigned char unknown_d40[4];
    W8DialogBase* dialog;
    unsigned char item_mode;
    unsigned char unknown_d49[3];
    W8CampCharacterInfo* character_info;
    unsigned char flag_d50;
    unsigned char unknown_d51[3];
};
static_assert(sizeof(W8CampScreenState0069C0F4) == 0xd54, "W8CampScreenState_size");

extern W8CampScreenState0069C0F4* g_camp_screen_0069c0f4;
extern int g_rcs_mode_0064cbe8;
extern W8Character* g_value_0069c0f8;
extern int g_camp_entry_parameter_0069c0fc;
extern W8Character* g_camp_character_0069c100;
extern unsigned char g_camp_character_pending_0069c104;
extern unsigned int g_camp_item_region_set_0069c108;
extern unsigned int g_camp_spell_region_sets_0069c40c[6];
extern unsigned int g_camp_skill_region_set_0069c51c;

/* Panel controls owned by the camp screen, created by
   CreateCampSecondaryPanel005B9900, CreateCampActionPanel005B9070 and
   CreateItemsTabPanel005B9350 and read here and in RCSItemsPage.cpp. The
   secondary panel owns the Items/Character info page tabs, the character-info
   help text, the seven attribute rows and the four secondary value labels. */
extern W8Widget* g_camp_info_labels_0069c42c[4];
extern W8TextControl* g_camp_page_tabs_0069c43c[2];
extern W8HelpTextControl* g_camp_help_text_0069c444;
extern W8Widget* g_camp_stat_labels_0069c448[7];
extern W8TextControl* g_camp_action_buttons_0069c468[2];
extern W8TextControl* g_camp_realm_tabs_0069c470[7];

/* Camp-screen gap functions, declared here for the call sites in
   RCSCommon.cpp and RCSItemsPage.cpp. */
void ClearOtherRealmFilters005A49D0(unsigned int realm);
void RebuildCampItemList005A4A00(void);
void SetCampInputMode005A4BC0(int mode);
void DisplayCampDialog(W8DialogBase* dialog);
void DismissSelectedPartyCharacter(void);

void CreateRcsLevelUpPanel(void);
void DestroyRcsLevelUpPanel(void);
void CreateRcsDismissPanel(void);
void DestroyRcsDismissPanel(void);
void DrawRcsText(const wchar_t* text, int left, int top, int width, unsigned int layout_mode);
void DrawRcsBoldText(const wchar_t* text, int left, int top, int width, unsigned int layout_mode);
void DrawTallRcsText(const wchar_t* text, int left, int top, int width, unsigned int layout_mode);
/* 0x005B6FD0: like DrawRcsText but the box height is caller-provided and the
   text is rendered through mprintf with the current font. */
void DrawRcsTextJustified(const wchar_t* text, int left, int top, int width, int height,
                          unsigned int layout_mode);

/* Unresolved gap callees of the camp item handler in this unit. 0x005A6090
   gates an item click on the character's remaining action allowance in
   combat; 0x005A6440 programs a pending use-item action aimed at an item. */
char Function5A6090(int party_slot);
int Function5A6440(int party_slot, W8ItemInstance* item, W8CombatSlot* target);

/* 0x005A5DA0: move a single unit between the clicked stack and the item in
   hand - split one off into the hand, or add one onto the held stack. */
void TakeItemUnitToHand005A5DA0(W8ItemInstance* item, unsigned short slot, unsigned int origin);

void CampScreenInitializeRegions(void);
void LayoutCampSecondaryRegions(void);
unsigned char CampScreenInitialize(void);
unsigned char CampScreenEnter(void);
void CampScreenFrame(void);
unsigned char CampScreenLeave(int leaving);

/* Camp spell-page pieces in ReviewCharacterScreen.cpp: the character switch,
   the six realm scrollbars, the page renderer, the resistance bars and the
   spell-list region callback that opens per-spell info dialogs. */
void SetCampSpellRangesEnabled005B71C0(unsigned char enable);
void RefreshCampSpellRanges005B7290(void);
void EnableCampActionButtons005B9270(void);
void DrawCampSpellPages005B7300(void);
void DrawCampResistances005B7790(void);
unsigned char SpellListRegionHandler005B79F0(const W8RegionEvent* event, W8Region* region);
void OpenSpellInfoDialog005B7BB0(unsigned int spell_id);
void Function5A4570(void);
void Function5C4EE0(void);

extern int g_effect_005ee6ec;
extern int g_effect_argument_005ed8cc;

void Function5187E0(void);

/* 0x005A6620: begin the timed screen fade and run `callback` when it
   finishes; `fade_to_black` selects the alpha ramp. */
void Function5A6620(int fade_to_black, int arg_2, int fade_code, void (*callback)(void), char flag,
                    char arg_6);
/* Ending sequence picker run when that fade completes. */
void Function5A6B90(void); /* 0x005A6B90 */
/* 0x005A6580 */
void BeginEndgameSequence005A6580(void);
