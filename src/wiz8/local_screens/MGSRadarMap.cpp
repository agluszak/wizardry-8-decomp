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
const float g_radar_blip_colors_0064ca90[18][3] = {
    {0.5f, 0.5f, 0.0f}, {0.7f, 0.7f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.5f, 0.0f, 0.0f},
    {0.7f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.5f, 0.0f}, {0.0f, 0.7f, 0.0f},
    {0.0f, 1.0f, 0.0f}, {0.2f, 0.2f, 0.2f}, {0.5f, 0.5f, 0.5f}, {0.7f, 0.7f, 0.7f},
    {0.5f, 0.5f, 0.5f}, {0.7f, 0.7f, 0.7f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.5f},
    {0.0f, 0.0f, 0.7f}, {0.0f, 0.0f, 1.0f},
};

/* 0x0064CB68: ubDisposition to blip class for live monsters. */
// GLOBAL: WIZ8 0x0064cb68
const unsigned char g_radar_disposition_class_0064cb68[4] = {0, 1, 2, 0};

/* 0x0064CB6C: screen offsets at which the zoomed map image gets each
   occupied formation cell's party-order chip painted. */
// GLOBAL: WIZ8 0x0064cb6c
const int g_radar_cell_offsets_0064cb6c[15][2] = {
    {45, 37}, {42, 38}, {48, 38}, {53, 45}, {52, 42}, {52, 48}, {45, 53}, {48, 52},
    {42, 52}, {37, 45}, {38, 48}, {38, 42}, {45, 42}, {43, 46}, {47, 46},
};

/* The radar overlay's runtime state: the lazily built backdrop, the zoom
   preset flag, the three sprites, the eighteen sector blip pools with their
   per-sector reuse cursors, and the active range/scale band. */
// GLOBAL: WIZ8 0x0069bf58
stModelInstance2D* g_radar_backdrop_0069bf58 = 0;
// GLOBAL: WIZ8 0x0069bf5c
unsigned char g_radar_zoomed_0069bf5c = 0;
// GLOBAL: WIZ8 0x0069bf60
stModelInstance2D* g_radar_compass_0069bf60 = 0;
// GLOBAL: WIZ8 0x0069c088
stModelInstance2D* g_radar_frame_0069c088 = 0;
// GLOBAL: WIZ8 0x0069bf68
W8GrowableVector<stModelInstance2D*> g_radar_icon_pools_0069bf68[18];
// GLOBAL: WIZ8 0x0069c08c
int g_radar_icon_cursors_0069c08c[18];
// GLOBAL: WIZ8 0x0069c0d4
float g_radar_outer_radius_0069c0d4;
// GLOBAL: WIZ8 0x0069c0d8
float g_radar_inner_radius_0069c0d8;
// GLOBAL: WIZ8 0x0069c0dc
stModelInstance2D* g_radar_map_0069c0dc = 0;
// GLOBAL: WIZ8 0x0069c0e0
float g_radar_map_scale_0069c0e0;
// GLOBAL: WIZ8 0x0069c0e4
unsigned char g_radar_map_enabled_0069c0e4 = 0;

// GLOBAL: WIZ8 0x005eecd8
const double g_double_005eecd8 = 3.141592653589793;
// GLOBAL: WIZ8 0x005eece8
const float g_float_005eece8 = 404.0f;
// GLOBAL: WIZ8 0x005eecec
const float g_float_005eecec = 75.0f;
// GLOBAL: WIZ8 0x005eecf0
const float g_float_005eecf0 = 38.0f;

static stModelInstance2D* AcquireRadarBlip(int sector, unsigned char lit);
static unsigned char PlaceRadarBlip(srVector3T<float>* delta, int group, unsigned char lit);

// FUNCTION: WIZ8 0x005a20e0
void EnableRadarMap(char enable)
{
    g_radar_map_enabled_0069c0e4 = enable;
    if (enable == 0) {
        for (int sector = 0; sector < 18; ++sector) {
            W8GrowableVector<stModelInstance2D*>* pool = &g_radar_icon_pools_0069bf68[sector];
            int count = pool->count;

            g_radar_icon_cursors_0069c08c[sector] = 0;
            for (int index = 0; index < count; ++index) {
                (*pool->GetAt(index))->setFlag(srNode::FLAG_DISABLE);
            }
        }
    }
}

// FUNCTION: WIZ8 0x005a2140
void EnsureRadarMapOverlay(void)
{
    if (g_radar_backdrop_0069bf58 == 0) {
        W8ControlsRect bounds;

        bounds.left = 0x17;
        bounds.top = 0x166;
        bounds.right = 0x80;
        bounds.bottom = 0x1c2;
        g_radar_backdrop_0069bf58 = Function4253F0(-0xe, &bounds, 0, 0, 1);
        PositionToolTipNode(g_radar_backdrop_0069bf58, 0x17, 0x166, 0);
        g_radar_backdrop_0069bf58->render_state_164.display_state = 4;
    }
}

// FUNCTION: WIZ8 0x005a21b0
static stModelInstance2D* AcquireRadarBlip(int sector, unsigned char lit)
{
    W8GrowableVector<stModelInstance2D*>* pool = &g_radar_icon_pools_0069bf68[sector];
    stModelInstance2D* icon;
    int cursor = g_radar_icon_cursors_0069c08c[sector];

    if (cursor < pool->count) {
        g_radar_icon_cursors_0069c08c[sector] = cursor + 1;
        icon = *pool->GetAt(cursor);
    } else {
        icon = new stModelInstance2D(0);
        *icon = **pool->GetAt(0);
        pool->Add(icon);
    }
    if (icon != 0) {
        icon->clearFlag(srNode::FLAG_DISABLE);
        icon->setParent(0, 1);
        icon->setParent(g_scene_square_65965c, 1);
        icon->state_160 |= 1;
        if (lit == 0) {
            icon->SetGlowEnabled00480EB0(0);
        } else {
            srVector4T<float> first;
            srVector4T<float> second;
            int group = sector - sector % 3;

            icon->SetGlowEnabled00480EB0(1);
            second.w = 1.0f;
            first.w = 1.0f;
            second.x = g_radar_blip_colors_0064ca90[group + 2][0];
            second.y = g_radar_blip_colors_0064ca90[group + 2][1];
            second.z = g_radar_blip_colors_0064ca90[group + 2][2];
            first.x = g_radar_blip_colors_0064ca90[group][0];
            first.y = g_radar_blip_colors_0064ca90[group][1];
            first.z = g_radar_blip_colors_0064ca90[group][2];
            icon->SetGlowColors00480FF0(&first, &second);
            icon->render_state_164.render_depth = 1000;
        }
    }
    return icon;
}

// FUNCTION: WIZ8 0x005a23e0
void ReleaseRadarMap(void)
{
    if (g_radar_map_0069c0dc != 0) {
        ReleaseObject004257F0(g_radar_map_0069c0dc);
        g_radar_map_0069c0dc = 0;
    }
    if (g_radar_frame_0069c088 != 0) {
        ReleaseObject004257F0(g_radar_frame_0069c088);
        g_radar_frame_0069c088 = 0;
    }
    if (g_radar_compass_0069bf60 != 0) {
        ReleaseObject004257F0(g_radar_compass_0069bf60);
        g_radar_compass_0069bf60 = 0;
    }
    for (int sector = 0; sector < 18; ++sector) {
        W8GrowableVector<stModelInstance2D*>* pool = &g_radar_icon_pools_0069bf68[sector];

        while (pool->count != 0) {
            stModelInstance2D* icon = pool->RemoveAt(0);
            if (icon != 0) {
                icon->release();
            }
        }
    }
    if (g_radar_backdrop_0069bf58 != 0) {
        g_radar_backdrop_0069bf58->release();
        g_radar_backdrop_0069bf58 = 0;
    }
}

// FUNCTION: WIZ8 0x005a24a0
void RefreshRadarMap(void)
{
    int sector;

    if (g_radar_map_enabled_0069c0e4 == 0) {
        return;
    }
    if (g_radar_map_0069c0dc != 0) {
        ReleaseObject004257F0(g_radar_map_0069c0dc);
        g_radar_map_0069c0dc = 0;
    }
    if (g_radar_frame_0069c088 != 0) {
        ReleaseObject004257F0(g_radar_frame_0069c088);
        g_radar_frame_0069c088 = 0;
    }
    if (g_radar_compass_0069bf60 != 0) {
        ReleaseObject004257F0(g_radar_compass_0069bf60);
        g_radar_compass_0069bf60 = 0;
    }
    for (sector = 0; sector < 18; ++sector) {
        W8GrowableVector<stModelInstance2D*>* pool = &g_radar_icon_pools_0069bf68[sector];

        while (pool->count != 0) {
            stModelInstance2D* icon = *pool->GetAt(0);
            pool->RemoveAt(0);
            if (icon != 0) {
                icon->release();
            }
        }
    }
    if (g_radar_backdrop_0069bf58 != 0) {
        g_radar_backdrop_0069bf58->release();
        g_radar_backdrop_0069bf58 = 0;
    }

    unsigned int map_surface;
    unsigned int handle;
    if (g_radar_zoomed_0069bf5c == 0) {
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
            W8PartySlotRow* row = &g_status_685170.buffers.party_rows[slot];
            W8PartyFormationPosition* position = &g_status_685170.formation.positions[slot];

            if (row->occupied != 0 && position->bQuadrant != -1) {
                int cell = position->bQuadrant * 3 + position->bQuadrantSlot;
                DrawCatalogImage((int)map_surface, 0xa5, 0, row->party_order_index,
                                 g_radar_cell_offsets_0064cb6c[cell][0],
                                 g_radar_cell_offsets_0064cb6c[cell][1], 2, 0);
            }
        }
    }

    if (g_level_block->flag_326 == 0) {
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

    g_radar_compass_0069bf60 = CreateSpriteFromSurface(compass_surface, 0, 1, 0, 1);
    PositionToolTipNode(g_radar_compass_0069bf60, 0x1e, 0x167, 0);
    g_radar_compass_0069bf60->render_state_164.display_state = 4;
    g_radar_frame_0069c088 = CreateSpriteFromSurface(frame_surface, 0, 1, 0, 1);
    PositionToolTipNode(g_radar_frame_0069c088, 0x1e, 0x167, 0);
    g_radar_frame_0069c088->render_state_164.display_state = 4;
    g_radar_map_0069c0dc = CreateSpriteFromSurface(map_surface, 0, 1, 0, 1);
    PositionToolTipNode(g_radar_map_0069c0dc, 0x1e, 0x167, 0);
    g_radar_map_0069c0dc->render_state_164.display_state = 4;

    for (sector = 0; sector < 18; ++sector) {
        srVector4T<float> color;
        stModelInstance2D* icon;

        color.x = g_radar_blip_colors_0064ca90[sector][0];
        color.y = g_radar_blip_colors_0064ca90[sector][1];
        color.z = g_radar_blip_colors_0064ca90[sector][2];
        color.w = 1.0f;
        icon = Function424790(2, 2, &color, 0);
        g_radar_icon_pools_0069bf68[sector].Add(icon);
        icon->state_160 |= 1;
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
    unsigned char detect_all;

    for (int sector = 0; sector < 18; ++sector) {
        W8GrowableVector<stModelInstance2D*>* pool = &g_radar_icon_pools_0069bf68[sector];
        int count = pool->count;

        g_radar_icon_cursors_0069c08c[sector] = 0;
        for (int index = 0; index < count; ++index) {
            (*pool->GetAt(index))->setFlag(srNode::FLAG_DISABLE);
        }
    }
    if (g_radar_map_enabled_0069c0e4 == 0 || gXStatus.fSurprisePossible != 0) {
        return;
    }
    if (g_radar_compass_0069bf60 != 0) {
        RotateNodeInDegrees00425840(g_radar_compass_0069bf60,
                                    g_status_685170.party_facing -
                                        static_cast<int>(g_status_685170.party_heading) + 0x168);
    }
    if (g_radar_frame_0069c088 != 0) {
        RotateNodeInDegrees00425840(g_radar_frame_0069c088, g_status_685170.party_facing);
    }
    GetCameraPosition(&camera);
    detect_all = PartyHasCondition(0x40);

    W8WorldItem* world_item = GetNextWorldItem(1);
    while (world_item != 0) {
        W8Item* item = world_item->owner;

        if (item != 0) {
            W8ItemRep* rep = static_cast<W8ItemRep*>(item->m_pRep);
            if ((rep->flags & 4) == 0) {
                rep->GetLocation004B8890(&position);
                item->GetCachedLocalBounds(&bounds_min.x, &bounds_max.x);
                center.Set((bounds_min.x + bounds_max.x) * g_double_005ebe80,
                           (bounds_min.y + bounds_max.y) * g_double_005ebe80,
                           (bounds_min.z + bounds_max.z) * g_double_005ebe80);
                position.x += center.x;
                position.y += center.y;
                position.z += center.z;
                party = g_startup_world_659c0c->GetPosition();
                delta = position - party;
                if ((detect_all != 0 || ((rep->flags >> 3) & 1) != 0 ||
                     HasCameraLineOfSight(&position)) &&
                    delta.Length() <= g_radar_outer_radius_0069c0d4) {
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
        W8Monster* monster = info->monster;

        if (monster != 0 && info->fActive != 0 && info->within_viewing_distance != 0 &&
            (monster->flag_217 == 0 || detect_all != 0)) {
            unsigned char hostile = 0;

            if (gXStatus.fCombatMode != 0 && g_status_685170.selected_character != -1 &&
                g_status_685170.buffers.party_rows[g_status_685170.selected_character].occupied !=
                    0 &&
                ((unsigned char)(1 << g_status_685170.selected_character) &
                 MonsterGetRuntimeFlag5BC(monster)) != 0) {
                hostile = 1;
            }
            if (monster->IsRenderable004C7C00(1) == 0 && detect_all == 0) {
                if (info->party_threat.state_04 == 2) {
                    monster->GetAnimationBounds(&bounds_min, &bounds_max);
                    center.Set((bounds_min.x + bounds_max.x) * g_double_005ebe80,
                               (bounds_min.y + bounds_max.y) * g_double_005ebe80,
                               (bounds_min.z + bounds_max.z) * g_double_005ebe80);
                    position.x = center.x + info->party_threat.camera_position_0c.x;
                    position.y = center.y + info->party_threat.camera_position_0c.y;
                    position.z = center.z + info->party_threat.camera_position_0c.z;
                    party = g_startup_world_659c0c->GetPosition();
                    delta = position - party;
                    float distance = delta.Length();
                    if (distance - monster->radius_084 < g_radar_outer_radius_0069c0d4) {
                        distance = distance / (distance - monster->radius_084);
                        delta.x *= distance;
                        delta.z *= distance;
                        PlaceRadarBlip(&delta, 3, hostile);
                    } else {
                        delta.y = 0.0f;
                        distance = delta.Length();
                        if (distance - monster->radius_084 > g_radar_outer_radius_0069c0d4) {
                            distance =
                                g_radar_outer_radius_0069c0d4 / (distance - monster->radius_084);
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
                position.x = center.x + party.x;
                position.y = center.y + party.y;
                position.z = center.z + party.z;
                party = g_startup_world_659c0c->GetPosition();
                delta = position - party;
                float distance = delta.Length();
                if (distance - monster->radius_084 < g_radar_outer_radius_0069c0d4) {
                    distance = distance / (distance - monster->radius_084);
                    delta.x *= distance;
                    delta.z *= distance;
                    PlaceRadarBlip(&delta, g_radar_disposition_class_0064cb68[info->ubDisposition],
                                   hostile);
                } else {
                    delta.y = 0.0f;
                    distance = delta.Length();
                    if (distance - monster->radius_084 > g_radar_outer_radius_0069c0d4) {
                        distance = g_radar_outer_radius_0069c0d4 / (distance - monster->radius_084);
                        delta.x *= distance;
                        delta.z *= distance;
                        PlaceRadarBlip(&delta,
                                       g_radar_disposition_class_0064cb68[info->ubDisposition],
                                       hostile);
                    }
                }
            }
        }
        info = GetNextMonsterInfo(0);
    }

    W8Missile* missile = NextMissile004A2760(1);
    while (missile != 0) {
        if (missile->flag_1e1 == 0) {
            missile->GetAnimationBounds(&bounds_min, &bounds_max);
            center.x = (bounds_min.x + bounds_max.x) * g_double_005ebe80;
            center.y = (bounds_min.y + bounds_max.y) * g_double_005ebe80;
            center.z = (bounds_min.z + bounds_max.z) * g_double_005ebe80;
            party = missile->GetPosition();
            position.x = center.x + party.x;
            position.y = center.y + party.y;
            position.z = center.z + party.z;
            party = g_startup_world_659c0c->GetPosition();
            delta = position - party;
            if (delta.Length() < g_radar_outer_radius_0069c0d4) {
                PlaceRadarBlip(&delta, 5, 0);
            }
        }
        missile = NextMissile004A2760(0);
    }
    SetRendererModePair();
}

// FUNCTION: WIZ8 0x005a3060
static unsigned char PlaceRadarBlip(srVector3T<float>* delta, int group, unsigned char lit)
{
    int ring = 0;
    srMatrix3T<float> rotation;
    int left;
    int top;

    if (fabs(delta->y) >= g_double_005ee768) {
        if (delta->y > g_zero_005ebb40) {
            ring = 2;
        }
    } else {
        ring = 1;
    }

    rotation.vectors[0].x = 1.0f;
    rotation.vectors[0].y = 0.0f;
    rotation.vectors[1].x = 0.0f;
    rotation.vectors[1].y = 1.0f;
    rotation.vectors[2].x = 0.0f;
    rotation.vectors[2].y = 0.0f;
    double angle = g_double_005eecd8 * g_float_005ebcf8 * (0x168 - g_status_685170.party_facing);
    rotation.vectors[0].z = 0.0f;
    rotation.vectors[1].z = 0.0f;
    rotation.vectors[2].z = 1.0f;
    if (angle != g_zero_005ebb40) {
        srVector3T<float> first;
        srVector3T<float> second;
        srVector3T<float> third;
        srMatrix3T<float> heading;
        float cosine = (float)cos(angle);
        float sine;

        third.y = 0.0f;
        second.x = 0.0f;
        second.y = 1.0f;
        second.z = 0.0f;
        sine = (float)sin(angle);
        third.x = -sine;
        third.z = cosine;
        first.Set((double)cosine, 0.0, (double)sine);
        rotation.MultiplyBy(*heading.SetRows(first, second, third));
    }

    delta->Transform(rotation);
    delta->y = 0.0f;

    float distance = delta->Length();
    if (distance <= g_radar_inner_radius_0069c0d8) {
        float scale;

        if (group == 5 || group == 4) {
            scale = g_radar_map_scale_0069c0e0 / g_radar_inner_radius_0069c0d8;
        } else {
            scale = g_radar_map_scale_0069c0e0 / distance;
        }
        left = (int)(scale * delta->x) + 0x4b;
        top = 0x194 - (int)(scale * delta->z);
    } else {
        float scale = g_radar_map_scale_0069c0e0 +
                      (distance - g_radar_inner_radius_0069c0d8) *
                          (g_float_005eecf0 - g_radar_map_scale_0069c0e0) /
                          (g_radar_outer_radius_0069c0d4 - g_radar_inner_radius_0069c0d8);

        if (scale > g_float_005eecf0) {
            scale = g_float_005eecf0;
        }
        scale = scale / distance;
        left = (int)(scale * delta->x + g_float_005eecec);
        top = (int)(g_float_005eece8 - scale * delta->z);
    }
    stModelInstance2D* blip = AcquireRadarBlip(ring + group * 3, lit);
    PositionToolTipNode(blip, left, top, 0);
    return 1;
}

// FUNCTION: WIZ8 0x005a3360
void ToggleRadarMapZoom(void)
{
    if (g_radar_zoomed_0069bf5c == 0) {
        float radius = g_startup_world_659c0c->radius_084;

        g_radar_zoomed_0069bf5c = 1;
        g_radar_map_scale_0069c0e0 = 13.0f;
        g_radar_inner_radius_0069c0d8 = CalcRangeDistance(W8_RANGE_TOUCH) + radius;
        g_radar_outer_radius_0069c0d4 = CalcRangeDistance(W8_RANGE_LONG) + radius;
        RefreshRadarMap();
        return;
    }
    float radius = g_startup_world_659c0c->radius_084;

    g_radar_zoomed_0069bf5c = 0;
    g_radar_map_scale_0069c0e0 = 2.0f;
    g_radar_inner_radius_0069c0d8 = radius;
    g_radar_outer_radius_0069c0d4 = CalcRangeDistance(W8_RANGE_EXTREME) + radius;
    RefreshRadarMap();
}

// FUNCTION: WIZ8 0x005a3410
void ZoomRadarMapIn(void)
{
    float radius = g_startup_world_659c0c->radius_084;

    g_radar_zoomed_0069bf5c = 1;
    g_radar_map_scale_0069c0e0 = 13.0f;
    g_radar_inner_radius_0069c0d8 = CalcRangeDistance(W8_RANGE_TOUCH) + radius;
    g_radar_outer_radius_0069c0d4 = CalcRangeDistance(W8_RANGE_LONG) + radius;
    RefreshRadarMap();
}

// FUNCTION: WIZ8 0x005a3470
void ZoomRadarMapOut(void)
{
    float radius = g_startup_world_659c0c->radius_084;

    g_radar_zoomed_0069bf5c = 0;
    g_radar_map_scale_0069c0e0 = 2.0f;
    g_radar_inner_radius_0069c0d8 = radius;
    g_radar_outer_radius_0069c0d4 = CalcRangeDistance(W8_RANGE_EXTREME) + radius;
    RefreshRadarMap();
}
