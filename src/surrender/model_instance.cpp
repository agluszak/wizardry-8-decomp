#include "surrender/srModelInstance.h"

#include "surrender/srCore.h"
#include "surrender/srDebug.h"
#include "surrender/srGERD.h"

#include <ostream>

// FUNCTION: SURRENDER 0x1004F920
srModelInstance::srModelInstance(srNode* parent)
    : srClassSupport<srModelInstance, srNode, 0, 0x1100>(static_cast<srNode*>(0))
{
    alignment_flags.value = 0;
    align_angle = 0.0f;
    align_axis.Set(0.0f, 0.0f, 1.0f);
    exclusion_mask = 0;
    if (parent != 0) {
        setParent(parent, 0);
    }
}

// FUNCTION: SURRENDER 0x1004F890
srModelInstance& srModelInstance::operator=(const srModelInstance& other)
{
    if (this != &other) {
        srNode::operator=(other);
        alignment_flags = other.alignment_flags;
        setModel(other.getModel());
        align_angle = other.align_angle;
        align_axis = other.align_axis;
        exclusion_mask = other.exclusion_mask;
    }
    return *this;
}

// FUNCTION: SURRENDER 0x1004FA40
srModelInstance::~srModelInstance() {}

// FUNCTION: SURRENDER 0x1004FF80
srClass* srModelInstance::vInstance()
{
    return new srModelInstance(static_cast<srNode*>(0));
}

// FUNCTION: SURRENDER 0x1004F3A0
void srModelInstance::traverse(TraverseInfo& info)
{
    if (next_sibling_ != 0) {
        next_sibling_->traverse(info);
    }
    if (testFlag(FLAG_DISABLE) == 0 && getModel() != 0) {
        TraverseInfo::Entry& entry = info.entries[info.entry_count];
        entry.node = this;
        entry.value = 0;
        ++info.entry_count;
    }
    if (testFlag(FLAG_TERMINATE) == 0 && first_child_ != 0) {
        first_child_->traverse(info);
    }
}

// FUNCTION: SURRENDER 0x1004F350
void srModelInstance::getLocalBounds(BoundInfo& bounds)
{
    srModel* model = getModel();
    if (model == 0) {
        bounds.state = 0;
        return;
    }
    bounds.state = 1;
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
    ++srCore.getStatisticsManager()->statistics_00.meshes_traversed;
    if ((alignment_flags.value & 1) == 0) {
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
        srVector3T<float> translation = position.xyz();
        renderer->translate(translation);
        if (align_angle != 0.0f) {
            renderer->rotate(align_angle, align_axis);
        }
        renderer->scale((float)world_scale.x * length_x, (float)world_scale.y * length_y,
                        -((float)world_scale.z * length_z));
    }
    if (exclusion_mask != 0) {
        unsigned long mask = renderer->getExclusionMask();
        renderer->setExclusionMask(mask | exclusion_mask);
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
    stream << "  Is aligned: " << srBoolToString(alignment_flags.value & 1) << '\n';
    if ((alignment_flags.value & 1) != 0) {
        stream.width(0x20);
        stream << "    Align axis: ";
        stream << '{' << align_axis.x << ',' << align_axis.y << ',' << align_axis.z
               << '}' << '\n';
        stream.width(0x20);
        stream << "    Align angle: " << (double)align_angle << '\n';
    }
    stream.width(0x20);
    stream << "  Model: ";
    stream << (getModel() != 0 ? getModel()->getName() : "none") << '\n';
    stream.width(0x20);
    stream << "  Exclusion mask: " << exclusion_mask << '\n';
    stream.flags(flags & 0x7fff);
}

// FUNCTION: SURRENDER 0x1004FF40
double srModelInstance::getAlignAngle() const
{
    return (double)align_angle;
}

// FUNCTION: SURRENDER 0x1004FF50
srVector3T<float> srModelInstance::getAlignAxis() const
{
    return align_axis;
}

// FUNCTION: SURRENDER 0x1004FE90
int srModelInstance::isAligned() const
{
    return (int)(alignment_flags.value & 1);
}

// FUNCTION: SURRENDER 0x10050000
unsigned long srModelInstance::getExclusionMask() const
{
    return exclusion_mask;
}

// FUNCTION: SURRENDER 0x1004FE60
void srModelInstance::setAlignment(int enabled)
{
    if (enabled != 0) {
        alignment_flags.value |= 1;
        return;
    }
    alignment_flags.value &= ~1u;
}

// FUNCTION: SURRENDER 0x1004FEA0
void srModelInstance::setAlignAngle(double angle)
{
    align_angle = (float)angle;
    alignment_flags.value |= 1;
}

// FUNCTION: SURRENDER 0x1004FEC0
void srModelInstance::setAlignAxis(srVector3T<float> axis)
{
    align_axis = axis;
    float length_squared = align_axis.z * align_axis.z +
                           align_axis.y * align_axis.y +
                           align_axis.x * align_axis.x;
    if (length_squared != 0.0) {
        float scale = (float)(1.0 / sqrt(length_squared));
        align_axis *= scale;
    }
    alignment_flags.value |= 1;
}

// FUNCTION: SURRENDER 0x1004FFF0
void srModelInstance::setExclusionMask(unsigned long mask)
{
    exclusion_mask = mask;
}
