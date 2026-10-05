#include "wiz8/local_screens/MGSRadarMap.h"

#include <math.h>

#include "surrender/srMath.h"
#include "surrender/srNode.h"
#include "surrender/srScene.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/GrCycle.h"
#include "wiz8/engine_code/Item.h"
#include "wiz8/engine_code/Missile.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/float_constants.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/item_spawning.h"
#include "wiz8/local_code/ControlsRect.h"
#include "wiz8/local_code/FormationAndFacing.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/startup_world.h"
#include "wiz8/vector.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/xstatus.h"
#include "wiz8/engine_code/GameData.h"

/* 0x0064CA90: the blip palette, six classes of three distance rings. Items
   take class 4 and missiles class 5; monsters take their disposition-mapped
   class while a merely-sensed one and anything in the near field use class 3.
   AcquireRadarBlip's highlight path reuses rows group*3 and group*3+2 as the
   facing/up orientation vectors. */
// GLOBAL: WIZ8 0x0064ca90
static float g_radar_blip_colors[18][3] = {
    {0.5f, 0.5f, 0.0f}, {0.7f, 0.7f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.5f, 0.0f, 0.0f},
    {0.7f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.5f, 0.0f}, {0.0f, 0.7f, 0.0f},
    {0.0f, 1.0f, 0.0f}, {0.2f, 0.2f, 0.2f}, {0.5f, 0.5f, 0.5f}, {0.7f, 0.7f, 0.7f},
    {0.5f, 0.5f, 0.5f}, {0.7f, 0.7f, 0.7f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.5f},
    {0.0f, 0.0f, 0.7f}, {0.0f, 0.0f, 1.0f},
};

/* 0x0064CB68: ubDisposition to blip class for live monsters. */
// GLOBAL: WIZ8 0x0064cb68
static unsigned char g_radar_disposition_class[4] = {0, 1, 2, 0};

/* 0x0064CB6C: screen offsets at which the zoomed map image gets each
   occupied formation cell's party-order chip painted. */
// GLOBAL: WIZ8 0x0064cb6c
static srVector2i g_radar_cell_offsets[15] = {
    {45, 37}, {42, 38}, {48, 38}, {53, 45}, {52, 42}, {52, 48}, {45, 53}, {48, 52},
    {42, 52}, {37, 45}, {38, 48}, {38, 42}, {45, 42}, {43, 46}, {47, 46},
};

/* The radar overlay's runtime state: the lazily built backdrop, the zoom
   preset flag, the three sprites, the eighteen sector blip pools with their
   per-sector reuse cursors, and the active range/scale band. */
// GLOBAL: WIZ8 0x0069bf58
static stModelInstance2D* g_radar_backdrop = 0;
// GLOBAL: WIZ8 0x0069bf5c
static bool g_radar_zoomed = false;
// GLOBAL: WIZ8 0x0069bf60
static stModelInstance2D* g_radar_compass = 0;
// GLOBAL: WIZ8 0x0069c088
static stModelInstance2D* g_radar_frame = 0;
// GLOBAL: WIZ8 0x0069bf68
static W8GrowableVector<stModelInstance2D*> g_radar_icon_pools[18];
// GLOBAL: WIZ8 0x0069c08c
static int g_radar_icon_cursors[18];
// GLOBAL: WIZ8 0x0069c0d4
static float g_radar_outer_radius;
// GLOBAL: WIZ8 0x0069c0d8
static float g_radar_inner_radius;
// GLOBAL: WIZ8 0x0069c0dc
static stModelInstance2D* g_radar_map = 0;
// GLOBAL: WIZ8 0x0069c0e0
static float g_radar_map_scale;
// GLOBAL: WIZ8 0x0069c0e4
static bool g_radar_map_enabled = 0;

// GLOBAL: WIZ8 0x005eecd8
const double g_double_005eecd8 = 3.141592653589793;
// GLOBAL: WIZ8 0x005eece8
const float g_float_005eece8 = 404.0f;
// GLOBAL: WIZ8 0x005eecec
const float g_float_005eecec = 75.0f;
// GLOBAL: WIZ8 0x005eecf0
const float g_float_005eecf0 = 38.0f;

static stModelInstance2D* AcquireRadarBlip(int sector, bool lit);
static unsigned char PlaceRadarBlip(srVector3T<float>* delta, int group, bool lit);

static void ResetRadarBlips()
{
    for (int sector = 0; sector < 18; ++sector) {
        W8GrowableVector<stModelInstance2D*>* pool = &g_radar_icon_pools[sector];
        int count = pool->count;

        g_radar_icon_cursors[sector] = 0;
        for (int index = 0; index < count; ++index) {
            (*pool->GetAt(index))->setFlag(srNode::FLAG_DISABLE);
        }
    }
}

// FUNCTION: WIZ8 0x005a20e0
void EnableRadarMap(bool enable)
{
    g_radar_map_enabled = enable;
    if (!enable) {
        ResetRadarBlips();
    }
}

// FUNCTION: WIZ8 0x005a2140
void EnsureRadarMapOverlay(void)
{
    if (g_radar_backdrop == 0) {
        W8ControlsRect bounds;

        bounds.left = 0x17;
        bounds.top = 0x166;
        bounds.right = 0x80;
        bounds.bottom = 0x1c2;
        g_radar_backdrop = CreateSpriteFromVideoSurface(-0xe, &bounds, 0, 0, 1);
        Position2DNodeUnsnapped(g_radar_backdrop, 0x17, 0x166);
        SetModelInstance2DDisplayState(g_radar_backdrop, 4);
    }
}

// FUNCTION: WIZ8 0x005a21b0
static stModelInstance2D* AcquireRadarBlip(int sector, bool lit)
{
    W8GrowableVector<stModelInstance2D*>* pool = &g_radar_icon_pools[sector];
    stModelInstance2D* icon;
    int cursor = g_radar_icon_cursors[sector];

    if (cursor < pool->count) {
        g_radar_icon_cursors[sector] = cursor + 1;
        icon = *pool->GetAt(cursor);
    } else {
        icon = new stModelInstance2D(0);
        *icon = **pool->GetAt(0);
        pool->Add(icon);
    }
    if (icon != 0) {
        icon->clearFlag(srNode::FLAG_DISABLE);
        icon->setParent(0, 1);
        icon->setParent(g_scene_square, 1);
        icon->overlay_scene_flag |= 1;
        if (!lit) {
            icon->SetGlowEnabled(0);
        } else {
            srVector4T<float> first;
            srVector4T<float> second;
            int group = sector - sector % 3;

            icon->SetGlowEnabled(1);
            second.w = 1.0f;
            first.w = 1.0f;
            second.x = g_radar_blip_colors[group + 2][0];
            second.y = g_radar_blip_colors[group + 2][1];
            second.z = g_radar_blip_colors[group + 2][2];
            first.x = g_radar_blip_colors[group][0];
            first.y = g_radar_blip_colors[group][1];
            first.z = g_radar_blip_colors[group][2];
            icon->SetGlowColors(&first, &second);
            icon->render_state.render_depth = 1000;
        }
    }
    return icon;
}

// FUNCTION: WIZ8 0x005a23e0
void ReleaseRadarMap(void)
{
    if (g_radar_map != 0) {
        ReleaseObject(g_radar_map);
        g_radar_map = 0;
    }
    if (g_radar_frame != 0) {
        ReleaseObject(g_radar_frame);
        g_radar_frame = 0;
    }
    if (g_radar_compass != 0) {
        ReleaseObject(g_radar_compass);
        g_radar_compass = 0;
    }
    for (int sector = 0; sector < 18; ++sector) {
        W8GrowableVector<stModelInstance2D*>* pool = &g_radar_icon_pools[sector];

        while (pool->count != 0) {
            stModelInstance2D* icon = pool->RemoveAt(0);
            if (icon != 0) {
                icon->release();
            }
        }
    }
    if (g_radar_backdrop != 0) {
        g_radar_backdrop->release();
        g_radar_backdrop = 0;
    }
}

// FUNCTION: WIZ8 0x005a24a0
void RefreshRadarMap(void)
{
    int sector;

    if (!g_radar_map_enabled) {
        return;
    }
    if (g_radar_map != 0) {
        ReleaseObject(g_radar_map);
        g_radar_map = 0;
    }
    if (g_radar_frame != 0) {
        ReleaseObject(g_radar_frame);
        g_radar_frame = 0;
    }
    if (g_radar_compass != 0) {
        ReleaseObject(g_radar_compass);
        g_radar_compass = 0;
    }
    for (sector = 0; sector < 18; ++sector) {
        W8GrowableVector<stModelInstance2D*>* pool = &g_radar_icon_pools[sector];

        while (pool->count != 0) {
            stModelInstance2D* icon = *pool->GetAt(0);
            pool->RemoveAt(0);
            if (icon != 0) {
                icon->release();
            }
        }
    }
    if (g_radar_backdrop != 0) {
        g_radar_backdrop->release();
        g_radar_backdrop = 0;
    }

    unsigned int map_surface;
    unsigned int handle;
    if (!g_radar_zoomed) {
        handle = GetCatalogVideoObjectHandle(0xa4, 0);
        if (handle == 0) {
            return;
        }
        MakeVSurfaceFromVObject(handle, 0, &map_surface);
    } else {
        handle = GetCatalogVideoObjectHandle(0xa3, 0);
        if (handle == 0) {
            return;
        }
        MakeVSurfaceFromVObject(handle, 0, &map_surface);
        for (int slot = 0; slot < 8; ++slot) {
            W8PartySlotRow* row = &g_status.buffers.XChar[slot];
            W8PartyFormationPosition* position = &g_status.formation.positions[slot];

            if (row->fOccupied && position->bQuadrant != -1) {
                int cell = position->bQuadrant * 3 + position->bQuadrantSlot;
                DrawCatalogImage(static_cast<int>(map_surface), 0xa5, 0, row->party_order_index,
                                 g_radar_cell_offsets[cell].x, g_radar_cell_offsets[cell].y, 2, 0);
            }
        }
    }

    if (g_level_block->radar_map_alternate == 0) {
        handle = GetCatalogVideoObjectHandle(0xa1, 0);
    } else {
        handle = GetCatalogVideoObjectHandle(0xa2, 0);
    }
    if (handle == 0) {
        return;
    }
    unsigned int frame_surface;
    MakeVSurfaceFromVObject(handle, 0, &frame_surface);
    handle = GetCatalogVideoObjectHandle(0x9b, 0);
    if (handle == 0) {
        return;
    }
    unsigned int compass_surface;
    MakeVSurfaceFromVObject(handle, 0, &compass_surface);

    g_radar_compass = CreateSpriteFromSurface(compass_surface, 0, 1, 0, 1);
    Position2DNodeUnsnapped(g_radar_compass, 0x1e, 0x167);
    SetModelInstance2DDisplayState(g_radar_compass, 4);
    g_radar_frame = CreateSpriteFromSurface(frame_surface, 0, 1, 0, 1);
    Position2DNodeUnsnapped(g_radar_frame, 0x1e, 0x167);
    SetModelInstance2DDisplayState(g_radar_frame, 4);
    g_radar_map = CreateSpriteFromSurface(map_surface, 0, 1, 0, 1);
    Position2DNodeUnsnapped(g_radar_map, 0x1e, 0x167);
    SetModelInstance2DDisplayState(g_radar_map, 4);

    for (sector = 0; sector < 18; ++sector) {
        srVector4T<float> color;
        stModelInstance2D* icon;

        color.x = g_radar_blip_colors[sector][0];
        color.y = g_radar_blip_colors[sector][1];
        color.z = g_radar_blip_colors[sector][2];
        color.w = 1.0f;
        icon = CreateColoredPolygonSprite(2, 2, &color, 0);
        g_radar_icon_pools[sector].Add(icon);
        icon->overlay_scene_flag |= 1;
    }
    UpdateRadarBlips();
}

// FUNCTION: WIZ8 0x005a2800
void UpdateRadarBlips(void)
{
    srVector3T<float> camera;
    srVector3T<float> position;
    srVector3T<float> delta;
    srVector3T<float> center;
    srVector3T<float> party;
    srVector3T<float> bounds_min;
    srVector3T<float> bounds_max;
    bool detect_all;

    ResetRadarBlips();
    if (!g_radar_map_enabled || gXStatus.fSurprisePossible) {
        return;
    }
    if (g_radar_compass != 0) {
        RotateNodeInDegrees(g_radar_compass, g_status.party_facing -
                                                 static_cast<int>(g_status.party_heading) + 0x168);
    }
    if (g_radar_frame != 0) {
        RotateNodeInDegrees(g_radar_frame, g_status.party_facing);
    }
    GetCameraPosition(&camera);
    detect_all = PartyHasCondition(0x40);

    W8WorldItem* world_item = GetNextWorldItem(1);
    while (world_item != 0) {
        W8Item* item = world_item->p3D;

        if (item != 0) {
            W8ItemRep* rep = static_cast<W8ItemRep*>(item->m_pRep);
            if ((rep->flags & 4) == 0) {
                rep->GetLocation(&position);
                item->GetCachedLocalBounds(&bounds_min, &bounds_max);
                center.Set((bounds_min.x + bounds_max.x) * g_double_005ebe80,
                           (bounds_min.y + bounds_max.y) * g_double_005ebe80,
                           (bounds_min.z + bounds_max.z) * g_double_005ebe80);
                position += center;
                party = g_startup_world->GetPosition();
                delta = position - party;
                if ((detect_all || ((rep->flags >> 3) & 1) != 0 ||
                     HasCameraLineOfSight(&position)) &&
                    delta.Length() <= g_radar_outer_radius) {
                    if (PlaceRadarBlip(&delta, 4, item->IsRadarBlipLit()) != 0) {
                        rep->flags |= 8;
                    }
                }
            }
        }
        world_item = GetNextWorldItem(0);
    }

    W8MonsterInfo* info = GetNextMonsterInfo(1);
    while (info != 0) {
        W8Monster* monster = info->p3D;

        if (monster != 0 && info->fActive && info->within_viewing_distance != 0 &&
            (!monster->disabled || detect_all)) {
            bool hostile = false;

            if (gXStatus.fCombatMode && g_status.selected_character != -1 &&
                g_status.buffers.XChar[g_status.selected_character].fOccupied != 0 &&
                (static_cast<unsigned char>(1 << g_status.selected_character) &
                 MonsterGetHighlightMask(monster)) != 0) {
                hostile = 1;
            }
            if (monster->IsRenderable(1) == 0 && !detect_all) {
                if (info->party_threat.sight_state == W8_SIGHT_RECENT) {
                    monster->GetAnimationBounds(&bounds_min, &bounds_max);
                    center.Set((bounds_min.x + bounds_max.x) * g_double_005ebe80,
                               (bounds_min.y + bounds_max.y) * g_double_005ebe80,
                               (bounds_min.z + bounds_max.z) * g_double_005ebe80);
                    position = center + info->party_threat.camera_position;
                    party = g_startup_world->GetPosition();
                    delta = position - party;
                    float distance = delta.Length();
                    if (distance - monster->radius < g_radar_outer_radius) {
                        distance /= distance - monster->radius;
                        delta.x *= distance;
                        delta.z *= distance;
                        PlaceRadarBlip(&delta, 3, hostile);
                    } else {
                        delta.y = 0.0f;
                        distance = delta.Length();
                        if (distance - monster->radius > g_radar_outer_radius) {
                            distance = g_radar_outer_radius / (distance - monster->radius);
                            delta.x *= distance;
                            delta.z *= distance;
                            PlaceRadarBlip(&delta, 3, hostile);
                        }
                    }
                }
            } else {
                monster->GetAnimationBounds(&bounds_min, &bounds_max);
                center.Set((bounds_min.x + bounds_max.x) * g_double_005ebe80,
                           (bounds_min.y + bounds_max.y) * g_double_005ebe80,
                           (bounds_min.z + bounds_max.z) * g_double_005ebe80);
                party = monster->GetPosition();
                position = center + party;
                party = g_startup_world->GetPosition();
                delta = position - party;
                float distance = delta.Length();
                if (distance - monster->radius < g_radar_outer_radius) {
                    distance /= distance - monster->radius;
                    delta.x *= distance;
                    delta.z *= distance;
                    PlaceRadarBlip(&delta, g_radar_disposition_class[info->ubDisposition], hostile);
                } else {
                    delta.y = 0.0f;
                    distance = delta.Length();
                    if (distance - monster->radius > g_radar_outer_radius) {
                        distance = g_radar_outer_radius / (distance - monster->radius);
                        delta.x *= distance;
                        delta.z *= distance;
                        PlaceRadarBlip(&delta, g_radar_disposition_class[info->ubDisposition],
                                       hostile);
                    }
                }
            }
        }
        info = GetNextMonsterInfo(0);
    }

    W8Missile* missile = NextMissile(1);
    while (missile != 0) {
        if (!missile->impacting) {
            missile->GetAnimationBounds(&bounds_min, &bounds_max);
            center.Set((bounds_min.x + bounds_max.x) * g_double_005ebe80,
                       (bounds_min.y + bounds_max.y) * g_double_005ebe80,
                       (bounds_min.z + bounds_max.z) * g_double_005ebe80);
            party = missile->GetPosition();
            position = center + party;
            party = g_startup_world->GetPosition();
            delta = position - party;
            if (delta.Length() < g_radar_outer_radius) {
                PlaceRadarBlip(&delta, 5, 0);
            }
        }
        missile = NextMissile(0);
    }
    SetRendererModePair();
}

// FUNCTION: WIZ8 0x005a3060
static unsigned char PlaceRadarBlip(srVector3T<float>* delta, int group, bool lit)
{
    int ring = 0;
    srMatrix3T<float> rotation;
    int left;
    int top;

    if (fabs(delta->y) >= g_double_005ee768) {
        if (delta->y > g_double_zero) {
            ring = 2;
        }
    } else {
        ring = 1;
    }

    rotation.SetIdentity();
    double angle = g_double_005eecd8 * g_float_005ebcf8 * (0x168 - g_status.party_facing);
    if (angle != g_double_zero) {
        float cosine = static_cast<float>(cos(angle));
        float sine = static_cast<float>(sin(angle));
        rotation.RotateAboutY(sine, cosine);
    }

    delta->Transform(rotation);
    delta->y = 0.0f;

    float distance = delta->Length();
    if (distance <= g_radar_inner_radius) {
        float scale;

        if (group == 5 || group == 4) {
            scale = g_radar_map_scale / g_radar_inner_radius;
        } else {
            scale = g_radar_map_scale / distance;
        }
        left = static_cast<int>(scale * delta->x) + 0x4b;
        top = 0x194 - static_cast<int>(scale * delta->z);
    } else {
        float scale = g_radar_map_scale + (distance - g_radar_inner_radius) *
                                              (g_float_005eecf0 - g_radar_map_scale) /
                                              (g_radar_outer_radius - g_radar_inner_radius);

        if (scale > g_float_005eecf0) {
            scale = g_float_005eecf0;
        }
        scale /= distance;
        left = static_cast<int>(scale * delta->x + g_float_005eecec);
        top = static_cast<int>(g_float_005eece8 - scale * delta->z);
    }
    stModelInstance2D* blip = AcquireRadarBlip(ring + group * 3, lit);
    Position2DNodeUnsnapped(blip, left, top);
    return 1;
}

// FUNCTION: WIZ8 0x005a3360
void ToggleRadarMapZoom(void)
{
    if (!g_radar_zoomed) {
        float radius = g_startup_world->radius;

        g_radar_zoomed = true;
        g_radar_map_scale = 13.0f;
        g_radar_inner_radius = CalcRangeDistance(W8_RANGE_TOUCH) + radius;
        g_radar_outer_radius = CalcRangeDistance(W8_RANGE_LONG) + radius;
        RefreshRadarMap();
        return;
    }
    ZoomRadarMapOut();
}

// FUNCTION: WIZ8 0x005a3410
void ZoomRadarMapIn(void)
{
    g_radar_zoomed = true;
    g_radar_map_scale = 13.0f;
    g_radar_inner_radius = CalcRangeDistance(W8_RANGE_TOUCH) + g_startup_world->radius;
    g_radar_outer_radius = CalcRangeDistance(W8_RANGE_LONG) + g_startup_world->radius;
    RefreshRadarMap();
}

// FUNCTION: WIZ8 0x005a3470
void ZoomRadarMapOut(void)
{
    float radius = g_startup_world->radius;

    g_radar_zoomed = false;
    g_radar_map_scale = 2.0f;
    g_radar_inner_radius = radius;
    g_radar_outer_radius = CalcRangeDistance(W8_RANGE_EXTREME) + radius;
    RefreshRadarMap();
}
