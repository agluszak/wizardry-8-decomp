#pragma once

#include "surrender/srMath.h"

#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"

class srModelInstance;
class srScene;

/* Element type of W8QuadCell::objects. Retail never populates the list, so
   this layout is recovered only from UpdateWorldMeshFromQuads004BAD40's reads:
   a kind byte (== 1 selects the node-adoption path), an index into the
   embedded pair records at +0x1c whose first member is a node-table pointer,
   and a node slot at +0x30. The true extent of pairs_01c is unobserved; two
   entries are declared so node_slot_030 lands at its probed offset. */
struct W8QuadCellObjectPair {
    void* node_table;
    void* unknown_004;
};

struct W8QuadCellObject {
    unsigned char kind_000;
    unsigned char unknown_001[0x13];
    long pair_index_014;
    long unknown_018;
    W8QuadCellObjectPair pairs_01c[2];
    long unknown_02c;
    long node_slot_030;
};

static_assert(sizeof(W8QuadCellObject) == 0x34, "W8QuadCellObject_must_be_0x34");

struct W8QuadCell {
    W8IList* polygon_indices;
    W8PList* objects;
    unsigned int dirty_stamp_08;
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
W8Quad* BuildWorldQuad004BE200(srModelInstance* instance, int positional_08, float minimum_x_0c,
                               float minimum_y_10, float minimum_z_14, float maximum_x_18,
                               float maximum_y_1c, float maximum_z_20, srScene* scene,
                               int positional_28);

/* Pick-angle helpers (GetHeadingAngle, GetElevationAngle, HeadingToTargetCPP,
   ElevationToTargetCPP) are declared in wiz8/engine_code/PolyPick.h. */
