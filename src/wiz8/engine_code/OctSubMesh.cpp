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

/* Engine Code\OctSubMesh.cpp. Read at 0x0049E9A0 asserts this unit
   (lines 0x1ad and 0x1d2); the constructor and destructor immediately before
   it are the preceding attribution gap and stay here provisionally. */

/* The first Read captures material[0], texture[0] and the render flags as
   the default shader state later consumed by the path visualization and
   trace models in OctPath.cpp and GameData.cpp. */
// GLOBAL: WIZ8 0x00652dbc
stMaterial* g_oct_mesh_default_material;
// GLOBAL: WIZ8 0x00652dc0
srTextureIFace* g_oct_mesh_default_texture;
// GLOBAL: WIZ8 0x00652dc4
srShader* g_oct_mesh_default_shader;

/* The loader verifies every array the same way: a null getter result and a
   failed bulk read each stop with the call site's own diagnostic. */
template <class T>
void ReadMeshArray(int file, T* values, int count, const char* get_message,
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
    : version_00(0), m_link_index(0), next_link(0), m_material_index(0), m_map_count(0),
      m_vertex_locations(0), m_vertex_map(0), m_vertex_materials(0), m_poly_vertices(0),
      m_poly_uv_index(0), m_poly_textures(0), m_vertex_normals(0), m_vertex_lights(0),
      m_poly_equations(0), m_sun_lights(0), m_packed_header(0), m_vertex_count(0),
      m_polygon_count(0)
{
}

// FUNCTION: WIZ8 0x0049E500
OctMeshModel::~OctMeshModel()
{
    if (m_vertex_locations != 0) {
        srHeap.free(m_vertex_locations);
    }
    if (m_vertex_map != 0) {
        srHeap.free(m_vertex_map);
    }
    if (m_vertex_materials != 0) {
        free(m_vertex_materials);
    }
    if (m_poly_vertices != 0) {
        srHeap.free(m_poly_vertices);
    }
    if (m_poly_uv_index != 0) {
        srHeap.free(m_poly_uv_index);
    }
    if (m_poly_textures != 0) {
        free(m_poly_textures);
    }
    if (m_vertex_normals != 0) {
        srHeap.free(m_vertex_normals);
    }
    if (m_vertex_lights != 0) {
        srHeap.free(m_vertex_lights);
    }
    if (m_poly_equations != 0) {
        srHeap.free(m_poly_equations);
    }
    if (m_sun_lights != 0) {
        for (short index = 0; index < version_00; ++index) {
            free(m_sun_lights[index]);
        }
    }
}

// FUNCTION: WIZ8 0x0049E5D0
bool OctMeshModel::Write(int hFile)
{
    unsigned char success;
    unsigned char write_result;
    int index;

    m_packed_header |= static_cast<unsigned int>(version_00) << 8;
    success = FileWrite(hFile, &m_packed_header, 4, 0);
    success &= FileWrite(hFile, &m_vertex_count, 4, 0);
    success &= FileWrite(hFile, &m_map_count, 4, 0);
    success &= FileWrite(hFile, &m_polygon_count, 4, 0);
    success &= FileWrite(hFile, &m_link_index, 4, 0);
    success &= FileWrite(hFile, &next_link, 4, 0);
    success &= FileWrite(hFile, &m_material_index, 4, 0);
    if (success == 0) {
        srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\OctSubMesh.cpp", 0x105,
                     "OctMeshModel::Write -- Could not write INT32 fields.\n");
    }

    write_result = WriteVectorArray(hFile, m_vertex_locations, m_vertex_count);
    if (write_result == 0) {
        srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\OctSubMesh.cpp", 0x109,
                     "OctMeshModel::Write -- Could not write m_psrVertLoc.\n");
    }
    srHeap.free(m_vertex_locations);
    m_vertex_locations = 0;

    write_result = WriteVectorArray(hFile, m_vertex_map, m_map_count);
    if (write_result == 0) {
        srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\OctSubMesh.cpp", 0x10f,
                     "OctMeshModel::Write -- Could not write m_psrMap.\n");
    }
    srHeap.free(m_vertex_map);
    m_vertex_map = 0;

    if (m_material_index < 0) {
        write_result = FileWrite(hFile, m_vertex_materials, m_vertex_count * sizeof(int), 0);
        if (write_result == 0) {
            srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\OctSubMesh.cpp", 0x116,
                         "OctMeshModel::Write -- Could not write m_plVertMats.\n");
        }
    }
    if (m_vertex_materials != 0) {
        free(m_vertex_materials);
    }
    m_vertex_materials = 0;

    write_result = WriteVectorArray(hFile, m_poly_uv_index, m_polygon_count);
    if (write_result == 0) {
        srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\OctSubMesh.cpp", 0x11f,
                     "OctMeshModel::Write -- Could not write m_psrPolyUVIndex.\n");
    }
    srHeap.free(m_poly_uv_index);
    m_poly_uv_index = 0;

    write_result = WriteVectorArray(hFile, m_poly_vertices, m_polygon_count);
    if (write_result == 0) {
        srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\OctSubMesh.cpp", 0x125,
                     "OctMeshModel::Write -- Could not write m_psrPolyVertex.\n");
    }
    srHeap.free(m_poly_vertices);
    m_poly_vertices = 0;

    write_result = FileWrite(hFile, m_poly_textures, m_polygon_count * sizeof(int), 0);
    if (write_result == 0) {
        srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\OctSubMesh.cpp", 0x12a,
                     "OctMeshModel::Write -- Could not write m_plPolyTextures.\n");
    }
    free(m_poly_textures);
    m_poly_textures = 0;

    write_result = WriteVectorArray(hFile, m_vertex_normals, m_vertex_count);
    if (write_result == 0) {
        srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\OctSubMesh.cpp", 0x130,
                     "OctMeshModel::Write -- Could not write m_pVertNorms.\n");
    }
    srHeap.free(m_vertex_normals);
    m_vertex_normals = 0;

    write_result = WriteVectorArray(hFile, m_vertex_lights, m_vertex_count);
    if (write_result == 0) {
        srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\OctSubMesh.cpp", 0x136,
                     "OctMeshModel::Write -- Could not write m_pVertLights.\n");
    }
    srHeap.free(m_vertex_lights);
    m_vertex_lights = 0;

    write_result = WriteVectorArray(hFile, m_poly_equations, m_polygon_count);
    if (write_result == 0) {
        srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\OctSubMesh.cpp", 0x13c,
                     "OctMeshModel::Write -- Could not write m_psrPolyEqtns.\n");
    }
    srHeap.free(m_poly_equations);
    m_poly_equations = 0;

    if (version_00 != 0) {
        write_result = FileWrite(hFile, m_sun_lights[0], m_vertex_count * sizeof(float), 0);
        if (write_result == 0) {
            srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\OctSubMesh.cpp", 0x143,
                         "OctMeshModel::Write -- Could not write m_ppflSunLights.\n");
        }
        for (index = 0; index < version_00; ++index) {
            free(m_sun_lights[index]);
        }
        free(m_sun_lights);
    }
    m_sun_lights = 0;

    int terminator = -1;
    write_result = FileWrite(hFile, &terminator, 4, 0);
    return write_result & success;
}

// FUNCTION: WIZ8 0x0049E9A0
stMeshModel* OctMeshModel::Read(int file, srMaterialIFace** materials, srTextureIFace** textures,
                                srShader* render_flags, stMeshModel** meshes, int material_count)
{
    if (g_oct_mesh_default_material == 0) {
        g_oct_mesh_default_material = new stMaterial;
        *static_cast<srMaterial*>(g_oct_mesh_default_material) =
            *static_cast<srMaterial*>(materials[0]);
        g_oct_mesh_default_texture = textures[0];
        delete g_oct_mesh_default_shader;
        g_oct_mesh_default_shader = new srShader;
        *g_oct_mesh_default_shader = render_flags[0];
    }

    unsigned char read_ok = 1;
    read_ok &= FileRead(file, &m_packed_header, 4, 0);
    read_ok &= FileRead(file, &m_vertex_count, 4, 0);
    read_ok &= FileRead(file, &m_map_count, 4, 0);
    read_ok &= FileRead(file, &m_polygon_count, 4, 0);
    read_ok &= FileRead(file, &m_link_index, 4, 0);
    read_ok &= FileRead(file, &next_link, 4, 0);
    read_ok &= FileRead(file, &m_material_index, 4, 0);
    if (read_ok == 0) {
        ShutdownWithErrorBox("OctMeshModel::Read -- Could not read Integer fields.\n");
    }

    unsigned int header = m_packed_header;
    bool unweighted = (header & 0xff) == 0;
    m_packed_header = header & 0xff;
    version_00 = static_cast<short>((header >> 8) & 0xff);

    int index_count = m_vertex_count;
    if (index_count < m_polygon_count) {
        index_count = m_polygon_count;
    }
    m_vertex_materials = static_cast<int*>(malloc(index_count * sizeof(int)));
    if (m_vertex_materials == 0) {
        ShutdownWithErrorBox("OctMeshModel::Read -- Could not allocate m_plVertMats.\n");
    }

    stMeshModel* model = new stMeshModel(m_polygon_count, m_vertex_count);
    if (model == 0) {
        ShutdownWithErrorBox("OctMeshModel::Read -- Could not create pstMeshModel.\n");
    }
    model->autoRelease();
    if (unweighted) {
        model->flags_3a0 &= ~1U;
    } else {
        model->flags_3a0 |= 1;
    }

    m_vertex_locations = model->getVertexLoc();
    ReadMeshArray(file, m_vertex_locations, m_vertex_count,
                  "OctMeshModel::Read -- Could not get vertex location array.\n",
                  "OctMeshModel::Read -- Could not read Integer fields.\n");

    model->setUVCount(m_map_count);
    m_vertex_map = model->getVertexTexCoords(0, 0, 1);
    ReadMeshArray(file, m_vertex_map, m_map_count,
                  "OctMeshModel::Read -- Could not get vertex mapping array.\n",
                  "OctMeshModel::Read -- Could not read m_psrMap.");

    int selected_material = m_material_index;
    int index;
    if (m_material_index < 0) {
        srPtr<srMaterialIFace>* vertex_materials =
            model->getVertexMaterial(0, static_cast<srMeshModel::e_side>(0), 1);
        if (vertex_materials == 0) {
            ShutdownWithErrorBox("OctMeshModel::Read -- Could not get vertex material array.\n");
        }
        if (!FileRead(file, m_vertex_materials, m_vertex_count * sizeof(int), 0)) {
            ShutdownWithErrorBox("OctMeshModel::Read -- Could not read m_plVertMats.\n");
        }
        selected_material = m_vertex_materials[0];
        for (index = 0; index < m_vertex_count; ++index) {
            int material_index = m_vertex_materials[index];
            if (material_index < 0) {
                srAssertFail("m_plVertMats[iCount] >= 0",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\OctSubMesh.cpp", 0x1ad, 0);
            }
            vertex_materials[index] = materials[material_index];
            vertex_materials[index]->addReference();
        }
    } else {
        model->setMaterial(materials[m_material_index], 0, static_cast<srMeshModel::e_side>(0));
    }

    m_poly_uv_index = model->getPolyUVIndex(0, 1);
    ReadMeshArray(file, m_poly_uv_index, m_polygon_count,
                  "OctMeshModel::Read -- Could not get poly-UV array.\n",
                  "OctMeshModel::Read -- Could not read m_psrPolyUVIndex.\n");

    m_poly_vertices = model->getPolyVertex();
    ReadMeshArray(file, m_poly_vertices, m_polygon_count,
                  "OctMeshModel::Read -- Could not get poly-vertex array.\n",
                  "OctMeshModel::Read -- Could not read m_psrPolyVertex.\n");

    srPtr<srTextureIFace>* polygon_textures = model->getPolyTexture(0, 0, 1);
    if (polygon_textures == 0) {
        ShutdownWithErrorBox("OctMeshModel::Read -- Could not get poly texture array.\n");
    }
    if (!FileRead(file, m_vertex_materials, m_polygon_count * sizeof(int), 0)) {
        ShutdownWithErrorBox("OctMeshModel::Read -- Could not read m_plPolyTextures.\n");
    }
    for (index = 0; index < m_polygon_count; ++index) {
        int texture_index = m_vertex_materials[index];
        if (texture_index < 0) {
            srAssertFail("m_plVertMats[iCount] >= 0",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\OctSubMesh.cpp", 0x1d2, 0);
        }
        polygon_textures[index] = textures[texture_index];
    }

    m_vertex_normals = model->getVertexNormal();
    ReadMeshArray(file, m_vertex_normals, m_vertex_count,
                  "OctMeshModel::Read -- Could not get vertex normal array.\n",
                  "OctMeshModel::Read -- Could not read m_pVertNorms.\n");

    m_vertex_lights = model->GetVertexLights(1, -1);
    ReadMeshArray(file, m_vertex_lights, m_vertex_count,
                  "OctMeshModel::Read -- Could not get static lighting array.\n",
                  "OctMeshModel::Read -- Could not read m_pVertLights.\n");

    m_poly_equations = model->getPolyEq();
    ReadMeshArray(file, m_poly_equations, m_polygon_count,
                  "OctMeshModel::Read -- Could not get poly equation array.\n",
                  "OctMeshModel::Read -- Could not read m_psrPolyEqtns.\n");

    float* weights = model->GetVertexSunlight(1);
    if (weights == 0) {
        ShutdownWithErrorBox("OctMeshModel::Read -- Could not allocate intensity array.\n");
    }
    if (version_00 != 0 && !FileRead(file, weights, m_vertex_count * sizeof(float), 0)) {
        ShutdownWithErrorBox("OctMeshModel::Read -- Could not read Sunlight array.\n");
    }

    unsigned long* shade_indices = model->getVertexShadeIndex(1);
    if (m_vertex_locations == 0) {
        ShutdownWithErrorBox("OctMeshModel::Read -- Could not get vertex location array.\n");
    }
    for (unsigned long shade_index = 0; shade_index < static_cast<unsigned long>(m_vertex_count);
         ++shade_index) {
        shade_indices[shade_index] = shade_index;
    }

    int terminator;
    FileRead(file, &terminator, sizeof(terminator), 0);
    if (terminator != -1) {
        ShutdownWithErrorBox("Mesh Model in .pvl file is wrong length.");
    }

    model->setShader(render_flags[selected_material], 0);
    if ((render_flags[selected_material].value & 0x6000) == 0x4000) {
        if (unweighted) {
            ShutdownWithErrorBox("OctMeshModel::Read -- Wrong shader type.\n");
        } else {
            model->enable(srMeshModel::CONTROL_STARTUP);
        }
    } else if (!unweighted) {
        model->enable(srMeshModel::CONTROL_STARTUP);
    }

    if (m_link_index >= 0) {
        meshes[m_link_index]->LinkTo(model);
        model->NotifyLinkedModel(meshes[m_link_index]);
    }

    m_vertex_locations = 0;
    m_vertex_map = 0;
    m_poly_vertices = 0;
    m_poly_uv_index = 0;
    m_vertex_normals = 0;
    m_vertex_lights = 0;
    m_poly_equations = 0;
    free(m_vertex_materials);
    m_vertex_materials = 0;
    return model;
}
