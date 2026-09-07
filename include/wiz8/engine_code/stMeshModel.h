#pragma once

#include "surrender/srMeshModel.h"
#include "wiz8/vector.h"
#include "surrender/srTypeRegistry.h"

/* Engine Code\stMeshModel.cpp. Only fields reached by reviewed bodies are
   modeled. The two short-vector pairs are parallel key/value tables; their
   semantic domain is not established, so the names stay positional. */
// VTABLE: WIZ8 0x005ec454 stMeshModel
// VTABLE: WIZ8 0x005ec4b0 srClassSupport<stMeshModel, srMeshModel, 0, 65539>
class stMeshModel
    : public srClassSupport<stMeshModel, srMeshModel, false, 0x10003> {
public:
    static const char* sGetClassName() { return "stMeshModel"; }

    stMeshModel(long polygons, long vertices);    /* 0x00470B00 */

    int FindMappedIndex(short key);       /* 0x004712D0 */
    void SetMappedVertex00471160(short vertex, short key);
    void LinkTo(stMeshModel* other);      /* 0x00471D60 */
    void* GetVertex(unsigned int index);  /* 0x00471AA0 */
    int FindSkinTable004736D0(const char* name);
    int CreateSkinTable00473260(const char* name, int base_table);
    srPtr<srTextureIFace>* GetTextureTable00473720(int table); /* 0x00473720 */
    void RemoveSkinTable00473830(int index);
    void RemoveSkinTablesForCycle00473780(const char* cycle_name);
    srVector3T<float>* GetVertexLocations00471AD0(
        unsigned int frame, char load, float interpolation);
    srVector3T<float>* GetVertexDIG00472100(
        char initialize, int table);
    float* InitializeVertexWeights004721E0(char initialize);
    void Function5AA400(stMeshModel* linked_model);
    void InitializeVertexFrames00473B00(int frame_count);
    void FinalizeVertexFrame00473180(int frame);

    stMeshModel* next;                    /* 0x398 */
    stMeshModel* previous;                /* 0x39c */
    unsigned int flags_3a0;
    unsigned char unknown_3a4[0x28];
    unsigned char flag_3cc;
    unsigned char unknown_3cd[3];
    unsigned int vertex_count;            /* 0x3d0 */
    unsigned char unknown_3d4[0xc];
    void** vertices;                      /* 0x3e0 */
    unsigned char unknown_3e4[0xc];
    W8GrowableVector<int> skin_table_ids; /* 0x3f0; count at 0x3f4 */
    W8GrowableVector<srPtr<srTextureIFace>*> skin_texture_tables; /* 0x400 */
    W8GrowableVector<char*> skin_table_names; /* 0x410 */
    W8GrowableVector<short> mapped_values; /* 0x420 */
    W8GrowableVector<short> mapped_keys;   /* 0x430 */
    unsigned char unknown_440[4];
    float vertex_compression_scale_444;
    unsigned char unknown_448[0x10];
    W8GrowableVector<int*>* skin_blanking_apt_458;
    W8GrowableVector<int>* skin_blanking_apt_number_45c;
    W8GrowableVector<unsigned char>* skin_blanking_checked_460;
};

static_assert(sizeof(stMeshModel) == 0x464,
              "stMeshModel_size_must_be_0x464");

int FindMappedIndexInMeshChain(
    stMeshModel** mesh, int key);          /* 0x004A8D10 */
