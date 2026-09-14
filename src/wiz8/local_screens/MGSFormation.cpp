#include "wiz8/local_screens/MGSFormation.h"

#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/MGSRadarMap.h"
#include "wiz8/local_screens/RCSCommon.h"
#include "wiz8/local_code/ButtonSound.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/local_code/FormationAndFacing.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/cursor.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/notices.h"
#include "wiz8/regions.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/xstatus.h"

#include "input.h"
#include "timer.h"
#include "wiz8/local_screens/MGSTextBox.h"

#include <string.h>

/*
 * Local Screens\MGSFormation.cpp.
 *
 * The party-formation screen. The board is three sprites anchored at
 * (0x207, 0x167): the board art (catalogue object 0x99, or 0x9a for the
 * highlighted variant) with the occupied cells' facing arrow (0x9c) and
 * marching-order chip (0x9d) baked into its image, a compass needle (0x9b)
 * rotated to party_facing - party_heading, and a lazily created overlay.
 * Below the board sit fifteen cell controls in five rows of three over a
 * Controls panel; edits stage into gXStatus.edited_formation and commit
 * through Function5545F0, or are only previewed while in combat.
 */

/* 0x0064DAF4: on-board (left, top) of each of the fifteen cell markers,
   relative to the board anchor. */
// GLOBAL: WIZ8 0x0064daf4
int g_formation_marker_offsets_0064daf4[15][2] = {
    {0x26, 0x9},  {0x19, 0xd},  {0x33, 0xd},  {0x43, 0x26}, {0x3f, 0x19},
    {0x3f, 0x33}, {0x26, 0x43}, {0x33, 0x3f}, {0x19, 0x3f}, {0x9, 0x26},
    {0xd, 0x33},  {0xd, 0x19},  {0x26, 0x1e}, {0x1e, 0x2a}, {0x2e, 0x2a},
};

/* 0x0064DB6C: screen (left, top) of the fifteen cell controls. */
// GLOBAL: WIZ8 0x0064db6c
int g_formation_cell_positions_0064db6c[15][2] = {
    {0x5a, 0x1c}, {0x3d, 0x25}, {0x77, 0x25}, {0x98, 0x5a}, {0x8f, 0x3d},
    {0x8f, 0x77}, {0x5a, 0x98}, {0x77, 0x8f}, {0x3d, 0x8f}, {0x1c, 0x5a},
    {0x25, 0x77}, {0x25, 0x3d}, {0x5a, 0x48}, {0x48, 0x61}, {0x6c, 0x61},
};

// GLOBAL: WIZ8 0x0069c2f0
int g_formation_active_cell_0069c2f0;
// GLOBAL: WIZ8 0x0069c2f4
int g_formation_drag_slot_0069c2f4;
/* 0x0069C304: the party slot occupying each cell control, or -1. */
// GLOBAL: WIZ8 0x0069c304
int g_formation_cell_slots_0069c304[15];
// GLOBAL: WIZ8 0x0069c340
unsigned int g_formation_drag_clock_0069c340;
// GLOBAL: WIZ8 0x0069c380
int g_formation_drag_cell_0069c380;

static void UpdateFormationCells(void);
static void ResetFormationCellControls(int cell);

/* Draw each occupied slot's facing arrow and marching-order chip into the
   board image; the slot the board highlights gets the +1 frame of each. */
// FUNCTION: WIZ8 0x005b1f40
static void DrawFormationSlotMarkers(int target)
{
    int slot;

    for (slot = 0; slot < 8; ++slot) {
        W8PartySlotRow* row = &g_status_685170.buffers.party_rows[slot];
        W8PartyFormationPosition* position = &g_status_685170.formation.positions[slot];
        int left;
        int top;
        int image;
        int order_image;

        if (row->occupied == 0 || position->row == -1) {
            continue;
        }
        left = g_formation_marker_offsets_0064daf4[position->row * 3 + position->column][0];
        top = g_formation_marker_offsets_0064daf4[position->row * 3 + position->column][1];
        image = position->facing * 3;
        order_image = row->party_order_index * 3;
        if (slot == g_level_block->values_170[7]) {
            image += 1;
            order_image += 1;
        }
        DrawCatalogImage(target, 0x9c, 0, image, left, top, 2, 0);
        DrawCatalogImage(target, 0x9d, 0, order_image, left, top, 2, 0);
    }
}

/* Rebuild the board, compass and overlay sprites and redraw the slot markers
   into the board image. */
// FUNCTION: WIZ8 0x005b1c80
void RefreshFormationBoard(void)
{
    unsigned int board_image;
    unsigned int compass_image;
    unsigned int video_object;

    if (g_status_685170.selected_character != -1) {
        Function5554A0(g_status_685170.selected_character);
    }
    if (g_level_block->formation_board_visible == 0) {
        return;
    }
    if (g_level_block->formation_board_sprite != 0) {
        ReleaseObject004257F0(g_level_block->formation_board_sprite);
        g_level_block->formation_board_sprite = 0;
    }
    if (g_level_block->formation_compass_sprite != 0) {
        ReleaseObject004257F0(g_level_block->formation_compass_sprite);
        g_level_block->formation_compass_sprite = 0;
    }
    if (g_level_block->formation_overlay_sprite != 0) {
        ReleaseObject004257F0(g_level_block->formation_overlay_sprite);
        g_level_block->formation_overlay_sprite = 0;
    }
    if (g_level_block->formation_board_alternate == 0) {
        video_object = GetCatalogVideoObjectHandle(0x99, 0);
    } else {
        video_object = GetCatalogVideoObjectHandle(0x9a, 0);
    }
    if (video_object == 0) {
        return;
    }
    Function4048D0(video_object, 0, &board_image);
    video_object = GetCatalogVideoObjectHandle(0x9b, 0);
    if (video_object == 0) {
        return;
    }
    Function4048D0(video_object, 0, &compass_image);
    DrawFormationSlotMarkers(board_image);
    g_level_block->formation_compass_sprite = Function4255C0(compass_image, 0, 1, 0, 1);
    PositionToolTipNode(g_level_block->formation_compass_sprite, 0x207, 0x167, 0);
    g_level_block->formation_compass_sprite->render_state_164.display_state = 4;
    g_level_block->formation_board_sprite = Function4255C0(board_image, 0, 1, 0, 1);
    PositionToolTipNode(g_level_block->formation_board_sprite, 0x207, 0x167, 0);
    g_level_block->formation_board_sprite->render_state_164.display_state = 4;
    if (g_level_block->formation_compass_sprite != 0) {
        Function425840(g_level_block->formation_compass_sprite,
                       g_status_685170.party_facing - g_status_685170.party_heading + 0x168);
    }
    SetRendererModePair();
}

/* Rotate the compass needle to party_facing - party_heading. */
// FUNCTION: WIZ8 0x005b1e70
void UpdateFormationCompass(void)
{
    if (g_level_block->formation_compass_sprite != 0) {
        Function425840(g_level_block->formation_compass_sprite,
                       g_status_685170.party_facing - g_status_685170.party_heading + 0x168);
    }
    SetRendererModePair();
}

/* Lazily create the overlay sprite above the board. */
// FUNCTION: WIZ8 0x005b1ea0
void CreateFormationBoardOverlay(void)
{
    W8ControlsRect bounds;

    if (g_level_block->formation_overlay_sprite == 0 &&
        g_level_block->formation_board_sprite != 0) {
        bounds.left = 0x200;
        bounds.top = 0x166;
        bounds.right = 0x269;
        bounds.bottom = 0x1c2;
        g_level_block->formation_overlay_sprite = Function4255C0(-14, &bounds, 0, 0, 1);
        Function4257D0(g_level_block->formation_overlay_sprite, 0x200, 0x166);
        Function4264F0(g_level_block->formation_overlay_sprite, 4);
    }
}

/* Board callback: opens the formation panel on release, hit-tests the cell
   markers for tooltips, and switches board art on hover transitions. */
// FUNCTION: WIZ8 0x005b2020
unsigned char FormationBoardRegionEvent(const InputAtom* event, W8Region* region)
{
    switch (event->usEvent) {
    case LEFT_BUTTON_DOWN:
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        return 1;
    case LEFT_BUTTON_UP:
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0 &&
            gXStatus.fReviewCharacterMode == 0) {
            OpenFormationPanel();
        }
        return 1;
    case RIGHT_BUTTON_DOWN:
        region->flags |= W8_REGION_RIGHT_BUTTON_HELD;
        return 1;
    case RIGHT_BUTTON_UP:
        return 1;
    case MOUSE_POS:
        break;
    default:
        return 0;
    }

    int slot;
    int hit;

    hit = -1;
    for (slot = 0; slot < 8; ++slot) {
        W8PartySlotRow* row = &g_status_685170.buffers.party_rows[slot];
        W8PartyFormationPosition* position = &g_status_685170.formation.positions[slot];
        int cell;

        if (row->occupied == 0 || position->row == -1) {
            continue;
        }
        cell = position->row * 3 + position->column;
        if (IsCursorInRectangle(g_formation_marker_offsets_0064daf4[cell][0] + 0x207,
                                g_formation_marker_offsets_0064daf4[cell][1] + 0x167,
                                g_formation_marker_offsets_0064daf4[cell][0] + 0x211,
                                g_formation_marker_offsets_0064daf4[cell][1] + 0x171)) {
            hit = slot;
            break;
        }
    }
    if (hit != g_level_block->values_170[7]) {
        SetTooltipSubject(6, hit);
    }
    if ((region->flags & W8_REGION_MOUSE_ENTER) == 0) {
        if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
            g_level_block->formation_board_alternate = 0;
            RefreshFormationBoard();
        }
    } else {
        g_level_block->formation_board_alternate = 1;
        RefreshFormationBoard();
    }
    return 0;
}

static void SelectFormationCell(void);
static void AcceptFormationChanges(void);
static void ResetFormationPanel(void);

/* Build the panel and its fifteen pairs of cell controls plus the three
   buttons (accept, close, reset). On any allocation failure the partial panel
   is released. */
// FUNCTION: WIZ8 0x005b2260
static unsigned char CreateFormationPanel(void)
{
    W8TextControl** button;
    int index;

    g_formation_panel = new Controls(0xd6, 0x3c, 0x1ab, 0x12f, 0x9e, 0, 0);
    if (g_formation_panel == 0) {
        return 0;
    }
    for (index = 0; index < 15; ++index) {
        int left = g_formation_cell_positions_0064db6c[index][0];
        int top = g_formation_cell_positions_0064db6c[index][1];
        W8TextControl* cell;

        cell = new W8TextControl(g_formation_panel, index + 0xa1, left, top, left + 0x20,
                                 top + 0x20, 0x9f, 0, -1, -1, -1, -1, -1);
        g_formation_cell_controls[index] = cell;
        if (cell == 0) {
            DestroyFormationPanel();
            return 0;
        }
        cell->AddLayoutFlags(g_W8TextControlMask005ED578);
        cell->m_primaryActivationCallback = SelectFormationCell;
        cell = new W8TextControl(g_formation_panel, -1, left, top, left + 0x20, top + 0x20, 0xa0, 0,
                                 -1, -1, -1, -1, -1);
        g_formation_cell_overlays[index] = cell;
        if (cell == 0) {
            DestroyFormationPanel();
            return 0;
        }
    }
    g_formation_action_buttons[0] =
        new W8TextControl(g_formation_panel, 0xb0, 0x85, 0xcb, 0xa0, 0xe6, 0x8e, 0, 0, -1, 1, 2, 3);
    g_formation_action_buttons[1] =
        new W8TextControl(g_formation_panel, 0xb1, 0xa7, 0xcb, 0xc2, 0xe6, 0x8e, 0, 4, -1, 5, 6, 7);
    g_formation_action_buttons[2] =
        new W8TextControl(g_formation_panel, 0xb2, 0xf, 0xcd, 0x29, 0xe7, 0x8f, 0, 0, -1, 1, 2, 3);
    button = g_formation_action_buttons;
    while (*button != 0) {
        ++button;
        if (button > &g_formation_action_buttons[2]) {
            g_formation_action_buttons[0]->m_primaryActivationCallback = AcceptFormationChanges;
            g_formation_action_buttons[1]->m_primaryActivationCallback = CloseFormationPanel;
            g_formation_action_buttons[2]->m_primaryActivationCallback = ResetFormationPanel;
            g_formation_panel->SetEnabled(true);
            return 1;
        }
    }
    DestroyFormationPanel();
    return 0;
}

/* Open the formation panel: build the controls, snapshot the live formation
   into gXStatus.edited_formation and pause the world. */
// FUNCTION: WIZ8 0x005b2150
void OpenFormationPanel(void)
{
    int index;

    if (g_formation_panel != 0) {
        g_formation_panel = 0;
    }
    for (index = 0; index < 15; ++index) {
        if (g_formation_cell_controls[index] != 0) {
            g_formation_cell_controls[index] = 0;
        }
        if (g_formation_cell_overlays[index] != 0) {
            g_formation_cell_overlays[index] = 0;
        }
    }
    for (index = 0; index < 3; ++index) {
        if (g_formation_action_buttons[index] != 0) {
            g_formation_action_buttons[index] = 0;
        }
    }
    if (CreateFormationPanel() != 0) {
        g_formation_active_cell_0069c2f0 = -1;
        g_formation_drag_cell_0069c380 = -1;
        g_formation_drag_slot_0069c2f4 = -1;
        g_formation_drag_clock_0069c340 = SetCountdownClock(0);
        gXStatus.fReviewCharacterMode = 1;
        RegionSetEnable(0x1b);
        RequestRedraw(0x1000);
        Function5545D0(&gXStatus.edited_formation, &g_status_685170.formation);
        PauseMainGameWorld();
        UpdateFormationCells();
    }
}

/* Re-derive every cell's sprites and enabled state from the edited formation
   and re-highlight the selected character's cell. */
// FUNCTION: WIZ8 0x005b2600
static void UpdateFormationCells(void)
{
    int cell;
    int slot;

    for (cell = 0; cell < 15; ++cell) {
        g_formation_cell_slots_0069c304[cell] = -1;
        ResetFormationCellControls(cell);
    }
    for (slot = 0; slot < 8; ++slot) {
        W8PartySlotRow* row = &g_status_685170.buffers.party_rows[slot];
        W8PartyFormationPosition* position = &gXStatus.edited_formation.positions[slot];
        W8TextControl* primary;
        W8TextControl* overlay;
        int sprite;

        if (row->occupied != 0 && position->row != -1) {
            cell = position->row * 3 + position->column;
            primary = g_formation_cell_controls[cell];
            overlay = g_formation_cell_overlays[cell];
            if (CanHoldFormationPlace(slot) == 0) {
                sprite = position->facing * 3 + 2;
                primary->m_normalSprite = sprite;
                primary->m_alternateNormalSprite = sprite;
                primary->m_pressedSprite = sprite;
                primary->m_alternatePressedSprite = sprite;
                primary->m_disabledSprite = sprite;
                sprite = row->party_order_index * 3 + 2;
                overlay->m_normalSprite = sprite;
                overlay->m_alternateNormalSprite = sprite;
                overlay->m_pressedSprite = sprite;
                overlay->m_alternatePressedSprite = sprite;
            } else {
                sprite = position->facing * 3;
                primary->m_normalSprite = sprite;
                primary->m_alternateNormalSprite = sprite;
                primary->m_pressedSprite = sprite + 1;
                primary->m_alternatePressedSprite = sprite + 1;
                primary->m_disabledSprite = sprite + 2;
                sprite = row->party_order_index * 3;
                overlay->m_normalSprite = sprite;
                overlay->m_alternateNormalSprite = sprite + 1;
                overlay->m_pressedSprite = sprite;
                overlay->m_alternatePressedSprite = sprite + 1;
                sprite = sprite + 2;
            }
            overlay->m_disabledSprite = sprite;
            primary->SetActive(true);
            overlay->SetActive(true);
            primary->SetEnabled(true);
            overlay->SetEnabled(true);
            g_formation_cell_slots_0069c304[cell] = slot;
            if (slot == g_status_685170.selected_character &&
                (primary->m_stateFlags & g_W8TextControlMask005ED570) == 0) {
                primary->EnableSecondaryState(0);
            }
        }
    }
    g_formation_panel->Invalidate(0);
}

/* The cells' activation callback: highlight the active cell and make its
   party slot the selected character. */
// FUNCTION: WIZ8 0x005b27e0
static void SelectFormationCell(void)
{
    int index;

    if (CanHoldFormationPlace(g_formation_cell_slots_0069c304[g_formation_active_cell_0069c2f0]) !=
        0) {
        for (index = 0; index < 15; ++index) {
            W8TextControl* control = g_formation_cell_controls[index];

            if (index == g_formation_active_cell_0069c2f0) {
                control->EnableSecondaryState(0);
            } else if ((control->m_stateFlags & g_W8TextControlMask005ED570) != 0) {
                control->DisableSecondaryState(0);
                g_formation_cell_overlays[index]->Invalidate(0);
            }
        }
        Function565740(g_formation_cell_slots_0069c304[g_formation_active_cell_0069c2f0]);
    }
}

/* The accept button: wake any slot whose row changed, reconcile the edited
   formation into the live one (or just warn while in combat) and close the
   panel. */
// FUNCTION: WIZ8 0x005b2860
static void AcceptFormationChanges(void)
{
    unsigned int slot;

    for (slot = 0; slot < 8; ++slot) {
        if (g_status_685170.buffers.party_rows[slot].occupied != 0 &&
            CanHoldFormationPlace(slot) != 0 &&
            gXStatus.edited_formation.positions[slot].row !=
                g_status_685170.formation.positions[slot].row) {
            StartBreathCycle(slot, 0);
        }
    }
    if (gXStatus.fCombatMode == 0) {
        Function5545F0(&gXStatus.edited_formation, &g_status_685170.formation);
    } else if (memcmp(&gXStatus.edited_formation, &g_status_685170.formation,
                      sizeof(W8PartyFormationState)) != 0) {
        ShowNotice(8, gppStringList[0x1f64 / 4], -1, -1, 0);
    }
    RefreshFormationBoard();
    RefreshRadarMap();
    DestroyFormationPanel();
    gXStatus.fReviewCharacterMode = 0;
    UpdateHeldItemCursor();
    RegionSetDisable(0x1b);
    RequestRedraw(0x200);
    ClearSurfaceRect(0xd6, 0x3c, 0x1ab, 0x12f);
    InvalidateRegion(0xd6, 0x3c, 0x1ab, 0x12f, 0);
    ResumeMainGameWorld();
}

/* The reset button: throw the staged edits away and rebuild the cells. */
// FUNCTION: WIZ8 0x005b2960
static void ResetFormationPanel(void)
{
    Function5545D0(&gXStatus.edited_formation, &g_status_685170.formation);
    UpdateFormationCells();
}

static void BeginFormationDrag(const InputAtom* event);
static void DropFormationSlot(int cell);

/* Region callback the fifteen formation cells share: left press arms the
   drag clock, a repeat while held begins the drag, left release either
   activates the cell or completes the drop, and mouse transitions drive the
   hover art and tooltip. */
// FUNCTION: WIZ8 0x005b29d0
unsigned char FormationCellRegionEvent(const InputAtom* event, W8Region* region)
{
    if (gXStatus.fReviewCharacterMode == 0) {
        return 0;
    }
    if (g_formation_cell_slots_0069c304[region->callback_id] == -1) {
        PushButtonSoundScheme005587C0(0, 1);
        if (event->usEvent != LEFT_BUTTON_UP) {
            return 1;
        }
        if (g_formation_drag_cell_0069c380 == -1) {
            return 1;
        }
    }
    if (event->usEvent < 0x41) {
        if (event->usEvent == LEFT_BUTTON_REPEAT) {
            if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0 &&
                ClockIsTicking(g_formation_drag_clock_0069c340) == 0 &&
                g_formation_drag_cell_0069c380 == -1 && g_formation_active_cell_0069c2f0 != -1) {
                if (CanHoldFormationPlace(g_formation_cell_slots_0069c304[region->callback_id]) ==
                    0) {
                    region->flags &= ~W8_REGION_LEFT_BUTTON_HELD;
                    return 1;
                }
                BeginFormationDrag(event);
                g_formation_cell_overlays[region->callback_id]->Invalidate(0);
                return 1;
            }
        } else {
            if (event->usEvent == LEFT_BUTTON_DOWN) {
                g_formation_cell_controls[region->callback_id]->OnLeftButtonDown(0);
                g_formation_cell_overlays[region->callback_id]->Invalidate(0);
                region->flags |= W8_REGION_LEFT_BUTTON_HELD;
                g_formation_drag_clock_0069c340 = SetCountdownClock(0xfa);
                return 1;
            }
            if (event->usEvent != LEFT_BUTTON_UP) {
                return 0;
            }
            if (g_formation_cell_slots_0069c304[region->callback_id] != -1) {
                g_formation_active_cell_0069c2f0 = region->callback_id;
                g_formation_cell_controls[region->callback_id]->OnLeftButtonUp(0);
                g_formation_cell_overlays[region->callback_id]->Invalidate(0);
            }
            if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
                region->flags &= ~W8_REGION_LEFT_BUTTON_HELD;
            }
            if (g_formation_drag_cell_0069c380 != -1) {
                DropFormationSlot(region->callback_id);
                return 1;
            }
        }
    } else {
        if (event->usEvent != MOUSE_POS) {
            return 0;
        }
        if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
            g_formation_cell_controls[region->callback_id]->OnMouseLeave(0);
            g_formation_cell_overlays[region->callback_id]->OnMouseLeave(0);
            g_formation_cell_overlays[region->callback_id]->Invalidate(0);
            if (gfLeftButtonState != 0 && g_formation_drag_cell_0069c380 == -1 &&
                CanHoldFormationPlace(g_formation_cell_slots_0069c304[region->callback_id]) != 0) {
                g_formation_active_cell_0069c2f0 = region->callback_id;
                BeginFormationDrag(event);
                g_formation_cell_overlays[region->callback_id]->Invalidate(0);
            }
            g_formation_active_cell_0069c2f0 = -1;
            SetTooltipSubject(7, -1);
            return 1;
        }
        if ((region->flags & W8_REGION_MOUSE_ENTER) == 0 &&
            g_formation_active_cell_0069c2f0 != -1) {
            return 0;
        }
        g_formation_cell_controls[region->callback_id]->OnMouseEnter(0);
        g_formation_cell_overlays[region->callback_id]->OnMouseEnter(0);
        g_formation_cell_overlays[region->callback_id]->Invalidate(0);
        g_formation_active_cell_0069c2f0 = region->callback_id;
        SetTooltipSubject(7, g_formation_cell_slots_0069c304[region->callback_id]);
    }
    return 1;
}

/* Pick up the active cell's character: clear the cell, switch the mouse
   cursor to the dragged chip and blank both controls' sprites. */
// FUNCTION: WIZ8 0x005b2e10
static void BeginFormationDrag(const InputAtom*)
{
    POINT point;
    unsigned short region;
    unsigned int video_object;
    int index;
    int sprite;

    if (CanHoldFormationPlace(g_formation_cell_slots_0069c304[g_formation_active_cell_0069c2f0]) !=
        0) {
        for (index = 0; index < 15; ++index) {
            W8TextControl* control = g_formation_cell_controls[index];

            if (index == g_formation_active_cell_0069c2f0) {
                control->EnableSecondaryState(0);
            } else if ((control->m_stateFlags & g_W8TextControlMask005ED570) != 0) {
                control->DisableSecondaryState(0);
                g_formation_cell_overlays[index]->Invalidate(0);
            }
        }
        Function565740(g_formation_cell_slots_0069c304[g_formation_active_cell_0069c2f0]);
    }
    g_formation_cell_controls[g_formation_active_cell_0069c2f0]->DisableSecondaryState(0);
    g_formation_drag_slot_0069c2f4 =
        g_formation_cell_slots_0069c304[g_formation_active_cell_0069c2f0];
    g_formation_drag_cell_0069c380 = g_formation_active_cell_0069c2f0;
    g_formation_cell_slots_0069c304[g_formation_active_cell_0069c2f0] = -1;
    SGPMouseGetPos(&point);
    Function427E70();
    sprite = g_formation_cell_controls[g_formation_drag_cell_0069c380]->m_alternatePressedSprite;
    region = GetCatalogVideoObjectYOffset(0x9f) + (short)sprite;
    video_object = GetCatalogVideoObjectHandle(0x9f, 0);
    SetMouseCursorFromVideoObject(video_object, region, 0x10, 0x10);
    DrawCatalogImage(
        -0xd, 0xa0, 0,
        static_cast<short>(
            g_formation_cell_overlays[g_formation_drag_cell_0069c380]->m_alternateNormalSprite),
        0, 0, 2, 0);
    Function4280C0(point.x - 0x10, point.y - 0x10);
    RefreshMouseCursorTexture();
    gXStatus.iCurrentCursor = 7;
    ResetFormationCellControls(g_formation_drag_cell_0069c380);
}

/* Drop the dragged character: into an empty cell it re-seats the row, onto a
   movable occupant it swaps positions, onto a dead one it cancels. */
// FUNCTION: WIZ8 0x005b2f70
static void DropFormationSlot(int cell)
{
    if (cell != -1 && cell != g_formation_drag_cell_0069c380) {
        if (g_formation_cell_slots_0069c304[cell] == -1) {
            SetFormationPosition(&gXStatus.edited_formation, g_formation_drag_slot_0069c2f4, -1, -1,
                                 0, 1, 1);
            Function555080(&gXStatus.edited_formation, g_formation_drag_slot_0069c2f4, cell / 3);
        } else if (CanHoldFormationPlace(g_formation_cell_slots_0069c304[cell]) == 0) {
            cell = -1;
        } else {
            Function555160(&gXStatus.edited_formation, g_formation_drag_slot_0069c2f4,
                           g_formation_cell_slots_0069c304[cell]);
        }
    }
    UpdateHeldItemCursor();
    UpdateFormationCells();
    g_formation_cell_overlays[g_formation_drag_cell_0069c380]->SetAlternateTextEnabled(0);
    if (cell != -1) {
        g_formation_cell_overlays[cell]->SetAlternateTextEnabled(1);
        g_formation_active_cell_0069c2f0 = cell;
        g_level_block->values_170[7] = g_formation_cell_slots_0069c304[cell];
        RequestRedraw(1 << (g_level_block->values_170[7] & 0x1f));
    }
    g_formation_drag_cell_0069c380 = -1;
}

/* Blank every sprite on a cell's control pair. */
// FUNCTION: WIZ8 0x005b3080
static void ResetFormationCellControls(int cell)
{
    g_formation_cell_controls[cell]->m_normalSprite = -1;
    g_formation_cell_controls[cell]->m_alternateNormalSprite = -1;
    g_formation_cell_controls[cell]->m_pressedSprite = -1;
    g_formation_cell_controls[cell]->m_alternatePressedSprite = -1;
    g_formation_cell_controls[cell]->m_disabledSprite = -1;
    g_formation_cell_controls[cell]->Invalidate(1);
    g_formation_cell_overlays[cell]->m_normalSprite = -1;
    g_formation_cell_overlays[cell]->m_alternateNormalSprite = -1;
    g_formation_cell_overlays[cell]->m_pressedSprite = -1;
    g_formation_cell_overlays[cell]->m_alternatePressedSprite = -1;
    g_formation_cell_overlays[cell]->m_disabledSprite = -1;
    g_formation_cell_overlays[cell]->Invalidate(1);
}

/* Highlight the cell holding one party slot and un-highlight the rest. */
// FUNCTION: WIZ8 0x005b3100
void SelectFormationSlotCell(int party_slot)
{
    int index;

    for (index = 0; index < 15; ++index) {
        W8TextControl* control = g_formation_cell_controls[index];

        if (g_formation_cell_slots_0069c304[index] == party_slot) {
            control->EnableSecondaryState(0);
            g_formation_cell_overlays[index]->Invalidate(0);
        } else if ((control->m_stateFlags & g_W8TextControlMask005ED570) != 0) {
            control->DisableSecondaryState(0);
            g_formation_cell_overlays[index]->Invalidate(0);
        }
    }
}
