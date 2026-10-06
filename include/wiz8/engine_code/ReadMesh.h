#pragma once

#include "surrender/srMath.h"
struct W8MaterialRecord;

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
class srShader;
class stMeshModel;
class srMeshModel;
class srClass;

bool IsTextureInReadMeshScratch(const srTextureIFace* texture);
unsigned char ReadSingleLevelMesh(W8ReadLevelInfo* info, srModelInstance** instance,
                                  int unused_first, int unused_second, const char* name,
                                  bool load_materials);
unsigned char ReadMultipleLevelMeshes(W8ReadLevelInfo* info, srModelInstance** instances,
                                      unsigned long count, const char* name);
unsigned char SkipSingleLevelMesh(W8ReadLevelInfo* info);
void ReleaseReadMeshScratch();
void ReleaseRetainedMaterials();
bool IsReadMeshMaterial(const srClass* material);
unsigned char ReadSingleLevelMeshBody(W8ReadLevelInfo* info, srModelInstance** instance, int, int,
                                      const char* name, bool load_materials);
stMeshModel*
BuildSingleLevelMesh(int face_count, W8ReadMeshFace* faces, int vertex_count, int material_count,
                     srMaterialIFace** materials, srTextureIFace** textures, srShader* render_flags,
                     unsigned int* mesh_count, int*** vertex_maps, unsigned int* vertex_map_count,
                     W8GrowableVector<short>* mapped_values, W8GrowableVector<short>* mapped_keys);

enum {
    W8_MESH_ORDER_POLYGONS = 1UL,
    W8_MESH_ORDER_VERTICES = 2UL,
    W8_MESH_ORDER_TRIANGLE_STRIPS = 4UL
};

void OptimizeMeshOrder(srMeshModel* model, unsigned long flags);

void ClearMaterialRecordPadding(W8MaterialRecord* material);
void ReadMeshTransform(int file, srVector3T<float>* location, srMatrix3T<float>* rotation,
                       srVector3T<float>* scale);
