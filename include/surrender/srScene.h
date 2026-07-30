#pragma once

#include "srNode.h"

class srGERD;
class srModeler;

class SR_DLL_IMPORT srScene : public srNode {
public:
    enum e_enable {
        ENABLE_POSITIONAL_0 = 0
    };

    struct Statistics {
        double value_00;
        unsigned long value_08;
        unsigned long value_0c;
        unsigned long value_10;
        unsigned long value_14;
    };

    srScene(srNode* parent);
    srScene& operator=(const srScene& other);

    virtual void dump(std::ostream& stream) override;
    virtual ~srScene() override;
    virtual srClass* vInstance() override;
    virtual void traverse(TraverseInfo& info) override;
    virtual void process(const ProcessInfo& info, e_processType type) override;

    void disable(e_enable option);
    void enable(e_enable option);
    void getAmbientLight(srVector3T<float>& color) const;
    srVector3T<float> getAmbientLight() const;
    void getFogColor(srVector3T<float>& color) const;
    srVector3T<float> getFogColor() const;
    void getStatistics(Statistics& statistics);
    int isEnabled(e_enable option) const;
    void render(srGERD& renderer, class srCamera* camera);
    void resetStatistics();
    static const char* sGetClassName();
    void setAmbientLight(float red, float green, float blue);
    void setAmbientLight(const srVector3T<float>& color);
    void setFogColor(float red, float green, float blue);
    void setFogColor(const srVector3T<float>& color);

protected:
    srFlags<e_enable> enabled_138;          /* 0x138 */
    unsigned long unknown_13c_;             /* 0x13c */
    Statistics statistics_140;              /* 0x140 */
    TraverseInfo traversal_158;              /* 0x158 */
    ProcessInfo process_info_170;            /* 0x170 */
    srVector3T<float> ambient_light_174;     /* 0x174 */
    srVector3T<float> fog_color_180;         /* 0x180 */
    unsigned long unknown_18c_;              /* 0x18c */
};

class SR_DLL_IMPORT srCamera : public srNode {
public:
    enum e_project {
        PROJECT_POSITIONAL_0 = 0
    };

    enum e_projectionResult {
        PROJECTION_RESULT_POSITIONAL_0 = 0
    };

    struct Rect {
        double left;
        double bottom;
        double right;
        double top;
    };

    srCamera(srNode* parent);
    srCamera& operator=(const srCamera& other);

    virtual void dump(std::ostream& stream) override;
    virtual ~srCamera() override;
    virtual srClass* vInstance() override;
    virtual void process(const ProcessInfo& info, e_processType type) override;

    void setClipRange(double near_plane, double far_plane);
    /* The reader for the pair setClipRange writes. Both planes come back
       through out-parameters, which is why the caller at 0x0046E440 keeps two
       doubles on its frame and returns only the far one. */
    void getClipRange(double& near_plane, double& far_plane) const;
    double getHorizontalFOV() const;
    double getVerticalFOV() const;
    e_projectionResult project(
        srVector3T<float>& output,
        const srVector3T<double>& input);
    void setProjectionType(e_project projection);
    void setViewPlane(const Rect& rectangle, double distance);
    void setViewPlane(double width, double height);
    void getViewPlane(Rect& rectangle, double& distance) const;
    void setEnvironmentRange(float near_range, float far_range);

private:
    unsigned char unknown_138_[0x50];
};

static_assert((sizeof(srScene) == 0x190), "srScene_must_be_0x190");
static_assert(
    (sizeof(srScene::Statistics) == 0x18),
    "srScene_Statistics_must_be_0x18");
static_assert((sizeof(srCamera) == 0x188), "srCamera_must_be_0x188");
