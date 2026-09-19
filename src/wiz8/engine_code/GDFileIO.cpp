#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/OctBuildTree.h"
#include "wiz8/engine_code/GameTimeAccumulator0043A910.h"
#include "wiz8/engine_code/BitArray.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/engine_code/Trigger.hpp"
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
#include "wiz8/engine_code/3d.h"

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

// GLOBAL: WIZ8 0x005ec1a4
float g_path_endpoint_scale_005ec1a4 = 0.9900000095367432f;

/* Opens a game-data file, builds its record, and pulls the polygon and
   vertex banks through the record reader. */
// FUNCTION: WIZ8 0x00447570
W8GameData* ReadGameData00447570(const char* path, bool secondary)
{
    HANDLE file = CreateFileA(path, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    W8GameData* game_data;
    unsigned char got_polygons;
    unsigned char got_vertices;

    if (file == INVALID_HANDLE_VALUE) {
        return 0;
    }
    game_data = new W8GameData(0, secondary);
    if (game_data == 0) {
        srAssertFail("pGameData", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0xa7, 0);
    }
    got_polygons = game_data->Function447660(file, 0);
    got_vertices = game_data->Function447660(file, 1);
    if (got_vertices == 0 && got_polygons == 0) {
        ReportBuildStatus00497690(7, "ReadGameData: No polygons or vertices in GameData!\n");
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
        if (m_ppTriggers == 0) {
            g_integrated_trigger_count_00659a58 = 0;
            m_ppTriggers = static_cast<Trigger**>(malloc(m_iNumTriggers * sizeof(Trigger*) + 4));
            if (m_ppTriggers == 0) {
                srAssertFail("m_ppTriggers", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                             0x256, "AddTriggerPlane: Couldn't allocate trigger array.");
            }
        }
        if (g_integrated_trigger_count_00659a58 >= m_iNumTriggers) {
            srAssertFail("iTriggerCount < m_iNumTriggers",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x259,
                         "AddTriggerPlane: Too many triggers.");
        }
        m_ppTriggers[g_integrated_trigger_count_00659a58++] = trigger;
        return;
    }

    if (m_pTrigSurfaces == 0) {
        m_pTrigSurfaces = static_cast<W8GDSurface*>(malloc(500 * sizeof(W8GDSurface)));
        if (m_pTrigSurfaces == 0) {
            srAssertFail("m_pTrigSurfaces", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                         0x263, "AddTriggerPlane: Couldn't allocate trigger surfaces.");
        }
        m_pTrigVertices =
            static_cast<srVector3T<float>*>(srHeap.allocate(1000 * sizeof(srVector3T<float>)));
        if (m_pTrigVertices == 0) {
            srAssertFail("m_pTrigVertices", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                         0x265, "AddTriggerPlane: Couldn't allocate trigger vertices.");
        }
        m_ppTriggers = static_cast<Trigger**>(malloc(500 * sizeof(Trigger*)));
        if (m_ppTriggers == 0) {
            srAssertFail("m_ppTriggers", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                         0x267, "AddTriggerPlane: Couldn't allocate trigger array.");
        }
        m_iNumTrigSurfaces = 0;
        m_iNumTrigVertices = 0;
        m_iNumTriggers = 0;
    }
    if (m_iNumTrigSurfaces >= 500) {
        srAssertFail("m_iNumTrigSurfaces < MAX_TRIG_SURFACES",
                     "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x26c, 0);
    }

    for (index = 0; index < m_iNumTriggers && trigger_index == 0; ++index) {
        if (m_ppTriggers[index] == trigger) {
            trigger_index = index;
        }
    }
    if (trigger_index == 0) {
        trigger_index = m_iNumTriggers++;
        m_ppTriggers[trigger_index] = trigger;
    }
    for (index = 0; index < 4; ++index) {
        m_pTrigVertices[m_iNumTrigVertices++] = trigger_vertices[index];
    }

    W8GDSurface* surface = &m_pTrigSurfaces[m_iNumTrigSurfaces];
    surface->flags_00 = 0x80;
    surface->index_04 = m_iNumSurfaces + m_iNumTrigSurfaces;
    surface->trigger_index_08 = trigger_index;
    surface->value_40 = 1.1f;
    surface->vertex_indices_18[0] = m_iNumTrigVertices - 4;
    surface->vertex_indices_18[1] = m_iNumTrigVertices - 3;
    surface->vertex_indices_18[2] = m_iNumTrigVertices - 2;
    ClassifySurfacePlane004498C0(m_pTrigVertices, surface);
    for (index = 0; index < 3; ++index) {
        surface->vertex_indices_18[index] += m_iNumVertices;
    }
    surface->positional_0c = -1;
    surface->positional_10 = -1;
    surface->positional_14 = -1;
    surface->hit_plane_38 = 0;
    ++m_iNumTrigSurfaces;

    surface = &m_pTrigSurfaces[m_iNumTrigSurfaces];
    surface->flags_00 = 0x80;
    surface->index_04 = m_iNumSurfaces + m_iNumTrigSurfaces;
    surface->trigger_index_08 = trigger_index;
    surface->value_40 = 1.1f;
    surface->vertex_indices_18[0] = m_iNumTrigVertices - 2;
    surface->vertex_indices_18[1] = m_iNumTrigVertices - 1;
    surface->vertex_indices_18[2] = m_iNumTrigVertices - 4;
    ClassifySurfacePlane004498C0(m_pTrigVertices, surface);
    for (index = 0; index < 3; ++index) {
        surface->vertex_indices_18[index] += m_iNumVertices;
    }
    surface->positional_0c = -1;
    surface->positional_10 = -1;
    surface->positional_14 = -1;
    surface->hit_plane_38 = 0;
    ++m_iNumTrigSurfaces;
}

// FUNCTION: WIZ8 0x00448840
void W8GameData::IntegrateTriggers()
{
    if (m_iNumTrigVertices == 0) {
        return;
    }

    int combined_vertex_count = m_iNumVertices + m_iNumTrigVertices;
    srVector3T<float>* combined_vertices = static_cast<srVector3T<float>*>(
        srHeap.allocate((combined_vertex_count + 1) * sizeof(srVector3T<float>)));
    if (combined_vertices == 0) {
        srAssertFail("pNewVertices", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x31b,
                     "IntegrateTriggers: Couldn't allocate new vertex array.");
    }
    memcpy(combined_vertices, m_pVertices, m_iNumVertices * sizeof(srVector3T<float>));
    memcpy(combined_vertices + m_iNumVertices, m_pTrigVertices,
           m_iNumTrigVertices * sizeof(srVector3T<float>));
    m_iNumVertices = combined_vertex_count;
    srHeap.free(m_pVertices);
    srHeap.free(m_pTrigVertices);
    integrated_surface_count_34 = m_iNumTrigSurfaces;
    m_pVertices = combined_vertices;
    m_pTrigVertices = 0;
    m_iNumTrigVertices = 0;

    W8GDSurface* new_surfaces =
        static_cast<W8GDSurface*>(malloc((m_iNumTrigSurfaces + 1) * sizeof(W8GDSurface)));
    if (new_surfaces == 0) {
        srAssertFail("pNewSurfaces", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x32c,
                     "IntegrateTriggers: Couldn't allocate new surface array.");
    }
    memcpy(new_surfaces, m_pTrigSurfaces, m_iNumTrigSurfaces * sizeof(W8GDSurface));
    free(m_pTrigSurfaces);
    m_pTrigSurfaces = new_surfaces;

    int end = m_iNumSurfaces + m_iNumTrigSurfaces;
    for (int index = m_iNumSurfaces; index < end; ++index) {
        W8GDSurface* surface =
            index < m_iNumSurfaces ? &m_pSurfaces[index] : &m_pTrigSurfaces[index - m_iNumSurfaces];
        geometry_index_00->InsertSurface00446820(surface, 3);
    }
    bits_58 = new BitArray(m_iNumTriggers);
    bits_5c = new BitArray(m_iNumTriggers);
}

struct W8ProcessedGameDataHeader {
    unsigned int version_00;
    srVector3T<float> minimum_04;
    srVector3T<float> maximum_10;
    int vertex_count_1c;
    int surface_count_20;
    int positional_24;
    int positional_28;
    int integrated_surface_count_2c;
    int value_30;
    int value_34;
    int total_surface_count_38;
    int value_3c;
    int environ_count_40;
    unsigned char unknown_44[0x24];
};

static_assert(sizeof(W8ProcessedGameDataHeader) == 0x68, "W8ProcessedGameDataHeader_must_be_0x68");

// FUNCTION: WIZ8 0x0041a820
unsigned char W8EnvironRecord::RescaleToReference(const W8EnvironRecord* reference)
{
    if (reference == 0) {
        float difference = static_cast<float>(fabs(g_navigator_gravity_00603acc + vector_24.y));
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
    if (header.version_00 != 1) {
        srAssertFail("(FileGD.iVersion == GAMEDATA_VERSION)",
                     "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x46c,
                     reinterpret_cast<const char*>( // reinterpret-ok: String returns UINT8*
                         String("ReadProcessedGameData: File version %d does not match program "
                                "version %d.",
                                header.version_00, 1)));
    }

    minimum_08 = header.minimum_04;
    maximum_14 = header.maximum_10;
    m_iNumSurfaces = header.surface_count_20;
    positional_2c_00 = header.positional_24;
    positional_2c_04 = header.positional_28;
    integrated_surface_count_34 = header.integrated_surface_count_2c;
    m_iNumVertices = header.vertex_count_1c;
    m_iNumInterfaces = header.value_30;
    m_iNumStates = header.value_34;
    m_iNumTriggers = header.total_surface_count_38;
    m_iNumCondPolys = header.value_3c;
    m_iNumEnvirons = header.environ_count_40;

    bits_58 = new BitArray(m_iNumTriggers);
    bits_5c = new BitArray(m_iNumTriggers);

    m_pVertices =
        static_cast<srVector3T<float>*>(srHeap.allocate((m_iNumVertices * 3 + 6) * sizeof(float)));
    if (m_pVertices == 0) {
        srAssertFail("m_pVertices", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x483,
                     "ReadProcessedGameData: Couldn't allocate vertices.");
    }
    if (FileRead(handle, m_pVertices, m_iNumVertices * 0xc, &bytes_read) == 0) {
        srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x487,
                     "ReadProcessedGameData: Couldn't read vertices.");
    }

    m_pSurfaces =
        static_cast<W8GDSurface*>(malloc((m_iNumSurfaces * 0x13 + 0x26) * sizeof(unsigned int)));
    if (m_pSurfaces == 0) {
        srAssertFail("m_pSurfaces", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x48c,
                     "ReadProcessedGameData: Couldn't allocate pSurfaces.");
    }
    if (FileRead(handle, m_pSurfaces, m_iNumSurfaces * 0x4c, &bytes_read) == 0) {
        srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x490,
                     "ReadProcessedGameData: Couldn't read Surface info.");
    }

    if (m_iNumInterfaces != 0) {
        m_pInterfaces =
            static_cast<W8GDInterface*>(malloc((m_iNumInterfaces * 3 + 3) * sizeof(unsigned int)));
        if (m_pInterfaces == 0) {
            srAssertFail("m_pInterfaces", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                         0x497, "ReadProcessedGameData: Couldn't allocate switch interface info.");
        }
        if (FileRead(handle, m_pInterfaces, m_iNumInterfaces * 0xc, &bytes_read) == 0) {
            srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x49a,
                         "ReadProcessedGameData: Couldn't read switch interface info.");
        }
    }

    if (m_iNumStates != 0) {
        m_pStates =
            static_cast<W8GDInterfaceState*>(malloc((m_iNumStates * 3 + 3) * sizeof(unsigned int)));
        if (m_pStates == 0) {
            srAssertFail("m_pStates", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x4a2,
                         "ReadProcessedGameData: Couldn't allocate switch state info.");
        }
        if (FileRead(handle, m_pStates, m_iNumStates * 0xc, &bytes_read) == 0) {
            srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x4a5,
                         "ReadProcessedGameData: Couldn't read switch state info.");
        }
    }

    if (m_iNumCondPolys != 0) {
        m_piCondPolys = static_cast<int*>(malloc(m_iNumCondPolys * sizeof(unsigned int) + 4));
        if (m_piCondPolys == 0) {
            srAssertFail("m_piCondPolys", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                         0x4ad, "ReadProcessedGameData: Couldn't allocate conditional poly list.");
        }
        if (FileRead(handle, m_piCondPolys, m_iNumCondPolys * sizeof(unsigned int), &bytes_read) ==
            0) {
            srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x4b0,
                         "ReadProcessedGameData: Couldn't read conditional poly list.");
        }
    }

    if (m_iNumEnvirons != 0) {
        m_ppEnvirons = static_cast<W8EnvironRecord**>(malloc(m_iNumEnvirons * sizeof(void*)));
        if (m_ppEnvirons == 0) {
            srAssertFail("m_ppEnvirons", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                         0x4b8, "ReadProcessedGameData: Couldn't allocate environment info.");
        }
        memset(m_ppEnvirons, 0, m_iNumEnvirons * sizeof(void*));
        for (index = 0; index < m_iNumEnvirons; ++index) {
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
            environ_record->vector_24.x = 0.0f;
            environ_record->vector_24.y = 0.0f;
            environ_record->vector_24.z = 0.0f;
            environ_record->value_30 = g_default_world_height_00603ac8;
            environ_record->value_34 =
                g_camera_level_forward_scale_603aac * g_navigator_linked_radius_scale_005ebc98;
            environ_record->value_38 = g_float_00603ab8;
            environ_record->value_3c = g_float_00603abc;
            environ_record->value_40 = 1.0f;
            m_ppEnvirons[index] = environ_record;
            if (FileRead(handle, environ_record, 0x44, &bytes_read) == 0) {
                srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                             0x4c2, "ReadProcessedGameData: Couldn't read GD_Environ.");
            }
        }
        m_ppEnvirons[0]->RescaleToReference(0);
        if (m_ppEnvirons[0]->RescaleToReference(0) != 0) {
            for (index = 1; index < m_iNumEnvirons; ++index) {
                m_ppEnvirons[index]->RescaleToReference(m_ppEnvirons[0]);
            }
            m_ppEnvirons[0]->RescaleToReference(m_ppEnvirons[0]);
        }
    }
}

/* Builds the processed game-data record in place: zeroed storage, bound
   extremes, the shared engine-time object on first use, a default
   environment bank, and the previous level-data teardown. The zero stores
   below follow the image order rather than field order. */
// FUNCTION: WIZ8 0x00449010
W8GameData::W8GameData(int handle, bool secondary)
{
    geometry_index_00 = 0;
    positional_04 = 0;
    m_iNumVertices = 0;
    m_pVertices = 0;
    integrated_surface_count_34 = 0;
    positional_2c_04 = 0;
    positional_2c_00 = 0;
    m_iNumSurfaces = 0;
    m_pSurfaces = 0;
    m_pTrigSurfaces = 0;
    m_pTrigVertices = 0;
    m_ppTriggers = 0;
    m_iNumTrigSurfaces = 0;
    m_iNumTrigVertices = 0;
    m_iNumTriggers = 0;
    bits_58 = 0;
    bits_5c = 0;
    value_54 = 0;
    m_iNumInterfaces = 0;
    m_pInterfaces = 0;
    m_iNumStates = 0;
    m_pStates = 0;
    m_iNumCondPolys = 0;
    m_piCondPolys = 0;
    m_iNumNames = 0;
    m_ppNames = 0;
    m_iNumEnvirons = 0;
    m_ppEnvirons = 0;
    value_88 = 0;
    minimum_08 = 1.0e8f;
    maximum_14 = -1.0e8f;
    if (!secondary) {
        MoveTimer(4);
        if (g_game_time_accumulator_6598bc == 0) {
            g_game_time_accumulator_6598bc = new W8GameTimeAccumulator0043A910();
        }
    }
    if (handle != 0) {
        ReadProcessedGameData(handle);
    }
    if (g_environ_00652DB4 != 0) {
        delete g_environ_00652DB4;
    }
    if (m_iNumEnvirons == 0) {
        m_iNumEnvirons = 1;
        m_ppEnvirons = static_cast<W8EnvironRecord**>(malloc(0x28));
        if (m_ppEnvirons == 0) {
            srAssertFail("m_ppEnvirons", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                         0x441, 0);
        }
        for (int index = 0; index < 10; ++index) {
            m_ppEnvirons[index] = 0;
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
            environ_record->vector_24.x = 0.0f;
            environ_record->vector_24.y = 0.0f;
            environ_record->vector_24.z = 0.0f;
            environ_record->value_30 = g_default_world_height_00603ac8;
            environ_record->value_34 =
                g_camera_level_forward_scale_603aac * g_navigator_linked_radius_scale_005ebc98;
            environ_record->value_38 = g_float_00603ab8;
            environ_record->value_3c = g_float_00603abc;
            environ_record->value_40 = 1.0f;
        }
        m_ppEnvirons[0] = environ_record;
    }
    W8LevelDataRecord* old_level = g_level_data_00652dac;
    g_environ_00652DB4 = m_ppEnvirons[0];
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

    for (int index = 0; index < game_data->m_iNumSurfaces; ++index) {
        if (game_data->geometry_index_00->InsertSurface00446820(&game_data->m_pSurfaces[index],
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
    BuildTrianglePlane00449A40(
        reinterpret_cast<srVector4T<float>*>(&surface->plane_24), /* reinterpret-ok:
            the union's plane arm is a 4-float vector */
        &vertices[surface->vertex_indices_18[0]], &vertices[surface->vertex_indices_18[1]],
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
            surface->slope_48 = g_float_005ebb38;
        }
        if (surface->slope_48 < g_float_005ebb34) {
            surface->flags_00 |= 0x20;
            surface->slope_48 = g_float_005ebb34;
        }
    } else if (surface->value_40 < g_float_005ec028 &&
               g_path_endpoint_scale_005ec1a4 < surface->value_40 && (surface->flags_00 & 4) != 0) {
        surface->value_40 = 0.1f;
    }

    flags = surface->flags_00;
    surface->value_40 *= g_world_scale_005ebc40;
    if ((flags & 4) == 0) {
        surface->slope_48 = g_float_005ebb34;
    } else if (surface->slope_48 < g_float_005ebc58 && (flags & 0x20) == 0) {
        if (surface->plane_24[1] <= g_float_005ebccc) {
            upper_value = surface->plane_24[1];
        }
        surface->slope_48 = upper_value;
    }
    surface->flags_00 = flags & ~8U;
}

/* Header-visible SetPlaneFromThreePoints. This TU unrolls the three-point
   copy; 0x0046D660 lowers the same assignments as a component countdown. */
// FUNCTION: WIZ8 0x00449a40
void BuildTrianglePlane00449A40(srVector4T<float>* plane, const srVector3T<float>* first,
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

    ReleaseLevelData0041A9E0();
    if (geometry_index_00 != 0) {
        delete geometry_index_00;
    }
    if (m_pVertices != 0) {
        srHeap.free(m_pVertices);
    }
    if (m_pSurfaces != 0) {
        free(m_pSurfaces);
    }
    if (bits_58 != 0) {
        delete bits_58;
    }
    if (bits_5c != 0) {
        delete bits_5c;
    }
    if (m_ppNames != 0) {
        if (m_iNumNames > 0) {
            index = 0;
            do {
                if (m_ppNames[index] != 0) {
                    free(m_ppNames[index]);
                }
                ++index;
            } while (index < m_iNumNames);
        }
        free(m_ppNames);
        m_iNumNames = 0;
        m_ppNames = 0;
    }
    if (m_pInterfaces != 0) {
        free(m_pInterfaces);
        m_pInterfaces = 0;
        m_iNumInterfaces = 0;
    }
    if (m_piCondPolys != 0) {
        free(m_piCondPolys);
        m_piCondPolys = 0;
        m_iNumCondPolys = 0;
    }
    if (m_pStates != 0) {
        free(m_pStates);
        m_pStates = 0;
        m_iNumStates = 0;
    }
    if (m_ppTriggers != 0) {
        free(m_ppTriggers);
        m_ppTriggers = 0;
    }
    m_iNumTriggers = 0;
    if (m_ppEnvirons != 0) {
        if (m_iNumEnvirons > 0) {
            index = 0;
            do {
                if (m_ppEnvirons[index] != 0) {
                    delete m_ppEnvirons[index];
                }
                ++index;
            } while (index < m_iNumEnvirons);
        }
        free(m_ppEnvirons);
        m_ppEnvirons = 0;
    }
    g_octree_game_data_00652db0 = 0;
}

/* Serializes the processed game-data block WriteOctFile appends after the
   octree sections: the 0x68-byte versioned header, the vertex and surface
   banks, the optional switch interface/state and conditional-poly lists, then
   each environment record.  Every failure returns 0 after logging; the open
   handle is never closed here. */
// FUNCTION: WIZ8 0x0044aa40
unsigned char W8GameData::WriteGameData0044AA40(int handle)
{
    W8ProcessedGameDataHeader header;
    int index;

    header.version_00 = 1;
    header.minimum_04 = minimum_08;
    header.maximum_10 = maximum_14;
    header.vertex_count_1c = m_iNumVertices;
    header.surface_count_20 = m_iNumSurfaces;
    header.positional_24 = positional_2c_00;
    header.positional_28 = positional_2c_04;
    header.integrated_surface_count_2c = integrated_surface_count_34;
    header.value_30 = m_iNumInterfaces;
    header.value_34 = m_iNumStates;
    header.total_surface_count_38 = m_iNumTriggers;
    header.value_3c = m_iNumCondPolys;
    header.environ_count_40 = m_iNumEnvirons;
    memset(header.unknown_44, 0, sizeof(header.unknown_44));

    if (handle == 0) {
        ReportBuildStatus00497690(7, "WriteGameData: File not open.\n");
        return 0;
    }
    if (FileWrite(handle, &header, 0x68, 0) == 0) {
        ReportBuildStatus00497690(7, "WriteGameData: Couldn't write GameData info.\n");
        return 0;
    }
    if (FileWrite(handle, m_pVertices, m_iNumVertices * 0xc, 0) == 0) {
        ReportBuildStatus00497690(7, "WriteGameData: Couldn't write vertex info.\n");
        return 0;
    }
    if (FileWrite(handle, m_pSurfaces, m_iNumSurfaces * 0x4c, 0) == 0) {
        ReportBuildStatus00497690(7, "WriteGameData: Couldn't write Surface info.\n");
        return 0;
    }
    if (m_iNumInterfaces != 0 && FileWrite(handle, m_pInterfaces, m_iNumInterfaces * 0xc, 0) == 0) {
        ReportBuildStatus00497690(7, "WriteGameData: Couldn't write switch interface info.\n");
        return 0;
    }
    if (m_iNumStates != 0 && FileWrite(handle, m_pStates, m_iNumStates * 0xc, 0) == 0) {
        ReportBuildStatus00497690(7, "WriteGameData: Couldn't write switch state info.\n");
        return 0;
    }
    if (m_iNumCondPolys != 0 && FileWrite(handle, m_piCondPolys, m_iNumCondPolys * 4, 0) == 0) {
        ReportBuildStatus00497690(7, "WriteGameData: Couldn't write conditional poly list.\n");
        return 0;
    }
    if (m_iNumEnvirons != 0) {
        for (index = 0; index < m_iNumEnvirons; ++index) {
            if (FileWrite(handle, m_ppEnvirons[index], 0x44, 0) == 0) {
                ReportBuildStatus00497690(7, "WriteGameData: Couldn't write GD_Environ.\n");
                return 0;
            }
        }
    }
    return 1;
}
