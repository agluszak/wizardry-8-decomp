#pragma once

#include "surrender/srMath.h"

#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"

class srModelInstance;
class srScene;

struct W8QuadCell {
    W8IList* polygon_indices;
    W8PList* objects;
    unsigned int dirty_stamp;
    bool occupied;
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
    srVector2T<float> origin;
    float cell_size;
    W8QuadRow* rows;
    unsigned int dirty;
};

static_assert(sizeof(W8Quad) == 0x1c, "W8Quad_must_be_0x1c");

void DestroyWorldQuad(W8Quad* quad);
W8Quad* BuildWorldQuad(srModelInstance* instance, int positional_08, float minimum_x,
                       float minimum_y, float minimum_z, float maximum_x,
                       float maximum_y, float maximum_z, srScene* scene, int positional_28);

/* Pick-angle helpers (GetHeadingAngle, GetElevationAngle, HeadingToTargetCPP,
   ElevationToTargetCPP) are declared in wiz8/engine_code/PolyPick.h. */
