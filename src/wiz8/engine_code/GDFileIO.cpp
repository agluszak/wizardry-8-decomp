#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/OctBuildTree.h"
#include "wiz8/engine_code/Object0043A910.h"
#include "wiz8/engine_code/BitArray.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/engine_code/Trigger.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/float_constants.h"
#include "wiz8/sr_api.h"
#include "wiz8/engine_code/GDFileIO.h"
#include "wiz8/engine_code/materials.h"

#include "DEBUG.H"
#include "FileMan.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <new>

/* Engine Code\GDFileIO.cpp. The game-data file reader and trigger-plane
   integration, plus the level record's constructor, destructor and
   spatial-index build. The adjacent 0x004497C0, 0x004498C0 and 0x00449A40
   bodies are the surrounding attribution gaps placed with their asserted
   companion. */

// GLOBAL: WIZ8 0x005ec1a8
float g_float_005ec1a8 = -0.3333333432674408f;
// GLOBAL: WIZ8 0x005ebc58
float g_float_005ebc58 = 1.0000000116860974e-07f;
// GLOBAL: WIZ8 0x005ec028
float g_float_005ec028 = 1.0099999904632568f;
// GLOBAL: WIZ8 0x005ec1a0
float g_float_005ec1a0 = 0.9959999918937683f;

// GLOBAL: WIZ8 0x00659a58
int g_integrated_trigger_count_00659a58;

// GLOBAL: WIZ8 0x00603ab8
float g_float_00603ab8 = 0.30000001192092896f;
// GLOBAL: WIZ8 0x00603abc
float g_float_00603abc = 112.5f;

extern float g_path_endpoint_scale_005ec1a4;
// GLOBAL: WIZ8 0x005ec1a4
float g_path_endpoint_scale_005ec1a4 = 0.9900000095367432f;

/* Opens a game-data file, builds its record, and pulls the polygon and
   vertex banks through the record reader. */
// FUNCTION: WIZ8 0x00447570
W8GameData* ReadGameData00447570(const char* path, void* parent)
{
    HANDLE file = CreateFileA(path, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    W8GameData* game_data;
    unsigned char got_polygons;
    unsigned char got_vertices;

    if (file == INVALID_HANDLE_VALUE) {
        return 0;
    }
    game_data = new W8GameData(0, parent);
    if (game_data == 0) {
        srAssertFail("pGameData", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0xa7, 0);
    }
    got_polygons = game_data->Function447660(file, 0);
    got_vertices = game_data->Function447660(file, 1);
    if (got_vertices == 0 && got_polygons == 0) {
        Function497690(7, "ReadGameData: No polygons or vertices in GameData!\n");
    }
    CloseHandle(file);
    return game_data;
}

// FUNCTION: WIZ8 0x00448310
void W8GameData::AddTriggerPlane(const srVector3T<float>* trigger_vertices, Trigger* trigger)
{
    int trigger_index = 0;
    int index;
    if (positional_04 != 0) {
        if (trigger_table_50 == 0) {
            g_integrated_trigger_count_00659a58 = 0;
            trigger_table_50 =
                static_cast<Trigger**>(malloc(total_surface_count_44 * sizeof(Trigger*) + 4));
            if (trigger_table_50 == 0) {
                srAssertFail("m_ppTriggers", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                             0x256, "AddTriggerPlane: Couldn't allocate trigger array.");
            }
        }
        if (g_integrated_trigger_count_00659a58 >= total_surface_count_44) {
            srAssertFail("iTriggerCount < m_iNumTriggers",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x259,
                         "AddTriggerPlane: Too many triggers.");
        }
        trigger_table_50[g_integrated_trigger_count_00659a58++] = trigger;
        return;
    }

    if (overflow_surfaces_48 == 0) {
        overflow_surfaces_48 = static_cast<W8GDSurface*>(malloc(500 * sizeof(W8GDSurface)));
        if (overflow_surfaces_48 == 0) {
            srAssertFail("m_pTrigSurfaces", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                         0x263, "AddTriggerPlane: Couldn't allocate trigger surfaces.");
        }
        overflow_vertices_4c =
            static_cast<srVector3T<float>*>(srHeap.allocate(1000 * sizeof(srVector3T<float>)));
        if (overflow_vertices_4c == 0) {
            srAssertFail("m_pTrigVertices", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                         0x265, "AddTriggerPlane: Couldn't allocate trigger vertices.");
        }
        trigger_table_50 = static_cast<Trigger**>(malloc(500 * sizeof(Trigger*)));
        if (trigger_table_50 == 0) {
            srAssertFail("m_ppTriggers", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                         0x267, "AddTriggerPlane: Couldn't allocate trigger array.");
        }
        overflow_surface_count_3c = 0;
        overflow_vertex_count_40 = 0;
        total_surface_count_44 = 0;
    }
    if (overflow_surface_count_3c >= 500) {
        srAssertFail("m_iNumTrigSurfaces < MAX_TRIG_SURFACES",
                     "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x26c, 0);
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
    srVector3T<float>* combined_vertices = static_cast<srVector3T<float>*>(
        srHeap.allocate((combined_vertex_count + 1) * sizeof(srVector3T<float>)));
    if (combined_vertices == 0) {
        srAssertFail("pNewVertices", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x31b,
                     "IntegrateTriggers: Couldn't allocate new vertex array.");
    }
    memcpy(combined_vertices, vertices_24, vertex_count_20 * sizeof(srVector3T<float>));
    memcpy(combined_vertices + vertex_count_20, overflow_vertices_4c,
           overflow_vertex_count_40 * sizeof(srVector3T<float>));
    vertex_count_20 = combined_vertex_count;
    srHeap.free(vertices_24);
    srHeap.free(overflow_vertices_4c);
    integrated_surface_count_34 = overflow_surface_count_3c;
    vertices_24 = combined_vertices;
    overflow_vertices_4c = 0;
    overflow_vertex_count_40 = 0;

    W8GDSurface* new_surfaces =
        static_cast<W8GDSurface*>(malloc((overflow_surface_count_3c + 1) * sizeof(W8GDSurface)));
    if (new_surfaces == 0) {
        srAssertFail("pNewSurfaces", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x32c,
                     "IntegrateTriggers: Couldn't allocate new surface array.");
    }
    memcpy(new_surfaces, overflow_surfaces_48, overflow_surface_count_3c * sizeof(W8GDSurface));
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

struct W8ProcessedGameDataHeader {
    unsigned int value_00;
    unsigned int version_04;
    srVector3T<float> minimum_08;
    srVector3T<float> maximum_14;
    int integrated_surface_count_20;
    int surface_count_24;
    int unknown_28;
    int unknown_2c;
    int vertex_count_30;
    int value_34;
    int value_38;
    int total_surface_count_3c;
    int value_40;
    int environ_count_44;
    unsigned char unknown_48[0x20];
};

static_assert(sizeof(W8ProcessedGameDataHeader) == 0x68, "W8ProcessedGameDataHeader_must_be_0x68");

// FUNCTION: WIZ8 0x0041a820
unsigned char W8EnvironRecord::RescaleToReference(const W8EnvironRecord* reference)
{
    if (reference == 0) {
        float difference = (float)fabs(g_navigator_gravity_00603acc + value_28);
        if (g_navigator_gravity_00603acc * g_camera_snap_epsilon_005ebc2c < difference) {
            return 1;
        }
        difference = (float)fabs(value_34 - g_camera_level_forward_scale_603aac);
        if (g_camera_level_forward_scale_603aac * g_camera_snap_epsilon_005ebc2c < difference) {
            return 1;
        }
        difference = (float)fabs(value_38 - g_float_00603abc);
        if (g_float_00603abc * g_camera_snap_epsilon_005ebc2c < difference) {
            return 1;
        }
        difference = (float)fabs(value_3c - g_float_00603ab8);
        if (g_float_00603ab8 * g_camera_snap_epsilon_005ebc2c < difference) {
            return 1;
        }
        return 0;
    }

    float scale = g_navigator_gravity_00603acc / -reference->value_14;
    value_10 *= scale;
    value_14 *= scale;
    value_18 *= scale;
    value_34 *= (g_camera_level_forward_scale_603aac / reference->value_34);
    value_38 *= (g_float_00603abc / reference->value_38);
    value_3c *= (g_float_00603ab8 / reference->value_3c);
    return 0;
}

/* Read the processed GameData header and all of its variable-size banks. */
// FUNCTION: WIZ8 0x00449240
void W8GameData::ReadProcessedGameData(int handle)
{
    W8ProcessedGameDataHeader header;
    unsigned int bytes_read;
    int index;

    if (FileRead(handle, &header, sizeof(header), &bytes_read) == 0) {
        srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x465,
                     "ReadProcessedGameData: Couldn't read GameData info.");
    }
    if (header.version_04 != 1) {
        srAssertFail("(FileGD.iVersion == GAMEDATA_VERSION)",
                     "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x46c,
                     reinterpret_cast<const char*>( // reinterpret-ok: String returns UINT8*
                         String("ReadProcessedGameData: File version %d does not match program "
                                "version %d.",
                                header.version_04, 1)));
    }

    minimum_08 = header.minimum_08;
    maximum_14 = header.maximum_14;
    integrated_surface_count_34 = header.integrated_surface_count_20;
    surface_count_28 = header.surface_count_24;
    vertex_count_20 = header.vertex_count_30;
    value_60 = header.value_34;
    value_68 = header.value_38;
    total_surface_count_44 = header.total_surface_count_3c;
    value_70 = header.value_40;
    environ_count_80 = header.environ_count_44;

    bits_58 = new BitArray(total_surface_count_44);
    bits_5c = new BitArray(total_surface_count_44);

    vertices_24 =
        static_cast<srVector3T<float>*>(srHeap.allocate((vertex_count_20 * 3 + 6) * sizeof(float)));
    if (vertices_24 == 0) {
        srAssertFail("m_pVertices", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x483,
                     "ReadProcessedGameData: Couldn't allocate vertices.");
    }
    if (FileRead(handle, vertices_24, vertex_count_20 * 0xc, &bytes_read) == 0) {
        srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x487,
                     "ReadProcessedGameData: Couldn't read vertices.");
    }

    surfaces_38 =
        static_cast<W8GDSurface*>(malloc((surface_count_28 * 0x13 + 0x26) * sizeof(unsigned int)));
    if (surfaces_38 == 0) {
        srAssertFail("m_pSurfaces", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x48c,
                     "ReadProcessedGameData: Couldn't allocate pSurfaces.");
    }
    if (FileRead(handle, surfaces_38, surface_count_28 * 0x4c, &bytes_read) == 0) {
        srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x490,
                     "ReadProcessedGameData: Couldn't read Surface info.");
    }

    if (value_60 != 0) {
        block_64 = malloc((value_60 * 3 + 3) * sizeof(unsigned int));
        if (block_64 == 0) {
            srAssertFail("m_pInterfaces", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                         0x497, "ReadProcessedGameData: Couldn't allocate switch interface info.");
        }
        if (FileRead(handle, block_64, value_60 * 0xc, &bytes_read) == 0) {
            srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x49a,
                         "ReadProcessedGameData: Couldn't read switch interface info.");
        }
    }

    if (value_68 != 0) {
        block_6c = malloc((value_68 * 3 + 3) * sizeof(unsigned int));
        if (block_6c == 0) {
            srAssertFail("m_pStates", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x4a2,
                         "ReadProcessedGameData: Couldn't allocate switch state info.");
        }
        if (FileRead(handle, block_6c, value_68 * 0xc, &bytes_read) == 0) {
            srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x4a5,
                         "ReadProcessedGameData: Couldn't read switch state info.");
        }
    }

    if (value_70 != 0) {
        block_74 = malloc(value_70 * sizeof(unsigned int) + 4);
        if (block_74 == 0) {
            srAssertFail("m_piCondPolys", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                         0x4ad, "ReadProcessedGameData: Couldn't allocate conditional poly list.");
        }
        if (FileRead(handle, block_74, value_70 * sizeof(unsigned int), &bytes_read) == 0) {
            srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x4b0,
                         "ReadProcessedGameData: Couldn't read conditional poly list.");
        }
    }

    if (environ_count_80 != 0) {
        environs_84 = static_cast<W8EnvironRecord**>(malloc(environ_count_80 * sizeof(void*)));
        if (environs_84 == 0) {
            srAssertFail("m_ppEnvirons", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                         0x4b8, "ReadProcessedGameData: Couldn't allocate environment info.");
        }
        memset(environs_84, 0, environ_count_80 * sizeof(void*));
        for (index = 0; index < environ_count_80; ++index) {
            W8EnvironRecord* environ_record = new W8EnvironRecord();
            if (environ_record == 0) {
                srAssertFail("m_ppEnvirons[i]",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x4be,
                             "ReadProcessedGameData: Couldn't allocate environment.");
            }
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
            environ_record->value_30 = g_default_world_height_00603ac8;
            environ_record->value_34 =
                g_camera_level_forward_scale_603aac * g_navigator_linked_radius_scale_005ebc98;
            environ_record->value_38 = g_float_00603ab8;
            environ_record->value_3c = g_float_00603abc;
            environ_record->value_40 = 1.0f;
            environs_84[index] = environ_record;
            if (FileRead(handle, environ_record, 0x44, &bytes_read) == 0) {
                srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                             0x4c2, "ReadProcessedGameData: Couldn't read GD_Environ.");
            }
        }
        environs_84[0]->RescaleToReference(0);
        if (environs_84[0]->RescaleToReference(0) != 0) {
            for (index = 1; index < environ_count_80; ++index) {
                environs_84[index]->RescaleToReference(environs_84[0]);
            }
            environs_84[0]->RescaleToReference(environs_84[0]);
        }
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
    minimum_08 = 1.0e8f;
    maximum_14 = -1.0e8f;
    if (parent == 0) {
        MoveTimer(4);
        if (g_object_6598bc == 0) {
            g_object_6598bc = new W8Object0043A910();
        }
    }
    if (handle != 0) {
        ReadProcessedGameData(handle);
    }
    if (g_environ_00652DB4 != 0) {
        delete g_environ_00652DB4;
    }
    if (environ_count_80 == 0) {
        environ_count_80 = 1;
        environs_84 = static_cast<W8EnvironRecord**>(malloc(0x28));
        if (environs_84 == 0) {
            srAssertFail("m_ppEnvirons", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                         0x441, 0);
        }
        for (int index = 0; index < 10; ++index) {
            environs_84[index] = 0;
        }
        W8EnvironRecord* environ_record = new W8EnvironRecord();
        if (environ_record == 0) {
            environ_record = 0;
        } else {
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
            environ_record->value_30 = g_default_world_height_00603ac8;
            environ_record->value_34 =
                g_camera_level_forward_scale_603aac * g_navigator_linked_radius_scale_005ebc98;
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
        game_data->geometry_index_00 =
            new W8OctBuildTree00446390(2000.0f, &minimum, &maximum, 0x40, 0);
    }

    for (int index = 0; index < game_data->surface_count_28; ++index) {
        if (game_data->geometry_index_00->InsertSurface00446820(&game_data->surfaces_38[index],
                                                                3) == 0) {
            return 0;
        }
    }
    return 1;
}

/* Rebuild one indexed surface plane and derive the runtime classification
   carried by the level-geometry record. Bit 0x80 requests dominant-axis
   selection; bit 4 is the walkable slope classification. */
// FUNCTION: WIZ8 0x004498c0
void ClassifySurfacePlane004498C0(const srVector3T<float>* vertices, W8GDSurface* surface)
{
    BuildTrianglePlane00449A40(surface->plane_24, &vertices[surface->vertex_indices_18[0]],
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
        if ((surface->flags_00 & 4) == 0 && g_float_005ec1a0 < surface->plane_24[1]) {
            surface->flags_00 |= 4;
            surface->value_48 = g_float_005ebb38;
        }
        if (surface->value_48 < g_float_005ebb34) {
            surface->flags_00 |= 0x20;
            surface->value_48 = g_float_005ebb34;
        }
    } else if (surface->value_40 < g_float_005ec028 &&
               g_path_endpoint_scale_005ec1a4 < surface->value_40 && (surface->flags_00 & 4) != 0) {
        surface->value_40 = 0.1f;
    }

    flags = surface->flags_00;
    surface->value_40 *= g_world_scale_005ebc40;
    if ((flags & 4) == 0) {
        surface->value_48 = g_float_005ebb34;
    } else if (surface->value_48 < g_float_005ebc58 && (flags & 0x20) == 0) {
        if (surface->plane_24[1] <= g_float_005ebccc) {
            upper_value = surface->plane_24[1];
        }
        surface->value_48 = upper_value;
    }
    surface->flags_00 = flags & ~8U;
}

/* Header-visible SetPlaneFromThreePoints. This TU unrolls the three-point
   copy; 0x0046D660 lowers the same assignments as a component countdown. */
// FUNCTION: WIZ8 0x00449a40
void BuildTrianglePlane00449A40(float* plane, const srVector3T<float>* first,
                                const srVector3T<float>* second, const srVector3T<float>* third)
{
    SetPlaneFromThreePoints(plane, first, second, third);
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
