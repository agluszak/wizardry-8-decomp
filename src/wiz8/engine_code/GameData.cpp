#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/OctBuildTree.h"
#include "wiz8/engine_code/Object0043A910.h"
#include "wiz8/engine_code/BitArray.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/float_constants.h"
#include "wiz8/sr_api.h"

#include <math.h>
#include <stdlib.h>
#include <windows.h>
#include <new>

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
extern unsigned char g_flag_00652dce;
extern const float g_world_scale_005ebc40;
extern float g_path_endpoint_scale_005ec1a4;
extern void Function43AAD0();

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
            Function43AAD0();
            return;
        }
    }
    if (g_flag_00652dce != 0) {
        Function439CA0();
        g_flag_00652dce = 0;
    }
    Function43AAD0();
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
        data->vector_40.x = 0;
        data->vector_40.y = 0;
        data->vector_40.z = 0;
        data->camera_forward_4c.x = 0;
        data->camera_forward_4c.y = 0;
        data->camera_forward_4c.z = 0;
        data->scaled_camera_forward_7c.x = 0;
        data->scaled_camera_forward_7c.y = 0;
        data->scaled_camera_forward_7c.z = 0;
        data->vector_64.x = 0;
        data->vector_64.y = 0;
        data->vector_64.z = 0;
        data->vector_70.x = 0;
        data->vector_70.y = 0;
        data->vector_70.z = 0;
        data->vector_a0.x = 0;
        data->vector_a0.y = 0;
        data->vector_a0.z = 0;
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
extern float Function420B40(int value);
extern W8EnvironRecord* g_environ_00652DB4;

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
    *(int*)&positional_2c[8] = 0;
    *(int*)&positional_2c[4] = 0;
    *(int*)&positional_2c[0] = 0;
    surface_count_28 = 0;
    surfaces_38 = 0;
    overflow_surfaces_48 = 0;
    value_4c = 0;
    block_50 = 0;
    *(int*)&positional_3c[0] = 0;
    *(int*)&positional_3c[4] = 0;
    *(int*)&positional_3c[8] = 0;
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
    minimum_08.x = 1.0e8f;
    minimum_08.y = 1.0e8f;
    minimum_08.z = 1.0e8f;
    maximum_14.x = -1.0e8f;
    maximum_14.y = -1.0e8f;
    maximum_14.z = -1.0e8f;
    if (parent == 0) {
        Function420B40(4);
        if (g_object_6598bc == 0) {
            void* object = ::operator new(sizeof(W8Object0043A910));
            if (object == 0) {
                g_object_6598bc = 0;
            }
            else {
                g_object_6598bc = new (object) W8Object0043A910();
            }
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
    if (block_50 != 0) {
        free(block_50);
        block_50 = 0;
    }
    *(int*)&positional_3c[8] = 0;
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
