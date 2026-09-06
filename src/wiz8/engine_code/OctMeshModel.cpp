#include "wiz8/engine_code/OctMeshModel.h"

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

static stMaterial* g_oct_mesh_default_material_00652dbc;
static srTextureIFace* g_oct_mesh_default_texture_00652dc0;
static unsigned long* g_oct_mesh_default_render_flags_00652dc4;

extern unsigned char ReadVector4Array004374C0(
    int file, srVector4T<float>* values, int count);
extern unsigned char ReadVector3Array004374E0(
    int file, srVector3T<float>* values, int count);
extern unsigned char ReadVector2Array00437510(
    int file, srVector2T<float>* values, int count);
// FUNCTION: WIZ8 0x0049E4C0
OctMeshModel::OctMeshModel()
    : version_00(0),
      link_index_04(0),
      uv_count_08(0),
      material_index_0c(0),
      positional_10(0),
      heap_14(0),
      heap_18(0),
      allocated_1c(0),
      heap_20(0),
      heap_24(0),
      allocated_28(0),
      heap_2c(0),
      heap_30(0),
      heap_34(0),
      allocated_rows_38(0),
      packed_header_3c(0),
      vertex_count_40(0),
      polygon_count_44(0)
{
}

// FUNCTION: WIZ8 0x0049E500
OctMeshModel::~OctMeshModel()
{
    if (heap_14 != 0) {
        srHeap.free(heap_14);
    }
    if (heap_18 != 0) {
        srHeap.free(heap_18);
    }
    if (allocated_1c != 0) {
        free(allocated_1c);
    }
    if (heap_20 != 0) {
        srHeap.free(heap_20);
    }
    if (heap_24 != 0) {
        srHeap.free(heap_24);
    }
    if (allocated_28 != 0) {
        free(allocated_28);
    }
    if (heap_2c != 0) {
        srHeap.free(heap_2c);
    }
    if (heap_30 != 0) {
        srHeap.free(heap_30);
    }
    if (heap_34 != 0) {
        srHeap.free(heap_34);
    }
    if (allocated_rows_38 != 0) {
        for (short index = 0; index < version_00; ++index) {
            free(allocated_rows_38[index]);
        }
    }
}

// FUNCTION: WIZ8 0x0049E9A0
stMeshModel* OctMeshModel::Read0049E9A0(
    int file,
    srMaterialIFace** materials,
    srTextureIFace** textures,
    unsigned long* render_flags,
    stMeshModel** meshes,
    int material_count)
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
    read_ok &= ReadVirtualFile(file, &packed_header_3c, 4, 0);
    read_ok &= ReadVirtualFile(file, &vertex_count_40, 4, 0);
    read_ok &= ReadVirtualFile(file, &positional_10, 4, 0);
    read_ok &= ReadVirtualFile(file, &polygon_count_44, 4, 0);
    read_ok &= ReadVirtualFile(file, &link_index_04, 4, 0);
    read_ok &= ReadVirtualFile(file, &uv_count_08, 4, 0);
    read_ok &= ReadVirtualFile(file, &material_index_0c, 4, 0);
    if (read_ok == 0) {
        ReportError00401920("OctMeshModel::Read -- Could not read Integer fields.");
    }

    unsigned int header = packed_header_3c;
    bool unweighted = (header & 0xff) == 0;
    packed_header_3c = header & 0xff;
    version_00 = static_cast<short>((header >> 8) & 0xff);

    int index_count = vertex_count_40;
    if (index_count < polygon_count_44) {
        index_count = polygon_count_44;
    }
    allocated_1c = malloc(index_count * sizeof(int));
    if (allocated_1c == 0) {
        ReportError00401920("OctMeshModel::Read -- Could not allocate m_plVertMats.");
    }

    stMeshModel* model = new stMeshModel(polygon_count_44, vertex_count_40);
    if (model == 0) {
        ReportError00401920("OctMeshModel::Read -- Could not create pstMeshModel.");
    }
    model->autoRelease();
    if (unweighted) {
        model->flags_3a0 &= ~1U;
    }
    else {
        model->flags_3a0 |= 1;
    }

    heap_14 = model->getVertexLoc();
    if (heap_14 == 0) {
        ReportError00401920("OctMeshModel::Read -- Could not get vertex location array.");
    }
    if (!ReadVector3Array004374E0(
            file, static_cast<srVector3T<float>*>(heap_14), vertex_count_40)) {
        ReportError00401920("OctMeshModel::Read -- Could not read Integer fields.");
    }

    model->setUVCount(positional_10);
    heap_18 = model->getVertexTexCoords(0, 0, 1);
    if (heap_18 == 0) {
        ReportError00401920("OctMeshModel::Read -- Could not get vertex mapping array.");
    }
    if (!ReadVector2Array00437510(
            file, static_cast<srVector2T<float>*>(heap_18), positional_10)) {
        ReportError00401920("OctMeshModel::Read -- Could not read m_psrMap.");
    }

    int selected_material = material_index_0c;
    int index;
    if (material_index_0c < 0) {
        srPtr<srMaterialIFace>* vertex_materials =
            model->getVertexMaterial(
                0, static_cast<srMeshModel::e_side>(0), 1);
        if (vertex_materials == 0) {
            ReportError00401920("OctMeshModel::Read -- Could not get vertex material array.");
        }
        if (!ReadVirtualFile(file, allocated_1c,
                vertex_count_40 * sizeof(int), 0)) {
            ReportError00401920("OctMeshModel::Read -- Could not read m_plVertMats.");
        }
        selected_material = static_cast<int*>(allocated_1c)[0];
        for (index = 0; index < vertex_count_40; ++index) {
            int material_index = static_cast<int*>(allocated_1c)[index];
            if (material_index < 0) {
                srAssertFail("m_plVertMats[iCount] >= 0",
                    "C:\\Projects\\Wizardry 8\\Engine Code\\OctSubMesh.cpp",
                    0x1ad, 0);
            }
            vertex_materials[index] = materials[material_index];
            vertex_materials[index]->addReference();
        }
    }
    else {
        model->setMaterial(
            materials[material_index_0c], 0,
            static_cast<srMeshModel::e_side>(0));
    }

    heap_24 = model->getPolyUVIndex(0, 1);
    if (heap_24 == 0) {
        ReportError00401920("OctMeshModel::Read -- Could not get poly-UV array.");
    }
    if (!ReadVector3Array004374E0(
            file, reinterpret_cast<srVector3T<float>*>(heap_24),
            polygon_count_44)) {
        ReportError00401920("OctMeshModel::Read -- Could not read m_psrPolyUVIndex.");
    }

    heap_20 = model->getPolyVertex();
    if (heap_20 == 0) {
        ReportError00401920("OctMeshModel::Read -- Could not get poly-vertex array.");
    }
    if (!ReadVector3Array004374E0(
            file, reinterpret_cast<srVector3T<float>*>(heap_20),
            polygon_count_44)) {
        ReportError00401920("OctMeshModel::Read -- Could not read m_psrPolyVertex.");
    }

    srPtr<srTextureIFace>* polygon_textures =
        model->getPolyTexture(0, 0, 1);
    if (polygon_textures == 0) {
        ReportError00401920("OctMeshModel::Read -- Could not get poly texture array.");
    }
    if (!ReadVirtualFile(file, allocated_1c,
            polygon_count_44 * sizeof(int), 0)) {
        ReportError00401920("OctMeshModel::Read -- Could not read m_plPolyTextures.");
    }
    for (index = 0; index < polygon_count_44; ++index) {
        int texture_index = static_cast<int*>(allocated_1c)[index];
        if (texture_index < 0) {
            srAssertFail("m_plVertMats[iCount] >= 0",
                "C:\\Projects\\Wizardry 8\\Engine Code\\OctSubMesh.cpp",
                0x1d2, 0);
        }
        polygon_textures[index] = textures[texture_index];
    }

    heap_2c = model->getVertexNormal();
    if (heap_2c == 0) {
        ReportError00401920("OctMeshModel::Read -- Could not get vertex normal array.");
    }
    if (!ReadVector3Array004374E0(
            file, static_cast<srVector3T<float>*>(heap_2c), vertex_count_40)) {
        ReportError00401920("OctMeshModel::Read -- Could not read m_pVertNorms.");
    }

    heap_30 = model->GetVertexDIG00472100(1, -1);
    if (heap_30 == 0) {
        ReportError00401920("OctMeshModel::Read -- Could not get static lighting array.");
    }
    if (!ReadVector3Array004374E0(
            file, static_cast<srVector3T<float>*>(heap_30), vertex_count_40)) {
        ReportError00401920("OctMeshModel::Read -- Could not read m_pVertLights.");
    }

    heap_34 = model->getPolyEq();
    if (heap_34 == 0) {
        ReportError00401920("OctMeshModel::Read -- Could not get poly equation array.");
    }
    if (!ReadVector4Array004374C0(
            file, static_cast<srVector4T<float>*>(heap_34), polygon_count_44)) {
        ReportError00401920("OctMeshModel::Read -- Could not read m_psrPolyEqtns.");
    }

    float* weights = model->InitializeVertexWeights004721E0(1);
    if (weights == 0) {
        ReportError00401920("OctMeshModel::Read -- Could not allocate intensity array.");
    }
    if (version_00 != 0 &&
        !ReadVirtualFile(file, weights, vertex_count_40 * sizeof(float), 0)) {
        ReportError00401920("OctMeshModel::Read -- Could not read Sunlight array.");
    }

    unsigned long* shade_indices = model->getVertexShadeIndex(1);
    if (heap_14 == 0) {
        ReportError00401920("OctMeshModel::Read -- Could not get vertex location array.");
    }
    for (unsigned long shade_index = 0;
         shade_index < static_cast<unsigned long>(vertex_count_40);
         ++shade_index) {
        shade_indices[shade_index] = shade_index;
    }

    int terminator;
    ReadVirtualFile(file, &terminator, sizeof(terminator), 0);
    if (terminator != -1) {
        ReportError00401920("Mesh Model in .pvl file is wrong length.");
    }

    srShader shader;
    CopyLevelDataHandle(
        reinterpret_cast<int*>(&shader.value),
        reinterpret_cast<const int*>(&render_flags[selected_material]));
    model->setShader(shader, 0);
    if ((render_flags[selected_material] & 0x6000) == 0x4000) {
        if (unweighted) {
            ReportError00401920("OctMeshModel::Read -- Wrong shader type.");
        }
        else {
            model->enableStartupControls();
        }
    }
    else if (!unweighted) {
        model->enableStartupControls();
    }

    if (link_index_04 >= 0) {
        meshes[link_index_04]->LinkTo(model);
        model->Function5AA400(meshes[link_index_04]);
    }

    heap_14 = 0;
    heap_18 = 0;
    heap_20 = 0;
    heap_24 = 0;
    heap_2c = 0;
    heap_30 = 0;
    heap_34 = 0;
    free(allocated_1c);
    allocated_1c = 0;
    return model;
}
