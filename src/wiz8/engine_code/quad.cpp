#include "wiz8/engine_code/quad.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

#include "surrender/srMeshModel.h"
#include "surrender/srModelInstance.h"
#include "wiz8/sr_api.h"

#define QUAD_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\quad.cpp"

float g_quad_cell_size_0060e5d0 = 5500.0f;

// FUNCTION: WIZ8 0x004BE0A0
void DestroyWorldQuad004BE0A0(W8Quad* quad)
{
    unsigned int row;

    for (row = 0; row < quad->row_count; ++row) {
        W8QuadCell* cells = quad->rows[row].cells;

        if (cells != 0) {
            if (cells->polygon_indices != 0) {
                IListDestroy(cells->polygon_indices);
            }
            if (cells->objects != 0) {
                PListDestroy(cells->objects);
            }
            free(cells);
        }
    }
    free(quad->rows);
    free(quad);
}

// FUNCTION: WIZ8 0x004BE100
static W8QuadCell* GetPolygonQuadCell004BE100(
    W8Quad* quad, srModelInstance* instance, int polygon,
    unsigned int* row, unsigned int* column,
    float origin_x, float origin_z)
{
    srMeshModel* model = static_cast<srMeshModel*>(instance->model());
    srVector3i* polygon_vertices = model->getPolyVertex();
    srVector3T<float>* vertices = model->getVertexLoc();
    srVector3T<float> center = {0.0f, 0.0f, 0.0f};
    int vertex;

    for (vertex = 0; vertex < 3; ++vertex) {
        int index = (&polygon_vertices[polygon].x)[vertex];
        center.x += vertices[index].x;
        center.y += vertices[index].y;
        center.z += vertices[index].z;
    }

    center.x *= 0.33333334f;
    center.z *= 0.33333334f;
    *row = static_cast<unsigned int>(
        fabs((center.x - origin_x) / g_quad_cell_size_0060e5d0));
    *column = static_cast<unsigned int>(
        fabs((center.z - origin_z) / g_quad_cell_size_0060e5d0));

    if (*row < quad->row_count && *column < quad->column_count) {
        return &quad->rows[*row].cells[*column];
    }
    return 0;
}

// FUNCTION: WIZ8 0x004BE200
W8Quad* BuildWorldQuad004BE200(
    srModelInstance* instance, int,
    float positional_0c, float positional_10, float positional_14,
    float positional_18, float, float positional_20, srScene*, int)
{
    unsigned int row_count = static_cast<unsigned int>(
        (positional_18 - positional_10) / g_quad_cell_size_0060e5d0) + 1;
    unsigned int column_count = static_cast<unsigned int>(
        (positional_20 - positional_14) / g_quad_cell_size_0060e5d0) + 1;
    W8Quad* quad = static_cast<W8Quad*>(malloc(sizeof(W8Quad)));
    unsigned int row;

    memset(quad, 0, sizeof(W8Quad));
    if (quad == 0) {
        return 0;
    }

    quad->rows = static_cast<W8QuadRow*>(
        malloc(row_count * sizeof(W8QuadRow)));
    memset(quad->rows, 0, row_count * sizeof(W8QuadRow));
    quad->row_count = row_count;
    quad->column_count = column_count;
    quad->dirty = 0;

    for (row = 0; row < row_count; ++row) {
        W8QuadCell* cells = static_cast<W8QuadCell*>(
            malloc(column_count * sizeof(W8QuadCell)));
        unsigned int column;

        if (cells == 0) {
            srAssertFail("pq", QUAD_CPP, 0x3a, 0);
        }
        memset(cells, 0, column_count * sizeof(W8QuadCell));
        for (column = 0; cells != 0 && column < column_count; ++column) {
            cells[column].polygon_indices = 0;
            cells[column].objects = 0;
            cells[column].value_08 = 0;
            cells[column].occupied = 0;
        }
        quad->rows[row].cells = cells;
        quad->rows[row].count = column_count;
    }

    srMeshModel* model = static_cast<srMeshModel*>(instance->model());
    for (int polygon = 0; polygon < model->polygon_count_230; ++polygon) {
        unsigned int polygon_row;
        unsigned int polygon_column;
        W8QuadCell* cell = GetPolygonQuadCell004BE100(
            quad, instance, polygon, &polygon_row, &polygon_column,
            positional_0c, positional_14);

        cell->occupied = 1;
        if (cell->polygon_indices == 0) {
            cell->polygon_indices = IListCreate();
        }
        IListAdd(cell->polygon_indices, polygon);
    }

    quad->cell_size = g_quad_cell_size_0060e5d0;
    quad->origin_x = positional_0c;
    quad->origin_z = positional_14;
    return quad;
}
