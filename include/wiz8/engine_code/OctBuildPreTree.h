#ifndef WIZ8_ENGINE_CODE_OCT_BUILD_PRE_TREE_H
#define WIZ8_ENGINE_CODE_OCT_BUILD_PRE_TREE_H

#include "wiz8/engine_code/OctBuildTree.h"
#include "wiz8/engine_code/ReadMesh.h"
#include "wiz8/engine_code/stHash.hpp"
#include "wiz8/geometry.h"

class BitArray;
struct W8LevelFileParticleSystem;
struct W8LevelFileProp;

extern const float g_float_three;

struct W8OctPreTreeVertex;
struct W8OctPreTreeGeometry;

struct W8OctRegionPolygon {
    enum { AXIS_MASK = 3u, MULTIPLE_REGIONS = 4u, CENTER_IN_REGION = 8u };
    /* & 3 selects the axis the plane test uses; bit2 marks a vertex shared
       across regions (cleared with bit3 after the duplicate pass). */
    unsigned long flags;
    /* 1-based ordinal into the geometry polygon array. */
    unsigned long ordinal;
    W8Plane plane; /* unit normal plus signed distance */
    srVector3T<float> position;
    /* Canonical material-group index assigned by the material sort. */
    unsigned long material;
    /* The per-polygon texture/material index CreateSubMeshes copies into
       OctMeshModel's m_plPolyTextures row. */
    unsigned long texture;
    /* The automesh kind (1..3) SplitMeshes partitions polygon lists on. */
    unsigned long kind;
    /* Set by the polygon builder when the face collapses. */
    unsigned char degenerate;
    bool visited;
    unsigned short region;
    /* Corner vertices of the shared build-vertex array; the material sort
       and SplitVertices repoint these at split copies. */
    W8OctPreTreeVertex* vertices[3];
    unsigned short face_count;
    unsigned char padding_42[2];
    /* Growable per-polygon run the cleanup releases. */
    int* face_indices;
    /* Whole source mesh face copied by the polygon builder. */
    W8ReadMeshFace face;
    unsigned char padding_71[3];

    /* Tests the polygon's representative point against six frustum planes;
       inside means every plane distance is non-negative. */
    bool InsideFrustumPlanes(const W8Plane* planes) const;
    bool ContainsPoint(const srVector3T<float>* bounds) const;
};

static_assert(sizeof(W8OctRegionPolygon) == 0x74, "W8OctRegionPolygon_must_be_0x74");

extern int g_build_node_instances;
extern unsigned long g_poly_list_count;
/* Build scratch carries mode-2 region polygons and mode-3 GD surfaces. */
extern void** g_poly_list;
extern W8GDSurface** g_gd_surface_list;
extern unsigned short* g_region_id_list;
extern unsigned short g_region_id_count;

/* Retail assertion text gives the class and member names directly:
   "OctBuildPreTree::m_ppPolyList too long.", "m_pulRegPaths" and
   "m_psrvRegCenters" in Engine Code\OctBuildPreTree.cpp. */
struct OctBuildPreTree : W8OctBuildTree {
    OctBuildPreTree(float leaf_size, srVector3T<float>* minimum, srVector3T<float>* maximum,
                    unsigned short item_limit, unsigned long path_capacity, short extent_mode);
    OctPreTree* BuildOctPreTree();
    unsigned short BuildRegions();
    unsigned char BuildParticleRegions(const W8LevelFileParticleSystem* particles,
                                       int particle_count);
    unsigned char BuildGeometryRegions(const W8LevelFileProp* records, int record_count,
                                       int base_index, bool finalize);

    void AssignInitialRegions(const W8OctSpatialState* spatial);
    unsigned char UpdateRegionForGeometry(const srVector3T<float>* geometry, short value,
                                          short mode);
    unsigned char UpdateRegionMap(const W8OctSpatialState* spatial,
                                  const srVector3T<float>* geometry, short value, short mode);
    W8OctBuildNode* FindNode(unsigned int path);
    unsigned char MergeAdjacentRegion(W8OctBuildNode* node, unsigned int path);
    unsigned char MergeRegion(W8OctBuildNode* node, const int* cell);
    void FinalizeRegionMapping();
    void AssignRegionFromSurfaces(const W8OctSpatialState* spatial);
    void ValidatePolygonRegions();
    void ValidateRegionBounds(const W8BoundingBox* region_bounds);
    /* SortGeometry: welds duplicate vertices, drops degenerate polygons,
       repacks both arrays and re-inserts every polygon with mode 2. */
    unsigned char SortGeometry(W8OctPreTreeGeometry* geometry);
    /* Inserts one region polygon into the octree working state. */
    unsigned char InsertSurface(W8OctRegionPolygon* polygon, unsigned long mode);
    /* Recursive inserter for InsertSurface: subdivides to the leaf, collecting
       overlapping region ids on first touch and appending the polygon to the
       leaf's mode link list. */
    unsigned char InsertSurfaceRecursive(W8OctSpatialState* working, W8OctRegionPolygon* polygon,
                                         unsigned long mode);
    /* Fill the leaf's region-id list with every region volume overlapping
       `bounds`; grows a 50-entry scratch list on first use. */
    void FindLeafRegions(W8OctBuildNode* node, const W8BoundingBox* bounds);
    /* Loads the .rlk region file beside the level and folds its bounds into
       the build. */
    unsigned short LoadRegionFile(const char* stem, srVector3T<float>* minimum,
                                  srVector3T<float>* maximum);
    /* Walks the node tree remapping leaf region ids through region_remap. */
    void RemapNodeRegions(W8OctBuildNode* node, int depth);
    /* Assigns a polygon's region from the region volume containing its
       representative point, falling back to the corner vertices' regions;
       marks multi-region polygons with flags bit2 and counts the assignment
       on the volume. */
    void AssignPolygonRegion(W8OctRegionPolygon* polygon);
    /* Region assignment pass run by SortGeometry: builds each vertex's
       polygon-reference run, assigns vertex and polygon regions against the
       region volumes, compacts dead regions and finishes in BuildRegions. */
    unsigned char AssignPolygonRegions(W8OctPreTreeGeometry* geometry);
    /* Resolves a shared polygon (flags bit2): histograms the neighboring
       polygons' regions collected through the corner vertices' face runs,
       picks the most frequent region the polygon actually touches, assigns it
       and counts it on the volume. Answers the polygon's region. */
    unsigned short SplitSharedPolygon(W8OctPreTreeGeometry* geometry, int index);

    unsigned long path_capacity;
    unsigned short selected_depth;
    unsigned short padding_c2;
    unsigned long level_counts[10];
    unsigned long* m_pulRegPaths;
    unsigned long region_path_count;
    bool mesh_linking;
    unsigned char padding_f5[3];
    BitArray* region_bits;
    srVector3T<float>* m_psrvRegCenters;
    /* Region remap table indexed by old region id; RemapNodeRegions frees it
       after rewriting every leaf's region list through it. */
    unsigned short* region_remap;
    unsigned short* mesh_particle_lookup;
    unsigned short* mesh_particles;
    unsigned short mesh_particle_count;
    unsigned short padding_10e;
    unsigned short* mesh_prop_lookup;
    unsigned short* mesh_props;
    unsigned short mesh_prop_count;
    unsigned short padding_11a;
    unsigned long particle_count;
    unsigned long prop_count;
    W8HashTable<unsigned short, unsigned long>* region_path_map;
    /* Allocated next to region_path_map but never read, inserted into, or
       freed in recovered code - dead table kept for layout fidelity. */
    W8HashTable<unsigned int, short>* positional;
    W8HashTable<unsigned short, short>* inside_region_map;
    W8HashTable<unsigned short, short>* overlap_region_map;
    W8OctPreTreeGeometry* game_data;
    unsigned long unknown_138;
    unsigned long unknown_13c;
};

static_assert(sizeof(OctBuildPreTree) == 0x140, "OctBuildPreTree_must_be_0x140");

int GetBuildNodeInstanceCount(void);

#endif
