#include "surrender/srFog.h"

#include "surrender/srCore.h"
#include "surrender/srDebug.h"
#include "surrender/srHeap.h"
#include "surrender/srVectorProcessor.h"
#include "surrender/srVertexPipe.h"

// FUNCTION: SURRENDER 0x1004C0E0
void srFog::setDensity(float density)
{
    if (density <= 0.0f) {
        density_160 = 0.0f;
        return;
    }
    if (density >= 1.0f) {
        density_160 = 1.0f;
        return;
    }
    density_160 = density;
}

// FUNCTION: SURRENDER 0x1004C130
float srFog::getDensity() const
{
    return density_160;
}

// FUNCTION: SURRENDER 0x1004C140
void srFog::setRange(double start, double end)
{
    fog_start_150 = start;
    fog_end_158 = end;
}

// FUNCTION: SURRENDER 0x1004C170
void srFog::getRange(double& start, double& end)
{
    start = fog_start_150;
    end = fog_end_158;
}

// FUNCTION: SURRENDER 0x1004C1A0
const char* srFog::sGetClassName()
{
    return "srFog";
}

// FUNCTION: SURRENDER 0x1004B6F0
int srFog::isActive(srVertexPipe& pipe)
{
    srVector3T<float> center;
    float radius;
    if ((group_mask_13c & pipe.getExclusionMask()) != 0) {
        return 0;
    }
    if (density_160 > 0.0f) {
        pipe.getEyeSpaceBoundingSphere(center, radius);
        if ((radius <= (float)fog_start_150) &&
            (center.x * center.x + center.y * center.y + center.z * center.z <
             ((float)fog_start_150 - radius) * ((float)fog_start_150 - radius))) {
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
    if ((density_160 < 0.0) || (1.0 < density_160)) {
        srAssertFail("(density >= 0.0) && (density <= 1.0)",
                     "D:\\srsdk1x\\sources\\corelib\\srFog.cpp", 0x3c, 0);
    }
    if (fog_start_150 < 0.0) {
        srAssertFail("fogStart >= 0.0", "D:\\srsdk1x\\sources\\corelib\\srFog.cpp", 0x3d, 0);
    }
    if (fog_end_158 < 0.0) {
        srAssertFail("fogEnd >= 0.0", "D:\\srsdk1x\\sources\\corelib\\srFog.cpp", 0x3e, 0);
    }
    if (fog_end_158 < fog_start_150) {
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
        float limit = radius + (float)fog_end_158;
        long count = (long)pipe.getVertexCount();
        if (center.x * center.x + center.y * center.y + center.z * center.z <= limit * limit) {
            if (fog_end_158 == fog_start_150) {
                scale = 1e+08f;
            } else {
                scale = (float)(1.0 / (fog_end_158 - fog_start_150));
            }
            const float* distances = pipe.getEyeSpaceDist();
            if (count != 0) {
                if ((float)fog_start_150 == 0.0f) {
                    if (values != distances) {
                        srVectorProcessor::memcopy(values, distances, count * 4);
                    }
                } else {
                    srVectorProcessor::add(values, -(float)fog_start_150, distances, count);
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
            density = density_160;
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
            if (density_160 >= 1.0f) {
                srVectorProcessor::copy(reinterpret_cast<SRDWORD*>(fog),
                                        /* reinterpret-ok: VP dword fill */ 0x3f800000, count);
                return;
            }
            density = 1.0f - density_160;
            if ((count != 0) && (density != 1.0f)) {
                if (density == 0.0f) {
                    srVectorProcessor::copy(
                        reinterpret_cast<SRDWORD*>(fog), /* reinterpret-ok: VP dword fill */
                        0, count);
                } else {
                    srVectorProcessor::mul(fog, density, fog, count);
                }
            }
            if ((count != 0) && (density_160 != 0.0f)) {
                srVectorProcessor::add(fog, density_160, fog, count);
            }
        }
    }
}

// FUNCTION: SURRENDER 0x1004BB60
srFog::srFog(srNode* parent) : srIlluminator(0)
{
    srCore.getRegistry()->registerInstance(ClientType::sGetClassNode(), this);
    if (parent != 0) {
        setParent(parent, 0);
    }
    fog_start_150 = 0.0;
    fog_end_158 = 1000.0;
    density_160 = 0.5f;
}

// FUNCTION: SURRENDER 0x1004BCA0
srFog& srFog::operator=(const srFog& other)
{
    if (this != &other) {
        srIlluminator::operator=(other);
        density_160 = other.density_160;
        fog_start_150 = other.fog_start_150;
        fog_end_158 = other.fog_end_158;
    }
    return *this;
}

// FUNCTION: SURRENDER 0x1004BD00
void srFog::dump(std::ostream& stream)
{
    srNode::dump(stream);
    long flags = stream.flags();
    stream.flags((flags & 0xfffffe7fL) | 0x40);
    stream.width(0x20);
    stream << "Density: " << density_160 << '\n';
    stream.width(0x20);
    stream << "Fog start: " << fog_start_150 << '\n';
    stream.width(0x20);
    stream << "Fog end: " << fog_end_158 << '\n';
    stream.flags(flags & 0x7fff);
}

// FUNCTION: SURRENDER 0x1004C1B0
srClass* srFog::vInstance()
{
    srFog* instance = static_cast<srFog*>(srHeap.allocate(0x168));
    if (instance != 0) {
        return new (instance) srFog(0);
    }
    return 0;
}

// FUNCTION: SURRENDER 0x1004C210
srFog::srFog(const srFog& other) : srIlluminator(0)
{
    srCore.getRegistry()->registerInstance(ClientType::sGetClassNode(), this);
    *this = other;
}

// FUNCTION: SURRENDER 0x1004C350
srFog::~srFog()
{
    srCore.getRegistry()->unregisterInstance(ClientType::sGetClassNode(), this);
}
