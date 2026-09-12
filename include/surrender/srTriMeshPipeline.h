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
        inline Record() : flags_00(0), disable_mask_04(0), colors_0c(0), color_format_10(2) {}

        unsigned long flags_00;
        unsigned long disable_mask_04;
        srMaterialIFace* material_08;
        /* Bit 0: DIG or particle colors (+0x0c) with format at +0x10.
           Bit 1: DCG at +0x14. Bit 2: SCG at +0x18. */
        void* colors_0c;
        unsigned long color_format_10;
        srVector4T<float>* dcg_14;
        srVector4T<float>* scg_18;
        /* Optional per-vertex arrays, each gated by its own flags_00 bit:
           0x004994D0 sets +0x1c under bit 3 and +0x20 under bit 4. */
        float* alphas_1c;
        srVector2T<float>* st0_20;
        srVector2T<float>* st1_24;
        srPtr<srMaterialIFace>* vertex_materials_28;
        unsigned char unknown_2c_[0x30];
    };

    struct Pass {
        inline Pass()
        {
            flags_08.value = 0x0100241b; /* default packed srShader */
        }

        srTextureIFace* texture_00;
        unsigned long pass_value_04;
        srShader flags_08;
        void* texture_array_0c;
        unsigned long value_10;
        const srShader* shader_14;
        srVector2T<float>* st_18;
        unsigned long value_1c;
    };

    static_assert(sizeof(Record) == 0x5c, "srTriMeshPipeline_Record_must_be_0x5c");
    static_assert(sizeof(Pass) == 0x20, "srTriMeshPipeline_Pass_must_be_0x20");

    static srTriMeshPipeline* Get004750A0(srGERD* renderer);
    /* By value, not by reference: 0x004994D0 reserves a four-byte argument
       slot and constructs the flag object straight into it. */
    void SetFlags004752C0(srShader shader);
    void Reset004753F0(srGERD* renderer);
    void Flush00475510();
    void PrepareSlot00475540();

    /* The guarded header-visible boundary expands at the stParticle call
       sites. Its original spelling is not present in the binary. */
    inline void FlushIfCurrent()
    {
        srTriMeshPipeline* current = pipe;

        if (this == current) {
            current->flushing_8c = 1;
            if (current->slot_count_84 > 0) {
                current->FlushSlots00475600();
            }
            current->flushing_8c = 0;
        }
    }

    /* Slot 0 of vtable 0x005ec520. */
    virtual void FlushSlots00475600();
    /* Slot 1 / complete destructor at 0x004752F0. */
    virtual ~srTriMeshPipeline();

    srHeapArray<srVertexProcessor*> vertex_processors_04;
    srHeapArray<unsigned long> culler_scratch_0c;
    Record* current_record_14;
    Pass* current_pass_18;
    unsigned long triangle_count_1c;
    unsigned long vertex_count_20;
    unsigned long active_triangle_count_24;
    /* Bit 0: run getClipMask (frustum 0x3f plus user planes in bits 6+).
       Bit 1: vertex/triangle batch-limit path. Reset/Get always set both. */
    unsigned long flags_28;
    const unsigned long* active_triangles_2c;
    const srVector4T<float>* projected_vertices_30;
    const srVector3i* triangles_34;
    /* stParticle stores allocation_160 (vec3*) here; Reset/Get null it.
       FlushSlots00475600 then CALLINDs vp+0x18c (_minMax vec4) with this
       pointer and the packed vec3 min/max at +0x44/+0x50. Stores and the
       xyz-only center math keep these as vec3; the vec4 slot is recorded,
       not a reason to widen the fields. */
    const srVector3T<float>* positions_38;
    const void* vertex_extras_3c;
    unsigned long extra_40;
    srVector3T<float> bounds_minimum_44;
    srVector3T<float> bounds_maximum_50;
    srVector3T<float> bounds_center_5c;
    float bounds_radius_68;
    unsigned long bounds_state_6c;
    unsigned long unknown_70;
    srShader shader_74;
    srTextureIFace* texture_78;
    unsigned long pass_value_7c;
    srMaterialIFace* material_80;
    unsigned long slot_count_84;
    srGERD* renderer_88;
    volatile unsigned long flushing_8c;
    srVertexPipe* vertex_pipe_90;
    srArray<Record> records_94;
    srArray<Pass> passes_9c;
    srArray<srVertexArray> vertex_arrays_a4;

protected:
    static SR_DLL_IMPORT srTriMeshPipeline* pipe;

private:
    srTriMeshPipeline();
};
#pragma pack(pop)

static_assert(sizeof(srTriMeshPipeline) == 0xac, "srTriMeshPipeline_must_be_0xac");
