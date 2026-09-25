/* D:\srsdk1x\sources\corelib\srMaterial.cpp */

#include "surrender/srMaterial.h"

#include "surrender/srCore.h"
#include "surrender/srDebug.h"
#include "surrender/srVP.h"
#include "surrender/srVectorProcessor.h"

/* Comma-separated operation names srMaterial::dump walks while printing the
   operations_6c bits. Retail .data holds a zero-initialized pointer here; no
   in-range provider code ever stores to it, so the source spelling stays
   unresolved beyond the address binding. */
// GLOBAL: SURRENDER 0x100A48C0
static const char* s_oper_names_100a48c0;

// FUNCTION: SURRENDER 0x100335D0
void srMaterial::verify(srRuntimeClass::e_verify mode)
{
    srClass::verify(mode);
    if (parms.ambient.x < 0.0 || parms.ambient.x > 1.0) {
        srAssertFail("parms.ambient.x >= 0.0 && parms.ambient.x <= 1.0",
                     "D:\\srsdk1x\\sources\\corelib\\srMaterial.cpp", 0x29, 0);
    }
    if (parms.ambient.y < 0.0 || parms.ambient.y > 1.0) {
        srAssertFail("parms.ambient.y >= 0.0 && parms.ambient.y <= 1.0",
                     "D:\\srsdk1x\\sources\\corelib\\srMaterial.cpp", 0x2a, 0);
    }
    if (parms.ambient.z < 0.0 || parms.ambient.z > 1.0) {
        srAssertFail("parms.ambient.z >= 0.0 && parms.ambient.z <= 1.0",
                     "D:\\srsdk1x\\sources\\corelib\\srMaterial.cpp", 0x2b, 0);
    }
    if (parms.ambient.w < 0.0 || parms.ambient.w > 1.0) {
        srAssertFail("parms.ambient.w >= 0.0 && parms.ambient.w <= 1.0",
                     "D:\\srsdk1x\\sources\\corelib\\srMaterial.cpp", 0x2c, 0);
    }
    if (parms.diffuse.x < 0.0 || parms.diffuse.x > 1.0) {
        srAssertFail("parms.diffuse.x >= 0.0 && parms.diffuse.x <= 1.0",
                     "D:\\srsdk1x\\sources\\corelib\\srMaterial.cpp", 0x2d, 0);
    }
    if (parms.diffuse.y < 0.0 || parms.diffuse.y > 1.0) {
        srAssertFail("parms.diffuse.y >= 0.0 && parms.diffuse.y <= 1.0",
                     "D:\\srsdk1x\\sources\\corelib\\srMaterial.cpp", 0x2e, 0);
    }
    if (parms.diffuse.z < 0.0 || parms.diffuse.z > 1.0) {
        srAssertFail("parms.diffuse.z >= 0.0 && parms.diffuse.z <= 1.0",
                     "D:\\srsdk1x\\sources\\corelib\\srMaterial.cpp", 0x2f, 0);
    }
    if (parms.diffuse.w < 0.0 || parms.diffuse.w > 1.0) {
        srAssertFail("parms.diffuse.w >= 0.0 && parms.diffuse.w <= 1.0",
                     "D:\\srsdk1x\\sources\\corelib\\srMaterial.cpp", 0x30, 0);
    }
    if (parms.emissive.x < 0.0 || parms.emissive.x > 1.0) {
        srAssertFail("parms.emissive.x >= 0.0 && parms.emissive.x <= 1.0",
                     "D:\\srsdk1x\\sources\\corelib\\srMaterial.cpp", 0x31, 0);
    }
    if (parms.emissive.y < 0.0 || parms.emissive.y > 1.0) {
        srAssertFail("parms.emissive.y >= 0.0 && parms.emissive.y <= 1.0",
                     "D:\\srsdk1x\\sources\\corelib\\srMaterial.cpp", 0x32, 0);
    }
    if (parms.emissive.z < 0.0 || parms.emissive.z > 1.0) {
        srAssertFail("parms.emissive.z >= 0.0 && parms.emissive.z <= 1.0",
                     "D:\\srsdk1x\\sources\\corelib\\srMaterial.cpp", 0x33, 0);
    }
    if (parms.emissive.w < 0.0 || parms.emissive.w > 1.0) {
        srAssertFail("parms.emissive.w >= 0.0 && parms.emissive.w <= 1.0",
                     "D:\\srsdk1x\\sources\\corelib\\srMaterial.cpp", 0x34, 0);
    }
    if (parms.specular.x < 0.0 || parms.specular.x > 1.0) {
        srAssertFail("parms.specular.x >= 0.0 && parms.specular.x <= 1.0",
                     "D:\\srsdk1x\\sources\\corelib\\srMaterial.cpp", 0x35, 0);
    }
    if (parms.specular.y < 0.0 || parms.specular.y > 1.0) {
        srAssertFail("parms.specular.y >= 0.0 && parms.specular.y <= 1.0",
                     "D:\\srsdk1x\\sources\\corelib\\srMaterial.cpp", 0x36, 0);
    }
    if (parms.specular.z < 0.0 || parms.specular.z > 1.0) {
        srAssertFail("parms.specular.z >= 0.0 && parms.specular.z <= 1.0",
                     "D:\\srsdk1x\\sources\\corelib\\srMaterial.cpp", 0x37, 0);
    }
    if (parms.specular.w < 0.0 || parms.specular.w > 1.0) {
        srAssertFail("parms.specular.w >= 0.0 && parms.specular.w <= 1.0",
                     "D:\\srsdk1x\\sources\\corelib\\srMaterial.cpp", 0x38, 0);
    }
    if (parms.shininess < 0.0 || parms.shininess > 127.0) {
        srAssertFail("parms.shininess >= 0.0 && parms.shininess <= 127.0",
                     "D:\\srsdk1x\\sources\\corelib\\srMaterial.cpp", 0x39, 0);
    }
    if (parms.translucency < 0.0 || parms.translucency > 1.0) {
        srAssertFail("parms.translucency >= 0.0 && parms.translucency <= 1.0",
                     "D:\\srsdk1x\\sources\\corelib\\srMaterial.cpp", 0x3a, 0);
    }
}

// FUNCTION: SURRENDER 0x100339B0
void srMaterial::updateParms()
{
    parms.flags = 0;
    if (parms.diffuse.x == 0.0f && parms.diffuse.y == 0.0f && parms.diffuse.z == 0.0f) {
        parms.flags = 0x400;
    }
    if (parms.ambient.x == 0.0f && parms.ambient.y == 0.0f && parms.ambient.z == 0.0f) {
        parms.flags |= 0x200;
    }
    if (parms.specular.x == 0.0f && parms.specular.y == 0.0f && parms.specular.z == 0.0f) {
        parms.flags |= 4;
    }
    if (parms.value_4c == 0.0f) {
        parms.flags |= 0x10;
    }
    dirty_74 = 0;
}

// FUNCTION: SURRENDER 0x10033A80
srMaterial& srMaterial::operator=(const srMaterial& other)
{
    if (&other != this) {
        srMaterialIFace::operator=(other);
        parms = other.parms;
        operations_6c.value = other.operations_6c.value;
        mapper_70 = other.mapper_70;
        dirty_74 = 1;
        return *this;
    }
    return *this;
}

// FUNCTION: SURRENDER 0x10033B50
void srMaterial::getMaterialInfo(srVertexProcessor::MaterialInfo& info)
{
    if (dirty_74 != 0) {
        updateParms();
    }
    info = parms;
}

// FUNCTION: SURRENDER 0x10033C00
void srMaterial::preProcess(srVertexPipe& pipe)
{
    if ((operations_6c.value & 0x10) != 0) {
        pipe.enableChannel(srVertexProcessor::CHANNEL_DIFFUSE);
        pipe.enableChannel(srVertexProcessor::CHANNEL_LIGHT_DIFFUSE);
        pipe.enableChannel(srVertexProcessor::CHANNEL_LIGHT_AMBIENT);
        pipe.enableChannel(srVertexProcessor::CHANNEL_SPECULAR);
        return;
    }
    if ((operations_6c.value & 8) != 0) {
        pipe.enableChannel(srVertexProcessor::CHANNEL_DIFFUSE);
        pipe.enableChannel(srVertexProcessor::CHANNEL_LIGHT_DIFFUSE);
        pipe.enableChannel(srVertexProcessor::CHANNEL_LIGHT_AMBIENT);
        pipe.disableChannel(srVertexProcessor::CHANNEL_SPECULAR);
        return;
    }
    if ((operations_6c.value & 4) != 0) {
        pipe.disableChannel(srVertexProcessor::CHANNEL_DIFFUSE);
        pipe.disableChannel(srVertexProcessor::CHANNEL_LIGHT_DIFFUSE);
        pipe.disableChannel(srVertexProcessor::CHANNEL_LIGHT_AMBIENT);
        pipe.enableChannel(srVertexProcessor::CHANNEL_SPECULAR);
    }
}

// FUNCTION: SURRENDER 0x10033C90
void srMaterial::postProcess(srVertexPipe& pipe)
{
    unsigned long operations;
    unsigned long vertex_count;
    unsigned long blend;
    float* channel;

    if (mapper_70 != 0) {
        if (mapper_70->isActive(pipe) != 0) {
            mapper_70->process(pipe);
        }
    }
    operations = operations_6c.value;
    vertex_count = pipe.vertex_count_88;
    blend = operations & 1;
    if (operations != 0) {
        if ((operations & 0x10) == 0) {
            if ((operations & 8) == 0) {
                if ((operations & 4) == 0) {
                    goto channels_done;
                }
                if (blend != 0) {
                    if ((static_cast<unsigned char*>(pipe.scratch_00)[0xb00] & 0x10) == 0) {
                        pipe.setupDepthCue();
                    }
                    srCore.getStatisticsManager()->statistics_00.specular_operations_28 +=
                        pipe.vertex_count_88;
                    if ((pipe.lazy_setup_mask_10 & 4) == 0) {
                        pipe.setupSpecular();
                    }
                    channel =
                        reinterpret_cast<float*>(pipe.vertex_array_78->specular_08 +
                                                 pipe.batch_base_80 + pipe.sub_batch_offset_84);
                    if (vertex_count != 0) {
                        srVectorProcessor::vp->_mul(channel, channel,
                                                    static_cast<float*>(pipe.scratch_00) + 0x200 +
                                                        pipe.sub_batch_offset_84,
                                                    vertex_count);
                    }
                    blend = 0;
                }
                pipe.copySpecularToDiffuse();
                pipe.enableChannel(srVertexProcessor::CHANNEL_DIFFUSE);
            } else {
                if (blend != 0) {
                    if ((static_cast<unsigned char*>(pipe.scratch_00)[0xb00] & 0x10) == 0) {
                        pipe.setupDepthCue();
                    }
                    srCore.getStatisticsManager()->statistics_00.diffuse_operations_24 +=
                        pipe.vertex_count_88;
                    if ((pipe.lazy_setup_mask_10 & 2) == 0) {
                        pipe.setupDiffuse();
                    }
                    channel =
                        reinterpret_cast<float*>(pipe.vertex_array_78->diffuse_04 +
                                                 pipe.batch_base_80 + pipe.sub_batch_offset_84);
                    if (vertex_count != 0) {
                        srVectorProcessor::vp->_mul(channel, channel,
                                                    static_cast<float*>(pipe.scratch_00) + 0x200 +
                                                        pipe.sub_batch_offset_84,
                                                    vertex_count);
                    }
                    blend = 0;
                }
                pipe.copyDiffuseToSpecular();
                pipe.enableChannel(srVertexProcessor::CHANNEL_SPECULAR);
            }
        } else {
            pipe.swapDiffuseAndSpecular();
        }
    }
channels_done:
    if (blend != 0) {
        if ((pipe.channel_mask_0c & 2) != 0) {
            if ((static_cast<unsigned char*>(pipe.scratch_00)[0xb00] & 0x10) == 0) {
                pipe.setupDepthCue();
            }
            srCore.getStatisticsManager()->statistics_00.diffuse_operations_24 +=
                pipe.vertex_count_88;
            if ((pipe.lazy_setup_mask_10 & 2) == 0) {
                pipe.setupDiffuse();
            }
            channel = reinterpret_cast<float*>(pipe.vertex_array_78->diffuse_04 +
                                               pipe.batch_base_80 + pipe.sub_batch_offset_84);
            if (vertex_count != 0) {
                srVectorProcessor::vp->_mul(channel, channel,
                                            static_cast<float*>(pipe.scratch_00) + 0x200 +
                                                pipe.sub_batch_offset_84,
                                            vertex_count);
            }
        }
        if ((pipe.channel_mask_0c & 4) != 0) {
            if ((static_cast<unsigned char*>(pipe.scratch_00)[0xb00] & 0x10) == 0) {
                pipe.setupDepthCue();
            }
            srCore.getStatisticsManager()->statistics_00.specular_operations_28 +=
                pipe.vertex_count_88;
            if ((pipe.lazy_setup_mask_10 & 4) == 0) {
                pipe.setupSpecular();
            }
            channel = reinterpret_cast<float*>(pipe.vertex_array_78->specular_08 +
                                               pipe.batch_base_80 + pipe.sub_batch_offset_84);
            if (vertex_count != 0) {
                srVectorProcessor::vp->_mul(channel, channel,
                                            static_cast<float*>(pipe.scratch_00) + 0x200 +
                                                pipe.sub_batch_offset_84,
                                            vertex_count);
            }
        }
    }
    if ((operations_6c.value & 2) != 0 && (pipe.channel_mask_0c & 8) != 0) {
        if ((static_cast<unsigned char*>(pipe.scratch_00)[0xb00] & 0x10) == 0) {
            pipe.setupDepthCue();
        }
        srCore.getStatisticsManager()->statistics_00.alpha_operations_2c += pipe.vertex_count_88;
        if ((pipe.lazy_setup_mask_10 & 8) == 0) {
            pipe.setupAlpha();
        }
        channel = static_cast<float*>(pipe.scratch_00) + 0x240 + pipe.sub_batch_offset_84;
        if (vertex_count != 0) {
            srVectorProcessor::vp->_mul(channel, channel,
                                        static_cast<float*>(pipe.scratch_00) + 0x200 +
                                            pipe.sub_batch_offset_84,
                                        vertex_count);
        }
    }
}

// FUNCTION: SURRENDER 0x10033F60
void srMaterial::reset()
{
    parms.ambient.x = 0.2f;
    parms.ambient.y = 0.2f;
    parms.ambient.z = 0.2f;
    parms.ambient.w = 1.0f;
    parms.diffuse.w = 1.0f;
    parms.diffuse.x = 0.8f;
    parms.diffuse.y = 0.8f;
    parms.diffuse.z = 0.8f;
    parms.specular.x = 0.0f;
    parms.specular.y = 0.0f;
    parms.specular.z = 0.0f;
    parms.specular.w = 1.0f;
    parms.emissive.x = 0.0f;
    parms.emissive.y = 0.0f;
    parms.emissive.z = 0.0f;
    parms.emissive.w = 1.0f;
    parms.shininess = 1.0f;
    parms.translucency = 0.0f;
    parms.flags = 0;
    parms.value_38 = 0.0f;
    dirty_74 = 1;
    operations_6c.value = 0;
    mapper_70 = 0;
}

// FUNCTION: SURRENDER 0x10033FC0
void srMaterial::dump(std::ostream& stream)
{
    long flags;

    srClass::dump(stream);
    flags = stream.flags();
    stream.flags((flags & 0xfffffe7fL) | 0x40);
    stream.width(0x20);
    stream << "Ambient: " << '{' << parms.ambient.x << ',' << parms.ambient.y << ','
           << parms.ambient.z << ',' << parms.ambient.w << '}' << '\n';
    stream.width(0x20);
    stream << "Diffuse: " << '{' << parms.diffuse.x << ',' << parms.diffuse.y << ','
           << parms.diffuse.z << ',' << parms.diffuse.w << '}' << '\n';
    stream.width(0x20);
    stream << "Emissive: " << '{' << parms.emissive.x << ',' << parms.emissive.y << ','
           << parms.emissive.z << ',' << parms.emissive.w << '}' << '\n';
    stream.width(0x20);
    stream << "Specular: " << '{' << parms.specular.x << ',' << parms.specular.y << ','
           << parms.specular.z << ',' << parms.specular.w << '}' << '\n';
    stream.width(0x20);
    stream << "Shininess: " << parms.shininess << '\n';
    stream.width(0x20);
    stream << "Translucency: " << parms.translucency << '\n';
    stream.width(0x20);
    stream << "Op. Control Flags: ";
    if (operations_6c.value == 0) {
        stream << "<NONE>";
    } else {
        stream << '[';
        bool first = true;
        const char* names = s_oper_names_100a48c0;
        const char* name = names;
        for (unsigned long bit = 0; bit < 0x20; ++bit) {
            if ((operations_6c.value & (1 << bit)) == 0) {
                if (name != 0) {
                    while (*name != 0 && *name != ',') {
                        ++name;
                    }
                    if (*name == ',') {
                        ++name;
                    }
                }
            } else {
                if (first) {
                    first = false;
                } else {
                    stream << ',';
                }
                if (name == 0 || *name == 0) {
                    stream << bit;
                } else {
                    while (*name != 0 && *name != ',') {
                        stream << *name;
                        ++name;
                    }
                    if (*name == ',') {
                        ++name;
                    }
                }
            }
        }
        stream << ']';
    }
    stream << '\n';
    stream.width(0x20);
    stream << "Texture mapper: " << mapper_70 << '\n';
    stream.width(0x20);
    stream << "Dirty: " << srBoolToString(dirty_74) << '\n';
    stream.flags(flags & 0x7fff);
}

// FUNCTION: SURRENDER 0x100343B0
const char* srMaterial::sGetClassName()
{
    return "srMaterial";
}

// FUNCTION: SURRENDER 0x10016880
srMaterial::~srMaterial()
{
    srCore.getRegistry()->unregisterInstance(sGetClassNode(), this);
}

// FUNCTION: SURRENDER 0x100343C0
srClass* srMaterial::vInstance()
{
    return new srMaterial;
}

// FUNCTION: SURRENDER 0x100344B0
srMaterial::srMaterial(const srMaterial& other)
{
    srCore.getRegistry()->registerInstance(sGetClassNode(), this);
    *this = other;
    parms = other.parms;
    operations_6c.value = other.operations_6c.value;
    mapper_70 = other.mapper_70;
    dirty_74 = other.dirty_74;
}

// FUNCTION: SURRENDER 0x10034640
srVector4T<float> srMaterial::getAmbient() const
{
    return parms.ambient;
}

// FUNCTION: SURRENDER 0x10034670
srVector4T<float> srMaterial::getDiffuse() const
{
    return parms.diffuse;
}

// FUNCTION: SURRENDER 0x100346A0
srVector4T<float> srMaterial::getEmissive() const
{
    return parms.emissive;
}

// FUNCTION: SURRENDER 0x100346D0
srVector4T<float> srMaterial::getSpecular() const
{
    return parms.specular;
}

// FUNCTION: SURRENDER 0x10034830
void srMaterial::getAmbient(srVector4T<float>& ambient) const
{
    ambient = parms.ambient;
}

// FUNCTION: SURRENDER 0x10034850
void srMaterial::getDiffuse(srVector4T<float>& diffuse) const
{
    diffuse = parms.diffuse;
}

// FUNCTION: SURRENDER 0x10034870
void srMaterial::getEmissive(srVector4T<float>& emissive) const
{
    emissive = parms.emissive;
}

// FUNCTION: SURRENDER 0x10034890
void srMaterial::getSpecular(srVector4T<float>& specular) const
{
    specular = parms.specular;
}

// FUNCTION: SURRENDER 0x100348B0
float srMaterial::getShininess() const
{
    return parms.shininess;
}

// FUNCTION: SURRENDER 0x100348C0
float srMaterial::getTranslucency() const
{
    return parms.translucency;
}

// FUNCTION: SURRENDER 0x100348D0
float srMaterial::getOpacity() const
{
    return parms.diffuse.w;
}

// FUNCTION: SURRENDER 0x100348E0
void srMaterial::setAmbientAndDiffuse(const srVector4T<float>& color)
{
    parms.ambient = color;
    dirty_74 = 1;
    parms.diffuse = color;
    dirty_74 = 1;
}

// FUNCTION: SURRENDER 0x100349F0
void srMaterial::setShininess(double shininess)
{
    parms.shininess = static_cast<float>(shininess);
    if (static_cast<float>(shininess) < 1.0f) {
        parms.shininess = 1.0f;
    }
    if (parms.shininess > 127.0f) {
        parms.shininess = 127.0f;
    }
    dirty_74 = 1;
}

// FUNCTION: SURRENDER 0x10034A30
void srMaterial::setTranslucency(double translucency)
{
    float value = static_cast<float>(translucency);
    if (value <= 0.0f) {
        dirty_74 = 1;
        parms.translucency = 0.0f;
        return;
    }
    if (value >= 1.0f) {
        value = 1.0f;
    }
    parms.translucency = value;
    dirty_74 = 1;
}

// FUNCTION: SURRENDER 0x10034AA0
void srMaterial::setVector(srVector4T<float>& destination, const srVector4T<float>& source)
{
    destination = source;
    dirty_74 = 1;
}

// FUNCTION: SURRENDER 0x10034AD0
void srMaterial::disable(e_oper operation)
{
    operations_6c.value &= ~(1 << (operation & 0x1f));
    dirty_74 = 1;
}

// FUNCTION: SURRENDER 0x10034B00
void srMaterial::enable(e_oper operation)
{
    operations_6c.value |= 1 << (operation & 0x1f);
    dirty_74 = 1;
}

// FUNCTION: SURRENDER 0x10034B20
int srMaterial::isEnabled(e_oper operation) const
{
    return (operations_6c.value & (1 << (operation & 0x1f))) != 0;
}

// FUNCTION: SURRENDER 0x10034B50
srVertexProcessor* srMaterial::getMapper() const
{
    return mapper_70;
}

// FUNCTION: SURRENDER 0x10034DA0
const char* srMaterialIFace::sGetClassName()
{
    return "srMaterialIFace";
}

// FUNCTION: SURRENDER 0x10034D80
srMaterialIFace& srMaterialIFace::operator=(const srMaterialIFace& other)
{
    if (&other != this) {
        srClass::operator=(other);
    }
    return *this;
}

/* The srMaterialIFace constructors live in the class declaration; retail
   emitted standalone copies in this TU at 0x10034BE0/0x10034C70 while folding
   the same bodies into callers. */

// FUNCTION: SURRENDER 0x10034DB0
std::ostream& operator<<(std::ostream& stream, const srShader& shader)
{
    switch (shader.value & srShader::PASS_MASK) {
    case srShader::PASS_NEVER:
        stream << "PASS_NEVER";
        break;
    case srShader::PASS_LESS:
        stream << "PASS_LESS";
        break;
    case srShader::PASS_EQUAL:
        stream << "PASS_EQUAL";
        break;
    case srShader::PASS_LEQUAL:
        stream << "PASS_LEQUAL";
        break;
    case srShader::PASS_GREATER:
        stream << "PASS_GREATER";
        break;
    case srShader::PASS_NOTEQUAL:
        stream << "PASS_NOTEQUAL";
        break;
    case srShader::PASS_GEQUAL:
        stream << "PASS_GEQUAL";
        break;
    case srShader::PASS_ALWAYS:
        stream << "PASS_ALWAYS";
        break;
    }
    stream << '/';
    switch ((shader.value >> 0x3) & 0x1) {
    case srShader::DEPTH_WRITE_DISABLE:
        stream << "DEPTH_WRITE_DISABLE";
        break;
    case srShader::DEPTH_WRITE_ENABLE:
        stream << "DEPTH_WRITE_ENABLE";
        break;
    }
    stream << '/';
    switch ((shader.value >> 0x4) & 0x1) {
    case srShader::COLOR_WRITE_DISABLE:
        stream << "COLOR_WRITE_DISABLE";
        break;
    case srShader::COLOR_WRITE_ENABLE:
        stream << "COLOR_WRITE_ENABLE";
        break;
    }
    stream << '/';
    switch ((shader.value >> srShader::DETAILALPHA0_SHIFT) & 0x7) {
    case srShader::DETAILALPHA_DISABLE:
        stream << "DETAILALPHA_DISABLE";
        break;
    case srShader::DETAILALPHA_DETAIL:
        stream << "DETAILALPHA_DETAIL";
        break;
    case srShader::DETAILALPHA_SCALE:
        stream << "DETAILALPHA_SCALE";
        break;
    case srShader::DETAILALPHA_INVSCALE:
        stream << "DETAILALPHA_INVSCALE";
        break;
    }
    stream << '/';
    switch ((shader.value >> srShader::DETAILCOLOR0_SHIFT) & 0xf) {
    case srShader::DETAILCOLOR_DISABLE:
        stream << "DETAILCOLOR_DISABLE";
        break;
    case srShader::DETAILCOLOR_DETAIL:
        stream << "DETAILCOLOR_DETAIL";
        break;
    case srShader::DETAILCOLOR_SCALE:
        stream << "DETAILCOLOR_SCALE";
        break;
    case srShader::DETAILCOLOR_INVSCALE:
        stream << "DETAILCOLOR_INVSCALE";
        break;
    case srShader::DETAILCOLOR_ADD:
        stream << "DETAILCOLOR_ADD";
        break;
    case srShader::DETAILCOLOR_SUB:
        stream << "DETAILCOLOR_SUB";
        break;
    case srShader::DETAILCOLOR_SUBR:
        stream << "DETAILCOLOR_SUBR";
        break;
    case srShader::DETAILCOLOR_BLEND:
        stream << "DETAILCOLOR_BLEND";
        break;
    case srShader::DETAILCOLOR_DETAILBLEND:
        stream << "DETAILCOLOR_DETAILBLEND";
        break;
    }
    stream << '/';
    switch (shader.value >> srShader::DETAILALPHA1_SHIFT) {
    case srShader::DETAILALPHA_DISABLE:
        stream << "DETAILALPHA_DISABLE";
        break;
    case srShader::DETAILALPHA_DETAIL:
        stream << "DETAILALPHA_DETAIL";
        break;
    case srShader::DETAILALPHA_SCALE:
        stream << "DETAILALPHA_SCALE";
        break;
    case srShader::DETAILALPHA_INVSCALE:
        stream << "DETAILALPHA_INVSCALE";
        break;
    }
    stream << '/';
    switch ((shader.value >> srShader::DETAILCOLOR1_SHIFT) & 0xf) {
    case srShader::DETAILCOLOR_DISABLE:
        stream << "DETAILCOLOR_DISABLE";
        break;
    case srShader::DETAILCOLOR_DETAIL:
        stream << "DETAILCOLOR_DETAIL";
        break;
    case srShader::DETAILCOLOR_SCALE:
        stream << "DETAILCOLOR_SCALE";
        break;
    case srShader::DETAILCOLOR_INVSCALE:
        stream << "DETAILCOLOR_INVSCALE";
        break;
    case srShader::DETAILCOLOR_ADD:
        stream << "DETAILCOLOR_ADD";
        break;
    case srShader::DETAILCOLOR_SUB:
        stream << "DETAILCOLOR_SUB";
        break;
    case srShader::DETAILCOLOR_SUBR:
        stream << "DETAILCOLOR_SUBR";
        break;
    case srShader::DETAILCOLOR_BLEND:
        stream << "DETAILCOLOR_BLEND";
        break;
    case srShader::DETAILCOLOR_DETAILBLEND:
        stream << "DETAILCOLOR_DETAILBLEND";
        break;
    }
    stream << '/';
    switch ((shader.value >> 0x17) & 0x1) {
    case srShader::ALPHATEST_DISABLE:
        stream << "ALPHATEST_DISABLE";
        break;
    case srShader::ALPHATEST_ENABLE:
        stream << "ALPHATEST_ENABLE";
        break;
    }
    stream << '/';
    switch ((shader.value >> 0x18) & 0x1) {
    case srShader::DITHER_DISABLE:
        stream << "DITHER_DISABLE";
        break;
    case srShader::DITHER_ENABLE:
        stream << "DITHER_ENABLE";
        break;
    }
    stream << '/';
    switch ((shader.value >> srShader::SRCBLEND_SHIFT) & 0x3) {
    case srShader::SRCBLEND_ZERO:
        stream << "SRCBLEND_ZERO";
        break;
    case srShader::SRCBLEND_ONE:
        stream << "SRCBLEND_ONE";
        break;
    case srShader::SRCBLEND_SRC_ALPHA:
        stream << "SRCBLEND_SRC_ALPHA";
        break;
    case srShader::SRCBLEND_ONE_MINUS_SRC_ALPHA:
        stream << "SRCBLEND_ONE_MINUS_SRC_ALPHA";
        break;
    }
    stream << '/';
    switch ((shader.value >> srShader::DSTBLEND_SHIFT) & 0x7) {
    case srShader::DSTBLEND_ZERO:
        stream << "DSTBLEND_ZERO";
        break;
    case srShader::DSTBLEND_ONE:
        stream << "DSTBLEND_ONE";
        break;
    case srShader::DSTBLEND_SRC_COLOR:
        stream << "DSTBLEND_SRC_COLOR";
        break;
    case srShader::DSTBLEND_ONE_MINUS_SRC_COLOR:
        stream << "DSTBLEND_ONE_MINUS_SRC_COLOR";
        break;
    case srShader::DSTBLEND_SRC_ALPHA:
        stream << "DSTBLEND_SRC_ALPHA";
        break;
    case srShader::DSTBLEND_ONE_MINUS_SRC_ALPHA:
        stream << "DSTBLEND_ONE_MINUS_SRC_ALPHA";
        break;
    }
    stream << '/';
    switch ((shader.value >> srShader::FOG_SHIFT) & 0x3) {
    case srShader::FOG_DISABLE:
        stream << "FOG_DISABLE";
        break;
    case srShader::FOG_ENABLE:
        stream << "FOG_ENABLE";
        break;
    case srShader::FOG_SCALE_FRAGMENT:
        stream << "FOG_SCALE_FRAGMENT";
        break;
    case srShader::FOG_WHITE:
        stream << "FOG_WHITE";
        break;
    }
    stream << '/';
    switch ((shader.value >> 0xa) & 0x3) {
    case srShader::GRADIENT_DISABLE:
        stream << "GRADIENT_DISABLE";
        break;
    case srShader::GRADIENT_MODULATE:
        stream << "GRADIENT_MODULATE";
        break;
    case srShader::GRADIENT_ADD:
        stream << "GRADIENT_ADD";
        break;
    }
    stream << '/';
    switch ((shader.value >> 0xc) & 0x1) {
    case srShader::SECONDARY_GRADIENT_DISABLE:
        stream << "SECONDARY_GRADIENT_DISABLE";
        break;
    case srShader::SECONDARY_GRADIENT_ENABLE:
        stream << "SECONDARY_GRADIENT_ENABLE";
        break;
    }
    stream << '/';
    switch ((shader.value >> 0xf) & 0x1) {
    case srShader::TEXTURING_DISABLE:
        stream << "TEXTURING_DISABLE";
        break;
    case srShader::TEXTURING_ENABLE:
        stream << "TEXTURING_ENABLE";
        break;
    }
    return stream;
}

// TEMPLATE: SURRENDER 0X100347F0
// srClassSupport<srMaterialIFace, srClass, true, 0x2200>::sGetClassNode

// SYNTHETIC: SURRENDER 0X10034B70
// std::ios_base::Init global static-init block

// SYNTHETIC: SURRENDER 0X10034B80
// std::ios_base::Init global atexit registrar

// SYNTHETIC: SURRENDER 0X10034BB0
// std::_Winit global static-init block

// SYNTHETIC: SURRENDER 0X10034BC0
// std::_Winit global atexit registrar

// SYNTHETIC: SURRENDER 0X10034D00
// srMaterialIFace scalar deleting destructor

// SYNTHETIC: SURRENDER 0X10035230
// std::ios_base::Init global static-init block

// SYNTHETIC: SURRENDER 0X10035240
// std::ios_base::Init global atexit registrar

// SYNTHETIC: SURRENDER 0X10035270
// std::_Winit global static-init block

// SYNTHETIC: SURRENDER 0X10035280
// std::_Winit global atexit registrar

// SYNTHETIC: SURRENDER 0X100352B0
// srEnvironmentMapper global static-init block

// SYNTHETIC: SURRENDER 0X100352C0
// srEnvironmentMapper global atexit registrar
