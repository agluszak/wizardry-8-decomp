#include "surrender/srCamera.h"

#include "surrender/srCore.h"
#include "surrender/srGERD.h"
#include "surrender/srHeap.h"

#include <ostream>

/* Per-class flag-name table: unlike srNode's lazily assigned list, retail
   leaves this global zero-initialized for srCamera, so dump prints numeric
   bit indices. Retail references absolute 0x100A49BC, a slot inside the
   .bss extent already claimed by timer.cpp's storage_class; no GLOBAL
   marker, since the shared address cannot be claimed twice. */
static const char* flag_names;

// FUNCTION: SURRENDER 0x10047D60
std::ostream& operator<<(std::ostream& stream, const srCamera::Rect& rectangle)
{
    stream << "{{" << rectangle.left << ',' << rectangle.bottom << "},{" << rectangle.right << ','
           << rectangle.top << "}}";
    return stream;
}

// FUNCTION: SURRENDER 0x10047DF0
void srCamera::process(const ProcessInfo& info, e_processType type)
{
    if (type == PROCESS_PUSH) {
        processPush(info.renderer);
        return;
    }
    processPop(info.renderer);
}

// FUNCTION: SURRENDER 0x10047E10
srCamera::e_project srCamera::getProjectionType() const
{
    return static_cast<e_project>(flags_138.value & (1UL << FLAG_PROJECTION_TYPE));
}

// FUNCTION: SURRENDER 0x10047E20
void srCamera::setProjectionType(e_project projection)
{
    if (projection == PROJECT_PERSPECTIVE) {
        flags_138.value &= ~(1UL << FLAG_PROJECTION_TYPE);
        return;
    }
    flags_138.value |= 1UL << FLAG_PROJECTION_TYPE;
}

// FUNCTION: SURRENDER 0x10047E50
void srCamera::processPush(srGERD* renderer)
{
    srMatrix4T<double> world;
    getWorldSpaceMatrix(world);

    /* The view transform is the inverse world matrix with the translation
       vector negated. */
    srMatrix4T<double> view;
    view.AdjugateFrom(&world.vectors[0].x);
    double determinant = world.Det();
    if (determinant != 1.0) {
        view.Scale(1.0 / determinant);
    }
    view.vectors[3].x = -view.vectors[3].x;
    view.vectors[3].y = -view.vectors[3].y;
    view.vectors[3].z = -view.vectors[3].z;
    view.vectors[3].w = -view.vectors[3].w;

    renderer->matrixMode(srGERD::MATRIX_MODELVIEW);
    renderer->pushMatrix();
    renderer->loadMatrix(view);
    renderer->matrixMode(srGERD::MATRIX_PROJECTION);
    renderer->pushMatrix();
    renderer->loadIdentity();

    double near_plane;
    double far_plane;
    getClipRange(near_plane, far_plane);

    Rect rectangle;
    double distance;
    getViewPlane(rectangle, distance);
    double scale = near_plane / distance;
    rectangle.left *= scale;
    rectangle.right *= scale;
    rectangle.top *= scale;
    rectangle.bottom *= scale;

    if ((flags_138.value & (1UL << FLAG_PROJECTION_TYPE)) == 0) {
        renderer->frustum(rectangle.left, rectangle.right, rectangle.bottom, rectangle.top,
                          near_plane, far_plane);
    } else {
        renderer->ortho(rectangle.left, rectangle.right, rectangle.bottom, rectangle.top,
                        near_plane, far_plane);
    }

    renderer->pushEnvironment();
    renderer->setEnvironmentRange(environment_near, environment_far);
    renderer->setEnvironmentScaleFactor(environment_near_scale, environment_far_scale);
}

// FUNCTION: SURRENDER 0x10048030
srClass* srCamera::vInstance()
{
    srCamera* instance = static_cast<srCamera*>(srHeap.allocate(0x188));
    if (instance != 0) {
        return new (instance) srCamera(0);
    }
    return 0;
}

// FUNCTION: SURRENDER 0x100482E0
void srCamera::processPop(srGERD* renderer)
{
    renderer->matrixMode(srGERD::MATRIX_MODELVIEW);
    renderer->popMatrix();
    renderer->matrixMode(srGERD::MATRIX_PROJECTION);
    renderer->popMatrix();
    renderer->popEnvironment();
}

// FUNCTION: SURRENDER 0x10048430
double srCamera::getAspectRatio() const
{
    return (view_plane.right - view_plane.left) /
           (view_plane.top - view_plane.bottom);
}

// FUNCTION: SURRENDER 0x10048450
srCamera::srCamera(srNode* parent)
    : srClassSupport<srCamera, srNode, 0, 0x1400>(static_cast<srNode*>(0))
{
    flags_138.value = 0;
    near_clip_168 = 0.1;
    far_clip_170 = 1000.0;
    environment_near = 0.0f;
    environment_far = 1000.0f;
    environment_near_scale = 0.0f;
    environment_far_scale = 1.0f;
    if (parent != 0) {
        setParent(parent, 0);
    }
    setFOV(3.141592653589793 * (1.0f / 180.0f) * 90.0f, 1.3333333333333333);
}

// FUNCTION: SURRENDER 0x100485A0
void srCamera::getNormalizedViewPlane(Rect& rectangle) const
{
    rectangle.left = view_plane.left / view_plane_distance;
    rectangle.bottom = view_plane.bottom / view_plane_distance;
    rectangle.right = view_plane.right / view_plane_distance;
    rectangle.top = view_plane.top / view_plane_distance;
}

// FUNCTION: SURRENDER 0x100485F0
void srCamera::normalizeViewPlane()
{
    view_plane.left /= view_plane_distance;
    view_plane.bottom /= view_plane_distance;
    view_plane.right /= view_plane_distance;
    view_plane.top /= view_plane_distance;
    view_plane_distance = 1.0;
}

// FUNCTION: SURRENDER 0x10048650
void srCamera::getViewPlane(Rect& rectangle, double& distance) const
{
    rectangle.left = view_plane.left;
    rectangle.bottom = view_plane.bottom;
    rectangle.right = view_plane.right;
    rectangle.top = view_plane.top;
    distance = view_plane_distance;
}

// FUNCTION: SURRENDER 0x100486C0
void srCamera::setEnvironmentRange(float near_range, float far_range)
{
    environment_near = near_range;
    environment_far = far_range;
}

// FUNCTION: SURRENDER 0x100486E0
void srCamera::setEnvironmentScale(float near_scale, float far_scale)
{
    environment_near_scale = near_scale;
    environment_far_scale = far_scale;
}

// FUNCTION: SURRENDER 0x10048700
void srCamera::getEnvironmentRange(float& near_range, float& far_range) const
{
    near_range = environment_near;
    far_range = environment_far;
}

// FUNCTION: SURRENDER 0x10048720
void srCamera::getEnvironmentScale(float& near_scale, float& far_scale) const
{
    near_scale = environment_near_scale;
    far_scale = environment_far_scale;
}

// FUNCTION: SURRENDER 0x10048740
void srCamera::setClipRange(double near_plane, double far_plane)
{
    near_clip_168 = near_plane;
    far_clip_170 = far_plane;
    if (near_clip_168 > far_clip_170) {
        double swap = near_clip_168;
        near_clip_168 = far_clip_170;
        far_clip_170 = swap;
    }
    if (near_clip_168 < 5.9604644775390625e-08) {
        near_clip_168 = 5.9604644775390625e-08;
    }
    if (far_clip_170 < 5.9604644775390625e-08) {
        far_clip_170 = 5.9604644775390625e-08;
    }
}

// FUNCTION: SURRENDER 0x100487F0
void srCamera::getClipRange(double& near_plane, double& far_plane) const
{
    near_plane = near_clip_168;
    far_plane = far_clip_170;
}

// FUNCTION: SURRENDER 0x10048820
void srCamera::setViewPlane(const Rect& rectangle, double distance)
{
    if (distance > 5.9604644775390625e-08) {
        view_plane.left = rectangle.left;
        view_plane.bottom = rectangle.bottom;
        view_plane.right = rectangle.right;
        view_plane.top = rectangle.top;
        view_plane_distance = distance;
    }
}

// FUNCTION: SURRENDER 0x100488A0
void srCamera::setFocalLength(double focal_length, double aspect_ratio)
{
    if (focal_length < 5.9604644775390625e-08) {
        focal_length = 5.9604644775390625e-08;
    }
    if (aspect_ratio < 5.9604644775390625e-08) {
        aspect_ratio = 5.9604644775390625e-08;
    }
    double angle = atan(0.018 / focal_length);
    setViewPlane((angle + angle) * aspect_ratio, angle + angle);
}

// FUNCTION: SURRENDER 0x10048920
void srCamera::setViewPlane(double width, double height)
{
    if (width <= -3.141592653589793) {
        width = -3.141592653589793;
    } else if (width >= 3.141592653589793) {
        width = 3.141592653589793;
    }
    if (height <= -3.141592653589793) {
        height = -3.141592653589793;
    } else if (height >= 3.141592653589793) {
        height = 3.141592653589793;
    }
    double half_width = tan(width * 0.5);
    view_plane_distance = 1.0;
    double half_height = tan(height * 0.5);
    Rect rectangle;
    rectangle.left = -half_width;
    rectangle.bottom = -half_height;
    rectangle.right = half_width;
    rectangle.top = half_height;
    view_plane = rectangle;
}

// FUNCTION: SURRENDER 0x10048A10
void srCamera::setFOV(double field_of_view, double aspect_ratio)
{
    double angle = fabs(field_of_view);
    double maximum = 3.141592653589793 - 5.9604644775390625e-08;
    if (angle <= 5.9604644775390625e-08) {
        angle = 5.9604644775390625e-08;
    } else if (angle >= maximum) {
        angle = maximum;
    }
    double half_angle = tan(angle * 0.5);
    view_plane_distance = 1.0;
    Rect rectangle;
    rectangle.left = -(half_angle * aspect_ratio);
    rectangle.bottom = -half_angle;
    rectangle.right = half_angle * aspect_ratio;
    rectangle.top = half_angle;
    view_plane = rectangle;
}

// FUNCTION: SURRENDER 0x10048AB0
double srCamera::getHorizontalFOV() const
{
    return atan(view_plane.right / view_plane_distance) -
           atan(view_plane.left / view_plane_distance);
}

// FUNCTION: SURRENDER 0x10048AE0
double srCamera::getVerticalFOV() const
{
    return atan(view_plane.top / view_plane_distance) -
           atan(view_plane.bottom / view_plane_distance);
}

// FUNCTION: SURRENDER 0x10048B10
void srCamera::flipVertical()
{
    double swap = view_plane.bottom;
    view_plane.bottom = view_plane.top;
    view_plane.top = swap;
}

// FUNCTION: SURRENDER 0x10048B40
void srCamera::flipHorizontal()
{
    double swap = view_plane.left;
    view_plane.left = view_plane.right;
    view_plane.right = swap;
}

// FUNCTION: SURRENDER 0x10048B70
void srCamera::dump(std::ostream& stream)
{
    srNode::dump(stream);
    long flags = stream.flags();
    stream.flags((flags & 0xfffffe7fL) | 0x40);
    stream.width(0x20);
    stream << "Control flags: ";
    if (flags_138.value == 0) {
        stream << "[NONE]";
    } else {
        stream << '[';
        int first = 1;
        const char* names = flag_names;
        for (unsigned long bit = 0; bit < 0x20; ++bit) {
            if ((flags_138.value & (1UL << bit)) != 0) {
                if (first == 0) {
                    stream << ',';
                } else {
                    first = 0;
                }
                if (names == 0 || *names == '\0') {
                    stream << bit;
                } else {
                    while (*names != '\0' && *names != ',') {
                        stream << *names;
                        ++names;
                    }
                    if (*names == ',') {
                        ++names;
                    }
                }
            } else if (names != 0) {
                while (*names != '\0' && *names != ',') {
                    ++names;
                }
                if (*names == ',') {
                    ++names;
                }
            }
        }
        stream << ']';
    }
    stream << '\n';
    stream.width(0x20);
    stream << "  Viewplane: " << view_plane << '\n';
    stream.width(0x20);
    stream << "  Viewplane distance: " << view_plane_distance << '\n';
    stream.width(0x20);
    double near_plane;
    double far_plane;
    getClipRange(near_plane, far_plane);
    stream << "  Object space Clip Range (near,far): " << near_plane << ',' << far_plane << '\n';
    stream.width(0x20);
    float near_range;
    float far_range;
    getEnvironmentRange(near_range, far_range);
    stream << "  Environment Range (near,far): " << near_range << ',' << far_range << '\n';
    float near_scale;
    float far_scale;
    getEnvironmentScale(near_scale, far_scale);
    stream << "  Environment Scale (near,far): " << near_scale << ',' << far_scale << '\n';
    stream.flags(flags & 0x7fff);
}

// FUNCTION: SURRENDER 0x10048E30
srCamera::e_projectionResult srCamera::project(srVector3T<float>& output,
                                               const srVector3T<double>& input)
{
    srMatrix3T<double> rotation;
    getWorldSpaceRotation(rotation);
    srVector3T<double> location = getWorldSpaceLocation();
    srVector3T<double> delta = input - location;
    srVector3T<double> point = rotation.Transform(delta);

    double near_clip = near_clip_168 < far_clip_170 ? near_clip_168 : far_clip_170;
    double far_clip = near_clip_168 < far_clip_170 ? far_clip_170 : near_clip_168;
    if (near_clip <= point.z && point.z <= far_clip) {
        double inverse_distance = 1.0 / view_plane_distance;
        double bottom = inverse_distance * view_plane.bottom;
        double top = inverse_distance * view_plane.top;
        double lower = top;
        double upper = bottom;
        if (bottom < top) {
            lower = bottom;
            upper = top;
        }
        double left = inverse_distance * view_plane.left;
        double right = inverse_distance * view_plane.right;
        double x_lower = right;
        double x_upper = left;
        if (left < right) {
            x_lower = left;
            x_upper = right;
        }
        output.x = (float)((point.x / point.z - left) / (right - left));
        output.y = (float)((point.y / point.z - top) / (bottom - top));
        output.z = (float)point.z;
        if (point.z * x_upper <= point.x && point.x <= point.z * x_lower &&
            point.z * lower <= point.y && point.y <= point.z * upper) {
            return PROJECTION_RESULT_ACCEPTED;
        }
        return static_cast<e_projectionResult>(1);
    }
    output.SetZero();
    return static_cast<e_projectionResult>(2);
}

// FUNCTION: SURRENDER 0x100490B0
int srCamera::unproject(srVector3T<float>& output, const srVector3T<double>& input)
{
    double left = view_plane.left / view_plane_distance;
    double right = view_plane.right / view_plane_distance;
    double bottom = view_plane.bottom / view_plane_distance;
    double top = view_plane.top / view_plane_distance;
    double depth = input.z;
    double x = ((right - left) * input.x + left) * depth;
    double y = ((bottom - top) * input.y + top) * depth;
    double near_clip = near_clip_168 < far_clip_170 ? near_clip_168 : far_clip_170;
    double far_clip = near_clip_168 < far_clip_170 ? far_clip_170 : near_clip_168;
    output.SetZero();
    if (near_clip <= depth && depth <= far_clip) {
        double x_upper = left;
        double x_lower = right;
        if (left < right) {
            x_upper = right;
            x_lower = left;
        }
        double y_lower = top;
        double y_upper = bottom;
        if (bottom < top) {
            y_lower = bottom;
            y_upper = top;
        }
        if (y_lower * depth <= y && y <= y_upper * depth && x_lower * depth <= x &&
            x <= x_upper * depth) {
            srMatrix3T<double> rotation;
            getWorldSpaceRotation(rotation);
            srVector3T<double> location = getWorldSpaceLocation();
            srVector3T<double> point;
            point.x = x;
            point.y = y;
            point.z = depth;
            srVector3T<double> world = rotation.TransformTransposed(point);
            output.x = (float)(world.x + location.x);
            output.y = (float)(world.y + location.y);
            output.z = (float)(world.z + location.z);
            return 1;
        }
    }
    return 0;
}
