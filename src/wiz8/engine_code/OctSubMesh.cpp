#include "wiz8/engine_code/OctMeshModel.h"
#include "wiz8/engine_code/Octree.h"

#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/materials.h"
#include "wiz8/engine_code/stMeshModel.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/virtual_file.h"
#include "surrender/srMaterial.h"
#include "surrender/srPtr.h"
#include "surrender/srTexture.h"
#include "surrender/srHeap.h"

#include <stdlib.h>

/* Engine Code\OctSubMesh.cpp. Read0049E9A0 at 0x0049E9A0 asserts this unit
   (lines 0x1ad and 0x1d2); the constructor and destructor immediately before
   it are the preceding attribution gap and stay here provisionally. */

static stMaterial* g_oct_mesh_default_material_00652dbc;
static srTextureIFace* g_oct_mesh_default_texture_00652dc0;
static unsigned long* g_oct_mesh_default_render_flags_00652dc4;

/* The loader verifies every array the same way: a null getter result and a
   failed bulk read each stop with the call site's own diagnostic. */
template <class T>
inline void ReadMeshArray(int file, T* values, int count, const char* get_message,
                          const char* read_message)
{
    if (values == 0) {
        ShutdownWithErrorBox(get_message);
    }
    if (!ReadVectorArray(file, values, count)) {
        ShutdownWithErrorBox(read_message);
    }
}

// FUNCTION: WIZ8 0x0049E4C0
OctMeshModel::OctMeshModel()
    : version_00(0), link_index_04(0), uv_count_08(0), material_index_0c(0), map_count_10(0),
      vertex_locations_14(0), vertex_map_18(0), vertex_materials_1c(0), poly_vertices_20(0),
      poly_uv_index_24(0), poly_textures_28(0), vertex_normals_2c(0), vertex_lights_30(0),
      poly_equations_34(0), sun_lights_38(0), packed_header_3c(0), vertex_count_40(0),
      polygon_count_44(0)
{
}

// FUNCTION: WIZ8 0x0049E500
OctMeshModel::~OctMeshModel()
{
    if (vertex_locations_14 != 0) {
        srHeap.free(vertex_locations_14);
    }
    if (vertex_map_18 != 0) {
        srHeap.free(vertex_map_18);
    }
    if (vertex_materials_1c != 0) {
        free(vertex_materials_1c);
    }
    if (poly_vertices_20 != 0) {
        srHeap.free(poly_vertices_20);
    }
    if (poly_uv_index_24 != 0) {
        srHeap.free(poly_uv_index_24);
    }
    if (poly_textures_28 != 0) {
        free(poly_textures_28);
    }
    if (vertex_normals_2c != 0) {
        srHeap.free(vertex_normals_2c);
    }
    if (vertex_lights_30 != 0) {
        srHeap.free(vertex_lights_30);
    }
    if (poly_equations_34 != 0) {
        srHeap.free(poly_equations_34);
    }
    if (sun_lights_38 != 0) {
        for (short index = 0; index < version_00; ++index) {
            free(sun_lights_38[index]);
        }
    }
}

// FUNCTION: WIZ8 0x0049E5D0
bool OctMeshModel::Write0049E5D0(int hFile)
{
    unsigned char success;
    unsigned char write_result;
    int index;

    packed_header_3c |= static_cast<unsigned int>(version_00) << 8;
    success = FileWrite(hFile, &packed_header_3c, 4, 0);
    success &= FileWrite(hFile, &vertex_count_40, 4, 0);
    success &= FileWrite(hFile, &map_count_10, 4, 0);
    success &= FileWrite(hFile, &polygon_count_44, 4, 0);
    success &= FileWrite(hFile, &link_index_04, 4, 0);
    success &= FileWrite(hFile, &uv_count_08, 4, 0);
    success &= FileWrite(hFile, &material_index_0c, 4, 0);
    if (success == 0) {
        srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\OctSubMesh.cpp", 0x105,
                     "OctMeshModel::Write -- Could not write INT32 fields.\n");
    }

    write_result =
        FileWrite(hFile, vertex_locations_14, vertex_count_40 * sizeof(srVector3T<float>), 0);
    if (write_result == 0) {
        srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\OctSubMesh.cpp", 0x109,
                     "OctMeshModel::Write -- Could not write m_psrVertLoc.\n");
    }
    srHeap.free(vertex_locations_14);
    vertex_locations_14 = 0;

    write_result = FileWrite(hFile, vertex_map_18, uv_count_08 * sizeof(srVector2T<float>), 0);
    if (write_result == 0) {
        srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\OctSubMesh.cpp", 0x10f,
                     "OctMeshModel::Write -- Could not write m_psrMap.\n");
    }
    srHeap.free(vertex_map_18);
    vertex_map_18 = 0;

    if (material_index_0c < 0) {
        write_result = FileWrite(hFile, vertex_materials_1c, vertex_count_40 * sizeof(int), 0);
        if (write_result == 0) {
            srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\OctSubMesh.cpp", 0x116,
                         "OctMeshModel::Write -- Could not write m_plVertMats.\n");
        }
    }
    if (vertex_materials_1c != 0) {
        free(vertex_materials_1c);
    }
    vertex_materials_1c = 0;

    write_result = FileWrite(hFile, poly_uv_index_24, polygon_count_44 * sizeof(srVector3i), 0);
    if (write_result == 0) {
        srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\OctSubMesh.cpp", 0x11f,
                     "OctMeshModel::Write -- Could not write m_psrPolyUVIndex.\n");
    }
    srHeap.free(poly_uv_index_24);
    poly_uv_index_24 = 0;

    write_result = FileWrite(hFile, poly_vertices_20, polygon_count_44 * sizeof(srVector3i), 0);
    if (write_result == 0) {
        srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\OctSubMesh.cpp", 0x125,
                     "OctMeshModel::Write -- Could not write m_psrPolyVertex.\n");
    }
    srHeap.free(poly_vertices_20);
    poly_vertices_20 = 0;

    write_result = FileWrite(hFile, poly_textures_28, polygon_count_44 * sizeof(int), 0);
    if (write_result == 0) {
        srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\OctSubMesh.cpp", 0x12a,
                     "OctMeshModel::Write -- Could not write m_plPolyTextures.\n");
    }
    free(poly_textures_28);
    poly_textures_28 = 0;

    write_result =
        FileWrite(hFile, vertex_normals_2c, vertex_count_40 * sizeof(srVector3T<float>), 0);
    if (write_result == 0) {
        srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\OctSubMesh.cpp", 0x130,
                     "OctMeshModel::Write -- Could not write m_pVertNorms.\n");
    }
    srHeap.free(vertex_normals_2c);
    vertex_normals_2c = 0;

    write_result =
        FileWrite(hFile, vertex_lights_30, vertex_count_40 * sizeof(srVector3T<float>), 0);
    if (write_result == 0) {
        srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\OctSubMesh.cpp", 0x136,
                     "OctMeshModel::Write -- Could not write m_pVertLights.\n");
    }
    srHeap.free(vertex_lights_30);
    vertex_lights_30 = 0;

    write_result =
        FileWrite(hFile, poly_equations_34, polygon_count_44 * sizeof(srVector4T<float>), 0);
    if (write_result == 0) {
        srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\OctSubMesh.cpp", 0x13c,
                     "OctMeshModel::Write -- Could not write m_psrPolyEqtns.\n");
    }
    srHeap.free(poly_equations_34);
    poly_equations_34 = 0;

    if (version_00 != 0) {
        write_result = FileWrite(hFile, sun_lights_38[0], vertex_count_40 * sizeof(float), 0);
        if (write_result == 0) {
            srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\OctSubMesh.cpp", 0x143,
                         "OctMeshModel::Write -- Could not write m_ppflSunLights.\n");
        }
        for (index = 0; index < version_00; ++index) {
            free(sun_lights_38[index]);
        }
        free(sun_lights_38);
    }
    sun_lights_38 = 0;

    int terminator = -1;
    write_result = FileWrite(hFile, &terminator, 4, 0);
    return write_result & success;
}

// FUNCTION: WIZ8 0x0049E9A0
stMeshModel* OctMeshModel::Read0049E9A0(int file, srMaterialIFace** materials,
                                        srTextureIFace** textures, unsigned long* render_flags,
                                        stMeshModel** meshes, int material_count)
{
    if (g_oct_mesh_default_material_00652dbc == 0) {
        g_oct_mesh_default_material_00652dbc = new stMaterial;
        *static_cast<srMaterial*>(g_oct_mesh_default_material_00652dbc) =
            *static_cast<srMaterial*>(materials[0]);
        g_oct_mesh_default_texture_00652dc0 = textures[0];
        delete g_oct_mesh_default_render_flags_00652dc4;
        g_oct_mesh_default_render_flags_00652dc4 = new unsigned long;
        *g_oct_mesh_default_render_flags_00652dc4 = render_flags[0];
    }

    unsigned char read_ok = 1;
    read_ok &= FileRead(file, &packed_header_3c, 4, 0);
    read_ok &= FileRead(file, &vertex_count_40, 4, 0);
    read_ok &= FileRead(file, &map_count_10, 4, 0);
    read_ok &= FileRead(file, &polygon_count_44, 4, 0);
    read_ok &= FileRead(file, &link_index_04, 4, 0);
    read_ok &= FileRead(file, &uv_count_08, 4, 0);
    read_ok &= FileRead(file, &material_index_0c, 4, 0);
    if (read_ok == 0) {
        ShutdownWithErrorBox("OctMeshModel::Read -- Could not read Integer fields.");
    }

    unsigned int header = packed_header_3c;
    bool unweighted = (header & 0xff) == 0;
    packed_header_3c = header & 0xff;
    version_00 = static_cast<short>((header >> 8) & 0xff);

    int index_count = vertex_count_40;
    if (index_count < polygon_count_44) {
        index_count = polygon_count_44;
    }
    vertex_materials_1c = static_cast<int*>(malloc(index_count * sizeof(int)));
    if (vertex_materials_1c == 0) {
        ShutdownWithErrorBox("OctMeshModel::Read -- Could not allocate m_plVertMats.");
    }

    stMeshModel* model = new stMeshModel(polygon_count_44, vertex_count_40);
    if (model == 0) {
        ShutdownWithErrorBox("OctMeshModel::Read -- Could not create pstMeshModel.");
    }
    model->autoRelease();
    if (unweighted) {
        model->flags_3a0 &= ~1U;
    } else {
        model->flags_3a0 |= 1;
    }

    vertex_locations_14 = model->getVertexLoc();
    ReadMeshArray(file, vertex_locations_14, vertex_count_40,
                  "OctMeshModel::Read -- Could not get vertex location array.",
                  "OctMeshModel::Read -- Could not read Integer fields.");

    model->setUVCount(map_count_10);
    vertex_map_18 = model->getVertexTexCoords(0, 0, 1);
    ReadMeshArray(file, vertex_map_18, map_count_10,
                  "OctMeshModel::Read -- Could not get vertex mapping array.",
                  "OctMeshModel::Read -- Could not read m_psrMap.");

    int selected_material = material_index_0c;
    int index;
    if (material_index_0c < 0) {
        srPtr<srMaterialIFace>* vertex_materials =
            model->getVertexMaterial(0, static_cast<srMeshModel::e_side>(0), 1);
        if (vertex_materials == 0) {
            ShutdownWithErrorBox("OctMeshModel::Read -- Could not get vertex material array.");
        }
        if (!FileRead(file, vertex_materials_1c, vertex_count_40 * sizeof(int), 0)) {
            ShutdownWithErrorBox("OctMeshModel::Read -- Could not read m_plVertMats.");
        }
        selected_material = vertex_materials_1c[0];
        for (index = 0; index < vertex_count_40; ++index) {
            int material_index = vertex_materials_1c[index];
            if (material_index < 0) {
                srAssertFail("m_plVertMats[iCount] >= 0",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\OctSubMesh.cpp", 0x1ad, 0);
            }
            vertex_materials[index] = materials[material_index];
            vertex_materials[index]->addReference();
        }
    } else {
        model->setMaterial(materials[material_index_0c], 0, static_cast<srMeshModel::e_side>(0));
    }

    poly_uv_index_24 = model->getPolyUVIndex(0, 1);
    ReadMeshArray(file, poly_uv_index_24, polygon_count_44,
                  "OctMeshModel::Read -- Could not get poly-UV array.",
                  "OctMeshModel::Read -- Could not read m_psrPolyUVIndex.");

    poly_vertices_20 = model->getPolyVertex();
    ReadMeshArray(file, poly_vertices_20, polygon_count_44,
                  "OctMeshModel::Read -- Could not get poly-vertex array.",
                  "OctMeshModel::Read -- Could not read m_psrPolyVertex.");

    srPtr<srTextureIFace>* polygon_textures = model->getPolyTexture(0, 0, 1);
    if (polygon_textures == 0) {
        ShutdownWithErrorBox("OctMeshModel::Read -- Could not get poly texture array.");
    }
    if (!FileRead(file, vertex_materials_1c, polygon_count_44 * sizeof(int), 0)) {
        ShutdownWithErrorBox("OctMeshModel::Read -- Could not read m_plPolyTextures.");
    }
    for (index = 0; index < polygon_count_44; ++index) {
        int texture_index = vertex_materials_1c[index];
        if (texture_index < 0) {
            srAssertFail("m_plVertMats[iCount] >= 0",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\OctSubMesh.cpp", 0x1d2, 0);
        }
        polygon_textures[index] = textures[texture_index];
    }

    vertex_normals_2c = model->getVertexNormal();
    ReadMeshArray(file, vertex_normals_2c, vertex_count_40,
                  "OctMeshModel::Read -- Could not get vertex normal array.",
                  "OctMeshModel::Read -- Could not read m_pVertNorms.");

    vertex_lights_30 = model->GetVertexLights(1, -1);
    ReadMeshArray(file, vertex_lights_30, vertex_count_40,
                  "OctMeshModel::Read -- Could not get static lighting array.",
                  "OctMeshModel::Read -- Could not read m_pVertLights.");

    poly_equations_34 = model->getPolyEq();
    ReadMeshArray(file, poly_equations_34, polygon_count_44,
                  "OctMeshModel::Read -- Could not get poly equation array.",
                  "OctMeshModel::Read -- Could not read m_psrPolyEqtns.");

    float* weights = model->GetVertexSunlight(1);
    if (weights == 0) {
        ShutdownWithErrorBox("OctMeshModel::Read -- Could not allocate intensity array.");
    }
    if (version_00 != 0 && !FileRead(file, weights, vertex_count_40 * sizeof(float), 0)) {
        ShutdownWithErrorBox("OctMeshModel::Read -- Could not read Sunlight array.");
    }

    unsigned long* shade_indices = model->getVertexShadeIndex(1);
    if (vertex_locations_14 == 0) {
        ShutdownWithErrorBox("OctMeshModel::Read -- Could not get vertex location array.");
    }
    for (unsigned long shade_index = 0; shade_index < static_cast<unsigned long>(vertex_count_40);
         ++shade_index) {
        shade_indices[shade_index] = shade_index;
    }

    int terminator;
    FileRead(file, &terminator, sizeof(terminator), 0);
    if (terminator != -1) {
        ShutdownWithErrorBox("Mesh Model in .pvl file is wrong length.");
    }

    srShader shader;
    CopyLevelDataHandle(&shader.value, &render_flags[selected_material]);
    model->setShader(shader, 0);
    if ((render_flags[selected_material] & 0x6000) == 0x4000) {
        if (unweighted) {
            ShutdownWithErrorBox("OctMeshModel::Read -- Wrong shader type.");
        } else {
            model->enableStartupControls();
        }
    } else if (!unweighted) {
        model->enableStartupControls();
    }

    if (link_index_04 >= 0) {
        meshes[link_index_04]->LinkTo(model);
        model->NotifyLinkedModel005AA400(meshes[link_index_04]);
    }

    vertex_locations_14 = 0;
    vertex_map_18 = 0;
    poly_vertices_20 = 0;
    poly_uv_index_24 = 0;
    vertex_normals_2c = 0;
    vertex_lights_30 = 0;
    poly_equations_34 = 0;
    free(vertex_materials_1c);
    vertex_materials_1c = 0;
    return model;
}
