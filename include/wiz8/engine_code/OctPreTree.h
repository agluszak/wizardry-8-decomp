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

    unsigned char ContainsPoint(const srVector3T<float>* point) const;
};

/* A reusable, non-polymorphic 0x9c spatial record.  Octree.cpp constructs one
   at the start of its 0x29c owner, while OctBuildTree.cpp constructs the same
   value at the start of its 0xbc build tree and uses standalone copies while
   inserting surfaces.  Same-address construction proves the common value
   boundary, but not whether either owner used inheritance or a first member. */
struct W8OctSpatialState {
    explicit W8OctSpatialState(const W8OctSpatialState* source = 0);
    ~W8OctSpatialState();

    void Reset();
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
bool BoundsOverlapStrict(const srVector3T<float>* first, const srVector3T<float>* second);
bool PointInsideBoxBounds(const srVector3T<float>* bounds, const srVector3T<float>* point);

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

    /* Grow an owned face-index run at capacity boundaries. Retail narrows
       count + capacity to 16 bits before allocation and returns a byte. */
    unsigned char CheckArrayLength(int** run, unsigned short count, unsigned short capacity);

    /* Frees the per-vertex polygon runs, both arrays and the owned +0x14
       buffer; the preprocessing driver runs it during cleanup. */
    void Release();
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
    enum { VERSION = 0x22 };
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

#include "wiz8/evidence/OctPreTree_layout.inc"

#endif
