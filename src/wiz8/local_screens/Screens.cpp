#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/game_status.h"
#include "wiz8/screen_state.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"
#include "wiz8/local_screens/Screens.h"
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
        if (*reinterpret_cast<int*>(&g_level_block->unknown_0f8[4]) == 0 ||
            g_level_block->unknown_108[1 + party_slot] !=
                0) { /* reinterpret-ok: int gate at +0xfc in opaque 0xf8 range */
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
                party_slot == *reinterpret_cast<unsigned int*>(&g_level_block->unknown_170[0x1c]) ||
                party_slot == static_cast<unsigned int>(g_level_block->held_item_display_190)) {
                /* reinterpret-ok: int at +0x18c in opaque unknown_170 */
                highlighted = 1;
            }
            RedrawPartyPortraitOverlay(party_slot, highlighted, overlay_ready,
                                       g_level_block->unknown_108[1 + party_slot] == 0);
            reinterpret_cast<unsigned char*>(&g_portrait_animation_states[party_slot])[0x5c] =
                1; /* reinterpret-ok: dirty byte at +0x5c in portrait animation state */
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

// FUNCTION: WIZ8 0x0055F2B0
unsigned char GetTable647CCCEntry(char index)
{
    return g_table_647ccc[index];
}

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
    unsigned int offset;

    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_level_block != 0) {
        g_level_block->redraw_flags = static_cast<unsigned int>(-1);
    }
    g_level_block->transition_pending = 0;
    g_level_block->camera_mode_100 = 7;
    g_level_block->unknown_000[0xf0] = IsMessageBoxActive();
    g_level_block->unknown_108[0] = 0;
    g_level_block->unknown_200[0x10] = 0;
    *reinterpret_cast<int*>(g_level_block->unknown_194) = -1; /* reinterpret-ok: int at +0x194 */
    g_level_block->highlight_override = -1;
    *reinterpret_cast<int*>(&g_level_block->unknown_170[0x00]) =
        -1; /* reinterpret-ok: opaque ints */
    *reinterpret_cast<int*>(&g_level_block->unknown_170[0x04]) = -1;
    *reinterpret_cast<int*>(&g_level_block->unknown_170[0x08]) = -1;
    *reinterpret_cast<int*>(&g_level_block->unknown_170[0x0c]) = -1;
    *reinterpret_cast<int*>(&g_level_block->unknown_170[0x14]) = -1;
    *reinterpret_cast<int*>(&g_level_block->unknown_170[0x10]) = -1;
    *reinterpret_cast<int*>(&g_level_block->unknown_170[0x18]) = 0;
    *reinterpret_cast<int*>(&g_level_block->unknown_170[0x1c]) = -1;
    g_level_block->held_item_display_190 = -1;
    *reinterpret_cast<int*>(&g_level_block->unknown_200[0x3c]) = -1; /* reinterpret-ok: +0x23c */
    *reinterpret_cast<int*>(&g_level_block->unknown_200[0x40]) = 0;  /* +0x240 */
    *reinterpret_cast<int*>(&g_level_block->unknown_194[0x04]) = 0x35;
    *reinterpret_cast<int*>(&g_level_block->unknown_200[0x00]) = -1;
    *reinterpret_cast<int*>(&g_level_block->unknown_200[0x04]) = -1;
    *reinterpret_cast<int*>(&g_level_block->unknown_200[0x08]) = -1;
    *reinterpret_cast<int*>(&g_level_block->unknown_200[0x0c]) = -1;
    *reinterpret_cast<int*>(&g_level_block->unknown_194[0x0c]) =
        CurrentTextLineHasContent() ? 0x57 : -1;
    *reinterpret_cast<int*>(&g_level_block->unknown_194[0x10]) =
        CurrentDialogueLineHasContent() ? 0x5a : -1;
    g_level_block->world_update_flags = 0;
    g_level_block->world_render_flags = 0;
    g_level_block->highlighted_item = -1;
    g_level_block->selected_item = -1;
    *reinterpret_cast<unsigned int*>(&g_level_block->unknown_200[0x14]) = GetClock(); /* +0x214 */
    g_level_block->unknown_200[0x18] = 0;
    slot = 0;
    offset = 0x134;
    do {
        g_level_block->unknown_108[1 + slot] = 0;
        *reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(g_level_block) + offset - 0x20) =
            0; /* reinterpret-ok: parallel int arrays at +0x114/+0x134 */
        *reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(g_level_block) + offset) = 0;
        offset += 4;
        ++slot;
    } while (offset < 0x154);
    g_level_block->combat_end_notification = -1;
    previous_mode = g_flag_006850ce;
    g_flag_006850ce = -1;
    ApplyMainGameModeFlag(previous_mode, 1);
    g_level_block->character_update_timer = SetCountdownClock(0);
    g_level_block->world_update_timer = SetCountdownClock(0);
    *reinterpret_cast<unsigned int*>(&g_level_block->unknown_258[0]) =
        SetCountdownClock(60000); /* reinterpret-ok: timer dword at +0x258 */
    *reinterpret_cast<unsigned int*>(&g_level_block->unknown_258[4]) = SetCountdownClock(0);
    *reinterpret_cast<unsigned int*>(&g_level_block->unknown_26c[0]) = SetCountdownClock(0xfa);
    g_level_block->unknown_26c[4] = 1;
    g_level_block->unknown_26c[5] = 1;
    g_level_block->dialogue_open = 0;
    g_level_block->unknown_26c[6] = 0;
    g_level_block->dialogue_owner = 0;
    *reinterpret_cast<int*>(&g_level_block->unknown_26c[0x0c]) = 0;
    *reinterpret_cast<unsigned int*>(&g_level_block->unknown_26c[8]) = GetTickCount();
    *reinterpret_cast<int*>(&g_level_block->unknown_284[0]) = 0;
    *reinterpret_cast<int*>(&g_level_block->unknown_284[4]) = 0;
    DisableRegionInput(0xe5);
    *reinterpret_cast<int*>(&g_level_block->unknown_284[8]) = -1;
    *reinterpret_cast<int*>(&g_level_block->unknown_2ac[0]) = 0;
    *reinterpret_cast<int*>(&g_level_block->unknown_2ac[8]) = 0;
    *reinterpret_cast<int*>(&g_level_block->unknown_2ac[4]) = 0;
    g_level_block->text_lines[4 + g_text_line_cursor_00686905] = FindStoppedTextLine();
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
    *reinterpret_cast<unsigned short**>(g_level_block->unknown_2ec) =
        g_colour_68ee08; /* reinterpret-ok: palette pointer at +0x2ec */
    g_level_block->selection_kind = -1;
    *reinterpret_cast<int*>(g_level_block->unknown_2f4) = -1;
    g_level_block->selection_settled = 0;
    g_level_block->tooltip_since = 0;
    g_level_block->tooltip_pending = 0;
    g_level_block->tooltip_subject = -1;
    g_level_block->tooltip_kind = -1;
    *reinterpret_cast<unsigned int*>(g_level_block->unknown_30c) = SetCountdownClock(0);
    g_level_block->combat_slot = -1;
    g_level_block->flag_314 = 0;
    g_level_block->hover_combat_slot = -1;
    g_level_block->unknown_31c[0] = 0;
    *reinterpret_cast<unsigned int*>(&g_level_block->unknown_31c[4]) = SetCountdownClock(0);
    g_level_block->unknown_31c[8] = 0;
    g_level_block->unknown_31c[9] = 0;
    g_level_block->unknown_31c[10] = 0;
    g_level_block->flag_327 = 0;
    *reinterpret_cast<unsigned int*>(&g_level_block->unknown_329[3]) = SetCountdownClock(0);
    ResetMessageStorage();
}
