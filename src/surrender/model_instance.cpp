#include "surrender/srModelInstance.h"

#include "surrender/srCore.h"
#include "surrender/srDebug.h"
#include "surrender/srGERD.h"

// FUNCTION: SURRENDER 0x1004F920
srModelInstance::srModelInstance(srNode* parent)
    : srClassSupport<srModelInstance, srNode, 0, 0x1100>(static_cast<srNode*>(0))
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x1100);
    if (node == 0) {
        node = registry->getClassNode(0x1000);
        if (node == 0) {
            node = registry->registerClass(srNode::sGetClassName(), srClass::sGetClassNode(),
                                           0x1000, 1);
        }
        node = registry->registerClass(sGetClassName(), node, 0x1100, 0);
    }
    registry->registerInstance(node, this);
    alignment_flags_148.value = 0;
    align_angle_158 = 0.0f;
    align_axis_14c.x = 0.0f;
    align_axis_14c.y = 0.0f;
    align_axis_14c.z = 1.0f;
    exclusion_mask_15c = 0;
    if (parent != 0) {
        setParent(parent, 0);
    }
}

// FUNCTION: SURRENDER 0x10050010
srModelInstance::srModelInstance(const srModelInstance& other)
    : srClassSupport<srModelInstance, srNode, 0, 0x1100>(static_cast<srNode*>(0)),
      srModel::Client(other)
{
    *this = other;
    alignment_flags_148 = other.alignment_flags_148;
    align_axis_14c = other.align_axis_14c;
    align_angle_158 = other.align_angle_158;
    exclusion_mask_15c = other.exclusion_mask_15c;
}

// FUNCTION: SURRENDER 0x1004F890
srModelInstance& srModelInstance::operator=(const srModelInstance& other)
{
    if (this != &other) {
        srNode::operator=(other);
        alignment_flags_148 = other.alignment_flags_148;
        setModel(other.getModel());
        align_angle_158 = other.align_angle_158;
        align_axis_14c = other.align_axis_14c;
        exclusion_mask_15c = other.exclusion_mask_15c;
    }
    return *this;
}

// FUNCTION: SURRENDER 0x1004FA40
srModelInstance::~srModelInstance()
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x1100);
    if (node == 0) {
        node = registry->getClassNode(0x1000);
        if (node == 0) {
            node = registry->registerClass(srNode::sGetClassName(), srClass::sGetClassNode(),
                                           0x1000, 1);
        }
        node = registry->registerClass(sGetClassName(), node, 0x1100, 0);
    }
    registry->unregisterInstance(node, this);
}

// FUNCTION: SURRENDER 0x1004FF80
srClass* srModelInstance::vInstance()
{
    return new srModelInstance(static_cast<srNode*>(0));
}

// FUNCTION: SURRENDER 0x1004FFE0
const char* srModelInstance::sGetClassName()
{
    return "srModelInstance";
}

// FUNCTION: SURRENDER 0x1004F3A0
void srModelInstance::traverse(TraverseInfo& info)
{
    if (nextSibling() != 0) {
        nextSibling()->traverse(info);
    }
    if (testFlag(FLAG_DISABLE) == 0 && getModel() != 0) {
        TraverseInfo::Entry& entry = info.entries[info.entry_count];
        entry.node = this;
        entry.value = 0;
        ++info.entry_count;
    }
    if (testFlag(FLAG_TERMINATE) == 0 && firstChild() != 0) {
        firstChild()->traverse(info);
    }
}

// FUNCTION: SURRENDER 0x1004F350
void srModelInstance::getLocalBounds(BoundInfo& bounds)
{
    srModel* model = getModel();
    if (model == 0) {
        bounds.state_28 = 0;
        return;
    }
    bounds.state_28 = 1;
    model->getBoundingBox(bounds.minimum, bounds.maximum);
    model->getBoundingSphere(bounds.center, bounds.radius);
}

// FUNCTION: SURRENDER 0x1004F320
void srModelInstance::updateClient(srModel::Client::e_update update)
{
    if (update == 0) {
        notifyParents(srFlags<e_notify>(0));
    }
}

// FUNCTION: SURRENDER 0x1004F4B0
void srModelInstance::process(const ProcessInfo& info, e_processType type)
{
    srGERD* renderer = info.renderer;
    ++srCore.getStatisticsManager()->statistics_00.meshes_traversed_08;
    if ((alignment_flags_148.value & 1) == 0) {
        applyWorldSpaceMatrix(*renderer);
    } else {
        renderer->matrixMode(srGERD::MATRIX_MODELVIEW);
        renderer->pushMatrix();
        srMatrix4T<float> matrix;
        renderer->getMatrix(srGERD::MATRIX_MODELVIEW, matrix);
        srVector3T<double> location = getWorldSpaceLocation();
        float x = (float)location.x;
        float y = (float)location.y;
        float z = (float)location.z;
        srVector3T<double> world_scale = getWorldSpaceScale();
        srVector4T<float> position;
        position.Set(matrix.vectors[0].x * x + matrix.vectors[0].z * z + matrix.vectors[0].y * y +
                         matrix.vectors[0].w,
                     matrix.vectors[1].x * x + matrix.vectors[1].z * z + matrix.vectors[1].y * y +
                         matrix.vectors[1].w,
                     matrix.vectors[2].x * x + matrix.vectors[2].z * z + matrix.vectors[2].y * y +
                         matrix.vectors[2].w,
                     matrix.vectors[3].x * x + matrix.vectors[3].z * z + matrix.vectors[3].y * y +
                         matrix.vectors[3].w);
        float length_x = (float)sqrt(matrix.vectors[0].x * matrix.vectors[0].x +
                                     matrix.vectors[1].x * matrix.vectors[1].x +
                                     matrix.vectors[2].x * matrix.vectors[2].x);
        float length_y = (float)sqrt(matrix.vectors[0].y * matrix.vectors[0].y +
                                     matrix.vectors[1].y * matrix.vectors[1].y +
                                     matrix.vectors[2].y * matrix.vectors[2].y);
        float length_z = (float)sqrt(matrix.vectors[0].z * matrix.vectors[0].z +
                                     matrix.vectors[1].z * matrix.vectors[1].z +
                                     matrix.vectors[2].z * matrix.vectors[2].z);
        float determinant = matrix.vectors[0].x * (matrix.vectors[2].z * matrix.vectors[1].y -
                                                   matrix.vectors[2].y * matrix.vectors[1].z) +
                            matrix.vectors[1].x * (matrix.vectors[2].y * matrix.vectors[0].z -
                                                   matrix.vectors[2].z * matrix.vectors[0].y) +
                            matrix.vectors[2].x * (matrix.vectors[1].z * matrix.vectors[0].y -
                                                   matrix.vectors[1].y * matrix.vectors[0].z);
        if (determinant > 0.0f) {
            length_x = -length_x;
            length_y = -length_y;
            length_z = -length_z;
        }
        renderer->loadIdentity();
        srVector3T<float> translation;
        translation.Set(position.x, position.y, position.z);
        renderer->translate(translation);
        if (align_angle_158 != 0.0f) {
            renderer->rotate(align_angle_158, align_axis_14c);
        }
        renderer->scale((float)world_scale.x * length_x, (float)world_scale.y * length_y,
                        -((float)world_scale.z * length_z));
    }
    if (exclusion_mask_15c != 0) {
        unsigned long mask = renderer->getExclusionMask();
        renderer->setExclusionMask(mask | exclusion_mask_15c);
        getModel()->render(*renderer);
        renderer->setExclusionMask(mask);
    } else {
        getModel()->render(*renderer);
    }
    renderer->matrixMode(srGERD::MATRIX_MODELVIEW);
    renderer->popMatrix();
}

// FUNCTION: SURRENDER 0x1004FB20
void srModelInstance::dump(std::ostream& stream)
{
    srNode::dump(stream);
    long flags = stream.flags();
    stream.flags((flags & 0xfffffe7fL) | 0x40);
    stream.width(0x20);
    stream << "  Is aligned: " << srBoolToString(alignment_flags_148.value & 1) << '\n';
    if ((alignment_flags_148.value & 1) != 0) {
        stream.width(0x20);
        stream << "    Align axis: ";
        stream << '{' << align_axis_14c.x << ',' << align_axis_14c.y << ',' << align_axis_14c.z
               << '}' << '\n';
        stream.width(0x20);
        stream << "    Align angle: " << (double)align_angle_158 << '\n';
    }
    stream.width(0x20);
    stream << "  Model: ";
    stream << (getModel() != 0 ? getModel()->getName() : "none") << '\n';
    stream.width(0x20);
    stream << "  Exclusion mask: " << exclusion_mask_15c << '\n';
    stream.flags(flags & 0x7fff);
}

// FUNCTION: SURRENDER 0x1004FF40
double srModelInstance::getAlignAngle() const
{
    return (double)align_angle_158;
}

// FUNCTION: SURRENDER 0x1004FF50
srVector3T<float> srModelInstance::getAlignAxis() const
{
    return align_axis_14c;
}

// FUNCTION: SURRENDER 0x1004FE90
int srModelInstance::isAligned() const
{
    return (int)(alignment_flags_148.value & 1);
}

// FUNCTION: SURRENDER 0x10050000
unsigned long srModelInstance::getExclusionMask() const
{
    return exclusion_mask_15c;
}

// FUNCTION: SURRENDER 0x1004FE60
void srModelInstance::setAlignment(int enabled)
{
    if (enabled != 0) {
        alignment_flags_148.value |= 1;
        return;
    }
    alignment_flags_148.value &= ~1u;
}

// FUNCTION: SURRENDER 0x1004FEA0
void srModelInstance::setAlignAngle(double angle)
{
    align_angle_158 = (float)angle;
    alignment_flags_148.value |= 1;
}

// FUNCTION: SURRENDER 0x1004FEC0
void srModelInstance::setAlignAxis(srVector3T<float> axis)
{
    align_axis_14c = axis;
    float length_squared = align_axis_14c.z * align_axis_14c.z +
                           align_axis_14c.y * align_axis_14c.y +
                           align_axis_14c.x * align_axis_14c.x;
    if (length_squared != 0.0) {
        float scale = (float)(1.0 / sqrt(length_squared));
        align_axis_14c *= scale;
    }
    alignment_flags_148.value |= 1;
}

// FUNCTION: SURRENDER 0x1004FFF0
void srModelInstance::setExclusionMask(unsigned long mask)
{
    exclusion_mask_15c = mask;
}

// SYNTHETIC: SURRENDER 0x10050170
// srModelInstance default constructor closure

// SYNTHETIC: SURRENDER 0x10050180
// srModelInstance scalar deleting destructor

// SYNTHETIC: SURRENDER 0x100501A0
// srModelInstance vector deleting destructor

// TEMPLATE: SURRENDER 0x1004FDA0
// srClassSupport<srModelInstance, srNode, 0, 0x1100>::~srClassSupport

// SYNTHETIC: SURRENDER 0x10050200
// srClassSupport<srModelInstance, srNode, 0, 0x1100> scalar deleting destructor

// LIBRARY: SURRENDER 0x10050230
// std::ios_base::Init::Init

// SYNTHETIC: SURRENDER 0x10050240
// std::ios_base::Init global atexit registrar

// LIBRARY: SURRENDER 0x10050270
// std::_Winit::_Winit

// SYNTHETIC: SURRENDER 0x10050280
// std::_Winit global atexit registrar

// LIBRARY: SURRENDER 0x100502B0
// MFC CRect::CRect
