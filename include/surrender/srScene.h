#pragma once

#include "srNode.h"

class srCamera;
class srGERD;
class srModeler;

class SR_DLL_IMPORT srScene : public srClassSupport<srScene, srNode, 0, 0x1010> {
public:
    enum e_enable { ENABLE_POSITIONAL_0 = 0 };

    struct Statistics {
        double elapsed_00;              /* seconds since the last reset */
        unsigned long render_calls_08;  /* scene renders accumulated */
        unsigned long node_calls_0c;    /* node visits accumulated per render */
        unsigned long process_calls_10; /* per-node process calls accumulated */
        unsigned long value_14;
    };

    srScene(srNode* parent = 0);
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
#if defined(SURRENDER_BUILD)
    /* Retail exports out-of-line copies (0x10056C90) even though consumer
       overlay builders expand the load inline. */
    srVector3T<float> getFogColor() const;
#else
    srVector3T<float> getFogColor() const
    {
        return fog_color_180;
    }
#endif
    void getStatistics(Statistics& statistics);
    int isEnabled(e_enable option) const;
    void render(srGERD& renderer, class srCamera* camera);
    void resetStatistics();
#if defined(SURRENDER_BUILD)
    static const char* sGetClassName();
#else
    static const char* sGetClassName()
    {
        return "srScene";
    }
#endif
    /* The overlay builders expand these component stores at every call site.
       They are the ordinary header-visible SurRender setters, not a Wizardry
       aggregate helper around the scene object. The provider still exports
       its own out-of-line copies from scene.cpp. */
#if defined(SURRENDER_BUILD)
    void setAmbientLight(float red, float green, float blue);
#else
    inline void setAmbientLight(float red, float green, float blue)
    {
        ambient_light_174.x = red;
        ambient_light_174.y = green;
        ambient_light_174.z = blue;
    }
#endif
    void setAmbientLight(const srVector3T<float>& color);
#if defined(SURRENDER_BUILD)
    void setFogColor(float red, float green, float blue);
#else
    inline void setFogColor(float red, float green, float blue)
    {
        fog_color_180.x = red;
        fog_color_180.y = green;
        fog_color_180.z = blue;
    }
#endif
    void setFogColor(const srVector3T<float>& color);

protected:
    srFlags<e_enable> enabled_138;       /* 0x138 */
    Statistics statistics_140;           /* 0x140 */
    TraverseInfo traversal_158;          /* 0x158 (renderer at 0x170) */
    srVector3T<float> ambient_light_174; /* 0x174 */
    srVector3T<float> fog_color_180;     /* 0x180 */
    unsigned long unknown_18c_;          /* 0x18c */
};

static_assert((sizeof(srScene) == 0x190), "srScene_must_be_0x190");
static_assert((sizeof(srScene::Statistics) == 0x18), "srScene_Statistics_must_be_0x18");
