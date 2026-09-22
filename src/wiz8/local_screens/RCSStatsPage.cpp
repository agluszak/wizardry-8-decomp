/* Local Screens\RCSStatsPage.cpp - the review character screen's stats pages.

   Retail retains no path string for this unit; the official demo carries two
   "E:\Wizardry 8\Local Screens\RCSStatsPage.cpp" anchors at demo 0x005CE490
   and 0x005CE640 (line 3376), matching retail 0x005C5240 and 0x005C53C0. The
   unit occupies the retail span between mipeEdit.cpp (0x005C4340) and
   CGSSpellsPage.cpp (0x005C87B0): the camp stats page and its condition /
   equipment effect list, the camp skills page, and the character screen's
   skills and final pages. */

#include "wiz8/local_screens/RCSStatsPage.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/OptionsScreen.h"
#include "wiz8/layouts/character.h"
#include "wiz8/layouts/item_instance.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/character_skills.h"
#include "wiz8/character_event_queue.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/cursor.h"
#include "wiz8/fonts.h"
#include "wiz8/regions.h"
#include "wiz8/text_input.h"
#include "wiz8/utility.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/local_code/ButtonSound.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/ControlSelection.h"
#include "wiz8/local_code/ControlsRect.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/dialog_code/StatInfoDialogs.h"
#include "wiz8/dialog_code/SpellInfoDialog.h"

#include "Container.h"
#include "Font.h"
#include "input.h"
#include "vobject_blitters.h"

#include <new>
#include <string.h>

/* 0x0069C514/0x0069C518: the stats page's text origin, initialized by the
   page renderer before every full redraw. */
// GLOBAL: WIZ8 0x0069c514
int g_camp_stats_origin_y_0069c514;
// GLOBAL: WIZ8 0x0069c518
int g_camp_stats_origin_x_0069c518;
// GLOBAL: WIZ8 0x0069c51c
unsigned int g_camp_stats_range_region_set_0069c51c;
// GLOBAL: WIZ8 0x0069c520
unsigned int g_camp_stats_controls_region_set_0069c520;
/* 0x0069C524: the skills page row the cursor last hovered, kept so the help
   text only resets on a real change. */
// GLOBAL: WIZ8 0x0069c524
int g_camp_skill_hover_row_0069c524;
// GLOBAL: WIZ8 0x0069c528
unsigned int g_camp_skill_regions_0069c528;

static unsigned char CampStatsMouseWheel(const InputAtom* event, W8Region*);
static void DrawCampEffectList005C53C0(void);
struct W8CampEffectEntry;
static void DrawCampEffectEntry005C54A0(W8CampEffectEntry* entry, int* line_out);

/* The five skill-category blocks shared by the camp skills page: (x, y)
   origins, the row count that bounds each region, and the catalog frame each
   header draws. Category 4 sits below the right-hand column; the others fill
   the left and right columns. */
// GLOBAL: WIZ8 0x0064ef40
int g_camp_skill_category_positions_64ef40[5][2] = {
    {0x160, 0x8}, {0x160, 0xcc}, {0x22, 0x9}, {0x22, 0x86}, {0x2a, 0x21},
};
// GLOBAL: WIZ8 0x0064ef68
int g_camp_skill_category_rows_64ef68[5] = {0xb, 5, 8, 0xa, 7};
// GLOBAL: WIZ8 0x0064ef7c
int g_camp_skill_category_images_64ef7c[5] = {2, 3, 0, 1, 4};

// GLOBAL: WIZ8 0x0069c530
unsigned int g_character_page2_region_set_0069c530;
// GLOBAL: WIZ8 0x0069c52c
unsigned int g_character_page4_region_set_0069c52c;
// GLOBAL: WIZ8 0x0064ef90
int g_character_page2_category_geometry_64ef90[5][2] = {
    {0xf9, 0x0a}, {0xf9, 0xcd}, {0x22, 0x0a}, {0x22, 0x87}, {0xf9, 0x120},
};
// GLOBAL: WIZ8 0x0064efb8
int g_character_page2_category_frames_64efb8[5] = {2, 3, 0, 1, 4};

struct W8PortraitGroup {
    int count;
    int portraits[14];
};
static_assert(sizeof(W8PortraitGroup) == 0x3c, "W8PortraitGroup_size");
// GLOBAL: WIZ8 0x00648950
W8PortraitGroup g_portrait_groups_648950[12] = {
    {14, {0, 1, 2, 3, 76, 4, 5, 6, 7, 8, 9, 77, 10, 11}},
    {8, {12, 13, 14, 78, 15, 16, 17, 79}},
    {6, {18, 19, 20, 21, 22, 23}},
    {4, {24, 25, 26, 27}},
    {4, {28, 29, 30, 31}},
    {4, {32, 33, 34, 35}},
    {4, {36, 37, 38, 39}},
    {4, {40, 41, 42, 43}},
    {4, {44, 45, 46, 47}},
    {4, {48, 49, 50, 51}},
    {4, {52, 53, 54, 55}},
    {2, {56, 57}},
};

/* One row of the stats page's effect list: a character condition, an
   enchantment, or the modifiers of one equipped item. */
struct W8CampEffectEntry {
    unsigned char items;       /* 0x00: 1 when the row lists an equipped item */
    bool visible;              /* 0x01: passes the current filter */
    unsigned char beneficial;  /* 0x02 */
    unsigned char detrimental; /* 0x03 */
    int kind;                  /* 0x04: 0 condition, 1 enchantment, 2 equipment */
    int index;                 /* 0x08: condition, enchantment or equipment-slot index */
    int enchantment;           /* 0x0c: the enchantment id for kind 1 */
    int turns;                 /* 0x10: remaining turns; 9999 is permanent */
    int lines;                 /* 0x14: rendered height in 0xe-pixel lines */
};
static_assert(sizeof(W8CampEffectEntry) == 0x18, "W8CampEffectEntry_size");

// FUNCTION: WIZ8 0x005c4430
W8CampStatsRange::W8CampStatsRange()
{
    m_range =
        new W8RangeControl(0x264, 0xbe, 0x276, 0x1b5, &g_camp_stats_range_region_set_0069c51c);
    m_range->m_listener = this;
    m_range->SetEnabled(1);
}

// FUNCTION: WIZ8 0x005c44c0
W8CampStatsRange::~W8CampStatsRange()
{
    delete m_range;
}

// FUNCTION: WIZ8 0x005c44e0
void W8CampStatsRange::OnRangeChanged(W8RangeControl*)
{
    g_camp_screen_0069c0f4->effect_scroll = m_range->m_value;
    g_camp_screen_0069c0f4->redraw_flags |= 0x20000;
}

/* The shared camp-range refresh, emitted inside this unit in both retail and
   the demo. Retail calls it directly on the stats and spell listeners as
   well, which is what proves the common W8CampRangeListener base. */
// FUNCTION: WIZ8 0x005c4510
void W8CampRangeListener::UpdateRange(unsigned char range_changed)
{
    if (range_changed != 0) {
        m_range->Invalidate(0);
    }
    m_range->Redraw();
}

// FUNCTION: WIZ8 0x005c4540
W8CampStatsControls::W8CampStatsControls()
{
    AcquireRegionSet(&g_camp_stats_controls_region_set_0069c520);
    m_buttons[0] = new W8TextControl(this, -1, 0x13c, 0xbe, 0, 0, 0x145, 0, 0, 1, 2, 4, 3);
    m_buttons[0]->AddLayoutFlags(g_W8TextControlMask005ED588 | g_W8TextControlMask005ED578);
    m_buttons[0]->m_listener = this;
    m_buttons[0]->EnableRegionHelp(0x954);
    m_buttons[1] = new W8TextControl(this, -1, 0x13c, 0xd6, 0, 0, 0x145, 0, 5, 6, 7, 9, 8);
    m_buttons[1]->AddLayoutFlags(g_W8TextControlMask005ED588 | g_W8TextControlMask005ED578);
    m_buttons[1]->m_listener = this;
    m_buttons[1]->EnableRegionHelp(0x955);
    m_buttons[2] = new W8TextControl(this, -1, 0x13c, 0xf3, 0, 0, 0x145, 0, 10, 15, 12, 17, 13);
    m_buttons[2]->AddLayoutFlags(g_W8TextControlMask005ED588 | g_W8TextControlMask005ED578);
    m_buttons[2]->m_listener = this;
    m_buttons[2]->EnableRegionHelp(0x956);
    if (g_camp_screen_0069c0f4->effect_items_only) {
        m_buttons[2]->EnableSecondaryState(0);
    }
    unsigned int region = AddRegionToSet(g_camp_stats_controls_region_set_0069c520);
    SetRegionCallback(region, CampStatsMouseWheel, 0);
    SetRegionBounds(region, 0x15d, 0xbe, 0x260, 0x1b5);
    Controls::SetEnabled(1);
}

// SYNTHETIC: WIZ8 0x005c4780
// W8CampStatsControls::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005c47a0
W8CampStatsControls::~W8CampStatsControls()
{
    DestroyAllControls();
}

// FUNCTION: WIZ8 0x005c4800
void W8CampStatsControls::OnPrimary(W8TextControl* control)
{
    if (control == m_buttons[0]) {
        if (static_cast<unsigned char>(m_buttons[0]->m_stateFlags & g_W8TextControlMask005ED570)) {
            m_buttons[1]->DisableSecondaryState(0);
            g_camp_screen_0069c0f4->effect_filter = 1;
        } else {
            g_camp_screen_0069c0f4->effect_filter = 0;
        }
    } else if (control == m_buttons[1]) {
        if (static_cast<unsigned char>(m_buttons[1]->m_stateFlags & g_W8TextControlMask005ED570)) {
            m_buttons[0]->DisableSecondaryState(0);
            g_camp_screen_0069c0f4->effect_filter = 2;
        } else {
            g_camp_screen_0069c0f4->effect_filter = 0;
        }
    } else {
        g_camp_screen_0069c0f4->effect_items_only =
            static_cast<unsigned char>(m_buttons[2]->m_stateFlags & g_W8TextControlMask005ED570) !=
            0;
    }
    FilterCampEffectList005C5240();
    g_camp_screen_0069c0f4->redraw_flags |= 0x20000;
}

/* The stats page's full redraw: attribute rows with their base/effective
   delta bars, the profession's bonus skill, the trait grid, then the effect
   list region when its redraw flag is up. */
// FUNCTION: WIZ8 0x005c48b0
void DrawCampStatsPage005C48B0(void)
{
    SetFont(g_font_683660);
    if (g_camp_screen_0069c0f4->redraw_flags == 0xfffffff) {
        DrawCatalogImageAndInvalidate(-0xe, 0x142, 0, 1, 0, 0xa5, 2, 0);
        g_camp_stats_origin_x_0069c518 = 0;
        g_camp_stats_origin_y_0069c514 = 0xa5;
        int index;
        wchar_t* text = gppStringList[0x937];
        gprintf(g_camp_stats_origin_x_0069c518 + 10 +
                    ((0x11f - StringPixLength(text, g_font_683660)) >> 1),
                g_camp_stats_origin_y_0069c514 + 10, const_cast<wchar_t*>(g_format_s_006068e4),
                text);
        int row_y = 0xbf;
        for (index = 0; index < 7; ++index) {
            wchar_t* label = gppStringList[g_attribute_label_ids_64dd30[index]];
            gprintf(g_camp_stats_origin_x_0069c518 + 10 +
                        ((0x7b - StringPixLength(label, g_font_683660)) >> 1),
                    row_y - 0xa6 + g_camp_stats_origin_y_0069c514,
                    const_cast<wchar_t*>(g_format_s_006068e4), label);
            unsigned int effective = g_value_0069c0f8->attributes[index].effective;
            unsigned int base = g_value_0069c0f8->attributes[index].value;
            int gained;
            int lost;
            unsigned int shown;
            if (effective < base) {
                gained = 0;
                lost = base - effective;
                shown = effective;
            } else {
                gained = effective - base;
                lost = 0;
                shown = base;
            }
            SGPRect saved_clip;
            GetClippingRect(&saved_clip);
            SGPRect clip;
            clip.iTop = 0;
            clip.iBottom = 0x1e0;
            if (shown != 0) {
                clip.iLeft = 0x88;
                clip.iRight = shown + 0x88;
                SetClippingRect(&clip);
                DrawCatalogImageAndInvalidate(-0xe, 0x143, 0, 0, 0x88, row_y, 2, 0);
            }
            if (gained != 0) {
                clip.iLeft = shown + 0x88;
                clip.iRight = gained + 0x88 + shown;
                SetClippingRect(&clip);
                DrawCatalogImageAndInvalidate(-0xe, 0x143, 0, 1, 0x88, row_y, 2, 0);
            } else if (lost != 0) {
                clip.iLeft = shown + 0x88;
                clip.iRight = lost + 0x88 + shown;
                SetClippingRect(&clip);
                DrawCatalogImageAndInvalidate(-0xe, 0x143, 0, 2, 0x88, row_y, 2, 0);
            }
            SetClippingRect(&saved_clip);
            swprintf(g_camp_screen_0069c0f4->caption, const_cast<wchar_t*>(g_format_d_0060aa20),
                     effective);
            gprintf(
                g_camp_stats_origin_x_0069c518 + 0x108 +
                    ((0x21 - StringPixLength(g_camp_screen_0069c0f4->caption, g_font_683660)) >> 1),
                row_y - 0xa6 + g_camp_stats_origin_y_0069c514,
                const_cast<wchar_t*>(g_format_s_006068e4), g_camp_screen_0069c0f4->caption);
            row_y += 0xe;
        }
        text = gppStringList[0x938];
        gprintf(g_camp_stats_origin_x_0069c518 + 10 +
                    ((0x11f - StringPixLength(text, g_font_683660)) >> 1),
                g_camp_stats_origin_y_0069c514 + 0x8d, const_cast<wchar_t*>(g_format_s_006068e4),
                text);
        swprintf(g_camp_screen_0069c0f4->caption, const_cast<wchar_t*>(g_format_s_space_s_00617584),
                 gppStringList[g_character_skill_name_ids_61e454
                                   [g_profession_bonus_skills[g_value_0069c0f8->iProfession]]],
                 gppStringList[0x8c5]);
        gprintf(0x10, 0x142, const_cast<wchar_t*>(g_format_s_006068e4),
                g_camp_screen_0069c0f4->caption);
        int trait_count = 0;
        char traits[0x20];
        for (index = 0; index < 0x20; ++index) {
            if (CharacterHasTrait00547940(g_value_0069c0f8, index)) {
                traits[index] = 1;
                ++trait_count;
            } else {
                traits[index] = 0;
            }
        }
        int step = (trait_count < 8) ? 2 : 0;
        int trait_y = step + 0x14e;
        for (index = 0; index < 0x20; ++index) {
            if (traits[index] != 0) {
                gprintf(0x10, trait_y, const_cast<wchar_t*>(g_format_s_006068e4),
                        gppStringList[g_character_trait_name_ids_61e530[index]]);
                trait_y += step + 0xc;
            }
        }
        text = gppStringList[0x939];
        gprintf(g_camp_stats_origin_x_0069c518 + 0x13b +
                    ((0x13b - StringPixLength(text, g_font_683660)) >> 1),
                g_camp_stats_origin_y_0069c514 + 10, const_cast<wchar_t*>(g_format_s_006068e4),
                text);
        g_camp_screen_0069c0f4->stats_controls->Invalidate(0);
        g_camp_screen_0069c0f4->stats_range->m_range->Invalidate(0);
    }
    if ((g_camp_screen_0069c0f4->redraw_flags & 0x20000) != 0) {
        InvalidateRegion(0x15d, 0xbe, 0x260, 0x1b5, 0);
        BlitCatalogSurfaceRectTo16BPP(-0xe, 0x15d, 0xbe, 0x260, 0x1b5, 0x1b6, 0, 0);
        DrawCampEffectList005C53C0();
    }
    g_camp_screen_0069c0f4->stats_range->m_range->Redraw();
}

/* 0x005C4D40/0x005C4E20: how many beneficial/detrimental lines the equipped
   item in `slot` contributes to its effect-list entry. The armor-class bonus
   counts only on slots that actually take armor (not the two weapon/shield
   rows, 0 and 4/5 and 10/11) and never on equip-class 5 items. */
// FUNCTION: WIZ8 0x005c4d40
unsigned int CountEquipItemBenefits005C4D40(int slot)
{
    W8ItemDatabaseRecord* record = &g_item_records[g_value_0069c0f8->EquippedItem[slot].iItemNo];
    unsigned int count = record->attack_damage_bonus > 0;
    int index;
    for (index = 0; index < 0x10; ++index) {
        if (record->missile_values_050[index] != 0) {
            ++count;
        }
    }
    if (record->attack_hit_bonus > 0) {
        ++count;
    }
    if (record->slays_kind_061 != 0xff) {
        ++count;
    }
    if (record->modifier_06c > 0) {
        ++count;
    }
    if (record->modifier_06d > 0) {
        ++count;
    }
    if (record->modifier_06e > 0) {
        ++count;
    }
    if (record->armor_class_bonus > 0 && slot != 0 && slot != 4 && slot != 5 && slot != 10 &&
        slot != 0xb && record->equip_class != 5) {
        ++count;
    }
    if (record->modifier_0b3_index != -1 && record->modifier_0b3_value > 0) {
        ++count;
    }
    if (record->modifier_0b1_index != -1 && record->modifier_0b1_value > 0) {
        ++count;
    }
    for (index = 0; index < 6; ++index) {
        if (record->resistance_bonus_06f[index] > 0) {
            ++count;
        }
    }
    return count;
}

// FUNCTION: WIZ8 0x005c4e20
unsigned int CountEquipItemPenalties005C4E20(int slot)
{
    W8ItemDatabaseRecord* record = &g_item_records[g_value_0069c0f8->EquippedItem[slot].iItemNo];
    unsigned int count = record->attack_damage_bonus < 0;
    int index;
    if (record->attack_hit_bonus < 0) {
        ++count;
    }
    if (record->modifier_06c < 0) {
        ++count;
    }
    if (record->modifier_06d < 0) {
        ++count;
    }
    if (record->modifier_06e < 0) {
        ++count;
    }
    if (record->armor_class_bonus < 0) {
        ++count;
    }
    for (index = 0; index < 6; ++index) {
        if (record->resistance_bonus_06f[index] < 0) {
            ++count;
        }
    }
    if (record->modifier_0b3_index != -1 && record->modifier_0b3_value < 0) {
        ++count;
    }
    if (record->modifier_0b1_index != -1 && record->modifier_0b1_value < 0) {
        ++count;
    }
    if (record->binds_on_equip != 0 && g_value_0069c0f8->EquippedItem[slot].bound != 0) {
        ++count;
    }
    return count;
}

/* Rebuilds the effect list under the stats page: every active condition, then
   the enchantments, then one entry per worn item that has any modifier at all.
   Alternate-hand slots 8 and 9 are skipped, and unidentified items contribute
   nothing. */
// FUNCTION: WIZ8 0x005c4ee0
void RebuildCampEffectList005C4EE0(void)
{
    W8CampScreenState0069C0F4* screen = g_camp_screen_0069c0f4;
    if (screen->effect_list != 0) {
        DeleteStack(screen->effect_list);
        screen->effect_list = 0;
    }
    screen->effect_list = CreateList(10, sizeof(W8CampEffectEntry));
    screen->effect_beneficial_count = 0;
    screen->effect_detrimental_count = 0;
    screen->effect_selection = 0;
    screen->effect_first_visible = 0;
    screen->effect_last_visible = 0;
    screen->effect_visible_lines = 0;
    screen->effect_scroll = 0;
    W8Character* character = g_value_0069c0f8;
    for (int condition = 0x13; condition >= 0; --condition) {
        if (character->uiCondition[condition] != 0) {
            W8CampEffectEntry entry;
            memset(&entry, 0, sizeof(entry));
            entry.kind = 0;
            entry.detrimental = 1;
            entry.turns = character->uiCondition[condition];
            entry.lines = 1;
            if (entry.turns == 9999) {
                entry.lines = 2;
            }
            if (condition == 1) {
                if (character->hp_adjustment != 0) {
                    ++entry.lines;
                }
                if (character->fatigue_penalty_0b21 != 0) {
                    ++entry.lines;
                }
            }
            entry.index = condition;
            screen->effect_list =
                AddtoList(screen->effect_list, &entry, StackSize(screen->effect_list));
            if (entry.beneficial != 0) {
                ++screen->effect_beneficial_count;
            }
            if (entry.detrimental != 0) {
                ++screen->effect_detrimental_count;
            }
        }
    }
    for (int index = 7; index >= 0; --index) {
        if (character->enchantments[index].turns_08 != 0) {
            W8CampEffectEntry entry;
            memset(&entry, 0, sizeof(entry));
            entry.kind = 1;
            entry.beneficial = 1;
            entry.enchantment = character->enchantments[index].power_00;
            entry.turns = character->enchantments[index].turns_08;
            entry.lines = 2;
            entry.index = index;
            screen->effect_list =
                AddtoList(screen->effect_list, &entry, StackSize(screen->effect_list));
            if (entry.beneficial != 0) {
                ++screen->effect_beneficial_count;
            }
            if (entry.detrimental != 0) {
                ++screen->effect_detrimental_count;
            }
        }
    }
    for (int slot = 0; slot < 12; ++slot) {
        W8ItemInstance* item = &g_value_0069c0f8->EquippedItem[slot];
        if (slot != 8 && slot != 9 && item->identified != 0 && item->iItemNo != -1) {
            int beneficial = CountEquipItemBenefits005C4D40(slot);
            int detrimental = CountEquipItemPenalties005C4E20(slot);
            if (beneficial != 0 || detrimental != 0) {
                W8CampEffectEntry entry;
                memset(&entry, 0, sizeof(entry));
                entry.beneficial = beneficial != 0;
                entry.detrimental = detrimental != 0;
                entry.lines = detrimental + beneficial + 1;
                entry.items = 1;
                entry.kind = 2;
                entry.turns = 9999;
                entry.index = slot;
                screen->effect_list =
                    AddtoList(screen->effect_list, &entry, StackSize(screen->effect_list));
                if (entry.beneficial != 0) {
                    ++screen->effect_beneficial_count;
                }
                if (entry.detrimental != 0) {
                    ++screen->effect_detrimental_count;
                }
            }
        }
    }
    FilterCampEffectList005C5240();
}

/* Remarks each list entry against the current tab and beneficial/detrimental
   filter, tracks the first/last visible rows, and reprograms the scrollbar to
   the visible line total. */
// FUNCTION: WIZ8 0x005c5240
void FilterCampEffectList005C5240(void)
{
    W8CampScreenState0069C0F4* screen = g_camp_screen_0069c0f4;
    screen->effect_visible_lines = 0;
    bool any_visible = false;
    unsigned int count = StackSize(screen->effect_list);
    for (unsigned int pos = 0; pos < count; ++pos) {
        W8CampEffectEntry entry;
        if (PeekList(screen->effect_list, &entry, pos) == 0) {
            return;
        }
        entry.visible = 0;
        if (entry.items == screen->effect_items_only &&
            (screen->effect_filter == 0 || (screen->effect_filter == 1 && entry.beneficial != 0) ||
             (screen->effect_filter == 2 && entry.detrimental != 0))) {
            entry.visible = 1;
            if (!any_visible) {
                screen->effect_first_visible = pos;
                any_visible = true;
            }
            screen->effect_last_visible = pos;
        }
        if (entry.visible != 0) {
            screen->effect_visible_lines += entry.lines + 1;
        }
        StoreListNode(screen->effect_list, &entry, pos);
        count = StackSize(screen->effect_list);
    }
    int second = screen->effect_visible_lines - 0x11;
    W8RangeControl* range = screen->stats_range->m_range;
    if (second < 1) {
        second = 0;
        range->SetRangeEnabled(0);
    } else {
        range->SetRangeEnabled(1);
        range->SetRange(0, second);
    }
    if (screen->effect_scroll > second) {
        screen->effect_scroll = second;
    }
    range->SetValue(screen->effect_scroll);
    range->Invalidate(0);
}

/* Draws the visible slice of the effect list into the page's clipping
   window, skipping entries that sit entirely above the scrolled view. */
// FUNCTION: WIZ8 0x005c53c0
static void DrawCampEffectList005C53C0(void)
{
    W8CampScreenState0069C0F4* screen = g_camp_screen_0069c0f4;
    SetFontDestBuffer(0xfffffff2, 0, 0xbe, 0x280, 0x1ac, 0);
    int line = -screen->effect_scroll;
    unsigned int count = StackSize(screen->effect_list);
    for (unsigned int pos = 0; pos < count; ++pos) {
        if (line > 0x10) {
            break;
        }
        W8CampEffectEntry entry;
        if (PeekList(screen->effect_list, &entry, pos) == 0) {
            return;
        }
        if (entry.visible != 0) {
            if (line + entry.lines < 0) {
                line += entry.lines + 1;
            } else {
                DrawCampEffectEntry005C54A0(&entry, &line);
            }
        }
        count = StackSize(screen->effect_list);
    }
    SetFontDestBuffer(0xfffffff2, 0, 0, 0x280, 0x1e0, 0);
}

/* 0x005C54A0: renders one effect-list entry at `line` (in 0xe-pixel rows from
   y 0xbf) and advances the counter past its height plus one row of spacing.
   Kind 0 is a condition, kind 1 an enchantment, kind 2 an equipped item. */
// FUNCTION: WIZ8 0x005c54a0
static void DrawCampEffectEntry005C54A0(W8CampEffectEntry* entry, int* line_out)
{
    int index;
    int line = *line_out;
    if (entry->kind == 0) {
        SetFontObjectPalette16BPP(g_font_683660, g_font_state_palettes_68ee1c[1]);
        gprintf(0x15e, line * 0xe + 0xbf, const_cast<wchar_t*>(g_format_s_space_s_00617584),
                gppStringList[0x8d1],
                gppStringList[g_condition_notices_0061E570[entry->index * 4]]);
        SetFontObjectPalette16BPP(g_font_683660, g_colour_68ee08);
        int next = line + 1;
        if (entry->turns == 9999) {
            gprintf(0x15e, (line + 1) * 0xe + 0xbf, gppStringList[0x8d2]);
            next = line + 2;
        }
        line = next;
        if (entry->index == 1) {
            if (g_value_0069c0f8->hp_adjustment != 0) {
                gprintf(0x15e, line * 0xe + 0xbf, L"%s: %+d", gppStringList[0x8da],
                        g_value_0069c0f8->hp_adjustment);
                ++line;
            }
            if (g_value_0069c0f8->fatigue_penalty_0b21 != 0) {
                gprintf(0x15e, line * 0xe + 0xbf, L"%s: %+d", gppStringList[0x8dc],
                        -g_value_0069c0f8->fatigue_penalty_0b21);
                ++line;
            }
        }
    } else if (entry->kind == 1) {
        SetFontObjectPalette16BPP(g_font_683660, g_font_state_palettes_68ee1c[1]);
        gprintf(0x15e, line * 0xe + 0xbf, L"%s %s (%d)", gppStringList[0x8d4],
                gppStringList[g_condition_notices_0061E570[entry->index + 100]],
                entry->enchantment);
        SetFontObjectPalette16BPP(g_font_683660, g_colour_68ee08);
        gprintf(0x15e, (line + 1) * 0xe + 0xbf, const_cast<wchar_t*>(g_format_d_s_0061a128),
                entry->turns, gppStringList[0x8d3]);
        *line_out = line + 3;
        return;
    } else if (entry->kind == 2) {
        W8ItemDatabaseRecord* record =
            &g_item_records[g_value_0069c0f8->EquippedItem[entry->index].iItemNo];
        SetFontObjectPalette16BPP(g_font_683660, g_font_state_palettes_68ee1c[1]);
        gprintf(0x15e, line * 0xe + 0xbf, const_cast<wchar_t*>(g_format_s_006068e4),
                GetItemDisplayName(&g_value_0069c0f8->EquippedItem[entry->index]));
        SetFontObjectPalette16BPP(g_font_683660, g_colour_68ee08);
        int next = line + 1;
        if (record->attack_damage_bonus != 0) {
            gprintf(0x15e, (line + 1) * 0xe + 0xbf, L"%s %+d", gppStringList[0x8b1],
                    record->attack_damage_bonus);
            next = line + 2;
        }
        line = next;
        wchar_t value_text[12];
        for (index = 0; index < 0x10; ++index) {
            if (record->missile_values_050[index] != 0) {
                wcscpy(g_camp_screen_0069c0f4->caption, &g_wchar_00689b34);
                wcscat(g_camp_screen_0069c0f4->caption,
                       gppStringList[g_damage_type_name_ids_61e9cc[index]]);
                wcscat(g_camp_screen_0069c0f4->caption, L" ");
                swprintf(value_text, const_cast<wchar_t*>(g_format_d_percent_0064bab0),
                         record->missile_values_050[index]);
                wcscat(g_camp_screen_0069c0f4->caption, value_text);
                if (index == 2) {
                    wcscat(g_camp_screen_0069c0f4->caption, L" (");
                    wcscat(g_camp_screen_0069c0f4->caption, gppStringList[0x8d5]);
                    swprintf(value_text, L"%d)", record->missile_value_060);
                    wcscat(g_camp_screen_0069c0f4->caption, value_text);
                }
                gprintf(0x15e, line * 0xe + 0xbf, const_cast<wchar_t*>(g_format_s_colon_s_0061c3e0),
                        gppStringList[0x8d6], g_camp_screen_0069c0f4->caption);
                ++line;
            }
        }
        if (record->attack_hit_bonus != 0) {
            gprintf(0x15e, line * 0xe + 0xbf, L"%s %+d", gppStringList[0x8b7],
                    record->attack_hit_bonus);
            ++line;
        }
        if (record->slays_kind_061 != 0xff) {
            gprintf(0x15e, line * 0xe + 0xbf, const_cast<wchar_t*>(g_format_s_colon_s_0061c3e0),
                    gppStringList[0x8d8],
                    gppStringList[g_special_category_name_ids_61ea78[record->slays_kind_061]]);
            ++line;
        }
        if (record->modifier_06c > 0) {
            gprintf(0x15e, line * 0xe + 0xbf, L"%s: %+d", gppStringList[0x8d9],
                    record->modifier_06c);
            ++line;
        }
        if (record->modifier_06c < 0) {
            gprintf(0x15e, line * 0xe + 0xbf, L"%s: %+d", gppStringList[0x8da],
                    record->modifier_06c);
            ++line;
        }
        if (record->modifier_06d > 0) {
            gprintf(0x15e, line * 0xe + 0xbf, L"%s: %+d", gppStringList[0x8db],
                    record->modifier_06d);
            ++line;
        }
        if (record->modifier_06d < 0) {
            gprintf(0x15e, line * 0xe + 0xbf, L"%s: %+d", gppStringList[0x8dc],
                    record->modifier_06d);
            ++line;
        }
        if (record->modifier_06e > 0) {
            gprintf(0x15e, line * 0xe + 0xbf, L"%s: %+d", gppStringList[0x8dd],
                    record->modifier_06e);
            ++line;
        }
        if (record->modifier_06e < 0) {
            gprintf(0x15e, line * 0xe + 0xbf, L"%s: %+d", gppStringList[0x8de],
                    record->modifier_06e);
            ++line;
        }
        if (record->armor_class_bonus != 0 &&
            (record->armor_class_bonus < 0 ||
             (entry->index != 0 && entry->index != 4 && entry->index != 5 && entry->index != 10 &&
              entry->index != 0xb && record->equip_class != 5))) {
            gprintf(0x15e, line * 0xe + 0xbf, L"%s %+d", gppStringList[0x8df],
                    record->armor_class_bonus);
            ++line;
        }
        if (record->modifier_0b3_index != -1) {
            gprintf(
                0x15e, line * 0xe + 0xbf, L"%s %+d",
                gppStringList[g_character_description_first_ids_61e3a4[record->modifier_0b3_index]],
                record->modifier_0b3_value);
            ++line;
        }
        if (record->modifier_0b1_index != -1) {
            gprintf(0x15e, line * 0xe + 0xbf, L"%s %+d",
                    gppStringList[g_character_skill_name_ids_61e454[record->modifier_0b1_index]],
                    record->modifier_0b1_value);
            ++line;
        }
        for (index = 0; index < 6; ++index) {
            if (record->resistance_bonus_06f[index] != 0) {
                gprintf(0x15e, line * 0xe + 0xbf, L"%s %s %+d%%",
                        gppStringList[g_attr_table_61E50C[6 + index]], gppStringList[0x8e0],
                        record->resistance_bonus_06f[index]);
                ++line;
            }
        }
        if (record->binds_on_equip != 0 &&
            g_value_0069c0f8->EquippedItem[entry->index].bound != 0) {
            gprintf(0x15e, line * 0xe + 0xbf, gppStringList[0x8e1]);
            *line_out = line + 2;
            return;
        }
    }
    *line_out = line + 1;
}

/* 0x005C5C60: wheel input over the stats page's effect-list region steps the
   scrollbar. */
// FUNCTION: WIZ8 0x005c5c60
static unsigned char CampStatsMouseWheel(const InputAtom* event, W8Region*)
{
    PushButtonSoundScheme005587C0(0, 1);
    if (event->usEvent != 0x800) {
        return 0;
    }
    int delta = GetMouseWheelDeltaValue(event->usParam);
    while (delta > 0) {
        g_camp_screen_0069c0f4->stats_range->m_range->Decrement();
        --delta;
    }
    while (delta < 0) {
        g_camp_screen_0069c0f4->stats_range->m_range->Increment();
        ++delta;
    }
    return 1;
}

/* Creates the five skill-category regions once per camp session and points
   them at the list handler. Category 4 is the overflow block under the right
   column. */
// FUNCTION: WIZ8 0x005c5cd0
void CreateCampSkillRegions005C5CD0(void)
{
    if (g_camp_skill_regions_0069c528 == 0) {
        g_camp_skill_regions_0069c528 = CreateRegionSet();
        for (unsigned int category = 0; category < 5; ++category) {
            unsigned int region = AddRegionToSet(g_camp_skill_regions_0069c528);
            SetRegionCallback(region, CampSkillListRegionHandler005C6230,
                              static_cast<unsigned short>(category));
            SetRegionHelp(region, 1, -1);
            unsigned short x =
                static_cast<unsigned short>(g_camp_skill_category_positions_64ef40[category][0]);
            unsigned short y =
                static_cast<unsigned short>(g_camp_skill_category_positions_64ef40[category][1]);
            if (category == 4) {
                x += 0x136;
            } else {
                y += 0xa5;
            }
            SetRegionBounds(region, x, y, x + 0x6d,
                            g_camp_skill_category_rows_64ef68[category] * 0xe + y);
        }
    }
    RegionSetEnable(g_camp_skill_regions_0069c528);
}

// FUNCTION: WIZ8 0x005c5d70
void DisableCampSkillRegions005C5D70(void)
{
    RegionSetDisable(g_camp_skill_regions_0069c528);
}

/* The skills page's full redraw: the two column panels, the five category
   headers, then every known skill's name and level with the base/current
   delta bar. */
// FUNCTION: WIZ8 0x005c5d80
void DrawCampSkillsPage005C5D80(void)
{
    SetFont(g_font_683660);
    if (g_camp_screen_0069c0f4->redraw_flags == 0xfffffff) {
        bool has_fifth = false;
        int skill;
        for (skill = 0; skill < 0x29; ++skill) {
            if ((g_value_0069c0f8->skills[skill].flag_00 != 0 ||
                 g_value_0069c0f8->skills[skill].level != 0) &&
                g_skill_attributes[skill].category == 4) {
                has_fifth = true;
                break;
            }
        }
        DrawCatalogImageAndInvalidate(-0xe, 0x141, 0, has_fifth, 0x136, 0, 2, 0);
        DrawCatalogImageAndInvalidate(-0xe, 0x141, 0, 2, 0, 0xa5, 2, 0);
        for (int category = 0; category < 5; ++category) {
            if (category != 4 || has_fifth) {
                int x = g_camp_skill_category_positions_64ef40[category][0];
                int y = g_camp_skill_category_positions_64ef40[category][1];
                if (category == 4) {
                    x += 0x136;
                } else {
                    y += 0xa5;
                }
                DrawCatalogImage(-0xe, 0x144, 0,
                                 static_cast<short>(g_camp_skill_category_images_64ef7c[category]),
                                 x - 0x16, y - 3, 2, 0);
            }
        }
        int category_count[5] = {0, 0, 0, 0, 0};
        for (skill = 0; skill < 0x29; ++skill) {
            W8CharacterSkill* value = &g_value_0069c0f8->skills[skill];
            if (value->flag_00 != 0 || value->value_02 != 0 || value->level != 0) {
                int category = g_skill_attributes[skill].category;
                int left = g_camp_skill_category_positions_64ef40[category][0];
                int top = g_camp_skill_category_positions_64ef40[category][1];
                if (category == 4) {
                    left += 0x136;
                } else {
                    top += 0xa5;
                }
                top += category_count[category] * 0xe;
                DrawCatalogImage(-0xe, 0x141, 0, 3, left, top, 2, 0);
                unsigned int level = value->level;
                unsigned int base = value->value_02;
                int gained;
                int lost;
                if (level < base) {
                    lost = base - level;
                    gained = 0;
                } else {
                    gained = level - base;
                    lost = 0;
                    level = base;
                }
                SGPRect saved_clip;
                GetClippingRect(&saved_clip);
                SGPRect clip;
                clip.iTop = 0;
                clip.iBottom = 0x1e0;
                if (level != 0) {
                    clip.iLeft = left + 0x6f;
                    clip.iRight = level + 0x6f + left;
                    SetClippingRect(&clip);
                    DrawCatalogImage(-0xe, 0x143, 0, 0, left + 0x6f, top + 2, 2, 0);
                }
                if (gained == 0) {
                    if (lost != 0) {
                        clip.iLeft = level + 0x6f + left;
                        clip.iRight = lost + level + 0x6f + left;
                        SetClippingRect(&clip);
                        DrawCatalogImageAndInvalidate(-0xe, 0x143, 0, 2, left + 0x6f, top + 2, 2,
                                                      0);
                    }
                } else {
                    clip.iLeft = level + 0x6f + left;
                    clip.iRight = gained + level + 0x6f + left;
                    SetClippingRect(&clip);
                    DrawCatalogImage(-0xe, 0x143, 0, 1, left + 0x6f, top + 2, 2, 0);
                }
                SetClippingRect(&saved_clip);
                unsigned short* palette;
                if (!g_status_685170.game_started || value->level == 0) {
                    palette = g_font_state_palettes_68ee1c[11];
                    if (value->flag_00 != 0) {
                        palette = g_colour_68ee08;
                    }
                } else {
                    bool best = true;
                    for (int slot = 0; slot < 8; ++slot) {
                        if (g_status_685170.buffers.XChar[slot].fOccupied &&
                            value->level < g_status_685170.buffers.Char[slot].skills[skill].level) {
                            best = false;
                            break;
                        }
                    }
                    if (!best) {
                        palette = g_font_state_palettes_68ee1c[11];
                        if (value->flag_00 != 0) {
                            palette = g_colour_68ee08;
                        }
                    } else {
                        palette = g_font_state_palettes_68ee1c[12];
                        if (value->flag_00 != 0) {
                            palette = g_font_state_palettes_68ee1c[5];
                        }
                    }
                }
                SetFontObjectPalette16BPP(g_font_683660, palette);
                short width = StringPixLength(
                    gppStringList[g_character_skill_name_ids_61e454[skill]], g_font_683660);
                gprintf((0x6b - width) / 2 + 2 + left, top + 1,
                        const_cast<wchar_t*>(g_format_s_006068e4),
                        gppStringList[g_character_skill_name_ids_61e454[skill]]);
                palette = g_colour_68ee08;
                if (value->improved_12 != 0) {
                    palette = g_font_state_palettes_68ee1c[1];
                }
                SetFontObjectPalette16BPP(g_font_683660, palette);
                short value_width = StringPixLengthArg(
                    g_font_683660, 3, const_cast<wchar_t*>(g_format_d_0060aa20), value->level);
                gprintfDirty((0x24 - value_width) / 2 + 0xee + left, top + 1,
                             const_cast<wchar_t*>(g_format_d_0060aa20), value->level);
                SetFontObjectPalette16BPP(g_font_683660, g_colour_68ee08);
                SetObjectShade(g_wiz_text_font_secondary_object_683680, 4);
                ++category_count[category];
            }
        }
    }
}

/* The skills page's region handler: hover updates the help line with the
   skill's info hint, and a right click opens the skill info dialog. The
   dialog's second argument is raised when the character's level is the best
   among the occupied party slots. */
// FUNCTION: WIZ8 0x005c6230
unsigned char CampSkillListRegionHandler005C6230(const InputAtom* event, W8Region* region)
{
    int row = (GetAtomCursorY004285A0(event) - region->y1 - 1) / 0xe;
    int skill = -1;
    int occurrence = 0;
    for (int index = 0; index < 0x29; ++index) {
        if (g_skill_attributes[index].category == static_cast<int>(region->callback_id) &&
            (g_value_0069c0f8->skills[index].flag_00 != 0 ||
             g_value_0069c0f8->skills[index].level != 0)) {
            if (occurrence == row) {
                skill = index;
                break;
            }
            ++occurrence;
        }
    }
    PushButtonSoundScheme005587C0(0, 1);
    if (event->usEvent == RIGHT_BUTTON_DOWN) {
        region->flags |= W8_REGION_RIGHT_BUTTON_HELD;
    } else if (event->usEvent != RIGHT_BUTTON_UP) {
        if (event->usEvent != MOUSE_POS) {
            return 0;
        }
        if ((region->flags & W8_REGION_MOUSE_ENTER) != 0 ||
            row != g_camp_skill_hover_row_0069c524) {
            wchar_t* text = skill == -1 ? 0 : gppStringList[0x958];
            SetRegionHelpText(text);
            ResetRegionHelp(1);
            g_camp_skill_hover_row_0069c524 = row;
        }
        return 0;
    }
    if ((region->flags & W8_REGION_RIGHT_BUTTON_HELD) != 0 && skill != -1) {
        unsigned char best = 0;
        if (g_status_685170.game_started && g_value_0069c0f8->skills[skill].level != 0) {
            best = 1;
            for (int slot = 0; slot < 8; ++slot) {
                if (g_status_685170.buffers.XChar[slot].fOccupied &&
                    g_value_0069c0f8->skills[skill].level <
                        g_status_685170.buffers.Char[slot].skills[skill].level) {
                    best = 0;
                    break;
                }
            }
        }
        W8SkillInfoDialog* dialog = new W8SkillInfoDialog(
            skill, best, g_value_0069c0f8->skills[skill].flag_00 == 0,
            skill == g_profession_bonus_skills[g_value_0069c0f8->iProfession]);
        DisplayCampDialog(dialog);
    }
    return 1;
}

/* The character screen's fourth page (name, portrait, personality and voice)
   and second page (skills). W8_SCREEN_CHARACTER is the camp screen's character
   editor, so both classes sit in this unit between the skills-page handler and
   the CGSSpellsPage anchor; their retail span 0x005C6460..0x005C7D60 matches
   demo 0x005CFAD0..0x005D0FE0 inside the same hull. */

// VTABLE: WIZ8 0x005ef57c W8CharacterPage005EF57C
// VTABLE: WIZ8 0x005ef578 W8ControlSelectionListener
// VTABLE: WIZ8 0x005ef570 W8TextControl::Listener
// class W8CharacterPage005EF57C

// FUNCTION: WIZ8 0x005c6460
void W8CharacterPage005EF57C::SetCharacter(W8Character* character,
                                           W8CharacterCreationState* creation_state, int mode)
{
    AcquireRegionSet(&g_character_page4_region_set_0069c52c);
    W8CharacterPage::SetCharacter(character, creation_state, mode);
    W8TextControl::Listener* action_listener = this;

    m_control_07c =
        new W8TextControl(this, 0xffffffff, 100, 0x19, 0, 0, 0x10a, 0, 10, 0xc, 0xb, 0xe, 0xd);
    m_control_07c->m_listener = action_listener;
    m_control_078 = new W8TextControl(this, 0xffffffff, 0x144, 0x19, 0, 0, 0x10a, 0, 0xf, 0x11,
                                      0x10, 0x13, 0x12);
    m_control_078->m_listener = action_listener;
    m_control_084 =
        new W8TextControl(this, 0xffffffff, 100, 0x67, 0, 0, 0x10a, 0, 10, 0xc, 0xb, 0xe, 0xd);
    m_control_084->m_listener = action_listener;
    m_control_080 = new W8TextControl(this, 0xffffffff, 0x144, 0x67, 0, 0, 0x10a, 0, 0xf, 0x11,
                                      0x10, 0x13, 0x12);
    m_control_080->m_listener = action_listener;
    m_randomize_088 = new W8TextControl(this, 0xffffffff, 0x16d, 0x155, 0, 0, 0x10a, 0, 0x14, 0x16,
                                        0x15, 0x18, 0x17);
    m_randomize_088->m_listener = action_listener;
    m_randomize_088->EnableRegionHelp(0xf5);

    int index;
    for (index = 0; index < 9; ++index) {
        int column = index % 3;
        int row = index / 3;
        W8TextControl* entry =
            new W8TextControl(this, 0xffffffff, column * 0x80 + 0x24, row * 0xe + 0x107,
                              column * 0x80 + 0xa3, row * 0xe + 0x114, 0x105, 0, 5, 7, 6, 8, -1);
        entry->AddLayoutFlags(g_W8TextControlMask005ED594);
        m_personality_selection_08c.AddEntry(entry);
    }
    m_personality_selection_08c.SetSelected(character->personality_0081);
    m_personality_selection_08c.m_selectionListener = this;

    for (index = 0; index < 2; ++index) {
        int top = index == 0 ? 0x140 : 0x15d;
        W8TextControl* entry = new W8TextControl(this, 0xffffffff, 0x21, top, 0x69, top + 0xe,
                                                 0x105, 0, 5, 7, 6, 8, -1);
        entry->AddLayoutFlags(g_W8TextControlMask005ED594);
        m_voice_selection_0b0.AddEntry(entry);
    }
    ClampInteger(&character->voice_0085, 0, 1);
    m_voice_selection_0b0.SetSelected(character->voice_0085);
    m_voice_selection_0b0.m_selectionListener = this;
}

// FUNCTION: WIZ8 0x005c6820
void W8CharacterPage005EF57C::Activate()
{
    EnableRegionSet(1);
    m_prepared_06c = 1;
    InitTextInputModeWithScheme(1);
    AddTextInputField(origin_x + 0x97, origin_y + 0xab, 0x106, 0x10, 0x7f,
                      m_character_060->name_part_2, 0x27, 0xf, 1);
    AddTextInputField(origin_x + 0x97, origin_y + 0xc7, 0x106, 0x10, 0x7f, m_character_060->name, 9,
                      0xf, 1);
    if (GetTextInputFieldLength(0) == 0)
        SetActiveField(0);
    else if (GetTextInputFieldLength(1) == 0)
        SetActiveField(1);
    m_personality_selection_08c.SetSelected(m_character_060->personality_0081);
    m_voice_selection_0b0.SetSelected(m_character_060->voice_0085);
}

// FUNCTION: WIZ8 0x005c68f0
void W8CharacterPage005EF57C::Deactivate()
{
    EnableRegionSet(0);
    RemoveTextInputField(1);
    RemoveTextInputField(0);
    KillTextInputMode();
}

// FUNCTION: WIZ8 0x005c6910
void W8CharacterPage005EF57C::Accept()
{
    if (m_mode_068 == 0) {
        InvalidateAndRecalculateCharacterClassData00558610(m_character_060);
    } else {
        W8Character* original = m_screen_05c->GetOriginalCharacter();
        m_character_060->personality_0081 = original->personality_0081;
        m_character_060->portrait_index = original->portrait_index;
        m_character_060->voice_0085 = original->voice_0085;
        wcscpy(m_character_060->name_part_2, original->name_part_2);
        wcscpy(m_character_060->name, original->name);
    }
    Refresh();
    Invalidate(0);
    m_screen_05c->UpdateNavigation(this);
}

// FUNCTION: WIZ8 0x005c69a0
void W8CharacterPage005EF57C::GetNavigationState(bool* next_enabled, bool* exit_enabled)
{
    *next_enabled = GetTextInputFieldLength(0) != 0 && GetTextInputFieldLength(1) != 0;
    if (m_mode_068 != 0) {
        W8Character* original = m_screen_05c->GetOriginalCharacter();
        *exit_enabled = false;
        if (m_character_060->personality_0081 == original->personality_0081 &&
            m_character_060->portrait_index == original->portrait_index &&
            m_character_060->voice_0085 == original->voice_0085 &&
            wcscmp(m_character_060->name_part_2, original->name_part_2) == 0 &&
            wcscmp(m_character_060->name, original->name) == 0) {
            return;
        }
        *exit_enabled = true;
    } else {
        *exit_enabled = true;
    }
}

// FUNCTION: WIZ8 0x005c6a60
void W8CharacterPage005EF57C::HandleInput(InputAtom* input)
{
    if (m_screen_05c->HasDialog())
        return;
    if (input->usEvent != KEY_DOWN && input->usEvent != KEY_REPEAT) {
        DispatchMainGameMouseButtons(input);
        return;
    }

    unsigned short character =
        TranslateKeyToCharacter(static_cast<unsigned short>(input->usParam), input->usKeyState);
    if (character != 0 && strchr("\\/:*?\"<>|", static_cast<unsigned char>(character)) != 0) {
        return;
    }
    if (input->usParam == VK_TAB) {
        SelectNextField();
        return;
    }
    if (!HandleTextInput(input))
        return;

    short field = GetActiveTextInputField();
    if (field == 0) {
        Get16BitStringFromField(0, m_character_060->name_part_2);
    } else if (field == 1) {
        Get16BitStringFromField(1, m_character_060->name);
    }
    m_screen_05c->UpdateNavigation(this);
}

// FUNCTION: WIZ8 0x005c6b20
void W8CharacterPage005EF57C::Refresh()
{
    SetInputFieldStringWith16BitString(0, m_character_060->name_part_2);
    SetInputFieldStringWith16BitString(1, m_character_060->name);
    m_personality_selection_08c.SetSelected(m_character_060->personality_0081);
    m_voice_selection_0b0.SetSelected(m_character_060->voice_0085);
}

/* The final page's whole redraw: advance the portrait animation, draw the
   fixed text labels and the nine personality labels, then let the dirty
   portrait, description and name-row blocks refresh themselves. */
// FUNCTION: WIZ8 0x005c6b70
void W8CharacterPage005EF57C::Redraw()
{
    if (m_animation_active_0fc != 0) {
        int elapsed = static_cast<int>(m_animation_timer_0d4.GetProgress());
        if (elapsed > 0) {
            m_animation_frame_0f8 = (m_animation_frame_0f8 + elapsed) % 3;
            DrawCatalogImageAndInvalidate(-14, 0x105, 0, m_animation_frame_0f8 + 2,
                                          origin_x + 0x156, origin_y + 0x137, 2, 0);
        }
    }

    bool redraw = m_fEnabled && m_fDirty;
    W8CharacterPage::Redraw();
    if (!m_screen_05c->HasDialog()) {
        RenderAllTextFields();
    }

    if (redraw) {
        W8TextBuffer text;
        W8ControlsRect bounds;

        bounds.left = origin_x + 0x2c;
        bounds.right = origin_x + 100;
        bounds.top = origin_y + 0x19;
        bounds.bottom = origin_y + 0x31;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0xee], g_font_683660);
        text.RenderToTarget(0, 1, -14);

        bounds.top = origin_y + 0x67;
        bounds.bottom = origin_y + 0x7f;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0xf0], g_font_683660);
        text.RenderToTarget(0, 1, -14);

        bounds.left = origin_x + 0x160;
        bounds.right = origin_x + 0x198;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0xf1], g_font_683660);
        text.RenderToTarget(0, 1, -14);

        bounds.top = origin_y + 0x19;
        bounds.bottom = origin_y + 0x31;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0xef], g_font_683660);
        text.RenderToTarget(0, 1, -14);

        bounds.left = origin_x + 0x24;
        bounds.right = origin_x + 0x92;
        bounds.top = origin_y + 0xaa;
        bounds.bottom = origin_y + 0xbc;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0x84], g_font_683660);
        text.RenderToTarget(0, 1, -14);

        bounds.top = origin_y + 0xc6;
        bounds.bottom = origin_y + 0xd8;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0x85], g_font_683660);
        text.RenderToTarget(0, 1, -14);

        bounds.left = origin_x + 0x18;
        bounds.top = origin_y + 0xee;
        bounds.right = origin_x + 0x1a8;
        bounds.bottom = origin_y + 0xfa;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0x86], g_font_683660);
        text.RenderToTarget(0, 1, -14);

        int index = 0;
        for (const unsigned short* message_id = g_personality_message_ids_61e674;
             message_id < g_personality_message_ids_61e674 + 9; ++message_id, ++index) {
            bounds.left = (index % 3) * 0x80 + 0x30 + origin_x;
            bounds.right = bounds.left + 0x80;
            bounds.top = origin_y + 0x106 + (index / 3) * 0xe;
            bounds.bottom = bounds.top + 0xe;
            text.SetLayoutBounds(&bounds, 1, 1);
            text.SetLayoutMode(g_W8TextBufferLayoutMask005ED558 | g_W8TextBufferLayoutMask005ED548);
            text.SetText(gppStringList[*message_id], g_font_683660);
            text.RenderToTarget(0, 1, -14);
        }

        bounds.top = origin_y + 0x13f;
        bounds.bottom = origin_y + 0x14d;
        bounds.left = origin_x + 0x29;
        bounds.right = origin_x + 0x69;
        text.SetLayoutMode(g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED558);
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0x8d], g_font_683660);
        text.RenderToTarget(0, 1, -14);

        bounds.top = origin_y + 0x15c;
        bounds.bottom = origin_y + 0x16a;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0x8e], g_font_683660);
        text.RenderToTarget(0, 1, -14);

        m_portrait_dirty_0fe = 1;
        m_description_dirty_0fd = 1;
        m_animation_active_0fc = 0;
    }

    if (m_portrait_dirty_0fe) {
        DrawCatalogImageAndInvalidate(-14, 0x11, m_character_060->portrait_index, 0,
                                      origin_x + 0x86, origin_y + 5, 0, 0);
        m_portrait_dirty_0fe = 0;
    }

    if (m_description_dirty_0fd) {
        W8TextBuffer text;
        W8ControlsRect bounds;
        W8CharacterEvent element(m_character_060, g_effect_005ee588, 0, g_effect_argument_005ed8c8,
                                 g_effect_argument_005ed914);

        bounds.top = origin_y + 0x13a;
        bounds.bottom = origin_y + 0x16a;
        bounds.left = origin_x + 0x70;
        bounds.right = origin_x + 0x150;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(element.GetQuoteText(), g_font_683660);
        text.FillBounds(0x8000);
        text.RenderToTarget(0, 1, -14);
        m_description_dirty_0fd = 0;
    }

    if (m_prepared_06c) {
        W8TextBuffer text;
        W8ControlsRect bounds = {9, 0xec, 0xbd, 0x184};
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0xe9], g_font_683660);
        text.RenderToTarget(0, 1, -14);
        m_prepared_06c = 0;
    }
}

// FUNCTION: WIZ8 0x005c7220
void W8CharacterPage005EF57C::OnPrimary(W8TextControl* control)
{
    int portrait = m_character_060->portrait_index;
    if (control == m_control_078) {
        int group = g_portrait_descriptors_6483d0[portrait].group + 1;
        if (group > 11)
            group = 0;
        m_character_060->portrait_index = g_portrait_groups_648950[group].portraits[0];
        m_portrait_dirty_0fe = 1;
    } else if (control == m_control_07c) {
        int group = g_portrait_descriptors_6483d0[portrait].group - 1;
        if (group < 0)
            group = 11;
        m_character_060->portrait_index = g_portrait_groups_648950[group].portraits[0];
        m_portrait_dirty_0fe = 1;
    } else if (control == m_control_080 || control == m_control_084) {
        int group = g_portrait_descriptors_6483d0[portrait].group;
        W8PortraitGroup* portraits = &g_portrait_groups_648950[group];
        int index = 0;
        while (index < portraits->count && portraits->portraits[index] != portrait) {
            ++index;
        }
        if (control == m_control_080) {
            ++index;
            if (index >= portraits->count)
                index = 0;
        } else {
            --index;
            if (index < 0)
                index = portraits->count - 1;
        }
        m_character_060->portrait_index = portraits->portraits[index];
        m_portrait_dirty_0fe = 1;
    } else if (control == m_randomize_088) {
        m_animation_active_0fc = 1;
        m_animation_frame_0f8 = 2;
        m_animation_timer_0d4.Restart();
        ShadowVideoSurfaceRect(-14, 0, 0, 0x280, 0x1e0);
        ResetTransientRenderScenes();
        m_screen_05c->ShowCharacterSummary();
    }

    if (control != m_randomize_088) {
        m_screen_05c->UpdateNavigation(this);
    }
}

// FUNCTION: WIZ8 0x005c73b0
void W8CharacterPage005EF57C::OnSelectionChanged(W8ControlSelection* control, int selected)
{
    if (control == &m_voice_selection_0b0) {
        m_character_060->voice_0085 = selected;
    } else {
        m_character_060->personality_0081 = selected;
    }
    m_description_dirty_0fd = 1;
    m_screen_05c->UpdateNavigation(this);
}

// FUNCTION: WIZ8 0x005c73f0
W8CharacterPage005EF57C* CreateCharacterPage005C73F0()
{
    return new W8CharacterPage005EF57C;
}

// SYNTHETIC: WIZ8 0x005c74c0
// W8CharacterPage005EF57C::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005c74e0
W8CharacterPage005EF57C::~W8CharacterPage005EF57C() {}

// VTABLE: WIZ8 0x005ef5c8 W8CharacterPage005EF5C8
// VTABLE: WIZ8 0x005ef5c0 W8CharacterPageEntryListener
// class W8CharacterPage005EF5C8

// FUNCTION: WIZ8 0x005c7580
void W8CharacterPage005EF5C8::SetCharacter(W8Character* character,
                                           W8CharacterCreationState* creation_state, int mode)
{
    AcquireRegionSet(&g_character_page2_region_set_0069c530);
    W8CharacterPage::SetCharacter(character, creation_state, mode);
    int category_count[5] = {0, 0, 0, 0, 0};
    for (int skill = 0; skill < 0x29; ++skill) {
        int category = g_skill_attributes[skill].category;
        W8CharacterPageEntry* entry =
            new W8CharacterPageEntry(this, g_character_page2_category_geometry_64ef90[category][0],
                                     g_character_page2_category_geometry_64ef90[category][1] +
                                         category_count[category] * 0xe,
                                     1);
        AddEntry(entry);
        entry->m_listener_004 = this;
        ++category_count[category];
    }
    m_navigation_state_076 = false;
}

// FUNCTION: WIZ8 0x005c76a0
void W8CharacterPage005EF5C8::Activate()
{
    EnableRegionSet(1);
    Refresh();
    m_dirty_06d = 1;
    m_prepared_06c = 1;
}

void W8CharacterPage005EF5C8::Deactivate()
{
    EnableRegionSet(0);
}

// FUNCTION: WIZ8 0x005c76c0
void W8CharacterPage005EF5C8::Accept()
{
    RefundAllSkillPoints(m_character_060, m_creation_state_064);
    Invalidate(0);
    m_dirty_06d = 1;
    m_screen_05c->UpdateNavigation(this);
    for (int index = 0; index < m_entries_04c.count; ++index) {
        m_entries_04c.data[index]->UpdateButtons();
    }
}

// FUNCTION: WIZ8 0x005c7720
void W8CharacterPage005EF5C8::GetNavigationState(bool* next_enabled, bool* exit_enabled)
{
    *next_enabled = m_creation_state_064->skills_complete;
    *exit_enabled =
        m_creation_state_064->skill_points_remaining < m_creation_state_064->skill_points_total;
    if (*next_enabled != m_navigation_state_076) {
        for (int index = 0; index < m_entries_04c.count; ++index) {
            m_entries_04c.data[index]->SetIncrementAllowed(!*next_enabled);
        }
        m_navigation_state_076 = *next_enabled;
    }
}

// FUNCTION: WIZ8 0x005c7790
void W8CharacterPage005EF5C8::AdjustEntry(W8CharacterPageEntry* entry, int delta)
{
    entry->MarkDirty();
    m_dirty_06d = 1;
    InitializeLevelUpAttributePool(m_character_060, m_creation_state_064, entry->m_id_02c, delta);
    m_screen_05c->UpdateNavigation(this);
}

// FUNCTION: WIZ8 0x005c77d0
void W8CharacterPage005EF5C8::ShowEntryInfo(W8CharacterPageEntry* entry)
{
    m_screen_05c->ShowDialog005B08E0(entry->m_id_02c);
}

// FUNCTION: WIZ8 0x005c77f0
void W8CharacterPage005EF5C8::Redraw()
{
    unsigned char redraw = static_cast<unsigned char>(m_fEnabled && m_fDirty);
    if (m_force_redraw_074) {
        UpdateEntries();
        Invalidate(0);
        redraw = 1;
        m_force_redraw_074 = 0;
    }
    W8CharacterPage::Redraw();

    if (redraw) {
        for (int category = 0; category < 5; ++category) {
            if (category != 4 || m_show_fifth_category_075) {
                DrawCatalogImage(
                    -14, 0x144, 0,
                    static_cast<short>(g_character_page2_category_frames_64efb8[category]),
                    origin_x + g_character_page2_category_geometry_64ef90[category][0] - 0x16,
                    origin_y + g_character_page2_category_geometry_64ef90[category][1] - 3, 2, 0);
            }
        }
        if (!m_show_fifth_category_075) {
            DrawCatalogImage(-14, 0x108, 0, 1, origin_x, origin_y + 0x118, 2, 0);
        }
    }

    if (m_prepared_06c) {
        W8TextBuffer text;
        W8ControlsRect bounds = {4, 0xec, 0xc2, 0x162};
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0xe8], g_font_683660);
        text.RenderToTarget(0, 1, -14);
        bounds.top = 0x162;
        bounds.right = 0x8f;
        bounds.bottom = 0x179;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0xe3], g_font_683660);
        text.RenderToTarget(0, 1, -14);
        bounds.top = 0x184;
        bounds.bottom = 0x19b;
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(gppStringList[0xe4], g_font_683660);
        text.RenderToTarget(0, 1, -14);
        bounds.left = 0x8f;
        bounds.top = 0x162;
        bounds.right = 0xbf;
        bounds.bottom = 0x179;
        DrawCatalogImage(-14, 0x107, 0, 5, 0x8f, 0x162, 2, 0);
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(FormatWideString(L"%d", m_creation_state_064->skill_step_limit),
                     g_options_detail_font_683614);
        text.RenderToTarget(0, 1, -14);
        m_prepared_06c = 0;
    }

    if (m_dirty_06d) {
        W8TextBuffer text;
        W8ControlsRect bounds = {0x8f, 0x184, 0xbf, 0x19b};
        DrawCatalogImage(-14, 0x107, 0, 5, 0x8f, 0x184, 2, 0);
        text.SetLayoutBounds(&bounds, 1, 1);
        text.SetText(FormatWideString(L"%d/%d", m_creation_state_064->skill_points_remaining,
                                      m_creation_state_064->skill_points_total),
                     g_options_detail_font_683614);
        text.RenderToTarget(0, 1, -14);
        m_dirty_06d = 0;
    }
}

// FUNCTION: WIZ8 0x005c7b50
void W8CharacterPage005EF5C8::UpdateEntries()
{
    int index;
    for (index = 0; index < 0x29; ++index) {
        m_entries_04c.data[index]->SetEnabled(0);
    }

    int category_count[5] = {0, 0, 0, 0, 0};
    m_show_fifth_category_075 = 0;
    for (int skill = 0; skill < 0x29; ++skill) {
        W8CharacterSkill* value = &m_character_060->skills[skill];
        if (value->flag_00 || value->value_02 != 0) {
            int category = g_skill_attributes[skill].category;
            int entry_index = 0;
            int occurrence = 0;
            for (; entry_index < 0x29; ++entry_index) {
                if (g_skill_attributes[entry_index].category == category &&
                    occurrence++ == category_count[category]) {
                    break;
                }
            }
            W8CharacterPageEntry* entry = m_entries_04c.data[entry_index];
            ++category_count[category];
            if (category == 4)
                m_show_fifth_category_075 = 1;
            entry->SetContent(skill, gppStringList[g_character_skill_name_ids_61e454[skill]],
                              &value->value_02, &m_creation_state_064->skill_points_spent[skill],
                              &m_creation_state_064->skill_limits[skill], 0x101);
            entry->SetLabelFontState(
                skill == g_profession_bonus_skills[m_character_060->iProfession] ? 3 : -1);
        }
    }
}

// FUNCTION: WIZ8 0x005c7cc0
W8CharacterPage005EF5C8* CreateCharacterPage005C7CC0()
{
    return new W8CharacterPage005EF5C8;
}

// FUNCTION: WIZ8 0x005c7d30
void W8CharacterPage005EF5C8::Refresh()
{
    m_force_redraw_074 = 1;
}

// SYNTHETIC: WIZ8 0x005c7d40
// W8CharacterPage005EF5C8::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005c7d60
W8CharacterPage005EF5C8::~W8CharacterPage005EF5C8() {}
