#include "surrender/srFog.h"

#include "surrender/srCore.h"
#include "surrender/srDebug.h"
#include "surrender/srHeap.h"
#include "surrender/srVectorProcessor.h"
#include "surrender/srVertexPipe.h"

#include <ostream>

// FUNCTION: SURRENDER 0x1004C0E0
void srFog::setDensity(float density)
{
    if (density <= 0.0f) {
        this->density = 0.0f;
        return;
    }
    if (density >= 1.0f) {
        this->density = 1.0f;
        return;
    }
    this->density = density;
}

// FUNCTION: SURRENDER 0x1004C130
float srFog::getDensity() const
{
    return density;
}

// FUNCTION: SURRENDER 0x1004C140
void srFog::setRange(double start, double end)
{
    fog_start = start;
    fog_end = end;
}

// FUNCTION: SURRENDER 0x1004C170
void srFog::getRange(double& start, double& end)
{
    start = fog_start;
    end = fog_end;
}

// FUNCTION: SURRENDER 0x1004B6F0
int srFog::isActive(srVertexPipe& pipe)
{
    srVector3T<float> center;
    float radius;
    if ((group_mask & pipe.getExclusionMask()) != 0) {
        return 0;
    }
    if (density > 0.0f) {
        pipe.getEyeSpaceBoundingSphere(center, radius);
        if ((radius <= static_cast<float>(fog_start)) &&
            (center.LengthSquared() <
             (static_cast<float>(fog_start) - radius) * (static_cast<float>(fog_start) - radius))) {
            return 0;
        }
        return 1;
    }
    return 0;
}

// FUNCTION: SURRENDER 0x1004B7A0
void srFog::verify(srRuntimeClass::e_verify mode)
{
    srClass::verify(mode);
    if ((density < 0.0) || (1.0 < density)) {
        srAssertFail("(density >= 0.0) && (density <= 1.0)",
                     "D:\\srsdk1x\\sources\\corelib\\srFog.cpp", 0x3c, 0);
    }
    if (fog_start < 0.0) {
        srAssertFail("fogStart >= 0.0", "D:\\srsdk1x\\sources\\corelib\\srFog.cpp", 0x3d, 0);
    }
    if (fog_end < 0.0) {
        srAssertFail("fogEnd >= 0.0", "D:\\srsdk1x\\sources\\corelib\\srFog.cpp", 0x3e, 0);
    }
    if (fog_end < fog_start) {
        srAssertFail("fogEnd >= fogStart", "D:\\srsdk1x\\sources\\corelib\\srFog.cpp", 0x3f, 0);
    }
}

// FUNCTION: SURRENDER 0x1004B870
void srFog::process(srVertexPipe& pipe)
{
    float values[64];
    float scale;
    float density;
    if (pipe.isChannelAvailable(srVertexProcessor::CHANNEL_FOG)) {
        srVector3T<float> center;
        float radius;
        pipe.getEyeSpaceBoundingSphere(center, radius);
        float limit = radius + static_cast<float>(fog_end);
        long count = (long)pipe.getVertexCount();
        if (center.LengthSquared() <= limit * limit) {
            if (fog_end == fog_start) {
                scale = 1e+08f;
            } else {
                scale = (float)(1.0 / (fog_end - fog_start));
            }
            const float* distances = pipe.getEyeSpaceDist();
            if (count != 0) {
                if (static_cast<float>(fog_start) == 0.0f) {
                    if (values != distances) {
                        srVectorProcessor::memcopy(values, distances, count * 4);
                    }
                } else {
                    srVectorProcessor::add(values, -static_cast<float>(fog_start), distances,
                                           count);
                }
                if (scale != 1.0f) {
                    if (scale == 0.0f) {
                        srVectorProcessor::copy(
                            reinterpret_cast<SRDWORD*>(values), /* reinterpret-ok: VP dword fill */
                            0, count);
                    } else {
                        srVectorProcessor::mul(values, scale, values, count);
                    }
                }
                srVectorProcessor::clampUnit(values, values, count);
            }
            density = this->density;
            if ((count != 0) && (density != 1.0f)) {
                if (density == 0.0f) {
                    srVectorProcessor::copy(
                        reinterpret_cast<SRDWORD*>(values), /* reinterpret-ok: VP dword fill */
                        0, count);
                } else {
                    srVectorProcessor::mul(values, density, values, count);
                }
            }
            pipe.applyFog(values);
        } else {
            float* fog = pipe.getFog();
            if (this->density >= 1.0f) {
                srVectorProcessor::copy(reinterpret_cast<SRDWORD*>(fog),
                                        /* reinterpret-ok: VP dword fill */ 0x3f800000, count);
                return;
            }
            density = 1.0f - this->density;
            if ((count != 0) && (density != 1.0f)) {
                if (density == 0.0f) {
                    srVectorProcessor::copy(
                        reinterpret_cast<SRDWORD*>(fog), /* reinterpret-ok: VP dword fill */
                        0, count);
                } else {
                    srVectorProcessor::mul(fog, density, fog, count);
                }
            }
            if ((count != 0) && (this->density != 0.0f)) {
                srVectorProcessor::add(fog, this->density, fog, count);
            }
        }
    }
}

// FUNCTION: SURRENDER 0x1004BB60
srFog::srFog(srNode* parent)
    : srClassSupport<srFog, srIlluminator, false, 0x1210>(static_cast<srNode*>(0))
{
    if (parent != 0) {
        setParent(parent, 0);
    }
    fog_start = 0.0;
    fog_end = 1000.0;
    density = 0.5f;
}

// FUNCTION: SURRENDER 0x1004BD00
void srFog::dump(std::ostream& stream)
{
    srNode::dump(stream);
    long flags = stream.flags();
    stream.flags((flags & 0xfffffe7fL) | 0x40);
    stream.width(0x20);
    stream << "Density: " << density << '\n';
    stream.width(0x20);
    stream << "Fog start: " << fog_start << '\n';
    stream.width(0x20);
    stream << "Fog end: " << fog_end << '\n';
    stream.flags(flags & 0x7fff);
}

// FUNCTION: SURRENDER 0x1004C1B0
srClass* srFog::vInstance()
{
    return new srFog(static_cast<srNode*>(0));
}

// FUNCTION: SURRENDER 0x1004C350
srFog::~srFog() {}
