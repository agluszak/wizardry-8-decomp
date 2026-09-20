#include "surrender/srCore.h"
#include "surrender/srGERD.h"
#include "surrender/srMaterial.h"
#include "surrender/srMeshModel.h"
#include "surrender/srModelInstance.h"
#include "surrender/srStatisticsManager.h"
#include "surrender/srVertexPipe.h"
#include "wiz8/engine_code/materials.h"
#include "wiz8/engine_code/3d.h"
#include "wiz8/engine_code/BitArray.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/GDFileIO.h"
#include "wiz8/engine_code/LevelFile.h"
#include "wiz8/engine_code/OctBuildPreTree.h"
#include "wiz8/engine_code/OctPreTree.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/ReadLevel.h"
#include "wiz8/engine_code/ReadMesh.h"
#include "wiz8/engine_code/stHash.hpp"
#include "wiz8/engine_code/stTextureAnim.h"
#include "wiz8/engine_code/stTextureFile.h"
#include "wiz8/float_constants.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/environment_colour.h"
#include "wiz8/fonts.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/PleaseWaitScreen.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/virtual_file.h"

#include "DEBUG.H"
#include "FileMan.h"
#include "Font.h"
#include "input.h"
#include "sgp.h"
#include "wiz8/local_code/Gameloop.h"

#include <cstdio>
#include <cstring>
#include <cmath>
#include <io.h>
#include <new>

#define MATERIALS_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\materials.cpp"

/* The global at 0x0065BEA8 is a real zero-storage srVertexProcessor subclass:
   its non-template process body establishes the boundary independently of its
   constructor and vtable, and that body maps eye-space normals into the first
   texture-coordinate set. No source or export name survives, so the class
   keeps a descriptive name anchored at its constructor address. */
// VTABLE: WIZ8 0x005ED0D0
// class W8NormalTexcoordMapper004B89A0
class W8NormalTexcoordMapper004B89A0 : public srVertexProcessor {
public:
    W8NormalTexcoordMapper004B89A0();
    virtual ~W8NormalTexcoordMapper004B89A0() override {}
    // FUNCTION: WIZ8 0x004D6190
    virtual int isActive(srVertexPipe&) override
    {
        return 1;
    }
    virtual void process(srVertexPipe& pipe) override;
};

static_assert(sizeof(W8NormalTexcoordMapper004B89A0) == 4,
              "W8NormalTexcoordMapper004B89A0_must_be_4");

// GLOBAL: WIZ8 0x0065BEA8
W8NormalTexcoordMapper004B89A0 g_normal_texcoord_mapper_0065bea8;

// GLOBAL: WIZ8 0x0065BA9E
bool g_material_diffuse_scale_enabled_0065ba9e;
// GLOBAL: WIZ8 0x0065BAA0
float g_material_diffuse_scale_0065baa0;
// GLOBAL: WIZ8 0x0065BAA4
bool g_material_emissive_override_enabled_0065baa4;
// GLOBAL: WIZ8 0x0065BAA8
float g_material_emissive_override_0065baa8;

// FUNCTION: WIZ8 0x004B89A0
W8NormalTexcoordMapper004B89A0::W8NormalTexcoordMapper004B89A0() {}

/* Convert eye-space normals to the material's first texture-coordinate set.
   The exported srVertexPipe queries preserve the closed renderer's ownership
   of its internal workspace while expressing every operation in this body. */
// FUNCTION: WIZ8 0x004B89B0
void W8NormalTexcoordMapper004B89A0::process(srVertexPipe& pipe)
{
    const srVector3T<float>* normals;
    srVector2T<float>* coordinates;
    unsigned long count;
    unsigned long index;

    if (!pipe.isChannelAvailable(srVertexProcessor::CHANNEL_ST0)) {
        return;
    }
    normals = pipe.getEyeSpaceNormal();
    coordinates = pipe.getST(0, 0);
    count = pipe.getVertexCount();
    srCore.getStatisticsManager()->statistics_00.texture_coordinate_operations_34 += count;
    for (index = 0; index < count; ++index) {
        coordinates[index].x = (normals[index].x + g_float_005ebb38) * g_float_005ebc7c;
        coordinates[index].y = (normals[index].y + g_float_005ebb38) * g_float_005ebc7c;
    }
}

// SYNTHETIC: WIZ8 0x004B8A50
// W8NormalTexcoordMapper004B89A0::`scalar deleting destructor'

// FUNCTION: WIZ8 0x004925B0
stMaterial::stMaterial()
{
    m_field_78 = 0;
}

// TEMPLATE: WIZ8 0x00492940
// srClassSupport<stMaterial,srMaterial,0,65538>::getClassID

// TEMPLATE: WIZ8 0x00492950
// srClassSupport<stMaterial,srMaterial,0,65538>::getClassName

// TEMPLATE: WIZ8 0x00492960
// srClassSupport<stMaterial,srMaterial,0,65538>::getClassNode

// FUNCTION: WIZ8 0x00492D00
srClass* stMaterial::vInstance()
{
    return new stMaterial;
}

// FUNCTION: WIZ8 0x00492A00
srClass* stMaterial::clone()
{
    stMaterial* instance = static_cast<stMaterial*>(vInstance());

    if (this != instance) {
        *static_cast<srMaterial*>(instance) = *this;
        instance->m_field_78 = m_field_78;
    }
    return instance;
}

// FUNCTION: WIZ8 0x004928F0
void stMaterial::getMaterialInfo(srVertexProcessor::MaterialInfo& info)
{
    srMaterial::getMaterialInfo(info);
    if (g_material_diffuse_scale_enabled_0065ba9e) {
        info.diffuse.w *= g_material_diffuse_scale_0065baa0;
    }
    if (g_material_emissive_override_enabled_0065baa4) {
        info.emissive.Set(g_material_emissive_override_0065baa8,
                          g_material_emissive_override_0065baa8,
                          g_material_emissive_override_0065baa8, 1.0f);
    }
}

// FUNCTION: WIZ8 0x00492720
stMaterial::~stMaterial()
{
    if (IsReadMeshMaterial00489AC0(this) != 0) {
        ReleaseReadMeshScratch004881D0();
    }
}

// SYNTHETIC: WIZ8 0x004926F0
// stMaterial::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00492A30
// srClassSupport<stMaterial,srMaterial,0,65538>::~srClassSupport<stMaterial,srMaterial,0,65538>

// SYNTHETIC: WIZ8 0x00492C40
// srClassSupport<stMaterial,srMaterial,0,65538>::`scalar deleting destructor'

/* ===== OctBuild level preprocessing =====
   The retail level preprocessor lives in this TU between the stMaterial
   cluster and LoadMaterial. Its option globals are plain .data and its
   build state is TU-private .bss. */

// GLOBAL: WIZ8 0x0060AC70
unsigned char g_option_pathing_0060ac70 = 1;
// GLOBAL: WIZ8 0x0060AC71
unsigned char g_option_shadow_test_0060ac71 = 1;
// GLOBAL: WIZ8 0x0060AC72
unsigned char g_option_logging_0060ac72 = 1;
// GLOBAL: WIZ8 0x0060AC73
unsigned char g_option_mesh_linking_0060ac73 = 1;
// GLOBAL: WIZ8 0x0060AC74
float g_option_path_node_spacing_0060ac74 = 500.0f;
// GLOBAL: WIZ8 0x0060AC78
float g_option_path_head_room_0060ac78 = 1000.0f;
// GLOBAL: WIZ8 0x0060AC7C
int g_option_delete_percentage_0060ac7c = 10;
// GLOBAL: WIZ8 0x0060AC80
float g_option_min_leaf_size_0060ac80 = 1000.0f;
// GLOBAL: WIZ8 0x0060AC84
int g_option_max_path_nodes_0060ac84 = 64;
// GLOBAL: WIZ8 0x0060AC88
int g_option_max_leaf_count_0060ac88 = 20000;
// GLOBAL: WIZ8 0x0060AC8C
unsigned char g_status_scroll_0060ac8c = 1;
// GLOBAL: WIZ8 0x0060AC8D
unsigned char g_status_buffers_freed_0060ac8d = 1;

// GLOBAL: WIZ8 0x0065BAB0
srVector3T<float> g_weld_min_0065bab0;
// GLOBAL: WIZ8 0x0065BACC
srVector3T<float> g_weld_max_0065bacc;
// GLOBAL: WIZ8 0x0065BADC
char g_log_path_0065badc[0x200];
// GLOBAL: WIZ8 0x0065BCDC
unsigned int g_weld_stride_x_0065bcdc;
// GLOBAL: WIZ8 0x0065BCE0
unsigned int g_weld_stride_y_0065bce0;
// GLOBAL: WIZ8 0x0065BCE4
unsigned int g_weld_stride_z_0065bce4;
// GLOBAL: WIZ8 0x0065BCE8
unsigned short* g_status_lines_0065bce8[6];
// GLOBAL: WIZ8 0x0065BD0C
int g_oct_node_count_0065bd0c;
// GLOBAL: WIZ8 0x0065BD10
int g_oct_leaf_count_0065bd10;
// GLOBAL: WIZ8 0x0065BD14
int g_lights_unblocked_0065bd14;
// GLOBAL: WIZ8 0x0065BD18
int g_light_candidates_0065bd18;
// GLOBAL: WIZ8 0x0065BD1C
int g_lights_facing_0065bd1c;
// GLOBAL: WIZ8 0x0065BD2D
unsigned char g_option_rename_alphas_0065bd2d;
// GLOBAL: WIZ8 0x0065BD30
float g_option_auto_region_size_0065bd30;
// GLOBAL: WIZ8 0x0065BD34
W8OctPreTreeVertex* g_gd_vertices_0065bd34;
// GLOBAL: WIZ8 0x0065BD38
W8OctRegionPolygon* g_gd_polygons_0065bd38;
// GLOBAL: WIZ8 0x0065BD3C
BitArray* g_prop_sun_bits_0065bd3c;
// GLOBAL: WIZ8 0x0065BD44
short g_status_cursor_0065bd44;
// GLOBAL: WIZ8 0x0065BD48
int g_progress_total_0065bd48;
// GLOBAL: WIZ8 0x0065BD4C
int g_progress_done_0065bd4c;
// GLOBAL: WIZ8 0x0065BD50
int g_progress_mark_0065bd50;
// GLOBAL: WIZ8 0x0065BD54
FILE* g_log_file_0065bd54;

// GLOBAL: WIZ8 0x005ECBB0
const float g_float_005ecbb0 = 268435456.0f;
// GLOBAL: WIZ8 0x005ECBB8
const float g_float_005ecbb8 = 2.5f;
// GLOBAL: WIZ8 0x005ECBBC
const float g_float_005ecbbc = -0.995f;

unsigned char PreprocessLevel00493120(int handle, char* stem);
int WeldVertex00494800(W8HashTable<unsigned int, int>* table, W8OctPreTreeVertex* vertices,
                       unsigned int index, unsigned int link);
int AccumulateVertexLight00495CF0(OctPreTree* tree, W8OctPreTreeVertex* vertex, short light_count,
                                  W8LevelFileLight* lights, int* sun_map);
int PropReceivesLight00495E90(OctPreTree* tree, W8LevelFileProp* prop, W8LevelFileLight* light);
int BuildRegionPolygons00494B90(W8LevelFile* level, W8OctPreTreeGeometry* geometry,
                                unsigned char* classify);
int SplitVerticesByMaterial00495860(W8OctPreTreeGeometry* geometry);
unsigned char* ClassifyTextures00496000(W8MaterialRecord004B8A70* textures, int count, char* stem);
int MaterialSort00496500(W8OctPreTreeGeometry* geometry, W8MaterialRecord004B8A70* textures,
                         int count, unsigned char* classify);
void OctBuildOptions00496CD0(char* stem);
void GetWorldColour00427290(EnvironmentColour* colour);

// FUNCTION: WIZ8 0x00492E60
char BuildPreprocessedFiles00492E60(const char* level_path)
{
    char level_name[1024];
    char stem[1024];
    char line[1024];
    char* extension;
    unsigned char result;
    unsigned int handle;
    int move_pvl;
    int move_oct;

    strcpy(level_name, level_path);
    sprintf(stem, "OctBuild Version %d\n\n", 0x22);
    ReportStartupMessage004969D0(stem);
    extension = strrchr(level_name, '.');
    if (extension != 0) {
        *extension = '\0';
    }
    OctBuildOptions00496CD0(level_name);
    strcpy(stem, level_name);
    strcat(level_name, ".lvl");
    sprintf(line, "Processing level %s.\n", level_name);
    ReportBuildStatus00497690(6, line);
    sprintf(line, "%s.log", stem);
    ReportBuildStatus00497690(0, line);
    ReportBuildStatus00497690(6, "Now chewing level.\n\n");
    handle = FileOpen(level_name, FILE_ACCESS_READ, 0);
    result = 0;
    if (handle == 0) {
        ReportBuildStatus00497690(7, "Could not find and\\or open level file.\n");
    } else {
        unsigned char built = PreprocessLevel00493120(handle, stem);
        FileClose(handle);
        if (built != 0) {
            sprintf(line, "%s.pvl", stem);
            if (FileExists(line) != 0) {
                if (_access(line, 2) != 0) {
                    _chmod(line, 0x180);
                }
                DeleteFileA(line);
            }
            move_pvl = MoveFileA("NewLevel.lvl", line);
            sprintf(line, "%s.rlk", stem);
            if (FileExists(line) != 0) {
                ReportStartupMessage004969D0(
                    "LINK FILE (.RLK) FOUND! IF THE GEOMETRY HAS CHANGED, THE LINK FILE MAY BE "
                    "OBSOLETE!!!\n");
            }
            sprintf(line, "%s.oct", stem);
            if (FileExists(line) != 0) {
                if (_access(line, 2) != 0) {
                    _chmod(line, 0x180);
                }
                DeleteFileA(line);
            }
            move_oct = MoveFileA("NewLevel.oct", line);
            result =
                built & static_cast<unsigned char>(move_pvl) & static_cast<unsigned char>(move_oct);
            if (result == 0) {
                ReportBuildStatus00497690(
                    6, "File copying failed!  OCT or PVL files may be write protected.");
            } else {
                ReportStartupMessage004969D0("PREPROCESSING SUCCESSFUL -- NOW LOADING LEVEL.");
            }
        }
    }
    ReportBuildStatus00497690(8, 0);
    ReportStartupMessage004969D0(0);
    return result;
}

/* The level-preprocessing driver: reads the .lvl, welds and re-scales the
   mesh vertices, reads the .wgd game data, builds the OctBuildPreTree,
   computes vertex lighting and sun visibility, sorts materials, splits
   vertices, writes NewLevel.lvl/.oct and releases everything it made. */
// FUNCTION: WIZ8 0x00493120
unsigned char PreprocessLevel00493120(int handle, char* stem)
{
    char message[1024];
    char name[20];
    srVector3T<float> minimum;
    srVector3T<float> maximum;
    srVector3T<float> bound_min;
    srVector3T<float> bound_max;
    W8OctPreTreeGeometry geometry;
    W8LevelFileLight* lights;
    int* sun_map;
    float* sun_pool;
    unsigned char* classify;
    W8OctPreTreeVertex* vertices;
    W8LevelFile* level;
    W8GameData* value;
    OctBuildPreTree* build_tree;
    OctPreTree* tree;
    OctMeshModel* submeshes;
    BitArray* sun_bits;
    unsigned char result;
    unsigned char report;
    int redundant;
    int lit_vertices;
    int last_sun;
    int sun_count;
    int percent;
    int mark;
    int i;
    int j;
    int light;
    int lit;
    unsigned int file;
    W8LevelFileMesh* mesh;
    W8LevelFileLight* src_light;

    result = 0;
    submeshes = 0;
    sun_map = 0;
    lights = 0;
    {
        W8HashTable<unsigned int, int> weld;
        weld.Clear();
        sun_pool = 0;
        level = ReadLevelFile004CFDC0(handle);
        if (level == 0) {
            ReportBuildStatus00497690(7, "Could not process LVL file.\n");
            return 0;
        }
        memset(&geometry, 0, sizeof(geometry));
        mesh = level->pMeshes;
        if (mesh->num_vertices_04 < 1 || mesh->num_faces_08 < 1) {
            return 0;
        }
        mesh->num_vertices_04 = mesh->num_vertices_04 + 1;
        classify = ClassifyTextures00496000(level->pTextures, level->nTextures, stem);
        vertices = static_cast<W8OctPreTreeVertex*>(malloc(mesh->num_vertices_04 * 0xc0));
        if (vertices != 0) {
            memset(vertices, 0, mesh->num_vertices_04 * 0xc0);
            minimum.x = 1e7f;
            minimum.y = 1e7f;
            minimum.z = 1e7f;
            maximum.x = -1e7f;
            maximum.y = -1e7f;
            maximum.z = -1e7f;
            redundant = 0;
            for (i = 1; i < mesh->num_vertices_04; ++i) {
                const float* source = mesh->pstVertices + (i - 1) * 3;
                vertices[i].position_0c.x = source[0] * g_world_scale_005ebc40;
                vertices[i].position_0c.y = source[1] * g_world_scale_005ebc40;
                vertices[i].position_0c.z = source[2] * g_world_scale_005ebc40;
                vertices[i].original_position_54.x = source[0];
                vertices[i].original_position_54.y = source[1];
                vertices[i].original_position_54.z = source[2];
                vertices[i].flag_0a = 0;
                for (j = 0; j < 3; ++j) {
                    float v = (&vertices[i].position_0c.x)[j];
                    if (v <= (&maximum.x)[j]) {
                        if (v < (&minimum.x)[j]) {
                            (&minimum.x)[j] = v;
                            (&g_weld_min_0065bab0.x)[j] = v;
                        }
                    } else {
                        (&maximum.x)[j] = v;
                        (&g_weld_max_0065bacc.x)[j] = v;
                    }
                }
            }
            geometry.vertices_04 = vertices;
            g_weld_stride_z_0065bce4 = static_cast<unsigned int>(
                (g_float_005ecbb0 /
                 ((g_weld_max_0065bacc.x - g_weld_min_0065bab0.x) * g_float_005ecbb4)));
            g_weld_stride_y_0065bce0 =
                static_cast<unsigned int>(sqrt(static_cast<double>(g_weld_stride_z_0065bce4)));
            g_weld_stride_x_0065bcdc =
                static_cast<unsigned int>(sqrt(static_cast<double>(g_weld_stride_y_0065bce0)));
            ReportStartupMessage004969D0("Welding vertices and discarding redundant vertices.\n");
            mark = 0;
            report = 1;
            if (1 < mesh->num_vertices_04) {
                i = 1;
                do {
                    percent =
                        static_cast<unsigned int>((i * g_octree_cell_scale_005ebcd0 /
                                                   static_cast<float>(mesh->num_vertices_04)));
                    if (mark + 10 < percent) {
                        report = 1;
                        mark = mark + 10;
                    }
                    lit = WeldVertex00494800(&weld, vertices, i, 0xffffffff);
                    i = i + 1;
                    if (lit != i) {
                        redundant = redundant + 1;
                    }
                    if (report != 0) {
                        sprintf(message, "  %d%% Complete:  %d Redundant Vertices  \r", mark,
                                redundant);
                        ReportStartupMessage004969D0(message);
                    }
                    report = 0;
                    vertices[i - 1].flag_0a = 0;
                } while (i < mesh->num_vertices_04);
            }
            sprintf(message, "\n\nNumber of Verticies: %d   Number of polygons: %d\n",
                    mesh->num_vertices_04, mesh->num_faces_08);
            ReportBuildStatus00497690(6, message);
            sprintf(message, "Number of Redundant Verticies: %d\n", redundant);
            ReportBuildStatus00497690(6, message);
            ReportBuildStatus00497690(6, "\nReading GameData...\n");
            sprintf(message, "%s.wgd", stem);
            value = ReadGameData00447570(message, true);
            if (value == 0) {
                ReportBuildStatus00497690(7, "\n Error -- Cannot load or find game data.\n");
            } else {
                for (i = 0; i < 3; ++i) {
                    if ((&value->minimum_08.x)[i] < (&minimum.x)[i]) {
                        (&minimum.x)[i] = (&value->minimum_08.x)[i];
                    }
                    if ((&maximum.x)[i] < (&value->maximum_14.x)[i]) {
                        (&maximum.x)[i] = (&value->maximum_14.x)[i];
                    }
                }
                ReportBuildStatus00497690(6, "\nBuilding OctBuildPreTree ---------------------\n");
                build_tree = new OctBuildPreTree(g_option_min_leaf_size_0060ac80, &minimum,
                                                 &maximum, g_option_max_path_nodes_0060ac84,
                                                 g_option_max_leaf_count_0060ac88, 0);
                if (build_tree != 0) {
                    build_tree->LoadRegionFile004B0C90(stem, &minimum, &maximum);
                    if (g_option_mesh_linking_0060ac73 == 0) {
                        build_tree->active_f4 = 0;
                    }
                    build_tree->spatial_00.region_grid_cell_54 = g_option_auto_region_size_0065bd30;
                    int alpha_polys = BuildRegionPolygons00494B90(level, &geometry, classify);
                    build_tree->SortGeometry004AFEA0(&geometry);
                    short light_total = level->nLights;
                    last_sun = -1;
                    if (light_total != 0) {
                        lights = static_cast<W8LevelFileLight*>(malloc(light_total * 0x44));
                        memcpy(lights, level->pLights, light_total * 0x44);
                        sun_map = static_cast<int*>(malloc(light_total * 4));
                        memset(sun_map, 0, light_total * 4);
                        sun_count = 1;
                        for (i = 0; i < static_cast<int>(light_total); ++i) {
                            src_light = lights + i;
                            src_light->position_08.x =
                                src_light->position_08.x * g_world_scale_005ebc40;
                            src_light->position_08.y =
                                src_light->position_08.y * g_world_scale_005ebc40;
                            src_light->position_08.z =
                                src_light->position_08.z * g_world_scale_005ebc40;
                            src_light->colour_14.x =
                                src_light->colour_14.x * g_world_scale_005ebc40;
                            strcpy(name, src_light->name_28);
                            name[19] = 0;
                            TrimAndLowercaseString(name);
                            if (strstr(name, "sun") != 0 || strstr(name, "moon") != 0 ||
                                strstr(name, "lightning") != 0) {
                                sun_map[i] = sun_count;
                                sun_count = sun_count + 1;
                                last_sun = i;
                            }
                        }
                        sun_count = sun_count - 1;
                        if (sun_count != 0) {
                            sun_pool = static_cast<float*>(
                                malloc(geometry.vertex_count_00 * sun_count * 4));
                            if (sun_pool == 0) {
                                ReportBuildStatus00497690(7, "Could not allocate pflSunLights!\n");
                            }
                            memset(sun_pool, 0, geometry.vertex_count_00 * sun_count * 4);
                            float* run = sun_pool;
                            for (i = 0; i < static_cast<int>(geometry.vertex_count_00); ++i) {
                                vertices[i].sun_lights_3c = run;
                                run = run + sun_count;
                            }
                        }
                    } else {
                        sun_count = last_sun;
                    }
                    ReportBuildStatus00497690(6,
                                              "\nOctree Statistics:\n==========================\n");
                    sprintf(message, "Width of Level: %f\t\tTree Depth: %d\n",
                            build_tree->spatial_00.extent_04, build_tree->spatial_00.depth_44);
                    ReportBuildStatus00497690(6, message);
                    sprintf(message, "Width of Leaves: %f,  %f metres\n",
                            build_tree->spatial_00.node_extent_70,
                            (build_tree->spatial_00.node_extent_70 * g_float_005ebc60));
                    ReportBuildStatus00497690(6, message);
                    sprintf(message, "Width of auto-generated regions: %f metres\n",
                            (build_tree->spatial_00.region_grid_cell_54 * g_float_005ebc60));
                    g_oct_node_count_0065bd0c = GetValue65BE60();
                    g_oct_leaf_count_0065bd10 = build_tree->positional_b8;
                    ReportBuildStatus00497690(3, message);
                    sprintf(message, "World Minimum Corner: \t%f  \t%f  \t%f\n",
                            build_tree->spatial_00.minimum_0c.x,
                            build_tree->spatial_00.minimum_0c.y,
                            build_tree->spatial_00.minimum_0c.z);
                    ReportBuildStatus00497690(6, message);
                    sprintf(message, "World Maximum Corner: \t%f  \t%f  \t%f\n",
                            build_tree->spatial_00.maximum_18.x,
                            build_tree->spatial_00.maximum_18.y,
                            build_tree->spatial_00.maximum_18.z);
                    ReportBuildStatus00497690(6, message);
                    sprintf(message, "World Dimensions:\n\tX: %fm  \tY: %fm  \tZ: %fm\n",
                            ((build_tree->spatial_00.clipped_maximum_30.x -
                              build_tree->spatial_00.clipped_minimum_24.x) *
                             g_float_005ebc60),
                            ((build_tree->spatial_00.clipped_maximum_30.y -
                              build_tree->spatial_00.clipped_minimum_24.y) *
                             g_float_005ebc60),
                            ((build_tree->spatial_00.clipped_maximum_30.z -
                              build_tree->spatial_00.clipped_minimum_24.z) *
                             g_float_005ebc60));
                    ReportBuildStatus00497690(6, message);
                    for (i = 0; i < level->num_switch_triggers_6c1; ++i) {
                        int slot = value->FindPointerByName004482A0(
                            level->switch_triggers_6c5[i]->switch_name_223 + 1);
                        level->switch_triggers_6c5[i]->switch_name_223[0] = '\0';
                        sprintf(level->switch_triggers_6c5[i]->switch_name_223 + 1, "%d", slot);
                    }
                    for (i = 0; i < level->num_invisible_planes_1665; ++i) {
                        value->AddLevelPlane004485F0(level->invisible_planes_1669[i]);
                    }
                    for (i = 0; i < level->num_linked_records_2609; ++i) {
                        W8LevelFileLinkedRecord* record = level->linked_records_260d[i];
                        value->AddLinkedRecord00448BF0(record->vertices_01, record->value_1b3,
                                                       record->value_1b7, &record->linked_face_1b1);
                    }
                    value->geometry_index_00 = build_tree;
                    value->CompileGameData00449D10();
                    for (i = 0; i < value->m_iNumSurfaces; ++i) {
                        W8OctRegionPolygon* surface = g_gd_polygons_0065bd38 + i;
                        if (build_tree->InsertSurface004B02F0(surface, 3) == 0) {
                            sprintf(message,
                                    "Warning: GD Polygon %d cannot be inserted into tree\n", i);
                            ReportBuildStatus00497690(6, message);
                            sprintf(message, "\tVertex 1: \t%f  \t%f  \t%f\n",
                                    surface->vertices_34[0]->position_0c.x,
                                    surface->vertices_34[0]->position_0c.y,
                                    surface->vertices_34[0]->position_0c.z);
                            ReportBuildStatus00497690(6, message);
                            sprintf(message, "\tVertex 2: \t%f  \t%f  \t%f\n",
                                    surface->vertices_34[1]->position_0c.x,
                                    surface->vertices_34[1]->position_0c.y,
                                    surface->vertices_34[1]->position_0c.z);
                            ReportBuildStatus00497690(6, message);
                            sprintf(message, "\tVertex 3: \t%f  \t%f  \t%f\n\n",
                                    surface->vertices_34[2]->position_0c.x,
                                    surface->vertices_34[2]->position_0c.y,
                                    surface->vertices_34[2]->position_0c.z);
                            ReportBuildStatus00497690(6, message);
                        }
                    }
                    build_tree->RemapNodeRegions004B16B0(0, 0);
                    ReportBuildStatus00497690(6,
                                              "\nInserting props and particles into regions... \n");
                    build_tree->BuildParticleRegions004B3820(level->pParticleSystems,
                                                             level->nParticleSystems);
                    build_tree->BuildGeometryRegions004B3F90(level->pProps, level->nProps, 0, 0);
                    build_tree->BuildGeometryRegions004B3F90(level->pBitmaps, level->nBitmaps,
                                                             level->nProps, 1);
                    build_tree->spatial_00.root_90->RearrangeNodePolys004AF7B0(
                        0, build_tree->spatial_00.depth_44);
                    for (i = 0; i < static_cast<int>(geometry.vertex_count_00); ++i) {
                        vertices[i].flag_0a = 0;
                    }
                    for (i = 0; i < static_cast<int>(geometry.polygon_count_08); ++i) {
                        geometry.polygons_0c[i].positional_31 = 0;
                    }
                    ReportBuildStatus00497690(
                        6, "\nCompiling OctPreTree --------------------------\n");
                    tree = build_tree->BuildOctPreTree004B4640();
                    if (tree == 0) {
                        ReportBuildStatus00497690(7, "Could not create OctPreTree.\n");
                        return 0;
                    }
                    tree->SetPathStem(stem);
                    tree->spatial_000.SetWorkingBounds00467B70(&g_weld_min_0065bab0,
                                                               &g_weld_max_0065bacc);
                    tree->m_positional_1b0 = alpha_polys;
                    value->positional_04 = tree;
                    sprintf(message, "Poly List Len: %d\n",
                            static_cast<int>(tree->polygon_cursor_3a0));
                    ReportBuildStatus00497690(6, message);
                    tree->spatial_000.polygon_count_3c = geometry.polygon_count_08;
                    SetOctreeGameData0046D7D0(value);
                    if (light_total != 0) {
                        tree->m_sun_count_296 = static_cast<unsigned short>(sun_count);
                        ReportBuildStatus00497690(
                            6, "\nCalculating Vertex Lighting  --------------\n");
                        mark = 0;
                        lit_vertices = 0;
                        for (i = 1; i < static_cast<int>(geometry.vertex_count_00); ++i) {
                            percent = static_cast<unsigned int>(
                                (i * g_octree_cell_scale_005ebcd0 /
                                 static_cast<float>(geometry.vertex_count_00)));
                            if (mark + 10 < percent) {
                                mark = mark + 10;
                                sprintf(message, "  %d%% Complete:  %d Vertices Lit \r", mark, i);
                                ReportStartupMessage004969D0(message);
                            }
                            if (AccumulateVertexLight00495CF0(tree, vertices + i, light_total,
                                                              lights, sun_map) != 0) {
                                lit_vertices = lit_vertices + 1;
                            }
                        }
                        short live_lights = 0;
                        if (0 < light_total) {
                            src_light = lights;
                            for (i = light_total; i != 0; --i) {
                                if (src_light->version_00 < 2 ||
                                    ((src_light->flags_02 & 0xff) == 0 &&
                                     (src_light->flags_02 & 0xff00) != 0)) {
                                    live_lights = live_lights + 1;
                                }
                                ++src_light;
                            }
                        }
                        int combinations = live_lights * i;
                        sprintf(message, "\n\n%d vertices, %d lights\n",
                                static_cast<int>(geometry.vertex_count_00), light_total);
                        ReportBuildStatus00497690(6, message);
                        sprintf(message, "%d possible light/vertex combinations \n", combinations);
                        ReportBuildStatus00497690(6, message);
                        if (combinations != 0) {
                            sprintf(message,
                                    "%d (%d percent) combinations within lighting "
                                    "distance\n",
                                    g_light_candidates_0065bd18,
                                    g_light_candidates_0065bd18 * 100 / combinations);
                            ReportBuildStatus00497690(6, message);
                        }
                        if (g_light_candidates_0065bd18 != 0) {
                            sprintf(message, "%d (%d percent) of these face light\n",
                                    g_lights_facing_0065bd1c,
                                    g_lights_facing_0065bd1c * 100 / g_light_candidates_0065bd18);
                            ReportBuildStatus00497690(6, message);
                        }
                        if (g_lights_facing_0065bd1c != 0) {
                            sprintf(message,
                                    "%d (%d percent) of these were blocked by vertex "
                                    "shadowing\n",
                                    g_lights_facing_0065bd1c - g_lights_unblocked_0065bd14,
                                    100 - g_lights_unblocked_0065bd14 * 100 /
                                              g_lights_facing_0065bd1c);
                            ReportBuildStatus00497690(6, message);
                            sprintf(message,
                                    "%d (%d percent) submitted were not blocked by "
                                    "shadowing\n",
                                    g_lights_unblocked_0065bd14,
                                    g_lights_unblocked_0065bd14 * 100 / g_lights_facing_0065bd1c);
                            ReportBuildStatus00497690(6, message);
                        }
                        if (geometry.vertex_count_00 != 0) {
                            sprintf(message,
                                    "%d (%d percent) of vertices actually receive "
                                    "light\n",
                                    lit_vertices,
                                    lit_vertices * 100 /
                                        static_cast<int>(geometry.vertex_count_00));
                            ReportBuildStatus00497690(6, message);
                        }
                        sun_bits = new BitArray(level->nBitmaps + level->nProps);
                        if (level->nProps != 0 && last_sun >= 0) {
                            int* read_entry = sun_map;
                            int* write_entry = sun_map;
                            for (i = light_total; i > 0; --i) {
                                if (*read_entry != 0) {
                                    *write_entry = *read_entry;
                                    ++write_entry;
                                    *read_entry = 0;
                                }
                                ++read_entry;
                            }
                            for (int* sun_entry = sun_map; sun_count-- > 0; ++sun_entry) {
                                for (i = 0; i < level->nProps; ++i) {
                                    if (PropReceivesLight00495E90(tree, level->pProps + i,
                                                                  lights + *sun_entry) != 0) {
                                        sun_bits->Set(i);
                                    }
                                }
                                for (i = 0; i < level->nBitmaps; ++i) {
                                    if (PropReceivesLight00495E90(tree, level->pBitmaps + i,
                                                                  lights + *sun_entry) != 0) {
                                        sun_bits->Set(level->nProps + i);
                                    }
                                }
                            }
                            sprintf(message, "%d of %d props receive sunlight\n",
                                    sun_bits->CountSetBits(),
                                    static_cast<char>((level->nProps << 1)));
                            ReportBuildStatus00497690(6, message);
                        }
                        tree->SetPropSunBits(sun_bits);
                    }
                    ReportBuildStatus00497690(6, "\nSorting Materials and Textures...\n");
                    result = MaterialSort00496500(&geometry, level->pTextures, level->nTextures,
                                                  classify);
                    if (result == 0) {
                        ReportBuildStatus00497690(7,
                                                  "Could not generate polygon list by regions.\n");
                    } else {
                        sprintf(message, "%d Textures, \t%d Materials\n",
                                static_cast<int>(geometry.texture_count_1c),
                                static_cast<int>(geometry.material_count_18));
                        ReportBuildStatus00497690(6, message);
                        result = SplitVerticesByMaterial00495860(&geometry);
                        if (result == 0) {
                            ReportBuildStatus00497690(
                                7, "Could not generate polygon list by regions.\n");
                        } else {
                            ReportBuildStatus00497690(
                                6, "\nCreating Submeshes ------------------------\n");
                            submeshes = tree->CreateSubMeshes00468C30(&geometry);
                            sprintf(message, "Number of Submeshes: %d\n",
                                    static_cast<int>(tree->GetMeshCount()));
                            ReportBuildStatus00497690(6, message);
                            sprintf(message,
                                    "   %d Normal,   %d Sutractive Alpha,   %d "
                                    "Additive Alpha\n",
                                    static_cast<int>(tree->m_root_mesh_count_1a8),
                                    static_cast<int>((tree->m_kind1_submesh_count_1ac -
                                                      tree->m_root_mesh_count_1a8)),
                                    static_cast<int>(
                                        (tree->GetMeshCount() - tree->m_kind1_submesh_count_1ac)));
                            ReportBuildStatus00497690(6, message);
                            if (g_option_pathing_0060ac70 != 0) {
                                tree->m_region_cell_178 = g_option_path_node_spacing_0060ac74;
                                /* Retail copies the head-room float's bits
                                   into the unsigned-long field. */
                                tree->m_path_clearance_17c =
                                    (unsigned long&)/* c-style-cast-ok: float-bit copy */
                                    g_option_path_head_room_0060ac78;
                                tree->BuildPathLists0046B060(value, level,
                                                             g_option_delete_percentage_0060ac7c);
                            }
                            if (submeshes == 0) {
                                if (tree->GetMeshCount() == 0) {
                                    ReportBuildStatus00497690(7,
                                                              "Could not create submesh data.\n");
                                    result = 0;
                                }
                            } else if (tree->GetMeshCount() == 0) {
                                ReportBuildStatus00497690(7, "Could not create submesh data.\n");
                                result = 0;
                            } else {
                                tree->spatial_000.GetWorkingBounds0046CDF0(&bound_min, &bound_max);
                                ReportBuildStatus00497690(6, "Graphic Data Bounding Box:\n");
                                sprintf(message, "     Minimum: %f   %f   %f\n", bound_min.x,
                                        bound_min.y, bound_min.z);
                                ReportBuildStatus00497690(6, message);
                                sprintf(message, "     Maximum: %f   %f   %f\n\n", bound_max.x,
                                        bound_max.y, bound_max.z);
                                ReportBuildStatus00497690(6, message);
                                ReportBuildStatus00497690(6, "\nWriting Oct File...\n");
                                result = tree->WriteOctFile004683F0(&geometry, value);
                            }
                        }
                    }
                    file = 0;
                    if (result != 0) {
                        int total_faces = 0;
                        int total_vertices = 0;
                        for (i = 0; i < static_cast<int>(tree->GetMeshCount()); ++i) {
                            const char* fmt;
                            if (submeshes[i].packed_header_3c == 0) {
                                fmt = "Submesh %d:  \t%d Faces,  \t%d Vertices  "
                                      "\t(Opaque)";
                            } else if (submeshes[i].packed_header_3c == 1) {
                                fmt = "Submesh %d:  \t%d Faces,  \t%d Vertices  "
                                      "\t(Subtractive Alpha)";
                            } else {
                                fmt = "Submesh %d:  \t%d Faces,  \t%d Vertices  "
                                      "\t(Additive Alpha)";
                            }
                            sprintf(message, fmt, i, submeshes[i].polygon_count_44,
                                    submeshes[i].vertex_count_40);
                            strcat(message, static_cast<int>(tree->spatial_000.region_count_46) <= i
                                                ? "\n"
                                                : " Regioned Manually\n");
                            ReportBuildStatus00497690(5, message);
                            total_vertices += submeshes[i].vertex_count_40;
                            total_faces += submeshes[i].polygon_count_44;
                        }
                        sprintf(message, "Totals:      \t%d Faces,  \t%d Vertices\n", total_faces,
                                total_vertices);
                        ReportBuildStatus00497690(6, message);
                        sprintf(message, "Total duplicated vertices: %d\n",
                                total_vertices - static_cast<int>(geometry.vertex_count_00));
                        ReportBuildStatus00497690(6, message);
                        file = FileOpen("NewLevel.lvl", 0x22, 0);
                        if (file == 0) {
                            ReportBuildStatus00497690(7, "Could not open new level file.\n");
                            result = 0;
                        } else {
                            level->field_00 = tree->GetMeshCount();
                            level->field_04 = tree->m_meshCount_1b4;
                            level->pModels_0c = submeshes;
                            ReportBuildStatus00497690(6, "\nWriting PVL File...\n");
                            result = result & WriteLevelFile004D07C0(file, handle, level);
                            FileClose(file);
                            if (result != 0) {
                                goto write_done;
                            }
                        }
                        ReportBuildStatus00497690(7, "Could not write new level file.\n");
                    }
                write_done:
                    if (file != 0) {
                        FileClose(file);
                    }
                    ReportStartupMessage004969D0("Cleaning up preprocessing data...");
                    geometry.Release004CFC10();
                    if (g_gd_polygons_0065bd38 != 0) {
                        free(g_gd_polygons_0065bd38);
                    }
                    if (g_gd_vertices_0065bd34 != 0) {
                        free(g_gd_vertices_0065bd34);
                    }
                    if (classify != 0) {
                        free(classify);
                    }
                    delete tree;
                    delete value;
                    delete g_prop_sun_bits_0065bd3c;
                    g_prop_sun_bits_0065bd3c = 0;
                    if (sun_pool != 0) {
                        free(sun_pool);
                    }
                    if (sun_map != 0) {
                        free(sun_map);
                    }
                    if (lights != 0) {
                        free(lights);
                    }
                    return result;
                }
                ReportBuildStatus00497690(7, "Could not create OctPreTree.\n");
            }
        }
        return 0;
    }
}

// FUNCTION: WIZ8 0x00497690
void ReportBuildStatus00497690(int channel, const char* message)
{
    char line[120];

    switch (channel) {
    case 0:
        if (g_log_file_0065bd54 == 0) {
            g_progress_total_0065bd48 = 0;
            g_progress_done_0065bd4c = 0;
            if (message == 0) {
                g_log_file_0065bd54 = fopen(g_log_path_0065badc, "w");
                return;
            }
            if (*message != '\0') {
                strcpy(g_log_path_0065badc, message);
                if (g_option_logging_0060ac72 != 0) {
                    g_log_file_0065bd54 = fopen(message, "w");
                    return;
                }
            }
        }
        break;
    case 1:
        g_progress_done_0065bd4c = g_progress_done_0065bd4c + 1;
        if (g_progress_mark_0065bd50 + 10 <
            static_cast<int>((g_progress_done_0065bd4c * 100.0f / g_progress_total_0065bd48))) {
            g_progress_mark_0065bd50 = g_progress_mark_0065bd50 + 10;
            sprintf(line, "  %d%% Complete \r", g_progress_mark_0065bd50);
            ReportStartupMessage004969D0(line);
            return;
        }
        break;
    case 2:
        g_progress_total_0065bd48 = g_progress_total_0065bd48 + 1;
        return;
    case 3:
        if (g_log_file_0065bd54 != 0) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
            fprintf(g_log_file_0065bd54, message);
#pragma clang diagnostic pop
            fprintf(g_log_file_0065bd54, "Number of Nodes: %d             Number of Leaves: %d\n",
                    g_oct_node_count_0065bd0c, g_progress_total_0065bd48);
            fprintf(g_log_file_0065bd54, "Most Objects in Any Node: %d\n\n",
                    g_oct_leaf_count_0065bd10);
            return;
        }
        break;
    case 4:
        ReportStartupMessage004969D0(message);
        return;
    case 5:
        if (g_log_file_0065bd54 != 0) {
            fprintf(g_log_file_0065bd54, "%s", message);
            return;
        }
        break;
    case 6:
        ReportStartupMessage004969D0(message);
        if (g_log_file_0065bd54 != 0) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
            fprintf(g_log_file_0065bd54, message);
#pragma clang diagnostic pop
            return;
        }
        break;
    case 7:
        if (g_log_file_0065bd54 != 0) {
            sprintf(line, "ERROR: %s", message);
            fprintf(g_log_file_0065bd54, "\n\n%s", message);
            fclose(g_log_file_0065bd54);
        }
        ShutdownWithErrorBox(message);
        return;
    case 8:
        if (g_log_file_0065bd54 != 0) {
            fclose(g_log_file_0065bd54);
            g_log_file_0065bd54 = 0;
        }
    }
}

// FUNCTION: WIZ8 0x004969D0
void ReportStartupMessage004969D0(const char* message)
{
    static EnvironmentColour s_black_0065bd00;
    static EnvironmentColour s_saved_colour_0065bac0;
    unsigned short* line;
    short length;
    unsigned char scroll;
    unsigned char scrolled;
    int index;
    int top;

    scrolled = 0;
    if (message == 0) {
        if (g_status_buffers_freed_0060ac8d == 0) {
            for (index = 0; index < 6; ++index) {
                delete g_status_lines_0065bce8[index];
            }
            g_status_buffers_freed_0060ac8d = 1;
        }
        PublishLightDirection(&s_saved_colour_0065bac0);
        return;
    }
    if (g_status_buffers_freed_0060ac8d != 0) {
        for (index = 0; index < 6; ++index) {
            unsigned short* buffer = new unsigned short[0x100];
            g_status_lines_0065bce8[index] = buffer;
            for (length = 0x80; length != 0; --length) {
                *reinterpret_cast<unsigned long*>(buffer) =
                    0; /* reinterpret-ok: status cells cleared through the raw buffer */
                buffer += 2;
            }
        }
        GetWorldColour00427290(&s_saved_colour_0065bac0);
        PublishLightDirection(&s_black_0065bd00);
        g_status_buffers_freed_0060ac8d = 0;
    }
    if (g_status_cursor_0065bd44 < 6) {
        index = g_status_cursor_0065bd44;
        g_status_cursor_0065bd44 = g_status_cursor_0065bd44 + 1;
        line = g_status_lines_0065bce8[index];
    } else {
        line = g_status_lines_0065bce8[5];
        if (g_status_scroll_0060ac8c != 0) {
            unsigned short* oldest = g_status_lines_0065bce8[0];
            for (index = 0; index < 5; ++index) {
                g_status_lines_0065bce8[index] = g_status_lines_0065bce8[index + 1];
            }
            g_status_lines_0065bce8[5] = oldest;
            scrolled = 1;
            line = oldest;
        }
    }
    scroll = 1;
    length = 0;
    while (*message != '\0') {
        if (*message > '\x1f') {
            line[length] = static_cast<unsigned short>(*message);
            length = length + 1;
        }
        if (*message == '\r' && message[1] == '\0') {
            scroll = 0;
        }
        ++message;
    }
    line[length] = 0;
    SetFont(g_smfnt_font_683694);
    SetRGBFontShadow(0, 0, 0);
    SetFontObjectPalette16BPP(g_smfnt_font_683694, g_font_state_palettes_68ee1c[5]);
    if (scrolled != 0) {
        ClearSurfaceRect(0, 400, 0x27f, 0x1df);
        index = 0x191;
        for (top = 0; top < 6; ++top) {
            gprintfDirty(1, index, const_cast<UINT16*>(L"%s"), g_status_lines_0065bce8[top]);
            index = index + 0xd;
        }
        InvalidateRegion(0, 400, 0x27f, 0x1df, 4);
    } else {
        index = (g_status_cursor_0065bd44 - 1) * 0xd;
        top = index + 400;
        ClearSurfaceRect(0, top, 0x27f, g_status_cursor_0065bd44 * 0xd + 400);
        gprintfDirty(1, index + 0x191, const_cast<UINT16*>(L"%s"), line);
        InvalidateRegion(0, top, 0x27f, g_status_cursor_0065bd44 * 0xd + 400, 4);
    }
    g_status_scroll_0060ac8c = scroll;
    RenderFrame();
    RenderFrame();
}

/* Quantize the vertex position into the weld grid and scan the 27 neighbouring
   cells for a positional match within 2.5 units; first hit wins and reports
   the stored 1-based index.  Otherwise the vertex is entered under its cell
   key with value index + 1.  A `link` of -1 maintains the vertex self-link or
   redirect fields. */
// FUNCTION: WIZ8 0x00494800
int WeldVertex00494800(W8HashTable<unsigned int, int>* table, W8OctPreTreeVertex* vertices,
                       unsigned int index, unsigned int link)
{
    unsigned int vertex = link;
    unsigned int cell_x;
    unsigned int cell_y;
    unsigned int cell_z;
    unsigned int key;
    unsigned int start_x;
    unsigned int start_y;
    unsigned int start_z;
    int slot;
    unsigned char found;
    unsigned char matched;
    unsigned int scan_x;
    unsigned int scan_y;
    unsigned int scan_z;
    unsigned int scan_key;
    int match_index;
    W8OctPreTreeVertex* current;
    W8OctPreTreeVertex* candidate;

    matched = 0;
    found = 0;
    if (link == 0xffffffff) {
        vertex = index;
    }
    current = vertices + vertex;
    cell_x = static_cast<unsigned int>(
        ((current->position_0c.x - g_weld_min_0065bab0.x) * g_float_005ecbb4));
    cell_y = static_cast<unsigned int>(
        ((current->position_0c.y - g_weld_min_0065bab0.y) * g_float_005ecbb4));
    cell_z = static_cast<unsigned int>(
        ((current->position_0c.z - g_weld_min_0065bab0.z) * g_float_005ecbb4));
    key = cell_z * g_weld_stride_z_0065bce4 + cell_y * g_weld_stride_y_0065bce0 +
          cell_x * g_weld_stride_x_0065bcdc;
    start_x = cell_x;
    if (cell_x != 0) {
        start_x = cell_x - 1;
    }
    start_y = cell_y;
    if (cell_y != 0) {
        start_y = cell_y - 1;
    }
    start_z = cell_z;
    if (cell_z != 0) {
        start_z = cell_z - 1;
    }
    for (scan_x = start_x; scan_x <= cell_x + 1; ++scan_x) {
        for (scan_y = start_y; scan_y <= cell_y + 1; ++scan_y) {
            for (scan_z = start_z; scan_z <= cell_z + 1; ++scan_z) {
                scan_key = scan_z * g_weld_stride_z_0065bce4 + scan_y * g_weld_stride_y_0065bce0 +
                           scan_x * g_weld_stride_x_0065bcdc;
                slot = table->bucket_heads[W8HashValue(scan_key) & (table->bucket_count - 1)];
                while (slot != -1) {
                    if (table->entries[slot].key == scan_key) {
                        match_index = table->entries[slot].value - 1;
                        if (match_index >= 0 && matched == 0) {
                            candidate = vertices + match_index;
                            if (fabs(current->position_0c.x - candidate->position_0c.x) <
                                    g_float_005ecbb8 &&
                                fabs(current->position_0c.y - candidate->position_0c.y) <
                                    g_float_005ecbb8 &&
                                fabs(current->position_0c.z - candidate->position_0c.z) <
                                    g_float_005ecbb8) {
                                if (link == 0xffffffff) {
                                    vertices[index].vertex_index_04 = match_index;
                                    vertices[index].flags_00 |= 1;
                                }
                                found = 1;
                            }
                        }
                    }
                    slot = table->entries[slot].next_index;
                }
            }
        }
    }
    if (found != 0) {
        return match_index + 1;
    }
    if (table->free_head == -1) {
        table->Grow();
    }
    slot = table->free_head;
    table->free_head = table->entries[slot].next_index;
    unsigned int bucket = W8HashValue(key) & (table->bucket_count - 1);
    table->entries[slot].key = key;
    table->entries[slot].value = index + 1;
    table->entries[slot].next_index = table->bucket_heads[bucket];
    table->bucket_heads[bucket] = slot;
    if (link == 0xffffffff) {
        vertices[index].vertex_index_04 = index;
    }
    return index + 1;
}

/* Converts the level mesh faces into the shared polygon array: each face's
   1-based corner indices are bounds-checked back into the file record, the
   polygon keeps an embedded copy of the face, plane/centroid are derived and
   two-sided or opposing-normal polygons weld their corners and emit a mirrored
   backface. Returns the alpha polygon count. */
// FUNCTION: WIZ8 0x00494B90
int BuildRegionPolygons00494B90(W8LevelFile* level, W8OctPreTreeGeometry* geometry,
                                unsigned char* classify)
{
    W8LevelFileMesh* mesh = level->pMeshes;
    W8MaterialRecord004B8A70* materials = level->pTextures;
    W8OctPreTreeVertex* vertices = geometry->vertices_04;
    char message[1024];
    int vertex_index[3];
    int kind_counts[3];
    int corner;
    int degenerate_count;
    int ordinal;
    int last_percent;
    unsigned long axis;
    float largest;
    float length;
    float offset;
    unsigned char opposing;
    srVector3T<float> normal;
    W8OctRegionPolygon* polygons;
    W8OctRegionPolygon* polygon;
    W8OctRegionPolygon* back;
    W8ReadMeshFace* face;
    W8OctPreTreeVertex* current;
    W8OctPreTreeVertex* created;

    {
        W8HashTable<unsigned int, int> keys;
        keys.Grow();
    }
    W8HashTable<unsigned int, int> weld_table;
    weld_table.Grow();
    kind_counts[0] = 0;
    kind_counts[1] = 0;
    kind_counts[2] = 0;
    mesh->num_faces_08 = mesh->num_faces_08 + 1;
    int poly_total = mesh->num_faces_08;
    polygons =
        static_cast<W8OctRegionPolygon*>(malloc(poly_total * 2 * sizeof(W8OctRegionPolygon)));
    if (polygons == 0) {
        return 0;
    }
    memset(polygons, 0, poly_total * 2 * sizeof(W8OctRegionPolygon));
    degenerate_count = 0;
    last_percent = 0;
    ReportBuildStatus00497690(6, "\nSorting and optimizing graphic polygons... \n");
    ordinal = 1;
    if (1 < poly_total) {
        polygon = polygons + 1;
        face = mesh->pstFaces;
        do {
            if (last_percent < static_cast<int>((static_cast<float>(ordinal) * 100.0f /
                                                 static_cast<float>(poly_total)))) {
                ++last_percent;
                sprintf(message, "  %d%% Complete:  %d Polygons processed \r", last_percent,
                        ordinal);
                ReportStartupMessage004969D0(message);
            }
            face->vertices[0] = face->vertices[0] + 1;
            face->vertices[1] = face->vertices[1] + 1;
            face->vertices[2] = face->vertices[2] + 1;
            if (face->vertices[0] < 1 || mesh->num_vertices_04 <= face->vertices[0]) {
                sprintf(message, "Mesh Read: Polygon %d Vertex 0 has invalid index: %d.", ordinal,
                        face->vertices[0]);
                goto invalid;
            }
            if (face->vertices[1] < 1 || mesh->num_vertices_04 <= face->vertices[1]) {
                sprintf(message, "Mesh Read: Polygon %d Vertex 1 has invalid index: %d.", ordinal,
                        face->vertices[1]);
                goto invalid;
            }
            if (face->vertices[2] < 1 || mesh->num_vertices_04 <= face->vertices[2]) {
                sprintf(message, "Mesh Read: Polygon %d Vertex 2 has invalid index: %d.", ordinal,
                        face->vertices[2]);
                goto invalid;
            }
            memcpy(&polygon->face_vertices_48, face, sizeof(W8ReadMeshFace));
            polygon->ordinal_04 = ordinal;
            polygon->material_24 = face->material_index;
            for (corner = 0; corner < 3; ++corner) {
                int index = face->vertices[corner];
                if (vertices[index].vertex_index_04 != static_cast<unsigned int>(index)) {
                    index = vertices[index].vertex_index_04;
                }
                vertex_index[corner] = index;
                polygon->vertices_34[corner] = vertices + index;
                polygon->position_18.x =
                    vertices[index].position_0c.x * g_float_005ec410 + polygon->position_18.x;
                polygon->position_18.y =
                    vertices[index].position_0c.y * g_float_005ec410 + polygon->position_18.y;
                polygon->position_18.z =
                    vertices[index].position_0c.z * g_float_005ec410 + polygon->position_18.z;
            }
            polygon->degenerate_30 = 0;
            if (vertex_index[0] == vertex_index[1] || vertex_index[0] == vertex_index[2] ||
                vertex_index[1] == vertex_index[2]) {
                ++degenerate_count;
                polygon->degenerate_30 = 1;
            }
            if (polygon->degenerate_30 == 0) {
                normal.x = 0.0f;
                normal.y = 0.0f;
                normal.z = 0.0f;
                int step = 2;
                for (corner = 0; corner < 3; ++corner) {
                    const W8OctPreTreeVertex* prev = vertices + vertex_index[(step - 1) % 3];
                    const W8OctPreTreeVertex* next = vertices + vertex_index[step % 3];
                    const W8OctPreTreeVertex* cur = vertices + vertex_index[corner];
                    ++step;
                    normal.x += (prev->original_position_54.z - next->original_position_54.z) *
                                cur->original_position_54.y;
                    normal.y += (prev->original_position_54.x - next->original_position_54.x) *
                                cur->original_position_54.z;
                    normal.z += (prev->original_position_54.y - next->original_position_54.y) *
                                cur->original_position_54.x;
                }
                length = sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
                normal.x = normal.x / length;
                normal.y = normal.y / length;
                normal.z = normal.z / length;
                offset = 0.0f;
                for (corner = 0; corner < 3; ++corner) {
                    current = vertices + vertex_index[corner];
                    offset = normal.z * current->position_0c.z + normal.y * current->position_0c.y +
                             normal.x * current->position_0c.x + offset;
                }
                offset = offset * g_float_005ec1a8;
                axis = 0;
                largest = 0.0f;
                polygon->plane_08[0] = normal.x;
                polygon->plane_08[1] = normal.y;
                polygon->plane_08[2] = normal.z;
                for (corner = 0; corner < 3; ++corner) {
                    float component = polygon->plane_08[corner];
                    if (largest < fabs(component)) {
                        largest = fabs(component);
                        axis = corner;
                    }
                }
                polygon->plane_08[3] = offset;
                polygon->flags_00 = axis;
                opposing = 0;
                for (corner = 0; corner < 3; ++corner) {
                    current = vertices + vertex_index[corner];
                    if (current->normal_count_18 != 0) {
                        length = sqrt(current->normal_24.x * current->normal_24.x +
                                      current->normal_24.y * current->normal_24.y +
                                      current->normal_24.z * current->normal_24.z);
                        if ((current->normal_24.z / length) * normal.z +
                                (current->normal_24.y / length) * normal.y +
                                (current->normal_24.x / length) * normal.x <
                            g_float_005ecbbc) {
                            opposing = 1;
                        }
                    }
                }
                if (opposing == 0) {
                    for (corner = 0; corner < 3; ++corner) {
                        current = vertices + vertex_index[corner];
                        current->normal_24.x = normal.x + current->normal_24.x;
                        current->normal_24.y = normal.y + current->normal_24.y;
                        current->normal_24.z = normal.z + current->normal_24.z;
                        current->normal_count_18 = current->normal_count_18 + 1;
                    }
                }
                if ((materials[face->material_index].shader_flags_116 & 1) != 0) {
                    opposing = 1;
                }
                polygon->positional_31 = 0;
                if (face->material_index == 0) {
                    polygon->kind_2c = 3;
                } else {
                    polygon->kind_2c = classify[face->material_index];
                }
                kind_counts[polygon->kind_2c] = kind_counts[polygon->kind_2c] + 1;
                if (ordinal < poly_total && opposing != 0) {
                    for (corner = 0; corner < 3; ++corner) {
                        int found = WeldVertex00494800(&weld_table, vertices, mesh->num_vertices_04,
                                                       vertex_index[corner]);
                        if (found == mesh->num_vertices_04 + 1) {
                            current = vertices + vertex_index[corner];
                            created = vertices + mesh->num_vertices_04;
                            memcpy(created, current, 0x60);
                            created->vertex_index_04 = mesh->num_vertices_04;
                            current->flags_00 = current->flags_00 | 2;
                            vertex_index[corner] = mesh->num_vertices_04;
                            mesh->num_vertices_04 = mesh->num_vertices_04 + 1;
                            created->flags_00 = created->flags_00 | 2;
                            created->normal_24.x = 0.0f;
                            created->normal_24.y = 0.0f;
                            created->normal_24.z = 0.0f;
                            created->normal_count_18 = 0;
                        } else {
                            vertex_index[corner] = found - 1;
                        }
                        if ((materials[face->material_index].shader_flags_116 & 1) == 0) {
                            polygon->vertices_34[corner] = vertices + vertex_index[corner];
                        }
                        current = vertices + vertex_index[corner];
                        current->normal_24.x = normal.x + current->normal_24.x;
                        current->normal_24.y = normal.y + current->normal_24.y;
                        current->normal_24.z = normal.z + current->normal_24.z;
                        current->normal_count_18 = current->normal_count_18 + 1;
                    }
                    if ((materials[face->material_index].shader_flags_116 & 1) != 0) {
                        back = polygons + mesh->num_faces_08;
                        memcpy(back, polygon, sizeof(W8OctRegionPolygon));
                        back->ordinal_04 = mesh->num_faces_08;
                        back->vertices_34[0] = vertices + vertex_index[1];
                        back->vertices_34[1] = vertices + vertex_index[0];
                        back->vertices_34[2] = vertices + vertex_index[2];
                        back->plane_08[0] = back->plane_08[0] * g_negative_one_005ebc38;
                        back->plane_08[1] = back->plane_08[1] * g_negative_one_005ebc38;
                        back->plane_08[2] = back->plane_08[2] * g_negative_one_005ebc38;
                        back->plane_08[3] = back->plane_08[3] * g_negative_one_005ebc38;
                        back->face_vertices_48[0] = vertex_index[1];
                        back->face_vertices_48[1] = vertex_index[0];
                        back->face_vertices_48[2] = vertex_index[2];
                        back->uvs_54[0] = face->texture_coordinates[1];
                        back->uvs_54[1] = face->texture_coordinates[0];
                        mesh->num_faces_08 = mesh->num_faces_08 + 1;
                    }
                }
            }
            ++ordinal;
            ++face;
            ++polygon;
        } while (ordinal < poly_total);
    }
    geometry->vertex_count_00 = mesh->num_vertices_04;
    geometry->polygon_count_08 = mesh->num_faces_08;
    geometry->polygons_0c = polygons;
    sprintf(message, "  100%% Complete:  %d Polygons processed \r", ordinal);
    ReportBuildStatus00497690(6, message);
    sprintf(message, "Number of Two-sided Polys (Polys added): %d\n",
            mesh->num_faces_08 - poly_total);
    ReportBuildStatus00497690(6, message);
    sprintf(message, "Number of Non-Alpha Polys: %d\n", kind_counts[0]);
    ReportBuildStatus00497690(6, message);
    sprintf(message, "Number of Subtractive Alpha Polys: %d\n", kind_counts[1]);
    ReportBuildStatus00497690(6, message);
    sprintf(message, "Number of Additive Alpha Polys: %d\n", kind_counts[2]);
    ReportBuildStatus00497690(6, message);
    if (degenerate_count != 0) {
        sprintf(message, "Number of Degenerate Polys: %d\n", degenerate_count);
        ReportBuildStatus00497690(6, message);
    }
    ReportBuildStatus00497690(6, "\n");
    return kind_counts[1] + kind_counts[2];
invalid:
    ReportBuildStatus00497690(7, message);
    return 0;
}

/* Duplicates build vertices whose polygons disagree on material so every
   polygon corner can point at a vertex carrying that polygon's material and
   uv, then repacks the vertex array and repoints the corners. */
// FUNCTION: WIZ8 0x00495860
int SplitVerticesByMaterial00495860(W8OctPreTreeGeometry* geometry)
{
    char message[1024];
    unsigned int source;
    unsigned int slot;
    unsigned int next;
    int corner;
    int remaining;
    unsigned char fresh;
    int* faces;
    W8OctPreTreeVertex* split;
    W8OctPreTreeVertex* vertices;
    W8OctPreTreeVertex* record;
    W8OctPreTreeVertex* candidate;
    W8OctPreTreeVertex** corner_vertex;
    W8OctRegionPolygon* polygon;

    ReportBuildStatus00497690(6, "Splitting vertices by Material...\n");
    split = static_cast<W8OctPreTreeVertex*>(malloc(geometry->vertex_count_00 * 0x180));
    if (split == 0) {
        ReportBuildStatus00497690(7, "SplitVertices: Could not allocate pSplitVerts.\n");
        return 0;
    }
    memset(split, 0, geometry->vertex_count_00 * 0x180);
    for (source = 1; source < geometry->vertex_count_00; ++source) {
        geometry->vertices_04[source].flag_0a = 0;
    }
    next = 1;
    for (source = 1; source < geometry->vertex_count_00; ++source, ++next) {
        unsigned int first = next;
        record = split + next;
        memcpy(record, geometry->vertices_04 + source, 0x60);
        record->vertex_index_04 = next;
        record->flag_0a = 1;
        faces = geometry->vertices_04[source].face_indices_44;
        remaining = geometry->vertices_04[source].face_count_40 - 1;
        polygon = geometry->polygons_0c + *faces;
        record->material_1c = polygon->material_24;
        record->kind_20 = polygon->kind_2c;
        for (corner = 0; corner < 3; ++corner) {
            corner_vertex = polygon->vertices_34 + corner;
            if ((*corner_vertex)->vertex_index_04 == source && (*corner_vertex)->flag_0a == 0) {
                *corner_vertex = record;
                record->uv_4c = polygon->uvs_54[corner];
            }
        }
        while (remaining != 0) {
            ++faces;
            polygon = geometry->polygons_0c + *faces;
            fresh = 1;
            slot = first;
            candidate = split + first;
            while (slot <= next) {
                if (fresh == 0) {
                    break;
                }
                for (corner = 0; corner < 3; ++corner) {
                    corner_vertex = polygon->vertices_34 + corner;
                    if ((*corner_vertex)->vertex_index_04 == source &&
                        (*corner_vertex)->flag_0a == 0 &&
                        candidate->material_1c == static_cast<int>(polygon->material_24)) {
                        *corner_vertex = candidate;
                        fresh = 0;
                    }
                }
                ++slot;
                ++candidate;
            }
            if (fresh != 0) {
                ++next;
                record = split + next;
                memcpy(record, geometry->vertices_04 + source, 0x60);
                record->vertex_index_04 = next;
                record->flag_0a = 1;
                record->material_1c = polygon->material_24;
                record->kind_20 = polygon->kind_2c;
                for (corner = 0; corner < 3; ++corner) {
                    corner_vertex = polygon->vertices_34 + corner;
                    if ((*corner_vertex)->vertex_index_04 == source &&
                        (*corner_vertex)->flag_0a == 0) {
                        *corner_vertex = record;
                        record->uv_4c = polygon->uvs_54[corner];
                    }
                }
            }
            --remaining;
        }
    }
    free(geometry->vertices_04);
    slot = next * 3 + 3;
    vertices = static_cast<W8OctPreTreeVertex*>(malloc(slot * 0x20));
    geometry->vertices_04 = vertices;
    if (vertices == 0) {
        ReportBuildStatus00497690(7, "SplitVertices: Could not allocate pGeom->pVerts.\n");
    }
    memset(vertices, 0, slot * 0x20);
    memcpy(vertices, split, slot * 0x20);
    sprintf(message, "Split %d vertices into %d new vertices--ratio is 1 to %.1f.\n\n",
            static_cast<int>(geometry->vertex_count_00), next,
            static_cast<double>(next) / static_cast<double>(geometry->vertex_count_00));
    ReportBuildStatus00497690(6, message);
    geometry->vertex_count_00 = next;
    if (1 < geometry->polygon_count_08) {
        polygon = geometry->polygons_0c + 1;
        for (source = 1; source < geometry->polygon_count_08; ++source, ++polygon) {
            for (corner = 0; corner < 3; ++corner) {
                polygon->vertices_34[corner] =
                    vertices + polygon->vertices_34[corner]->vertex_index_04;
            }
        }
    }
    free(split);
    return 1;
}

// FUNCTION: WIZ8 0x00495CF0
int AccumulateVertexLight00495CF0(OctPreTree* tree, W8OctPreTreeVertex* vertex, short light_count,
                                  W8LevelFileLight* lights, int* sun_map)
{
    float delta_x;
    float delta_y;
    float delta_z;
    float distance;
    float dot;
    float scale;
    short lit;
    int* sun;
    W8LevelFileLight* light;

    lit = 0;
    if (light_count < 1) {
        return 0;
    }
    light = lights;
    sun = sun_map;
    do {
        if ((light->version_00 < 2) ||
            (((light->flags_02 & 0xff) == 0) && ((light->flags_02 & 0xff00) != 0))) {
            delta_x = light->position_08.x - vertex->position_0c.x;
            delta_y = light->position_08.y - vertex->position_0c.y;
            delta_z = light->position_08.z - vertex->position_0c.z;
            distance = sqrt(delta_x * delta_x + delta_y * delta_y + delta_z * delta_z);
            if ((distance < light->range_24) || ((sun_map != 0) && (*sun != 0))) {
                g_light_candidates_0065bd18 = g_light_candidates_0065bd18 + 1;
                dot = (delta_x / distance) * vertex->normal_24.x +
                      (delta_y / distance) * vertex->normal_24.y +
                      (delta_z / distance) * vertex->normal_24.z;
                if (g_float_005ebb34 < dot) {
                    g_lights_facing_0065bd1c = g_lights_facing_0065bd1c + 1;
                    if (g_option_shadow_test_0060ac71 != 0) {
                        if (!tree->SegmentClear00467BB0(&light->position_08,
                                                        &vertex->position_0c)) {
                            goto next_light;
                        }
                    }
                    g_lights_unblocked_0065bd14 = g_lights_unblocked_0065bd14 + 1;
                    lit = lit + 1;
                    if ((sun_map == 0) || (*sun == 0)) {
                        scale = dot * light->intensity_20 *
                                (g_float_005ebb38 - distance / light->range_24);
                        vertex->light_30.x = scale * light->colour_14.x + vertex->light_30.x;
                        vertex->light_30.y = scale * light->colour_14.y + vertex->light_30.y;
                        vertex->light_30.z = scale * light->colour_14.z + vertex->light_30.z;
                    } else {
                        *vertex->sun_lights_3c = dot * light->intensity_20 + *vertex->sun_lights_3c;
                    }
                }
            }
        }
    next_light:
        sun = sun + 1;
        ++light;
        light_count = light_count - 1;
        if (light_count == 0) {
            return lit;
        }
    } while (true);
}

/* Tests whether a prop/bitmap record sees a given sun light: the segment from
   the light to the record position must be clear, else each corner of each
   recorded bounds pair is tried. */
// FUNCTION: WIZ8 0x00495E90
int PropReceivesLight00495E90(OctPreTree* tree, W8LevelFileProp* prop, W8LevelFileLight* light)
{
    int corner_x;
    int corner_y;
    int corner_z;
    unsigned char bound;
    float bounds[6];
    srVector3T<float> position;
    srVector3T<float> corner;

    position.x = prop->position_03.x * g_world_scale_005ebc40;
    position.y = prop->position_03.y * g_world_scale_005ebc40;
    position.z = prop->position_03.z * g_world_scale_005ebc40;
    if (tree->SegmentClear00467BB0(&light->position_08, &position)) {
        return 1;
    }
    bound = 0;
    if (prop->anim_obj_53.num_bound_box_47 != 0) {
        const W8LevelFileBounds* boxes = prop->anim_obj_53.pBoundBox;
        do {
            memcpy(bounds, boxes + bound, 0x18);
            for (corner_x = 0; corner_x < 2; ++corner_x) {
                float x = bounds[corner_x * 3] * g_world_scale_005ebc40;
                for (corner_y = 0; corner_y < 2; ++corner_y) {
                    float y = bounds[corner_y * 3 + 1] * g_world_scale_005ebc40;
                    for (corner_z = 0; corner_z < 2; ++corner_z) {
                        corner.z = bounds[corner_z * 3 + 2] * g_world_scale_005ebc40;
                        corner.x = x;
                        corner.y = y;
                        if (tree->SegmentClear00467BB0(&light->position_08, &corner)) {
                            return 1;
                        }
                    }
                }
            }
            bound = bound + 1;
        } while (bound < prop->anim_obj_53.num_bound_box_47);
    }
    return 0;
}

/* Classifies every material's automesh kind: picks the first populated texture
   name, probes the bitmap (following .ifl indirection) for a real alpha channel
   when the record claims full opacity, and records missing textures in the
   prop/sun bit array. Returns the per-material kind byte array. */
// FUNCTION: WIZ8 0x00496000
unsigned char* ClassifyTextures00496000(W8MaterialRecord004B8A70* textures, int count, char* stem)
{
    char folder[1024];
    char texture[1024];
    char path[1024];
    char message[1024];
    unsigned char more;
    unsigned char* kinds;
    int missing;
    int index;
    int file;
    unsigned char kind;
    stTextureFile* probe;

    missing = 0;
    probe = 0;
    kinds = static_cast<unsigned char*>(malloc(count));
    memset(kinds, 0, count);
    strcpy(folder, stem);
    char* slash = strrchr(folder, '\\');
    if (slash != 0) {
        slash[1] = '\0';
    }
    strcat(folder, "Bitmaps\\");
    if (DirectoryExists(folder) == 0) {
        ReportBuildStatus00497690(
            7, "Couldn't find bitmaps directory--cannot check texture types.\n\n");
        kinds = 0;
    } else {
        g_prop_sun_bits_0065bd3c = new BitArray(count);
        for (index = 0; index < count; ++index) {
            W8MaterialRecord004B8A70* record = textures + index;
            unsigned char opaque = 1.0f <= record->opacity_0fd;
            texture[0] = '\0';
            if (record->positional_001[0] != 0) {
                strcpy(texture, reinterpret_cast<const char*>(/* reinterpret-ok: texture-name text
                            bytes */
                                                              record->positional_001));
            }
            if (record->texture_names_029[0][0] == '\0') {
                if (record->texture_names_029[1][0] == '\0') {
                    if (record->texture_names_029[2][0] == '\0') {
                        if (record->texture_names_029[3][0] == '\0') {
                            goto probe;
                        }
                        strcpy(texture, record->texture_names_029[3]);
                        kind = 2;
                    } else {
                        strcpy(texture, record->texture_names_029[2]);
                        kind = 2;
                    }
                } else {
                    strcpy(texture, record->texture_names_029[1]);
                    kind = 1;
                }
            } else {
                strcpy(texture, record->texture_names_029[0]);
                if (opaque != 0) {
                    goto probe;
                }
                kind = 1;
            }
            goto store;
        probe:
            kind = 0;
            if (texture[0] == '\0') {
                kind = 3;
            } else {
                sprintf(path, "%s%s", folder, texture);
                file = FileOpen(path, FILE_ACCESS_READ, 0);
                if (file == 0) {
                    g_prop_sun_bits_0065bd3c->Set(index);
                    ++missing;
                    sprintf(message, "Could not find texture file %s.\n", path);
                    ReportBuildStatus00497690(5, message);
                } else {
                    if (_stricmp(path + strlen(path) - 4, "ifl") == 0) {
                        ReadTextLine004CEE40(file, texture, 0x3ff, &more);
                        FileClose(file);
                        sprintf(path, "%s%s", folder, texture);
                        file = FileOpen(path, FILE_ACCESS_READ, 0);
                    }
                    if (file == 0) {
                        g_prop_sun_bits_0065bd3c->Set(index);
                        ++missing;
                        sprintf(message, "Could not find texture file %s.\n", path);
                        ReportBuildStatus00497690(5, message);
                    } else {
                        FileClose(file);
                        if (probe == 0) {
                            probe = new stTextureFile(path, 1);
                        } else {
                            probe->setFileName(path);
                        }
                        if (probe == 0) {
                            sprintf(message, "Could not create stTextureFile object.\n");
                            ReportBuildStatus00497690(7, message);
                            return 0;
                        }
                        probe->loadSurface();
                        kind = probe->hasAlpha() != 0;
                        probe->releaseSurface();
                    }
                }
            }
        store:
            kinds[index] = kind;
        }
        if (missing != 0) {
            sprintf(message, "%d missing texture names found!\n", missing);
            ReportBuildStatus00497690(6, message);
        }
    }
    return kinds;
}

/* Groups materials by canonical texture name and by their rendered signature
   string, remaps each polygon's material index at its group's representative
   and moves the old index into the texture slot. */
// FUNCTION: WIZ8 0x00496500
int MaterialSort00496500(W8OctPreTreeGeometry* geometry, W8MaterialRecord004B8A70* textures,
                         int count, unsigned char* classify)
{
    char name[516];
    char* material_names;
    char* texture_names;
    int* material_lookup;
    int* texture_lookup;
    int material_count;
    int texture_count;
    int missing;
    int index;
    int group;
    int canonical;
    int scan;
    const char* texture_name;
    W8OctRegionPolygon* polygon;

    missing = 0;
    material_names = static_cast<char*>(malloc(count << 9));
    if (material_names == 0) {
        ReportBuildStatus00497690(7, "MaterialSort: Could not allocate acMatNames\n");
    }
    memset(material_names, 0, count << 9);
    texture_names = static_cast<char*>(malloc(count << 9));
    if (texture_names == 0) {
        ReportBuildStatus00497690(7, "MaterialSort: Could not allocate acTextNames\n");
        return 0;
    }
    memset(texture_names, 0, count << 9);
    material_lookup = static_cast<int*>(malloc(count * 4));
    if (material_lookup == 0) {
        ReportBuildStatus00497690(7, "MaterialSort: Could not allocate piMatLookup\n");
        return 0;
    }
    memset(material_lookup, 0, count * 4);
    texture_lookup = static_cast<int*>(malloc(count * 4));
    if (texture_lookup == 0) {
        ReportBuildStatus00497690(7, "MaterialSort: Could not allocate piTextLookup\n");
        return 0;
    }
    memset(texture_lookup, 0, count * 4);
    material_count = 0;
    texture_count = 0;
    if (1 < count) {
        W8MaterialRecord004B8A70* record = textures + 1;
        int* material_slot = material_lookup;
        for (index = 1; index < count; ++index) {
            ++material_slot;
            ++record;
            texture_name = reinterpret_cast<const char*>(/* reinterpret-ok: texture-name text
                    bytes */
                                                         record->positional_001);
            if (*texture_name == '\0') {
                texture_name = record->texture_names_029[0];
            }
            if (*texture_name == '\0') {
                texture_name = record->texture_names_029[1];
            }
            if (*texture_name == '\0') {
                texture_name = record->texture_names_029[2];
            }
            if (*texture_name == '\0') {
                texture_name = record->texture_names_029[3];
            }
            if (*texture_name == '\0') {
                ReportBuildStatus00497690(7, "MaterialSort: Missing Texture Name\n");
                return 0;
            }
            strcpy(name, texture_name);
            for (scan = 0; scan < texture_count; ++scan) {
                if (strcmp(texture_names + scan * 0x200, name) == 0) {
                    break;
                }
            }
            texture_lookup[index] = scan;
            if (scan == texture_count) {
                strcpy(texture_names + scan * 0x200, name);
                ++texture_count;
            }
            sprintf(name, "Mat:%1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %d %c",
                    record->diffuse_0d5[0], record->diffuse_0d5[1], record->diffuse_0d5[2],
                    record->specular_0ed[0], record->specular_0ed[1], record->specular_0ed[2],
                    record->positional_0f9, record->opacity_0fd, record->emission_101,
                    static_cast<int>(record->shader_flags_116), classify[index]);
            for (scan = 0; scan < material_count; ++scan) {
                if (strcmp(material_names + scan * 0x200, name) == 0) {
                    break;
                }
            }
            *material_slot = scan;
            if (scan == material_count) {
                strcpy(material_names + scan * 0x200, name);
                ++material_count;
            }
        }
    }
    for (group = 0; group < material_count; ++group) {
        canonical = -1;
        for (scan = 1; scan < count; ++scan) {
            if (material_lookup[scan] == group) {
                if (canonical < 0) {
                    canonical = scan + 1 + count;
                }
                material_lookup[scan] = canonical;
            }
        }
    }
    if (1 < count) {
        for (index = 1; index < count; ++index) {
            material_lookup[index] = material_lookup[index] + (-1 - count);
        }
    }
    if (1 < static_cast<int>(geometry->polygon_count_08)) {
        polygon = geometry->polygons_0c + 1;
        for (index = 1; index < static_cast<int>(geometry->polygon_count_08); ++index, ++polygon) {
            if (g_prop_sun_bits_0065bd3c->Test(index) != 0) {
                ++missing;
            }
            polygon->texture_28 = polygon->material_24;
            polygon->material_24 = material_lookup[polygon->material_24];
        }
        if (missing != 0) {
            sprintf(name, "\nWARNING!!! Missing textures assigned to %d polys!\n", missing);
            ReportBuildStatus00497690(6, name);
        }
    }
    geometry->material_count_18 = material_count;
    geometry->texture_count_1c = texture_count;
    free(material_names);
    free(texture_names);
    free(material_lookup);
    free(texture_lookup);
    return 1;
}

/* Interactive OctBuild option screen: draws the seven status lines, handles the
   toggle keys and the six numeric editors, then prints the preprocessing banner
   line and restores the saved light direction. */
// FUNCTION: WIZ8 0x00496CD0
void OctBuildOptions00496CD0(char* stem)
{
    unsigned char done = g_build_level_links_0065bd2c;
    EnvironmentColour colour_saved;
    EnvironmentColour colour_backup;
    char* lines[7];
    unsigned short* wide[7];
    char log_state[8];
    char rename_state[8];
    char mesh_state[8];
    char pathing_state[8];
    char spare_state[8];
    char edit_buffer[20];
    char edit_char[8];
    InputAtom atom;
    int edit_mode;
    int line;
    int index;
    short length;
    MSG message;

    memset(&colour_saved, 0, sizeof(colour_saved));
    memset(&colour_backup, 0, sizeof(colour_backup));
    GetWorldColour00427290(&colour_backup);
    PublishLightDirection(&colour_saved);
    SetFont(g_smfnt_font_683694);
    SetRGBFontShadow(0, 0, 0);
    SetFontObjectPalette16BPP(g_smfnt_font_683694, g_font_state_palettes_68ee1c[5]);
    edit_char[0] = 0;
    edit_char[4] = 0;
    for (index = 0; index < 7; ++index) {
        lines[index] = new char[0x100];
        memset(lines[index], 0, 0x100);
        wide[index] = new unsigned short[0x100];
        memset(wide[index], 0, 0x200);
    }
    strcpy(log_state, "OFF");
    if (g_option_pathing_0060ac70 == 0) {
        strcpy(pathing_state, "OFF");
    } else {
        strcpy(pathing_state, "ON");
    }
    strcpy(rename_state, "OFF");
    strcpy(mesh_state, "ON ");
    strcpy(spare_state, "OFF");
    strcpy(edit_buffer, "");
    while (done == 0) {
        edit_mode = 0;
        for (;;) {
            const char* prompt;
            sprintf(lines[0], "OCTBUILD VERSION %d -- OPTIONS: ", 0x22);
            sprintf(lines[1], "(L)og %s               ", log_state);
            if (g_option_pathing_0060ac70 == 0) {
                sprintf(lines[2], "(P)athing %s", pathing_state);
            } else {
                sprintf(lines[2],
                        "(P)athing %s    (N)ode Spacing: %5.2fm    (H)ead Room:   %5.2fm"
                        "    (D)elete Percentage: %d",
                        pathing_state, (g_option_path_node_spacing_0060ac74 * g_float_005ebc60),
                        (g_option_path_head_room_0060ac78 * g_float_005ebc60),
                        g_option_delete_percentage_0060ac7c);
            }
            sprintf(lines[3], "(R)ename Alphas %s    (M)esh Linking %s", rename_state, mesh_state);
            sprintf(lines[4],
                    "Min. Leaf (S)ize %5.2fm    Max. Leaf (C)ount %d    (A)uto Region"
                    " Size %5.2fm",
                    (g_option_min_leaf_size_0060ac80 * g_float_005ebc60),
                    g_option_max_leaf_count_0060ac88,
                    (g_option_auto_region_size_0065bd30 * g_float_005ebc60));
            sprintf(lines[5], "Hit ENTER to accept,  ESC to cancel and exit");
            if (edit_mode == 0) {
                sprintf(lines[6], " ");
            } else {
                if (edit_mode == 1) {
                    prompt = " Path Node Spacing: %s"
                             "                                          ";
                } else if (edit_mode == 2) {
                    prompt = " Minimum Leaf Size: %s"
                             "                           ";
                } else if (edit_mode == 3) {
                    prompt = " Auto Region Size: %s"
                             "                                             ";
                } else if (edit_mode == 4) {
                    prompt = " Maximum Number of Octree Leaves: %s"
                             "                              ";
                } else if (edit_mode == 5) {
                    prompt = " Path Node Head Room: %s"
                             "                                          ";
                } else {
                    prompt = " Delete path node groups less than this %% of the total: %s"
                             "       ";
                }
                sprintf(lines[6], prompt, edit_buffer);
            }
            ClearSurfaceRect(0, 0x183, 0x27f, 0x1df);
            for (line = 0; line < 7; ++line) {
                length = 0;
                if (lines[line][0] != '\0') {
                    do {
                        if (0x59 < length) {
                            goto draw;
                        }
                        wide[line][length] = static_cast<short>(lines[line][length]);
                        ++length;
                    } while (lines[line][length] != '\0');
                }
                while (length < 0x5a) {
                    wide[line][length] = 0x20;
                    ++length;
                }
            draw:
                wide[line][length] = 0;
                gprintfDirty(1, 0x184 + line * 0xd, const_cast<UINT16*>(g_format_s_006068e4),
                             wide[line]);
            }
            InvalidateRegion(0, 0x183, 0x27f, 0x1df, 4);
            while (DequeueEvent(&atom) == 0) {
                RenderFrame();
                RenderFrame();
                WaitMessage();
                if (PeekMessageA(&message, (HWND)0, 0, 0, 0) != 0 &&
                    GetMessageA(&message, (HWND)0, 0, 0) != 0) {
                    TranslateMessage(&message);
                    DispatchMessageA(&message);
                }
            }
            if (atom.usEvent != 1) {
                continue;
            }
            if (atom.usParam == 0x1b) {
                ShutdownWithErrorBox("Cancelled!  Program exiting...");
                goto accepted;
            }
            if (atom.usParam == 0x0d) {
                break;
            }
            if (edit_mode == 0) {
                switch (toupper(atom.usParam)) {
                case 0x41:
                    edit_mode = 3;
                    break;
                case 0x43:
                    edit_mode = 4;
                    break;
                case 0x44:
                    if (g_option_pathing_0060ac70 != 0) {
                        edit_mode = 6;
                    }
                    break;
                case 0x48:
                    if (g_option_pathing_0060ac70 != 0) {
                        edit_mode = 5;
                    }
                    break;
                case 0x4c:
                    if (g_option_logging_0060ac72 == 0) {
                        strcpy(log_state, "ON ");
                        g_option_logging_0060ac72 = 1;
                    } else {
                        strcpy(log_state, "OFF");
                        g_option_logging_0060ac72 = 0;
                    }
                    break;
                case 0x4d:
                    if (g_option_mesh_linking_0060ac73 == 0) {
                        strcpy(mesh_state, "ON ");
                        g_option_mesh_linking_0060ac73 = 1;
                    } else {
                        strcpy(mesh_state, "OFF");
                        g_option_mesh_linking_0060ac73 = 0;
                    }
                    break;
                case 0x4e:
                    edit_mode = 1;
                    break;
                case 0x50:
                    if (g_option_pathing_0060ac70 == 0) {
                        strcpy(pathing_state, "ON ");
                        g_option_pathing_0060ac70 = 1;
                    } else {
                        strcpy(pathing_state, "OFF");
                        g_option_pathing_0060ac70 = 0;
                    }
                    break;
                case 0x52:
                    if (g_option_rename_alphas_0065bd2d == 0) {
                        strcpy(rename_state, "ON ");
                        g_option_rename_alphas_0065bd2d = 1;
                    } else {
                        strcpy(rename_state, "OFF");
                        g_option_rename_alphas_0065bd2d = 0;
                    }
                    break;
                case 0x53:
                    edit_mode = 2;
                    break;
                }
            } else {
                char key = static_cast<char>(toupper(atom.usParam));
                if ((key < '0' || '9' < key) && key != '.') {
                    if (key == '\b') {
                        edit_buffer[strlen(edit_buffer) - 1] = '\0';
                    }
                } else {
                    edit_char[0] = key;
                    strcat(edit_buffer, edit_char);
                }
            }
        }
        if (edit_mode == 0) {
            done = 1;
        } else if (edit_mode == 1) {
            g_option_path_node_spacing_0060ac74 =
                static_cast<float>(atof(edit_buffer)) * g_world_scale_005ebc40;
        } else if (edit_mode == 2) {
            g_option_min_leaf_size_0060ac80 =
                static_cast<float>(atof(edit_buffer)) * g_world_scale_005ebc40;
        } else if (edit_mode == 3) {
            g_option_auto_region_size_0065bd30 =
                static_cast<float>(atof(edit_buffer)) * g_world_scale_005ebc40;
        } else if (edit_mode == 4) {
            g_option_max_leaf_count_0060ac88 = atoi(edit_buffer);
        } else if (edit_mode == 5) {
            g_option_path_head_room_0060ac78 =
                static_cast<float>(atof(edit_buffer)) * g_world_scale_005ebc40;
        } else if (edit_mode == 6) {
            g_option_delete_percentage_0060ac7c = atoi(edit_buffer);
        }
        edit_buffer[0] = '\0';
    }
accepted:
    ClearSurfaceRect(0, 0x183, 0x27f, 0x1df);
    sprintf(lines[1], "OCTBUILD VERSION %d -- Preprocessing %s: ", 0x22, stem);
    length = 0;
    if (lines[1][0] != '\0') {
        do {
            if (0x59 < length) {
                goto shown;
            }
            wide[1][length] = static_cast<short>(lines[1][length]);
            ++length;
        } while (lines[1][length] != '\0');
    }
    while (length < 0x5a) {
        wide[1][length] = 0x20;
        ++length;
    }
shown:
    wide[1][length] = 0;
    gprintfDirty(1, 0x184, const_cast<UINT16*>(g_format_s_006068e4), wide[1]);
    InvalidateRegion(0, 0x183, 0x27f, 0x1df, 4);
    RenderFrame();
    RenderFrame();
    for (index = 0; index < 7; ++index) {
        delete[] lines[index];
        delete[] wide[index];
    }
    PublishLightDirection(&colour_saved);
}

/* Select the first populated texture layer, load its file or IFL animation,
   derive the renderer flags, and retain a registry-cached stMaterial keyed by
   all serialized parameters that affect it. The sixth caller argument is an
   established cdecl extra argument: retail 0x004B8A70 never reads it and
   always passes required=1 to the texture loaders. */
// FUNCTION: WIZ8 0x004B8A70
unsigned char LoadMaterial004B8A70(const char* bitmap_folder,
                                   const W8MaterialRecord004B8A70* source,
                                   srMaterialIFace** material, srTextureIFace** texture,
                                   unsigned long* render_flags, int)
{
    char texture_path[80] = "";
    char material_name[80] = "";
    char drive[_MAX_PATH];
    char directory[_MAX_PATH];
    char file_name[_MAX_PATH];
    char extension[_MAX_PATH];
    char texture_folder[_MAX_PATH];
    char texture_file[_MAX_PATH];
    int texture_index = -1;
    unsigned char has_alpha = 0;
    int index;

    *render_flags = 0x0100a51b; /* packed srShader; TEXTURING set until no texture */
    for (index = 0; index < 4; ++index) {
        if (source->texture_names_029[index][0] != '\0') {
            if (bitmap_folder[0] == '\0') {
                strcpy(texture_path, source->texture_names_029[index]);
            } else {
                sprintf(texture_path, "%s\\%s", bitmap_folder, source->texture_names_029[index]);
            }
            texture_index = index;
            break;
        }
    }

    if (texture_path[0] == '\0') {
        *render_flags &= ~srShader::MASK_TEXTURING;
    } else {
        _splitpath(texture_path, drive, directory, file_name, extension);
        strcpy(texture_folder, drive);
        strcat(texture_folder, directory);
        strcpy(texture_file, file_name);
        strcat(texture_file, extension);

        if (strlen(texture_path) > 3 && _strnicmp(extension, ".IFL", 4) == 0) {
            *texture = LoadAnimatedTexture004B98F0(texture_folder, texture_file, source, 1);
        } else {
            *texture = LoadTexture004B95D0(texture_folder, texture_file, 1);
        }
        if (*texture == 0) {
            return 0;
        }

        if ((*texture)->getClassID() == 0x10001 &&
            static_cast<stTextureFile*>(*texture)->hasAlpha()) {
            has_alpha = 1;
        }
        if ((*texture)->getClassID() == stTextureAnim::CLASS_ID) {
            stTextureAnim* animation = static_cast<stTextureAnim*>(*texture);

            if (animation->Prepare004857B0()) {
                has_alpha = 1;
            }
            if (source->version_00 > 3 && source->texture_modes_11a[texture_index] > 0.0f) {
                float mode = source->texture_modes_11a[texture_index];
                if (mode <= 1.0f) {
                    animation->value_70 = 1;
                } else {
                    animation->value_70 = 2;
                    mode -= 1.0f;
                }
                animation->value_74 = mode;
            }
        }
    }

    if (source->opacity_0fd < 1.0f || has_alpha) {
        if (texture_index == 0 || texture_index == 1) {
            *render_flags = (*render_flags & 0xffffdfbfUL) | 0x40a0;
        } else if (texture_index == 2 || texture_index == 3) {
            *render_flags = (*render_flags & 0xffffdc3fUL) | 0x4020;
        }
        *render_flags &= ~8UL;
    }

    sprintf(material_name,
            "Mt%1.2f%1.2f%1.2f%1.2f%1.2f%1.2f%1.2f%1.2f%1.2f"
            "%1.2f%1.2f%1.2f%1.2f%1.2f%d%c",
            source->ambient_0c9[0], source->ambient_0c9[1], source->ambient_0c9[2],
            source->diffuse_0d5[0], source->diffuse_0d5[1], source->diffuse_0d5[2],
            source->specular_0ed[0], source->specular_0ed[1], source->specular_0ed[2],
            source->positional_0f9, source->opacity_0fd, source->emission_101, source->emission_101,
            source->emission_101, static_cast<int>(source->shader_flags_116),
            texture_path[0] == '\0' ? 'F' : 'T');

    {
        srRegistry* registry = srCore.getRegistry();
        srRegistry::ClassNode* node = registry->getClassNode(0x10002);
        stMaterial* concrete;

        if (node == 0) {
            node = registry->registerClass(
                "stMaterial",
                srClassSupport<srMaterial, srMaterialIFace, false, 0x2210>::sGetClassNode(),
                0x10002, 0);
        }
        concrete = static_cast<stMaterial*>(
            registry->find(node, material_name, static_cast<const srRuntimeClass*>(0)));
        *material = concrete;
        if (concrete == 0) {
            concrete = new stMaterial;
            *material = concrete;
            if (concrete == 0) {
                srAssertFail("*ppstMaterial", MATERIALS_CPP, 0xe4, 0);
            }
            concrete->setName(material_name);
            concrete->autoRelease();

            concrete->parms_18.specular.Set(source->specular_0ed[0], source->specular_0ed[1],
                                            source->specular_0ed[2], 0.0f);
            concrete->dirty_74 = 1;
            concrete->parms_18.shininess = 1.0f;
            concrete->dirty_74 = 1;

            concrete->parms_18.diffuse.x = source->diffuse_0d5[0];
            concrete->parms_18.diffuse.y = source->diffuse_0d5[1];
            concrete->parms_18.diffuse.z = source->diffuse_0d5[2];
            concrete->parms_18.diffuse.w = source->opacity_0fd == 0.0f ? 0.7f : source->opacity_0fd;
            concrete->dirty_74 = 1;
            concrete->setOpacity(source->opacity_0fd == 0.0f ? 0.7 : source->opacity_0fd);

            if (texture_path[0] == '\0') {
                concrete->parms_18.ambient.Set(source->diffuse_0d5[0], source->diffuse_0d5[1],
                                               source->diffuse_0d5[2], 1.0f);
                concrete->dirty_74 = 1;
                concrete->parms_18.emissive = 0.0f;
            } else {
                concrete->parms_18.ambient.Set(source->ambient_0c9[0], source->ambient_0c9[1],
                                               source->ambient_0c9[2], 0.0f);
                concrete->dirty_74 = 1;
                concrete->parms_18.emissive.Set(source->emission_101, source->emission_101,
                                                source->emission_101, 1.0f);
            }
            concrete->dirty_74 = 1;
            concrete->m_field_78 = source->shader_flags_116;
            if ((source->shader_flags_116 & 0x1fe) != 0) {
                concrete->setMapper(&g_normal_texcoord_mapper_0065bea8);
            }
        }
    }
    (*material)->addReference();
    return 1;
}

// GLOBAL: WIZ8 0x0060e0f4
const char g_default_material_name_0060e0f4[] = "Default Material";

// FUNCTION: WIZ8 0x004B9280
unsigned char CreateDefaultMaterial(srMaterialIFace** material, srTextureIFace** texture,
                                    unsigned long* render_flags)
{
    char name[64] = "";
    srRegistry* registry;
    srRegistry::ClassNode* node;
    stMaterial* concrete;
    srVector4T<float> color;

    *render_flags = 0x0100251b;
    sprintf(name, g_default_material_name_0060e0f4);
    *texture = 0;

    registry = srCore.getRegistry();
    node = stMaterial::sGetClassNode();
    concrete =
        static_cast<stMaterial*>(registry->find(node, name, static_cast<const srRuntimeClass*>(0)));
    *material = concrete;
    if (concrete != 0) {
        return 1;
    }

    concrete = new stMaterial;
    *material = concrete;
    if (concrete == 0) {
        srAssertFail("ppstMaterial", MATERIALS_CPP, 0x130, 0);
    }
    concrete->setName(name);
    concrete->autoRelease();

    color.Set(1.0f, 1.0f, 1.0f, 1.0f);
    concrete->setAmbient(color);
    color.Set(1.0f, 1.0f, 1.0f, 1.0f);
    concrete->setDiffuse(color);
    color.Set(0.0f, 0.0f, 0.0f, 0.0f);
    concrete->setSpecular(color);
    concrete->dirty_74 = 1;
    concrete->parms_18.shininess = 1.0f;
    concrete->parms_18.diffuse.w = 1.0f;
    concrete->dirty_74 = 1;
    concrete->parms_18.emissive = 0.0f;
    concrete->dirty_74 = 1;
    concrete->m_field_78 = 0;
    return 1;
}

/* Load a texture by full path: split it into folder and file, then take the
   animated loader for .IFL names and the plain one for everything else. */
// FUNCTION: WIZ8 0x004B9460
srTextureIFace* LoadTexture004B9460(const char* path, const W8MaterialRecord004B8A70* source,
                                    unsigned char required)
{
    char drive[_MAX_PATH];
    char directory[_MAX_PATH];
    char file_name[_MAX_PATH];
    char extension[_MAX_PATH];
    char texture_folder[_MAX_PATH];
    char texture_file[_MAX_PATH];

    _splitpath(path, drive, directory, file_name, extension);
    strcpy(texture_folder, drive);
    strcat(texture_folder, directory);
    strcpy(texture_file, file_name);
    strcat(texture_file, extension);

    if (strlen(path) > 3 && _strnicmp(extension, ".IFL", 4) == 0) {
        return LoadAnimatedTexture004B98F0(texture_folder, texture_file, source, required);
    }
    return LoadTexture004B95D0(texture_folder, texture_file, required);
}

// FUNCTION: WIZ8 0x004B95D0
srTexture* LoadTexture004B95D0(const char* folder, const char* name, unsigned char required)
{
    char path[_MAX_PATH];
    char* extension;
    srRegistry* registry;
    srRegistry::ClassNode* node;
    stTextureFile* texture;

    if (g_current_screen_state.id == 4) {
        UpdatePleaseWaitLoadFrame005915A0();
    }
    strcpy(path, folder);
    strcat(path, name);
    extension = path + strlen(path) - 3;
    *extension = '\0';

    registry = srCore.getRegistry();
    node = registry->getClassNode(0x10001);
    if (node == 0) {
        node = registry->registerClass("stTextureFile", stTextureFile::sGetClassNode(), 0x10001, 0);
    }
    texture = static_cast<stTextureFile*>(
        registry->find(node, name, static_cast<const srRuntimeClass*>(0)));
    if (texture == 0) {
        strcat(extension, "tga");
        if (required != 0) {
            texture = new stTextureFile(path, g_flag_65beaf);
            if (texture == 0) {
                srAssertFail("psrTexture", MATERIALS_CPP, 0x191, 0);
            }
            if (g_gerd_659634 != 0) {
                g_gerd_659634->setTexture(texture, 0);
                g_gerd_659634->setTexture(0, 0);
            }
            if (texture->getTextureFrameHandle() == 0) {
                *extension = '\0';
                strcat(extension, "jpg");
                texture->setFileName(path);
                if (g_gerd_659634 != 0) {
                    g_gerd_659634->setTexture(texture, 0);
                    g_gerd_659634->setTexture(0, 0);
                }
                if (texture->getTextureFrameHandle() == 0) {
                    ShutdownWithErrorBox(
                        reinterpret_cast<const char*>(String("Missing texture file: %s", path)));
                }
            }
        } else {
            if (!FileExists(path)) {
                *extension = '\0';
                strcat(extension, "jpg");
                if (!FileExists(path)) {
                    ShutdownWithErrorBox(
                        reinterpret_cast<const char*>(String("Missing texture file: %s", path)));
                }
            }
            texture = new stTextureFile(path, g_flag_65beaf);
            if (texture == 0) {
                srAssertFail("psrTexture", MATERIALS_CPP, 0x1a7, 0);
            }
        }
        texture->autoRelease();
        *extension = '\0';
        texture->setName(name);
    }
    return texture;
}

// FUNCTION: WIZ8 0x004B98F0
stTextureAnim* LoadAnimatedTexture004B98F0(const char* folder, const char* name,
                                           const W8MaterialRecord004B8A70* source,
                                           unsigned char required)
{
    char buffer[_MAX_PATH];
    unsigned char more = 1;
    int handle;
    stTextureAnim* animation;

    strcpy(buffer, folder);
    strcat(buffer, name);
    handle = FileOpen(buffer, 0x41, 0);
    if (handle == 0) {
        ShutdownWithErrorBox(
            reinterpret_cast<const char*>(String("Cannot load/find material: %s", buffer)));
    }

    animation = new stTextureAnim;
    animation->autoRelease();
    animation->setName(name);
    while (more != 0) {
        ReadTextLine004CEE40(handle, buffer, 200, &more);
        if (strlen(buffer) <= 2) {
            more = 0;
        } else {
            srTexture* texture = LoadTexture004B95D0(folder, buffer, required);
            if (texture != 0) {
                animation->AddTexture00485420(texture);
            }
            if (more != 0) {
                continue;
            }
        }
        break;
    }
    animation->setupDefaultValues();
    FileClose(handle);
    if (source != 0) {
        int frame = source->animation_frame_10e;
        animation->flag_60 = source->animation_mode_10d;
        animation->value_64 = frame;
        animation->frame_58 = frame;
        animation->frame_rate_68 = source->animation_rate_112;
    }
    return animation;
}

/* getPolyTexture selects the layer and table up front, then returns one smart
   pointer per polygon. Report whether any selected texture is animated. */
// FUNCTION: WIZ8 0x004b9aa0
bool MeshHasAnimatedTexture004B9AA0(srMeshModel* model)
{
    if (model != 0) {
        srPtr<srTextureIFace>* textures = model->getPolyTexture(0, 0, 0);

        if (textures != 0) {
            int polygon;

            for (polygon = 0; polygon < model->polygon_count_230; ++polygon) {
                srTextureIFace* texture = textures[polygon].get();

                if (texture != 0 && texture->getClassID() == stTextureAnim::CLASS_ID) {
                    return true;
                }
            }
        }
    }
    return false;
}

/* The model instance's srModel::Client base supplies its mesh model. The first
   polygon texture is the shared animation object whose frame is restarted. */
// FUNCTION: WIZ8 0x004b9b00
void SetModelAnimatedTextureFrame004B9B00(srModelInstance* instance, int frame)
{
    if (instance != 0) {
        srMeshModel* model = static_cast<srMeshModel*>(instance->model());

        if (model != 0) {
            srPtr<srTextureIFace>* textures = model->getPolyTexture(0, 0, 0);

            if (textures != 0) {
                srTextureIFace* texture = textures[0].get();

                if (texture != 0 && texture->getClassID() == stTextureAnim::CLASS_ID) {
                    static_cast<stTextureAnim*>(texture)->SetFrame00485400(frame);
                }
            }
        }
    }
}

// FUNCTION: WIZ8 0x004B9B50
stTextureAnim* GetModelAnimatedTexture004B9B50(srModelInstance* instance)
{
    if (instance != 0) {
        srMeshModel* model = static_cast<srMeshModel*>(instance->model());

        if (model != 0) {
            srPtr<srTextureIFace>* textures = model->getPolyTexture(0, 0, 0);

            if (textures != 0) {
                srTextureIFace* texture = textures[0].get();

                if (texture != 0 && texture->getClassID() == stTextureAnim::CLASS_ID) {
                    return static_cast<stTextureAnim*>(texture);
                }
            }
        }
    }
    return 0;
}
