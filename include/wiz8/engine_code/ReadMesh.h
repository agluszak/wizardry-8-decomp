#pragma once

#include "surrender/srMath.h"
struct W8MaterialRecord004B8A70;

#include "wiz8/vector.h"

struct W8ReadLevelInfo;

#pragma pack(push, 1)
struct W8ReadMeshFace {
    int vertices[3];
    srVector2T<float> texture_coordinates[3];
    int material_index;
    unsigned char flags;
};
#pragma pack(pop)

static_assert(sizeof(W8ReadMeshFace) == 0x29, "W8ReadMeshFace_size_must_be_0x29");
class srMaterialIFace;
class srModelInstance;
class srTextureIFace;
class stMeshModel;
class srMeshModel;
class srClass;

bool IsTextureInReadMeshScratch(const srTextureIFace* texture);
unsigned char ReadSingleLevelMesh00485B20(W8ReadLevelInfo* info, srModelInstance** instance,
                                          int positional_0, int positional_1, const char* name,
                                          unsigned char load_materials);
unsigned char ReadMultipleLevelMeshes00488240(W8ReadLevelInfo* info, srModelInstance** instances,
                                              unsigned long count, const char* name);
unsigned char SkipSingleLevelMesh00487BD0(W8ReadLevelInfo* info);
void ReleaseReadMeshScratch004881D0();
void ReleaseRetainedMaterials00489920();
unsigned char IsReadMeshMaterial00489AC0(const srClass* material);
unsigned char ReadSingleLevelMeshBody00485C10(W8ReadLevelInfo* info, srModelInstance** instance,
                                              int positional_0, int positional_1, const char* name,
                                              unsigned char load_materials);
stMeshModel* BuildSingleLevelMesh00488650(int face_count, W8ReadMeshFace* faces, int vertex_count,
                                          int material_count, srMaterialIFace** materials,
                                          srTextureIFace** textures, unsigned long* render_flags,
                                          unsigned int* mesh_count, int*** vertex_maps,
                                          unsigned int* vertex_map_count,
                                          W8GrowableVector<short>* mapped_values,
                                          W8GrowableVector<short>* mapped_keys);

void OptimizeMeshOrder(srMeshModel* model, unsigned long flags);

void ClearMaterialRecordPadding(W8MaterialRecord004B8A70* material);
void ReadMeshTransform(int file, srVector3T<float>* location, srMatrix3T<float>* rotation,
                       srVector3T<float>* scale);
