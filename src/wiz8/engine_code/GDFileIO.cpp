#include "wiz8/engine_code/GDFileIO.h"

#include "wiz8/engine_code/Trigger.h"
#include "wiz8/sr_api.h"

#include <math.h>
#include <stdlib.h>

struct GDTriggerSurface {
    unsigned int flags_00;
    int index_04;
    int trigger_index_08;
    int unknown_0c;
    int unknown_10;
    int unknown_14;
    int vertex_indices_18[3];
    float plane_24[4];
    int unknown_34;
    int value_38;
    int unknown_3c;
    float value_40;
    int unknown_44;
    float value_48;
};

static int g_trigger_array_count_00659a58;

/* The three points are coplanar by construction. The retail helper averages
   their three signed distances after normalizing the triangle normal. */
// FUNCTION: WIZ8 0x00449a40
static void CalculateTriggerPlane00449A40(
    float* plane, const srVector3T<float>* first,
    const srVector3T<float>* second, const srVector3T<float>* third)
{
    srVector3T<float> edge_1;
    srVector3T<float> edge_2;
    float length;

    edge_1.x = second->x - first->x;
    edge_1.y = second->y - first->y;
    edge_1.z = second->z - first->z;
    edge_2.x = third->x - first->x;
    edge_2.y = third->y - first->y;
    edge_2.z = third->z - first->z;

    plane[0] = edge_1.y * edge_2.z - edge_1.z * edge_2.y;
    plane[1] = edge_1.z * edge_2.x - edge_1.x * edge_2.z;
    plane[2] = edge_1.x * edge_2.y - edge_1.y * edge_2.x;
    length = static_cast<float>(sqrt(
        plane[0] * plane[0] + plane[1] * plane[1] +
        plane[2] * plane[2]));
    plane[0] /= length;
    plane[1] /= length;
    plane[2] /= length;
    plane[3] = -(
        first->x * plane[0] + first->y * plane[1] + first->z * plane[2] +
        second->x * plane[0] + second->y * plane[1] + second->z * plane[2] +
        third->x * plane[0] + third->y * plane[1] + third->z * plane[2]) /
        3.0f;
}

// FUNCTION: WIZ8 0x004498c0
static void ClassifyTriggerSurface004498C0(
    const srVector3T<float>* vertices, GDTriggerSurface* surface)
{
    int dominant = 0;
    int index;
    float magnitude = 0.0f;

    CalculateTriggerPlane00449A40(
        surface->plane_24,
        vertices + surface->vertex_indices_18[0],
        vertices + surface->vertex_indices_18[1],
        vertices + surface->vertex_indices_18[2]);
    if ((surface->flags_00 & 0x80) != 0) {
        for (index = 0; index < 3; ++index) {
            if (magnitude < static_cast<float>(fabs(surface->plane_24[index]))) {
                magnitude = static_cast<float>(fabs(surface->plane_24[index]));
                dominant = index;
            }
        }
        surface->flags_00 |= dominant;
    }
    if ((surface->flags_00 & 4) != 0) {
        surface->flags_00 |= 0x40;
    }
    if (surface->plane_24[1] > 0.5f) {
        if ((surface->flags_00 & 4) == 0 && surface->plane_24[1] > 0.996f) {
            surface->flags_00 |= 4;
            surface->value_48 = 1.0f;
        }
        if (surface->value_48 < 0.0f) {
            surface->flags_00 |= 0x20;
            surface->value_48 = 0.0f;
        }
    }
    else if (surface->value_40 < 1.01f && surface->value_40 > 0.99f &&
             (surface->flags_00 & 4) != 0) {
        surface->value_40 = 0.1f;
    }
    if ((surface->flags_00 & 4) == 0) {
        surface->value_40 *= 500.0f;
        surface->value_48 = 0.0f;
    }
    else {
        surface->value_40 *= 500.0f;
        if (surface->value_48 < 1.0e-7f &&
            (surface->flags_00 & 0x20) == 0) {
            surface->value_48 = surface->plane_24[1] <= 0.75f
                ? surface->plane_24[1] : 1.0f;
        }
    }
    surface->flags_00 &= ~8;
}

// FUNCTION: WIZ8 0x00448310
void GDFileIO::AddTriggerPlane(
    const srVector3T<float>* vertices, Trigger* trigger)
{
    int trigger_index = 0;
    int index;

    if (trigger_array_mode_004 != 0) {
        if (triggers_050 == 0) {
            g_trigger_array_count_00659a58 = 0;
            triggers_050 = static_cast<Trigger**>(
                malloc(trigger_count_044 * sizeof(Trigger*) + sizeof(Trigger*)));
            if (triggers_050 == 0) {
                srAssertFail("m_ppTriggers",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                             0x256, "AddTriggerPlane: Couldn't allocate trigger array.");
            }
        }
        if (g_trigger_array_count_00659a58 >= trigger_count_044) {
            srAssertFail("iTriggerCount < m_iNumTriggers",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                         0x259, "AddTriggerPlane: Too many triggers for trigger array.");
        }
        triggers_050[g_trigger_array_count_00659a58++] = trigger;
        return;
    }

    if (trigger_surfaces_048 == 0) {
        trigger_surfaces_048 = malloc(500 * sizeof(GDTriggerSurface));
        if (trigger_surfaces_048 == 0) {
            srAssertFail("m_pTrigSurfaces",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                         0x263, "AddTriggerPlane: Couldn't allocate trigger surfaces.");
        }
        trigger_vertices_04c = new srVector3T<float>[1000];
        if (trigger_vertices_04c == 0) {
            srAssertFail("m_pTrigVertices",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                         0x265, "AddTriggerPlane: Couldn't allocate trigger vertices.");
        }
        triggers_050 = static_cast<Trigger**>(malloc(500 * sizeof(Trigger*)));
        if (triggers_050 == 0) {
            srAssertFail("m_ppTriggers",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                         0x267, "AddTriggerPlane: Couldn't allocate trigger array.");
        }
        trigger_surface_count_03c = 0;
        trigger_vertex_count_040 = 0;
        trigger_count_044 = 0;
    }
    if (trigger_surface_count_03c >= 500) {
        srAssertFail("m_iNumTrigSurfaces < MAX_TRIG_SURFACES",
                     "C:\\Projects\\Wizardry 8\\Engine Code\\GDFileIO.cpp",
                     0x26c, 0);
    }
    for (index = 0; index < trigger_count_044; ++index) {
        if (triggers_050[index] == trigger) {
            trigger_index = index;
            break;
        }
    }
    if (index == trigger_count_044) {
        trigger_index = trigger_count_044++;
        triggers_050[trigger_index] = trigger;
    }
    for (index = 0; index < 4; ++index) {
        trigger_vertices_04c[trigger_vertex_count_040++] = vertices[index];
    }

    GDTriggerSurface* surface =
        static_cast<GDTriggerSurface*>(trigger_surfaces_048) +
        trigger_surface_count_03c;
    surface->flags_00 = 0x80;
    surface->index_04 = trigger_surface_count_03c + surface_index_base_028;
    surface->trigger_index_08 = trigger_index;
    surface->value_40 = 1.1f;
    surface->vertex_indices_18[0] = trigger_vertex_count_040 - 4;
    surface->vertex_indices_18[1] = trigger_vertex_count_040 - 3;
    surface->vertex_indices_18[2] = trigger_vertex_count_040 - 2;
    ClassifyTriggerSurface004498C0(trigger_vertices_04c, surface);
    for (index = 0; index < 3; ++index) {
        surface->vertex_indices_18[index] += vertex_index_base_020;
    }
    surface->unknown_0c = -1;
    surface->unknown_10 = -1;
    surface->unknown_14 = -1;
    surface->value_38 = 0;
    ++trigger_surface_count_03c;

    surface = static_cast<GDTriggerSurface*>(trigger_surfaces_048) +
              trigger_surface_count_03c;
    surface->flags_00 = 0x80;
    surface->index_04 = trigger_surface_count_03c + surface_index_base_028;
    surface->trigger_index_08 = trigger_index;
    surface->value_40 = 1.1f;
    surface->vertex_indices_18[0] = trigger_vertex_count_040 - 2;
    surface->vertex_indices_18[1] = trigger_vertex_count_040 - 1;
    surface->vertex_indices_18[2] = trigger_vertex_count_040 - 4;
    ClassifyTriggerSurface004498C0(trigger_vertices_04c, surface);
    for (index = 0; index < 3; ++index) {
        surface->vertex_indices_18[index] += vertex_index_base_020;
    }
    surface->unknown_0c = -1;
    surface->unknown_10 = -1;
    surface->unknown_14 = -1;
    surface->value_38 = 0;
    ++trigger_surface_count_03c;
}
