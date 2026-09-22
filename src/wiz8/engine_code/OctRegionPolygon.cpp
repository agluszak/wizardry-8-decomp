#include "wiz8/engine_code/OctBuildPreTree.h"
#include "wiz8/engine_code/materials.h"
#include "wiz8/float_constants.h"

#include <stdlib.h>
#include <string.h>

/* Test the polygon's representative point against six frustum planes; inside
   means every plane distance is non-negative. */
// FUNCTION: WIZ8 0x004cfae0
unsigned char W8OctRegionPolygon::InsideFrustumPlanes004CFAE0(const W8Plane* planes) const
{
    for (short plane = 0; plane < 6; ++plane) {
        float distance = planes[plane].normal.x * position_18.x +
                         planes[plane].normal.y * position_18.y +
                         planes[plane].normal.z * position_18.z + planes[plane].w;
        if (distance < g_float_005ebb34) {
            return 0;
        }
    }
    return 1;
}

/* Test the polygon's representative point against an inclusive axis-aligned
   box. The method's original translation-unit owner is not yet proved. */
// FUNCTION: WIZ8 0x004cfb30
unsigned char W8OctRegionPolygon::ContainsPoint004CFB30(const srVector3T<float>* bounds) const
{
    for (short axis = 0; axis < 3; ++axis) {
        float value = (&position_18.x)[axis];
        if (value < (&bounds[0].x)[axis] || (&bounds[1].x)[axis] < value) {
            return 0;
        }
    }
    return 1;
}

/* Grow `*run` to (count + capacity) dwords when count lands on a capacity
   boundary, preserving existing entries. */
// FUNCTION: WIZ8 0x004cfb70
int CheckArrayLength004CFB70(int** run, unsigned short count, unsigned short capacity)
{
    if (count % capacity == 0) {
        unsigned short total = count + capacity;
        int* grown = static_cast<int*>(malloc(total * 4));
        if (grown != 0) {
            memset(grown, 0, total * 4);
            if (*run != 0) {
                memcpy(grown, *run, count * 4);
                free(*run);
            }
            *run = grown;
            return 1;
        }
        ReportBuildStatus00497690(7, "CheckArrayLength: Could not allocate new array.\n");
    }
    return 0;
}

/* Free every run the geometry owns: per-vertex face-index and owned arrays
   (consecutive vertices may share one allocation, so each distinct pointer is
   freed once when it changes), the vertex and polygon arrays themselves, and
   the owned +0x14 buffer. */
// FUNCTION: WIZ8 0x004cfc10
void W8OctPreTreeGeometry::Release004CFC10()
{
    if (vertices_04 != 0) {
        int* last_faces = vertices_04[1].face_indices_44;
        int* last_owned = vertices_04[1].owned_48;
        for (unsigned long index = 1; index < vertex_count_00; ++index) {
            W8OctPreTreeVertex* vertex = &vertices_04[index];
            if (vertex->face_indices_44 != 0 && vertex->face_indices_44 != last_faces) {
                if (last_faces != 0) {
                    free(last_faces);
                }
                last_faces = vertex->face_indices_44;
                vertex->face_indices_44 = 0;
            }
            if (vertex->owned_48 != 0 && vertex->owned_48 != last_owned) {
                if (last_owned != 0) {
                    free(last_owned);
                }
                last_owned = vertex->owned_48;
                vertex->owned_48 = 0;
            }
        }
        if (last_faces != 0) {
            free(last_faces);
        }
        if (last_owned != 0) {
            free(last_owned);
        }
        free(vertices_04);
    }
    if (polygons_0c != 0) {
        int* last_faces = polygons_0c[1].face_indices_44;
        for (unsigned long index = 1; index < polygon_count_08; ++index) {
            W8OctRegionPolygon* polygon = &polygons_0c[index];
            if (polygon->face_indices_44 != 0 && polygon->face_indices_44 != last_faces) {
                if (last_faces != 0) {
                    free(last_faces);
                }
                last_faces = polygon->face_indices_44;
                polygon->face_indices_44 = 0;
            }
        }
        if (last_faces != 0) {
            free(last_faces);
        }
        free(polygons_0c);
    }
    if (owned_14 != 0) {
        free(owned_14);
    }
}
