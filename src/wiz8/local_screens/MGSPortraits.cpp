#include "wiz8/local_screens/MGSPortraits.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/NPCInteractionSubscreen.h"
#include "wiz8/local_screens/PartySelectionScreen.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/layouts/character.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/fonts.h"
#include "wiz8/item_video_object_vector.h"
#include "wiz8/regions.h"
#include "wiz8/sr_api.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"
#include "wiz8/xstatus.h"
#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/utility.h"
#include "Font.h"
#include "himage.h"
#include "input.h"
#include "line.h"
#include "timer.h"
#include "vobject.h"
#include "vsurface.h"

void DrawDamageSplatOverlay(unsigned int party_slot); /* 0x0059ADD0 */
void DrawPortraitEffectIcon(unsigned int party_slot); /* 0x0059B0F0 */

/* 0x006488D0: dead-character portrait catalog ids, two per race - the small
   party-strip image at [race][0] and the large header portrait at [race][1]. */
// GLOBAL: WIZ8 0x006488D0
int g_dead_portrait_catalog_ids_6488d0[16][2] = {
    {20, 31}, {20, 31}, {20, 31}, {20, 31}, {20, 31}, {20, 31}, {23, 34}, {21, 32},
    {22, 33}, {25, 36}, {24, 35}, {27, 38}, {26, 37}, {28, 39}, {29, 40}, {30, 41},
};

/* 0x00649DD4: empty-hand catalog ids when a primary hand slot is bare. Each race
   stores the right-hand id at +0 and the left-hand id at +4 (retail also reaches
   the left id through 0x00649DD8). */
// GLOBAL: WIZ8 0x00649DD4
int g_empty_hand_catalog_ids_649dd4[32] = {
    103, 102, 103, 102, 103, 102, 103, 102, 103, 102, 105, 104, 109, 108, 109, 108,
    107, 106, 107, 106, 107, 106, 107, 106, 109, 108, 103, 102, 109, 108, 111, 110,
};

// GLOBAL: WIZ8 0x0069B940
Controls* g_panel_69b940; /* gpLevelButtonsPanel */
// GLOBAL: WIZ8 0x0061AA9C
char s_spell_sound_format_0061aa9c[] = "Data\\Spells\\Sounds\\%s.wav";
// GLOBAL: WIZ8 0x0064C664
char s_general_magic_sound_0064c664[] = "Data\\Spells\\Sounds\\GeneralMagic.wav";

// GLOBAL: WIZ8 0x0069B920
W8TextControl* g_portrait_controls_0069b920[8]; /* gpLevelButtons[uiSlot] */

// The condition-buttons panel and its eight buttons, created together by
// CreateConditionButtons. The asserts there name them gpConditionButtonsPanel
// and gpConditionButtons[uiSlot].
// GLOBAL: WIZ8 0x0069B900
W8ConditionButton* g_condition_buttons_0069b900[8];
// GLOBAL: WIZ8 0x0069B944
Controls* g_condition_buttons_panel_0069b944;
/* Party slot the level-up portrait button opens; -1 while idle. */
// GLOBAL: WIZ8 0x0069B948
int giLevelUpChar;

void OnLevelButtonActivate(void);

/* Refresh cached HP/stamina/spell portrait bar widths; dirty + redraw when
   any slot's displayed fraction (or numeric HP) changes. */
// FUNCTION: WIZ8 0x0059A3A0
void SyncPartyPortraitVitalsBars(void)
{
    int slot;

    for (slot = 0; slot < 8; ++slot) {
        W8MonsterManagerEntry* entry = &gXStatus.monster_manager_entries[slot];
        unsigned int hp_bar = 0;
        unsigned int stamina_bar = 0;
        unsigned int spell_bar = 0;
        W8Character* character;

        if (g_status_685170.buffers.XChar[slot].fOccupied == 0) {
            continue;
        }

        character = &g_status_685170.buffers.Char[slot];
        if (character->hp_current == 0) {
            hp_bar = 0;
            stamina_bar = 0;
        } else {
            unsigned int stamina;
            int left;
            int total;

            hp_bar = (character->hp_current * 0x2d) / static_cast<unsigned int>(character->uiHPMax);
            if (hp_bar == 0) {
                hp_bar = 1;
            }

            stamina = character->stamina;
            stamina_bar = ((((static_cast<int>(stamina) < 0) - 1) & stamina) * 0x2d) /
                          static_cast<unsigned int>(character->uiStaminaMax);
            if (stamina_bar == 0 && stamina != 0) {
                stamina_bar = 1;
            }

            if (SumCharacterSpellPoints(character) == 0) {
                spell_bar = 0;
            } else {
                left = SumCharacterSpellPointsLeft(character);
                if (left < 0) {
                    left = 0;
                } else {
                    left = SumCharacterSpellPointsLeft(character);
                }
                total = SumCharacterSpellPoints(character);
                spell_bar =
                    (static_cast<unsigned int>(left) * 0x2d) / static_cast<unsigned int>(total);
                if (spell_bar == 0 && SumCharacterSpellPointsLeft(character) != 0) {
                    spell_bar = 1;
                }
            }

            if (hp_bar != static_cast<unsigned int>(entry->cached_hp_bar) ||
                stamina_bar != static_cast<unsigned int>(entry->cached_stamina_bar) ||
                spell_bar != static_cast<unsigned int>(entry->cached_spell_bar) ||
                (g_settings_6850c8.numeric_hit_points != 0 &&
                 static_cast<int>(character->hp_current) != entry->cached_hp)) {
                entry->portrait_stats_dirty = 1;
                RequestRedraw(0x80000000);
            }
        }

        entry->cached_hp_bar = static_cast<int>(hp_bar);
        entry->cached_stamina_bar = static_cast<int>(stamina_bar);
        entry->cached_spell_bar = static_cast<int>(spell_bar);
        entry->cached_hp = static_cast<int>(character->hp_current);
    }
}

/* Open the slot's floating damage-number splat on a fresh hit, or accumulate
   into it while one is already up: the death variant runs the longer
   0x92-catalog animation when the character's hit points are gone, a forced
   portrait refresh defers the frame to -1, and a still-playing effect icon
   defers it likewise. */
// FUNCTION: WIZ8 0x0059AC40
void RecordCharacterDamage(int party_slot, unsigned int amount)
{
    bool splat_started = false;

    if (gXStatus.fSurprisePossible != 0) {
        return;
    }
    W8MonsterManagerEntry* entry = &gXStatus.monster_manager_entries[party_slot];
    if (entry->damage_splat_active == 0) {
        entry->damage_splat_amount = amount;
        entry->damage_splat_active = 1;
        if (Random(2) == 0) {
            entry->damage_splat_catalog = 0x90;
        } else {
            entry->damage_splat_catalog = 0x91;
        }
        if (g_status_685170.buffers.Char[party_slot].hp_current == 0) {
            entry->damage_splat_death_variant = 1;
            entry->damage_splat_end_frame = 0x1e;
        } else {
            entry->damage_splat_death_variant = 0;
            entry->damage_splat_end_frame = 8;
        }
        entry->damage_splat_frame = 0;
        splat_started = true;
        if (g_settings_6850c8.main_ui_mode != W8_MAIN_UI_MODE_PORTRAITS &&
            g_level_block->portrait_refresh_pending[party_slot] == 0) {
            RefreshSelectedPartyPortrait(party_slot);
            entry->auto_portrait_refresh = 1;
            entry->damage_splat_frame = -1;
        }
    } else {
        entry->damage_splat_amount += amount;
        entry->damage_splat_frame = 0;
        if (g_status_685170.buffers.Char[party_slot].hp_current == 0 &&
            entry->damage_splat_death_variant == 0) {
            entry->damage_splat_death_variant = 1;
            entry->damage_splat_end_frame = 0x1e;
        }
    }
    if (entry->keyboard_menu_open == 0) {
        RequestRedraw(1u << party_slot);
    }
    if (entry->effect_icon_active != 0 && splat_started) {
        entry->damage_splat_frame = -1;
        return;
    }
    entry->portrait_fx_clock = SetCountdownClock(100);
}

/* Draw the slot's floating damage-number splat over the portrait: the picked
   (or death-variant) catalog image at the current frame plus the running
   damage total in bold text for the first six frames. Frame -1 means the
   splat is deferred behind a portrait refresh or a still-playing effect
   icon. */
// FUNCTION: WIZ8 0x0059ADD0
void DrawDamageSplatOverlay(unsigned int party_slot)
{
    W8MonsterManagerEntry* entry = &gXStatus.monster_manager_entries[party_slot];
    int frame = entry->damage_splat_frame;
    int top = 0;

    if (frame != -1) {
        int left = ((party_slot & 1) != 0 ? 0x1ff : 0) + 0x16;
        switch (party_slot >> 1) {
        case 0:
            top = 0x12;
            break;
        case 1:
            top = 0x67;
            break;
        case 2:
            top = 0xbc;
            break;
        case 3:
            top = 0x111;
            break;
        }
        unsigned int object =
            entry->damage_splat_death_variant == 0 ? entry->damage_splat_catalog : 0x92;
        DrawCatalogImage(-0xe, object, 0, static_cast<short>(frame), left, top, 2, 0);
        if (entry->damage_splat_frame < 6) {
            W8ControlsRect bounds;
            bounds.left = left;
            bounds.top = top + 4;
            bounds.right = left + 0x52;
            bounds.bottom = top + 0x4c;
            W8TextBuffer splat_text(&bounds, 0, 0, 0, 4);
            splat_text.SetText(FormatWideString(g_format_d_0060aa20, entry->damage_splat_amount),
                               g_wiz_text_bold_font_683664);
            splat_text.RenderToTarget(0, 0, -0xe);
        }
        if (gXStatus.fCombatMode != 0) {
            entry->combat_portrait_dirty = 1;
        }
    }
}

/* Draw the in-flight spell/condition effect icon over the slot's portrait.
   frame -1 is the deferred-start state and draws nothing; the icon is skipped
   while a pending portrait refresh sits under another overlay (the settings
   mode parked off PORTRAITS with the slot's refresh flag set). */
// FUNCTION: WIZ8 0x0059B0F0
void DrawPortraitEffectIcon(unsigned int party_slot)
{
    int image = gXStatus.monster_manager_entries[party_slot].effect_icon_frame;
    int top = 0;
    if (image != -1) {
        switch (party_slot >> 1) {
        case 0:
            top = 0x12;
            break;
        case 1:
            top = 0x67;
            break;
        case 2:
            top = 0xbc;
            break;
        case 3:
            top = 0x111;
        }
        if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_PORTRAITS ||
            g_level_block->portrait_refresh_pending[party_slot] != 0) {
            DrawCatalogImageAndInvalidate(
                -0xe, gXStatus.monster_manager_entries[party_slot].effect_icon_catalog, 0, image,
                (party_slot & 1) << 9 | 0x17, top, 2, 0);
        }
    }
}

/* Advance the per-slot portrait FX counters on a 100ms clock and request a
   redraw when a visible slot's animation frame advances. */
// FUNCTION: WIZ8 0x0059B1A0
void TickPartyPortraitFx(void)
{
    unsigned char slot;

    for (slot = 0; slot < 8; ++slot) {
        W8MonsterManagerEntry* entry = &gXStatus.monster_manager_entries[slot];
        bool clock_expired;
        bool dirty = false;

        if (g_status_685170.buffers.XChar[slot].fOccupied == 0) {
            continue;
        }

        clock_expired = ClockIsTicking(entry->portrait_fx_clock) == 0;
        if (entry->damage_splat_active != 0) {
            if (clock_expired != 0) {
                entry->damage_splat_frame = entry->damage_splat_frame + 1;
                dirty = 1;
            }
            if (entry->damage_splat_frame == entry->damage_splat_end_frame) {
                entry->damage_splat_active = 0;
                entry->damage_splat_death_variant = 0;
            } else if (entry->damage_splat_death_variant != 0 && entry->damage_splat_frame == 0xd &&
                       entry->dead_portrait_revealed == 0) {
                entry->dead_portrait_revealed = 1;
            }
        }
        if (entry->effect_icon_active != 0) {
            if (clock_expired != 0) {
                entry->effect_icon_frame = entry->effect_icon_frame + 1;
                dirty = 1;
            }
            if (entry->effect_icon_frame == entry->effect_icon_end_frame) {
                entry->effect_icon_active = 0;
            }
        }
        if (clock_expired != 0) {
            entry->portrait_fx_clock = SetCountdownClock(100);
        }
        if (dirty != 0 && entry->keyboard_menu_open == 0) {
            RequestRedraw(1u << (slot & 0x1f));
        }
    }
}

/* Clear each occupied slot's damage-splat and effect-icon portrait overlays
   and rearm the shared FX clock; the main-game screen leave runs it so a
   pending animation does not survive the screen transition. */
// FUNCTION: WIZ8 0x0059B270
void ResetPartyPortraitFx(void)
{
    unsigned char slot;

    for (slot = 0; slot < 8; ++slot) {
        W8MonsterManagerEntry* entry = &gXStatus.monster_manager_entries[slot];

        if (g_status_685170.buffers.XChar[slot].fOccupied == 0) {
            continue;
        }
        entry->effect_icon_active = 0;
        entry->effect_icon_frame = -1;
        entry->effect_icon_catalog = -1;
        entry->damage_splat_active = 0;
        entry->damage_splat_death_variant = 0;
        entry->dead_portrait_revealed = 0;
        entry->damage_splat_frame = -1;
        entry->portrait_fx_clock = SetCountdownClock(0);
    }
}

/* Draw each occupied, living and eligible party slot's combat portrait in the
   side strip once the slot's dirty flag is raised: the normal frame, or the
   alternate while the slot is the hovered combat slot, then the can't-act
   badge or the party-member target marker. The slot currently acting is drawn
   by the action panel instead. */
// FUNCTION: WIZ8 0x0059B720
void RedrawCombatPortraits0059B720(void)
{
    int portrait_x;
    int badge_x;
    int row_y;
    int portrait_image;
    int slot;

    for (slot = 0; slot < 8; ++slot) {
        W8PartySlotRow* party_row = &g_status_685170.buffers.XChar[slot];
        W8Character* character = &g_status_685170.buffers.Char[slot];
        W8MonsterManagerEntry* entry = &gXStatus.monster_manager_entries[slot];
        W8CombatCharacterRow* combat_row = &g_combat_state->characters[slot];

        if (party_row->fOccupied == 0 || character->hp_current == 0 ||
            character->highest_condition >= W8_CONDITION_DEAD ||
            combat_row->portrait_image_084 == -1 || entry->combat_portrait_dirty == 0 ||
            slot == g_level_block->combat_slot) {
            continue;
        }
        if ((slot & 1) == 0) {
            portrait_x = 0x19;
            badge_x = 0x19;
        } else {
            portrait_x = 0x253;
            badge_x = 0x25b;
        }
        row_y = (slot >> 1) * 0x55;
        if (g_settings_6850c8.main_ui_mode != W8_MAIN_UI_MODE_PORTRAITS &&
            g_level_block->portrait_refresh_pending[slot] == 0) {
            ClearSurfaceRect(portrait_x, row_y + 0x37, portrait_x + 0x14, row_y + 0x58);
            InvalidateRegion(portrait_x, row_y + 0x37, portrait_x + 0x14, row_y + 0x58, 0);
        }
        if (g_level_block->party_slots_170[4] == -1 || g_level_block->party_slots_170[4] != slot) {
            portrait_image = combat_row->portrait_image_084;
        } else {
            portrait_image = combat_row->portrait_image_alternate_088;
        }
        DrawCatalogImageAndInvalidate(-0xe, 0x8a, 0, portrait_image, portrait_x, row_y + 0x44, 2,
                                      0);
        if (CharacterCanSwitchTo(slot, W8_TARGETING_CONTEXT_IN_COMBAT, 0, 0) == 0) {
            DrawCatalogImageAndInvalidate(-0xe, 0x8b, 0, 0, badge_x, row_y + 0x37, 2, 0);
        } else if (party_row->target_in_combat.iType == W8_TARGET_KIND_CHARACTER) {
            DrawCatalogImageAndInvalidate(
                -0xe, 0x8c, 0,
                g_status_685170.buffers.XChar[party_row->target_in_combat.iChar].party_order_index,
                badge_x, row_y + 0x38, 2, 0);
        }
        entry->combat_portrait_dirty = 0;
    }
}

/* Toggle numeric hit-point display on the party portraits and invalidate all
   eight slot masks so the new mode repaints everywhere. */
// FUNCTION: WIZ8 0x0059AA30
void ToggleNumericHitPoints(void)
{
    g_settings_6850c8.numeric_hit_points = g_settings_6850c8.numeric_hit_points == 0;
    for (unsigned int slot = 0; slot < 8; slot++) {
        RequestRedraw(1u << slot);
    }
}

/* The slot's anchor positions for the keyboard menu and portrait band: the
   panel corner, the band's two x edges (their order swaps with the column),
   the grid row and the column pixel. 'adjust' applies the compact-display
   shift used when the party display is a single column. */
// FUNCTION: WIZ8 0x0059AA60
void GetPartySlotMenuAnchor(int party_slot, int* menu_x, int* menu_y, int* band_menu_edge,
                            int* band_portrait_edge, int* grid_row, int* column_x, int adjust)
{
    switch (party_slot) {
    case 0:
        *grid_row = 1;
        *column_x = 0;
        *menu_x = 0;
        *menu_y = 0x12;
        *band_menu_edge = *menu_x + 2;
        *band_portrait_edge = *menu_x + 0x69;
        break;
    case 1:
        *grid_row = 2;
        *column_x = 0x200;
        *menu_x = 0x200;
        *menu_y = 0x12;
        *band_menu_edge = *menu_x + 0x6b;
        *band_portrait_edge = *menu_x;
        break;
    case 2:
        *grid_row = 5;
        *column_x = 0;
        *menu_x = 0;
        *menu_y = 0x67;
        *band_menu_edge = *menu_x + 2;
        *band_portrait_edge = *menu_x + 0x69;
        break;
    case 3:
        *grid_row = 6;
        *column_x = 0x200;
        *menu_x = 0x200;
        *menu_y = 0x67;
        *band_menu_edge = *menu_x + 0x6b;
        *band_portrait_edge = *menu_x;
        break;
    case 4:
        *grid_row = 9;
        *column_x = 0;
        *menu_x = 0;
        *menu_y = 0xbc;
        *band_menu_edge = *menu_x + 2;
        *band_portrait_edge = *menu_x + 0x69;
        break;
    case 5:
        *grid_row = 10;
        *column_x = 0x200;
        *menu_x = 0x200;
        *menu_y = 0xbc;
        *band_menu_edge = *menu_x + 0x6b;
        *band_portrait_edge = *menu_x;
        break;
    case 6:
        *grid_row = 0xd;
        *column_x = 0;
        *menu_x = 0;
        *menu_y = 0x111;
        *band_menu_edge = *menu_x + 2;
        *band_portrait_edge = *menu_x + 0x69;
        break;
    case 7:
        *grid_row = 0xe;
        *column_x = 0x200;
        *menu_x = 0x200;
        *menu_y = 0x111;
        *band_menu_edge = *menu_x + 0x6b;
        *band_portrait_edge = *menu_x;
        break;
    default:
        break;
    }
    if (adjust != 0 && g_settings_6850c8.main_ui_mode != W8_MAIN_UI_MODE_PORTRAITS &&
        static_cast<unsigned int>(g_settings_6850c8.main_ui_mode) <=
            static_cast<unsigned int>(W8_MAIN_UI_MODE_RADAR)) {
        if ((party_slot & 1) == 0) {
            *menu_x -= 0x69;
            *band_menu_edge = -1;
            *band_portrait_edge -= 0x69;
            --*grid_row;
            *column_x = 0;
        } else {
            *menu_x = 0x269;
            *band_menu_edge = -1;
            *band_portrait_edge = 0x269;
            ++*grid_row;
            *column_x = 0x269;
        }
    }
}

/* Repaint one party slot's HP, stamina and optional spell-point bars beside
   the portrait, including numeric HP text when that option is enabled. */
// FUNCTION: WIZ8 0x0059A540
void RedrawPartyPortraitBars(unsigned int party_slot, char slot_enabled)
{
    W8MonsterManagerEntry* entry = &gXStatus.monster_manager_entries[party_slot];
    W8Character* character = &g_status_685170.buffers.Char[party_slot];
    int menu_x;
    int menu_y;
    int band_menu_edge;
    int band_portrait_edge;
    int grid_row;
    int column_x;
    int bar_y;
    int hp_bar_x;
    unsigned int stamina_bar_x;
    unsigned int spell_bar_x;
    int hp_catalog;
    int stamina_catalog;
    int spell_catalog;
    unsigned int numeric_hp_mode;
    UINT32 pitch;
    unsigned short fill_color;

    if (character->hp_current != 0) {
        GetPartySlotMenuAnchor(party_slot, &menu_x, &menu_y, &band_menu_edge, &band_portrait_edge,
                               &grid_row, &column_x, slot_enabled);
        if (g_settings_6850c8.numeric_hit_points != 0) {
            bar_y = 0x11;
            hp_bar_x = 3;
            stamina_bar_x = 9;
            spell_bar_x = 0xf;
            hp_catalog = 0x52;
            stamina_catalog = 0x53;
            spell_catalog = 0x54;
        } else {
            bar_y = 0x17;
            hp_bar_x = 6;
            stamina_bar_x = 10;
            spell_bar_x = 0xe;
            hp_catalog = 0x4f;
            stamina_catalog = 0x50;
            spell_catalog = 0x51;
        }
        numeric_hp_mode = g_settings_6850c8.numeric_hit_points != 0;

        DrawCatalogImage(-14, hp_catalog, 0, 0, band_portrait_edge + hp_bar_x, bar_y + menu_y, 2,
                         0);
        {
            int fill_height = 0x2d - entry->cached_hp_bar;
            if (fill_height != 0) {
                int draw_y = (bar_y - numeric_hp_mode) + menu_y;
                int draw_x = hp_bar_x + band_portrait_edge;
                int line_count = (-(numeric_hp_mode != 0) & 2) + 3;
                char* screen = static_cast<char*>(LockPrimarySurface(&pitch));

                if (line_count != 0) {
                    int end_y = draw_y + fill_height;
                    fill_color = Get16BPPColor(0x10101);
                    do {
                        LineDraw(1, draw_x, draw_y, draw_x, end_y, fill_color, screen);
                        draw_x = draw_x + 1;
                        line_count = line_count - 1;
                    } while (line_count != 0);
                }
                UnlockPrimarySurface();
            }
        }

        DrawCatalogImage(-14, stamina_catalog, 0, 0, band_portrait_edge + stamina_bar_x,
                         bar_y + menu_y, 2, 0);
        {
            int fill_height = 0x2d - entry->cached_stamina_bar;
            if (fill_height != 0) {
                int draw_y = (bar_y - numeric_hp_mode) + menu_y;
                int draw_x = stamina_bar_x + band_portrait_edge;
                int line_count = (-(numeric_hp_mode != 0) & 2) + 3;
                char* screen = static_cast<char*>(LockPrimarySurface(&pitch));

                if (line_count != 0) {
                    int end_y = draw_y + fill_height;
                    fill_color = Get16BPPColor(0x10101);
                    do {
                        LineDraw(1, draw_x, draw_y, draw_x, end_y, fill_color, screen);
                        draw_x = draw_x + 1;
                        line_count = line_count - 1;
                    } while (line_count != 0);
                }
                UnlockPrimarySurface();
            }
        }

        if (SumCharacterSpellPoints(character) != 0) {
            DrawCatalogImage(-14, spell_catalog, 0, 0, band_portrait_edge + spell_bar_x,
                             bar_y + menu_y, 2, 0);
            {
                int fill_height = 0x2d - entry->cached_spell_bar;
                if (fill_height != 0) {
                    int draw_y = (bar_y - numeric_hp_mode) + menu_y;
                    int draw_x = spell_bar_x + band_portrait_edge;
                    int line_count = (-(numeric_hp_mode != 0) & 2) + 3;
                    char* screen = static_cast<char*>(LockPrimarySurface(&pitch));

                    if (line_count != 0) {
                        int end_y = draw_y + fill_height;
                        fill_color = Get16BPPColor(0x10101);
                        do {
                            LineDraw(1, draw_x, draw_y, draw_x, end_y, fill_color, screen);
                            draw_x = draw_x + 1;
                            line_count = line_count - 1;
                        } while (line_count != 0);
                    }
                    UnlockPrimarySurface();
                }
            }
        }

        InvalidateRegion(band_portrait_edge, bar_y + menu_y, band_portrait_edge + 0x18,
                         bar_y + menu_y + 0x2d, 0);
        if (g_settings_6850c8.numeric_hit_points != 0) {
            W8TextBuffer text;
            W8ControlsRect bounds;

            bounds.left = band_portrait_edge + 2;
            bounds.right = band_portrait_edge + 0x14;
            bounds.top = menu_y + 0x3e;
            bounds.bottom = menu_y + 0x46;
            text.SetLayoutMode(g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C);
            text.SetLayoutBounds(&bounds, 1, 1);
            SetFontObjectPalette16BPP(g_smfnt_font_683694, g_font_palette_smfnt_68ee10);
            text.SetText(FormatWideString(g_format_d_0060aa20, character->hp_current),
                         g_smfnt_font_683694);
            InvalidateRegion(bounds.left, bounds.top + 1, bounds.right, bounds.bottom, 0);
            ColorFillVideoSurfaceArea(-14, bounds.left, bounds.top + 1, bounds.right, bounds.bottom,
                                      0x8000);
            text.RenderToTarget(0, 0, -14);
        }
    }

    entry->portrait_stats_dirty = 0;
}

// FUNCTION: WIZ8 0x0059AF40
void StageMonsterCastIcon0059AF40(unsigned int party_slot, int realm, char alternate, int spell_id)
{
    if (g_current_screen_state.id != W8_SCREEN_MAIN_GAME) {
        return;
    }
    W8MonsterManagerEntry* entry = &gXStatus.monster_manager_entries[party_slot];
    entry->effect_icon_active = 1;
    entry->effect_icon_frame = 0;
    int catalog;
    switch (realm) {
    case 0:
        catalog = 0xac - (alternate != 0);
        break;
    case 1:
        catalog = 0xae - (alternate != 0);
        break;
    case 2:
        catalog = 0xb0 - (alternate != 0);
        break;
    case 3:
        catalog = 0xb2 - (alternate != 0);
        break;
    case 4:
        catalog = 0xb4 - (alternate != 0);
        break;
    case 5:
        catalog = 0xb6 - (alternate != 0);
        break;
    default:
        catalog = 0xaa;
    }
    entry->effect_icon_catalog = catalog;
    entry->effect_icon_end_frame = GetCatalogVideoObject(catalog, 0, 0)->usNumberOfObjects;
    char* sound =
        spell_id != 0 && g_spell_records[spell_id].sound_name[0] != 0
            ? FormatString(s_spell_sound_format_0061aa9c, g_spell_records[spell_id].sound_name)
            : s_general_magic_sound_0064c664;
    SoundPlay(sound, 0);
    if (g_settings_6850c8.main_ui_mode != W8_MAIN_UI_MODE_PORTRAITS &&
        g_level_block->portrait_refresh_pending[party_slot] == 0) {
        RefreshSelectedPartyPortrait(party_slot);
        entry->auto_portrait_refresh = 1;
        entry->effect_icon_frame = -1;
    }
    if (entry->combat_portrait_dirty == 0) {
        RequestRedraw(1 << (party_slot & 0x1f));
    }
    if (entry->damage_splat_active == 0) {
        entry->portrait_fx_clock = SetCountdownClock(100);
        return;
    }
    entry->effect_icon_frame = -1;
}

// FUNCTION: WIZ8 0x005993A0
bool PreparePartyPortraitOverlay(unsigned int party_slot, unsigned int left, unsigned int top)
{
    if (gXStatus.fNpcDialogueMode != 0 && (party_slot & 1) != 0 &&
        IsPortraitObscuredByNpcDialogue(party_slot) != 0) {
        return 0;
    }
    if ((g_level_block == 0 ||
         (g_settings_6850c8.main_ui_mode != W8_MAIN_UI_MODE_FORMATION &&
          g_settings_6850c8.main_ui_mode != W8_MAIN_UI_MODE_RADAR) ||
         g_level_block->portrait_refresh_pending[party_slot] != 0) &&
        g_status_685170.buffers.XChar[party_slot].fOccupied != 0) {
        W8Character* character = &g_status_685170.buffers.Char[party_slot];
        if (character->hp_current != 0) {
            int portrait = character->portrait_index;
            int flags = 2;
            if ((g_portrait_descriptors_6483d0[portrait].render_mode == 1 &&
                 (party_slot & 1) == 0) ||
                (g_portrait_descriptors_6483d0[portrait].render_mode == 2 &&
                 (party_slot & 1) != 0)) {
                flags = 0x1002;
            }
            if (BlitPartyPortraitAnimation(portrait, left, top, flags, party_slot, 0) != 0 &&
                ((gXStatus.fCombatMode != 0 &&
                  g_combat_state->characters[party_slot].dead_34 != 0) ||
                 gXStatus.fSurprisePossible != 0 || character->highest_condition == 0x13)) {
                return 1;
            }
        }
    }
    return 0;
}

/* Repaint one party-slot portrait band: frame, live or dead portrait, item
   hands, HP/stamina chrome, labels, condition/enchantment icons, and any
   active overlay callees for that slot. */
// FUNCTION: WIZ8 0x005994C0
void RedrawPartyPortraitOverlay(unsigned int party_slot, char highlighted, char overlay_ready,
                                char slot_enabled)
{
    int menu_x;
    int menu_y;
    int band_menu_edge;
    int band_portrait_edge;
    int grid_row;
    int column_x;
    W8Character* character;
    W8PartySlotRow* party_row;
    W8MonsterManagerEntry* entry;
    int portrait_flags;
    int portrait_catalog;
    int left_condition_x;
    int right_condition_x;
    int condition_frame;
    int enchantment_frame;
    wchar_t text[64];
    short text_width;
    unsigned int text_shade;
    int main_hand_item_id;
    int off_hand_item_id;
    unsigned char show_off_hand_row;
    unsigned short hp_bar_frame;

    if (gXStatus.fNpcDialogueMode != 0 && (party_slot & 1) != 0) {
        IsPortraitObscuredByNpcDialogue(party_slot);
    }

    GetPartySlotMenuAnchor(party_slot, &menu_x, &menu_y, &band_menu_edge, &band_portrait_edge,
                           &grid_row, &column_x, slot_enabled);

    party_row = &g_status_685170.buffers.XChar[party_slot];
    character = &g_status_685170.buffers.Char[party_slot];
    entry = &gXStatus.monster_manager_entries[party_slot];

    if (party_row->fOccupied == 0) {
        DrawCatalogImage(-14, 0x31, 0, 0, menu_x + 0x14, menu_y, 2, 0);
    }

    if (overlay_ready != 0) {
        if (party_row->fOccupied == 0) {
            portrait_catalog = 0x34;
            portrait_flags = 2;
            DrawCatalogImage(-14, portrait_catalog, 0, 0, menu_x + 0x14, menu_y, portrait_flags, 0);
        } else if (character->hp_current == 0 &&
                   (entry->damage_splat_death_variant == 0 || entry->dead_portrait_revealed != 0)) {
            portrait_catalog = g_dead_portrait_catalog_ids_6488d0[character->iRace][1];
            portrait_flags = (party_slot & 1) == 0 ? 2 : 0x1002;
            DrawCatalogImage(-14, portrait_catalog, 0, 0, menu_x + 0x14, menu_y, portrait_flags, 0);
        } else {
            portrait_catalog = character->portrait_index;
            portrait_flags = 2;
            if ((g_portrait_descriptors_6483d0[portrait_catalog].render_mode == 1 &&
                 (party_slot & 1) == 0) ||
                (g_portrait_descriptors_6483d0[portrait_catalog].render_mode == 2 &&
                 (party_slot & 1) != 0)) {
                portrait_flags = 0x1002;
            }
            RenderPartyPortrait0052EB00(portrait_catalog, menu_x + 0x14, menu_y, portrait_flags, 1,
                                        party_slot);
        }
        DrawCatalogImage(-14, 0x82, 0, static_cast<short>(grid_row), column_x, menu_y, 2, 0);
    }

    DrawCatalogImage(-14, 0x82, 0, 0x10, menu_x + 0x17, menu_y, 2, 0);

    if (party_row->fOccupied != 0) {
        if (overlay_ready != 0) {
            if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_PORTRAITS ||
                g_level_block->portrait_refresh_pending[party_slot] != 0) {
                main_hand_item_id = character->EquippedItem[6].iItemNo;
                if (main_hand_item_id == -1 ||
                    (g_item_records[main_hand_item_id].flags_041 & 4) == 0) {
                    show_off_hand_row = 0;
                    hp_bar_frame = 0;
                } else {
                    show_off_hand_row = 1;
                    hp_bar_frame = 2;
                }
                DrawCatalogImage(-14, 0x80, 0, static_cast<short>(hp_bar_frame), band_menu_edge,
                                 menu_y, 2, 0);

                if (main_hand_item_id == -1) {
                    DrawCatalogImage(-14, g_empty_hand_catalog_ids_649dd4[character->iRace * 2], 0,
                                     0, band_menu_edge + 4, menu_y + 0x17, 2, 0);
                } else {
                    DrawCatalogImage(
                        -14, g_item_video_objects_68ec68.GetOrCreateVideoObject(main_hand_item_id),
                        0, 2, band_menu_edge + 3, menu_y + 0x17, 2, 0);
                    if (character->EquippedItem[6].stack_count != 0) {
                        swprintf(text, g_format_d_0060aa20,
                                 static_cast<int>(character->EquippedItem[6].stack_count));
                        SetFont(g_smfnt_font_683694);
                        SetFontObjectPalette16BPP(g_smfnt_font_683694, g_font_palette_smfnt_68ee10);
                        text_width = StringPixLength(text, g_smfnt_font_683694);
                        gprintf((band_menu_edge - (text_width + 1) / 2) + 8, menu_y + 0x26,
                                const_cast<UINT16*>(g_format_s_006068e4), text);
                    }
                }

                if (show_off_hand_row == 0) {
                    off_hand_item_id = character->EquippedItem[7].iItemNo;
                    if (off_hand_item_id == -1) {
                        DrawCatalogImage(-14,
                                         g_empty_hand_catalog_ids_649dd4[character->iRace * 2 + 1],
                                         0, 0, band_menu_edge + 4, menu_y + 0x2f, 2, 0);
                    } else {
                        DrawCatalogImage(
                            -14,
                            g_item_video_objects_68ec68.GetOrCreateVideoObject(off_hand_item_id), 0,
                            2, band_menu_edge + 3, menu_y + 0x2f, 2, 0);
                        if (character->EquippedItem[7].stack_count != 0) {
                            swprintf(text, g_format_d_0060aa20,
                                     static_cast<int>(character->EquippedItem[7].stack_count));
                            SetFont(g_smfnt_font_683694);
                            SetFontObjectPalette16BPP(g_smfnt_font_683694,
                                                      g_font_palette_smfnt_68ee10);
                            text_width = StringPixLength(text, g_smfnt_font_683694);
                            gprintf((band_menu_edge - (text_width + 1) / 2) + 0xb, menu_y + 0x3e,
                                    const_cast<UINT16*>(g_format_s_006068e4), text);
                        }
                    }
                }
            }

            hp_bar_frame = g_settings_6850c8.numeric_hit_points == 0 ? 1 : 3;
            DrawCatalogImage(-14, 0x80, 0, static_cast<short>(hp_bar_frame), band_portrait_edge,
                             menu_y, 2, 0);
            RedrawPartyPortraitBars(party_slot, slot_enabled);

            SetFont(g_smfnt_font_683694);
            if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_PORTRAITS ||
                g_level_block->portrait_refresh_pending[party_slot] != 0) {
                swprintf(text, g_format_d_0060aa20, character->armor_class_average);
                if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
                    unsigned short* palette = g_font_palette_smfnt_68ee10;
                    if (character->load_category != 0) {
                        palette = g_font_state_palettes_68ee1c
                            [g_load_category_palettes_648c48[character->load_category]];
                    }
                    SetFontObjectPalette16BPP(g_smfnt_font_683694, palette);
                }
                text_width = StringPixLength(text, g_smfnt_font_683694);
                gprintf((0xd - text_width) / 2 + 3 + band_menu_edge, menu_y + 3,
                        const_cast<UINT16*>(g_format_s_006068e4), text);
            }

            wcscpy(
                text,
                gppStringList[g_profession_name_message_ids_61e3f0[character->iProfession + 16]]);
            if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
                SetFontObjectPalette16BPP(
                    g_smfnt_font_683694,
                    g_font_state_palettes_68ee1c[party_row->party_order_index]);
            }
            text_width = StringPixLength(text, g_smfnt_font_683694);
            gprintf((0x12 - text_width) / 2 + 2 + band_portrait_edge, menu_y + 3,
                    const_cast<UINT16*>(g_format_s_006068e4), text);

            SetFont(g_font_683660);
            if (g_current_screen_state.id != W8_SCREEN_MAIN_GAME ||
                (SetFontObjectPalette16BPP(g_font_683660, g_colour_68ee08),
                 g_current_screen_state.id != W8_SCREEN_MAIN_GAME) ||
                (text_shade = 1,
                 g_level_block->party_slots_170[2] != static_cast<int>(party_slot))) {
                text_shade = 4;
            }
            SetObjectShade(g_wiz_text_font_secondary_object_683680, text_shade);
            text_width = StringPixLength(character->name, g_font_683660);
            gprintf((0x54 - text_width) / 2 + 0x15 + menu_x, menu_y + 0x49,
                    const_cast<UINT16*>(g_format_s_006068e4), character->name);
            SetObjectShade(g_wiz_text_font_secondary_object_683680, 4);

            if (gXStatus.fCombatMode != 0) {
                entry->combat_portrait_dirty = 1;
            }
            g_condition_buttons_0069b900[party_slot]->Invalidate(0);
        }

        if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
            (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_FORMATION ||
             g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_RADAR) &&
            g_level_block->portrait_refresh_pending[party_slot] == 0) {
            int highlight_x;
            int highlight_y;
            int highlight_catalog;

            if (highlighted == 0 || gfLeftButtonState != 0 ||
                g_level_block->portrait_flash_218 != 0) {
                if (g_status_685170.selected_character != static_cast<int>(party_slot)) {
                    goto draw_condition_icons;
                }
                highlight_x = band_portrait_edge + 1;
                highlight_catalog = 0x5f;
                highlight_y = menu_y + 2;
            } else {
                highlight_x = band_portrait_edge + 1;
                highlight_catalog = 0x5e;
                highlight_y = menu_y + 2;
            }
            DrawCatalogImage(-14, highlight_catalog, 0, 0, highlight_x, highlight_y, 2, 0);
        } else {
            int highlight_x;
            int highlight_y;
            int highlight_catalog;

            highlight_y = menu_y;
            if (highlighted == 0 ||
                (gfLeftButtonState != 0 && gXStatus.fReviewCharacterMode == 0) ||
                (g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
                 g_level_block->portrait_flash_218 != 0)) {
                if (g_status_685170.selected_character != static_cast<int>(party_slot)) {
                    goto draw_condition_icons;
                }
                highlight_x = menu_x + 0x17;
                highlight_catalog = 0x33;
            } else {
                highlight_x = menu_x + 0x17;
                highlight_catalog = 0x32;
            }
            DrawCatalogImage(-14, highlight_catalog, 0, 0, highlight_x, highlight_y, 2, 0);
        }
    }

draw_condition_icons:
    if ((party_slot & 1) == 0) {
        left_condition_x = menu_x + 0x19;
        right_condition_x = menu_x + 0x58;
        condition_frame = 0x2f;
        enchantment_frame = 0x30;
    } else {
        right_condition_x = menu_x + 0x19;
        left_condition_x = menu_x + 0x58;
        enchantment_frame = 0x2f;
        condition_frame = 0x30;
    }

    if (character->highest_condition != 0) {
        condition_frame = static_cast<int>(character->highest_condition) + 0xb6;
    }
    DrawCatalogImage(-14, condition_frame, 0, 0, left_condition_x, menu_y + 3, 2, 0);

    if (character->enchantment_top != 0) {
        enchantment_frame = character->enchantment_top + 0xc9;
    }
    DrawCatalogImage(-14, enchantment_frame, 0, 0, right_condition_x, menu_y + 3, 2, 0);

    if (party_row->fOccupied != 0 && g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
        if (g_level_block->party_slots_170[0] == static_cast<int>(party_slot)) {
            DrawCatalogImage(-14, 0x60, 0, 0, left_condition_x - 1, menu_y + 2, 2, 0);
        } else if (character->highest_condition != 0) {
            DrawCatalogImage(-14, 0x61, 0, 0, left_condition_x - 1, menu_y + 2, 2, 0);
        }

        if (g_level_block->party_slots_170[1] == static_cast<int>(party_slot)) {
            DrawCatalogImage(-14, 0x60, 0, 0, right_condition_x - 1, menu_y + 2, 2, 0);
        } else if (character->highest_condition != 0) {
            DrawCatalogImage(-14, 0x61, 0, 0, right_condition_x - 1, menu_y + 2, 2, 0);
        }

        if (g_level_block->party_slots_170[2] == static_cast<int>(party_slot) &&
            (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_PORTRAITS ||
             g_level_block->portrait_refresh_pending[party_slot] != 0)) {
            DrawCatalogImage(-14, 0x62, 0, 0, menu_x + 0x13, menu_y + 0x48, 2, 0);
        }

        if ((g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_PORTRAITS ||
             g_level_block->portrait_refresh_pending[party_slot] != 0) &&
            g_level_block->party_slots_170[5] == static_cast<int>(party_slot)) {
            int assay_y;
            int assay_catalog;

            if (g_level_block->portrait_assay_hover_mode == 1) {
                assay_y = menu_y + 0x15;
                assay_catalog = 99;
            } else if (g_level_block->portrait_assay_hover_mode == 2) {
                assay_y = menu_y + 0x2d;
                assay_catalog = 99;
            } else if (g_level_block->portrait_assay_hover_mode == 3) {
                assay_y = menu_y + 0x15;
                assay_catalog = 0x73;
            } else {
                goto portrait_fx;
            }
            DrawCatalogImage(-14, assay_catalog, 0, 0, band_menu_edge + 1, assay_y, 2, 0);
        }
    }

portrait_fx:
    if (entry->damage_splat_active != 0) {
        DrawDamageSplatOverlay(party_slot);
    }
    if (entry->effect_icon_active != 0) {
        DrawPortraitEffectIcon(party_slot);
    }

    if (g_level_block->party_slots_170[3] == static_cast<int>(party_slot)) {
        int hp_overlay_y;
        int hp_overlay_x;
        int hp_overlay_catalog;

        if (g_settings_6850c8.numeric_hit_points == 0) {
            hp_overlay_y = menu_y + 0x15;
            hp_overlay_x = band_portrait_edge + 3;
            hp_overlay_catalog = 100;
        } else {
            hp_overlay_y = menu_y + 0xe;
            hp_overlay_catalog = 0x65;
            hp_overlay_x = band_portrait_edge;
        }
        DrawCatalogImage(-14, hp_overlay_catalog, 0, 0, hp_overlay_x, hp_overlay_y, 2, 0);
    }

    if (gXStatus.fCombatMode == 0 && party_slot < 8 && party_row->fOccupied != 0) {
        g_portrait_controls_0069b920[party_slot]->Invalidate(0);
    }

    if (g_level_block->portrait_overlay_party_slot == static_cast<int>(party_slot)) {
        DrawPortraitConditionOverlay(g_level_block->portrait_overlay_party_slot);
    }
    if (g_level_block->condition_orb_party_slot == static_cast<int>(party_slot)) {
        DrawPortraitEnchantmentOverlay(g_level_block->condition_orb_party_slot);
    }
    if (g_level_block->enchantment_orb_party_slot == static_cast<int>(party_slot)) {
        DrawPortraitVitalsOverlay(g_level_block->enchantment_orb_party_slot);
    }
    if (g_level_block->condition_highlight_party_slot == static_cast<int>(party_slot)) {
        DrawPortraitStatusOverlay(g_level_block->condition_highlight_party_slot);
    }

    if (g_settings_6850c8.main_ui_mode != W8_MAIN_UI_MODE_PORTRAITS &&
        g_level_block->portrait_refresh_pending[party_slot] != 0 &&
        (g_level_block->condition_highlight_party_slot != -1 ||
         g_level_block->portrait_overlay_party_slot != -1)) {
        RefreshTrackedPortraitOverlay();
    }

    if (overlay_ready != 0 && gXStatus.fNpcDialogueMode != 0 &&
        g_screen_state_00649f1c->dialogue_layout == W8_DIALOGUE_LAYOUT_TRANSCRIPT &&
        g_screen_state_00649f1c->scripted_dialogue == 0 &&
        g_screen_state_00649f1c->dialogue_panel_hidden == 0 &&
        g_screen_state_00649f1c->script_busy == 0 &&
        g_screen_state_00649f1c->dialogue_hidden == 0 && gXStatus.scripted_scene_19b7 == 0) {
        SetNpcDialoguePanelVisible(1);
    }
}

// FUNCTION: WIZ8 0x0059A110
void ShadeStatusBarGap0059A110(int length, int left, int top)
{
    if (length != 0) {
        int rows = (g_settings_6850c8.numeric_hit_points != 0 ? 2 : 0) + 3;
        unsigned int pitch;
        char* screen = static_cast<char*>(LockPrimarySurface(&pitch));
        while (rows != 0) {
            short color = Get16BPPColor(0x10101);
            LineDraw(1, left, top, left, top + length, color, screen);
            ++left;
            --rows;
        }
        UnlockPrimarySurface();
    }
}

// FUNCTION: WIZ8 0x0059BAD0
void ReleasePortraitControls(void)
{
    RegionSetDisable(5);
    W8TextControl** control = g_portrait_controls_0069b920;
    do {
        (*control)->SetActive(0);
        ++control;
    } while (control < g_portrait_controls_0069b920 + 8);
    Controls* panel = g_panel_69b940;
    if (panel != 0) {
        delete panel;
        g_panel_69b940 = 0;
    }
    control = g_portrait_controls_0069b920;
    do {
        if (*control != 0) {
            delete *control;
            *control = 0;
        }
        ++control;
    } while (control < g_portrait_controls_0069b920 + 8);
}

// FUNCTION: WIZ8 0x0059BF70
void ReleaseConditionButtons(void)
{
    Controls* panel = g_condition_buttons_panel_0069b944;
    if (panel != 0) {
        delete panel;
        g_condition_buttons_panel_0069b944 = 0;
    }
    W8ConditionButton** control = g_condition_buttons_0069b900;
    do {
        if (*control != 0) {
            delete *control;
            *control = 0;
        }
        ++control;
    } while (control < g_condition_buttons_0069b900 + 8);
}

// FUNCTION: WIZ8 0x0059BB40
void DisablePortraitControls0059BB40(void)
{
    RegionSetDisable(5);
    W8TextControl** control = g_portrait_controls_0069b920;
    do {
        (*control)->SetActive(0);
        ++control;
    } while (control < g_portrait_controls_0069b920 + 8);
}

/* Hide the condition-button region set and clear any open condition highlight. */
// FUNCTION: WIZ8 0x0059C030
void DisableConditionButtons0059C030(void)
{
    RegionSetDisable(6);
    g_condition_buttons_panel_0069b944->SetEnabled(false);
    if (g_level_block->condition_highlight_party_slot != -1) {
        g_level_block->condition_highlight_party_slot = -1;
        DismissHighlightOverlay();
        RequestRedraw(0x8000);
        RequestRedraw(0xff);
    }
}

/* Show the condition-button region set for a non-normal layout. */
// FUNCTION: WIZ8 0x0059BFC0
void EnableConditionButtons0059BFC0(void)
{
    W8ConditionButton** control;

    if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_PORTRAITS) {
        srAssertFail("gConfig.uiCurrentLayout != LAYOUT_NORMAL",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\MGSPortraits.cpp", 0xa0c, 0);
    }
    RegionSetEnable(6);
    g_condition_buttons_panel_0069b944->SetEnabled(true);
    control = g_condition_buttons_0069b900;
    do {
        (*control)->SetEnabled(true);
        ++control;
    } while (control < g_condition_buttons_0069b900 + 8);
    g_condition_buttons_panel_0069b944->Invalidate(0);
}

// FUNCTION: WIZ8 0x0059BB70
void EnablePortraitAdvanceRegions0059BB70(void)
{
    RegionSetEnable(5);
    unsigned int state_offset = 0;
    int party_slot = 0;
    do {
        if (!IsCharacterReadyToAdvance(party_slot) ||
            g_status_685170.buffers.XChar[party_slot].portrait_advance_103 == 0) {
            DisableRegionInput(party_slot + 0x12);
        } else {
            EnableRegionInput(party_slot + 0x12);
        }
        state_offset += 0x106;
        ++party_slot;
    } while (state_offset < 0x830);
}

// FUNCTION: WIZ8 0x0059BBD0
void InvalidatePortraitControl0059BBD0(unsigned int party_slot)
{
    if (party_slot < 8 && g_status_685170.buffers.XChar[party_slot].fOccupied) {
        g_portrait_controls_0069b920[party_slot]->Invalidate(0);
    }
}

// FUNCTION: WIZ8 0x0059BC00
void RedrawPanel69B940(void)
{
    g_panel_69b940->Invalidate(0);
}

/* Keep each slot's level-advance control active exactly while its character
   may advance, no NPC dialogue is up and the slot row still allows it;
   activating one invalidates the panel, and the panel redraws afterward. */
// FUNCTION: WIZ8 0x0059BC10
void UpdatePortraitAdvanceButtons0059BC10(void)
{
    int slot;
    W8TextControl** control;

    for (slot = 0, control = g_portrait_controls_0069b920;
         control < &g_portrait_controls_0069b920[8]; ++slot, ++control) {
        if (IsCharacterReadyToAdvance(slot) && gXStatus.fNpcDialogueMode == 0 &&
            g_status_685170.buffers.XChar[slot].portrait_advance_103 != 0) {
            if (!(*control)->m_active) {
                (*control)->SetActive(true);
                g_panel_69b940->Invalidate(0);
            }
        } else if ((*control)->m_active) {
            (*control)->SetActive(false);
        }
    }
    g_panel_69b940->Redraw();
}

/* Open the character screen for giLevelUpChar when that portrait button fires. */
// FUNCTION: WIZ8 0x0059BCA0
void OnLevelButtonActivate(void)
{
    int slot = giLevelUpChar;

    if (slot == -1 || slot >= 8) {
        return;
    }
    if (g_status_685170.buffers.XChar[slot].fOccupied == 0) {
        srAssertFail("fCHAR_OCCUPIED(giLevelUpChar)",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\MGSPortraits.cpp", 0x988, 0);
    }
    g_pending_screen_state.parameter_3 = &g_status_685170.buffers.Char[slot];
    g_pending_screen_state.mode = 2;
    SetPendingScreenState(W8_SCREEN_CHARACTER);
}

/* The eight level-up portrait buttons sit in two columns on gpLevelButtonsPanel,
   one row per party pair. CreateConditionButtons mirrors this layout with a
   different region base and icon set. */
// FUNCTION: WIZ8 0x0059B940
void CreateLevelButtons(void)
{
    unsigned int uiSlot;
    unsigned int column_x;
    int row_y;
    int count;
    W8TextControl** control;

    g_panel_69b940 = 0;
    control = g_portrait_controls_0069b920;
    for (count = 8; count != 0; --count) {
        *control = 0;
        ++control;
    }

    g_panel_69b940 = new Controls(0, 0, 0x280, 0x1e0, -1, 0, -1);
    if (g_panel_69b940 == 0) {
        srAssertFail("gpLevelButtonsPanel",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\MGSPortraits.cpp", 0x8e0, 0);
    }

    uiSlot = 0;
    control = g_portrait_controls_0069b920;
    do {
        column_x = (uiSlot & 1) != 0 ? 0x23b : 0;
        row_y = (uiSlot >> 1) * 0x55;
        W8TextControl* button =
            new W8TextControl(g_panel_69b940, uiSlot + 0x12, column_x + 0x19, row_y + 0x46,
                              column_x + 0x2b, row_y + 0x58, 0xa7, 0, 0, 2, 1, 4, 3);
        *control = button;
        button->m_primaryActivationCallback = OnLevelButtonActivate;
        if (*control == 0) {
            srAssertFail("gpLevelButtons[uiSlot]",
                         "C:\\Projects\\Wizardry 8\\Local Screens\\MGSPortraits.cpp", 0x8f7, 0);
        }
        ++control;
        ++uiSlot;
    } while (control < g_portrait_controls_0069b920 + 8);

    giLevelUpChar = -1;
    g_panel_69b940->SetEnabled(true);
    control = g_portrait_controls_0069b920;
    do {
        (*control)->SetActive(false);
        ++control;
    } while (control < g_portrait_controls_0069b920 + 8);
}

// SYNTHETIC: WIZ8 0x005991A0
// W8ConditionButton::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005991C0
W8ConditionButton::~W8ConditionButton() {}

/* The base text control draws the frame; the derived pass overlays the slot's
   condition icon when the widget is active and either the redraw is full or
   the widget was already dirty. The icon is m_image_object_bc plus the
   per-condition offset the selector byte chooses. */
// FUNCTION: WIZ8 0x00599210
void W8ConditionButton::Redraw(int full_redraw)
{
    bool dirty = m_dirty;
    int left;
    int top;

    W8TextControl::Redraw(full_redraw);
    if (!m_active) {
        return;
    }
    if (m_pPanel == 0) {
        return;
    }
    if (full_redraw == 0 && !dirty) {
        return;
    }
    GetTextOrigin(&left, &top);
    left += 2;
    top += 2;
    if (m_condition_b8 == 0) {
        DrawCatalogImageAndInvalidate(-14, m_image_object_bc + 0xb6, 0, 0, left, top, 2, 0);
    } else if (m_condition_b8 == 1) {
        DrawCatalogImageAndInvalidate(-14, m_image_object_bc + 0xc9, 0, 0, left, top, 2, 0);
    }
}

/* Leaving the button drops the hovered-slot tracking; if a hover was showing,
   its highlight overlay comes down with it. */
// FUNCTION: WIZ8 0x005992E0
void W8ConditionButton::OnMouseLeave(int event)
{
    W8TextControl::OnMouseLeave(event);
    if (g_level_block->condition_highlight_party_slot != -1) {
        g_level_block->condition_highlight_party_slot = -1;
        DismissHighlightOverlay();
        RequestRedraw(0x8000);
        RequestRedraw(0xff);
    }
}

// FUNCTION: WIZ8 0x00599330
void W8ConditionButton::OnLeftButtonDown(int event)
{
    W8TextControl::OnLeftButtonDown(event);
    g_level_block->condition_highlight_party_slot = m_ui_slot_c0;
    RequestRedraw(0x8000);
}

// FUNCTION: WIZ8 0x00599360
void W8ConditionButton::OnLeftButtonUp(int event)
{
    W8TextControl::OnLeftButtonUp(event);
    g_level_block->condition_highlight_party_slot = -1;
    DismissHighlightOverlay();
    RequestRedraw(0x8000);
    RequestRedraw(0xff);
}

/* The eight buttons sit in two columns inside their own panel, one row per
   party pair. Each tracks one party slot through the region set and the
   recorded ui_slot. */
// FUNCTION: WIZ8 0x0059BDB0
void CreateConditionButtons(void)
{
    unsigned int uiSlot;
    unsigned int column_x;
    int row_y;
    int count;
    W8ConditionButton** control;

    g_condition_buttons_panel_0069b944 = 0;
    control = g_condition_buttons_0069b900;
    for (count = 8; count != 0; --count) {
        *control = 0;
        ++control;
    }

    g_condition_buttons_panel_0069b944 = new Controls(0, 0, 0x280, 0x1e0, -1, 0, -1);
    if (g_condition_buttons_panel_0069b944 == 0) {
        srAssertFail("gpConditionButtonsPanel",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\MGSPortraits.cpp", 0x9db, 0);
    }

    uiSlot = 0;
    control = g_condition_buttons_0069b900;
    do {
        column_x = (uiSlot & 1) != 0 ? 0x23f : 0;
        row_y = (uiSlot >> 1) * 0x55;
        W8ConditionButton* button = new W8ConditionButton(
            g_condition_buttons_panel_0069b944, uiSlot + 0x1a, column_x + 0x17, row_y + 0x13,
            column_x + 0x2a, row_y + 0x26, 0xa8, 0, 0, 0, 1, 1, -1, uiSlot);
        *control = button;
        if (button == 0) {
            srAssertFail("gpConditionButtons[uiSlot]",
                         "C:\\Projects\\Wizardry 8\\Local Screens\\MGSPortraits.cpp", 0x9ec, 0);
        }
        ++control;
        ++uiSlot;
    } while (control < g_condition_buttons_0069b900 + 8);

    RegionSetDisable(6);
    g_condition_buttons_panel_0069b944->SetEnabled(false);
    if (g_level_block->condition_highlight_party_slot != -1) {
        g_level_block->condition_highlight_party_slot = -1;
        DismissHighlightOverlay();
        RequestRedraw(0x8000);
        RequestRedraw(0xff);
    }
}

/* Forward left-button and hover events to the portrait level-up control for
   the region's callback_id slot; release records giLevelUpChar. */
// FUNCTION: WIZ8 0x0059BD20
unsigned char PortraitControlRegionEvent(const InputAtom* event, W8Region* region)
{
    W8TextControl* control = g_portrait_controls_0069b920[region->callback_id];
    if (control == 0) {
        return 0;
    }
    switch (event->usEvent) {
    case LEFT_BUTTON_DOWN:
        control->OnLeftButtonDown(0);
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        return 1;
    case LEFT_BUTTON_UP:
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
            giLevelUpChar = region->callback_id;
            control->OnLeftButtonUp(0);
            region->flags &= ~W8_REGION_LEFT_BUTTON_HELD;
        }
        return 1;
    case MOUSE_POS:
        if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
            giLevelUpChar = -1;
            control->OnMouseLeave(0);
            return 1;
        }
        if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
            control->OnMouseEnter(0);
            return 1;
        }
        break;
    }
    return 0;
}

/* Per-frame condition-button refresh: while a slot is empty, mid-refresh or
   suppressed its button stays hidden (and a tracked highlight is dropped);
   otherwise the icon tracks the slot's highest condition or top enchantment
   and the button stays live, then the panel redraws. */
// FUNCTION: WIZ8 0x0059C080
void UpdateConditionButtons0059C080(void)
{
    int image;
    int slot;

    for (slot = 0; slot < 8; ++slot) {
        W8ConditionButton* button = g_condition_buttons_0069b900[slot];
        W8MonsterManagerEntry* entry = &gXStatus.monster_manager_entries[slot];
        image = 0;
        if (g_status_685170.buffers.XChar[slot].fOccupied != 0 &&
            g_level_block->portrait_refresh_pending[slot] == 0 && entry->keyboard_menu_open == 0) {
            image = g_status_685170.buffers.Char[slot].highest_condition;
            if (image == 0) {
                image = g_status_685170.buffers.Char[slot].enchantment_top;
                if (image != 0) {
                    if (button->m_image_object_bc != image) {
                        button->m_image_object_bc = image;
                        button->Invalidate(0);
                    }
                    if (button->m_condition_b8 != 1) {
                        button->m_condition_b8 = 1;
                        button->Invalidate(0);
                    }
                }
            } else {
                if (button->m_image_object_bc != image) {
                    button->m_image_object_bc = image;
                    button->Invalidate(0);
                }
                if (button->m_condition_b8 != 0) {
                    button->m_condition_b8 = 0;
                    button->Invalidate(0);
                }
            }
        }
        if (image == 0) {
            if (button->m_active) {
                button->SetActive(false);
                button->Invalidate(0);
                if (g_level_block->portrait_refresh_pending[slot] == 0 &&
                    entry->keyboard_menu_open == 0) {
                    ClearSurfaceRect(button->m_left + g_condition_buttons_panel_0069b944->origin_x,
                                     button->m_top + g_condition_buttons_panel_0069b944->origin_y,
                                     button->m_right + g_condition_buttons_panel_0069b944->origin_x,
                                     button->m_bottom +
                                         g_condition_buttons_panel_0069b944->origin_y);
                }
                if (g_level_block->condition_highlight_party_slot == slot) {
                    g_level_block->condition_highlight_party_slot = -1;
                    DismissHighlightOverlay();
                    RequestRedraw(0x8000);
                    RequestRedraw(0xff);
                }
            }
        } else if (!button->m_active) {
            button->SetActive(true);
            button->Invalidate(0);
        }
    }
    g_condition_buttons_panel_0069b944->Redraw();
}

/* Forward left-button and hover events to the condition button for the
   region's callback_id slot. */
// FUNCTION: WIZ8 0x0059C260
unsigned char ConditionButtonRegionEvent(const InputAtom* event, W8Region* region)
{
    unsigned short us_event = event->usEvent;
    unsigned short slot = region->callback_id;
    if (us_event == LEFT_BUTTON_DOWN) {
        g_condition_buttons_0069b900[slot]->OnLeftButtonDown(0);
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
    } else {
        if (us_event != LEFT_BUTTON_UP) {
            if (us_event == MOUSE_POS) {
                if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
                    g_condition_buttons_0069b900[slot]->OnMouseLeave(0);
                    return 1;
                }
                if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
                    g_condition_buttons_0069b900[slot]->OnMouseEnter(0);
                    return 1;
                }
            }
            return 0;
        }
        g_condition_buttons_0069b900[slot]->OnLeftButtonUp(0);
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
            region->flags &= ~W8_REGION_LEFT_BUTTON_HELD;
            return 1;
        }
    }
    return 1;
}
