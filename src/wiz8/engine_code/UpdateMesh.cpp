#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/UpdateMesh.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/quad.h"
#include "wiz8/engine_code/3dapi.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/sr_api.h"

#include <math.h>

#include "surrender/srMeshModel.h"
#include "surrender/srModelInstance.h"
#include "surrender/srCamera.h"

/* Yaw-spread factor paired with g_float_00609c88 when the two view-triangle
   edges are rotated off the camera forward vector; retail never writes it, so
   both rotations collapse to zero and the triangle degenerates to a ray. */
// GLOBAL: WIZ8 0x005ED168
float g_float_005ed168 = 0.01745329424738884f;

/* Camera-visible quad-cell coordinates and count: rows[]/cells[] index pairs
   into W8Quad, filled by CollectViewQuadCells004BA530 and consumed by
   UpdateWorldMeshFromQuads004BAD40. */
// GLOBAL: WIZ8 0x0065BEB8
long g_visible_quad_rows_0065beb8[20000];
// GLOBAL: WIZ8 0x0066F738
long g_visible_quad_columns_0066f738[20000];
// GLOBAL: WIZ8 0x00682FB8
long g_visible_quad_count_00682fb8;

static int ScanQuadTriangleBase004BAA00(W8World* world, long x1, long y1, long x2, long y2, long x3,
                                        long y3, long* x_list, long* y_list, long* count);
static void ScanQuadTriangleTop004BA800(W8World* world, long x1, long y1, long x2, long y2, long x3,
                                        long y3, long* x_list, long* y_list, long* count,
                                        int skip_boundary);
static void RasterizeQuadTriangle004BABE0(W8World* world, long x1, long y1, long x2, long y2,
                                          long x3, long y3, long* x_list, long* y_list,
                                          long* count);

/* Builds the camera view triangle in quad-cell space: the camera's own cell is
   the apex and the two far points sit view-distance ahead along the camera
   forward vector, optionally yawed by +/- (g_float_00609c88 * g_float_005ed168).
   The factor global is never written in retail, so both edges keep the camera
   forward direction and the rasterized region is a thin wedge. */
// FUNCTION: WIZ8 0x004BA530
static void CollectViewQuadCells004BA530(W8World* world, long* x_list, long* y_list, long* count)
{
    W8Quad* quad = world->m_owned_06c;
    float cell_size = quad->cell_size;

    long cam_x = static_cast<long>((world->camera->getLocationX() - quad->origin_x) / cell_size);
    long cam_y = static_cast<long>((world->camera->getLocationZ() - quad->origin_z) / cell_size);

    srMatrix3T<float> rotation;
    world->camera->getRotation(rotation);

    srVector3T<float> direction(0.0f, 0.0f, world->value_78);
    srMatrix3T<float> work = rotation;
    double angle = -g_float_00609c88 * g_float_005ed168;
    if (angle != 0.0) {
        work.RotateAboutY(sin(angle), cos(angle));
    }
    direction = work.Transform(direction);
    long edge1_x = cam_x + static_cast<long>(direction.x / cell_size);
    long edge1_y = cam_y + static_cast<long>(direction.z / cell_size);

    direction.x = 0.0f;
    direction.y = 0.0f;
    direction.z = world->value_78;
    work = rotation;
    angle = g_float_00609c88 * g_float_005ed168;
    if (angle != 0.0) {
        srVector3T<float> first;
        srVector3T<float> second;
        srVector3T<float> third;
        srMatrix3T<float> yaw;
        yaw.SetRows(*first.Set(cos(angle), 0.0, sin(angle)), *second.Set(0.0, 1.0, 0.0),
                    *third.Set(-sin(angle), 0.0, cos(angle)));
        work.MultiplyBy(yaw);
    }
    direction = work.Transform(direction);
    long edge2_x = cam_x + static_cast<long>(direction.x / cell_size);
    long edge2_y = cam_y + static_cast<long>(direction.z / cell_size);

    *count = 0;
    RasterizeQuadTriangle004BABE0(world, cam_x, cam_y, edge1_x, edge1_y, edge2_x, edge2_y, x_list,
                                  y_list, count);
}

/* Scans the upper half of a cell-space triangle: a flat top edge between
   (x1,y1) and (x2,y2) converging to the apex (x3,y3). Each emitted row is
   padded one cell outward on both sides. skip_boundary suppresses the first
   row when the lower half already emitted it. */
// FUNCTION: WIZ8 0x004BA800
static void ScanQuadTriangleTop004BA800(W8World* world, long x1, long y1, long x2, long y2, long x3,
                                        long y3, long* x_list, long* y_list, long* count,
                                        int skip_boundary)
{
    if (x2 < x1) {
        long temp = x1;
        x1 = x2;
        x2 = temp;
    }
    long row = y1;
    float left_step = (x3 - x1) / static_cast<float>(y3 - y1);
    float right_step = (x3 - x2) / static_cast<float>(y3 - y1);
    float left_edge = x1;
    float right_edge = x2 + 0.5f;
    if (row < 0) {
        left_edge -= row * left_step;
        right_edge -= row * right_step;
        row = y1 = 0;
    }
    long max_row = static_cast<long>(world->m_owned_06c->column_count) - 1;
    if (y3 > max_row) {
        y3 = max_row;
    }
    long max_x = static_cast<long>(world->m_owned_06c->row_count) - 1;

    if (x1 < 0 || x1 > max_x || x2 < 0 || x2 > max_x || x3 < 0 || x3 > max_x) {
        for (; row <= y3; ++row) {
            long left = static_cast<long>(left_edge);
            long right = static_cast<long>(right_edge);
            left_edge += left_step;
            right_edge += right_step;
            if (left < 0) {
                left = 0;
                if (right < 0) {
                    continue;
                }
            }
            if (right > max_x) {
                right = max_x;
                if (left > max_x) {
                    continue;
                }
            }
            if (row < 0) {
                continue;
            }
            if (skip_boundary && row == y1) {
                continue;
            }
            if (left > 0) {
                --left;
            }
            if (right < max_x) {
                ++right;
            }
            for (long x = left; x <= right; ++x) {
                x_list[*count] = x;
                y_list[*count] = row;
                ++*count;
            }
        }
        return;
    }
    for (; row <= y3; ++row) {
        if (!skip_boundary || row != y1) {
            long right = static_cast<long>(right_edge);
            long left = static_cast<long>(left_edge);
            if (left > 0) {
                --left;
            }
            if (right < max_x) {
                ++right;
            }
            for (long x = left; x <= right; ++x) {
                x_list[*count] = x;
                y_list[*count] = row;
                ++*count;
            }
        }
        left_edge += left_step;
        right_edge += right_step;
    }
}

/* Scans the lower half of a cell-space triangle: the apex (x1,y1) fanning out
   to the flat base between (x2,y2) and (x3,y3). Each emitted row is padded one
   cell outward on both sides. Returns whether any row survived clipping. */
// FUNCTION: WIZ8 0x004BAA00
static int ScanQuadTriangleBase004BAA00(W8World* world, long x1, long y1, long x2, long y2, long x3,
                                        long y3, long* x_list, long* y_list, long* count)
{
    int emitted = 0;
    long x_left = x2;
    long x_right = x3;
    if (x_right < x_left) {
        x_left = x3;
        x_right = x2;
    }
    long row = y1;
    float left_step = (x_left - x1) / static_cast<float>(y3 - y1);
    float right_step = (x_right - x1) / static_cast<float>(y3 - y1);
    float left_edge = x1;
    float right_edge = x1 + 0.5f;
    if (row < 0) {
        left_edge -= row * left_step;
        right_edge -= row * right_step;
        row = 0;
    }
    long max_row = static_cast<long>(world->m_owned_06c->column_count) - 1;
    if (y3 > max_row) {
        y3 = max_row;
    }
    long max_x = static_cast<long>(world->m_owned_06c->row_count) - 1;

    if (x1 < 0 || x1 > max_x || x_left < 0 || x_left > max_x || x_right < 0 || x_right > max_x) {
        for (; row <= y3; ++row) {
            long left = static_cast<long>(left_edge);
            long right = static_cast<long>(right_edge);
            left_edge += left_step;
            right_edge += right_step;
            if (left < 0) {
                left = 0;
                if (right < 0) {
                    continue;
                }
            }
            if (right > max_x) {
                right = max_x;
                if (left > max_x) {
                    continue;
                }
            }
            if (left > 0) {
                --left;
            }
            if (right < max_x) {
                ++right;
            }
            if (left <= right) {
                for (long x = left; x <= right; ++x) {
                    x_list[*count] = x;
                    y_list[*count] = row;
                    ++*count;
                }
                emitted = 1;
            }
        }
        return emitted;
    }
    for (; row <= y3; ++row) {
        long right = static_cast<long>(right_edge);
        long left = static_cast<long>(left_edge);
        if (left > 0) {
            --left;
        }
        if (right < max_x) {
            ++right;
        }
        for (long x = left; x <= right; ++x) {
            x_list[*count] = x;
            y_list[*count] = row;
            ++*count;
        }
        left_edge += left_step;
        right_edge += right_step;
        emitted = 1;
    }
    return emitted;
}

/* Rasterizes a cell-space triangle into (x,y) coordinate pairs: sorts the
   vertices by y, then scans a flat-top half, a flat-bottom half, or both halves
   split at the middle vertex's row. */
// FUNCTION: WIZ8 0x004BABE0
static void RasterizeQuadTriangle004BABE0(W8World* world, long x1, long y1, long x2, long y2,
                                          long x3, long y3, long* x_list, long* y_list, long* count)
{
    if ((x1 == x2 && x2 == x3) || (y1 == y2 && y2 == y3)) {
        return;
    }
    long temp;
    if (y2 < y1) {
        temp = x1;
        x1 = x2;
        x2 = temp;
        temp = y1;
        y1 = y2;
        y2 = temp;
    }
    if (y3 < y1) {
        temp = x1;
        x1 = x3;
        x3 = temp;
        temp = y1;
        y1 = y3;
        y3 = temp;
    }
    if (y3 < y2) {
        temp = x2;
        x2 = x3;
        x3 = temp;
        temp = y2;
        y2 = y3;
        y3 = temp;
    }
    if (y1 == y2) {
        ScanQuadTriangleTop004BA800(world, x1, y1, x2, y2, x3, y3, x_list, y_list, count, 0);
        return;
    }
    if (y2 == y3) {
        ScanQuadTriangleBase004BAA00(world, x1, y1, x2, y2, x3, y3, x_list, y_list, count);
        return;
    }
    long split = static_cast<long>(static_cast<float>(x3 - x1) * (y2 - y1) / (y3 - y1));
    int emitted =
        ScanQuadTriangleBase004BAA00(world, x1, y1, x1 + split, y2, x2, y2, x_list, y_list, count);
    ScanQuadTriangleTop004BA800(world, x2, y2, x1 + split, y2, x3, y3, x_list, y_list, count,
                                emitted != 0);
}

// FUNCTION: WIZ8 0x004BAD40
void UpdateWorldMeshFromQuads004BAD40(W8World* world)
{
    long polygon_count = 0;
    if (world == 0) {
        srAssertFail("pWorld", "C:\\Projects\\Wizardry 8\\Engine Code\\UpdateMesh.cpp", 0x21f, 0);
    }
    if (world->m_owned_06c == 0) {
        srAssertFail("pWorld->Tree.pQuads", "C:\\Projects\\Wizardry 8\\Engine Code\\UpdateMesh.cpp",
                     0x220, 0);
    }
    srMeshModel* mesh = static_cast<srMeshModel*>(world->update_mesh_source->model());
    W8Quad* quad = world->m_owned_06c;
    g_visible_quad_count_00682fb8 = 0;
    CollectViewQuadCells004BA530(world, g_visible_quad_rows_0065beb8,
                                 g_visible_quad_columns_0066f738, &g_visible_quad_count_00682fb8);
    long index;
    for (index = 0; index < g_visible_quad_count_00682fb8; ++index) {
        W8QuadCell* cell = &quad->rows[g_visible_quad_rows_0065beb8[index]]
                                .cells[g_visible_quad_columns_0066f738[index]];
        if (cell != 0 && cell->polygon_indices != 0) {
            polygon_count += ILLength(cell->polygon_indices);
        }
    }
    mesh->setActivePolygonCount(polygon_count);
    unsigned long* table = mesh->getActivePolygonTable(1);
    long offset = 0;
    for (index = 0; index < g_visible_quad_count_00682fb8; ++index) {
        W8QuadCell* cell = &quad->rows[g_visible_quad_rows_0065beb8[index]]
                                .cells[g_visible_quad_columns_0066f738[index]];
        if (cell != 0 && (cell->polygon_indices != 0 || cell->objects != 0)) {
            long j;
            if (cell->objects != 0 && cell->value_08 < quad->dirty) {
                cell->value_08 = quad->dirty;
                long object_count = static_cast<long>(PLLength(cell->objects));
                for (j = 0; j < object_count; ++j) {
                    char* item = static_cast<char*>(PLGet(cell->objects, j));
                    if (item[0] == 1) {
                        long entry =
                            *reinterpret_cast< // reinterpret-ok: quad-cell object elements are never populated in retail; element layout is unresolved
                                long*>(item + 0x14);
                        void** nodes =
                            *reinterpret_cast< // reinterpret-ok: embedded pair-table at +0x1c resolved by retail offset
                                void***>(item + entry * 8 + 0x1c);
                        long slot = *reinterpret_cast< // reinterpret-ok: slot index field at +0x30
                            long*>(item + 0x30);
                        PLAdoptAppend(&world->m_list_09c, nodes[slot]);
                    }
                }
            }
            long index_count = static_cast<long>(ILLength(cell->polygon_indices));
            for (j = 0; j < index_count; ++j) {
                table[offset] = IListGetAt(cell->polygon_indices, j);
                ++offset;
            }
        }
    }
    mesh->setControlMask(0);
}

// FUNCTION: WIZ8 0x004BAF50
void UpdateWorldOctree004BAF50(W8World* world)
{
    world->octree->UpdateCameraVisibility0042F7E0();
}

/* Rebuild the active-polygon table for the world's mesh and raise the control
   changed bit through the canonical mesh setter; the retail site is the
   mask-0 expansion of setControlMask. */
// FUNCTION: WIZ8 0x004baf60
void UpdateWorldMesh004BAF60(W8World* world)
{
    srModelInstance* source;
    srMeshModel* mesh;
    unsigned long* table;
    long polygon_count;
    unsigned long index;

    if (world == 0) {
        srAssertFail("pWorld", "C:\\Projects\\Wizardry 8\\Engine Code\\UpdateMesh.cpp", 0x2a2, 0);
    }
    source = world->update_mesh_source;
    if (source != 0) {
        mesh = static_cast<srMeshModel*>(source->model());
        polygon_count = mesh->polygon_count_230;
        mesh->setActivePolygonCount(polygon_count);
        table = mesh->getActivePolygonTable(1);
        for (index = 0; static_cast<long>(index) < polygon_count; ++index) {
            table[index] = index;
        }
        mesh->setControlMask(0);
    }
}
