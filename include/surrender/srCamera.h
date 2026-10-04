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

    enum e_projectionResult { PROJECTION_RESULT_ACCEPTED = 0 };

    struct Rect {
        double left;
        double bottom;
        double right;
        double top;
    };

    srCamera(srNode* parent = 0);

    /* Copy construction, assignment and destruction are the ordinary
       srNode-base/member special members emitted by the class export. */

    virtual void dump(std::ostream& stream) override;
    virtual srClass* vInstance() override;
    virtual void process(const ProcessInfo& info, e_processType type) override;

    void flipHorizontal();
    void flipVertical();
    double getAspectRatio() const;
    void setClipRange(double near_plane, double far_plane);
    /* The reader for the pair setClipRange writes. Both planes come back
       through out-parameters, which is why the caller at 0x0046E440 keeps two
       doubles on its frame and returns only the far one. */
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
    /* Header-visible on both sides: the client getClassName/getClassNode
       emissions (0x0042A020, 0x0042A030) read the consumer literal
       s_srCamera_0060445c directly, the consumer import table has no entry,
       and provider TUs inline the same literal inside the srClassSupport
       registrations. The provider still exports an out-of-line copy. */
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

    srFlags<e_flag> flags_138; /* 0x138 */
private:
    Rect view_plane;              /* 0x140 */
    double view_plane_distance;   /* 0x160 */
    double near_clip_168;             /* 0x168 */
    double far_clip_170;              /* 0x170 */
    float environment_near;       /* 0x178 */
    float environment_far;        /* 0x17c */
    float environment_near_scale; /* 0x180 */
    float environment_far_scale;  /* 0x184 */
};

static_assert((sizeof(srCamera) == 0x188), "srCamera_must_be_0x188");
static_assert(sizeof(srCamera::Rect) == 0x20, "srCamera_Rect_must_be_0x20");
