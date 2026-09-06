#pragma once

#include "srStringTable.h"
#include "srTexture.h"
#include "srTypeRegistry.h"
#include "srMath.h"
#include "srMeshModel.h"
#include "srFlags.h"

#include "srDD.h"
class srModelInstance;
class srVertexProcessor;
struct srVertexArray;

class srRendererDefs {
public:
    enum e_primitive {};
    enum e_clip {};
    enum e_type {
        TYPE_POSITIONAL_1 = 1
    };
    enum e_vertexArray {};
};

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
    enum e_matrixMode {
        MATRIX_MODE_POSITIONAL_0 = 0
    };
    enum e_antiAlias {};
    enum e_cullMode {
        CULL_MODE_POSITIONAL_0 = 0,
        CULL_MODE_POSITIONAL_1 = 1,
        CULL_MODE_POSITIONAL_2 = 2
    };
    enum e_enable {
        ENABLE_POSITIONAL_1 = 1
    };
    enum e_winding {
        WINDING_POSITIONAL_0 = 0
    };
    enum e_visibility {
        VISIBILITY_POSITIONAL_0 = 0
    };

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
    e_error createContext(unsigned long window);
    long getDisplayMode(unsigned long width, unsigned long height,
                        unsigned long depth) const;
    e_error openWindow();
    e_error openWindow(long mode);
    void closeWindow(e_closeHint hint);
    int isWindowOpen() const;
    void setGamma(const srVector3T<float>& gamma);
    e_error beginFrame();
    void endFrame();
    void flush();
    void flushRenderers();
    void clear(const srFlags<e_buffer>& buffers);
    long getHeight() const;
    long getWidth() const;
    void resetStatistics();
    void setClearColor(float red, float green, float blue, float alpha);
    void setScissor(unsigned long x, unsigned long y,
                    unsigned long width, unsigned long height);
    void setTextureReduction(long reduction);
    void setViewPort(unsigned long x, unsigned long y,
                     unsigned long width, unsigned long height);
    void matrixMode(e_matrixMode mode);
    void getMatrix(e_matrixMode mode, srMatrix4T<float>& matrix);
    void getEyeSpaceBounds(srVector3T<float>& center, float& radius,
                           const srVector3T<float>& object_center,
                           float object_radius);
    void getInverseModelViewMatrix(srMatrix4T<float>& matrix);
    void getClipPlanes(ClipPlanes& planes);
    void getProjectClipNearMatrix(srMatrix4T<float>& matrix);
    void getNormalMatrix(srMatrix4T<float>& matrix);
    srMatrix4T<float>::e_scaleType getModelViewScaleType();
    e_cullMode getCullMode() const;
    e_winding getWinding() const;
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
    /* The pipeline's single-stage mask branch inlines this exported getter. */
    long getMaxTextureStages() const { return max_texture_stages_78_; }
    void pushMatrix();
    void popMatrix();
    void pushEnable();
    void popEnable();
    void loadIdentity();
    void rotate(double angle, const srVector3T<float>& axis);
    void scale(double x, double y, double z);
    void translate(const srVector3T<float>& offset);
    e_visibility testBoundingSphere(
        const srVector3T<float>& center, float radius);
    e_visibility testBoundingBox(
        const srVector3T<float>& minimum, const srVector3T<float>& maximum);
    void setPickKey(unsigned long key);
    void ortho(double left, double right, double bottom, double top,
               double near_plane, double far_plane);
    void setClipState(srFlags<srRendererDefs::e_clip> state);
    void setAntiAlias(e_antiAlias mode);
    void setTexture(srTextureIFace* texture, unsigned long layer);
    void setTextureDefaultMagFilter(srTextureIFace::e_filter filter);
    void setTextureDefaultMinFilter(srTextureIFace::e_filter filter);
    void setTextureDefaultMipmap(srTextureIFace::e_mipmap mipmap);
    void setTextureSubImage(srTextureIFace* texture, long mipmap,
                            long x, long y, long width, long height);
    void drawArrays(srRendererDefs::e_primitive primitive, long first,
                    unsigned long count);
    void popPick(Pick& pick);
    void pushPick(const Pick& pick);
    void toggle(e_enable option);
    void invalidateResidentTextures();
    void invalidateTextureCache();
    unsigned long getTextureCacheSize() const;
    void setTextureCacheSize(unsigned long bytes);
    void setSwapInterval(unsigned long interval);

    /* These ordinary methods are header-visible in Wiz8 call sites even
       though SR.DLL also exports out-of-line copies. */
    int isPickStackEmpty() const { return pick_depth_19ec_ == 0; }

    int isEnabled(e_enable option) const {
        return (enable_flags_20_.value & (1UL << option)) != 0;
    }

    void setCullMode(e_cullMode mode) {
        if (cull_mode_1648_ != mode) {
            cull_mode_1648_ = mode;
            dirty_24_ |= 0x2000;
        }
    }

    void setShader(const srShader& shader) {
        if (shader_1ff8_.value != shader.value) {
            shader_1ff8_ = shader;
            dirty_24_ |= 0x1000;
        }
    }

    void setVertexArrayMask(srFlags<srRendererDefs::e_vertexArray> mask) {
        vertex_array_mask_21c4_ = mask;
        dirty_21c0_ |= 1;
    }

    void setTexCoordPointer(long components, srRendererDefs::e_type type,
                            unsigned long stride, const void* values,
                            unsigned long layer) {
        unsigned long index = layer + 4;
        array_components_21d0_[index] = components;
        array_types_21e8_[index] = type;
        array_strides_2200_[index] = stride;
        arrays_2218_[index] = values;
        dirty_21c0_ |= 1;
    }

    void setVertexPointer(long primitive, srRendererDefs::e_type type,
                          unsigned long stride, const void* values,
                          long count) {
        vertex_count_21c8_ = count < 0 ? 0 : count;
        array_components_21d0_[0] = primitive;
        array_types_21e8_[0] = type;
        array_strides_2200_[0] = stride;
        arrays_2218_[0] = values;
        dirty_21c0_ |= 1;
    }

private:
    srGERD& operator=(const srGERD& other);

    unsigned char unknown_0c_[0x14];
    srFlags<e_enable> enable_flags_20_;
    unsigned long dirty_24_;
    unsigned char unknown_28_[0x50];
    long max_texture_stages_78_;
    unsigned char unknown_7c_[0x15cc];
    e_cullMode cull_mode_1648_;
    unsigned char unknown_164c_[0x3a0];
    int pick_depth_19ec_;                  /* 0x19ec */
    unsigned char unknown_19f0_[0x608];
    srShader shader_1ff8_;
    unsigned char unknown_1ffc_[0x1c4];
    unsigned long dirty_21c0_;
    srFlags<srRendererDefs::e_vertexArray> vertex_array_mask_21c4_;
    unsigned long vertex_count_21c8_;
    unsigned char unknown_21cc_[4];
    long array_components_21d0_[6];
    srRendererDefs::e_type array_types_21e8_[6];
    unsigned long array_strides_2200_[6];
    const void* arrays_2218_[6];
    unsigned char unknown_2230_[8];
};

static_assert(sizeof(srGERD) == 0x2238, "srGERD_must_be_0x2238");
static_assert(sizeof(srGERD::ClipPlanes) == 0x208,
              "srGERD_ClipPlanes_must_be_0x208");
static_assert(sizeof(srGERD::Renderer::TriInput) == 0x28,
              "srGERD_Renderer_TriInput_must_be_0x28");
