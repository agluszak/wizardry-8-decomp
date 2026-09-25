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
#include "wiz8/local_screens/PleaseWaitScreen.h"
#include "wiz8/local_screens/PartySelectionScreen.h"
#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/local_screens/MainMenuScreen.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "surrender/srMeshModel.h"
#include "surrender/srMaterial.h"
#include "surrender/srShader.h"
#include "wiz8/sound_man.h"
#include "wiz8/regions.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/learned_spells.h"
#include "wiz8/character_event_queue.h"
#include "wiz8/xstatus.h"
#include "wiz8/float_constants.h"
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/local_code/GameplayTime.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_screens/MainGameScreen.h"
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
W8CampScreenState* g_camp_screen_0069c0f4;
// GLOBAL: WIZ8 0x0064cbe8
int giReviewCharSlot = -1;
// GLOBAL: WIZ8 0x0069c0f8
W8Character* g_review_character_0069c0f8;
// GLOBAL: WIZ8 0x0069c0fc
W8Character* g_camp_entry_parameter_0069c0fc; /* gpIdentifyingPC */
// GLOBAL: WIZ8 0x0069c100
W8Character* g_camp_character_0069c100;
// GLOBAL: WIZ8 0x0069c104
bool g_camp_character_pending_0069c104;
// GLOBAL: WIZ8 0x0069c108
unsigned int g_camp_item_region_set_0069c108;
// GLOBAL: WIZ8 0x0069c10c
unsigned long g_fade_tick_base_0069c10c;
// GLOBAL: WIZ8 0x0069c110
void (*g_fade_callback_0069c110)(void);
// GLOBAL: WIZ8 0x0069c114
unsigned char g_fade_flag_0069c114;
// GLOBAL: WIZ8 0x0069c118
unsigned int g_fade_duration_0069c118;
// GLOBAL: WIZ8 0x0069c11c
stModelInstance2D* g_fade_overlay_0069c11c;
// GLOBAL: WIZ8 0x0069c120
int g_fade_out_0069c120;
// GLOBAL: WIZ8 0x0069c124
unsigned int g_ending_sound_0069c124;
// GLOBAL: WIZ8 0x0069c128
unsigned char g_ending_screen_0069c128;
// GLOBAL: WIZ8 0x0069c129
unsigned char g_ending_autosave_0069c129;
// GLOBAL: WIZ8 0x0069c40c
unsigned int g_camp_spell_region_sets_0069c40c[6];
// GLOBAL: WIZ8 0x0069c408
unsigned int g_camp_character_info_region_set;

// GLOBAL: WIZ8 0x005ee6ec
int g_effect_005ee6ec = 109;

// GLOBAL: WIZ8 0x005ed8cc
int g_effect_argument_005ed8cc = 1;

int CreateCampActionPanel(void);
int CreateItemsTabPanel(void);
int CreateCampSecondaryPanel(void);
void DrawCampCharacterInfo(void);
void DrawCampBackpackItems(void);
void DrawCampEquipmentItems(void);
void DrawCampItemPool(void);
void DrawCampItemQuantity(W8ItemInstance* item, int left, int top, int width);
void ActivateCampPage(void);
void DeactivateCampPage(void);
void DrawCampScreen(void);
void DrawCampRegenStats(void);
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
void ReleaseCampActionPanel(void)
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
void ReleaseItemsTabPanel(void)
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
void InvalidateCampPanel(void)
{
    g_camp_secondary_panel_0069c428->Invalidate(0);
}

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
class W8CampInfoLabel : public W8TextControl {
public:
    W8CampInfoLabel(Controls* panel, unsigned int region, int left, int top, int right, int bottom,
                    int text_40, int text_44, int text_48, int text_4c, int text_54, int text_50,
                    int text_58)
        : W8TextControl(panel, region, left, top, right, bottom, text_40, text_44, text_48, text_4c,
                        text_54, text_50, text_58)
    {
    }
    // SYNTHETIC: WIZ8 0x005b7c20
    // W8CampInfoLabel::`scalar deleting destructor'
    // FUNCTION: WIZ8 0x005b7c40
    virtual ~W8CampInfoLabel() override {}
    virtual void OnMouseEnter(int event) override;
    virtual void OnLeftButtonDown(int event) override;
    virtual void OnLeftButtonUp(int event) override;
    virtual void OnLeftButtonDoubleClick(int event) override;
};

// FUNCTION: WIZ8 0x005b7c90
void W8CampInfoLabel::OnMouseEnter(int event)
{
    PushButtonSoundScheme(0, 1);
    W8TextControl::OnMouseEnter(event);
}

/* Identical body to W8HelpTextControl::OnLeftButtonDown; ICF folds it to
   0x005B7CB0. */
void W8CampInfoLabel::OnLeftButtonDown(int event)
{
    PushButtonSoundScheme(0, 1);
    W8TextControl::OnLeftButtonDown(event);
}

/* Identical body to W8HelpTextControl::OnLeftButtonUp; ICF folds it to
   0x005B7CD0. */
void W8CampInfoLabel::OnLeftButtonUp(int event)
{
    PushButtonSoundScheme(0, 1);
    W8TextControl::OnLeftButtonUp(event);
}

// FUNCTION: WIZ8 0x005b7cf0
void W8CampInfoLabel::OnLeftButtonDoubleClick(int event)
{
    PushButtonSoundScheme(0, 1);
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
void SetCampSpellRangesEnabled(unsigned char enable)
{
    W8CampSpellRange* spell_range;
    int realm;
    int second;

    for (realm = 0; realm < 6; ++realm) {
        spell_range = g_camp_screen_0069c0f4->spell_ranges[realm];
        spell_range->m_range->EnableRegionSet(enable);
        if (enable != 0) {
            second = g_review_character_0069c0f8->skill_unlocks[0x1c + spell_range->m_realm] - 8;
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
void RefreshCampSpellRanges(void)
{
    W8CampSpellRange* spell_range;
    int realm;
    int second;

    for (realm = 0; realm < 6; ++realm) {
        spell_range = g_camp_screen_0069c0f4->spell_ranges[realm];
        spell_range->m_range->EnableRegionSet(1);
        second = g_review_character_0069c0f8->skill_unlocks[0x1c + spell_range->m_realm] - 8;
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
void DrawCampSpellPages(void)
{
    W8CampScreenState* state;
    W8Character* character = g_review_character_0069c0f8;
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
        DrawCampResistances();
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
            gprintf(left + 0x1b, top + 8, const_cast<wchar_t*>(g_format_s_0064dd28),
                    gppStringList[0x231c / 4]);
            gprintf(left + 0x72, top + 8, gppStringList[0x2324 / 4]);
            SetFontObjectPalette16BPP(g_font_683660, g_colour_68ee08);
            width = StringPixLengthArg(g_font_683660, wcslen(gppStringList[0x231c / 4]) + 2,
                                       const_cast<UINT16*>(g_format_s_colon_00648164),
                                       gppStringList[0x231c / 4]);
            gprintf(left + 0x1b + width, top + 8, const_cast<wchar_t*>(g_format_d_0064dd20),
                    character->skills[0x1c + realm].level);
            width = StringPixLengthArg(
                g_font_683660, 7, const_cast<UINT16*>(g_format_d_slash_d_00614b58),
                GetCharacterRealmSpellPoints(character, realm), character->sp_max[realm]);
            gprintf(left + 0xc8 - width, top + 8, const_cast<wchar_t*>(g_format_d_slash_d_00614b58),
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
                               character->iSPLeft[realm] &&
                           SpellUsableNow(spell_id, 0)) {
                    palette = g_colour_68ee08;
                } else {
                    palette = g_font_state_palettes_68ee1c[0];
                }
                SetFontObjectPalette16BPP(g_font_683660, palette);
                width =
                    StringPixLengthArg(g_font_683660, 3, const_cast<UINT16*>(g_format_3d_0064dd14),
                                       g_spell_records[spell_id].spell_point_cost);
                gprintf(left + 0x1c, row_top, const_cast<wchar_t*>(g_format_s_006068e4),
                        g_spell_records[spell_id].display_name);
                gprintf(left + 0xb1 - width, row_top, const_cast<wchar_t*>(g_format_d_0064dd20),
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
void DrawCampResistances(void)
{
    SGPRect saved_clip;
    SGPRect clip;
    W8Character* character = g_review_character_0069c0f8;
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
    gprintf((0x134 - width) / 2 + 0x144, 0x1e, text);
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
        gprintf(left + 0x93 - width, top + 3, const_cast<wchar_t*>(g_format_d_0060aa20),
                character->resistances[index].total);
    }
}

/* The spell-list region callback: the callback id is the realm index, the
   event's cursor position picks the highlighted row within the eight visible
   ones, wheel input scrolls the realm's range control, and activating a valid
   row opens its spell info dialog. */
// FUNCTION: WIZ8 0x005B79F0
unsigned char SpellListRegionHandler(const InputAtom* event, W8Region* region)
{
    unsigned short realm;
    int row;
    unsigned int visible;
    int delta;
    int count;

    PushButtonSoundScheme(0, 1);
    realm = region->callback_id;
    row = (GetAtomCursorY(event) - region->y1 - 1) / 0xd;
    visible = g_review_character_0069c0f8->skill_unlocks[0x1c + realm] -
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
            OpenSpellInfoDialog(
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
        OpenSpellInfoDialog(
            g_camp_screen_0069c0f4->learned_spells
                .spell_ids_by_realm[realm]
                                   [g_camp_screen_0069c0f4->learned_spells.scroll[realm] + row]);
        return 1;
    }
    return 1;
}

/* Open the spell info dialog for one spell id off the camp spell lists. */
// FUNCTION: WIZ8 0x005B7BB0
void OpenSpellInfoDialog(unsigned int spell_id)
{
    W8SpellInfoDialog* dialog = new W8SpellInfoDialog(spell_id);
    dialog->SetText(&g_wchar_00689b34);
    DisplayCampDialog(dialog);
}

/* The items-page redraw driver, run once per update: each pending group of
   redraw flags is cleared by repainting that block, and the two bottom/right
   panels repaint when their own bits are raised. */
// FUNCTION: WIZ8 0x005b7d10
void RedrawCampItemsPage(void)
{
    SetFont(g_font_683660);
    SetObjectShade(g_wiz_text_font_secondary_object_683680, 4);
    if ((g_camp_screen_0069c0f4->redraw_flags & 0x2000) != 0) {
        DrawCampCharacterInfo();
    }
    if ((g_camp_screen_0069c0f4->item_redraw_flags & 0x1ff) != 0) {
        DrawCampBackpackItems();
    }
    if ((g_camp_screen_0069c0f4->item_redraw_flags & 0x3ffe00) != 0) {
        DrawCampEquipmentItems();
    }
    if ((g_camp_screen_0069c0f4->item_redraw_flags & 0x7fc00000) != 0) {
        DrawCampItemPool();
        g_camp_screen_0069c0f4->item_range->UpdateRange(1);
    } else {
        g_camp_screen_0069c0f4->item_range->UpdateRange(0);
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
void DrawCampCharacterInfo(void)
{
    W8CampScreenState* state = g_camp_screen_0069c0f4;
    W8Character* character = g_review_character_0069c0f8;
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
    swprintf(state->caption, g_format_d_slash_d_00614b58, character->hp_current,
             character->uiHPMax);
    DrawRcsText(state->caption, 0x18d, 0x3a, 0x2c,
                g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C);
    swprintf(state->caption, g_format_d_slash_d_00614b58, character->stamina,
             character->uiStaminaMax);
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
void DrawCampBackpackItems(void)
{
    W8CampScreenState* state = g_camp_screen_0069c0f4;
    W8Character* character = g_review_character_0069c0f8;
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
        item_id = character->backpack[slot].iItemNo;
        if (item_id == -1 || CanCharacterUseItem(character, item_id) != 0) {
            BlitCatalogSurfaceRectTo16BPP(-14, left, top, right, bottom, 0x1b6, 0, 0);
        } else {
            DrawCatalogImage(-14, 0x11a, 0, 0, left, top, 2, 0);
        }
        if (item_id != -1) {
            DrawCatalogImage(-14, g_item_video_objects_68ec68.GetOrCreateVideoObject(item_id), 0, 0,
                             left + 1, top + 1, 2, 0);
            DrawCampItemQuantity(&character->backpack[slot], left + 1, top + 0x28, 0x2c);
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
void DrawCampEquipmentItems(void)
{
    W8CampScreenState* state = g_camp_screen_0069c0f4;
    W8Character* character = g_review_character_0069c0f8;
    const W8CampScreenRegion* region;
    unsigned int slot;
    int item_id;
    int frame;

    if ((state->item_redraw_flags & 0x200) != 0) {
        DrawCatalogImageAndInvalidate(-14, 0x114, 0, 2, 0x71, 0xa5, 2, 0);
        DrawCatalogImageAndInvalidate(
            -14, g_race_portrait_images_64cda0[character->iRace * 3 + character->gender], 0, 0,
            0xc2, 0xa5, 2, 0);
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
        item_id = character->EquippedItem[slot].iItemNo;
        if (item_id == -1) {
            if (slot == 7 || slot == 9) {
                int paired = character->EquippedItem[GetPairedEquipSlot(slot)].iItemNo;
                if (paired != -1 && (g_item_records[paired].flags_041 & 4) != 0) {
                    DrawCatalogImageAndInvalidate(-14, 0x146, 0, 0, region->x, region->y, 2, 0);
                }
            }
        } else {
            DrawCatalogImageAndInvalidate(
                -14, g_item_video_objects_68ec68.GetOrCreateVideoObject(item_id), 0, 1, region->x,
                region->y, 2, 0);
            DrawCampItemQuantity(&character->EquippedItem[slot], region->x + 2,
                                 region->y + region->height - 0xd, region->width - 4);
            if (character->EquippedItem[slot].identified == 0) {
                DrawCatalogImage(-14, 0x11b, 0, static_cast<short>(region->unidentified_frame_14),
                                 region->x, region->y, 2, 0);
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
        DrawCatalogImageAndInvalidate(-14, 0x115, 0, region->frame_10 + 3, region->x - 2,
                                      region->y - 2, 2, 0);
        if (item_id != -1 && g_item_records[item_id].binds_on_equip != 0 &&
            character->EquippedItem[slot].bound != 0) {
            DrawCatalogImageAndInvalidate(-14, 0x115, 0, region->frame_10 + 2, region->x - 2,
                                          region->y - 2, 2, 0);
        }
        if (state->hover_region == slot + 0xfc) {
            if (g_status_685170.item_in_cursor == 0) {
                if (character->EquippedItem[slot].iItemNo == -1) {
                    continue;
                }
            } else if (state->entry_mode == 1 ||
                       !CanEquipItemInSlot(character, g_status_685170.item_in_hand_235b.iItemNo,
                                           static_cast<unsigned char>(slot), 1) ||
                       !CanCharacterUseItem(character, g_status_685170.item_in_hand_235b.iItemNo)) {
                if (g_status_685170.item_in_cursor == 0 || state->entry_mode != 1) {
                    continue;
                }
                if (character->EquippedItem[slot].iItemNo == -1) {
                    continue;
                }
            }
            frame = region->frame_10;
        } else {
            if (g_status_685170.item_in_cursor == 0 || state->entry_mode == 1 ||
                !IsPartySlotEligible(giReviewCharSlot) ||
                !CanEquipItemInSlot(character, g_status_685170.item_in_hand_235b.iItemNo,
                                    static_cast<unsigned char>(slot), 1) ||
                !CanCharacterUseItem(character, g_status_685170.item_in_hand_235b.iItemNo)) {
                continue;
            }
            frame = region->frame_10 + 1;
        }
        DrawCatalogImageAndInvalidate(-14, 0x115, 0, frame, region->x - 2, region->y - 2, 2, 0);
    }
}

/* The shared party item pool: the bottom-right grid of up to eight visible
   cells, plus the header strip and the gold counter when bit 22 is raised. */
// FUNCTION: WIZ8 0x005b8b20
void DrawCampItemPool(void)
{
    W8CampScreenState* state = g_camp_screen_0069c0f4;
    W8Character* character = g_review_character_0069c0f8;
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
        if (CanCharacterUseItem(character, item->iItemNo) == 0) {
            DrawCatalogImage(-14, 0x11a, 0, 0, left, top, 2, 0);
        } else {
            BlitCatalogSurfaceRectTo16BPP(-14, left, top, right, bottom, 0x1b6, 0, 0);
        }
        DrawCatalogImage(-14, g_item_video_objects_68ec68.GetOrCreateVideoObject(item->iItemNo), 0,
                         0, left + 1, top + 1, 2, 0);
        DrawCampItemQuantity(item, left + 1, top + 0x28, 0x2c);
        if (item->identified == 0) {
            DrawCatalogImage(-14, 0x11b, 0, 0, left, top, 2, 0);
        }
        if (g_item_records[item->iItemNo].binds_on_equip != 0 && item->bound != 0) {
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
void DrawCampItemQuantity(W8ItemInstance* item, int left, int top, int width)
{
    W8CampScreenState* state = g_camp_screen_0069c0f4;
    int height;

    switch (g_item_records[item->iItemNo].quantity_kind) {
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
int CreateCampActionPanel(void)
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
            g_camp_action_buttons_0069c468[0]->m_primaryActivationCallback = UnequipBothHands;
            g_camp_action_buttons_0069c468[1]->m_primaryActivationCallback = TogglePartyRowFlag;
            g_camp_action_panel_0069c464->SetEnabled(1);
            return 1;
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x005b9270
void EnableCampActionButtons(void)
{
    g_camp_action_buttons_0069c468[0]->SetActive(1);
    g_camp_action_buttons_0069c468[1]->SetActive(1);
    if (g_status_685170.game_started != 0) {
        g_camp_action_buttons_0069c468[1]->SetEnabled(1);
        if (g_status_685170.buffers.XChar[giReviewCharSlot].item_action_pending_0f5 != 0) {
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
void DisableCampActionButtons(void)
{
    g_camp_action_buttons_0069c468[0]->SetActive(0);
    g_camp_action_buttons_0069c468[1]->SetActive(0);
}

// FUNCTION: WIZ8 0x005b9330
void RefreshCampActionPanel(char invalidate)
{
    if (invalidate != 0) {
        g_camp_action_panel_0069c464->Invalidate(0);
    }
    g_camp_action_panel_0069c464->Redraw();
}

/* The right-hand panel of seven tabs: the six realm filters and the pool sort
   button. */
// FUNCTION: WIZ8 0x005b9350
int CreateItemsTabPanel(void)
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
void UpdateItemsRealmTabs(void)
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
void DisableItemsRealmTabs(void)
{
    int index;

    for (index = 0; index < 7; ++index) {
        g_camp_realm_tabs_0069c470[index]->SetActive(0);
    }
}

// FUNCTION: WIZ8 0x005b98e0
void RefreshItemsTabPanel(char invalidate)
{
    if (invalidate != 0) {
        g_camp_realm_tab_panel_0069c48c->Invalidate(0);
    }
    g_camp_realm_tab_panel_0069c48c->Redraw();
}

/* The top secondary panel: the Items/Character info page tabs, the help line,
   the seven attribute labels and the four secondary value labels. The labels
   are W8CampInfoLabel controls created with absolute coordinates
   relative to the panel origin. */
// FUNCTION: WIZ8 0x005b9900
int CreateCampSecondaryPanel(void)
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
        g_camp_stat_labels_0069c448[index] =
            new W8CampInfoLabel(panel, -1, left, top, right, top + 0xc, -1, -1, -1, -1, -1, -1, -1);
        g_camp_stat_labels_0069c448[index]->EnableRegionHelp(0x958);
        top += 0xe;
    }
    left = 0x144 - panel->origin_x;
    right = 0x1b9 - panel->origin_x;
    top = -panel->origin_y;
    g_camp_info_labels_0069c42c[0] = new W8CampInfoLabel(panel, -1, left, top + 0x3a, right,
                                                         top + 0x46, -1, -1, -1, -1, -1, -1, -1);
    g_camp_info_labels_0069c42c[1] = new W8CampInfoLabel(panel, -1, left, top + 0x48, right,
                                                         top + 0x54, -1, -1, -1, -1, -1, -1, -1);
    g_camp_info_labels_0069c42c[2] = new W8CampInfoLabel(panel, -1, left, top + 0x80, right,
                                                         top + 0x8c, -1, -1, -1, -1, -1, -1, -1);
    g_camp_info_labels_0069c42c[3] = new W8CampInfoLabel(panel, -1, left, top + 0x8e, right,
                                                         top + 0x9a, -1, -1, -1, -1, -1, -1, -1);
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
void EnableCampSecondaryPanel(void)
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
void DisableCampSecondaryPanel(void)
{
    int index;

    g_camp_secondary_panel_0069c428->EnableRegionSet(0);
    for (index = 0; index < 2; ++index) {
        g_camp_page_tabs_0069c43c[index]->SetActive(0);
    }
}

// FUNCTION: WIZ8 0x005b9f90
void RefreshCampSecondaryPanel(char invalidate)
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
    } else if (g_review_character_0069c0f8->armor_class_components[11] > 0) {
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
        ((g_review_character_0069c0f8->armor_class_components[11] > 0 && m_renderArg_20 != 2) ||
         (g_review_character_0069c0f8->armor_class_components[11] <= 0 && m_renderArg_20 == 2))) {
        SetCombatView(0);
    }
    Controls::Redraw();
    if (!redraw)
        return;
    InvalidateCampPanel();
    DrawRcsText(gppStringList[0x24d4 / 4], 0x15e, 0x84, 0x4e,
                g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED548);
    swprintf(g_camp_screen_0069c0f4->caption, L"%d", g_review_character_0069c0f8->kill_count_09f9);
    DrawRcsText(g_camp_screen_0069c0f4->caption, 0x1ae, 0x84, 0x20,
                g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
    DrawRcsText(gppStringList[0x24d8 / 4], 0x15e, 0x92, 0x4e,
                g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED548);
    swprintf(g_camp_screen_0069c0f4->caption, L"%d", g_review_character_0069c0f8->death_count_09fd);
    DrawRcsText(g_camp_screen_0069c0f4->caption, 0x1ae, 0x92, 0x20,
                g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
    if (m_combat_view) {
        DrawRcsText(gppStringList[0x22c0 / 4], 0x15b, 10, 0x11d,
                    g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
        DrawRcsText(gppStringList[0x22c4 / 4], 0x15e, 0x30, 0x4e,
                    g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED548);
        swprintf(g_camp_screen_0069c0f4->caption, L"%d", g_review_character_0069c0f8->initiative);
        DrawRcsText(g_camp_screen_0069c0f4->caption, 0x1ae, 0x30, 0x20,
                    g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
        if (g_review_character_0069c0f8->EquippedItem[6].iItemNo == -1 &&
            g_review_character_0069c0f8->EquippedItem[7].iItemNo == -1) {
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
            ItemHasSingledOutGenericName(g_review_character_0069c0f8->EquippedItem[6].iItemNo) &&
            !g_review_character_0069c0f8->EquippedItem[7].identified;
        for (unsigned int hand = 0; hand < 2; ++hand) {
            W8HandAttack* attack = &g_review_character_0069c0f8->Hand[hand];
            if (!attack->in_play)
                continue;
            int x = hand ? 600 : 0x1d6;
            if (g_review_character_0069c0f8->EquippedItem[hand + 6].iItemNo != -1 &&
                (!g_review_character_0069c0f8->EquippedItem[hand + 6].identified ||
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
            GetCharacterHandDamageDice(g_review_character_0069c0f8, hand, &dice);
            unsigned int damage_bonus =
                GetCharacterHandDamageBonus(g_review_character_0069c0f8, hand);
            unsigned int minimum = ((dice.base + dice.count) * (100 + damage_bonus) + 50) / 100;
            unsigned int maximum =
                ((dice.base + dice.count * dice.sides) * (100 + damage_bonus) + 50) / 100;
            if (minimum < 2)
                minimum = 1;
            if (maximum < 2)
                maximum = 1;
            int hit_bonus =
                attack->hit_bonus + g_review_character_0069c0f8->bonus_1770.hit_bonus_01;
            int skill_bonus =
                (attack->attack_score < 0 ? attack->attack_score - 2 : attack->attack_score + 2) /
                5;
            swprintf(g_camp_screen_0069c0f4->caption, L"%+d",
                     attack->damage_bonus +
                         g_review_character_0069c0f8->bonus_1770.damage_bonus_00);
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
                     attack->attack_bonus_25 +
                         g_review_character_0069c0f8->bonus_1770.attack_bonus_02);
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
            if (component == 11 && g_review_character_0069c0f8->armor_class_components[11] == 0)
                continue;
            int y = (component % 6) * 14 + 0x22;
            int label_x = component / 6 ? 0x1ef : 0x15e;
            int value_x = component / 6 ? 600 : 0x1c7;
            DrawRcsText(gppStringList[g_camp_armor_class_labels[component]], label_x, y, 0x67,
                        g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED548);
            int value = g_review_character_0069c0f8->armor_class_components[component];
            if (value) {
                swprintf(g_camp_screen_0069c0f4->caption, L"%+d", value);
                DrawRcsText(g_camp_screen_0069c0f4->caption, value_x, y, 0x20,
                            g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
            }
        }
        DrawRcsText(gppStringList[0x22fc / 4], 0x1da, 0x84, 0x7c,
                    g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED548);
        swprintf(g_camp_screen_0069c0f4->caption, L"%d%%",
                 g_review_character_0069c0f8->damage_reduction);
        DrawRcsText(g_camp_screen_0069c0f4->caption, 600, 0x84, 0x20,
                    g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554);
    }
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
    g_review_character_0069c0f8 = static_cast<W8Character*>(g_current_screen_state.parameter_3);
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
    gXStatus.item_drag_active = 0;
    gXStatus.dragged_item = 0;
    gXStatus.dragged_item_origin = 0xff;
    gXStatus.dragged_character_slot = -1;
    if (!g_camp_screen_0069c0f4) {
        g_camp_screen_0069c0f4 = static_cast<W8CampScreenState*>(malloc(sizeof(W8CampScreenState)));
        if (!g_camp_screen_0069c0f4) {
            if (IsMessageBoxActive()) {
                CloseMessageBox();
            }
            BeginCombatRound();
            RequestScreenTransition();
            return 0;
        }
        memset(g_camp_screen_0069c0f4, 0, sizeof(W8CampScreenState));
    }
    SetClippingRegionAndImageWidth(0x500, 0, 0, 0x280, 0x1e0);
    g_camp_screen_0069c0f4->entry_mode = entry_mode;
    MSYS_Init();
    for (unsigned int realm = 0; realm < 6; ++realm) {
        g_camp_screen_0069c0f4->realm_flags[realm] = 0;
    }
    g_camp_screen_0069c0f4->item_timer_active = false;
    g_camp_screen_0069c0f4->item_timer_expired = false;
    CreateCampButtonPanel();
    CreateCampActionPanel();
    CreateItemsTabPanel();
    CreateCampSecondaryPanel();
    CreateRcsLevelUpPanel();
    CreateRcsDismissPanel();
    g_camp_screen_0069c0f4->page = 0;
    g_camp_screen_0069c0f4->item_mode = 0;
    g_camp_screen_0069c0f4->item_range = new W8CampItemRange;
    for (int range_index = 0; range_index < 6; ++range_index) {
        g_camp_screen_0069c0f4->spell_ranges[range_index] = new W8CampSpellRange(range_index);
    }
    g_camp_screen_0069c0f4->effect_list = 0;
    g_camp_screen_0069c0f4->effect_items_only = 1;
    g_camp_screen_0069c0f4->effect_filter = 0;
    g_camp_screen_0069c0f4->stats_range = new W8CampStatsRange;
    g_camp_screen_0069c0f4->stats_controls = new W8CampStatsControls;
    g_camp_screen_0069c0f4->character_info = new W8CampCharacterInfo;
    g_camp_screen_0069c0f4->item_icons_drawn_d50 = false;
    ActivateCampPage();
    g_camp_screen_0069c0f4->animation_timer = SetCountdownClock(50);
    for (unsigned int animation = 0; animation < 6; ++animation) {
        g_camp_screen_0069c0f4->animation_frames[animation] =
            Random(g_spell_realm_animations_00648c90[animation].frame_count);
    }
    if (gXStatus.fCombatMode && g_combat_state->equip_phase_a50) {
        if (g_combat_state->equip_pending_a51 == 1) {
            for (int slot = 0; slot < 8; ++slot) {
                if (g_status_685170.buffers.XChar[slot].fOccupied && IsPartySlotEligible(slot) &&
                    g_status_685170.buffers.XChar[slot].pending_action == 9) {
                    swprintf(g_camp_screen_0069c0f4->caption, L"%s %s",
                             g_status_685170.buffers.Char[slot].name, gppStringList[0x2464 / 4]);
                    goto show_equip_message;
                }
            }
        } else {
            unsigned char count = 0;
            for (int slot = 0; slot < 8; ++slot) {
                if (g_status_685170.buffers.XChar[slot].fOccupied && IsPartySlotEligible(slot) &&
                    g_status_685170.buffers.XChar[slot].pending_action == 9) {
                    ++count;
                    if (count == 1) {
                        swprintf(g_camp_screen_0069c0f4->caption, L"%s",
                                 g_status_685170.buffers.Char[slot].name);
                    } else {
                        if (count == g_combat_state->equip_pending_a51) {
                            wcscat(g_camp_screen_0069c0f4->caption, L" ");
                            wcscat(g_camp_screen_0069c0f4->caption,
                                   FormatWideString(gppStringList[0x2460 / 4],
                                                    g_status_685170.buffers.Char[slot].name));
                            goto show_equip_message;
                        }
                        wcscat(g_camp_screen_0069c0f4->caption, L", ");
                        wcscat(g_camp_screen_0069c0f4->caption,
                               g_status_685170.buffers.Char[slot].name);
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
        StartMusicResource("MainMenu.MPL", 1, 1);
    }
    return 1;
}

// FUNCTION: WIZ8 0x005a3ae0
void CampScreenFrame(void)
{
    if (g_dev_mode_689b32) {
        RequestExitScreen();
    }
    if (IsMessageBoxActive()) {
        ProcessMessageBoxInput();
    }
    ServiceMusicPlaylist();
    if (g_camp_screen_0069c0f4->dialog && !ProcessDialogInput(g_camp_screen_0069c0f4->dialog)) {
        ClearActiveRegionIfMatches(0x138);
        delete g_camp_screen_0069c0f4->dialog;
        g_camp_screen_0069c0f4->dialog = 0;
        g_camp_screen_0069c0f4->redraw_flags |= 0x0fffffff;
    }
    if (gXStatus.level_up_notice_027 && g_suspended_screen_id != W8_SCREEN_CHARACTER &&
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
                            CloseMessageBox();
                        }
                        BeginCombatRound();
                        RequestScreenTransition();
                    } else {
                        SelectCampCharacter(CharacterPointerToPartySlot(g_camp_character_0069c100));
                        if (!IsPartySlotEligible(giReviewCharSlot)) {
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
        g_camp_screen_0069c0f4->item_timer_expired = true;
        g_camp_screen_0069c0f4->item_redraw_flags |= 0x3ffe00;
    }
    if (!g_camp_screen_0069c0f4->input_mode) {
        gXStatus.character_event_queue->ProcessDeferredCharacterEvents();
        UpdateCharacterEventState();
    }
    DrawCampScreen();
}

// FUNCTION: WIZ8 0x005a3ee0
unsigned char CampScreenLeave(int)
{
    DeactivateCampPage();
    SetFontObjectPalette16BPP(g_smfnt_font_683694, g_font_palette_smfnt_68ee10);
    SetFontObjectPalette16BPP(g_calligraphy_font_6835f8, g_font_palette_calligraphy_68edfc);
    SetFontObjectPalette16BPP(g_calligraphy_shadow_font_6835f4,
                              g_font_palette_calligraphy_shadow_68ee18);
    SetFontObjectPalette16BPP(g_wiz_text_font_683640, g_font_palette_wiz_text_68ee14);
    DestroyCampButtonPanel();
    ReleaseCampActionPanel();
    ReleaseItemsTabPanel();
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
    delete g_camp_screen_0069c0f4->stats_range;
    delete g_camp_screen_0069c0f4->stats_controls;
    delete g_camp_screen_0069c0f4->character_info;
    free(g_camp_screen_0069c0f4);
    g_camp_screen_0069c0f4 = 0;
    MSYS_Shutdown();
    ResetRegions();
    if (gXStatus.fCampMode) {
        gXStatus.dialogue_sync_pending_026 = 1;
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
        SelectCampCharacter(CharacterPointerToPartySlot(g_camp_character_0069c100));
        if (IsPartySlotEligible(giReviewCharSlot) != 0) {
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
        CloseMessageBox();
    }
    BeginCombatRound();
    RequestScreenTransition();
}

/* The camp screen's per-frame update: on the first frame after combat mode
   ends it re-raises every redraw flag, then repaints whichever page is active
   together with the message box, the level-up/dismissal panels and the dialog
   on top of the frame. */
// FUNCTION: WIZ8 0x005a42a0
void DrawCampScreen(void)
{
    W8CampScreenState* state = g_camp_screen_0069c0f4;
    unsigned int index;

    NoOp();
    if (!g_monster_combat_timer_enabled_006f0531 && state->item_icons_drawn_d50) {
        state->redraw_flags |= 0xfffffff;
        state->item_icons_drawn_d50 = false;
    }
    if (state->redraw_flags != 0 || state->item_redraw_flags != 0 || IsMessageBoxActive() ||
        g_status_685170.item_in_cursor) {
        if (state->redraw_flags == 0xfffffff) {
            state->item_redraw_flags = 0xffffffff;
            if (state->dialog != 0) {
                state->dialog->m_dirty_flags |= 1;
            }
        }
        DrawCampHeader();
        switch (state->page) {
        case 0:
            RedrawCampItemsPage();
            break;
        case 1:
            DrawCampStatsPage();
            break;
        case 2:
            DrawCampSkillsPage();
            break;
        case 3:
            DrawCampSpellPages();
            break;
        case 4:
            DrawCampRegenStats();
            break;
        }
        RefreshCampItemActions((state->redraw_flags & 0x1000) != 0);
        SetFont(g_calligraphy_font_6835f8);
        SetObjectShade(g_calligraphy_font_object_683628, 4);
        state->item_redraw_flags = 0;
        state->redraw_flags = 0;
        if (IsMessageBoxActive()) {
            RenderMessageBox();
            if (!IsMessageBoxActive()) {
                state->redraw_flags |= 0xfffffff;
            }
        }
    }
    state->character_info->Redraw();
    if (state->page == 0) {
        RefreshCampActionPanel(0);
        RefreshItemsTabPanel(0);
        RefreshCampSecondaryPanel(0);
        state->item_range->m_range->Redraw();
        if (g_monster_combat_timer_enabled_006f0531 && state->dialog == 0) {
            DrawCampItemIcons();
            state->item_icons_drawn_d50 = true;
        }
    } else if (state->page == 1) {
        state->stats_range->UpdateRange(0);
        state->stats_controls->Redraw();
    } else if (state->page == 3) {
        for (index = 0; index < 6; ++index) {
            state->spell_ranges[index]->UpdateRange(0);
        }
    }
    RefreshCampItemActions(0);
    if (gXStatus.fCombatMode == 0) {
        if (giReviewCharSlot != -1) {
            if (g_status_685170.game_started != 0 ||
                (g_previous_screen_id == 5 && PartySelectionInReviewMode() != 0)) {
                UpdateRcsLevelUpPanel();
            }
            if (gXStatus.fCombatMode != 0) {
                goto done;
            }
        }
        if (g_status_685170.game_started != 0) {
            UpdateRcsDismissPanel();
        }
    }
done:
    RedrawPortraitQuoteBubbles();
    if (state->dialog != 0) {
        DrawDialog(state->dialog);
    }
    RenderFrame();
}

// FUNCTION: WIZ8 0x005A4570
void SyncReviewCharInputRegion(void)
{
    if (giReviewCharSlot != -1 && g_status_685170.buffers.XChar[giReviewCharSlot].npc_index != -1) {
        DisableRegionInput(0xf2);
        return;
    }
    EnableRegionInput(0xf2);
}

/* Switch the active camp page: tear the old page down, record the new index,
   bring it up and repaint the whole character block. */
// FUNCTION: WIZ8 0x005a4540
void SwitchCampPage(int page)
{
    DeactivateCampPage();
    g_camp_screen_0069c0f4->page = page;
    ActivateCampPage();
    g_camp_screen_0069c0f4->redraw_flags |= 0xfffffff;
}

/* Enter the active camp page: resets the item action mode when leaving the
   items page, gates the party-portrait regions on whether a character is being
   reviewed, then enables the page's own region sets and controls. */
// FUNCTION: WIZ8 0x005a45b0
void ActivateCampPage(void)
{
    W8CampScreenState* state = g_camp_screen_0069c0f4;
    unsigned int index;

    RegionSetEnable(0x29);
    if (state->page != 0) {
        SetCampItemActionMode005B59B0(0);
    }
    if (giReviewCharSlot == -1) {
        for (index = 0; index < 8; ++index) {
            DisableRegionInput(index + 0xea);
        }
    } else if (g_status_685170.buffers.XChar[giReviewCharSlot].npc_index == -1) {
        EnableRegionInput(0xf2);
    } else {
        DisableRegionInput(0xf2);
    }
    state->input_mode = 0;
    state->redraw_flags |= 0x7ff;
    switch (state->page) {
    case 0:
        RegionSetEnable(0x2a);
        EnableCampActionButtons();
        UpdateItemsRealmTabs();
        EnableCampSecondaryPanel();
        SetItemPageMode005B9FD0(state->item_mode);
        state->item_range->m_range->EnableRegionSet(1);
        if (g_status_685170.game_started != 0) {
            RegionSetEnable(0x2b);
            state->item_scroll = 0;
            RebuildCampItemList();
            return;
        }
        break;
    case 1:
        RebuildCampEffectList();
        state->stats_range->m_range->EnableRegionSet(1);
        state->stats_controls->EnableRegionSet(1);
        state->character_info->SetEnabled(1);
        return;
    case 2:
        CreateCampSkillRegions();
        return;
    case 3:
        RegionSetEnable(0x2c);
        BuildLearnedSpellState(&state->learned_spells, g_review_character_0069c0f8);
        RefreshCampSpellRanges();
        gXStatus.fSpellCastMode = false;
        state->selected_spell_row = -1;
        break;
    }
}

/* Leave the active camp page: disables the page's region sets and controls and
   releases the page's rebuilt state - the effect list on the stats page, the
   skill-improvement flags on the skills page. */
// FUNCTION: WIZ8 0x005a4770
void DeactivateCampPage(void)
{
    W8CampScreenState* state = g_camp_screen_0069c0f4;
    unsigned int index;

    switch (state->page) {
    case 0:
        RegionSetDisable(0x2a);
        DisableItemsRealmTabs();
        DisableCampActionButtons();
        DisableCampSecondaryPanel();
        state->character_info->SetEnabled(0);
        state->item_range->m_range->EnableRegionSet(0);
        if (g_status_685170.game_started != 0) {
            RegionSetDisable(0x2b);
            return;
        }
        break;
    case 1:
        state->stats_range->m_range->EnableRegionSet(0);
        state->stats_controls->EnableRegionSet(0);
        state->character_info->SetEnabled(0);
        if (state->effect_list != 0) {
            DeleteStack(state->effect_list);
            state->effect_list = 0;
            return;
        }
        break;
    case 2:
        DisableCampSkillRegions();
        for (index = 0; index < 0x29; ++index) {
            g_review_character_0069c0f8->skills[index].improved_12 = 0;
        }
        return;
    case 3:
        RegionSetDisable(0x2c);
        SetCampSpellRangesEnabled(0);
        break;
    }
}

/* Camp page 4: the reviewed character's health, stamina and per-realm spell
   point regeneration rates. */
// FUNCTION: WIZ8 0x005a4890
void DrawCampRegenStats(void)
{
    unsigned int index;

    SetFont(g_calligraphy_font_6835f8);
    SetObjectShade(g_calligraphy_font_object_683628, 4);
    SetObjectShade(g_calligraphy_font_object_683628, 0);
    gprintfDirty(0x14a, 5, gppStringList[0x8ce]);
    SetObjectShade(g_calligraphy_font_object_683628, 4);
    gprintfDirty(0x221, 5, L"%6.3f", g_review_character_0069c0f8->health_regen_rate_0b69);
    SetObjectShade(g_calligraphy_font_object_683628, 0);
    gprintfDirty(0x14a, 0x14, gppStringList[0x8cd]);
    SetObjectShade(g_calligraphy_font_object_683628, 4);
    gprintfDirty(0x221, 0x14, L"%6.3f", g_review_character_0069c0f8->stamina_regen_rate_0b71);
    SetObjectShade(g_calligraphy_font_object_683628, 0);
    gprintfDirty(0x14a, 0x23, gppStringList[0x8d0]);
    SetObjectShade(g_calligraphy_font_object_683628, 4);
    for (index = 0; index < 6; ++index) {
        gprintfDirty(index * 0x14 + 0x1fe, 0x23, L"%6.3f/",
                     g_review_character_0069c0f8->spell_regen_rates_0b79[index * 2]);
    }
}

/* Kicked off by the post-quake camera-shake callback: latches the endgame
   flags, resets input regions, then starts the fade whose completion runs
   ShowEndingScreen - the ending sequence picker. Fact 0x1a2 forces the long
   fade, fact 0x2f4 swaps the timing and marks the variant. */
/* Realm filters 2-5 are exclusive: selecting one clears the others. */
// FUNCTION: WIZ8 0x005a49d0
void ClearOtherRealmFilters(unsigned int realm)
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
void RebuildCampItemList(void)
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
        if (pool->iItemNo != -1 &&
            (g_camp_screen_0069c0f4->realm_flags[0] == 0 ||
             CanCharacterUseItem(g_review_character_0069c0f8, pool->iItemNo) != 0) &&
            (g_camp_screen_0069c0f4->realm_flags[1] == 0 || pool->identified == 0) &&
            (filter == 0 || (filter & static_cast<unsigned char>(
                                          1 << GetItemEquipSlotGroup(pool->iItemNo))) != 0)) {
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
void SetCampInputMode(int mode)
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
void HandleCampItemClick(W8ItemInstance* item, unsigned int slot_index, unsigned int origin)
{
    W8CombatSlot target;
    short related_kind;
    bool changed = false;
    bool merged = false;
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
    if (g_status_685170.item_in_cursor != 0 && item->iItemNo != -1 &&
        GetItemMergeKind(item->iItemNo, &related_kind) != 0 &&
        g_item_records[g_status_685170.item_in_hand_235b.iItemNo].unidentified_name_index ==
            related_kind) {
        same_kind = 1;
    }
    if (item->iItemNo == -1 && g_status_685170.item_in_cursor == 0) {
        return;
    }
    if (origin == 1 && g_status_685170.item_in_cursor != 0 &&
        g_camp_screen_0069c0f4->entry_mode != 1) {
        if (same_kind == 0 &&
            CanEquipItemInSlot(g_review_character_0069c0f8,
                               g_status_685170.item_in_hand_235b.iItemNo, slot_index, 1) == 0) {
            return;
        }
        if (CanCharacterUseItem(g_review_character_0069c0f8,
                                g_status_685170.item_in_hand_235b.iItemNo) == 0) {
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
    character = &g_status_685170.buffers.Char[giReviewCharSlot];
    if (character->uiCondition[0x13] != 0 && (origin == 1 || origin == 0)) {
        text = gppStringList[0x241c / 4];
        dialog = static_cast<W8MessageDialogBase*>(CreateDialogByKind(1));
        dialog->SetClientExtent(250, 200);
        dialog->SetMessage(text, 1, 50, 1, 0, 1, 1, 0, 350);
        SetDialogDestroyCallback(dialog, 0);
        g_camp_screen_0069c0f4->dialog = dialog;
        ActivateDialogRegion(0x138);
        return;
    }
    if (character->uiCondition[0xe] != 0 && (origin == 1 || origin == 0)) {
        text = gppStringList[0x2420 / 4];
        dialog = static_cast<W8MessageDialogBase*>(CreateDialogByKind(1));
        dialog->SetClientExtent(250, 200);
        dialog->SetMessage(text, 1, 50, 1, 0, 1, 1, 0, 350);
        SetDialogDestroyCallback(dialog, 0);
        g_camp_screen_0069c0f4->dialog = dialog;
        ActivateDialogRegion(0x138);
        return;
    }
    if (character->uiCondition[W8_CONDITION_HOSTILE] != 0 && (origin == 1 || origin == 0)) {
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
        g_status_685170.item_in_cursor != 0 && gXStatus.held_item_source != giReviewCharSlot) {
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
            ((g_item_records[g_status_685170.item_in_hand_235b.iItemNo].equip_class == 2 ||
              g_item_records[g_status_685170.item_in_hand_235b.iItemNo].equip_class == 4) &&
             gXStatus.held_item_source == giReviewCharSlot &&
             (origin != 1 || HeldItemFitsPairedSlot(giReviewCharSlot, slot_index) != 0))) {
            if (item->iItemNo == -1 || g_item_records[item->iItemNo].equip_class == 2 ||
                g_item_records[item->iItemNo].equip_class == 4 || same_kind != 0) {
                gated = 0;
            }
        }
    }
    if (gated != 0 && IsCampActionAllowed(giReviewCharSlot) == 0) {
        return;
    }
    if (gfKeyState[0x10] != 0) {
        TakeItemUnitToHand(item, slot_index, origin);
        return;
    }
    if (g_camp_screen_0069c0f4->entry_mode == 2) {
        party_slot = CharacterPointerToPartySlot(g_camp_entry_parameter_0069c0fc);
        if (CanItemLeaveItsSlot(item) == 0) {
            QueueCharacterEvent(&g_status_685170.buffers.Char[party_slot],
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
        W8PartySlotRow* row = &g_status_685170.buffers.XChar[party_slot];
        if (row->pending_action == 8 &&
            g_item_records[row->pending_action_detail_015.item_use.item->iItemNo].spell_id ==
                0x17) {
            reidentify = 1;
            result = CommitPartySlotItemUse(party_slot,
                                            row->pending_action_detail_015.item_use.item, &target);
        } else {
            result = CommitPartySlotSpell(party_slot, 0x17, 8, &target);
        }
        if (origin == 2 && reidentify != 0 &&
            old_pool_count != g_status_685170.party_item_count_1791) {
            item = &g_status_685170.party_item_pool_0021[g_status_685170.party_item_count_1791 -
                                                         old_pool_count + slot_index];
            RebuildCampItemList();
            RecalculateCarriedWeight(g_review_character_0069c0f8);
            RedistributePartyEncumbrance();
            g_camp_screen_0069c0f4->redraw_flags |= 0x2000;
            g_camp_screen_0069c0f4->item_redraw_flags |= 0x7fc00000;
        }
        if (result == 1 && item->iItemNo != -1) {
            OpenItemInfoDialog(item, 0);
            if (item->identified == 0) {
                QueueCharacterEvent(&g_status_685170.buffers.Char[party_slot],
                                    g_character_event_kind_005ee65c, 0, g_effect_argument_005ed8c8,
                                    g_effect_argument_005ed914);
            }
        }
        SetCampItemActionMode005B59B0(0);
        return;
    }
    if (g_camp_screen_0069c0f4->entry_mode == 8) {
        UseHeldItemOnItem(item);
        return;
    }
    if (g_camp_screen_0069c0f4->entry_mode == 3) {
        IdentifyAndOpenItemInfo(item);
        return;
    }
    if (g_camp_screen_0069c0f4->entry_mode == 4) {
        if (item->iItemNo != -1) {
            OpenSplitStackDialog(item);
            return;
        }
    } else if (g_camp_screen_0069c0f4->entry_mode == 5) {
        if (CanCharacterUseItemEntry(g_review_character_0069c0f8, item) != 0) {
            UseItem005BA4F0(item);
        }
        return;
    } else if (g_camp_screen_0069c0f4->entry_mode == 1) {
        if (g_status_685170.item_in_cursor != 0) {
            if (item->iItemNo == -1) {
                SetCampItemActionMode005B59B0(0);
                return;
            }
            if (g_camp_character_pending_0069c104 != 0) {
                SelectCampCharacter(CharacterPointerToPartySlot(g_camp_character_0069c100));
                if (IsPartySlotEligible(giReviewCharSlot) != 0) {
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
            MergeItemStacksWithHeld(item);
            return;
        }
        if (item->iItemNo == -1) {
            SetCampItemActionMode005B59B0(0);
            return;
        }
    }

    /* Equipped items that may not be removed get one warning dialog and a
       bound mark; the next click then unequips them. */
    if (item->iItemNo != -1 && origin == 1) {
        if (CanUnequipSlotItem(g_review_character_0069c0f8, slot_index) == 0) {
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
            BindEquippedItem(g_review_character_0069c0f8, slot_index);
            return;
        }
        item->bound = 1;
    }

    if (same_kind != 0) {
        MergeItemUses(&g_status_685170.buffers.Char[giReviewCharSlot], item,
                      &g_status_685170.item_in_hand_235b);
        SetCampItemActionMode005B59B0(0);
        changed = 1;
        if (origin == 1) {
            RebuildEquipmentAndDerivedStatsForSlot(giReviewCharSlot);
            if (GetPairedEquipSlot(slot_index) != -1) {
                g_status_685170.buffers.XChar[giReviewCharSlot].weapon_swap_pending_105 = 0;
            }
            RebuildCampItemList();
        } else if (origin == 0) {
            RecalculateCharacterDerivedStats(&g_status_685170.buffers.Char[giReviewCharSlot]);
        } else {
            RebuildCampItemList();
        }
    } else {
        /* Fold the held stack onto a matching stack; for the pool the scan
           continues across every row until the held stack is consumed. */
        if (g_status_685170.item_in_cursor != 0 &&
            g_item_records[g_status_685170.item_in_hand_235b.iItemNo].quantity_kind == 1) {
            if (item->iItemNo != -1 && item->iItemNo == g_status_685170.item_in_hand_235b.iItemNo) {
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
                        SelectCampCharacter(CharacterPointerToPartySlot(g_camp_character_0069c100));
                        if (IsPartySlotEligible(giReviewCharSlot) != 0) {
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
                    } else if (InsertItemIntoPartyPool(&g_status_685170.item_in_hand_235b,
                                                       slot_index) != 0) {
                        changed = 1;
                        RebuildCampItemList();
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
                        RebuildCampItemList();
                    } else if (g_camp_screen_0069c0f4->entry_mode == 1) {
                        SetHandCursors(0);
                        RebuildCampItemList();
                    } else {
                        RebuildCampItemList();
                    }
                }
            }
        } else {
            if (merge_tried == 0 && ((g_status_685170.item_in_cursor == 0 && item->iItemNo != -1) ||
                                     (g_status_685170.item_in_cursor != 0 && merged == 0))) {
                if (g_camp_character_pending_0069c104 == 0 ||
                    g_camp_character_0069c100 == g_review_character_0069c0f8) {
                    g_camp_character_pending_0069c104 = 0;
                    if (item->iItemNo != -1 && (giReviewCharSlot == 0 || giReviewCharSlot == 1)) {
                        npc =
                            GetNpcState(g_status_685170.buffers.XChar[giReviewCharSlot].npc_index);
                        if (npc != 0 && NpcWantsItem(npc, item) != 0) {
                            g_camp_character_pending_0069c104 = 1;
                            g_camp_character_0069c100 = g_review_character_0069c0f8;
                        }
                    }
                    if (g_status_685170.item_in_cursor != 0) {
                        choose_character = gXStatus.held_item_source == -1;
                        DeliverExceptionalItemReaction(&g_status_685170.item_in_hand_235b,
                                                       choose_character,
                                                       g_review_character_0069c0f8);
                    }
                    if (origin == 1 && g_status_685170.item_in_cursor != 0 &&
                        HeldItemFitsPairedSlot(giReviewCharSlot, slot_index) == 0) {
                        paired_slot = GetPairedEquipSlot(slot_index);
                        paired = &g_review_character_0069c0f8->EquippedItem[paired_slot];
                        if (CanUnequipSlotItem(g_review_character_0069c0f8, paired_slot) == 0) {
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
                            BindEquippedItem(g_review_character_0069c0f8, paired_slot);
                            return;
                        }
                        paired->bound = 1;
                        if (item->iItemNo == -1) {
                            SwapItemInstances(item, &g_status_685170.item_in_hand_235b,
                                              g_review_character_0069c0f8, 1);
                            g_camp_character_pending_0069c104 = 0;
                            if (paired->iItemNo != -1 &&
                                (giReviewCharSlot == 0 || giReviewCharSlot == 1)) {
                                npc = GetNpcState(
                                    g_status_685170.buffers.XChar[giReviewCharSlot].npc_index);
                                if (npc != 0 && NpcWantsItem(npc, paired) != 0) {
                                    g_camp_character_pending_0069c104 = 1;
                                    g_camp_character_0069c100 = g_review_character_0069c0f8;
                                }
                            }
                            SwapItemInstances(paired, &g_status_685170.item_in_hand_235b,
                                              g_review_character_0069c0f8, 1);
                            changed = 1;
                        } else if (AddItemToCharacter(g_review_character_0069c0f8, paired, 0, 0,
                                                      1) != 0) {
                            SwapItemInstances(item, &g_status_685170.item_in_hand_235b,
                                              g_review_character_0069c0f8, 1);
                            changed = 1;
                        } else {
                            if (giReviewCharSlot == 0 || giReviewCharSlot == 1) {
                                if (NpcWantsItem(
                                        GetNpcState(g_status_685170.buffers.XChar[giReviewCharSlot]
                                                        .npc_index),
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
                                              g_review_character_0069c0f8, 1);
                            changed = 1;
                        }
                    } else {
                        SwapItemInstances(item, &g_status_685170.item_in_hand_235b,
                                          g_review_character_0069c0f8, 1);
                        changed = 1;
                        if (origin == 0 &&
                            g_review_character_0069c0f8->backpack[slot_index].iItemNo != -1 &&
                            Random(100) < 5) {
                            QueueCharacterEvent(
                                g_review_character_0069c0f8, g_special_event_0068c55c, 0,
                                g_effect_argument_005ed8c8, g_effect_argument_005ed914);
                        }
                    }
                } else {
                    SelectCampCharacter(CharacterPointerToPartySlot(g_camp_character_0069c100));
                    if (IsPartySlotEligible(giReviewCharSlot) != 0) {
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
                SetHandCursors(0);
            }
            if (changed != 0) {
                if (origin == 1) {
                    RebuildEquipmentAndDerivedStatsForSlot(giReviewCharSlot);
                    if (GetPairedEquipSlot(slot_index) != -1) {
                        g_status_685170.buffers.XChar[giReviewCharSlot].weapon_swap_pending_105 = 0;
                    }
                    RebuildCampItemList();
                } else if (origin == 0) {
                    RecalculateCharacterDerivedStats(
                        &g_status_685170.buffers.Char[giReviewCharSlot]);
                } else {
                    RebuildCampItemList();
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
    RecalculateCarriedWeight(g_review_character_0069c0f8);
    RedistributePartyEncumbrance();
    g_camp_screen_0069c0f4->redraw_flags |= 0x2000;
    g_camp_screen_0069c0f4->redraw_flags |= 0x100;
}

// FUNCTION: WIZ8 0x005a5da0
void TakeItemUnitToHand(W8ItemInstance* item, unsigned short slot, unsigned int origin)
{
    W8ItemInstance single;
    bool moved = false;

    if (g_camp_screen_0069c0f4->entry_mode != 0) {
        return;
    }
    if (item->iItemNo == -1) {
        return;
    }
    if (CanSplitItemStack(item) == 0) {
        return;
    }
    if (g_status_685170.item_in_cursor == 0) {
        single = *item;
        single.stack_count = 1;
        --item->stack_count;
        if (origin == 1 || origin == 0) {
            CopyItemInstance(&g_status_685170.item_in_hand_235b, &single,
                             &g_status_685170.buffers.Char[giReviewCharSlot], 1);
            gXStatus.held_item_origin = origin;
            gXStatus.held_item_slot = slot;
        } else {
            CopyItemInstance(&g_status_685170.item_in_hand_235b, &single, 0, 1);
        }
        moved = 1;
    } else if (item->iItemNo == g_status_685170.item_in_hand_235b.iItemNo &&
               g_status_685170.item_in_hand_235b.stack_count <
                   g_item_records[g_status_685170.item_in_hand_235b.iItemNo].maximum_quantity) {
        ++g_status_685170.item_in_hand_235b.stack_count;
        --item->stack_count;
        moved = 1;
    }
    if (item->stack_count == 0) {
        EmptyItemRecord(item, g_review_character_0069c0f8, 1);
    } else if (moved == 0) {
        return;
    }
    RebuildEquipmentAndDerivedStatsForSlot(giReviewCharSlot);
    RebuildCampItemList();
    RecalculateCharacterDerivedStats(&g_status_685170.buffers.Char[giReviewCharSlot]);
    RecalculateCarriedWeight(g_review_character_0069c0f8);
    RedistributePartyEncumbrance();
    g_camp_screen_0069c0f4->redraw_flags |= 0x0fffffff;
}

/* Commits the pending companion swap left by MarkCampCharacterPending:
   selects the new character and either queues its join event or opens the
   refusal dialog. Returns zero when a swap was pending and consumed, one when
   the caller may proceed. force ignores the same-character early out. */
// FUNCTION: WIZ8 0x005A5F30
bool ResolvePendingCampCharacter(bool force)
{
    unsigned int slot;
    wchar_t* text;
    W8MessageDialogBase* dialog;

    if (g_camp_character_pending_0069c104 != 0 &&
        (g_camp_character_0069c100 != g_review_character_0069c0f8 || force != 0)) {
        slot = CharacterPointerToPartySlot(g_camp_character_0069c100);
        SelectCampCharacter(slot);
        if (IsPartySlotEligible(giReviewCharSlot) != 0) {
            QueueCharacterEvent(g_camp_character_0069c100, g_effect_005ee6ec, 0,
                                g_effect_argument_005ed8cc, g_effect_argument_005ed914);
            return 0;
        }
        text = FormatWideString(gppStringList[0x24c4 / 4], g_camp_character_0069c100->name);
        dialog = static_cast<W8MessageDialogBase*>(CreateDialogByKind(1));
        dialog->SetClientExtent(0xfa, 200);
        dialog->SetMessage(text, 1, 0x32, 1, 0, 1, 1, 0, 0x15e);
        SetDialogDestroyCallback(dialog, 0);
        g_camp_screen_0069c0f4->dialog = dialog;
        ActivateDialogRegion(0x138);
        return 0;
    }
    return 1;
}

/* When the reviewed slot is one of the two companion slots and its bound NPC
   wants the item being used, the current character is remembered as pending a
   swap (cleared first so a failed attempt leaves none). */
// FUNCTION: WIZ8 0x005A6020
void MarkCampCharacterPending(W8ItemInstance* item)
{
    W8NpcState* npc;

    g_camp_character_pending_0069c104 = 0;
    if (item->iItemNo != -1 && (giReviewCharSlot == 0 || giReviewCharSlot == 1)) {
        npc = GetNpcState(g_status_685170.buffers.XChar[giReviewCharSlot].npc_index);
        if (npc != 0 && NpcWantsItem(npc, item) != 0) {
            g_camp_character_pending_0069c104 = 1;
            g_camp_character_0069c100 = g_review_character_0069c0f8;
        }
    }
}

/* Whether the camp item action may proceed for the reviewed slot: outside
   combat always; in combat the sleeping/paralyzed/unconscious/dead conditions
   and the equip-in-progress actions each carry their own refusal message. */
// FUNCTION: WIZ8 0x005A6090
bool IsCampActionAllowed(int party_slot)
{
    const wchar_t* message;
    W8MessageDialogBase* dialog;
    W8PartySlotRow* row;
    W8Character* character;

    if (gXStatus.fCombatMode == 0) {
        return 1;
    }
    character = &g_status_685170.buffers.Char[party_slot];
    if (character->uiCondition[W8_CONDITION_DEAD] != 0 ||
        character->uiCondition[W8_CONDITION_EXHAUSTED] != 0 ||
        character->uiCondition[W8_CONDITION_PARALYZED] != 0 ||
        character->uiCondition[W8_CONDITION_ASLEEP] != 0) {
        if (g_combat_state->equip_phase_a50 != 0) {
            return 1;
        }
        message = gppStringList[0x2428 / 4];
    } else {
        row = &g_status_685170.buffers.XChar[party_slot];
        if (g_combat_state->equip_phase_a50 == 0) {
            if (g_combat_state->characters[party_slot].dead_34 != 0) {
                if (row->pending_action == W8_ACTION_EQUIP) {
                    message = gppStringList[0x240c / 4];
                } else if (row->action_03d == W8_ACTION_EQUIP) {
                    message = gppStringList[0x2410 / 4];
                } else {
                    message = gppStringList[0x2408 / 4];
                }
            } else if (row->action_03d == W8_ACTION_EQUIP) {
                message = gppStringList[0x240c / 4];
            } else {
                message = gppStringList[0x2408 / 4];
            }
        } else {
            if (row->pending_action == W8_ACTION_EQUIP) {
                return 1;
            }
            if (row->action_03d == W8_ACTION_EQUIP) {
                message = gppStringList[0x2410 / 4];
            } else {
                message = gppStringList[0x2408 / 4];
            }
        }
    }
    dialog = static_cast<W8MessageDialogBase*>(CreateDialogByKind(1));
    dialog->SetClientExtent(0xfa, 200);
    dialog->SetMessage(message, 1, 0x32, 1, 0, 1, 1, 0, 0x15e);
    SetDialogDestroyCallback(dialog, 0);
    g_camp_screen_0069c0f4->dialog = dialog;
    ActivateDialogRegion(0x138);
    return 0;
}

// FUNCTION: WIZ8 0x005A6310
bool IsEquippableItemClass(W8ItemInstance* item)
{
    char equip_class = g_item_records[item->iItemNo].equip_class;
    if (equip_class != 2 && equip_class != 4) {
        return 0;
    }
    return 1;
}

/* Books a party slot's spell cast: stamps the row's cast-spell action with
   the spell id, power level and target, spends the spell points and fatigues
   the caster, then plays fizzle/learned/general-cast audio for the result. */
// FUNCTION: WIZ8 0x005A6340
int CommitPartySlotSpell(int party_slot, int spell_id, int power_level, W8CombatSlot* target)
{
    int cost;
    int result;
    W8PartySlotRow* row;

    StartBreathCycle(party_slot, 0);
    row = &g_status_685170.buffers.XChar[party_slot];
    row->pending_action = W8_ACTION_CAST_SPELL;
    row->attack_mode[0] = spell_id;
    row->attack_mode[1] = -1;
    row->pending_action_detail_015.spell.power_level = power_level;
    row->pending_action_detail_015.spell.unused = 0;
    row->target_out_of_combat = *target;
    result = ExecuteCharacterSpellCast(party_slot, spell_id, power_level, &cost, 0);
    SetPartySlotSpell(party_slot, spell_id, power_level, target);
    FatigueCharacter(party_slot, cost, 1, 0);
    if (result != 1) {
        SoundPlay("Data\\Sound\\Misc\\Spell Fizzle 01.wav", 0);
        return result;
    }
    if (spell_id == 0x17 && target->pPCItem->identified != 0) {
        SoundPlay("Data\\Sound\\Misc\\Spell Learned.wav", 0);
        return 1;
    }
    SoundPlay("Data\\Spells\\Sounds\\GeneralMagic.wav", 0);
    return 1;
}

/* Books a party slot's item use: stamps the row's use-item action with the
   item and target, resolves the use, stages the slot's item detail for the
   follow-up bookkeeping, and fatigues the user by the attempt's reported
   cost (or the standard use-item cost when no use happened). The audio cues
   mirror the spell path: fizzle, learned for a successful identify, general
   magic otherwise. */
// FUNCTION: WIZ8 0x005A6440
int CommitPartySlotItemUse(int party_slot, W8ItemInstance* item, W8CombatSlot* target)
{
    int uses;
    int result;
    W8PartySlotRow* row;
    W8ItemInstance* used;

    StartBreathCycle(party_slot, 0);
    row = &g_status_685170.buffers.XChar[party_slot];
    row->pending_action = W8_ACTION_USE_ITEM;
    row->attack_mode[0] = -1;
    row->attack_mode[1] = -1;
    row->pending_action_detail_015.item_use.kind = -1;
    row->pending_action_detail_015.item_use.item = item;
    row->target_out_of_combat = *target;
    result = UseItem(&g_status_685170.buffers.Char[party_slot], item, &uses);
    StagePartySlotItemUse(party_slot, item, target);
    if (uses == -1) {
        uses = CharacterActionFatigueCost(party_slot, W8_ACTION_USE_ITEM);
    }
    FatigueCharacter(party_slot, uses, 1, 0);
    if (result == 1) {
        used = target->pPCItem;
        if (g_item_records[used->iItemNo].spell_id == 0x17 && used->identified != 0) {
            SoundPlay("Data\\Sound\\Misc\\Spell Learned.wav", 0);
        } else {
            SoundPlay("Data\\Spells\\Sounds\\GeneralMagic.wav", 0);
        }
    } else {
        SoundPlay("Data\\Sound\\Misc\\Spell Fizzle 01.wav", 0);
    }
    return result;
}

// FUNCTION: WIZ8 0x005A6580
void BeginEndgameSequence(void)
{
    int fade_to_black = 0;
    int fade_code = 0x5dc;
    int endgame_variant = 0;

    g_status_685170.endgame_started_49c0 = 1;
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
    BeginScreenFade(fade_to_black, 0, fade_code, ShowEndingScreen, 1, endgame_variant);
}

/* Begin a timed full-screen fade: spawn a 640x480 colored quad over the UI,
   switch its blend shader for the requested ramp direction and seed the
   fade state. `callback` runs from UpdateScreenFade once the ramp
   finishes. `fade_to_black` selects the subtractive ramp (white quad
   darkening to black); `fade_out` selects the direction the opacity runs. */
// FUNCTION: WIZ8 0x005A6620
void BeginScreenFade(int fade_to_black, int fade_out, int duration, void (*callback)(void),
                     char flag, char arg_6)
{
    srShader shader;
    srVector4T<float> color;

    g_fade_duration_0069c118 = duration;
    g_fade_out_0069c120 = fade_out;
    g_fade_callback_0069c110 = callback;
    g_fade_flag_0069c114 = arg_6;
    g_level_block->review_transition_done_328 = 1;
    if (flag != 0) {
        SetFullscreenSceneLast(1);
    }
    if (fade_to_black != 0) {
        color.x = 1.0f;
        color.y = 1.0f;
        color.z = 1.0f;
    } else {
        color.x = 0.0f;
        color.y = 0.0f;
        color.z = 0.0f;
    }
    color.w = 1.0f;
    g_fade_overlay_0069c11c = CreateColoredPolygonSprite(0x280, 0x1e0, &color, 1);
    PositionToolTipNode(g_fade_overlay_0069c11c, 0, 0, 0);
    shader = static_cast<srMeshModel*>(g_fade_overlay_0069c11c->model())->getShader(0);
    if (fade_to_black == 0) {
        shader.value = (shader.value & ~0x6040) | 0xa0;
    } else {
        shader.value = (shader.value & ~0x20c0) | 0x4020;
    }
    static_cast<srMeshModel*>(g_fade_overlay_0069c11c->model())->setShader(shader, 0);
    static_cast<srMaterial*>(static_cast<srMeshModel*>(g_fade_overlay_0069c11c->model())
                                 ->getMaterial(0, static_cast<srMeshModel::e_side>(0)))
        ->setOpacity(fade_out != 0 ? 1.0f : 0.0f);
    g_fade_tick_base_0069c10c = GetTickCount();
}

/* Advance the pending screen fade: interpolate the overlay's opacity over
   g_fade_duration_0069c118 (reversed when fading back out) and render a
   frame per tick while g_fade_flag_0069c114 is set. On completion a
   fade-in snaps the quad opaque and renders twice, then the overlay is
   released and the stored callback runs. Returns g_fade_flag_0069c114. */
// FUNCTION: WIZ8 0x005A6790
unsigned char UpdateScreenFade(void)
{
    if (g_level_block->review_transition_done_328 == 0) {
        return 0;
    }
    unsigned long elapsed = GetTickCount() - g_fade_tick_base_0069c10c;
    if (g_fade_duration_0069c118 < elapsed) {
        g_level_block->review_transition_done_328 = 0;
        if (g_fade_out_0069c120 == 0) {
            static_cast<srMaterial*>(static_cast<srMeshModel*>(g_fade_overlay_0069c11c->model())
                                         ->getMaterial(0, static_cast<srMeshModel::e_side>(0)))
                ->setOpacity(1.0f);
            RenderFrame();
            RenderFrame();
        }
        g_fade_overlay_0069c11c->release();
        SetFullscreenSceneLast(0);
        if (g_fade_callback_0069c110 != 0) {
            g_fade_callback_0069c110();
        }
        return g_fade_flag_0069c114;
    }
    float progress = static_cast<float>(elapsed) / g_fade_duration_0069c118;
    srMaterial* material =
        static_cast<srMaterial*>(static_cast<srMeshModel*>(g_fade_overlay_0069c11c->model())
                                     ->getMaterial(0, static_cast<srMeshModel::e_side>(0)));
    if (g_fade_out_0069c120 == 0) {
        material->setOpacity(progress);
    } else {
        material->setOpacity(g_float_005ebb38 - progress);
    }
    if (g_fade_flag_0069c114 != 0) {
        RenderFrame();
    }
    return g_fade_flag_0069c114;
}

/* Start the party-death transition: Iron Man runs delete the current saves
   first, then the death sting and CombatLose playlist start while the input
   regions collapse to the modal review region and the fade carries the
   screen to DrawPartyDeathScreen. */
// FUNCTION: WIZ8 0x005A68C0
void BeginPartyDeath(void)
{
    if (g_level_block->review_transition_active) {
        return;
    }
    if (g_status_685170.iron_man != 0 && gXStatus.party_moving == 0) {
        DeleteCurrentSaveFiles();
    }
    if (gXStatus.fSurprisePossible != 0) {
        RestoreSurpriseView();
    }
    UpdateHeldItemCursor();
    SoundPlay("Data\\Sound\\Misc\\PartyDead.wav", 0);
    StartMusicResource("CombatLose.MPL", 0, 1);
    MSYS_Init();
    ResetRegions();
    ActivateDialogRegion(0x138);
    VideoRemoveToolTip();
    g_level_block->transition_pending = 1;
    BeginScreenFade(1, 0, 0x7d0, DrawPartyDeathScreen, 1, 0);
    g_level_block->review_transition_active = 1;
}

/* Pump the post-fade review/death state: on the first tick an armed ending
   autosave writes the next free Ending slot and reports it on the main-menu
   message line; a dirty level reloads; any queued key-down or button-up
   schedules the closing fade back through EndReviewTransition. */
// FUNCTION: WIZ8 0x005A6970
void PumpReviewTransition005A6970(void)
{
    InputAtom input;
    char name[260];
    bool exit_review;

    if (!g_level_block->review_transition_active ||
        g_level_block->review_transition_done_328 != 0) {
        return;
    }
    if (g_ending_autosave_0069c129 != 0) {
        if (FindFreeEndingSaveName(name) != 0) {
            SaveGame(name, 0);
            SetMainMenuMessage(
                FormatWideString(L"%s %S.%S", gppStringList[0x78b], name, g_save_extension));
        }
        g_ending_autosave_0069c129 = 0;
    }
    if (g_status_685170.current_level != -1) {
        LoadCurrentLevelData();
    }
    exit_review = false;
    while (DequeueEvent(&input) == 1) {
        switch (input.usEvent) {
        case KEY_DOWN:
        case LEFT_BUTTON_UP:
        case RIGHT_BUTTON_UP:
            exit_review = true;
            break;
        }
    }
    if (exit_review) {
        BeginScreenFade(0, 0, 0x320, EndReviewTransition, 1, 1);
    }
    RenderFrame();
}

/* Fade-completion callback for party death: draw the review backdrop and
   the centered party-death line over it, drop the radar/formation panels,
   then schedule the holding fade that waits for input. */
// FUNCTION: WIZ8 0x005A6A70
void DrawPartyDeathScreen(void)
{
    const wchar_t* text;

    DrawCatalogImageAndInvalidate(-14, 0x1df, 0, 0, 0, 0, 2, 0);
    if (gXStatus.party_moving != 0) {
        text = gppStringList[0x777];
    } else {
        text = gppStringList[0x778];
    }
    SetFont(g_level_load_font_69b7c0);
    gprintf(0x276 - StringPixLength(const_cast<wchar_t*>(text), g_level_load_font_69b7c0), 0x1c7,
            const_cast<wchar_t*>(text));
    SetRadarMapVisible(0);
    SetFormationBoardVisible(0);
    VideoRemoveToolTip();
    g_world_render_enabled_65970d = 0;
    BeginScreenFade(1, 1, 0x4b0, 0, 1, 1);
}

/* Fade-completion callback leaving the review/death screen: release the
   modal region, stop the ending voice-over, reset the main-game mode and
   leave the transition. The endgame path continues to the credits screen;
   party death just stops the playlist. */
// FUNCTION: WIZ8 0x005A6B20
void EndReviewTransition(void)
{
    ClearActiveRegionIfMatches(0x138);
    if (g_ending_sound_0069c124 != 0) {
        SoundStop(g_ending_sound_0069c124);
        g_ending_sound_0069c124 = 0;
    }
    g_world_render_enabled_65970d = 1;
    ResetMainGameMode();
    g_level_block->review_transition_active = 0;
    if (g_ending_screen_0069c128 != 0) {
        SetPendingScreenState(9);
        g_ending_screen_0069c128 = 0;
        return;
    }
    StopMusicPlaylist(1);
}

/* Fade-completion callback for the ending sequence: pick the ending's
   backdrop, string-table text and voice-over from the ending facts, draw
   them over a cleared screen and schedule the holding fade. Fact 0x1a2 is
   the losing ending - it swaps in the CombatLose playlist and skips the
   credits-screen handoff and autosave arming. */
// FUNCTION: WIZ8 0x005A6B90
void ShowEndingScreen(void)
{
    int fade_to_black;
    bool schedule_fade;
    char* music;
    char* sound;
    int image;
    wchar_t text[512];
    W8ControlsRect bounds;
    SOUNDPARMS parms;

    fade_to_black = 0;
    schedule_fade = true;
    music = "EndCredit.MPL";
    g_ending_screen_0069c128 = 1;
    g_ending_autosave_0069c129 = 1;
    if (GetFact(0x2f4) != 0) {
        image = 0x1e1;
        schedule_fade = false;
        wcscpy(text, gppStringList[0x787]);
        sound = "Data\\Sound\\NPCs\\VOC\\ENDGAME1.VOC";
    } else if (GetFact(0x219) != 0) {
        image = 0x1e2;
        wcscpy(text, gppStringList[0x788]);
        sound = "Data\\Sound\\NPCs\\VOC\\ENDGAME2.VOC";
    } else if (GetFact(0x21b) != 0) {
        image = 0x1e2;
        wcscpy(text, gppStringList[0x789]);
        sound = "Data\\Sound\\NPCs\\VOC\\ENDGAME3.VOC";
    } else if (GetFact(0x1a2) != 0) {
        image = 0x1e3;
        fade_to_black = 1;
        wcscpy(text, gppStringList[0x78a]);
        sound = "Data\\Sound\\NPCs\\VOC\\ENDGAME4.VOC";
        music = "CombatLose.MPL";
        g_ending_screen_0069c128 = 0;
        g_ending_autosave_0069c129 = 0;
    } else {
        image = 0x1df;
        sound = "";
        text[0] = 0;
    }
    DrawCatalogImageAndInvalidate(-14, image, 0, 0, 0, 0, 2, 0);
    bounds.left = 0x46;
    bounds.top = 0;
    bounds.right = 0x239;
    bounds.bottom = 0x1d0;
    {
        W8TextBuffer buffer(&bounds, text, g_options_detail_font_683614,
                            g_W8TextBufferLayoutMask005ED55C, 4);
        buffer.RenderToTarget(0, 0, -14);
    }
    SetRadarMapVisible(0);
    SetFormationBoardVisible(0);
    VideoRemoveToolTip();
    g_world_render_enabled_65970d = 0;
    if (*music != 0) {
        StartMusicResource(music, 0, 1);
    }
    if (*sound != 0) {
        memset(&parms, 0xff, sizeof(SOUNDPARMS));
        parms.uiVolume = g_settings_6850c8.voice_volume * 0x7f / 0xff;
        g_ending_sound_0069c124 = SoundPlayStreamedFile(sound, &parms);
    }
    if (schedule_fade) {
        BeginScreenFade(fade_to_black, 1, 0x4b0, 0, 1, 1);
    }
}
