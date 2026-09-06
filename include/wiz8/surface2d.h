#pragma once

#include "surrender/srColorSurface.h"
#include "surrender/srNode.h"
#include "surrender/srTexture.h"

// VTABLE: WIZ8 0x005EC6C0 stTexture2D
// VTABLE: WIZ8 0x005EC704 srClassSupport<stTexture2D, srTexture, 0, 65551>
class stTexture2D
    : public srClassSupport<stTexture2D, srTexture, false, 0x1000f> {
public:
    static const char* sGetClassName() { return "stTexture2D"; }

    stTexture2D();
    virtual ~stTexture2D() override;
    virtual srClass* vInstance() override;
    virtual unsigned long getTextureFrameHandle() override;
    virtual void getMipmapData(MultiRequest& request) override;
    virtual void getMipmapLevelPartial(PartialRequest& request) override;
    virtual void invalidate() override;

protected:
    virtual void setupDefaultValues() override;

public:

    int left;                              /* 0x54 */
    int top;                               /* 0x58 */
    int right;                             /* 0x5c */
    int bottom;                            /* 0x60 */
    unsigned long frame_handle;            /* 0x64 */
    srColorSurfaceIFace* surface;           /* 0x68 */
};

// VTABLE: WIZ8 0x005EC748 stSurface2D
// VTABLE: WIZ8 0x005EC77C srClassSupport<stSurface2D, srNode, 0, 65550>
class stSurface2D
    : public srClassSupport<stSurface2D, srNode, false, 0x1000e> {
public:
    static const char* sGetClassName() { return "stSurface2D"; }

    stSurface2D(srColorSurfaceIFace* surface, int width, int height,
                srNode* parent, int tile_size);
    virtual ~stSurface2D() override;
    virtual void traverse(TraverseInfo& info) override;
    virtual void process(const ProcessInfo& info, e_processType type) override;

    void enableRendererFlag(unsigned int flag);
    void invalidateTiles();
    void updateRectangle(srGERD* renderer, void* pixels, long pitch,
                         int left, int top, int right, int bottom);

    srColorSurfaceIFace* source_surface;   /* 0x138 */
    int state;                             /* 0x13c */
    unsigned int flags;                    /* 0x140 */
    int tile_size;                         /* 0x144 */
    int columns;                           /* 0x148 */
    int rows;                              /* 0x14c */
    int tile_count;                        /* 0x150 */
    int width;                             /* 0x154 */
    int height;                            /* 0x158 */
    stTexture2D** tiles;                   /* 0x15c */
    float tile_u;                          /* 0x160 */
    float tile_v;                          /* 0x164 */
    int field_168;
    float field_16c;
    float coordinates[8];                  /* 0x170 */
    float scale;                           /* 0x190 */
    int field_194;
};

static_assert((sizeof(stTexture2D) == 0x6c), "stTexture2D_must_be_0x6c");
static_assert((sizeof(stSurface2D) == 0x198), "stSurface2D_must_be_0x198");
