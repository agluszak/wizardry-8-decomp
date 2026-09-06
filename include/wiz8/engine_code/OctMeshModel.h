#pragma once

class srMaterialIFace;
class srTextureIFace;
class stMeshModel;

/* Engine Code\OctSubMesh.cpp's serialized mesh workspace. The assertion in
   its write method names the original class OctMeshModel; the adjacent
   constructor, destructor, read, and write bodies all use this same 0x48-byte
   receiver. Only fields whose roles are established by those bodies are named. */
class OctMeshModel {
public:
    OctMeshModel();                                      /* 0x0049E4C0 */
    ~OctMeshModel();                                     /* 0x0049E500 */
    stMeshModel* Read0049E9A0(
        int file,
        srMaterialIFace** materials,
        srTextureIFace** textures,
        unsigned long* render_flags,
        stMeshModel** meshes,
        int material_count);

    short version_00;
    short padding_02;
    int link_index_04;
    int uv_count_08;
    int material_index_0c;
    int positional_10;
    void* heap_14;
    void* heap_18;
    void* allocated_1c;
    void* heap_20;
    void* heap_24;
    void* allocated_28;
    void* heap_2c;
    void* heap_30;
    void* heap_34;
    void** allocated_rows_38;
    unsigned int packed_header_3c;
    int vertex_count_40;
    int polygon_count_44;
};

static_assert(sizeof(OctMeshModel) == 0x48,
              "OctMeshModel_size_must_be_0x48");
