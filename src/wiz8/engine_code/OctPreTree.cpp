#include "wiz8/engine_code/OctPreTree.h"
#include "wiz8/engine_code/3d.h"
#include "wiz8/engine_code/GDProp.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/LevelFile.h"
#include "wiz8/engine_code/OctBuildPreTree.h"
#include "wiz8/engine_code/OctMeshModel.h"
#include "wiz8/engine_code/OctPath.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/materials.h"
#include "wiz8/float_constants.h"
#include "wiz8/vector.h"

#include "FileMan.h"
#include "DEBUG.H"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OCTPRETREE_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\OctPreTree.cpp"

static char PropFramesDiffer0046C6A0(W8LevelFileAnimObj* anim, unsigned short first,
                                     unsigned short last);

/* Same 50.0f descent step Octree.cpp emits at 0x005EC02C; the linker folds the
   identical constants into one address. */
static const float NAVIGATOR_MINIMUM_HORIZONTAL_DISTANCE = 50.0f;
// GLOBAL: WIZ8 0x005ebc28
float g_float_005ebc28 = 5.0f;
// GLOBAL: WIZ8 0x005ebc70
double g_double_005ebc70 = 0.0001;
// GLOBAL: WIZ8 0x005ebc90
float g_float_005ebc90 = 9.999999747378752e-05f;
// GLOBAL: WIZ8 0x005ec410
float g_float_005ec410 = 0.3333333432674408f;
// GLOBAL: WIZ8 0x005ec414
float g_float_005ec414 = 0.9998999834060669f;

// GLOBAL: WIZ8 0x00659c74
OctPreTree* g_oct_pre_tree_659c74 = 0;

/* Paired item/key sorts shared through stHash.hpp; these emissions are this
   file's unsigned long and unsigned short instantiations. */

// TEMPLATE: WIZ8 0x00467640
// QuickSortByKey<unsigned long>

// TEMPLATE: WIZ8 0x004677a0
// QuickSortByKey<unsigned short>

/* The build-time runtime tree extends the ordinary 0x29c octree with transfer
   bookkeeping and one separately owned pointer vector.  Its only recovered
   construction caller allocates exactly 0x3bc bytes. */
// FUNCTION: WIZ8 0x004679e0
OctPreTree::OctPreTree() : W8Octree(0, 0)
{
    game_data_3a4 = 0;
    padding_3a8 = 0;
    padding_3ac = 0;
    deepest_link_list_3b0 = 0;
    m_region_cell_178 = 0.0f;
    path_node_extent_3b4 = 0;
    automesh_cells_29c = 0;
    pre_pathing_2a0 = 0;
    props_3b8 = new W8GrowableVector<GDProp*>;
    g_oct_pre_tree_659c74 = this;
}

/* Tears down the automesh cell map, the owned pre-pathing service and the
   registered-prop vector, then clears the global before the octree base. */
// FUNCTION: WIZ8 0x00467ab0
OctPreTree::~OctPreTree()
{
    if (automesh_cells_29c != 0) {
        delete automesh_cells_29c;
        automesh_cells_29c = 0;
    }
    if (pre_pathing_2a0 != 0) {
        delete pre_pathing_2a0;
        pre_pathing_2a0 = 0;
    }
    if (props_3b8 != 0) {
        delete props_3b8;
    }
    g_oct_pre_tree_659c74 = 0;
}

/* Copies the working bounds straight into the spatial state; the build tree
   conversion calls it once its clipped bounds are known. */
// FUNCTION: WIZ8 0x00467b70
void W8OctSpatialState::SetWorkingBounds00467B70(const srVector3T<float>* minimum,
                                                 const srVector3T<float>* maximum)
{
    working_minimum_78 = *minimum;
    working_maximum_84 = *maximum;
}

/* Resets the collected-id run and appends every not-yet-seen polygon id the
   leaf under `cell` lists.  The trace walk inlines this sequence at each of
   the six cells it probes. */
inline void OctPreTree::CollectLeafPolygons(const int* cell)
{
    m_gd_result_count_1b8 = 0;
    unsigned int leaf_index = LeafIndexForCell(cell);
    if (leaf_index != 0 && m_owned_0a0[leaf_index].polygon_offset_08 != 0) {
        const unsigned long* stream = m_owned_0d0 + m_owned_0a0[leaf_index].polygon_offset_08;
        for (int remaining = *stream; remaining != 0; --remaining) {
            ++stream;
            if (m_owned_190->Set(*stream) == 0) {
                m_aulGDObjs[m_gd_result_count_1b8] = *stream;
                ++m_gd_result_count_1b8;
            }
        }
    }
}

/* Walks the `from`-`to` segment through the leaf grid, collecting each
   visited leaf's region-polygon ids and plane/slab-testing them.  Answers
   whether the segment is unobstructed; the light-visibility callers
   accumulate its result. */
// FUNCTION: WIZ8 0x00467bb0
bool OctPreTree::SegmentClear00467BB0(const srVector3T<float>* from, const srVector3T<float>* to)
{
    W8OctreeTrace trace;
    W8OctreeWalk walk;
    int cell[3];
    int end_cell[3];
    int span = 0;
    unsigned char blocked = 0;

    trace.Seed(from, to);
    m_gd_result_count_1b8 = 0;
    m_owned_190->ClearAll();
    for (int axis = 0; axis < 3; ++axis) {
        cell[axis] = static_cast<int>(((&from->x)[axis] - (&spatial_000.minimum_0c.x)[axis]) /
                                      spatial_000.node_extent_70);
        end_cell[axis] = static_cast<int>(((&to->x)[axis] - (&spatial_000.minimum_0c.x)[axis]) /
                                          spatial_000.node_extent_70);
        int difference = cell[axis] - end_cell[axis];
        if (difference < 0) {
            span -= difference;
        } else {
            span += difference;
        }
    }
    if (span < 2) {
        CollectLeafPolygons(cell);
        blocked = TestCollectedPolygons004681E0(&trace);
        if (!blocked && span != 0) {
            CollectLeafPolygons(end_cell);
            blocked = TestCollectedPolygons004681E0(&trace);
        }
    } else {
        BuildCellWalk(*from, *to, &walk);
        int error_a = walk.error_2c;
        int error_b = walk.error_38;
        for (int index = 0; index < walk.count_24; ++index) {
            if (blocked != 0) {
                break;
            }
            CollectLeafPolygons(cell);
            if (m_gd_result_count_1b8 != 0) {
                blocked = TestCollectedPolygons004681E0(&trace);
            }
            if (error_a < error_b) {
                if (error_a < 0 && !blocked) {
                    cell[walk.minor_axis_1c] += walk.step_0c[walk.minor_axis_1c];
                    error_a += walk.error_reset_30;
                    CollectLeafPolygons(cell);
                    if (m_gd_result_count_1b8 != 0) {
                        blocked = TestCollectedPolygons004681E0(&trace);
                    }
                    if (error_b < 0 && !blocked) {
                        cell[walk.minor_axis_20] += walk.step_0c[walk.minor_axis_20];
                        error_b += walk.error_reset_3c;
                        CollectLeafPolygons(cell);
                        if (m_gd_result_count_1b8 != 0) {
                            blocked = TestCollectedPolygons004681E0(&trace);
                        }
                    }
                }
            } else {
                if (error_b < 0 && !blocked) {
                    cell[walk.minor_axis_20] += walk.step_0c[walk.minor_axis_20];
                    error_b += walk.error_reset_3c;
                    CollectLeafPolygons(cell);
                    if (m_gd_result_count_1b8 != 0) {
                        blocked = TestCollectedPolygons004681E0(&trace);
                    }
                    if (error_a < 0 && !blocked) {
                        cell[walk.minor_axis_1c] += walk.step_0c[walk.minor_axis_1c];
                        error_a += walk.error_reset_30;
                        CollectLeafPolygons(cell);
                        if (m_gd_result_count_1b8 != 0) {
                            blocked = TestCollectedPolygons004681E0(&trace);
                        }
                    }
                }
            }
            cell[walk.major_axis_18] += walk.step_0c[walk.major_axis_18];
            error_a -= walk.error_delta_28;
            error_b -= walk.error_delta_34;
        }
    }
    return blocked == 0;
}

/* Tests the collected region polygons' planes against the trace segment.  A
   polygon blocks only when its plane faces the ray, the crossing lies inside
   the segment's `length_28 - 1.0f` window, and either the start point sits
   within 1.0f of the plane or the ray exits at least 5.0f behind it, with the
   resulting contact point landing inside the polygon. */
// FUNCTION: WIZ8 0x004681e0
bool OctPreTree::TestCollectedPolygons004681E0(W8OctreeTrace* trace)
{
    float limit = trace->length_28 - g_float_005ebb38;
    unsigned char blocked = 0;

    for (unsigned long index = 0; index < m_gd_result_count_1b8; ++index) {
        if (blocked != 0) {
            break;
        }
        W8OctRegionPolygon* polygon = &game_data_3a4->polygons_0c[m_aulGDObjs[index]];
        const float* plane = &polygon->plane_08.normal.x;
        if (plane[0] * trace->step_18.x + trace->step_18.y * plane[1] +
                trace->step_18.z * plane[2] <=
            g_float_005ebb34) {
            float front = trace->start_00.x * plane[0] + trace->start_00.y * plane[1] +
                          trace->start_00.z * plane[2] + plane[3];
            if (front <= limit && g_float_005ebb34 < front) {
                srVector3T<float> contact;
                if (g_float_005ebb38 <= front) {
                    float back = trace->end_0c.x * plane[0] + trace->end_0c.y * plane[1] +
                                 trace->end_0c.z * plane[2] + plane[3];
                    if (g_float_005ebc28 <= back) {
                        continue;
                    }
                    back = -back;
                    if (back < g_float_005ebc28) {
                        continue;
                    }
                    front = front / (back + front) * trace->length_28;
                    contact.x = trace->step_18.x * front + trace->start_00.x;
                    contact.y = trace->step_18.y * front + trace->start_00.y;
                    contact.z = trace->step_18.z * front + trace->start_00.z;
                } else {
                    contact = trace->start_00;
                }
                srVector3T<float> vertices[3];
                vertices[0] = polygon->vertices_34[0]->position_0c;
                vertices[1] = polygon->vertices_34[1]->position_0c;
                vertices[2] = polygon->vertices_34[2]->position_0c;
                if (PointInsideTriangle0046D530(vertices, polygon->flags_00 & 3, &contact) != 0) {
                    blocked = 1;
                }
            }
        }
    }
    return blocked != 0;
}

/* Serializes the finished octree to NewLevel.oct: the 0xf5-byte header, then
   the branch/leaf arrays, polygon streams, uiLeaf grid, lookup and region
   tables, the submesh index, optional alpha/mesh-link tables, path nodes,
   prop sunlight bits and finally the game-data block.  The header's +0xb4
   dword and the +0xc9..+0xf4 tail stay unwritten, exactly as retail leaves
   them. */
// FUNCTION: WIZ8 0x004683f0
unsigned char OctPreTree::WriteOctFile004683F0(W8OctPreTreeGeometry* geometry,
                                               W8GameData* game_data)
{
    unsigned char written;
    int file;
    unsigned long sentinel = 0xffffffff;
    W8OctFileHeader header;

    if (m_leaf_count_0b8 != 0) {
        for (unsigned long index = 0; index < m_leaf_count_0b8; ++index) {
            m_owned_0a0[index].flags_00 &= 0xfffffffe;
        }
    }
    header.extent_02 = spatial_000.extent_04;
    header.cell_size_06 = spatial_000.cell_size_08;
    header.node_extent_0a = spatial_000.node_extent_70;
    header.version_00 = 0x22;
    for (int axis = 0; axis < 3; ++axis) {
        (&header.bounds_0e[0].x)[axis] = (&spatial_000.minimum_0c.x)[axis];
        (&header.bounds_0e[1].x)[axis] = (&spatial_000.maximum_18.x)[axis];
        (&header.bounds_0e[2].x)[axis] = (&spatial_000.clipped_minimum_24.x)[axis];
        (&header.bounds_0e[3].x)[axis] = (&spatial_000.clipped_maximum_30.x)[axis];
        (&header.bounds_0e[4].x)[axis] = (&spatial_000.working_minimum_78.x)[axis];
        (&header.bounds_0e[5].x)[axis] = (&spatial_000.working_maximum_84.x)[axis];
        header.grid_dims_56[axis] = (&m_leaf_grid_dim_x_0a4)[axis];
    }
    header.depth_62 = spatial_000.depth_44;
    header.region_id_bound_64 = spatial_000.region_id_bound_58;
    header.root_mesh_count_9a = m_root_mesh_count_1a8;
    header.submesh_count_66 = spatial_000.submesh_count_74;
    header.branch_count_6a = m_branch_count_0b4;
    header.region_count_96 = spatial_000.region_count_46;
    header.leaf_level_98 = spatial_000.leaf_level_52;
    header.mesh_total_9e = m_meshCount_1b4;
    header.leaf_count_6e = m_leaf_count_0b8;
    header.polygon_count_72 = geometry->polygon_count_08;
    header.vertex_count_76 = geometry->vertex_count_00;
    header.surface_count_7a = game_data->m_iNumSurfaces;
    header.gd_surface_stream_len_86 = m_gd_surface_stream_len_124;
    header.leaf_polygon_stream_len_82 = polygon_cursor_3a0;
    header.trigger_count_8a = m_trigger_count_128;
    header.region_list_len_92 = m_region_list_len_138;
    header.region_cell_ac = m_region_cell_178;
    header.max_region_radius_b9 = spatial_000.max_region_radius_60;
    header.particle_len_c5 = m_usMeshParticlesLen_0e8;
    header.region_grid_cell_a6 = spatial_000.region_grid_cell_54;
    header.prop_len_c7 = m_usMeshPropsLen_0f4;
    header.prop_count_bd = m_ulNumProps;
    header.prop_sun_bits_b8 = m_pPropSunBits != 0;
    header.particle_count_c1 = m_ulNumParticles;
    header.kind1_submesh_count_a2 = m_kind1_submesh_count_1ac;
    header.zero_8e = 0;
    if (pre_pathing_2a0 == 0) {
        header.path_nodes_7e = 0;
        header.edge_node_count_b0 = 0;
    } else {
        header.path_nodes_7e = pre_pathing_2a0->size_004;
        header.edge_node_count_b0 = pre_pathing_2a0->edge_node_count_008;
    }
    file = FileOpen("NewLevel.oct", FILE_ACCESS_WRITE | FILE_CREATE_ALWAYS, 0);
    if (file == 0) {
        ReportBuildStatus00497690(7, "WriteOctFile: Couldn't create file.\n");
        return 0;
    }
    /* Every write-failure path below returns without FileClose: retail leaks
       the handle on each of them (verified at 0x4686b4 et seq.). */
    if (FileWrite(file, &header, 0xf5, 0) == 0) {
        ReportBuildStatus00497690(7, "WriteOctFile: Couldn't write tree info.\n");
        return 0;
    }
    FileWrite(file, &sentinel, 4, 0);
    if (FileWrite(file, m_owned_09c, header.branch_count_6a * 0x24, 0) == 0) {
        ReportBuildStatus00497690(7, "WriteOctFile: Couldn't write Node info.\n");
        return 0;
    }
    if (FileWrite(file, m_owned_0a0, header.leaf_count_6e * 0x28, 0) == 0) {
        ReportBuildStatus00497690(7, "WriteOctFile: Couldn't write Leaves info.\n");
        return 0;
    }
    if (FileWrite(file, m_owned_0d0, header.leaf_polygon_stream_len_82 * 4, 0) == 0) {
        ReportBuildStatus00497690(7, "WriteOctFile: Couldn't write Poly List info.\n");
        return 0;
    }
    unsigned int grid_cells = m_leaf_grid_dim_z_0ac * m_leaf_grid_dim_y_0a8 * m_leaf_grid_dim_x_0a4;
    if (grid_cells < 250000 && FileWrite(file, m_owned_0b0, grid_cells * 4, 0) == 0) {
        ReportBuildStatus00497690(7, "WriteOctFile: Couldn't write uiLeafGrid info.\n");
        return 0;
    }
    if (FileWrite(file, m_aulPolyLookup, header.polygon_count_72 * 4, 0) == 0) {
        ReportBuildStatus00497690(7, "WriteOctFile: Couldn't write Poly Lookup table.\n");
        return 0;
    }
    if (header.region_list_len_92 != 0 &&
        FileWrite(file, m_owned_148, header.region_list_len_92 * 2, 0) == 0) {
        ReportBuildStatus00497690(7, "WriteOctFile: Couldn't write region list.\n");
        return 0;
    }
    if (header.gd_surface_stream_len_86 != 0 &&
        FileWrite(file, m_owned_12c, header.gd_surface_stream_len_86 * 4, 0) == 0) {
        ReportBuildStatus00497690(7, "WriteOctFile: Couldn't write Game Data Poly List.\n");
        return 0;
    }
    /* Retail writes this trigger list as 4-byte elements while ReadOctFile
       reads it back as 2-byte elements (0x4688c3 vs 0x42c351): the asymmetry
       is authentic.  In practice the count is always zero. */
    if (header.trigger_count_8a != 0 &&
        FileWrite(file, m_owned_130, header.trigger_count_8a * 4, 0) == 0) {
        ReportBuildStatus00497690(7, "WriteOctFile: Couldn't write Trigger list.\n");
        return 0;
    }
    if (header.region_count_96 > 1 &&
        FileWrite(file, spatial_000.owned_5c, header.region_count_96 * 0xe8, 0) == 0) {
        ReportBuildStatus00497690(7, "WriteOctFile: Couldn't write Region array.\n");
        return 0;
    }
    FileWrite(file, &sentinel, 4, 0);
    if (header.submesh_count_66 != 0) {
        if (FileWrite(file, m_pSubmeshes, (header.submesh_count_66 + 1) * 0x10, 0) == 0) {
            ReportBuildStatus00497690(7, "WriteOctFile: Couldn't write submesh array.\n");
            return 0;
        }
        if (m_meshCount_1b4 != 0 && m_pAlphaBits != 0 && m_pAlphaBits->Save(file) == 0) {
            srAssertFail("m_pAlphaBits->Save(hOctFile)", OCTPRETREE_CPP, 0x224,
                         "ReadOctFile: Failure writing Alpha Bits.");
        }
        if (m_ulNumParticles != 0) {
            if (FileWrite(file, m_pusMeshParticleLookup, header.mesh_total_9e * 2 + 2, 0) == 0) {
                ReportBuildStatus00497690(
                    7, "WriteOctFile: Couldn't write Mesh Particle Lookup Table.\n");
                return 0;
            }
            if (FileWrite(file, m_pusMeshParticles,
                          static_cast<unsigned int>(m_usMeshParticlesLen_0e8) << 1, 0) == 0) {
                ReportBuildStatus00497690(
                    7, "WriteOctFile: Couldn't write Mesh Particle Link Table.\n");
                return 0;
            }
        }
        if (m_ulNumProps != 0) {
            if (FileWrite(file, m_pusMeshPropLookup, header.mesh_total_9e * 2 + 2, 0) == 0) {
                ReportBuildStatus00497690(7,
                                          "WriteOctFile: Couldn't write Mesh Prop Lookup Table.\n");
                return 0;
            }
            if (FileWrite(file, m_pusMeshProps,
                          static_cast<unsigned int>(m_usMeshPropsLen_0f4) << 1, 0) == 0) {
                ReportBuildStatus00497690(7,
                                          "WriteOctFile: Couldn't write Mesh Prop Link Table.\n");
                return 0;
            }
        }
    }
    /* Every section terminator is the same 0xffffffff dword: retail keeps one
       -1 local for all of them (verified at 0x468406). */
    if (FileWrite(file, &sentinel, 4, 0) == 0) {
        ReportBuildStatus00497690(
            7, "WriteOctFile: Couldn't write Terminator after Mesh Prop Link Table.\n");
        return 0;
    }
    if (pre_pathing_2a0 != 0 && pre_pathing_2a0->WritePathNodes00458AD0(file) == 0) {
        ReportBuildStatus00497690(7, "WriteOctFile: Couldn't write Path Nodes.\n");
        return 0;
    }
    if (m_ulNumProps != 0 && m_pPropSunBits != 0 && m_pPropSunBits->Save(file) == 0) {
        ReportBuildStatus00497690(7, "WriteOctFile: Couldn't write Prop Sunlight bits array.\n");
        return 0;
    }
    written = FileWrite(file, &sentinel, 4, 0);
    if (written == 0) {
        ReportBuildStatus00497690(7, "WriteOctFile: Couldn't write Terminator field.\n");
        return 0;
    }
    /* game_data was already dereferenced unconditionally filling the header
       above (verified at 0x468572); this null check is authentic but
       unreachable-with-null. */
    if (game_data != 0 && header.gd_surface_stream_len_86 != 0) {
        written = game_data->WriteGameData0044AA40(file);
        /* Retail bitwise-ORs the game-data and terminator results: a failed
           game-data write followed by a successful four-byte write reports
           success.  Verified at 0x468be0-0x468bec. */
        written |= FileWrite(file, &sentinel, 4, 0);
        if (written == 0) {
            ReportBuildStatus00497690(7, "WriteOctFile: Couldn't write final Terminator field.\n");
            return 0;
        }
    }
    FileClose(file);
    return written;
}

/* Releases the two per-record id runs SplitMeshes allocates; retail inlines
   this same loop at every CreateSubMeshes exit. */
static void FreeSubmeshBuildArrays(W8OctSubmeshBuild* records, unsigned long count)
{
    for (unsigned int index = 0; index < count; ++index) {
        if (records[index].vertex_ids_20 != 0) {
            free(records[index].vertex_ids_20);
        }
        if (records[index].polygon_ids_24 != 0) {
            free(records[index].polygon_ids_24);
        }
    }
}

/* Builds the OctMeshModel array from the split records: partitions the
   geometry through AllocateSubMesh/SplitMeshes, transfers the per-submesh
   vertex/polygon data, counts the per-kind totals and marks the alpha-bit
   rows whose packed header is nonzero. */
// FUNCTION: WIZ8 0x00468c30
OctMeshModel* OctPreTree::CreateSubMeshes00468C30(W8OctPreTreeGeometry* geometry)
{
    OctMeshModel* models;
    W8OctSubmeshBuild* records;
    unsigned int kind_counts[4];
    unsigned int root_count;
    unsigned int record_index;
    unsigned int model_index;

    if (spatial_000.submesh_count_74 == 0) {
        return 0;
    }
    m_vertex_count_0c8 = geometry->vertex_count_00;
    VerifyPolygonRegions0046ABF0();
    /* 0x9c = three 0x34-byte build records per original unit.  SplitMeshes
       appends at most one record per source on each of its three kind
       passes while an emptied source keeps its slot, so the true slot
       bound is count + 3*(count-1) - covered only while count <= 6. */
    records = static_cast<W8OctSubmeshBuild*>(malloc((spatial_000.submesh_count_74 + 1) * 0x9c));
    if (records == 0) {
        ReportBuildStatus00497690(7, "\nCreateSubMeshes: Could not allocate submeshes.\n");
    } else {
        memset(records, 0, (spatial_000.submesh_count_74 + 1) * 0x9c);
        AllocateSubMesh0046A790(records);
        SplitMeshes00469670(geometry, records);
        m_aulPolyLookup = static_cast<unsigned long*>(malloc(geometry->polygon_count_08 * 4 + 4));
        if (m_aulPolyLookup == 0) {
            ReportBuildStatus00497690(7,
                                      "\nCreateSubMeshes: Could not allocate m_aulPolyLookup.\n");
            FreeSubmeshBuildArrays(records, spatial_000.submesh_count_74);
        } else {
            m_pSubmeshes =
                static_cast<W8OctSubmesh*>(malloc((spatial_000.submesh_count_74 + 1) * 0x10));
            models = new OctMeshModel[spatial_000.submesh_count_74 + 1];
            if (m_pSubmeshes != 0 && models != 0) {
                memset(m_pSubmeshes, 0, (spatial_000.submesh_count_74 + 1) * 0x10);
                root_count = 0;
                kind_counts[3] = 0;
                kind_counts[2] = 0;
                kind_counts[1] = 0;
                kind_counts[0] = 0;
                record_index = 1;
                model_index = 0;
                if (spatial_000.submesh_count_74 > 1) {
                    W8OctSubmeshBuild* record = records + 1;
                    W8OctSubmesh* submesh = m_pSubmeshes + 1;
                    /* Retail writes models from index 0: records[1..] and
                       submeshes[1..] are one-based, but the model cursor
                       starts at models[0]. */
                    OctMeshModel* model = models;
                    do {
                        if (record->polygon_count_1c == 0) {
                            ReportBuildStatus00497690(
                                7, "CreateSubMeshes: Found mesh with no polys.\n");
                            FreeSubmeshBuildArrays(records, spatial_000.submesh_count_74);
                            goto cleanup;
                        }
                        submesh->mesh_04 = model_index;
                        submesh->polygon_count_0c = record->polygon_count_1c;
                        submesh->next_link_08 = record->next_link_10;
                        model->packed_header_3c = record->kind_08;
                        model->version_00 = m_sun_count_296;
                        kind_counts[record->kind_08] += 1;
                        model->vertex_count_40 = record->vertex_count_14;
                        model->map_count_10 = record->map_count_18;
                        model->polygon_count_44 = record->polygon_count_1c;
                        model->link_index_04 = record->prev_link_0c - 1;
                        if (record->prev_link_0c == 0) {
                            ++root_count;
                        }
                        model->next_link_08 = record->next_link_10 - 1;
                        model->vertex_locations_14 = static_cast<srVector3T<float>*>(
                            srHeap.allocate(record->vertex_count_14 * 0xc));
                        model->vertex_map_18 = record->uv_map_30;
                        model->poly_vertices_20 = record->poly_vertices_28;
                        model->poly_uv_index_24 = record->poly_uv_index_2c;
                        model->poly_equations_34 = static_cast<srVector4T<float>*>(
                            srHeap.allocate(record->polygon_count_1c << 4));
                        model->vertex_normals_2c = static_cast<srVector3T<float>*>(
                            srHeap.allocate(record->vertex_count_14 * 0xc));
                        model->vertex_lights_30 = static_cast<srVector3T<float>*>(
                            srHeap.allocate(record->vertex_count_14 * 0xc));
                        model->vertex_materials_1c =
                            static_cast<int*>(malloc(record->vertex_count_14 << 2));
                        model->poly_textures_28 =
                            static_cast<int*>(malloc(record->polygon_count_1c << 2));
                        if (m_sun_count_296 != 0) {
                            model->sun_lights_38 = static_cast<float**>(
                                malloc(static_cast<int>(m_sun_count_296) << 2));
                            if (model->sun_lights_38 == 0) {
                                ReportBuildStatus00497690(7,
                                                          "\nCreateSubMeshes: Could not allocate "
                                                          "ppsrSunLights.\n");
                                FreeSubmeshBuildArrays(records, spatial_000.submesh_count_74);
                                goto cleanup;
                            }
                            for (short sun = 0; sun < static_cast<short>(m_sun_count_296); ++sun) {
                                model->sun_lights_38[sun] =
                                    static_cast<float*>(malloc(record->vertex_count_14 << 2));
                                if (model->sun_lights_38[sun] == 0) {
                                    ReportBuildStatus00497690(
                                        7, "\nCreateSubMeshes: Could not allocate "
                                           "ppsrSunLights array.\n");
                                    FreeSubmeshBuildArrays(records, spatial_000.submesh_count_74);
                                    goto cleanup;
                                }
                            }
                        }
                        if (model->vertex_locations_14 == 0 || model->vertex_map_18 == 0 ||
                            model->poly_vertices_20 == 0 || model->poly_equations_34 == 0 ||
                            model->vertex_normals_2c == 0 || model->vertex_lights_30 == 0 ||
                            model->vertex_materials_1c == 0 || model->poly_textures_28 == 0) {
                            ReportBuildStatus00497690(7,
                                                      "\nCreateSubMeshes: Could not allocate mesh "
                                                      "model arrays.\n");
                            FreeSubmeshBuildArrays(records, spatial_000.submesh_count_74);
                            goto cleanup;
                        }
                        for (unsigned long polygon = 0; polygon < record->polygon_count_1c;
                             ++polygon) {
                            unsigned long id = record->polygon_ids_24[polygon];
                            m_aulPolyLookup[id] = model_index * 0x10000 + polygon;
                            model->poly_textures_28[polygon] = geometry->polygons_0c[id].texture_28;
                            model->poly_equations_34[polygon].x =
                                geometry->polygons_0c[id].plane_08.normal.x;
                            model->poly_equations_34[polygon].y =
                                geometry->polygons_0c[id].plane_08.normal.y;
                            model->poly_equations_34[polygon].z =
                                geometry->polygons_0c[id].plane_08.normal.z;
                            model->poly_equations_34[polygon].w =
                                geometry->polygons_0c[id].plane_08.w;
                        }
                        model->material_index_0c =
                            geometry->vertices_04[record->vertex_ids_20[0]].material_1c;
                        for (unsigned long vertex = 0; vertex < record->vertex_count_14; ++vertex) {
                            W8OctPreTreeVertex* source =
                                &geometry->vertices_04[record->vertex_ids_20[vertex]];
                            model->vertex_locations_14[vertex] = source->position_0c;
                            model->vertex_normals_2c[vertex] = source->normal_24;
                            model->vertex_lights_30[vertex] = source->light_30;
                            model->vertex_materials_1c[vertex] = source->material_1c;
                            for (short sun = 0; sun < static_cast<short>(m_sun_count_296); ++sun) {
                                model->sun_lights_38[sun][vertex] = source->sun_lights_3c[sun];
                            }
                            if (model->vertex_materials_1c[vertex] != model->material_index_0c) {
                                model->material_index_0c = -1;
                            }
                        }
                        ++record;
                        ++submesh;
                        ++model;
                        ++model_index;
                        ++record_index;
                    } while (record_index < spatial_000.submesh_count_74);
                }
                spatial_000.submesh_count_74 = model_index;
                m_meshCount_1b4 = root_count;
                m_root_mesh_count_1a8 = kind_counts[0];
                m_kind1_submesh_count_1ac = kind_counts[1];
                m_pAlphaBits = new BitArray(m_meshCount_1b4);
                if (root_count > 1) {
                    for (unsigned int index = 1; index < root_count; ++index) {
                        if (models[index].packed_header_3c != 0) {
                            m_pAlphaBits->Set(index);
                        }
                    }
                }
                /* Verified retail behavior: submesh_count_74 was just overwritten
                   with model_index, so the cleanup frees records[0..model_index)
                   and leaks the final record's vertex/polygon id arrays. */
                FreeSubmeshBuildArrays(records, spatial_000.submesh_count_74);
                free(records);
                return models;
            }
            ReportBuildStatus00497690(7, "\nCreateSubMeshes: Could not allocate mesh arrays.\n");
            FreeSubmeshBuildArrays(records, spatial_000.submesh_count_74);
        }
    cleanup:
        free(records);
    }
    return 0;
}

/* Second CreateSubMeshes phase: partitions every non-empty automesh record's
   polygon list by polygon kind (1..3) into new records appended through the
   +0x0c/+0x10 link chain, compacts the record array preserving chain order,
   then rebuilds each record's deduplicated vertex id list (sorted by
   material), its corner-to-vertex map (sorted by texture) and its UV map via
   SplitUVMaps.  Retail leaks the five sort arrays on the allocation-failure
   paths; kept as-is. */
// FUNCTION: WIZ8 0x00469670
unsigned long OctPreTree::SplitMeshes00469670(W8OctPreTreeGeometry* geometry,
                                              W8OctSubmeshBuild* records)
{
    unsigned long count = spatial_000.submesh_count_74;
    if (count == 0) {
        return 0;
    }

    unsigned long max_polygons = 0;
    if (count > 1) {
        for (unsigned long index = 1; index < count; ++index) {
            if (max_polygons < records[index].polygon_count_1c) {
                max_polygons = records[index].polygon_count_1c;
            }
        }
    }
    unsigned long* staging = static_cast<unsigned long*>(malloc((max_polygons + 5) * 4));
    int* vertex_ids = static_cast<int*>(malloc((max_polygons + 5) * 0xc));
    unsigned long* keys = static_cast<unsigned long*>(malloc((max_polygons + 5) * 0xc));
    unsigned long* scratch = static_cast<unsigned long*>(malloc((max_polygons + 5) * 0xc));
    unsigned long* order = static_cast<unsigned long*>(malloc((max_polygons + 5) * 0xc));
    if (staging == 0 || vertex_ids == 0 || keys == 0 || scratch == 0 || order == 0) {
        ReportBuildStatus00497690(7, "SplitMeshes: Error in allocating sort arrays.\n");
        return 0;
    }

    m_vertex_count_0c8 = geometry->vertex_count_00;
    count = spatial_000.submesh_count_74;
    m_meshCount_1b4 = count;
    m_root_mesh_count_1a8 = count;
    unsigned long kind_counts[4];
    kind_counts[0] = count;
    kind_counts[1] = 0;
    kind_counts[2] = 0;
    /* Verified retail behavior: kind_counts[3] is incremented (by
       kind == 3 targets) but never initialised or read - the slot stays a
       dead stack increment under MSVC6. Deliberately left uninitialised. */

    unsigned long next_free = count;
    for (unsigned long kind = 1; kind < 4; ++kind) {
        for (unsigned long index = 1; index < count; ++index) {
            W8OctSubmeshBuild* record = records + index;
            W8OctSubmeshBuild* target = records + next_free;
            if (record->polygon_count_1c == 0) {
                if (kind == 1) {
                    --m_root_mesh_count_1a8;
                }
                continue;
            }
            record->index_04 = index;
            record->kind_08 = 0;
            record->prev_link_0c = 0;
            for (unsigned long face = 0; face < record->polygon_count_1c; ++face) {
                unsigned long id = record->polygon_ids_24[face];
                if (geometry->polygons_0c[id].kind_2c == kind) {
                    staging[target->polygon_count_1c] = id;
                    ++target->polygon_count_1c;
                    record->polygon_ids_24[face] = 0;
                }
            }
            if (target->polygon_count_1c != 0) {
                target->polygon_ids_24 =
                    static_cast<int*>(malloc(target->polygon_count_1c * 4 + 4));
                if (target->polygon_ids_24 == 0) {
                    ReportBuildStatus00497690(
                        7, "SplitMeshes: Could not allocate pMeshes[uiFinal].aulFaces.\n");
                    return 0;
                }
                memcpy(target->polygon_ids_24, staging, target->polygon_count_1c * 4);
                target->polygon_ids_24[target->polygon_count_1c] = 0;
                target->kind_08 = kind;
                unsigned long tail = index;
                unsigned long link = record->next_link_10;
                while (link != 0) {
                    tail = link;
                    link = records[tail].next_link_10;
                }
                records[tail].next_link_10 = next_free;
                target->prev_link_0c = tail;
            }
            unsigned long kept = 0;
            for (unsigned long i = 0; i < record->polygon_count_1c; ++i) {
                int id = record->polygon_ids_24[i];
                if (id != 0) {
                    record->polygon_ids_24[kept] = id;
                    ++kept;
                }
            }
            record->polygon_count_1c = kept;
            if (kept == 0) {
                --m_root_mesh_count_1a8;
            }
            if (target->polygon_count_1c != 0) {
                ++next_free;
                ++kind_counts[kind];
            }
        }
    }

    if (spatial_000.submesh_count_74 == m_root_mesh_count_1a8) {
        spatial_000.submesh_count_74 = next_free;
    } else {
        spatial_000.submesh_count_74 = next_free;
        unsigned long new_index = 1;
        if (next_free > 1) {
            for (unsigned long index = 1; index < next_free; ++index) {
                unsigned long slot = index;
                while (slot != 0) {
                    W8OctSubmeshBuild* candidate = records + slot;
                    if (candidate->polygon_count_1c == 0) {
                        --kind_counts[candidate->kind_08];
                        --spatial_000.submesh_count_74;
                        slot = candidate->next_link_10;
                        candidate = records + slot;
                        candidate->flags_00 |= 1;
                        if (candidate->prev_link_0c != 0) {
                            candidate->prev_link_0c = records[candidate->prev_link_0c].prev_link_0c;
                        }
                        continue;
                    }
                    if (slot != index || (candidate->flags_00 & 1) == 0) {
                        if (slot > 1) {
                            for (unsigned long i = 1; i < slot; ++i) {
                                if (records[i].next_link_10 == slot) {
                                    records[i].next_link_10 = new_index;
                                }
                            }
                        }
                        if (slot + 1 < next_free) {
                            for (unsigned long i = slot + 1; i < next_free; ++i) {
                                if (records[i].prev_link_0c == slot) {
                                    records[i].prev_link_0c = new_index;
                                }
                            }
                        }
                        /* The copy lands at the new_index cursor, not at
                           index - when a chain produces no live record (all
                           emptied, or a promoted record already in place)
                           index runs ahead of new_index. */
                        records[new_index] = records[slot];
                        ++new_index;
                    }
                    break;
                }
            }
        }
    }

    unsigned long total_maps = 0;
    unsigned long total_vertices = 0;
    if (spatial_000.submesh_count_74 > 1) {
        for (unsigned long index = 1; index < spatial_000.submesh_count_74; ++index) {
            W8OctSubmeshBuild* record = records + index;
            record->poly_vertices_28 =
                static_cast<srVector3i*>(srHeap.allocate(record->polygon_count_1c * 0xc));
            if (record->poly_vertices_28 == 0) {
                ReportBuildStatus00497690(7, "SplitMeshes: Could not allocate psrPolyVertex.\n");
                return 0;
            }
            unsigned long vertex_count = 0;
            for (unsigned long poly = 0; poly < record->polygon_count_1c; ++poly) {
                unsigned long id = record->polygon_ids_24[poly];
                unsigned long texture = geometry->polygons_0c[id].texture_28;
                scratch[poly] = texture;
                keys[poly] = texture;
                for (unsigned long corner = 0; corner < 3; ++corner) {
                    unsigned long vertex_id =
                        geometry->polygons_0c[id].vertices_34[corner]->vertex_index_04;
                    unsigned long slot = 0;
                    while (slot < vertex_count && vertex_ids[slot] != static_cast<int>(vertex_id)) {
                        ++slot;
                    }
                    if (slot == vertex_count) {
                        vertex_ids[vertex_count] = vertex_id;
                    }
                    (&record->poly_vertices_28[poly].x)[corner] = slot;
                    if (slot == vertex_count) {
                        ++vertex_count;
                    }
                }
            }
            record->vertex_count_14 = vertex_count;

            QuickSortByKey(record->polygon_ids_24, keys, 0,
                           static_cast<int>(record->polygon_count_1c) - 1);
            QuickSortByKey(record->poly_vertices_28, scratch, 0,
                           static_cast<int>(record->polygon_count_1c) - 1);

            for (unsigned long i = 0; i < vertex_count; ++i) {
                unsigned long material = geometry->vertices_04[vertex_ids[i]].material_1c;
                scratch[i] = material;
                keys[i] = material;
                order[i] = i;
            }
            QuickSortByKey(vertex_ids, scratch, 0, static_cast<int>(vertex_count) - 1);
            memcpy(scratch, order, vertex_count * 4);
            QuickSortByKey(scratch, keys, 0, static_cast<int>(vertex_count) - 1);
            QuickSortByKey(order, scratch, 0, static_cast<int>(vertex_count) - 1);

            for (unsigned long face = 0; face < record->polygon_count_1c; ++face) {
                for (int c = 0; c < 3; ++c) {
                    int* slot = &(&record->poly_vertices_28[face].x)[c];
                    *slot = order[*slot];
                }
            }

            record->vertex_ids_20 = static_cast<int*>(malloc(vertex_count * 4 + 4));
            if (record->vertex_ids_20 == 0) {
                ReportBuildStatus00497690(7, "SplitMeshes: Could not allocate aulVertices.\n");
                return 0;
            }
            memcpy(record->vertex_ids_20, vertex_ids, vertex_count * 4);
            total_vertices += vertex_count;
            total_maps += SplitUVMaps0046A4B0(record, geometry);
        }
    }

    char text[1024];
    sprintf(text,
            "Vertex UV Map Split: %d original vertices, %d final UV map Entries.\n\t"
            "Split Ratio: %3.1f to 1.\n",
            static_cast<int>(total_vertices), static_cast<int>(total_maps),
            static_cast<double>(total_maps) / static_cast<int>(total_vertices));
    ReportBuildStatus00497690(6, text);
    m_root_mesh_count_1a8 = kind_counts[0];
    m_kind1_submesh_count_1ac = kind_counts[1] + kind_counts[0];
    free(staging);
    free(vertex_ids);
    free(keys);
    free(scratch);
    free(order);
    VerifyAutoMeshes0046AD10(geometry, records);
    return spatial_000.submesh_count_74;
}

/* UV dedup pool for SplitUVMaps: slots 0..vertex_count-1 are per-vertex
   heads (link -1 = unused, 0 = chain end, N = next pool index); overflow
   entries chain from the running uv count upward. */
struct W8OctUvPoolEntry {
    long link;
    float u;
    float v;
};

static_assert(sizeof(W8OctUvPoolEntry) == 0xc, "W8OctUvPoolEntry_must_be_0xc");

/* Builds one record's UV map: walks the three corners of every polygon,
   deduplicates uvs through the pool and emits the corner-to-uv index
   triplets plus the final srVector2 map.  Returns the uv count. */
// TEMPLATE: WIZ8 0x0046a490
// srVector3T<float>::operator=

// FUNCTION: WIZ8 0x0046a4b0
unsigned long OctPreTree::SplitUVMaps0046A4B0(W8OctSubmeshBuild* record,
                                              W8OctPreTreeGeometry* geometry)
{
    W8OctUvPoolEntry* table =
        static_cast<W8OctUvPoolEntry*>(malloc(record->polygon_count_1c * 0x30));
    if (table == 0) {
        ReportBuildStatus00497690(7, "SplitUVMaps: Could not allocate pUVMaps.\n");
        return 0;
    }
    memset(table, 0, record->polygon_count_1c * 0x30);
    srVector3i* uv_index =
        static_cast<srVector3i*>(srHeap.allocate(record->polygon_count_1c * 0xc));
    if (uv_index == 0) {
        ReportBuildStatus00497690(7, "SplitUVMaps: Could not allocate psrPolyUVIndex.\n");
        return 0;
    }
    for (unsigned long i = 0; i < record->vertex_count_14; ++i) {
        table[i].link = -1;
    }
    unsigned long uv_count = record->vertex_count_14;
    for (unsigned long poly = 0; poly < record->polygon_count_1c; ++poly) {
        const srVector2T<float>* uvs =
            geometry->polygons_0c[record->polygon_ids_24[poly]].face_48.texture_coordinates;
        for (unsigned long corner = 0; corner < 3; ++corner) {
            int vertex = (&record->poly_vertices_28[poly].x)[corner];
            int chain = vertex + 1;
            int last = vertex;
            if (chain != 0 && table[chain - 1].link >= 0) {
                while (table[chain - 1].u != uvs[corner].x || table[chain - 1].v != uvs[corner].y) {
                    last = chain - 1;
                    if (table[last].link == 0) {
                        /* End of the seam chain: allocate a new uv entry; only
                           an exact u/v match may reuse one. */
                        chain = 0;
                        break;
                    }
                    chain = table[last].link + 1;
                    if (chain == 0) {
                        break;
                    }
                }
                if (chain != 0) {
                    (&uv_index[poly].x)[corner] = chain - 1;
                    continue;
                }
            }
            W8OctUvPoolEntry* entry = &table[last];
            if (table[last].link < 0) {
                (&uv_index[poly].x)[corner] = last;
                entry->u = uvs[corner].x;
                entry->v = uvs[corner].y;
                entry->link = 0;
            } else {
                if (static_cast<int>(record->polygon_count_1c * 4) <= static_cast<int>(uv_count)) {
                    ReportBuildStatus00497690(7, "SplitUVMaps: UV Map count too high.\n");
                    free(table);
                    return 0;
                }
                (&uv_index[poly].x)[corner] = uv_count;
                table[uv_count].link = 0;
                table[uv_count].u = uvs[corner].x;
                table[uv_count].v = uvs[corner].y;
                if (static_cast<int>(record->polygon_count_1c * 4) <= last || last < 0) {
                    ReportBuildStatus00497690(7, "SplitUVMaps: Counter value iLast too high.\n");
                    free(table);
                    return 0;
                }
                entry->link = uv_count;
                ++uv_count;
            }
        }
    }
    record->poly_uv_index_2c = uv_index;
    record->map_count_18 = uv_count;
    record->uv_map_30 = static_cast<srVector2T<float>*>(srHeap.allocate(uv_count * 8));
    if (record->uv_map_30 == 0) {
        ReportBuildStatus00497690(7, "SplitUVMaps: Could not allocate pMesh->psrMaps.\n");
        free(table);
        return 0;
    }
    for (unsigned long uv = 0; uv < uv_count; ++uv) {
        record->uv_map_30[uv].x = table[uv].u;
        record->uv_map_30[uv].y = table[uv].v;
    }
    free(table);
    return uv_count;
}

/* First CreateSubMeshes phase: counts each region's polygons into its build
   record, allocates the polygon id run and refills it while tracking the
   corner bounds, then checks every automesh cell hashed under the region id
   overlaps those bounds.  The cell hash walk uses the table's single-fold
   hash on purpose - the same folding the builder inserted with. */
// FUNCTION: WIZ8 0x0046a790
unsigned long OctPreTree::AllocateSubMesh0046A790(W8OctSubmeshBuild* records)
{
    if (spatial_000.polygon_count_3c > 1) {
        for (unsigned long poly = 1; poly < spatial_000.polygon_count_3c; ++poly) {
            unsigned short region = game_data_3a4->polygons_0c[poly].region_32;
            if (region == 0 || region >= spatial_000.submesh_count_74) {
                char text[1024];
                sprintf(text, "Polygon %d in invalid submesh %d\n", static_cast<int>(poly),
                        static_cast<unsigned int>(region));
                ReportBuildStatus00497690(6, text);
            } else {
                ++records[region].polygon_count_1c;
            }
        }
    }
    for (unsigned long index = 1; index < spatial_000.submesh_count_74; ++index) {
        W8OctSubmeshBuild* record = records + index;
        /* Retail initialises the bounds only on this path; for empty records
           the cell check below reads whatever the stack held. */
        float min_x, min_y, min_z, max_x, max_y, max_z;
        if (record->polygon_count_1c != 0) {
            min_x = 1e+06f;
            min_y = 1e+06f;
            min_z = 1e+06f;
            max_x = -1e+06f;
            max_y = -1e+06f;
            max_z = -1e+06f;
            record->polygon_ids_24 = static_cast<int*>(malloc(record->polygon_count_1c * 4 + 8));
            if (record->polygon_ids_24 == 0) {
                ReportBuildStatus00497690(7, "\nAllocateSubMesh: Could not allocate aulFaces.\n");
                return 0;
            }
            memset(record->polygon_ids_24, 0, record->polygon_count_1c * 4 + 8);
            unsigned long found = 0;
            if (spatial_000.polygon_count_3c > 1) {
                for (unsigned long poly = 1; poly < spatial_000.polygon_count_3c; ++poly) {
                    if (game_data_3a4->polygons_0c[poly].region_32 == index) {
                        record->polygon_ids_24[found] = poly;
                        ++found;
                        if (found == 5000) {
                            ReportBuildStatus00497690(
                                6, "One of your regions has more than 5000 polys in it!\n");
                        }
                        for (int corner = 0; corner < 3; ++corner) {
                            float* position = &game_data_3a4->polygons_0c[poly]
                                                   .vertices_34[corner]
                                                   ->position_0c.x;
                            if (max_x < position[0]) {
                                max_x = position[0];
                            }
                            if (position[0] < min_x) {
                                min_x = position[0];
                            }
                            if (max_y < position[1]) {
                                max_y = position[1];
                            }
                            if (position[1] < min_y) {
                                min_y = position[1];
                            }
                            if (max_z < position[2]) {
                                max_z = position[2];
                            }
                            if (position[2] < min_z) {
                                min_z = position[2];
                            }
                        }
                    }
                }
            }
            if (record->polygon_count_1c != found) {
                ReportBuildStatus00497690(
                    6, "Mismatch between expected number of mesh polys and actual number");
            }
            record->polygon_count_1c = found;
        }
        unsigned short key = static_cast<unsigned short>(index);
        W8HashTable<unsigned short, unsigned long>* cells = automesh_cells_29c;
        int slot = cells->bucket_heads[((key >> 10) ^ key) & (cells->bucket_count - 1)];
        while (slot != -1) {
            if (cells->entries[slot].key == static_cast<short>(index)) {
                unsigned long cell = cells->entries[slot].value;
                float cell_x =
                    static_cast<float>((cell >> 0x10) & 0xff) * spatial_000.region_grid_cell_54 +
                    spatial_000.minimum_0c.x;
                float cell_y =
                    static_cast<float>((cell >> 8) & 0xff) * spatial_000.region_grid_cell_54 +
                    spatial_000.minimum_0c.y;
                float cell_z = static_cast<float>(cell & 0xff) * spatial_000.region_grid_cell_54 +
                               spatial_000.minimum_0c.z;
                if (cell_x + spatial_000.region_grid_cell_54 < min_x || max_x < cell_x ||
                    cell_y + spatial_000.region_grid_cell_54 < min_y || max_y < cell_y ||
                    cell_z + spatial_000.region_grid_cell_54 < min_z || max_z < cell_z) {
                    ReportBuildStatus00497690(7, "AutoMesh has no vertices inside region.");
                }
            }
            slot = cells->entries[slot].next_index;
        }
    }
    return spatial_000.submesh_count_74;
}

/* Rebuilds the leaf-level mask, then for every polygon assigned to an
   auto-region (region_32 >= region_count) descends to its position's leaf and
   reports when the leaf's region differs from the polygon's. */
// FUNCTION: WIZ8 0x0046abf0
void OctPreTree::VerifyPolygonRegions0046ABF0()
{
    m_region_mask_140 = 0;
    for (unsigned long level = spatial_000.leaf_level_52; level != 0; --level) {
        m_region_mask_140 = m_region_mask_140 * 2 + 1;
    }
    for (unsigned long poly = 1; poly < game_data_3a4->polygon_count_08; ++poly) {
        W8OctRegionPolygon* polygon = &game_data_3a4->polygons_0c[poly];
        if (polygon->region_32 >= spatial_000.region_count_46) {
            unsigned int cell[4];
            cell[0] = m_region_mask_140;
            cell[1] =
                static_cast<unsigned int>((polygon->position_18.x - spatial_000.minimum_0c.x) /
                                          spatial_000.region_grid_cell_54);
            cell[2] =
                static_cast<unsigned int>((polygon->position_18.y - spatial_000.minimum_0c.y) /
                                          spatial_000.region_grid_cell_54);
            cell[3] =
                static_cast<unsigned int>((polygon->position_18.z - spatial_000.minimum_0c.z) /
                                          spatial_000.region_grid_cell_54);
            int node = DescendByMask(cell);
            if (m_owned_09c[node].region_02 != static_cast<short>(polygon->region_32)) {
                char text[256];
                sprintf(text, "Poly %d not found in correct region.\n", static_cast<int>(poly));
                ReportBuildStatus00497690(6, text);
            }
        }
    }
}

/* Per root automesh (1..m_meshCount_1b4): walks every cell hashed under the
   mesh id, descends to its node, reports a region mismatch, and checks the
   cell overlaps the vertex bounds of the whole split-record chain. */
// FUNCTION: WIZ8 0x0046ad10
void OctPreTree::VerifyAutoMeshes0046AD10(W8OctPreTreeGeometry* geometry,
                                          W8OctSubmeshBuild* records)
{
    for (unsigned long mesh = 1; mesh < m_meshCount_1b4; ++mesh) {
        unsigned short key = static_cast<unsigned short>(mesh);
        int slot = automesh_cells_29c
                       ->bucket_heads[((key >> 10) ^ key) & (automesh_cells_29c->bucket_count - 1)];
        while (slot != -1) {
            if (automesh_cells_29c->entries[slot].key == static_cast<short>(mesh)) {
                unsigned int cell[4];
                unsigned long packed = automesh_cells_29c->entries[slot].value;
                cell[0] = packed >> 0x18;
                cell[1] = packed >> 0x10 & 0xff;
                cell[2] = packed >> 8 & 0xff;
                cell[3] = packed & 0xff;
                float cell_x = static_cast<float>(cell[1]) * spatial_000.region_grid_cell_54 +
                               spatial_000.minimum_0c.x;
                float cell_y = static_cast<float>(cell[2]) * spatial_000.region_grid_cell_54 +
                               spatial_000.minimum_0c.y;
                float cell_z = static_cast<float>(cell[3]) * spatial_000.region_grid_cell_54 +
                               spatial_000.minimum_0c.z;
                int node = DescendByMask(cell);
                if (node != 0) {
                    if (m_owned_09c[node].region_02 != static_cast<short>(mesh)) {
                        ReportBuildStatus00497690(7, "Region has wrong automesh.");
                    }
                    float min_x = g_float_005ec3c0;
                    float min_y = 1e+06f;
                    float min_z = 1e+06f;
                    float max_x = -1e+06f;
                    float max_y = -1e+06f;
                    float max_z = -1e+06f;
                    for (unsigned long link = mesh; link != 0; link = records[link].next_link_10) {
                        W8OctSubmeshBuild* record = records + link;
                        for (unsigned long i = 0; i < record->vertex_count_14; ++i) {
                            float* position =
                                &geometry->vertices_04[record->vertex_ids_20[i]].position_0c.x;
                            if (max_x < position[0]) {
                                max_x = position[0];
                            }
                            if (position[0] < min_x) {
                                min_x = position[0];
                            }
                            if (max_y < position[1]) {
                                max_y = position[1];
                            }
                            if (position[1] < min_y) {
                                min_y = position[1];
                            }
                            if (max_z < position[2]) {
                                max_z = position[2];
                            }
                            if (position[2] < min_z) {
                                min_z = position[2];
                            }
                        }
                    }
                    if (cell_x + spatial_000.region_grid_cell_54 < min_x || max_x < cell_x ||
                        cell_y + spatial_000.region_grid_cell_54 < min_y || max_y < cell_y ||
                        cell_z + spatial_000.region_grid_cell_54 < min_z || max_z < cell_z) {
                        ReportBuildStatus00497690(7, "AutoMesh has no vertices inside region.");
                    }
                }
            }
            slot = automesh_cells_29c->entries[slot].next_index;
        }
    }
}

/* Grid-walks the level bounds cell by cell: snaps each candidate to the
   ground, runs the obstruction probe, appends a 0x10-byte node record to the
   pre-pathing chunk table, and cross-links conditional props. The node map
   keys each (z<<16 | x) cell so one pass emits at most one node per cell
   unless a duplicate key arrives with an empty value. */
// FUNCTION: WIZ8 0x0046b060
unsigned char OctPreTree::BuildPathLists0046B060(W8GameData* game_data, W8LevelFile* level,
                                                 unsigned int min_component_percent)
{
    W8HashTable<unsigned int, int> node_map;
    W8HashTable<unsigned int, CondPathNode*> cond_map;
    char message[0x400];
    W8PreProp* preprops = 0;
    srVector3T<float> node;

    object_registry = new W8OctreeObjectRegistry;
    g_octree_game_data_00652db0 = game_data;
    delete m_owned_194;
    m_owned_194 = new BitArray(spatial_000.item_count_40 + 0x14);
    pre_pathing_2a0 = new PrePathing;
    pre_pathing_2a0->SnapNamedPositions004CD130(level->pNamedPositions, level->nNamedPositions,
                                                min_component_percent, this);
    ReportBuildStatus00497690(6, "\nBuilding Path Lists:\n=======================\n");
    path_node_extent_3b4 = m_region_cell_178 + m_region_cell_178;
    float level_height =
        (spatial_000.maximum_18.y - spatial_000.minimum_0c.y) * g_path_span_scale_005ec344;
    int x_cells = static_cast<int>((spatial_000.maximum_18.x - spatial_000.minimum_0c.x) /
                                   m_region_cell_178) +
                  1;
    int z_cells = static_cast<int>((spatial_000.maximum_18.z - spatial_000.minimum_0c.z) /
                                   m_region_cell_178) +
                  1;

    int prop_count = CreatePathProps0046C0F0(level, &preprops);
    W8PrePathNode* record = pre_pathing_2a0->GetPathNode();
    W8PrePathNode* head = record;
    path_node_count_2a4 = 1;
    int last_percent = 0;
    if (x_cells > 0) {
        float x_cells_f = static_cast<float>(x_cells);
        for (int x = 0; x < x_cells; ++x) {
            int percent = static_cast<int>(static_cast<float>(x) * 100.0f / x_cells_f);
            if (last_percent < percent) {
                ++last_percent;
                sprintf(message, "  %d%% Complete:  %d Path Nodes Created \r", last_percent,
                        path_node_count_2a4);
                ReportStartupMessage004969D0(message);
            }
            unsigned int cell = static_cast<unsigned int>(x);
            node.x = (static_cast<float>(x) + g_float_005ebc7c) * m_region_cell_178 +
                     spatial_000.minimum_0c.x;
            for (int z = 0; z < z_cells; ++z) {
                node.z = (static_cast<float>(z) + g_float_005ebc7c) * m_region_cell_178 +
                         spatial_000.minimum_0c.z;
                node.y = spatial_000.maximum_18.y;
                while (SnapToGround(&node, 1)) {
                    m_lNumBlocks_2ac = 0;
                    m_lNumSupports_2a8 = 0;
                    if (current_prop >= 0) {
                        float snapped = node.y;
                        if (!SnapToGround(&node, 0) || g_float_005ec3f8 < fabsf(node.y - snapped)) {
                            m_lSupports_2b0[m_lNumSupports_2a8] = current_prop;
                            ++m_lNumSupports_2a8;
                        }
                        node.y = snapped;
                    }
                    if (PathNodeObstructed0046B700(&node) != 1) {
                        W8PrePathNode* next = pre_pathing_2a0->GetPathNode();
                        record->next = next;
                        record = next;
                        record->cell = cell;
                        record->y = node.y;
                        record->level_flags =
                            static_cast<unsigned int>(static_cast<int>(
                                (node.y - spatial_000.minimum_0c.y) / level_height)) +
                            1;
                        if (InsertConditionalNodes0046B9D0(&cond_map, cell, record->level_flags,
                                                           preprops, prop_count)) {
                            record->level_flags |= 0x4000000;
                        }
                        int slot = node_map.FindNextEntry(&cell, -1);
                        if (slot == -1 || node_map.entries[slot].value == 0) {
                            node_map.Insert(&cell, &path_node_count_2a4);
                            record->level_flags |= 0x10000000;
                        }
                        ++path_node_count_2a4;
                    }
                    node.y -= m_lNumSupports_2a8 != 0 ? NAVIGATOR_MINIMUM_HORIZONTAL_DISTANCE
                                                      : g_world_scale_005ebc40;
                }
                cell += 0x10000;
            }
        }
    }
    sprintf(message, "%d Path Nodes Created               \n", path_node_count_2a4);
    ReportBuildStatus00497690(6, message);
    if (path_node_count_2a4 != 0) {
        if (pre_pathing_2a0 == 0) {
            ReportBuildStatus00497690(7, "Could not create PrePathing object\n");
        }
        pre_pathing_2a0->ConfigureForLevel(
            path_node_count_2a4, m_region_cell_178, static_cast<int>(m_path_clearance_17c),
            reinterpret_cast< // reinterpret-ok: minimum_0c/maximum_18 are the adjacent bounds pair
                const W8BoundingBox*>(&spatial_000.minimum_0c),
            m_owned_0c0);
        /* Verified retail behavior: this early return runs only the two
           local hash-table destructors.  preprops (and its pStopMeshes
           arrays), object_registry and g_octree_game_data_00652db0 are all
           left behind - the registry pointer and global stay live. */
        if (!pre_pathing_2a0->BuildPathList(head, &node_map)) {
            return 0;
        }
        pre_pathing_2a0->LinkCollideableProps(prop_count, preprops, &cond_map);
        pre_pathing_2a0->CreateAutomapNodes004CE070(level);
    }
    for (int i = 0; i < prop_count; ++i) {
        /* Verified retail oddity: the binary tests pStopMeshes twice around
           the count check. */
        if (preprops[i].pStopMeshes != 0 && preprops[i].num_stop_meshes_40 != 0 &&
            preprops[i].pStopMeshes != 0) {
            delete[] preprops[i].pStopMeshes;
        }
    }
    free(preprops);
    delete object_registry;
    object_registry = 0;
    g_octree_game_data_00652db0 = 0;
    return 1;
}

/* Probes one node candidate: box-tests the cell against surfaces/props, then
   checks the four footprint corners for ground contact. Sector ids that can
   support a node are deduplicated into m_lSupports (result 6); a prop-blocked
   or floating node yields 1. */
// FUNCTION: WIZ8 0x0046b700
char OctPreTree::PathNodeObstructed0046B700(const srVector3T<float>* node)
{
    srVector3T<float> bounds_min, bounds_max;
    srVector3T<float> corner;
    char result;
    char probe;

    /* The serialized +0x17c header word is a float the pathing code reads
       bit-wise: the probe-box height above the node. */
    float clearance;
    memcpy(&clearance, &m_path_clearance_17c, sizeof(clearance));
    bounds_min.y = node->y + clearance * g_navigator_mode3_scale_005ebca4;
    bounds_max.y = bounds_min.y + clearance;
    float half = m_region_cell_178 * g_float_005ebc7c;
    bounds_min.x = node->x - half;
    bounds_max.x = node->x + half;
    bounds_min.z = node->z - half;
    bounds_max.z = bounds_min.z + half + half;

    result = TestPathPropBounds0046BEC0(&bounds_min, &bounds_max);
    if (result != 1) {
        for (int i = 0; i < 4; ++i) {
            char out = 1;
            if (result == 1)
                break;
            switch (i) {
            case 0:
                corner.x = bounds_min.x;
                corner.z = bounds_min.z;
                break;
            case 1:
                corner.x = bounds_min.x;
                corner.z = bounds_max.z;
                break;
            case 2:
                corner.x = bounds_max.x;
                corner.z = bounds_min.z;
                break;
            case 3:
                corner.x = bounds_max.x;
                corner.z = bounds_max.z;
                break;
            }
            corner.y = bounds_min.y + g_world_scale_005ebc40;
            out = SnapToGround(&corner, 1);
            if (out == 0) {
                out = 1;
            } else if (current_prop != -1 ||
                       fabsf(node->y - corner.y) <=
                           clearance * static_cast<float>(g_double_005ebe80)) {
                out = result;
                if (current_prop >= 0) {
                    corner.y = bounds_min.y + g_world_scale_005ebc40;
                    probe = SnapToGround(&corner, 0);
                    if (probe == 0 || clearance * static_cast<float>(g_double_005ebe80) <
                                          fabsf(node->y - corner.y)) {
                        for (int b = 0; b < m_lNumBlocks_2ac && result != 1; ++b) {
                            if (current_prop == m_lBlocks_328[b])
                                result = 1;
                        }
                        out = result;
                        if (result != 1) {
                            bool absent = true;
                            for (int s = 0; s < m_lNumSupports_2a8; ++s) {
                                if (current_prop == m_lSupports_2b0[s])
                                    absent = false;
                            }
                            if (absent) {
                                m_lSupports_2b0[m_lNumSupports_2a8] = current_prop;
                                ++m_lNumSupports_2a8;
                            }
                            out = 6;
                        }
                    }
                }
            } else {
                out = 1;
            }
            result = out;
        }
    }
    /* Verified retail order: the appends above write m_lSupports_2b0[30]/
       m_lBlocks_328[30] before this check ever runs, so a 31st entry goes out
       of bounds before the assertion fires. */
    if (m_lNumSupports_2a8 > 29 || m_lNumBlocks_2ac > 29) {
        srAssertFail("(m_lNumSupports < 30 && m_lNumBlocks < 30)", OCTPRETREE_CPP, 0x7ab,
                     "PathNodeObstructed: Too many props affecting one pathnode.");
    }
    return result;
}

/* Cross-links each support/blocker prop id to the pre-prop record that
   produced it and stores a CondPathNode {value, cell} pair in the
   conditional-node map keyed by (stop-mesh frame << 16 | preprop index + 1).
   Blocker nodes are flagged 0x2000000. */
// FUNCTION: WIZ8 0x0046b9d0
unsigned char
OctPreTree::InsertConditionalNodes0046B9D0(W8HashTable<unsigned int, CondPathNode*>* nodes,
                                           unsigned int cell, unsigned int node,
                                           W8PreProp* preprops, int preprop_count)
{
    if (m_lNumSupports_2a8 == 0 && m_lNumBlocks_2ac == 0)
        return 0;

    for (int s = 0; s < m_lNumSupports_2a8; ++s) {
        int prop_id = m_lSupports_2b0[s];
        bool found = false;
        for (int p = 0; p < preprop_count && !found; ++p) {
            W8PreProp* pp = preprops + p;
            if (pp->num_stop_meshes_40 != 0 &&
                static_cast<int>(pp->first_prop_number_42) <= prop_id &&
                prop_id < static_cast<int>(pp->first_prop_number_42 + pp->num_stop_meshes_40)) {
                unsigned int key =
                    (static_cast<unsigned int>(
                         pp->pStopMeshes[prop_id - pp->first_prop_number_42].m_prop_number_02)
                     << 16) |
                    static_cast<unsigned int>(p + 1);
                CondPathNode* cond_node = static_cast<CondPathNode*>(malloc(sizeof(CondPathNode)));
                if (cond_node == 0) {
                    srAssertFail(
                        "pCondNode", OCTPRETREE_CPP, 0x7e0,
                        "InsertConditionalNodes: Could not allocate CondPathNode stucture.");
                }
                cond_node->value = node;
                cond_node->cell = cell;
                nodes->Insert(&key, &cond_node);
                found = true;
            }
        }
        if (!found) {
            srAssertFail("FALSE", OCTPRETREE_CPP, 0x7e8,
                         "Could not find prop and frame for conditional node.");
        }
    }
    for (int s2 = 0; s2 < m_lNumBlocks_2ac; ++s2) {
        int prop_id = m_lBlocks_328[s2];
        bool found = false;
        for (int p2 = 0; p2 < preprop_count && !found; ++p2) {
            W8PreProp* pp = preprops + p2;
            if (pp->num_stop_meshes_40 != 0 &&
                static_cast<int>(pp->first_prop_number_42) <= prop_id &&
                prop_id < static_cast<int>(pp->first_prop_number_42 + pp->num_stop_meshes_40)) {
                unsigned int key =
                    (static_cast<unsigned int>(
                         pp->pStopMeshes[prop_id - pp->first_prop_number_42].m_prop_number_02)
                     << 16) |
                    static_cast<unsigned int>(p2 + 1);
                CondPathNode* cond_node = static_cast<CondPathNode*>(malloc(sizeof(CondPathNode)));
                if (cond_node == 0) {
                    srAssertFail(
                        "pCondNode", OCTPRETREE_CPP, 0x7fc,
                        "InsertConditionalNodes: Could not allocate CondPathNode stucture.");
                }
                cond_node->value = node | 0x2000000;
                cond_node->cell = cell;
                nodes->Insert(&key, &cond_node);
                found = true;
            }
        }
        if (!found) {
            srAssertFail("FALSE", OCTPRETREE_CPP, 0x804,
                         "Could not find prop and frame for conditional node.");
        }
    }
    return 1;
}

// FUNCTION: WIZ8 0x0046bec0
char OctPreTree::TestPathPropBounds0046BEC0(const srVector3T<float>* minimum,
                                            const srVector3T<float>* maximum)
{
    unsigned long* ids = 0;
    srVector3T<float> bounds[2];
    srVector3T<float> triangle[3];
    unsigned char hit = 0;

    bounds[0] = *minimum;
    bounds[1] = *maximum;
    int count = QueryObjects(&ids, minimum, maximum, 3, -1);
    for (int i = 0; i < count && hit == 0; ++i) {
        W8GDSurface* surface = g_octree_game_data_00652db0->m_pSurfaces + ids[i];
        if ((surface->flags_00 & 0x1080) == 0) {
            triangle[0] = g_octree_game_data_00652db0->m_pVertices[surface->vertex_indices_18[0]];
            triangle[1] = g_octree_game_data_00652db0->m_pVertices[surface->vertex_indices_18[1]];
            triangle[2] = g_octree_game_data_00652db0->m_pVertices[surface->vertex_indices_18[2]];
            hit = TestSpatialTriangle0046CE60(bounds, triangle, surface->Normal());
        }
    }
    current_prop = -1;
    if (hit != 0)
        return 1;

    count = QueryObjects(&ids, minimum, maximum, 8, -1);
    bool prop_hit = false;
    for (int k = 0; k < count; ++k) {
        int id = ids[k];
        GDProp* prop = *props_3b8->GetAt(id);
        if (prop->BoundsOverlap004B7620(minimum, maximum) != 0) {
            prop_hit = true;
            if ((prop->m_flags_00 & 1) != 0)
                return 1;
            /* Dead in retail: current_prop was just set to -1 above and
               QueryObjects never republishes it, while support entries are
               registered prop ids >= 0 - the comparison can never fire. */
            if (m_lNumSupports_2a8 != 0 && current_prop == m_lSupports_2b0[0])
                return 1;
            m_lBlocks_328[m_lNumBlocks_2ac] = id;
            ++m_lNumBlocks_2ac;
        }
    }
    if (prop_hit)
        return 3;
    return 0;
}

/* Builds one W8PreProp per level prop flagged for pathing: allocates the
   per-frame GDPreProp stop meshes, applies each anim frame to seed bounds,
   and registers every element in props_3b8. Returns the record count and the
   malloc'd array through `preprops`. */
// FUNCTION: WIZ8 0x0046c0f0
int OctPreTree::CreatePathProps0046C0F0(W8LevelFile* level, W8PreProp** preprops)
{
    int count = level->nProps;
    unsigned short prop_number = 0;
    W8PreProp* records = static_cast<W8PreProp*>(malloc(count * 0x48));
    memset(records, 0, count * 0x48);
    if (count < 1) {
        *preprops = records;
    } else {
        W8BoundingBox bounds;
        for (int i = 0; i < count; ++i) {
            W8LevelFileProp* prop = level->pProps + i;
            W8PreProp* record = records + i;
            if ((prop->flags_0f & 1) == 0)
                continue;
            if (prop->num_frame_pos_b7 == 0) {
                record->num_stop_meshes_40 =
                    PropFramesDiffer0046C6A0(&prop->anim_obj_53, 0, 0xffff) ? 2 : 1;
                record->first_prop_number_42 = prop_number;
                record->pStopMeshes = new GDPreProp[record->num_stop_meshes_40];
                if (record->pStopMeshes == 0) {
                    srAssertFail("pPreProps[i].pStopMeshes", OCTPRETREE_CPP, 0x88e,
                                 "CreatePathProps: Couldn't allocate GDPreProp objects.");
                }
                strcpy(record->name, prop->name_13);
                record->pStopMeshes[0].ApplyAnimFrame004B7C00(0, &prop->anim_obj_53);
                record->pStopMeshes[0].ComputeBounds004B7500(&bounds.minimum, &bounds.maximum);
                AddCollidablePropBounds(prop_number, &bounds);
                props_3b8->Add(record->pStopMeshes);
                if (record->num_stop_meshes_40 == 2) {
                    record->pStopMeshes[1].ApplyAnimFrame004B7C00(0xffff, &prop->anim_obj_53);
                    record->pStopMeshes[1].ComputeBounds004B7500(&bounds.minimum, &bounds.maximum);
                    AddCollidablePropBounds(static_cast<unsigned short>(prop_number + 1), &bounds);
                    props_3b8->Add(record->pStopMeshes + 1);
                    prop_number += 2;
                } else {
                    record->pStopMeshes[0].m_flags_00 |= 1;
                    unsigned short last = 0xffff;
                    if (prop->anim_obj_53.num_transforms_5a > 0) {
                        W8LevelFileTransform* t = prop->anim_obj_53.pTransforms_5b;
                        for (int t_i = prop->anim_obj_53.num_transforms_5a; t_i != 0; --t_i, ++t) {
                            if (t->pathAI_06.path_count_0a <= static_cast<int>(last)) {
                                last = static_cast<unsigned short>(t->pathAI_06.path_count_0a - 1);
                            }
                        }
                    }
                    record->pStopMeshes[0].last_frame_58 = last;
                    ++prop_number;
                }
            } else {
                record->num_stop_meshes_40 = static_cast<unsigned short>(prop->num_frame_pos_b7);
                record->first_prop_number_42 = prop_number;
                strcpy(record->name, prop->name_13);
                record->pStopMeshes = new GDPreProp[record->num_stop_meshes_40];
                if (record->pStopMeshes == 0) {
                    srAssertFail("pPreProps[i].pStopMeshes", OCTPRETREE_CPP, 0x8ad,
                                 "CreatePathProps: Couldn't allocate GDPreProp objects.");
                }
                for (unsigned short j = 0; j < record->num_stop_meshes_40; ++j) {
                    unsigned short frame = prop->usFrame_Pos[j].frame;
                    if (static_cast<unsigned short>(prop->bNumFrames) <= frame) {
                        srAssertFail(
                            "(pLVL->pProps[i].usFrame_Pos[j*2] < "
                            "(UINT16)(pLVL->pProps[i].bNumFrames))", /* c-style-cast-ok: verbatim
                                retail assertion text, kept for .rdata match */
                            OCTPRETREE_CPP, 0x8b4,
                            reinterpret_cast<const char*>( // reinterpret-ok: String returns UINT8*
                                String("%s Prop Error:Segment frame number %d is out of range",
                                       prop->name_13, frame)));
                    }
                    record->pStopMeshes[j].ApplyAnimFrame004B7C00(frame, &prop->anim_obj_53);
                    record->pStopMeshes[j].ComputeBounds004B7500(&bounds.minimum, &bounds.maximum);
                    AddCollidablePropBounds(prop_number, &bounds);
                    ++prop_number;
                    props_3b8->Add(record->pStopMeshes + j);
                }
            }
        }
        *preprops = records;
    }
    return count;
}

/* Compares two animation frames across every transform channel: a prop
   differs when the sampled position moves more than half a unit, the frame
   quaternions disagree beyond the snap epsilon (unless they are the mirrored
   same rotation), or scaled-path tails drift. */
// FUNCTION: WIZ8 0x0046c6a0
static char PropFramesDiffer0046C6A0(W8LevelFileAnimObj* anim, unsigned short first,
                                     unsigned short last)
{
    char count = anim->num_transforms_5a;
    if (count > 0) {
        W8LevelFileTransform* t = anim->pTransforms_5b;
        for (int i = count; i != 0; --i, ++t) {
            if (t->pathAI_06.path_count_0a <= static_cast<int>(last)) {
                last = static_cast<unsigned short>(t->pathAI_06.path_count_0a - 1);
            }
        }
    }
    char differ = 0;
    if (count > 0) {
        W8LevelFileTransform* t = anim->pTransforms_5b;
        for (int i = count; i != 0; --i, ++t) {
            W8LevelFileScaledPathNode a, b;
            if (t->pathAI_06.scaled_01 == 2) {
                a = t->pathAI_06.pScaledPaths[first];
                b = t->pathAI_06.pScaledPaths[last];
                if (g_camera_snap_epsilon_005ebc2c < fabsf(a.scale.x - b.scale.x) ||
                    g_camera_snap_epsilon_005ebc2c < fabsf(a.scale.y - b.scale.y) ||
                    g_camera_snap_epsilon_005ebc2c < fabsf(a.scale.z - b.scale.z)) {
                    differ = 1;
                }
            } else {
                a.path = t->pathAI_06.pPaths[first];
                b.path = t->pathAI_06.pPaths[last];
            }
            if (g_float_005ebc7c < fabsf(a.path.position_00.x - b.path.position_00.x) ||
                g_float_005ebc7c < fabsf(a.path.position_00.y - b.path.position_00.y) ||
                g_float_005ebc7c < fabsf(a.path.position_00.z - b.path.position_00.z)) {
                differ = 1;
            }
            if (g_camera_snap_epsilon_005ebc2c < fabsf(a.path.angle_0c - b.path.angle_0c) ||
                g_camera_snap_epsilon_005ebc2c < fabsf(a.path.axis_10.x - b.path.axis_10.x) ||
                g_camera_snap_epsilon_005ebc2c < fabsf(a.path.axis_10.y - b.path.axis_10.y) ||
                g_camera_snap_epsilon_005ebc2c < fabsf(a.path.axis_10.z - b.path.axis_10.z)) {
                if (!(fabsf((b.path.angle_0c + a.path.angle_0c) - g_camera_half_pi_005ec3fc) <=
                          g_float_005ebc3c &&
                      fabsf(b.path.axis_10.x + a.path.axis_10.x) <=
                          g_camera_snap_epsilon_005ebc2c &&
                      fabsf(b.path.axis_10.y + a.path.axis_10.y) <=
                          g_camera_snap_epsilon_005ebc2c &&
                      fabsf(b.path.axis_10.z + a.path.axis_10.z) <=
                          g_camera_snap_epsilon_005ebc2c)) {
                    differ = 1;
                }
            }
        }
    }
    return differ;
}

// TEMPLATE: WIZ8 0x0046ca50
// QuickSortByKey<srVector3i>

// SYNTHETIC: WIZ8 0x0046cc50
// W8Octree::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x0046cc80
// W8OctSpatialState::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x0046cca0
// W8GrowableVector<GDProp*>::~W8GrowableVector

/* Construct the spatial value used by both the runtime octree and the level
   build tree.  A source value describes the next child: its extent halves and
   its depth advances only when the source is the root-kind record. */
// FUNCTION: WIZ8 0x0046ccc0
W8OctSpatialState::W8OctSpatialState(const W8OctSpatialState* source)
{
    Reset0046CDC0();
    level_kind_6c = 1;
    if (source != 0) {
        for (int axis = 0; axis != 3; ++axis) {
            (&minimum_0c.x)[axis] = (&source->minimum_0c.x)[axis];
            (&maximum_18.x)[axis] = (&source->maximum_18.x)[axis];
            (&clipped_minimum_24.x)[axis] = (&source->clipped_minimum_24.x)[axis];
            (&clipped_maximum_30.x)[axis] = (&source->clipped_maximum_30.x)[axis];
        }
        if (source->level_kind_6c == 1) {
            extent_04 = source->extent_04 * g_float_005ebc7c;
            depth_44 = source->depth_44 + 1;
        } else {
            extent_04 = source->extent_04;
            depth_44 = source->depth_44;
        }
        cell_size_08 = source->cell_size_08;
        polygon_count_3c = source->polygon_count_3c;
        region_id_bound_58 = source->region_id_bound_58;
        leaf_grid_stride_x_64 = source->leaf_grid_stride_x_64;
        leaf_grid_stride_y_68 = source->leaf_grid_stride_y_68;
        node_extent_70 = source->node_extent_70;
        root_90 = source->root_90;
        node_index_94 = source->node_index_94;
        owned_98 = source->owned_98;
        flags_00 = source->flags_00;
        item_count_40 = source->item_count_40;
        submesh_count_74 = source->submesh_count_74;
        region_count_46 = source->region_count_46;
        leaf_level_52 = source->leaf_level_52;
        owned_5c = source->owned_5c;
        node_extent_70 = source->node_extent_70;
        max_region_radius_60 = source->max_region_radius_60;
    }
}

// FUNCTION: WIZ8 0x0046cdc0
void W8OctSpatialState::Reset0046CDC0()
{
    memset(this, 0, sizeof(*this));
}

// FUNCTION: WIZ8 0x0046cdf0
void W8OctSpatialState::GetWorkingBounds0046CDF0(srVector3T<float>* minimum,
                                                 srVector3T<float>* maximum)
{
    minimum->x = working_minimum_78.x;
    minimum->y = working_minimum_78.y;
    minimum->z = working_minimum_78.z;
    maximum->x = working_maximum_84.x;
    maximum->y = working_maximum_84.y;
    maximum->z = working_maximum_84.z;
}

// FUNCTION: WIZ8 0x0046ce30
void W8OctSpatialState::GetClippedBounds0046CE30(srVector3T<float>* minimum,
                                                 srVector3T<float>* maximum)
{
    minimum->x = clipped_minimum_24.x;
    minimum->y = clipped_minimum_24.y;
    minimum->z = clipped_minimum_24.z;
    maximum->x = clipped_maximum_30.x;
    maximum->y = clipped_maximum_30.y;
    maximum->z = clipped_maximum_30.z;
}

// FUNCTION: WIZ8 0x0046cdd0
W8OctSpatialState::~W8OctSpatialState()
{
    owned_5c = 0;
    root_90 = 0;
    owned_98 = 0;
}

/* Strict axis-aligned overlap: touching faces are not an intersection. */
// FUNCTION: WIZ8 0x0046d470
unsigned char BoundsOverlap0046D470(const srVector3T<float>* first, const srVector3T<float>* second)
{
    return first[1].x > second[0].x && first[0].x < second[1].x && first[1].y > second[0].y &&
           first[0].y < second[1].y && first[1].z > second[0].z && first[0].z < second[1].z;
}

/* Inclusive point containment for an axis-aligned box. */
// FUNCTION: WIZ8 0x0046d4d0
unsigned char PointInsideBounds0046D4D0(const srVector3T<float>* bounds,
                                        const srVector3T<float>* point)
{
    return bounds[0].x <= point->x && point->x <= bounds[1].x && bounds[0].y <= point->y &&
           point->y <= bounds[1].y && bounds[0].z <= point->z && point->z <= bounds[1].z;
}

/* Test a triangle against an axis-aligned box.  The inexpensive containment
   and separating-axis checks precede explicit triangle-edge intersections
   with all six box faces. */
// FUNCTION: WIZ8 0x0046ce60
unsigned char TestSpatialTriangle0046CE60(const srVector3T<float>* bounds,
                                          const srVector3T<float>* vertices,
                                          const srVector3T<float>* plane_normal)
{
    const float* minimum = &bounds[0].x;
    const float* maximum = &bounds[1].x;
    const float* vertex_values[3] = {&vertices[0].x, &vertices[1].x, &vertices[2].x};

    short vertex_index;
    for (vertex_index = 0; vertex_index < 3; ++vertex_index) {
        const float* vertex = &vertices[vertex_index].x;
        if (minimum[0] <= vertex[0] && vertex[0] <= maximum[0] && minimum[1] <= vertex[1] &&
            vertex[1] <= maximum[1] && minimum[2] <= vertex[2] && vertex[2] <= maximum[2]) {
            return 1;
        }
    }

    if (plane_normal == 0) {
        return 0;
    }
    const float* normal = &plane_normal->x;

    unsigned char near_axis = 0;
    float plane_point[3];
    for (short axis = 0; axis < 3; ++axis) {
        if (vertex_values[0][axis] < minimum[axis] && vertex_values[1][axis] < minimum[axis] &&
            vertex_values[2][axis] < minimum[axis]) {
            return 0;
        }
        if (maximum[axis] < vertex_values[0][axis] && maximum[axis] < vertex_values[1][axis] &&
            maximum[axis] < vertex_values[2][axis]) {
            return 0;
        }
        if (g_float_005ec414 < (float)fabs(normal[axis]) &&
            minimum[axis] <= vertex_values[0][axis] && vertex_values[0][axis] <= maximum[axis]) {
            near_axis = 1;
        }
        plane_point[axis] =
            (vertex_values[0][axis] + vertex_values[1][axis] + vertex_values[2][axis]) *
            g_float_005ec410;
    }

    if (near_axis == 0) {
        unsigned char negative = 0;
        unsigned char positive = 0;
        for (int x = 0; x != 2; ++x) {
            for (int y = 0; y != 2; ++y) {
                for (int z = 0; z != 2; ++z) {
                    float distance =
                        ((x == 0 ? minimum[0] : maximum[0]) - plane_point[0]) * plane_normal->x +
                        ((y == 0 ? minimum[1] : maximum[1]) - plane_point[1]) * plane_normal->y +
                        ((z == 0 ? minimum[2] : maximum[2]) - plane_point[2]) * plane_normal->z;
                    if (distance <= g_float_005ebb34) {
                        negative = 1;
                    }
                    if (g_float_005ebb34 <= distance) {
                        positive = 1;
                    }
                }
            }
        }
        if (negative == 0 || positive == 0) {
            return 0;
        }
    }

    float edge_start[3][3];
    float edge_delta[3][3];
    for (int edge = 0; edge != 3; ++edge) {
        int next = edge == 2 ? 0 : edge + 1;
        for (int axis = 0; axis != 3; ++axis) {
            edge_start[edge][axis] = vertex_values[edge][axis];
            edge_delta[edge][axis] = vertex_values[next][axis] - vertex_values[edge][axis];
        }
    }

    for (short face_axis = 0; face_axis < 3; ++face_axis) {
        short first_axis = face_axis == 2 ? 0 : face_axis + 1;
        short second_axis = face_axis == 0 ? 2 : face_axis - 1;
        for (short side = 0; side < 2; ++side) {
            float intersection[2][2];
            short intersection_count = 0;
            float face = side == 0 ? minimum[face_axis] : maximum[face_axis];

            for (short edge = 0; edge < 3; ++edge) {
                if ((float)g_double_005ebc70 < (float)fabs(edge_delta[edge][face_axis])) {
                    float amount =
                        (face - edge_start[edge][face_axis]) / edge_delta[edge][face_axis];
                    if (g_float_005ebb34 <= amount && amount <= g_float_005ebb38) {
                        float first =
                            edge_start[edge][first_axis] + amount * edge_delta[edge][first_axis];
                        float second =
                            edge_start[edge][second_axis] + amount * edge_delta[edge][second_axis];
                        if (minimum[first_axis] <= first && first <= maximum[first_axis] &&
                            minimum[second_axis] <= second && second <= maximum[second_axis]) {
                            return 1;
                        }
                        if (intersection_count < 2) {
                            intersection[intersection_count][0] = first;
                            intersection[intersection_count][1] = second;
                        }
                        ++intersection_count;
                    }
                }
            }

            if (intersection_count == 2) {
                float delta[2] = {intersection[1][0] - intersection[0][0],
                                  intersection[1][1] - intersection[0][1]};
                short rectangle_axes[2] = {first_axis, second_axis};
                for (short coordinate = 0; coordinate < 2; ++coordinate) {
                    if (g_float_005ebc90 < (float)fabs(delta[coordinate])) {
                        short other = coordinate == 0 ? 1 : 0;
                        for (short edge_side = 0; edge_side < 2; ++edge_side) {
                            float boundary = edge_side == 0 ? minimum[rectangle_axes[coordinate]]
                                                            : maximum[rectangle_axes[coordinate]];
                            float amount =
                                (boundary - intersection[0][coordinate]) / delta[coordinate];
                            if (g_float_005ebb34 <= amount && amount <= g_float_005ebb38) {
                                float crossing = intersection[0][other] + amount * delta[other];
                                if (minimum[rectangle_axes[other]] <= crossing &&
                                    crossing <= maximum[rectangle_axes[other]]) {
                                    return 1;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
