#include "surrender/srEnvironmentMapper.h"

#include "surrender/srCore.h"
#include "surrender/srVectorProcessor.h"
#include "surrender/srVertexPipe.h"

#include <math.h>

// FUNCTION: SURRENDER 0x10035430
srEnvironmentMapper::srEnvironmentMapper() {}

// FUNCTION: SURRENDER 0x10035440
srEnvironmentMapper::srEnvironmentMapper(const srEnvironmentMapper&) {}

// FUNCTION: SURRENDER 0x10035450
srEnvironmentMapper& srEnvironmentMapper::operator=(const srEnvironmentMapper&)
{
    return *this;
}

// FUNCTION: SURRENDER 0x10035460
srEnvironmentMapper::~srEnvironmentMapper() {}

// FUNCTION: SURRENDER 0x100352e0
int srEnvironmentMapper::isActive(srVertexPipe&)
{
    return 1;
}

// FUNCTION: SURRENDER 0x100352f0
void srEnvironmentMapper::process(srVertexPipe& pipe)
{
    unsigned long count = pipe.vertex_count_88;
    srVertexPipe::Scratch* scratch = static_cast<srVertexPipe::Scratch*>(pipe.scratch_00);
    if ((scratch->flags_b00 & 1) == 0) {
        pipe.setupEyeSpaceDirAndDist();
    }
    const srVector3T<float>* directions = scratch->dir_000 + pipe.sub_batch_offset_84;
    if ((scratch->flags_b00 & 8) == 0) {
        pipe.setupEyeSpaceNormal();
    }
    const srVector3T<float>* normals = scratch->normals_300 + pipe.sub_batch_offset_84;
    srVector2T<float>* st =
        pipe.vertex_array_78->st0_0c + pipe.batch_base_80 + pipe.sub_batch_offset_84;
    srCore.getStatisticsManager()->statistics_00.texture_coordinate_operations_34 += count;
    pipe.lazy_setup_mask_10 |= 1 << srVertexProcessor::CHANNEL_ST0;
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

// SYNTHETIC: SURRENDER 0x100354B0
// srEnvironmentMapper scalar deleting destructor
