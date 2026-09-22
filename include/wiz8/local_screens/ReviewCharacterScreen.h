#pragma once

#include "wiz8/local_code/RangeControl.h"
#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/layouts/learned_spells.h"
#include "wiz8/local_screens/RCSStatsPage.h"
#include "input.h"

class W8DialogBase;
struct Controls;
class W8HelpTextControl;
class W8TextControl;
class W8Widget;
struct W8Character;
struct W8CombatSlot;
struct W8ItemInstance;
struct W8Region;

/* The three listeners own different range controls. Their callbacks update
   the item, spell-realm and stats effect-list scroll positions respectively.
   W8CampStatsRange and W8CampStatsControls are authored in RCSStatsPage.cpp
   (declared in RCSStatsPage.h). */
// VTABLE: WIZ8 0x005eed08
class W8CampItemRange : public W8CampRangeListener {
public:
    W8CampItemRange();
    ~W8CampItemRange()
    {
        delete m_range;
    }
    virtual void OnRangeChanged(W8RangeControl* control) override;
};

// VTABLE: WIZ8 0x005ef298
class W8CampSpellRange : public W8CampRangeListener {
public:
    explicit W8CampSpellRange(int realm);
    ~W8CampSpellRange();
    virtual void OnRangeChanged(W8RangeControl* control) override;
    int m_realm;
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
static_assert(sizeof(W8CampCharacterInfo) == 0x6c, "W8CampCharacterInfo_size");
/* Retail secondary vftable 0x005ef270 places W8TextControl::Listener at +0x4c. */
W8_ASSERT_BASE_END(W8CampCharacterInfo, W8TextControl::Listener, m_combat_view, 0x4c);

/* malloc(0xd54) in Camp entry owns the record. Suspension destroys this UI;
   the screen-state stack retains the arguments needed to recreate it. */
struct W8CampScreenState0069C0F4 {
    wchar_t caption[120];
    int page; /* 0x0f0 */
    unsigned int hover_region;
    unsigned int redraw_flags;
    unsigned int item_redraw_flags;
    W8LearnedSpellState learned_spells; /* 0x100 */
    unsigned char realm_flags[6];       /* 0x4dc */
    unsigned char padding_4e2[2];
    unsigned int item_scroll;
    /* 0x4e8: the displayed item-pool indices - the count and the list of pool
       slots RCSItemsPage.cpp renders. RebuildCampItemList005A4A00 rebuilds it; the pool
       handler reads it through item_scroll. */
    unsigned int item_list_count;
    int item_list_4ec[500];
    W8CampItemRange* item_range; /* 0xcbc */
    W8CampSpellRange* spell_ranges[6];
    W8CampStatsRange* stats_range; /* 0xcd8 */
    W8CampStatsControls* stats_controls;
    unsigned int item_timer; /* 0xce0 */
    unsigned char item_timer_active;
    unsigned char item_timer_expired;
    unsigned char padding_ce6[2];
    unsigned int animation_timer;
    unsigned int animation_frames[6];
    int input_mode; /* 0xd04 */
    /* 0xd08..0xd30: the stats page's condition/equipment effect list, rebuilt
       by RebuildCampEffectList005C4EE0 and refiltered by
       FilterCampEffectList005C5240. */
    unsigned char effect_items_only; /* 0xd08: 1 lists equipped items, 0 conditions/enchantments */
    unsigned char padding_d09[3];
    int effect_filter; /* 0xd0c: 0 all, 1 beneficial only, 2 detrimental only */
    int effect_beneficial_count;
    int effect_detrimental_count;
    int effect_selection; /* 0xd18: cleared when the reviewed character changes */
    unsigned char unknown_d1c[4];
    int effect_first_visible; /* 0xd20 */
    int effect_last_visible;
    int effect_visible_lines;
    int effect_scroll;      /* 0xd2c */
    void* effect_list;      /* 0xd30: HLIST of W8CampEffectEntry rows */
    int selected_spell_row; /* 0xd34 */
    unsigned char unknown_d38[7];
    unsigned char entry_mode;
    /* 0xd40[0]: mouse is over the reviewed portrait; drives the highlight
       frame and a redraw. */
    unsigned char portrait_hovered_d40[4];
    W8DialogBase* dialog;
    unsigned char item_mode;
    unsigned char padding_d49[3];
    W8CampCharacterInfo* character_info;
    /* 0xd50: the camp item icons were drawn while the monster/combat
       timer was enabled; its stop forces a full redraw to drop them. */
    unsigned char item_icons_drawn_d50;
    unsigned char padding_d51[3];
};
static_assert(sizeof(W8CampScreenState0069C0F4) == 0xd54, "W8CampScreenState_size");
static_assert(offsetof(W8CampScreenState0069C0F4, learned_spells) == 0x100,
              "W8CampScreenState_learned_spells_offset");
static_assert(offsetof(W8CampScreenState0069C0F4, learned_spells) +
                      offsetof(W8LearnedSpellState, scroll) ==
                  0x4c0,
              "W8CampScreenState_spell_scroll_offset");
static_assert(offsetof(W8CampScreenState0069C0F4, learned_spells) +
                      offsetof(W8LearnedSpellState, learned_total) ==
                  0x4d8,
              "W8CampScreenState_learned_total_offset");

extern W8CampScreenState0069C0F4* g_camp_screen_0069c0f4;
extern int giReviewCharSlot;
extern W8Character* g_review_character_0069c0f8;
extern W8Character* g_camp_entry_parameter_0069c0fc; /* gpIdentifyingPC */
extern W8Character* g_camp_character_0069c100;
extern bool g_camp_character_pending_0069c104;
extern unsigned int g_camp_item_region_set_0069c108;
extern unsigned int g_camp_spell_region_sets_0069c40c[6];

/* Panel controls owned by the camp screen, created by
   CreateCampSecondaryPanel005B9900, CreateCampActionPanel005B9070 and
   CreateItemsTabPanel005B9350 and read here and in RCSItemsPage.cpp. The
   secondary panel owns the Items/Character info page tabs, the character-info
   help text, the seven attribute rows and the four secondary value labels. */
extern Controls* g_camp_secondary_panel_0069c428;
extern W8Widget* g_camp_info_labels_0069c42c[4];
extern W8TextControl* g_camp_page_tabs_0069c43c[2];
extern W8HelpTextControl* g_camp_help_text_0069c444;
extern W8Widget* g_camp_stat_labels_0069c448[7];
extern Controls* g_camp_action_panel_0069c464;
extern W8TextControl* g_camp_action_buttons_0069c468[2];
extern W8TextControl* g_camp_realm_tabs_0069c470[7];
extern Controls* g_camp_realm_tab_panel_0069c48c;
extern unsigned int g_camp_secondary_region_set_0069c490;
/* One gppStringList id per primary attribute row; defined in
   ReviewCharacterScreen.cpp, drawn by RCSStatsPage.cpp's stats page. */
extern int g_attribute_label_ids_64dd30[7];

/* 0x0064CBF0: the twelve camp-screen regions the layout rules do not cover,
   given as explicit rectangles. The region initializer reads the first four
   fields; the trailing five are never touched there and stay positional. */
struct W8CampScreenRegion {
    int x;                     /* 0x00 */
    int y;                     /* 0x04 */
    int width;                 /* 0x08 */
    int height;                /* 0x0c */
    int frame_10;              /* 0x10: catalog frame for the slot border */
    int unidentified_frame_14; /* 0x14: overlay frame while the item is unidentified */
    int label_x_18;            /* 0x18: item label left */
    int label_y_1c;            /* 0x1c: item label top */
    int label_flag_20;         /* 0x20: label draw flag */
};

extern const W8CampScreenRegion g_camp_screen_regions_64cbf0[12];
/* 0x00648C48: load-category font-palette selectors indexed by
   W8Character::load_category. */
extern int g_load_category_palettes_648c48[5];
/* 0x0064CDA0: the three portrait catalog ids per race used by the camp
   party strip. */
extern int g_race_portrait_images_64cda0[0x30];

void SwitchCampPage005A4540(int page);
void ClearOtherRealmFilters005A49D0(unsigned int realm);
void RebuildCampItemList005A4A00(void);
void SetCampInputMode005A4BC0(int mode);
void DisplayCampDialog(W8DialogBase* dialog);
void DismissSelectedPartyCharacter(void);

/* The pending-companion swap set by MarkCampCharacterPending005A6020 and
   consumed by ResolvePendingCampCharacter005A5F30. */
bool ResolvePendingCampCharacter005A5F30(bool force);
void MarkCampCharacterPending005A6020(W8ItemInstance* item);
/* 0x005A6090 gates an item click on the character's remaining action
   allowance in combat; 0x005A6440 programs a pending use-item action aimed at
   an item. */
bool IsCampActionAllowed005A6090(int party_slot);
int CommitPartySlotSpell005A6340(int party_slot, int spell_id, int power_level,
                                 W8CombatSlot* target);
int CommitPartySlotItemUse005A6440(int party_slot, W8ItemInstance* item, W8CombatSlot* target);

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
void DrawCampSpellPages005B7300(void);
void DrawCampResistances005B7790(void);
unsigned char SpellListRegionHandler005B79F0(const InputAtom* event, W8Region* region);
void OpenSpellInfoDialog005B7BB0(unsigned int spell_id);
void SyncReviewCharInputRegion005A4570(void);
bool IsEquippableItemClass005A6310(W8ItemInstance* item); /* 0x005A6310 */

extern int g_effect_005ee6ec;
extern int g_effect_argument_005ed8cc;

/* 0x005A6620: begin the timed screen fade and run `callback` when it
   finishes; `fade_to_black` selects the alpha ramp. */
void BeginScreenFade(int fade_to_black, int arg_2, int fade_code, void (*callback)(void), char flag,
                     char arg_6);
unsigned char UpdateScreenFade005A6790(void);
void BeginPartyDeath005A68C0(void);
void PumpReviewTransition005A6970(void);
void DrawPartyDeathScreen005A6A70(void);
void EndReviewTransition005A6B20(void);
/* Ending sequence picker run when that fade completes. */
void ShowEndingScreen005A6B90(void); /* 0x005A6B90 */
/* 0x005A6580 */
void BeginEndgameSequence005A6580(void);
/* Camp and main-game notice dialogs ShowNoticeLine forwards into. */
void ShowCampNoticeLine(wchar_t* text, W8DialogDestroyCallback callback, int confirmation,
                        int cancel); /* 0x005A4C00 */
void HandleCampItemClick005A4C70(W8ItemInstance* item, unsigned int slot_index,
                                 unsigned int origin);
