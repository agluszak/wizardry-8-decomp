#pragma once

#include "srArray.h"
#include "srMaterialIFace.h"
#include "srMath.h"
#include "srPtr.h"
#include "srShader.h"
#include "srTextureIFace.h"
#include "srVertexPipe.h"

class srGERD;
class srMaterialIFace;

/* Shared lazy singleton behind srTriMeshPipeline::pipe. Wizardry implements it (Engine
   Code\stMeshModel.cpp) and owns an srVertexPipe in it. */
#pragma pack(push, 4)
/* The EXE and DLL each implement the methods/vtable, but import one pipe
   static from the DLL. Native clients must bind their recovered game methods
   locally instead of interposing on the renderer's implementation. */
class srTriMeshPipeline {
public:
    enum { FRUSTUM_CLIPPING = 1u, LIMIT_VERTEX_BATCHES = 2u };
    struct Record {
        inline Record()
            : flags(0), disable_mask(0), colors(0),
              color_format(srVertexPipe::Record::ColorSource::FORMAT_VECTOR4)
        {
        }

        w8_ulong flags;
        w8_ulong disable_mask;
        srMaterialIFace* material;
        /* Bit 0: DIG or particle colors with format. Bit 1: DCG. Bit 2: SCG. */
        void* colors;
        srVertexPipe::Record::ColorSource::e_format color_format;
        srVector4T<float>* dcg;
        srVector4T<float>* scg;
        /* Optional per-vertex arrays, each gated by its own flags bit. */
        float* alphas;
        srVector2T<float>* st0;
        srVector2T<float>* st1;
        srPtr<srMaterialIFace>* vertex_materials;
        unsigned char unknown_2c_[0x30];
    };

    struct Pass {
        inline Pass()
        {
            shader.value = 0x0100241b; /* default packed srShader */
        }

        srTextureIFace* textures[2];
        srShader shader;
        /* Borrowed per-stage srPtr<srTextureIFace> or stTextureAnim* tables.
           The renderer reads pointer-sized entries without owning the tables. */
        void* texture_tables[2];
        const srShader* shaders;
        srVector2T<float>* texcoords;
        /* The mesh's per-triangle poly-UV corner source table. */
        const srVector3i* poly_uv;
    };

    W8_ABI_ASSERT(sizeof(Record) == 0x5c, "srTriMeshPipeline_Record_must_be_0x5c");
    W8_ABI_ASSERT(sizeof(Pass) == 0x20, "srTriMeshPipeline_Pass_must_be_0x20");

    static srTriMeshPipeline* Get(srGERD* renderer);
    void SetFlags(srShader shader);
    void Reset(srGERD* renderer);
    void Flush();
    void PrepareSlot();

    inline void FlushIfCurrent()
    {
        srTriMeshPipeline* current = pipe;

        if (this == current) {
            current->flushing = 1;
            if (current->slot_count > 0) {
                current->FlushSlots();
            }
            current->flushing = 0;
        }
    }

    virtual void FlushSlots();
    virtual ~srTriMeshPipeline();

    srHeapBuffer<srVertexProcessor*> vertex_processors;
    srHeapBuffer<w8_ulong> culler_scratch;
    Record* current_record;
    Pass* current_pass;
    w8_ulong triangle_count;
    w8_ulong vertex_count;
    w8_ulong active_triangle_count;
    /* Bit 0: run getClipMask (frustum 0x3f plus user planes in bits 6+).
       Bit 1: vertex/triangle batch-limit path. Reset/Get always set both. */
    w8_ulong flags;
    const w8_ulong* active_triangles;
    const srVector4T<float>* projected_vertices;
    const srVector3i* triangles;
    /* stParticle stores its vertex positions here; Reset/Get null it. */
    const srVector3T<float>* positions;
    const srVector3T<float>* vertex_extras;
    float sort_bias;
    srVector3T<float> bounds_minimum;
    srVector3T<float> bounds_maximum;
    srVector3T<float> bounds_center;
    float bounds_radius;
    enum e_boundsSource { BOUNDS_FROM_VERTICES = 0, BOUNDS_SPHERE = 1, BOUNDS_BOX = 2 };
    e_boundsSource bounds_source;
    w8_ulong unknown_70;
    srShader shader;
    srTextureIFace* texture0;
    srTextureIFace* texture1;
    srMaterialIFace* material;
    w8_ulong slot_count;
    srGERD* renderer;
    volatile w8_ulong flushing;
    srVertexPipe* vertex_pipe;
    srArray<Record> records;
    srArray<Pass> passes;
    srArray<srVertexArray> vertex_arrays;

protected:
    /* srExit releases the singleton through this protected static. */
    friend SR_DLL_IMPORT int __cdecl srExit(void);

    static SR_DLL_IMPORT srTriMeshPipeline* pipe;

private:
    srTriMeshPipeline();
};
#pragma pack(pop)

W8_ABI_ASSERT(sizeof(srTriMeshPipeline) == 0xac, "srTriMeshPipeline_must_be_0xac");
