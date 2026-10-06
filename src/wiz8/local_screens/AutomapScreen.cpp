#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/engine_code/BitArray.h"
#include "wiz8/engine_code/stHash.hpp"
#include "wiz8/local_screens/AutomapScreen.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/local_screens/mipe.h"
#include "wiz8/vector.h"
#include "surrender/srTypeRegistry.h"
#include "surrender/srClipPlane.h"
#include "surrender/srModelInstance.h"
#include "surrender/srCamera.h"
#include "surrender/srScene.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/3d.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/engine_code/UpdateMesh.h"
#include "wiz8/engine_code/Level.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/stMeshModel.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/engine_code/stTextureFile.h"
#include "wiz8/engine_code/stLight.hpp"
#include "wiz8/engine_code/stScript.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/float_constants.h"
#include "wiz8/sr_api.h"
#include "wiz8/item_spawning.h"
#include "wiz8/dialog_code/DialogButton.h"
#include "surrender/srColorSurface.h"
#include "wiz8/cursor.h"
#include "wiz8/engine_code/Quality.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/utility.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/item_video_object_vector.h"
#include "wiz8/xstatus.h"
#include "wiz8/wiz8_windows.h"
#include "wiz8/regions.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/fonts.h"
#include "wiz8/local_screens/MGSKeyboard.h"
#include "input.h"
#include "Types.h"
#include "FileMan.h"
#include "mousesystem.h"
#include "Font.h"
#include "vobject.h"
#include "vobject_blitters.h"
#include "line.h"
#include "surrender/srMeshModel.h"
#include "surrender/srVectorProcessor.h"

#include <stdlib.h>
#include <wchar.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* Lifecycle record 8; the automap screen. Its original screen-class name is
   unknown, so the existing compilation boundary is retained. */

// GLOBAL: WIZ8 0x0068F258
W8Vector<W8AutomapNote*>* g_automap_notes;

// FUNCTION: WIZ8 0x0057e5d0
unsigned char AutomapScreenInitialize(void)
{
    W8Vector<W8AutomapNote*>* list;

    list = new W8Vector<W8AutomapNote*>();
    g_automap_notes = list;
    if (!list) {
        return 0;
    }
    return 1;
}

// GLOBAL: WIZ8 0x0068f104
bool g_mipe_menu_active;
// GLOBAL: WIZ8 0x0068f105
bool g_mipe_active;

/* Lifecycle record 8's own state, all of it released by the finalizer below and
   nothing here naming what any of it holds. The note list is created by this record's initializer at 0x0057E5D0. */

/* One bit per automap cell: raised when the party camera enters the cell. */
// GLOBAL: WIZ8 0x0068F288
BitArray* g_automap_visited_cells;
/* One bit per visited cell already turned into vertex lights. */
// GLOBAL: WIZ8 0x0068F28C
BitArray* g_automap_lit_cells;
/* Packed cell keys read from the level file, indexed by cell number. */
// GLOBAL: WIZ8 0x0068F280
unsigned int* g_automap_cell_keys;

/* Packed cell key -> cell number + 1. */
// GLOBAL: WIZ8 0x0068F284
W8HashTable<unsigned int, int>* g_automap_cell_index;
// GLOBAL: WIZ8 0x0068F29C
stModelInstance2D* g_automap_party_marker;
// GLOBAL: WIZ8 0x0068F2A0
stModelInstance2D* g_automap_friendly_marker;
// GLOBAL: WIZ8 0x0068F2A4
stModelInstance2D* g_automap_neutral_marker;
// GLOBAL: WIZ8 0x0068F2A8
stModelInstance2D* g_automap_hostile_marker;
// GLOBAL: WIZ8 0x0068F2AC
stModelInstance2D* g_automap_text_marker;

/* Leave releases the pointed-to objects and erases their vector entries. */
// GLOBAL: WIZ8 0x0068F1F4
W8Vector<srClass*>* g_automap_markers;

// GLOBAL: WIZ8 0x0068f220
W8GrowableVector<srClipPlane::ClientType*> g_automap_created_layers;
// GLOBAL: WIZ8 0x0068f24c
W8DialogButton** g_automap_buttons;
struct W8AutomapState {
    unsigned char unknown_000[0xf4];
    unsigned int blink_time;
    bool blink_enabled;
    unsigned char pending_cell_lighting;
    unsigned char unknown_0fa[2];
};
static_assert(sizeof(W8AutomapState) == 0xfc, "W8AutomapState_size");
// GLOBAL: WIZ8 0x0068f268
W8AutomapState* g_automap_state;
// GLOBAL: WIZ8 0x0068f274
srColorSurface* g_automap_surface;

void AutomapZoomInButton(void);
void AutomapZoomOutButton(void);
void AutomapSelectNoteToolButton(void);
void AutomapSelectEraseToolButton(void);
void AutomapCyclePageButton(void);
void AutomapLayerDownButton(void);
void AutomapLayerUpButton(void);
void AutomapPanNorthButton(void);
void AutomapPanSouthButton(void);
void AutomapPanWestButton(void);
void AutomapPanEastButton(void);
void AutomapExitButton(void);
void ResetAutomapZoom(void);

/* String-table tooltip indexes for the sixteen automap chrome buttons. */
// GLOBAL: WIZ8 0x0064b7a4
int g_automap_button_tooltips[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
/* Left-click actions. ResetAutomapZoom / RestoreAutomapCameraPosition are the
   already-recovered bodies at the retail callback addresses. */
// GLOBAL: WIZ8 0x0064b7e4
void (*g_automap_button_callbacks[16])(void) = {
    AutomapZoomInButton,         AutomapZoomOutButton,         ResetAutomapZoom,
    AutomapSelectNoteToolButton, AutomapSelectEraseToolButton, AutomapCyclePageButton,
    AutomapCyclePageButton,      AutomapCyclePageButton,       AutomapLayerDownButton,
    AutomapLayerUpButton,        AutomapPanNorthButton,        AutomapPanSouthButton,
    AutomapPanWestButton,        AutomapPanEastButton,         RestoreAutomapCameraPosition,
    AutomapExitButton,
};
/* Catalog object ids ConfigureVObjButton loads for each button. */
// GLOBAL: WIZ8 0x0064b824
int g_automap_button_catalogs[16] = {388, 392, 396, 380, 384, 348, 352, 356,
                                     340, 344, 360, 364, 368, 372, 376, 336};
/* Screen positions for the sixteen buttons. */
// GLOBAL: WIZ8 0x0064b864
srVector2i g_automap_button_positions[16] = {
    {490, 124}, {541, 124}, {591, 124}, {490, 70},  {541, 70},  {591, 70},  {591, 70},  {591, 70},
    {490, 164}, {591, 164}, {540, 218}, {540, 326}, {486, 272}, {594, 272}, {540, 272}, {588, 441},
};
// GLOBAL: WIZ8 0x0064b8e4
srVector2i g_automap_cursor_offsets[5] = {{0, 0}, {8, 7}, {1, 24}, {1, 24}, {8, 7}};
// GLOBAL: WIZ8 0x0064b910
float g_automap_range = 10000.0f;
/* Developer I-key latch; retail has no rendering consumer for this toggle. */
// GLOBAL: WIZ8 0x0064b90d
bool g_automap_developer_i_toggle = true;
// GLOBAL: WIZ8 0x0064b918
int g_automap_layer = -1;
// GLOBAL: WIZ8 0x0064b91c
bool g_automap_bounds_dirty = true;
// GLOBAL: WIZ8 0x0068f138
EnvironmentColour g_automap_saved_light_direction;
// GLOBAL: WIZ8 0x0068f144
EnvironmentColour g_automap_saved_ambient_light;
// GLOBAL: WIZ8 0x0068f150
bool g_automap_saved_sky;
// GLOBAL: WIZ8 0x0068f154
W8WorldCameraState g_automap_saved_camera;
// GLOBAL: WIZ8 0x0068f190
float g_automap_saved_far_clip;
// GLOBAL: WIZ8 0x0068f194
float g_automap_saved_world_value;
// GLOBAL: WIZ8 0x0068f198
unsigned char g_automap_saved_render_flags[4];
// GLOBAL: WIZ8 0x0068f19c
int g_automap_saved_texture_policy;
// GLOBAL: WIZ8 0x0068f1a8
W8GrowableVector<srClipPlane::ClientType*> g_automap_layers;
// GLOBAL: WIZ8 0x0068f1b8
srVector3T<float> g_automap_bounds_max;
// GLOBAL: WIZ8 0x0068f1c8
srVector3T<float> g_automap_grid_max;
// GLOBAL: WIZ8 0x0068f1d8
srVector3T<float> g_automap_grid_min;
// GLOBAL: WIZ8 0x0068f1e8
srVector3T<float> g_automap_position;
// GLOBAL: WIZ8 0x0068f1f8
srVector3T<float> g_automap_grid_center;
// GLOBAL: WIZ8 0x0068f204
float g_automap_top_y;
// GLOBAL: WIZ8 0x0068f210
srVector3T<float> g_automap_bounds_min;
// GLOBAL: WIZ8 0x0068f230
W8ScreenRect g_automap_viewport;
// GLOBAL: WIZ8 0x0068f240
srVector3T<float> g_automap_grid_origin;
// GLOBAL: WIZ8 0x0068f250
int g_automap_tool;
// GLOBAL: WIZ8 0x0068f254
bool g_automap_cursor_inside;
// GLOBAL: WIZ8 0x0068f25c
bool g_automap_redraw;
// GLOBAL: WIZ8 0x0068f260
unsigned int g_automap_page;
// GLOBAL: WIZ8 0x0068f264
bool g_automap_show_all_monsters;
// GLOBAL: WIZ8 0x0068f25d
bool g_automap_overlay_redraw;
// GLOBAL: WIZ8 0x0068f26c
float g_automap_zoom;
// GLOBAL: WIZ8 0x0068f270
bool g_automap_surface_mode;
/* The camera's near clip: how far above the selected layer it floats. */
// GLOBAL: WIZ8 0x0064b920
float g_automap_near_clip = 1.0f;
// GLOBAL: WIZ8 0x0068f278
int g_automap_zoom_mode;
// GLOBAL: WIZ8 0x0068f27c
int g_automap_cell_count;
// GLOBAL: WIZ8 0x0068f290
bool g_automap_position_initialized;
// GLOBAL: WIZ8 0x0068f294
W8AutomapNote* g_automap_editing_note;
// GLOBAL: WIZ8 0x0068f298
W8AutomapNote* g_automap_hovered_note;

unsigned char HandleAutomapKey(const InputAtom* input);
unsigned char HandleAutomapNoteInput(const InputAtom* input);
unsigned char ShowAutomapNoteTooltip(W8AutomapNote* note);
unsigned char ZoomAutomapIn(const srVector3T<float>* point);
void SetAutomapCameraPoint(srVector3T<float>* position);
void SetAutomapButtonMode(int update);
void RenderAutomapFrame(void);
void UpdateAutomapPageButtons(void);
void ResetAutomapLighting(void);
void LightAutomapCell(const srVector3T<float>* position);
unsigned int LightPendingAutomapCells(unsigned int max_count);
int RestoreAutomapRect(W8ScreenRect* rect);
void SetAutomapLayer(int layer);
stModelInstance2D* CreateAutomapItemMarker(int item_id);
stModelInstance2D* CreateAutomapMonsterMarker(int type);
stModelInstance2D* CreateAutomapTextMarker(void);

// FUNCTION: WIZ8 0x00581000
bool HasAutomapLayer(int layer)
{
    return layer >= 0 && layer < g_automap_layers.GetCount() && *g_automap_layers.GetAt(layer) != 0;
}

/* Full-screen automap background: arm on left-down, dismiss on left-up. */
// FUNCTION: WIZ8 0x00581790
unsigned char AutomapBackgroundRegionEvent(const InputAtom* event, W8Region* region)
{
    switch (event->usEvent) {
    case LEFT_BUTTON_DOWN:
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        break;
    case LEFT_BUTTON_UP:
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
            RequestScreenTransition();
            return 1;
        }
        break;
    default:
        return 0;
    }
    return 1;
}

static void ClearAutomapNotes()
{
    while (g_automap_notes->GetCount() != 0) {
        W8AutomapNote* note = *g_automap_notes->GetAt(0);
        free(note->text);
        delete note;
        g_automap_notes->RemoveAt(0);
    }
}

/* Rebuild the automap view for the level that just loaded: release every
   note, size the query range from the level, seed the cell grid from the
   octree bounds (or a fixed cube when there is no octree), and mark the
   camera's own cell in the visited bitmap, retrying one cell higher when the
   packed cell misses the record table. */
// FUNCTION: WIZ8 0x005817d0
void ResetAutomapView(void)
{
    if (g_automap_state == 0) {
        g_automap_state = static_cast<W8AutomapState*>(malloc(sizeof(W8AutomapState)));
        if (g_automap_state == 0) {
            srAssertFail("gpAMSV", "C:\\Projects\\Wizardry 8\\Local Screens\\AutomapScreen.cpp",
                         0x8b5, 0);
        }
        memset(g_automap_state, 0, sizeof(W8AutomapState));
    }
    ClearAutomapNotes();
    g_automap_redraw = true;
    if (g_status.current_level == 0x18 ||
        (g_status.current_level > 0x1a && g_status.current_level <= 0x22)) {
        g_automap_range = 30000.0f;
    } else {
        g_automap_range = 10000.0f;
    }
    if (g_octree == 0) {
        g_automap_grid_min = -250000.0f;
        g_automap_grid_max = 250000.0f;
    } else {
        g_octree->m_spatial.GetClippedBounds(&g_automap_grid_min, &g_automap_grid_max);
    }
    g_automap_grid_origin = g_automap_grid_min;
    g_automap_grid_center.Set((g_automap_grid_min.x + g_automap_grid_max.x) * g_float_half, 0.0f,
                              (g_automap_grid_min.z + g_automap_grid_max.z) * g_float_half);
    g_automap_bounds_dirty = true;
    float span_x = g_automap_grid_max.x - g_automap_grid_min.x;
    float span_z = g_automap_grid_max.z - g_automap_grid_min.z;
    float largest = span_z < span_x ? span_x : span_z;
    if (largest <= g_float_twenty_five_thousand) {
        g_automap_top_y = 25000.0f;
    } else {
        g_automap_top_y = largest;
    }
    g_automap_position_initialized = false;
    g_automap_layer = 0;

    srVector3T<float> camera;
    GetCameraPosition(&camera);
    srVector3T<float> relative = camera - g_automap_grid_origin;
    unsigned int key = AutomapNodeKey(&relative);
    int cell = g_automap_cell_index->Lookup(&key);
    if (cell > 1) {
        g_automap_visited_cells->Set(cell - 1);
        return;
    }
    relative.y = camera.y + g_automap_grid_cell_size - g_automap_grid_origin.y;
    key = AutomapNodeKey(&relative);
    cell = g_automap_cell_index->Lookup(&key);
    if (cell > 1) {
        g_automap_visited_cells->Set(cell - 1);
    }
}

/* Pack `position` into a cell key (retrying one cell higher on miss). When the
   record table knows the cell, mark it visited and return 1 only if that bit
   was newly raised — used by the world camera update to gate automap lighting. */
// FUNCTION: WIZ8 0x00581B30
bool AutomapHasCellAt(const srVector3T<float>* position)
{
    srVector3T<float> relative = *position - g_automap_grid_origin;
    unsigned int key = AutomapNodeKey(&relative);
    int cell = g_automap_cell_index->Lookup(&key);
    if (cell > 1) {
        if (!g_automap_visited_cells->Set(cell - 1)) {
            return true;
        }
        return false;
    }
    relative.y = position->y + g_automap_grid_cell_size - g_automap_grid_origin.y;
    key = AutomapNodeKey(&relative);
    cell = g_automap_cell_index->Lookup(&key);
    if (cell > 1) {
        if (!g_automap_visited_cells->Set(cell - 1)) {
            return true;
        }
    }
    return false;
}

// FUNCTION: WIZ8 0x0057E490
bool CanUseCurrentAutomapTool(void)
{
    if (g_mipe_active) {
        switch (g_mipe_mode) {
        case 6:
        case 7:
        case 15:
        case 18:
        case 23:
        case 29:
            return false;
        }
    }
    return true;
}

// FUNCTION: WIZ8 0x00584210
void RestoreAutomapCameraPosition(void)
{
    g_automap_position.x = g_automap_saved_camera.position.x;
    g_automap_position.z = g_automap_saved_camera.position.z;
    SetAutomapCameraPoint(&g_automap_position);
}

/* Build the sixteen chrome buttons from the adjacent catalog / callback /
   tooltip / position tables. */
// FUNCTION: WIZ8 0x00583BC0
void CreateAutomapButtons(void)
{
    int index;
    int base_frame;
    HVOBJECT object;

    g_automap_buttons = new W8DialogButton*[16];
    if (g_automap_buttons == 0) {
        srAssertFail("gpstAutomapButtons",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\AutomapScreen.cpp", 0xd05, 0);
    }
    for (index = 0; index < 16; ++index) {
        object = GetCatalogVideoObject(g_automap_button_catalogs[index], 0, &base_frame);
        if (object != 0) {
            g_automap_buttons[index] = new W8DialogButton;
            if (g_automap_buttons[index] != 0) {
                W8DialogButtonCallback callback = reinterpret_cast<W8DialogButtonCallback>(
                    g_automap_button_callbacks[index]); // reinterpret-ok: void() vs button*
                if (g_automap_buttons[index]->ConfigureVObjButton(object, base_frame, callback,
                                                                  false)) {
                    g_automap_buttons[index]->SetPosition(g_automap_button_positions[index].x,
                                                          g_automap_button_positions[index].y);
                    g_automap_buttons[index]->SetTooltipIndex(g_automap_button_tooltips[index]);
                }
            }
        }
    }
}

// FUNCTION: WIZ8 0x00583CE0
void AutomapZoomInButton(void)
{
    srVector3T<float> center;
    center.Set(0.5f, 0.5f, 0.0f);
    ZoomAutomapIn(&center);
}

/* Mirror of the right-click zoom-out path: one step when the camera is below
   the full top height, otherwise restore the full explored span. The one-step
   arm inlines SetAutomapToolCursor the same way retail does. */
// FUNCTION: WIZ8 0x00583D10
void AutomapZoomOutButton(void)
{
    if (g_automap_position.y < g_automap_top_y) {
        if (g_automap_zoom_mode == 1) {
            g_automap_zoom = g_automap_top_y - g_automap_bounds_min.y;
            srVector3T<float> position(g_automap_bounds_min.x + g_automap_bounds_max.x,
                                       g_automap_bounds_max.y + g_automap_bounds_min.y,
                                       g_automap_bounds_min.z + g_automap_bounds_max.z);
            position = position * 0.5;
            position.y = g_automap_top_y;
            SetAutomapCameraPoint(&position);
            SetAutomapToolCursor(g_automap_tool);
            SetAutomapButtonMode(0);
        } else {
            float ground_y = g_automap_position.y - g_automap_zoom;
            float height = g_automap_top_y - (g_automap_top_y - ground_y) * g_float_half;
            if (g_automap_position.y <= height) {
                g_automap_position.y = height;
                if (g_automap_buttons != 0) {
                    g_automap_buttons[0]->SetEnabled(true);
                    g_automap_buttons[1]->SetEnabled(true);
                    g_automap_buttons[0]->m_dirty = true;
                    g_automap_buttons[0]->Draw();
                    g_automap_buttons[1]->m_dirty = true;
                    g_automap_buttons[1]->Draw();
                }
                g_automap_zoom = g_automap_position.y - ground_y;
                g_automap_zoom_mode = 1;
                SetAutomapCameraPoint(&g_automap_position);
                SetAutomapToolCursor(g_automap_tool);
            } else {
                g_automap_zoom = g_automap_top_y - g_automap_bounds_min.y;
                srVector3T<float> position(g_automap_bounds_min.x + g_automap_bounds_max.x,
                                           g_automap_bounds_max.y + g_automap_bounds_min.y,
                                           g_automap_bounds_min.z + g_automap_bounds_max.z);
                position = position * 0.5;
                position.y = g_automap_top_y;
                SetAutomapCameraPoint(&position);
                SetAutomapToolCursor(g_automap_tool);
                SetAutomapButtonMode(0);
            }
        }
    }
}

// FUNCTION: WIZ8 0x00583FC0
void AutomapSelectNoteToolButton(void)
{
    if (g_automap_tool == 2) {
        return;
    }
    g_automap_tool = 2;
    SetMouseCursorFromVideoObject(GetCatalogVideoObjectHandle(0x14d, 0),
                                  GetCatalogVideoObjectYOffset(0x14d),
                                  static_cast<short>(g_automap_cursor_offsets[2].x),
                                  static_cast<short>(g_automap_cursor_offsets[2].y));
    gXStatus.iCurrentCursor = W8_CURSOR_INVALID_TARGET;
    RefreshMouseCursorTexture();
}

// FUNCTION: WIZ8 0x00584020
void AutomapSelectEraseToolButton(void)
{
    if (g_automap_tool == 3) {
        return;
    }
    g_automap_tool = 3;
    SetMouseCursorFromVideoObject(GetCatalogVideoObjectHandle(0x14e, 0),
                                  GetCatalogVideoObjectYOffset(0x14e),
                                  static_cast<short>(g_automap_cursor_offsets[3].x),
                                  static_cast<short>(g_automap_cursor_offsets[3].y));
    gXStatus.iCurrentCursor = W8_CURSOR_INVALID_TARGET;
    RefreshMouseCursorTexture();
}

// FUNCTION: WIZ8 0x00584080
void AutomapCyclePageButton(void)
{
    g_automap_page = (g_automap_page + 1) % 3;
    UpdateAutomapPageButtons();
    g_automap_redraw = true;
}

// FUNCTION: WIZ8 0x00584110
void AutomapLayerDownButton(void)
{
    if (g_automap_layer > 0) {
        SetAutomapLayer(g_automap_layer - 1);
    }
}

// FUNCTION: WIZ8 0x00584130
void AutomapLayerUpButton(void)
{
    if (g_automap_layer < g_automap_layers.GetCount()) {
        SetAutomapLayer(g_automap_layer + 1);
    }
}

// FUNCTION: WIZ8 0x00584150
void AutomapPanNorthButton(void)
{
    g_automap_position.z += g_float_thirty_five_hundredths * g_automap_zoom;
    SetAutomapCameraPoint(&g_automap_position);
}

// FUNCTION: WIZ8 0x00584180
void AutomapPanSouthButton(void)
{
    g_automap_position.z -= g_float_thirty_five_hundredths * g_automap_zoom;
    SetAutomapCameraPoint(&g_automap_position);
}

// FUNCTION: WIZ8 0x005841B0
void AutomapPanWestButton(void)
{
    g_automap_position.x -= g_float_thirty_five_hundredths * g_automap_zoom;
    SetAutomapCameraPoint(&g_automap_position);
}

// FUNCTION: WIZ8 0x005841E0
void AutomapPanEastButton(void)
{
    g_automap_position.x += g_float_thirty_five_hundredths * g_automap_zoom;
    SetAutomapCameraPoint(&g_automap_position);
}

// FUNCTION: WIZ8 0x00584240
void AutomapExitButton(void)
{
    if (g_automap_editing_note == 0) {
        RequestScreenTransition();
    }
}

// FUNCTION: WIZ8 0x0057e660
unsigned char AutomapScreenEnter(void)
{
    stScript script;
    MSYS_Init();
    GetLightDirection(&g_automap_saved_light_direction);
    GetWorldLightValue(g_world, &g_automap_saved_ambient_light);
    GetWorldCameraState(GetWorld(), &g_automap_saved_camera);
    g_automap_saved_far_clip = static_cast<float>(WorldGetFarClip(g_world));
    g_automap_saved_world_value = WorldGetRenderRange(g_world);
    g_automap_saved_sky = g_sky_enabled;
    g_automap_saved_render_flags[0] = GetRenderOptionState(W8_RENDER_OPTION_HIGH_TEXTURE_DETAIL);
    g_automap_saved_render_flags[1] = GetRenderOptionState(W8_RENDER_OPTION_MESH_SKY);
    g_automap_saved_render_flags[2] = g_monster_shadow_updates_enabled;
    g_automap_saved_render_flags[3] = g_world_render_enabled;
    g_automap_saved_texture_policy = g_resident_texture_policy;
    EnvironmentColour direction;
    direction = 0.0;
    SetLightDirection(&direction);
    SetWorldEnvironmentColour(g_world, EnvironmentColour(0.0, 0.0, 0.0));
    DisableSky();
    DisableRenderOption(W8_RENDER_OPTION_MESH_SKY);
    g_monster_shadow_updates_enabled = 1;
    g_world_render_enabled = 1;
    DisableSky();
    g_world->camera->setClipRange(g_automap_near_clip, 1500000.0);
    WorldSetRenderRange(g_world, 1500000.0f);
    g_world->camera->setRotation(3.141592653589793 * (1.0f / 180.0f) * 90.0f, 0.0, 0.0);
    int layer_number = 1;
    g_world->camera->setProjectionType(srCamera::PROJECT_ORTHOGRAPHIC);
    RestoreAutomapCameraPosition();
    UpdateWorldMesh(g_world);
    SetWorldMeshVertexLightTable(g_world, 1);
    g_light_update_flags &= ~1u;
    SetWorldModelPickingEnabled(false);
    if (!g_automap_state) {
        g_automap_state = static_cast<W8AutomapState*>(malloc(sizeof(W8AutomapState)));
        if (!g_automap_state)
            return 0;
        memset(g_automap_state, 0, sizeof(W8AutomapState));
    }
    g_automap_state->blink_time = GetTickCount();
    g_automap_viewport.left = 12;
    g_automap_viewport.top = 32;
    g_automap_viewport.right = 467;
    g_automap_viewport.bottom = 467;
    PauseMainGameWorld();
    g_automap_layers.Clear();
    g_automap_layers.Add(0);
    char layer_name[16];
    sprintf(layer_name, "LAYER %d", layer_number);
    srClipPlane::ClientType* layer = static_cast<srClipPlane::ClientType*>(
        srCore.getRegistry()->find(srClipPlane::ClientType::sGetClassNode(), layer_name, 0));
    while (layer) {
        g_automap_layers.Add(layer);
        ++layer_number;
        sprintf(layer_name, "LAYER %d", layer_number);
        layer = static_cast<srClipPlane::ClientType*>(
            srCore.getRegistry()->find(srClipPlane::ClientType::sGetClassNode(), layer_name, 0));
    }
    unsigned int monster_count = PLLength(gXStatus.plsMonsterList);
    for (unsigned int index = 0; index < monster_count; ++index) {
        W8MonsterInfo* monster = GetMonsterListEntry(index);
        if (monster->p3D)
            monster->p3D->DetachRepresentation(g_world);
    }
    for (W8WorldItem* item = GetNextWorldItem(true); item; item = GetNextWorldItem(false)) {
        if (item->p3D)
            item->p3D->DetachMesh(g_world);
    }
    gfTrackMousePos = 1;
    g_automap_cursor_inside = IsCursorInsideViewport();
    g_automap_tool = 0;
    SetAutomapToolCursor(0);
    CreateAutomapMarkerSprites();
    CreateAutomapButtons();
    g_automap_party_marker->setParent(0, 1);
    g_automap_surface_mode = RendererBufferIsLockable();
    if (g_automap_surface_mode) {
        g_world_render_enabled = 0;
        g_monster_shadow_updates_enabled = 0;
        if (!g_automap_surface) {
            srColorSurface* surface =
                SR_NEW(W8ColorSurface)(srPixelConvert::SURFACE_ARGB1555, 640, 480);
            g_automap_surface = surface;
            if (surface)
                surface->setFilter(&srBoxFilter);
        }
    }
    g_automap_redraw = true;
    g_automap_overlay_redraw = true;
    SetPrimarySurfaceTextureHint2Enabled(false);
    ClearSurfaceRect(0, 0, 640, 480);
    DisableCursorScene();
    DrawCatalogImageAndInvalidate(FRAME_BUFFER, 0x14a, 0, 0, 0, 0, VO_BLT_SRCTRANSPARENCY, 0);
    UpdateAutomapPageButtons();
    for (int button = 0; button < 16; ++button) {
        if (g_automap_buttons[button]) {
            g_automap_buttons[button]->m_dirty = true;
            g_automap_buttons[button]->Draw();
        }
    }
    ResetTransientRenderScenes();
    RenderFrame();
    RenderFrame();
    EnableCursorScene();
    SetAutomapScaledViewport(12, 32, 467, 467);
    UpdateAutomapBounds();
    if (script.Load("Data\\Automap\\MapFilters.txt")) {
        W8Vector<char*> excluded_textures(5);
        int line = 0;
        int section = -1;
        while (section < g_status.current_level && line < script.lines.GetCount()) {
            if (strchr((*script.lines.GetAt(line))->text, '['))
                ++section;
            ++line;
        }
        if (line < script.lines.GetCount() && section == g_status.current_level) {
            for (; line < script.lines.GetCount(); ++line) {
                char* text = (*script.lines.GetAt(line))->text;
                if (strchr(text, '['))
                    break;
                if (!strstr(text, "LAYER=")) {
                    excluded_textures.Add(text);
                } else {
                    float height = static_cast<float>(atof(text + 6));
                    srClipPlane::ClientType* clip = SR_NEW(srClipPlane)(static_cast<srNode*>(0));
                    if (clip) {
                        sprintf(layer_name, "LAYER %d", layer_number);
                        clip->setName(layer_name);
                        srVector4T<float> plane;
                        plane.Set(0.0f, 1.0f, 0.0f, 0.0f);
                        clip->setClipPlane(plane);
                        srVector3T<double> position(0.0, static_cast<double>(height * 500.0f), 0.0);
                        clip->setLocation(position);
                        clip->setFlag(static_cast<srNode::e_flag>(2));
                        clip->setClipType(srClipPlane::CLIP_POSITIONAL_0);
                        clip->setFlag(static_cast<srNode::e_flag>(0));
                        g_automap_layers.Add(clip);
                        ++layer_number;
                        g_automap_created_layers.Add(clip);
                    }
                }
            }
            for (unsigned int mesh = 0; mesh < g_world->octree->m_meshCount; ++mesh) {
                for (stMeshModel* model =
                         static_cast<stMeshModel*>(g_world->psrMeshes[mesh]->getModel());
                     model; model = model->next) {
                    model->ApplyAutomapPolygonFilter(&excluded_textures);
                }
            }
        }
    }
    if (!g_automap_position_initialized) {
        g_automap_zoom = g_automap_top_y - g_automap_bounds_min.y;
        g_automap_position_initialized = true;
        srVector3T<float> position = (g_automap_bounds_min + g_automap_bounds_max) / 2.0;
        position.y = g_automap_top_y;
        SetAutomapCameraPoint(&position);
        SetAutomapToolCursor(g_automap_tool);
        SetAutomapButtonMode(0);
    } else {
        RestoreAutomapCameraPosition();
    }
    return 1;
}

// FUNCTION: WIZ8 0x005806b0
EnvironmentColour::EnvironmentColour(double red_value, double green_value, double blue_value)
{
    Set(red_value, green_value, blue_value);
}

// FUNCTION: WIZ8 0x00580940
void EnvironmentColour::Set(double red_value, double green_value, double blue_value)
{
    x = static_cast<float>(red_value);
    y = static_cast<float>(green_value);
    z = static_cast<float>(blue_value);
    if (x > 0.0f) {
        if (x >= 1.0f)
            x = 1.0f;
    } else {
        x = 0.0f;
    }
    if (y > 0.0f) {
        if (y >= 1.0f)
            y = 1.0f;
    } else {
        y = 0.0f;
    }
    if (z > 0.0f) {
        if (z >= 1.0f)
            z = 1.0f;
    } else {
        z = 0.0f;
    }
}

// FUNCTION: WIZ8 0x00581360
W8AutomapNote* CreateAutomapNote(const srVector2T<float>* position, int layer, const wchar_t* text)
{
    if (text && wcslen(text) < 40 && g_automap_notes->GetCount() < 200) {
        W8AutomapNote* note = new W8AutomapNote;
        if (note) {
            note->position = *position;
            note->layer = layer;
            note->text = static_cast<wchar_t*>(malloc(0x50));
            wcscpy(note->text, text);
            g_automap_notes->Add(note);
            g_automap_redraw = true;
            return note;
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x0057f1f0
void AutomapScreenFrame(void)
{
    InputAtom input;
    while (DequeueEvent(&input) == 1) {
        if (g_automap_editing_note) {
            HandleAutomapNoteInput(&input);
            continue;
        }
        if (HandleAutomapKey(&input))
            continue;
        if (!IsCursorInsideViewport()) {
            if (g_automap_cursor_inside) {
                g_automap_cursor_inside = false;
                SetMouseCursorFromVideoObject(
                    GetCatalogVideoObjectHandle(g_automap_tool + 0x14b, 0),
                    GetCatalogVideoObjectYOffset(g_automap_tool + 0x14b),
                    g_automap_cursor_offsets[g_automap_tool].x,
                    g_automap_cursor_offsets[g_automap_tool].y);
                gXStatus.iCurrentCursor = W8_CURSOR_INVALID_TARGET;
                RefreshMouseCursorTexture();
            }
            POINT point;
            SGPMouseGetPos(&point);
            MSYS_SGP_Mouse_Handler_Hook(MOUSE_POS, point.x, point.y, gfLeftButtonState,
                                        gfRightButtonState);
            unsigned short reason;
            switch (input.usEvent) {
            case LEFT_BUTTON_DOWN:
            case LEFT_BUTTON_REPEAT:
                reason = LEFT_BUTTON_DOWN;
                break;
            case LEFT_BUTTON_UP:
                reason = LEFT_BUTTON_UP;
                break;
            case RIGHT_BUTTON_DOWN:
                reason = RIGHT_BUTTON_DOWN;
                break;
            case RIGHT_BUTTON_UP:
                reason = RIGHT_BUTTON_UP;
                break;
            default:
                continue;
            }
            MSYS_SGP_Mouse_Handler_Hook(reason, point.x, point.y, gfLeftButtonState,
                                        gfRightButtonState);
            continue;
        }
        if (!g_automap_cursor_inside) {
            g_automap_cursor_inside = true;
            int cursor = g_automap_tool;
            if (cursor == 0)
                cursor = g_automap_zoom > 25000.0f ? 1 : 4;
            SetMouseCursorFromVideoObject(GetCatalogVideoObjectHandle(cursor + 0x14b, 0),
                                          GetCatalogVideoObjectYOffset(cursor + 0x14b),
                                          g_automap_cursor_offsets[cursor].x,
                                          g_automap_cursor_offsets[cursor].y);
            gXStatus.iCurrentCursor = W8_CURSOR_INVALID_TARGET;
            RefreshMouseCursorTexture();
        }
        if (input.usEvent == LEFT_BUTTON_UP) {
            srVector3T<float> point;
            if (g_automap_tool == 2) {
                if (GetCursorPositionInViewport(&point)) {
                    int layer = g_automap_layer + 1;
                    if (HasAutomapLayer(layer)) {
                        (*g_automap_layers.GetAt(layer))->getLocationY();
                    }
                    srVector2T<float> location;
                    location.Set((point.x - 0.5f) * g_automap_zoom + g_automap_position.x,
                                 g_automap_position.z - (point.y - 0.5f) * g_automap_zoom);
                    g_automap_editing_note = CreateAutomapNote(&location, g_automap_layer, L"_");
                }
            } else if (g_automap_tool == 3) {
                W8AutomapNote* note = FindAutomapNoteUnderCursor();
                if (note) {
                    g_automap_notes->Remove(note);
                    free(note->text);
                    delete note;
                    if (g_automap_hovered_note == note)
                        g_automap_hovered_note = 0;
                    if (g_automap_editing_note == note)
                        g_automap_editing_note = 0;
                    g_automap_redraw = true;
                }
            } else if (GetCursorPositionInViewport(&point)) {
                ZoomAutomapIn(&point);
            }
        } else if (input.usEvent == RIGHT_BUTTON_UP) {
            if (g_automap_tool == 0) {
                if (g_automap_position.y < g_automap_top_y) {
                    if (g_automap_zoom_mode == 1) {
                        g_automap_zoom = g_automap_top_y - g_automap_bounds_min.y;
                        srVector3T<float> position =
                            (g_automap_bounds_max + g_automap_bounds_min) / 2.0;
                        position.y = g_automap_top_y;
                        SetAutomapCameraPoint(&position);
                        SetAutomapToolCursor(g_automap_tool);
                        SetAutomapButtonMode(0);
                    } else {
                        float ground_y = g_automap_position.y - g_automap_zoom;
                        float height = g_automap_top_y - (g_automap_top_y - ground_y) * 0.5f;
                        if (g_automap_position.y <= height) {
                            g_automap_position.y = height;
                            SetAutomapButtonMode(1);
                            g_automap_zoom = g_automap_position.y - ground_y;
                            SetAutomapCameraPoint(&g_automap_position);
                            SetAutomapToolCursor(g_automap_tool);
                        } else {
                            ResetAutomapZoom();
                        }
                    }
                }
            } else {
                g_automap_tool = 0;
                SetAutomapToolCursor(0);
            }
        } else if (input.usEvent == MOUSE_POS) {
            W8AutomapNote* previous = g_automap_hovered_note;
            g_automap_hovered_note = FindAutomapNoteUnderCursor();
            if (previous != g_automap_hovered_note) {
                if (previous)
                    ShowAutomapNoteTooltip(previous);
                RenderAutomapMarkers();
            }
            srVector3T<float> point;
            if (GetAutomapPositionUnderCursor(&point)) {
                point.y = 0.0f;
                srVector3T<float> distance(point.x - g_automap_saved_camera.position.x, 0.0f,
                                           point.z - g_automap_saved_camera.position.z);
                if (distance.Length() < g_automap_zoom * 0.05f && g_automap_tool == 0 &&
                    g_automap_zoom_mode != 2) {
                    DisableCursorScene();
                    srVector3T<double> scale(0.44f, 0.44f, 0.44f);
                    g_automap_party_marker->setScale(scale);
                    continue;
                }
            }
            float factor = (1.0f / (g_automap_zoom * 0.00004f)) * 0.44f;
            srVector3T<double> scale(factor, factor, factor);
            g_automap_party_marker->setScale(scale);
            EnableCursorScene();
        }
    }
    bool moved = false;
    if (gfKeyState[0x25]) {
        g_automap_position.x -= g_float_thirty_five_hundredths * g_automap_zoom;
        moved = true;
    }
    if (gfKeyState[0x27]) {
        g_automap_position.x += g_float_thirty_five_hundredths * g_automap_zoom;
        moved = true;
    }
    if (gfKeyState[0x26]) {
        g_automap_position.z += g_float_thirty_five_hundredths * g_automap_zoom;
        moved = true;
    }
    if (gfKeyState[0x28]) {
        g_automap_position.z -= g_float_thirty_five_hundredths * g_automap_zoom;
        moved = true;
    }
    if (moved)
        SetAutomapCameraPoint(&g_automap_position);
    if (GetTickCount() - g_automap_state->blink_time > 500) {
        if (g_automap_state->blink_enabled)
            DrawVideoInspector(0xdc, 0x32);
        g_automap_state->blink_time = GetTickCount();
    }
    RenderAutomapFrame();
}

// FUNCTION: WIZ8 0x0057fb40
void RestoreAutomapWorldSettings(void)
{
    SetWorldEnvironmentColour(g_world, g_automap_saved_ambient_light);
    SetLightDirection(&g_automap_saved_light_direction);
    RestoreWorldCameraState(GetWorld(), 0, &g_automap_saved_camera);
    WorldSetFarClip(g_world, g_automap_saved_far_clip);
    WorldSetRenderRange(g_world, g_automap_saved_world_value);
    if (g_automap_saved_sky)
        EnableSky();
    SetRenderOption(W8_RENDER_OPTION_HIGH_TEXTURE_DETAIL, g_automap_saved_render_flags[0]);
    SetRenderOption(W8_RENDER_OPTION_MESH_SKY, g_automap_saved_render_flags[1]);
    g_monster_shadow_updates_enabled = g_automap_saved_render_flags[2];
    g_world_render_enabled = g_automap_saved_render_flags[3];
    g_world->camera->setRotation(0.0, 0.0, 0.0);
    g_world->camera->flags.value &= ~1ul;
    SetWorldMeshVertexLightTable(g_world, 0);
    g_light_update_flags |= 1u;
    SetResidentTexturePolicy(g_resident_texture_policy);
    SetWorldModelPickingEnabled(true);
}

// FUNCTION: WIZ8 0x0057efe0
unsigned char AutomapScreenLeave(int)
{
    RestoreAutomapWorldSettings();
    free(g_automap_state);
    g_automap_state = 0;
    MarkRendererReady();
    SetOverlayViewport(0);
    ResumeMainGameWorld();
    srClass* clipping_plane = static_cast<srClass*>(srCore.getRegistry()->find(
        srClipPlane::ClientType::sGetClassNode(), "Clipping Plane 1", 0));
    if (clipping_plane) {
        g_world->level->setParent(g_world->static_scene, 1);
        g_world->dynamic_scene->setParent(g_world->static_scene, 1);
        clipping_plane->release();
    }
    gfTrackMousePos = 0;
    if (g_automap_surface) {
        g_automap_surface->release();
        g_automap_surface = 0;
    }
    while (g_automap_markers->GetCount()) {
        srClass* object = *g_automap_markers->GetAt(0);
        object->release();
        g_automap_markers->Remove(object);
    }
    g_automap_party_marker->setParent(0, 1);
    for (int index = 0; index < 16; ++index) {
        delete g_automap_buttons[index];
    }
    delete[] g_automap_buttons;
    g_automap_buttons = 0;
    MSYS_Shutdown();
    UpdateHeldItemCursor();
    EnableCursorScene();
    for (unsigned int mesh = 0; mesh < g_world->octree->m_meshCount; ++mesh) {
        for (stMeshModel* model = static_cast<stMeshModel*>(g_world->psrMeshes[mesh]->getModel());
             model; model = model->next) {
            model->ClearAutomapPolygonFilter();
        }
    }
    while (g_automap_created_layers.GetCount()) {
        if (*g_automap_created_layers.GetAt(0))
            (*g_automap_created_layers.GetAt(0))->release();
        g_automap_created_layers.RemoveAt(0);
    }
    return 1;
}

/* Lifecycle record 8's finalizer, the fifth slot of its record and the third
   allocate/release pair that establishes what that slot is for. Every guard is
   the original's own, and each pointer is cleared after its release. */
// FUNCTION: WIZ8 0x0057fa20
unsigned char AutomapScreenFinalize(void)
{
    if (g_automap_notes) {
        delete g_automap_notes;
        g_automap_notes = 0;
    }
    BitArray* bits = g_automap_visited_cells;
    if (bits) {
        delete bits;
        g_automap_visited_cells = 0;
    }
    bits = g_automap_lit_cells;
    if (bits) {
        delete bits;
        g_automap_lit_cells = 0;
    }
    if (g_automap_cell_keys) {
        free(g_automap_cell_keys);
        g_automap_cell_keys = 0;
    }
    W8HashTable<unsigned int, int>* record = g_automap_cell_index;
    if (record) {
        delete record;
        g_automap_cell_index = 0;
    }
    if (g_automap_party_marker) {
        g_automap_party_marker->release();
        g_automap_party_marker = 0;
    }
    if (g_automap_friendly_marker) {
        g_automap_friendly_marker->release();
        g_automap_friendly_marker = 0;
    }
    if (g_automap_neutral_marker) {
        g_automap_neutral_marker->release();
        g_automap_neutral_marker = 0;
    }
    if (g_automap_hostile_marker) {
        g_automap_hostile_marker->release();
        g_automap_hostile_marker = 0;
    }
    if (g_automap_markers) {
        delete g_automap_markers;
        g_automap_markers = 0;
    }
    return 1;
}

/* Select which automap buttons are enabled for the update mode, then dirty
   and redraw both and remember the mode. */
// FUNCTION: WIZ8 0x0057FD90
void SetAutomapButtonMode(int update)
{
    if (g_automap_buttons == 0) {
        g_automap_zoom_mode = update;
        return;
    }
    if (update == 0) {
        g_automap_buttons[0]->SetEnabled(true);
        g_automap_buttons[1]->SetEnabled(false);
    } else if (update == 1) {
        g_automap_buttons[0]->SetEnabled(true);
        g_automap_buttons[1]->SetEnabled(true);
    } else if (update == 2) {
        g_automap_buttons[0]->SetEnabled(false);
        g_automap_buttons[1]->SetEnabled(true);
    }
    g_automap_buttons[0]->m_dirty = true;
    g_automap_buttons[0]->Draw();
    g_automap_buttons[1]->m_dirty = true;
    g_automap_buttons[1]->Draw();
    g_automap_zoom_mode = update;
}

/* Show the mouse cursor for an automap tool. With no tool selected and the
   cursor over the map, the zoom level picks the zoom-in or zoom-out cursor. */
// FUNCTION: WIZ8 0x005820F0
void SetAutomapToolCursor(int tool)
{
    if (g_automap_tool == 0 && g_automap_cursor_inside) {
        if (g_automap_zoom <= g_float_twenty_five_thousand) {
            tool = 4;
        } else {
            tool = 1;
        }
    }
    SetMouseCursorFromVideoObject(
        GetCatalogVideoObjectHandle(tool + 0x14b, 0), GetCatalogVideoObjectYOffset(tool + 0x14b),
        g_automap_cursor_offsets[tool].x, g_automap_cursor_offsets[tool].y);
    gXStatus.iCurrentCursor = W8_CURSOR_INVALID_TARGET;
    RefreshMouseCursorTexture();
}

// GLOBAL: WIZ8 0x0064b914
float g_automap_grid_cell_size = 2000.0f;
// GLOBAL: WIZ8 0x0068f2b0
W8MainUiMode g_ui_mode_current;
// GLOBAL: WIZ8 0x0068f2c4
W8MainUiMode g_ui_mode_saved;

// FUNCTION: WIZ8 0x00585300
void SetAutomapGridCellSize(float value)
{
    g_automap_grid_cell_size = value;
}

// FUNCTION: WIZ8 0x00585310
float GetAutomapGridCellSize(void)
{
    return g_automap_grid_cell_size;
}

// FUNCTION: WIZ8 0x00587C10
void SetCurrentAutomapUiMode(W8MainUiMode value)
{
    g_ui_mode_current = value;
}

// FUNCTION: WIZ8 0x0058A870
void SetSavedAutomapUiMode(W8MainUiMode value)
{
    g_ui_mode_saved = value;
}
/* Park the automap camera over the requested point with its xz clamped into
   the explored bounds, reselect the layer underneath and refresh the mesh. */
// FUNCTION: WIZ8 0x0057FC70
void SetAutomapCameraPoint(srVector3T<float>* position)
{
    srVector3T<float> clamped;
    clamped.x = g_automap_bounds_min.x <= position->x ? position->x : g_automap_bounds_min.x;
    clamped.x = clamped.x < g_automap_bounds_max.x
                    ? (g_automap_bounds_min.x <= position->x ? position->x : g_automap_bounds_min.x)
                    : g_automap_bounds_max.x;
    clamped.y = position->y;
    clamped.z = g_automap_bounds_min.z <= position->z ? position->z : g_automap_bounds_min.z;
    clamped.z = clamped.z < g_automap_bounds_max.z
                    ? (position->z < g_automap_bounds_min.z ? g_automap_bounds_min.z : position->z)
                    : g_automap_bounds_max.z;
    g_world->camera->setLocation(srVector3T<double>(clamped.x, clamped.y, clamped.z));
    g_automap_position = clamped;
    SetAutomapLayer(g_automap_layer);
    g_automap_redraw = true;
    g_automap_overlay_redraw = true;
    if (g_world->octree != 0) {
        MarkRendererReady();
        UpdateWorldMeshAfterLoad();
    }
}

/* Select the automap layer, remember how far above it the camera floats and
   refresh the layer-up/layer-down buttons. */
// FUNCTION: WIZ8 0x00580F20
void SetAutomapLayer(int layer)
{
    if (layer < 0 || g_automap_layers.GetCount() == 0 || g_automap_layers.GetCount() <= layer ||
        *g_automap_layers.GetAt(layer) == 0) {
        g_automap_near_clip = 1.0f;
    } else {
        float height = g_automap_position.y -
                       static_cast<float>((*g_automap_layers.GetAt(layer))->getLocation().y);
        if (height < 1.0f) {
            g_automap_near_clip = 1.0f;
        } else {
            g_automap_near_clip = height;
        }
    }
    g_automap_layer = layer;
    g_automap_redraw = true;
    g_automap_overlay_redraw = true;
    if (g_automap_buttons != 0) {
        if (g_automap_buttons[9] != 0) {
            g_automap_buttons[9]->SetEnabled(layer != g_automap_layers.GetCount() - 1);
        }
        if (g_automap_buttons[8] != 0) {
            g_automap_buttons[8]->SetEnabled(layer != 0);
        }
    }
}

/* Reset the zoom to the full explored span, recenter over the bounds and
   restore the neutral button mode and cursor. */
// FUNCTION: WIZ8 0x0057FE40
void ResetAutomapZoom(void)
{
    g_automap_zoom = g_automap_top_y - g_automap_bounds_min.y;
    srVector3T<float> position;
    position.Set((g_automap_bounds_min.x + g_automap_bounds_max.x) * 0.5,
                 (g_automap_bounds_min.y + g_automap_bounds_max.y) * 0.5,
                 (g_automap_bounds_min.z + g_automap_bounds_max.z) * 0.5);
    position.y = g_automap_top_y;
    SetAutomapCameraPoint(&position);
    int tool = g_automap_tool;
    if (g_automap_tool == 0 && g_automap_cursor_inside) {
        tool = g_float_twenty_five_thousand < g_automap_zoom ? 1 : 4;
    }
    SetMouseCursorFromVideoObject(GetCatalogVideoObjectHandle(tool + 0x14b, 0),
                                  GetCatalogVideoObjectYOffset(tool + 0x14b),
                                  static_cast<short>(g_automap_cursor_offsets[tool].x),
                                  static_cast<short>(g_automap_cursor_offsets[tool].y));
    gXStatus.iCurrentCursor = W8_CURSOR_INVALID_TARGET;
    RefreshMouseCursorTexture();
    SetAutomapButtonMode(0);
}

/* Left-click zoom: drop a sight line onto the clicked point, halve the
   remaining height above ground and move the camera there. */
// FUNCTION: WIZ8 0x0057FFC0
unsigned char ZoomAutomapIn(const srVector3T<float>* point)
{
    srVector3T<float> position;
    position.x = (point->x - g_float_half) * g_automap_zoom + g_automap_position.x;
    position.y = g_automap_top_y;
    position.z = g_automap_position.z - (point->y - g_float_half) * g_automap_zoom;

    srVector3T<float> from;
    srVector3T<float> to;
    from.y = g_automap_top_y + g_world_scale;
    to.y = g_automap_grid_min.y;
    from.x = position.x;
    from.z = position.z;
    to.x = position.x;
    to.z = position.z;

    float ground;
    if (g_octree_game_data->octree->TraceLineOfSight(&from, &to, true, -3, -3, true, 0) == 0) {
        ground = g_automap_grid_min.y;
        if (g_automap_layers.GetCount() != 0 && 1 < g_automap_layers.GetCount() &&
            *g_automap_layers.GetAt(1) != 0) {
            ground = static_cast<float>((*g_automap_layers.GetAt(1))->getLocationY());
        }
    } else {
        ground = to.y;
    }

    float height = (g_automap_top_y - ground) * g_float_half;
    float minimum = ground + g_float_twenty_five_thousand;
    if (g_automap_zoom_mode != 0 || g_automap_top_y <= minimum || height < minimum) {
        SetAutomapButtonMode(2);
        height = minimum;
    } else {
        SetAutomapButtonMode(1);
    }
    g_automap_zoom = height - ground;
    position.y = height;
    SetAutomapCameraPoint(&position);
    int tool = g_automap_tool;
    if (g_automap_tool == 0 && g_automap_cursor_inside) {
        tool = g_float_twenty_five_thousand < g_automap_zoom ? 1 : 4;
    }
    SetMouseCursorFromVideoObject(GetCatalogVideoObjectHandle(tool + 0x14b, 0),
                                  GetCatalogVideoObjectYOffset(tool + 0x14b),
                                  static_cast<short>(g_automap_cursor_offsets[tool].x),
                                  static_cast<short>(g_automap_cursor_offsets[tool].y));
    gXStatus.iCurrentCursor = W8_CURSOR_INVALID_TARGET;
    RefreshMouseCursorTexture();
    return 1;
}

/* Zero every world mesh's vertex-light table and flag it dirty, then clear
   the cell-processed bitmap so UpdateAutomapBounds rebuilds the lighting for
   every visited cell. */
// FUNCTION: WIZ8 0x00580270
void ResetAutomapLighting(void)
{
    if (g_world->octree != 0) {
        for (unsigned int mesh = 0; mesh < g_world->octree->m_meshCount; ++mesh) {
            srModelInstance* instance = g_world->psrMeshes[mesh];
            if (instance != 0) {
                for (stMeshModel* model = static_cast<stMeshModel*>(instance->getModel());
                     model != 0; model = model->next) {
                    model->ClearVertexLights();
                }
            }
        }
    } else {
        for (stMeshModel* model =
                 static_cast<stMeshModel*>(g_world->update_mesh_source->getModel());
             model != 0; model = model->next) {
            model->ClearVertexLights();
        }
    }
    g_automap_lit_cells->ClearAll();
}

static srVector3T<float> GetAutomapCellPosition(unsigned int index)
{
    srVector3T<float> cell;
    cell.SetZero();
    if (g_automap_cell_keys != 0 || index < static_cast<unsigned int>(g_automap_cell_count)) {
        unsigned int key = g_automap_cell_keys[index];
        float half = g_automap_grid_cell_size * g_float_half;
        cell.Set((key >> 0x15) * g_automap_grid_cell_size + half,
                 (key & 0x3ff) * g_automap_grid_cell_size + half,
                 ((key >> 10) & 0x7ff) * g_automap_grid_cell_size + half);
    }
    return cell + g_automap_grid_origin;
}

/* Light up to `max_count` visited cells that have not yet been processed into
   vertex lights. Returns how many cells were lit; a zero answer means the
   pending set is empty. */
// FUNCTION: WIZ8 0x005807B0
unsigned int LightPendingAutomapCells(unsigned int max_count)
{
    unsigned int lit = 0;
    unsigned int bit = 0;
    if (g_automap_cell_count != 0) {
        do {
            if (max_count <= lit) {
                return lit;
            }
            if (0x20 < g_automap_cell_count) {
                while (bit < static_cast<unsigned int>(g_automap_cell_count - 0x20) &&
                       g_automap_visited_cells->puiIndex[bit >> 5] == 0) {
                    bit += 0x20;
                }
            }
            if (g_automap_visited_cells->Test(bit)) {
                srVector3T<float> position = GetAutomapCellPosition(bit);
                if (!g_automap_lit_cells->Test(bit)) {
                    g_automap_lit_cells->Set(bit);
                    LightAutomapCell(&position);
                    ++lit;
                }
            }
            ++bit;
        } while (bit < static_cast<unsigned int>(g_automap_cell_count));
    }
    return lit;
}

// FUNCTION: WIZ8 0x00580760
void RefreshDirtyAutomap(void)
{
    if (g_automap_state != 0 && g_automap_state->pending_cell_lighting != 0) {
        SetWorldMeshVertexLightTable(g_world, 1);
        unsigned int lit = LightPendingAutomapCells(10);
        if (lit == 0) {
            g_automap_state->pending_cell_lighting = 0;
        }
        SetWorldMeshVertexLightTable(g_world, 0);
    }
}

/* Recompute the explored-bounds box from the visited-cell bitmap: mark new
   cells for the node builder and grow the box to cover each visited cell. */
// FUNCTION: WIZ8 0x00580380
void UpdateAutomapBounds(void)
{
    W8Vector<stModelInstance*> models(5);
    PartyHasCondition(0x40);
    if (g_world->octree != 0) {
        if (g_automap_bounds_dirty) {
            ResetAutomapLighting();
            g_automap_bounds_dirty = false;
        }
        g_automap_bounds_min.y = g_automap_grid_min.y;
        g_automap_bounds_max.y = g_automap_grid_max.y;
        g_automap_bounds_min.x = 1e+09f;
        g_automap_bounds_min.z = 1e+09f;
        g_automap_bounds_max.x = -1e+09f;
        g_automap_bounds_max.z = -1e+09f;
        unsigned int bit = 0;
        if (g_automap_cell_count != 0) {
            do {
                if (0x20 < g_automap_cell_count) {
                    while (bit < static_cast<unsigned int>(g_automap_cell_count - 0x20) &&
                           g_automap_visited_cells->puiIndex[bit >> 5] == 0) {
                        bit += 0x20;
                    }
                }
                if (g_automap_visited_cells->Test(bit)) {
                    srVector3T<float> position = GetAutomapCellPosition(bit);
                    if (position.x <= g_automap_bounds_min.x) {
                        g_automap_bounds_min.x = position.x;
                    }
                    if (position.z <= g_automap_bounds_min.z) {
                        g_automap_bounds_min.z = position.z;
                    }
                    if (g_automap_bounds_max.x <= position.x) {
                        g_automap_bounds_max.x = position.x;
                    }
                    if (g_automap_bounds_max.z <= position.z) {
                        g_automap_bounds_max.z = position.z;
                    }
                    if (!g_automap_lit_cells->Test(bit)) {
                        g_automap_lit_cells->Set(bit);
                        LightAutomapCell(&position);
                    }
                }
                ++bit;
            } while (bit < static_cast<unsigned int>(g_automap_cell_count));
        }
        g_automap_bounds_min.x -= g_float_five_thousand;
        g_automap_bounds_max.x += g_float_five_thousand;
        g_automap_bounds_min.z -= g_float_five_thousand;
        g_automap_bounds_max.z += g_float_five_thousand;
        float span = g_automap_bounds_max.x - g_automap_bounds_min.x;
        if (span <= g_automap_bounds_max.z - g_automap_bounds_min.z) {
            span = g_automap_bounds_max.z - g_automap_bounds_min.z;
        }
        g_automap_top_y = span + g_automap_bounds_max.y;
    }
}

/* Reveal one visited cell: collect the model instances the octree reports
   near the cell position, then for every linked mesh either flood its
   vertex-light table with 1.0 when the whole bounding box is inside the query
   range, or compute per-vertex distances and fill only the in-range runs. */
// FUNCTION: WIZ8 0x005809F0
void LightAutomapCell(const srVector3T<float>* position)
{
    W8Vector<stModelInstance*> instances(5);
    srArray<float> distances;
    srArray<srVector3T<float> > vertices;
    float range = g_automap_range;

    if (g_world->octree->CollectModelsNearPoint(&instances, position, range, 0, false) != 0) {
        for (int index = 0; index < instances.GetCount(); ++index) {
            stModelInstance* instance = *instances.GetAt(index);
            if (instance == 0) {
                continue;
            }
            stMeshModel* model = static_cast<stMeshModel*>(instance->getModel());
            srVector3T<float> location;
            location = instance->getLocation();
            srVector3T<float> minimum;
            srVector3T<float> maximum;
            model->getBoundingBox(minimum, maximum);
            minimum += location;
            maximum += location;
            bool inside =
                (minimum - *position).Length() <= range && (maximum - *position).Length() <= range;
            while (model != 0) {
                srVector3T<float>* lights = model->GetVertexLights(true, -1);
                srVector3T<float>* source = model->getVertexLoc();
                int count = model->vertex_location_count;
                if (inside) {
                    float light_value = 1.0f;
                    // reinterpret-ok: vertex-light floats filled via dword fill.
                    srVectorProcessor::copy(reinterpret_cast<SRDWORD*>(lights),
                                            // reinterpret-ok: fill pattern read as dword.
                                            reinterpret_cast<SRDWORD&>(light_value),
                                            static_cast<SRDWORD>(count) * 3);
                } else {
                    if (static_cast<int>(distances.capacity) < count) {
                        distances.setCapacity(count);
                    }
                    if (static_cast<int>(vertices.capacity) < count) {
                        vertices.setCapacity(count);
                    }
                    srVector3T<float>* transformed = &vertices[0];
                    if (IsZeroVector(position) != 0) {
                        CopyDwordBuffer(transformed, source, count * 3);
                    } else {
                        srVector3T<float> offset = -*position;
                        OffsetVertices(transformed, source, &offset, count);
                    }
                    if (count != 0) {
                        srVectorProcessor::length(&distances[0], &vertices[0],
                                                  static_cast<SRDWORD>(count));
                    }
                    float* distance = &distances[0];
                    if (count != 0) {
                        srVector3T<float>* light = lights;
                        unsigned int vertex = 0;
                        while (vertex < static_cast<unsigned int>(count)) {
                            unsigned int lit = vertex;
                            while (lit < static_cast<unsigned int>(count) &&
                                   *distance <= g_automap_range) {
                                ++lit;
                                ++distance;
                            }
                            unsigned int lit_count = lit - vertex;
                            if (lit_count != 0) {
                                float light_value = 1.0f;
                                // reinterpret-ok: vertex-light floats filled via
                                // dword fill.
                                srVectorProcessor::copy(reinterpret_cast<SRDWORD*>(light),
                                                        // reinterpret-ok: fill pattern as dword.
                                                        reinterpret_cast<SRDWORD&>(light_value),
                                                        lit_count * 3);
                                light += lit_count;
                                vertex = lit;
                            }
                            unsigned int dark = vertex;
                            while (dark < static_cast<unsigned int>(count) &&
                                   *distance > g_automap_range) {
                                ++dark;
                                ++distance;
                            }
                            if (dark - vertex != 0) {
                                light += dark - vertex;
                                vertex = dark;
                            }
                        }
                    }
                }
                model = model->next;
            }
        }
    }
}

/* Per-frame automap refresh: re-render the world into the automap surface
   when it is dirty, blit it into the primary surface and redraw buttons. */
// FUNCTION: WIZ8 0x00581030
void RenderAutomapFrame(void)
{
    if (g_automap_redraw) {
        if (!g_automap_surface_mode) {
            RenderAutomapMarkers();
        } else {
            if (g_automap_overlay_redraw) {
                SetResidentTexturePolicy(3);
                float half = g_automap_zoom * g_float_half;
                srCamera::Rect view;
                view.left = static_cast<double>(-half);
                view.bottom = static_cast<double>(-half);
                view.right = static_cast<double>(half);
                view.top = static_cast<double>(half);
                g_world->camera->setViewPlane(view, static_cast<double>(g_automap_near_clip));
                g_world->camera->setClipRange(g_automap_near_clip, 1500000.0);
                RenderWorldToSurface(g_automap_surface, &g_automap_viewport, false);
                g_automap_overlay_redraw = false;
                SetResidentTexturePolicy(0);
            }
            unsigned int pitch;
            void* pixels = LockPrimarySurface(&pitch);
            srColorSurface* surface = SR_NEW(W8ColorSurface)(srPixelConvert::SURFACE_ARGB1555,
                                                             pixels, 0x280, 0x1e0, pitch);
            surface->setFilter(&srBoxFilter);
            surface->blit(0xc, 0x20, *g_automap_surface, 0xc, 0x20, 0x1d3, 0x1d3);
            surface->release();
            UnlockPrimarySurface();
            RenderAutomapMarkers();
            InvalidateRegion(0xc, 0x20, 0x1d3, 0x1d3, 0);
            SetRendererAutoFlipEnabled(false);
            RenderFrame();
            SetRendererAutoFlipEnabled(true);
        }
        g_automap_redraw = false;
    }
    for (int button = 0; button < 16; ++button) {
        if (g_automap_buttons[button] != 0) {
            g_automap_buttons[button]->Draw();
        }
    }
    RenderFrame();
}

// FUNCTION: WIZ8 0x00581200
void UpdateAutomapPageButtons(void)
{
    g_automap_buttons[5]->SetVisible(g_automap_page == 0);
    g_automap_buttons[6]->SetVisible(g_automap_page == 1);
    g_automap_buttons[7]->SetVisible(g_automap_page == 2);
    g_automap_buttons[g_automap_page + 5]->m_dirty = true;
    g_automap_buttons[g_automap_page + 5]->Draw();
}

/* Blit the saved automap surface back over a screen rect to erase whatever
   was drawn there. */
// FUNCTION: WIZ8 0x00581280
int RestoreAutomapRect(W8ScreenRect* rect)
{
    if (g_automap_surface != 0) {
        unsigned int pitch;
        void* pixels = LockPrimarySurface(&pitch);
        srColorSurface* surface =
            SR_NEW(W8ColorSurface)(srPixelConvert::SURFACE_ARGB1555, pixels, 0x280, 0x1e0, pitch);
        surface->setFilter(&srBoxFilter);
        surface->blit(rect->left, rect->top, *g_automap_surface, rect->left, rect->top, rect->right,
                      rect->bottom);
        surface->release();
        UnlockPrimarySurface();
    }
    return 1;
}

/* When the cursor leaves a hovered note, erase its tooltip rectangle and
   invalidate it for redraw. Returns nonzero when a tooltip was dismissed. */
// FUNCTION: WIZ8 0x00581460
unsigned char ShowAutomapNoteTooltip(W8AutomapNote* note)
{
    if (note == 0) {
        return 0;
    }
    float floor_y;
    int layer = note->layer + 1;
    if (layer < 0 || g_automap_layers.GetCount() == 0 || g_automap_layers.GetCount() <= layer ||
        *g_automap_layers.GetAt(layer) == 0) {
        floor_y = g_automap_grid_min.y;
    } else if (layer < g_automap_layers.GetCount()) {
        floor_y = static_cast<float>(g_automap_layers.data[layer]->getLocationY());
    } else {
        floor_y = static_cast<float>((*g_automap_layers.data)->getLocationY());
    }
    int index = 0;
    if (0 < g_automap_layers.GetCount()) {
        do {
            if (index >= 0 && g_automap_layers.GetCount() != 0 &&
                index < g_automap_layers.GetCount() && *g_automap_layers.GetAt(index) != 0) {
                if ((*g_automap_layers.GetAt(index))->getLocation().y < floor_y) {
                    break;
                }
            }
            ++index;
        } while (index < g_automap_layers.GetCount());
    }
    if (index - 1 == g_automap_layer) {
        float left = g_automap_position.x - g_automap_zoom * g_float_half;
        float bottom = g_automap_position.z - g_automap_zoom * g_float_half;
        if (left <= note->position.x && note->position.x <= left + g_automap_zoom &&
            bottom <= note->position.y && note->position.y <= bottom + g_automap_zoom) {
            int screen_x = static_cast<int>((note->position.x - left) / g_automap_zoom * -455.0f);
            int screen_y =
                static_cast<int>((1.0f - (note->position.y - bottom) / g_automap_zoom) * -435.0f);
            int marker_width;
            if (note == g_automap_editing_note) {
                marker_width = static_cast<int>(g_automap_text_marker->GetScaledWidth() * 0.22);
            } else {
                marker_width = static_cast<int>(
                    (1.0f / (g_automap_zoom * 0.00004f)) * 0.44f *
                    static_cast<double>(g_automap_text_marker->GetScaledWidth()) * 0.5f);
            }
            g_automap_text_marker->GetScaledHeight();
            W8ScreenRect rect;
            rect.top =
                (0x20 - screen_y) - ((unsigned)GetFontHeight(g_wiz_text_font_secondary) >> 1);
            rect.left = marker_width / 2 + (0xe - screen_x);
            rect.bottom = rect.top + GetFontHeight(g_wiz_text_font_secondary) + 3;
            rect.right = StringPixLength(note->text, g_wiz_text_font_secondary) + 3 + rect.left;
            if (rect.left < 0xc) {
                rect.left = 0xc;
            }
            if (rect.top < 0x20) {
                rect.top = 0x20;
            }
            if (0x1d3 < rect.right) {
                rect.right = 0x1d3;
            }
            if (0x1d3 < rect.bottom) {
                rect.bottom = 0x1d3;
            }
            if (rect.right != rect.left && -1 < rect.right - rect.left && rect.bottom != rect.top &&
                -1 < rect.bottom - rect.top) {
                RestoreAutomapRect(&rect);
                InvalidateScreenRects(&rect, 1, 0);
                return 1;
            }
        }
    }
    return 0;
}

/* Persist the visited-cell bitmap and every note's position, layer and text.
   A 0xF00DF00D signature brackets the note records. */
// FUNCTION: WIZ8 0x00581CE0
bool SaveAutomapNotes(int handle)
{
    int signature = 0xf00df00d;
    unsigned char saved;
    if (g_automap_visited_cells != 0) {
        saved = g_automap_visited_cells->Save(handle);
    } else {
        saved = static_cast<unsigned char>(handle);
    }
    if (saved == 0) {
        return false;
    }
    if (FileWrite(handle, &signature, 4, 0) == 0) {
        return false;
    }
    unsigned int count = g_automap_notes->GetCount();
    if (FileWrite(handle, &count, 4, 0) == 0) {
        return false;
    }
    for (unsigned int index = 0; index < count; ++index) {
        W8AutomapNote* note = *g_automap_notes->GetAt(index);
        if (note != 0) {
            bool ok = FileWrite(handle, &note->position.x, 4, 0) != 0 &&
                      FileWrite(handle, &note->position.y, 4, 0) != 0 &&
                      FileWrite(handle, &note->layer, 4, 0) != 0;
            int length = wcslen(note->text) + 1;
            if (!ok) {
                return false;
            }
            if (FileWrite(handle, &length, 4, 0) == 0) {
                return false;
            }
            if (FileWrite(handle, note->text, length * 2, 0) == 0) {
                return false;
            }
        }
    }
    return FileWrite(handle, &signature, 4, 0) != 0;
}

/* Release every note, restore the visited-cell bitmap for the loaded level,
   then rebuild the notes from the saved records. */
// FUNCTION: WIZ8 0x00581E60
bool LoadAutomapNotes(int handle)
{
    int signature = 0;
    ClearAutomapNotes();
    g_automap_redraw = true;
    if (g_automap_visited_cells != 0 && 1 < g_automap_visited_cells->bit_count) {
        g_automap_visited_cells->Load(handle);
        if (g_automap_visited_cells->bit_count == static_cast<unsigned int>(g_automap_cell_count)) {
            g_automap_state->pending_cell_lighting = 1;
        } else {
            g_automap_visited_cells->SetSize(g_automap_cell_count);
        }
        g_automap_bounds_dirty = true;
        unsigned int count;
        if (FileRead(handle, &signature, 4, 0) != 0 && signature == static_cast<int>(0xf00df00d) &&
            FileRead(handle, &count, 4, 0) != 0) {
            for (unsigned int index = 0; index < count; ++index) {
                srVector2T<float> position;
                int layer = 0;
                int length;
                /* Retail fed `length` to malloc even when the FileRead chain
                   short-circuited before filling it; the recovery keeps that read. */
                bool ok = FileRead(handle, &position.x, 4, 0) != 0 &&
                          FileRead(handle, &position.y, 4, 0) != 0 &&
                          FileRead(handle, &layer, 4, 0) != 0 &&
                          FileRead(handle, &length, 4, 0) != 0;
                wchar_t* text = static_cast<wchar_t*>(malloc(length * 2));
                if (!ok) {
                    return false;
                }
                if (FileRead(handle, text, length * 2, 0) == 0) {
                    return false;
                }
                CreateAutomapNote(&position, layer, text);
                free(text);
            }
            return FileRead(handle, &signature, 4, 0) != 0;
        }
    }
    return false;
}

/* Convert the cursor position to an automap world position on the layer
   below the current one, defaulting the height to the grid minimum when no
   clip plane exists there. */
// FUNCTION: WIZ8 0x00582050
unsigned char GetAutomapPositionUnderCursor(srVector3T<float>* position)
{
    srVector3T<float> point;

    if (GetCursorPositionInViewport(&point) != 0) {
        int layer = g_automap_layer + 1;
        float height;
        if (layer >= 0 && g_automap_layers.GetCount() != 0 && layer < g_automap_layers.GetCount() &&
            g_automap_layers.data[layer] != 0) {
            height = static_cast<float>((*g_automap_layers.GetAt(layer))->getLocationY());
        } else {
            height = g_automap_grid_min.y;
        }
        position->y = height;
        position->x = (point.x - g_float_half) * g_automap_zoom + g_automap_position.x;
        position->z = g_automap_position.z - (point.y - g_float_half) * g_automap_zoom;
        return 1;
    }
    return 0;
}

/* Find the note on the current layer nearest the cursor, limited to a
   zoom-scaled pick radius. */
// FUNCTION: WIZ8 0x00582180
W8AutomapNote* FindAutomapNoteUnderCursor(void)
{
    W8AutomapNote* result = 0;
    double best = 99999999999.9;
    srVector3T<float> point;

    if (g_automap_page != 2 && GetCursorPositionInViewport(&point) != 0) {
        int layer = g_automap_layer + 1;
        if (layer >= 0 && g_automap_layers.GetCount() != 0 && layer < g_automap_layers.GetCount() &&
            *g_automap_layers.GetAt(layer) != 0) {
            (*g_automap_layers.GetAt(layer))->getLocationY();
        }
        float x = (point.x - g_float_half) * g_automap_zoom + g_automap_position.x;
        float y = g_automap_position.z - (point.y - g_float_half) * g_automap_zoom;
        for (unsigned int index = 0; index < static_cast<unsigned int>(g_automap_notes->GetCount());
             ++index) {
            W8AutomapNote* note = *g_automap_notes->GetAt(index);
            if (note->layer == g_automap_layer) {
                float dx = x - note->position.x;
                float dy = y - note->position.y;
                double distance = sqrt(dx * dx + dy * dy);
                if (distance < best && distance < g_automap_zoom * 0.05) {
                    best = distance;
                    result = note;
                }
            }
        }
    }
    return result;
}

/* Descriptive name for the common marker-texture loading operation. */
static stTextureFile* LoadAutomapMarkerTexture(const char* path)
{
    stTextureFile* texture = new stTextureFile(path, 0);
    texture->autoRelease();
    texture->enableHint(srTextureIFace::HINT_ONE_BIT_ALPHA);
    texture->enableHint(srTextureIFace::HINT_NO_MIPMAPS);
    texture->setMipmap(srTextureIFace::MIPMAP_NONE);
    texture->setCorrection(srTextureIFace::CORRECTION_FASTEST);
    texture->setMagFilter(srTextureIFace::FILTER_BEST);
    texture->setMinFilter(srTextureIFace::FILTER_BEST);
    texture->loadSurface();
    return texture;
}

/* Load the automap marker textures and build the party, monster and text
   marker sprites over them. */
// FUNCTION: WIZ8 0x005822C0
void CreateAutomapMarkerSprites(void)
{
    if (g_automap_markers == 0) {
        g_automap_markers = new W8Vector<srClass*>(5);
    }
    if (g_automap_party_marker == 0) {
        stTextureFile* texture = LoadAutomapMarkerTexture("Data\\Automap\\map_partymarker_a.tga");
        srColorSurface* surface = texture->getSurface();
        if (surface != 0) {
            g_automap_party_marker =
                CreateSpriteFromTexture(texture, surface->getWidth() * g_viewport_x_scale,
                                        surface->getHeight() * g_viewport_x_scale, true, true);
            g_automap_party_marker->setParent(g_scene_square, 1);
            static_cast<srMeshModel*>(g_automap_party_marker->getModel())
                ->enable(srMeshModel::CONTROL_SORTED_RENDERING);
            surface->setFilter(&srBSplineFilter);
            g_automap_party_marker->SetGlowEnabled(true);
            srVector4T<float> first;
            srVector4T<float> second;
            first.Set(0.0f, 0.25f, 0.0f, 1.0f);
            second.Set(0.0f, 0.75f, 0.0f, 1.0f);
            g_automap_party_marker->SetGlowColors(&first, &second);
            g_automap_party_marker->render_state.render_depth = 2000;
        }
    }
    if (g_automap_friendly_marker == 0) {
        stTextureFile* texture =
            LoadAutomapMarkerTexture("Data\\Automap\\map_monsterfriendly_a.tga");
        srColorSurface* surface = texture->getSurface();
        if (surface != 0) {
            g_automap_friendly_marker =
                CreateSpriteFromTexture(texture, surface->getWidth() * g_viewport_x_scale,
                                        surface->getHeight() * g_viewport_y_scale, true, false);
            static_cast<srMeshModel*>(g_automap_friendly_marker->getModel())
                ->enable(srMeshModel::CONTROL_SORTED_RENDERING);
            surface->setFilter(&srBSplineFilter);
        }
    }
    if (g_automap_neutral_marker == 0) {
        stTextureFile* texture =
            LoadAutomapMarkerTexture("Data\\Automap\\map_monsterneutral_a.tga");
        srColorSurface* surface = texture->getSurface();
        if (surface != 0) {
            g_automap_neutral_marker =
                CreateSpriteFromTexture(texture, surface->getWidth() * g_viewport_x_scale,
                                        surface->getHeight() * g_viewport_y_scale, true, false);
            static_cast<srMeshModel*>(g_automap_neutral_marker->getModel())
                ->enable(srMeshModel::CONTROL_SORTED_RENDERING);
            surface->setFilter(&srBSplineFilter);
        }
    }
    if (g_automap_hostile_marker == 0) {
        stTextureFile* texture =
            LoadAutomapMarkerTexture("Data\\Automap\\map_monsterhostile_a.tga");
        srColorSurface* surface = texture->getSurface();
        if (surface != 0) {
            g_automap_hostile_marker =
                CreateSpriteFromTexture(texture, surface->getWidth() * g_viewport_x_scale,
                                        surface->getHeight() * g_viewport_y_scale, true, false);
            static_cast<srMeshModel*>(g_automap_hostile_marker->getModel())
                ->enable(srMeshModel::CONTROL_SORTED_RENDERING);
            surface->setFilter(&srBSplineFilter);
        }
    }
    if (g_automap_text_marker == 0) {
        stTextureFile* texture = LoadAutomapMarkerTexture("Data\\Automap\\map_textmarker_a.tga");
        srColorSurface* surface = texture->getSurface();
        if (surface != 0) {
            g_automap_text_marker =
                CreateSpriteFromTexture(texture, surface->getWidth() * g_viewport_x_scale,
                                        surface->getHeight() * g_viewport_y_scale, true, false);
            static_cast<srMeshModel*>(g_automap_text_marker->getModel())
                ->enable(srMeshModel::CONTROL_SORTED_RENDERING);
            surface->setFilter(&srBSplineFilter);
        }
    }
}

/* Redraw the transient party, monster, item and note markers over the automap
   surface. */
// FUNCTION: WIZ8 0x00582930
void RenderAutomapMarkers(void)
{
    bool detect_all = PartyHasCondition(0x40);
    while (g_automap_markers->GetCount()) {
        srClass* object = *g_automap_markers->GetAt(0);
        object->release();
        g_automap_markers->Remove(object);
    }
    float left = g_automap_position.x - g_automap_zoom * g_float_half;
    srVector3T<float> point(g_automap_saved_camera.position.x, 1.0f,
                            g_automap_saved_camera.position.z);
    float top = g_automap_position.z - g_automap_zoom * g_float_half;
    if (point.x < left || left + g_automap_zoom < point.x || point.z < top ||
        top + g_automap_zoom < point.z) {
        g_automap_party_marker->setFlag(srNode::FLAG_DISABLE);
    } else {
        int x = 0xc - static_cast<int>((point.x - left) / g_automap_zoom * -455.0f);
        int y = 0x20 - static_cast<int>((1.0f - (point.z - top) / g_automap_zoom) * -435.0f);
        float factor = (g_float_one / (g_automap_zoom * 0.00004f)) * 0.44f;
        srVector3T<double> scale(factor, factor, factor);
        g_automap_party_marker->setScale(scale);
        if (g_automap_party_marker->GetScaledHeight() < 1) {
            g_automap_party_marker->setScale(srVector3T<double>(1.0, 1.0, 1.0));
            factor = g_float_one / g_automap_party_marker->GetScaledHeight();
            scale.Set(factor, factor, factor);
            g_automap_party_marker->setScale(scale);
        }
        unsigned short width = g_automap_party_marker->GetScaledWidth();
        unsigned short height = g_automap_party_marker->GetScaledHeight();
        PositionToolTipNode(g_automap_party_marker, x - (width >> 1), y - (height >> 1), false);
        g_automap_party_marker->setRotation(0.0, 0.0, -static_cast<double>(GetCameraYawRadians()));
        g_automap_party_marker->clearFlag(srNode::FLAG_DISABLE);
        g_automap_party_marker->setParent(0, 1);
        g_automap_party_marker->setParent(g_scene_square, 1);
    }
    unsigned int count = PLLength(gXStatus.plsMonsterList);
    for (unsigned int index = 0; index < count; ++index) {
        W8MonsterInfo* info = GetMonsterListEntry(index);
        W8Monster* monster = info->p3D;
        srVector3T<float> location;
        location = 0.0f;
        monster->m_pRep->GetLocation(&location);
        for (int layer = 0; layer < g_automap_layers.GetCount(); ++layer) {
            if (layer >= 0 && g_automap_layers.GetCount() != 0 &&
                layer < g_automap_layers.GetCount() && *g_automap_layers.GetAt(layer) != 0 &&
                (*g_automap_layers.GetAt(layer))->getLocation().y < location.y) {
                break;
            }
        }
        if (g_automap_show_all_monsters || detect_all ||
            (!monster->disabled && info->party_threat.sight_state == W8_SIGHT_SEEN)) {
            left = g_automap_position.x - g_automap_zoom * g_float_half;
            top = g_automap_position.z - g_automap_zoom * g_float_half;
            if (location.x < left || left + g_automap_zoom < location.x || location.z < top ||
                top + g_automap_zoom < location.z) {
                continue;
            }
            int x = 0xc - static_cast<int>((location.x - left) / g_automap_zoom * -455.0f);
            int y = 0x20 - static_cast<int>((1.0f - (location.z - top) / g_automap_zoom) * -435.0f);
            stModelInstance2D* marker;
            switch (info->ubDisposition) {
            case W8_DISPOSITION_NEUTRAL:
                marker = CreateAutomapMonsterMarker(1);
                break;
            case W8_DISPOSITION_HOSTILE:
                marker = CreateAutomapMonsterMarker(2);
                break;
            case W8_DISPOSITION_FRIENDLY:
                marker = CreateAutomapMonsterMarker(0);
                break;
            }
            unsigned short width = marker->GetScaledWidth();
            x -= width >> 1;
            unsigned short height = marker->GetScaledHeight();
            y -= height >> 1;
            marker->setParent(g_scene_fullscreen, 1);
            Position2DNodeUnsnapped(marker, x, y);
        }
    }
    for (W8WorldItem* world_item = GetNextWorldItem(true); world_item != 0;
         world_item = GetNextWorldItem(false)) {
        W8Item* item = world_item->p3D;
        if (item == 0) {
            continue;
        }
        srVector3T<float> location;
        item->GetSearchPosition(&location);
        int layer = 0;
        for (; layer < g_automap_layers.GetCount(); ++layer) {
            if (layer >= 0 && g_automap_layers.GetCount() != 0 &&
                layer < g_automap_layers.GetCount() && *g_automap_layers.GetAt(layer) != 0 &&
                (*g_automap_layers.GetAt(layer))->getLocation().y < location.y) {
                break;
            }
        }
        if ((static_cast<W8ItemRep*>(item->m_pRep)->flags & W8_ITEM_ENTITY_NO_PICKUP) == 0 &&
            (detect_all ||
             (static_cast<W8ItemRep*>(item->m_pRep)->flags & W8_ITEM_ENTITY_RADAR_SEEN) != 0 ||
             HasCameraLineOfSight(&location)) &&
            layer - 1 == g_automap_layer) {
            left = g_automap_position.x - g_automap_zoom * g_float_half;
            top = g_automap_position.z - g_automap_zoom * g_float_half;
            if (location.x < left || left + g_automap_zoom < location.x || location.z < top ||
                top + g_automap_zoom < location.z) {
                continue;
            }
            int x = 0xc - static_cast<int>((location.x - left) / g_automap_zoom * -455.0f);
            int y = 0x20 - static_cast<int>((1.0f - (location.z - top) / g_automap_zoom) * -435.0f);
            stModelInstance2D* marker = CreateAutomapItemMarker(world_item->item.iItemNo);
            if (marker != 0) {
                unsigned short width = marker->GetScaledWidth();
                unsigned short height = marker->GetScaledHeight();
                PositionToolTipNode(marker, x - (width >> 1), y - (height >> 1), false);
                marker->setParent(g_scene_fullscreen, 1);
            }
        }
    }
    if (g_automap_page != 2) {
        SGPRect saved_clip;
        GetClippingRect(&saved_clip);
        SGPRect clip = {0xc, 0x20, 0x1d3, 0x1d3};
        SetClippingRect(&clip);
        SetFont(g_wiz_text_font_secondary);
        SetObjectShade(g_wiz_text_font_secondary_object, 4);
        SetFontDestClip(0xc, 0x20, 0x1d3, 0x1d3);
        int font_height = GetFontHeight(g_wiz_text_font_secondary);
        unsigned int count = g_automap_notes->GetCount();
        for (unsigned int index = 0; index < count; ++index) {
            W8AutomapNote* note = *g_automap_notes->GetAt(index);
            int layer = note->layer + 1;
            if (layer >= 0 && g_automap_layers.GetCount() != 0 &&
                layer < g_automap_layers.GetCount() && *g_automap_layers.GetAt(layer) != 0) {
                (*g_automap_layers.GetAt(layer))->getLocationY();
            }
            left = g_automap_position.x - g_automap_zoom * g_float_half;
            top = g_automap_position.z - g_automap_zoom * g_float_half;
            if (note->position.x < left || left + g_automap_zoom < note->position.x ||
                note->position.y < top || top + g_automap_zoom < note->position.y) {
                continue;
            }
            int x = 0xc - static_cast<int>((note->position.x - left) / g_automap_zoom * -455.0f);
            int y = 0x20 -
                    static_cast<int>((1.0f - (note->position.y - top) / g_automap_zoom) * -435.0f);
            stModelInstance2D* marker = CreateAutomapTextMarker();
            if (note == g_automap_editing_note)
                marker->setScale(srVector3T<double>(0.22f, 0.22f, 0.22f));
            if (marker != 0) {
                unsigned short width = marker->GetScaledWidth();
                x -= width >> 1;
                unsigned short height = marker->GetScaledHeight();
                y -= height >> 1;
                Position2DNodeUnsnapped(marker, x, y);
                marker->setParent(g_scene_fullscreen, 1);
                if (note->layer == g_automap_layer && note->text != 0 &&
                    (g_automap_page == 0 || note == g_automap_hovered_note ||
                     note == g_automap_editing_note)) {
                    height = marker->GetScaledHeight();
                    y += (height >> 1) - ((font_height & 0xffff) >> 1);
                    width = marker->GetScaledWidth();
                    x += width + 2;
                    gprintfDirty(x, y, note->text);
                    if (note == g_automap_editing_note) {
                        marker->setScale(srVector3T<double>(0.22f, 0.22f, 0.22f));
                        unsigned int pitch;
                        char* buffer = static_cast<char*>(LockPrimarySurface(&pitch));
                        SetClippingRegionAndImageWidth(pitch, 0xc, 0x20, 0x1c7, 0x1b3);
                        int color = Get16BPPColor(0x569bef);
                        int length = StringPixLength(note->text, g_wiz_text_font_secondary);
                        RectangleDraw(TRUE, x - 1, y, x + length + 2, y + (font_height & 0xffff),
                                      color, buffer);
                        UnlockPrimarySurface();
                    }
                }
            }
        }
        SetClippingRect(&saved_clip);
    }
}

/* Create a scaled automap blip for a world item by blitting its inventory icon
   into a scratch surface and wrapping it in a 2D brush. */
// FUNCTION: WIZ8 0x005833E0
stModelInstance2D* CreateAutomapItemMarker(int item_id)
{
    int object = g_item_video_objects.GetOrCreateVideoObject(item_id);
    ETRLEObject properties;
    unsigned short region = GetCatalogVideoObjectYOffset(object);
    unsigned int handle = GetCatalogVideoObjectHandle(object, 0);
    if (!GetVideoObjectETRLEPropertiesFromIndex(handle, &properties, region)) {
        return 0;
    }
    srColorSurface* surface = SR_NEW(srColorSurface)(srPixelConvert::SURFACE_ARGB1555, 0x40, 0x40);
    surface->autoRelease();
    surface->fill(0);
    surface->setFilter(&srBSplineFilter);
    int y = (0x40 - properties.usHeight) / 2;
    int x = (0x40 - properties.usWidth) / 2;
    BlitVideoObjectToColorSurface(GetCatalogVideoObjectHandle(object, 0),
                                  GetCatalogVideoObjectYOffset(object), surface, x, y);
    float factor = (g_float_one / (g_automap_zoom * 0.00004f)) * 0.44f;
    stModelInstance2D* marker = static_cast<stModelInstance2D*>(
        MakePolygonBrush(g_scene_fullscreen, surface, surface->getWidth() * g_viewport_x_scale,
                         surface->getHeight() * g_viewport_y_scale, 0.0f, 0.0f, 1.0f, 1.0f, 1));
    if (marker == 0) {
        return 0;
    }
    static_cast<srMeshModel*>(marker->getModel())->enable(srMeshModel::CONTROL_SORTED_RENDERING);
    g_automap_markers->Add(marker);
    marker->SetGlowEnabled(true);
    srVector4T<float> first;
    srVector4T<float> second;
    second.Set(0.0f, 0.0f, 1.0f, 1.0f);
    first.Set(0.0f, 0.0f, 0.25f, 1.0f);
    marker->SetGlowColors(&first, &second);
    marker->setRenderDepth(2000);
    if (marker->GetScaledHeight() * factor < g_float_one) {
        marker->setScale(srVector3T<double>(1.0, 1.0, 1.0));
        factor = g_float_one / marker->GetScaledHeight();
        marker->setScale(srVector3T<double>(factor, factor, factor));
        return marker;
    }
    marker->setScale(srVector3T<double>(factor, factor, factor));
    return marker;
}

/* Clone the friendly, neutral or hostile blip sprite for one monster entry and
   register the clone for release after the pass. */
// FUNCTION: WIZ8 0x00583710
stModelInstance2D* CreateAutomapMonsterMarker(int type)
{
    stModelInstance2D* marker = new stModelInstance2D(0);
    if (marker == 0) {
        return 0;
    }
    switch (type) {
    case 0: {
        srVector4T<float> first;
        srVector4T<float> second;
        *marker = *g_automap_friendly_marker;
        second.Set(0.0f, 0.5f, 0.0f, 1.0f);
        first.Set(0.0f, 0.0f, 0.0f, 1.0f);
        marker->SetGlowColors(&first, &second);
        break;
    }
    case 1: {
        srVector4T<float> first;
        srVector4T<float> second;
        *marker = *g_automap_neutral_marker;
        second.Set(0.25f, 0.25f, 0.0f, 1.0f);
        first.Set(0.0f, 0.0f, 0.0f, 1.0f);
        marker->SetGlowColors(&first, &second);
        break;
    }
    case 2: {
        srVector4T<float> first;
        srVector4T<float> second;
        *marker = *g_automap_hostile_marker;
        second.Set(0.5f, 0.0f, 0.0f, 1.0f);
        first.Set(0.0f, 0.0f, 0.0f, 1.0f);
        marker->SetGlowColors(&first, &second);
        break;
    }
    }
    g_automap_markers->Add(marker);
    marker->SetGlowEnabled(true);
    marker->setRenderDepth(2000);
    /* Retail scales by 0.85 as a step of its own; written as one product, VC6
       folds 0.44 * 0.85 into a single constant. */
    float factor = (g_float_one / (g_automap_zoom * 0.00004f)) * 0.44f;
    factor *= 0.85f;
    if (marker->GetScaledHeight() * factor < g_float_one) {
        marker->setScale(srVector3T<double>(1.0, 1.0, 1.0));
        factor = g_float_one / marker->GetScaledHeight();
        marker->setScale(srVector3T<double>(factor, factor, factor));
    } else {
        marker->setScale(srVector3T<double>(factor, factor, factor));
    }
    return marker;
}

/* Clone the text blip sprite and register the clone for release after the
   pass. */
// FUNCTION: WIZ8 0x005839D0
stModelInstance2D* CreateAutomapTextMarker(void)
{
    stModelInstance2D* marker = new stModelInstance2D(0);
    if (marker == 0) {
        return 0;
    }
    *marker = *g_automap_text_marker;
    g_automap_markers->Add(marker);
    float factor = (g_float_one / (g_automap_zoom * 0.00004f)) * 0.44f * g_float_half;
    if (marker->GetScaledHeight() * factor < g_float_one) {
        marker->setScale(srVector3T<double>(1.0, 1.0, 1.0));
        factor = g_float_one / marker->GetScaledHeight();
        marker->setScale(srVector3T<double>(factor, factor, factor));
    } else {
        marker->setScale(srVector3T<double>(factor, factor, factor));
    }
    marker->SetGlowEnabled(true);
    srVector4T<float> first;
    srVector4T<float> second;
    second.Set(1.0f, 0.4f, 0.0f, 1.0f);
    first.Set(0.0f, 0.0f, 0.0f, 1.0f);
    marker->SetGlowColors(&first, &second);
    marker->setRenderDepth(2000);
    return marker;
}
/* While a note is being edited, forward key events into its text buffer and
   let left-button-up drop the note at the cursor. */
// FUNCTION: WIZ8 0x00584250
unsigned char HandleAutomapNoteInput(const InputAtom* input)
{
    if (input->usEvent != KEY_DOWN && input->usEvent != KEY_REPEAT) {
        if (input->usEvent == LEFT_BUTTON_UP && IsCursorInsideViewport()) {
            srVector3T<float> point;
            if (GetCursorPositionInViewport(&point) != 0) {
                int layer = g_automap_layer + 1;
                if (layer >= 0 && g_automap_layers.GetCount() != 0 &&
                    layer < g_automap_layers.GetCount() && *g_automap_layers.GetAt(layer) != 0) {
                    (*g_automap_layers.GetAt(layer))->getLocationY();
                }
                g_automap_editing_note->position.Set(
                    (point.x - g_float_half) * g_automap_zoom + g_automap_position.x,
                    g_automap_position.z - (point.y - g_float_half) * g_automap_zoom);
                g_automap_redraw = true;
            }
        }
        return 0;
    }
    unsigned short key = TranslateKeyToCharacter(input->usParam, input->usKeyState);
    ShowAutomapNoteTooltip(g_automap_editing_note);
    unsigned int last = wcslen(g_automap_editing_note->text) - 1;
    if (isprint(key) != 0 && last <= 0x26) {
        g_automap_editing_note->text[last] = key;
        g_automap_editing_note->text[last + 1] = L'_';
        g_automap_editing_note->text[last + 2] = 0;
        g_automap_redraw = true;
    } else {
        if (input->usParam == 8) {
            if (last != 0) {
                g_automap_editing_note->text[last - 1] = L'_';
                g_automap_editing_note->text[last] = 0;
            }
            g_automap_redraw = true;
        } else if (input->usParam == 0xd) {
            g_automap_editing_note->text[last] = 0;
            if (wcslen(g_automap_editing_note->text) == 0) {
                g_automap_notes->Remove(g_automap_editing_note);
                free(g_automap_editing_note->text);
                delete g_automap_editing_note;
                if (g_automap_hovered_note == g_automap_editing_note) {
                    g_automap_hovered_note = 0;
                }
                g_automap_editing_note = 0;
                g_automap_redraw = true;
            }
            if (g_automap_tool != 0) {
                g_automap_tool = 0;
                SetMouseCursorFromVideoObject(GetCatalogVideoObjectHandle(0x14b, 0),
                                              GetCatalogVideoObjectYOffset(0x14b),
                                              static_cast<short>(g_automap_cursor_offsets[0].x),
                                              static_cast<short>(g_automap_cursor_offsets[0].y));
                gXStatus.iCurrentCursor = W8_CURSOR_INVALID_TARGET;
                RefreshMouseCursorTexture();
            }
            g_automap_editing_note = 0;
            g_automap_redraw = true;
        } else if (input->usParam == 0x1b) {
            g_automap_notes->Remove(g_automap_editing_note);
            free(g_automap_editing_note->text);
            delete g_automap_editing_note;
            if (g_automap_hovered_note == g_automap_editing_note) {
                g_automap_hovered_note = 0;
            }
            g_automap_editing_note = 0;
            g_automap_redraw = true;
            if (g_automap_tool != 0) {
                g_automap_tool = 0;
                SetMouseCursorFromVideoObject(GetCatalogVideoObjectHandle(0x14b, 0),
                                              GetCatalogVideoObjectYOffset(0x14b),
                                              static_cast<short>(g_automap_cursor_offsets[0].x),
                                              static_cast<short>(g_automap_cursor_offsets[0].y));
                gXStatus.iCurrentCursor = W8_CURSOR_INVALID_TARGET;
                RefreshMouseCursorTexture();
            }
        }
    }
    if (g_automap_editing_note != 0) {
        RenderAutomapMarkers();
        return 1;
    }
    if (g_automap_tool != 0) {
        g_automap_tool = 0;
        SetMouseCursorFromVideoObject(GetCatalogVideoObjectHandle(0x14b, 0),
                                      GetCatalogVideoObjectYOffset(0x14b),
                                      static_cast<short>(g_automap_cursor_offsets[0].x),
                                      static_cast<short>(g_automap_cursor_offsets[0].y));
        gXStatus.iCurrentCursor = W8_CURSOR_INVALID_TARGET;
        RefreshMouseCursorTexture();
    }
    return 1;
}

/* Automap key handling: screen exit, zoom keys, page cycling and the
   per-tool and cheat-gated bindings. */
// FUNCTION: WIZ8 0x00584690
unsigned char HandleAutomapKey(const InputAtom* input)
{
    if (input->usEvent != KEY_DOWN) {
        return 0;
    }
    MGSKeyBinding* binding =
        g_mgs_keyboard->GetBinding(g_mgs_keyboard->FindBinding(W8_MGS_COMMAND_AUTOMAP));
    if (input->usParam == binding->key && input->usKeyState == binding->modifiers) {
        RequestScreenTransition();
        return 1;
    }
    int tool;
    switch (input->usParam) {
    case 8:
        /* Backspace zooms out one step, or all the way from the full view. */
        if (g_automap_position.y < g_automap_top_y) {
            if (g_automap_zoom_mode == 1) {
                g_automap_zoom = g_automap_top_y - g_automap_bounds_min.y;
                srVector3T<float> position = (g_automap_bounds_max + g_automap_bounds_min) / 2.0;
                position.y = g_automap_top_y;
                SetAutomapCameraPoint(&position);
                SetAutomapToolCursor(g_automap_tool);
                SetAutomapButtonMode(0);
                return 1;
            }
            float floor = g_automap_position.y - g_automap_zoom;
            float height = g_automap_top_y - (g_automap_top_y - floor) * 0.5f;
            if (height < g_automap_position.y) {
                ResetAutomapZoom();
                return 1;
            }
            g_automap_position.y = height;
            SetAutomapButtonMode(1);
            g_automap_zoom = g_automap_position.y - floor;
            SetAutomapCameraPoint(&g_automap_position);
            SetAutomapToolCursor(g_automap_tool);
        }
        return 1;
    case 0xd: {
        AutomapZoomInButton();
        return 1;
    }
    case 0x1b:
    drop_tool:
        /* Escape drops the tool, or leaves the screen when none is held. */
        if (g_automap_tool == 0) {
            RequestScreenTransition();
            return 1;
        }
        g_automap_tool = 0;
        tool = 0;
        if (g_automap_cursor_inside) {
            tool = g_float_twenty_five_thousand < g_automap_zoom ? 1 : 4;
        }
        SetMouseCursorFromVideoObject(GetCatalogVideoObjectHandle(tool + 0x14b, 0),
                                      GetCatalogVideoObjectYOffset(tool + 0x14b),
                                      static_cast<short>(g_automap_cursor_offsets[tool].x),
                                      static_cast<short>(g_automap_cursor_offsets[tool].y));
        gXStatus.iCurrentCursor = W8_CURSOR_INVALID_TARGET;
        RefreshMouseCursorTexture();
        return 1;
    case 0x20:
        AutomapCyclePageButton();
        return 1;
    case 0x23:
        RestoreAutomapCameraPosition();
        break;
    case 0x24: {
        g_automap_zoom = g_automap_top_y - g_automap_bounds_min.y;
        {
            srVector3T<float> center(g_automap_bounds_min.x + g_automap_bounds_max.x,
                                     g_automap_bounds_max.y + g_automap_bounds_min.y,
                                     g_automap_bounds_min.z + g_automap_bounds_max.z);
            srVector3T<float> position = center / 2.0;
            position.y = g_automap_top_y;
            SetAutomapCameraPoint(&position);
        }
        SetAutomapToolCursor(g_automap_tool);
        SetAutomapButtonMode(0);
        return 1;
    }
    case 0x2d:
        if (g_automap_tool == 2) {
            return 1;
        }
        g_automap_tool = 2;
        tool = 0x14d;
        goto set_tool_cursor;
    case 0x2e:
        if (g_automap_tool == 3) {
            return 1;
        }
        g_automap_tool = 3;
        tool = 0x14e;
    set_tool_cursor:
        SetMouseCursorFromVideoObject(
            GetCatalogVideoObjectHandle(tool, 0), GetCatalogVideoObjectYOffset(tool),
            static_cast<short>(g_automap_cursor_offsets[g_automap_tool].x),
            static_cast<short>(g_automap_cursor_offsets[g_automap_tool].y));
        gXStatus.iCurrentCursor = W8_CURSOR_INVALID_TARGET;
        RefreshMouseCursorTexture();
        return 1;
    case 0x31:
    case 0x32:
    case 0x33:
    case 0x34:
        SetAutomapLayer(input->usParam - 0x31);
        return 1;
    case 0x41:
        /* Developer: reveal the whole map. */
        if (g_dev_mode) {
            g_automap_visited_cells->SetAll();
            g_automap_bounds_dirty = true;
            g_automap_redraw = true;
            g_automap_overlay_redraw = true;
            UpdateAutomapBounds();
            g_automap_zoom = g_automap_top_y - g_automap_bounds_min.y;
            {
                srVector3T<float> center(g_automap_bounds_min.x + g_automap_bounds_max.x,
                                         g_automap_bounds_max.y + g_automap_bounds_min.y,
                                         g_automap_bounds_min.z + g_automap_bounds_max.z);
                srVector3T<float> position = center / 2.0;
                position.y = g_automap_top_y;
                SetAutomapCameraPoint(&position);
            }
            SetAutomapToolCursor(g_automap_tool);
            SetAutomapButtonMode(0);
            RenderAutomapFrame();
            return 1;
        }
        break;
    case 0x43:
        /* Developer: forget the whole map. */
        if (g_dev_mode) {
            g_automap_visited_cells->ClearAll();
            g_automap_bounds_dirty = true;
            UpdateAutomapBounds();
            return 1;
        }
        break;
    case 0x49:
        if (g_dev_mode) {
            g_automap_developer_i_toggle = !g_automap_developer_i_toggle;
            g_automap_redraw = true;
            return 1;
        }
        break;
    case 0x53:
        if (g_dev_mode) {
            g_automap_redraw = true;
            g_automap_show_all_monsters = !g_automap_show_all_monsters;
            return 1;
        }
        break;
    case 0x70:
        if (g_dev_mode) {
            g_automap_saved_camera.position = g_automap_position;
            ResetCurrentEnvironment();
            ResetCurrentEnvironment();
            goto drop_tool;
        }
        break;
    case 0x79:
        if (g_dev_mode) {
            g_automap_state->blink_enabled = !g_automap_state->blink_enabled;
            return 1;
        }
        break;
    }
    return 0;
}

/* Release the previous level's automap query state, then read the cell size
   and record count. A level with no records still gets one zero cell so the
   visited-bit arrays and record table stay valid; otherwise the key list is
   read whole and every cell key is inserted with its one-based index. */
// FUNCTION: WIZ8 0x00584DD0
unsigned char ReadAutomapNodes(int hFile)
{
    if (g_automap_visited_cells != 0) {
        delete g_automap_visited_cells;
        g_automap_visited_cells = 0;
    }
    if (g_automap_lit_cells != 0) {
        delete g_automap_lit_cells;
        g_automap_lit_cells = 0;
    }
    if (g_automap_cell_keys != 0) {
        free(g_automap_cell_keys);
        g_automap_cell_keys = 0;
    }
    if (g_automap_cell_index != 0) {
        delete g_automap_cell_index;
        g_automap_cell_index = 0;
    }

    FileRead(hFile, &g_automap_grid_cell_size, 4, 0);
    unsigned char ok = FileRead(hFile, &g_automap_cell_count, 4, 0);
    if (g_automap_cell_count == 0) {
        g_automap_visited_cells = new BitArray(1);
        g_automap_lit_cells = new BitArray(1);
        g_automap_cell_keys = static_cast<unsigned int*>(malloc(sizeof(*g_automap_cell_keys)));
        *g_automap_cell_keys = 0;
        g_automap_cell_count = 1;
        g_automap_cell_index = new W8HashTable<unsigned int, int>();
        return ok;
    }

    g_automap_visited_cells = new BitArray(g_automap_cell_count);
    if (g_automap_visited_cells == 0) {
        return 0;
    }
    g_automap_lit_cells = new BitArray(g_automap_cell_count);
    if (g_automap_lit_cells == 0) {
        if (g_automap_visited_cells != 0) {
            delete g_automap_visited_cells;
        }
        g_automap_visited_cells = 0;
        return 0;
    }
    g_automap_cell_keys =
        static_cast<unsigned int*>(malloc(g_automap_cell_count * sizeof(*g_automap_cell_keys)));
    if (g_automap_cell_keys == 0) {
        if (g_automap_visited_cells != 0) {
            delete g_automap_visited_cells;
        }
        if (g_automap_lit_cells != 0) {
            delete g_automap_lit_cells;
        }
        g_automap_visited_cells = 0;
        g_automap_lit_cells = 0;
        return 0;
    }

    unsigned char success = 0;
    if (ok != 0) {
        success = FileRead(hFile, g_automap_cell_keys,
                           g_automap_cell_count * sizeof(*g_automap_cell_keys), 0);
    }
    g_automap_cell_index = new W8HashTable<unsigned int, int>();
    for (int index = 0; index < g_automap_cell_count; ++index) {
        int value = index + 1;
        g_automap_cell_index->Insert(g_automap_cell_keys + index, &value);
    }
    return success;
}

/* Pack a world position into an automap node key: eleven bits of z, then
   eleven of x, then ten of y, each scaled to grid cells. */
// FUNCTION: WIZ8 0x005852B0
unsigned int AutomapNodeKey(const srVector3T<float>* position)
{
    return ((static_cast<unsigned int>(position->z / g_automap_grid_cell_size) & 0x7ff) |
            static_cast<unsigned int>(position->x / g_automap_grid_cell_size) << 11)
               << 10 |
           (static_cast<unsigned int>(position->y / g_automap_grid_cell_size) & 0x3ff);
}

/* The large-map level set: the same levels that get the 30000-unit automap
   range in ResetAutomapView also get 4000-unit cells instead of 2000. */
// FUNCTION: WIZ8 0x00585320
bool AutomapLevelIsLarge(void)
{
    if (g_status.current_level == 0x18 ||
        (g_status.current_level > 0x1a && g_status.current_level <= 0x22)) {
        return true;
    }
    return false;
}
