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
    unsigned short m_region;
    unsigned char positional_06[6];
    /* The bit the visibility pass tests and sets for this volume. */
    unsigned int m_region_bit;
    /* Two record dwords LoadRegionFile copies from the .cub record at +0x10
       and +0x18, and the dword it zeroes at +0x14. */
    unsigned long value_10;
    /* Region polygon count: the assignment pass increments it per contained
       polygon, and compaction remaps away regions where it stays zero. */
    unsigned long m_polygon_count;
    unsigned long value_18;
    /* Nine 12-byte points from +0x1c to +0x88; 0x004301C0 projects the first
       against the camera and falls back to the other eight. */
    srVector3T<float> m_points[9];
    W8Plane m_planes[6];

    unsigned char ContainsPoint(const srVector3T<float>* point) const;
};

/* A reusable, non-polymorphic 0x9c spatial record.  Octree.cpp constructs one
   at the start of its 0x29c owner, while OctBuildTree.cpp constructs the same
   value at the start of its 0xbc build tree and uses standalone copies while
   inserting surfaces.  Same-address construction proves the common value
   boundary, but not whether either owner used inheritance or a first member.
   Copies borrow the region/root/triangle pointers; the destructor only clears
   them. W8Octree releases its region allocation separately. */
struct W8OctSpatialState {
    explicit W8OctSpatialState(const W8OctSpatialState* source = 0);
    ~W8OctSpatialState();

    void Reset();
    void GetClippedBounds(srVector3T<float>* minimum, srVector3T<float>* maximum);
    void GetWorkingBounds(srVector3T<float>* minimum, srVector3T<float>* maximum);
    void SetWorkingBounds(const srVector3T<float>* minimum, const srVector3T<float>* maximum);

    unsigned long flags_00;
    float m_extent;
    float m_cell_size;
    srVector3T<float> m_minimum;
    srVector3T<float> m_maximum;
    srVector3T<float> m_clipped_minimum;
    srVector3T<float> m_clipped_maximum;
    /* Mode-2 insertions increment this once per region polygon; the build
       conversion copies it to the pre-tree and sizes m_visited_polygon_bits from it. */
    unsigned long m_polygon_count;
    unsigned long m_item_count;
    unsigned short m_depth;
    unsigned short m_region_count;
    unsigned short m_item_limit;
    unsigned char padding_4a[6];
    /* Packed auto-region cell coordinate bound per axis, derived from the
       leaf level when a loaded octree is initialized. */
    unsigned short m_region_cells_per_axis;
    /* The bottom octree level: insertion stops and masks saturate here. */
    unsigned short m_leaf_level;
    /* Auto-region grid pitch: packed (x<<16|y<<8|z) cell coordinates scale
       to world units through it. */
    float m_region_grid_cell;
    /* Auto-region id allocator bound: each new region takes this value and
       bumps it; m_region_count mirrors it during the build. */
    unsigned short m_region_id_bound;
    unsigned short padding_5a;
    W8OctRegionVolume* m_region_volumes;
    /* Maximum vertex distance from its region's center across the
       auto-regions. */
    float m_max_region_radius;
    /* Leaf-grid strides: dim_y*dim_z for one x step, dim_z for one y
       step. */
    unsigned long m_leaf_grid_stride_x;
    unsigned long m_leaf_grid_stride_y;
    unsigned short m_level_kind;
    unsigned short padding_6e;
    float m_node_extent;
    /* Emitted submesh record bound: the build packs kind-0 then kind-1
       records beneath it. */
    unsigned long submesh_count_74;
    srVector3T<float> m_working_minimum;
    srVector3T<float> m_working_maximum;
    /* The build octree root: W8OctBuildNode or the counted subclass
       when the owning build tree counts surfaces per node. */
    W8OctBuildNode* m_root;
    unsigned long m_node_index;
    /* The working triangle's three vertices, borrowed from the inserter's
       stack for the recursion's bounds tests. */
    const srVector3T<float>* m_triangle_vertices;
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
    /* bit0: welded into an earlier vertex - m_vertex_index then holds the
       redirect; bit1: material-split copy; bit2: sits inside more than one
       region volume (m_region zeroed). */
    unsigned long flags_00;
    /* Ordinal into the geometry vertex array; on welded vertices the merge
       helper stores the surviving vertex's index here. */
    unsigned long m_vertex_index;
    /* Owning auto-region id written by the region assignment pass. */
    unsigned short m_region;
    /* Per-vertex traversal latch the shared-polygon split raises so each
       vertex's face run is walked once per pass. */
    bool m_visited;
    unsigned char padding_0b;
    srVector3T<float> position_0c;
    /* Number of polygons referencing this vertex; SortGeometry counts and
       the region pass tracks the run bound. */
    short m_normal_count;
    unsigned char padding_1a[2];
    int m_material;
    /* The polygon automesh kind SplitVertices copies onto material-split
       duplicates. */
    unsigned long m_kind;
    srVector3T<float> m_normal;
    srVector3T<float> m_light;
    float* m_sun_lights;
    unsigned short face_count_40;
    unsigned char padding_42[2];
    /* Growable run of polygon ordinals sharing this vertex, built by the
       region pass; consecutive vertices may share one allocation. */
    int* face_indices_44;
    /* Second owned run freed by the geometry cleanup. */
    int* owned_48;
    /* Corner texture coordinate written when a polygon vertex is split. */
    srVector2T<float> m_uv;
    /* Unscaled level-file position kept beside the engine-scaled
       position_0c. */
    srVector3T<float> m_original_position;
};

/* The shared geometry context the 0x00493120 driver builds and hands to
   CreateSubMeshes, SplitMeshes and WriteOctFile: the deduplicated build
   vertices plus the 0x74-byte region polygons.  The material sort appends
   the canonical material/texture group counts; +0x10/+0x14 hold an owned
   buffer the cleanup releases. */
struct W8OctPreTreeGeometry {
    unsigned long vertex_count_00;
    W8OctPreTreeVertex* m_vertices;
    unsigned long m_polygon_count;
    W8OctRegionPolygon* m_polygons;
    unsigned long positional_10;
    void* owned_14;
    unsigned long m_material_count;
    unsigned long m_texture_count;
    /* Largest per-vertex polygon reference count, refreshed by the region
       assignment pass. */
    unsigned short m_max_face_count;
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
   m_link_index/next_link_08 link fields are emitted as these indices minus
   one) and +0x08 carries the 1..3 mesh kind that becomes m_packed_header. */
struct W8OctSubmeshBuild {
    unsigned long flags_00;
    unsigned long index_04;
    unsigned long m_kind;
    unsigned long m_prev_link;
    unsigned long m_next_link;
    unsigned long vertex_count_14;
    unsigned long m_map_count;
    unsigned long m_polygon_count;
    int* m_vertex_ids;
    int* m_polygon_ids;
    /* Per-corner vertex indices (three per polygon); becomes the model's
       m_psrPolyVertex. */
    srVector3i* m_poly_vertices;
    /* Per-corner deduplicated uv indices; becomes the model's
       m_psrPolyUVIndex. */
    srVector3i* m_poly_uv_index;
    /* The deduplicated uv pairs SplitUVMaps emits; becomes m_psrMap. */
    srVector2T<float>* m_uv_map;
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
    float m_extent;
    float m_cell_size;
    float m_node_extent;
    srVector3T<float> m_bounds[6];
    /* The uiLeaf grid dimensions: three integer cell counts serialized in the
       vector slot. */
    unsigned long m_grid_dims[3];
    unsigned short m_depth;
    /* Auto-region id bound; the reader sizes the region-indexed
       m_owned_154/m_pfRegsVisited arrays from it. */
    unsigned short m_region_id_bound;
    /* The finished submesh record bound (the octree's +0x74), not a
       mesh-file count. */
    unsigned long m_submesh_count;
    unsigned long m_branch_count;
    unsigned long m_leaf_count;
    unsigned long m_polygon_count;
    unsigned long m_vertex_count;
    unsigned long m_surface_count;
    unsigned long m_path_nodes;
    /* Dword length of the count-prefixed per-leaf polygon-id stream. */
    unsigned long m_leaf_polygon_stream_len;
    /* Dword length of the per-leaf GD surface-id stream; doubles as the
       GameData-block presence gate on the read side. */
    unsigned long m_gd_surface_stream_len;
    /* Trigger-list element count; always 0 - the machinery is vestigial and
       the reader even uses a different element size than the writer. */
    unsigned long m_trigger_count;
    unsigned long m_zero;
    /* u16 length of the per-leaf 0-terminated region-id stream. */
    unsigned long m_region_list_len;
    unsigned short m_region_count;
    /* The spatial state's leaf level; the reader derives the region mask and
       packed cell bound from it. */
    unsigned short m_leaf_level;
    /* Kind-0 emitted submesh count; carries the same value as
       m_mesh_total. */
    unsigned long m_root_mesh_count;
    unsigned long m_mesh_total;
    /* Kind-1 emitted submesh count.  Serialized for information only - the
       lookup tables on both sides size from m_mesh_total. */
    unsigned long m_kind1_submesh_count;
    /* +0xa6 and +0xb9 serialize m_spatial's +0x54/+0x60 floats; retail
       copies them with plain movs, which requires float-typed fields. */
    float m_region_grid_cell;
    unsigned short pad_aa;
    float m_region_cell;
    unsigned long m_edge_node_count;
    /* Path probe-clearance height, float bits. Retail never assigns this
       field - the file carries stack garbage - but the reader still loads
       it into +0x17c and feeds it to ConfigureForLevel. */
    unsigned long m_path_clearance;
    unsigned char m_prop_sun_bits;
    float m_max_region_radius;
    unsigned long m_prop_count;
    unsigned long m_particle_count;
    unsigned short m_particle_len;
    unsigned short m_prop_len;
    unsigned char pad_c9[0x2c];
};
#pragma pack(pop)

#include "wiz8/evidence/OctPreTree_layout.inc"

#endif
