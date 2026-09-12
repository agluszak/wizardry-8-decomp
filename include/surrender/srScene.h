#pragma once

#include "srNode.h"

class srCamera;
class srGERD;
class srModeler;

class SR_DLL_IMPORT srScene : public srClassSupport<srScene, srNode, 0, 0x1010> {
public:
    enum e_enable { ENABLE_POSITIONAL_0 = 0 };

    struct Statistics {
        double value_00;
        unsigned long value_08;
        unsigned long value_0c;
        unsigned long value_10;
        unsigned long value_14;
    };

    srScene(srNode* parent);
    srScene(const srScene& other);
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
    static const char* sGetClassName()
    {
        return "srScene";
    }
    /* The overlay builders expand these component stores at every call site.
       They are the ordinary header-visible SurRender setters, not a Wizardry
       aggregate helper around the scene object. */
    inline void setAmbientLight(float red, float green, float blue)
    {
        ambient_light_174.x = red;
        ambient_light_174.y = green;
        ambient_light_174.z = blue;
    }
    void setAmbientLight(const srVector3T<float>& color);
    inline void setFogColor(float red, float green, float blue)
    {
        fog_color_180.x = red;
        fog_color_180.y = green;
        fog_color_180.z = blue;
    }
    void setFogColor(const srVector3T<float>& color);

protected:
    srFlags<e_enable> enabled_138;       /* 0x138 */
    Statistics statistics_140;           /* 0x140 */
    TraverseInfo traversal_158;          /* 0x158 */
    ProcessInfo process_info_170;        /* 0x170 */
    srVector3T<float> ambient_light_174; /* 0x174 */
    srVector3T<float> fog_color_180;     /* 0x180 */
    unsigned long unknown_18c_;          /* 0x18c */
};

static_assert((sizeof(srScene) == 0x190), "srScene_must_be_0x190");
static_assert((sizeof(srScene::Statistics) == 0x18), "srScene_Statistics_must_be_0x18");
