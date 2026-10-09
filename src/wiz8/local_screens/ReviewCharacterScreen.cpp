#include <windows.h>
#include "wiz8/spell_ids.h"
#include "wiz8/conditions.h"
#include "wiz8/sgp_text.h"
#include "soundman.h"
#include "wiz8/integer_constants.h"
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
#include "wiz8/local_screens/MGSPortraits.h"
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
W8CampScreenState* g_camp_screen;
// GLOBAL: WIZ8 0x0064cbe8
int giReviewCharSlot = -1;
// GLOBAL: WIZ8 0x0069c0f8
W8Character* g_review_character;
// GLOBAL: WIZ8 0x0069c0fc
W8Character* g_camp_identifying_character; /* gpIdentifyingPC */
// GLOBAL: WIZ8 0x0069c100
W8Character* g_camp_character;
// GLOBAL: WIZ8 0x0069c104
bool g_camp_character_pending;
// GLOBAL: WIZ8 0x0069c108
unsigned int g_camp_item_region_set;
// GLOBAL: WIZ8 0x0069c10c
w8_ulong g_fade_tick_base;
// GLOBAL: WIZ8 0x0069c110
void (*g_fade_callback)(void);
// GLOBAL: WIZ8 0x0069c114
unsigned char g_fade_flag;
// GLOBAL: WIZ8 0x0069c118
unsigned int g_fade_duration;
// GLOBAL: WIZ8 0x0069c11c
stModelInstance2D* g_fade_overlay;
// GLOBAL: WIZ8 0x0069c120
int g_fade_out;
// GLOBAL: WIZ8 0x0069c124
unsigned int g_ending_sound;
// GLOBAL: WIZ8 0x0069c128
bool g_ending_screen;
// GLOBAL: WIZ8 0x0069c129
bool g_ending_autosave;
// GLOBAL: WIZ8 0x0069c40c
unsigned int g_camp_spell_region_sets[6];
// GLOBAL: WIZ8 0x0069c408
unsigned int g_camp_character_info_region_set;

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
Controls* g_camp_secondary_panel;

/* The camp screen's three panels and their controls: the top secondary panel
   carries the page tabs, help line, attribute rows and secondary labels; the
   bottom-left action panel carries two buttons; the right item-filter panel carries
   the six filters and the sort button. */
// GLOBAL: WIZ8 0x0069c42c
W8Widget* g_camp_info_labels[4];
// GLOBAL: WIZ8 0x0069c43c
W8TextControl* g_camp_page_tabs[2];
// GLOBAL: WIZ8 0x0069c444
W8HelpTextControl* g_camp_help_text;
// GLOBAL: WIZ8 0x0069c448
W8Widget* g_camp_stat_labels[7];
// GLOBAL: WIZ8 0x0069c464
Controls* g_camp_action_panel;
// GLOBAL: WIZ8 0x0069c468
W8TextControl* g_camp_action_buttons[2];
// GLOBAL: WIZ8 0x0069c470
W8TextControl* g_camp_item_filter_buttons[7];
// GLOBAL: WIZ8 0x0069c48c
Controls* g_camp_item_filter_panel;

/* The font-state palette index selected for each load category
   while the weight line is drawn; zero leaves the default palette in place. */
// GLOBAL: WIZ8 0x00648C48
int g_load_category_palettes[5] = {0xf, 3, 1, 5, 0};

/* One portrait frame per race and gender, race-major in threes. */
// GLOBAL: WIZ8 0x0064CDA0
int g_race_portrait_images[0x30] = {
    0x125, 0x126, 0x125, 0x127, 0x128, 0x127, 0x129, 0x12a, 0x129, 0x12b, 0x12c, 0x12b,
    0x12d, 0x12e, 0x12d, 0x12f, 0x130, 0x12f, 0x131, 0x132, 0x131, 0x133, 0x134, 0x133,
    0x135, 0x136, 0x135, 0x137, 0x138, 0x137, 0x139, 0x13a, 0x139, 0x13b, 0x13b, 0x13b,
    0x13c, 0x13c, 0x13c, 0x13d, 0x13d, 0x13d, 0x13e, 0x13e, 0x13e, 0x13f, 0x13f, 0x13f,
};

/* The seven attribute label message ids, drawn top to bottom on
   the secondary panel. The last two entries swap relative to attribute order. */
// GLOBAL: WIZ8 0x0064DD30
int g_attribute_label_ids[7] = {0x924, 0x925, 0x926, 0x927, 0x928, 0x92a, 0x929};

/* The help-line weight breakdown, "<personal>: n, <party>: n". */
// GLOBAL: WIZ8 0x0064DD4C
wchar_t g_format_s_colon_d_s_colon_d[] = L"%s: %d, %s: %d";

// GLOBAL: WIZ8 0x0064CBF0
W8CampScreenRegion g_camp_screen_regions[12] = {
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

/* 0x0064DD14, 0x0064DD20, 0x0064DD28, 0x00648164: the spell page's small
   formats - zero-padded cost, plain number, plain string and the realm label
   prefix. */
// GLOBAL: WIZ8 0x0064DD14
wchar_t g_format[] = L"%3.3d";
// GLOBAL: WIZ8 0x0064DD20
wchar_t g_format_d0[] = L"%3d";
// GLOBAL: WIZ8 0x0064DD28
wchar_t g_format_s0[] = L"%s:";
// GLOBAL: WIZ8 0x00648164
wchar_t g_format_s_colon[] = L"%s: ";

/* Enable or disable the six spell-realm scrollbars together. While enabling, a
   realm whose learned spells fit the eight visible rows keeps its bar off. */
// FUNCTION: WIZ8 0x005B71C0
void SetCampSpellRangesEnabled(bool enable)
{
    W8CampSpellRange* spell_range;
    int realm;
    int second;

    for (realm = 0; realm < W8_SPELL_REALM_COUNT; ++realm) {
        spell_range = g_camp_screen->spell_ranges[realm];
        spell_range->m_range->EnableRegionSet(enable);
        if (enable) {
            second = g_review_character->skill_unlocks[0x1c + spell_range->m_realm] - 8;
            if (second < 1) {
                spell_range->m_range->SetRangeEnabled(false);
            } else {
                spell_range->m_range->SetRangeEnabled(true);
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

    for (realm = 0; realm < W8_SPELL_REALM_COUNT; ++realm) {
        spell_range = g_camp_screen->spell_ranges[realm];
        spell_range->m_range->EnableRegionSet(true);
        second = g_review_character->skill_unlocks[0x1c + spell_range->m_realm] - 8;
        if (second < 1) {
            spell_range->m_range->SetRangeEnabled(false);
        } else {
            spell_range->m_range->SetRangeEnabled(true);
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
    W8Character* character = g_review_character;
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

    SetFont(g_wiz_text_font_secondary);
    if ((g_camp_screen->redraw_flags & W8_CAMP_REDRAW_RESISTANCES) != 0 &&
        !gXStatus.fSpellCastMode) {
        DrawCampResistances();
    }
    if (g_camp_screen->redraw_flags == W8_CAMP_REDRAW_ALL) {
        DrawCatalogImageAndInvalidate(FRAME_BUFFER, 0x140, 0, 2, 0, 0xa5, VO_BLT_SRCTRANSPARENCY,
                                      0);
    }
    for (realm = 0; realm < W8_SPELL_REALM_COUNT; ++realm) {
        if ((g_camp_screen->redraw_flags & (W8_CAMP_REDRAW_REALM_SPELLS_FIRST << realm)) == 0) {
            continue;
        }
        left = (realm % 3) * 0xd5 + 3;
        top = (realm / 3) * 0x8c + 0xaa;
        if (character->skill_unlocks[0x1c + realm] != 0) {
            DrawCatalogImageAndInvalidate(FRAME_BUFFER, 0x140, 0, 3, left, top,
                                          VO_BLT_SRCTRANSPARENCY, 0);
            SetFontObjectPalette16BPP(g_wiz_text_font_secondary,
                                      g_font_state_palettes[W8_FONT_PALETTE_GREEN]);
            gprintf(left + 0x1b, top + 8, Wiz8ToSgpWideText(g_format_s0), gppStringList[0x8c7]);
            gprintf(left + 0x72, top + 8, gppStringList[0x8c9]);
            SetFontObjectPalette16BPP(g_wiz_text_font_secondary, g_wiz_text_font_secondary_palette);
            width = StringPixLengthArg(g_wiz_text_font_secondary, wcslen(gppStringList[0x8c7]) + 2,
                                       Wiz8ToSgpWideText(g_format_s_colon), gppStringList[0x8c7]);
            gprintf(left + 0x1b + width, top + 8, Wiz8ToSgpWideText(g_format_d0),
                    character->skills[W8_SKILL_FIRE_MAGIC + realm].level);
            width = StringPixLengthArg(
                g_wiz_text_font_secondary, 7, Wiz8ToSgpWideText(g_format_d_slash_d),
                GetCharacterRealmSpellPoints(character, static_cast<W8SpellRealm>(realm)),
                character->sp_max[realm]);
            gprintf(left + 0xc8 - width, top + 8, Wiz8ToSgpWideText(g_format_d_slash_d),
                    GetCharacterRealmSpellPoints(character, static_cast<W8SpellRealm>(realm)),
                    character->sp_max[realm]);
            visible = character->skill_unlocks[0x1c + realm];
            if (visible >= 8) {
                visible = 8;
            }
            row_top = top + 0x19;
            for (row = 0; static_cast<unsigned int>(row) < visible; ++row) {
                spell_id =
                    g_camp_screen->learned_spells
                        .spell_ids_by_realm[realm]
                                           [g_camp_screen->learned_spells.scroll[realm] + row];
                if (g_camp_screen->hover_region == static_cast<unsigned int>(realm + 0x119) &&
                    g_camp_screen->selected_spell_row == row) {
                    palette = g_font_state_palettes[W8_FONT_PALETTE_YELLOW];
                } else if (g_spell_records[spell_id].spell_point_cost <=
                               character->iSPLeft[realm] &&
                           SpellUsableNow(spell_id, false)) {
                    palette = g_wiz_text_font_secondary_palette;
                } else {
                    palette = g_font_state_palettes[W8_FONT_PALETTE_RED];
                }
                SetFontObjectPalette16BPP(g_wiz_text_font_secondary, palette);
                width =
                    StringPixLengthArg(g_wiz_text_font_secondary, 3, Wiz8ToSgpWideText(g_format),
                                       g_spell_records[spell_id].spell_point_cost);
                gprintf(left + 0x1c, row_top, Wiz8ToSgpWideText(g_format_s),
                        g_spell_records[spell_id].display_name);
                gprintf(left + 0xb1 - width, row_top, Wiz8ToSgpWideText(g_format_d0),
                        g_spell_records[spell_id].spell_point_cost);
                row_top += 0xd;
            }
            SetFontObjectPalette16BPP(g_wiz_text_font_secondary, g_wiz_text_font_secondary_palette);
        }
        state = g_camp_screen;
        state->spell_ranges[realm]->UpdateRange(true);
    }
    for (realm = 0; realm < W8_SPELL_REALM_COUNT; ++realm) {
        if (g_camp_screen->dialog != 0 && realm != 0 && realm != 3) {
            continue;
        }
        if ((g_camp_screen->redraw_flags &
             ((W8_CAMP_REDRAW_REALM_SPELLS_FIRST << realm) | 0x10000)) == 0) {
            continue;
        }
        animation = &g_spell_realm_animations[realm];
        if (character->sp_max[realm] != 0) {
            DrawCatalogImageAndInvalidate(
                FRAME_BUFFER, animation->image, 0, g_camp_screen->animation_frames[realm],
                (realm % 3) * 213 + 5, (realm / 3) * 140 + 0xac, VO_BLT_SRCTRANSPARENCY, 0);
        } else {
            DrawCatalogImageAndInvalidate(FRAME_BUFFER, animation->image, 0,
                                          animation->initial_frame, (realm % 3) * 213 + 5,
                                          (realm / 3) * 140 + 0xac, VO_BLT_SRCTRANSPARENCY, 0);
        }
    }
}

/* The six resistance bars along the spell page's top strip: each bar clips its
   fill to the learned portion, the overflow to the bonus band or the shortfall
   to the gap, then right-aligns the value inside the bar. */
// FUNCTION: WIZ8 0x005B7790
void DrawCampResistances(void)
{
    W8Character* character = g_review_character;
    const W8SpellRealmAnimation* animation;
    wchar_t* text;
    int index;
    int left;
    int top;
    int width;

    DrawCatalogImageAndInvalidate(FRAME_BUFFER, 0x140, 0, 0, 0x136, 0, VO_BLT_SRCTRANSPARENCY, 0);
    text = gppStringList[0x8ca];
    width = StringPixLengthArg(
        g_wiz_text_font_secondary, wcslen(text),
        reinterpret_cast< // reinterpret-ok: SGP's historical UINT16 text ABI stores wchar_t data
            CHAR16*>(text));
    gprintf((0x134 - width) / 2 + 0x144, 0x1e, text);
    for (index = 0; index < 6; ++index) {
        animation = &g_spell_realm_animations[index];
        left = (index & 1) * 156 + 0x144;
        top = (index >> 1) * 28 + 0x35;
        DrawCatalogImage(FRAME_BUFFER, animation->image, 0, animation->initial_frame, left, top,
                         VO_BLT_SRCTRANSPARENCY, 0);
        DrawCampValueBar(character->resistances[index].total, character->resistances[index].base,
                         left + 0x18, top + 4);
        width = StringPixLengthArg(g_wiz_text_font_secondary, 5, Wiz8ToSgpWideText(g_format_d),
                                   character->resistances[index].total);
        gprintf(left + 0x93 - width, top + 3, Wiz8ToSgpWideText(g_format_d),
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

    PushButtonSoundScheme(0, true);
    realm = region->callback_id;
    row = (GetAtomCursorY(event) - region->y1 - 1) / 0xd;
    visible = g_review_character->skill_unlocks[0x1c + realm] -
              g_camp_screen->learned_spells.scroll[realm];
    if (visible >= 8) {
        visible = 8;
    }
    if (row >= static_cast<int>(visible)) {
        row = -1;
    }
    if (row != g_camp_screen->selected_spell_row) {
        g_camp_screen->selected_spell_row = row;
        g_camp_screen->redraw_flags |= W8_CAMP_REDRAW_REALM_SPELLS_FIRST << realm;
    }
    if (event->usEvent > RIGHT_BUTTON_UP) {
        if (event->usEvent == MOUSE_POS) {
            if ((region->flags & W8_REGION_MOUSE_TRANSITION_MASK) != 0) {
                g_camp_screen->redraw_flags |= W8_CAMP_REDRAW_REALM_SPELLS_FIRST << realm;
            }
            return 0;
        }
        if (event->usEvent == MOUSE_WHEEL) {
            delta = GetMouseWheelDeltaValue(event->usParam);
            if (delta > 0) {
                for (count = delta; count != 0; --count) {
                    g_camp_screen->spell_ranges[realm]->m_range->Decrement();
                }
                delta = 0;
            }
            if (delta < 0) {
                for (count = -delta; count != 0; --count) {
                    g_camp_screen->spell_ranges[realm]->m_range->Increment();
                }
            }
            return 1;
        }
        return 0;
    }
    if (event->usEvent == RIGHT_BUTTON_UP) {
        if ((region->flags & W8_REGION_RIGHT_BUTTON_HELD) != 0 && row != -1) {
            OpenSpellInfoDialog(
                g_camp_screen->learned_spells
                    .spell_ids_by_realm[realm][g_camp_screen->learned_spells.scroll[realm] + row]);
        }
        return 1;
    }
    if (event->usEvent == LEFT_BUTTON_DOWN) {
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        return 1;
    }
    if (event->usEvent != LEFT_BUTTON_UP) {
        if (event->usEvent != RIGHT_BUTTON_DOWN) {
            return 0;
        }
        region->flags |= W8_REGION_RIGHT_BUTTON_HELD;
        return 1;
    }
    if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0 && row != -1) {
        OpenSpellInfoDialog(
            g_camp_screen->learned_spells
                .spell_ids_by_realm[realm][g_camp_screen->learned_spells.scroll[realm] + row]);
        return 1;
    }
    return 1;
}

/* Open the spell info dialog for one spell id off the camp spell lists. */
// FUNCTION: WIZ8 0x005B7BB0
void OpenSpellInfoDialog(unsigned int spell_id)
{
    W8SpellInfoDialog* dialog = new W8SpellInfoDialog(spell_id);
    dialog->SetText(&g_empty_wide_string);
    DisplayCampDialog(dialog);
}

/* The items-page redraw driver, run once per update: each pending group of
   redraw flags is cleared by repainting that block, and the two bottom/right
   panels repaint when their own bits are raised. */
// FUNCTION: WIZ8 0x005b7d10
void RedrawCampItemsPage(void)
{
    SetFont(g_wiz_text_font_secondary);
    SetObjectShade(g_wiz_text_font_secondary_object, 4);
    if ((g_camp_screen->redraw_flags & W8_CAMP_REDRAW_CHARACTER_INFO) != 0) {
        DrawCampCharacterInfo();
    }
    if ((g_camp_screen->item_redraw_flags & W8_CAMP_ITEM_REDRAW_BACKPACK) != 0) {
        DrawCampBackpackItems();
    }
    if ((g_camp_screen->item_redraw_flags & W8_CAMP_ITEM_REDRAW_EQUIPMENT) != 0) {
        DrawCampEquipmentItems();
    }
    if ((g_camp_screen->item_redraw_flags & W8_CAMP_ITEM_REDRAW_POOL) != 0) {
        DrawCampItemPool();
        g_camp_screen->item_range->UpdateRange(true);
    } else {
        g_camp_screen->item_range->UpdateRange(false);
    }
    if ((g_camp_screen->redraw_flags & W8_CAMP_REDRAW_ACTION_PANEL) != 0) {
        g_camp_action_panel->Invalidate(0);
        g_camp_action_panel->Redraw();
    } else {
        g_camp_action_panel->Redraw();
    }
    if ((g_camp_screen->redraw_flags & W8_CAMP_REDRAW_REALM_TABS) != 0) {
        g_camp_item_filter_panel->Invalidate(0);
        g_camp_item_filter_panel->Redraw();
    } else {
        g_camp_item_filter_panel->Redraw();
    }
}

/* The character block of the items page: name-free value fields, the
   experience bar, the six realm icons with their point pools, the attribute
   rows and the armor summaries. */
// FUNCTION: WIZ8 0x005b7e00
void DrawCampCharacterInfo(void)
{
    W8CampScreenState* state = g_camp_screen;
    W8Character* character = g_review_character;
    int index;
    int realm;
    int top;
    unsigned int filled;
    SGPRect clip;
    SGPRect previous_clip;

    if (state->info_page != W8_CAMP_INFO_PAGE_ITEMS) {
        state->character_info->Invalidate(0);
        InvalidateCampPanel();
        return;
    }
    DrawCatalogImageAndInvalidate(FRAME_BUFFER, 0x114, 0, 0, 0x136, 0, VO_BLT_SRCTRANSPARENCY, 0);
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
            DrawCatalogImageAndInvalidate(FRAME_BUFFER, 0x11d, 0, 0, 0x1c0, 0x26,
                                          VO_BLT_SRCTRANSPARENCY, 0);
            SetClippingRect(&previous_clip);
        }
    }
    DrawRcsText(gppStringList[0x91b], 0x159, 10, 0xce,
                g_W8TextBufferAlignMiddle | g_W8TextBufferAlignCenter);
    DrawRcsText(gppStringList[0x91c], 0x15e, 0x18, 0x65,
                g_W8TextBufferAlignLeft | g_W8TextBufferAlignMiddle);
    DrawRcsText(gppStringList[0x91d], 0x15e, 0x26, 0x65,
                g_W8TextBufferAlignLeft | g_W8TextBufferAlignMiddle);
    FormatUnsignedIntegerWithCommas(state->text_buffer, character->experience);
    DrawRcsText(state->text_buffer, 0x1c0, 0x18, 0x67,
                g_W8TextBufferAlignMiddle | g_W8TextBufferAlignCenter);
    FormatUnsignedIntegerWithCommas(state->text_buffer, character->experience_goal);
    DrawRcsText(state->text_buffer, 0x1c0, 0x26, 0x67,
                g_W8TextBufferAlignMiddle | g_W8TextBufferAlignCenter);
    DrawRcsText(gppStringList[0x91e], 0x144, 0x3a, 0x45,
                g_W8TextBufferAlignLeft | g_W8TextBufferAlignMiddle);
    DrawRcsText(gppStringList[0x91f], 0x144, 0x48, 0x45,
                g_W8TextBufferAlignLeft | g_W8TextBufferAlignMiddle);
    DrawRcsText(gppStringList[0x920], 0x144, 0x56, 0x45,
                g_W8TextBufferAlignLeft | g_W8TextBufferAlignMiddle);
    swprintf(state->text_buffer, g_format_d_slash_d, character->hp_current, character->uiHPMax);
    DrawRcsText(state->text_buffer, 0x18d, 0x3a, 0x2c,
                g_W8TextBufferAlignMiddle | g_W8TextBufferAlignCenter);
    swprintf(state->text_buffer, g_format_d_slash_d, character->stamina, character->uiStaminaMax);
    DrawRcsText(state->text_buffer, 0x18d, 0x48, 0x2c,
                g_W8TextBufferAlignMiddle | g_W8TextBufferAlignCenter);
    if (character->load_category != 0) {
        SetFontObjectPalette16BPP(
            g_wiz_text_font_secondary,
            g_font_state_palettes[g_load_category_palettes[character->load_category]]);
    }
    swprintf(state->text_buffer, g_format_d_slash_d, character->total_carried_weight / 10,
             character->carrying_capacity / 10);
    DrawRcsTextJustified(state->text_buffer, 0x18d, 0x56, 0x2c, 0xc,
                         g_W8TextBufferAlignMiddle | g_W8TextBufferAlignCenter);
    swprintf(state->text_buffer, g_format_s_colon_d_s_colon_d, gppStringList[0x8c0],
             character->inventory_weight / 10, gppStringList[0x8c1],
             character->party_weight_share / 10);
    g_camp_help_text->SetRegionHelp(state->text_buffer);
    SetFontObjectPalette16BPP(g_wiz_text_font_secondary, g_wiz_text_font_secondary_palette);
    DrawRcsText(gppStringList[0x921], 0x144, 0x72, 0x75,
                g_W8TextBufferAlignMiddle | g_W8TextBufferAlignCenter);
    top = 0x3a;
    for (index = 0; index < 7; ++index) {
        DrawRcsText(gppStringList[g_attribute_label_ids[index]], 0x1c2, top, 0x4c,
                    g_W8TextBufferAlignLeft | g_W8TextBufferAlignMiddle);
        swprintf(state->text_buffer, g_format_d, character->attributes[index].effective);
        DrawRcsText(state->text_buffer, 0x212, top, 0x14,
                    g_W8TextBufferAlignMiddle | g_W8TextBufferAlignCenter);
        top += 0xe;
    }
    top = 0xd;
    for (realm = 0; realm < W8_SPELL_REALM_COUNT; ++realm) {
        unsigned int frame;
        if (character->sp_max[realm] == 0) {
            frame = g_spell_realm_animations[realm].frame_count;
        } else {
            swprintf(state->text_buffer, g_format_d_slash_d,
                     GetCharacterRealmSpellPoints(character, static_cast<W8SpellRealm>(realm)),
                     character->sp_max[realm]);
            DrawTallRcsText(state->text_buffer, 0x242, top, 0x32,
                            g_W8TextBufferAlignMiddle | g_W8TextBufferAlignCenter);
            frame = g_spell_realm_animations[realm].initial_frame;
        }
        DrawCatalogImageAndInvalidate(FRAME_BUFFER, g_character_resistance_images[realm], 0, frame,
                                      0x22d, top, VO_BLT_SRCTRANSPARENCY, 0);
        top += 0x18;
    }
    DrawRcsText(gppStringList[0x922], 0x144, 0x80, 0x45,
                g_W8TextBufferAlignLeft | g_W8TextBufferAlignMiddle);
    swprintf(state->text_buffer, g_format_d, character->armor_class_total);
    DrawRcsText(state->text_buffer, 0x18d, 0x80, 0x2c,
                g_W8TextBufferAlignMiddle | g_W8TextBufferAlignCenter);
    DrawRcsText(gppStringList[0x923], 0x144, 0x8e, 0x45,
                g_W8TextBufferAlignLeft | g_W8TextBufferAlignMiddle);
    swprintf(state->text_buffer, g_format_d, character->armor_class_average);
    DrawRcsText(state->text_buffer, 0x18d, 0x8e, 0x2c,
                g_W8TextBufferAlignMiddle | g_W8TextBufferAlignCenter);
    InvalidateCampPanel();
}

/* The eight backpack cells under the character block, flag bits 1..8 of
   item_redraw_flags; bit 0 repaints the header strip and caption. */
// FUNCTION: WIZ8 0x005b8440
void DrawCampBackpackItems(void)
{
    W8CampScreenState* state = g_camp_screen;
    W8Character* character = g_review_character;
    unsigned int slot;
    int left;
    int top;
    int right;
    int bottom;
    int item_id;

    if ((state->item_redraw_flags & W8_CAMP_ITEM_REDRAW_BACKPACK_HEADER) != 0) {
        DrawCatalogImageAndInvalidate(FRAME_BUFFER, 0x114, 0, 1, 0, 0xa5, VO_BLT_SRCTRANSPARENCY,
                                      0);
        DrawRcsBoldText(gppStringList[0x92b], 0xc, 0xae, 0x5d,
                        g_W8TextBufferAlignMiddle | g_W8TextBufferAlignCenter);
    }
    for (slot = 0; slot < 8; ++slot) {
        left = (slot & 1) * 0x31 + 0xb;
        top = (slot >> 1) * 0x39 + 0xc0;
        right = left + 0x2e;
        bottom = top + 0x36;
        if ((state->item_redraw_flags & (W8_CAMP_ITEM_REDRAW_BACKPACK_CELL_FIRST << slot)) == 0) {
            continue;
        }
        InvalidateRegion(left, top, right, bottom, 0);
        item_id = character->backpack[slot].iItemNo;
        if (item_id == -1 || CanCharacterUseItem(character, item_id)) {
            BlitCatalogSurfaceRectTo16BPP(FRAME_BUFFER, left, top, right, bottom, 0x1b6, 0, 0);
        } else {
            DrawCatalogImage(FRAME_BUFFER, 0x11a, 0, 0, left, top, VO_BLT_SRCTRANSPARENCY, 0);
        }
        if (item_id != -1) {
            DrawCatalogImage(FRAME_BUFFER, g_item_video_objects.GetOrCreateVideoObject(item_id), 0,
                             0, left + 1, top + 1, VO_BLT_SRCTRANSPARENCY, 0);
            DrawCampItemQuantity(&character->backpack[slot], left + 1, top + 0x28, 0x2c);
            if (!character->backpack[slot].identified) {
                DrawCatalogImage(FRAME_BUFFER, 0x11b, 0, 0, left, top, VO_BLT_SRCTRANSPARENCY, 0);
            }
        }
        if (item_id != -1 && g_item_records[item_id].binds_on_equip != 0 &&
            character->backpack[slot].bound) {
            DrawCatalogImage(FRAME_BUFFER, 0x115, 0, 0x32, left, top, VO_BLT_SRCTRANSPARENCY, 0);
        }
        if (state->hover_region == slot + 0xf4 &&
            (item_id != -1 ||
             (g_status.item_in_cursor && state->item_action != W8_CAMP_ITEM_ACTION_MOVE))) {
            DrawCatalogImage(FRAME_BUFFER, 0x115, 0, 0x30, left, top, VO_BLT_SRCTRANSPARENCY, 0);
        }
    }
}

/* The twelve equipment cells; bit 9 of item_redraw_flags repaints the paper
   doll backdrop and race/gender portrait, bits 10..21 the slots. */
// FUNCTION: WIZ8 0x005b8690
void DrawCampEquipmentItems(void)
{
    W8CampScreenState* state = g_camp_screen;
    W8Character* character = g_review_character;
    const W8CampScreenRegion* region;
    unsigned int slot;
    int item_id;
    int frame;

    if ((state->item_redraw_flags & W8_CAMP_ITEM_REDRAW_PAPER_DOLL) != 0) {
        DrawCatalogImageAndInvalidate(FRAME_BUFFER, 0x114, 0, 2, 0x71, 0xa5, VO_BLT_SRCTRANSPARENCY,
                                      0);
        DrawCatalogImageAndInvalidate(
            FRAME_BUFFER,
            g_race_portrait_images[character->iRace * W8_RACE_GNOME + character->gender], 0, 0,
            0xc2, 0xa5, VO_BLT_SRCTRANSPARENCY, 0);
        state->redraw_flags |= W8_CAMP_REDRAW_ACTION_PANEL;
    }
    for (slot = 0; slot < 12; ++slot) {
        if ((state->item_redraw_flags & (W8_CAMP_ITEM_REDRAW_EQUIPMENT_CELL_FIRST << slot)) == 0) {
            continue;
        }
        region = &g_camp_screen_regions[slot];
        InvalidateRegion(region->x, region->y, region->x + region->width,
                         region->y + region->height, 0);
        BlitCatalogSurfaceRectTo16BPP(FRAME_BUFFER, region->x, region->y, region->x + region->width,
                                      region->y + region->height, 0x1b6, 0, 0);
        item_id = character->EquippedItem[slot].iItemNo;
        if (item_id == -1) {
            if (slot == 7 || slot == 9) {
                int paired =
                    character->EquippedItem[GetPairedEquipSlot(static_cast<W8EquipSlot>(slot))]
                        .iItemNo;
                if (paired != -1 && (g_item_records[paired].flags & W8_ITEM_FLAG_TWO_HANDED) != 0) {
                    DrawCatalogImageAndInvalidate(FRAME_BUFFER, 0x146, 0, 0, region->x, region->y,
                                                  VO_BLT_SRCTRANSPARENCY, 0);
                }
            }
        } else {
            DrawCatalogImageAndInvalidate(FRAME_BUFFER,
                                          g_item_video_objects.GetOrCreateVideoObject(item_id), 0,
                                          1, region->x, region->y, VO_BLT_SRCTRANSPARENCY, 0);
            DrawCampItemQuantity(&character->EquippedItem[slot], region->x + 2,
                                 region->y + region->height - 0xd, region->width - 4);
            if (!character->EquippedItem[slot].identified) {
                DrawCatalogImage(FRAME_BUFFER, 0x11b, 0,
                                 static_cast<short>(region->unidentified_frame), region->x,
                                 region->y, VO_BLT_SRCTRANSPARENCY, 0);
            }
        }
        switch (slot) {
        case W8_EQUIP_SLOT_HEAD:
            frame = 0;
            break;
        case W8_EQUIP_SLOT_TORSO:
            frame = 1;
            break;
        case W8_EQUIP_SLOT_HANDS:
            frame = 3;
            break;
        case W8_EQUIP_SLOT_LEGS:
            frame = 2;
            break;
        case 0xb:
            frame = 4;
            break;
        default:
            goto no_armor_label;
        }
        swprintf(state->text_buffer, g_format_d, character->armor_class_by_location[frame]);
        DrawCatalogImageAndInvalidate(FRAME_BUFFER, 0x11c, 0, 0, region->x + 2, region->y + 1,
                                      VO_BLT_SRCTRANSPARENCY, 0);
        DrawRcsText(state->text_buffer, region->x + 2, region->y + 2, 0x11,
                    g_W8TextBufferAlignMiddle | g_W8TextBufferAlignCenter);
    no_armor_label:
        DrawCatalogImageAndInvalidate(FRAME_BUFFER, 0x115, 0, region->frame + 3, region->x - 2,
                                      region->y - 2, VO_BLT_SRCTRANSPARENCY, 0);
        if (item_id != -1 && g_item_records[item_id].binds_on_equip != 0 &&
            character->EquippedItem[slot].bound) {
            DrawCatalogImageAndInvalidate(FRAME_BUFFER, 0x115, 0, region->frame + 2, region->x - 2,
                                          region->y - 2, VO_BLT_SRCTRANSPARENCY, 0);
        }
        if (state->hover_region == slot + 0xfc) {
            if (!g_status.item_in_cursor) {
                if (character->EquippedItem[slot].iItemNo == -1) {
                    continue;
                }
            } else if (state->item_action == W8_CAMP_ITEM_ACTION_MOVE ||
                       !CanEquipItemInSlot(character, g_status.item_in_hand.iItemNo,
                                           static_cast<unsigned char>(slot), true) ||
                       !CanCharacterUseItem(character, g_status.item_in_hand.iItemNo)) {
                if (!g_status.item_in_cursor || state->item_action != W8_CAMP_ITEM_ACTION_MOVE) {
                    continue;
                }
                if (character->EquippedItem[slot].iItemNo == -1) {
                    continue;
                }
            }
            frame = region->frame;
        } else {
            if (!g_status.item_in_cursor || state->item_action == W8_CAMP_ITEM_ACTION_MOVE ||
                !IsPartySlotEligible(giReviewCharSlot) ||
                !CanEquipItemInSlot(character, g_status.item_in_hand.iItemNo,
                                    static_cast<unsigned char>(slot), true) ||
                !CanCharacterUseItem(character, g_status.item_in_hand.iItemNo)) {
                continue;
            }
            frame = region->frame + 1;
        }
        DrawCatalogImageAndInvalidate(FRAME_BUFFER, 0x115, 0, frame, region->x - 2, region->y - 2,
                                      VO_BLT_SRCTRANSPARENCY, 0);
    }
}

/* The shared party item pool: the bottom-right grid of up to eight visible
   cells, plus the header strip and the gold counter when bit 22 is raised. */
// FUNCTION: WIZ8 0x005b8b20
void DrawCampItemPool(void)
{
    W8CampScreenState* state = g_camp_screen;
    W8Character* character = g_review_character;
    unsigned int visible;
    unsigned int i;
    int left;
    int top;
    int right;
    int bottom;
    W8ItemInstance* item;

    if ((state->item_redraw_flags & W8_CAMP_ITEM_REDRAW_POOL_HEADER) != 0) {
        DrawCatalogImageAndInvalidate(FRAME_BUFFER, 0x114, 0, 3, 0x1de, 0xa5,
                                      VO_BLT_SRCTRANSPARENCY, 0);
        DrawCatalogImageAndInvalidate(FRAME_BUFFER, 0x118, 0, 0, 0x202, 0x1a6,
                                      VO_BLT_SRCTRANSPARENCY, 0);
        state->redraw_flags |= W8_CAMP_REDRAW_REALM_TABS;
    }
    if (!g_status.game_started) {
        return;
    }
    if ((state->item_redraw_flags & W8_CAMP_ITEM_REDRAW_POOL_HEADER) != 0) {
        DrawRcsBoldText(gppStringList[0x92c], 0x1e6, 0xae, 0x8d,
                        g_W8TextBufferAlignMiddle | g_W8TextBufferAlignCenter);
        FormatUnsignedIntegerWithCommas(state->text_buffer, g_status.party_gold);
        DrawRcsText(state->text_buffer, 0x229, 0x1a9, 0x30,
                    g_W8TextBufferAlignMiddle | g_W8TextBufferAlignCenter);
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
        item = &g_status.party_item_pool[state->item_list[state->item_scroll + i]];
        if ((state->item_redraw_flags & (W8_CAMP_ITEM_REDRAW_POOL_CELL_FIRST << i)) == 0) {
            continue;
        }
        InvalidateRegion(left, top, right, bottom, 0);
        if (!CanCharacterUseItem(character, item->iItemNo)) {
            DrawCatalogImage(FRAME_BUFFER, 0x11a, 0, 0, left, top, VO_BLT_SRCTRANSPARENCY, 0);
        } else {
            BlitCatalogSurfaceRectTo16BPP(FRAME_BUFFER, left, top, right, bottom, 0x1b6, 0, 0);
        }
        DrawCatalogImage(FRAME_BUFFER, g_item_video_objects.GetOrCreateVideoObject(item->iItemNo),
                         0, 0, left + 1, top + 1, VO_BLT_SRCTRANSPARENCY, 0);
        DrawCampItemQuantity(item, left + 1, top + 0x28, 0x2c);
        if (!item->identified) {
            DrawCatalogImage(FRAME_BUFFER, 0x11b, 0, 0, left, top, VO_BLT_SRCTRANSPARENCY, 0);
        }
        if (g_item_records[item->iItemNo].binds_on_equip != 0 && item->bound) {
            DrawCatalogImage(FRAME_BUFFER, 0x115, 0, 0x32, left, top, VO_BLT_SRCTRANSPARENCY, 0);
        }
        if (state->hover_region == i + 0x108) {
            DrawCatalogImage(FRAME_BUFFER, 0x115, 0, 0x30, left, top, VO_BLT_SRCTRANSPARENCY, 0);
        }
    }
    for (i = state->item_list_count - state->item_scroll; i < 8; ++i) {
        left = (i & 1) * 0x31 + 0x200;
        top = (i >> 1) * 0x39 + 0xc0;
        right = left + 0x2e;
        bottom = top + 0x36;
        if ((state->item_redraw_flags & (W8_CAMP_ITEM_REDRAW_POOL_CELL_FIRST << i)) != 0) {
            InvalidateRegion(left, top, right, bottom, 0);
            BlitCatalogSurfaceRectTo16BPP(FRAME_BUFFER, left, top, right, bottom, 0x1b6, 0, 0);
            if (state->hover_region == i + 0x108 && g_status.item_in_cursor &&
                state->item_action != W8_CAMP_ITEM_ACTION_MOVE) {
                DrawCatalogImage(FRAME_BUFFER, 0x115, 0, 0x30, left, top, VO_BLT_SRCTRANSPARENCY,
                                 0);
            }
        }
    }
}

/* The stack count or charge figure in a cell's bottom-right corner, keyed off
   the item record's quantity kind; charge figures ride the alternate font
   palettes and restore the default afterwards. */
// FUNCTION: WIZ8 0x005b8ec0
void DrawCampItemQuantity(W8ItemInstance* item, int left, int top, int width)
{
    W8CampScreenState* state = g_camp_screen;
    int height;

    switch (g_item_records[item->iItemNo].quantity_kind) {
    case W8_ITEM_QUANTITY_STACK:
        if (item->stack_count > 1) {
            swprintf(state->text_buffer, g_format_d, item->stack_count);
            DrawRcsText(state->text_buffer, left, top, width,
                        g_W8TextBufferAlignMiddle | g_W8TextBufferAlignRight);
        }
        break;
    case W8_ITEM_QUANTITY_CHARGES:
    case W8_ITEM_QUANTITY_USES:
        SetFontObjectPalette16BPP(g_wiz_text_font_secondary,
                                  g_font_state_palettes[W8_FONT_PALETTE_BLUE]);
        if (!item->identified) {
            swprintf(state->text_buffer, L"?");
        } else {
            swprintf(state->text_buffer, g_format_d, item->uses_or_charges);
        }
        height = GetFontHeight(g_wiz_text_font_secondary);
        DrawRcsTextJustified(state->text_buffer, left, top, width, height,
                             g_W8TextBufferAlignMiddle | g_W8TextBufferAlignRight);
        break;
    case W8_ITEM_QUANTITY_SHOTS:
        SetFontObjectPalette16BPP(g_wiz_text_font_secondary,
                                  g_font_state_palettes[W8_FONT_PALETTE_YELLOW]);
        swprintf(state->text_buffer, g_format_d, item->uses_or_charges);
        height = GetFontHeight(g_wiz_text_font_secondary);
        DrawRcsTextJustified(state->text_buffer, left, top, width, height,
                             g_W8TextBufferAlignMiddle | g_W8TextBufferAlignRight);
        break;
    }
    SetFontObjectPalette16BPP(g_wiz_text_font_secondary, g_wiz_text_font_secondary_palette);
}

// FUNCTION: WIZ8 0x005b3150
W8CampCharacterInfo::W8CampCharacterInfo() : Controls(0x136, 0, 0x280, 0xa5, 0x122, 0, 0)
{
    AcquireRegionSet(&g_camp_character_info_region_set);
    m_combat_view = true;
    m_stats_tab = new W8TextControl(this, -1, 8, 0x8b, 0, 0, 0x121, 0, 15, 16, 17, 19, 18);
    m_stats_tab->m_listener = this;
    m_stats_tab->EnableRegionHelp(0x960);
    m_combat_tab = new W8TextControl(this, -1, 8, 0x8b, 0, 0, 0x121, 0, 10, 11, 12, 14, 13);
    m_combat_tab->m_listener = this;
    m_combat_tab->EnableRegionHelp(0x95f);
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
    m_stats_tab->SetActive(enabled);
    m_combat_tab->SetActive(!enabled);
    m_values[0]->SetActive(enabled);
    m_values[1]->SetActive(enabled);
    m_values[2]->SetActive(enabled);
    m_values[3]->SetActive(enabled);
    if (enabled) {
        m_catalogImage = 0;
    } else if (g_review_character->armor_class_components[W8_AC_COMPONENT_REFLEXTION] > 0) {
        m_catalogImage = 2;
    } else {
        m_catalogImage = 1;
    }
    Invalidate(0);
}

// FUNCTION: WIZ8 0x005b3470
void W8CampCharacterInfo::OnPrimary(W8TextControl* control)
{
    SetCombatView(control == m_combat_tab);
}

// GLOBAL: WIZ8 0x0061e798
unsigned short g_camp_armor_class_labels[12] = {1052, 1053, 1054, 1055, 1056, 1057,
                                                1058, 1059, 1060, 1061, 1062, 1063};

// FUNCTION: WIZ8 0x005b34a0
void W8CampCharacterInfo::Redraw()
{
    bool redraw = m_fEnabled && m_fDirty;
    if (!m_combat_view &&
        ((g_review_character->armor_class_components[W8_AC_COMPONENT_REFLEXTION] > 0 &&
          m_catalogImage != 2) ||
         (g_review_character->armor_class_components[W8_AC_COMPONENT_REFLEXTION] <= 0 &&
          m_catalogImage == 2))) {
        SetCombatView(false);
    }
    Controls::Redraw();
    if (!redraw)
        return;
    InvalidateCampPanel();
    DrawRcsText(gppStringList[0x935], 0x15e, 0x84, 0x4e,
                g_W8TextBufferAlignMiddle | g_W8TextBufferAlignLeft);
    swprintf(g_camp_screen->text_buffer, L"%d", g_review_character->kill_count);
    DrawRcsText(g_camp_screen->text_buffer, 0x1ae, 0x84, 0x20,
                g_W8TextBufferAlignCenter | g_W8TextBufferAlignMiddle);
    DrawRcsText(gppStringList[0x936], 0x15e, 0x92, 0x4e,
                g_W8TextBufferAlignMiddle | g_W8TextBufferAlignLeft);
    swprintf(g_camp_screen->text_buffer, L"%d", g_review_character->death_count);
    DrawRcsText(g_camp_screen->text_buffer, 0x1ae, 0x92, 0x20,
                g_W8TextBufferAlignCenter | g_W8TextBufferAlignMiddle);
    if (m_combat_view) {
        DrawRcsText(gppStringList[0x8b0], 0x15b, 10, 0x11d,
                    g_W8TextBufferAlignCenter | g_W8TextBufferAlignMiddle);
        DrawRcsText(gppStringList[0x8b1], 0x15e, 0x30, 0x4e,
                    g_W8TextBufferAlignMiddle | g_W8TextBufferAlignLeft);
        swprintf(g_camp_screen->text_buffer, L"%d", g_review_character->initiative);
        DrawRcsText(g_camp_screen->text_buffer, 0x1ae, 0x30, 0x20,
                    g_W8TextBufferAlignCenter | g_W8TextBufferAlignMiddle);
        if (g_review_character->EquippedItem[W8_EQUIP_SLOT_PRIMARY_WEAPON].iItemNo == -1 &&
            g_review_character->EquippedItem[W8_EQUIP_SLOT_SECONDARY_WEAPON].iItemNo == -1) {
            DrawRcsText(gppStringList[0x8b5], 0x1d6, 0x22, 0x50,
                        g_W8TextBufferAlignCenter | g_W8TextBufferAlignMiddle);
            DrawRcsText(gppStringList[0x8b4], 0x228, 0x22, 0x50,
                        g_W8TextBufferAlignCenter | g_W8TextBufferAlignMiddle);
        } else {
            DrawRcsText(gppStringList[0x8b2], 0x1d6, 0x22, 0x50,
                        g_W8TextBufferAlignCenter | g_W8TextBufferAlignMiddle);
            DrawRcsText(gppStringList[0x8b3], 0x228, 0x22, 0x50,
                        g_W8TextBufferAlignCenter | g_W8TextBufferAlignMiddle);
        }
        DrawRcsText(gppStringList[0x8b6], 0x1f8, 0x30, 0x5e,
                    g_W8TextBufferAlignCenter | g_W8TextBufferAlignMiddle);
        DrawRcsText(gppStringList[0x8bd], 0x1f8, 0x3e, 0x5e,
                    g_W8TextBufferAlignCenter | g_W8TextBufferAlignMiddle);
        DrawRcsText(gppStringList[0x8bc], 0x1f8, 0x4c, 0x5e,
                    g_W8TextBufferAlignCenter | g_W8TextBufferAlignMiddle);
        DrawRcsText(gppStringList[0x8ba], 0x1f8, 0x5a, 0x5e,
                    g_W8TextBufferAlignCenter | g_W8TextBufferAlignMiddle);
        DrawRcsText(gppStringList[0x8bb], 0x1f8, 0x68, 0x5e,
                    g_W8TextBufferAlignCenter | g_W8TextBufferAlignMiddle);
        DrawRcsText(gppStringList[0x8b7], 0x1f8, 0x76, 0x5e,
                    g_W8TextBufferAlignCenter | g_W8TextBufferAlignMiddle);
        DrawRcsText(gppStringList[0x8b8], 0x1f8, 0x84, 0x5e,
                    g_W8TextBufferAlignCenter | g_W8TextBufferAlignMiddle);
        DrawRcsText(gppStringList[0x8b9], 0x1f8, 0x92, 0x5e,
                    g_W8TextBufferAlignCenter | g_W8TextBufferAlignMiddle);
        bool unknown_partner =
            ItemHasSingledOutGenericName(
                g_review_character->EquippedItem[W8_EQUIP_SLOT_PRIMARY_WEAPON].iItemNo) &&
            !g_review_character->EquippedItem[W8_EQUIP_SLOT_SECONDARY_WEAPON].identified;
        for (unsigned int hand = 0; hand < 2; ++hand) {
            W8HandAttack* attack = &g_review_character->Hand[hand];
            if (!attack->in_play)
                continue;
            int x = hand ? 600 : 0x1d6;
            if (g_review_character->EquippedItem[hand + 6].iItemNo != -1 &&
                (!g_review_character->EquippedItem[hand + 6].identified ||
                 (hand == 0 && unknown_partner))) {
                for (int row = 0; row < 8; ++row) {
                    DrawRcsText(L"?", x, 0x30 + row * 14, 0x20,
                                g_W8TextBufferAlignCenter | g_W8TextBufferAlignMiddle);
                }
                wcscpy(g_camp_screen->text_buffer, gppStringList[0x95c]);
                wcscat(g_camp_screen->text_buffer, L"?");
                wcscat(g_camp_screen->text_buffer, gppStringList[0x95b]);
                wcscat(g_camp_screen->text_buffer, L"?");
                wcscat(g_camp_screen->text_buffer, gppStringList[0x95a]);
                wcscat(g_camp_screen->text_buffer, L"?");
                m_values[hand + 2]->SetRegionHelp(g_camp_screen->text_buffer);
                wcscpy(g_camp_screen->text_buffer, gppStringList[0x959]);
                wcscat(g_camp_screen->text_buffer, L"?");
                wcscat(g_camp_screen->text_buffer, gppStringList[0x95a]);
                wcscat(g_camp_screen->text_buffer, L"?");
                m_values[hand]->SetRegionHelp(g_camp_screen->text_buffer);
                continue;
            }
            W8Dice dice;
            GetCharacterHandDamageDice(g_review_character, hand, &dice);
            unsigned int damage_bonus = GetCharacterHandDamageBonus(g_review_character, hand);
            unsigned int minimum = (dice.Minimum() * (100 + damage_bonus) + 50) / 100;
            unsigned int maximum = (dice.Maximum() * (100 + damage_bonus) + 50) / 100;
            if (minimum < 2)
                minimum = 1;
            if (maximum < 2)
                maximum = 1;
            int hit_bonus = attack->hit_bonus + g_review_character->bonus.hit_bonus;
            int skill_bonus =
                (attack->attack_score < 0 ? attack->attack_score - 2 : attack->attack_score + 2) /
                5;
            swprintf(g_camp_screen->text_buffer, L"%+d",
                     attack->damage_bonus + g_review_character->bonus.damage_bonus);
            DrawRcsText(g_camp_screen->text_buffer, x, 0x30, 0x20,
                        g_W8TextBufferAlignCenter | g_W8TextBufferAlignMiddle);
            swprintf(g_camp_screen->text_buffer, L"%d-%d", minimum, maximum);
            DrawRcsText(g_camp_screen->text_buffer, x, 0x3e, 0x20,
                        g_W8TextBufferAlignCenter | g_W8TextBufferAlignMiddle);
            wcscpy(g_camp_screen->text_buffer, gppStringList[0x95c]);
            wcscat(g_camp_screen->text_buffer, FormatWideString(L" %d, ", dice.Minimum()));
            wcscat(g_camp_screen->text_buffer, gppStringList[0x95b]);
            wcscat(g_camp_screen->text_buffer, FormatWideString(L" %d, ", dice.Maximum()));
            wcscat(g_camp_screen->text_buffer, gppStringList[0x95a]);
            wcscat(g_camp_screen->text_buffer, FormatWideString(L" %+d%%", damage_bonus));
            m_values[hand + 2]->SetRegionHelp(g_camp_screen->text_buffer);
            swprintf(g_camp_screen->text_buffer, L"%d", skill_bonus + hit_bonus);
            DrawRcsText(g_camp_screen->text_buffer, x, 0x4c, 0x20,
                        g_W8TextBufferAlignCenter | g_W8TextBufferAlignMiddle);
            wcscpy(g_camp_screen->text_buffer, gppStringList[0x959]);
            wcscat(g_camp_screen->text_buffer, FormatWideString(L" %d, ", skill_bonus));
            wcscat(g_camp_screen->text_buffer, gppStringList[0x95a]);
            wcscat(g_camp_screen->text_buffer, FormatWideString(L" %+d", hit_bonus));
            m_values[hand]->SetRegionHelp(g_camp_screen->text_buffer);
            swprintf(g_camp_screen->text_buffer, L"%d", attack->attacks);
            DrawRcsText(g_camp_screen->text_buffer, x, 0x5a, 0x20,
                        g_W8TextBufferAlignCenter | g_W8TextBufferAlignMiddle);
            swprintf(g_camp_screen->text_buffer, L"%d", attack->swings);
            DrawRcsText(g_camp_screen->text_buffer, x, 0x68, 0x20,
                        g_W8TextBufferAlignCenter | g_W8TextBufferAlignMiddle);
            swprintf(g_camp_screen->text_buffer, L"%+d", hit_bonus);
            DrawRcsText(g_camp_screen->text_buffer, x, 0x76, 0x20,
                        g_W8TextBufferAlignCenter | g_W8TextBufferAlignMiddle);
            swprintf(g_camp_screen->text_buffer, L"%+d",
                     attack->attack_bonus + g_review_character->bonus.attack_bonus);
            DrawRcsText(g_camp_screen->text_buffer, x, 0x84, 0x20,
                        g_W8TextBufferAlignCenter | g_W8TextBufferAlignMiddle);
            swprintf(g_camp_screen->text_buffer, L"%+d%%", damage_bonus);
            DrawRcsText(g_camp_screen->text_buffer, x, 0x92, 0x20,
                        g_W8TextBufferAlignCenter | g_W8TextBufferAlignMiddle);
        }
    } else {
        DrawRcsText(gppStringList[0x8be], 0x15b, 10, 0x11d,
                    g_W8TextBufferAlignCenter | g_W8TextBufferAlignMiddle);
        for (unsigned int component = 0; component < 12; ++component) {
            if (component == 11 &&
                g_review_character->armor_class_components[W8_AC_COMPONENT_REFLEXTION] == 0)
                continue;
            int y = (component % 6) * 14 + 0x22;
            int label_x = component / 6 ? 0x1ef : 0x15e;
            int value_x = component / 6 ? 600 : 0x1c7;
            DrawRcsText(gppStringList[g_camp_armor_class_labels[component]], label_x, y, 0x67,
                        g_W8TextBufferAlignMiddle | g_W8TextBufferAlignLeft);
            int value = g_review_character->armor_class_components[component];
            if (value) {
                swprintf(g_camp_screen->text_buffer, L"%+d", value);
                DrawRcsText(g_camp_screen->text_buffer, value_x, y, 0x20,
                            g_W8TextBufferAlignCenter | g_W8TextBufferAlignMiddle);
            }
        }
        DrawRcsText(gppStringList[0x8bf], 0x1da, 0x84, 0x7c,
                    g_W8TextBufferAlignMiddle | g_W8TextBufferAlignLeft);
        swprintf(g_camp_screen->text_buffer, L"%d%%", g_review_character->damage_reduction);
        DrawRcsText(g_camp_screen->text_buffer, 600, 0x84, 0x20,
                    g_W8TextBufferAlignCenter | g_W8TextBufferAlignMiddle);
    }
}

W8CampItemRange::W8CampItemRange()
{
    m_range = new W8RangeControl(0x263, 0xc1, 0x275, 0x1a1, &g_camp_item_region_set);
    m_range->SetEnabled(true);
    m_range->m_listener = this;
}

// FUNCTION: WIZ8 0x005a34c0
void W8CampItemRange::OnRangeChanged(W8RangeControl*)
{
    g_camp_screen->item_scroll = m_range->m_value << 1;
    if (gfKeyState[VK_CONTROL]) {
        g_camp_screen->redraw_flags |= W8_CAMP_REDRAW_ALL;
        return;
    }
    g_camp_screen->item_redraw_flags |= W8_CAMP_ITEM_REDRAW_POOL;
}

// FUNCTION: WIZ8 0x005b7090
W8CampSpellRange::W8CampSpellRange(W8SpellRealm realm)
{
    m_realm = realm;
    int x = (realm % 3) * 0xd5;
    int y = (realm / 3) * 0x8c;
    m_range = new W8RangeControl(x + 0xbb, y + 0xc3, x + 0xcd, y + 0x129,
                                 &g_camp_spell_region_sets[realm]);
    m_range->m_listener = this;
    m_range->SetEnabled(true);
}

// FUNCTION: WIZ8 0x005b7160
W8CampSpellRange::~W8CampSpellRange()
{
    delete m_range;
}

// FUNCTION: WIZ8 0x005b7180
void W8CampSpellRange::OnRangeChanged(W8RangeControl*)
{
    g_camp_screen->learned_spells.scroll[m_realm] = m_range->m_value;
    g_camp_screen->redraw_flags |= W8_CAMP_REDRAW_REALM_SPELLS_FIRST << m_realm;
}

/* Lifecycle record 6's initializer - the camp record, which is what
   W8_SCREEN_CAMP selects. It drops the camp screen's state pointer rather than
   releasing it; the block is owned by the enter/leave pair. */
// FUNCTION: WIZ8 0x005a3500
unsigned char CampScreenInitialize(void)
{
    g_camp_screen = 0;
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
        const W8CampScreenRegion& region = g_camp_screen_regions[index];
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
    g_review_character = static_cast<W8Character*>(g_current_screen_state.parameter_3);
    giReviewCharSlot = g_current_screen_state.parameter_2;
    unsigned char initial_item_action;
    if (g_current_screen_state.parameter_4 == 0) {
        g_camp_identifying_character = 0;
        initial_item_action = W8_CAMP_ITEM_ACTION_NONE;
    } else {
        g_camp_identifying_character = g_current_screen_state.parameter_4;
        initial_item_action = W8_CAMP_ITEM_ACTION_IDENTIFY_SPELL;
        SoundPlay(s_general_magic_sound, 0);
        SetTargetingMode(W8_TARGET_NEED_ITEM);
    }
    ClearItemDrag();
    if (!g_camp_screen) {
        g_camp_screen = static_cast<W8CampScreenState*>(malloc(sizeof(W8CampScreenState)));
        if (!g_camp_screen) {
            if (IsMessageBoxActive()) {
                CloseMessageBox();
            }
            BeginCombatRound();
            RequestScreenTransition();
            return 0;
        }
        memset(g_camp_screen, 0, sizeof(W8CampScreenState));
    }
    SetClippingRegionAndImageWidth(0x500, 0, 0, 0x280, 0x1e0);
    g_camp_screen->item_action = initial_item_action;
    MSYS_Init();
    for (unsigned int filter = 0; filter < W8_CAMP_ITEM_FILTER_COUNT; ++filter) {
        g_camp_screen->item_filters[filter] = 0;
    }
    g_camp_screen->item_timer_active = false;
    g_camp_screen->item_timer_expired = false;
    CreateCampButtonPanel();
    CreateCampActionPanel();
    CreateItemsTabPanel();
    CreateCampSecondaryPanel();
    CreateRcsLevelUpPanel();
    CreateRcsDismissPanel();
    g_camp_screen->page = W8_CAMP_PAGE_ITEMS;
    g_camp_screen->info_page = W8_CAMP_INFO_PAGE_ITEMS;
    g_camp_screen->item_range = new W8CampItemRange;
    for (int range_index = 0; range_index < 6; ++range_index) {
        g_camp_screen->spell_ranges[range_index] =
            new W8CampSpellRange(static_cast<W8SpellRealm>(range_index));
    }
    g_camp_screen->effect_list = 0;
    g_camp_screen->effect_items_only = 1;
    g_camp_screen->effect_filter = W8_CAMP_EFFECT_FILTER_ALL;
    g_camp_screen->stats_range = new W8CampStatsRange;
    g_camp_screen->stats_controls = new W8CampStatsControls;
    g_camp_screen->character_info = new W8CampCharacterInfo;
    g_camp_screen->item_icons_drawn = false;
    ActivateCampPage();
    g_camp_screen->animation_timer = SetCountdownClock(50);
    for (unsigned int animation = 0; animation < 6; ++animation) {
        g_camp_screen->animation_frames[animation] =
            Random(g_spell_realm_animations[animation].frame_count);
    }
    if (gXStatus.fCombatMode && g_combat_state->equip_phase) {
        if (g_combat_state->equip_pending == 1) {
            for (int slot = 0; slot < 8; ++slot) {
                if (g_status.buffers.XChar[slot].fOccupied && IsPartySlotEligible(slot) &&
                    g_status.buffers.XChar[slot].pending_action == W8_ACTION_EQUIP) {
                    swprintf(g_camp_screen->text_buffer, L"%s %s", g_status.buffers.Char[slot].name,
                             gppStringList[0x919]);
                    goto show_equip_message;
                }
            }
        } else {
            unsigned char count = 0;
            for (int slot = 0; slot < 8; ++slot) {
                if (g_status.buffers.XChar[slot].fOccupied && IsPartySlotEligible(slot) &&
                    g_status.buffers.XChar[slot].pending_action == W8_ACTION_EQUIP) {
                    ++count;
                    if (count == 1) {
                        swprintf(g_camp_screen->text_buffer, L"%s",
                                 g_status.buffers.Char[slot].name);
                    } else {
                        if (count == g_combat_state->equip_pending) {
                            wcscat(g_camp_screen->text_buffer, L" ");
                            wcscat(g_camp_screen->text_buffer,
                                   FormatWideString(gppStringList[0x918],
                                                    g_status.buffers.Char[slot].name));
                            goto show_equip_message;
                        }
                        wcscat(g_camp_screen->text_buffer, L", ");
                        wcscat(g_camp_screen->text_buffer, g_status.buffers.Char[slot].name);
                    }
                }
            }
        }
        srAssertFail("fFoundEquipChar",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\ReviewCharacterScreen.cpp", 0x16f,
                     0);
    show_equip_message:
        ShowCampNoticeLine(g_camp_screen->text_buffer, 0, true, false);
    }
    g_camp_screen->redraw_flags |= W8_CAMP_REDRAW_ALL;
    ResetTransientRenderScenes();
    SetPrimarySurfaceTextureHint2Enabled(false);
    if (!g_status.game_started) {
        StartMusicResource("MainMenu.MPL", 1, 1);
    }
    return 1;
}

// FUNCTION: WIZ8 0x005a3ae0
void CampScreenFrame(void)
{
    if (g_dev_mode) {
        RequestExitScreen();
    }
    if (IsMessageBoxActive()) {
        ProcessMessageBoxInput();
    }
    ServiceMusicPlaylist();
    if (g_camp_screen->dialog && !ProcessDialogInput(g_camp_screen->dialog)) {
        ClearActiveRegionIfMatches(0x138);
        delete g_camp_screen->dialog;
        g_camp_screen->dialog = 0;
        g_camp_screen->redraw_flags |= W8_CAMP_REDRAW_ALL;
    }
    if (gXStatus.level_up_notice && g_suspended_screen_id != W8_SCREEN_CHARACTER &&
        !gXStatus.fCombatMode && !IsScreenTransitionPending()) {
        RefreshLevelUpReadyNotices();
    }
    POINT point;
    SGPMouseGetPos(&point);
    g_camp_screen->hover_region = UpdateRegionMousePosition(point.x, point.y);
    InputAtom input;
    while (DequeueEvent(&input) == 1) {
        if (!DispatchRegionInput(&input) && input.usEvent == KEY_DOWN) {
            if (input.usParam == ESC) {
                if (g_camp_screen->item_action == W8_CAMP_ITEM_ACTION_NONE ||
                    g_camp_screen->page != W8_CAMP_PAGE_ITEMS) {
                    if (!g_camp_character_pending) {
                        if (IsMessageBoxActive()) {
                            CloseMessageBox();
                        }
                        BeginCombatRound();
                        RequestScreenTransition();
                    } else {
                        SelectCampCharacter(CharacterPointerToPartySlot(g_camp_character));
                        if (!IsPartySlotEligible(giReviewCharSlot)) {
                            wchar_t* text =
                                FormatWideString(gppStringList[0x931], g_camp_character->name);
                            ShowCampNoticeLine(text, 0, true, false);
                        } else {
                            QueueCharacterEvent(g_camp_character, g_effect36, 0, g_effect_argument0,
                                                g_character_event_full_volume);
                        }
                    }
                } else {
                    SetCampItemActionMode(W8_CAMP_ITEM_ACTION_NONE);
                }
            } else if (input.usParam == 'P') {
                if (g_status.game_started) {
                    SortPartyItemPool();
                }
            } else if (input.usParam == 'X' && gfKeyState[VK_MENU] && !gfKeyState[VK_CONTROL] &&
                       !gfKeyState[VK_SHIFT]) {
                ShowCampNoticeLine(gppStringList[0x832], OnQuitGameDialogClosed, true, true);
            }
        }
    }
    if (g_camp_screen->page == W8_CAMP_PAGE_SPELLS &&
        !ClockIsTicking(g_camp_screen->animation_timer)) {
        for (unsigned int realm = 0; realm < W8_SPELL_REALM_COUNT; ++realm) {
            ++g_camp_screen->animation_frames[realm];
            if (g_camp_screen->animation_frames[realm] ==
                g_spell_realm_animations[realm].frame_count) {
                g_camp_screen->animation_frames[realm] = 0;
            }
        }
        g_camp_screen->animation_timer = SetCountdownClock(50);
        g_camp_screen->redraw_flags |= 0x10000;
    }
    if (g_camp_screen->page == W8_CAMP_PAGE_ITEMS && g_camp_screen->item_timer_active &&
        !g_camp_screen->item_timer_expired && !ClockIsTicking(g_camp_screen->item_timer)) {
        g_camp_screen->item_timer_expired = true;
        g_camp_screen->item_redraw_flags |= W8_CAMP_ITEM_REDRAW_EQUIPMENT;
    }
    if (g_camp_screen->header_mode == W8_CAMP_HEADER_SUMMARY) {
        gXStatus.character_event_queue->ProcessDeferredCharacterEvents();
        UpdateCharacterEventState();
    }
    DrawCampScreen();
}

// FUNCTION: WIZ8 0x005a3ee0
unsigned char CampScreenLeave(int)
{
    DeactivateCampPage();
    SetFontObjectPalette16BPP(g_smfnt_font, g_font_palette_smfnt);
    SetFontObjectPalette16BPP(g_calligraphy_font, g_font_palette_calligraphy);
    SetFontObjectPalette16BPP(g_calligraphy_shadow_font, g_font_palette_calligraphy_shadow);
    SetFontObjectPalette16BPP(g_wiz_text_font, g_font_palette_wiz_text);
    DestroyCampButtonPanel();
    ReleaseCampActionPanel();
    ReleaseItemsTabPanel();
    ReleaseCampSecondaryPanel();
    DestroyRcsLevelUpPanel();
    DestroyRcsDismissPanel();
    delete g_camp_screen->item_range;
    if (g_camp_screen->dialog) {
        ClearActiveRegionIfMatches(0x138);
        delete g_camp_screen->dialog;
    }
    for (int realm = 0; realm < W8_SPELL_REALM_COUNT; ++realm) {
        delete g_camp_screen->spell_ranges[realm];
    }
    delete g_camp_screen->stats_range;
    delete g_camp_screen->stats_controls;
    delete g_camp_screen->character_info;
    free(g_camp_screen);
    g_camp_screen = 0;
    MSYS_Shutdown();
    ResetRegions();
    if (gXStatus.fCampMode) {
        gXStatus.dialogue_sync_pending = true;
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
    if (g_camp_character_pending) {
        SelectCampCharacter(CharacterPointerToPartySlot(g_camp_character));
        if (IsPartySlotEligible(giReviewCharSlot)) {
            QueueCharacterEvent(g_camp_character, g_effect36, 0, g_effect_argument0,
                                g_character_event_full_volume);
            return;
        }
        wchar_t* text = FormatWideString(gppStringList[0x931], g_camp_character->name);
        ShowCampNoticeLine(text, 0, true, false);
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
    W8CampScreenState* state = g_camp_screen;
    unsigned int index;

    NoOp();
    if (!gfKeyState[VK_CONTROL] && state->item_icons_drawn) {
        state->redraw_flags |= W8_CAMP_REDRAW_ALL;
        state->item_icons_drawn = false;
    }
    if (state->redraw_flags != 0 || state->item_redraw_flags != 0 || IsMessageBoxActive() ||
        g_status.item_in_cursor) {
        if (state->redraw_flags == W8_CAMP_REDRAW_ALL) {
            state->item_redraw_flags = 0xffffffff;
            if (state->dialog != 0) {
                state->dialog->m_dirty_flags |= W8_DIALOG_DIRTY_REDRAW;
            }
        }
        DrawCampHeader();
        switch (state->page) {
        case W8_CAMP_PAGE_ITEMS:
            RedrawCampItemsPage();
            break;
        case W8_CAMP_PAGE_STATS:
            DrawCampStatsPage();
            break;
        case W8_CAMP_PAGE_SKILLS:
            DrawCampSkillsPage();
            break;
        case W8_CAMP_PAGE_SPELLS:
            DrawCampSpellPages();
            break;
        case W8_CAMP_PAGE_REGENERATION:
            DrawCampRegenStats();
            break;
        }
        RefreshCampItemActions((state->redraw_flags & W8_CAMP_REDRAW_ITEM_ACTIONS) != 0);
        SetFont(g_calligraphy_font);
        SetObjectShade(g_calligraphy_font_object, 4);
        state->item_redraw_flags = 0;
        state->redraw_flags = 0;
        if (IsMessageBoxActive()) {
            RenderMessageBox();
            if (!IsMessageBoxActive()) {
                state->redraw_flags |= W8_CAMP_REDRAW_ALL;
            }
        }
    }
    state->character_info->Redraw();
    if (state->page == W8_CAMP_PAGE_ITEMS) {
        RefreshCampActionPanel(false);
        RefreshItemsTabPanel(false);
        RefreshCampSecondaryPanel(false);
        state->item_range->m_range->Redraw();
        if (gfKeyState[VK_CONTROL] && state->dialog == 0) {
            DrawCampItemIcons();
            state->item_icons_drawn = true;
        }
    } else if (state->page == W8_CAMP_PAGE_STATS) {
        state->stats_range->UpdateRange(false);
        state->stats_controls->Redraw();
    } else if (state->page == W8_CAMP_PAGE_SPELLS) {
        for (index = 0; index < 6; ++index) {
            state->spell_ranges[index]->UpdateRange(false);
        }
    }
    RefreshCampItemActions(false);
    if (!gXStatus.fCombatMode) {
        if (giReviewCharSlot != -1) {
            if (g_status.game_started || (g_previous_screen_id == W8_SCREEN_PARTY_SELECTION &&
                                          PartySelectionInReviewMode())) {
                UpdateRcsLevelUpPanel();
            }
            if (gXStatus.fCombatMode) {
                goto done;
            }
        }
        if (g_status.game_started) {
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
    if (giReviewCharSlot != -1 && g_status.buffers.XChar[giReviewCharSlot].npc_index != -1) {
        DisableRegionInput(0xf2);
        return;
    }
    EnableRegionInput(0xf2);
}

/* Switch the active camp page: tear the old page down, record the new index,
   bring it up and repaint the whole character block. */
// FUNCTION: WIZ8 0x005a4540
void SwitchCampPage(W8CampPage page)
{
    DeactivateCampPage();
    g_camp_screen->page = page;
    ActivateCampPage();
    g_camp_screen->redraw_flags |= W8_CAMP_REDRAW_ALL;
}

/* Enter the active camp page: resets the item action mode when leaving the
   items page, gates the party-portrait regions on whether a character is being
   reviewed, then enables the page's own region sets and controls. */
// FUNCTION: WIZ8 0x005a45b0
void ActivateCampPage(void)
{
    W8CampScreenState* state = g_camp_screen;
    unsigned int index;

    RegionSetEnable(0x29);
    if (state->page != W8_CAMP_PAGE_ITEMS) {
        SetCampItemActionMode(W8_CAMP_ITEM_ACTION_NONE);
    }
    if (giReviewCharSlot == -1) {
        for (index = 0; index < 8; ++index) {
            DisableRegionInput(index + 0xea);
        }
    } else if (g_status.buffers.XChar[giReviewCharSlot].npc_index == -1) {
        EnableRegionInput(0xf2);
    } else {
        DisableRegionInput(0xf2);
    }
    state->header_mode = W8_CAMP_HEADER_SUMMARY;
    state->redraw_flags |= 0x7ff;
    switch (state->page) {
    case W8_CAMP_PAGE_ITEMS:
        RegionSetEnable(0x2a);
        EnableCampActionButtons();
        UpdateCampItemFilters();
        EnableCampSecondaryPanel();
        SetCampInfoPageMode(state->info_page);
        state->item_range->m_range->EnableRegionSet(true);
        if (g_status.game_started) {
            RegionSetEnable(0x2b);
            state->item_scroll = 0;
            RebuildCampItemList();
            return;
        }
        break;
    case W8_CAMP_PAGE_STATS:
        RebuildCampEffectList();
        state->stats_range->m_range->EnableRegionSet(true);
        state->stats_controls->EnableRegionSet(true);
        state->character_info->SetEnabled(true);
        return;
    case W8_CAMP_PAGE_SKILLS:
        CreateCampSkillRegions();
        return;
    case W8_CAMP_PAGE_SPELLS:
        RegionSetEnable(0x2c);
        BuildLearnedSpellState(&state->learned_spells, g_review_character);
        RefreshCampSpellRanges();
        gXStatus.fSpellCastMode = false;
        state->selected_spell_row = -1;
        break;
    default:
        break;
    }
}

/* Leave the active camp page: disables the page's region sets and controls and
   releases the page's rebuilt state - the effect list on the stats page, the
   skill-improvement flags on the skills page. */
// FUNCTION: WIZ8 0x005a4770
void DeactivateCampPage(void)
{
    W8CampScreenState* state = g_camp_screen;
    unsigned int index;

    switch (state->page) {
    case W8_CAMP_PAGE_ITEMS:
        RegionSetDisable(0x2a);
        DisableItemsRealmTabs();
        DisableCampActionButtons();
        DisableCampSecondaryPanel();
        state->character_info->SetEnabled(false);
        state->item_range->m_range->EnableRegionSet(false);
        if (g_status.game_started) {
            RegionSetDisable(0x2b);
            return;
        }
        break;
    case W8_CAMP_PAGE_STATS:
        state->stats_range->m_range->EnableRegionSet(false);
        state->stats_controls->EnableRegionSet(false);
        state->character_info->SetEnabled(false);
        if (state->effect_list != 0) {
            DeleteList(state->effect_list);
            state->effect_list = 0;
            return;
        }
        break;
    case W8_CAMP_PAGE_SKILLS:
        DisableCampSkillRegions();
        for (index = 0; index < 0x29; ++index) {
            g_review_character->skills[index].improved = false;
        }
        return;
    case W8_CAMP_PAGE_SPELLS:
        RegionSetDisable(0x2c);
        SetCampSpellRangesEnabled(false);
        break;
    default:
        break;
    }
}

/* Camp page 4: the reviewed character's health, stamina and per-realm spell
   point regeneration rates. */
// FUNCTION: WIZ8 0x005a4890
void DrawCampRegenStats(void)
{
    unsigned int index;

    SetFont(g_calligraphy_font);
    SetObjectShade(g_calligraphy_font_object, 4);
    SetObjectShade(g_calligraphy_font_object, 0);
    gprintfDirty(0x14a, 5, gppStringList[0x8ce]);
    SetObjectShade(g_calligraphy_font_object, 4);
    gprintfDirty(0x221, 5, L"%6.3f", g_review_character->health_regen_rate);
    SetObjectShade(g_calligraphy_font_object, 0);
    gprintfDirty(0x14a, 0x14, gppStringList[0x8cd]);
    SetObjectShade(g_calligraphy_font_object, 4);
    gprintfDirty(0x221, 0x14, L"%6.3f", g_review_character->stamina_regen_rate);
    SetObjectShade(g_calligraphy_font_object, 0);
    gprintfDirty(0x14a, 0x23, gppStringList[0x8d0]);
    SetObjectShade(g_calligraphy_font_object, 4);
    for (index = 0; index < 6; ++index) {
        gprintfDirty(index * 0x14 + 0x1fe, 0x23, L"%6.3f/",
                     g_review_character->spell_regen_rates[index * 2]);
    }
}

/* Equip-slot-group item filters are exclusive: selecting one clears the others. */
// FUNCTION: WIZ8 0x005a49d0
void ClearOtherCampItemGroupFilters(W8CampItemFilter filter)
{
    unsigned int index;

    for (index = 2; index < 6; ++index) {
        if (index != filter) {
            g_camp_screen->item_filters[index] = 0;
        }
    }
}

/* Rebuild the visible item list from the party pool under the item filters:
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

    g_camp_screen->item_list_count = 0;
    filter = 0;
    for (index = 0; index < 6; ++index) {
        if (index != 0 && index != 1 && g_camp_screen->item_filters[index] != 0) {
            filter |= 1 << index;
        }
    }
    pool = g_status.party_item_pool;
    for (index = 0; index < g_status.party_item_count; ++index, ++pool) {
        if (pool->iItemNo != -1 &&
            (g_camp_screen->item_filters[W8_CAMP_ITEM_FILTER_USABLE] == 0 ||
             CanCharacterUseItem(g_review_character, pool->iItemNo)) &&
            (g_camp_screen->item_filters[W8_CAMP_ITEM_FILTER_UNIDENTIFIED] == 0 ||
             !pool->identified) &&
            (filter == 0 || (filter & static_cast<unsigned char>(
                                          1 << GetItemEquipSlotGroup(pool->iItemNo))) != 0)) {
            g_camp_screen->item_list[g_camp_screen->item_list_count] = index;
            ++g_camp_screen->item_list_count;
        }
    }
    scrollbar = g_camp_screen->item_range->m_range;
    scrollbar->SetRangeEnabled(g_camp_screen->item_list_count > 8);
    rows = g_camp_screen->item_list_count - 8;
    if (rows > 0) {
        scrollbar->SetRange(0, rows % 2 == 0 ? rows / 2 : rows / 2 + 1);
    }
    if (g_camp_screen->item_list_count == 0) {
        g_camp_screen->item_scroll = 0;
    } else if (g_camp_screen->item_list_count <= g_camp_screen->item_scroll) {
        g_camp_screen->item_scroll = (g_camp_screen->item_list_count - 1) / 8 * 8;
    }
    if ((g_camp_screen->item_scroll & 1) == 0) {
        scrollbar->SetValue(g_camp_screen->item_scroll >> 1);
    } else {
        scrollbar->SetValue((g_camp_screen->item_scroll >> 1) + 1);
    }
}

// FUNCTION: WIZ8 0x005a4bc0
void SetCampHeaderMode(W8CampHeaderMode mode)
{
    g_camp_screen->header_mode = mode;
    g_camp_screen->redraw_flags |= 0x7ff;
}

// FUNCTION: WIZ8 0x005a4be0
void DisplayCampDialog(W8DialogBase* dialog)
{
    g_camp_screen->dialog = dialog;
    ActivateDialogRegion(0x138);
}

// FUNCTION: WIZ8 0x005a4c00
void ShowCampNoticeLine(const wchar_t* text, W8DialogDestroyCallback callback, bool confirmation,
                        bool cancel)
{
    W8MessageDialogBase* dialog =
        static_cast<W8MessageDialogBase*>(CreateDialogByKind(W8_DIALOG_MESSAGE));

    dialog->SetClientExtent(0xfa, 200);
    dialog->SetMessage(text, 1, 0x32, confirmation, cancel, true, true, 0, 0x15e);
    SetDialogDestroyCallback(dialog, callback);
    DisplayCampDialog(dialog);
}

/* The shared click handler for the camp item regions: a backpack slot arrives
   with origin 0, a worn slot with origin 1 and a party-pool row with origin
   2. The screen's item_action picks the interpretation - plain handling,
   identify, stack split, use, use held item on item - and the held item can
   merge onto the clicked stack, swap with it, or drop into the pool. */
static void SelectPendingCampCharacter()
{
    SelectCampCharacter(CharacterPointerToPartySlot(g_camp_character));
    if (IsPartySlotEligible(giReviewCharSlot)) {
        QueueCharacterEvent(g_camp_character, g_effect36, 0, g_effect_argument0,
                            g_character_event_full_volume);
    } else {
        wchar_t* text = FormatWideString(gppStringList[0x931], g_camp_character->name);
        ShowCampNoticeLine(text, 0, true, false);
    }
}

static void RefreshCampItemOrigin(W8ItemOrigin origin, W8EquipSlot equip_slot)
{
    if (origin == W8_ITEM_ORIGIN_EQUIPPED) {
        RebuildEquipmentAndDerivedStatsForSlot(giReviewCharSlot);
        if (GetPairedEquipSlot(equip_slot) != -1) {
            g_status.buffers.XChar[giReviewCharSlot].weapon_swap_pending = false;
        }
        RebuildCampItemList();
    } else if (origin == W8_ITEM_ORIGIN_BACKPACK) {
        RecalculateCharacterDerivedStats(&g_status.buffers.Char[giReviewCharSlot]);
    } else {
        RebuildCampItemList();
    }
}

// FUNCTION: WIZ8 0x005a4c70
void HandleCampItemClick(W8ItemInstance* item, unsigned int slot_index, W8ItemOrigin origin)
{
    W8CombatSlot target;
    short related_kind;
    bool changed = false;
    bool merged = false;
    bool partially_merged = false;
    bool merge_tried = false;
    bool same_kind = false;
    bool choose_character;
    unsigned int index;
    unsigned int old_pool_count;
    int result;
    int party_slot;
    W8EquipSlot paired_slot;
    W8EquipSlot equip_slot = static_cast<W8EquipSlot>(slot_index);
    bool reidentify;
    W8ItemInstance* paired;
    W8Character* character;
    W8NpcState* npc;
    wchar_t* text;

    if (item == 0) {
        srAssertFail("pPCItem != NULL",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\ReviewCharacterScreen.cpp", 0x4e5,
                     0);
    }
    if (static_cast<unsigned int>(origin) >= W8_ITEM_ORIGIN_COUNT) {
        srAssertFail("uiSlotType < SLOT_TYPE_COUNT",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\ReviewCharacterScreen.cpp", 0x4e6,
                     0);
    }

    /* A held stackable whose name kind the clicked item merges with counts as
       the same item for the use-merge path below. */
    if (g_status.item_in_cursor && item->iItemNo != -1 &&
        GetItemMergeKind(item->iItemNo, &related_kind) &&
        g_item_records[g_status.item_in_hand.iItemNo].unidentified_name_index == related_kind) {
        same_kind = true;
    }
    if (item->iItemNo == -1 && !g_status.item_in_cursor) {
        return;
    }
    if (origin == W8_ITEM_ORIGIN_EQUIPPED && g_status.item_in_cursor &&
        g_camp_screen->item_action != W8_CAMP_ITEM_ACTION_MOVE) {
        if (!same_kind && !CanEquipItemInSlot(g_review_character, g_status.item_in_hand.iItemNo,
                                              slot_index, true)) {
            return;
        }
        if (!CanCharacterUseItem(g_review_character, g_status.item_in_hand.iItemNo)) {
            return;
        }
    }
    if (g_camp_screen->item_action == W8_CAMP_ITEM_ACTION_CAST_SPELL ||
        g_camp_screen->item_action == W8_CAMP_ITEM_ACTION_USE_ON_CHARACTER) {
        return;
    }
    if (!g_status.game_started) {
        text = gppStringList[0x900];
        ShowCampNoticeLine(text, 0, true, false);
        return;
    }
    character = &g_status.buffers.Char[giReviewCharSlot];
    if (character->uiCondition[W8_CONDITION_MISSING] != 0 &&
        (origin == W8_ITEM_ORIGIN_EQUIPPED || origin == W8_ITEM_ORIGIN_BACKPACK)) {
        text = gppStringList[0x907];
        ShowCampNoticeLine(text, 0, true, false);
        return;
    }
    if (character->uiCondition[W8_CONDITION_WEBBED] != 0 &&
        (origin == W8_ITEM_ORIGIN_EQUIPPED || origin == W8_ITEM_ORIGIN_BACKPACK)) {
        text = gppStringList[0x908];
        ShowCampNoticeLine(text, 0, true, false);
        return;
    }
    if (character->uiCondition[W8_CONDITION_TURNCOAT] != 0 &&
        (origin == W8_ITEM_ORIGIN_EQUIPPED || origin == W8_ITEM_ORIGIN_BACKPACK)) {
        text = gppStringList[0x909];
        ShowCampNoticeLine(text, 0, true, false);
        return;
    }
    if (character->highest_condition >= W8_CONDITION_TURNCOAT &&
        origin != W8_ITEM_ORIGIN_PARTY_POOL && g_status.item_in_cursor &&
        gXStatus.held_item_source != giReviewCharSlot) {
        text = gppStringList[0x901];
        ShowCampNoticeLine(text, 0, true, false);
        return;
    }

    /* In combat an item click spends the character's action allowance unless
       it only touches a weapon-class slot or folds into the same-kind merge.
       A weapon or shield being moved onto a slot it cannot pair with stays
       gated as well. */
    bool gated = true;
    if (gXStatus.fCombatMode && origin != W8_ITEM_ORIGIN_PARTY_POOL) {
        if (!g_status.item_in_cursor ||
            ((g_item_records[g_status.item_in_hand.iItemNo].equip_class ==
                  W8_ITEM_EQUIP_CLASS_THROWN_WEAPON ||
              g_item_records[g_status.item_in_hand.iItemNo].equip_class ==
                  W8_ITEM_EQUIP_CLASS_AMMUNITION) &&
             gXStatus.held_item_source == giReviewCharSlot &&
             (origin != W8_ITEM_ORIGIN_EQUIPPED ||
              HeldItemFitsPairedSlot(giReviewCharSlot, equip_slot)))) {
            if (item->iItemNo == -1 ||
                g_item_records[item->iItemNo].equip_class == W8_ITEM_EQUIP_CLASS_THROWN_WEAPON ||
                g_item_records[item->iItemNo].equip_class == W8_ITEM_EQUIP_CLASS_AMMUNITION ||
                same_kind) {
                gated = false;
            }
        }
    }
    if (gated && !IsCampActionAllowed(giReviewCharSlot)) {
        return;
    }
    if (gfKeyState[VK_SHIFT] != 0) {
        TakeItemUnitToHand(item, slot_index, origin);
        return;
    }
    if (g_camp_screen->item_action == W8_CAMP_ITEM_ACTION_IDENTIFY_SPELL) {
        party_slot = CharacterPointerToPartySlot(g_camp_identifying_character);
        if (!CanItemLeaveItsSlot(item)) {
            QueueCharacterEvent(&g_status.buffers.Char[party_slot], g_character_event_kind2, 0,
                                g_character_event_no_flags, g_character_event_full_volume);
            return;
        }
        if (g_camp_identifying_character == 0) {
            srAssertFail("gpIdentifyingPC != NULL",
                         "C:\\Projects\\Wizardry 8\\Local Screens\\ReviewCharacterScreen.cpp",
                         0x553, 0);
        }
        old_pool_count = g_status.party_item_count;
        reidentify = false;
        ResetCombatSlot(&target);
        target.iType = W8_TARGET_KIND_ITEM;
        target.pPCItem = item;
        StartBreathCycle(party_slot, false);
        W8PartySlotRow* row = &g_status.buffers.XChar[party_slot];
        if (row->pending_action == W8_ACTION_USE_ITEM &&
            g_item_records[row->pending_action_detail.item_use.item->iItemNo].spell_id ==
                W8_SPELL_IDENTIFY_ITEM) {
            reidentify = true;
            result = CommitPartySlotItemUse(party_slot, row->pending_action_detail.item_use.item,
                                            &target);
        } else {
            result = CommitPartySlotSpell(party_slot, W8_SPELL_IDENTIFY_ITEM, 8, &target);
        }
        if (origin == W8_ITEM_ORIGIN_PARTY_POOL && reidentify &&
            old_pool_count != g_status.party_item_count) {
            item =
                &g_status.party_item_pool[g_status.party_item_count - old_pool_count + slot_index];
            RebuildCampItemList();
            RecalculateCarriedWeight(g_review_character);
            RedistributePartyEncumbrance();
            g_camp_screen->redraw_flags |= W8_CAMP_REDRAW_CHARACTER_INFO;
            g_camp_screen->item_redraw_flags |= W8_CAMP_ITEM_REDRAW_POOL;
        }
        if (result == 1 && item->iItemNo != -1) {
            OpenItemInfoDialog(item, 0);
            if (!item->identified) {
                QueueCharacterEvent(&g_status.buffers.Char[party_slot], g_character_event_kind2, 0,
                                    g_character_event_no_flags, g_character_event_full_volume);
            }
        }
        SetCampItemActionMode(W8_CAMP_ITEM_ACTION_NONE);
        return;
    }
    if (g_camp_screen->item_action == W8_CAMP_ITEM_ACTION_USE_ON_ITEM) {
        UseHeldItemOnItem(item);
        return;
    }
    if (g_camp_screen->item_action == W8_CAMP_ITEM_ACTION_IDENTIFY) {
        IdentifyAndOpenItemInfo(item);
        return;
    }
    if (g_camp_screen->item_action == W8_CAMP_ITEM_ACTION_SPLIT_STACK) {
        if (item->iItemNo != -1) {
            OpenSplitStackDialog(item);
            return;
        }
    } else if (g_camp_screen->item_action == W8_CAMP_ITEM_ACTION_USE) {
        if (CanCharacterUseItemEntry(g_review_character, item) != 0) {
            UseCampItem(item);
        }
        return;
    } else if (g_camp_screen->item_action == W8_CAMP_ITEM_ACTION_MOVE) {
        if (g_status.item_in_cursor) {
            if (item->iItemNo == -1) {
                SetCampItemActionMode(W8_CAMP_ITEM_ACTION_NONE);
                return;
            }
            if (g_camp_character_pending) {
                SelectPendingCampCharacter();
                return;
            }
            MergeItemStacksWithHeld(item);
            return;
        }
        if (item->iItemNo == -1) {
            SetCampItemActionMode(W8_CAMP_ITEM_ACTION_NONE);
            return;
        }
    }

    /* Equipped items that may not be removed get one warning dialog and a
       bound mark; the next click then unequips them. */
    if (item->iItemNo != -1 && origin == W8_ITEM_ORIGIN_EQUIPPED) {
        if (!CanUnequipSlotItem(g_review_character, equip_slot)) {
            text = gppStringList[0x90b];
            ShowCampNoticeLine(text, 0, true, false);
            if (item->bound) {
                return;
            }
            BindEquippedItem(g_review_character, equip_slot);
            return;
        }
        item->bound = true;
    }

    if (same_kind) {
        MergeItemUses(&g_status.buffers.Char[giReviewCharSlot], item, &g_status.item_in_hand);
        SetCampItemActionMode(W8_CAMP_ITEM_ACTION_NONE);
        changed = true;
        RefreshCampItemOrigin(origin, equip_slot);
    } else {
        /* Fold the held stack onto a matching stack; for the pool the scan
           continues across every row until the held stack is consumed. */
        if (g_status.item_in_cursor &&
            g_item_records[g_status.item_in_hand.iItemNo].quantity_kind == W8_ITEM_QUANTITY_STACK) {
            if (item->iItemNo != -1 && item->iItemNo == g_status.item_in_hand.iItemNo) {
                merged = MergeItemStacks(item, &g_status.item_in_hand, &partially_merged);
                merge_tried = true;
            }
            if (!merged && origin == W8_ITEM_ORIGIN_PARTY_POOL) {
                for (index = 0; index < g_status.party_item_count; ++index) {
                    if (MergeItemStacks(&g_status.party_item_pool[index], &g_status.item_in_hand,
                                        &partially_merged)) {
                        merged = true;
                        break;
                    }
                }
            }
        }
        if (origin == W8_ITEM_ORIGIN_PARTY_POOL) {
            if (!merged) {
                if (g_status.item_in_cursor) {
                    if (g_camp_character_pending) {
                        SelectPendingCampCharacter();
                    } else if (InsertItemIntoPartyPool(&g_status.item_in_hand, slot_index)) {
                        changed = true;
                        RebuildCampItemList();
                    } else {
                        text = gppStringList[0x90c];
                        ShowCampNoticeLine(text, 0, true, false);
                    }
                } else if (slot_index < g_status.party_item_count) {
                    /* An empty hand picks the clicked pool row up. */
                    CopyItemInstance(&g_status.item_in_hand, item, 0, true);
                    changed = true;
                    if (g_camp_screen->item_action == W8_CAMP_ITEM_ACTION_DROP) {
                        DropHeldCampItem();
                        RebuildCampItemList();
                    } else if (g_camp_screen->item_action == W8_CAMP_ITEM_ACTION_MOVE) {
                        SetHandCursors(0);
                        RebuildCampItemList();
                    } else {
                        RebuildCampItemList();
                    }
                }
            }
        } else {
            if (!merge_tried && ((!g_status.item_in_cursor && item->iItemNo != -1) ||
                                 (g_status.item_in_cursor && !merged))) {
                if (!g_camp_character_pending || g_camp_character == g_review_character) {
                    g_camp_character_pending = false;
                    if (item->iItemNo != -1 && (giReviewCharSlot == 0 || giReviewCharSlot == 1)) {
                        npc = GetNpcState(g_status.buffers.XChar[giReviewCharSlot].npc_index);
                        if (npc != 0 && NpcWantsItem(npc, item)) {
                            g_camp_character_pending = true;
                            g_camp_character = g_review_character;
                        }
                    }
                    if (g_status.item_in_cursor) {
                        choose_character = gXStatus.held_item_source == -1;
                        DeliverExceptionalItemReaction(&g_status.item_in_hand, choose_character,
                                                       g_review_character);
                    }
                    if (origin == W8_ITEM_ORIGIN_EQUIPPED && g_status.item_in_cursor &&
                        !HeldItemFitsPairedSlot(giReviewCharSlot, equip_slot)) {
                        paired_slot = GetPairedEquipSlot(equip_slot);
                        paired = &g_review_character->EquippedItem[paired_slot];
                        if (!CanUnequipSlotItem(g_review_character, paired_slot)) {
                            text = gppStringList[0x916];
                            ShowCampNoticeLine(text, 0, true, false);
                            if (paired->bound) {
                                return;
                            }
                            BindEquippedItem(g_review_character, paired_slot);
                            return;
                        }
                        paired->bound = true;
                        if (item->iItemNo == -1) {
                            SwapItemInstances(item, &g_status.item_in_hand, g_review_character,
                                              true);
                            g_camp_character_pending = false;
                            if (paired->iItemNo != -1 &&
                                (giReviewCharSlot == 0 || giReviewCharSlot == 1)) {
                                npc =
                                    GetNpcState(g_status.buffers.XChar[giReviewCharSlot].npc_index);
                                if (npc != 0 && NpcWantsItem(npc, paired)) {
                                    g_camp_character_pending = true;
                                    g_camp_character = g_review_character;
                                }
                            }
                            SwapItemInstances(paired, &g_status.item_in_hand, g_review_character,
                                              true);
                            changed = true;
                        } else if (AddItemToCharacter(g_review_character, paired, false, false,
                                                      true)) {
                            SwapItemInstances(item, &g_status.item_in_hand, g_review_character,
                                              true);
                            changed = true;
                        } else {
                            if (giReviewCharSlot == 0 || giReviewCharSlot == 1) {
                                if (NpcWantsItem(
                                        GetNpcState(
                                            g_status.buffers.XChar[giReviewCharSlot].npc_index),
                                        paired)) {
                                    text = gppStringList[0x90c];
                                    ShowCampNoticeLine(text, 0, true, false);
                                    return;
                                }
                            }
                            if (!AddItemToParty(paired, false, true)) {
                                text = gppStringList[0x90c];
                                ShowCampNoticeLine(text, 0, true, false);
                                return;
                            }
                            SwapItemInstances(item, &g_status.item_in_hand, g_review_character,
                                              true);
                            changed = true;
                        }
                    } else {
                        SwapItemInstances(item, &g_status.item_in_hand, g_review_character, true);
                        changed = true;
                        if (origin == W8_ITEM_ORIGIN_BACKPACK &&
                            g_review_character->backpack[slot_index].iItemNo != -1 &&
                            Random(100) < 5) {
                            QueueCharacterEvent(g_review_character, g_special_event14, 0,
                                                g_character_event_no_flags,
                                                g_character_event_full_volume);
                        }
                    }
                } else {
                    SelectPendingCampCharacter();
                }
            }
            if (g_camp_screen->item_action == W8_CAMP_ITEM_ACTION_DROP && g_status.item_in_cursor) {
                DropHeldCampItem();
            } else if (g_camp_screen->item_action == W8_CAMP_ITEM_ACTION_MOVE &&
                       g_status.item_in_cursor) {
                SetHandCursors(0);
            }
            if (changed) {
                RefreshCampItemOrigin(origin, equip_slot);
            }
        }
    }
    if (!changed && !partially_merged && !merged) {
        if (same_kind) {
            g_camp_screen->redraw_flags |= W8_CAMP_REDRAW_ITEM_ACTIONS;
        }
        return;
    }
    g_camp_screen->redraw_flags |= W8_CAMP_REDRAW_ITEM_ACTIONS;
    if (gfKeyState[VK_CONTROL]) {
        g_camp_screen->redraw_flags |= W8_CAMP_REDRAW_ALL;
    } else {
        if (origin == W8_ITEM_ORIGIN_BACKPACK) {
            if (partially_merged || merged) {
                g_camp_screen->item_redraw_flags |= W8_CAMP_ITEM_REDRAW_BACKPACK;
            } else {
                g_camp_screen->item_redraw_flags |= W8_CAMP_ITEM_REDRAW_BACKPACK_CELL_FIRST
                                                    << slot_index;
            }
        } else if (origin == W8_ITEM_ORIGIN_EQUIPPED) {
            if (partially_merged || merged) {
                g_camp_screen->item_redraw_flags |= W8_CAMP_ITEM_REDRAW_EQUIPMENT_CELL_FIRST
                                                    << slot_index;
            } else {
                g_camp_screen->redraw_flags |= W8_CAMP_REDRAW_CHARACTER_INFO;
                g_camp_screen->redraw_flags |= W8_CAMP_REDRAW_PORTRAIT;
            }
        } else if (origin == W8_ITEM_ORIGIN_PARTY_POOL) {
            g_camp_screen->item_redraw_flags |= W8_CAMP_ITEM_REDRAW_POOL;
        }
        g_camp_screen->item_redraw_flags |= W8_CAMP_ITEM_REDRAW_EQUIPMENT_CELLS;
    }
    RecalculateCarriedWeight(g_review_character);
    RedistributePartyEncumbrance();
    g_camp_screen->redraw_flags |= W8_CAMP_REDRAW_CHARACTER_INFO;
    g_camp_screen->redraw_flags |= W8_CAMP_REDRAW_PORTRAIT;
}

// FUNCTION: WIZ8 0x005a5da0
void TakeItemUnitToHand(W8ItemInstance* item, unsigned short slot, W8ItemOrigin origin)
{
    W8ItemInstance single;
    bool moved = false;

    if (g_camp_screen->item_action != W8_CAMP_ITEM_ACTION_NONE) {
        return;
    }
    if (item->iItemNo == -1) {
        return;
    }
    if (!CanSplitItemStack(item)) {
        return;
    }
    if (!g_status.item_in_cursor) {
        single = *item;
        single.stack_count = 1;
        --item->stack_count;
        if (origin == W8_ITEM_ORIGIN_EQUIPPED || origin == W8_ITEM_ORIGIN_BACKPACK) {
            CopyItemInstance(&g_status.item_in_hand, &single,
                             &g_status.buffers.Char[giReviewCharSlot], true);
            gXStatus.held_item_origin = origin;
            gXStatus.held_item_slot = slot;
        } else {
            CopyItemInstance(&g_status.item_in_hand, &single, 0, true);
        }
        moved = true;
    } else if (item->iItemNo == g_status.item_in_hand.iItemNo &&
               g_status.item_in_hand.stack_count <
                   g_item_records[g_status.item_in_hand.iItemNo].maximum_quantity) {
        ++g_status.item_in_hand.stack_count;
        --item->stack_count;
        moved = true;
    }
    if (item->stack_count == 0) {
        EmptyItemRecord(item, g_review_character, true);
    } else if (!moved) {
        return;
    }
    RebuildEquipmentAndDerivedStatsForSlot(giReviewCharSlot);
    RebuildCampItemList();
    RecalculateCharacterDerivedStats(&g_status.buffers.Char[giReviewCharSlot]);
    RecalculateCarriedWeight(g_review_character);
    RedistributePartyEncumbrance();
    g_camp_screen->redraw_flags |= W8_CAMP_REDRAW_ALL;
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

    if (g_camp_character_pending && (g_camp_character != g_review_character || force)) {
        slot = CharacterPointerToPartySlot(g_camp_character);
        SelectCampCharacter(slot);
        if (IsPartySlotEligible(giReviewCharSlot)) {
            QueueCharacterEvent(g_camp_character, g_effect36, 0, g_effect_argument0,
                                g_character_event_full_volume);
            return false;
        }
        text = FormatWideString(gppStringList[0x931], g_camp_character->name);
        ShowCampNoticeLine(text, 0, true, false);
        return false;
    }
    return true;
}

/* When the reviewed slot is one of the two companion slots and its bound NPC
   wants the item being used, the current character is remembered as pending a
   swap (cleared first so a failed attempt leaves none). */
// FUNCTION: WIZ8 0x005A6020
void MarkCampCharacterPending(W8ItemInstance* item)
{
    W8NpcState* npc;

    g_camp_character_pending = false;
    if (item->iItemNo != -1 && (giReviewCharSlot == 0 || giReviewCharSlot == 1)) {
        npc = GetNpcState(g_status.buffers.XChar[giReviewCharSlot].npc_index);
        if (npc != 0 && NpcWantsItem(npc, item)) {
            g_camp_character_pending = true;
            g_camp_character = g_review_character;
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
    W8PartySlotRow* row;
    W8Character* character;

    if (!gXStatus.fCombatMode) {
        return true;
    }
    character = &g_status.buffers.Char[party_slot];
    if (character->uiCondition[W8_CONDITION_DEAD] != 0 ||
        character->uiCondition[W8_CONDITION_UNCONSCIOUS] != 0 ||
        character->uiCondition[W8_CONDITION_PARALYZED] != 0 ||
        character->uiCondition[W8_CONDITION_ASLEEP] != 0) {
        if (g_combat_state->equip_phase != 0) {
            return true;
        }
        message = gppStringList[0x90a];
    } else {
        row = &g_status.buffers.XChar[party_slot];
        if (g_combat_state->equip_phase == 0) {
            if (g_combat_state->characters[party_slot].dead) {
                if (row->pending_action == W8_ACTION_EQUIP) {
                    message = gppStringList[0x903];
                } else if (row->action == W8_ACTION_EQUIP) {
                    message = gppStringList[0x904];
                } else {
                    message = gppStringList[0x902];
                }
            } else if (row->action == W8_ACTION_EQUIP) {
                message = gppStringList[0x903];
            } else {
                message = gppStringList[0x902];
            }
        } else {
            if (row->pending_action == W8_ACTION_EQUIP) {
                return true;
            }
            if (row->action == W8_ACTION_EQUIP) {
                message = gppStringList[0x904];
            } else {
                message = gppStringList[0x902];
            }
        }
    }
    ShowCampNoticeLine(message, 0, true, false);
    return false;
}

// FUNCTION: WIZ8 0x005A6310
bool IsEquippableItemClass(W8ItemInstance* item)
{
    char equip_class = g_item_records[item->iItemNo].equip_class;
    if (equip_class != W8_ITEM_EQUIP_CLASS_THROWN_WEAPON &&
        equip_class != W8_ITEM_EQUIP_CLASS_AMMUNITION) {
        return false;
    }
    return true;
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

    StartBreathCycle(party_slot, false);
    row = &g_status.buffers.XChar[party_slot];
    row->pending_action = W8_ACTION_CAST_SPELL;
    row->attack_mode[0] = spell_id;
    row->attack_mode[1] = -1;
    row->pending_action_detail.spell.power_level = power_level;
    row->pending_action_detail.spell.unused = 0;
    row->target_out_of_combat = *target;
    result = ExecuteCharacterSpellCast(party_slot, spell_id, power_level, &cost, false);
    SetPartySlotSpell(party_slot, spell_id, power_level, target);
    FatigueCharacter(party_slot, cost, true, 0);
    if (result != 1) {
        SoundPlay("Data\\Sound\\Misc\\Spell Fizzle 01.wav", 0);
        return result;
    }
    if (spell_id == W8_SPELL_IDENTIFY_ITEM && target->pPCItem->identified) {
        SoundPlay("Data\\Sound\\Misc\\Spell Learned.wav", 0);
        return 1;
    }
    SoundPlay(s_general_magic_sound, 0);
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

    StartBreathCycle(party_slot, false);
    row = &g_status.buffers.XChar[party_slot];
    row->pending_action = W8_ACTION_USE_ITEM;
    row->attack_mode[0] = -1;
    row->attack_mode[1] = -1;
    row->pending_action_detail.item_use.kind = -1;
    row->pending_action_detail.item_use.item = item;
    row->target_out_of_combat = *target;
    result = UseItem(&g_status.buffers.Char[party_slot], item, &uses);
    StagePartySlotItemUse(party_slot, item, target);
    if (uses == -1) {
        uses = CharacterActionFatigueCost(party_slot, W8_ACTION_USE_ITEM);
    }
    FatigueCharacter(party_slot, uses, true, 0);
    if (result == 1) {
        used = target->pPCItem;
        if (g_item_records[used->iItemNo].spell_id == W8_SPELL_IDENTIFY_ITEM && used->identified) {
            SoundPlay("Data\\Sound\\Misc\\Spell Learned.wav", 0);
        } else {
            SoundPlay(s_general_magic_sound, 0);
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
    bool endgame_variant = false;

    g_status.endgame_started = true;
    UpdateHeldItemCursor();
    if (GetFact(W8_FACT_ENDING_BOFFO_ONE) != 0) {
        fade_to_black = 1;
    } else if (GetFact(W8_FACT_ENDGAME_JOIN_SAVANT) != 0) {
        fade_code = 1;
        endgame_variant = true;
    }
    MSYS_Init();
    ResetRegions();
    ActivateDialogRegion(0x138);
    VideoRemoveToolTip();
    g_level_block->transition_pending = true;
    g_level_block->review_transition_active = true;
    BeginScreenFade(fade_to_black, 0, fade_code, ShowEndingScreen, true, endgame_variant);
}

/* Begin a timed full-screen fade: spawn a 640x480 colored quad over the UI,
   switch its blend shader for the requested ramp direction and seed the
   fade state. `callback` runs from UpdateScreenFade once the ramp
   finishes. `fade_to_black` selects the subtractive ramp (white quad
   darkening to black); `fade_out` selects the direction the opacity runs. */
// FUNCTION: WIZ8 0x005A6620
void BeginScreenFade(int fade_to_black, int fade_out, int duration, void (*callback)(void),
                     bool fullscreen_scene_last, char render_each_tick)
{
    srShader shader;
    srVector4T<float> color;

    g_fade_duration = duration;
    g_fade_out = fade_out;
    g_fade_callback = callback;
    g_fade_flag = render_each_tick;
    g_level_block->review_transition_done = true;
    if (fullscreen_scene_last) {
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
    g_fade_overlay = CreateColoredPolygonSprite(0x280, 0x1e0, &color, true);
    Position2DNodeUnsnapped(g_fade_overlay, 0, 0);
    shader = static_cast<srMeshModel*>(g_fade_overlay->getModel())->getShader(0);
    if (fade_to_black == 0) {
        shader.value = (shader.value & ~0x6040) | 0xa0;
    } else {
        shader.value = (shader.value & ~0x20c0) | 0x4020;
    }
    static_cast<srMeshModel*>(g_fade_overlay->getModel())->setShader(shader, 0);
    static_cast<srMaterial*>(static_cast<srMeshModel*>(g_fade_overlay->getModel())
                                 ->getMaterial(0, srMeshModel::SIDE_FRONT))
        ->setOpacity(fade_out != 0 ? 1.0f : 0.0f);
    g_fade_tick_base = GetTickCount();
}

/* Advance the pending screen fade: interpolate the overlay's opacity over
   g_fade_duration (reversed when fading back out) and render a
   frame per tick while g_fade_flag is set. On completion a
   fade-in snaps the quad opaque and renders twice, then the overlay is
   released and the stored callback runs. Returns g_fade_flag. */
// FUNCTION: WIZ8 0x005A6790
unsigned char UpdateScreenFade(void)
{
    if (!g_level_block->review_transition_done) {
        return 0;
    }
    w8_ulong elapsed = GetTickCount() - g_fade_tick_base;
    if (g_fade_duration < elapsed) {
        g_level_block->review_transition_done = false;
        if (g_fade_out == 0) {
            static_cast<srMaterial*>(static_cast<srMeshModel*>(g_fade_overlay->getModel())
                                         ->getMaterial(0, srMeshModel::SIDE_FRONT))
                ->setOpacity(1.0f);
            RenderFrame();
            RenderFrame();
        }
        g_fade_overlay->release();
        SetFullscreenSceneLast(0);
        if (g_fade_callback != 0) {
            g_fade_callback();
        }
        return g_fade_flag;
    }
    float progress = static_cast<float>(elapsed) / g_fade_duration;
    srMaterial* material =
        static_cast<srMaterial*>(static_cast<srMeshModel*>(g_fade_overlay->getModel())
                                     ->getMaterial(0, srMeshModel::SIDE_FRONT));
    if (g_fade_out == 0) {
        material->setOpacity(progress);
    } else {
        material->setOpacity(g_float_one - progress);
    }
    if (g_fade_flag != 0) {
        RenderFrame();
    }
    return g_fade_flag;
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
    if (g_status.iron_man && !gXStatus.party_moving) {
        DeleteCurrentSaveFiles();
    }
    if (gXStatus.fSurprisePossible) {
        RestoreSurpriseView();
    }
    UpdateHeldItemCursor();
    SoundPlay("Data\\Sound\\Misc\\PartyDead.wav", 0);
    StartMusicResource("CombatLose.MPL", 0, 1);
    MSYS_Init();
    ResetRegions();
    ActivateDialogRegion(0x138);
    VideoRemoveToolTip();
    g_level_block->transition_pending = true;
    BeginScreenFade(1, 0, 0x7d0, DrawPartyDeathScreen, true, 0);
    g_level_block->review_transition_active = true;
}

/* Pump the post-fade review/death state: on the first tick an armed ending
   autosave writes the next free Ending slot and reports it on the main-menu
   message line; a dirty level reloads; any queued key-down or button-up
   schedules the closing fade back through EndReviewTransition. */
// FUNCTION: WIZ8 0x005A6970
void PumpReviewTransition(void)
{
    InputAtom input;
    char name[260];
    bool exit_review;

    if (!g_level_block->review_transition_active || g_level_block->review_transition_done) {
        return;
    }
    if (g_ending_autosave) {
        if (FindFreeEndingSaveName(name)) {
            SaveGame(name, 0);
            SetMainMenuMessage(
                FormatWideString(L"%s %S.%S", gppStringList[0x78b], name, g_save_extension));
        }
        g_ending_autosave = false;
    }
    if (g_status.current_level != -1) {
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
        BeginScreenFade(0, 0, 0x320, EndReviewTransition, true, 1);
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

    DrawCatalogImageAndInvalidate(FRAME_BUFFER, 0x1df, 0, 0, 0, 0, VO_BLT_SRCTRANSPARENCY, 0);
    if (gXStatus.party_moving) {
        text = gppStringList[0x777];
    } else {
        text = gppStringList[0x778];
    }
    SetFont(g_level_load_font);
    gprintf(0x276 - StringPixLength(const_cast<wchar_t*>(text), g_level_load_font), 0x1c7,
            const_cast<wchar_t*>(text));
    SetRadarMapVisible(false);
    SetFormationBoardVisible(false);
    VideoRemoveToolTip();
    g_world_render_enabled = 0;
    BeginScreenFade(1, 1, 0x4b0, 0, true, 1);
}

/* Fade-completion callback leaving the review/death screen: release the
   modal region, stop the ending voice-over, reset the main-game mode and
   leave the transition. The endgame path continues to the credits screen;
   party death just stops the playlist. */
// FUNCTION: WIZ8 0x005A6B20
void EndReviewTransition(void)
{
    ClearActiveRegionIfMatches(0x138);
    if (g_ending_sound != 0) {
        SoundStop(g_ending_sound);
        g_ending_sound = 0;
    }
    g_world_render_enabled = 1;
    ResetMainGameMode();
    g_level_block->review_transition_active = false;
    if (g_ending_screen) {
        SetPendingScreenState(W8_SCREEN_CREDITS);
        g_ending_screen = false;
        return;
    }
    StopMusicPlaylist(true);
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
    g_ending_screen = true;
    g_ending_autosave = true;
    if (GetFact(W8_FACT_ENDGAME_JOIN_SAVANT) != 0) {
        image = 0x1e1;
        schedule_fade = false;
        wcscpy(text, gppStringList[0x787]);
        sound = "Data\\Sound\\NPCs\\VOC_ENDGAME1\\VOC_ENDGAME1_005.mp3";
    } else if (GetFact(W8_FACT_QUE_ENDGAME2) != 0) {
        image = 0x1e2;
        wcscpy(text, gppStringList[0x788]);
        sound = "Data\\Sound\\NPCs\\VOC_ENDGAME2\\VOC_ENDGAME2_005.mp3";
    } else if (GetFact(W8_FACT_ENDGAME_SAVANT_PHOON_SPLIT) != 0) {
        image = 0x1e2;
        wcscpy(text, gppStringList[0x789]);
        sound = "Data\\Sound\\NPCs\\VOC_ENDGAME3\\VOC_ENDGAME3_000.mp3";
    } else if (GetFact(W8_FACT_ENDING_BOFFO_ONE) != 0) {
        image = 0x1e3;
        fade_to_black = 1;
        wcscpy(text, gppStringList[0x78a]);
        sound = "Data\\Sound\\NPCs\\VOC_ENDGAME4\\VOC_ENDGAME4_000.mp3";
        music = "CombatLose.MPL";
        g_ending_screen = false;
        g_ending_autosave = false;
    } else {
        image = 0x1df;
        sound = "";
        text[0] = 0;
    }
    DrawCatalogImageAndInvalidate(FRAME_BUFFER, image, 0, 0, 0, 0, VO_BLT_SRCTRANSPARENCY, 0);
    bounds.left = 0x46;
    bounds.top = 0;
    bounds.right = 0x239;
    bounds.bottom = 0x1d0;
    {
        W8TextBuffer buffer(&bounds, text, g_options_detail_font, g_W8TextBufferAlignBottom, 4);
        buffer.RenderToTarget(0, false, FRAME_BUFFER);
    }
    SetRadarMapVisible(false);
    SetFormationBoardVisible(false);
    VideoRemoveToolTip();
    g_world_render_enabled = 0;
    if (*music != 0) {
        StartMusicResource(music, 0, 1);
    }
    if (*sound != 0) {
        memset(&parms, 0xff, sizeof(SOUNDPARMS));
        parms.uiVolume = g_settings.voice_volume * 0x7f / 0xff;
        g_ending_sound = SoundPlayStreamedFile(sound, &parms);
    }
    if (schedule_fade) {
        BeginScreenFade(fade_to_black, 1, 0x4b0, 0, true, 1);
    }
}
