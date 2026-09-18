#include "soundman.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/fact_state.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/npc_interaction.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_screens/MGSRadarMap.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/fonts.h"
#include "wiz8/layouts/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/cursor.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/dialog_code/MessageDialogBase.h"
#include "wiz8/dialog_code/SpellInfoDialog.h"
#include "wiz8/local_code/ButtonSound.h"
#include "wiz8/local_code/RangeControl.h"
#include "wiz8/local_code/PartyImport.h"
#include "wiz8/local_code/LoadSaveGame.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/music_playlist.h"
#include "wiz8/sound_man.h"
#include "wiz8/regions.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/character_event_queue.h"
#include "wiz8/xstatus.h"
#include "Font.h"
#include "english.h"
#include "input.h"
#include "Types.h"
#include "mousesystem.h"
#include "random.h"
#include "timer.h"

#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "line.h"
#include "wiz8/local_screens/RCSCommon.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/npc_state.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/layouts/item_instance.h"
#include "wiz8/local_screens/RCSItemsPage.h"
#include "wiz8/engine_code/3dapi.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/item_video_object_vector.h"
#include "wiz8/video_object_catalog.h"
#include "vobject_blitters.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/local_screens/OptionsScreen.h"

/* Local Screens\ReviewCharacterScreen.cpp, named by entry's
   fFoundEquipChar assertion. This is state 6, reached both from the party
   selector and from the running game. */

// GLOBAL: WIZ8 0x0069c0f4
W8CampScreenState0069C0F4* g_camp_screen_0069c0f4;
// GLOBAL: WIZ8 0x0064cbe8
int giReviewCharSlot = -1;
// GLOBAL: WIZ8 0x0069c0f8
W8Character* g_value_0069c0f8;
// GLOBAL: WIZ8 0x0069c0fc
W8Character* g_camp_entry_parameter_0069c0fc; /* gpIdentifyingPC */
// GLOBAL: WIZ8 0x0069c100
W8Character* g_camp_character_0069c100;
// GLOBAL: WIZ8 0x0069c104
unsigned char g_camp_character_pending_0069c104;
// GLOBAL: WIZ8 0x0069c108
unsigned int g_camp_item_region_set_0069c108;
// GLOBAL: WIZ8 0x0069c40c
unsigned int g_camp_spell_region_sets_0069c40c[6];
// GLOBAL: WIZ8 0x0069c51c
unsigned int g_camp_skill_region_set_0069c51c;
// GLOBAL: WIZ8 0x0069c520
unsigned int g_camp_skill_controls_region_set;
// GLOBAL: WIZ8 0x0069c408
unsigned int g_camp_character_info_region_set;

// GLOBAL: WIZ8 0x005ee6ec
int g_effect_005ee6ec = 109;

// GLOBAL: WIZ8 0x005ed8cc
int g_effect_argument_005ed8cc = 1;

int CreateCampActionPanel005B9070(void);
int CreateItemsTabPanel005B9350(void);
int CreateCampSecondaryPanel005B9900(void);
void DrawCampCharacterInfo005B7E00(void);
void DrawCampBackpackItems005B8440(void);
void DrawCampEquipmentItems005B8690(void);
void DrawCampItemPool005B8B20(void);
void DrawCampItemQuantity005B8EC0(W8ItemInstance* item, int left, int top, int width);
void Function5A45B0(void);
void Function5A4770(void);
void Function5A42A0(void);
void Function5C5240(void);
// GLOBAL: WIZ8 0x0069c428
Controls* g_camp_secondary_panel_0069c428;

/* The camp screen's three panels and their controls: the top secondary panel
   carries the page tabs, help line, attribute rows and secondary labels; the
   bottom-left action panel carries two buttons; the right realm panel carries
   the six realm tabs and the sort button. */
// GLOBAL: WIZ8 0x0069c42c
W8Widget* g_camp_info_labels_0069c42c[4];
// GLOBAL: WIZ8 0x0069c43c
W8TextControl* g_camp_page_tabs_0069c43c[2];
// GLOBAL: WIZ8 0x0069c444
W8HelpTextControl* g_camp_help_text_0069c444;
// GLOBAL: WIZ8 0x0069c448
W8Widget* g_camp_stat_labels_0069c448[7];
// GLOBAL: WIZ8 0x0069c464
Controls* g_camp_action_panel_0069c464;
// GLOBAL: WIZ8 0x0069c468
W8TextControl* g_camp_action_buttons_0069c468[2];
// GLOBAL: WIZ8 0x0069c470
W8TextControl* g_camp_realm_tabs_0069c470[7];
// GLOBAL: WIZ8 0x0069c48c
Controls* g_camp_realm_tab_panel_0069c48c;
// GLOBAL: WIZ8 0x0069c490
unsigned int g_camp_secondary_region_set_0069c490;

/* 0x00648C48: the font-state palette index selected for each load category
   while the weight line is drawn; zero leaves the default palette in place. */
// GLOBAL: WIZ8 0x00648C48
int g_load_category_palettes_648c48[5] = {0xf, 3, 1, 5, 0};

/* 0x0064CDA0: one portrait frame per race and gender, race-major in threes. */
// GLOBAL: WIZ8 0x0064CDA0
int g_race_portrait_images_64cda0[0x30] = {
    0x125, 0x126, 0x125, 0x127, 0x128, 0x127, 0x129, 0x12a, 0x129, 0x12b, 0x12c, 0x12b,
    0x12d, 0x12e, 0x12d, 0x12f, 0x130, 0x12f, 0x131, 0x132, 0x131, 0x133, 0x134, 0x133,
    0x135, 0x136, 0x135, 0x137, 0x138, 0x137, 0x139, 0x13a, 0x139, 0x13b, 0x13b, 0x13b,
    0x13c, 0x13c, 0x13c, 0x13d, 0x13d, 0x13d, 0x13e, 0x13e, 0x13e, 0x13f, 0x13f, 0x13f,
};

/* 0x0064DD30: the seven attribute label message ids, drawn top to bottom on
   the secondary panel. The last two entries swap relative to attribute order. */
// GLOBAL: WIZ8 0x0064DD30
int g_attribute_label_ids_64dd30[7] = {0x924, 0x925, 0x926, 0x927, 0x928, 0x92a, 0x929};

/* 0x0064DD4C: the help-line weight breakdown, "<personal>: n, <party>: n". */
// GLOBAL: WIZ8 0x0064DD4C
const wchar_t g_format_s_colon_d_s_colon_d_0064dd4c[] = L"%s: %d, %s: %d";

// FUNCTION: WIZ8 0x005B9220
void ReleaseCampActionPanel005B9220(void)
{
    Controls* panel = g_camp_action_panel_0069c464;
    if (panel != 0) {
        delete panel;
        g_camp_action_panel_0069c464 = 0;
    }
    W8TextControl** control = g_camp_action_buttons_0069c468;
    do {
        if (*control != 0) {
            delete *control;
            *control = 0;
        }
        ++control;
    } while (control < g_camp_action_buttons_0069c468 + 2);
}

// FUNCTION: WIZ8 0x005B9760
void ReleaseItemsTabPanel005B9760(void)
{
    Controls* panel = g_camp_realm_tab_panel_0069c48c;
    if (panel != 0) {
        delete panel;
        g_camp_realm_tab_panel_0069c48c = 0;
    }
    W8TextControl** control = g_camp_realm_tabs_0069c470;
    do {
        if (*control != 0) {
            delete *control;
            *control = 0;
        }
        ++control;
    } while (control < g_camp_realm_tabs_0069c470 + 7);
}

// FUNCTION: WIZ8 0x005B9EA0
void ReleaseCampSecondaryPanel(void)
{
    Controls* panel = g_camp_secondary_panel_0069c428;
    if (panel != 0) {
        delete panel;
        g_camp_secondary_panel_0069c428 = 0;
    }
    W8TextControl** control = g_camp_page_tabs_0069c43c;
    do {
        if (*control != 0) {
            delete *control;
            *control = 0;
        }
        ++control;
    } while (control < g_camp_page_tabs_0069c43c + 2);
}

// FUNCTION: WIZ8 0x005b9ef0
void InvalidateCampPanel005B9EF0(void)
{
    g_camp_secondary_panel_0069c428->Invalidate(0);
}

/* 0x0064CBF0: the twelve camp-screen regions the layout rules do not cover,
   given as explicit rectangles. The region initializer reads the first four
   fields; the trailing five are never touched there and stay positional. */
struct W8CampScreenRegion {
    int x;      /* 0x00 */
    int y;      /* 0x04 */
    int width;  /* 0x08 */
    int height; /* 0x0c */
    int unknown_10;
    int unknown_14;
    int unknown_18;
    int unknown_1c;
    int unknown_20;
};

// GLOBAL: WIZ8 0x0064CBF0
const W8CampScreenRegion g_camp_screen_regions_64cbf0[12] = {
    {0x0bc, 0x0b0, 0x2d, 0x39, 0x00, 0x01, 0x0ee, 0x0c1, 0},
    {0x1af, 0x0c6, 0x28, 0x28, 0x28, 0x08, 0x1aa, 0x0de, 1},
    {0x1af, 0x0f1, 0x28, 0x28, 0x24, 0x08, 0x1aa, 0x0f1, 1},
    {0x178, 0x0b0, 0x26, 0x49, 0x2c, 0x09, 0x173, 0x0b0, 1},
    {0x086, 0x0f1, 0x39, 0x3a, 0x04, 0x02, 0x0c4, 0x11c, 0},
    {0x183, 0x101, 0x23, 0x2b, 0x20, 0x07, 0x17e, 0x10b, 1},
    {0x09e, 0x134, 0x22, 0x65, 0x0c, 0x03, 0x0a0, 0x134, 0},
    {0x190, 0x134, 0x22, 0x48, 0x18, 0x06, 0x1b0, 0x134, 1},
    {0x079, 0x134, 0x22, 0x65, 0x08, 0x03, 0x07b, 0x154, 0},
    {0x1b5, 0x134, 0x22, 0x48, 0x1c, 0x06, 0x1d1, 0x154, 1},
    {0x0cb, 0x167, 0x22, 0x4d, 0x10, 0x04, 0x0f2, 0x18f, 0},
    {0x16a, 0x17a, 0x1a, 0x3a, 0x14, 0x05, 0x165, 0x19f, 1}};

/* The camp screen's clickable labels: the seven attribute rows and the four
   secondary value labels on the character-info panel are plain text controls
   that additionally play the button sound on entry and on left presses. The
   left-button handlers compile identically to W8HelpTextControl's and fold to
   0x005B7CB0/0x005B7CD0. */
// VTABLE: WIZ8 0x005ef2b0
class W8CampInfoLabel005EF2B0 : public W8TextControl {
public:
    W8CampInfoLabel005EF2B0(Controls* panel, unsigned int region, int left, int top, int right,
                            int bottom, int text_40, int text_44, int text_48, int text_4c,
                            int text_54, int text_50, int text_58)
        : W8TextControl(panel, region, left, top, right, bottom, text_40, text_44, text_48, text_4c,
                        text_54, text_50, text_58)
    {
    }
    // SYNTHETIC: WIZ8 0x005b7c20
    // W8CampInfoLabel005EF2B0::`scalar deleting destructor'
    // FUNCTION: WIZ8 0x005b7c40
    virtual ~W8CampInfoLabel005EF2B0() override {}
    virtual void OnMouseEnter(int event) override;
    virtual void OnLeftButtonDown(int event) override;
    virtual void OnLeftButtonUp(int event) override;
    virtual void OnLeftButtonDoubleClick(int event) override;
};

// FUNCTION: WIZ8 0x005b7c90
void W8CampInfoLabel005EF2B0::OnMouseEnter(int event)
{
    PushButtonSoundScheme005587C0(0, 1);
    W8TextControl::OnMouseEnter(event);
}

/* Identical body to W8HelpTextControl::OnLeftButtonDown; ICF folds it to
   0x005B7CB0. */
void W8CampInfoLabel005EF2B0::OnLeftButtonDown(int event)
{
    PushButtonSoundScheme005587C0(0, 1);
    W8TextControl::OnLeftButtonDown(event);
}

/* Identical body to W8HelpTextControl::OnLeftButtonUp; ICF folds it to
   0x005B7CD0. */
void W8CampInfoLabel005EF2B0::OnLeftButtonUp(int event)
{
    PushButtonSoundScheme005587C0(0, 1);
    W8TextControl::OnLeftButtonUp(event);
}

// FUNCTION: WIZ8 0x005b7cf0
void W8CampInfoLabel005EF2B0::OnLeftButtonDoubleClick(int event)
{
    PushButtonSoundScheme005587C0(0, 1);
    W8TextControl::OnLeftButtonDoubleClick(event);
}

/* 0x0064DD14, 0x0064DD20, 0x0064DD28, 0x00648164: the spell page's small
   formats - zero-padded cost, plain number, plain string and the realm label
   prefix. */
// GLOBAL: WIZ8 0x0064DD14
const wchar_t g_format_3d_0064dd14[] = L"%3.3d";
// GLOBAL: WIZ8 0x0064DD20
const wchar_t g_format_d_0064dd20[] = L"%3d";
// GLOBAL: WIZ8 0x0064DD28
const wchar_t g_format_s_0064dd28[] = L"%s:";
// GLOBAL: WIZ8 0x00648164
const wchar_t g_format_s_colon_00648164[] = L"%s: ";

/* Enable or disable the six spell-realm scrollbars together. While enabling, a
   realm whose learned spells fit the eight visible rows keeps its bar off. */
// FUNCTION: WIZ8 0x005B71C0
void SetCampSpellRangesEnabled005B71C0(unsigned char enable)
{
    W8CampSpellRange* spell_range;
    int realm;
    int second;

    for (realm = 0; realm < 6; ++realm) {
        spell_range = g_camp_screen_0069c0f4->spell_ranges[realm];
        spell_range->m_range->EnableRegionSet(enable);
        if (enable != 0) {
            second = g_value_0069c0f8->skill_unlocks[0x1c + spell_range->m_realm] - 8;
            if (second < 1) {
                spell_range->m_range->SetRangeEnabled(0);
            } else {
                spell_range->m_range->SetRangeEnabled(1);
                spell_range->m_range->SetRange(0, second);
            }
        }
    }
}

/* Re-enable all six spell-realm scrollbars after the learned-spell lists were
   rebuilt for a new character. */
// FUNCTION: WIZ8 0x005B7290
void RefreshCampSpellRanges005B7290(void)
{
    W8CampSpellRange* spell_range;
    int realm;
    int second;

    for (realm = 0; realm < 6; ++realm) {
        spell_range = g_camp_screen_0069c0f4->spell_ranges[realm];
        spell_range->m_range->EnableRegionSet(1);
        second = g_value_0069c0f8->skill_unlocks[0x1c + spell_range->m_realm] - 8;
        if (second < 1) {
            spell_range->m_range->SetRangeEnabled(0);
        } else {
            spell_range->m_range->SetRangeEnabled(1);
            spell_range->m_range->SetRange(0, second);
        }
    }
}

/* The spell page proper: one panel per realm, each showing the realm name and
   skill level, the spell-point pool and up to eight learned spell rows; the
   row under the cursor gets the highlight palette and unaffordable or
   unusable spells dim. */
// FUNCTION: WIZ8 0x005B7300
void DrawCampSpellPages005B7300(void)
{
    W8CampScreenState0069C0F4* state;
    W8Character* character = g_value_0069c0f8;
    const W8SpellRealmAnimation* animation;
    int realm;
    int row;
    int left;
    int top;
    int row_top;
    int spell_id;
    int width;
    unsigned int visible;
    unsigned short* palette;

    SetFont(g_font_683660);
    if ((g_camp_screen_0069c0f4->redraw_flags & 0x100000) != 0 && gXStatus.fSpellCastMode == 0) {
        DrawCampResistances005B7790();
    }
    if (g_camp_screen_0069c0f4->redraw_flags == 0xfffffff) {
        DrawCatalogImageAndInvalidate(-14, 0x140, 0, 2, 0, 0xa5, 2, 0);
    }
    for (realm = 0; realm < 6; ++realm) {
        if ((g_camp_screen_0069c0f4->redraw_flags & (0x200000 << realm)) == 0) {
            continue;
        }
        left = (realm % 3) * 0xd5 + 3;
        top = (realm / 3) * 0x8c + 0xaa;
        if (character->skill_unlocks[0x1c + realm] != 0) {
            DrawCatalogImageAndInvalidate(-14, 0x140, 0, 3, left, top, 2, 0);
            SetFontObjectPalette16BPP(g_font_683660, g_font_state_palettes_68ee1c[1]);
            mprintf(left + 0x1b, top + 8, const_cast<wchar_t*>(g_format_s_0064dd28),
                    gppStringList[0x231c / 4]);
            mprintf(left + 0x72, top + 8, gppStringList[0x2324 / 4]);
            SetFontObjectPalette16BPP(g_font_683660, g_colour_68ee08);
            width = StringPixLengthArg(g_font_683660, wcslen(gppStringList[0x231c / 4]) + 2,
                                       const_cast<UINT16*>(g_format_s_colon_00648164),
                                       gppStringList[0x231c / 4]);
            mprintf(left + 0x1b + width, top + 8, const_cast<wchar_t*>(g_format_d_0064dd20),
                    character->skills[0x1c + realm].level);
            width = StringPixLengthArg(
                g_font_683660, 7, const_cast<UINT16*>(g_format_d_slash_d_00614b58),
                GetCharacterRealmSpellPoints(character, realm), character->sp_max[realm]);
            mprintf(left + 0xc8 - width, top + 8, const_cast<wchar_t*>(g_format_d_slash_d_00614b58),
                    GetCharacterRealmSpellPoints(character, realm), character->sp_max[realm]);
            visible = character->skill_unlocks[0x1c + realm];
            if (visible >= 8) {
                visible = 8;
            }
            row_top = top + 0x19;
            for (row = 0; static_cast<unsigned int>(row) < visible; ++row) {
                spell_id = g_camp_screen_0069c0f4->learned_spells.spell_ids_by_realm
                               [realm][g_camp_screen_0069c0f4->learned_spells.scroll[realm] + row];
                if (g_camp_screen_0069c0f4->hover_region ==
                        static_cast<unsigned int>(realm + 0x119) &&
                    g_camp_screen_0069c0f4->selected_spell_row == row) {
                    palette = g_font_state_palettes_68ee1c[5];
                } else if (g_spell_records[spell_id].spell_point_cost <=
                               character->sp_left[realm] &&
                           SpellUsableNow(spell_id, 0)) {
                    palette = g_colour_68ee08;
                } else {
                    palette = g_font_state_palettes_68ee1c[0];
                }
                SetFontObjectPalette16BPP(g_font_683660, palette);
                width =
                    StringPixLengthArg(g_font_683660, 3, const_cast<UINT16*>(g_format_3d_0064dd14),
                                       g_spell_records[spell_id].spell_point_cost);
                mprintf(left + 0x1c, row_top, const_cast<wchar_t*>(g_format_s_006068e4),
                        g_spell_records[spell_id].display_name);
                mprintf(left + 0xb1 - width, row_top, const_cast<wchar_t*>(g_format_d_0064dd20),
                        g_spell_records[spell_id].spell_point_cost);
                row_top += 0xd;
            }
            SetFontObjectPalette16BPP(g_font_683660, g_colour_68ee08);
        }
        state = g_camp_screen_0069c0f4;
        state->spell_ranges[realm]->m_range->Invalidate(0);
        state->spell_ranges[realm]->m_range->Redraw();
    }
    for (realm = 0; realm < 6; ++realm) {
        if (g_camp_screen_0069c0f4->dialog != 0 && realm != 0 && realm != 3) {
            continue;
        }
        if ((g_camp_screen_0069c0f4->redraw_flags & ((0x200000 << realm) | 0x10000)) == 0) {
            continue;
        }
        animation = &g_spell_realm_animations_00648c90[realm];
        if (character->sp_max[realm] != 0) {
            DrawCatalogImageAndInvalidate(-14, animation->image, 0,
                                          g_camp_screen_0069c0f4->animation_frames[realm],
                                          (realm % 3) * 213 + 5, (realm / 3) * 140 + 0xac, 2, 0);
        } else {
            DrawCatalogImageAndInvalidate(-14, animation->image, 0, animation->initial_frame,
                                          (realm % 3) * 213 + 5, (realm / 3) * 140 + 0xac, 2, 0);
        }
    }
}

/* The six resistance bars along the spell page's top strip: each bar clips its
   fill to the learned portion, the overflow to the bonus band or the shortfall
   to the gap, then right-aligns the value inside the bar. */
// FUNCTION: WIZ8 0x005B7790
void DrawCampResistances005B7790(void)
{
    SGPRect saved_clip;
    SGPRect clip;
    W8Character* character = g_value_0069c0f8;
    const W8SpellRealmAnimation* animation;
    wchar_t* text;
    int index;
    int left;
    int top;
    unsigned int filled;
    unsigned int extra;
    unsigned int missing;
    int width;

    DrawCatalogImageAndInvalidate(-14, 0x140, 0, 0, 0x136, 0, 2, 0);
    text = gppStringList[0x2328 / 4];
    width = StringPixLengthArg(
        g_font_683660, wcslen(text),
        reinterpret_cast< // reinterpret-ok: SGP's historical UINT16 text ABI stores wchar_t data
            UINT16*>(text));
    mprintf((0x134 - width) / 2 + 0x144, 0x1e, text);
    for (index = 0; index < 6; ++index) {
        animation = &g_spell_realm_animations_00648c90[index];
        left = (index & 1) * 156 + 0x144;
        top = (index >> 1) * 28 + 0x35;
        DrawCatalogImage(-14, animation->image, 0, animation->initial_frame, left, top, 2, 0);
        if (character->resistances[index].total >= character->resistances[index].base) {
            filled = character->resistances[index].base;
            extra = character->resistances[index].total - filled;
            missing = 0;
        } else {
            filled = character->resistances[index].total;
            extra = 0;
            missing = character->resistances[index].base - filled;
        }
        GetClippingRect(&saved_clip);
        clip.iTop = 0;
        clip.iBottom = 0x1e0;
        if (filled != 0) {
            clip.iLeft = left + 0x18;
            clip.iRight = left + 0x18 + filled;
            SetClippingRect(&clip);
            DrawCatalogImageAndInvalidate(-14, 0x143, 0, 0, left + 0x18, top + 4, 2, 0);
        }
        if (extra != 0) {
            clip.iLeft = left + 0x18 + filled;
            clip.iRight = left + 0x18 + filled + extra;
            SetClippingRect(&clip);
            DrawCatalogImageAndInvalidate(-14, 0x143, 0, 1, left + 0x18, top + 4, 2, 0);
        } else if (missing != 0) {
            clip.iLeft = left + 0x18 + filled;
            clip.iRight = left + 0x18 + filled + missing;
            SetClippingRect(&clip);
            DrawCatalogImageAndInvalidate(-14, 0x143, 0, 2, left + 0x18, top + 4, 2, 0);
        }
        SetClippingRect(&saved_clip);
        width = StringPixLengthArg(g_font_683660, 5, const_cast<UINT16*>(g_format_d_0060aa20),
                                   character->resistances[index].total);
        mprintf(left + 0x93 - width, top + 3, const_cast<wchar_t*>(g_format_d_0060aa20),
                character->resistances[index].total);
    }
}

/* The spell-list region callback: the callback id is the realm index, the
   event's cursor position picks the highlighted row within the eight visible
   ones, wheel input scrolls the realm's range control, and activating a valid
   row opens its spell info dialog. */
// FUNCTION: WIZ8 0x005B79F0
unsigned char SpellListRegionHandler005B79F0(const InputAtom* event, W8Region* region)
{
    unsigned short realm;
    int row;
    unsigned int visible;
    int delta;
    int count;

    PushButtonSoundScheme005587C0(0, 1);
    realm = region->callback_id;
    row = (GetAtomCursorY004285A0(event) - region->y1 - 1) / 0xd;
    visible = g_value_0069c0f8->skill_unlocks[0x1c + realm] -
              g_camp_screen_0069c0f4->learned_spells.scroll[realm];
    if (visible >= 8) {
        visible = 8;
    }
    if (row >= static_cast<int>(visible)) {
        row = -1;
    }
    if (row != g_camp_screen_0069c0f4->selected_spell_row) {
        g_camp_screen_0069c0f4->selected_spell_row = row;
        g_camp_screen_0069c0f4->redraw_flags |= 0x200000 << realm;
    }
    if (event->usEvent > 0x100) {
        if (event->usEvent == 0x400) {
            if ((region->flags & W8_REGION_MOUSE_TRANSITION_MASK) != 0) {
                g_camp_screen_0069c0f4->redraw_flags |= 0x200000 << realm;
            }
            return 0;
        }
        if (event->usEvent == 0x800) {
            delta = GetMouseWheelDeltaValue(event->usParam);
            if (delta > 0) {
                for (count = delta; count != 0; --count) {
                    g_camp_screen_0069c0f4->spell_ranges[realm]->m_range->Decrement();
                }
                delta = 0;
            }
            if (delta < 0) {
                for (count = -delta; count != 0; --count) {
                    g_camp_screen_0069c0f4->spell_ranges[realm]->m_range->Increment();
                }
            }
            return 1;
        }
        return 0;
    }
    if (event->usEvent == 0x100) {
        if ((region->flags & W8_REGION_RIGHT_BUTTON_HELD) != 0 && row != -1) {
            OpenSpellInfoDialog005B7BB0(
                g_camp_screen_0069c0f4->learned_spells.spell_ids_by_realm
                    [realm][g_camp_screen_0069c0f4->learned_spells.scroll[realm] + row]);
        }
        return 1;
    }
    if (event->usEvent == 8) {
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        return 1;
    }
    if (event->usEvent != 0x10) {
        if (event->usEvent != 0x80) {
            return 0;
        }
        region->flags |= W8_REGION_RIGHT_BUTTON_HELD;
        return 1;
    }
    if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0 && row != -1) {
        OpenSpellInfoDialog005B7BB0(
            g_camp_screen_0069c0f4->learned_spells
                .spell_ids_by_realm[realm]
                                   [g_camp_screen_0069c0f4->learned_spells.scroll[realm] + row]);
        return 1;
    }
    return 1;
}

/* Open the spell info dialog for one spell id off the camp spell lists. */
// FUNCTION: WIZ8 0x005B7BB0
void OpenSpellInfoDialog005B7BB0(unsigned int spell_id)
{
    W8SpellInfoDialog* dialog = new W8SpellInfoDialog(spell_id);
    dialog->SetText(&g_wchar_00689b34);
    DisplayCampDialog(dialog);
}

/* The items-page redraw driver, run once per update: each pending group of
   redraw flags is cleared by repainting that block, and the two bottom/right
   panels repaint when their own bits are raised. */
// FUNCTION: WIZ8 0x005b7d10
void RedrawCampItemsPage005B7D10(void)
{
    SetFont(g_font_683660);
    SetObjectShade(g_wiz_text_font_secondary_object_683680, 4);
    if ((g_camp_screen_0069c0f4->redraw_flags & 0x2000) != 0) {
        DrawCampCharacterInfo005B7E00();
    }
    if ((g_camp_screen_0069c0f4->item_redraw_flags & 0x1ff) != 0) {
        DrawCampBackpackItems005B8440();
    }
    if ((g_camp_screen_0069c0f4->item_redraw_flags & 0x3ffe00) != 0) {
        DrawCampEquipmentItems005B8690();
    }
    if ((g_camp_screen_0069c0f4->item_redraw_flags & 0x7fc00000) != 0) {
        DrawCampItemPool005B8B20();
        g_camp_screen_0069c0f4->item_range->UpdateItems(1);
    } else {
        g_camp_screen_0069c0f4->item_range->UpdateItems(0);
    }
    if ((g_camp_screen_0069c0f4->redraw_flags & 0x8000000) != 0) {
        g_camp_action_panel_0069c464->Invalidate(0);
        g_camp_action_panel_0069c464->Redraw();
    } else {
        g_camp_action_panel_0069c464->Redraw();
    }
    if ((g_camp_screen_0069c0f4->redraw_flags & 0x10000000) != 0) {
        g_camp_realm_tab_panel_0069c48c->Invalidate(0);
        g_camp_realm_tab_panel_0069c48c->Redraw();
    } else {
        g_camp_realm_tab_panel_0069c48c->Redraw();
    }
}

/* The character block of the items page: name-free value fields, the
   experience bar, the six realm icons with their point pools, the attribute
   rows and the armor summaries. */
// FUNCTION: WIZ8 0x005b7e00
void DrawCampCharacterInfo005B7E00(void)
{
    W8CampScreenState0069C0F4* state = g_camp_screen_0069c0f4;
    W8Character* character = g_value_0069c0f8;
    int index;
    int realm;
    int top;
    unsigned int filled;
    SGPRect clip;
    SGPRect previous_clip;

    if (state->item_mode != 0) {
        state->character_info->Invalidate(0);
        g_camp_secondary_panel_0069c428->Invalidate(0);
        return;
    }
    DrawCatalogImageAndInvalidate(-14, 0x114, 0, 0, 0x136, 0, 2, 0);
    if (character->experience != character->experience_previous_goal) {
        if (character->experience < character->experience_goal) {
            filled = (character->experience - character->experience_previous_goal) * 0x67 /
                     (character->experience_goal - character->experience_previous_goal);
        } else {
            filled = 0x67;
        }
        if (filled != 0) {
            GetClippingRect(&previous_clip);
            clip.iLeft = 0x1c0;
            clip.iTop = 0;
            clip.iRight = 0x1c0 + filled;
            clip.iBottom = 0x1e0;
            SetClippingRect(&clip);
            DrawCatalogImageAndInvalidate(-14, 0x11d, 0, 0, 0x1c0, 0x26, 2, 0);
            SetClippingRect(&previous_clip);
        }
    }
    DrawRcsText(gppStringList[0x246c / 4], 0x159, 10, 0xce,
                g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C);
    DrawRcsText(gppStringList[0x2470 / 4], 0x15e, 0x18, 0x65,
                g_W8TextBufferLayoutMask005ED548 | g_W8TextBufferLayoutMask005ED554);
    DrawRcsText(gppStringList[0x2474 / 4], 0x15e, 0x26, 0x65,
                g_W8TextBufferLayoutMask005ED548 | g_W8TextBufferLayoutMask005ED554);
    FormatUnsignedIntegerWithCommas(state->caption, character->experience);
    DrawRcsText(state->caption, 0x1c0, 0x18, 0x67,
                g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C);
    FormatUnsignedIntegerWithCommas(state->caption, character->experience_goal);
    DrawRcsText(state->caption, 0x1c0, 0x26, 0x67,
                g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C);
    DrawRcsText(gppStringList[0x2478 / 4], 0x144, 0x3a, 0x45,
                g_W8TextBufferLayoutMask005ED548 | g_W8TextBufferLayoutMask005ED554);
    DrawRcsText(gppStringList[0x247c / 4], 0x144, 0x48, 0x45,
                g_W8TextBufferLayoutMask005ED548 | g_W8TextBufferLayoutMask005ED554);
    DrawRcsText(gppStringList[0x2480 / 4], 0x144, 0x56, 0x45,
                g_W8TextBufferLayoutMask005ED548 | g_W8TextBufferLayoutMask005ED554);
    swprintf(state->caption, g_format_d_slash_d_00614b58, character->hp_current, character->hp_max);
    DrawRcsText(state->caption, 0x18d, 0x3a, 0x2c,
                g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C);
    swprintf(state->caption, g_format_d_slash_d_00614b58, character->stamina,
             character->stamina_max);
    DrawRcsText(state->caption, 0x18d, 0x48, 0x2c,
                g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C);
    if (character->load_category != 0) {
        SetFontObjectPalette16BPP(g_font_683660,
                                  g_font_state_palettes_68ee1c
                                      [g_load_category_palettes_648c48[character->load_category]]);
    }
    swprintf(state->caption, g_format_d_slash_d_00614b58, character->total_carried_weight / 10,
             character->carrying_capacity / 10);
    DrawRcsTextJustified(state->caption, 0x18d, 0x56, 0x2c, 0xc,
                         g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C);
    swprintf(state->caption, g_format_s_colon_d_s_colon_d_0064dd4c, gppStringList[0x2300 / 4],
             character->inventory_weight / 10, gppStringList[0x2304 / 4],
             character->party_weight_share / 10);
    g_camp_help_text_0069c444->SetRegionHelp(state->caption);
    SetFontObjectPalette16BPP(g_font_683660, g_colour_68ee08);
    DrawRcsText(gppStringList[0x2484 / 4], 0x144, 0x72, 0x75,
                g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C);
    top = 0x3a;
    for (index = 0; index < 7; ++index) {
        DrawRcsText(gppStringList[g_attribute_label_ids_64dd30[index]], 0x1c2, top, 0x4c,
                    g_W8TextBufferLayoutMask005ED548 | g_W8TextBufferLayoutMask005ED554);
        swprintf(state->caption, g_format_d_0060aa20, character->attributes[index].effective);
        DrawRcsText(state->caption, 0x212, top, 0x14,
                    g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C);
        top += 0xe;
    }
    top = 0xd;
    for (realm = 0; realm < 6; ++realm) {
        unsigned int frame;
        if (character->sp_max[realm] == 0) {
            frame = g_spell_realm_animations_00648c90[realm].frame_count;
        } else {
            swprintf(state->caption, g_format_d_slash_d_00614b58,
                     GetCharacterRealmSpellPoints(character, realm), character->sp_max[realm]);
            DrawTallRcsText(state->caption, 0x242, top, 0x32,
                            g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C);
            frame = g_spell_realm_animations_00648c90[realm].initial_frame;
        }
        DrawCatalogImageAndInvalidate(-14, g_character_resistance_images_0064ce60[realm], 0, frame,
                                      0x22d, top, 2, 0);
        top += 0x18;
    }
    DrawRcsText(gppStringList[0x2488 / 4], 0x144, 0x80, 0x45,
                g_W8TextBufferLayoutMask005ED548 | g_W8TextBufferLayoutMask005ED554);
    swprintf(state->caption, g_format_d_0060aa20, character->armor_class_total);
    DrawRcsText(state->caption, 0x18d, 0x80, 0x2c,
                g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C);
    DrawRcsText(gppStringList[0x248c / 4], 0x144, 0x8e, 0x45,
                g_W8TextBufferLayoutMask005ED548 | g_W8TextBufferLayoutMask005ED554);
    swprintf(state->caption, g_format_d_0060aa20, character->armor_class_average);
    DrawRcsText(state->caption, 0x18d, 0x8e, 0x2c,
                g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C);
    g_camp_secondary_panel_0069c428->Invalidate(0);
}

/* The eight backpack cells under the character block, flag bits 1..8 of
   item_redraw_flags; bit 0 repaints the header strip and caption. */
// FUNCTION: WIZ8 0x005b8440
void DrawCampBackpackItems005B8440(void)
{
    W8CampScreenState0069C0F4* state = g_camp_screen_0069c0f4;
    W8Character* character = g_value_0069c0f8;
    unsigned int slot;
    int left;
    int top;
    int right;
    int bottom;
    int item_id;

    if ((state->item_redraw_flags & 1) != 0) {
        DrawCatalogImageAndInvalidate(-14, 0x114, 0, 1, 0, 0xa5, 2, 0);
        DrawRcsBoldText(gppStringList[0x24ac / 4], 0xc, 0xae, 0x5d,
                        g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C);
    }
    for (slot = 0; slot < 8; ++slot) {
        left = (slot & 1) * 0x31 + 0xb;
        top = (slot >> 1) * 0x39 + 0xc0;
        right = left + 0x2e;
        bottom = top + 0x36;
        if ((state->item_redraw_flags & (2 << slot)) == 0) {
            continue;
        }
        InvalidateRegion(left, top, right, bottom, 0);
        item_id = character->backpack[slot].item_id;
        if (item_id == -1 || CanCharacterUseItem(character, item_id) != 0) {
            BlitCatalogSurfaceRectTo16BPP(-14, left, top, right, bottom, 0x1b6, 0, 0);
        } else {
            DrawCatalogImage(-14, 0x11a, 0, 0, left, top, 2, 0);
        }
        if (item_id != -1) {
            DrawCatalogImage(-14, g_item_video_objects_68ec68.GetOrCreateVideoObject(item_id), 0, 0,
                             left + 1, top + 1, 2, 0);
            DrawCampItemQuantity005B8EC0(&character->backpack[slot], left + 1, top + 0x28, 0x2c);
            if (character->backpack[slot].identified == 0) {
                DrawCatalogImage(-14, 0x11b, 0, 0, left, top, 2, 0);
            }
        }
        if (item_id != -1 && g_item_records[item_id].binds_on_equip != 0 &&
            character->backpack[slot].bound != 0) {
            DrawCatalogImage(-14, 0x115, 0, 0x32, left, top, 2, 0);
        }
        if (state->hover_region == slot + 0xf4 &&
            (item_id != -1 || (g_status_685170.item_in_cursor != 0 && state->entry_mode != 1))) {
            DrawCatalogImage(-14, 0x115, 0, 0x30, left, top, 2, 0);
        }
    }
}

/* The twelve equipment cells; bit 9 of item_redraw_flags repaints the paper
   doll backdrop and race/gender portrait, bits 10..21 the slots. */
// FUNCTION: WIZ8 0x005b8690
void DrawCampEquipmentItems005B8690(void)
{
    W8CampScreenState0069C0F4* state = g_camp_screen_0069c0f4;
    W8Character* character = g_value_0069c0f8;
    const W8CampScreenRegion* region;
    unsigned int slot;
    int item_id;
    int frame;

    if ((state->item_redraw_flags & 0x200) != 0) {
        DrawCatalogImageAndInvalidate(-14, 0x114, 0, 2, 0x71, 0xa5, 2, 0);
        DrawCatalogImageAndInvalidate(
            -14, g_race_portrait_images_64cda0[character->race * 3 + character->gender], 0, 0, 0xc2,
            0xa5, 2, 0);
        state->redraw_flags |= 0x8000000;
    }
    for (slot = 0; slot < 12; ++slot) {
        if ((state->item_redraw_flags & (0x400 << slot)) == 0) {
            continue;
        }
        region = &g_camp_screen_regions_64cbf0[slot];
        InvalidateRegion(region->x, region->y, region->x + region->width,
                         region->y + region->height, 0);
        BlitCatalogSurfaceRectTo16BPP(-14, region->x, region->y, region->x + region->width,
                                      region->y + region->height, 0x1b6, 0, 0);
        item_id = character->equipment[slot].item_id;
        if (item_id == -1) {
            if (slot == 7 || slot == 9) {
                int paired = character->equipment[GetPairedEquipSlot(slot)].item_id;
                if (paired != -1 && (g_item_records[paired].flags_041 & 4) != 0) {
                    DrawCatalogImageAndInvalidate(-14, 0x146, 0, 0, region->x, region->y, 2, 0);
                }
            }
        } else {
            DrawCatalogImageAndInvalidate(
                -14, g_item_video_objects_68ec68.GetOrCreateVideoObject(item_id), 0, 1, region->x,
                region->y, 2, 0);
            DrawCampItemQuantity005B8EC0(&character->equipment[slot], region->x + 2,
                                         region->y + region->height - 0xd, region->width - 4);
            if (character->equipment[slot].identified == 0) {
                DrawCatalogImage(-14, 0x11b, 0, static_cast<short>(region->unknown_14), region->x,
                                 region->y, 2, 0);
            }
        }
        switch (slot) {
        case 0:
            frame = 0;
            break;
        case 4:
            frame = 1;
            break;
        case 5:
            frame = 3;
            break;
        case 10:
            frame = 2;
            break;
        case 0xb:
            frame = 4;
            break;
        default:
            goto no_armor_label;
        }
        swprintf(state->caption, g_format_d_0060aa20, character->armor_class_by_location[frame]);
        DrawCatalogImageAndInvalidate(-14, 0x11c, 0, 0, region->x + 2, region->y + 1, 2, 0);
        DrawRcsText(state->caption, region->x + 2, region->y + 2, 0x11,
                    g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C);
    no_armor_label:
        DrawCatalogImageAndInvalidate(-14, 0x115, 0, region->unknown_10 + 3, region->x - 2,
                                      region->y - 2, 2, 0);
        if (item_id != -1 && g_item_records[item_id].binds_on_equip != 0 &&
            character->equipment[slot].bound != 0) {
            DrawCatalogImageAndInvalidate(-14, 0x115, 0, region->unknown_10 + 2, region->x - 2,
                                          region->y - 2, 2, 0);
        }
        if (state->hover_region == slot + 0xfc) {
            if (g_status_685170.item_in_cursor == 0) {
                if (character->equipment[slot].item_id == -1) {
                    continue;
                }
            } else if (state->entry_mode == 1 ||
                       !CanEquipItemInSlot(character, g_status_685170.item_in_hand_235b.item_id,
                                           static_cast<unsigned char>(slot), 1) ||
                       !CanCharacterUseItem(character, g_status_685170.item_in_hand_235b.item_id)) {
                if (g_status_685170.item_in_cursor == 0 || state->entry_mode != 1) {
                    continue;
                }
                if (character->equipment[slot].item_id == -1) {
                    continue;
                }
            }
            frame = region->unknown_10;
        } else {
            if (g_status_685170.item_in_cursor == 0 || state->entry_mode == 1 ||
                !IsPartySlotEligible00524A10(giReviewCharSlot) ||
                !CanEquipItemInSlot(character, g_status_685170.item_in_hand_235b.item_id,
                                    static_cast<unsigned char>(slot), 1) ||
                !CanCharacterUseItem(character, g_status_685170.item_in_hand_235b.item_id)) {
                continue;
            }
            frame = region->unknown_10 + 1;
        }
        DrawCatalogImageAndInvalidate(-14, 0x115, 0, frame, region->x - 2, region->y - 2, 2, 0);
    }
}

/* The shared party item pool: the bottom-right grid of up to eight visible
   cells, plus the header strip and the gold counter when bit 22 is raised. */
// FUNCTION: WIZ8 0x005b8b20
void DrawCampItemPool005B8B20(void)
{
    W8CampScreenState0069C0F4* state = g_camp_screen_0069c0f4;
    W8Character* character = g_value_0069c0f8;
    unsigned int visible;
    unsigned int i;
    unsigned int region_id;
    int left;
    int top;
    int right;
    int bottom;
    W8ItemInstance* item;

    if ((state->item_redraw_flags & 0x400000) != 0) {
        DrawCatalogImageAndInvalidate(-14, 0x114, 0, 3, 0x1de, 0xa5, 2, 0);
        DrawCatalogImageAndInvalidate(-14, 0x118, 0, 0, 0x202, 0x1a6, 2, 0);
        state->redraw_flags |= 0x10000000;
    }
    if (g_status_685170.game_started == 0) {
        return;
    }
    if ((state->item_redraw_flags & 0x400000) != 0) {
        DrawRcsBoldText(gppStringList[0x24b0 / 4], 0x1e6, 0xae, 0x8d,
                        g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C);
        FormatUnsignedIntegerWithCommas(state->caption, g_status_685170.party_gold);
        DrawRcsText(state->caption, 0x229, 0x1a9, 0x30,
                    g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C);
    }
    for (i = 0;; ++i) {
        visible = state->item_list_count - state->item_scroll;
        if (visible > 8) {
            visible = 8;
        }
        if (i >= visible) {
            break;
        }
        left = (i & 1) * 0x31 + 0x200;
        top = (i >> 1) * 0x39 + 0xc0;
        right = left + 0x2e;
        bottom = top + 0x36;
        item = &g_status_685170.party_item_pool_0021[state->item_list_4ec[state->item_scroll + i]];
        if ((state->item_redraw_flags & (0x800000 << i)) == 0) {
            continue;
        }
        InvalidateRegion(left, top, right, bottom, 0);
        if (CanCharacterUseItem(character, item->item_id) == 0) {
            DrawCatalogImage(-14, 0x11a, 0, 0, left, top, 2, 0);
        } else {
            BlitCatalogSurfaceRectTo16BPP(-14, left, top, right, bottom, 0x1b6, 0, 0);
        }
        DrawCatalogImage(-14, g_item_video_objects_68ec68.GetOrCreateVideoObject(item->item_id), 0,
                         0, left + 1, top + 1, 2, 0);
        DrawCampItemQuantity005B8EC0(item, left + 1, top + 0x28, 0x2c);
        if (item->identified == 0) {
            DrawCatalogImage(-14, 0x11b, 0, 0, left, top, 2, 0);
        }
        if (g_item_records[item->item_id].binds_on_equip != 0 && item->bound != 0) {
            DrawCatalogImage(-14, 0x115, 0, 0x32, left, top, 2, 0);
        }
        if (state->hover_region == i + 0x108) {
            DrawCatalogImage(-14, 0x115, 0, 0x30, left, top, 2, 0);
        }
    }
    i = state->item_list_count - state->item_scroll;
    if (i < 8) {
        region_id = i + 0x108;
        do {
            left = (i & 1) * 0x31 + 0x200;
            top = (i >> 1) * 0x39 + 0xc0;
            right = left + 0x2e;
            bottom = top + 0x36;
            if ((state->item_redraw_flags & (0x800000 << i)) != 0) {
                InvalidateRegion(left, top, right, bottom, 0);
                BlitCatalogSurfaceRectTo16BPP(-14, left, top, right, bottom, 0x1b6, 0, 0);
                if (state->hover_region == region_id && g_status_685170.item_in_cursor != 0 &&
                    state->entry_mode != 1) {
                    DrawCatalogImage(-14, 0x115, 0, 0x30, left, top, 2, 0);
                }
            }
            ++i;
            ++region_id;
        } while (region_id < 0x110);
    }
}

/* The stack count or charge figure in a cell's bottom-right corner, keyed off
   the item record's quantity kind; charge figures ride the alternate font
   palettes and restore the default afterwards. */
// FUNCTION: WIZ8 0x005b8ec0
void DrawCampItemQuantity005B8EC0(W8ItemInstance* item, int left, int top, int width)
{
    W8CampScreenState0069C0F4* state = g_camp_screen_0069c0f4;
    int height;

    switch (g_item_records[item->item_id].quantity_kind) {
    case 1:
        if (item->stack_count > 1) {
            swprintf(state->caption, g_format_d_0060aa20, item->stack_count);
            DrawRcsText(state->caption, left, top, width,
                        g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED550);
        }
        break;
    case 2:
    case 3:
        SetFontObjectPalette16BPP(g_font_683660, g_font_state_palettes_68ee1c[3]);
        if (item->identified == 0) {
            swprintf(state->caption, L"?");
        } else {
            swprintf(state->caption, g_format_d_0060aa20, item->uses_or_charges);
        }
        height = GetFontHeight(g_font_683660);
        DrawRcsTextJustified(state->caption, left, top, width, height,
                             g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED550);
        break;
    case 4:
        SetFontObjectPalette16BPP(g_font_683660, g_font_state_palettes_68ee1c[5]);
        swprintf(state->caption, g_format_d_0060aa20, item->uses_or_charges);
        height = GetFontHeight(g_font_683660);
        DrawRcsTextJustified(state->caption, left, top, width, height,
                             g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED550);
        break;
    }
    SetFontObjectPalette16BPP(g_font_683660, g_colour_68ee08);
}

/* The bottom-left panel and its two buttons: drop the held equipment and
   toggle the character's party-row flag. */
// FUNCTION: WIZ8 0x005b9070
int CreateCampActionPanel005B9070(void)
{
    int index;

    g_camp_action_panel_0069c464 = 0;
    for (index = 0; index < 2; ++index) {
        if (g_camp_action_buttons_0069c468[index] != 0) {
            g_camp_action_buttons_0069c468[index] = 0;
        }
    }
    g_camp_action_panel_0069c464 = new Controls(0x78, 0x19c, 0xbf, 0x1b4, -1, 0, 0);
    if (g_camp_action_panel_0069c464 == 0) {
        return 0;
    }
    g_camp_action_buttons_0069c468[0] = new W8TextControl(g_camp_action_panel_0069c464, 0x117, 5, 0,
                                                          0x21, 0x18, 0x11e, 0, 0, 2, 1, 2, 3);
    g_camp_action_buttons_0069c468[1] = new W8TextControl(g_camp_action_panel_0069c464, 0x118, 0x26,
                                                          0, 0x42, 0x18, 0x11e, 0, 8, 4, 9, 5, 0xb);
    index = 0;
    while (g_camp_action_buttons_0069c468[index] != 0) {
        ++index;
        if (index > 1) {
            g_camp_action_buttons_0069c468[1]->AddLayoutFlags(g_W8TextControlMask005ED578);
            g_camp_action_buttons_0069c468[0]->m_primaryActivationCallback =
                UnequipBothHands005BB010;
            g_camp_action_buttons_0069c468[1]->m_primaryActivationCallback =
                TogglePartyRowFlag005BB140;
            g_camp_action_panel_0069c464->SetEnabled(1);
            return 1;
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x005b9270
void EnableCampActionButtons005B9270(void)
{
    g_camp_action_buttons_0069c468[0]->SetActive(1);
    g_camp_action_buttons_0069c468[1]->SetActive(1);
    if (g_status_685170.game_started != 0) {
        g_camp_action_buttons_0069c468[1]->SetEnabled(1);
        if (g_status_685170.buffers.party_rows[giReviewCharSlot].flag_0f5 != 0) {
            g_camp_action_buttons_0069c468[1]->EnableSecondaryState(0);
            g_camp_action_buttons_0069c468[0]->SetEnabled(0);
        } else {
            g_camp_action_buttons_0069c468[1]->DisableSecondaryState(0);
            g_camp_action_buttons_0069c468[0]->SetEnabled(1);
        }
    } else {
        g_camp_action_buttons_0069c468[0]->SetEnabled(0);
        g_camp_action_buttons_0069c468[1]->SetEnabled(0);
    }
}

// FUNCTION: WIZ8 0x005b9310
void DisableCampActionButtons005B9310(void)
{
    g_camp_action_buttons_0069c468[0]->SetActive(0);
    g_camp_action_buttons_0069c468[1]->SetActive(0);
}

// FUNCTION: WIZ8 0x005b9330
void RefreshCampActionPanel005B9330(char invalidate)
{
    if (invalidate != 0) {
        g_camp_action_panel_0069c464->Invalidate(0);
    }
    g_camp_action_panel_0069c464->Redraw();
}

/* The right-hand panel of seven tabs: the six realm filters and the pool sort
   button. */
// FUNCTION: WIZ8 0x005b9350
int CreateItemsTabPanel005B9350(void)
{
    int index;

    g_camp_realm_tab_panel_0069c48c = 0;
    for (index = 0; index < 7; ++index) {
        if (g_camp_realm_tabs_0069c470[index] != 0) {
            g_camp_realm_tabs_0069c470[index] = 0;
        }
    }
    g_camp_realm_tab_panel_0069c48c = new Controls(0x1e5, 0xc1, 0x1fd, 0x189, -1, 0, 0);
    if (g_camp_realm_tab_panel_0069c48c == 0) {
        return 0;
    }
    g_camp_realm_tabs_0069c470[0] = new W8TextControl(g_camp_realm_tab_panel_0069c48c, 0x110, 0, 0,
                                                      0x18, 0x18, 0x11f, 0, 0, 2, 1, 4, 3);
    g_camp_realm_tabs_0069c470[1] =
        new W8TextControl(g_camp_realm_tab_panel_0069c48c, 0x111, 0, 0x19, 0x18, 0x31, 0x11f, 0,
                          0xf, 0x11, 0x10, 0x13, 0x12);
    g_camp_realm_tabs_0069c470[2] =
        new W8TextControl(g_camp_realm_tab_panel_0069c48c, 0x112, 0, 0x4b, 0x18, 0x63, 0x11f, 0,
                          0x14, 0x16, 0x15, 0x18, 0x17);
    g_camp_realm_tabs_0069c470[3] = new W8TextControl(g_camp_realm_tab_panel_0069c48c, 0x113, 0,
                                                      0x32, 0x18, 0x4a, 0x11f, 0, 5, 7, 6, 9, 8);
    g_camp_realm_tabs_0069c470[4] =
        new W8TextControl(g_camp_realm_tab_panel_0069c48c, 0x114, 0, 0x6c, 0x18, 0x84, 0x11f, 0, 10,
                          0xc, 0xb, 0xe, 0xd);
    g_camp_realm_tabs_0069c470[5] =
        new W8TextControl(g_camp_realm_tab_panel_0069c48c, 0x115, 0, 0x8e, 0x18, 0xa6, 0x11f, 0,
                          0x28, 0x2a, 0x29, 0x2c, 0x2b);
    g_camp_realm_tabs_0069c470[6] =
        new W8TextControl(g_camp_realm_tab_panel_0069c48c, 0x116, 0, 0xb0, 0x18, 0xc8, 0x11f, 0,
                          0x1e, 0x20, 0x1f, 0x22, 0x21);
    index = 0;
    while (g_camp_realm_tabs_0069c470[index] != 0) {
        ++index;
        if (index > 6) {
            for (index = 0; index < 6; ++index) {
                g_camp_realm_tabs_0069c470[index]->AddLayoutFlags(g_W8TextControlMask005ED578);
            }
            g_camp_realm_tabs_0069c470[0]->m_primaryActivationCallback =
                SelectItemsRealmTab005BB1C0;
            g_camp_realm_tabs_0069c470[1]->m_primaryActivationCallback =
                SelectItemsRealmTab005BB1D0;
            g_camp_realm_tabs_0069c470[2]->m_primaryActivationCallback =
                SelectItemsRealmTab005BB1E0;
            g_camp_realm_tabs_0069c470[3]->m_primaryActivationCallback =
                SelectItemsRealmTab005BB1F0;
            g_camp_realm_tabs_0069c470[4]->m_primaryActivationCallback =
                SelectItemsRealmTab005BB200;
            g_camp_realm_tabs_0069c470[5]->m_primaryActivationCallback =
                SelectItemsRealmTab005BB210;
            g_camp_realm_tabs_0069c470[6]->m_primaryActivationCallback = SortPartyItemPool005BB220;
            g_camp_realm_tab_panel_0069c48c->SetEnabled(1);
            return 1;
        }
    }
    return 0;
}

/* Realm tab activation state: every tab is active and enabled while the game
   runs, and a realm with its flag set keeps the secondary (highlighted) state.
   The sort button has no realm flag of its own. */
// FUNCTION: WIZ8 0x005b97b0
void UpdateItemsRealmTabs005B97B0(void)
{
    int index;
    unsigned char flag;

    for (index = 0; index < 7; ++index) {
        g_camp_realm_tabs_0069c470[index]->SetActive(1);
    }
    if (g_status_685170.game_started == 0) {
        for (index = 0; index < 7; ++index) {
            g_camp_realm_tabs_0069c470[index]->SetEnabled(0);
        }
        return;
    }
    for (index = 0; index < 7; ++index) {
        g_camp_realm_tabs_0069c470[index]->SetEnabled(1);
        switch (index) {
        case 0:
            flag = g_camp_screen_0069c0f4->realm_flags[2];
            break;
        case 1:
            flag = g_camp_screen_0069c0f4->realm_flags[3];
            break;
        case 2:
            flag = g_camp_screen_0069c0f4->realm_flags[5];
            break;
        case 3:
            flag = g_camp_screen_0069c0f4->realm_flags[4];
            break;
        case 4:
            flag = g_camp_screen_0069c0f4->realm_flags[0];
            break;
        case 5:
            flag = g_camp_screen_0069c0f4->realm_flags[1];
            break;
        default:
            continue;
        }
        if (flag != 0) {
            g_camp_realm_tabs_0069c470[index]->EnableSecondaryState(0);
        }
    }
}

// FUNCTION: WIZ8 0x005b98c0
void DisableItemsRealmTabs005B98C0(void)
{
    int index;

    for (index = 0; index < 7; ++index) {
        g_camp_realm_tabs_0069c470[index]->SetActive(0);
    }
}

// FUNCTION: WIZ8 0x005b98e0
void RefreshItemsTabPanel005B98E0(char invalidate)
{
    if (invalidate != 0) {
        g_camp_realm_tab_panel_0069c48c->Invalidate(0);
    }
    g_camp_realm_tab_panel_0069c48c->Redraw();
}

/* The top secondary panel: the Items/Character info page tabs, the help line,
   the seven attribute labels and the four secondary value labels. The labels
   are W8CampInfoLabel005EF2B0 controls created with absolute coordinates
   relative to the panel origin. */
// FUNCTION: WIZ8 0x005b9900
int CreateCampSecondaryPanel005B9900(void)
{
    Controls* panel;
    int index;
    int left;
    int right;
    int top;

    g_camp_secondary_panel_0069c428 = 0;
    for (index = 0; index < 2; ++index) {
        if (g_camp_page_tabs_0069c43c[index] != 0) {
            g_camp_page_tabs_0069c43c[index] = 0;
        }
    }
    g_camp_help_text_0069c444 = 0;
    for (index = 0; index < 7; ++index) {
        if (g_camp_stat_labels_0069c448[index] != 0) {
            g_camp_stat_labels_0069c448[index] = 0;
        }
    }
    for (index = 0; index < 4; ++index) {
        if (g_camp_info_labels_0069c42c[index] != 0) {
            g_camp_info_labels_0069c42c[index] = 0;
        }
    }
    panel = new Controls(0x13c, 7, 0x154, 0x34, 0x120, 0, 0);
    g_camp_secondary_panel_0069c428 = panel;
    if (panel == 0) {
        return 0;
    }
    panel->AcquireRegionSet(&g_camp_secondary_region_set_0069c490);
    g_camp_page_tabs_0069c43c[0] =
        new W8TextControl(panel, -1, 2, 2, 0x16, 0x16, 0x121, 0, 0, 2, 1, 4, 3);
    g_camp_page_tabs_0069c43c[1] =
        new W8TextControl(panel, -1, 2, 0x17, 0x16, 0x2b, 0x121, 0, 5, 7, 6, 9, 8);
    for (index = 0; index < 2; ++index) {
        if (g_camp_page_tabs_0069c43c[index] == 0) {
            return 0;
        }
    }
    g_camp_help_text_0069c444 = new W8HelpTextControl(panel, -1, 5, 0x4d, 0x7c, 0x59);
    if (g_camp_help_text_0069c444 == 0) {
        return 0;
    }
    left = 0x1c2 - panel->origin_x;
    right = 0x20e - panel->origin_x;
    top = 0x3a - panel->origin_y;
    for (index = 0; index < 7; ++index) {
        g_camp_stat_labels_0069c448[index] = new W8CampInfoLabel005EF2B0(
            panel, -1, left, top, right, top + 0xc, -1, -1, -1, -1, -1, -1, -1);
        g_camp_stat_labels_0069c448[index]->EnableRegionHelp(0x958);
        top += 0xe;
    }
    left = 0x144 - panel->origin_x;
    right = 0x1b9 - panel->origin_x;
    top = -panel->origin_y;
    g_camp_info_labels_0069c42c[0] = new W8CampInfoLabel005EF2B0(
        panel, -1, left, top + 0x3a, right, top + 0x46, -1, -1, -1, -1, -1, -1, -1);
    g_camp_info_labels_0069c42c[1] = new W8CampInfoLabel005EF2B0(
        panel, -1, left, top + 0x48, right, top + 0x54, -1, -1, -1, -1, -1, -1, -1);
    g_camp_info_labels_0069c42c[2] = new W8CampInfoLabel005EF2B0(
        panel, -1, left, top + 0x80, right, top + 0x8c, -1, -1, -1, -1, -1, -1, -1);
    g_camp_info_labels_0069c42c[3] = new W8CampInfoLabel005EF2B0(
        panel, -1, left, top + 0x8e, right, top + 0x9a, -1, -1, -1, -1, -1, -1, -1);
    g_camp_stat_labels_0069c448[0]->m_secondaryActivationCallback = OpenStatInfoDialog005BA200;
    g_camp_stat_labels_0069c448[1]->m_secondaryActivationCallback = OpenStatInfoDialog005BA210;
    g_camp_stat_labels_0069c448[2]->m_secondaryActivationCallback = OpenStatInfoDialog005BA220;
    g_camp_stat_labels_0069c448[3]->m_secondaryActivationCallback = OpenStatInfoDialog005BA230;
    g_camp_stat_labels_0069c448[4]->m_secondaryActivationCallback = OpenStatInfoDialog005BA240;
    g_camp_stat_labels_0069c448[5]->m_secondaryActivationCallback = OpenStatInfoDialog005BA250;
    g_camp_stat_labels_0069c448[6]->m_secondaryActivationCallback = OpenStatInfoDialog005BA260;
    g_camp_info_labels_0069c42c[0]->m_secondaryActivationCallback =
        OpenSecondaryStatInfoDialog005BA270;
    g_camp_info_labels_0069c42c[1]->m_secondaryActivationCallback =
        OpenSecondaryStatInfoDialog005BA280;
    g_camp_info_labels_0069c42c[2]->m_secondaryActivationCallback =
        OpenSecondaryStatInfoDialog005BA290;
    g_camp_info_labels_0069c42c[3]->m_secondaryActivationCallback =
        OpenSecondaryStatInfoDialog005BA290;
    for (index = 0; index < 4; ++index) {
        g_camp_info_labels_0069c42c[index]->EnableRegionHelp(0x958);
    }
    g_camp_help_text_0069c444->m_secondaryActivationCallback = OpenSecondaryStatInfoDialog005BA2A0;
    g_camp_page_tabs_0069c43c[0]->AddLayoutFlags(g_W8TextControlMask005ED578);
    g_camp_page_tabs_0069c43c[1]->AddLayoutFlags(g_W8TextControlMask005ED578);
    g_camp_page_tabs_0069c43c[0]->m_primaryActivationCallback = SetItemPageMode005B9FB0;
    g_camp_page_tabs_0069c43c[1]->m_primaryActivationCallback = SetItemPageMode005B9FC0;
    g_camp_page_tabs_0069c43c[0]->EnableRegionHelp(0x95d);
    g_camp_page_tabs_0069c43c[1]->EnableRegionHelp(0x95e);
    panel->SetEnabled(1);
    return 1;
}

// FUNCTION: WIZ8 0x005b9f00
void EnableCampSecondaryPanel005B9F00(void)
{
    int index;

    g_camp_secondary_panel_0069c428->EnableRegionSet(1);
    for (index = 0; index < 2; ++index) {
        g_camp_page_tabs_0069c43c[index]->SetActive(1);
        g_camp_page_tabs_0069c43c[index]->SetEnabled(1);
    }
    if (g_camp_screen_0069c0f4->item_mode == 0) {
        SetItemPageMode005B9FD0(0);
        return;
    }
    SetItemPageMode005B9FD0(1);
}

// FUNCTION: WIZ8 0x005b9f60
void DisableCampSecondaryPanel005B9F60(void)
{
    int index;

    g_camp_secondary_panel_0069c428->EnableRegionSet(0);
    for (index = 0; index < 2; ++index) {
        g_camp_page_tabs_0069c43c[index]->SetActive(0);
    }
}

// FUNCTION: WIZ8 0x005b9f90
void RefreshCampSecondaryPanel005B9F90(char invalidate)
{
    if (invalidate != 0) {
        g_camp_secondary_panel_0069c428->Invalidate(0);
    }
    g_camp_secondary_panel_0069c428->Redraw();
}

// FUNCTION: WIZ8 0x005b3150
W8CampCharacterInfo::W8CampCharacterInfo() : Controls(0x136, 0, 0x280, 0xa5, 0x122, 0, 0)
{
    AcquireRegionSet(&g_camp_character_info_region_set);
    m_combat_view = 1;
    m_button_058 = new W8TextControl(this, -1, 8, 0x8b, 0, 0, 0x121, 0, 15, 16, 17, 19, 18);
    m_button_058->m_listener = this;
    m_button_058->EnableRegionHelp(0x960);
    m_button_054 = new W8TextControl(this, -1, 8, 0x8b, 0, 0, 0x121, 0, 10, 11, 12, 14, 13);
    m_button_054->m_listener = this;
    m_button_054->EnableRegionHelp(0x95f);
    m_values[0] = new W8HelpTextControl(this, -1, 0xa0, 0x4c, 0xc0, 0x58);
    m_values[1] = new W8HelpTextControl(this, -1, 0x122, 0x4c, 0x142, 0x58);
    m_values[2] = new W8HelpTextControl(this, -1, 0xa0, 0x3e, 0xc0, 0x4a);
    m_values[3] = new W8HelpTextControl(this, -1, 0x122, 0x3e, 0x142, 0x4a);
}

// FUNCTION: WIZ8 0x005b33a0
void W8CampCharacterInfo::SetEnabled(bool enabled)
{
    EnableRegionSet(enabled);
    Controls::SetEnabled(enabled);
    if (enabled)
        SetCombatView(m_combat_view);
}

// FUNCTION: WIZ8 0x005b33d0
void W8CampCharacterInfo::SetCombatView(bool enabled)
{
    m_combat_view = enabled;
    m_button_058->SetActive(enabled);
    m_button_054->SetActive(!enabled);
    m_values[0]->SetActive(enabled);
    m_values[1]->SetActive(enabled);
    m_values[2]->SetActive(enabled);
    m_values[3]->SetActive(enabled);
    if (enabled) {
        m_renderArg_20 = 0;
    } else if (g_value_0069c0f8->armor_class_components[11] > 0) {
        m_renderArg_20 = 2;
    } else {
        m_renderArg_20 = 1;
    }
    Invalidate(0);
}

// FUNCTION: WIZ8 0x005b3470
void W8CampCharacterInfo::OnPrimary(W8TextControl* control)
{
    SetCombatView(control == m_button_058);
}

// GLOBAL: WIZ8 0x0061e798
const unsigned short g_camp_armor_class_labels[12] = {1052, 1053, 1054, 1055, 1056, 1057,
                                                      1058, 1059, 1060, 1061, 1062, 1063};

// FUNCTION: WIZ8 0x005b34a0
void W8CampCharacterInfo::Redraw()
{
    bool redraw = m_fEnabled && m_fDirty;
    if (!m_combat_view &&
        ((g_value_0069c0f8->armor_class_components[11] > 0 && m_renderArg_20 != 2) ||
         (g_value_0069c0f8->armor_class_components[11] <= 0 && m_renderArg_20 == 2))) {
        SetCombatView(0);
    }
    Controls::Redraw();
    if (!redraw)
        return;
    InvalidateCampPanel005B9EF0();
    DrawRcsText(gppStringList[0x24d4 / 4], 0x15e, 0x84, 0x4e,
                g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED548);
    swprintf(g_camp_screen_0069c0f4->caption, L"%d", g_value_0069c0f8->value_09f9);
    DrawRcsText(g_camp_screen_0069c0f4->caption, 0x1ae, 0x84, 0x20,
                g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
    DrawRcsText(gppStringList[0x24d8 / 4], 0x15e, 0x92, 0x4e,
                g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED548);
    swprintf(g_camp_screen_0069c0f4->caption, L"%d", g_value_0069c0f8->death_count_09fd);
    DrawRcsText(g_camp_screen_0069c0f4->caption, 0x1ae, 0x92, 0x20,
                g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
    if (m_combat_view) {
        DrawRcsText(gppStringList[0x22c0 / 4], 0x15b, 10, 0x11d,
                    g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
        DrawRcsText(gppStringList[0x22c4 / 4], 0x15e, 0x30, 0x4e,
                    g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED548);
        swprintf(g_camp_screen_0069c0f4->caption, L"%d", g_value_0069c0f8->initiative);
        DrawRcsText(g_camp_screen_0069c0f4->caption, 0x1ae, 0x30, 0x20,
                    g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
        if (g_value_0069c0f8->equipment[6].item_id == -1 &&
            g_value_0069c0f8->equipment[7].item_id == -1) {
            DrawRcsText(gppStringList[0x22d4 / 4], 0x1d6, 0x22, 0x50,
                        g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
            DrawRcsText(gppStringList[0x22d0 / 4], 0x228, 0x22, 0x50,
                        g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
        } else {
            DrawRcsText(gppStringList[0x22c8 / 4], 0x1d6, 0x22, 0x50,
                        g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
            DrawRcsText(gppStringList[0x22cc / 4], 0x228, 0x22, 0x50,
                        g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
        }
        DrawRcsText(gppStringList[0x22d8 / 4], 0x1f8, 0x30, 0x5e,
                    g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
        DrawRcsText(gppStringList[0x22f4 / 4], 0x1f8, 0x3e, 0x5e,
                    g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
        DrawRcsText(gppStringList[0x22f0 / 4], 0x1f8, 0x4c, 0x5e,
                    g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
        DrawRcsText(gppStringList[0x22e8 / 4], 0x1f8, 0x5a, 0x5e,
                    g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
        DrawRcsText(gppStringList[0x22ec / 4], 0x1f8, 0x68, 0x5e,
                    g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
        DrawRcsText(gppStringList[0x22dc / 4], 0x1f8, 0x76, 0x5e,
                    g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
        DrawRcsText(gppStringList[0x22e0 / 4], 0x1f8, 0x84, 0x5e,
                    g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
        DrawRcsText(gppStringList[0x22e4 / 4], 0x1f8, 0x92, 0x5e,
                    g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
        bool unknown_partner =
            ItemHasSingledOutGenericName(g_value_0069c0f8->equipment[6].item_id) &&
            !g_value_0069c0f8->equipment[7].identified;
        for (unsigned int hand = 0; hand < 2; ++hand) {
            W8HandAttack* attack = &g_value_0069c0f8->hand_attacks[hand];
            if (!attack->in_play)
                continue;
            int x = hand ? 600 : 0x1d6;
            if (g_value_0069c0f8->equipment[hand + 6].item_id != -1 &&
                (!g_value_0069c0f8->equipment[hand + 6].identified ||
                 (hand == 0 && unknown_partner))) {
                for (int row = 0; row < 8; ++row) {
                    DrawRcsText(L"?", x, 0x30 + row * 14, 0x20,
                                g_W8TextBufferLayoutMask005ED54C |
                                    g_W8TextBufferLayoutMask005ED554);
                }
                wcscpy(g_camp_screen_0069c0f4->caption, gppStringList[0x2570 / 4]);
                wcscat(g_camp_screen_0069c0f4->caption, L"?");
                wcscat(g_camp_screen_0069c0f4->caption, gppStringList[0x256c / 4]);
                wcscat(g_camp_screen_0069c0f4->caption, L"?");
                wcscat(g_camp_screen_0069c0f4->caption, gppStringList[0x2568 / 4]);
                wcscat(g_camp_screen_0069c0f4->caption, L"?");
                m_values[hand + 2]->SetRegionHelp(g_camp_screen_0069c0f4->caption);
                wcscpy(g_camp_screen_0069c0f4->caption, gppStringList[0x2564 / 4]);
                wcscat(g_camp_screen_0069c0f4->caption, L"?");
                wcscat(g_camp_screen_0069c0f4->caption, gppStringList[0x2568 / 4]);
                wcscat(g_camp_screen_0069c0f4->caption, L"?");
                m_values[hand]->SetRegionHelp(g_camp_screen_0069c0f4->caption);
                continue;
            }
            W8Dice dice;
            GetCharacterHandDamageDice(g_value_0069c0f8, hand, &dice);
            unsigned int damage_bonus = GetCharacterHandDamageBonus(g_value_0069c0f8, hand);
            unsigned int minimum = ((dice.base + dice.count) * (100 + damage_bonus) + 50) / 100;
            unsigned int maximum =
                ((dice.base + dice.count * dice.sides) * (100 + damage_bonus) + 50) / 100;
            if (minimum < 2)
                minimum = 1;
            if (maximum < 2)
                maximum = 1;
            int hit_bonus = attack->hit_bonus + g_value_0069c0f8->bonus_1770.value_01;
            int skill_bonus =
                (attack->attack_score < 0 ? attack->attack_score - 2 : attack->attack_score + 2) /
                5;
            swprintf(g_camp_screen_0069c0f4->caption, L"%+d",
                     attack->damage_bonus + g_value_0069c0f8->bonus_1770.value_00);
            DrawRcsText(g_camp_screen_0069c0f4->caption, x, 0x30, 0x20,
                        g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
            swprintf(g_camp_screen_0069c0f4->caption, L"%d-%d", minimum, maximum);
            DrawRcsText(g_camp_screen_0069c0f4->caption, x, 0x3e, 0x20,
                        g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
            wcscpy(g_camp_screen_0069c0f4->caption, gppStringList[0x2570 / 4]);
            wcscat(g_camp_screen_0069c0f4->caption,
                   FormatWideString(L" %d, ", dice.base + dice.count));
            wcscat(g_camp_screen_0069c0f4->caption, gppStringList[0x256c / 4]);
            wcscat(g_camp_screen_0069c0f4->caption,
                   FormatWideString(L" %d, ", dice.base + dice.count * dice.sides));
            wcscat(g_camp_screen_0069c0f4->caption, gppStringList[0x2568 / 4]);
            wcscat(g_camp_screen_0069c0f4->caption, FormatWideString(L" %+d%%", damage_bonus));
            m_values[hand + 2]->SetRegionHelp(g_camp_screen_0069c0f4->caption);
            swprintf(g_camp_screen_0069c0f4->caption, L"%d", skill_bonus + hit_bonus);
            DrawRcsText(g_camp_screen_0069c0f4->caption, x, 0x4c, 0x20,
                        g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
            wcscpy(g_camp_screen_0069c0f4->caption, gppStringList[0x2564 / 4]);
            wcscat(g_camp_screen_0069c0f4->caption, FormatWideString(L" %d, ", skill_bonus));
            wcscat(g_camp_screen_0069c0f4->caption, gppStringList[0x2568 / 4]);
            wcscat(g_camp_screen_0069c0f4->caption, FormatWideString(L" %+d", hit_bonus));
            m_values[hand]->SetRegionHelp(g_camp_screen_0069c0f4->caption);
            swprintf(g_camp_screen_0069c0f4->caption, L"%d", attack->attacks);
            DrawRcsText(g_camp_screen_0069c0f4->caption, x, 0x5a, 0x20,
                        g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
            swprintf(g_camp_screen_0069c0f4->caption, L"%d", attack->swings);
            DrawRcsText(g_camp_screen_0069c0f4->caption, x, 0x68, 0x20,
                        g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
            swprintf(g_camp_screen_0069c0f4->caption, L"%+d", hit_bonus);
            DrawRcsText(g_camp_screen_0069c0f4->caption, x, 0x76, 0x20,
                        g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
            swprintf(g_camp_screen_0069c0f4->caption, L"%+d",
                     attack->value_25 + g_value_0069c0f8->bonus_1770.value_02);
            DrawRcsText(g_camp_screen_0069c0f4->caption, x, 0x84, 0x20,
                        g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
            swprintf(g_camp_screen_0069c0f4->caption, L"%+d%%", damage_bonus);
            DrawRcsText(g_camp_screen_0069c0f4->caption, x, 0x92, 0x20,
                        g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
        }
    } else {
        DrawRcsText(gppStringList[0x22f8 / 4], 0x15b, 10, 0x11d,
                    g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
        for (unsigned int component = 0; component < 12; ++component) {
            if (component == 11 && g_value_0069c0f8->armor_class_components[11] == 0)
                continue;
            int y = (component % 6) * 14 + 0x22;
            int label_x = component / 6 ? 0x1ef : 0x15e;
            int value_x = component / 6 ? 600 : 0x1c7;
            DrawRcsText(gppStringList[g_camp_armor_class_labels[component]], label_x, y, 0x67,
                        g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED548);
            int value = g_value_0069c0f8->armor_class_components[component];
            if (value) {
                swprintf(g_camp_screen_0069c0f4->caption, L"%+d", value);
                DrawRcsText(g_camp_screen_0069c0f4->caption, value_x, y, 0x20,
                            g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
            }
        }
        DrawRcsText(gppStringList[0x22fc / 4], 0x1da, 0x84, 0x7c,
                    g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED548);
        swprintf(g_camp_screen_0069c0f4->caption, L"%d%%", g_value_0069c0f8->damage_reduction);
        DrawRcsText(g_camp_screen_0069c0f4->caption, 600, 0x84, 0x20,
                    g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
    }
}

// FUNCTION: WIZ8 0x005c5c60
unsigned char CampSkillMouseWheel(const InputAtom* event, W8Region*)
{
    PushButtonSoundScheme005587C0(0, 1);
    if (event->usEvent != 0x800) {
        return 0;
    }
    int delta = GetMouseWheelDeltaValue(event->usParam);
    int step;
    for (step = 0; step < delta; ++step) {
        g_camp_screen_0069c0f4->skill_range->m_range->Decrement();
    }
    for (step = 0; step < -delta; ++step) {
        g_camp_screen_0069c0f4->skill_range->m_range->Increment();
    }
    return 1;
}

// FUNCTION: WIZ8 0x005c4540
W8CampSkillControls::W8CampSkillControls()
{
    AcquireRegionSet(&g_camp_skill_controls_region_set);
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
    if (g_camp_screen_0069c0f4->skill_flag) {
        m_buttons[2]->EnableSecondaryState(0);
    }
    unsigned int region = AddRegionToSet(g_camp_skill_controls_region_set);
    SetRegionCallback(region, CampSkillMouseWheel, 0);
    SetRegionBounds(region, 0x15d, 0xbe, 0x260, 0x1b5);
    Controls::SetEnabled(1);
}

// SYNTHETIC: WIZ8 0x005c4780
// W8CampSkillControls::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005c47a0
W8CampSkillControls::~W8CampSkillControls()
{
    DestroyAllControls();
}

// FUNCTION: WIZ8 0x005c4800
void W8CampSkillControls::OnPrimary(W8TextControl* control)
{
    if (control == m_buttons[0]) {
        if (m_buttons[0]->m_stateFlags & g_W8TextControlMask005ED570) {
            m_buttons[1]->DisableSecondaryState(0);
            g_camp_screen_0069c0f4->skill_scroll = 1;
        } else {
            g_camp_screen_0069c0f4->skill_scroll = 0;
        }
    } else if (control == m_buttons[1]) {
        if (m_buttons[1]->m_stateFlags & g_W8TextControlMask005ED570) {
            m_buttons[0]->DisableSecondaryState(0);
            g_camp_screen_0069c0f4->skill_scroll = 2;
        } else {
            g_camp_screen_0069c0f4->skill_scroll = 0;
        }
    } else {
        g_camp_screen_0069c0f4->skill_flag =
            (m_buttons[2]->m_stateFlags & g_W8TextControlMask005ED570) != 0;
    }
    Function5C5240();
    g_camp_screen_0069c0f4->redraw_flags |= 0x20000;
}

W8CampItemRange::W8CampItemRange()
{
    m_range = new W8RangeControl(0x263, 0xc1, 0x275, 0x1a1, &g_camp_item_region_set_0069c108);
    m_range->SetEnabled(1);
    m_range->m_listener = this;
}

// FUNCTION: WIZ8 0x005a34c0
void W8CampItemRange::OnRangeChanged(W8RangeControl*)
{
    g_camp_screen_0069c0f4->item_scroll = m_range->m_value << 1;
    if (gfKeyState[0x11]) {
        g_camp_screen_0069c0f4->redraw_flags |= 0x0fffffff;
        return;
    }
    g_camp_screen_0069c0f4->item_redraw_flags |= 0x7fc00000;
}

// FUNCTION: WIZ8 0x005c4510
void W8CampItemRange::UpdateItems(unsigned char items_changed)
{
    if (items_changed != 0) {
        m_range->Invalidate(0);
    }
    m_range->Redraw();
}

// FUNCTION: WIZ8 0x005b7090
W8CampSpellRange::W8CampSpellRange(int realm)
{
    m_realm = realm;
    int x = (realm % 3) * 0xd5;
    int y = (realm / 3) * 0x8c;
    m_range = new W8RangeControl(x + 0xbb, y + 0xc3, x + 0xcd, y + 0x129,
                                 &g_camp_spell_region_sets_0069c40c[realm]);
    m_range->m_listener = this;
    m_range->SetEnabled(1);
}

// FUNCTION: WIZ8 0x005b7160
W8CampSpellRange::~W8CampSpellRange()
{
    delete m_range;
}

// FUNCTION: WIZ8 0x005b7180
void W8CampSpellRange::OnRangeChanged(W8RangeControl*)
{
    g_camp_screen_0069c0f4->learned_spells.scroll[m_realm] = m_range->m_value;
    g_camp_screen_0069c0f4->redraw_flags |= 0x200000 << m_realm;
}

// FUNCTION: WIZ8 0x005c4430
W8CampSkillRange::W8CampSkillRange()
{
    m_range = new W8RangeControl(0x264, 0xbe, 0x276, 0x1b5, &g_camp_skill_region_set_0069c51c);
    m_range->m_listener = this;
    m_range->SetEnabled(1);
}

// FUNCTION: WIZ8 0x005c44c0
W8CampSkillRange::~W8CampSkillRange()
{
    delete m_range;
}

// FUNCTION: WIZ8 0x005c44e0
void W8CampSkillRange::OnRangeChanged(W8RangeControl*)
{
    g_camp_screen_0069c0f4->skill_list_scroll = m_range->m_value;
    g_camp_screen_0069c0f4->redraw_flags |= 0x20000;
}

/* Lifecycle record 6's initializer - the camp record, which is what
   W8_SCREEN_CAMP selects. It drops the camp screen's state pointer rather than
   releasing it; the block is owned by the enter/leave pair. */
// FUNCTION: WIZ8 0x005a3500
unsigned char CampScreenInitialize(void)
{
    g_camp_screen_0069c0f4 = 0;
    CampScreenInitializeRegions();
    LayoutCampSecondaryRegions();
    return 1;
}

/* The four region blocks the camp screen lays out by rule, and the twelve it
   lays out from the explicit table below. Only the leading four fields of each
   record are read here, so the remaining five stay positional. */
// FUNCTION: WIZ8 0x005a4090
void CampScreenInitializeRegions(void)
{
    unsigned int index;
    for (index = 0; index < 8; ++index) {
        unsigned short x = static_cast<unsigned short>((index & 1) * 0x30 + 6);
        unsigned short y = static_cast<unsigned short>((index >> 1) * 0x27 + 5);
        SetRegionBounds(index + 0xea, x, y, x + 0x2d, y + 0x24);
    }
    for (index = 0; index < 8; ++index) {
        unsigned short x = static_cast<unsigned short>((index & 1) * 0x31 + 0xb);
        unsigned short y = static_cast<unsigned short>((index >> 1) * 0x39 + 0xc0);
        SetRegionBounds(index + 0xf4, x, y, x + 0x2e, y + 0x36);
    }
    for (index = 0; index < 12; ++index) {
        const W8CampScreenRegion& region = g_camp_screen_regions_64cbf0[index];
        SetRegionBounds(index + 0xfc, static_cast<unsigned short>(region.x),
                        static_cast<unsigned short>(region.y),
                        static_cast<unsigned short>(region.x + region.width),
                        static_cast<unsigned short>(region.y + region.height));
    }
    for (index = 0; index < 8; ++index) {
        unsigned short x = static_cast<unsigned short>((index & 1) * 0x31 + 0x200);
        unsigned short y = static_cast<unsigned short>((index >> 1) * 0x39 + 0xc0);
        SetRegionBounds(index + 0x108, x, y, x + 0x2e, y + 0x36);
    }
}

/* The camp screen's remaining six regions, three across and two down. The
   initializer calls this immediately after the block above; nothing else
   reaches it, and nothing here names what the six cells hold. */
// FUNCTION: WIZ8 0x005b7230
void LayoutCampSecondaryRegions(void)
{
    unsigned int index;
    for (index = 0; index < 6; ++index) {
        unsigned short x = static_cast<unsigned short>((index % 3) * 0xd5 + 0x1d);
        unsigned short y = static_cast<unsigned short>((index / 3) * 0x8c + 0xc3);
        SetRegionBounds(index + 0x119, x, y, x + 0x99, y + 0x65);
    }
}

// FUNCTION: WIZ8 0x005a3520
unsigned char CampScreenEnter(void)
{
    ClearPrimarySurface();
    g_value_0069c0f8 = static_cast<W8Character*>(g_current_screen_state.parameter_3);
    giReviewCharSlot = g_current_screen_state.parameter_2;
    unsigned char entry_mode;
    if (g_current_screen_state.parameter_4 == 0) {
        g_camp_entry_parameter_0069c0fc = 0;
        entry_mode = 0;
    } else {
        g_camp_entry_parameter_0069c0fc = g_current_screen_state.parameter_4;
        entry_mode = 2;
        SoundPlay("Data\\Spells\\Sounds\\GeneralMagic.wav", 0);
        SetTargetingMode(6);
    }
    g_flag_00685071 = 0;
    g_value_00685072 = 0;
    g_flag_00685076 = 0xff;
    g_value_00685077 = -1;
    if (!g_camp_screen_0069c0f4) {
        g_camp_screen_0069c0f4 =
            static_cast<W8CampScreenState0069C0F4*>(malloc(sizeof(W8CampScreenState0069C0F4)));
        if (!g_camp_screen_0069c0f4) {
            if (IsMessageBoxActive()) {
                Function5187E0();
            }
            BeginCombatRound();
            RequestScreenTransition();
            return 0;
        }
        memset(g_camp_screen_0069c0f4, 0, sizeof(W8CampScreenState0069C0F4));
    }
    SetClippingRegionAndImageWidth(0x500, 0, 0, 0x280, 0x1e0);
    g_camp_screen_0069c0f4->entry_mode = entry_mode;
    MSYS_Init();
    for (unsigned int realm = 0; realm < 6; ++realm) {
        g_camp_screen_0069c0f4->realm_flags[realm] = 0;
    }
    g_camp_screen_0069c0f4->item_timer_active = 0;
    g_camp_screen_0069c0f4->item_timer_expired = 0;
    Function5B4EB0();
    CreateCampActionPanel005B9070();
    CreateItemsTabPanel005B9350();
    CreateCampSecondaryPanel005B9900();
    CreateRcsLevelUpPanel();
    CreateRcsDismissPanel();
    g_camp_screen_0069c0f4->page = 0;
    g_camp_screen_0069c0f4->item_mode = 0;
    g_camp_screen_0069c0f4->item_range = new W8CampItemRange;
    for (int range_index = 0; range_index < 6; ++range_index) {
        g_camp_screen_0069c0f4->spell_ranges[range_index] = new W8CampSpellRange(range_index);
    }
    g_camp_screen_0069c0f4->skill_stack = 0;
    g_camp_screen_0069c0f4->skill_flag = 1;
    g_camp_screen_0069c0f4->skill_scroll = 0;
    g_camp_screen_0069c0f4->skill_range = new W8CampSkillRange;
    g_camp_screen_0069c0f4->skill_controls = new W8CampSkillControls;
    g_camp_screen_0069c0f4->character_info = new W8CampCharacterInfo;
    g_camp_screen_0069c0f4->flag_d50 = 0;
    Function5A45B0();
    g_camp_screen_0069c0f4->animation_timer = SetCountdownClock(50);
    for (unsigned int animation = 0; animation < 6; ++animation) {
        g_camp_screen_0069c0f4->animation_frames[animation] =
            Random(g_spell_realm_animations_00648c90[animation].frame_count);
    }
    if (gXStatus.fCombatMode && g_combat_state->flag_a50) {
        if (g_combat_state->flag_a51 == 1) {
            for (int slot = 0; slot < 8; ++slot) {
                if (g_status_685170.buffers.party_rows[slot].occupied &&
                    IsPartySlotEligible00524A10(slot) &&
                    g_status_685170.buffers.party_rows[slot].pending_action == 9) {
                    swprintf(g_camp_screen_0069c0f4->caption, L"%s %s",
                             g_status_685170.buffers.characters[slot].name,
                             gppStringList[0x2464 / 4]);
                    goto show_equip_message;
                }
            }
        } else {
            unsigned char count = 0;
            for (int slot = 0; slot < 8; ++slot) {
                if (g_status_685170.buffers.party_rows[slot].occupied &&
                    IsPartySlotEligible00524A10(slot) &&
                    g_status_685170.buffers.party_rows[slot].pending_action == 9) {
                    ++count;
                    if (count == 1) {
                        swprintf(g_camp_screen_0069c0f4->caption, L"%s",
                                 g_status_685170.buffers.characters[slot].name);
                    } else {
                        if (count == g_combat_state->flag_a51) {
                            wcscat(g_camp_screen_0069c0f4->caption, L" ");
                            wcscat(g_camp_screen_0069c0f4->caption,
                                   FormatWideString(gppStringList[0x2460 / 4],
                                                    g_status_685170.buffers.characters[slot].name));
                            goto show_equip_message;
                        }
                        wcscat(g_camp_screen_0069c0f4->caption, L", ");
                        wcscat(g_camp_screen_0069c0f4->caption,
                               g_status_685170.buffers.characters[slot].name);
                    }
                }
            }
        }
        srAssertFail("fFoundEquipChar",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\ReviewCharacterScreen.cpp", 0x16f,
                     0);
    show_equip_message:
        W8MessageDialogBase* dialog = static_cast<W8MessageDialogBase*>(CreateDialogByKind(1));
        dialog->SetClientExtent(250, 200);
        dialog->SetMessage(g_camp_screen_0069c0f4->caption, 1, 50, 1, 0, 1, 1, 0, 350);
        SetDialogDestroyCallback(dialog, 0);
        g_camp_screen_0069c0f4->dialog = dialog;
        ActivateDialogRegion(0x138);
    }
    g_camp_screen_0069c0f4->redraw_flags |= 0x0fffffff;
    ResetTransientRenderScenes();
    SetPrimarySurfaceTextureHint2Enabled(0);
    if (!g_status_685170.game_started) {
        StartMusicResource0048FC10("MainMenu.MPL", 1, 1);
    }
    return 1;
}

// FUNCTION: WIZ8 0x005a3ae0
void CampScreenFrame(void)
{
    if (g_flag_689b32) {
        RequestExitScreen();
    }
    if (IsMessageBoxActive()) {
        ProcessMessageBoxInput();
    }
    ServiceMusicPlaylist0048F9E0();
    if (g_camp_screen_0069c0f4->dialog && !ProcessDialogInput(g_camp_screen_0069c0f4->dialog)) {
        ClearActiveRegionIfMatches(0x138);
        delete g_camp_screen_0069c0f4->dialog;
        g_camp_screen_0069c0f4->dialog = 0;
        g_camp_screen_0069c0f4->redraw_flags |= 0x0fffffff;
    }
    if (gXStatus.unknown_026[1] && g_suspended_screen_id != W8_SCREEN_CHARACTER &&
        !gXStatus.fCombatMode && !IsScreenTransitionPending()) {
        RefreshLevelUpReadyNotices();
    }
    POINT point;
    SGPMouseGetPos(&point);
    g_camp_screen_0069c0f4->hover_region = UpdateRegionMousePosition(point.x, point.y);
    InputAtom input;
    while (DequeueEvent(&input) == 1) {
        if (!DispatchRegionInput(&input) && input.usEvent == KEY_DOWN) {
            if (input.usParam == ESC) {
                if (!g_camp_screen_0069c0f4->entry_mode || g_camp_screen_0069c0f4->page != 0) {
                    if (!g_camp_character_pending_0069c104) {
                        if (IsMessageBoxActive()) {
                            Function5187E0();
                        }
                        BeginCombatRound();
                        RequestScreenTransition();
                    } else {
                        SelectCampCharacter005B6B30(
                            CharacterPointerToPartySlot(g_camp_character_0069c100));
                        if (!IsPartySlotEligible00524A10(giReviewCharSlot)) {
                            wchar_t* text = FormatWideString(gppStringList[0x24c4 / 4],
                                                             g_camp_character_0069c100->name);
                            W8MessageDialogBase* dialog =
                                static_cast<W8MessageDialogBase*>(CreateDialogByKind(1));
                            dialog->SetClientExtent(250, 200);
                            dialog->SetMessage(text, 1, 50, 1, 0, 1, 1, 0, 350);
                            SetDialogDestroyCallback(dialog, 0);
                            g_camp_screen_0069c0f4->dialog = dialog;
                            ActivateDialogRegion(0x138);
                        } else {
                            QueueCharacterEvent(g_camp_character_0069c100, g_effect_005ee6ec, 0,
                                                g_effect_argument_005ed8cc,
                                                g_effect_argument_005ed914);
                        }
                    }
                } else {
                    SetCampItemActionMode005B59B0(0);
                }
            } else if (input.usParam == 'P') {
                if (g_status_685170.game_started) {
                    SortPartyItemPool();
                }
            } else if (input.usParam == 'X' && gfKeyState[0x12] && !gfKeyState[0x11] &&
                       !gfKeyState[0x10]) {
                W8MessageDialogBase* dialog =
                    static_cast<W8MessageDialogBase*>(CreateDialogByKind(1));
                dialog->SetClientExtent(250, 200);
                dialog->SetMessage(gppStringList[0x20c8 / 4], 1, 50, 1, 1, 1, 1, 0, 350);
                SetDialogDestroyCallback(dialog, OnQuitGameDialogClosed);
                g_camp_screen_0069c0f4->dialog = dialog;
                ActivateDialogRegion(0x138);
            }
        }
    }
    if (g_camp_screen_0069c0f4->page == 3 &&
        !ClockIsTicking(g_camp_screen_0069c0f4->animation_timer)) {
        for (unsigned int realm = 0; realm < 6; ++realm) {
            ++g_camp_screen_0069c0f4->animation_frames[realm];
            if (g_camp_screen_0069c0f4->animation_frames[realm] ==
                g_spell_realm_animations_00648c90[realm].frame_count) {
                g_camp_screen_0069c0f4->animation_frames[realm] = 0;
            }
        }
        g_camp_screen_0069c0f4->animation_timer = SetCountdownClock(50);
        g_camp_screen_0069c0f4->redraw_flags |= 0x10000;
    }
    if (g_camp_screen_0069c0f4->page == 0 && g_camp_screen_0069c0f4->item_timer_active &&
        !g_camp_screen_0069c0f4->item_timer_expired &&
        !ClockIsTicking(g_camp_screen_0069c0f4->item_timer)) {
        g_camp_screen_0069c0f4->item_timer_expired = 1;
        g_camp_screen_0069c0f4->item_redraw_flags |= 0x3ffe00;
    }
    if (!g_camp_screen_0069c0f4->input_mode) {
        gXStatus.character_event_queue->ProcessDeferredCharacterEvents();
        UpdateCharacterEventState();
    }
    Function5A42A0();
}

// FUNCTION: WIZ8 0x005a3ee0
unsigned char CampScreenLeave(int)
{
    Function5A4770();
    SetFontObjectPalette16BPP(g_smfnt_font_683694, g_font_palette_smfnt_68ee10);
    SetFontObjectPalette16BPP(g_calligraphy_font_6835f8, g_font_palette_calligraphy_68edfc);
    SetFontObjectPalette16BPP(g_calligraphy_shadow_font_6835f4,
                              g_font_palette_calligraphy_shadow_68ee18);
    SetFontObjectPalette16BPP(g_wiz_text_font_683640, g_font_palette_wiz_text_68ee14);
    Function5B55F0();
    ReleaseCampActionPanel005B9220();
    ReleaseItemsTabPanel005B9760();
    ReleaseCampSecondaryPanel();
    DestroyRcsLevelUpPanel();
    DestroyRcsDismissPanel();
    delete g_camp_screen_0069c0f4->item_range;
    if (g_camp_screen_0069c0f4->dialog) {
        ClearActiveRegionIfMatches(0x138);
        delete g_camp_screen_0069c0f4->dialog;
    }
    for (unsigned int realm = 0; realm < 6; ++realm) {
        delete g_camp_screen_0069c0f4->spell_ranges[realm];
    }
    delete g_camp_screen_0069c0f4->skill_range;
    delete g_camp_screen_0069c0f4->skill_controls;
    delete g_camp_screen_0069c0f4->character_info;
    free(g_camp_screen_0069c0f4);
    g_camp_screen_0069c0f4 = 0;
    MSYS_Shutdown();
    ResetRegions();
    if (gXStatus.fCampMode) {
        gXStatus.unknown_026[0] = 1;
    }
    return 1;
}

/* Leave camp. When an item is held out for the pending character, it goes to
   that character if the selected slot can carry it and the refusal dialog
   opens otherwise; with no pending character the camp simply closes into a
   fresh combat round. */
// FUNCTION: WIZ8 0x005a41b0
void DismissSelectedPartyCharacter(void)
{
    if (g_camp_character_pending_0069c104 != 0) {
        SelectCampCharacter005B6B30(CharacterPointerToPartySlot(g_camp_character_0069c100));
        if (IsPartySlotEligible00524A10(giReviewCharSlot) != 0) {
            QueueCharacterEvent(g_camp_character_0069c100, g_effect_005ee6ec, 0,
                                g_effect_argument_005ed8cc, g_effect_argument_005ed914);
            return;
        }
        wchar_t* text =
            FormatWideString(gppStringList[0x24c4 / 4], g_camp_character_0069c100->name);
        W8MessageDialogBase* dialog = static_cast<W8MessageDialogBase*>(CreateDialogByKind(1));
        dialog->SetClientExtent(250, 200);
        dialog->SetMessage(text, 1, 50, 1, 0, 1, 1, 0, 350);
        SetDialogDestroyCallback(dialog, 0);
        g_camp_screen_0069c0f4->dialog = dialog;
        ActivateDialogRegion(0x138);
        return;
    }
    if (IsMessageBoxActive()) {
        Function5187E0();
    }
    BeginCombatRound();
    RequestScreenTransition();
}

// FUNCTION: WIZ8 0x005A4570
void SyncReviewCharInputRegion005A4570(void)
{
    if (giReviewCharSlot != -1 &&
        g_status_685170.buffers.party_rows[giReviewCharSlot].animation_0fa != -1) {
        DisableRegionInput(0xf2);
        return;
    }
    EnableRegionInput(0xf2);
}

/* Kicked off by the post-quake camera-shake callback: latches the endgame
   flags, resets input regions, then starts the fade whose completion runs
   Function5A6B90 - the ending sequence picker. Fact 0x1a2 forces the long
   fade, fact 0x2f4 swaps the timing and marks the variant. */
/* Realm filters 2-5 are exclusive: selecting one clears the others. */
// FUNCTION: WIZ8 0x005a49d0
void ClearOtherRealmFilters005A49D0(unsigned int realm)
{
    unsigned int index;

    for (index = 2; index < 6; ++index) {
        if (index != realm) {
            g_camp_screen_0069c0f4->realm_flags[index] = 0;
        }
    }
}

/* Rebuild the visible item list from the party pool under the realm filters:
   flag 0 restricts to items the displayed character can use, flag 1 to
   unidentified items, and flags 2-5 each admit one equip-slot group. The
   scrollbar's range and value track the count; the scroll position snaps back
   inside an emptied or shrunken list. */
// FUNCTION: WIZ8 0x005a4a00
void RebuildCampItemList005A4A00(void)
{
    unsigned int index;
    unsigned char filter;
    W8ItemInstance* pool;
    W8RangeControl* scrollbar;
    int rows;

    g_camp_screen_0069c0f4->item_list_count = 0;
    filter = 0;
    for (index = 0; index < 6; ++index) {
        if (index != 0 && index != 1 && g_camp_screen_0069c0f4->realm_flags[index] != 0) {
            filter |= 1 << index;
        }
    }
    pool = g_status_685170.party_item_pool_0021;
    for (index = 0; index < static_cast<unsigned int>(g_status_685170.party_item_count_1791);
         ++index, ++pool) {
        if (pool->item_id != -1 &&
            (g_camp_screen_0069c0f4->realm_flags[0] == 0 ||
             CanCharacterUseItem(g_value_0069c0f8, pool->item_id) != 0) &&
            (g_camp_screen_0069c0f4->realm_flags[1] == 0 || pool->identified == 0) &&
            (filter == 0 || (filter & static_cast<unsigned char>(
                                          1 << GetItemEquipSlotGroup(pool->item_id))) != 0)) {
            g_camp_screen_0069c0f4->item_list_4ec[g_camp_screen_0069c0f4->item_list_count] = index;
            ++g_camp_screen_0069c0f4->item_list_count;
        }
    }
    scrollbar = g_camp_screen_0069c0f4->item_range->m_range;
    scrollbar->SetRangeEnabled(g_camp_screen_0069c0f4->item_list_count > 8);
    rows = g_camp_screen_0069c0f4->item_list_count - 8;
    if (rows > 0) {
        scrollbar->SetRange(0, rows % 2 == 0 ? rows / 2 : rows / 2 + 1);
    }
    if (g_camp_screen_0069c0f4->item_list_count == 0) {
        g_camp_screen_0069c0f4->item_scroll = 0;
    } else if (g_camp_screen_0069c0f4->item_list_count <= g_camp_screen_0069c0f4->item_scroll) {
        g_camp_screen_0069c0f4->item_scroll = (g_camp_screen_0069c0f4->item_list_count - 1) / 8 * 8;
    }
    if ((g_camp_screen_0069c0f4->item_scroll & 1) == 0) {
        scrollbar->SetValue(g_camp_screen_0069c0f4->item_scroll >> 1);
    } else {
        scrollbar->SetValue((g_camp_screen_0069c0f4->item_scroll >> 1) + 1);
    }
}

// FUNCTION: WIZ8 0x005a4bc0
void SetCampInputMode005A4BC0(int mode)
{
    g_camp_screen_0069c0f4->input_mode = mode;
    g_camp_screen_0069c0f4->redraw_flags |= 0x7ff;
}

// FUNCTION: WIZ8 0x005a4be0
void DisplayCampDialog(W8DialogBase* dialog)
{
    g_camp_screen_0069c0f4->dialog = dialog;
    ActivateDialogRegion(0x138);
}

// FUNCTION: WIZ8 0x005a4c00
void ShowCampNoticeLine(wchar_t* text, W8DialogDestroyCallback callback, int confirmation,
                        int cancel)
{
    W8MessageDialogBase* dialog = static_cast<W8MessageDialogBase*>(CreateDialogByKind(1));

    dialog->SetClientExtent(0xfa, 200);
    dialog->SetMessage(text, 1, 0x32, confirmation, cancel, 1, 1, 0, 0x15e);
    SetDialogDestroyCallback(dialog, callback);
    g_camp_screen_0069c0f4->dialog = dialog;
    ActivateDialogRegion(0x138);
}

/* The shared click handler for the camp item regions: a backpack slot arrives
   with origin 0, a worn slot with origin 1 and a party-pool row with origin
   2. The screen's entry_mode picks the interpretation - plain handling,
   identify, stack split, use, use held item on item - and the held item can
   merge onto the clicked stack, swap with it, or drop into the pool. */
// FUNCTION: WIZ8 0x005a4c70
void HandleCampItemClick005A4C70(W8ItemInstance* item, unsigned int slot_index, unsigned int origin)
{
    W8CombatSlot target;
    short related_kind;
    unsigned char changed = 0;
    unsigned char merged = 0;
    unsigned char partially_merged = 0;
    unsigned char merge_tried = 0;
    unsigned char same_kind = 0;
    unsigned char choose_character;
    unsigned int index;
    int old_pool_count;
    int result;
    int party_slot;
    int paired_slot;
    unsigned char reidentify;
    W8ItemInstance* paired;
    W8Character* character;
    W8NpcState* npc;
    W8MessageDialogBase* dialog;
    wchar_t* text;

    if (item == 0) {
        srAssertFail("pPCItem != NULL",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\ReviewCharacterScreen.cpp", 0x4e5,
                     0);
    }
    if (origin >= 3) {
        srAssertFail("uiSlotType < SLOT_TYPE_COUNT",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\ReviewCharacterScreen.cpp", 0x4e6,
                     0);
    }

    /* A held stackable whose name kind the clicked item merges with counts as
       the same item for the use-merge path below. */
    if (g_status_685170.item_in_cursor != 0 && item->item_id != -1 &&
        GetItemMergeKind0051E980(item->item_id, &related_kind) != 0 &&
        g_item_records[g_status_685170.item_in_hand_235b.item_id].unidentified_name_index ==
            related_kind) {
        same_kind = 1;
    }
    if (item->item_id == -1 && g_status_685170.item_in_cursor == 0) {
        return;
    }
    if (origin == 1 && g_status_685170.item_in_cursor != 0 &&
        g_camp_screen_0069c0f4->entry_mode != 1) {
        if (same_kind == 0 &&
            CanEquipItemInSlot(g_value_0069c0f8, g_status_685170.item_in_hand_235b.item_id,
                               slot_index, 1) == 0) {
            return;
        }
        if (CanCharacterUseItem(g_value_0069c0f8, g_status_685170.item_in_hand_235b.item_id) == 0) {
            return;
        }
    }
    if (g_camp_screen_0069c0f4->entry_mode == 7 || g_camp_screen_0069c0f4->entry_mode == 9) {
        return;
    }
    if (g_status_685170.game_started == 0) {
        text = gppStringList[0x2400 / 4];
        dialog = static_cast<W8MessageDialogBase*>(CreateDialogByKind(1));
        dialog->SetClientExtent(250, 200);
        dialog->SetMessage(text, 1, 50, 1, 0, 1, 1, 0, 350);
        SetDialogDestroyCallback(dialog, 0);
        g_camp_screen_0069c0f4->dialog = dialog;
        ActivateDialogRegion(0x138);
        return;
    }
    character = &g_status_685170.buffers.characters[giReviewCharSlot];
    if (character->condition_turns[0x13] != 0 && (origin == 1 || origin == 0)) {
        text = gppStringList[0x241c / 4];
        dialog = static_cast<W8MessageDialogBase*>(CreateDialogByKind(1));
        dialog->SetClientExtent(250, 200);
        dialog->SetMessage(text, 1, 50, 1, 0, 1, 1, 0, 350);
        SetDialogDestroyCallback(dialog, 0);
        g_camp_screen_0069c0f4->dialog = dialog;
        ActivateDialogRegion(0x138);
        return;
    }
    if (character->condition_turns[0xe] != 0 && (origin == 1 || origin == 0)) {
        text = gppStringList[0x2420 / 4];
        dialog = static_cast<W8MessageDialogBase*>(CreateDialogByKind(1));
        dialog->SetClientExtent(250, 200);
        dialog->SetMessage(text, 1, 50, 1, 0, 1, 1, 0, 350);
        SetDialogDestroyCallback(dialog, 0);
        g_camp_screen_0069c0f4->dialog = dialog;
        ActivateDialogRegion(0x138);
        return;
    }
    if (character->condition_turns[W8_CONDITION_HOSTILE] != 0 && (origin == 1 || origin == 0)) {
        text = gppStringList[0x2424 / 4];
        dialog = static_cast<W8MessageDialogBase*>(CreateDialogByKind(1));
        dialog->SetClientExtent(250, 200);
        dialog->SetMessage(text, 1, 50, 1, 0, 1, 1, 0, 350);
        SetDialogDestroyCallback(dialog, 0);
        g_camp_screen_0069c0f4->dialog = dialog;
        ActivateDialogRegion(0x138);
        return;
    }
    if (character->highest_condition >= W8_CONDITION_HOSTILE && origin != 2 &&
        g_status_685170.item_in_cursor != 0 && g_held_item_source_006840c0 != giReviewCharSlot) {
        text = gppStringList[0x2404 / 4];
        dialog = static_cast<W8MessageDialogBase*>(CreateDialogByKind(1));
        dialog->SetClientExtent(250, 200);
        dialog->SetMessage(text, 1, 50, 1, 0, 1, 1, 0, 350);
        SetDialogDestroyCallback(dialog, 0);
        g_camp_screen_0069c0f4->dialog = dialog;
        ActivateDialogRegion(0x138);
        return;
    }

    /* In combat an item click spends the character's action allowance unless
       it only touches a weapon-class slot or folds into the same-kind merge.
       A weapon or shield being moved onto a slot it cannot pair with stays
       gated as well. */
    unsigned char gated = 1;
    if (gXStatus.fCombatMode != 0 && origin != 2) {
        if (g_status_685170.item_in_cursor == 0 ||
            ((g_item_records[g_status_685170.item_in_hand_235b.item_id].equip_class == 2 ||
              g_item_records[g_status_685170.item_in_hand_235b.item_id].equip_class == 4) &&
             g_held_item_source_006840c0 == giReviewCharSlot &&
             (origin != 1 || HeldItemFitsPairedSlot0051CDE0(giReviewCharSlot, slot_index) != 0))) {
            if (item->item_id == -1 || g_item_records[item->item_id].equip_class == 2 ||
                g_item_records[item->item_id].equip_class == 4 || same_kind != 0) {
                gated = 0;
            }
        }
    }
    if (gated != 0 && Function5A6090(giReviewCharSlot) == 0) {
        return;
    }
    if (gfKeyState[0x10] != 0) {
        TakeItemUnitToHand005A5DA0(item, slot_index, origin);
        return;
    }
    if (g_camp_screen_0069c0f4->entry_mode == 2) {
        party_slot = CharacterPointerToPartySlot(g_camp_entry_parameter_0069c0fc);
        if (CanItemLeaveItsSlot(item) == 0) {
            QueueCharacterEvent(&g_status_685170.buffers.characters[party_slot],
                                g_character_event_kind_005ee65c, 0, g_effect_argument_005ed8c8,
                                g_effect_argument_005ed914);
            return;
        }
        if (g_camp_entry_parameter_0069c0fc == 0) {
            srAssertFail("gpIdentifyingPC != NULL",
                         "C:\\Projects\\Wizardry 8\\Local Screens\\ReviewCharacterScreen.cpp",
                         0x553, 0);
        }
        old_pool_count = g_status_685170.party_item_count_1791;
        reidentify = 0;
        ResetCombatSlot(&target);
        target.iType = W8_TARGET_KIND_ITEM;
        target.pPCItem = item;
        StartBreathCycle(party_slot, 0);
        W8PartySlotRow* row = &g_status_685170.buffers.party_rows[party_slot];
        if (row->pending_action == 8 &&
            g_item_records[row->pending_action_detail_015.item_use.item->item_id].spell_id ==
                0x17) {
            reidentify = 1;
            result =
                Function5A6440(party_slot, row->pending_action_detail_015.item_use.item, &target);
        } else {
            result = Function5A6340(party_slot, 0x17, 8, &target);
        }
        if (origin == 2 && reidentify != 0 &&
            old_pool_count != g_status_685170.party_item_count_1791) {
            item = &g_status_685170.party_item_pool_0021[g_status_685170.party_item_count_1791 -
                                                         old_pool_count + slot_index];
            RebuildCampItemList005A4A00();
            RecalculateCarriedWeight(g_value_0069c0f8);
            RedistributePartyEncumbrance();
            g_camp_screen_0069c0f4->redraw_flags |= 0x2000;
            g_camp_screen_0069c0f4->item_redraw_flags |= 0x7fc00000;
        }
        if (result == 1 && item->item_id != -1) {
            OpenItemInfoDialog005BA110(item, 0);
            if (item->identified == 0) {
                QueueCharacterEvent(&g_status_685170.buffers.characters[party_slot],
                                    g_character_event_kind_005ee65c, 0, g_effect_argument_005ed8c8,
                                    g_effect_argument_005ed914);
            }
        }
        SetCampItemActionMode005B59B0(0);
        return;
    }
    if (g_camp_screen_0069c0f4->entry_mode == 8) {
        UseHeldItemOnItem005BA740(item);
        return;
    }
    if (g_camp_screen_0069c0f4->entry_mode == 3) {
        IdentifyAndOpenItemInfo005BA370(item);
        return;
    }
    if (g_camp_screen_0069c0f4->entry_mode == 4) {
        if (item->item_id != -1) {
            OpenSplitStackDialog005BA400(item);
            return;
        }
    } else if (g_camp_screen_0069c0f4->entry_mode == 5) {
        if (CanCharacterUseItemEntry005BAA10(g_value_0069c0f8, item) != 0) {
            UseItem005BA4F0(item);
        }
        return;
    } else if (g_camp_screen_0069c0f4->entry_mode == 1) {
        if (g_status_685170.item_in_cursor != 0) {
            if (item->item_id == -1) {
                SetCampItemActionMode005B59B0(0);
                return;
            }
            if (g_camp_character_pending_0069c104 != 0) {
                SelectCampCharacter005B6B30(CharacterPointerToPartySlot(g_camp_character_0069c100));
                if (IsPartySlotEligible00524A10(giReviewCharSlot) != 0) {
                    QueueCharacterEvent(g_camp_character_0069c100, g_effect_005ee6ec, 0,
                                        g_effect_argument_005ed8cc, g_effect_argument_005ed914);
                } else {
                    text = FormatWideString(gppStringList[0x24c4 / 4],
                                            g_camp_character_0069c100->name);
                    dialog = static_cast<W8MessageDialogBase*>(CreateDialogByKind(1));
                    dialog->SetClientExtent(250, 200);
                    dialog->SetMessage(text, 1, 50, 1, 0, 1, 1, 0, 350);
                    SetDialogDestroyCallback(dialog, 0);
                    g_camp_screen_0069c0f4->dialog = dialog;
                    ActivateDialogRegion(0x138);
                }
                return;
            }
            MergeItemStacksWithHeld005BA5D0(item);
            return;
        }
        if (item->item_id == -1) {
            SetCampItemActionMode005B59B0(0);
            return;
        }
    }

    /* Equipped items that may not be removed get one warning dialog and a
       bound mark; the next click then unequips them. */
    if (item->item_id != -1 && origin == 1) {
        if (CanUnequipSlotItem(g_value_0069c0f8, slot_index) == 0) {
            text = gppStringList[0x242c / 4];
            dialog = static_cast<W8MessageDialogBase*>(CreateDialogByKind(1));
            dialog->SetClientExtent(250, 200);
            dialog->SetMessage(text, 1, 50, 1, 0, 1, 1, 0, 350);
            SetDialogDestroyCallback(dialog, 0);
            g_camp_screen_0069c0f4->dialog = dialog;
            ActivateDialogRegion(0x138);
            if (item->bound != 0) {
                return;
            }
            BindEquippedItem(g_value_0069c0f8, slot_index);
            return;
        }
        item->bound = 1;
    }

    if (same_kind != 0) {
        MergeItemUses(&g_status_685170.buffers.characters[giReviewCharSlot], item,
                      &g_status_685170.item_in_hand_235b);
        SetCampItemActionMode005B59B0(0);
        changed = 1;
        if (origin == 1) {
            RebuildEquipmentAndDerivedStatsForSlot(giReviewCharSlot);
            if (GetPairedEquipSlot(slot_index) != -1) {
                g_status_685170.buffers.party_rows[giReviewCharSlot].flag_105 = 0;
            }
            RebuildCampItemList005A4A00();
        } else if (origin == 0) {
            RecalculateCharacterDerivedStats(&g_status_685170.buffers.characters[giReviewCharSlot]);
        } else {
            RebuildCampItemList005A4A00();
        }
    } else {
        /* Fold the held stack onto a matching stack; for the pool the scan
           continues across every row until the held stack is consumed. */
        if (g_status_685170.item_in_cursor != 0 &&
            g_item_records[g_status_685170.item_in_hand_235b.item_id].quantity_kind == 1) {
            if (item->item_id != -1 && item->item_id == g_status_685170.item_in_hand_235b.item_id) {
                merged =
                    MergeItemStacks(item, &g_status_685170.item_in_hand_235b, &partially_merged);
                merge_tried = 1;
            }
            if (merged == 0 && origin == 2) {
                for (index = 0;
                     index < static_cast<unsigned int>(g_status_685170.party_item_count_1791);
                     ++index) {
                    if (MergeItemStacks(&g_status_685170.party_item_pool_0021[index],
                                        &g_status_685170.item_in_hand_235b,
                                        &partially_merged) != 0) {
                        merged = 1;
                        break;
                    }
                }
            }
        }
        if (origin == 2) {
            if (merged == 0) {
                if (g_status_685170.item_in_cursor != 0) {
                    if (g_camp_character_pending_0069c104 != 0) {
                        SelectCampCharacter005B6B30(
                            CharacterPointerToPartySlot(g_camp_character_0069c100));
                        if (IsPartySlotEligible00524A10(giReviewCharSlot) != 0) {
                            QueueCharacterEvent(g_camp_character_0069c100, g_effect_005ee6ec, 0,
                                                g_effect_argument_005ed8cc,
                                                g_effect_argument_005ed914);
                        } else {
                            text = FormatWideString(gppStringList[0x24c4 / 4],
                                                    g_camp_character_0069c100->name);
                            dialog = static_cast<W8MessageDialogBase*>(CreateDialogByKind(1));
                            dialog->SetClientExtent(250, 200);
                            dialog->SetMessage(text, 1, 50, 1, 0, 1, 1, 0, 350);
                            SetDialogDestroyCallback(dialog, 0);
                            g_camp_screen_0069c0f4->dialog = dialog;
                            ActivateDialogRegion(0x138);
                        }
                    } else if (InsertItemIntoPartyPool00521E20(&g_status_685170.item_in_hand_235b,
                                                               slot_index) != 0) {
                        changed = 1;
                        RebuildCampItemList005A4A00();
                    } else {
                        text = gppStringList[0x2430 / 4];
                        dialog = static_cast<W8MessageDialogBase*>(CreateDialogByKind(1));
                        dialog->SetClientExtent(250, 200);
                        dialog->SetMessage(text, 1, 50, 1, 0, 1, 1, 0, 350);
                        SetDialogDestroyCallback(dialog, 0);
                        g_camp_screen_0069c0f4->dialog = dialog;
                        ActivateDialogRegion(0x138);
                    }
                } else if (slot_index <
                           static_cast<unsigned int>(g_status_685170.party_item_count_1791)) {
                    /* An empty hand picks the clicked pool row up. */
                    CopyItemInstance(&g_status_685170.item_in_hand_235b, item, 0, 1);
                    changed = 1;
                    if (g_camp_screen_0069c0f4->entry_mode == 6) {
                        DropHeldItem005BA3D0();
                        RebuildCampItemList005A4A00();
                    } else if (g_camp_screen_0069c0f4->entry_mode == 1) {
                        SetHandCursors005BAFC0(0);
                        RebuildCampItemList005A4A00();
                    } else {
                        RebuildCampItemList005A4A00();
                    }
                }
            }
        } else {
            if (merge_tried == 0 && ((g_status_685170.item_in_cursor == 0 && item->item_id != -1) ||
                                     (g_status_685170.item_in_cursor != 0 && merged == 0))) {
                if (g_camp_character_pending_0069c104 == 0 ||
                    g_camp_character_0069c100 == g_value_0069c0f8) {
                    g_camp_character_pending_0069c104 = 0;
                    if (item->item_id != -1 && (giReviewCharSlot == 0 || giReviewCharSlot == 1)) {
                        npc = GetNpcState(
                            g_status_685170.buffers.party_rows[giReviewCharSlot].animation_0fa);
                        if (npc != 0 && NpcWantsItem0050DC50(npc, item) != 0) {
                            g_camp_character_pending_0069c104 = 1;
                            g_camp_character_0069c100 = g_value_0069c0f8;
                        }
                    }
                    if (g_status_685170.item_in_cursor != 0) {
                        choose_character = g_held_item_source_006840c0 == -1;
                        DeliverExceptionalItemReaction(&g_status_685170.item_in_hand_235b,
                                                       choose_character, g_value_0069c0f8);
                    }
                    if (origin == 1 && g_status_685170.item_in_cursor != 0 &&
                        HeldItemFitsPairedSlot0051CDE0(giReviewCharSlot, slot_index) == 0) {
                        paired_slot = GetPairedEquipSlot(slot_index);
                        paired = &g_value_0069c0f8->equipment[paired_slot];
                        if (CanUnequipSlotItem(g_value_0069c0f8, paired_slot) == 0) {
                            text = gppStringList[0x2458 / 4];
                            dialog = static_cast<W8MessageDialogBase*>(CreateDialogByKind(1));
                            dialog->SetClientExtent(250, 200);
                            dialog->SetMessage(text, 1, 50, 1, 0, 1, 1, 0, 350);
                            SetDialogDestroyCallback(dialog, 0);
                            g_camp_screen_0069c0f4->dialog = dialog;
                            ActivateDialogRegion(0x138);
                            if (paired->bound != 0) {
                                return;
                            }
                            BindEquippedItem(g_value_0069c0f8, paired_slot);
                            return;
                        }
                        paired->bound = 1;
                        if (item->item_id == -1) {
                            SwapItemInstances(item, &g_status_685170.item_in_hand_235b,
                                              g_value_0069c0f8, 1);
                            g_camp_character_pending_0069c104 = 0;
                            if (paired->item_id != -1 &&
                                (giReviewCharSlot == 0 || giReviewCharSlot == 1)) {
                                npc =
                                    GetNpcState(g_status_685170.buffers.party_rows[giReviewCharSlot]
                                                    .animation_0fa);
                                if (npc != 0 && NpcWantsItem0050DC50(npc, paired) != 0) {
                                    g_camp_character_pending_0069c104 = 1;
                                    g_camp_character_0069c100 = g_value_0069c0f8;
                                }
                            }
                            SwapItemInstances(paired, &g_status_685170.item_in_hand_235b,
                                              g_value_0069c0f8, 1);
                            changed = 1;
                        } else if (AddItemToCharacter(g_value_0069c0f8, paired, 0, 0, 1) != 0) {
                            SwapItemInstances(item, &g_status_685170.item_in_hand_235b,
                                              g_value_0069c0f8, 1);
                            changed = 1;
                        } else {
                            if (giReviewCharSlot == 0 || giReviewCharSlot == 1) {
                                if (NpcWantsItem0050DC50(
                                        GetNpcState(
                                            g_status_685170.buffers.party_rows[giReviewCharSlot]
                                                .animation_0fa),
                                        paired) != 0) {
                                    text = gppStringList[0x2430 / 4];
                                    dialog =
                                        static_cast<W8MessageDialogBase*>(CreateDialogByKind(1));
                                    dialog->SetClientExtent(250, 200);
                                    dialog->SetMessage(text, 1, 50, 1, 0, 1, 1, 0, 350);
                                    SetDialogDestroyCallback(dialog, 0);
                                    g_camp_screen_0069c0f4->dialog = dialog;
                                    ActivateDialogRegion(0x138);
                                    return;
                                }
                            }
                            if (AddItemToParty(paired, 0, 1) == 0) {
                                text = gppStringList[0x2430 / 4];
                                dialog = static_cast<W8MessageDialogBase*>(CreateDialogByKind(1));
                                dialog->SetClientExtent(250, 200);
                                dialog->SetMessage(text, 1, 50, 1, 0, 1, 1, 0, 350);
                                SetDialogDestroyCallback(dialog, 0);
                                g_camp_screen_0069c0f4->dialog = dialog;
                                ActivateDialogRegion(0x138);
                                return;
                            }
                            SwapItemInstances(item, &g_status_685170.item_in_hand_235b,
                                              g_value_0069c0f8, 1);
                            changed = 1;
                        }
                    } else {
                        SwapItemInstances(item, &g_status_685170.item_in_hand_235b,
                                          g_value_0069c0f8, 1);
                        changed = 1;
                        if (origin == 0 && g_value_0069c0f8->backpack[slot_index].item_id != -1 &&
                            Random(100) < 5) {
                            QueueCharacterEvent(g_value_0069c0f8, g_special_event_0068c55c, 0,
                                                g_effect_argument_005ed8c8,
                                                g_effect_argument_005ed914);
                        }
                    }
                } else {
                    SelectCampCharacter005B6B30(
                        CharacterPointerToPartySlot(g_camp_character_0069c100));
                    if (IsPartySlotEligible00524A10(giReviewCharSlot) != 0) {
                        QueueCharacterEvent(g_camp_character_0069c100, g_effect_005ee6ec, 0,
                                            g_effect_argument_005ed8cc, g_effect_argument_005ed914);
                    } else {
                        text = FormatWideString(gppStringList[0x24c4 / 4],
                                                g_camp_character_0069c100->name);
                        dialog = static_cast<W8MessageDialogBase*>(CreateDialogByKind(1));
                        dialog->SetClientExtent(250, 200);
                        dialog->SetMessage(text, 1, 50, 1, 0, 1, 1, 0, 350);
                        SetDialogDestroyCallback(dialog, 0);
                        g_camp_screen_0069c0f4->dialog = dialog;
                        ActivateDialogRegion(0x138);
                    }
                }
            }
            if (g_camp_screen_0069c0f4->entry_mode == 6 && g_status_685170.item_in_cursor != 0) {
                DropHeldItem005BA3D0();
            } else if (g_camp_screen_0069c0f4->entry_mode == 1 &&
                       g_status_685170.item_in_cursor != 0) {
                SetHandCursors005BAFC0(0);
            }
            if (changed != 0) {
                if (origin == 1) {
                    RebuildEquipmentAndDerivedStatsForSlot(giReviewCharSlot);
                    if (GetPairedEquipSlot(slot_index) != -1) {
                        g_status_685170.buffers.party_rows[giReviewCharSlot].flag_105 = 0;
                    }
                    RebuildCampItemList005A4A00();
                } else if (origin == 0) {
                    RecalculateCharacterDerivedStats(
                        &g_status_685170.buffers.characters[giReviewCharSlot]);
                } else {
                    RebuildCampItemList005A4A00();
                }
            }
        }
    }
    if (changed == 0 && partially_merged == 0 && merged == 0) {
        if (same_kind != 0) {
            g_camp_screen_0069c0f4->redraw_flags |= 0x1000;
        }
        return;
    }
    g_camp_screen_0069c0f4->redraw_flags |= 0x1000;
    if (gfKeyState[0x11] != 0) {
        g_camp_screen_0069c0f4->redraw_flags |= 0x0fffffff;
    } else {
        if (origin == 0) {
            if (partially_merged != 0 || merged != 0) {
                g_camp_screen_0069c0f4->item_redraw_flags |= 0x1ff;
            } else {
                g_camp_screen_0069c0f4->item_redraw_flags |= 2 << slot_index;
            }
        } else if (origin == 1) {
            if (partially_merged != 0 || merged != 0) {
                g_camp_screen_0069c0f4->item_redraw_flags |= 0x400 << slot_index;
            } else {
                g_camp_screen_0069c0f4->redraw_flags |= 0x2000;
                g_camp_screen_0069c0f4->redraw_flags |= 0x100;
            }
        } else if (origin == 2) {
            g_camp_screen_0069c0f4->item_redraw_flags |= 0x7fc00000;
        }
        g_camp_screen_0069c0f4->item_redraw_flags |= 0x3ffc00;
    }
    RecalculateCarriedWeight(g_value_0069c0f8);
    RedistributePartyEncumbrance();
    g_camp_screen_0069c0f4->redraw_flags |= 0x2000;
    g_camp_screen_0069c0f4->redraw_flags |= 0x100;
}

// FUNCTION: WIZ8 0x005a5da0
void TakeItemUnitToHand005A5DA0(W8ItemInstance* item, unsigned short slot, unsigned int origin)
{
    W8ItemInstance single;
    unsigned char moved = 0;

    if (g_camp_screen_0069c0f4->entry_mode != 0) {
        return;
    }
    if (item->item_id == -1) {
        return;
    }
    if (CanSplitItemStack005BAA50(item) == 0) {
        return;
    }
    if (g_status_685170.item_in_cursor == 0) {
        single = *item;
        single.stack_count = 1;
        --item->stack_count;
        if (origin == 1 || origin == 0) {
            CopyItemInstance(&g_status_685170.item_in_hand_235b, &single,
                             &g_status_685170.buffers.characters[giReviewCharSlot], 1);
            g_held_item_origin_006840c4 = origin;
            g_held_item_slot_006840c5 = slot;
        } else {
            CopyItemInstance(&g_status_685170.item_in_hand_235b, &single, 0, 1);
        }
        moved = 1;
    } else if (item->item_id == g_status_685170.item_in_hand_235b.item_id &&
               g_status_685170.item_in_hand_235b.stack_count <
                   g_item_records[g_status_685170.item_in_hand_235b.item_id].maximum_quantity) {
        ++g_status_685170.item_in_hand_235b.stack_count;
        --item->stack_count;
        moved = 1;
    }
    if (item->stack_count == 0) {
        EmptyItemRecord(item, g_value_0069c0f8, 1);
    } else if (moved == 0) {
        return;
    }
    RebuildEquipmentAndDerivedStatsForSlot(giReviewCharSlot);
    RebuildCampItemList005A4A00();
    RecalculateCharacterDerivedStats(&g_status_685170.buffers.characters[giReviewCharSlot]);
    RecalculateCarriedWeight(g_value_0069c0f8);
    RedistributePartyEncumbrance();
    g_camp_screen_0069c0f4->redraw_flags |= 0x0fffffff;
}

// FUNCTION: WIZ8 0x005A6310
char IsEquippableItemClass005A6310(W8ItemInstance* item)
{
    char equip_class = g_item_records[item->item_id].equip_class;
    if (equip_class != 2 && equip_class != 4) {
        return 0;
    }
    return 1;
}

// FUNCTION: WIZ8 0x005A6580
void BeginEndgameSequence005A6580(void)
{
    int fade_to_black = 0;
    int fade_code = 0x5dc;
    int endgame_variant = 0;

    g_status_685170.flag_49c0 = 1;
    UpdateHeldItemCursor();
    if (GetFact(0x1a2) != 0) {
        fade_to_black = 1;
    } else if (GetFact(0x2f4) != 0) {
        fade_code = 1;
        endgame_variant = 1;
    }
    MSYS_Init();
    ResetRegions();
    ActivateDialogRegion(0x138);
    VideoRemoveToolTip();
    g_level_block->transition_pending = 1;
    g_level_block->review_transition_active = 1;
    BeginScreenFade(fade_to_black, 0, fade_code, Function5A6B90, 1, endgame_variant);
}
