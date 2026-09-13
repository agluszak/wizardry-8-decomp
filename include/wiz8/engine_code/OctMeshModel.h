#pragma once

class srMaterialIFace;
class srTextureIFace;
class stMeshModel;

#include "surrender/srMath.h"

/* Engine Code\OctSubMesh.cpp's serialized mesh workspace. The assertion in
   its write method names the original class OctMeshModel; the adjacent
   constructor, destructor, read, and write bodies all use this same 0x48-byte
   receiver. Only fields whose roles are established by those bodies are named. */
class OctMeshModel {
public:
    OctMeshModel();  /* 0x0049E4C0 */
    ~OctMeshModel(); /* 0x0049E500 */
    stMeshModel* Read0049E9A0(int file, srMaterialIFace** materials, srTextureIFace** textures,
                              unsigned long* render_flags, stMeshModel** meshes,
                              int material_count);
    unsigned char Write0049E5D0(int hFile); /* 0x0049E5D0, body unrecovered */

    short version_00;
    short padding_02;
    int link_index_04;
    int uv_count_08;
    int material_index_0c;
    int map_count_10;                     /* m_psrMap entry count */
    srVector3T<float>* vertex_locations_14; /* m_psrVertLoc */
    srVector2T<float>* vertex_map_18;       /* m_psrMap */
    int* vertex_materials_1c;             /* m_plVertMats */
    srVector3i* poly_vertices_20;         /* m_psrPolyVertex */
    srVector3i* poly_uv_index_24;         /* m_psrPolyUVIndex */
    int* poly_textures_28;                /* m_plPolyTextures */
    srVector3T<float>* vertex_normals_2c; /* m_pVertNorms */
    srVector3T<float>* vertex_lights_30;  /* m_pVertLights */
    srVector4T<float>* poly_equations_34; /* m_psrPolyEqtns */
    float** sun_lights_38;                /* m_ppflSunLights */
    unsigned int packed_header_3c;
    int vertex_count_40;
    int polygon_count_44;
};

static_assert(sizeof(OctMeshModel) == 0x48, "OctMeshModel_size_must_be_0x48");
