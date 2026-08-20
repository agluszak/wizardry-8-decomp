#ifndef WIZ8_ENGINE_CODE_OCT_BUILD_PRE_TREE_H
#define WIZ8_ENGINE_CODE_OCT_BUILD_PRE_TREE_H

#include "wiz8/engine_code/OctBuildTree.h"
#include "wiz8/engine_code/stHash.hpp"

class BitArray;
struct W8VersionedLevelParticleRecord;

struct W8OctRegionVertex004B2A20 {
    unsigned char positional_00[0x0c];
    srVector3T<float> position_0c;
};

struct W8OctRegionPolygon {
    unsigned char positional_00[0x18];
    srVector3T<float> position_18;
    unsigned char positional_24[0x0e];
    unsigned short region_32;
    W8OctRegionVertex004B2A20* vertices_34[3];
    unsigned char positional_40[0x34];

    unsigned char ContainsPoint004CFB30(
        const srVector3T<float>* bounds) const;
};

struct W8OctRegionGameData {
    unsigned long positional_00;
    void* positional_04;
    unsigned long polygon_count_08;
    W8OctRegionPolygon* polygons_0c;
};

static_assert(sizeof(W8OctRegionVertex004B2A20) == 0x18,
              "W8OctRegionVertex004B2A20_must_be_0x18");
static_assert(sizeof(W8OctRegionPolygon) == 0x74,
              "W8OctRegionPolygon_must_be_0x74");

extern "C" {
extern int g_value_65be60;
extern unsigned long g_value_65be58;
extern W8GDSurface** g_pointer_65be64;
extern W8GDSurface** g_pointer_65be68;
extern unsigned short* g_pointer_65be5c;
extern unsigned short g_value_65be6c;
}

#pragma pack(push, 1)

/* The region builder advances these preprocessing records by 0xbf bytes. The
   established tail is a byte count followed by an unaligned pointer to pairs
   of minimum/maximum vectors. */
struct W8OctRegionGeometryRecord004B3F90 {
    unsigned char positional_000[0x9a];
    unsigned char bounds_count_09a;
    srVector3T<float>* bounds_09b;
    unsigned char positional_09f[0x20];
};

#pragma pack(pop)

static_assert(sizeof(W8OctRegionGeometryRecord004B3F90) == 0xbf,
              "W8OctRegionGeometryRecord004B3F90_must_be_0xbf");

struct W8OctBuildPreTree004AFDA0 : W8OctBuildTree00446390 {
    W8OctBuildPreTree004AFDA0(
        float leaf_size,
        srVector3T<float>* minimum,
        srVector3T<float>* maximum,
        unsigned short item_limit,
        unsigned long path_capacity,
        short extent_mode);
    W8OctPreTree004679E0* BuildOctPreTree004B4640();
    unsigned short BuildRegions004B19F0();
    unsigned char BuildParticleRegions004B3820(
        const W8VersionedLevelParticleRecord* particles,
        int particle_count);
    unsigned char BuildGeometryRegions004B3F90(
        const W8OctRegionGeometryRecord004B3F90* records,
        int record_count,
        int base_index,
        unsigned char finalize);

    void AssignInitialRegions004B1D90(
        const W8OctSpatialState0046CCC0* spatial);
    unsigned char UpdateRegionForGeometry004B06E0(
        const srVector3T<float>* geometry,
        short value,
        short mode);
    unsigned char UpdateRegionMap004B07E0(
        const W8OctSpatialState0046CCC0* spatial,
        const srVector3T<float>* geometry,
        short value,
        short mode);
    W8OctBuildNode00446330* FindNode004B23F0(unsigned int path);
    unsigned char MergeAdjacentRegion004B2450(
        W8OctBuildNode00446330* node, unsigned int path);
    unsigned char MergeRegion004B25C0(
        W8OctBuildNode00446330* node, const int* cell);
    void FinalizeRegionMapping004B2A20();
    void AssignRegionFromSurfaces004B3050(
        const W8OctSpatialState0046CCC0* spatial);
    void ValidatePolygonRegions004B3330();
    void ValidateRegionBounds004B35B0(
        const srVector3T<float>* region_bounds);

    unsigned long path_capacity_bc;
    unsigned short selected_depth_c0;
    unsigned short padding_c2;
    unsigned long level_counts_c4[10];
    unsigned long* region_paths_ec;
    unsigned long region_path_count_f0;
    unsigned char active_f4;
    unsigned char padding_f5[3];
    BitArray* region_bits_f8;
    srVector3T<float>* region_centers_fc;
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
    W8OctRegionGameData* game_data_134;
    unsigned long positional_138;
    unsigned long positional_13c;
};

static_assert(sizeof(W8OctBuildPreTree004AFDA0) == 0x140,
              "W8OctBuildPreTree004AFDA0_must_be_0x140");

int GetValue65BE60(void);

#endif
