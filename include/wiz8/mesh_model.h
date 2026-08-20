#pragma once

#include "surrender/srMeshModel.h"
#include "wiz8/vector.h"
#include "surrender/srTypeRegistry.h"

/* Engine Code\stMeshModel.cpp. Only fields reached by reviewed bodies are
   modeled. The two short-vector pairs are parallel key/value tables; their
   semantic domain is not established, so the names stay positional. */
class stMeshModel : public srMeshModel {
public:
    stMeshModel(long polygons, long vertices);    /* 0x00470B00 */
    const char* getClassName() const override;     /* 0x004741F0 */
    unsigned long getClassID() const override;     /* 0x004741E0 */
    class srRegistry::ClassNode* getClassNode() const override;  /* 0x00474820 */

    int FindMappedIndex(short key);       /* 0x004712D0 */
    void LinkTo(stMeshModel* other);      /* 0x00471D60 */
    void* GetVertex(unsigned int index);  /* 0x00471AA0 */
    int FindSkinTable004736D0(const char* name);
    int CreateSkinTable00473260(const char* name, int base_table);
    srPtr<srTextureIFace>* GetTextureTable00473720(int table); /* 0x00473720 */
    void RemoveSkinTable00473830(int index);
    void RemoveSkinTablesForCycle00473780(const char* cycle_name);
    srVector3T<float>* GetVertexLocations00471AD0(
        unsigned int frame, char load, float interpolation);
    srVector3T<float>* GetVertexNormals00471CA0(
        unsigned int frame, char load);
    void* GetFrameValues00471D00(unsigned int frame, char load);
    void SetAmbientColor00472990(const srVector3T<float>& color);
    unsigned long* GetPolygonTable00473CD0(
        int* count, int texture_table, char create);
    void RenderTriMesh00470380(
        class srGERD* renderer, srMeshModel::TriMesh& mesh,
        void* frame_values);
    unsigned char PrepareFrame00471720(unsigned int frame, int channels);
    unsigned char FinalizeFrame00471930(
        unsigned int frame, int channels, void* values);
    unsigned long ReleaseFrames004739E0();
    int InitializeVertexWeights004721E0(char initialize);

    stMeshModel* next;                    /* 0x398 */
    stMeshModel* previous;                /* 0x39c */
    unsigned int flags_3a0;
    srVector3T<float> ambient_color_3a4;
    unsigned char unknown_3b0[0x1c];
    unsigned char flag_3cc;
    unsigned char flag_3cd;
    unsigned char unknown_3ce[2];
    unsigned int frame_count_3d0;
    srVector3T<float>** frame_locations_3d4;
    srVector3T<float>** frame_normals_3d8;
    srVector3T<float>** frame_values_3dc;
    short** compressed_locations_3e0;
    unsigned char** compressed_normals_3e4;
    unsigned char** compressed_values_3e8;
    unsigned char unknown_3ec[0x04];
    W8GrowableVector<int> skin_table_ids; /* 0x3f0; count at 0x3f4 */
    W8GrowableVector<srPtr<srTextureIFace>*> skin_texture_tables; /* 0x400 */
    W8GrowableVector<char*> skin_table_names; /* 0x410 */
    W8GrowableVector<short> mapped_values; /* 0x420 */
    W8GrowableVector<short> mapped_keys;   /* 0x430 */
    unsigned long last_frame_use_440;
    float compression_scale_444;
    srVector3T<float>* lerp_buffer_448;
    unsigned long* blanking_apt_44c;
    int blanking_apt_number_450;
    unsigned char blanking_checked_454;
    unsigned char padding_455[3];
    W8GrowableVector<unsigned long*>* skin_blanking_apt_458;
    W8GrowableVector<int>* skin_blanking_apt_number_45c;
    W8GrowableVector<unsigned char>* skin_blanking_checked_460;
};

static_assert(sizeof(stMeshModel) == 0x464,
              "stMeshModel_size_must_be_0x464");

int FindMappedIndexInMeshChain(
    stMeshModel** mesh, int key);          /* 0x004A8D10 */
