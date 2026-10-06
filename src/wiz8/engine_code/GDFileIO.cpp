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
const float g_float_negative_one_third = -0.3333333432674408f;
// GLOBAL: WIZ8 0x005ebc58
const float g_vector_length_squared_epsilon = 1.0000000116860974e-07f;
// GLOBAL: WIZ8 0x005ec028
const float g_float_one_and_one_hundredth = 1.0099999904632568f;
// GLOBAL: WIZ8 0x005ec1a0
const float g_surface_flat_normal_threshold = 0.9959999918937683f;
// GLOBAL: WIZ8 0x005ec1ac
const float g_vertex_sort_x_weight = 3000.0f;
// GLOBAL: WIZ8 0x005ec1b0
const float g_vertex_sort_y_weight = 60000.0f;
// GLOBAL: WIZ8 0x005ec1b4
const float g_vertex_sort_z_weight = 900000.0f;
// GLOBAL: WIZ8 0x005ff56c
static char g_string[] = "\n";

// GLOBAL: WIZ8 0x00659a58
static int g_integrated_trigger_count;

// GLOBAL: WIZ8 0x00603ab8
float g_default_momentum_scale = 0.30000001192092896f;
// GLOBAL: WIZ8 0x00603abc
float g_default_motion_limit = 112.5f;

// GLOBAL: WIZ8 0x005ec1a4
const float g_path_endpoint_scale = 0.9900000095367432f;

/* Opens a game-data file, builds its record, and pulls the polygon and
   vertex banks through the record reader. */
// FUNCTION: WIZ8 0x00447570
W8GameData* ReadGameData(const char* path, bool secondary)
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
    got_polygons = game_data->ReadWGDList(file, 0);
    got_vertices = game_data->ReadWGDList(file, 1);
    if (got_vertices == 0 && got_polygons == 0) {
        ReportBuildStatus(7, "ReadGameData: No polygons or vertices in GameData!\n");
    }
    CloseHandle(file);
    return game_data;
}

/* The WGD face record's fixed head: three vertex indexes, the source plane,
   and a version tag checked before the rest of the record is read. */
struct W8GDFaceHeader { /* 0x1c */
    int vertex_indices[3];
    srVector3T<float> plane;
    int version;
};

static_assert(sizeof(W8GDFaceHeader) == 0x1c, "W8GDFaceHeader_must_be_0x1c");

/* The WGD face record's tail: classification flag, slope/value pair, footstep
   selectors, trace chance, and the trigger index the writer overrode. */
struct W8GDFaceData { /* 0x18 */
    int type;
    float slope;
    float contact_margin;
    unsigned char material;
    unsigned char surface;
    unsigned char pad_0e[2];
    int chance;
    int trigger_index;
};

static_assert(sizeof(W8GDFaceData) == 0x18, "W8GDFaceData_must_be_0x18");

/* The conditional-face record following a non-primary face: the group key the
   interface compiler buckets on and the interface's name. */
struct W8GDExtendedFace { /* 0x44 */
    int group;
    char name[0x40];
};

static_assert(sizeof(W8GDExtendedFace) == 0x44, "W8GDExtendedFace_must_be_0x44");

/* Reads one WGD vertex/polygon bank. poly_type 0 builds fresh arrays; any
   other type grows the existing banks and also consumes each face's extended
   name record into the interface tables. */
// FUNCTION: WIZ8 0x00447660
unsigned char W8GameData::ReadWGDList(HANDLE file, int poly_type)
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
    srVector3T<float> bounds[2];

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
                    delete[] old_vertices;
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
                    m_pVertices[index].Set(vertex.x * g_world_scale, vertex.y * g_world_scale,
                                           vertex.z * g_world_scale);
                    if (index == m_iNumVertices) {
                        minimum = vertex;
                        maximum = vertex;
                    } else {
                        if (vertex.x < minimum.x) {
                            minimum.x = vertex.x;
                        }
                        if (maximum.x < vertex.x) {
                            maximum.x = vertex.x;
                        }
                        if (vertex.y < minimum.y) {
                            minimum.y = vertex.y;
                        }
                        if (maximum.y < vertex.y) {
                            maximum.y = vertex.y;
                        }
                        if (vertex.z < minimum.z) {
                            minimum.z = vertex.z;
                        }
                        if (maximum.z < vertex.z) {
                            maximum.z = vertex.z;
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
                    if (header.version != 2) {
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
                    surface->contact_margin = data.contact_margin;
                    surface->slope = data.slope;
                    surface->chance = data.chance;
                    surface->trigger_index = data.trigger_index;
                    surface->footstep_material = data.material;
                    surface->footstep_surface = data.surface;
                    if (data.type == 1) {
                        surface->flags = W8_GD_SURFACE_WALKABLE | W8_GD_SURFACE_PATHFINDING;
                    } else {
                        surface->flags = 0;
                    }
                    surface->plane.normal.Set(header.plane.x, header.plane.y, header.plane.z);
                    float largest = static_cast<float>(fabs(surface->plane.normal.x));
                    unsigned int axis = 0;
                    if (largest < static_cast<float>(fabs(surface->plane.normal.y))) {
                        largest = static_cast<float>(fabs(surface->plane.normal.y));
                        axis = 1;
                    }
                    if (largest < static_cast<float>(fabs(surface->plane.normal.z))) {
                        axis = 2;
                    }
                    surface->flags |= axis;
                    surface->vertex_indices[0] = header.vertex_indices[0] + m_iNumVertices;
                    surface->vertex_indices[1] = header.vertex_indices[1] + m_iNumVertices;
                    surface->vertex_indices[2] = header.vertex_indices[2] + m_iNumVertices;
                    surface->index = index;
                    surface->trigger_index = 0;
                    surface->edge_link[2] = -1;
                    surface->edge_link[1] = -1;
                    surface->edge_link[0] = -1;
                    surface->hit_plane = 0;
                    ClassifySurfacePlane(m_pVertices, surface);
                    if (poly_type != 0) {
                        record[1] = index;
                        record[0] = 0;
                        record[2] = extended.group;
                        int existing_name = FindPointerByName(extended.name);
                        if (existing_name != -1) {
                            record[0] = existing_name;
                        } else {
                            name_index = m_iNumNames;
                            m_ppNames[name_index] = static_cast<char*>(malloc(0x40));
                            if (m_ppNames[name_index] == 0) {
                                srAssertFail("m_ppNames[i2]",
                                             "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                                             0x187, "ReadWGDList: Couldn't allocate name string.");
                            }
                            strcpy(m_ppNames[name_index], extended.name);
                            if (m_iNumInterfaces == 0) {
                                m_iNumInterfaces = 1;
                            }
                            record[0] = m_iNumInterfaces;
                            ++m_iNumInterfaces;
                            ++m_iNumNames;
                        }
                        surface->trigger_index = record[0];
                        ++record_count;
                        record += 3;
                    }
                    ++index;
                }
                ReadFile(file, &bounds[1].x, 4, &bytes_read, 0);
                ReadFile(file, &bounds[1].y, 4, &bytes_read, 0);
                ReadFile(file, &bounds[1].z, 4, &bytes_read, 0);
                ReadFile(file, &bounds[0].x, 4, &bytes_read, 0);
                ReadFile(file, &bounds[0].y, 4, &bytes_read, 0);
                ReadFile(file, &bounds[0].z, 4, &bytes_read, 0);
                for (index = 0; index < 3; ++index) {
                    (&bounds[1].x)[index] = (&bounds[1].x)[index] * g_world_scale;
                    (&bounds[0].x)[index] *= g_world_scale;
                }
                if (bounds[1].x < minimum.x) {
                    minimum.x = bounds[1].x;
                }
                if (bounds[1].y < minimum.y) {
                    minimum.y = bounds[1].y;
                }
                if (bounds[1].z < minimum.z) {
                    minimum.z = bounds[1].z;
                }
                if (maximum.x < bounds[0].x) {
                    maximum.x = bounds[0].x;
                }
                if (maximum.y < bounds[0].y) {
                    maximum.y = bounds[0].y;
                }
                if (maximum.z < bounds[0].z) {
                    maximum.z = bounds[0].z;
                }
                if (m_ppNames != 0 && cond_faces != 0) {
                    CompileGDInterfaces(cond_faces, record_count);
                    free(cond_faces);
                }
                m_iNumVertices += vertex_count;
                m_iNumSurfaces += face_count;
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
void W8GameData::CompileGDInterfaces(const int* records, int count)
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
        gd_interface->id = interface_id;
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
        gd_interface->state_count = group_count;
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
   else -1. */
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

void W8GameData::AddTriggerTriangle(int trigger_index, int vertex_0, int vertex_1, int vertex_2)
{
    W8GDSurface* surface = &m_pTrigSurfaces[m_iNumTrigSurfaces];
    surface->flags = W8_GD_SURFACE_CROSSING;
    surface->index = m_iNumSurfaces + m_iNumTrigSurfaces;
    surface->trigger_index = trigger_index;
    surface->contact_margin = 1.1f;
    surface->vertex_indices[0] = vertex_0;
    surface->vertex_indices[1] = vertex_1;
    surface->vertex_indices[2] = vertex_2;
    ClassifySurfacePlane(m_pTrigVertices, surface);
    for (int index = 0; index < 3; ++index) {
        surface->vertex_indices[index] += m_iNumVertices;
    }
    surface->edge_link[0] = -1;
    surface->edge_link[1] = -1;
    surface->edge_link[2] = -1;
    surface->hit_plane = 0;
    ++m_iNumTrigSurfaces;
}

// FUNCTION: WIZ8 0x00448310
void W8GameData::AddTriggerPlane(const srVector3T<float>* trigger_vertices, Trigger* trigger)
{
    int trigger_index = 0;
    int index;
    if (octree != 0) {
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
        m_pTrigVertices = new srVector3T<float>[1000];
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

    AddTriggerTriangle(trigger_index, m_iNumTrigVertices - 4, m_iNumTrigVertices - 3,
                       m_iNumTrigVertices - 2);

    AddTriggerTriangle(trigger_index, m_iNumTrigVertices - 2, m_iNumTrigVertices - 1,
                       m_iNumTrigVertices - 4);
}

/* Registers a level-file plane's two triangles (vertices 0,1,2 and 2,3,0) as
   a trigger-surface pair under the auto-numbered trigger index. */
// FUNCTION: WIZ8 0x004485F0
void W8GameData::AddLevelPlane(W8LevelFilePlane* plane)
{
    int index;
    const srVector3T<float>* vertices = plane->vertices;

    if (m_pTrigSurfaces == 0) {
        m_pTrigSurfaces = static_cast<W8GDSurface*>(malloc(500 * sizeof(W8GDSurface)));
        if (m_pTrigSurfaces == 0) {
            srAssertFail("m_pTrigSurfaces", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                         0x2c3, "AddTriggerPlane: Couldn't allocate trigger surfaces.");
        }
        m_pTrigVertices = new srVector3T<float>[1000];
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
        m_pTrigVertices[m_iNumTrigVertices].Set(vertices[index].x * g_world_scale,
                                                vertices[index].y * g_world_scale,
                                                vertices[index].z * g_world_scale);
        ++m_iNumTrigVertices;
    }

    AddTriggerTriangle(m_iNumTriggers, m_iNumTrigVertices - 4, m_iNumTrigVertices - 3,
                       m_iNumTrigVertices - 2);

    AddTriggerTriangle(m_iNumTriggers, m_iNumTrigVertices - 2, m_iNumTrigVertices - 1,
                       m_iNumTrigVertices - 4);
    ++m_iNumTriggers;
}

// FUNCTION: WIZ8 0x00448840
void W8GameData::IntegrateTriggers()
{
    if (m_iNumTrigVertices == 0) {
        return;
    }

    int combined_vertex_count = m_iNumVertices + m_iNumTrigVertices;
    srVector3T<float>* combined_vertices = new srVector3T<float>[combined_vertex_count + 1];
    if (combined_vertices == 0) {
        srAssertFail("pNewVertices", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x31b,
                     "IntegrateTriggers: Couldn't allocate new vertex array.");
    }
    memcpy(combined_vertices, m_pVertices, m_iNumVertices * sizeof(srVector3T<float>));
    memcpy(combined_vertices + m_iNumVertices, m_pTrigVertices,
           m_iNumTrigVertices * sizeof(srVector3T<float>));
    m_iNumVertices = combined_vertex_count;
    delete[] m_pVertices;
    delete[] m_pTrigVertices;
    integrated_surface_count = m_iNumTrigSurfaces;
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
        geometry_index->InsertSurface(surface, 3);
    }
    pending_trigger_bits = new BitArray(m_iNumTriggers);
    active_trigger_bits = new BitArray(m_iNumTriggers);
}

/* The CompileGameData counterpart of IntegrateTriggers: folds the
   trigger banks into the main vertex and surface arrays without rebuilding
   the spatial index. */
// FUNCTION: WIZ8 0x00448A60
void W8GameData::IntegrateTriggerGeometry()
{
    if (m_iNumTrigVertices != 0) {
        srVector3T<float>* new_vertices =
            new srVector3T<float>[m_iNumTrigVertices + 1 + m_iNumVertices];
        if (new_vertices == 0) {
            srAssertFail("pNewVertices", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                         0x351, "IntegrateTriggers: Couldn't allocate new vertex array.");
        }
        memcpy(new_vertices, m_pVertices, m_iNumVertices * sizeof(srVector3T<float>));
        memcpy(new_vertices + m_iNumVertices, m_pTrigVertices,
               m_iNumTrigVertices * sizeof(srVector3T<float>));
        m_iNumVertices += m_iNumTrigVertices;
        delete[] m_pVertices;
        delete[] m_pTrigVertices;
        m_pTrigVertices = 0;
        m_iNumTrigVertices = 0;
        integrated_surface_count = m_iNumTrigSurfaces;
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
        m_iNumSurfaces += m_iNumTrigSurfaces;
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
   g_double_five_hundred, and emits twelve consecutive trigger surfaces under the
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
        m_pTrigVertices = new srVector3T<float>[1000];
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
        m_pTrigVertices[m_iNumTrigVertices].Set(
            static_cast<float>(vertices[index].x * g_double_five_hundred),
            static_cast<float>(vertices[index].y * g_double_five_hundred),
            static_cast<float>(vertices[index].z * g_double_five_hundred));
        ++m_iNumTrigVertices;
    }
    for (index = 0; index < 12; ++index) {
        W8GDSurface* surface = &m_pTrigSurfaces[m_iNumTrigSurfaces];
        surface->flags = W8_GD_SURFACE_CROSSING_MASK;
        surface->index = m_iNumTrigSurfaces + m_iNumSurfaces;
        surface->trigger_index = m_iNumEnvirons;
        surface->vertex_indices[0] = vertex_base;
        surface->vertex_indices[1] = vertex_base + 1;
        surface->vertex_indices[2] = vertex_base + 2;
        surface->contact_margin = 0.0f;
        vertex_base += 3;
        ClassifySurfacePlane(m_pTrigVertices, surface);
        if (index == *face) {
            CreateGDEnviron(surface, value);
            W8EnvironRecord* environ_record = m_ppEnvirons[m_iNumEnvirons];
            environ_record->forward_scale *= scalar;
            environ_record->motion_limit =
                environ_record->forward_scale * environ_record->momentum_scale * 2.0f;
        }
        surface->vertex_indices[0] += m_iNumVertices;
        surface->vertex_indices[1] += m_iNumVertices;
        surface->vertex_indices[2] += m_iNumVertices;
        ++m_iNumTrigSurfaces;
    }
    ++m_iNumEnvirons;
}

/* Grows the environment pointer bank by ten records at a time and appends a
   fresh W8EnvironRecord whose motion vector derives from the linked surface's
   plane scaled by `scale`. */
// FUNCTION: WIZ8 0x00448E60
void W8GameData::CreateGDEnviron(const W8GDSurface* surface, float scale)
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

    m_ppEnvirons[m_iNumEnvirons] = environ_record;
    if (m_ppEnvirons[m_iNumEnvirons] == 0) {
        ReportBuildStatus(7, "CreateGDEnviron: Could not allocate GD_Environ.");
    }
    m_ppEnvirons[m_iNumEnvirons]->gravity_x = g_navigator_gravity * surface->plane.normal.x * scale;
    m_ppEnvirons[m_iNumEnvirons]->gravity_y =
        (scale * surface->plane.normal.y - g_float_one) * g_navigator_gravity;
    m_ppEnvirons[m_iNumEnvirons]->gravity_z = g_navigator_gravity * surface->plane.normal.z * scale;
}

struct W8ProcessedGameDataHeader {
    unsigned int version;
    srVector3T<float> minimum;
    srVector3T<float> maximum;
    int vertex_count;
    int surface_count;
    int trigger_surface_base;
    int trigger_surface_count;
    int integrated_surface_count;
    int interface_count;
    int state_count;
    int trigger_count;
    int cond_poly_count;
    int environ_count;
    unsigned char reserved[0x24];
};

static_assert(sizeof(W8ProcessedGameDataHeader) == 0x68, "W8ProcessedGameDataHeader_must_be_0x68");

// FUNCTION: WIZ8 0x0041a820
unsigned char W8EnvironRecord::RescaleToReference(const W8EnvironRecord* reference)
{
    if (reference == 0) {
        float difference = static_cast<float>(fabs(g_navigator_gravity + vector.y));
        if (g_navigator_gravity * g_camera_snap_epsilon < difference) {
            return 1;
        }
        difference = static_cast<float>(fabs(forward_scale - g_camera_level_forward_scale));
        if (g_camera_level_forward_scale * g_camera_snap_epsilon < difference) {
            return 1;
        }
        difference = static_cast<float>(fabs(motion_limit - g_default_motion_limit));
        if (g_default_motion_limit * g_camera_snap_epsilon < difference) {
            return 1;
        }
        difference = static_cast<float>(fabs(momentum_scale - g_default_momentum_scale));
        if (g_default_momentum_scale * g_camera_snap_epsilon < difference) {
            return 1;
        }
        return 0;
    }

    float scale = g_navigator_gravity / -reference->gravity_y;
    gravity_x *= scale;
    gravity_y *= scale;
    gravity_z *= scale;
    forward_scale *= g_camera_level_forward_scale / reference->forward_scale;
    motion_limit *= g_default_motion_limit / reference->motion_limit;
    momentum_scale *= g_default_momentum_scale / reference->momentum_scale;
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
    if (header.version != 1) {
        srAssertFail("(FileGD.iVersion == GAMEDATA_VERSION)",
                     "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x46c,
                     reinterpret_cast<const char*>( // reinterpret-ok: String returns UINT8*
                         String("ReadProcessedGameData: File version %d does not match program "
                                "version %d.",
                                header.version, 1)));
    }

    minimum = header.minimum;
    maximum = header.maximum;
    m_iNumSurfaces = header.surface_count;
    trigger_surface_base = header.trigger_surface_base;
    trigger_surface_count = header.trigger_surface_count;
    integrated_surface_count = header.integrated_surface_count;
    m_iNumVertices = header.vertex_count;
    m_iNumInterfaces = header.interface_count;
    m_iNumStates = header.state_count;
    m_iNumTriggers = header.trigger_count;
    m_iNumCondPolys = header.cond_poly_count;
    m_iNumEnvirons = header.environ_count;

    pending_trigger_bits = new BitArray(m_iNumTriggers);
    active_trigger_bits = new BitArray(m_iNumTriggers);

    m_pVertices = new srVector3T<float>[m_iNumVertices + 2];
    if (m_pVertices == 0) {
        srAssertFail("m_pVertices", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x483,
                     "ReadProcessedGameData: Couldn't allocate vertices.");
    }
    if (FileRead(handle, m_pVertices, m_iNumVertices * sizeof(srVector3T<float>), &bytes_read) ==
        0) {
        srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x487,
                     "ReadProcessedGameData: Couldn't read vertices.\n");
    }

    m_pSurfaces =
        static_cast<W8GDSurface*>(malloc((m_iNumSurfaces * 0x13 + 0x26) * sizeof(unsigned int)));
    if (m_pSurfaces == 0) {
        srAssertFail("m_pSurfaces", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x48c,
                     "ReadProcessedGameData: Couldn't allocate pSurfaces.");
    }
    if (FileRead(handle, m_pSurfaces, m_iNumSurfaces * sizeof(W8GDSurface), &bytes_read) == 0) {
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
        if (FileRead(handle, m_pInterfaces, m_iNumInterfaces * sizeof(W8GDInterface),
                     &bytes_read) == 0) {
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
        if (FileRead(handle, m_pStates, m_iNumStates * sizeof(W8GDInterfaceState), &bytes_read) ==
            0) {
            srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp", 0x4a5,
                         "ReadProcessedGameData: Couldn't read switch state info.");
        }
    }

    if (m_iNumCondPolys != 0) {
        m_piCondPolys = static_cast<int*>(malloc(m_iNumCondPolys * sizeof(unsigned int) + 4));
        if (m_piCondPolys == 0) {
            srAssertFail("m_piCondPolys", "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                         0x4ad, "ReadProcessedGameData: Couldn't allocate switch state info.");
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
            m_ppEnvirons[index] = environ_record;
            if (FileRead(handle, environ_record, sizeof(*environ_record), &bytes_read) == 0) {
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
   environment bank, and the previous level-data teardown. */
/* 0x0044902E is the constructor's shared body entry: the SEH wrapper at
   0x00449010 zeroes EBX and falls through into the code below. */
// FUNCTION: WIZ8 0x00449010
W8GameData::W8GameData(int handle, bool secondary)
{
    geometry_index = 0;
    octree = 0;
    m_iNumVertices = 0;
    m_pVertices = 0;
    integrated_surface_count = 0;
    trigger_surface_count = 0;
    trigger_surface_base = 0;
    m_iNumSurfaces = 0;
    m_pSurfaces = 0;
    m_pTrigSurfaces = 0;
    m_pTrigVertices = 0;
    m_ppTriggers = 0;
    m_iNumTrigSurfaces = 0;
    m_iNumTrigVertices = 0;
    m_iNumTriggers = 0;
    pending_trigger_bits = 0;
    active_trigger_bits = 0;
    last_hit_surface = 0;
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
    trace_flag4_gate = false;
    minimum = 1.0e8f;
    maximum = -1.0e8f;
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

        m_ppEnvirons[0] = environ_record;
    }
    W8LevelDataRecord* old_level = g_level_data;
    g_environ = m_ppEnvirons[0];
    if (old_level != 0) {
        delete old_level;
        g_level_data = 0;
    }
    SetOctreeGameData(this);
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

    srVector3T<float> minimum = game_data->minimum;
    srVector3T<float> maximum = game_data->maximum;
    if (game_data->geometry_index == 0) {
        game_data->geometry_index = new W8OctBuildTree(2000.0f, &minimum, &maximum, 0x40, 0);
    }

    for (int index = 0; index < game_data->m_iNumSurfaces; ++index) {
        if (game_data->geometry_index->InsertSurface(&game_data->m_pSurfaces[index], 3) == 0) {
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
    BuildTrianglePlane(&surface->plane, &vertices[surface->vertex_indices[0]],
                       &vertices[surface->vertex_indices[1]],
                       &vertices[surface->vertex_indices[2]]);

    unsigned int flags = surface->flags;
    if ((flags & W8_GD_SURFACE_CROSSING) != 0) {
        float largest = g_float_zero;
        unsigned int dominant_axis = 0;
        for (int axis = 0; axis < 3; ++axis) {
            float magnitude = static_cast<float>(fabs((&surface->plane.normal.x)[axis]));
            if (largest < magnitude) {
                largest = magnitude;
                dominant_axis = axis;
            }
        }
        flags |= dominant_axis;
        surface->flags = flags;
    }

    if ((surface->flags & W8_GD_SURFACE_WALKABLE) != 0) {
        surface->flags |= W8_GD_SURFACE_PATHFINDING;
    }

    float upper_value = g_float_one;
    if (g_float_half < surface->plane.normal.y) {
        if ((surface->flags & W8_GD_SURFACE_WALKABLE) == 0 &&
            g_surface_flat_normal_threshold < surface->plane.normal.y) {
            surface->flags |= W8_GD_SURFACE_WALKABLE;
            surface->slope = g_float_one;
        }
        if (surface->slope < g_float_zero) {
            surface->flags |= W8_GD_SURFACE_EXPLICIT_SLOPE;
            surface->slope = g_float_zero;
        }
    } else if (surface->contact_margin < g_float_one_and_one_hundredth &&
               g_path_endpoint_scale < surface->contact_margin &&
               (surface->flags & W8_GD_SURFACE_WALKABLE) != 0) {
        surface->contact_margin = 0.1f;
    }

    flags = surface->flags;
    surface->contact_margin *= g_world_scale;
    if ((flags & W8_GD_SURFACE_WALKABLE) == 0) {
        surface->slope = g_float_zero;
    } else if (surface->slope < g_vector_length_squared_epsilon &&
               (flags & W8_GD_SURFACE_EXPLICIT_SLOPE) == 0) {
        if (surface->plane.normal.y <= g_float_three_quarters) {
            upper_value = surface->plane.normal.y;
        }
        surface->slope = upper_value;
    }
    surface->flags = flags & ~W8_GD_SURFACE_COLLISION_PROCESSED;
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
    if (geometry_index != 0) {
        delete geometry_index;
    }
    if (m_pVertices != 0) {
        delete[] m_pVertices;
    }
    if (m_pSurfaces != 0) {
        free(m_pSurfaces);
    }
    if (pending_trigger_bits != 0) {
        delete pending_trigger_bits;
    }
    if (active_trigger_bits != 0) {
        delete active_trigger_bits;
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
    SetOctreeGameData(0);
}

bool ShareSurfaceEdge(W8GDSurface* first, W8GDSurface* second, srVector3T<float>* vertices);
static void LinkSurfaceEdge(int polygon, int edge, W8HashTable<unsigned int, int>* table,
                            W8GDSurface* surfaces, unsigned int multiplier,
                            srVector3T<float>* vertices);

/* Welds duplicate vertices through a spatial hash, repacks the surface array
   collision-flag faces first, fills the shared build vertex/polygon arrays
   and stitches polygon edge links. */
static void InitializeCompiledSurface(W8GDSurface* compiled, const W8GDSurface* surface,
                                      int polygon_count)
{
    *compiled = *surface;
    compiled->edge_link[2] = -1;
    compiled->edge_link[1] = -1;
    compiled->edge_link[0] = -1;
    compiled->hit_plane = 0;
    W8OctRegionPolygon* polygon = g_gd_polygons + polygon_count;
    polygon->ordinal = polygon_count;
    polygon->plane = compiled->plane;
    polygon->degenerate = 0;
    polygon->visited = false;
    polygon->vertices[0] = g_gd_vertices + compiled->vertex_indices[0];
    polygon->vertices[1] = g_gd_vertices + compiled->vertex_indices[1];
    polygon->vertices[2] = g_gd_vertices + compiled->vertex_indices[2];
}

// FUNCTION: WIZ8 0x00449D10
void W8GameData::CompileGameData()
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

    ReportStartupMessage(g_string);
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
    srVector3T<float>* new_vertices = new srVector3T<float>[m_iNumVertices];
    if (new_vertices == 0) {
        ReportBuildStatus(7, "CompileGameData: Couldn't allocate New vertex list.");
    }

    if (m_pVertices != 0 && 0 < m_iNumVertices) {
        W8OctPreTreeVertex* vertex = weld_records;
        const srVector3T<float>* source = m_pVertices;
        srVector3T<float>* new_vertex = new_vertices;
        W8OctPreTreeVertex* gd_vertex = g_gd_vertices;
        for (i = 0; i < m_iNumVertices; ++i) {
            vertex->position = *source;
            unsigned int percent =
                static_cast<unsigned int>(i * g_octree_cell_scale / m_iNumVertices);
            if (progress + 10 < percent) {
                announce = true;
                progress += 10;
            }
            found = false;
            unsigned int key = static_cast<unsigned int>(
                vertex->position.z * g_float_one_five_hundredth * g_vertex_sort_z_weight +
                vertex->position.y * g_float_one_five_hundredth * g_vertex_sort_y_weight +
                vertex->position.x * g_float_one_five_hundredth * g_vertex_sort_x_weight);
            int linked = weld_table.Lookup(&key);
            if (linked == 0) {
                int vertex_id = i + 1;
                weld_table.Insert(&key, &vertex_id);
            } else {
                int last = 0;
                while (linked != 0) {
                    if (found) {
                        break;
                    }
                    int candidate_index = linked - 1;
                    W8OctPreTreeVertex* candidate = weld_records + candidate_index;
                    if (fabs(vertex->position.x - candidate->position.x) >= g_camera_snap_epsilon ||
                        fabs(vertex->position.y - candidate->position.y) >= g_camera_snap_epsilon ||
                        fabs(vertex->position.z - candidate->position.z) >= g_camera_snap_epsilon) {
                        last = candidate_index;
                        linked = candidate->m_kind;
                    } else {
                        vertex->m_vertex_index = candidate->m_vertex_index;
                        ++redundant;
                        found = true;
                        linked = candidate_index;
                    }
                }
                if (!found) {
                    weld_records[last].m_kind = weld_count + 1;
                }
            }
            if (!found) {
                *new_vertex = *source;
                vertex->m_vertex_index = weld_count;
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
            vertex->m_visited = false;
            ++vertex;
            ++source;
            announce = false;
        }
    }

    for (i = 0; i < m_iNumSurfaces; ++i) {
        W8GDSurface* surface = m_pSurfaces + i;
        for (j = 0; j < 3; ++j) {
            surface->vertex_indices[j] = weld_records[surface->vertex_indices[j]].m_vertex_index;
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
        if (surface->vertex_indices[0] != surface->vertex_indices[1] &&
            surface->vertex_indices[0] != surface->vertex_indices[2] &&
            surface->vertex_indices[1] != surface->vertex_indices[2] &&
            (surface->flags & W8_GD_SURFACE_WALKABLE) != 0) {
            surface->index = polygon_count;
            W8GDSurface* compiled = new_surfaces + polygon_count;
            InitializeCompiledSurface(compiled, surface, polygon_count);
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
        if (surface->vertex_indices[0] != surface->vertex_indices[1] &&
            surface->vertex_indices[0] != surface->vertex_indices[2] &&
            surface->vertex_indices[1] != surface->vertex_indices[2] &&
            (surface->flags & W8_GD_SURFACE_WALKABLE) == 0) {
            surface->index = polygon_count;
            surface->slope = 0;
            W8GDSurface* compiled = new_surfaces + polygon_count;
            InitializeCompiledSurface(compiled, surface, polygon_count);
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
    trigger_surface_count = polygon_count - m_iNumSurfaces;
    trigger_surface_base = m_iNumSurfaces;
    m_iNumSurfaces = polygon_count;
    free(weld_records);
    delete[] m_pVertices;
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
   matching corner slot on each surface is linked to the other's index. */
// FUNCTION: WIZ8 0x0044A970
bool ShareSurfaceEdge(W8GDSurface* first, W8GDSurface* second, srVector3T<float>* vertices)
{
    int first_slot = -1;
    int second_slot = -1;
    int last_first = -1;
    int last_second = -1;
    int* first_index = first->vertex_indices;
    for (int i = 0; i < 3; ++i) {
        int* second_index = second->vertex_indices;
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
        return false;
    }
    if ((last_first < first_slot && last_first - first_slot < 2) ||
        (first_slot < last_first && 1 < last_first - first_slot)) {
        first_slot = last_first;
    }
    if ((last_second < second_slot && last_second - second_slot < 2) ||
        (second_slot < last_second && 1 < last_second - second_slot)) {
        second_slot = last_second;
    }
    first->edge_link[first_slot] = second->index;
    second->edge_link[second_slot] = first->index;
    return true;
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
    if (surface->vertex_indices[next] < surface->vertex_indices[edge]) {
        low = next;
        high = edge;
    }
    unsigned int key = surface->vertex_indices[low] * multiplier + surface->vertex_indices[high];
    int index = table->Lookup(&key);
    if (index != 0) {
        bool linked = false;
        do {
            if (linked) {
                return;
            }
            if (ShareSurfaceEdge(surfaces + index, surface, vertices)) {
                linked = true;
            } else {
                index = table->FindNextEntry(&key, index);
            }
        } while (index != 0);
        if (linked) {
            return;
        }
    }
    table->Insert(&key, &polygon);
}

// FUNCTION: WIZ8 0x0044aa40
unsigned char W8GameData::WriteGameData(int handle)
{
    W8ProcessedGameDataHeader header;
    int index;

    header.version = 1;
    header.minimum = minimum;
    header.maximum = maximum;
    header.vertex_count = m_iNumVertices;
    header.surface_count = m_iNumSurfaces;
    header.trigger_surface_base = trigger_surface_base;
    header.trigger_surface_count = trigger_surface_count;
    header.integrated_surface_count = integrated_surface_count;
    header.interface_count = m_iNumInterfaces;
    header.state_count = m_iNumStates;
    header.trigger_count = m_iNumTriggers;
    header.cond_poly_count = m_iNumCondPolys;
    header.environ_count = m_iNumEnvirons;
    memset(header.reserved, 0, sizeof(header.reserved));

    if (handle == 0) {
        ReportBuildStatus(7, "WriteGameData: File not open.\n");
        return 0;
    }
    if (FileWrite(handle, &header, sizeof(header), 0) == 0) {
        ReportBuildStatus(7, "WriteGameData: Couldn't write GameData info.\n");
        return 0;
    }
    if (FileWrite(handle, m_pVertices, m_iNumVertices * sizeof(srVector3T<float>), 0) == 0) {
        ReportBuildStatus(7, "WriteGameData: Couldn't write vertex info.\n");
        return 0;
    }
    if (FileWrite(handle, m_pSurfaces, m_iNumSurfaces * sizeof(W8GDSurface), 0) == 0) {
        ReportBuildStatus(7, "WriteGameData: Couldn't write Surface info.\n");
        return 0;
    }
    if (m_iNumInterfaces != 0 &&
        FileWrite(handle, m_pInterfaces, m_iNumInterfaces * sizeof(W8GDInterface), 0) == 0) {
        ReportBuildStatus(7, "WriteGameData: Couldn't write switch interface info.\n");
        return 0;
    }
    if (m_iNumStates != 0 &&
        FileWrite(handle, m_pStates, m_iNumStates * sizeof(W8GDInterfaceState), 0) == 0) {
        ReportBuildStatus(7, "WriteGameData: Couldn't write switch state info.\n");
        return 0;
    }
    if (m_iNumCondPolys != 0 && FileWrite(handle, m_piCondPolys, m_iNumCondPolys * 4, 0) == 0) {
        ReportBuildStatus(7, "WriteGameData: Couldn't write conditional poly list.\n");
        return 0;
    }
    if (m_iNumEnvirons != 0) {
        for (index = 0; index < m_iNumEnvirons; ++index) {
            if (FileWrite(handle, m_ppEnvirons[index], sizeof(W8EnvironRecord), 0) == 0) {
                ReportBuildStatus(7, "WriteGameData: Couldn't write GD_Environ.\n");
                return 0;
            }
        }
    }
    return 1;
}
