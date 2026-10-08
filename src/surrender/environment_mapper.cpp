#include "surrender/srEnvironmentMapper.h"

#include "surrender/srCore.h"
#include "surrender/srVectorProcessor.h"
#include "surrender/srVertexPipe.h"

#include <math.h>

// FUNCTION: SURRENDER 0x100352e0
int srEnvironmentMapper::isActive(srVertexPipe&)
{
    return 1;
}

// FUNCTION: SURRENDER 0x100352f0
void srEnvironmentMapper::process(srVertexPipe& pipe)
{
    unsigned long count = pipe.vertex_count;
    srVertexPipe::Scratch* scratch = pipe.scratch;
    if ((scratch->flags & srVertexPipe::Scratch::READY_EYE_DIRECTION) == 0) {
        pipe.setupEyeSpaceDirAndDist();
    }
    const srVector3T<float>* directions = scratch->dir + pipe.sub_batch_offset;
    if ((scratch->flags & srVertexPipe::Scratch::READY_EYE_NORMALS) == 0) {
        pipe.setupEyeSpaceNormal();
    }
    const srVector3T<float>* normals = scratch->normals + pipe.sub_batch_offset;
    srVector2T<float>* st = pipe.vertex_array->st0 + pipe.batch_base + pipe.sub_batch_offset;
    srCore.getStatisticsManager()->statistics.texture_coordinate_operations += count;
    pipe.lazy_setup_mask |= 1 << srVertexProcessor::CHANNEL_ST0;
    for (unsigned long index = 0; index < count; ++index) {
        const srVector3T<float>& direction = directions[index];
        const srVector3T<float>& normal = normals[index];
        /* Retail keeps the doubled projection, rz and the magnitude on the x87
           stack but stores n.x * projection, n.y * projection, rx and ry to float
           stack slots and reloads them; volatile reproduces those roundings. The
           dot product sums z, y, x and the squared length y, x, z. */
        double projection =
            (direction.z * normal.z + direction.y * normal.y) + direction.x * normal.x;
        projection = projection + projection;
        volatile float scaled_x = normal.x * projection;
        volatile float scaled_y = normal.y * projection;
        volatile float rx = direction.x - scaled_x;
        volatile float ry = direction.y - scaled_y;
        double rz = (direction.z - projection * normal.z) + 1.0f;
        double magnitude = sqrt((ry * ry + rx * rx) + rz * rz);
        magnitude = 1.0f / (magnitude + magnitude);
        st[index].x = (float)(rx * magnitude + 0.5f);
        st[index].y = (float)(ry * magnitude + 0.5f);
    }
}

// GLOBAL: SURRENDER 0x100A48CC
srEnvironmentMapper srEnvironmentMapper;
