#pragma once

#include "surrender/srColorSurface.h"
#include "surrender/srNode.h"
#include "surrender/srTexture.h"

class stTexture2D : public srTexture {
public:
    stTexture2D();
    virtual const char* getClassName() const;
    virtual unsigned long getClassID() const;
    virtual srRegistry::ClassNode* getClassNode() const;
    virtual ~stTexture2D();
    virtual srClass* vInstance();
    virtual srTextureIFace* clone();
    virtual unsigned long getTextureFrameHandle();
    virtual void getMipmapData(MultiRequest& request);
    virtual void getMipmapLevelPartial(PartialRequest& request);
    virtual void invalidate();
    virtual void update();

    int left;                              /* 0x54 */
    int top;                               /* 0x58 */
    int right;                             /* 0x5c */
    int bottom;                            /* 0x60 */
    unsigned long frame_handle;            /* 0x64 */
    srColorSurfaceIFace* surface;           /* 0x68 */
};

class stSurface2D : public srNode {
public:
    stSurface2D(srColorSurfaceIFace* surface, int width, int height,
                srNode* parent, int tile_size);
    virtual const char* getClassName() const;
    virtual unsigned long getClassID() const;
    virtual srRegistry::ClassNode* getClassNode() const;
    virtual ~stSurface2D();
    virtual srNode* clone();
    virtual void process(const ProcessInfo& info, e_processType type);

    void enableRendererFlag(unsigned int flag);
    void invalidateTiles();

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

typedef char stTexture2D_must_be_0x6c[(sizeof(stTexture2D) == 0x6c) ? 1 : -1];
typedef char stSurface2D_must_be_0x198[(sizeof(stSurface2D) == 0x198) ? 1 : -1];
