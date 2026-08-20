#ifndef WIZ8_ENGINE_CODE_OCT_BUILD_PRE_TREE_H
#define WIZ8_ENGINE_CODE_OCT_BUILD_PRE_TREE_H

#include "wiz8/engine_code/OctBuildTree.h"

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

    unsigned long path_capacity_bc;
    unsigned short selected_depth_c0;
    unsigned short padding_c2;
    unsigned long level_counts_c4[10];
    unsigned long* region_paths_ec;
    unsigned long region_path_count_f0;
    unsigned char active_f4;
    unsigned char padding_f5[3];
    void* region_bits_f8;
    void* region_centers_fc;
    unsigned long positional_100;
    unsigned long positional_104;
    unsigned long positional_108;
    unsigned short positional_10c;
    unsigned short padding_10e;
    unsigned long positional_110;
    unsigned long positional_114;
    unsigned short positional_118;
    unsigned short padding_11a;
    unsigned long positional_11c;
    unsigned long positional_120;
    void* positional_124;
    void* positional_128;
    unsigned long positional_12c;
    unsigned long positional_130;
    void* game_data_134;
    unsigned long positional_138;
    unsigned long positional_13c;
};

static_assert(sizeof(W8OctBuildPreTree004AFDA0) == 0x140,
              "W8OctBuildPreTree004AFDA0_must_be_0x140");

int GetValue65BE60(void);

#endif
