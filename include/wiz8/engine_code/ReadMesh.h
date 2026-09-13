#pragma once

#include "surrender/srMath.h"
struct W8MaterialRecord004B8A70;

#include "wiz8/vector.h"

struct W8ReadLevelInfo;
struct W8ReadMeshFace;
class srMaterialIFace;
class srModelInstance;
class srTextureIFace;
class stMeshModel;
class srMeshModel;

bool IsTextureInReadMeshScratch(const void* texture);
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
