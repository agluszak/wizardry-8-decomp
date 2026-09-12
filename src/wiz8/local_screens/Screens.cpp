#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/game_status.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/screen_state.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_screens/PartySelectionScreen.h"
#include "wiz8/local_screens/MGSPortraits.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_screens/RCSCommon.h"
#include "wiz8/notices.h"
#include "wiz8/xstatus.h"
#include "wiz8/cursor.h"
#include "wiz8/item_video_object_vector.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/dialog_code/DialogTextArea.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/engine_code/stTextureAnim.h"
#include "wiz8/fonts.h"
#include "wiz8/regions.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/wiz8_windows.h"
#include "Container.h"
#include "timer.h"

#include <string.h>

/*
 * Local Screens\Screens.cpp.
 *
 * Keeps the held-item cursor in step with the screen. On the two screens that
 * allow an item in hand it installs that item's cursor; anywhere else it puts
 * the default cursor back and forgets what was held. The screen ids and the
 * cursor ids are the numbers the original uses, and nothing here names them.
 */

/* Constructor 0x0055DE40 builds Controls, constructs the dialog member at
   +0x64, installs vtable 0x005EE920, and is the only value stored into the camp
   controller's +0x1B0 field. The 0xBC-byte allocation proves the complete
   extent below. */
// VTABLE: WIZ8 0x005ee920
class W8Controls005EE920 : public Controls {
public:
    bool Function55EBB0(unsigned int command);
    bool Function55EBE0(unsigned int command);

private:
    int m_positional_4c;
    int m_positional_50;
    int m_positional_54;
    int m_positional_58;
    int m_positional_5c;
    int m_positional_60;
    W8DialogTextArea m_dialog_64;
};
static_assert(sizeof(W8Controls005EE920) == 0xbc, "W8Controls005EE920_size");

/* Run the first screen command predicate and reset this target through its
   second virtual slot when command zero succeeds. */
// FUNCTION: WIZ8 0x0055EBB0
bool W8Controls005EE920::Function55EBB0(unsigned int command)
{
    if (m_dialog_64.ScrollDown(static_cast<unsigned char>(command)) != 0) {
        if (static_cast<char>(command) == 0) {
            Invalidate(0);
        }
        return true;
    }
    return false;
}

/* The parallel path using the second command predicate. */
// FUNCTION: WIZ8 0x0055EBE0
bool W8Controls005EE920::Function55EBE0(unsigned int command)
{
    if (m_dialog_64.ScrollUp(static_cast<unsigned char>(command)) != 0) {
        if (static_cast<char>(command) == 0) {
            Invalidate(0);
        }
        return true;
    }
    return false;
}

/* Return the requested screen id, falling back to the state at the top of the
   return stack when there is no explicit pending state. */
// FUNCTION: WIZ8 0x0055EC10
int GetPendingScreenState(void)
{
    W8ScreenStateRuntime state;

    if (g_pending_screen_state.id != -1) {
        return g_pending_screen_state.id;
    }
    if (PeekStack(g_screen_return_stack, &state)) {
        return state.id;
    }
    return -1;
}

// FUNCTION: WIZ8 0x0055ec50
void SetPendingScreenState(int value)
{
    g_pending_screen_state.id = value;
}

// FUNCTION: WIZ8 0x0055ec60
void RequestScreenTransition(void)
{
    g_screen_return_requested = 1;
}

/* Whether a transition is pending either explicitly or through the frame's
   transition flag. */
// FUNCTION: WIZ8 0x0055EC70
unsigned char IsScreenTransitionPending(void)
{
    if (g_pending_screen_state.id == -1 && g_screen_return_requested == 0) {
        return 0;
    }
    return 1;
}

/* Route one redraw bit to the active camp or main-game screen state. The slot
   travels as an int: the body only ever reads its low byte for the shift. */
// FUNCTION: WIZ8 0x0055EE30
void RequestPartySlotRedraw(int bit)
{
    if (g_current_screen_state.id == W8_SCREEN_CAMP) {
        g_camp_screen_0069c0f4->redraw_flags |= 0x100;
    } else if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
        RequestRedraw(1 << (bit & 31));
    }
}

/* Refresh one party slot's on-screen presentation for the active screen:
   character and party-selection forward into their own helpers; camp marks
   the review panels dirty; main game redraws the portrait overlay. */
// FUNCTION: WIZ8 0x0055EC90
void RefreshPartySlotDisplay(unsigned int party_slot)
{
    unsigned int top;
    unsigned char overlay_ready;
    char highlighted;

    switch (g_current_screen_state.id) {
    case W8_SCREEN_CHARACTER:
        RefreshCharacterScreenPartySlot(party_slot);
        return;
    case W8_SCREEN_PLEASE_WAIT:
        break;
    case W8_SCREEN_PARTY_SELECTION:
        RefreshPartySelectionPortrait(party_slot);
        break;
    case W8_SCREEN_CAMP:
        if (g_rcs_mode_0064cbe8 == static_cast<int>(party_slot) &&
            g_camp_screen_0069c0f4->input_mode == 0) {
            if (g_camp_screen_0069c0f4->unknown_d40[0] != 0) {
                g_camp_screen_0069c0f4->redraw_flags |= 0x100;
                return;
            }
            g_camp_screen_0069c0f4->redraw_flags |= 0x100;
            RedrawRcsLevelUpPanel();
            RedrawRcsDismissPanel();
            return;
        }
        break;
    case W8_SCREEN_MAIN_GAME:
        if (g_level_block->value_0fc == 0 || g_level_block->party_bytes_109[party_slot] != 0) {
            switch (party_slot) {
            case 0:
            case 1:
                top = 0x12;
                break;
            case 2:
            case 3:
                top = 0x67;
                break;
            case 4:
            case 5:
                top = 0xbc;
                break;
            case 6:
            case 7:
                top = 0x111;
                break;
            default:
                top = party_slot;
                break;
            }
            overlay_ready =
                PreparePartyPortraitOverlay(party_slot, (party_slot & 1) << 9 | 0x14, top);
            highlighted = 0;
            if (party_slot == static_cast<unsigned int>(g_level_block->highlight_override) ||
                party_slot == static_cast<unsigned int>(g_level_block->values_170[7]) ||
                party_slot == static_cast<unsigned int>(g_level_block->held_item_display_190)) {
                highlighted = 1;
            }
            RedrawPartyPortraitOverlay(party_slot, highlighted, overlay_ready,
                                       g_level_block->party_bytes_109[party_slot] == 0);
            g_monster_manager_entries[party_slot].field_0d1 = 1;
            InvalidatePortraitControl0059BBD0(party_slot);
            return;
        }
        break;
    }
}

/* Text-input mode installs cursor id 8. Lives here because the address sits
   in the Screens translation unit. */
// FUNCTION: WIZ8 0x0055EF80
int GetTextInputCursor(void)
{
    return W8_CURSOR_TEXT_INPUT;
}

/* Install a named cursor, or restore the held-item / default cursor when the
   caller passes W8_CURSOR_NONE (-1). Unchanged ids are ignored; a new id resets
   the frame and applies through ApplyCurrentCursor. */
// FUNCTION: WIZ8 0x0055EE70
void SetTargetCursor(int cursor)
{
    int object;

    if (cursor == gXStatus.iCurrentCursor) {
        return;
    }
    if (cursor == -1) {
        if ((g_current_screen_state.id == W8_SCREEN_MAIN_GAME ||
             g_current_screen_state.id == W8_SCREEN_CAMP) &&
            g_status_685170.item_in_cursor) {
            if (g_status_685170.item_in_hand_235b.item_id != -1) {
                g_status_685170.item_in_cursor = 1;
                object = g_item_video_objects_68ec68.GetOrCreateVideoObject(
                    g_status_685170.item_in_hand_235b.item_id);
                SetMouseCursorFromVideoObject(GetCatalogVideoObjectHandle(object, 0),
                                              GetCatalogVideoObjectYOffset(object), 0, 0);
                BlitToMouseCursor(GetCatalogVideoObjectHandle(0, 0),
                                  GetCatalogVideoObjectYOffset(0), 0, 0);
                RefreshMouseCursorTexture();
                gXStatus.iCurrentCursor = 7;
            }
        } else if (gXStatus.iCurrentCursor != -1) {
            SetMouseCursorFromVideoObject(GetCatalogVideoObjectHandle(0, 0),
                                          GetCatalogVideoObjectYOffset(0), 0, 0);
            RefreshMouseCursorTexture();
            gXStatus.iCurrentCursor = -1;
            gXStatus.current_cursor_frame = 0;
            gXStatus.current_cursor_time = 0;
        }
    } else {
        gXStatus.iCurrentCursor = cursor;
        gXStatus.current_cursor_frame = 0;
        ApplyCurrentCursor();
    }
}

// FUNCTION: WIZ8 0x0055ef90
void UpdateHeldItemCursor(void)
{
    int object;

    if ((g_current_screen_state.id == W8_SCREEN_MAIN_GAME ||
         g_current_screen_state.id == W8_SCREEN_CAMP) &&
        g_status_685170.item_in_cursor) {
        if (g_status_685170.item_in_hand_235b.item_id != -1) {
            g_status_685170.item_in_cursor = 1;
            object = g_item_video_objects_68ec68.GetOrCreateVideoObject(
                g_status_685170.item_in_hand_235b.item_id);
            SetMouseCursorFromVideoObject(GetCatalogVideoObjectHandle(object, 0),
                                          GetCatalogVideoObjectYOffset(object), 0, 0);
            BlitToMouseCursor(GetCatalogVideoObjectHandle(0, 0), GetCatalogVideoObjectYOffset(0), 0,
                              0);
            RefreshMouseCursorTexture();
            gXStatus.iCurrentCursor = 7;
            return;
        }
    } else if (gXStatus.iCurrentCursor != -1) {
        SetMouseCursorFromVideoObject(GetCatalogVideoObjectHandle(0, 0),
                                      GetCatalogVideoObjectYOffset(0), 0, 0);
        RefreshMouseCursorTexture();
        gXStatus.iCurrentCursor = -1;
        gXStatus.current_cursor_frame = 0;
        gXStatus.current_cursor_time = 0;
    }
}

/* Drive the mouse cursor from gXStatus.iCurrentCursor against the main-game
   resource-slot table: resize, install the slot's texture anim, select the
   current frame, and refresh the hotspot. Multi-frame cursors also arm the
   animation countdown. */
// FUNCTION: WIZ8 0x0055F080
void ApplyCurrentCursor(void)
{
    if (gXStatus.iCurrentCursor == -1) {
        srAssertFail("gXStatus.iCurrentCursor != -1",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\Screens.cpp", 0x18d, 0);
    }
    if (g_main_game_resource_slots[gXStatus.iCurrentCursor].object != 0) {
        ResizeMouseCursorSurface(g_main_game_resource_slots[gXStatus.iCurrentCursor].size_x,
                                 g_main_game_resource_slots[gXStatus.iCurrentCursor].size_y);
        SetMouseCursorTexture(static_cast<stTextureAnim*>(
            g_main_game_resource_slots[gXStatus.iCurrentCursor].object));
        static_cast<stTextureAnim*>(g_main_game_resource_slots[gXStatus.iCurrentCursor].object)
            ->SetFrame00485400(gXStatus.current_cursor_frame);
        SetMouseCursorHotspot(g_main_game_resource_slots[gXStatus.iCurrentCursor].hotspot_x,
                              g_main_game_resource_slots[gXStatus.iCurrentCursor].hotspot_y);
    }
    if (g_main_game_resource_slots[gXStatus.iCurrentCursor].frame_count > 1) {
        gXStatus.current_cursor_time = SetCountdownClock(0xfa);
    }
}

/* Empty the item-in-hand record and restore the normal cursor. The held item is
   the 0x0c-byte record at g_status_685170.item_in_hand_235b (0x006874CB); the
   byte directly before it is item_in_cursor. */
// FUNCTION: WIZ8 0x0055f1e0
void ClearHeldItemDisplay(void)
{
    memset(&g_status_685170.item_in_hand_235b, 0, sizeof(g_status_685170.item_in_hand_235b));
    g_status_685170.item_in_cursor = 0;
    g_status_685170.item_in_hand_235b.item_id = -1;

    if (gXStatus.iCurrentCursor != -1) {
        SetMouseCursorFromVideoObject(GetCatalogVideoObjectHandle(0, 0),
                                      GetCatalogVideoObjectYOffset(0), 0, 0);
        RefreshMouseCursorTexture();
        gXStatus.iCurrentCursor = -1;
        gXStatus.current_cursor_frame = 0;
        gXStatus.current_cursor_time = 0;
    }
}

/* The item-cursor state: the string index table the camp screens read through
   GetTable647CCCEntry, and the held-item cursor bookkeeping. */

// GLOBAL: WIZ8 0x00647ccc
unsigned char g_table_647ccc[128];

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wchar-subscripts"
/* The recovered index is a char; the 128-entry table is the domain, and this
   helper is not a virtual whose mangling we can widen. */
// FUNCTION: WIZ8 0x0055F2B0
unsigned char GetTable647CCCEntry(char index)
{
    return g_table_647ccc[index];
}
#pragma clang diagnostic pop

/* Point the mouse cursor at an item's video object, blitting it down as well.
   A negative held item id means the cursor keeps whatever it has. */
// FUNCTION: WIZ8 0x0055F160
void SetItemCursor(int item_id)
{
    int object;
    unsigned short y_offset;
    unsigned int handle;

    if (g_status_685170.item_in_hand_235b.item_id != -1) {
        g_status_685170.item_in_cursor = 1;
        object = g_item_video_objects_68ec68.GetOrCreateVideoObject(
            g_status_685170.item_in_hand_235b.item_id);
        y_offset = GetCatalogVideoObjectYOffset(object);
        handle = GetCatalogVideoObjectHandle(object, 0);
        SetMouseCursorFromVideoObject(handle, y_offset, 0, 0);
        y_offset = GetCatalogVideoObjectYOffset(item_id);
        handle = GetCatalogVideoObjectHandle(item_id, 0);
        BlitToMouseCursor(handle, y_offset, 0, 0);
        RefreshMouseCursorTexture();
        gXStatus.iCurrentCursor = 7;
    }
}

/* Dispatch one already-built notice line to the camp or main-game dialog. */
// FUNCTION: WIZ8 0x0055F260
void ShowNoticeLine(const wchar_t* text, int a, int b, int c)
{
    if (g_current_screen_state.id == W8_SCREEN_CAMP) {
        ShowCampNoticeLine(text, a, b, c);
    } else if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
        ShowMainGameNoticeLine(text, a, b, c);
    }
}

/* Default the live main-game level block after ResetMainGameScreenState: clear
   selection and text-box state, arm the timers, size the text regions, and
   reset message storage. */
// FUNCTION: WIZ8 0x0055F2C0
void InitializeMainGameLevelBlock(void)
{
    int previous_mode;
    int slot;

    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
        g_level_block->redraw_flags = static_cast<unsigned int>(-1);
    }
    g_level_block->transition_pending = 0;
    g_level_block->camera_mode_100 = 7;
    g_level_block->flag_0f0 = IsMessageBoxActive();
    g_level_block->flag_108 = 0;
    g_level_block->flag_210 = 0;
    g_level_block->values_194[0] = -1;
    g_level_block->highlight_override = -1;
    g_level_block->values_170[0] = -1;
    g_level_block->values_170[1] = -1;
    g_level_block->values_170[2] = -1;
    g_level_block->values_170[3] = -1;
    g_level_block->values_170[5] = -1;
    g_level_block->values_170[4] = -1;
    g_level_block->values_170[6] = 0;
    g_level_block->values_170[7] = -1;
    g_level_block->held_item_display_190 = -1;
    g_level_block->value_23c = -1;
    g_level_block->value_240 = 0;
    g_level_block->values_194[1] = 0x35;
    g_level_block->values_200[0] = -1;
    g_level_block->values_200[1] = -1;
    g_level_block->values_200[2] = -1;
    g_level_block->values_200[3] = -1;
    g_level_block->values_194[3] = CurrentTextLineHasContent() ? 0x57 : -1;
    g_level_block->values_194[4] = CurrentDialogueLineHasContent() ? 0x5a : -1;
    g_level_block->world_update_flags = 0;
    g_level_block->world_render_flags = 0;
    g_level_block->highlighted_item = -1;
    g_level_block->selected_item = -1;
    g_level_block->clock_214 = GetClock();
    g_level_block->flag_218 = 0;
    for (slot = 0; slot < 8; ++slot) {
        g_level_block->party_bytes_109[slot] = 0;
        g_level_block->values_114[slot] = 0;
        g_level_block->values_134[slot] = 0;
    }
    g_level_block->combat_end_notification = -1;
    previous_mode = g_settings_6850c8.field_006;
    g_settings_6850c8.field_006 = -1;
    ApplyMainGameModeFlag(previous_mode, 1);
    g_level_block->character_update_timer = SetCountdownClock(0);
    g_level_block->world_update_timer = SetCountdownClock(0);
    g_level_block->countdown_258 = SetCountdownClock(60000);
    g_level_block->countdown_25c = SetCountdownClock(0);
    g_level_block->countdown_26c = SetCountdownClock(0xfa);
    g_level_block->flag_270 = 1;
    g_level_block->flag_271 = 1;
    g_level_block->dialogue_open = 0;
    g_level_block->flag_272 = 0;
    g_level_block->dialogue_owner = 0;
    g_level_block->value_278 = 0;
    g_level_block->tick_274 = GetTickCount();
    g_level_block->value_284 = 0;
    g_level_block->value_288 = 0;
    DisableRegionInput(0xe5);
    g_level_block->value_28c = -1;
    g_level_block->value_2ac = 0;
    g_level_block->value_2b4 = 0;
    g_level_block->value_2b0 = 0;
    g_level_block->text_lines[4 + g_status_685170.text_line_cursor_1795] = FindStoppedTextLine();
    g_level_block->refresh_combat_panel = 1;
    g_level_block->combat_panel_timer = SetCountdownClock(0);
    g_level_block->refresh_party_panel = 1;
    g_level_block->unknown_158[1] = 1;
    SetTextBoxRegionBounds(0xa8, 0x16e, 0x1c4, 0x1ba);
    for (int line = 0; line < 12; ++line) {
        g_level_block->text_lines[line] = 0;
    }
    for (int slot_index = 0; slot_index < 4; ++slot_index) {
        g_level_block->text_slots_1d8[slot_index] = -1;
        g_level_block->text_slots_1e8[slot_index] = -1;
    }
    g_level_block->unknown_2e4[0] = 0;
    g_level_block->value_2e8 = g_font_683660;
    g_level_block->palette_2ec = g_colour_68ee08;
    g_level_block->selection_kind = -1;
    g_level_block->value_2f4 = -1;
    g_level_block->selection_settled = 0;
    g_level_block->tooltip_since = 0;
    g_level_block->tooltip_pending = 0;
    g_level_block->tooltip_subject = -1;
    g_level_block->tooltip_kind = -1;
    g_level_block->countdown_30c = SetCountdownClock(0);
    g_level_block->combat_slot = -1;
    g_level_block->flag_314 = 0;
    g_level_block->hover_combat_slot = -1;
    g_level_block->flag_31c = 0;
    g_level_block->countdown_320 = SetCountdownClock(0);
    g_level_block->flag_324 = 0;
    g_level_block->flag_325 = 0;
    g_level_block->flag_326 = 0;
    g_level_block->flag_327 = 0;
    g_level_block->countdown_32c = SetCountdownClock(0);
    ResetMessageStorage();
}
