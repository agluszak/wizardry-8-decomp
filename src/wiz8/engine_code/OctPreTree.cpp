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

/* Same 50.0f descent step Octree.cpp emits at 0x005EC02C; the linker folds the
   identical constants into one address. */
static const float NAVIGATOR_MINIMUM_HORIZONTAL_DISTANCE = 50.0f;
// GLOBAL: WIZ8 0x005ebc28
const float g_float_005ebc28 = 5.0f;
// GLOBAL: WIZ8 0x005ebc70
const double g_double_005ebc70 = 0.0001;
// GLOBAL: WIZ8 0x005ebc90
const float g_float_005ebc90 = 9.999999747378752e-05f;
// GLOBAL: WIZ8 0x005ec410
const float g_float_005ec410 = 0.3333333432674408f;
// GLOBAL: WIZ8 0x005ec414
const float g_float_005ec414 = 0.9998999834060669f;

// GLOBAL: WIZ8 0x00659c74
OctPreTree* g_oct_pre_tree = 0;

/* Paired item/key sorts shared through stHash.hpp; these emissions are this
   file's unsigned long and unsigned short instantiations. */

/* The build-time runtime tree extends the ordinary 0x29c octree with transfer
   bookkeeping and one separately owned pointer vector.  Its only recovered
   construction caller allocates exactly 0x3bc bytes. */
// FUNCTION: WIZ8 0x004679e0
OctPreTree::OctPreTree() : W8Octree(0, 0)
{
    game_data = 0;
    unknown_3a8 = 0;
    unknown_3ac = 0;
    deepest_link_list = 0;
    m_region_cell = 0.0f;
    path_node_extent = 0;
    automesh_cells = 0;
    pre_pathing = 0;
    props = new W8GrowableVector<GDProp*>;
    g_oct_pre_tree = this;
}

/* Tears down the automesh cell map, the owned pre-pathing service and the
   registered-prop vector, then clears the global before the octree base. */
// FUNCTION: WIZ8 0x00467ab0
OctPreTree::~OctPreTree()
{
    if (automesh_cells != 0) {
        delete automesh_cells;
        automesh_cells = 0;
    }
    if (pre_pathing != 0) {
        delete pre_pathing;
        pre_pathing = 0;
    }
    if (props != 0) {
        delete props;
    }
    g_oct_pre_tree = 0;
}

/* Copies the working bounds straight into the spatial state; the build tree
   conversion calls it once its clipped bounds are known. */
// FUNCTION: WIZ8 0x00467b70
void W8OctSpatialState::SetWorkingBounds(const srVector3T<float>* minimum,
                                         const srVector3T<float>* maximum)
{
    m_working_minimum = *minimum;
    m_working_maximum = *maximum;
}

/* Resets the collected-id run and appends every not-yet-seen polygon id the
   leaf under `cell` lists.  The trace walk inlines this sequence at each of
   the six cells it probes. */
void OctPreTree::CollectLeafPolygons(const srVector3T<int>* cell)
{
    m_gd_result_count = 0;
    unsigned int leaf_index = LeafIndexForCell(cell);
    if (leaf_index != 0 && m_leaves[leaf_index].polygon_offset != 0) {
        const unsigned long* stream =
            m_polygon_index_stream + m_leaves[leaf_index].polygon_offset;
        for (int remaining = *stream; remaining != 0; --remaining) {
            ++stream;
            if (m_visited_polygon_bits->Set(*stream) == 0) {
                m_aulGDObjs[m_gd_result_count] = *stream;
                ++m_gd_result_count;
            }
        }
    }
}

/* Walks the `from`-`to` segment through the leaf grid, collecting each
   visited leaf's region-polygon ids and plane/slab-testing them.  Answers
   whether the segment is unobstructed; the light-visibility callers
   accumulate its result. */
// FUNCTION: WIZ8 0x00467bb0
bool OctPreTree::SegmentClear(const srVector3T<float>* from, const srVector3T<float>* to)
{
    W8OctreeTrace trace;
    W8OctreeWalk walk;
    srVector3T<int> cell;
    srVector3T<int> end_cell;
    int span = 0;
    bool blocked = false;

    trace.Seed(from, to);
    m_gd_result_count = 0;
    m_visited_polygon_bits->ClearAll();
    for (int axis = 0; axis < 3; ++axis) {
        (&cell.x)[axis] = static_cast<int>(((&from->x)[axis] - (&m_spatial.m_minimum.x)[axis]) /
                                           m_spatial.m_node_extent);
        (&end_cell.x)[axis] = static_cast<int>(((&to->x)[axis] - (&m_spatial.m_minimum.x)[axis]) /
                                               m_spatial.m_node_extent);
        int difference = (&cell.x)[axis] - (&end_cell.x)[axis];
        if (difference < 0) {
            span -= difference;
        } else {
            span += difference;
        }
    }
    if (span < 2) {
        CollectLeafPolygons(&cell);
        blocked = TestCollectedPolygons(&trace);
        if (!blocked && span != 0) {
            CollectLeafPolygons(&end_cell);
            blocked = TestCollectedPolygons(&trace);
        }
    } else {
        BuildCellWalk(*from, *to, &walk);
        int error_a = walk.error0;
        int error_b = walk.error1;
        for (int index = 0; index < walk.count; ++index) {
            if (blocked != 0) {
                break;
            }
            CollectLeafPolygons(&cell);
            if (m_gd_result_count != 0) {
                blocked = TestCollectedPolygons(&trace);
            }
            if (error_a < error_b) {
                if (error_a < 0 && !blocked) {
                    (&cell.x)[walk.minor_axis0] += (&walk.step.x)[walk.minor_axis0];
                    error_a += walk.error_reset0;
                    CollectLeafPolygons(&cell);
                    if (m_gd_result_count != 0) {
                        blocked = TestCollectedPolygons(&trace);
                    }
                    if (error_b < 0 && !blocked) {
                        (&cell.x)[walk.minor_axis1] += (&walk.step.x)[walk.minor_axis1];
                        error_b += walk.error_reset1;
                        CollectLeafPolygons(&cell);
                        if (m_gd_result_count != 0) {
                            blocked = TestCollectedPolygons(&trace);
                        }
                    }
                }
            } else {
                if (error_b < 0 && !blocked) {
                    (&cell.x)[walk.minor_axis1] += (&walk.step.x)[walk.minor_axis1];
                    error_b += walk.error_reset1;
                    CollectLeafPolygons(&cell);
                    if (m_gd_result_count != 0) {
                        blocked = TestCollectedPolygons(&trace);
                    }
                    if (error_a < 0 && !blocked) {
                        (&cell.x)[walk.minor_axis0] += (&walk.step.x)[walk.minor_axis0];
                        error_a += walk.error_reset0;
                        CollectLeafPolygons(&cell);
                        if (m_gd_result_count != 0) {
                            blocked = TestCollectedPolygons(&trace);
                        }
                    }
                }
            }
            (&cell.x)[walk.major_axis] += (&walk.step.x)[walk.major_axis];
            error_a -= walk.error_delta0;
            error_b -= walk.error_delta1;
        }
    }
    return blocked == 0;
}

/* Tests the collected region polygons' planes against the trace segment.  A
   polygon blocks only when its plane faces the ray, the crossing lies inside
   the segment's `length - 1.0f` window, and either the start point sits
   within 1.0f of the plane or the ray exits at least 5.0f behind it, with the
   resulting contact point landing inside the polygon. */
// FUNCTION: WIZ8 0x004681e0
bool OctPreTree::TestCollectedPolygons(W8OctreeTrace* trace)
{
    float limit = trace->length - g_float_one;
    bool blocked = false;

    for (unsigned long index = 0; index < m_gd_result_count; ++index) {
        if (blocked != 0) {
            break;
        }
        W8OctRegionPolygon* polygon = &game_data->m_polygons[m_aulGDObjs[index]];
        const W8Plane* plane = &polygon->plane;
        if (plane->normal.x * trace->step.x + trace->step.y * plane->normal.y +
                trace->step.z * plane->normal.z <=
            g_float_zero) {
            float front = SignedPlaneDistance(*plane, trace->start);
            if (front <= limit && g_float_zero < front) {
                srVector3T<float> contact;
                if (g_float_one <= front) {
                    float back = SignedPlaneDistance(*plane, trace->end);
                    if (g_float_005ebc28 <= back) {
                        continue;
                    }
                    back = -back;
                    if (back < g_float_005ebc28) {
                        continue;
                    }
                    front = front / (back + front) * trace->length;
                    contact.Set(trace->step.x * front + trace->start.x,
                                trace->step.y * front + trace->start.y,
                                trace->step.z * front + trace->start.z);
                } else {
                    contact = trace->start;
                }
                srVector3T<float> vertices[3];
                vertices[0] = polygon->vertices[0]->position;
                vertices[1] = polygon->vertices[1]->position;
                vertices[2] = polygon->vertices[2]->position;
                if (PointInsideTriangle(vertices, polygon->flags & 3, &contact)) {
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
unsigned char OctPreTree::WriteOctFile(W8OctPreTreeGeometry* geometry, W8GameData* game_data)
{
    unsigned char written;
    int file;
    unsigned long sentinel = 0xffffffff;
    W8OctFileHeader header;

    if (m_leaf_count != 0) {
        for (unsigned long index = 0; index < m_leaf_count; ++index) {
            m_leaves[index].flags &= 0xfffffffe;
        }
    }
    header.m_extent = m_spatial.m_extent;
    header.m_cell_size = m_spatial.m_cell_size;
    header.m_node_extent = m_spatial.m_node_extent;
    header.version = W8OctFileHeader::VERSION;
    header.m_bounds[0] = m_spatial.m_minimum;
    header.m_bounds[1] = m_spatial.m_maximum;
    header.m_bounds[2] = m_spatial.m_clipped_minimum;
    header.m_bounds[3] = m_spatial.m_clipped_maximum;
    header.m_bounds[4] = m_spatial.m_working_minimum;
    header.m_bounds[5] = m_spatial.m_working_maximum;
    header.m_grid_dims = m_leaf_grid_dimensions;
    header.m_depth = m_spatial.m_depth;
    header.m_region_id_bound = m_spatial.m_region_id_bound;
    header.m_root_mesh_count = m_root_mesh_count;
    header.m_submesh_count = m_spatial.submesh_count;
    header.m_branch_count = m_branch_count;
    header.m_region_count = m_spatial.m_region_count;
    header.m_leaf_level = m_spatial.m_leaf_level;
    header.m_mesh_total = m_meshCount;
    header.m_leaf_count = m_leaf_count;
    header.m_polygon_count = geometry->m_polygon_count;
    header.m_vertex_count = geometry->vertex_count;
    header.m_surface_count = game_data->m_iNumSurfaces;
    header.m_gd_surface_stream_len = m_gd_surface_stream_len;
    header.m_leaf_polygon_stream_len = polygon_cursor;
    header.m_trigger_count = m_trigger_count;
    header.m_region_list_len = m_region_list_len;
    header.m_region_cell = m_region_cell;
    header.m_max_region_radius = m_spatial.m_max_region_radius;
    header.m_particle_len = m_usMeshParticlesLen;
    header.m_region_grid_cell = m_spatial.m_region_grid_cell;
    header.m_prop_len = m_usMeshPropsLen;
    header.m_prop_count = m_ulNumProps;
    header.m_prop_sun_bits = m_pPropSunBits != 0;
    header.m_particle_count = m_ulNumParticles;
    header.m_kind1_submesh_count = m_kind1_submesh_count;
    header.m_zero = 0;
    if (pre_pathing == 0) {
        header.m_path_nodes = 0;
        header.m_edge_node_count = 0;
    } else {
        header.m_path_nodes = pre_pathing->path_node_count;
        header.m_edge_node_count = pre_pathing->edge_node_count;
    }
    file = FileOpen("NewLevel.oct", FILE_ACCESS_WRITE | FILE_CREATE_ALWAYS, 0);
    if (file == 0) {
        ReportBuildStatus(7, "WriteOctFile: Couldn't create file.\n");
        return 0;
    }
    /* Every write-failure path below returns without FileClose: retail leaks
       the handle on each of them (verified at 0x4686b4 et seq.). */
    if (FileWrite(file, &header, sizeof(header), 0) == 0) {
        ReportBuildStatus(7, "WriteOctFile: Couldn't write tree info.\n");
        return 0;
    }
    FileWrite(file, &sentinel, 4, 0);
    if (FileWrite(file, m_branches, header.m_branch_count * sizeof(W8OctPreTreeBranch), 0) == 0) {
        ReportBuildStatus(7, "WriteOctFile: Couldn't write Node info.\n");
        return 0;
    }
    if (FileWrite(file, m_leaves, header.m_leaf_count * sizeof(W8OctPreTreeLeaf), 0) == 0) {
        ReportBuildStatus(7, "WriteOctFile: Couldn't write Leaves info.\n");
        return 0;
    }
    if (FileWrite(file, m_polygon_index_stream, header.m_leaf_polygon_stream_len * 4, 0) == 0) {
        ReportBuildStatus(7, "WriteOctFile: Couldn't write Poly List info.\n");
        return 0;
    }
    unsigned int grid_cells =
        m_leaf_grid_dimensions.z * m_leaf_grid_dimensions.y * m_leaf_grid_dimensions.x;
    if (grid_cells < 250000 && FileWrite(file, m_leaf_lookup, grid_cells * 4, 0) == 0) {
        ReportBuildStatus(7, "WriteOctFile: Couldn't write uiLeafGrid info.\n");
        return 0;
    }
    if (FileWrite(file, m_aulPolyLookup, header.m_polygon_count * 4, 0) == 0) {
        ReportBuildStatus(7, "WriteOctFile: Couldn't write Poly Lookup table.\n");
        return 0;
    }
    if (header.m_region_list_len != 0 &&
        FileWrite(file, m_region_index_stream, header.m_region_list_len * 2, 0) == 0) {
        ReportBuildStatus(7, "WriteOctFile: Couldn't write region list.\n");
        return 0;
    }
    if (header.m_gd_surface_stream_len != 0 &&
        FileWrite(file, m_gd_surface_index_stream, header.m_gd_surface_stream_len * 4, 0) == 0) {
        ReportBuildStatus(7, "WriteOctFile: Couldn't write Game Data Poly List.\n");
        return 0;
    }
    /* Retail writes this trigger list as 4-byte elements while ReadOctFile
       reads it back as 2-byte elements (0x4688c3 vs 0x42c351): the asymmetry
       is authentic.  In practice the count is always zero. */
    if (header.m_trigger_count != 0 &&
        FileWrite(file, m_trigger_indices, header.m_trigger_count * 4, 0) == 0) {
        ReportBuildStatus(7, "WriteOctFile: Couldn't write Trigger list.\n");
        return 0;
    }
    if (header.m_region_count > 1 &&
        FileWrite(file, m_spatial.m_region_volumes,
                  header.m_region_count * sizeof(W8OctRegionVolume), 0) == 0) {
        ReportBuildStatus(7, "WriteOctFile: Couldn't write Region array.\n");
        return 0;
    }
    FileWrite(file, &sentinel, 4, 0);
    if (header.m_submesh_count != 0) {
        if (FileWrite(file, m_pSubmeshes, (header.m_submesh_count + 1) * sizeof(W8OctSubmesh), 0) ==
            0) {
            ReportBuildStatus(7, "WriteOctFile: Couldn't write submesh array.\n");
            return 0;
        }
        if (m_meshCount != 0 && m_pAlphaBits != 0 && m_pAlphaBits->Save(file) == 0) {
            srAssertFail("m_pAlphaBits->Save(hOctFile)", OCTPRETREE_CPP, 0x224,
                         "ReadOctFile: Failure writing Alpha Bits.");
        }
        if (m_ulNumParticles != 0) {
            if (FileWrite(file, m_pusMeshParticleLookup, header.m_mesh_total * 2 + 2, 0) == 0) {
                ReportBuildStatus(7, "WriteOctFile: Couldn't write Mesh Particle Lookup Table.\n");
                return 0;
            }
            if (FileWrite(file, m_pusMeshParticles,
                          static_cast<unsigned int>(m_usMeshParticlesLen) << 1, 0) == 0) {
                ReportBuildStatus(7, "WriteOctFile: Couldn't write Mesh Particle Link Table.\n");
                return 0;
            }
        }
        if (m_ulNumProps != 0) {
            if (FileWrite(file, m_pusMeshPropLookup, header.m_mesh_total * 2 + 2, 0) == 0) {
                ReportBuildStatus(7, "WriteOctFile: Couldn't write Mesh Prop Lookup Table.\n");
                return 0;
            }
            if (FileWrite(file, m_pusMeshProps, static_cast<unsigned int>(m_usMeshPropsLen) << 1,
                          0) == 0) {
                ReportBuildStatus(7, "WriteOctFile: Couldn't write Mesh Prop Link Table.\n");
                return 0;
            }
        }
    }
    /* Every section terminator is the same 0xffffffff dword: retail keeps one
       -1 local for all of them (verified at 0x468406). */
    if (FileWrite(file, &sentinel, 4, 0) == 0) {
        ReportBuildStatus(7,
                          "WriteOctFile: Couldn't write Terminator after Mesh Prop Link Table.\n");
        return 0;
    }
    if (pre_pathing != 0 && pre_pathing->WritePathNodes(file) == 0) {
        ReportBuildStatus(7, "WriteOctFile: Couldn't write Path Nodes.\n");
        return 0;
    }
    if (m_ulNumProps != 0 && m_pPropSunBits != 0 && m_pPropSunBits->Save(file) == 0) {
        ReportBuildStatus(7, "WriteOctFile: Couldn't write Prop Sunlight bits array.\n");
        return 0;
    }
    written = FileWrite(file, &sentinel, 4, 0);
    if (written == 0) {
        ReportBuildStatus(7, "WriteOctFile: Couldn't write Terminator field.\n");
        return 0;
    }
    /* game_data was already dereferenced unconditionally filling the header
       above (verified at 0x468572); this null check is authentic but
       unreachable-with-null. */
    if (game_data != 0 && header.m_gd_surface_stream_len != 0) {
        written = game_data->WriteGameData(file);
        /* Retail bitwise-ORs the game-data and terminator results: a failed
           game-data write followed by a successful four-byte write reports
           success.  Verified at 0x468be0-0x468bec. */
        written |= FileWrite(file, &sentinel, 4, 0);
        if (written == 0) {
            ReportBuildStatus(7, "WriteOctFile: Couldn't write final Terminator field.\n");
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
        if (records[index].m_vertex_ids != 0) {
            free(records[index].m_vertex_ids);
        }
        if (records[index].m_polygon_ids != 0) {
            free(records[index].m_polygon_ids);
        }
    }
}

/* Builds the OctMeshModel array from the split records: partitions the
   geometry through AllocateSubMesh/SplitMeshes, transfers the per-submesh
   vertex/polygon data, counts the per-kind totals and marks the alpha-bit
   rows whose packed header is nonzero. */
// FUNCTION: WIZ8 0x00468c30
OctMeshModel* OctPreTree::CreateSubMeshes(W8OctPreTreeGeometry* geometry)
{
    OctMeshModel* models;
    W8OctSubmeshBuild* records;
    unsigned int kind_counts[4];
    unsigned int root_count;
    unsigned int record_index;
    unsigned int model_index;

    if (m_spatial.submesh_count == 0) {
        return 0;
    }
    m_vertex_count = geometry->vertex_count;
    VerifyPolygonRegions();
    /* Three build records per original unit.  SplitMeshes
       appends at most one record per source on each of its three kind
       passes while an emptied source keeps its slot, so the true slot
       bound is count + 3*(count-1) - covered only while count <= 6. */
    records = static_cast<W8OctSubmeshBuild*>(
        malloc((m_spatial.submesh_count + 1) * 3 * sizeof(W8OctSubmeshBuild)));
    if (records == 0) {
        ReportBuildStatus(7, "\nCreateSubMeshes: Could not allocate submeshes.\n");
    } else {
        memset(records, 0, (m_spatial.submesh_count + 1) * 3 * sizeof(W8OctSubmeshBuild));
        AllocateSubMesh(records);
        SplitMeshes(geometry, records);
        m_aulPolyLookup = static_cast<unsigned long*>(malloc(geometry->m_polygon_count * 4 + 4));
        if (m_aulPolyLookup == 0) {
            ReportBuildStatus(7, "\nCreateSubMeshes: Could not allocate m_aulPolyLookup.\n");
            FreeSubmeshBuildArrays(records, m_spatial.submesh_count);
        } else {
            m_pSubmeshes = static_cast<W8OctSubmesh*>(
                malloc((m_spatial.submesh_count + 1) * sizeof(W8OctSubmesh)));
            models = new OctMeshModel[m_spatial.submesh_count + 1];
            if (m_pSubmeshes != 0 && models != 0) {
                memset(m_pSubmeshes, 0, (m_spatial.submesh_count + 1) * sizeof(W8OctSubmesh));
                root_count = 0;
                kind_counts[3] = 0;
                kind_counts[2] = 0;
                kind_counts[1] = 0;
                kind_counts[0] = 0;
                record_index = 1;
                model_index = 0;
                if (m_spatial.submesh_count > 1) {
                    W8OctSubmeshBuild* record = records + 1;
                    W8OctSubmesh* submesh = m_pSubmeshes + 1;
                    /* Retail writes models from index 0: records[1..] and
                       submeshes[1..] are one-based, but the model cursor
                       starts at models[0]. */
                    OctMeshModel* model = models;
                    do {
                        if (record->m_polygon_count == 0) {
                            ReportBuildStatus(7, "CreateSubMeshes: Found mesh with no polys.\n");
                            FreeSubmeshBuildArrays(records, m_spatial.submesh_count);
                            goto cleanup;
                        }
                        submesh->mesh = model_index;
                        submesh->polygon_count = record->m_polygon_count;
                        submesh->next_link = record->m_next_link;
                        model->m_packed_header = record->m_kind;
                        model->version = m_sun_count;
                        kind_counts[record->m_kind] += 1;
                        model->m_vertex_count = record->vertex_count;
                        model->m_map_count = record->m_map_count;
                        model->m_polygon_count = record->m_polygon_count;
                        model->m_link_index = record->m_prev_link - 1;
                        if (record->m_prev_link == 0) {
                            ++root_count;
                        }
                        model->next_link = record->m_next_link - 1;
                        model->m_vertex_locations = static_cast<srVector3T<float>*>(
                            srHeap.allocate(record->vertex_count * 0xc));
                        model->m_vertex_map = record->m_uv_map;
                        model->m_poly_vertices = record->m_poly_vertices;
                        model->m_poly_uv_index = record->m_poly_uv_index;
                        model->m_poly_equations = static_cast<srVector4T<float>*>(
                            srHeap.allocate(record->m_polygon_count << 4));
                        model->m_vertex_normals = static_cast<srVector3T<float>*>(
                            srHeap.allocate(record->vertex_count * 0xc));
                        model->m_vertex_lights = static_cast<srVector3T<float>*>(
                            srHeap.allocate(record->vertex_count * 0xc));
                        model->m_vertex_materials =
                            static_cast<int*>(malloc(record->vertex_count << 2));
                        model->m_poly_textures =
                            static_cast<int*>(malloc(record->m_polygon_count << 2));
                        if (m_sun_count != 0) {
                            model->m_sun_lights =
                                static_cast<float**>(malloc(static_cast<int>(m_sun_count) << 2));
                            if (model->m_sun_lights == 0) {
                                ReportBuildStatus(7, "\nCreateSubMeshes: Could not allocate "
                                                     "ppsrSunLights.\n");
                                FreeSubmeshBuildArrays(records, m_spatial.submesh_count);
                                goto cleanup;
                            }
                            for (short sun = 0; sun < static_cast<short>(m_sun_count); ++sun) {
                                model->m_sun_lights[sun] =
                                    static_cast<float*>(malloc(record->vertex_count << 2));
                                if (model->m_sun_lights[sun] == 0) {
                                    ReportBuildStatus(7, "\nCreateSubMeshes: Could not allocate "
                                                         "ppsrSunLights array.\n");
                                    FreeSubmeshBuildArrays(records, m_spatial.submesh_count);
                                    goto cleanup;
                                }
                            }
                        }
                        if (model->m_vertex_locations == 0 || model->m_vertex_map == 0 ||
                            model->m_poly_vertices == 0 || model->m_poly_equations == 0 ||
                            model->m_vertex_normals == 0 || model->m_vertex_lights == 0 ||
                            model->m_vertex_materials == 0 || model->m_poly_textures == 0) {
                            ReportBuildStatus(7, "\nCreateSubMeshes: Could not allocate mesh "
                                                 "model arrays.\n");
                            FreeSubmeshBuildArrays(records, m_spatial.submesh_count);
                            goto cleanup;
                        }
                        for (unsigned long polygon = 0; polygon < record->m_polygon_count;
                             ++polygon) {
                            unsigned long id = record->m_polygon_ids[polygon];
                            m_aulPolyLookup[id] = model_index * 0x10000 + polygon;
                            model->m_poly_textures[polygon] = geometry->m_polygons[id].texture;
                            model->m_poly_equations[polygon].x =
                                geometry->m_polygons[id].plane.normal.x;
                            model->m_poly_equations[polygon].y =
                                geometry->m_polygons[id].plane.normal.y;
                            model->m_poly_equations[polygon].z =
                                geometry->m_polygons[id].plane.normal.z;
                            model->m_poly_equations[polygon].w =
                                geometry->m_polygons[id].plane.w;
                        }
                        model->m_material_index =
                            geometry->m_vertices[record->m_vertex_ids[0]].m_material;
                        for (unsigned long vertex = 0; vertex < record->vertex_count; ++vertex) {
                            W8OctPreTreeVertex* source =
                                &geometry->m_vertices[record->m_vertex_ids[vertex]];
                            model->m_vertex_locations[vertex] = source->position;
                            model->m_vertex_normals[vertex] = source->m_normal;
                            model->m_vertex_lights[vertex] = source->m_light;
                            model->m_vertex_materials[vertex] = source->m_material;
                            for (short sun = 0; sun < static_cast<short>(m_sun_count); ++sun) {
                                model->m_sun_lights[sun][vertex] = source->m_sun_lights[sun];
                            }
                            if (model->m_vertex_materials[vertex] != model->m_material_index) {
                                model->m_material_index = -1;
                            }
                        }
                        ++record;
                        ++submesh;
                        ++model;
                        ++model_index;
                        ++record_index;
                    } while (record_index < m_spatial.submesh_count);
                }
                m_spatial.submesh_count = model_index;
                m_meshCount = root_count;
                m_root_mesh_count = kind_counts[0];
                m_kind1_submesh_count = kind_counts[1];
                m_pAlphaBits = new BitArray(m_meshCount);
                if (root_count > 1) {
                    for (unsigned int index = 1; index < root_count; ++index) {
                        if (models[index].m_packed_header != 0) {
                            m_pAlphaBits->Set(index);
                        }
                    }
                }
                /* Verified retail behavior: submesh_count was just overwritten
                   with model_index, so the cleanup frees records[0..model_index)
                   and leaks the final record's vertex/polygon id arrays. */
                FreeSubmeshBuildArrays(records, m_spatial.submesh_count);
                free(records);
                return models;
            }
            ReportBuildStatus(7, "\nCreateSubMeshes: Could not allocate mesh arrays.\n");
            FreeSubmeshBuildArrays(records, m_spatial.submesh_count);
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
unsigned long OctPreTree::SplitMeshes(W8OctPreTreeGeometry* geometry, W8OctSubmeshBuild* records)
{
    unsigned long count = m_spatial.submesh_count;
    if (count == 0) {
        return 0;
    }

    unsigned long max_polygons = 0;
    if (count > 1) {
        for (unsigned long index = 1; index < count; ++index) {
            if (max_polygons < records[index].m_polygon_count) {
                max_polygons = records[index].m_polygon_count;
            }
        }
    }
    unsigned long* staging = static_cast<unsigned long*>(malloc((max_polygons + 5) * 4));
    int* vertex_ids = static_cast<int*>(malloc((max_polygons + 5) * 0xc));
    unsigned long* keys = static_cast<unsigned long*>(malloc((max_polygons + 5) * 0xc));
    unsigned long* scratch = static_cast<unsigned long*>(malloc((max_polygons + 5) * 0xc));
    unsigned long* order = static_cast<unsigned long*>(malloc((max_polygons + 5) * 0xc));
    if (staging == 0 || vertex_ids == 0 || keys == 0 || scratch == 0 || order == 0) {
        ReportBuildStatus(7, "SplitMeshes: Error in allocating sort arrays.\n");
        return 0;
    }

    m_vertex_count = geometry->vertex_count;
    count = m_spatial.submesh_count;
    m_meshCount = count;
    m_root_mesh_count = count;
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
            if (record->m_polygon_count == 0) {
                if (kind == 1) {
                    --m_root_mesh_count;
                }
                continue;
            }
            record->index = index;
            record->m_kind = 0;
            record->m_prev_link = 0;
            for (unsigned long face = 0; face < record->m_polygon_count; ++face) {
                unsigned long id = record->m_polygon_ids[face];
                if (geometry->m_polygons[id].kind == kind) {
                    staging[target->m_polygon_count] = id;
                    ++target->m_polygon_count;
                    record->m_polygon_ids[face] = 0;
                }
            }
            if (target->m_polygon_count != 0) {
                target->m_polygon_ids = static_cast<int*>(malloc(target->m_polygon_count * 4 + 4));
                if (target->m_polygon_ids == 0) {
                    ReportBuildStatus(
                        7, "SplitMeshes: Could not allocate pMeshes[uiFinal].aulFaces.\n");
                    return 0;
                }
                memcpy(target->m_polygon_ids, staging, target->m_polygon_count * 4);
                target->m_polygon_ids[target->m_polygon_count] = 0;
                target->m_kind = kind;
                unsigned long tail = index;
                unsigned long link = record->m_next_link;
                while (link != 0) {
                    tail = link;
                    link = records[tail].m_next_link;
                }
                records[tail].m_next_link = next_free;
                target->m_prev_link = tail;
            }
            unsigned long kept = 0;
            for (unsigned long i = 0; i < record->m_polygon_count; ++i) {
                int id = record->m_polygon_ids[i];
                if (id != 0) {
                    record->m_polygon_ids[kept] = id;
                    ++kept;
                }
            }
            record->m_polygon_count = kept;
            if (kept == 0) {
                --m_root_mesh_count;
            }
            if (target->m_polygon_count != 0) {
                ++next_free;
                ++kind_counts[kind];
            }
        }
    }

    if (m_spatial.submesh_count == m_root_mesh_count) {
        m_spatial.submesh_count = next_free;
    } else {
        m_spatial.submesh_count = next_free;
        unsigned long new_index = 1;
        if (next_free > 1) {
            for (unsigned long index = 1; index < next_free; ++index) {
                unsigned long slot = index;
                while (slot != 0) {
                    W8OctSubmeshBuild* candidate = records + slot;
                    if (candidate->m_polygon_count == 0) {
                        --kind_counts[candidate->m_kind];
                        --m_spatial.submesh_count;
                        slot = candidate->m_next_link;
                        candidate = records + slot;
                        candidate->flags |= 1;
                        if (candidate->m_prev_link != 0) {
                            candidate->m_prev_link = records[candidate->m_prev_link].m_prev_link;
                        }
                        continue;
                    }
                    if (slot != index || (candidate->flags & 1) == 0) {
                        if (slot > 1) {
                            for (unsigned long i = 1; i < slot; ++i) {
                                if (records[i].m_next_link == slot) {
                                    records[i].m_next_link = new_index;
                                }
                            }
                        }
                        if (slot + 1 < next_free) {
                            for (unsigned long i = slot + 1; i < next_free; ++i) {
                                if (records[i].m_prev_link == slot) {
                                    records[i].m_prev_link = new_index;
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
    if (m_spatial.submesh_count > 1) {
        for (unsigned long index = 1; index < m_spatial.submesh_count; ++index) {
            W8OctSubmeshBuild* record = records + index;
            record->m_poly_vertices =
                static_cast<srVector3i*>(srHeap.allocate(record->m_polygon_count * 0xc));
            if (record->m_poly_vertices == 0) {
                ReportBuildStatus(7, "SplitMeshes: Could not allocate psrPolyVertex.\n");
                return 0;
            }
            unsigned long vertex_count = 0;
            for (unsigned long poly = 0; poly < record->m_polygon_count; ++poly) {
                unsigned long id = record->m_polygon_ids[poly];
                unsigned long texture = geometry->m_polygons[id].texture;
                scratch[poly] = texture;
                keys[poly] = texture;
                for (unsigned long corner = 0; corner < 3; ++corner) {
                    unsigned long vertex_id =
                        geometry->m_polygons[id].vertices[corner]->m_vertex_index;
                    unsigned long slot = 0;
                    while (slot < vertex_count && vertex_ids[slot] != static_cast<int>(vertex_id)) {
                        ++slot;
                    }
                    if (slot == vertex_count) {
                        vertex_ids[vertex_count] = vertex_id;
                    }
                    (&record->m_poly_vertices[poly].x)[corner] = slot;
                    if (slot == vertex_count) {
                        ++vertex_count;
                    }
                }
            }
            record->vertex_count = vertex_count;

            QuickSortByKey(record->m_polygon_ids, keys, 0,
                           static_cast<int>(record->m_polygon_count) - 1);
            QuickSortByKey(record->m_poly_vertices, scratch, 0,
                           static_cast<int>(record->m_polygon_count) - 1);

            for (unsigned long i = 0; i < vertex_count; ++i) {
                unsigned long material = geometry->m_vertices[vertex_ids[i]].m_material;
                scratch[i] = material;
                keys[i] = material;
                order[i] = i;
            }
            QuickSortByKey(vertex_ids, scratch, 0, static_cast<int>(vertex_count) - 1);
            memcpy(scratch, order, vertex_count * 4);
            QuickSortByKey(scratch, keys, 0, static_cast<int>(vertex_count) - 1);
            QuickSortByKey(order, scratch, 0, static_cast<int>(vertex_count) - 1);

            for (unsigned long face = 0; face < record->m_polygon_count; ++face) {
                for (int c = 0; c < 3; ++c) {
                    int* slot = &(&record->m_poly_vertices[face].x)[c];
                    *slot = order[*slot];
                }
            }

            record->m_vertex_ids = static_cast<int*>(malloc(vertex_count * 4 + 4));
            if (record->m_vertex_ids == 0) {
                ReportBuildStatus(7, "SplitMeshes: Could not allocate aulVertices.\n");
                return 0;
            }
            memcpy(record->m_vertex_ids, vertex_ids, vertex_count * 4);
            total_vertices += vertex_count;
            total_maps += SplitUVMaps(record, geometry);
        }
    }

    char text[1024];
    sprintf(text,
            "Vertex UV Map Split: %d original vertices, %d final UV map Entries.\n\t"
            "Split Ratio: %3.1f to 1.\n",
            static_cast<int>(total_vertices), static_cast<int>(total_maps),
            static_cast<double>(total_maps) / static_cast<int>(total_vertices));
    ReportBuildStatus(6, text);
    m_root_mesh_count = kind_counts[0];
    m_kind1_submesh_count = kind_counts[1] + kind_counts[0];
    free(staging);
    free(vertex_ids);
    free(keys);
    free(scratch);
    free(order);
    VerifyAutoMeshes(geometry, records);
    return m_spatial.submesh_count;
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

// FUNCTION: WIZ8 0x0046a4b0
unsigned long OctPreTree::SplitUVMaps(W8OctSubmeshBuild* record, W8OctPreTreeGeometry* geometry)
{
    W8OctUvPoolEntry* table =
        static_cast<W8OctUvPoolEntry*>(malloc(record->m_polygon_count * 0x30));
    if (table == 0) {
        ReportBuildStatus(7, "SplitUVMaps: Could not allocate pUVMaps.\n");
        return 0;
    }
    memset(table, 0, record->m_polygon_count * 0x30);
    srVector3i* uv_index = static_cast<srVector3i*>(srHeap.allocate(record->m_polygon_count * 0xc));
    if (uv_index == 0) {
        ReportBuildStatus(7, "SplitUVMaps: Could not allocate psrPolyUVIndex.\n");
        return 0;
    }
    for (unsigned long i = 0; i < record->vertex_count; ++i) {
        table[i].link = -1;
    }
    unsigned long uv_count = record->vertex_count;
    for (unsigned long poly = 0; poly < record->m_polygon_count; ++poly) {
        const srVector2T<float>* uvs =
            geometry->m_polygons[record->m_polygon_ids[poly]].face.texture_coordinates;
        for (unsigned long corner = 0; corner < 3; ++corner) {
            int vertex = (&record->m_poly_vertices[poly].x)[corner];
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
                if (static_cast<int>(record->m_polygon_count * 4) <= static_cast<int>(uv_count)) {
                    ReportBuildStatus(7, "SplitUVMaps: UV Map count too high.\n");
                    free(table);
                    return 0;
                }
                (&uv_index[poly].x)[corner] = uv_count;
                table[uv_count].link = 0;
                table[uv_count].u = uvs[corner].x;
                table[uv_count].v = uvs[corner].y;
                if (static_cast<int>(record->m_polygon_count * 4) <= last || last < 0) {
                    ReportBuildStatus(7, "SplitUVMaps: Counter value iLast too high.\n");
                    free(table);
                    return 0;
                }
                entry->link = uv_count;
                ++uv_count;
            }
        }
    }
    record->m_poly_uv_index = uv_index;
    record->m_map_count = uv_count;
    record->m_uv_map = static_cast<srVector2T<float>*>(srHeap.allocate(uv_count * 8));
    if (record->m_uv_map == 0) {
        ReportBuildStatus(7, "SplitUVMaps: Could not allocate pMesh->psrMaps.\n");
        free(table);
        return 0;
    }
    for (unsigned long uv = 0; uv < uv_count; ++uv) {
        record->m_uv_map[uv].Set(table[uv].u, table[uv].v);
    }
    free(table);
    return uv_count;
}

/* First CreateSubMeshes phase: counts each region's polygons into its build
   record, allocates the polygon id run and refills it while tracking the
   corner bounds, then checks every automesh cell keyed by the region id
   overlaps those bounds. */
// FUNCTION: WIZ8 0x0046a790
unsigned long OctPreTree::AllocateSubMesh(W8OctSubmeshBuild* records)
{
    if (m_spatial.m_polygon_count > 1) {
        for (unsigned long poly = 1; poly < m_spatial.m_polygon_count; ++poly) {
            unsigned short region = game_data->m_polygons[poly].region;
            if (region == 0 || region >= m_spatial.submesh_count) {
                char text[1024];
                sprintf(text, "Polygon %d in invalid submesh %d\n", static_cast<int>(poly),
                        static_cast<unsigned int>(region));
                ReportBuildStatus(6, text);
            } else {
                ++records[region].m_polygon_count;
            }
        }
    }
    for (unsigned long index = 1; index < m_spatial.submesh_count; ++index) {
        W8OctSubmeshBuild* record = records + index;
        /* Retail initialises the bounds only on this path; for empty records
           the cell check below reads whatever the stack held. */
        srVector3T<float> minimum;
        srVector3T<float> maximum;
        if (record->m_polygon_count != 0) {
            minimum = 1e+06f;
            maximum = -1e+06f;
            record->m_polygon_ids = static_cast<int*>(malloc(record->m_polygon_count * 4 + 8));
            if (record->m_polygon_ids == 0) {
                ReportBuildStatus(7, "\nAllocateSubMesh: Could not allocate aulFaces.\n");
                return 0;
            }
            memset(record->m_polygon_ids, 0, record->m_polygon_count * 4 + 8);
            unsigned short found = 0;
            if (m_spatial.m_polygon_count > 1) {
                for (unsigned long poly = 1; poly < m_spatial.m_polygon_count; ++poly) {
                    if (game_data->m_polygons[poly].region == index) {
                        record->m_polygon_ids[found] = poly;
                        ++found;
                        if (found == 5000) {
                            ReportBuildStatus(
                                6, "One of your regions has more than 5000 polys in it!\n");
                        }
                        for (int corner = 0; corner < 3; ++corner) {
                            const srVector3T<float>* position =
                                &game_data->m_polygons[poly].vertices[corner]->position;
                            if (maximum.x < position->x) {
                                maximum.x = position->x;
                            }
                            if (position->x < minimum.x) {
                                minimum.x = position->x;
                            }
                            if (maximum.y < position->y) {
                                maximum.y = position->y;
                            }
                            if (position->y < minimum.y) {
                                minimum.y = position->y;
                            }
                            if (maximum.z < position->z) {
                                maximum.z = position->z;
                            }
                            if (position->z < minimum.z) {
                                minimum.z = position->z;
                            }
                        }
                    }
                }
            }
            if (record->m_polygon_count != found) {
                ReportBuildStatus(
                    6, "Mismatch between expected number of mesh polys and actual number");
            }
            record->m_polygon_count = found;
        }
        unsigned short key = static_cast<unsigned short>(index);
        W8HashTable<unsigned short, unsigned long>* cells = automesh_cells;
        int slot = cells->FindNextEntry(&key, -1);
        while (slot != -1) {
            unsigned long cell = cells->entries[slot].value;
            float cell_x =
                ((cell >> 0x10) & 0xff) * m_spatial.m_region_grid_cell + m_spatial.m_minimum.x;
            float cell_y =
                ((cell >> 8) & 0xff) * m_spatial.m_region_grid_cell + m_spatial.m_minimum.y;
            float cell_z = (cell & 0xff) * m_spatial.m_region_grid_cell + m_spatial.m_minimum.z;
            if (cell_x + m_spatial.m_region_grid_cell < minimum.x || maximum.x < cell_x ||
                cell_y + m_spatial.m_region_grid_cell < minimum.y || maximum.y < cell_y ||
                cell_z + m_spatial.m_region_grid_cell < minimum.z || maximum.z < cell_z) {
                ReportBuildStatus(7, "AutoMesh has no vertices inside region.");
            }
            slot = cells->FindNextEntry(&key, slot);
        }
    }
    return m_spatial.submesh_count;
}

/* Rebuilds the leaf-level mask, then for every polygon assigned to an
   auto-region (region >= region_count) descends to its position's leaf and
   reports when the leaf's region differs from the polygon's. */
// FUNCTION: WIZ8 0x0046abf0
void OctPreTree::VerifyPolygonRegions()
{
    m_region_mask = 0;
    for (unsigned long level = m_spatial.m_leaf_level; level != 0; --level) {
        m_region_mask = m_region_mask * 2 + 1;
    }
    for (unsigned long poly = 1; poly < game_data->m_polygon_count; ++poly) {
        W8OctRegionPolygon* polygon = &game_data->m_polygons[poly];
        if (polygon->region >= m_spatial.m_region_count) {
            unsigned int cell[4];
            cell[0] = m_region_mask;
            cell[1] = static_cast<unsigned int>((polygon->position.x - m_spatial.m_minimum.x) /
                                                m_spatial.m_region_grid_cell);
            cell[2] = static_cast<unsigned int>((polygon->position.y - m_spatial.m_minimum.y) /
                                                m_spatial.m_region_grid_cell);
            cell[3] = static_cast<unsigned int>((polygon->position.z - m_spatial.m_minimum.z) /
                                                m_spatial.m_region_grid_cell);
            int node = DescendByMask(cell);
            if (m_branches[node].region != polygon->region) {
                char text[256];
                sprintf(text, "Poly %d not found in correct region.\n", static_cast<int>(poly));
                ReportBuildStatus(6, text);
            }
        }
    }
}

/* Per root automesh (1..m_meshCount): walks every cell hashed under the
   mesh id, descends to its node, reports a region mismatch, and checks the
   cell overlaps the vertex bounds of the whole split-record chain. */
// FUNCTION: WIZ8 0x0046ad10
void OctPreTree::VerifyAutoMeshes(W8OctPreTreeGeometry* geometry, W8OctSubmeshBuild* records)
{
    for (unsigned long mesh = 1; mesh < m_meshCount; ++mesh) {
        unsigned short key = static_cast<unsigned short>(mesh);
        int slot = automesh_cells->FindNextEntry(&key, -1);
        while (slot != -1) {
            unsigned int cell[4];
            unsigned long packed = automesh_cells->entries[slot].value;
            cell[0] = packed >> 0x18;
            cell[1] = packed >> 0x10 & 0xff;
            cell[2] = packed >> 8 & 0xff;
            cell[3] = packed & 0xff;
            float cell_x = cell[1] * m_spatial.m_region_grid_cell + m_spatial.m_minimum.x;
            float cell_y = cell[2] * m_spatial.m_region_grid_cell + m_spatial.m_minimum.y;
            float cell_z = cell[3] * m_spatial.m_region_grid_cell + m_spatial.m_minimum.z;
            int node = DescendByMask(cell);
            if (node != 0) {
                if (m_branches[node].region != key) {
                    ReportBuildStatus(7, "Region has wrong automesh.");
                }
                srVector3T<float> minimum(g_float_005ec3c0, 1e+06f, 1e+06f);
                srVector3T<float> maximum(-1e+06f, -1e+06f, -1e+06f);
                for (unsigned long link = mesh; link != 0; link = records[link].m_next_link) {
                    W8OctSubmeshBuild* record = records + link;
                    for (unsigned long i = 0; i < record->vertex_count; ++i) {
                        const srVector3T<float>* position =
                            &geometry->m_vertices[record->m_vertex_ids[i]].position;
                        if (maximum.x < position->x) {
                            maximum.x = position->x;
                        }
                        if (position->x < minimum.x) {
                            minimum.x = position->x;
                        }
                        if (maximum.y < position->y) {
                            maximum.y = position->y;
                        }
                        if (position->y < minimum.y) {
                            minimum.y = position->y;
                        }
                        if (maximum.z < position->z) {
                            maximum.z = position->z;
                        }
                        if (position->z < minimum.z) {
                            minimum.z = position->z;
                        }
                    }
                }
                if (cell_x + m_spatial.m_region_grid_cell < minimum.x || maximum.x < cell_x ||
                    cell_y + m_spatial.m_region_grid_cell < minimum.y || maximum.y < cell_y ||
                    cell_z + m_spatial.m_region_grid_cell < minimum.z || maximum.z < cell_z) {
                    ReportBuildStatus(7, "AutoMesh has no vertices inside region.");
                }
            }
            slot = automesh_cells->FindNextEntry(&key, slot);
        }
    }
}

/* Grid-walks the level bounds cell by cell: snaps each candidate to the
   ground, runs the obstruction probe, appends a 0x10-byte node record to the
   pre-pathing chunk table, and cross-links conditional props. The node map
   keys each (z<<16 | x) cell so one pass emits at most one node per cell
   unless a duplicate key arrives with an empty value. */
// FUNCTION: WIZ8 0x0046b060
unsigned char OctPreTree::BuildPathLists(W8GameData* game_data, W8LevelFile* level,
                                         unsigned int min_component_percent)
{
    W8HashTable<unsigned int, int> node_map;
    W8HashTable<unsigned int, CondPathNode*> cond_map;
    char message[0x400];
    W8PreProp* preprops = 0;
    srVector3T<float> node;

    object_registry = new W8OctreeObjectRegistry;
    SetOctreeGameData(game_data);
    delete m_visited_object_bits;
    m_visited_object_bits = new BitArray(m_spatial.m_item_count + 0x14);
    pre_pathing = new PrePathing;
    pre_pathing->SnapNamedPositions(level->pNamedPositions, level->nNamedPositions,
                                        min_component_percent, this);
    ReportBuildStatus(6, "\nBuilding Path Lists:\n=======================\n");
    path_node_extent = m_region_cell + m_region_cell;
    float level_height = (m_spatial.m_maximum.y - m_spatial.m_minimum.y) * g_path_span_scale;
    int x_cells =
        static_cast<int>((m_spatial.m_maximum.x - m_spatial.m_minimum.x) / m_region_cell) + 1;
    int z_cells =
        static_cast<int>((m_spatial.m_maximum.z - m_spatial.m_minimum.z) / m_region_cell) + 1;

    int prop_count = CreatePathProps(level, &preprops);
    W8PrePathNode* record = pre_pathing->GetPathNode();
    W8PrePathNode* head = record;
    path_node_count = 1;
    int last_percent = 0;
    if (x_cells > 0) {
        float x_cells_f = static_cast<float>(x_cells);
        for (int x = 0; x < x_cells; ++x) {
            int percent = static_cast<int>(x * 100.0f / x_cells_f);
            if (last_percent < percent) {
                ++last_percent;
                sprintf(message, "  %d%% Complete:  %d Path Nodes Created \r", last_percent,
                        path_node_count);
                ReportStartupMessage(message);
            }
            unsigned int cell = static_cast<unsigned int>(x);
            node.x = (x + g_float_005ebc7c) * m_region_cell + m_spatial.m_minimum.x;
            for (int z = 0; z < z_cells; ++z) {
                node.z = (z + g_float_005ebc7c) * m_region_cell + m_spatial.m_minimum.z;
                node.y = m_spatial.m_maximum.y;
                while (SnapToGround(&node, 1)) {
                    m_lNumBlocks = 0;
                    m_lNumSupports = 0;
                    if (current_prop >= 0) {
                        float snapped = node.y;
                        if (!SnapToGround(&node, 0) || g_float_005ec3f8 < fabsf(node.y - snapped)) {
                            m_lSupports[m_lNumSupports] = current_prop;
                            ++m_lNumSupports;
                        }
                        node.y = snapped;
                    }
                    if (PathNodeObstructed(&node) != 1) {
                        W8PrePathNode* next = pre_pathing->GetPathNode();
                        record->next = next;
                        record = next;
                        record->cell = cell;
                        record->y = node.y;
                        record->level_flags =
                            static_cast<unsigned int>(
                                static_cast<int>((node.y - m_spatial.m_minimum.y) / level_height)) +
                            1;
                        if (InsertConditionalNodes(&cond_map, cell, record->level_flags, preprops,
                                                   prop_count)) {
                            record->level_flags |= 0x4000000;
                        }
                        int slot = node_map.FindNextEntry(&cell, -1);
                        if (slot == -1 || node_map.entries[slot].value == 0) {
                            node_map.Insert(&cell, &path_node_count);
                            record->level_flags |= 0x10000000;
                        }
                        ++path_node_count;
                    }
                    node.y -=
                        m_lNumSupports != 0 ? NAVIGATOR_MINIMUM_HORIZONTAL_DISTANCE : g_world_scale;
                }
                cell += 0x10000;
            }
        }
    }
    sprintf(message, "%d Path Nodes Created               \n", path_node_count);
    ReportBuildStatus(6, message);
    if (path_node_count != 0) {
        if (pre_pathing == 0) {
            ReportBuildStatus(7, "Could not create PrePathing object\n");
        }
        pre_pathing->ConfigureForLevel(path_node_count, m_region_cell,
                                           static_cast<int>(m_path_clearance), &m_spatial.m_minimum,
                                           m_owned_0c0);
        /* Verified retail behavior: this early return runs only the two
           local hash-table destructors.  preprops (and its pStopMeshes
           arrays), object_registry and g_octree_game_data are all
           left behind - the registry pointer and global stay live. */
        if (!pre_pathing->BuildPathList(head, &node_map)) {
            return 0;
        }
        pre_pathing->LinkCollideableProps(prop_count, preprops, &cond_map);
        pre_pathing->CreateAutomapNodes(level);
    }
    for (int i = 0; i < prop_count; ++i) {
        /* Verified retail oddity: the binary tests pStopMeshes twice around
           the count check. */
        if (preprops[i].pStopMeshes != 0 && preprops[i].num_stop_meshes != 0 &&
            preprops[i].pStopMeshes != 0) {
            delete[] preprops[i].pStopMeshes;
        }
    }
    free(preprops);
    delete object_registry;
    object_registry = 0;
    SetOctreeGameData(0);
    return 1;
}

/* Probes one node candidate: box-tests the cell against surfaces/props, then
   checks the four footprint corners for ground contact. Sector ids that can
   support a node are deduplicated into m_lSupports (result 6); a prop-blocked
   or floating node yields 1. */
// FUNCTION: WIZ8 0x0046b700
char OctPreTree::PathNodeObstructed(const srVector3T<float>* node)
{
    srVector3T<float> bounds_min, bounds_max;
    srVector3T<float> corner;
    char result;
    bool probe;

    /* The serialized +0x17c header word is a float the pathing code reads
       bit-wise: the probe-box height above the node. */
    float clearance;
    memcpy(&clearance, &m_path_clearance, sizeof(clearance));
    bounds_min.y = node->y + clearance * g_navigator_mode3_scale;
    bounds_max.y = bounds_min.y + clearance;
    float half = m_region_cell * g_float_005ebc7c;
    bounds_min.x = node->x - half;
    bounds_max.x = node->x + half;
    bounds_min.z = node->z - half;
    bounds_max.z = bounds_min.z + half + half;

    result = TestPathPropBounds(&bounds_min, &bounds_max);
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
            corner.y = bounds_min.y + g_world_scale;
            out = SnapToGround(&corner, 1);
            if (out == 0) {
                out = 1;
            } else if (current_prop != -1 ||
                       fabsf(node->y - corner.y) <=
                           clearance * static_cast<float>(g_double_005ebe80)) {
                out = result;
                if (current_prop >= 0) {
                    corner.y = bounds_min.y + g_world_scale;
                    probe = SnapToGround(&corner, 0);
                    if (probe == 0 || clearance * static_cast<float>(g_double_005ebe80) <
                                          fabsf(node->y - corner.y)) {
                        for (int b = 0; b < m_lNumBlocks && result != 1; ++b) {
                            if (current_prop == m_lBlocks[b])
                                result = 1;
                        }
                        out = result;
                        if (result != 1) {
                            bool absent = true;
                            for (int s = 0; s < m_lNumSupports; ++s) {
                                if (current_prop == m_lSupports[s])
                                    absent = false;
                            }
                            if (absent) {
                                m_lSupports[m_lNumSupports] = current_prop;
                                ++m_lNumSupports;
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
    /* Verified retail order: the appends above write m_lSupports[30]/
       m_lBlocks[30] before this check ever runs, so a 31st entry goes out
       of bounds before the assertion fires. */
    if (m_lNumSupports > 29 || m_lNumBlocks > 29) {
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
unsigned char OctPreTree::InsertConditionalNodes(W8HashTable<unsigned int, CondPathNode*>* nodes,
                                                 unsigned int cell, unsigned int node,
                                                 W8PreProp* preprops, int preprop_count)
{
    if (m_lNumSupports == 0 && m_lNumBlocks == 0)
        return 0;

    for (int s = 0; s < m_lNumSupports; ++s) {
        int prop_id = m_lSupports[s];
        bool found = false;
        for (int p = 0; p < preprop_count && !found; ++p) {
            W8PreProp* pp = preprops + p;
            if (pp->num_stop_meshes != 0 &&
                static_cast<int>(pp->first_prop_number) <= prop_id &&
                prop_id < static_cast<int>(pp->first_prop_number + pp->num_stop_meshes)) {
                unsigned int key =
                    (static_cast<unsigned int>(
                         pp->pStopMeshes[prop_id - pp->first_prop_number].m_prop_number)
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
    for (int s2 = 0; s2 < m_lNumBlocks; ++s2) {
        int prop_id = m_lBlocks[s2];
        bool found = false;
        for (int p2 = 0; p2 < preprop_count && !found; ++p2) {
            W8PreProp* pp = preprops + p2;
            if (pp->num_stop_meshes != 0 &&
                static_cast<int>(pp->first_prop_number) <= prop_id &&
                prop_id < static_cast<int>(pp->first_prop_number + pp->num_stop_meshes)) {
                unsigned int key =
                    (static_cast<unsigned int>(
                         pp->pStopMeshes[prop_id - pp->first_prop_number].m_prop_number)
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
char OctPreTree::TestPathPropBounds(const srVector3T<float>* minimum,
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
        W8GDSurface* surface = g_octree_game_data->m_pSurfaces + ids[i];
        if ((surface->flags & 0x1080) == 0) {
            triangle[0] = g_octree_game_data->m_pVertices[surface->vertex_indices[0]];
            triangle[1] = g_octree_game_data->m_pVertices[surface->vertex_indices[1]];
            triangle[2] = g_octree_game_data->m_pVertices[surface->vertex_indices[2]];
            hit = TestSpatialTriangle(bounds, triangle, surface->Normal());
        }
    }
    current_prop = -1;
    if (hit != 0)
        return 1;

    count = QueryObjects(&ids, minimum, maximum, 8, -1);
    bool prop_hit = false;
    for (int k = 0; k < count; ++k) {
        int id = ids[k];
        GDProp* prop = *props->GetAt(id);
        if (prop->BoundsOverlap(minimum, maximum) != 0) {
            prop_hit = true;
            if ((prop->m_flags & 1) != 0)
                return 1;
            /* Dead in retail: current_prop was just set to -1 above and
               QueryObjects never republishes it, while support entries are
               registered prop ids >= 0 - the comparison can never fire. */
            if (m_lNumSupports != 0 && current_prop == m_lSupports[0])
                return 1;
            m_lBlocks[m_lNumBlocks] = id;
            ++m_lNumBlocks;
        }
    }
    if (prop_hit)
        return 3;
    return 0;
}

/* Builds one W8PreProp per level prop flagged for pathing: allocates the
   per-frame GDPreProp stop meshes, applies each anim frame to seed bounds,
   and registers every element in props. Returns the record count and the
   malloc'd array through `preprops`. */
// FUNCTION: WIZ8 0x0046c0f0
int OctPreTree::CreatePathProps(W8LevelFile* level, W8PreProp** preprops)
{
    int count = level->nProps;
    unsigned short prop_number = 0;
    W8PreProp* records = static_cast<W8PreProp*>(malloc(count * sizeof(W8PreProp)));
    memset(records, 0, count * sizeof(W8PreProp));
    if (count < 1) {
        *preprops = records;
    } else {
        W8BoundingBox bounds;
        for (int i = 0; i < count; ++i) {
            W8LevelFileProp* prop = level->pProps + i;
            W8PreProp* record = records + i;
            if ((prop->flags & 1) == 0)
                continue;
            if (prop->num_frame_pos == 0) {
                record->num_stop_meshes = PropFramesDiffer(&prop->anim_obj, 0, 0xffff) ? 2 : 1;
                record->first_prop_number = prop_number;
                record->pStopMeshes = new GDPreProp[record->num_stop_meshes];
                if (record->pStopMeshes == 0) {
                    srAssertFail("pPreProps[i].pStopMeshes", OCTPRETREE_CPP, 0x88e,
                                 "CreatePathProps: Couldn't allocate GDPreProp objects.");
                }
                strcpy(record->name, prop->name);
                record->pStopMeshes[0].ApplyAnimFrame(0, &prop->anim_obj);
                record->pStopMeshes[0].ComputeBounds(&bounds.minimum, &bounds.maximum);
                AddCollidablePropBounds(prop_number, &bounds);
                props->Add(record->pStopMeshes);
                if (record->num_stop_meshes == 2) {
                    record->pStopMeshes[1].ApplyAnimFrame(0xffff, &prop->anim_obj);
                    record->pStopMeshes[1].ComputeBounds(&bounds.minimum, &bounds.maximum);
                    AddCollidablePropBounds(static_cast<unsigned short>(prop_number + 1), &bounds);
                    props->Add(record->pStopMeshes + 1);
                    prop_number += 2;
                } else {
                    record->pStopMeshes[0].m_flags |= 1;
                    unsigned short last = 0xffff;
                    if (prop->anim_obj.num_transforms > 0) {
                        W8LevelFileTransform* t = prop->anim_obj.pTransforms;
                        for (int t_i = prop->anim_obj.num_transforms; t_i != 0; --t_i, ++t) {
                            if (t->pathAI.path_count <= static_cast<int>(last)) {
                                last = static_cast<unsigned short>(t->pathAI.path_count - 1);
                            }
                        }
                    }
                    record->pStopMeshes[0].last_frame = last;
                    ++prop_number;
                }
            } else {
                record->num_stop_meshes = static_cast<unsigned short>(prop->num_frame_pos);
                record->first_prop_number = prop_number;
                strcpy(record->name, prop->name);
                record->pStopMeshes = new GDPreProp[record->num_stop_meshes];
                if (record->pStopMeshes == 0) {
                    srAssertFail("pPreProps[i].pStopMeshes", OCTPRETREE_CPP, 0x8ad,
                                 "CreatePathProps: Couldn't allocate GDPreProp objects.");
                }
                for (unsigned short j = 0; j < record->num_stop_meshes; ++j) {
                    unsigned short frame = prop->usFrame_Pos[j].frame;
                    if (static_cast<unsigned short>(prop->bNumFrames) <= frame) {
                        srAssertFail(
                            "(pLVL->pProps[i].usFrame_Pos[j*2] < "
                            "(UINT16)(pLVL->pProps[i].bNumFrames))", /* c-style-cast-ok: verbatim
                                retail assertion text, kept for .rdata match */
                            OCTPRETREE_CPP, 0x8b4,
                            reinterpret_cast<const char*>( // reinterpret-ok: String returns UINT8*
                                String("%s Prop Error:Segment frame number %d is out of range",
                                       prop->name, frame)));
                    }
                    record->pStopMeshes[j].ApplyAnimFrame(frame, &prop->anim_obj);
                    record->pStopMeshes[j].ComputeBounds(&bounds.minimum, &bounds.maximum);
                    AddCollidablePropBounds(prop_number, &bounds);
                    ++prop_number;
                    props->Add(record->pStopMeshes + j);
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
char OctPreTree::PropFramesDiffer(W8LevelFileAnimObj* anim, unsigned short first,
                                  unsigned short last)
{
    char count = anim->num_transforms;
    if (count > 0) {
        W8LevelFileTransform* t = anim->pTransforms;
        for (int i = count; i != 0; --i, ++t) {
            if (t->pathAI.path_count <= static_cast<int>(last)) {
                last = static_cast<unsigned short>(t->pathAI.path_count - 1);
            }
        }
    }
    bool differ = 0;
    if (count > 0) {
        W8LevelFileTransform* t = anim->pTransforms;
        for (int i = count; i != 0; --i, ++t) {
            W8LevelFileScaledPathNode a, b;
            if (t->pathAI.scaled == 2) {
                a = t->pathAI.pScaledPaths[first];
                b = t->pathAI.pScaledPaths[last];
                if (g_camera_snap_epsilon < fabsf(a.scale.x - b.scale.x) ||
                    g_camera_snap_epsilon < fabsf(a.scale.y - b.scale.y) ||
                    g_camera_snap_epsilon < fabsf(a.scale.z - b.scale.z)) {
                    differ = 1;
                }
            } else {
                a.path = t->pathAI.pPaths[first];
                b.path = t->pathAI.pPaths[last];
            }
            if (g_float_005ebc7c < fabsf(a.path.position.x - b.path.position.x) ||
                g_float_005ebc7c < fabsf(a.path.position.y - b.path.position.y) ||
                g_float_005ebc7c < fabsf(a.path.position.z - b.path.position.z)) {
                differ = 1;
            }
            if (g_camera_snap_epsilon < fabsf(a.path.angle - b.path.angle) ||
                g_camera_snap_epsilon < fabsf(a.path.axis.x - b.path.axis.x) ||
                g_camera_snap_epsilon < fabsf(a.path.axis.y - b.path.axis.y) ||
                g_camera_snap_epsilon < fabsf(a.path.axis.z - b.path.axis.z)) {
                if (!(fabsf((b.path.angle + a.path.angle) - g_camera_half_pi) <= g_float_005ebc3c &&
                      fabsf(b.path.axis.x + a.path.axis.x) <= g_camera_snap_epsilon &&
                      fabsf(b.path.axis.y + a.path.axis.y) <= g_camera_snap_epsilon &&
                      fabsf(b.path.axis.z + a.path.axis.z) <= g_camera_snap_epsilon)) {
                    differ = 1;
                }
            }
        }
    }
    return differ;
}

/* Construct the spatial value used by both the runtime octree and the level
   build tree.  A source value describes the next child: its extent halves and
   its depth advances only when the source is the root-kind record. */
// FUNCTION: WIZ8 0x0046ccc0
W8OctSpatialState::W8OctSpatialState(const W8OctSpatialState* source)
{
    Reset();
    m_level_kind = 1;
    if (source != 0) {
        for (int axis = 0; axis != 3; ++axis) {
            (&m_minimum.x)[axis] = (&source->m_minimum.x)[axis];
            (&m_maximum.x)[axis] = (&source->m_maximum.x)[axis];
            (&m_clipped_minimum.x)[axis] = (&source->m_clipped_minimum.x)[axis];
            (&m_clipped_maximum.x)[axis] = (&source->m_clipped_maximum.x)[axis];
        }
        if (source->m_level_kind == 1) {
            m_extent = source->m_extent * g_float_005ebc7c;
            m_depth = source->m_depth + 1;
        } else {
            m_extent = source->m_extent;
            m_depth = source->m_depth;
        }
        m_cell_size = source->m_cell_size;
        m_polygon_count = source->m_polygon_count;
        m_region_id_bound = source->m_region_id_bound;
        m_leaf_grid_stride_x = source->m_leaf_grid_stride_x;
        m_leaf_grid_stride_y = source->m_leaf_grid_stride_y;
        m_node_extent = source->m_node_extent;
        m_root = source->m_root;
        m_node_index = source->m_node_index;
        m_triangle_vertices = source->m_triangle_vertices;
        flags = source->flags;
        m_item_count = source->m_item_count;
        submesh_count = source->submesh_count;
        m_region_count = source->m_region_count;
        m_leaf_level = source->m_leaf_level;
        m_region_volumes = source->m_region_volumes;
        m_node_extent = source->m_node_extent;
        m_max_region_radius = source->m_max_region_radius;
    }
}

// FUNCTION: WIZ8 0x0046cdc0
void W8OctSpatialState::Reset()
{
    memset(this, 0, sizeof(*this));
}

// FUNCTION: WIZ8 0x0046cdf0
void W8OctSpatialState::GetWorkingBounds(srVector3T<float>* minimum, srVector3T<float>* maximum)
{
    *minimum = m_working_minimum;
    *maximum = m_working_maximum;
}

// FUNCTION: WIZ8 0x0046ce30
void W8OctSpatialState::GetClippedBounds(srVector3T<float>* minimum, srVector3T<float>* maximum)
{
    *minimum = m_clipped_minimum;
    *maximum = m_clipped_maximum;
}

// FUNCTION: WIZ8 0x0046cdd0
W8OctSpatialState::~W8OctSpatialState()
{
    m_region_volumes = 0;
    m_root = 0;
    m_triangle_vertices = 0;
}

/* Strict axis-aligned overlap: touching faces are not an intersection. */
// FUNCTION: WIZ8 0x0046d470
bool BoundsOverlapStrict(const srVector3T<float>* first, const srVector3T<float>* second)
{
    return first[1].x > second[0].x && first[0].x < second[1].x && first[1].y > second[0].y &&
           first[0].y < second[1].y && first[1].z > second[0].z && first[0].z < second[1].z;
}

/* A point on the minimum face is inside; the maximum face is excluded. */
// FUNCTION: WIZ8 0x0046d4d0
bool PointInsideBoxBounds(const srVector3T<float>* bounds, const srVector3T<float>* point)
{
    return bounds[0].x <= point->x && point->x < bounds[1].x && bounds[0].y <= point->y &&
           point->y < bounds[1].y && bounds[0].z <= point->z && point->z < bounds[1].z;
}

/* Test a triangle against an axis-aligned box.  The inexpensive containment
   and separating-axis checks precede explicit triangle-edge intersections
   with all six box faces. */
// FUNCTION: WIZ8 0x0046ce60
unsigned char TestSpatialTriangle(const srVector3T<float>* bounds,
                                  const srVector3T<float>* vertices,
                                  const srVector3T<float>* plane_normal)
{
    const srVector3T<float>& minimum = bounds[0];
    const srVector3T<float>& maximum = bounds[1];

    short vertex_index;
    for (vertex_index = 0; vertex_index < 3; ++vertex_index) {
        const srVector3T<float>& vertex = vertices[vertex_index];
        if (minimum.x <= vertex.x && vertex.x <= maximum.x && minimum.y <= vertex.y &&
            vertex.y <= maximum.y && minimum.z <= vertex.z && vertex.z <= maximum.z) {
            return 1;
        }
    }

    if (plane_normal == 0) {
        return 0;
    }

    bool near_axis = 0;
    srVector3T<float> plane_point;
    for (short axis = 0; axis < 3; ++axis) {
        if ((&vertices[0].x)[axis] < (&minimum.x)[axis] &&
            (&vertices[1].x)[axis] < (&minimum.x)[axis] &&
            (&vertices[2].x)[axis] < (&minimum.x)[axis]) {
            return 0;
        }
        if ((&maximum.x)[axis] < (&vertices[0].x)[axis] &&
            (&maximum.x)[axis] < (&vertices[1].x)[axis] &&
            (&maximum.x)[axis] < (&vertices[2].x)[axis]) {
            return 0;
        }
        if (g_float_005ec414 < static_cast<float>(fabs((&plane_normal->x)[axis])) &&
            (&minimum.x)[axis] <= (&vertices[0].x)[axis] &&
            (&vertices[0].x)[axis] <= (&maximum.x)[axis]) {
            near_axis = 1;
        }
        (&plane_point.x)[axis] =
            ((&vertices[0].x)[axis] + (&vertices[1].x)[axis] + (&vertices[2].x)[axis]) *
            g_float_005ec410;
    }

    if (near_axis == 0) {
        bool negative = 0;
        bool positive = 0;
        for (int x = 0; x != 2; ++x) {
            for (int y = 0; y != 2; ++y) {
                for (int z = 0; z != 2; ++z) {
                    float distance =
                        ((x == 0 ? minimum.x : maximum.x) - plane_point.x) * plane_normal->x +
                        ((y == 0 ? minimum.y : maximum.y) - plane_point.y) * plane_normal->y +
                        ((z == 0 ? minimum.z : maximum.z) - plane_point.z) * plane_normal->z;
                    if (distance <= g_float_zero) {
                        negative = 1;
                    }
                    if (g_float_zero <= distance) {
                        positive = 1;
                    }
                }
            }
        }
        if (negative == 0 || positive == 0) {
            return 0;
        }
    }

    srVector3T<float> edge_start[3];
    srVector3T<float> edge_delta[3];
    for (int edge = 0; edge != 3; ++edge) {
        int next = edge == 2 ? 0 : edge + 1;
        for (int axis = 0; axis != 3; ++axis) {
            (&edge_start[edge].x)[axis] = (&vertices[edge].x)[axis];
            (&edge_delta[edge].x)[axis] = (&vertices[next].x)[axis] - (&vertices[edge].x)[axis];
        }
    }

    for (short face_axis = 0; face_axis < 3; ++face_axis) {
        short first_axis = face_axis == 2 ? 0 : face_axis + 1;
        short second_axis = face_axis == 0 ? 2 : face_axis - 1;
        for (short side = 0; side < 2; ++side) {
            srVector2T<float> intersection[2];
            short intersection_count = 0;
            float face = side == 0 ? (&minimum.x)[face_axis] : (&maximum.x)[face_axis];

            for (short edge = 0; edge < 3; ++edge) {
                if (static_cast<float>(g_double_005ebc70) <
                    static_cast<float>(fabs((&edge_delta[edge].x)[face_axis]))) {
                    float amount = (face - (&edge_start[edge].x)[face_axis]) /
                                   (&edge_delta[edge].x)[face_axis];
                    if (g_float_zero <= amount && amount <= g_float_one) {
                        float first = (&edge_start[edge].x)[first_axis] +
                                      amount * (&edge_delta[edge].x)[first_axis];
                        float second = (&edge_start[edge].x)[second_axis] +
                                       amount * (&edge_delta[edge].x)[second_axis];
                        if ((&minimum.x)[first_axis] <= first &&
                            first <= (&maximum.x)[first_axis] &&
                            (&minimum.x)[second_axis] <= second &&
                            second <= (&maximum.x)[second_axis]) {
                            return 1;
                        }
                        if (intersection_count < 2) {
                            intersection[intersection_count].x = first;
                            intersection[intersection_count].y = second;
                        }
                        ++intersection_count;
                    }
                }
            }

            if (intersection_count == 2) {
                srVector2T<float> delta(intersection[1].x - intersection[0].x,
                                        intersection[1].y - intersection[0].y);
                short rectangle_axes[2] = {first_axis, second_axis};
                for (short coordinate = 0; coordinate < 2; ++coordinate) {
                    if (g_float_005ebc90 < static_cast<float>(fabs((&delta.x)[coordinate]))) {
                        short other = coordinate == 0 ? 1 : 0;
                        for (short edge_side = 0; edge_side < 2; ++edge_side) {
                            float boundary = edge_side == 0
                                                 ? (&minimum.x)[rectangle_axes[coordinate]]
                                                 : (&maximum.x)[rectangle_axes[coordinate]];
                            float amount = (boundary - (&intersection[0].x)[coordinate]) /
                                           (&delta.x)[coordinate];
                            if (g_float_zero <= amount && amount <= g_float_one) {
                                float crossing =
                                    (&intersection[0].x)[other] + amount * (&delta.x)[other];
                                if ((&minimum.x)[rectangle_axes[other]] <= crossing &&
                                    crossing <= (&maximum.x)[rectangle_axes[other]]) {
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
