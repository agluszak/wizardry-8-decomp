#ifndef WIZ8_ENGINE_CODE_OCT_BUILD_PRE_TREE_H
#define WIZ8_ENGINE_CODE_OCT_BUILD_PRE_TREE_H

#include "wiz8/engine_code/OctBuildTree.h"
#include "wiz8/engine_code/ReadMesh.h"
#include "wiz8/engine_code/stHash.hpp"
#include "wiz8/geometry.h"

class BitArray;
struct W8LevelFileParticleSystem;
struct W8LevelFileProp;

extern float g_float_005ec52c;
extern float g_float_005ebc28;

struct W8OctPreTreeVertex;
struct W8OctPreTreeGeometry;

struct W8OctRegionPolygon {
    /* & 3 selects the axis the plane test uses; bit2 marks a vertex shared
       across regions (cleared with bit3 after the duplicate pass). */
    unsigned long flags_00;
    /* 1-based ordinal into the geometry polygon array. */
    unsigned long ordinal_04;
    float plane_08[4]; /* normal xyz and offset d */
    srVector3T<float> position_18;
    /* Canonical material-group index assigned by the material sort. */
    unsigned long material_24;
    /* The per-polygon texture/material index CreateSubMeshes copies into
       OctMeshModel's m_plPolyTextures row. */
    unsigned long texture_28;
    /* The automesh kind (1..3) SplitMeshes partitions polygon lists on. */
    unsigned long kind_2c;
    /* Set by the polygon builder when the face collapses. */
    unsigned char degenerate_30;
    unsigned char positional_31;
    unsigned short region_32;
    /* Corner vertices of the shared build-vertex array; the material sort
       and SplitVertices repoint these at split copies. */
    W8OctPreTreeVertex* vertices_34[3];
    unsigned short face_count_40;
    unsigned char positional_42[2];
    /* Growable per-polygon run the cleanup releases. */
    int* face_indices_44;
    /* Whole source mesh face copied by the polygon builder. */
    W8ReadMeshFace face_48;
    unsigned char positional_71[3];

    unsigned char ContainsPoint004CFB30(const srVector3T<float>* bounds) const;
};

static_assert(sizeof(W8OctRegionPolygon) == 0x74, "W8OctRegionPolygon_must_be_0x74");

extern int g_value_65be60;
extern unsigned long g_value_65be58;
/* Build scratch carries mode-2 region polygons and mode-3 GD surfaces. */
extern void** g_pointer_65be64;
extern W8GDSurface** g_pointer_65be68;
extern unsigned short* g_pointer_65be5c;
extern unsigned short g_value_65be6c;

/* Retail assertion text gives the class and member names directly:
   "OctBuildPreTree::m_ppPolyList too long.", "m_pulRegPaths" and
   "m_psrvRegCenters" in Engine Code\OctBuildPreTree.cpp. */
struct OctBuildPreTree : W8OctBuildTree00446390 {
    OctBuildPreTree(float leaf_size, srVector3T<float>* minimum, srVector3T<float>* maximum,
                    unsigned short item_limit, unsigned long path_capacity, short extent_mode);
    OctPreTree* BuildOctPreTree004B4640();
    unsigned short BuildRegions004B19F0();
    unsigned char BuildParticleRegions004B3820(const W8LevelFileParticleSystem* particles,
                                               int particle_count);
    unsigned char BuildGeometryRegions004B3F90(const W8LevelFileProp* records, int record_count,
                                               int base_index, unsigned char finalize);

    void AssignInitialRegions004B1D90(const W8OctSpatialState* spatial);
    unsigned char UpdateRegionForGeometry004B06E0(const srVector3T<float>* geometry, short value,
                                                  short mode);
    unsigned char UpdateRegionMap004B07E0(const W8OctSpatialState* spatial,
                                          const srVector3T<float>* geometry, short value,
                                          short mode);
    W8OctBuildNode00446330* FindNode004B23F0(unsigned int path);
    unsigned char MergeAdjacentRegion004B2450(W8OctBuildNode00446330* node, unsigned int path);
    unsigned char MergeRegion004B25C0(W8OctBuildNode00446330* node, const int* cell);
    void FinalizeRegionMapping004B2A20();
    void AssignRegionFromSurfaces004B3050(const W8OctSpatialState* spatial);
    void ValidatePolygonRegions004B3330();
    void ValidateRegionBounds004B35B0(const W8BoundingBox* region_bounds);
    /* SortGeometry: welds duplicate vertices, drops degenerate polygons,
       repacks both arrays and re-inserts every polygon with mode 2. */
    unsigned char SortGeometry004AFEA0(W8OctPreTreeGeometry* geometry);
    /* Inserts one region polygon into the octree working state. */
    unsigned char InsertSurface004B02F0(W8OctRegionPolygon* polygon, unsigned long mode);
    /* Loads the .rlk region file beside the level and folds its bounds into
       the build. */
    unsigned char LoadRegionFile004B0C90(const char* stem, srVector3T<float>* minimum,
                                         srVector3T<float>* maximum);
    /* Walks the node tree remapping leaf region ids through positional_100. */
    void RemapNodeRegions004B16B0(W8OctBuildNode00446330* node, int depth);

    unsigned long path_capacity_bc;
    unsigned short selected_depth_c0;
    unsigned short padding_c2;
    unsigned long level_counts_c4[10];
    unsigned long* m_pulRegPaths;
    unsigned long region_path_count_f0;
    unsigned char active_f4;
    unsigned char padding_f5[3];
    BitArray* region_bits_f8;
    srVector3T<float>* m_psrvRegCenters;
    unsigned long positional_100;
    unsigned short* mesh_particle_lookup_104;
    unsigned short* mesh_particles_108;
    unsigned short mesh_particle_count_10c;
    unsigned short padding_10e;
    unsigned short* mesh_prop_lookup_110;
    unsigned short* mesh_props_114;
    unsigned short mesh_prop_count_118;
    unsigned short padding_11a;
    unsigned long particle_count_11c;
    unsigned long prop_count_120;
    W8HashTable<unsigned short, unsigned long>* positional_124;
    W8HashTable<unsigned int, short>* positional_128;
    W8HashTable<unsigned short, short>* positional_12c;
    W8HashTable<unsigned short, short>* positional_130;
    W8OctPreTreeGeometry* game_data_134;
    unsigned long positional_138;
    unsigned long positional_13c;
};

static_assert(sizeof(OctBuildPreTree) == 0x140, "OctBuildPreTree_must_be_0x140");

int GetValue65BE60(void);

#endif
