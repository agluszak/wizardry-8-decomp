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

/* Shared lazy singleton behind the imported static
   srTriMeshPipeline::pipe (IAT 0x005eb7fc). Retail allocates one 0xac-byte
   instance from Wiz8.exe (0x004750A0), installs the local vtable at
   0x005ec520, and owns an srVertexPipe at +0x90. Method bodies live in
   Engine Code\stMeshModel.cpp next to that vtable's object.

   Retail's Get path installs the vtable after some subobject setup; the
   recovered model uses an ordinary C++ constructor (vtable first) so the
   source stays compiler-owned. Get's residual divergence is recorded on
   wiz8-et0o.2. */
#pragma pack(push, 4)
class srTriMeshPipeline {
public:
    struct Record {
        inline Record() : flags(0), disable_mask(0), colors(0), color_format(2) {}

        unsigned long flags;
        unsigned long disable_mask;
        srMaterialIFace* material;
        /* Bit 0: DIG or particle colors (+0x0c) with format at +0x10.
           Bit 1: DCG at +0x14. Bit 2: SCG at +0x18. */
        void* colors;
        unsigned long color_format;
        srVector4T<float>* dcg;
        srVector4T<float>* scg;
        /* Optional per-vertex arrays, each gated by its own flags bit:
           0x004994D0 sets +0x1c under bit 3 and +0x20 under bit 4. */
        float* alphas;
        srVector2T<float>* st0;
        srVector2T<float>* st1;
        srPtr<srMaterialIFace>* vertex_materials;
        unsigned char unknown_2c_[0x30];
    };

    struct Pass {
        inline Pass()
        {
            flags.value = 0x0100241b; /* default packed srShader */
        }

        srTextureIFace* texture0;
        srTextureIFace* texture;
        srShader flags;
        /* Per-stage per-vertex texture tables; the mesh fills both slots of
           the {0x0c,0x10} pair through (&tex_table_0)[layer]. Writers
           store srPtr<srTextureIFace> or stTextureAnim frame tables; the
           renderer only copies each dword entry into the texture-set key. */
        void* tex_table_0;
        void* tex_table_1;
        const srShader* shaders;
        srVector2T<float>* st;
        /* The mesh's per-triangle poly-UV corner source table. */
        const srVector3i* poly_uv;
    };

    static_assert(sizeof(Record) == 0x5c, "srTriMeshPipeline_Record_must_be_0x5c");
    static_assert(sizeof(Pass) == 0x20, "srTriMeshPipeline_Pass_must_be_0x20");

    static srTriMeshPipeline* Get(srGERD* renderer);
    /* By value, not by reference: 0x004994D0 reserves a four-byte argument
       slot and constructs the flag object straight into it. */
    void SetFlags(srShader shader);
    void Reset(srGERD* renderer);
    void Flush();
    void PrepareSlot();

    /* The guarded header-visible boundary expands at the stParticle call
       sites. Its original spelling is not present in the binary. */
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

    /* Slot 0 of vtable 0x005ec520. */
    virtual void FlushSlots();
    /* Slot 1 / complete destructor at 0x004752F0. */
    virtual ~srTriMeshPipeline();

    srHeapBuffer<srVertexProcessor*> vertex_processors;
    srHeapBuffer<unsigned long> culler_scratch;
    Record* current_record;
    Pass* current_pass;
    unsigned long triangle_count;
    unsigned long vertex_count;
    unsigned long active_triangle_count;
    /* Bit 0: run getClipMask (frustum 0x3f plus user planes in bits 6+).
       Bit 1: vertex/triangle batch-limit path. Reset/Get always set both. */
    unsigned long flags;
    const unsigned long* active_triangles;
    const srVector4T<float>* projected_vertices;
    const srVector3i* triangles;
    /* stParticle stores vertex_positions (vec3*) here; Reset/Get null it.
       FlushSlots then CALLINDs vp+0x18c (_minMax vec4) with this
       pointer and the packed vec3 min/max at +0x44/+0x50. Stores and the
       xyz-only center math keep these as vec3; the vec4 slot is recorded,
       not a reason to widen the fields. */
    const srVector3T<float>* positions;
    const srVector3T<float>* vertex_extras;
    float sort_bias;
    srVector3T<float> bounds_minimum;
    srVector3T<float> bounds_maximum;
    srVector3T<float> bounds_center;
    float bounds_radius;
    unsigned long bounds_state;
    unsigned long unknown_70;
    srShader shader;
    srTextureIFace* texture0;
    srTextureIFace* texture1;
    srMaterialIFace* material;
    unsigned long slot_count;
    srGERD* renderer;
    volatile unsigned long flushing;
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

static_assert(sizeof(srTriMeshPipeline) == 0xac, "srTriMeshPipeline_must_be_0xac");
