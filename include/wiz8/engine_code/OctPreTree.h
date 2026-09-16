#ifndef WIZ8_ENGINE_CODE_OCT_PRE_TREE_H
#define WIZ8_ENGINE_CODE_OCT_PRE_TREE_H

#include "surrender/srMath.h"

#include <stddef.h>

/* One 0xe8-byte region volume from the spatial state's region array. The
   region id at +4 is consumed by the particle-region builder, and the six
   plane equations at +0x88 are consumed by 0x0049E460. */
struct W8OctRegionVolume {
    unsigned long positional_00;
    unsigned short region_04;
    unsigned char positional_06[6];
    /* The bit the visibility pass tests and sets for this volume. */
    unsigned int region_bit_0c;
    unsigned char positional_10[0xc];
    /* Nine 12-byte points from +0x1c to +0x88; 0x004301C0 projects the first
       against the camera and falls back to the other eight. */
    srVector3T<float> points_1c[9];
    srVector4T<float> planes_88[6];

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
    void GetClippedBounds0046CE30(srVector3T<float>* minimum, srVector3T<float>* maximum);
    void GetWorkingBounds0046CDF0(srVector3T<float>* minimum, srVector3T<float>* maximum);
    void SetWorkingBounds00467B70(const srVector3T<float>* minimum,
                                  const srVector3T<float>* maximum);

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
    unsigned char positional_4a[6];
    unsigned short positional_50;
    /* The bottom octree level: insertion stops and masks saturate here. */
    unsigned short leaf_level_52;
    float positional_54;
    unsigned short positional_58;
    unsigned short positional_5a;
    W8OctRegionVolume* owned_5c;
    float positional_60;
    unsigned long positional_64;
    unsigned long positional_68;
    unsigned short level_kind_6c;
    unsigned short positional_6e;
    float node_extent_70;
    unsigned long positional_74;
    srVector3T<float> working_minimum_78;
    srVector3T<float> working_maximum_84;
    void* root_90;
    unsigned long positional_94;
    void* owned_98;
};

unsigned char TestSpatialTriangle0046CE60(const srVector3T<float>* bounds,
                                          const srVector3T<float>* vertices,
                                          const srVector3T<float>* plane_normal);
unsigned char BoundsOverlap0046D470(const srVector3T<float>* first,
                                    const srVector3T<float>* second);
unsigned char PointInsideBounds0046D4D0(const srVector3T<float>* bounds,
                                        const srVector3T<float>* point);

static_assert(sizeof(W8OctSpatialState) == 0x9c, "W8OctSpatialState_must_be_0x9c");
static_assert(sizeof(W8OctRegionVolume) == 0xe8, "W8OctRegionVolume_must_be_0xe8");

struct W8OctRegionPolygon;

/* The 0x60-byte build vertex the 0x00493120 driver collects into the shared
   geometry context.  CreateSubMeshes reads the position at +0x0c, material
   at +0x1c, normal at +0x24, light at +0x30 and the per-sun light array
   pointer at +0x3c; the driver also clears flag bytes at +0x0a and +0x6a. */
struct W8OctPreTreeVertex {
    unsigned char positional_00[0x0c];
    srVector3T<float> position_0c;
    unsigned char positional_18[4];
    int material_1c;
    unsigned char positional_20[4];
    srVector3T<float> normal_24;
    srVector3T<float> light_30;
    float* sun_lights_3c;
    unsigned char positional_40[0x20];
};

/* The shared geometry context the 0x00493120 driver builds and hands to
   CreateSubMeshes, SplitMeshes and WriteOctFile: the deduplicated build
   vertices plus the 0x74-byte region polygons.  The driver writes further
   fields past +0x0c whose layout is unproven for this cluster. */
struct W8OctPreTreeGeometry {
    unsigned long vertex_count_00;
    W8OctPreTreeVertex* vertices_04;
    unsigned long polygon_count_08;
    W8OctRegionPolygon* polygons_0c;
};

/* The 0x34-byte per-submesh build record SplitMeshes partitions polygons and
   vertices into.  Records chain through +0x0c/+0x10 (the OctMeshModel
   link_index_04/uv_count_08 link fields are emitted as these indices minus
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

/* The 0xf5-byte NewLevel.oct file header WriteOctFile emits field by field;
   ReadOctFile's WriteMember calls give the authoritative byte offsets.  The
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
    unsigned short positional_64;
    /* The finished submesh count (the octree's +0x74), not a mesh-file count. */
    unsigned long polygon_count_66;
    unsigned long branch_count_6a;
    unsigned long leaf_count_6e;
    unsigned long polygon_count_72;
    unsigned long vertex_count_76;
    unsigned long surface_count_7a;
    unsigned long path_nodes_7e;
    unsigned long polygon_cursor_82;
    unsigned long positional_86;
    unsigned long triangle_count_8a;
    unsigned long zero_8e;
    unsigned long positional_92;
    unsigned short region_count_96;
    unsigned short positional_98;
    unsigned long positional_9a;
    unsigned long mesh_total_9e;
    unsigned long positional_a2;
    /* +0xa6 and +0xb9 serialize spatial_000's +0x54/+0x60 floats; retail
       copies them with plain movs, which requires float-typed fields. */
    float positional_a6;
    unsigned short pad_aa;
    float region_cell_ac;
    unsigned long pathing_b0;
    unsigned long positional_b4;
    unsigned char prop_sun_bits_b8;
    float positional_b9;
    unsigned long prop_count_bd;
    unsigned long particle_count_c1;
    unsigned short particle_len_c5;
    unsigned short prop_len_c7;
    unsigned char pad_c9[0x2c];
};
#pragma pack(pop)

static_assert(sizeof(W8OctPreTreeVertex) == 0x60, "W8OctPreTreeVertex_must_be_0x60");
static_assert(sizeof(W8OctSubmeshBuild) == 0x34, "W8OctSubmeshBuild_must_be_0x34");
static_assert(offsetof(W8OctSubmeshBuild, vertex_ids_20) == 0x20,
              "W8OctSubmeshBuild_vertex_ids_20");
static_assert(offsetof(W8OctSubmeshBuild, uv_map_30) == 0x30, "W8OctSubmeshBuild_uv_map_30");
static_assert(sizeof(W8OctFileHeader) == 0xf5, "W8OctFileHeader_must_be_0xf5");
static_assert(offsetof(W8OctFileHeader, grid_dims_56) == 0x56, "W8OctFileHeader_grid_dims_56");
static_assert(offsetof(W8OctFileHeader, polygon_count_66) == 0x66,
              "W8OctFileHeader_polygon_count_66");
static_assert(offsetof(W8OctFileHeader, path_nodes_7e) == 0x7e, "W8OctFileHeader_path_nodes_7e");
static_assert(offsetof(W8OctFileHeader, triangle_count_8a) == 0x8a,
              "W8OctFileHeader_triangle_count_8a");
static_assert(offsetof(W8OctFileHeader, region_count_96) == 0x96,
              "W8OctFileHeader_region_count_96");
static_assert(offsetof(W8OctFileHeader, mesh_total_9e) == 0x9e, "W8OctFileHeader_mesh_total_9e");
static_assert(offsetof(W8OctFileHeader, region_cell_ac) == 0xac, "W8OctFileHeader_region_cell_ac");
static_assert(offsetof(W8OctFileHeader, pathing_b0) == 0xb0, "W8OctFileHeader_pathing_b0");
static_assert(offsetof(W8OctFileHeader, prop_sun_bits_b8) == 0xb8,
              "W8OctFileHeader_prop_sun_bits_b8");
static_assert(offsetof(W8OctFileHeader, prop_count_bd) == 0xbd, "W8OctFileHeader_prop_count_bd");
static_assert(offsetof(W8OctFileHeader, particle_len_c5) == 0xc5,
              "W8OctFileHeader_particle_len_c5");

#endif
