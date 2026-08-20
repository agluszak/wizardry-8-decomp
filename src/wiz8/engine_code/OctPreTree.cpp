#include "wiz8/engine_code/OctPreTree.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/float_constants.h"

#include <math.h>
#include <string.h>

extern "C" W8OctPreTree004679E0* g_oct_pre_tree_659c74;

/* The build-time runtime tree extends the ordinary 0x29c octree with transfer
   bookkeeping and one separately owned pointer vector.  Its only recovered
   construction caller allocates exactly 0x3bc bytes. */
// FUNCTION: WIZ8 0x004679e0
W8OctPreTree004679E0::W8OctPreTree004679E0()
    : W8Octree(0, 0)
{
    positional_3a4 = 0;
    positional_3a8 = 0;
    positional_3ac = 0;
    positional_3b0 = 0;
    m_positional_178 = 0;
    positional_3b4 = 0;
    positional_29c = 0;
    positional_2a0 = 0;
    positional_3b8 = new W8GrowableVector<void*>;
    g_oct_pre_tree_659c74 = this;
}

/* Construct the spatial value used by both the runtime octree and the level
   build tree.  A source value describes the next child: its extent halves and
   its depth advances only when the source is the root-kind record. */
// FUNCTION: WIZ8 0x0046ccc0
W8OctSpatialState0046CCC0::W8OctSpatialState0046CCC0(
    const W8OctSpatialState0046CCC0* source)
{
    Reset0046CDC0();
    level_kind_6c = 1;
    if (source != 0) {
        for (int axis = 0; axis != 3; ++axis) {
            (&minimum_0c.x)[axis] = (&source->minimum_0c.x)[axis];
            (&maximum_18.x)[axis] = (&source->maximum_18.x)[axis];
            (&clipped_minimum_24.x)[axis] =
                (&source->clipped_minimum_24.x)[axis];
            (&clipped_maximum_30.x)[axis] =
                (&source->clipped_maximum_30.x)[axis];
        }
        if (source->level_kind_6c == 1) {
            extent_04 = source->extent_04 * g_float_005ebc7c;
            depth_44 = source->depth_44 + 1;
        }
        else {
            extent_04 = source->extent_04;
            depth_44 = source->depth_44;
        }
        cell_size_08 = source->cell_size_08;
        state_3c = source->state_3c;
        positional_58 = source->positional_58;
        positional_64 = source->positional_64;
        positional_68 = source->positional_68;
        node_extent_70 = source->node_extent_70;
        root_90 = source->root_90;
        positional_94 = source->positional_94;
        owned_98 = source->owned_98;
        flags_00 = source->flags_00;
        item_count_40 = source->item_count_40;
        positional_74 = source->positional_74;
        positional_46 = source->positional_46;
        positional_52 = source->positional_52;
        owned_5c = source->owned_5c;
        node_extent_70 = source->node_extent_70;
        positional_60 = source->positional_60;
    }
}

// FUNCTION: WIZ8 0x0046cdc0
void W8OctSpatialState0046CCC0::Reset0046CDC0()
{
    memset(this, 0, sizeof(*this));
}

// FUNCTION: WIZ8 0x0046cdd0
W8OctSpatialState0046CCC0::~W8OctSpatialState0046CCC0()
{
    owned_5c = 0;
    root_90 = 0;
    owned_98 = 0;
}

/* Test a triangle against an axis-aligned box.  The inexpensive containment
   and separating-axis checks precede explicit triangle-edge intersections
   with all six box faces. */
// FUNCTION: WIZ8 0x0046ce60
unsigned char TestSpatialTriangle0046CE60(
    const srVector3T<float>* bounds,
    const srVector3T<float>* vertices,
    const srVector3T<float>* plane_normal)
{
    const float* minimum = &bounds[0].x;
    const float* maximum = &bounds[1].x;
    const float* vertex_values[3] = {
        &vertices[0].x, &vertices[1].x, &vertices[2].x};

    short vertex_index;
    for (vertex_index = 0; vertex_index < 3; ++vertex_index) {
        const float* vertex = &vertices[vertex_index].x;
        if (minimum[0] <= vertex[0] && vertex[0] <= maximum[0] &&
            minimum[1] <= vertex[1] && vertex[1] <= maximum[1] &&
            minimum[2] <= vertex[2] && vertex[2] <= maximum[2]) {
            return 1;
        }
    }

    if (plane_normal == 0) {
        return 0;
    }
    const float* normal = &plane_normal->x;

    unsigned char near_axis = 0;
    float plane_point[3];
    for (short axis = 0; axis < 3; ++axis) {
        if (vertex_values[0][axis] < minimum[axis] &&
            vertex_values[1][axis] < minimum[axis] &&
            vertex_values[2][axis] < minimum[axis]) {
            return 0;
        }
        if (maximum[axis] < vertex_values[0][axis] &&
            maximum[axis] < vertex_values[1][axis] &&
            maximum[axis] < vertex_values[2][axis]) {
            return 0;
        }
        if (g_float_005ec414 < (float)fabs(normal[axis]) &&
            minimum[axis] <= vertex_values[0][axis] &&
            vertex_values[0][axis] <= maximum[axis]) {
            near_axis = 1;
        }
        plane_point[axis] =
            (vertex_values[0][axis] + vertex_values[1][axis] +
             vertex_values[2][axis]) *
            g_float_005ec410;
    }

    if (near_axis == 0) {
        unsigned char negative = 0;
        unsigned char positive = 0;
        for (int x = 0; x != 2; ++x) {
            for (int y = 0; y != 2; ++y) {
                for (int z = 0; z != 2; ++z) {
                    float distance =
                        ((x == 0 ? minimum[0] : maximum[0]) -
                         plane_point[0]) *
                            plane_normal->x +
                        ((y == 0 ? minimum[1] : maximum[1]) -
                         plane_point[1]) *
                            plane_normal->y +
                        ((z == 0 ? minimum[2] : maximum[2]) -
                         plane_point[2]) *
                            plane_normal->z;
                    if (distance <= g_float_005ebb34) {
                        negative = 1;
                    }
                    if (g_float_005ebb34 <= distance) {
                        positive = 1;
                    }
                }
            }
        }
        if (negative == 0 || positive == 0) {
            return 0;
        }
    }

    float edge_start[3][3];
    float edge_delta[3][3];
    for (int edge = 0; edge != 3; ++edge) {
        int next = edge == 2 ? 0 : edge + 1;
        for (int axis = 0; axis != 3; ++axis) {
            edge_start[edge][axis] = vertex_values[edge][axis];
            edge_delta[edge][axis] =
                vertex_values[next][axis] - vertex_values[edge][axis];
        }
    }

    for (short face_axis = 0; face_axis < 3; ++face_axis) {
        short first_axis = face_axis == 2 ? 0 : face_axis + 1;
        short second_axis = face_axis == 0 ? 2 : face_axis - 1;
        for (short side = 0; side < 2; ++side) {
            float intersection[2][2];
            short intersection_count = 0;
            float face = side == 0 ? minimum[face_axis] : maximum[face_axis];

            for (short edge = 0; edge < 3; ++edge) {
                if ((float)g_double_005ebc70 <
                    (float)fabs(edge_delta[edge][face_axis])) {
                    float amount =
                        (face - edge_start[edge][face_axis]) /
                        edge_delta[edge][face_axis];
                    if (g_float_005ebb34 <= amount &&
                        amount <= g_float_005ebb38) {
                        float first =
                            edge_start[edge][first_axis] +
                            amount * edge_delta[edge][first_axis];
                        float second =
                            edge_start[edge][second_axis] +
                            amount * edge_delta[edge][second_axis];
                        if (minimum[first_axis] <= first &&
                            first <= maximum[first_axis] &&
                            minimum[second_axis] <= second &&
                            second <= maximum[second_axis]) {
                            return 1;
                        }
                        if (intersection_count < 2) {
                            intersection[intersection_count][0] = first;
                            intersection[intersection_count][1] = second;
                        }
                        ++intersection_count;
                    }
                }
            }

            if (intersection_count == 2) {
                float delta[2] = {
                    intersection[1][0] - intersection[0][0],
                    intersection[1][1] - intersection[0][1]};
                short rectangle_axes[2] = {first_axis, second_axis};
                for (short coordinate = 0; coordinate < 2; ++coordinate) {
                    if (g_float_005ebc90 < (float)fabs(delta[coordinate])) {
                        short other = coordinate == 0 ? 1 : 0;
                        for (short edge_side = 0; edge_side < 2;
                             ++edge_side) {
                            float boundary =
                                edge_side == 0
                                    ? minimum[rectangle_axes[coordinate]]
                                    : maximum[rectangle_axes[coordinate]];
                            float amount =
                                (boundary - intersection[0][coordinate]) /
                                delta[coordinate];
                            if (g_float_005ebb34 <= amount &&
                                amount <= g_float_005ebb38) {
                                float crossing =
                                    intersection[0][other] +
                                    amount * delta[other];
                                if (minimum[rectangle_axes[other]] <= crossing &&
                                    crossing <= maximum[rectangle_axes[other]]) {
                                    return 1;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
