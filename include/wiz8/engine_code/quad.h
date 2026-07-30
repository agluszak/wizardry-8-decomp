#pragma once

#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"

class srModelInstance;
class srScene;

struct W8QuadCell {
    W8IList* polygon_indices;
    W8PList* objects;
    unsigned int value_08;
    unsigned char occupied;
    unsigned char padding_0d[3];
};

static_assert(sizeof(W8QuadCell) == 0x10, "W8QuadCell_must_be_0x10");

struct W8QuadRow {
    W8QuadCell* cells;
    unsigned int count;
};

static_assert(sizeof(W8QuadRow) == 0x8, "W8QuadRow_must_be_0x8");

struct W8Quad {
    unsigned int row_count;
    unsigned int column_count;
    float origin_x;
    float origin_z;
    float cell_size;
    W8QuadRow* rows;
    unsigned int dirty;
};

static_assert(sizeof(W8Quad) == 0x1c, "W8Quad_must_be_0x1c");

void DestroyWorldQuad004BE0A0(W8Quad* quad);
W8Quad* BuildWorldQuad004BE200(
    srModelInstance* instance, int positional_08,
    float positional_0c, float positional_10, float positional_14,
    float positional_18, float positional_1c, float positional_20,
    srScene* scene, int positional_28);
