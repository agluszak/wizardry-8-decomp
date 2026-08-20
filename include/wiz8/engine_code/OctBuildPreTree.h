#ifndef WIZ8_ENGINE_CODE_OCT_BUILD_PRE_TREE_H
#define WIZ8_ENGINE_CODE_OCT_BUILD_PRE_TREE_H

#include "wiz8/engine_code/OctBuildTree.h"
#include "wiz8/engine_code/stHash.hpp"

class BitArray;
struct W8OctRegionGameData;

extern "C" {
extern int g_value_65be60;
extern unsigned long g_value_65be58;
extern W8GDSurface** g_pointer_65be64;
extern W8GDSurface** g_pointer_65be68;
}

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

    void Function004B1D90(const W8OctSpatialState0046CCC0* spatial);
    unsigned char UpdateRegionMap004B07E0(
        const W8OctSpatialState0046CCC0* spatial,
        const srVector3T<float>* geometry,
        short value,
        short mode);
    W8OctBuildNode00446330* FindNode004B23F0(unsigned int path);
    unsigned char Function004B2450(
        W8OctBuildNode00446330* node, unsigned int path);
    unsigned char MergeRegion004B25C0(
        W8OctBuildNode00446330* node, const int* cell);
    void Function004B2A20();
    void Function004B3050(const W8OctSpatialState0046CCC0* spatial);
    void Function004B3330();

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
    void* positional_104;
    void* positional_108;
    unsigned short positional_10c;
    unsigned short padding_10e;
    void* positional_110;
    void* positional_114;
    unsigned short positional_118;
    unsigned short padding_11a;
    unsigned long positional_11c;
    unsigned long positional_120;
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
