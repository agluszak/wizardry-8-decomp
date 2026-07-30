#pragma once

#include "srStringTable.h"
#include "srTexture.h"
#include "srTypeRegistry.h"
#include "srMath.h"
#include "srFlags.h"

class srDD;

class srRendererDefs {
public:
    enum e_primitive {};
    enum e_clip {};
};

class SR_DLL_IMPORT srGERD : public srRuntimeClass {
public:
    struct Pick {
        unsigned char unknown_00_[0x14];
    };

    enum e_error {};
    enum e_matrixMode {};
    enum e_antiAlias {};
    enum e_enable {};

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
    int isWindowOpen() const;
    void setGamma(const srVector3T<float>& gamma);
    e_error beginFrame();
    void endFrame();
    void flush();
    void flushRenderers();
    void matrixMode(e_matrixMode mode);
    void pushMatrix();
    void popMatrix();
    void loadIdentity();
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

    int hasPickState() const { return pick_state_19ec_ != 0; }

    void configure2DSurface(unsigned long state, unsigned long flags,
                            const float* texture_coordinates) {
        render_state_21c4_ = state;
        dirty_21c0_ |= 1;
        if (render_mode_1648_ != 2) {
            render_mode_1648_ = 2;
            dirty_24_ |= 0x2000;
        }
        if (surface_flags_1ff8_ != flags) {
            surface_flags_1ff8_ = flags;
            dirty_24_ |= 0x1000;
        }
        stream_21e0_ = 2;
        stream_21f8_ = 1;
        stream_2210_ = 8;
        texture_coordinates_2228_ = texture_coordinates;
        dirty_21c0_ |= 1;
    }

    void configure2DQuad(const float* vertices) {
        vertex_count_21c8_ = 4;
        primitive_21d0_ = 3;
        stream_21e8_ = 1;
        stream_2200_ = 0xc;
        vertices_2218_ = vertices;
        dirty_21c0_ |= 1;
    }

private:
    srGERD& operator=(const srGERD& other);

    unsigned char unknown_0c_[0x18];
    unsigned long dirty_24_;
    unsigned char unknown_28_[0x1620];
    long render_mode_1648_;
    unsigned char unknown_164c_[0x3a0];
    int pick_state_19ec_;                  /* 0x19ec */
    unsigned char unknown_19f0_[0x608];
    unsigned long surface_flags_1ff8_;
    unsigned char unknown_1ffc_[0x1c4];
    unsigned long dirty_21c0_;
    unsigned long render_state_21c4_;
    unsigned long vertex_count_21c8_;
    unsigned char unknown_21cc_[4];
    unsigned long primitive_21d0_;
    unsigned char unknown_21d4_[0xc];
    unsigned long stream_21e0_;
    unsigned char unknown_21e4_[4];
    unsigned long stream_21e8_;
    unsigned char unknown_21ec_[0xc];
    unsigned long stream_21f8_;
    unsigned char unknown_21fc_[4];
    unsigned long stream_2200_;
    unsigned char unknown_2204_[0xc];
    unsigned long stream_2210_;
    unsigned char unknown_2214_[4];
    const float* vertices_2218_;
    unsigned char unknown_221c_[0xc];
    const float* texture_coordinates_2228_;
    unsigned char unknown_222c_[0xc];
};

static_assert(sizeof(srGERD) == 0x2238, "srGERD_must_be_0x2238");
