#include "surrender/srCamera.h"

#include "surrender/srCore.h"
#include "surrender/srGERD.h"
#include "surrender/srHeap.h"

/* Per-class flag-name table: unlike srNode's lazily assigned list, retail
   leaves this global zero-initialized for srCamera, so dump prints numeric
   bit indices. Retail references absolute 0x100A49BC, a slot inside the
   .bss extent already claimed by timer.cpp's storage_class; no GLOBAL
   marker, since the shared address cannot be claimed twice. */
static const char* flag_names;

// FUNCTION: SURRENDER 0x10047D60
std::ostream& operator<<(std::ostream& stream, const srCamera::Rect& rectangle)
{
    stream << '{' << rectangle.left << ',' << rectangle.bottom << "},{" << rectangle.right << ','
           << rectangle.top << '}';
    return stream;
}

// FUNCTION: SURRENDER 0x10047DF0
void srCamera::process(const ProcessInfo& info, e_processType type)
{
    if (static_cast<int>(type) == 1) {
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
    if (projection == PROJECT_POSITIONAL_0) {
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
    renderer->setEnvironmentRange(environment_near_178, environment_far_17c);
    renderer->setEnvironmentScaleFactor(environment_near_scale_180, environment_far_scale_184);
}

// FUNCTION: SURRENDER 0x10048020
const char* srCamera::sGetClassName()
{
    return "srCamera";
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

/* The retail copy constructor re-copies every member after running the
   assignment operator. */
// FUNCTION: SURRENDER 0x10048090
srCamera::srCamera(const srCamera& other)
    : srClassSupport<srCamera, srNode, 0, 0x1400>(static_cast<srNode*>(0))
{
    *this = other;
    flags_138.value = other.flags_138.value;
    view_plane_140 = other.view_plane_140;
    view_plane_distance_160 = other.view_plane_distance_160;
    near_clip_168 = other.near_clip_168;
    far_clip_170 = other.far_clip_170;
    environment_near_178 = other.environment_near_178;
    environment_far_17c = other.environment_far_17c;
    environment_near_scale_180 = other.environment_near_scale_180;
    environment_far_scale_184 = other.environment_far_scale_184;
}

// FUNCTION: SURRENDER 0x100481F0
srCamera::~srCamera() {}

// FUNCTION: SURRENDER 0x100482E0
void srCamera::processPop(srGERD* renderer)
{
    renderer->matrixMode(srGERD::MATRIX_MODELVIEW);
    renderer->popMatrix();
    renderer->matrixMode(srGERD::MATRIX_PROJECTION);
    renderer->popMatrix();
    renderer->popEnvironment();
}

// FUNCTION: SURRENDER 0x10048370
srCamera& srCamera::operator=(const srCamera& other)
{
    if (this != &other) {
        srNode::operator=(other);
        view_plane_140 = other.view_plane_140;
        view_plane_distance_160 = other.view_plane_distance_160;
        near_clip_168 = other.near_clip_168;
        far_clip_170 = other.far_clip_170;
        flags_138.value = other.flags_138.value;
        environment_near_178 = other.environment_near_178;
        environment_far_17c = other.environment_far_17c;
        environment_near_scale_180 = other.environment_near_scale_180;
        environment_far_scale_184 = other.environment_far_scale_184;
    }
    return *this;
}

// FUNCTION: SURRENDER 0x10048430
double srCamera::getAspectRatio() const
{
    return (view_plane_140.right - view_plane_140.left) /
           (view_plane_140.top - view_plane_140.bottom);
}

// FUNCTION: SURRENDER 0x10048450
srCamera::srCamera(srNode* parent)
    : srClassSupport<srCamera, srNode, 0, 0x1400>(static_cast<srNode*>(0))
{
    flags_138.value = 0;
    near_clip_168 = 0.1;
    far_clip_170 = 1000.0;
    environment_near_178 = 0.0f;
    environment_far_17c = 1000.0f;
    environment_near_scale_180 = 0.0f;
    environment_far_scale_184 = 1.0f;
    if (parent != 0) {
        setParent(parent, 0);
    }
    setFOV(3.141592653589793 * (1.0f / 180.0f) * 90.0f, 1.3333333333333333);
}

// FUNCTION: SURRENDER 0x100485A0
void srCamera::getNormalizedViewPlane(Rect& rectangle) const
{
    rectangle.left = view_plane_140.left / view_plane_distance_160;
    rectangle.bottom = view_plane_140.bottom / view_plane_distance_160;
    rectangle.right = view_plane_140.right / view_plane_distance_160;
    rectangle.top = view_plane_140.top / view_plane_distance_160;
}

// FUNCTION: SURRENDER 0x100485F0
void srCamera::normalizeViewPlane()
{
    view_plane_140.left /= view_plane_distance_160;
    view_plane_140.bottom /= view_plane_distance_160;
    view_plane_140.right /= view_plane_distance_160;
    view_plane_140.top /= view_plane_distance_160;
    view_plane_distance_160 = 1.0;
}

// FUNCTION: SURRENDER 0x10048650
void srCamera::getViewPlane(Rect& rectangle, double& distance) const
{
    rectangle.left = view_plane_140.left;
    rectangle.bottom = view_plane_140.bottom;
    rectangle.right = view_plane_140.right;
    rectangle.top = view_plane_140.top;
    distance = view_plane_distance_160;
}

// FUNCTION: SURRENDER 0x100486C0
void srCamera::setEnvironmentRange(float near_range, float far_range)
{
    environment_near_178 = near_range;
    environment_far_17c = far_range;
}

// FUNCTION: SURRENDER 0x100486E0
void srCamera::setEnvironmentScale(float near_scale, float far_scale)
{
    environment_near_scale_180 = near_scale;
    environment_far_scale_184 = far_scale;
}

// FUNCTION: SURRENDER 0x10048700
void srCamera::getEnvironmentRange(float& near_range, float& far_range) const
{
    near_range = environment_near_178;
    far_range = environment_far_17c;
}

// FUNCTION: SURRENDER 0x10048720
void srCamera::getEnvironmentScale(float& near_scale, float& far_scale) const
{
    near_scale = environment_near_scale_180;
    far_scale = environment_far_scale_184;
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
        view_plane_140.left = rectangle.left;
        view_plane_140.bottom = rectangle.bottom;
        view_plane_140.right = rectangle.right;
        view_plane_140.top = rectangle.top;
        view_plane_distance_160 = distance;
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
    view_plane_distance_160 = 1.0;
    double half_height = tan(height * 0.5);
    Rect rectangle;
    rectangle.left = -half_width;
    rectangle.bottom = -half_height;
    rectangle.right = half_width;
    rectangle.top = half_height;
    view_plane_140 = rectangle;
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
    view_plane_distance_160 = 1.0;
    Rect rectangle;
    rectangle.left = -(half_angle * aspect_ratio);
    rectangle.bottom = -half_angle;
    rectangle.right = half_angle * aspect_ratio;
    rectangle.top = half_angle;
    view_plane_140 = rectangle;
}

// FUNCTION: SURRENDER 0x10048AB0
double srCamera::getHorizontalFOV() const
{
    return atan(view_plane_140.right / view_plane_distance_160) -
           atan(view_plane_140.left / view_plane_distance_160);
}

// FUNCTION: SURRENDER 0x10048AE0
double srCamera::getVerticalFOV() const
{
    return atan(view_plane_140.top / view_plane_distance_160) -
           atan(view_plane_140.bottom / view_plane_distance_160);
}

// FUNCTION: SURRENDER 0x10048B10
void srCamera::flipVertical()
{
    double swap = view_plane_140.bottom;
    view_plane_140.bottom = view_plane_140.top;
    view_plane_140.top = swap;
}

// FUNCTION: SURRENDER 0x10048B40
void srCamera::flipHorizontal()
{
    double swap = view_plane_140.left;
    view_plane_140.left = view_plane_140.right;
    view_plane_140.right = swap;
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
    stream << "  Viewplane: " << view_plane_140 << '\n';
    stream.width(0x20);
    stream << "  Viewplane distance: " << view_plane_distance_160 << '\n';
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
    srVector3T<double> delta;
    delta.x = input.x - location.x;
    delta.y = input.y - location.y;
    delta.z = input.z - location.z;
    srVector3T<double> point = rotation.Transform(delta);

    double near_clip = near_clip_168 < far_clip_170 ? near_clip_168 : far_clip_170;
    double far_clip = near_clip_168 < far_clip_170 ? far_clip_170 : near_clip_168;
    if (near_clip <= point.z && point.z <= far_clip) {
        double inverse_distance = 1.0 / view_plane_distance_160;
        double bottom = inverse_distance * view_plane_140.bottom;
        double top = inverse_distance * view_plane_140.top;
        double lower = top;
        double upper = bottom;
        if (bottom < top) {
            lower = bottom;
            upper = top;
        }
        double left = inverse_distance * view_plane_140.left;
        double right = inverse_distance * view_plane_140.right;
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
            return PROJECTION_RESULT_POSITIONAL_0;
        }
        return static_cast<e_projectionResult>(1);
    }
    output.x = 0.0f;
    output.y = 0.0f;
    output.z = 0.0f;
    return static_cast<e_projectionResult>(2);
}

// FUNCTION: SURRENDER 0x100490B0
int srCamera::unproject(srVector3T<float>& output, const srVector3T<double>& input)
{
    double left = view_plane_140.left / view_plane_distance_160;
    double right = view_plane_140.right / view_plane_distance_160;
    double bottom = view_plane_140.bottom / view_plane_distance_160;
    double top = view_plane_140.top / view_plane_distance_160;
    double depth = input.z;
    double x = ((right - left) * input.x + left) * depth;
    double y = ((bottom - top) * input.y + top) * depth;
    double near_clip = near_clip_168 < far_clip_170 ? near_clip_168 : far_clip_170;
    double far_clip = near_clip_168 < far_clip_170 ? far_clip_170 : near_clip_168;
    output.x = 0.0f;
    output.y = 0.0f;
    output.z = 0.0f;
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

// TEMPLATE: SURRENDER 0x10049400
// srClassSupport<srCamera, srNode, 0, 0x1400>::clone

// TEMPLATE: SURRENDER 0x10049420
// srClassSupport<srCamera, srNode, 0, 0x1400>::getClassNode

// SYNTHETIC: SURRENDER 0x10049560
// srCamera scalar deleting destructor

// TEMPLATE: SURRENDER 0x10049580
// srMatrix4T<double>::Scale

// TEMPLATE: SURRENDER 0x10049620
// srMatrix4T<double>::Det

// TEMPLATE: SURRENDER 0x10049780
// srMatrix4T<double>::AdjugateFrom
