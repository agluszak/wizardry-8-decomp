#pragma once

class srMaterialIFace;
class srShader;
class srTextureIFace;
class stMaterial;
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
    stMeshModel* Read(int file, srMaterialIFace** materials, srTextureIFace** textures,
                      srShader* render_flags, stMeshModel** meshes, int material_count);
    bool Write(int hFile); /* 0x0049E5D0 */

    short version;
    short padding_02;
    int m_link_index;
    int next_link;
    int m_material_index;
    int m_map_count;                       /* m_psrMap entry count */
    srVector3T<float>* m_vertex_locations; /* m_psrVertLoc */
    srVector2T<float>* m_vertex_map;       /* m_psrMap */
    int* m_vertex_materials;               /* m_plVertMats */
    srVector3i* m_poly_vertices;           /* m_psrPolyVertex */
    srVector3i* m_poly_uv_index;           /* m_psrPolyUVIndex */
    int* m_poly_textures;                  /* m_plPolyTextures */
    srVector3T<float>* m_vertex_normals;   /* m_pVertNorms */
    srVector3T<float>* m_vertex_lights;    /* m_pVertLights */
    srVector4T<float>* m_poly_equations;   /* m_psrPolyEqtns */
    float** m_sun_lights;                  /* m_ppflSunLights */
    unsigned int m_packed_header;
    int m_vertex_count;
    int m_polygon_count;
};

static_assert(sizeof(OctMeshModel) == 0x48, "OctMeshModel_size_must_be_0x48");

/* The first Read call snapshots material/texture/render-flag zero as the
   shared default rendering state for the path and trace meshes. */
extern stMaterial* g_oct_mesh_default_material;
extern srTextureIFace* g_oct_mesh_default_texture;
extern srShader* g_oct_mesh_default_shader;
