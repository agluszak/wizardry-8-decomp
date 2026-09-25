#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/OctBuildTree.h"
#include "wiz8/engine_code/OctBuildPreTree.h"
#include "wiz8/engine_code/OctPreTree.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/LevelFile.h"
#include "wiz8/engine_code/stHash.hpp"
#include "wiz8/engine_code/GameTimeAccumulator.h"
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
#include <stdio.h>
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
// GLOBAL: WIZ8 0x005ec1ac
const float g_float_005ec1ac = 3000.0f;
// GLOBAL: WIZ8 0x005ec1b0
const float g_float_005ec1b0 = 60000.0f;
// GLOBAL: WIZ8 0x005ec1b4
const float g_float_005ec1b4 = 900000.0f;
// GLOBAL: WIZ8 0x005ff56c
char g_string_005ff56c[] = "\n";

// GLOBAL: WIZ8 0x00659a58
int g_integrated_trigger_count;

// GLOBAL: WIZ8 0x00603ab8
float g_default_momentum_scale = 0.30000001192092896f;
// GLOBAL: WIZ8 0x00603abc
float g_default_motion_limit = 112.5f;

// GLOBAL: WIZ8 0x005ec1a4
float g_path_endpoint_scale = 0.9900000095367432f;

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
    got_polygons = game_data->ReadWGDList00447660(file, 0);
    got_vertices = game_data->ReadWGDList00447660(file, 1);
    if (got_vertices == 0 && got_polygons == 0) {
        ReportBuildStatus(7, "ReadGameData: No polygons or vertices in GameData!\n");
    }
    CloseHandle(file);
    return game_data;
}

/* The WGD face record's fixed head: three vertex indexes, the source plane,
   and a version tag checked before the rest of the record is read. */
struct W8GDFaceHeader { /* 0x1c */
    int vertex_indices_00[3];
    float plane_0c[3];
    int version_18;
};

static_assert(sizeof(W8GDFaceHeader) == 0x1c, "W8GDFaceHeader_must_be_0x1c");

/* The WGD face record's tail: classification flag, slope/value pair, footstep
   selectors, an unused dword, and the trigger index the writer overrode. */
struct W8GDFaceData { /* 0x18 */
    int type_00;
    float slope_04;
    float contact_margin_08;
    unsigned char material_0c;
    unsigned char surface_0d;
    unsigned char pad_0e[2];
    int chance_10;
    int trigger_index_14;
};

static_assert(sizeof(W8GDFaceData) == 0x18, "W8GDFaceData_must_be_0x18");

/* The conditional-face record following a non-primary face: the group key the
   interface compiler buckets on and the interface's name. */
struct W8GDExtendedFace { /* 0x44 */
    int group_00;
    char name_04[0x40];
};

static_assert(sizeof(W8GDExtendedFace) == 0x44, "W8GDExtendedFace_must_be_0x44");

/* Reads one WGD vertex/polygon bank. poly_type 0 builds fresh arrays; any
   other type grows the existing banks and also consumes each face's extended
   name record into the interface tables. */
// FUNCTION: WIZ8 0x00447660
unsigned char W8GameData::ReadWGDList00447660(HANDLE file, int poly_type)
{
    DWORD bytes_read;
    int vertex_count;
    int face_count;
    int record_count;
    int* cond_faces;
    int index;
    int name_index;
    unsigned char success;
    char message[100];
    float bounds[6];

    record_count = 0;
    if (poly_type < 0 || 2 < poly_type) {
        ReportBuildStatus(7, "ReadWGDList: Invalid poly type.\n");
    }
    success = ReadFile(file, &vertex_count, 4, &bytes_read, 0) & 1 &
              ReadFile(file, &face_count, 4, &bytes_read, 0);
    if (success == 0) {
        srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0xe5,
                     "Error reading counts from WGD file.");
    }
    if (face_count > 0 && vertex_count > 0) {
        if (face_count < 0x30d41) {
            if (vertex_count < 0x30d41) {
                if (poly_type == 0) {
                    m_pVertices = new srVector3T<float>[vertex_count];
                    if (m_pVertices == 0) {
                        srAssertFail("m_pVertices",
                                     "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x114,
                                     "ReadWGDList: Could not allocate vertices.");
                    }
                    m_pSurfaces =
                        static_cast<W8GDSurface*>(malloc(face_count * sizeof(W8GDSurface)));
                    if (m_pSurfaces == 0) {
                        srAssertFail("m_pSurfaces",
                                     "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x116,
                                     "ReadWGDList: Could not allocate surfaces.");
                    }
                } else {
                    W8GDSurface* old_surfaces = m_pSurfaces;
                    srVector3T<float>* old_vertices = m_pVertices;
                    m_pVertices = new srVector3T<float>[m_iNumVertices + vertex_count];
                    if (m_pVertices == 0) {
                        srAssertFail("m_pVertices",
                                     "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x101,
                                     "ReadWGDList: Could not allocate vertices.");
                    }
                    memcpy(m_pVertices, old_vertices, m_iNumVertices * sizeof(srVector3T<float>));
                    srHeap.free(old_vertices);
                    m_pSurfaces = static_cast<W8GDSurface*>(
                        malloc((m_iNumSurfaces + face_count) * sizeof(W8GDSurface)));
                    if (m_pSurfaces == 0) {
                        srAssertFail("m_pSurfaces",
                                     "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x106,
                                     "ReadWGDList: Could not allocate surfaces.");
                    }
                    memcpy(m_pSurfaces, old_surfaces, m_iNumSurfaces * sizeof(W8GDSurface));
                    free(old_surfaces);
                    cond_faces = static_cast<int*>(malloc(face_count * 0xc));
                    if (cond_faces == 0) {
                        srAssertFail("pCondFaces",
                                     "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x10c,
                                     "ReadWGDList: Could not allocate pCondFaces.");
                    }
                    m_ppNames = static_cast<char**>(malloc(face_count * sizeof(char*)));
                    if (m_ppNames == 0) {
                        srAssertFail("m_ppNames",
                                     "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x10e,
                                     "ReadWGDList: Could not allocate name list.");
                    }
                    memset(m_ppNames, 0, face_count * sizeof(char*));
                }
                index = m_iNumVertices;
                name_index = 0;
                while (index < m_iNumVertices + vertex_count) {
                    srVector3T<float> vertex;
                    success &= ReadFile(file, &vertex, 0xc, &bytes_read, 0);
                    if (success == 0) {
                        srAssertFail("fSuccess",
                                     "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x120,
                                     "Error reading vertex from WGD file.");
                    }
                    m_pVertices[index].x = vertex.x * g_world_scale;
                    m_pVertices[index].y = vertex.y * g_world_scale;
                    m_pVertices[index].z = vertex.z * g_world_scale;
                    if (index == m_iNumVertices) {
                        minimum_08.x = vertex.x;
                        maximum_14.x = vertex.x;
                        minimum_08.y = vertex.y;
                        maximum_14.y = vertex.y;
                        minimum_08.z = vertex.z;
                        maximum_14.z = vertex.z;
                    } else {
                        if (vertex.x < minimum_08.x) {
                            minimum_08.x = vertex.x;
                        }
                        if (maximum_14.x < vertex.x) {
                            maximum_14.x = vertex.x;
                        }
                        if (vertex.y < minimum_08.y) {
                            minimum_08.y = vertex.y;
                        }
                        if (maximum_14.y < vertex.y) {
                            maximum_14.y = vertex.y;
                        }
                        if (vertex.z < minimum_08.z) {
                            minimum_08.z = vertex.z;
                        }
                        if (maximum_14.z < vertex.z) {
                            maximum_14.z = vertex.z;
                        }
                    }
                    ++index;
                }
                index = m_iNumSurfaces;
                int* record = cond_faces;
                while (index < m_iNumSurfaces + face_count) {
                    W8GDFaceHeader header;
                    W8GDFaceData data;
                    W8GDExtendedFace extended;
                    success = ReadFile(file, &header, 0x1c, &bytes_read, 0);
                    if (header.version_18 != 2) {
                        srAssertFail("(tfFace.iVersion == 2 )",
                                     "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x142,
                                     "Wrong version of WGD data--Get new plugin.");
                    }
                    success &= ReadFile(file, &data, 0x18, &bytes_read, 0);
                    if (poly_type != 0) {
                        success &= ReadFile(file, &extended, 0x44, &bytes_read, 0);
                    }
                    if (success == 0) {
                        srAssertFail("fSuccess",
                                     "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x146,
                                     "Error reading face from WGD file.");
                    }
                    W8GDSurface* surface = &m_pSurfaces[index];
                    surface->contact_margin_40 = data.contact_margin_08;
                    surface->slope_48 = data.slope_04;
                    surface->chance_44 = data.chance_10;
                    surface->trigger_index_08 = data.trigger_index_14;
                    surface->footstep_material_3d = data.material_0c;
                    surface->footstep_surface_3c = data.surface_0d;
                    if (data.type_00 == 1) {
                        surface->flags_00 = 0x44;
                    } else {
                        surface->flags_00 = 0;
                    }
                    surface->plane_24.normal.x = header.plane_0c[0];
                    surface->plane_24.normal.y = header.plane_0c[1];
                    surface->plane_24.normal.z = header.plane_0c[2];
                    float largest = static_cast<float>(fabs(surface->plane_24.normal.x));
                    unsigned int axis = 0;
                    if (largest < static_cast<float>(fabs(surface->plane_24.normal.y))) {
                        largest = static_cast<float>(fabs(surface->plane_24.normal.y));
                        axis = 1;
                    }
                    if (largest < static_cast<float>(fabs(surface->plane_24.normal.z))) {
                        axis = 2;
                    }
                    surface->flags_00 |= axis;
                    surface->vertex_indices_18[0] = header.vertex_indices_00[0] + m_iNumVertices;
                    surface->vertex_indices_18[1] = header.vertex_indices_00[1] + m_iNumVertices;
                    surface->vertex_indices_18[2] = header.vertex_indices_00[2] + m_iNumVertices;
                    surface->index_04 = index;
                    surface->trigger_index_08 = 0;
                    surface->edge_link_0c[2] = -1;
                    surface->edge_link_0c[1] = -1;
                    surface->edge_link_0c[0] = -1;
                    surface->hit_plane_38 = 0;
                    ClassifySurfacePlane(m_pVertices, surface);
                    if (poly_type != 0) {
                        record[1] = index;
                        record[0] = 0;
                        record[2] = extended.group_00;
                        name_index = 0;
                        while (name_index < m_iNumNames && record[0] == 0) {
                            if (strcmp(m_ppNames[name_index], extended.name_04) == 0) {
                                record[0] = name_index + 1;
                            }
                            ++name_index;
                        }
                        if (record[0] == 0) {
                            m_ppNames[name_index] = static_cast<char*>(malloc(0x40));
                            if (m_ppNames[name_index] == 0) {
                                srAssertFail("m_ppNames[i2]",
                                             "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                                             0x187, "ReadWGDList: Couldn't allocate name string.");
                            }
                            strcpy(m_ppNames[name_index], extended.name_04);
                            if (m_iNumInterfaces == 0) {
                                m_iNumInterfaces = 1;
                            }
                            record[0] = m_iNumInterfaces;
                            ++m_iNumInterfaces;
                            ++m_iNumNames;
                        }
                        surface->trigger_index_08 = record[0];
                        ++record_count;
                        record += 3;
                    }
                    ++index;
                }
                ReadFile(file, &bounds[3], 4, &bytes_read, 0);
                ReadFile(file, &bounds[4], 4, &bytes_read, 0);
                ReadFile(file, &bounds[5], 4, &bytes_read, 0);
                ReadFile(file, &bounds[0], 4, &bytes_read, 0);
                ReadFile(file, &bounds[1], 4, &bytes_read, 0);
                ReadFile(file, &bounds[2], 4, &bytes_read, 0);
                for (index = 0; index < 3; ++index) {
                    bounds[index + 3] = bounds[index + 3] * g_world_scale;
                    bounds[index] = bounds[index] * g_world_scale;
                }
                if (bounds[3] < minimum_08.x) {
                    minimum_08.x = bounds[3];
                }
                if (bounds[4] < minimum_08.y) {
                    minimum_08.y = bounds[4];
                }
                if (bounds[5] < minimum_08.z) {
                    minimum_08.z = bounds[5];
                }
                if (maximum_14.x < bounds[0]) {
                    maximum_14.x = bounds[0];
                }
                if (maximum_14.y < bounds[1]) {
                    maximum_14.y = bounds[1];
                }
                if (maximum_14.z < bounds[2]) {
                    maximum_14.z = bounds[2];
                }
                if (m_ppNames != 0 && cond_faces != 0) {
                    CompileGDInterfaces00447FB0(cond_faces, record_count);
                    free(cond_faces);
                }
                m_iNumVertices = m_iNumVertices + vertex_count;
                m_iNumSurfaces = m_iNumSurfaces + face_count;
                return 1;
            }
            sprintf(message, "Too many GameData vertices: %d!\n", vertex_count);
        } else {
            sprintf(message, "Too many GameData polygons: %d!\n", face_count);
        }
        ReportBuildStatus(7, message);
    }
    return 0;
}

/* Builds the switch-interface tables from the conditional-face triples
   collected by the non-primary WGD pass: one interface per id, one state per
   group, and the counted conditional-poly lists. The interface record id and
   first-state index are written before the group scan, the state count after. */
// FUNCTION: WIZ8 0x00447FB0
void W8GameData::CompileGDInterfaces00447FB0(const int* records, int count)
{
    int states[3000];
    int group_ids[100];
    int group_counts[100];
    int group_polys[100 * 100];
    int poly_scratch[5001];
    int record_index;
    int group;
    int poly;

    m_pInterfaces =
        static_cast<W8GDInterface*>(malloc((m_iNumInterfaces + 2) * sizeof(W8GDInterface)));
    if (m_pInterfaces == 0) {
        srAssertFail("m_pInterfaces", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x1df,
                     "CompileGDInterfaces: Couldn't allocate GD Interfaces.");
    }
    memset(m_pInterfaces, 0, (m_iNumInterfaces + 2) * sizeof(W8GDInterface));
    memset(states, 0, sizeof(states));
    m_iNumCondPolys = 1;
    int interface_id;
    for (interface_id = 1; interface_id < m_iNumInterfaces; ++interface_id) {
        W8GDInterface* gd_interface = &m_pInterfaces[interface_id];
        gd_interface->id_00 = interface_id;
        gd_interface->iStates = m_iNumStates;
        memset(group_counts, 0, sizeof(group_counts));
        int group_count = 1;
        for (record_index = 0; record_index < count; ++record_index) {
            const int* record = records + record_index * 3;
            if (record[0] == interface_id) {
                bool found = false;
                for (group = 0; group < group_count; ++group) {
                    if (record[2] == group_ids[group]) {
                        group_polys[group * 100 + group_counts[group]] = record[1];
                        ++group_counts[group];
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    group_ids[group] = record[2];
                    group_polys[group * 100 + group_counts[group]] = record[1];
                    ++group_counts[group];
                    ++group_count;
                }
            }
        }
        gd_interface->state_count_04 = group_count;
        for (group = 0; group < group_count; ++group) {
            states[m_iNumStates * 3] = group_ids[group];
            states[m_iNumStates * 3 + 1] = group_counts[group];
            states[m_iNumStates * 3 + 2] = m_iNumCondPolys;
            ++m_iNumStates;
            for (poly = 0; poly < group_counts[group]; ++poly) {
                poly_scratch[++m_iNumCondPolys] = group_polys[group * 100 + poly];
            }
            poly_scratch[++m_iNumCondPolys] = 0;
        }
    }
    m_pStates =
        static_cast<W8GDInterfaceState*>(malloc((m_iNumStates + 2) * sizeof(W8GDInterfaceState)));
    if (m_pStates == 0) {
        srAssertFail("m_pStates", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x20e,
                     "CompileGDInterfaces: Couldn't allocate GDState array.");
    }
    memcpy(m_pStates, states, m_iNumStates * sizeof(W8GDInterfaceState));
    m_piCondPolys = static_cast<int*>(malloc(m_iNumCondPolys * 4 + 8));
    if (m_piCondPolys == 0) {
        srAssertFail("m_piCondPolys", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x211,
                     "CompileGDInterfaces: Couldn't allocate Conditional poly array.");
    }
    memcpy(m_piCondPolys, poly_scratch, m_iNumCondPolys * sizeof(int));
    for (interface_id = 1; interface_id < m_iNumInterfaces; ++interface_id) {
        SetInterfaceState(interface_id, 0);
    }
}

/* Answers the 1-based ordinal of the name-table entry matching `name`,
   else -1. ReadWGDList keeps the same search inline instead of calling this. */
// FUNCTION: WIZ8 0x004482A0
int W8GameData::FindPointerByName(const char* name)
{
    if (m_ppNames != 0) {
        int index = 0;
        while (index < m_iNumNames) {
            if (strcmp(m_ppNames[index], name) == 0) {
                return index + 1;
            }
            ++index;
        }
    }
    return -1;
}

// FUNCTION: WIZ8 0x00448310
void W8GameData::AddTriggerPlane(const srVector3T<float>* trigger_vertices, Trigger* trigger)
{
    int trigger_index = 0;
    int index;
    if (octree_04 != 0) {
        if (m_ppTriggers == 0) {
            g_integrated_trigger_count = 0;
            m_ppTriggers = static_cast<Trigger**>(malloc(m_iNumTriggers * sizeof(Trigger*) + 4));
            if (m_ppTriggers == 0) {
                srAssertFail("m_ppTriggers", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                             0x256, "AddTriggerPlane: Couldn't allocate trigger array.");
            }
        }
        if (g_integrated_trigger_count >= m_iNumTriggers) {
            srAssertFail("(iTriggerCount < m_iNumTriggers)",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x259,
                         "AddTriggerPlane: Too many triggers for trigger array.");
        }
        m_ppTriggers[g_integrated_trigger_count++] = trigger;
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
        srAssertFail("(m_iNumTrigSurfaces < MAX_TRIG_SURFACES)",
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
    surface->contact_margin_40 = 1.1f;
    surface->vertex_indices_18[0] = m_iNumTrigVertices - 4;
    surface->vertex_indices_18[1] = m_iNumTrigVertices - 3;
    surface->vertex_indices_18[2] = m_iNumTrigVertices - 2;
    ClassifySurfacePlane(m_pTrigVertices, surface);
    for (index = 0; index < 3; ++index) {
        surface->vertex_indices_18[index] += m_iNumVertices;
    }
    surface->edge_link_0c[0] = -1;
    surface->edge_link_0c[1] = -1;
    surface->edge_link_0c[2] = -1;
    surface->hit_plane_38 = 0;
    ++m_iNumTrigSurfaces;

    surface = &m_pTrigSurfaces[m_iNumTrigSurfaces];
    surface->flags_00 = 0x80;
    surface->index_04 = m_iNumSurfaces + m_iNumTrigSurfaces;
    surface->trigger_index_08 = trigger_index;
    surface->contact_margin_40 = 1.1f;
    surface->vertex_indices_18[0] = m_iNumTrigVertices - 2;
    surface->vertex_indices_18[1] = m_iNumTrigVertices - 1;
    surface->vertex_indices_18[2] = m_iNumTrigVertices - 4;
    ClassifySurfacePlane(m_pTrigVertices, surface);
    for (index = 0; index < 3; ++index) {
        surface->vertex_indices_18[index] += m_iNumVertices;
    }
    surface->edge_link_0c[0] = -1;
    surface->edge_link_0c[1] = -1;
    surface->edge_link_0c[2] = -1;
    surface->hit_plane_38 = 0;
    ++m_iNumTrigSurfaces;
}

/* Registers a level-file plane's two triangles (vertices 0,1,2 and 2,3,0) as
   a trigger-surface pair under the auto-numbered trigger index. */
// FUNCTION: WIZ8 0x004485F0
void W8GameData::AddLevelPlane(W8LevelFilePlane* plane)
{
    int index;
    const srVector3T<float>* vertices = plane->vertices_00;

    if (m_pTrigSurfaces == 0) {
        m_pTrigSurfaces = static_cast<W8GDSurface*>(malloc(500 * sizeof(W8GDSurface)));
        if (m_pTrigSurfaces == 0) {
            srAssertFail("m_pTrigSurfaces", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                         0x2c3, "AddTriggerPlane: Couldn't allocate trigger surfaces.");
        }
        m_pTrigVertices =
            static_cast<srVector3T<float>*>(srHeap.allocate(1000 * sizeof(srVector3T<float>)));
        if (m_pTrigVertices == 0) {
            srAssertFail("m_pTrigVertices", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                         0x2c5, "AddTriggerPlane: Couldn't allocate trigger vertices.");
        }
        m_ppTriggers = static_cast<Trigger**>(malloc(500 * sizeof(Trigger*)));
        if (m_ppTriggers == 0) {
            srAssertFail("m_ppTriggers", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                         0x2c7, "AddTriggerPlane: Couldn't allocate trigger array.");
        }
        m_iNumTrigSurfaces = 0;
        m_iNumTrigVertices = 0;
        m_iNumTriggers = 0;
    }
    if (m_iNumTrigSurfaces >= 500) {
        srAssertFail("(m_iNumTrigSurfaces < MAX_TRIG_SURFACES)",
                     "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x2cc, 0);
    }
    for (index = 0; index < 4; ++index) {
        m_pTrigVertices[m_iNumTrigVertices].x = vertices[index].x * g_world_scale;
        m_pTrigVertices[m_iNumTrigVertices].y = vertices[index].y * g_world_scale;
        m_pTrigVertices[m_iNumTrigVertices].z = vertices[index].z * g_world_scale;
        ++m_iNumTrigVertices;
    }

    W8GDSurface* surface = &m_pTrigSurfaces[m_iNumTrigSurfaces];
    surface->flags_00 = 0x80;
    surface->index_04 = m_iNumSurfaces + m_iNumTrigSurfaces;
    surface->trigger_index_08 = m_iNumTriggers;
    surface->contact_margin_40 = 1.1f;
    surface->vertex_indices_18[0] = m_iNumTrigVertices - 4;
    surface->vertex_indices_18[1] = m_iNumTrigVertices - 3;
    surface->vertex_indices_18[2] = m_iNumTrigVertices - 2;
    ClassifySurfacePlane(m_pTrigVertices, surface);
    for (index = 0; index < 3; ++index) {
        surface->vertex_indices_18[index] += m_iNumVertices;
    }
    surface->edge_link_0c[0] = -1;
    surface->edge_link_0c[1] = -1;
    surface->edge_link_0c[2] = -1;
    surface->hit_plane_38 = 0;
    ++m_iNumTrigSurfaces;

    surface = &m_pTrigSurfaces[m_iNumTrigSurfaces];
    surface->flags_00 = 0x80;
    surface->index_04 = m_iNumSurfaces + m_iNumTrigSurfaces;
    surface->trigger_index_08 = m_iNumTriggers;
    surface->contact_margin_40 = 1.1f;
    surface->vertex_indices_18[0] = m_iNumTrigVertices - 2;
    surface->vertex_indices_18[1] = m_iNumTrigVertices - 1;
    surface->vertex_indices_18[2] = m_iNumTrigVertices - 4;
    ClassifySurfacePlane(m_pTrigVertices, surface);
    for (index = 0; index < 3; ++index) {
        surface->vertex_indices_18[index] += m_iNumVertices;
    }
    surface->edge_link_0c[0] = -1;
    surface->edge_link_0c[1] = -1;
    surface->edge_link_0c[2] = -1;
    surface->hit_plane_38 = 0;
    ++m_iNumTrigSurfaces;
    ++m_iNumTriggers;
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

/* The CompileGameData00449D10 counterpart of IntegrateTriggers: folds the
   trigger banks into the main vertex and surface arrays without rebuilding
   the spatial index. */
// FUNCTION: WIZ8 0x00448A60
void W8GameData::IntegrateTriggerGeometry()
{
    if (m_iNumTrigVertices != 0) {
        srVector3T<float>* new_vertices = static_cast<srVector3T<float>*>(
            srHeap.allocate((m_iNumTrigVertices + 1 + m_iNumVertices) * sizeof(srVector3T<float>)));
        if (new_vertices == 0) {
            srAssertFail("pNewVertices", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                         0x351, "IntegrateTriggers: Couldn't allocate new vertex array.");
        }
        memcpy(new_vertices, m_pVertices, m_iNumVertices * sizeof(srVector3T<float>));
        memcpy(new_vertices + m_iNumVertices, m_pTrigVertices,
               m_iNumTrigVertices * sizeof(srVector3T<float>));
        m_iNumVertices = m_iNumVertices + m_iNumTrigVertices;
        srHeap.free(m_pVertices);
        srHeap.free(m_pTrigVertices);
        m_pTrigVertices = 0;
        m_iNumTrigVertices = 0;
        integrated_surface_count_34 = m_iNumTrigSurfaces;
        m_pVertices = new_vertices;
        W8GDSurface* new_surfaces = static_cast<W8GDSurface*>(
            malloc((m_iNumSurfaces + 1 + m_iNumTrigSurfaces) * sizeof(W8GDSurface)));
        if (new_surfaces == 0) {
            srAssertFail("pNewSurfaces", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                         0x361, "IntegrateTriggers: Couldn't allocate new surface array.");
        }
        memcpy(new_surfaces, m_pSurfaces, m_iNumSurfaces * sizeof(W8GDSurface));
        memcpy(new_surfaces + m_iNumSurfaces, m_pTrigSurfaces,
               m_iNumTrigSurfaces * sizeof(W8GDSurface));
        free(m_pTrigSurfaces);
        free(m_pSurfaces);
        m_iNumSurfaces = m_iNumSurfaces + m_iNumTrigSurfaces;
        m_pSurfaces = new_surfaces;
        m_pTrigSurfaces = 0;
        m_iNumTrigSurfaces = 0;
    }
}

/* Copies the linked record's 36 serialized vertices into a scratch block and
   registers them as twelve trigger surfaces, then releases the copy. */
// FUNCTION: WIZ8 0x00448BF0
void W8GameData::AddLinkedRecord(const srVector3T<float>* vertices, float value, float scalar,
                                 const signed char* face)
{
    srVector3T<float>* copy =
        static_cast<srVector3T<float>*>(srHeap.allocate(36 * sizeof(srVector3T<float>)));
    for (int index = 0; index < 36; ++index) {
        copy[index] = vertices[index];
    }
    AddTriggerPlane(copy, value, scalar, face);
    srHeap.free(copy);
}

/* Appends a linked record's vertices to the trigger bank, scaled by
   g_double_005ec150, and emits twelve consecutive trigger surfaces under the
   current environment index. The surface numbered `*face` also grows an
   environment record scaled by `value`/`scalar`. */
// FUNCTION: WIZ8 0x00448C60
void W8GameData::AddTriggerPlane(const srVector3T<float>* vertices, float value, float scalar,
                                 const signed char* face)
{
    int index;
    int vertex_base;

    if (m_pTrigSurfaces == 0) {
        m_pTrigSurfaces = static_cast<W8GDSurface*>(malloc(500 * sizeof(W8GDSurface)));
        if (m_pTrigSurfaces == 0) {
            srAssertFail("m_pTrigSurfaces", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                         0x3b0, "AddTriggerPlane: Couldn't allocate trigger surfaces.");
        }
        m_pTrigVertices =
            static_cast<srVector3T<float>*>(srHeap.allocate(1000 * sizeof(srVector3T<float>)));
        if (m_pTrigVertices == 0) {
            srAssertFail("m_pTrigVertices", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                         0x3b2, "AddTriggerPlane: Couldn't allocate trigger vertices.");
        }
        m_ppTriggers = static_cast<Trigger**>(malloc(500 * sizeof(Trigger*)));
        if (m_ppTriggers == 0) {
            srAssertFail("m_ppTriggers", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                         0x3b4, "AddTriggerPlane: Couldn't allocate trigger array.");
        }
        m_iNumTrigSurfaces = 0;
        m_iNumTrigVertices = 0;
        m_iNumTriggers = 0;
    }
    if (m_iNumTrigSurfaces >= 500) {
        srAssertFail("(m_iNumTrigSurfaces < MAX_TRIG_SURFACES)",
                     "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x3b9, 0);
    }
    vertex_base = m_iNumTrigVertices;
    for (index = 0; index < 36; ++index) {
        m_pTrigVertices[m_iNumTrigVertices].x =
            static_cast<float>(vertices[index].x * g_double_005ec150);
        m_pTrigVertices[m_iNumTrigVertices].y =
            static_cast<float>(vertices[index].y * g_double_005ec150);
        m_pTrigVertices[m_iNumTrigVertices].z =
            static_cast<float>(vertices[index].z * g_double_005ec150);
        ++m_iNumTrigVertices;
    }
    for (index = 0; index < 12; ++index) {
        W8GDSurface* surface = &m_pTrigSurfaces[m_iNumTrigSurfaces];
        surface->flags_00 = 0x1080;
        surface->index_04 = m_iNumTrigSurfaces + m_iNumSurfaces;
        surface->trigger_index_08 = m_iNumEnvirons;
        surface->vertex_indices_18[0] = vertex_base;
        surface->vertex_indices_18[1] = vertex_base + 1;
        surface->vertex_indices_18[2] = vertex_base + 2;
        surface->contact_margin_40 = 0.0f;
        vertex_base += 3;
        ClassifySurfacePlane(m_pTrigVertices, surface);
        if (index == *face) {
            CreateGDEnviron00448E60(surface, value);
            W8EnvironRecord* environ_record = m_ppEnvirons[m_iNumEnvirons];
            environ_record->forward_scale_34 = environ_record->forward_scale_34 * scalar;
            environ_record->motion_limit_38 =
                environ_record->forward_scale_34 * environ_record->momentum_scale_3c * 2.0f;
        }
        surface->vertex_indices_18[0] += m_iNumVertices;
        surface->vertex_indices_18[1] += m_iNumVertices;
        surface->vertex_indices_18[2] += m_iNumVertices;
        ++m_iNumTrigSurfaces;
    }
    ++m_iNumEnvirons;
}

/* Grows the environment pointer bank by ten records at a time and appends a
   fresh W8EnvironRecord whose motion vector derives from the linked surface's
   plane scaled by `scale`. */
// FUNCTION: WIZ8 0x00448E60
void W8GameData::CreateGDEnviron00448E60(const W8GDSurface* surface, float scale)
{
    if (m_iNumEnvirons % 10 == 0) {
        unsigned int size = m_iNumEnvirons * sizeof(W8EnvironRecord*) + 0x28;
        W8EnvironRecord** grown = static_cast<W8EnvironRecord**>(malloc(size));
        if (grown == 0) {
            srAssertFail("ppTempEnvirons", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                         0x3ec, 0);
        }
        memset(grown, 0, size);
        for (int index = 0; index < m_iNumEnvirons; ++index) {
            grown[index] = m_ppEnvirons[index];
        }
        free(m_ppEnvirons);
        m_ppEnvirons = grown;
    }
    W8EnvironRecord* environ_record = new W8EnvironRecord();
    if (environ_record == 0) {
        environ_record = 0;
    } else {
        environ_record->ground_latch_04 = false;
        environ_record->value_00 = 0;
        environ_record->value_08 = 0;
        environ_record->gravity_x_10 = 0;
        environ_record->gravity_y_14 = -g_navigator_gravity;
        environ_record->gravity_z_18 = 0;
        environ_record->motion_factor_20 = 1.0f;
        environ_record->vector_24.x = 0.0f;
        environ_record->vector_24.y = 0.0f;
        environ_record->vector_24.z = 0.0f;
        environ_record->motion_step_1c = 0.05f;
        environ_record->world_height_30 = g_default_world_height;
        environ_record->forward_scale_34 =
            g_camera_level_forward_scale * g_navigator_linked_radius_scale;
        environ_record->value_40 = 1.0f;
        environ_record->momentum_scale_3c = g_default_momentum_scale;
        environ_record->motion_limit_38 = g_default_motion_limit;
    }
    m_ppEnvirons[m_iNumEnvirons] = environ_record;
    if (m_ppEnvirons[m_iNumEnvirons] == 0) {
        ReportBuildStatus(7, "CreateGDEnviron: Could not allocate GD_Environ.");
    }
    m_ppEnvirons[m_iNumEnvirons]->gravity_x_10 =
        g_navigator_gravity * surface->plane_24.normal.x * scale;
    m_ppEnvirons[m_iNumEnvirons]->gravity_y_14 =
        (scale * surface->plane_24.normal.y - g_float_005ebb38) * g_navigator_gravity;
    m_ppEnvirons[m_iNumEnvirons]->gravity_z_18 =
        g_navigator_gravity * surface->plane_24.normal.z * scale;
}

struct W8ProcessedGameDataHeader {
    unsigned int version_00;
    srVector3T<float> minimum_04;
    srVector3T<float> maximum_10;
    int vertex_count_1c;
    int surface_count_20;
    int trigger_surface_base_24;
    int trigger_surface_count_28;
    int integrated_surface_count_2c;
    int interface_count_30;
    int state_count_34;
    int trigger_count_38;
    int cond_poly_count_3c;
    int environ_count_40;
    unsigned char padding_44[0x24];
};

static_assert(sizeof(W8ProcessedGameDataHeader) == 0x68, "W8ProcessedGameDataHeader_must_be_0x68");

// FUNCTION: WIZ8 0x0041a820
unsigned char W8EnvironRecord::RescaleToReference(const W8EnvironRecord* reference)
{
    if (reference == 0) {
        float difference = static_cast<float>(fabs(g_navigator_gravity + vector_24.y));
        if (g_navigator_gravity * g_camera_snap_epsilon < difference) {
            return 1;
        }
        difference = static_cast<float>(fabs(forward_scale_34 - g_camera_level_forward_scale));
        if (g_camera_level_forward_scale * g_camera_snap_epsilon < difference) {
            return 1;
        }
        difference = static_cast<float>(fabs(motion_limit_38 - g_default_motion_limit));
        if (g_default_motion_limit * g_camera_snap_epsilon < difference) {
            return 1;
        }
        difference = static_cast<float>(fabs(momentum_scale_3c - g_default_momentum_scale));
        if (g_default_momentum_scale * g_camera_snap_epsilon < difference) {
            return 1;
        }
        return 0;
    }

    float scale = g_navigator_gravity / -reference->gravity_y_14;
    gravity_x_10 *= scale;
    gravity_y_14 *= scale;
    gravity_z_18 *= scale;
    forward_scale_34 *= (g_camera_level_forward_scale / reference->forward_scale_34);
    motion_limit_38 *= (g_default_motion_limit / reference->motion_limit_38);
    momentum_scale_3c *= (g_default_momentum_scale / reference->momentum_scale_3c);
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
    trigger_surface_base_2c_00 = header.trigger_surface_base_24;
    trigger_surface_count_2c_04 = header.trigger_surface_count_28;
    integrated_surface_count_34 = header.integrated_surface_count_2c;
    m_iNumVertices = header.vertex_count_1c;
    m_iNumInterfaces = header.interface_count_30;
    m_iNumStates = header.state_count_34;
    m_iNumTriggers = header.trigger_count_38;
    m_iNumCondPolys = header.cond_poly_count_3c;
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
                     "ReadProcessedGameData: Couldn't read vertices.\n");
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
            environ_record->ground_latch_04 = false;
            environ_record->value_08 = 0;
            environ_record->gravity_x_10 = 0;
            environ_record->gravity_y_14 = -g_navigator_gravity;
            environ_record->gravity_z_18 = 0;
            environ_record->motion_step_1c = 0.05f;
            environ_record->motion_factor_20 = 1.0f;
            environ_record->vector_24.x = 0.0f;
            environ_record->vector_24.y = 0.0f;
            environ_record->vector_24.z = 0.0f;
            environ_record->world_height_30 = g_default_world_height;
            environ_record->forward_scale_34 =
                g_camera_level_forward_scale * g_navigator_linked_radius_scale;
            environ_record->motion_limit_38 = g_default_momentum_scale;
            environ_record->momentum_scale_3c = g_default_motion_limit;
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
/* 0x0044902E is the constructor's shared body entry: the SEH wrapper at
   0x00449010 zeroes EBX and falls through into the code below. */
// SYNTHETIC: WIZ8 0x0044902E
// W8GameData::W8GameData shared constructor entry
// FUNCTION: WIZ8 0x00449010
W8GameData::W8GameData(int handle, bool secondary)
{
    geometry_index_00 = 0;
    octree_04 = 0;
    m_iNumVertices = 0;
    m_pVertices = 0;
    integrated_surface_count_34 = 0;
    trigger_surface_count_2c_04 = 0;
    trigger_surface_base_2c_00 = 0;
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
    last_hit_surface_54 = 0;
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
    trace_flag4_gate_88 = 0;
    minimum_08 = 1.0e8f;
    maximum_14 = -1.0e8f;
    if (!secondary) {
        MoveTimer(4);
        if (g_game_time_accumulator == 0) {
            g_game_time_accumulator = new W8GameTimeAccumulator();
        }
    }
    if (handle != 0) {
        ReadProcessedGameData(handle);
    }
    if (g_environ != 0) {
        delete g_environ;
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
            environ_record->ground_latch_04 = false;
            environ_record->value_08 = 0;
            environ_record->gravity_x_10 = 0;
            environ_record->gravity_y_14 = -g_navigator_gravity;
            environ_record->gravity_z_18 = 0;
            environ_record->motion_step_1c = 0.05f;
            environ_record->motion_factor_20 = 1.0f;
            environ_record->vector_24.x = 0.0f;
            environ_record->vector_24.y = 0.0f;
            environ_record->vector_24.z = 0.0f;
            environ_record->world_height_30 = g_default_world_height;
            environ_record->forward_scale_34 =
                g_camera_level_forward_scale * g_navigator_linked_radius_scale;
            environ_record->motion_limit_38 = g_default_momentum_scale;
            environ_record->momentum_scale_3c = g_default_motion_limit;
            environ_record->value_40 = 1.0f;
        }
        m_ppEnvirons[0] = environ_record;
    }
    W8LevelDataRecord* old_level = g_level_data;
    g_environ = m_ppEnvirons[0];
    if (old_level != 0) {
        delete old_level;
        g_level_data = 0;
    }
    g_octree_game_data = this;
}

/* Build the processed level's spatial index once and publish every surface
   from its primary 0x4c-byte bank.  The constructor expands only local bounds,
   leaving the serialized GameData limits unchanged. */
// FUNCTION: WIZ8 0x004497c0
unsigned char InitializeGameData(W8GameData* game_data)
{
    if (game_data == 0) {
        return 0;
    }

    srVector3T<float> minimum = game_data->minimum_08;
    srVector3T<float> maximum = game_data->maximum_14;
    if (game_data->geometry_index_00 == 0) {
        game_data->geometry_index_00 = new W8OctBuildTree(2000.0f, &minimum, &maximum, 0x40, 0);
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
void ClassifySurfacePlane(const srVector3T<float>* vertices, W8GDSurface* surface)
{
    BuildTrianglePlane(&surface->plane_24, &vertices[surface->vertex_indices_18[0]],
                       &vertices[surface->vertex_indices_18[1]],
                       &vertices[surface->vertex_indices_18[2]]);

    unsigned int flags = surface->flags_00;
    if ((flags & 0x80) != 0) {
        float largest = g_float_005ebb34;
        unsigned int dominant_axis = 0;
        for (int axis = 0; axis < 3; ++axis) {
            float magnitude = static_cast<float>(fabs((&surface->plane_24.normal.x)[axis]));
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
    if (g_float_005ebc7c < surface->plane_24.normal.y) {
        if ((surface->flags_00 & 4) == 0 && g_float_005ec1a0 < surface->plane_24.normal.y) {
            surface->flags_00 |= 4;
            surface->slope_48 = g_float_005ebb38;
        }
        if (surface->slope_48 < g_float_005ebb34) {
            surface->flags_00 |= 0x20;
            surface->slope_48 = g_float_005ebb34;
        }
    } else if (surface->contact_margin_40 < g_float_005ec028 &&
               g_path_endpoint_scale < surface->contact_margin_40 && (surface->flags_00 & 4) != 0) {
        surface->contact_margin_40 = 0.1f;
    }

    flags = surface->flags_00;
    surface->contact_margin_40 *= g_world_scale;
    if ((flags & 4) == 0) {
        surface->slope_48 = g_float_005ebb34;
    } else if (surface->slope_48 < g_float_005ebc58 && (flags & 0x20) == 0) {
        if (surface->plane_24.normal.y <= g_float_005ebccc) {
            upper_value = surface->plane_24.normal.y;
        }
        surface->slope_48 = upper_value;
    }
    surface->flags_00 = flags & ~8U;
}

/* Header-visible SetPlaneFromThreePoints. This TU unrolls the three-point
   copy; 0x0046D660 lowers the same assignments as a component countdown. */
// FUNCTION: WIZ8 0x00449a40
void BuildTrianglePlane(W8Plane* plane, const srVector3T<float>* first,
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

    ReleaseLevelData();
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
    g_octree_game_data = 0;
}

static char ShareSurfaceEdge(W8GDSurface* first, W8GDSurface* second, srVector3T<float>* vertices);
static void LinkSurfaceEdge(int polygon, int edge, W8HashTable<unsigned int, int>* table,
                            W8GDSurface* surfaces, unsigned int multiplier,
                            srVector3T<float>* vertices);

/* Welds duplicate vertices through a spatial hash, repacks the surface array
   collision-flag faces first, fills the shared build vertex/polygon arrays
   and stitches polygon edge links. */
// FUNCTION: WIZ8 0x00449D10
void W8GameData::CompileGameData00449D10()
{
    W8HashTable<unsigned int, int> weld_table;
    W8HashTable<unsigned int, int> edge_table;
    char message[1024];
    int i;
    int j;
    unsigned int progress = 0;
    int redundant = 0;
    int weld_count = 0;
    bool announce = false;
    bool found;

    ReportStartupMessage(g_string_005ff56c);
    ReportStartupMessage("Processing GameData geometry...\n");
    IntegrateTriggerGeometry();
    if (m_iNumVertices == 0 || m_iNumSurfaces == 0) {
        return;
    }

    W8OctPreTreeVertex* weld_records =
        static_cast<W8OctPreTreeVertex*>(malloc(m_iNumVertices * sizeof(W8OctPreTreeVertex)));
    if (weld_records == 0) {
        ReportBuildStatus(
            7, reinterpret_cast<const char*>( // reinterpret-ok: String returns UINT8*
                   String("CompileGameData: Couldn't allocate %d OctVerts (%dK).\n", m_iNumVertices,
                          m_iNumVertices * sizeof(W8OctPreTreeVertex) / 1024)));
    }
    memset(weld_records, 0, m_iNumVertices * sizeof(W8OctPreTreeVertex));
    g_gd_vertices =
        static_cast<W8OctPreTreeVertex*>(malloc(m_iNumVertices * sizeof(W8OctPreTreeVertex)));
    if (g_gd_vertices == 0) {
        ReportBuildStatus(
            7, reinterpret_cast<const char*>( // reinterpret-ok: String returns UINT8*
                   String("CompileGameData: Couldn't allocate %d NewGDVerts (%dK)\n",
                          m_iNumVertices, m_iNumVertices * sizeof(W8OctPreTreeVertex) / 1024)));
    }
    memset(g_gd_vertices, 0, m_iNumVertices * sizeof(W8OctPreTreeVertex));
    int* cond_polys = 0;
    if (m_iNumCondPolys != 0) {
        cond_polys = static_cast<int*>(malloc(m_iNumCondPolys * sizeof(int)));
        if (cond_polys == 0) {
            ReportBuildStatus(7, "CompileGameData: Couldn't allocate piNewCondPolys array.");
        }
        memset(cond_polys, 0, m_iNumCondPolys * sizeof(int));
    }
    srVector3T<float>* new_vertices = static_cast<srVector3T<float>*>(
        srHeap.allocate(m_iNumVertices * sizeof(srVector3T<float>)));
    if (new_vertices == 0) {
        ReportBuildStatus(7, "CompileGameData: Couldn't allocate New vertex list.");
    }

    if (m_pVertices != 0 && 0 < m_iNumVertices) {
        W8OctPreTreeVertex* vertex = weld_records;
        const srVector3T<float>* source = m_pVertices;
        srVector3T<float>* new_vertex = new_vertices;
        W8OctPreTreeVertex* gd_vertex = g_gd_vertices;
        for (i = 0; i < m_iNumVertices; ++i) {
            vertex->position_0c = *source;
            unsigned int percent =
                static_cast<unsigned int>(i * g_octree_cell_scale / m_iNumVertices);
            if (progress + 10 < percent) {
                announce = true;
                progress += 10;
            }
            found = false;
            unsigned int key = static_cast<unsigned int>(
                vertex->position_0c.z * g_float_005ebc60 * g_float_005ec1b4 +
                vertex->position_0c.y * g_float_005ebc60 * g_float_005ec1b0 +
                vertex->position_0c.x * g_float_005ebc60 * g_float_005ec1ac);
            int linked = 0;
            int slot = weld_table.bucket_heads[W8HashValue(key) & (weld_table.bucket_count - 1)];
            while (slot != -1) {
                if (weld_table.entries[slot].key == key) {
                    linked = weld_table.entries[slot].value;
                    break;
                }
                slot = weld_table.entries[slot].next_index;
            }
            if (linked == 0) {
                if (weld_table.free_head == -1) {
                    weld_table.Grow();
                }
                slot = weld_table.free_head;
                weld_table.free_head = weld_table.entries[slot].next_index;
                unsigned int bucket = W8HashValue(key) & (weld_table.bucket_count - 1);
                weld_table.entries[slot].key = key;
                weld_table.entries[slot].value = i + 1;
                weld_table.entries[slot].next_index = weld_table.bucket_heads[bucket];
                weld_table.bucket_heads[bucket] = slot;
            } else {
                int last = 0;
                while (linked != 0) {
                    if (found) {
                        break;
                    }
                    int candidate_index = linked - 1;
                    W8OctPreTreeVertex* candidate = weld_records + candidate_index;
                    if (fabs(vertex->position_0c.x - candidate->position_0c.x) >=
                            g_camera_snap_epsilon ||
                        fabs(vertex->position_0c.y - candidate->position_0c.y) >=
                            g_camera_snap_epsilon ||
                        fabs(vertex->position_0c.z - candidate->position_0c.z) >=
                            g_camera_snap_epsilon) {
                        last = candidate_index;
                        linked = candidate->kind_20;
                    } else {
                        vertex->vertex_index_04 = candidate->vertex_index_04;
                        ++redundant;
                        found = true;
                        linked = candidate_index;
                    }
                }
                if (!found) {
                    weld_records[last].kind_20 = weld_count + 1;
                }
            }
            if (!found) {
                *new_vertex = *source;
                vertex->vertex_index_04 = weld_count;
                *gd_vertex = *vertex;
                ++weld_count;
                ++gd_vertex;
                ++new_vertex;
            }
            if (announce) {
                sprintf(message, "  %d%% Complete:  %d Redundant Vertices  \r", progress,
                        redundant);
                ReportStartupMessage(message);
            }
            vertex->visited_0a = 0;
            ++vertex;
            ++source;
            announce = false;
        }
    }

    for (i = 0; i < m_iNumSurfaces; ++i) {
        W8GDSurface* surface = m_pSurfaces + i;
        for (j = 0; j < 3; ++j) {
            surface->vertex_indices_18[j] =
                weld_records[surface->vertex_indices_18[j]].vertex_index_04;
        }
    }
    m_iNumVertices = weld_count;
    unsigned int multiplier =
        weld_count < 0xffff ? 0xffff : 0xffffffffu / static_cast<unsigned int>(weld_count);

    g_gd_polygons =
        static_cast<W8OctRegionPolygon*>(malloc(m_iNumSurfaces * sizeof(W8OctRegionPolygon)));
    if (g_gd_polygons == 0) {
        ReportBuildStatus(7, "CompileGameData: Couldn't allocate gpGDPolys.");
    }
    memset(g_gd_polygons, 0, m_iNumSurfaces * sizeof(W8OctRegionPolygon));
    W8GDSurface* new_surfaces =
        static_cast<W8GDSurface*>(malloc(m_iNumSurfaces * sizeof(W8GDSurface)));
    if (new_surfaces == 0) {
        ReportBuildStatus(7, "CompileGameData: Couldn't allocate GameSurfaces.");
    }
    memset(new_surfaces, 0, m_iNumSurfaces * sizeof(W8GDSurface));

    int polygon_count = 0;
    int old_index;
    int old_surface_count;
    for (old_index = 0; old_index < m_iNumSurfaces; ++old_index) {
        W8GDSurface* surface = m_pSurfaces + old_index;
        if (surface->vertex_indices_18[0] != surface->vertex_indices_18[1] &&
            surface->vertex_indices_18[0] != surface->vertex_indices_18[2] &&
            surface->vertex_indices_18[1] != surface->vertex_indices_18[2] &&
            (surface->flags_00 & 4) != 0) {
            surface->index_04 = polygon_count;
            W8GDSurface* compiled = new_surfaces + polygon_count;
            *compiled = *surface;
            compiled->edge_link_0c[2] = -1;
            compiled->edge_link_0c[1] = -1;
            compiled->edge_link_0c[0] = -1;
            compiled->hit_plane_38 = 0;
            W8OctRegionPolygon* polygon = g_gd_polygons + polygon_count;
            polygon->ordinal_04 = polygon_count;
            polygon->plane_08 = compiled->plane_24;
            polygon->degenerate_30 = 0;
            polygon->visited_31 = false;
            polygon->vertices_34[0] = g_gd_vertices + compiled->vertex_indices_18[0];
            polygon->vertices_34[1] = g_gd_vertices + compiled->vertex_indices_18[1];
            polygon->vertices_34[2] = g_gd_vertices + compiled->vertex_indices_18[2];
            for (j = 0; j < 3; ++j) {
                LinkSurfaceEdge(polygon_count, j, &edge_table, new_surfaces, multiplier,
                                new_vertices);
            }
            for (j = 0; j < m_iNumCondPolys; ++j) {
                if (m_piCondPolys[j] == old_index) {
                    cond_polys[j] = polygon_count;
                }
            }
            ++polygon_count;
        }
    }
    old_surface_count = m_iNumSurfaces;
    m_iNumSurfaces = polygon_count;
    for (old_index = 0; old_index < old_surface_count; ++old_index) {
        W8GDSurface* surface = m_pSurfaces + old_index;
        if (surface->vertex_indices_18[0] != surface->vertex_indices_18[1] &&
            surface->vertex_indices_18[0] != surface->vertex_indices_18[2] &&
            surface->vertex_indices_18[1] != surface->vertex_indices_18[2] &&
            (surface->flags_00 & 4) == 0) {
            surface->index_04 = polygon_count;
            surface->slope_48 = 0;
            W8GDSurface* compiled = new_surfaces + polygon_count;
            *compiled = *surface;
            compiled->edge_link_0c[2] = -1;
            compiled->edge_link_0c[1] = -1;
            compiled->edge_link_0c[0] = -1;
            compiled->hit_plane_38 = 0;
            W8OctRegionPolygon* polygon = g_gd_polygons + polygon_count;
            polygon->ordinal_04 = polygon_count;
            polygon->plane_08 = compiled->plane_24;
            polygon->degenerate_30 = 0;
            polygon->visited_31 = false;
            polygon->vertices_34[0] = g_gd_vertices + compiled->vertex_indices_18[0];
            polygon->vertices_34[1] = g_gd_vertices + compiled->vertex_indices_18[1];
            polygon->vertices_34[2] = g_gd_vertices + compiled->vertex_indices_18[2];
            for (j = 0; j < 3; ++j) {
                LinkSurfaceEdge(polygon_count, j, &edge_table, new_surfaces, multiplier,
                                new_vertices);
            }
            for (j = 0; j < m_iNumCondPolys; ++j) {
                if (m_piCondPolys[j] == old_index) {
                    cond_polys[j] = polygon_count;
                }
            }
            ++polygon_count;
        }
    }
    trigger_surface_count_2c_04 = polygon_count - m_iNumSurfaces;
    trigger_surface_base_2c_00 = m_iNumSurfaces;
    m_iNumSurfaces = polygon_count;
    free(weld_records);
    srHeap.free(m_pVertices);
    free(m_pSurfaces);
    if (m_iNumCondPolys != 0) {
        free(m_piCondPolys);
        m_piCondPolys = cond_polys;
    }
    m_pVertices = new_vertices;
    m_pSurfaces = new_surfaces;
    sprintf(message, "GameData:  %d Polygons,  \t%d Vertices.\n\n", polygon_count, weld_count);
    ReportBuildStatus(6, message);
}

/* Tests two polygons for a shared vertex pair; when they share an edge the
   matching corner slot on each surface is linked to the other's index_04. */
// FUNCTION: WIZ8 0x0044A970
static char ShareSurfaceEdge(W8GDSurface* first, W8GDSurface* second, srVector3T<float>* vertices)
{
    int first_slot = -1;
    int second_slot = -1;
    int last_first = -1;
    int last_second = -1;
    int* first_index = first->vertex_indices_18;
    for (int i = 0; i < 3; ++i) {
        int* second_index = second->vertex_indices_18;
        for (int j = 0; j < 3; ++j) {
            if (*first_index == *second_index) {
                if (first_slot < 0) {
                    second_slot = j;
                    first_slot = i;
                } else {
                    last_first = i;
                    last_second = j;
                }
            }
            ++second_index;
        }
        ++first_index;
    }
    if (last_first < 0) {
        return 0;
    }
    if ((last_first < first_slot && last_first - first_slot < 2) ||
        (first_slot < last_first && 1 < last_first - first_slot)) {
        first_slot = last_first;
    }
    if ((last_second < second_slot && last_second - second_slot < 2) ||
        (second_slot < last_second && 1 < last_second - second_slot)) {
        second_slot = last_second;
    }
    first->edge_link_0c[first_slot] = second->index_04;
    second->edge_link_0c[second_slot] = first->index_04;
    return 1;
}

/* Registers one triangle edge in the edge hash under its undirected vertex
   pair key; when an earlier polygon carries the same key the pair is handed
   to ShareSurfaceEdge to link. */
// FUNCTION: WIZ8 0x0044A7D0
static void LinkSurfaceEdge(int polygon, int edge, W8HashTable<unsigned int, int>* table,
                            W8GDSurface* surfaces, unsigned int multiplier,
                            srVector3T<float>* vertices)
{
    W8GDSurface* surface = surfaces + polygon;
    int next = (edge + 1) % 3;
    int low = edge;
    int high = next;
    if (surface->vertex_indices_18[next] < surface->vertex_indices_18[edge]) {
        low = next;
        high = edge;
    }
    unsigned int key =
        surface->vertex_indices_18[low] * multiplier + surface->vertex_indices_18[high];
    unsigned int hash = W8HashValue(key);
    int slot = table->bucket_heads[hash & (table->bucket_count - 1)];
    if (slot != -1) {
        do {
            if (table->entries[slot].key == key) {
                int index = table->entries[slot].value;
                if (index != 0) {
                    bool linked = false;
                    do {
                        if (linked) {
                            return;
                        }
                        if (ShareSurfaceEdge(surfaces + index, surface, vertices) != 0) {
                            linked = true;
                        } else {
                            index = table->FindNextEntry(&key, index);
                        }
                    } while (index != 0);
                    if (linked) {
                        return;
                    }
                }
                break;
            }
            slot = table->entries[slot].next_index;
        } while (slot != -1);
    }
    if (table->free_head == -1) {
        table->Grow();
    }
    slot = table->free_head;
    table->free_head = table->entries[slot].next_index;
    unsigned int bucket = W8HashValue(key) & (table->bucket_count - 1);
    table->entries[slot].key = key;
    table->entries[slot].value = polygon;
    table->entries[slot].next_index = table->bucket_heads[bucket];
    table->bucket_heads[bucket] = slot;
}

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
    header.trigger_surface_base_24 = trigger_surface_base_2c_00;
    header.trigger_surface_count_28 = trigger_surface_count_2c_04;
    header.integrated_surface_count_2c = integrated_surface_count_34;
    header.interface_count_30 = m_iNumInterfaces;
    header.state_count_34 = m_iNumStates;
    header.trigger_count_38 = m_iNumTriggers;
    header.cond_poly_count_3c = m_iNumCondPolys;
    header.environ_count_40 = m_iNumEnvirons;
    memset(header.padding_44, 0, sizeof(header.padding_44));

    if (handle == 0) {
        ReportBuildStatus(7, "WriteGameData: File not open.\n");
        return 0;
    }
    if (FileWrite(handle, &header, 0x68, 0) == 0) {
        ReportBuildStatus(7, "WriteGameData: Couldn't write GameData info.\n");
        return 0;
    }
    if (FileWrite(handle, m_pVertices, m_iNumVertices * 0xc, 0) == 0) {
        ReportBuildStatus(7, "WriteGameData: Couldn't write vertex info.\n");
        return 0;
    }
    if (FileWrite(handle, m_pSurfaces, m_iNumSurfaces * 0x4c, 0) == 0) {
        ReportBuildStatus(7, "WriteGameData: Couldn't write Surface info.\n");
        return 0;
    }
    if (m_iNumInterfaces != 0 && FileWrite(handle, m_pInterfaces, m_iNumInterfaces * 0xc, 0) == 0) {
        ReportBuildStatus(7, "WriteGameData: Couldn't write switch interface info.\n");
        return 0;
    }
    if (m_iNumStates != 0 && FileWrite(handle, m_pStates, m_iNumStates * 0xc, 0) == 0) {
        ReportBuildStatus(7, "WriteGameData: Couldn't write switch state info.\n");
        return 0;
    }
    if (m_iNumCondPolys != 0 && FileWrite(handle, m_piCondPolys, m_iNumCondPolys * 4, 0) == 0) {
        ReportBuildStatus(7, "WriteGameData: Couldn't write conditional poly list.\n");
        return 0;
    }
    if (m_iNumEnvirons != 0) {
        for (index = 0; index < m_iNumEnvirons; ++index) {
            if (FileWrite(handle, m_ppEnvirons[index], 0x44, 0) == 0) {
                ReportBuildStatus(7, "WriteGameData: Couldn't write GD_Environ.\n");
                return 0;
            }
        }
    }
    return 1;
}
