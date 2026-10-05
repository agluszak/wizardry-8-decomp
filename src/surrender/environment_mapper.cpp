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
    if ((scratch->flags & 1) == 0) {
        pipe.setupEyeSpaceDirAndDist();
    }
    const srVector3T<float>* directions = scratch->dir + pipe.sub_batch_offset;
    if ((scratch->flags & 8) == 0) {
        pipe.setupEyeSpaceNormal();
    }
    const srVector3T<float>* normals = scratch->normals + pipe.sub_batch_offset;
    srVector2T<float>* st =
        pipe.vertex_array->st0 + pipe.batch_base + pipe.sub_batch_offset;
    srCore.getStatisticsManager()->statistics.texture_coordinate_operations += count;
    pipe.lazy_setup_mask |= 1 << srVertexProcessor::CHANNEL_ST0;
    for (unsigned long index = 0; index < count; ++index) {
        float projection = directions[index].x * normals[index].x +
                           directions[index].y * normals[index].y +
                           directions[index].z * normals[index].z;
        projection = projection + projection;
        float rx = directions[index].x - normals[index].x * projection;
        float ry = directions[index].y - normals[index].y * projection;
        float rz = (directions[index].z - projection * normals[index].z) + 1.0f;
        float magnitude = sqrtf(rz * rz + rx * rx + ry * ry);
        magnitude = 1.0f / (magnitude + magnitude);
        st[index].x = rx * magnitude + 0.5f;
        st[index].y = ry * magnitude + 0.5f;
    }
}

// GLOBAL: SURRENDER 0x100A48CC
srEnvironmentMapper srEnvironmentMapper;
