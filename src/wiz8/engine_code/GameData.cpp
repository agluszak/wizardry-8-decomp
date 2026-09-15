#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/engine_code/OctBuildTree.h"
#include "wiz8/engine_code/GameTimeAccumulator0043A910.h"
#include "wiz8/engine_code/BitArray.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/GDProp.h"
#include "wiz8/engine_code/3d.h"
#include "wiz8/float_constants.h"
#include "wiz8/sr_api.h"
#include "random.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <new>
#include "wiz8/engine_code/3d.h"

// GLOBAL: WIZ8 0x00652dac
W8LevelDataRecord* g_level_data_00652dac;

/*
 * Engine Code\GameData.cpp.
 *
 * The bits of the level the party is currently standing in. One global points
 * at that record, and the accessors below read and write single bits of the
 * flag word that leads it. Nothing here establishes what the bits mean, so
 * each is named for the bit it touches; three of them are read together by
 * bodies that do say something about the record.
 */

enum {
    W8_LEVEL_FLAG_0 = 0x001,
    W8_LEVEL_FLAG_4 = 0x010,
    W8_LEVEL_FLAG_5_TO_7 = 0x0e0,
    W8_LEVEL_FLAG_6 = 0x040,
    W8_LEVEL_FLAG_8 = 0x100,
    W8_LEVEL_FLAG_9 = 0x200
};

// GLOBAL: WIZ8 0x00652dba
unsigned char g_level_override_00652dba;
// GLOBAL: WIZ8 0x00652dce
unsigned char g_flag_00652dce;

/* Resolve one surface's three vertex indices through the active processed
   GameData vertex table.  The retail comparison is signed and accepts an index
   equal to vertex_count, so that historical boundary behavior is preserved. */
// FUNCTION: WIZ8 0x004214d0
unsigned char LoadSurfaceVertices004214D0(srVector3T<float>* output, const int* vertex_indices)
{
    short index = 0;
    do {
        if (g_octree_game_data_00652db0->vertex_count_20 < vertex_indices[index]) {
            return 0;
        }
        output[index] = g_octree_game_data_00652db0->vertices_24[vertex_indices[index]];
        ++index;
    } while (index < 3);
    return 1;
}

/* Ensure the shared game-data object exists, then run its update. */
// FUNCTION: WIZ8 0x0041F1F0
void UpdateSharedGameDataObject0041F1F0()
{
    if (g_game_time_accumulator_6598bc == 0) {
        g_game_time_accumulator_6598bc = new W8GameTimeAccumulator0043A910;
        if (g_game_time_accumulator_6598bc == 0) {
            return;
        }
    }
    g_game_time_accumulator_6598bc->Update();
}

// FUNCTION: WIZ8 0x0041F260
void UpdateGameDataRuntime0041F260()
{
    if (g_gd_camera_65a0f8 == 0) {
        g_gd_camera_65a0f8 = new GDCamera;
        if (g_gd_camera_65a0f8 == 0) {
            srAssertFail("gpGDCamera", "C:\\Projects\\Wizardry 8\\Engine Code\\GameData.cpp", 2739,
                         0);
        }
    }
    if (g_game_time_accumulator_6598bc == 0) {
        g_game_time_accumulator_6598bc = new W8GameTimeAccumulator0043A910;
        if (g_game_time_accumulator_6598bc == 0) {
            g_game_time_accumulator_6598bc->Update();
            return;
        }
    }
    if (g_flag_00652dce != 0) {
        ResumeSharedGameTimers00439CA0();
        g_flag_00652dce = 0;
    }
    g_game_time_accumulator_6598bc->Update();
}
/* 0x005EBB34: one float constant with two independent readings - the level
   vector's "no value" here, and Controls.cpp's own range start. Neither is
   proven, so it keeps its address. */
/* Run the buffered prop id list through TestProp and report the id of the
   last prop that hit; an empty list reports -1. */
// FUNCTION: WIZ8 0x0041c0d0
int W8GameData::TestPropSurfaces(int count, unsigned long* ids, W8OctreeTrace* trace,
                                 char skip_flag, char gate)
{
    int last_hit = -1;
    if (count == 0) {
        return -1;
    }
    do {
        if (TestProp(*ids, trace, skip_flag, gate) != 0) {
            last_hit = *ids;
        }
        ++ids;
        --count;
    } while (count != 0);
    return last_hit;
}

/* Point the shared surface/vertex arrays at one collidable prop's GDProp
   tables, ray-test them and put the arrays back. Without a pre-tree the prop
   comes out of the world's collidable list, its pathing representation is
   built on demand, and the ray is moved into prop space through the prop's
   position delta; on a hit the caller's record is reseeded from the start to
   the world-space contact. `gate` skips flag-4 props when set. */
// FUNCTION: WIZ8 0x0041c140
unsigned char W8GameData::TestProp(int prop_id, W8OctreeTrace* trace, char skip_flag, char gate)
{
    W8GDSurface* saved_surfaces = surfaces_38;
    srVector3T<float>* saved_vertices = vertices_24;
    unsigned char hit = 0;
    GDProp* gd_prop;
    W8Prop* prop;

    if (g_oct_pre_tree_659c74 == 0) {
        prop = *g_world->collidable_props->GetAt(prop_id);
        gd_prop = prop->m_gd_prop;
        prop->flags_1c |= 0x10;
        if (gd_prop == 0) {
            prop->BuildOrRefreshPathingRepresentation();
            gd_prop = prop->m_gd_prop;
        }
    } else {
        gd_prop = static_cast<GDProp*>(*g_oct_pre_tree_659c74->props_3b8->GetAt(prop_id));
    }
    surfaces_38 = gd_prop->m_pGDSurfaces;
    vertices_24 = gd_prop->m_pVertices;
    if (g_oct_pre_tree_659c74 == 0) {
        if (gate == 0 || (gd_prop->m_flags_00 & 4) == 0) {
            srVector3T<float> start = trace->start_00;
            srVector3T<float> end = trace->end_0c;
            srVector3T<float> delta;
            prop->GetDelta0044E130(&delta, &start);
            end.x -= delta.x;
            end.y -= delta.y;
            end.z -= delta.z;
            W8OctreeTrace prop_trace(&start, &end);
            hit = TestTraceResult(gd_prop->m_surface_count_14, 0, &prop_trace, skip_flag, 0);
            if (hit != 0) {
                end.x = prop_trace.end_0c.x + delta.x;
                end.y = prop_trace.end_0c.y + delta.y;
                end.z = prop_trace.end_0c.z + delta.z;
                trace->Reseed(&start, &end);
            }
        }
    } else {
        hit = TestTraceResult(gd_prop->m_surface_count_14, 0, trace, skip_flag, 0);
    }
    surfaces_38 = saved_surfaces;
    vertices_24 = saved_vertices;
    return hit;
}

/* Ray the trace record against `count` surfaces: all of surfaces_38 in order
   when `surface_ids` is null, else just the listed indexes. The value_88 flag
   admits flag-4 surfaces only, 0x1080-marked surfaces are skipped outright,
   `skip_flag` drops 0x8000-marked ones, and a nonzero positional_44 needs a
   passing `mode` roll. Each accepted surface's plane is tested both sides of
   the segment; a point-in-triangle pass on the contact keeps the closest hit,
   storing index_04 into value_54, the contact into end_0c and the hit
   distance into hit_limit_24/length_28. */
// FUNCTION: WIZ8 0x0041c330
char W8GameData::TestTraceResult(int count, unsigned long* surface_ids, W8OctreeTrace* trace,
                                 char skip_flag, int mode)
{
    char hit = 0;
    float best_x;
    float best_y;
    float best_z;

    value_54 = 0;
    if (count != 0) {
        unsigned long* id = surface_ids;
        int index = 0;
        int remaining = count;
        do {
            W8GDSurface* surface;
            if (surface_ids == 0) {
                surface = surfaces_38 + index;
            } else {
                surface = surfaces_38 + *id;
            }
            if (((value_88 == 0 || (surface->flags_00 & 4) != 0) &&
                 (surface->flags_00 & 0x1080) == 0 &&
                 (skip_flag == 0 || (surface->flags_00 & 0x8000) == 0)) &&
                (surface->positional_44 == 0 ||
                 (mode != -1 && (mode < 2 || static_cast<int>(surface->positional_44) < mode) &&
                  (mode != 1 ||
                   (static_cast<int>(surface->positional_44) < 100 &&
                    static_cast<int>(surface->positional_44) < static_cast<int>(Random(100)))))) &&
                surface->plane_24[0] * trace->step_18.x + surface->plane_24[1] * trace->step_18.y +
                        surface->plane_24[2] * trace->step_18.z <=
                    g_float_005ebb34) {
                float hit_distance = surface->plane_24[0] * trace->start_00.x +
                                     surface->plane_24[1] * trace->start_00.y +
                                     surface->plane_24[2] * trace->start_00.z +
                                     surface->plane_24[3];
                if (hit_distance <= trace->hit_limit_24 && g_float_005ebb34 < hit_distance) {
                    srVector3T<float> contact;
                    if (g_float_005ebb38 <= hit_distance) {
                        float back = surface->plane_24[0] * trace->end_0c.x +
                                     surface->plane_24[1] * trace->end_0c.y +
                                     surface->plane_24[2] * trace->end_0c.z + surface->plane_24[3];
                        if (g_float_005ebb38 <= back) {
                            goto next;
                        }
                        back = -back;
                        if (g_float_005ebb38 <= back || trace->hit_limit_24 < trace->length_28) {
                            hit_distance =
                                (hit_distance / (back + hit_distance)) * trace->length_28;
                            contact.x = trace->step_18.x * hit_distance + trace->start_00.x;
                            contact.y = trace->step_18.y * hit_distance + trace->start_00.y;
                            contact.z = trace->step_18.z * hit_distance + trace->start_00.z;
                        } else {
                            contact = trace->end_0c;
                            hit_distance = trace->length_28;
                        }
                    } else {
                        contact = trace->start_00;
                    }
                    srVector3T<float> vertices[3];
                    vertices[0] = vertices_24[surface->vertex_indices_18[0]];
                    vertices[1] = vertices_24[surface->vertex_indices_18[1]];
                    vertices[2] = vertices_24[surface->vertex_indices_18[2]];
                    if (PointInsideTriangle0046D530(vertices, surface->flags_00 & 3, &contact) !=
                            0 &&
                        hit_distance < trace->hit_limit_24) {
                        value_54 = surface->index_04;
                        hit = 1;
                        trace->hit_limit_24 = hit_distance;
                        best_y = contact.y;
                        best_z = contact.z;
                        best_x = contact.x;
                    }
                }
            }
        next:
            /* Retail verified at 0x0041C627: the cursor advances
               unconditionally even when `surface_ids` is null, so `++id` on a
               null pointer is the retail behavior rather than a defect. */
            ++id;
            ++index;
            --remaining;
        } while (remaining != 0);
        if (hit != 0) {
            trace->end_0c.x = best_x;
            trace->end_0c.y = best_y;
            trace->end_0c.z = best_z;
            trace->length_28 = trace->hit_limit_24;
            return hit;
        }
    }
    return 0;
}

/* Copy one four-byte handle over another. */
// FUNCTION: WIZ8 0x0041cf80
void CopyLevelDataHandle(unsigned long* destination, const unsigned long* source)
{
    *destination = *source;
}

/* VC6 vector constructor iterator, emitted for an ordinary array construction.
   This is compiler support, not an authored Wizardry callback wrapper. */
// LIBRARY: WIZ8 0x0041e880
// vector constructor iterator

// FUNCTION: WIZ8 0x0041ef50
void ResetInactiveLevelDataVectors0041EF50(void)
{
    W8LevelDataRecord* data = g_level_data_00652dac;

    if (data != 0 && (data->flags & W8_LEVEL_FLAG_0) == 0) {
        data->vector_40.SetZero();
        data->camera_forward_4c.SetZero();
        data->scaled_camera_forward_7c.SetZero();
        data->vector_64.SetZero();
        data->vector_70.SetZero();
        data->vector_a0.SetZero();
    }
}

/* Bit eight: read, cleared and set by three neighbouring bodies. */
// FUNCTION: WIZ8 0x0041efb0
unsigned int GetLevelDataFlag8(void)
{
    if (g_level_data_00652dac != 0) {
        return (g_level_data_00652dac->flags >> 8) & 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x0041efd0
void ClearLevelDataFlag8(void)
{
    if (g_level_data_00652dac != 0) {
        g_level_data_00652dac->flags &= ~W8_LEVEL_FLAG_8;
    }
}

// FUNCTION: WIZ8 0x0041efe0
void SetLevelDataFlag8(void)
{
    if (g_level_data_00652dac != 0) {
        g_level_data_00652dac->flags |= W8_LEVEL_FLAG_8;
    }
}

// FUNCTION: WIZ8 0x0041eff0
unsigned int GetLevelDataFlag9(void)
{
    if (g_level_data_00652dac != 0) {
        return (g_level_data_00652dac->flags >> 9) & 1;
    }
    return 0;
}

/* Bit four, read out of the low byte rather than the whole word. */
// FUNCTION: WIZ8 0x0041f070
unsigned int GetLevelDataFlag4(void)
{
    if (g_level_data_00652dac != 0) {
        return ((unsigned char)g_level_data_00652dac->flags >> 4) & 1;
    }
    return 0;
}

/* Bits five through seven together, cleared as a group. */
// FUNCTION: WIZ8 0x0041f0c0
void ClearLevelDataFlags5To7(void)
{
    if (g_level_data_00652dac != 0) {
        g_level_data_00652dac->flags &= ~W8_LEVEL_FLAG_5_TO_7;
    }
}

// FUNCTION: WIZ8 0x0041f140
unsigned int GetLevelDataFlag6(void)
{
    if (g_level_data_00652dac != 0) {
        return ((unsigned char)g_level_data_00652dac->flags >> 6) & 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x0041f160
void ClearLevelDataFlag6(void)
{
    if (g_level_data_00652dac != 0) {
        g_level_data_00652dac->flags &= ~W8_LEVEL_FLAG_6;
    }
}

/* Bit four again, but with a global override: with the bit down, the override
   being set is what withholds the answer. */
// FUNCTION: WIZ8 0x0041f090
int IsLevelDataFlag4EffectivelySet(void)
{
    if (g_level_data_00652dac == 0) {
        return 0;
    }
    if ((g_level_data_00652dac->flags & W8_LEVEL_FLAG_4) == 0 && g_level_override_00652dba != 0) {
        return 0;
    }
    return 1;
}

/* Whether the level has a live vector at 0x88: bit zero has to be up and at
   least one of the three floats has to differ from the default. */
// FUNCTION: WIZ8 0x0041f010
unsigned char HasLevelDataVector(void)
{
    if (g_level_data_00652dac == 0) {
        return 0;
    }
    if ((g_level_data_00652dac->flags & W8_LEVEL_FLAG_0) != 0 &&
        (g_level_data_00652dac->vector_88[0] != g_float_005ebb34 ||
         g_level_data_00652dac->vector_88[1] != g_float_005ebb34 ||
         g_level_data_00652dac->vector_88[2] != g_float_005ebb34)) {
        return 1;
    }
    return 0;
}

// GLOBAL: WIZ8 0x00652db4
W8EnvironRecord* g_environ_00652DB4;

/* The camera-sway mode halves navigator gravity, mirrors it into the active
   environment record and swaps the camera forward scale; the flag guards both
   transitions so repeated triggers are idempotent. */
// FUNCTION: WIZ8 0x0041a960
void BeginCameraSway0041A960(void)
{
    if (g_camera_sway_active_652da4) {
        return;
    }
    g_camera_forward_scale_603ab4 = g_camera_level_forward_scale_603aac;
    g_navigator_gravity_00603acc = 93.75f;
    if (g_environ_00652DB4 != 0) {
        g_environ_00652DB4->value_14 = -93.75f;
    }
    g_camera_sway_active_652da4 = 1;
}

// FUNCTION: WIZ8 0x0041a9a0
void EndCameraSway0041A9A0(void)
{
    if (!g_camera_sway_active_652da4) {
        return;
    }
    g_camera_forward_scale_603ab4 = g_camera_default_forward_scale_603ab0;
    g_navigator_gravity_00603acc = 187.5f;
    if (g_environ_00652DB4 != 0) {
        g_environ_00652DB4->value_14 = -187.5f;
    }
    g_camera_sway_active_652da4 = 0;
}

// FUNCTION: WIZ8 0x0041AA40
void ResetCurrentEnvironment0041AA40(void)
{
    if (g_environ_00652DB4 != 0) {
        if (g_octree_game_data_00652db0 != 0 && g_octree_game_data_00652db0->environs_84 != 0) {
            g_environ_00652DB4 = g_octree_game_data_00652db0->environs_84[0];
        }
        g_environ_00652DB4->value_24 = 0;
        g_environ_00652DB4->value_28 = 0;
        g_environ_00652DB4->value_2c = 0;
        if (g_environment_load_flag_00603ad0 != 0) {
            g_environ_00652DB4->value_20 = 1.0f;
        }
        g_environment_load_flag_00603ad0 = g_environment_load_flag_00603ad0 == 0;
        if (g_environment_load_flag_00603ad0 == 0) {
            g_level_override_00652dba = 0;
        }
        return;
    }
    g_environment_load_flag_00603ad0 = 0;
    g_level_override_00652dba = 0;
}

// FUNCTION: WIZ8 0x0041F0D0
void ResetLevelDataVectors0041F0D0(void)
{
    if (g_level_data_00652dac != 0) {
        g_level_data_00652dac->flags |= 0x40;
        if ((g_level_data_00652dac->flags & 1) == 0) {
            g_level_data_00652dac->vector_40.SetZero();
            g_level_data_00652dac->camera_forward_4c.SetZero();
            g_level_data_00652dac->vector_64.SetZero();
            g_level_data_00652dac->vector_70.SetZero();
            g_level_data_00652dac->scaled_camera_forward_7c.SetZero();
            g_level_data_00652dac->vector_a0.SetZero();
        }
        g_level_data_00652dac->flags &= ~0x100U;
    }
}

/* Camera facade, move timer and the party placement entry. */

// GLOBAL: WIZ8 0x00652da7
unsigned char g_flag_00652da7;
// GLOBAL: WIZ8 0x005ebc18
const double g_double_005ebc18 = 3.141592653589793;
// GLOBAL: WIZ8 0x005ebcf0
const float g_float_005ebcf0 = 57.295784f;
// GLOBAL: WIZ8 0x005ebca0
const float g_float_005ebca0 = 6.0f;
// GLOBAL: WIZ8 0x00652940
srVector3T<float> g_origin_652940;

// FUNCTION: WIZ8 0x00420b40
float MoveTimer(int value)
{
    if (g_game_time_accumulator_6598bc == 0) {
        g_game_time_accumulator_6598bc = new W8GameTimeAccumulator0043A910;
        if (g_game_time_accumulator_6598bc == 0) {
            return g_float_005ebb34;
        }
    }
    if (g_flag_00652dce != 0) {
        if ((value == 8 && g_current_screen_state.id == 7) || value == 4) {
            ResumeSharedGameTimers00439CA0();
            g_flag_00652dce = 0;
        } else {
            return g_float_005ebb34;
        }
    }
    if (value == 1) {
        PauseSharedGameTimers00439BC0();
        g_flag_00652dce = 1;
    }
    return g_game_time_accumulator_6598bc->GetValue28();
}

// FUNCTION: WIZ8 0x00420D40
srCamera* CreateOrSetGameCamera(srNode* parent, srCamera* camera)
{
    if (g_gd_camera_65a0f8 == 0) {
        g_gd_camera_65a0f8 = new GDCamera();
    }
    return g_gd_camera_65a0f8->CreateOrAttachCamera(parent, camera);
}

// FUNCTION: WIZ8 0x00420DC0
float GetCameraYawInDegrees()
{
    return g_gd_camera_65a0f8->m_yaw * g_float_005ebcf0;
}

// FUNCTION: WIZ8 0x00420DD0
float GetCameraYawRadians()
{
    return g_gd_camera_65a0f8->m_yaw;
}

// FUNCTION: WIZ8 0x00420DE0
float GetCameraPitchInDegrees()
{
    return g_gd_camera_65a0f8->m_pitch * g_float_005ebcf0;
}

// FUNCTION: WIZ8 0x00420DF0
float GetCameraPitchRadians()
{
    return g_gd_camera_65a0f8->m_pitch;
}

// FUNCTION: WIZ8 0x00420E00
void BeginManualCameraControl()
{
    g_gd_camera_65a0f8->SetManualControlActive(1);
}

// FUNCTION: WIZ8 0x00420F70
void LevelCamera()
{
    g_gd_camera_65a0f8->BeginLeveling();
    g_flag_00652da7 = 0;
}

// FUNCTION: WIZ8 0x00420FD0
void TurnCameraToDegrees(float degrees)
{
    double scale = g_double_005ebc18 * g_float_005ebcf8;
    g_gd_camera_65a0f8->BeginOrientationTransition(0.0f, (float)(scale * degrees), 0);
}

// FUNCTION: WIZ8 0x00421000
void SetCameraYawDegrees(float degrees)
{
    double scale = g_double_005ebc18 * g_float_005ebcf8;
    g_gd_camera_65a0f8->SetOrientationImmediate(0.0f, (float)(scale * degrees));
}

// FUNCTION: WIZ8 0x00421030
void ApplyCameraRotation(srMatrix3T<float>* rotation)
{
    g_gd_camera_65a0f8->ApplyRotationMatrix(rotation, g_level_data_00652dac);
}

/* Two whole-body reads through the pointer at 0x0065A0F8. The first hands back
   the twelve bytes at 0x8C as one block; the second converts the float at 0x04
   from radians to degrees and truncates it through the CRT's _ftol, which the
   original reaches as a tail jump because the conversion is the whole return
   value. The scale is one ULP above the float nearest 180/pi, so the original
   spelled it as a decimal literal rather than computing it from a pi constant;
   the literal here is the shortest decimal that reproduces the stored bytes. */
// FUNCTION: WIZ8 0x00421070
void GetCameraPosition(srVector3T<float>* position)
{
    *position = g_gd_camera_65a0f8->m_position_08c;
}

/* Zero the two six-float CamPos angle records, then store the live yaw in the
   first and the live pitch in the second. GetWorldCameraState passes the yaw
   record at +0x24 as angle and the pitch record at +0x0c as pitch. */
// FUNCTION: WIZ8 0x004213A0
void GetCameraOrientation(float* angle, float* pitch)
{
    int i;

    for (i = 0; i < 6; ++i) {
        angle[i] = 0.0f;
    }
    for (i = 0; i < 6; ++i) {
        pitch[i] = 0.0f;
    }
    *angle = g_gd_camera_65a0f8->m_yaw;
    *pitch = g_gd_camera_65a0f8->m_pitch;
}

// FUNCTION: WIZ8 0x004213E0
void SetCameraOrientation(float* angle, float* pitch, srMatrix3T<float>* rotation)
{
    g_gd_camera_65a0f8->SetYaw(*angle);
    g_gd_camera_65a0f8->SetPitch(*pitch);
    *angle = g_gd_camera_65a0f8->m_yaw;
    *pitch = g_gd_camera_65a0f8->m_pitch;
    if (rotation != 0) {
        g_gd_camera_65a0f8->GetRotationMatrix(rotation);
    }
}

// FUNCTION: WIZ8 0x00421550
int GetCameraYawDegrees(void)
{
    return (int)(g_gd_camera_65a0f8->m_yaw * 57.295784f);
}

/* Mark the renderer ready and copy the point into the game camera when it
   sits anywhere but the origin. */
// FUNCTION: WIZ8 0x00421090
void PlacePartyAtPoint(const srVector3T<float>* point)
{
    srVector3T<float> delta = *point - g_origin_652940;
    if (sqrtf(DotProduct(delta, delta)) != g_zero_005ebb40) {
        MarkRendererReady();
        g_gd_camera_65a0f8->m_position_08c.x = point->x;
        g_gd_camera_65a0f8->m_position_08c.y = point->y;
        g_gd_camera_65a0f8->m_position_08c.z = point->z;
    }
}
/* Hand the level's pending real/frame elapsed times to the caller, fold them
   into the session accumulators, clear the pending pair, and report whether
   either was above the camera-transition epsilon. */
// FUNCTION: WIZ8 0x0041f170
unsigned char ConsumeLevelElapsedTime0041F170(float* real_elapsed, float* frame_elapsed)
{
    W8LevelDataRecord* record = g_level_data_00652dac;
    unsigned char elapsed = 0;
    if (record != 0) {
        *real_elapsed = record->real_elapsed_24;
        *frame_elapsed = record->frame_elapsed_28;
        elapsed = record->real_elapsed_24 > g_camera_transition_epsilon_005ebc84 ||
                  record->frame_elapsed_28 > g_camera_transition_epsilon_005ebc84;
        record->frame_elapsed_28 = 0.0f;
        record->real_elapsed_24 = 0.0f;
        g_status_685170.real_elapsed_2391 += *real_elapsed;
        g_status_685170.frame_elapsed_2395 += *frame_elapsed;
    }
    return elapsed;
}
