#pragma once

#include "srNode.h"

class srGERD;

class SR_DLL_IMPORT srCamera : public srClassSupport<srCamera, srNode, 0, 0x1400> {
public:
    enum e_project { PROJECT_POSITIONAL_0 = 0 };

    enum e_projectionResult { PROJECTION_RESULT_POSITIONAL_0 = 0 };

    struct Rect {
        double left;
        double bottom;
        double right;
        double top;
    };

    srCamera(srNode* parent);
    srCamera(const srCamera& other);
    srCamera& operator=(const srCamera& other);

    virtual void dump(std::ostream& stream) override;
    virtual ~srCamera() override;
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
    Rect view_plane_140;              /* 0x140 */
    double view_plane_distance_160;   /* 0x160 */
    double near_clip_168;             /* 0x168 */
    double far_clip_170;              /* 0x170 */
    float environment_near_178;       /* 0x178 */
    float environment_far_17c;        /* 0x17c */
    float environment_near_scale_180; /* 0x180 */
    float environment_far_scale_184;  /* 0x184 */
};

static_assert((sizeof(srCamera) == 0x188), "srCamera_must_be_0x188");
