#pragma once

#include "srStringTable.h"
#include "srTexture.h"
#include "srTypeRegistry.h"
#include "srVertexPipe.h"
#include "srMath.h"
#include "srShader.h"
#include "srFlags.h"

#include "srRendererDefs.h"
#include "srDD.h"
class srCriticalSection;
class srDebugDD;
class srModelInstance;
class srVertexProcessor;
struct srVertexArray;

class SR_DLL_IMPORT srGERD : public srRuntimeClass {
public:
    struct Pick {
        float value_00;
        float value_04;
        float value_08;
        srModelInstance* selected_model_0c;
        unsigned long value_10;
    };

    struct ClipPlanes {
        srVector4T<float> planes_000[32];
        unsigned long mask_200;
        unsigned long value_204;
    };

    class Renderer {
    public:
        struct TriInput {
            unsigned long triangle_count_00;
            unsigned long record_count_04;
            unsigned long vertex_count_08;
            const unsigned long* indices_0c;
            const srVector3i* triangles_10;
            const unsigned long* vertices_14;
            const void* passes_18;
            int position_is_float3_1c;
            const srMatrix4T<float>* project_clip_near_20;
            unsigned long value_24;
        };

        void allocVertexArray(srVertexArray& arrays, unsigned long count);
        void render(const TriInput& input);
    };

    enum e_error {};
    enum e_closeHint {};
    enum e_buffer {};
    /* Wizardry uses 0 immediately before model-view loads and 1 immediately
       before identity+ortho. OpenGL srDD talks GL_MODELVIEW (0x1700) and
       GL_PROJECTION (0x1701) for those two stacks. */
    enum e_matrixMode { MATRIX_MODELVIEW = 0, MATRIX_PROJECTION = 1 };
    enum e_antiAlias { ANTIALIAS_NONE = 0 };
    /* OpenGL: 0 disables GL_CULL_FACE, 1 enables + GL_BACK, 2 enables + GL_FRONT.
       DirectX7: D3DCULL_NONE / D3DCULL_CCW / D3DCULL_CW. */
    enum e_cullMode { CULL_NONE = 0, CULL_BACK = 1, CULL_FRONT = 2 };
    /* toggle XORs 1<<option into +0x20. Option 0 also dirties dirty_24 bit 0
       (Wizardry render-option 5). Option 1 is the particle path. Option 4 is
       SetRendererOption4Enabled. Option 5 wraps/unwraps srDebugDD. GERD dump
       has no enable-name table. */
    enum e_enable { ENABLE_POSITIONAL_0 = 0, ENABLE_POSITIONAL_1 = 1, ENABLE_POSITIONAL_4 = 4 };
    enum e_winding { WINDING_POSITIONAL_0 = 0, WINDING_POSITIONAL_1 = 1 };
    enum e_visibility { VISIBILITY_POSITIONAL_0 = 0 };

    static const char* sGetClassName();
    static srRegistry::ClassNode* sGetClassNode();

    srGERD(srDD* device, void* module, const char* device_name);
    srGERD(const srGERD& other);
    virtual ~srGERD() override;

    virtual const char* getClassName() const override;
    virtual unsigned long getClassID() const override;
    virtual srRegistry::ClassNode* getClassNode() const override;
    virtual void dump(std::ostream& stream) override;

    static srGERD* loadDevice(srStringTable& devices, unsigned long flags);
    static srGERD* getFirst();
    srGERD* getNext() const;
    e_error createContext(unsigned long window);
    int isContextCreated() const;
    void deleteContext();
    long getDisplayMode(unsigned long width, unsigned long height, unsigned long depth) const;
    e_error openWindow();
    e_error openWindow(long mode);
    void closeWindow(e_closeHint hint);
    int isWindowOpen() const;
    unsigned long getWindowHandle() const;
    void setGamma(const srVector3T<float>& gamma);
    e_error beginFrame();
    void endFrame();
    void flush();
    void flushRenderers();
    void clear(const srFlags<e_buffer>& buffers);
    long getHeight() const;
    long getWidth() const;
    void resetStatistics();
    /* getStatistics buffer. The render probes return the double at +0x10
       through ftol; the 0x00427460 debug overlay prints the dword counters at
       +0x08/+0x0c (the TT pair), +0x20 (PO), +0x24 (VO), +0x34 (PI),
       +0x3c (VI), +0x4c (TC) and +0x68 (DD). */
    struct Statistics {
        unsigned char unknown_00[8];
        unsigned long value_08;
        unsigned long value_0c;
        double value_10;
        unsigned char unknown_18[8];
        unsigned long value_20;
        unsigned long value_24;
        unsigned char unknown_28[0xc];
        unsigned long value_34;
        unsigned char unknown_38[4];
        unsigned long value_3c;
        unsigned char unknown_40[0xc];
        unsigned long value_4c;
        unsigned char unknown_50[0x18];
        unsigned long value_68;
        unsigned char unknown_6c[0x10];
    };
    void getStatistics(Statistics& statistics);
    unsigned long getTextureCacheUsed() const;
    unsigned long getResidentTextureMemUsed() const;
    void setClearColor(const srVector4T<float>& color);
    void setClearColor(float red, float green, float blue, float alpha);
    void setClearDepth(double depth);
    void setAmbientLight(float red, float green, float blue, float alpha);
    void setAmbientLight(const srVector4T<float>& light);
    void setFogColor(const srVector3T<float>& color);
    void setScissor(unsigned long x, unsigned long y, unsigned long width, unsigned long height);
    void flipFrame();
    void setTextureReduction(long reduction);
    void setViewPort(unsigned long x, unsigned long y, unsigned long width, unsigned long height);
    void matrixMode(e_matrixMode mode);
    void getMatrix(e_matrixMode mode, srMatrix4T<float>& matrix);
    void getEyeSpaceBounds(srVector3T<float>& center, float& radius,
                           const srVector3T<float>& object_center, float object_radius);
    void getInverseModelViewMatrix(srMatrix4T<float>& matrix);
    void getClipPlanes(ClipPlanes& planes);
    void getProjectClipNearMatrix(srMatrix4T<float>& matrix);
    void getNormalMatrix(srMatrix4T<float>& matrix);
    srMatrix4T<float>::e_scaleType getModelViewScaleType();
    e_cullMode getCullMode() const;
    e_winding getWinding() const;
    void setWinding(e_winding winding);
    Renderer* lockRenderer();
    void unlockRenderer(Renderer* renderer, int submit);
    srColorSurfaceIFace* lockBuffer();
    void unlockBuffer();
    unsigned long getVertexProcessorCount() const;
    void getVertexProcessors(srVertexProcessor** processors) const;
    void getAmbientLight(srVector4T<float>& light);
    void getEnvironmentRange(float& minimum, float& maximum) const;
    void getEnvironmentScaleFactor(float& scale, float& inverse_scale);
    unsigned long getExclusionMask() const;
    void setExclusionMask(unsigned long mask);
    /* The pipeline's single-stage mask branch inlines this exported getter. */
    long getMaxTextureStages() const
    {
        return max_texture_stages_78_;
    }
    void pushMatrix();
    void pushMultMatrix(const srMatrix4x3T<float>& matrix);
    void popMatrix();
    void pushEnable();
    void popEnable();
    void loadIdentity();
    void rotate(double angle, const srVector3T<float>& axis);
    void scale(double x, double y, double z);
    void translate(const srVector3T<float>& offset);
    void translate(double x, double y, double z);
    e_visibility testBoundingSphere(const srVector3T<float>& center, float radius);
    e_visibility testBoundingBox(const srVector3T<float>& minimum,
                                 const srVector3T<float>& maximum);
    void setPickKey(unsigned long key);
    void ortho(double left, double right, double bottom, double top, double near_plane,
               double far_plane);
    void setClipState(srFlags<srRendererDefs::e_clip> state);
    void setAntiAlias(e_antiAlias mode);
    void setTexture(srTextureIFace* texture, unsigned long layer);
    void setTextureDefaultMagFilter(srTextureIFace::e_filter filter);
    void setTextureDefaultMinFilter(srTextureIFace::e_filter filter);
    void setTextureDefaultMipmap(srTextureIFace::e_mipmap mipmap);
    void setTextureSubImage(srTextureIFace* texture, long mipmap, long x, long y, long width,
                            long height);
    void drawArrays(srRendererDefs::e_primitive primitive, long first, unsigned long count);
    void popPick(Pick& pick);
    void pushPick(const Pick& pick);
    void toggle(e_enable option);
    void invalidateResidentTextures();
    void invalidateTextureCache();
    unsigned long getTextureCacheSize() const;
    void setTextureCacheSize(unsigned long bytes);
    void setSwapInterval(unsigned long interval);
    long getPolygonOffset() const;
    void setPolygonOffset(long offset);

    /* These ordinary methods are header-visible in Wiz8 call sites even
       though SR.DLL also exports out-of-line copies. */
    int isPickStackEmpty() const
    {
        return pick_depth_19ec_ == 0;
    }

    int isEnabled(e_enable option) const
    {
        return (enable_flags_20_.value & (1UL << option)) != 0;
    }

    void setCullMode(e_cullMode mode)
    {
        if (cull_mode_1648_ != mode) {
            cull_mode_1648_ = mode;
            dirty_24_ |= 0x2000;
        }
    }

    void setShader(const srShader& shader)
    {
        if (shader_1ff8_.value != shader.value) {
            shader_1ff8_ = shader;
            dirty_24_ |= 0x1000;
        }
    }

    void setVertexArrayMask(srFlags<srRendererDefs::e_vertexArray> mask)
    {
        vertex_array_mask_21c4_ = mask;
        dirty_21c0_ |= 1;
    }

    void setTexCoordPointer(long components, srRendererDefs::e_type type, unsigned long stride,
                            const void* values, unsigned long layer)
    {
        unsigned long index = layer + 4;
        array_components_21d0_[index] = components;
        array_types_21e8_[index] = type;
        array_strides_2200_[index] = stride;
        arrays_2218_[index] = values;
        dirty_21c0_ |= 1;
    }

    void setVertexPointer(long primitive, srRendererDefs::e_type type, unsigned long stride,
                          const void* values, long count)
    {
        vertex_count_21c8_ = count < 0 ? 0 : count;
        array_components_21d0_[0] = primitive;
        array_types_21e8_[0] = type;
        array_strides_2200_[0] = stride;
        arrays_2218_[0] = values;
        dirty_21c0_ |= 1;
    }

private:
    srGERD& operator=(const srGERD& other);

    srDD* getDD();
    void setError(e_error error);
    void resetTexture();

    static srGERD* first;

    unsigned char unknown_0c_[8];
    srCriticalSection* renderers_section_14_;
    srCriticalSection* state_section_18_;
    unsigned char unknown_1c_[4];
    srFlags<e_enable> enable_flags_20_;
    unsigned long dirty_24_;
    unsigned long state_flags_28_;
    e_error last_error_2c_;
    unsigned char unknown_30_[4];
    srGERD* next_34_;
    unsigned char unknown_38_[8];
    srDD* dd_40_;
    srDebugDD* debug_dd_44_;
    srDD* real_dd_48_;
    unsigned char unknown_4c_[0x2c];
    long max_texture_stages_78_;
    unsigned char unknown_7c_[0x2e4];
    void* texture_formats_360_;
    long texture_format_count_364_;
    unsigned long* display_modes_368_;
    long display_mode_count_36c_;
    unsigned char unknown_370_[4];
    unsigned long window_374_;
    unsigned char unknown_378_[8];
    long width_380_;
    long height_384_;
    unsigned char unknown_388_[0x12b0];
    unsigned long view_left_1638_;
    unsigned long view_top_163c_;
    unsigned long view_right_1640_;
    unsigned long view_bottom_1644_;
    e_cullMode cull_mode_1648_;
    e_winding winding_164c_;
    unsigned char unknown_1650_[0x108];
    srVector3T<float> gamma_1758_;
    unsigned long swap_interval_1764_;
    e_antiAlias antialias_1768_;
    Pick pick_stack_176c_[32];
    unsigned long pick_depth_19ec_; /* 0x19ec */
    unsigned long pick_key_19f0_;
    unsigned char unknown_19f4_[0x84];
    Statistics statistics_1a78_;
    unsigned char unknown_1af8_[0x10];
    srVector4T<float> clear_color_1b08_;
    unsigned char unknown_1b18_[0x10];
    double clear_depth_1b28_;
    unsigned char unknown_1b30_[0x43c];
    unsigned long mag_filter_map_1f6c_[4];
    unsigned long mag_filter_param_1f7c_;
    unsigned long min_filter_map_1f80_[4];
    unsigned long min_filter_param_1f90_;
    unsigned long mipmap_map_1f94_[3];
    unsigned long mipmap_param_1fa0_;
    unsigned char unknown_1fa4_[0x14];
    srTextureIFace::e_filter default_mag_filter_1fb8_;
    srTextureIFace::e_filter default_min_filter_1fbc_;
    srTextureIFace::e_mipmap default_mipmap_1fc0_;
    unsigned char unknown_1fc4_[0x20];
    long polygon_offset_1fe4_;
    unsigned char unknown_1fe8_[0x10];
    srShader shader_1ff8_;
    unsigned char unknown_1ffc_[8];
    unsigned char unknown_2004_[0x30];
    unsigned long texture_cache_used_2034_;
    unsigned long texture_cache_size_2038_;
    unsigned char unknown_203c_[4];
    long texture_reduction_2040_;
    unsigned char unknown_2044_[4];
    srVector4T<float> ambient_light_2048_;
    float environment_min_2058_;
    float environment_max_205c_;
    float environment_scale_2060_;
    float environment_inv_scale_2064_;
    unsigned char unknown_2068_[0x104];
    unsigned long enable_stack_216c_[16];
    unsigned long enable_depth_21ac_;
    srVertexProcessor** vertex_processors_21b0_;
    unsigned char unknown_21b4_[4];
    unsigned long vertex_processor_count_21b8_;
    unsigned long exclusion_mask_21bc_;
    unsigned long dirty_21c0_;
    srFlags<srRendererDefs::e_vertexArray> vertex_array_mask_21c4_;
    unsigned long vertex_count_21c8_;
    srFlags<srRendererDefs::e_clip> clip_state_21cc_;
    long array_components_21d0_[6];
    srRendererDefs::e_type array_types_21e8_[6];
    unsigned long array_strides_2200_[6];
    const void* arrays_2218_[6];
    unsigned char unknown_2230_[8];
};

static_assert(sizeof(srGERD) == 0x2238, "srGERD_must_be_0x2238");
static_assert(sizeof(srGERD::ClipPlanes) == 0x208, "srGERD_ClipPlanes_must_be_0x208");
static_assert(sizeof(srGERD::Renderer::TriInput) == 0x28, "srGERD_Renderer_TriInput_must_be_0x28");
