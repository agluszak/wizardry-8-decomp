#include "wiz8/engine_code/OctBuildPreTree.h"
#include "wiz8/engine_code/OctPreTree.h"
#include "wiz8/engine_code/materials.h"
#include "wiz8/float_constants.h"

#include <stdlib.h>
#include <string.h>

/* Test the polygon's representative point against six frustum planes; inside
   means every plane distance is non-negative. */
// FUNCTION: WIZ8 0x004cfae0
bool W8OctRegionPolygon::InsideFrustumPlanes(const W8Plane* planes) const
{
    for (short plane = 0; plane < 6; ++plane) {
        float distance = DotProduct(planes[plane].normal, position) + planes[plane].w;
        if (distance < g_float_zero) {
            return false;
        }
    }
    return true;
}

/* Test the polygon's representative point against an inclusive axis-aligned
   box. The method's original translation-unit owner is not yet proved. */
// FUNCTION: WIZ8 0x004cfb30
bool W8OctRegionPolygon::ContainsPoint(const srVector3T<float>* bounds) const
{
    for (short axis = 0; axis < 3; ++axis) {
        float value = (&position.x)[axis];
        if (value < (&bounds[0].x)[axis] || (&bounds[1].x)[axis] < value) {
            return false;
        }
    }
    return true;
}

/* Grow `*run` to (count + capacity) dwords when count lands on a capacity
   boundary, preserving existing entries. */
// FUNCTION: WIZ8 0x004cfb70
unsigned char W8OctPreTreeGeometry::CheckArrayLength(int** run, unsigned short count,
                                                     unsigned short capacity)
{
    if (count % capacity == 0) {
        unsigned short total = count + capacity;
        int* grown = static_cast<int*>(malloc(total * sizeof(int)));
        if (grown != 0) {
            memset(grown, 0, total * sizeof(*grown));
            if (*run != 0) {
                memcpy(grown, *run, count * sizeof(*grown));
                free(*run);
            }
            *run = grown;
            return 1;
        }
        ReportBuildStatus(7, "CheckArrayLength: Could not allocate new array.\n");
    }
    return 0;
}

/* Free every run the geometry owns: per-vertex face-index and owned arrays
   (consecutive vertices may share one allocation, so each distinct pointer is
   freed once when it changes), the vertex and polygon arrays themselves, and
   the owned +0x14 buffer. */
// FUNCTION: WIZ8 0x004cfc10
void W8OctPreTreeGeometry::Release()
{
    if (m_vertices != 0) {
        int* last_faces = m_vertices[1].face_indices;
        int* last_owned = m_vertices[1].owned;
        for (unsigned long index = 1; index < vertex_count; ++index) {
            W8OctPreTreeVertex* vertex = &m_vertices[index];
            if (vertex->face_indices != 0 && vertex->face_indices != last_faces) {
                if (last_faces != 0) {
                    free(last_faces);
                }
                last_faces = vertex->face_indices;
                vertex->face_indices = 0;
            }
            if (vertex->owned != 0 && vertex->owned != last_owned) {
                if (last_owned != 0) {
                    free(last_owned);
                }
                last_owned = vertex->owned;
                vertex->owned = 0;
            }
        }
        if (last_faces != 0) {
            free(last_faces);
        }
        if (last_owned != 0) {
            free(last_owned);
        }
        free(m_vertices);
    }
    if (m_polygons != 0) {
        int* last_faces = m_polygons[1].face_indices;
        for (unsigned long index = 1; index < m_polygon_count; ++index) {
            W8OctRegionPolygon* polygon = &m_polygons[index];
            if (polygon->face_indices != 0 && polygon->face_indices != last_faces) {
                if (last_faces != 0) {
                    free(last_faces);
                }
                last_faces = polygon->face_indices;
                polygon->face_indices = 0;
            }
        }
        if (last_faces != 0) {
            free(last_faces);
        }
        free(m_polygons);
    }
    if (owned != 0) {
        free(owned);
    }
}
