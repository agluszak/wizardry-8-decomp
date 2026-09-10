#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/OctBuildTree.h"
#include "wiz8/engine_code/Object0043A910.h"
#include "wiz8/engine_code/BitArray.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/engine_code/Trigger.h"
#include "wiz8/screen_state.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/float_constants.h"
#include "wiz8/sr_api.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <new>

extern "C" {
// GLOBAL: WIZ8 0x005ec1a8
float g_float_005ec1a8 = -0.3333333432674408f;
// GLOBAL: WIZ8 0x005ebc58
float g_float_005ebc58 = 1.0000000116860974e-07f;
// GLOBAL: WIZ8 0x005ec028
float g_float_005ec028 = 1.0099999904632568f;
// GLOBAL: WIZ8 0x005ec1a0
float g_float_005ec1a0 = 0.9959999918937683f;
}

// GLOBAL: WIZ8 0x00652dac
W8LevelDataRecord* g_level_data_00652dac;
// GLOBAL: WIZ8 0x00659a58
int g_integrated_trigger_count_00659a58;

// GLOBAL
float g_float_00603ac8;
// GLOBAL
float g_float_00603aac;
// GLOBAL: WIZ8 0x00603ab8
float g_float_00603ab8 = 0.30000001192092896f;
// GLOBAL: WIZ8 0x00603abc
float g_float_00603abc = 112.5f;
// GLOBAL
float g_float_005ebc98;

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

extern unsigned char g_level_override_00652dba;
extern unsigned char g_environment_load_flag_00603ad0;
// GLOBAL: WIZ8 0x00652dba
unsigned char g_level_override_00652dba;
// GLOBAL: WIZ8 0x00652dce
unsigned char g_flag_00652dce;
extern const float g_world_scale_005ebc40;
extern float g_path_endpoint_scale_005ec1a4;
// GLOBAL: WIZ8 0x005ec1a4
float g_path_endpoint_scale_005ec1a4 = 0.9900000095367432f;

/* Resolve one surface's three vertex indices through the active processed
   GameData vertex table.  The retail comparison is signed and accepts an index
   equal to vertex_count, so that historical boundary behavior is preserved. */
// FUNCTION: WIZ8 0x004214d0
unsigned char LoadSurfaceVertices004214D0(
    srVector3T<float>* output, const int* vertex_indices)
{
    short index = 0;
    do {
        if (g_octree_game_data_00652db0->vertex_count_20 <
            vertex_indices[index]) {
            return 0;
        }
        output[index] = g_octree_game_data_00652db0
                            ->vertices_24[vertex_indices[index]];
        ++index;
    } while (index < 3);
    return 1;
}

/* Rebuild one indexed surface plane and derive the runtime classification
   carried by the level-geometry record. Bit 0x80 requests dominant-axis
   selection; bit 4 is the walkable slope classification. */
// FUNCTION: WIZ8 0x004498c0
void ClassifySurfacePlane004498C0(
    const srVector3T<float>* vertices, W8GDSurface* surface)
{
    BuildTrianglePlane00449A40(
        surface->plane_24,
        &vertices[surface->vertex_indices_18[0]],
        &vertices[surface->vertex_indices_18[1]],
        &vertices[surface->vertex_indices_18[2]]);

    unsigned int flags = surface->flags_00;
    if ((flags & 0x80) != 0) {
        float largest = g_float_005ebb34;
        unsigned int dominant_axis = 0;
        for (int axis = 0; axis < 3; ++axis) {
            float magnitude = (float)fabs(surface->plane_24[axis]);
            if (largest < magnitude) {
                largest = magnitude;
                dominant_axis = axis;
            }
        }
        flags |= dominant_axis;
        surface->flags_00 = flags;
    }

    if ((surface->flags_00 & 4) != 0) {
        surface->flags_00 |= 0x40;
    }

    float upper_value = g_float_005ebb38;
    if (g_float_005ebc7c < surface->plane_24[1]) {
        if ((surface->flags_00 & 4) == 0 &&
            g_float_005ec1a0 < surface->plane_24[1]) {
            surface->flags_00 |= 4;
            surface->value_48 = g_float_005ebb38;
        }
        if (surface->value_48 < g_float_005ebb34) {
            surface->flags_00 |= 0x20;
            surface->value_48 = g_float_005ebb34;
        }
    }
    else if (surface->value_40 < g_float_005ec028 &&
             g_path_endpoint_scale_005ec1a4 < surface->value_40 &&
             (surface->flags_00 & 4) != 0) {
        surface->value_40 = 0.1f;
    }

    flags = surface->flags_00;
    surface->value_40 *= g_world_scale_005ebc40;
    if ((flags & 4) == 0) {
        surface->value_48 = g_float_005ebb34;
    }
    else if (surface->value_48 < g_float_005ebc58 &&
             (flags & 0x20) == 0) {
        if (surface->plane_24[1] <= g_float_005ebccc) {
            upper_value = surface->plane_24[1];
        }
        surface->value_48 = upper_value;
    }
    surface->flags_00 = flags & ~8U;
}

/* Build the normalized plane shared by level geometry and GDProp collision
   surfaces. The three determinant terms are accumulated cyclically so the
   winding, normal direction and degenerate-triangle division all remain the
   retail behavior. */
// FUNCTION: WIZ8 0x00449a40
void BuildTrianglePlane00449A40(
    float* plane,
    const srVector3T<float>* first,
    const srVector3T<float>* second,
    const srVector3T<float>* third)
{
    srVector3T<float> vertices[3];
    short index = 2;

    vertices[0] = *first;
    vertices[1] = *second;
    vertices[2] = *third;
    plane[0] = 0.0f;
    plane[1] = 0.0f;
    plane[2] = 0.0f;
    plane[3] = 0.0f;

    do {
        short next = (short)((index - 1) % 3);
        short following = (short)(index % 3);
        srVector3T<float>& vertex = vertices[index - 2];

        plane[0] += vertex.y *
                    (vertices[next].z - vertices[following].z);
        plane[1] += vertex.z *
                    (vertices[next].x - vertices[following].x);
        plane[2] += vertex.x *
                    (vertices[next].y - vertices[following].y);
        ++index;
    } while ((short)(index - 2) < 3);

    float scale = g_float_005ebb38 /
                  (float)sqrt(
                      plane[0] * plane[0] + plane[1] * plane[1] +
                      plane[2] * plane[2]);
    plane[0] *= scale;
    plane[1] *= scale;
    plane[2] *= scale;

    float distances[3];
    for (int vertex_index = 0; vertex_index != 3; ++vertex_index) {
        distances[vertex_index] =
            plane[0] * vertices[vertex_index].x +
            plane[1] * vertices[vertex_index].y +
            plane[2] * vertices[vertex_index].z;
    }
    plane[3] =
        (distances[0] + distances[1] + distances[2]) * g_float_005ec1a8;
}

/* Build the processed level's spatial index once and publish every surface
   from its primary 0x4c-byte bank.  The constructor expands only local bounds,
   leaving the serialized GameData limits unchanged. */
// FUNCTION: WIZ8 0x004497c0
unsigned char InitializeGameData004497C0(W8GameData* game_data)
{
    if (game_data == 0) {
        return 0;
    }

    srVector3T<float> minimum = game_data->minimum_08;
    srVector3T<float> maximum = game_data->maximum_14;
    if (game_data->geometry_index_00 == 0) {
        game_data->geometry_index_00 = new W8OctBuildTree00446390(
            2000.0f, &minimum, &maximum, 0x40, 0);
    }

    for (int index = 0; index < game_data->surface_count_28; ++index) {
        if (game_data->geometry_index_00->InsertSurface00446820(
                &game_data->surfaces_38[index], 3) == 0) {
            return 0;
        }
    }
    return 1;
}

// FUNCTION: WIZ8 0x00448310
void W8GameData::AddTriggerPlane(
    const srVector3T<float>* trigger_vertices, Trigger* trigger)
{
    int trigger_index = 0;
    int index;
    if (positional_04 != 0) {
        if (trigger_table_50 == 0) {
            g_integrated_trigger_count_00659a58 = 0;
            trigger_table_50 = static_cast<Trigger**>(
                malloc(total_surface_count_44 * sizeof(Trigger*) + 4));
            if (trigger_table_50 == 0) {
                srAssertFail("m_ppTriggers",
                    "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                    0x256, "AddTriggerPlane: Couldn't allocate trigger array.");
            }
        }
        if (g_integrated_trigger_count_00659a58 >= total_surface_count_44) {
            srAssertFail("iTriggerCount < m_iNumTriggers",
                "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                0x259, "AddTriggerPlane: Too many triggers.");
        }
        trigger_table_50[g_integrated_trigger_count_00659a58++] = trigger;
        return;
    }

    if (overflow_surfaces_48 == 0) {
        overflow_surfaces_48 = static_cast<W8GDSurface*>(malloc(500 * sizeof(W8GDSurface)));
        if (overflow_surfaces_48 == 0) {
            srAssertFail("m_pTrigSurfaces",
                "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                0x263, "AddTriggerPlane: Couldn't allocate trigger surfaces.");
        }
        overflow_vertices_4c = static_cast<srVector3T<float>*>(
            srHeap.allocate(1000 * sizeof(srVector3T<float>)));
        if (overflow_vertices_4c == 0) {
            srAssertFail("m_pTrigVertices",
                "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                0x265, "AddTriggerPlane: Couldn't allocate trigger vertices.");
        }
        trigger_table_50 = static_cast<Trigger**>(malloc(500 * sizeof(Trigger*)));
        if (trigger_table_50 == 0) {
            srAssertFail("m_ppTriggers",
                "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                0x267, "AddTriggerPlane: Couldn't allocate trigger array.");
        }
        overflow_surface_count_3c = 0;
        overflow_vertex_count_40 = 0;
        total_surface_count_44 = 0;
    }
    if (overflow_surface_count_3c >= 500) {
        srAssertFail("m_iNumTrigSurfaces < MAX_TRIG_SURFACES",
            "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
            0x26c, 0);
    }

    for (index = 0; index < total_surface_count_44 && trigger_index == 0; ++index) {
        if (trigger_table_50[index] == trigger) {
            trigger_index = index;
        }
    }
    if (trigger_index == 0) {
        trigger_index = total_surface_count_44++;
        trigger_table_50[trigger_index] = trigger;
    }
    for (index = 0; index < 4; ++index) {
        overflow_vertices_4c[overflow_vertex_count_40++] = trigger_vertices[index];
    }

    W8GDSurface* surface = &overflow_surfaces_48[overflow_surface_count_3c];
    surface->flags_00 = 0x80;
    surface->index_04 = surface_count_28 + overflow_surface_count_3c;
    surface->trigger_index_08 = trigger_index;
    surface->value_40 = 1.1f;
    surface->vertex_indices_18[0] = overflow_vertex_count_40 - 4;
    surface->vertex_indices_18[1] = overflow_vertex_count_40 - 3;
    surface->vertex_indices_18[2] = overflow_vertex_count_40 - 2;
    ClassifySurfacePlane004498C0(overflow_vertices_4c, surface);
    for (index = 0; index < 3; ++index) {
        surface->vertex_indices_18[index] += vertex_count_20;
    }
    surface->positional_0c = -1;
    surface->positional_10 = -1;
    surface->positional_14 = -1;
    surface->value_38 = 0;
    ++overflow_surface_count_3c;

    surface = &overflow_surfaces_48[overflow_surface_count_3c];
    surface->flags_00 = 0x80;
    surface->index_04 = surface_count_28 + overflow_surface_count_3c;
    surface->trigger_index_08 = trigger_index;
    surface->value_40 = 1.1f;
    surface->vertex_indices_18[0] = overflow_vertex_count_40 - 2;
    surface->vertex_indices_18[1] = overflow_vertex_count_40 - 1;
    surface->vertex_indices_18[2] = overflow_vertex_count_40 - 4;
    ClassifySurfacePlane004498C0(overflow_vertices_4c, surface);
    for (index = 0; index < 3; ++index) {
        surface->vertex_indices_18[index] += vertex_count_20;
    }
    surface->positional_0c = -1;
    surface->positional_10 = -1;
    surface->positional_14 = -1;
    surface->value_38 = 0;
    ++overflow_surface_count_3c;
}

// FUNCTION: WIZ8 0x00448840
void W8GameData::IntegrateTriggers()
{
    if (overflow_vertex_count_40 == 0) {
        return;
    }

    int combined_vertex_count = vertex_count_20 + overflow_vertex_count_40;
    srVector3T<float>* combined_vertices =
        static_cast<srVector3T<float>*>(srHeap.allocate(
            (combined_vertex_count + 1) * sizeof(srVector3T<float>)));
    if (combined_vertices == 0) {
        srAssertFail(
            "pNewVertices",
            "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
            0x31b,
            "IntegrateTriggers: Couldn't allocate new vertex array.");
    }
    memcpy(combined_vertices, vertices_24,
           vertex_count_20 * sizeof(srVector3T<float>));
    memcpy(combined_vertices + vertex_count_20, overflow_vertices_4c,
           overflow_vertex_count_40 * sizeof(srVector3T<float>));
    vertex_count_20 = combined_vertex_count;
    srHeap.free(vertices_24);
    srHeap.free(overflow_vertices_4c);
    integrated_surface_count_34 = overflow_surface_count_3c;
    vertices_24 = combined_vertices;
    overflow_vertices_4c = 0;
    overflow_vertex_count_40 = 0;

    W8GDSurface* new_surfaces = static_cast<W8GDSurface*>(
        malloc((overflow_surface_count_3c + 1) * sizeof(W8GDSurface)));
    if (new_surfaces == 0) {
        srAssertFail(
            "pNewSurfaces",
            "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
            0x32c,
            "IntegrateTriggers: Couldn't allocate new surface array.");
    }
    memcpy(new_surfaces, overflow_surfaces_48,
           overflow_surface_count_3c * sizeof(W8GDSurface));
    free(overflow_surfaces_48);
    overflow_surfaces_48 = new_surfaces;

    int end = surface_count_28 + overflow_surface_count_3c;
    for (int index = surface_count_28; index < end; ++index) {
        W8GDSurface* surface = index < surface_count_28
            ? &surfaces_38[index]
            : &overflow_surfaces_48[index - surface_count_28];
        geometry_index_00->InsertSurface00446820(surface, 3);
    }
    bits_58 = new BitArray(total_surface_count_44);
    bits_5c = new BitArray(total_surface_count_44);
}

/* Ensure the shared game-data object exists, then run its update. */
// FUNCTION: WIZ8 0x0041F1F0
void Function41F1F0()
{
    if (g_object_6598bc == 0) {
        g_object_6598bc = new W8Object0043A910;
        if (g_object_6598bc == 0) {
            return;
        }
    }
    g_object_6598bc->Update();
}

// FUNCTION: WIZ8 0x0041F260
void Function41F260()
{
    if (g_gd_camera_65a0f8 == 0) {
        g_gd_camera_65a0f8 = new GDCamera;
        if (g_gd_camera_65a0f8 == 0) {
            srAssertFail(
                "gpGDCamera",
                "C:\\Projects\\Wizardry 8\\Engine Code\\GameData.cpp",
                2739,
                0);
        }
    }
    if (g_object_6598bc == 0) {
        g_object_6598bc = new W8Object0043A910;
        if (g_object_6598bc == 0) {
            g_object_6598bc->Update();
            return;
        }
    }
    if (g_flag_00652dce != 0) {
        Function439CA0();
        g_flag_00652dce = 0;
    }
    g_object_6598bc->Update();
}
/* 0x005EBB34: one float constant with two independent readings - the level
   vector's "no value" here, and Controls.cpp's own range start. Neither is
   proven, so it keeps its address. */
/* Copy one four-byte handle over another. */
// FUNCTION: WIZ8 0x0041cf80
void CopyLevelDataHandle(int* destination, const int* source)
{
    *destination = *source;
}

/* VC6 vector constructor iterator, emitted for an ordinary array construction.
   This is compiler support, not an authored Wizardry callback wrapper. */
// LIBRARY: WIZ8 0x0041e880
// vector constructor iterator

// FUNCTION: WIZ8 0x0041ef50
void Function41EF50(void)
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
    if ((g_level_data_00652dac->flags & W8_LEVEL_FLAG_4) == 0 &&
        g_level_override_00652dba != 0) {
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

extern void Function449240(int handle);
extern void Function497690(int channel, const char* message);
extern W8EnvironRecord* g_environ_00652DB4;
// GLOBAL: WIZ8 0x00652db4
W8EnvironRecord* g_environ_00652DB4;

// FUNCTION: WIZ8 0x0041AA40
void ResetCurrentEnvironment0041AA40(void)
{
    if (g_environ_00652DB4 != 0) {
        if (g_octree_game_data_00652db0 != 0 &&
            g_octree_game_data_00652db0->environs_84 != 0) {
            g_environ_00652DB4 = g_octree_game_data_00652db0->environs_84[0];
        }
        g_environ_00652DB4->value_24 = 0;
        g_environ_00652DB4->value_28 = 0;
        g_environ_00652DB4->value_2c = 0;
        if (g_environment_load_flag_00603ad0 != 0) {
            g_environ_00652DB4->value_20 = 1.0f;
        }
        g_environment_load_flag_00603ad0 =
            g_environment_load_flag_00603ad0 == 0;
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

/* Builds the processed game-data record in place: zeroed storage, bound
   extremes, the shared engine-time object on first use, a default
   environment bank, and the previous level-data teardown. The zero stores
   below follow the image order rather than field order. */
// FUNCTION: WIZ8 0x00449010
W8GameData::W8GameData(int handle, void* parent)
{
    geometry_index_00 = 0;
    positional_04 = 0;
    vertex_count_20 = 0;
    vertices_24 = 0;
    integrated_surface_count_34 = 0;
    *(int*)&positional_2c[4] = 0;
    *(int*)&positional_2c[0] = 0;
    surface_count_28 = 0;
    surfaces_38 = 0;
    overflow_surfaces_48 = 0;
    overflow_vertices_4c = 0;
    trigger_table_50 = 0;
    overflow_surface_count_3c = 0;
    overflow_vertex_count_40 = 0;
    total_surface_count_44 = 0;
    bits_58 = 0;
    bits_5c = 0;
    value_54 = 0;
    value_60 = 0;
    block_64 = 0;
    value_68 = 0;
    block_6c = 0;
    value_70 = 0;
    block_74 = 0;
    count_78 = 0;
    array_7c = 0;
    environ_count_80 = 0;
    environs_84 = 0;
    value_88 = 0;
    minimum_08.Set(1.0e8f, 1.0e8f, 1.0e8f);
    maximum_14.Set(-1.0e8f, -1.0e8f, -1.0e8f);
    if (parent == 0) {
        MoveTimer(4);
        if (g_object_6598bc == 0) {
            g_object_6598bc = new W8Object0043A910();
        }
    }
    if (handle != 0) {
        Function449240(handle);
    }
    if (g_environ_00652DB4 != 0) {
        delete g_environ_00652DB4;
    }
    if (environ_count_80 == 0) {
        environ_count_80 = 1;
        environs_84 = static_cast<W8EnvironRecord**>(malloc(0x28));
        if (environs_84 == 0) {
            srAssertFail(
                "m_ppEnvirons",
                "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                0x441, 0);
        }
        for (int index = 0; index < 10; ++index) {
            environs_84[index] = 0;
        }
        W8EnvironRecord* environ_record = new W8EnvironRecord();
        if (environ_record == 0) {
            environ_record = 0;
        }
        else {
            environ_record->value_00 = 0;
            environ_record->value_04 = 0;
            environ_record->value_08 = 0;
            environ_record->value_10 = 0;
            environ_record->value_14 = -g_navigator_gravity_00603acc;
            environ_record->value_18 = 0;
            environ_record->value_1c = 0.05f;
            environ_record->value_20 = 1.0f;
            environ_record->value_24 = 0;
            environ_record->value_28 = 0;
            environ_record->value_2c = 0;
            environ_record->value_30 = g_float_00603ac8;
            environ_record->value_34 = g_float_00603aac * g_float_005ebc98;
            environ_record->value_38 = g_float_00603ab8;
            environ_record->value_3c = g_float_00603abc;
            environ_record->value_40 = 1.0f;
        }
        environs_84[0] = environ_record;
    }
    W8LevelDataRecord* old_level = g_level_data_00652dac;
    g_environ_00652DB4 = environs_84[0];
    if (old_level != 0) {
        /* The embedded timer's most-derived type is unrecovered, so a plain
           delete would dispatch the wrong destructor; the base teardown plus
           deallocation below is the entire model until that type is known. */
        ((W8GameTimer*)((unsigned char*)old_level + 0xC4))->~W8GameTimer();
        delete old_level;
        g_level_data_00652dac = 0;
    }
    g_octree_game_data_00652db0 = this;
}

/* Tears down owned storage: the geometry index, heap and malloc'd banks,
   both bit sets, the counted pointer blocks, and the environment bank. */
// FUNCTION: WIZ8 0x00449BB0
W8GameData::~W8GameData()
{
    int index;

    Function41A9E0();
    if (geometry_index_00 != 0) {
        delete geometry_index_00;
    }
    if (vertices_24 != 0) {
        srHeap.free(vertices_24);
    }
    if (surfaces_38 != 0) {
        free(surfaces_38);
    }
    if (bits_58 != 0) {
        delete bits_58;
    }
    if (bits_5c != 0) {
        delete bits_5c;
    }
    if (array_7c != 0) {
        if (count_78 > 0) {
            index = 0;
            do {
                if (array_7c[index] != 0) {
                    free(array_7c[index]);
                }
                ++index;
            } while (index < count_78);
        }
        free(array_7c);
        count_78 = 0;
        array_7c = 0;
    }
    if (block_64 != 0) {
        free(block_64);
        block_64 = 0;
        value_60 = 0;
    }
    if (block_74 != 0) {
        free(block_74);
        block_74 = 0;
        value_70 = 0;
    }
    if (block_6c != 0) {
        free(block_6c);
        block_6c = 0;
        value_68 = 0;
    }
    if (trigger_table_50 != 0) {
        free(trigger_table_50);
        trigger_table_50 = 0;
    }
    total_surface_count_44 = 0;
    if (environs_84 != 0) {
        if (environ_count_80 > 0) {
            index = 0;
            do {
                if (environs_84[index] != 0) {
                    delete environs_84[index];
                }
                ++index;
            } while (index < environ_count_80);
        }
        free(environs_84);
        environs_84 = 0;
    }
    g_octree_game_data_00652db0 = 0;
}

/* Opens a game-data file, builds its record, and pulls the polygon and
   vertex banks through the record reader. */
// FUNCTION: WIZ8 0x00447570
W8GameData* ReadGameData00447570(const char* path, void* parent)
{
    HANDLE file = CreateFileA(
        path, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    W8GameData* game_data;
    unsigned char got_polygons;
    unsigned char got_vertices;

    if (file == INVALID_HANDLE_VALUE) {
        return 0;
    }
    game_data = new W8GameData(0, parent);
    if (game_data == 0) {
        srAssertFail(
            "pGameData",
            "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
            0xa7, 0);
    }
    got_polygons = game_data->Function447660(file, 0);
    got_vertices = game_data->Function447660(file, 1);
    if (got_vertices == 0 && got_polygons == 0) {
        Function497690(7, "ReadGameData: No polygons or vertices in GameData!\n");
    }
    CloseHandle(file);
    return game_data;
}

/* Camera facade, move timer and the party placement entry. */

// GLOBAL: WIZ8 0x00652da7
unsigned char g_flag_00652da7;
// GLOBAL: WIZ8 0x005ebc18
const double g_double_005ebc18 = 3.141592653589793;
// GLOBAL: WIZ8 0x00652940
float g_origin_652940[3] = { 0.0f, 0.0f, 0.0f };

// FUNCTION: WIZ8 0x00420b40
float MoveTimer(int value)
{
    if (g_object_6598bc == 0) {
        g_object_6598bc = new W8Object0043A910;
        if (g_object_6598bc == 0) {
            return g_float_005ebb34;
        }
    }
    if (g_flag_00652dce != 0) {
        if ((value == 8 && g_current_screen_state.id == 7) || value == 4) {
            Function439CA0();
            g_flag_00652dce = 0;
        }
        else {
            return g_float_005ebb34;
        }
    }
    if (value == 1) {
        Function439BC0();
        g_flag_00652dce = 1;
    }
    return g_object_6598bc->GetValue28();
}

// FUNCTION: WIZ8 0x00420D40
srCamera* CreateOrSetGameCamera(
    srNode* parent, srCamera* camera)
{
    if (g_gd_camera_65a0f8 == 0) {
        g_gd_camera_65a0f8 = new GDCamera();
    }
    return g_gd_camera_65a0f8->CreateOrAttachCamera(parent, camera);
}

// FUNCTION: WIZ8 0x00420DD0
float GetCameraYawRadians()
{
    return g_gd_camera_65a0f8->m_yaw;
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
    g_gd_camera_65a0f8->BeginOrientationTransition(
        0.0f, (float)(scale * degrees), 0);
}

// FUNCTION: WIZ8 0x00421000
void SetCameraYawDegrees(float degrees)
{
    double scale = g_double_005ebc18 * g_float_005ebcf8;
    g_gd_camera_65a0f8->SetOrientationImmediate(
        0.0f, (float)(scale * degrees));
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

// FUNCTION: WIZ8 0x00421100
void Function421100(float distance, srVector3T<float>* output)
{
    srVector3T<float> result = *output;
    g_gd_camera_65a0f8->GetForwardPoint(distance, &result);
    *output = result;
}

// FUNCTION: WIZ8 0x00421150
void Function421150(float distance, srVector3T<float>* output)
{
    g_gd_camera_65a0f8->GetForwardPoint(distance, output);
}

// FUNCTION: WIZ8 0x004213E0
void SetCameraOrientation(
    float* angle, float* pitch, srMatrix3T<float>* rotation)
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
    if (sqrtf(
            (point->x - g_origin_652940[0]) * (point->x - g_origin_652940[0]) +
            (point->y - g_origin_652940[1]) * (point->y - g_origin_652940[1]) +
            (point->z - g_origin_652940[2]) * (point->z - g_origin_652940[2])) !=
        g_zero_005ebb40) {
        MarkRendererReady();
        g_gd_camera_65a0f8->m_position_08c.x = point->x;
        g_gd_camera_65a0f8->m_position_08c.y = point->y;
        g_gd_camera_65a0f8->m_position_08c.z = point->z;
    }
}
