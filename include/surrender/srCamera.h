#pragma once

#include "srNode.h"

class srGERD;

// VTABLE: SURRENDER 0x10076E74
// class srClassSupport<srCamera, srNode, 0, 5120>

// VTABLE: SURRENDER 0x10076E40 srCamera
// class srCamera
class SR_DLL_IMPORT SR_DLL_EXPORT srCamera : public srClassSupport<srCamera, srNode, 0, 0x1400> {
public:
    enum e_project { PROJECT_PERSPECTIVE = 0, PROJECT_ORTHOGRAPHIC = 1 };

    /* project(): a projected point inside the view is 0, outside its XY bounds is 1, outside the
       near/far interval is 2. */
    enum e_projectionResult {
        PROJECTION_RESULT_ACCEPTED = 0,
        PROJECTION_RESULT_OUTSIDE_VIEW = 1,
        PROJECTION_RESULT_OUTSIDE_CLIP_RANGE = 2
    };

    struct Rect {
        double left;
        double bottom;
        double right;
        double top;
    };

    srCamera(srNode* parent = 0);

#if !defined(SURRENDER_BUILD)
    srCamera& operator=(const srCamera& other);
    virtual ~srCamera() override;
#endif

    virtual void dump(std::ostream& stream) override;
    virtual srClass* vInstance() override;
    virtual void process(const ProcessInfo& info, e_processType type) override;

    void flipHorizontal();
    void flipVertical();
    double getAspectRatio() const;
    void setClipRange(double near_plane, double far_plane);
    void getClipRange(double& near_plane, double& far_plane) const;
    void getEnvironmentRange(float& near_range, float& far_range) const;
    void getEnvironmentScale(float& near_scale, float& far_scale) const;
    double getHorizontalFOV() const;
    void getNormalizedViewPlane(Rect& rectangle) const;
    e_project getProjectionType() const;
    double getVerticalFOV() const;
    e_projectionResult project(srVector3T<float>& output, const srVector3T<double>& input);
    void setProjectionType(e_project projection);
    void normalizeViewPlane();
    // FUNCTION: SURRENDER 0x10048020
    static const char* sGetClassName()
    {
        return "srCamera";
    }
    void setEnvironmentScale(float near_scale, float far_scale);
    void setFocalLength(double focal_length, double aspect_ratio);
    void setFOV(double field_of_view, double aspect_ratio);
    void setViewPlane(const Rect& rectangle, double distance);
    void setViewPlane(double width, double height);
    void getViewPlane(Rect& rectangle, double& distance) const;
    void setEnvironmentRange(float near_range, float far_range);
    int unproject(srVector3T<float>& output, const srVector3T<double>& input);

protected:
    void processPop(srGERD* renderer);
    void processPush(srGERD* renderer);

public:
    enum e_flag { FLAG_PROJECTION_TYPE = 0 };

    srFlags<e_flag> flags; /* 0x138 */
private:
    Rect view_plane;              /* 0x140 */
    double view_plane_distance;   /* 0x160 */
    double near_clip;             /* 0x168 */
    double far_clip;              /* 0x170 */
    float environment_near;       /* 0x178 */
    float environment_far;        /* 0x17c */
    float environment_near_scale; /* 0x180 */
    float environment_far_scale;  /* 0x184 */
};

static_assert((sizeof(srCamera) == 0x188), "srCamera_must_be_0x188");
static_assert(sizeof(srCamera::Rect) == 0x20, "srCamera_Rect_must_be_0x20");
