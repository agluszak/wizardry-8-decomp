#ifndef WIZ8_ENGINE_CODE_OCT_PRE_TREE_H
#define WIZ8_ENGINE_CODE_OCT_PRE_TREE_H

#include "surrender/srMath.h"
#include "wiz8/geometry.h"

#include <stddef.h>

struct W8OctBuildNode;

/* One 0xe8-byte region volume from the spatial state's region array. The
   region id at +4 is consumed by the particle-region builder, and the six
   plane equations at +0x88 are consumed by 0x0049E460. */
struct W8OctRegionVolume {
    unsigned long flags_00;
    unsigned short region_04;
    unsigned char positional_06[6];
    /* The bit the visibility pass tests and sets for this volume. */
    unsigned int region_bit_0c;
    /* Two record dwords LoadRegionFile copies from the .cub record at +0x10
       and +0x18, and the dword it zeroes at +0x14. */
    unsigned long value_10;
    /* Region polygon count: the assignment pass increments it per contained
       polygon, and compaction remaps away regions where it stays zero. */
    unsigned long polygon_count_14;
    unsigned long value_18;
    /* Nine 12-byte points from +0x1c to +0x88; 0x004301C0 projects the first
       against the camera and falls back to the other eight. */
    srVector3T<float> points_1c[9];
    W8Plane planes_88[6];

    unsigned char ContainsPoint0049E460(const srVector3T<float>* point) const;
};

/* A reusable, non-polymorphic 0x9c spatial record.  Octree.cpp constructs one
   at the start of its 0x29c owner, while OctBuildTree.cpp constructs the same
   value at the start of its 0xbc build tree and uses standalone copies while
   inserting surfaces.  Same-address construction proves the common value
   boundary, but not whether either owner used inheritance or a first member. */
struct W8OctSpatialState {
    explicit W8OctSpatialState(const W8OctSpatialState* source = 0);
    ~W8OctSpatialState();

    void Reset0046CDC0();
    void GetClippedBounds(srVector3T<float>* minimum, srVector3T<float>* maximum);
    void GetWorkingBounds(srVector3T<float>* minimum, srVector3T<float>* maximum);
    void SetWorkingBounds(const srVector3T<float>* minimum, const srVector3T<float>* maximum);

    unsigned long flags_00;
    float extent_04;
    float cell_size_08;
    srVector3T<float> minimum_0c;
    srVector3T<float> maximum_18;
    srVector3T<float> clipped_minimum_24;
    srVector3T<float> clipped_maximum_30;
    /* Mode-2 insertions increment this once per region polygon; the build
       conversion copies it to the pre-tree and sizes m_owned_190 from it. */
    unsigned long polygon_count_3c;
    unsigned long item_count_40;
    unsigned short depth_44;
    unsigned short region_count_46;
    unsigned short item_limit_48;
    unsigned char padding_4a[6];
    /* Packed auto-region cell coordinate bound per axis, derived from the
       leaf level when a loaded octree is initialized. */
    unsigned short region_cells_per_axis_50;
    /* The bottom octree level: insertion stops and masks saturate here. */
    unsigned short leaf_level_52;
    /* Auto-region grid pitch: packed (x<<16|y<<8|z) cell coordinates scale
       to world units through it. */
    float region_grid_cell_54;
    /* Auto-region id allocator bound: each new region takes this value and
       bumps it; region_count_46 mirrors it during the build. */
    unsigned short region_id_bound_58;
    unsigned short padding_5a;
    W8OctRegionVolume* owned_5c;
    /* Maximum vertex distance from its region's center across the
       auto-regions. */
    float max_region_radius_60;
    /* Leaf-grid strides: dim_y*dim_z for one x step, dim_z for one y
       step. */
    unsigned long leaf_grid_stride_x_64;
    unsigned long leaf_grid_stride_y_68;
    unsigned short level_kind_6c;
    unsigned short padding_6e;
    float node_extent_70;
    /* Emitted submesh record bound: the build packs kind-0 then kind-1
       records beneath it. */
    unsigned long submesh_count_74;
    srVector3T<float> working_minimum_78;
    srVector3T<float> working_maximum_84;
    /* The build octree root: W8OctBuildNode or the counted subclass
       when the owning build tree counts surfaces per node. */
    W8OctBuildNode* root_90;
    unsigned long node_index_94;
    /* The working triangle's three vertices, borrowed from the inserter's
       stack for the recursion's bounds tests. */
    const srVector3T<float>* owned_98;
};

unsigned char TestSpatialTriangle(const srVector3T<float>* bounds,
                                  const srVector3T<float>* vertices,
                                  const srVector3T<float>* plane_normal);
unsigned char BoundsOverlap0046D470(const srVector3T<float>* first,
                                    const srVector3T<float>* second);
bool PointInsideBounds0046D4D0(const srVector3T<float>* bounds, const srVector3T<float>* point);

static_assert(sizeof(W8OctSpatialState) == 0x9c, "W8OctSpatialState_must_be_0x9c");
static_assert(sizeof(W8OctRegionVolume) == 0xe8, "W8OctRegionVolume_must_be_0xe8");

struct W8OctRegionPolygon;

/* The 0x60-byte build vertex the 0x00493120 driver collects into the shared
   geometry context.  CreateSubMeshes reads the position at +0x0c, material
   at +0x1c, normal at +0x24, light at +0x30 and the per-sun light array
   pointer at +0x3c; the driver also clears flag bytes at +0x0a and +0x6a. */
struct W8OctPreTreeVertex {
    /* bit0: welded into an earlier vertex - vertex_index_04 then holds the
       redirect; bit1: material-split copy; bit2: sits inside more than one
       region volume (region_08 zeroed). */
    unsigned long flags_00;
    /* Ordinal into the geometry vertex array; on welded vertices the merge
       helper stores the surviving vertex's index here. */
    unsigned long vertex_index_04;
    /* Owning auto-region id written by the region assignment pass. */
    unsigned short region_08;
    /* Per-vertex traversal latch the shared-polygon split raises so each
       vertex's face run is walked once per pass. */
    bool visited_0a;
    unsigned char padding_0b;
    srVector3T<float> position_0c;
    /* Number of polygons referencing this vertex; SortGeometry counts and
       the region pass tracks the run bound. */
    short normal_count_18;
    unsigned char padding_1a[2];
    int material_1c;
    /* The polygon automesh kind SplitVertices copies onto material-split
       duplicates. */
    unsigned long kind_20;
    srVector3T<float> normal_24;
    srVector3T<float> light_30;
    float* sun_lights_3c;
    unsigned short face_count_40;
    unsigned char padding_42[2];
    /* Growable run of polygon ordinals sharing this vertex, built by the
       region pass; consecutive vertices may share one allocation. */
    int* face_indices_44;
    /* Second owned run freed by the geometry cleanup. */
    int* owned_48;
    /* Corner texture coordinate written when a polygon vertex is split. */
    srVector2T<float> uv_4c;
    /* Unscaled level-file position kept beside the engine-scaled
       position_0c. */
    srVector3T<float> original_position_54;
};

/* The shared geometry context the 0x00493120 driver builds and hands to
   CreateSubMeshes, SplitMeshes and WriteOctFile: the deduplicated build
   vertices plus the 0x74-byte region polygons.  The material sort appends
   the canonical material/texture group counts; +0x10/+0x14 hold an owned
   buffer the cleanup releases. */
struct W8OctPreTreeGeometry {
    unsigned long vertex_count_00;
    W8OctPreTreeVertex* vertices_04;
    unsigned long polygon_count_08;
    W8OctRegionPolygon* polygons_0c;
    unsigned long positional_10;
    void* owned_14;
    unsigned long material_count_18;
    unsigned long texture_count_1c;
    /* Largest per-vertex polygon reference count, refreshed by the region
       assignment pass. */
    unsigned short max_face_count_20;
    unsigned char positional_22[2];

    /* Frees the per-vertex polygon runs, both arrays and the owned +0x14
       buffer; the preprocessing driver runs it during cleanup. */
    void Release004CFC10();
};

/* The 0x34-byte per-submesh build record SplitMeshes partitions polygons and
   vertices into.  Records chain through +0x0c/+0x10 (the OctMeshModel
   link_index_04/next_link_08 link fields are emitted as these indices minus
   one) and +0x08 carries the 1..3 mesh kind that becomes packed_header_3c. */
struct W8OctSubmeshBuild {
    unsigned long flags_00;
    unsigned long index_04;
    unsigned long kind_08;
    unsigned long prev_link_0c;
    unsigned long next_link_10;
    unsigned long vertex_count_14;
    unsigned long map_count_18;
    unsigned long polygon_count_1c;
    int* vertex_ids_20;
    int* polygon_ids_24;
    /* Per-corner vertex indices (three per polygon); becomes the model's
       m_psrPolyVertex. */
    srVector3i* poly_vertices_28;
    /* Per-corner deduplicated uv indices; becomes the model's
       m_psrPolyUVIndex. */
    srVector3i* poly_uv_index_2c;
    /* The deduplicated uv pairs SplitUVMaps emits; becomes m_psrMap. */
    srVector2T<float>* uv_map_30;
};

/* The 0xf5-byte NewLevel.oct file header shared by WriteOctFile and
   ReadOctFile. The offsetof assertions pin the authoritative byte offsets. The
   seven consecutive vectors hold the spatial state's minimum, maximum,
   clipped and working bounds plus the octree's +0xa4 vector.  +0xb4 is left
   unwritten by retail; ReadOctFile still loads it into +0x17c. */
#pragma pack(push, 1)
struct W8OctFileHeader {
    unsigned short version_00; /* written 0x22 */
    float extent_02;
    float cell_size_06;
    float node_extent_0a;
    srVector3T<float> bounds_0e[6];
    /* The uiLeaf grid dimensions: three integer cell counts serialized in the
       vector slot. */
    unsigned long grid_dims_56[3];
    unsigned short depth_62;
    /* Auto-region id bound; the reader sizes the region-indexed
       m_owned_154/m_pfRegsVisited arrays from it. */
    unsigned short region_id_bound_64;
    /* The finished submesh record bound (the octree's +0x74), not a
       mesh-file count. */
    unsigned long submesh_count_66;
    unsigned long branch_count_6a;
    unsigned long leaf_count_6e;
    unsigned long polygon_count_72;
    unsigned long vertex_count_76;
    unsigned long surface_count_7a;
    unsigned long path_nodes_7e;
    /* Dword length of the count-prefixed per-leaf polygon-id stream. */
    unsigned long leaf_polygon_stream_len_82;
    /* Dword length of the per-leaf GD surface-id stream; doubles as the
       GameData-block presence gate on the read side. */
    unsigned long gd_surface_stream_len_86;
    /* Trigger-list element count; always 0 - the machinery is vestigial and
       the reader even uses a different element size than the writer. */
    unsigned long trigger_count_8a;
    unsigned long zero_8e;
    /* u16 length of the per-leaf 0-terminated region-id stream. */
    unsigned long region_list_len_92;
    unsigned short region_count_96;
    /* The spatial state's leaf level; the reader derives the region mask and
       packed cell bound from it. */
    unsigned short leaf_level_98;
    /* Kind-0 emitted submesh count; carries the same value as
       mesh_total_9e. */
    unsigned long root_mesh_count_9a;
    unsigned long mesh_total_9e;
    /* Kind-1 emitted submesh count.  Serialized for information only - the
       lookup tables on both sides size from mesh_total_9e. */
    unsigned long kind1_submesh_count_a2;
    /* +0xa6 and +0xb9 serialize spatial_000's +0x54/+0x60 floats; retail
       copies them with plain movs, which requires float-typed fields. */
    float region_grid_cell_a6;
    unsigned short pad_aa;
    float region_cell_ac;
    unsigned long edge_node_count_b0;
    /* Path probe-clearance height, float bits. Retail never assigns this
       field - the file carries stack garbage - but the reader still loads
       it into +0x17c and feeds it to ConfigureForLevel. */
    unsigned long path_clearance_b4;
    unsigned char prop_sun_bits_b8;
    float max_region_radius_b9;
    unsigned long prop_count_bd;
    unsigned long particle_count_c1;
    unsigned short particle_len_c5;
    unsigned short prop_len_c7;
    unsigned char pad_c9[0x2c];
};
#pragma pack(pop)

static_assert(sizeof(W8OctPreTreeVertex) == 0x60, "W8OctPreTreeVertex_must_be_0x60");
static_assert(sizeof(W8OctSubmeshBuild) == 0x34, "W8OctSubmeshBuild_must_be_0x34");
static_assert(offsetof(W8OctSubmeshBuild, flags_00) == 0x00, "W8OctSubmeshBuild_flags_00");
static_assert(offsetof(W8OctSubmeshBuild, index_04) == 0x04, "W8OctSubmeshBuild_index_04");
static_assert(offsetof(W8OctSubmeshBuild, kind_08) == 0x08, "W8OctSubmeshBuild_kind_08");
static_assert(offsetof(W8OctSubmeshBuild, prev_link_0c) == 0x0c, "W8OctSubmeshBuild_prev_link_0c");
static_assert(offsetof(W8OctSubmeshBuild, next_link_10) == 0x10, "W8OctSubmeshBuild_next_link_10");
static_assert(offsetof(W8OctSubmeshBuild, vertex_count_14) == 0x14,
              "W8OctSubmeshBuild_vertex_count_14");
static_assert(offsetof(W8OctSubmeshBuild, map_count_18) == 0x18, "W8OctSubmeshBuild_map_count_18");
static_assert(offsetof(W8OctSubmeshBuild, polygon_count_1c) == 0x1c,
              "W8OctSubmeshBuild_polygon_count_1c");
static_assert(offsetof(W8OctSubmeshBuild, vertex_ids_20) == 0x20,
              "W8OctSubmeshBuild_vertex_ids_20");
static_assert(offsetof(W8OctSubmeshBuild, polygon_ids_24) == 0x24,
              "W8OctSubmeshBuild_polygon_ids_24");
static_assert(offsetof(W8OctSubmeshBuild, poly_vertices_28) == 0x28,
              "W8OctSubmeshBuild_poly_vertices_28");
static_assert(offsetof(W8OctSubmeshBuild, poly_uv_index_2c) == 0x2c,
              "W8OctSubmeshBuild_poly_uv_index_2c");
static_assert(offsetof(W8OctSubmeshBuild, uv_map_30) == 0x30, "W8OctSubmeshBuild_uv_map_30");
static_assert(sizeof(W8OctFileHeader) == 0xf5, "W8OctFileHeader_must_be_0xf5");
static_assert(offsetof(W8OctFileHeader, version_00) == 0x00, "W8OctFileHeader_version_00");
static_assert(offsetof(W8OctFileHeader, extent_02) == 0x02, "W8OctFileHeader_extent_02");
static_assert(offsetof(W8OctFileHeader, cell_size_06) == 0x06, "W8OctFileHeader_cell_size_06");
static_assert(offsetof(W8OctFileHeader, node_extent_0a) == 0x0a, "W8OctFileHeader_node_extent_0a");
static_assert(offsetof(W8OctFileHeader, bounds_0e) == 0x0e, "W8OctFileHeader_bounds_0e");
static_assert(offsetof(W8OctFileHeader, grid_dims_56) == 0x56, "W8OctFileHeader_grid_dims_56");
static_assert(offsetof(W8OctFileHeader, depth_62) == 0x62, "W8OctFileHeader_depth_62");
static_assert(offsetof(W8OctFileHeader, region_id_bound_64) == 0x64,
              "W8OctFileHeader_region_id_bound_64");
static_assert(offsetof(W8OctFileHeader, submesh_count_66) == 0x66,
              "W8OctFileHeader_submesh_count_66");
static_assert(offsetof(W8OctFileHeader, branch_count_6a) == 0x6a,
              "W8OctFileHeader_branch_count_6a");
static_assert(offsetof(W8OctFileHeader, leaf_count_6e) == 0x6e, "W8OctFileHeader_leaf_count_6e");
static_assert(offsetof(W8OctFileHeader, polygon_count_72) == 0x72,
              "W8OctFileHeader_polygon_count_72");
static_assert(offsetof(W8OctFileHeader, vertex_count_76) == 0x76,
              "W8OctFileHeader_vertex_count_76");
static_assert(offsetof(W8OctFileHeader, surface_count_7a) == 0x7a,
              "W8OctFileHeader_surface_count_7a");
static_assert(offsetof(W8OctFileHeader, path_nodes_7e) == 0x7e, "W8OctFileHeader_path_nodes_7e");
static_assert(offsetof(W8OctFileHeader, leaf_polygon_stream_len_82) == 0x82,
              "W8OctFileHeader_leaf_polygon_stream_len_82");
static_assert(offsetof(W8OctFileHeader, gd_surface_stream_len_86) == 0x86,
              "W8OctFileHeader_gd_surface_stream_len_86");
static_assert(offsetof(W8OctFileHeader, trigger_count_8a) == 0x8a,
              "W8OctFileHeader_trigger_count_8a");
static_assert(offsetof(W8OctFileHeader, zero_8e) == 0x8e, "W8OctFileHeader_zero_8e");
static_assert(offsetof(W8OctFileHeader, region_list_len_92) == 0x92,
              "W8OctFileHeader_region_list_len_92");
static_assert(offsetof(W8OctFileHeader, region_count_96) == 0x96,
              "W8OctFileHeader_region_count_96");
static_assert(offsetof(W8OctFileHeader, leaf_level_98) == 0x98, "W8OctFileHeader_leaf_level_98");
static_assert(offsetof(W8OctFileHeader, root_mesh_count_9a) == 0x9a,
              "W8OctFileHeader_root_mesh_count_9a");
static_assert(offsetof(W8OctFileHeader, mesh_total_9e) == 0x9e, "W8OctFileHeader_mesh_total_9e");
static_assert(offsetof(W8OctFileHeader, kind1_submesh_count_a2) == 0xa2,
              "W8OctFileHeader_kind1_submesh_count_a2");
static_assert(offsetof(W8OctFileHeader, region_grid_cell_a6) == 0xa6,
              "W8OctFileHeader_region_grid_cell_a6");
static_assert(offsetof(W8OctFileHeader, pad_aa) == 0xaa, "W8OctFileHeader_pad_aa");
static_assert(offsetof(W8OctFileHeader, region_cell_ac) == 0xac, "W8OctFileHeader_region_cell_ac");
static_assert(offsetof(W8OctFileHeader, edge_node_count_b0) == 0xb0,
              "W8OctFileHeader_edge_node_count_b0");
static_assert(offsetof(W8OctFileHeader, path_clearance_b4) == 0xb4,
              "W8OctFileHeader_path_clearance_b4");
static_assert(offsetof(W8OctFileHeader, prop_sun_bits_b8) == 0xb8,
              "W8OctFileHeader_prop_sun_bits_b8");
static_assert(offsetof(W8OctFileHeader, max_region_radius_b9) == 0xb9,
              "W8OctFileHeader_max_region_radius_b9");
static_assert(offsetof(W8OctFileHeader, prop_count_bd) == 0xbd, "W8OctFileHeader_prop_count_bd");
static_assert(offsetof(W8OctFileHeader, particle_count_c1) == 0xc1,
              "W8OctFileHeader_particle_count_c1");
static_assert(offsetof(W8OctFileHeader, particle_len_c5) == 0xc5,
              "W8OctFileHeader_particle_len_c5");
static_assert(offsetof(W8OctFileHeader, prop_len_c7) == 0xc7, "W8OctFileHeader_prop_len_c7");
static_assert(offsetof(W8OctFileHeader, pad_c9) == 0xc9, "W8OctFileHeader_pad_c9");

#endif
