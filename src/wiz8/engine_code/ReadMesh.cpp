#include "wiz8/engine_code/ReadLevel.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/OctMeshModel.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/materials.h"
#include "wiz8/engine_code/stMeshModel.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/sr_api.h"
#include "wiz8/screen_state.h"
#include "wiz8/virtual_file.h"
#include "wiz8/vector.h"

#include "FileMan.h"
#include "surrender/srCore.h"
#include "surrender/srNode.h"

#include <stdlib.h>
#include <string.h>

#pragma pack(push, 1)
struct W8ReadMeshFace {
    int vertices[3];
    srVector2T<float> texture_coordinates[3];
    int material_index;
    unsigned char flags;
};

struct W8CompressedReadMeshFace {
    unsigned short vertices[3];
    srVector2T<float> texture_coordinates[3];
    unsigned short material_index;
    unsigned char flags;
};
#pragma pack(pop)

static_assert(sizeof(W8ReadMeshFace) == 0x29,
              "W8ReadMeshFace_size_must_be_0x29");
static_assert(sizeof(W8CompressedReadMeshFace) == 0x21,
              "W8CompressedReadMeshFace_size_must_be_0x21");

/* The material reader retains its three parallel result tables together with
   the normalized serialized records used to identify a reusable table. */
static srMaterialIFace** g_read_mesh_materials_65b9e8;
static srTextureIFace** g_read_mesh_textures_65b9ec;
static unsigned long* g_read_mesh_render_flags_65b9f0;
static W8MaterialRecord004B8A70* g_read_mesh_material_records_65b9f4;
static int g_read_mesh_scratch_count_65b9f8;
static int g_read_mesh_material_count_65b9cc;
static int g_read_mesh_index_65b9e4;
static srMaterialIFace** g_multi_mesh_materials_65ba00;
static srTextureIFace** g_multi_mesh_textures_65b9fc;
static unsigned long* g_multi_mesh_render_flags_65ba04;
static int g_retained_material_count_65b9d4;
static int g_retained_material_capacity_65b9d8;
static srClass** g_retained_materials_65b9dc;

extern stModelInstance005EC7D0* CreateModelInstance0046F5C0(
    stMeshModel* model);
unsigned char ReadSingleLevelMeshBody00485C10(
    W8ReadLevelInfo* info, srModelInstance** instance,
    int positional_0, int positional_1, const char* name,
    unsigned char load_materials);
extern void ReadMeshTransform004896C0(
    int file, srVector3T<float>* location, srMatrix3T<float>* rotation,
    srVector3T<float>* scale);
stMeshModel* BuildSingleLevelMesh00488650(
    int face_count, W8ReadMeshFace* faces, int vertex_count,
    int material_count, srMaterialIFace** materials,
    srTextureIFace** textures, unsigned long* render_flags,
    unsigned int* mesh_count, int*** vertex_maps,
    unsigned int* vertex_map_count,
    W8GrowableVector<short>* mapped_values,
    W8GrowableVector<short>* mapped_keys);
extern void UpdateMeshAfterVertexLoad004867F0(
    srMeshModel* model, int frame);
extern void NormalizeMaterialRecord00489980(
    W8MaterialRecord004B8A70* material);
extern unsigned char CreateDefaultMaterial004B9280(
    srMaterialIFace** material, srTextureIFace** texture,
    unsigned long* render_flags);
extern void UpdatePleaseWaitLoadFrame005915A0(void);

namespace {

// TEMPLATE: WIZ8 0x00489fe0
// W8HashTable<unsigned int,int>::W8HashTable

// TEMPLATE: WIZ8 0x0055db80
// W8HashTable<unsigned int,int>::~W8HashTable

bool ReadMeshFaceNeedsSplit(
    const W8ReadMeshFace& face, srMaterialIFace** materials)
{
    return (face.flags & 4) != 0 ||
        (static_cast<stMaterial*>(materials[face.material_index])->m_field_78 & 1) != 0;
}

}

// FUNCTION: WIZ8 0x00488650
stMeshModel* BuildSingleLevelMesh00488650(
    int face_count, W8ReadMeshFace* faces, int vertex_count,
    int material_count, srMaterialIFace** materials,
    srTextureIFace** textures, unsigned long* render_flags,
    unsigned int* mesh_count, int*** vertex_maps,
    unsigned int* vertex_map_count,
    W8GrowableVector<short>* mapped_values,
    W8GrowableVector<short>* mapped_keys)
{
    W8GrowableVector<unsigned long> polygon_types;
    W8OctreeIndex vertex_indices[8];
    W8GrowableVector<int> duplicated_from[8];
    W8GrowableVector<int> duplicated_to[8];
    W8GrowableVector<short> mapped_meshes;
    W8GrowableVector<short> mapped_vertices;
    int capacities[8];
    int polygon_counts[8];
    int vertex_counts[8];
    int extra_uv_counts[8];
    srPtr<srMaterialIFace>* vertex_materials[8];
    srPtr<srTextureIFace>* polygon_textures[8];
    srVector2T<float>* vertex_uvs[8];
    srVector2T<float>* extra_uvs[8];
    srVector3i* polygon_vertices[8];
    srVector3i* polygon_shades[8];
    unsigned long* vertex_shades[8];
    stMeshModel* models[8];
    stMeshModel* first_model = 0;
    stMeshModel* previous_model = 0;
    int type;
    int face_index;

    for (int index = 0; index < mapped_values->count; ++index) {
        mapped_meshes.Add(-1);
        mapped_vertices.Add(-1);
    }

    for (int material = 0; material < material_count; ++material) {
        if (polygon_types.IndexOf(render_flags[material]) == -1) {
            int capacity = 0;
            for (face_index = 0; face_index < face_count; ++face_index) {
                W8ReadMeshFace& face = faces[face_index];
                if (render_flags[face.material_index] == render_flags[material]) {
                    ++capacity;
                    if (ReadMeshFaceNeedsSplit(face, materials)) {
                        ++capacity;
                    }
                }
            }
            if (capacity != 0) {
                capacities[polygon_types.count] = capacity;
                polygon_types.Add(render_flags[material]);
            }
        }
    }

    if (mesh_count != 0) {
        *mesh_count = polygon_types.count;
    }
    *vertex_maps = static_cast<int**>(malloc(polygon_types.count * sizeof(int*)));
    if (vertex_map_count != 0) {
        *vertex_map_count = polygon_types.count;
    }

    for (type = 0; type < polygon_types.count; ++type) {
        polygon_counts[type] = 0;
        vertex_counts[type] = 0;
        extra_uv_counts[type] = 0;
        int capacity = capacities[type];
        vertex_materials[type] = new srPtr<srMaterialIFace>[capacity * 3];
        polygon_textures[type] = new srPtr<srTextureIFace>[capacity];
        vertex_uvs[type] = static_cast<srVector2T<float>*>(
            malloc(capacity * 3 * sizeof(srVector2T<float>)));
        extra_uvs[type] = static_cast<srVector2T<float>*>(
            malloc(capacity * 3 * sizeof(srVector2T<float>)));
        polygon_vertices[type] = static_cast<srVector3i*>(
            malloc(capacity * 3 * sizeof(srVector3i)));
        polygon_shades[type] = static_cast<srVector3i*>(
            malloc(capacity * sizeof(srVector3i)));
        vertex_shades[type] = static_cast<unsigned long*>(
            malloc(capacity * 3 * sizeof(unsigned long)));
        (*vertex_maps)[type] = static_cast<int*>(
            malloc(capacity * 3 * sizeof(int)));
    }

    for (face_index = 0; face_index < face_count; ++face_index) {
        W8ReadMeshFace& face = faces[face_index];
        type = polygon_types.IndexOf(render_flags[face.material_index]);
        if (type < 0) {
            srAssertFail("iPolyType >=0",
                "C:\\Projects\\Wizardry 8\\Engine Code\\ReadMesh.cpp",
                0x45b, 0);
        }
        srVector3i& poly = polygon_vertices[type][polygon_counts[type]];
        srVector3i& shade = polygon_shades[type][polygon_counts[type]];
        for (int corner = 0; corner < 3; ++corner) {
            int original_vertex = face.vertices[corner];
            unsigned int key =
                (reinterpret_cast<unsigned int>(materials[face.material_index]) & 0xfff) |
                (original_vertex << 12);
            int vertex = vertex_indices[type].Lookup(&key) - 1;
            if (vertex == -1) {
                vertex = vertex_counts[type];
                int stored_vertex = vertex + 1;
                vertex_indices[type].Insert(&key, &stored_vertex);
                vertex_materials[type][vertex] = materials[face.material_index];
                vertex_uvs[type][vertex] = face.texture_coordinates[corner];
                reinterpret_cast<int*>(&poly)[corner] = vertex;
                (*vertex_maps)[type][vertex] = original_vertex;
                vertex_shades[type][vertex] = vertex;
                ++vertex_counts[type];
                for (int mapped = 0; mapped < mapped_values->count; ++mapped) {
                    if (mapped_values->data[mapped] == original_vertex) {
                        mapped_vertices.SetAt(mapped, static_cast<short>(vertex));
                        mapped_meshes.SetAt(mapped, static_cast<short>(type));
                        break;
                    }
                }
            }
            else if (vertex_uvs[type][vertex].x == face.texture_coordinates[corner].x &&
                     vertex_uvs[type][vertex].y == face.texture_coordinates[corner].y) {
                reinterpret_cast<int*>(&poly)[corner] = vertex;
            }
            else {
                reinterpret_cast<int*>(&poly)[corner] =
                    -1 - extra_uv_counts[type];
                extra_uvs[type][extra_uv_counts[type]] =
                    face.texture_coordinates[corner];
                ++extra_uv_counts[type];
            }
            reinterpret_cast<int*>(&shade)[corner] = vertex;
        }
        polygon_textures[type][polygon_counts[type]] =
            textures[face.material_index];
        ++polygon_counts[type];

        if (ReadMeshFaceNeedsSplit(face, materials)) {
            srVector3i& split_poly = polygon_vertices[type][polygon_counts[type]];
            srVector3i& split_shade = polygon_shades[type][polygon_counts[type]];
            for (int corner = 0; corner < 3; ++corner) {
                int source_vertex = reinterpret_cast<int*>(&shade)[corner];
                int vertex = vertex_counts[type];
                vertex_materials[type][vertex] = vertex_materials[type][source_vertex];
                vertex_shades[type][vertex] = vertex;
                (*vertex_maps)[type][vertex] = (*vertex_maps)[type][source_vertex];
                vertex_uvs[type][vertex] = face.texture_coordinates[corner];
                duplicated_from[type].Add(source_vertex);
                duplicated_to[type].Add(vertex);
                reinterpret_cast<int*>(&split_poly)[corner] = vertex;
                reinterpret_cast<int*>(&split_shade)[corner] = vertex;
                ++vertex_counts[type];
            }
            polygon_textures[type][polygon_counts[type]] =
                polygon_textures[type][polygon_counts[type] - 1];
            ++polygon_counts[type];
        }
    }

    for (type = 0; type < polygon_types.count; ++type) {
        if (capacities[type] == 0) {
            continue;
        }
        stMeshModel* model = new stMeshModel(
            polygon_counts[type], vertex_counts[type]);
        models[type] = model;
        if (model == 0) {
            srAssertFail("pstMeshModel[iCount]",
                "C:\\Projects\\Wizardry 8\\Engine Code\\ReadMesh.cpp",
                0x4e7, 0);
        }

        for (int mapped = 0; mapped < mapped_meshes.count; ++mapped) {
            if (mapped_meshes.data[mapped] == type) {
                for (int entry = 0; entry < mapped_values->count; ++entry) {
                    model->SetMappedVertex00471160(
                        mapped_vertices.data[entry], mapped_keys->data[entry]);
                }
                break;
            }
        }

        srPtr<srTextureIFace>* model_textures =
            model->getPolyTexture(0, 0, 1);
        for (int polygon = 0; polygon < polygon_counts[type]; ++polygon) {
            model_textures[polygon] = polygon_textures[type][polygon];
        }

        bool one_material = true;
        for (int vertex = 0; vertex < vertex_counts[type] - 1; ++vertex) {
            if (vertex_materials[type][vertex].get() !=
                vertex_materials[type][vertex + 1].get()) {
                one_material = false;
            }
        }
        if (one_material) {
            model->setMaterial(vertex_materials[type][0], 0,
                static_cast<srMeshModel::e_side>(0));
        }
        else {
            srPtr<srMaterialIFace>* model_materials =
                model->getVertexMaterial(0,
                    static_cast<srMeshModel::e_side>(0), 1);
            for (int vertex = 0; vertex < vertex_counts[type]; ++vertex) {
                model_materials[vertex] = vertex_materials[type][vertex];
                model_materials[vertex]->addReference();
            }
        }

        srVector3i* model_polygons = model->getPolyVertex();
        model->setUVCount(vertex_counts[type] + extra_uv_counts[type]);
        srVector2T<float>* model_uvs = model->getVertexTexCoords(0, 0, 1);
        memcpy(model_uvs, vertex_uvs[type],
            vertex_counts[type] * sizeof(srVector2T<float>));
        memcpy(model_uvs + vertex_counts[type], extra_uvs[type],
            extra_uv_counts[type] * sizeof(srVector2T<float>));
        srVector3i* model_uv_indices = model->getPolyUVIndex(0, 1);
        for (int uv_polygon = 0; uv_polygon < polygon_counts[type]; ++uv_polygon) {
            for (int uv_corner = 0; uv_corner < 3; ++uv_corner) {
                int index = reinterpret_cast<int*>(
                    polygon_vertices[type] + uv_polygon)[uv_corner];
                if (index < 0) {
                    index = vertex_counts[type] + (-1 - index);
                }
                reinterpret_cast<int*>(model_uv_indices + uv_polygon)[uv_corner] = index;
            }
        }
        unsigned long* model_shades = model->getVertexShadeIndex(1);
        memcpy(model_polygons, polygon_shades[type],
            polygon_counts[type] * sizeof(srVector3i));
        memcpy(model_shades, vertex_shades[type],
            vertex_counts[type] * sizeof(unsigned long));

        srShader shader;
        shader.value = 0x0100241b;
        CopyLevelDataHandle(reinterpret_cast<int*>(&shader),
            reinterpret_cast<int*>(polygon_types.data + type));
        model->setShader(shader, 0);

        free(polygon_shades[type]);
        free(polygon_vertices[type]);
        free(vertex_uvs[type]);
        free(extra_uvs[type]);
        delete[] vertex_materials[type];
        delete[] polygon_textures[type];
        free(vertex_shades[type]);

        if ((model->control_state_390 & 1) == 0) {
            unsigned long state = model->control_state_390;
            model->control_state_390 = state | 9;
            model->reindexPolygons(0);
        }
        if ((model->control_state_390 & 2) == 0) {
            model->control_state_390 |= 10;
        }
        if ((model->control_state_390 & 4) == 0) {
            model->control_state_390 |= 12;
        }
        model->control_state_390 |= 8;
        if ((polygon_types.data[type] & 0x6000) == 0x4000) {
            model->flags_3a0 |= 1;
            model->control_state_394 |= 0x40;
            model->control_state_390 |= 8;
        }
        else {
            model->flags_3a0 &= ~1U;
        }
        if (previous_model != 0) {
            previous_model->LinkTo(model);
            previous_model->Function5AA400(model);
        }
        previous_model = model;
        if (first_model == 0) {
            first_model = model;
        }
    }
    if ((first_model->control_state_390 & 1) == 0) {
        unsigned long state = first_model->control_state_390;
        first_model->control_state_390 = state | 9;
        first_model->reindexPolygons(0);
    }
    return first_model;
}

// FUNCTION: WIZ8 0x00487E10
int ReadMeshMaterials00487E10(
    W8ReadLevelInfo* info,
    srMaterialIFace*** materials,
    srTextureIFace*** textures,
    unsigned long** render_flags,
    int load_materials)
{
    if (info == 0) {
        srAssertFail("pInfo", "C:\\Projects\\Wizardry 8\\Engine Code\\ReadMesh.cpp", 0x256, 0);
    }
    if (info->bitmap_folder == 0) {
        srAssertFail("pInfo->strBitmapDir", "C:\\Projects\\Wizardry 8\\Engine Code\\ReadMesh.cpp", 0x257, 0);
    }
    if (info->world == 0) {
        srAssertFail("pInfo->pWorld", "C:\\Projects\\Wizardry 8\\Engine Code\\ReadMesh.cpp", 0x258, 0);
    }
    if (info->hFile == 0) {
        srAssertFail("pInfo->hFile", "C:\\Projects\\Wizardry 8\\Engine Code\\ReadMesh.cpp", 0x259, 0);
    }

    short count;
    short index;
    ReadVirtualFile(info->hFile, &count, sizeof(count), 0);
    if (count < 1) {
        return 0;
    }

    W8MaterialRecord004B8A70* records =
        static_cast<W8MaterialRecord004B8A70*>(
            malloc(count * sizeof(W8MaterialRecord004B8A70)));
    memset(records, 0, count * sizeof(W8MaterialRecord004B8A70));
    ReadVirtualFile(info->hFile, records, 0x11a, 0);
    if (records[0].version_00 < 4) {
        for (index = 1; index < count; ++index) {
            ReadVirtualFile(info->hFile, records + index, 0x11a, 0);
        }
    }
    else {
        ReadVirtualFile(info->hFile,
            reinterpret_cast<unsigned char*>(records) + 0x11a,
            0x10, 0);
        if (count > 1) {
            ReadVirtualFile(info->hFile, records + 1,
                (count - 1) * sizeof(W8MaterialRecord004B8A70), 0);
        }
    }

    for (index = 0; index < count; ++index) {
        NormalizeMaterialRecord00489980(records + index);
    }

    if (g_read_mesh_scratch_count_65b9f8 == count &&
        memcmp(records, g_read_mesh_material_records_65b9f4,
               count * sizeof(W8MaterialRecord004B8A70)) == 0) {
        *materials = g_read_mesh_materials_65b9e8;
        *textures = g_read_mesh_textures_65b9ec;
        *render_flags = g_read_mesh_render_flags_65b9f0;
        free(records);
        return count;
    }

    ReleaseReadMeshScratch004881D0();
    *materials = static_cast<srMaterialIFace**>(
        malloc(count * sizeof(**materials)));
    *textures = static_cast<srTextureIFace**>(
        malloc(count * sizeof(**textures)));
    *render_flags = static_cast<unsigned long*>(
        malloc(count * sizeof(**render_flags)));
    memset(*materials, 0, count * sizeof(**materials));
    memset(*textures, 0, count * sizeof(**textures));
    memset(*render_flags, 0, count * sizeof(**render_flags));

    g_read_mesh_materials_65b9e8 = *materials;
    g_read_mesh_textures_65b9ec = *textures;
    g_read_mesh_render_flags_65b9f0 = *render_flags;
    g_read_mesh_material_records_65b9f4 = records;
    g_read_mesh_scratch_count_65b9f8 = count;

    for (index = 0; index < count; ++index) {
        if (index == 0) {
            CreateDefaultMaterial004B9280(
                *materials + index, *textures + index,
                *render_flags + index);
        }
        else {
            LoadMaterial004B8A70(
                info->bitmap_folder, records + index,
                *materials + index, *textures + index,
                *render_flags + index, load_materials);
        }
    }
    return count;
}

// FUNCTION: WIZ8 0x00485B20
unsigned char ReadSingleLevelMesh00485B20(
    W8ReadLevelInfo* info, srModelInstance** instance,
    int positional_0, int positional_1, const char* name,
    unsigned char load_materials)
{
    if (name != 0) {
        srRegistry* registry = srCore.getRegistry();
        srRegistry::ClassNode* node = registry->getClassNode(0x10003);
        if (node == 0) {
            node = registry->registerClass(
                "stMeshModel", srMeshModel::sGetClassNode(), 0x10003, 0);
        }

        stMeshModel* model = static_cast<stMeshModel*>(
            registry->find(node, name, 0));
        if (model != 0 && model->flag_3cc != 0) {
            stModelInstance005EC7D0* duplicate =
                CreateModelInstance0046F5C0(model);
            duplicate->setName("Read Mesh Duplicate Instance");
            *instance = duplicate;
            SkipSingleLevelMesh00487BD0(info);
            return 1;
        }
    }

    return ReadSingleLevelMeshBody00485C10(
        info, instance, positional_0, positional_1, name, load_materials);
}

// FUNCTION: WIZ8 0x00485C10
unsigned char ReadSingleLevelMeshBody00485C10(
    W8ReadLevelInfo* info, srModelInstance** instance,
    int positional_0, int positional_1, const char* name,
    unsigned char load_materials)
{
    W8GrowableVector<short> mapped_values;
    W8GrowableVector<short> mapped_keys;
    int version = 0;
    int vertex_count = 0;
    int face_count = 0;
    unsigned char flags = 0;
    unsigned int bytes_read;
    srVector3T<float>* vertices = 0;
    short** compressed_vertices = 0;
    short frame_count = 0;
    float compression_scale = 1000.0f;
    W8ReadMeshFace* faces = 0;
    int** vertex_maps = 0;
    unsigned int mesh_count = 0;
    unsigned int vertex_map_count = 0;
    stMeshModel* first_model = 0;

    (void)positional_0;
    (void)positional_1;

    if (info == 0) {
        srAssertFail("pInfo", "C:\\Projects\\Wizardry 8\\Engine Code\\ReadMesh.cpp", 0xd7, 0);
    }
    int file = info->hFile;
    if (file == 0) {
        srAssertFail("hFile", "C:\\Projects\\Wizardry 8\\Engine Code\\ReadMesh.cpp", 0xd9, 0);
    }

    ReadVirtualFile(file, &version, sizeof(version), 0);
    ReadVirtualFile(file, &vertex_count, sizeof(vertex_count), 0);
    unsigned char success =
        ReadVirtualFile(file, &face_count, sizeof(face_count), 0);
    if (vertex_count < 1 || face_count < 1) {
        return 0;
    }

    srVector3T<float> location;
    srVector3T<float> scale;
    srMatrix3T<float> rotation;
    if (version > 2) {
        success = ReadVirtualFile(file, &flags, sizeof(flags), 0);
    }
    if (version > 1) {
        ReadMeshTransform004896C0(file, &location, &rotation, &scale);
    }
    if (success == 0) {
        return 0;
    }

    if (version > 3) {
        signed char mapping_count;
        success = ReadVirtualFile(file, &mapping_count, sizeof(mapping_count), 0);
        for (short index = 0; index < mapping_count; ++index) {
            short value;
            short key;
            if (success == 0 ||
                !ReadVirtualFile(file, &value, sizeof(value), 0) ||
                !ReadVirtualFile(file, &key, sizeof(key), 0)) {
                success = 0;
            }
            mapped_values.Add(value);
            mapped_keys.Add(key);
        }
    }
    if (success == 0) {
        return 0;
    }

    if ((flags & 1) == 0) {
        vertices = static_cast<srVector3T<float>*>(
            malloc(vertex_count * sizeof(*vertices)));
        if (vertices == 0) {
            srAssertFail("pstVertices", "C:\\Projects\\Wizardry 8\\Engine Code\\ReadMesh.cpp", 0x139, 0);
        }
        success = ReadVirtualFile(
            file, vertices, vertex_count * sizeof(*vertices), &bytes_read);
        if (success == 0) {
            srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\ReadMesh.cpp", 0x13b, 0);
        }
        for (int index = 0; index < vertex_count; ++index) {
            vertices[index].x *= 500.0f;
            vertices[index].y *= 500.0f;
            vertices[index].z *= 500.0f;
        }
    }
    else {
        unsigned char compression_type;
        ReadVirtualFile(file, &compression_type, sizeof(compression_type), 0);
        ReadVirtualFile(file, &frame_count, sizeof(frame_count), 0);
        if (compression_type == 2) {
            ReadVirtualFile(
                file, &compression_scale, sizeof(compression_scale), 0);
        }
        if ((flags & 2) == 0) {
            srAssertFail("FALSE", "C:\\Projects\\Wizardry 8\\Engine Code\\ReadMesh.cpp", 0x131,
                "Uncompressed mesh, please re-export level.");
        }
        else {
            compressed_vertices = new short*[frame_count];
            if (compressed_vertices == 0) {
                srAssertFail("ppCompVertices", "C:\\Projects\\Wizardry 8\\Engine Code\\ReadMesh.cpp", 0x10e, 0);
            }
            for (short frame = 0; frame < frame_count; ++frame) {
                compressed_vertices[frame] = static_cast<short*>(
                    malloc(vertex_count * 3 * sizeof(short)));
                if (compressed_vertices[frame] == 0) {
                    srAssertFail("ppCompVertices[i]", "C:\\Projects\\Wizardry 8\\Engine Code\\ReadMesh.cpp", 0x114, 0);
                }
                success = ReadVirtualFile(file, compressed_vertices[frame],
                    vertex_count * 3 * sizeof(short), &bytes_read);
                if (success == 0) {
                    srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\ReadMesh.cpp", 0x118, 0);
                }
            }
        }
    }

    faces = static_cast<W8ReadMeshFace*>(
        malloc(face_count * sizeof(*faces)));
    if (faces == 0) {
        srAssertFail("pstFaces", "C:\\Projects\\Wizardry 8\\Engine Code\\ReadMesh.cpp", 0x148, 0);
    }
    if ((flags & 4) == 0) {
        success = ReadVirtualFile(
            file, faces, face_count * sizeof(*faces), &bytes_read);
        if (success == 0) {
            srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\ReadMesh.cpp", 0x166, 0);
        }
    }
    else {
        W8CompressedReadMeshFace* compressed_faces =
            static_cast<W8CompressedReadMeshFace*>(
                malloc(face_count * sizeof(*compressed_faces)));
        if (compressed_faces == 0) {
            srAssertFail("pCompPoly", "C:\\Projects\\Wizardry 8\\Engine Code\\ReadMesh.cpp", 0x14e, 0);
        }
        success = ReadVirtualFile(file, compressed_faces,
            face_count * sizeof(*compressed_faces), &bytes_read);
        if (success == 0) {
            srAssertFail("fSuccess", "C:\\Projects\\Wizardry 8\\Engine Code\\ReadMesh.cpp", 0x152, 0);
        }
        for (int index = 0; index < face_count; ++index) {
            for (int vertex = 0; vertex < 3; ++vertex) {
                faces[index].vertices[vertex] =
                    compressed_faces[index].vertices[vertex];
                faces[index].texture_coordinates[vertex] =
                    compressed_faces[index].texture_coordinates[vertex];
            }
            faces[index].material_index =
                compressed_faces[index].material_index;
            faces[index].flags = compressed_faces[index].flags;
        }
        free(compressed_faces);
    }

    srMaterialIFace** materials;
    srTextureIFace** textures;
    unsigned long* render_flags;
    int material_count = ReadMeshMaterials00487E10(
        info, &materials, &textures, &render_flags, load_materials);
    if (materials == 0) {
        srAssertFail("ppsrMats", "C:\\Projects\\Wizardry 8\\Engine Code\\ReadMesh.cpp", 0x16b, 0);
    }
    if (material_count == 0) {
        srAssertFail("uiMatCount", "C:\\Projects\\Wizardry 8\\Engine Code\\ReadMesh.cpp", 0x16c, 0);
    }

    first_model = BuildSingleLevelMesh00488650(
        face_count, faces, vertex_count, material_count,
        materials, textures, render_flags, &mesh_count,
        &vertex_maps, &vertex_map_count, &mapped_values, &mapped_keys);
    if (first_model != 0) {
        first_model->autoRelease();
        first_model->setName(name);
        stModelInstance005EC7D0* loaded_instance =
            CreateModelInstance0046F5C0(first_model);
        loaded_instance->setName("ReadSTMeshFromFile");
        if (version > 1 && loaded_instance != 0) {
            const double angle = 3.14159265358979323846;
            const float cosine = static_cast<float>(cos(angle));
            const float sine = static_cast<float>(sin(angle));
            srVector3T<float> first(1.0f, 0.0f, 0.0f);
            srVector3T<float> second(0.0f, cosine, -sine);
            srVector3T<float> third(0.0f, sine, cosine);
            srMatrix3T<float> conversion;
            conversion.method_004219F0(first, second, third);
            rotation.method_00421A40(conversion);
            srVector3T<double> translated(
                location.x * 500.0, location.y * 500.0,
                location.z * 500.0);
            loaded_instance->setLocation(translated);
            loaded_instance->setRotation(rotation);
        }
        *instance = loaded_instance;
        first_model->flag_3cc =
            MeshHasAnimatedTexture004B9AA0(first_model) ? 0 : 1;
    }

    if ((flags & 1) == 0) {
        int mesh_index = 0;
        for (stMeshModel* model = first_model;
             model != 0; model = model->next, ++mesh_index) {
            srVector3T<float>* model_vertices = model->getVertexLoc();
            for (int index = 0;
                 index < model->vertex_location_count_22c; ++index) {
                model_vertices[index] =
                    vertices[vertex_maps[mesh_index][index]];
            }
            UpdateMeshAfterVertexLoad004867F0(model, -1);
        }
    }
    else {
        for (short frame = 0; frame < frame_count; ++frame) {
            int mesh_index = 0;
            for (stMeshModel* model = first_model;
                 model != 0; model = model->next, ++mesh_index) {
                model->InitializeVertexFrames00473B00(frame_count);
                model->vertex_compression_scale_444 =
                    500.0f / compression_scale;
                short* model_vertices = static_cast<short*>(
                    model->GetVertex(frame));
                for (int index = 0;
                     index < model->vertex_location_count_22c; ++index) {
                    int source = vertex_maps[mesh_index][index];
                    model_vertices[index * 3] =
                        compressed_vertices[frame][source * 3];
                    model_vertices[index * 3 + 1] =
                        compressed_vertices[frame][source * 3 + 1];
                    model_vertices[index * 3 + 2] =
                        compressed_vertices[frame][source * 3 + 2];
                }
                model->FinalizeVertexFrame00473180(frame);
            }
        }
    }

    for (int index = 0; index < material_count; ++index) {
        srClass* material = static_cast<srClass*>(materials[index]);
        if (material != 0 && material->getReferenceCount() == 0) {
            int required = g_retained_material_count_65b9d4 + 1;
            if (required > g_retained_material_capacity_65b9d8) {
                srClass** previous = g_retained_materials_65b9dc;
                srClass** replacement = new srClass*[required];
                if (replacement != 0) {
                    g_retained_material_capacity_65b9d8 = required;
                    for (int retained_index = 0;
                         retained_index < g_retained_material_count_65b9d4;
                         ++retained_index) {
                        replacement[retained_index] = previous[retained_index];
                    }
                    delete[] previous;
                    g_retained_materials_65b9dc = replacement;
                }
            }
            g_retained_materials_65b9dc[
                g_retained_material_count_65b9d4++] = material;
            material->addReference();
        }
    }

    for (unsigned int map_index = 0;
         map_index < vertex_map_count; ++map_index) {
        free(vertex_maps[map_index]);
    }
    free(vertex_maps);
    if (compressed_vertices == 0) {
        free(vertices);
    }
    else {
        for (short frame = 0; frame < frame_count; ++frame) {
            free(compressed_vertices[frame]);
        }
        delete[] compressed_vertices;
    }
    free(faces);

    if (first_model != 0) {
        srVector3T<float> minimum;
        srVector3T<float> maximum;
        first_model->getBoundingBox(minimum, maximum);
    }
    return 1;
}

// FUNCTION: WIZ8 0x00488240
unsigned char ReadMultipleLevelMeshes00488240(
    W8ReadLevelInfo* info, srModelInstance** instances,
    unsigned long count, const char* name)
{
    OctMeshModel reader;
    unsigned int mesh_count;
    unsigned int root_count;
    int terminator;

    if (info == 0) {
        srAssertFail("pInfo", "C:\\Projects\\Wizardry 8\\Engine Code\\ReadMesh.cpp", 0x31e, 0);
    }
    if (count == 0) {
        srAssertFail("uiNumMeshes", "C:\\Projects\\Wizardry 8\\Engine Code\\ReadMesh.cpp", 0x31f, 0);
    }
    if (info->hFile == 0) {
        srAssertFail("hFile", "C:\\Projects\\Wizardry 8\\Engine Code\\ReadMesh.cpp", 0x323, 0);
    }

    unsigned char success = ReadVirtualFile(
        info->hFile, &mesh_count, sizeof(mesh_count), 0);
    if (success != 0) {
        ReadVirtualFile(info->hFile, &root_count, sizeof(root_count), 0);
    }

    g_read_mesh_material_count_65b9cc = ReadMeshMaterials00487E10(
        info, &g_multi_mesh_materials_65ba00,
        &g_multi_mesh_textures_65b9fc,
        &g_multi_mesh_render_flags_65ba04, 1);
    if (g_read_mesh_material_count_65b9cc == 0) {
        srAssertFail("uiMatCount", "C:\\Projects\\Wizardry 8\\Engine Code\\ReadMesh.cpp", 0x329, 0);
    }

    ReadVirtualFile(info->hFile, &terminator, sizeof(terminator), 0);
    if (terminator != -1) {
        srAssertFail("(uiTerminator == 0xffffffff)",
            "C:\\Projects\\Wizardry 8\\Engine Code\\ReadMesh.cpp", 0x32c,
            "NewReadMesh: Material list length is incorrect.");
    }
    if (count != mesh_count) {
        srAssertFail("(uiNumMeshes == uiMeshNum)",
            "C:\\Projects\\Wizardry 8\\Engine Code\\ReadMesh.cpp", 0x32e,
            "NewReadMesh: Mismatch in mesh count between .oct and .pvl files.");
    }

    stMeshModel** meshes = static_cast<stMeshModel**>(
        malloc(count * sizeof(*meshes)));
    srVector3T<float> minimum;
    srVector3T<float> maximum;
    for (g_read_mesh_index_65b9e4 = 0;
         g_read_mesh_index_65b9e4 < mesh_count;
         ++g_read_mesh_index_65b9e4) {
        if (g_screen_state_0068ec78.id == 4) {
            UpdatePleaseWaitLoadFrame005915A0();
        }
        stMeshModel* model = reader.Read0049E9A0(
            info->hFile, g_multi_mesh_materials_65ba00,
            g_multi_mesh_textures_65b9fc,
            g_multi_mesh_render_flags_65ba04, meshes,
            g_read_mesh_material_count_65b9cc);
        meshes[g_read_mesh_index_65b9e4] = model;
        if ((model->control_state_390 & 1) == 0) {
            unsigned long state = model->control_state_390;
            model->control_state_390 = state | 1;
            model->control_state_390 = state | 9;
            model->reindexPolygons(0);
        }
        model->getBoundingBox(minimum, maximum);
    }

    for (g_read_mesh_index_65b9e4 = 0;
         g_read_mesh_index_65b9e4 < root_count;
         ++g_read_mesh_index_65b9e4) {
        stMeshModel* model = meshes[g_read_mesh_index_65b9e4];
        model->setName(name);
        if (model->next == 0) {
            stModelInstance005EC7D0* instance =
                CreateModelInstance0046F5C0(model);
            instance->setName("Multi Mesh Instance");
            instance->state_17c = g_read_mesh_index_65b9e4;
            model->flag_3cc = MeshHasAnimatedTexture004B9AA0(model);
            instances[g_read_mesh_index_65b9e4] = instance;
        }
    }

    ReadVirtualFile(info->hFile, &terminator, sizeof(terminator), 0);
    if (terminator != -1) {
        srAssertFail("(uiTerminator == 0xffffffff)",
            "C:\\Projects\\Wizardry 8\\Engine Code\\ReadMesh.cpp", 0x35b,
            "NewReadMesh: Incorrect offset in file at end of mesh.");
    }

    for (int index = 0; index < g_read_mesh_material_count_65b9cc; ++index) {
        srClass* material = static_cast<srClass*>(
            g_multi_mesh_materials_65ba00[index]);
        if (material != 0 && material->getReferenceCount() == 0) {
            int required = g_retained_material_count_65b9d4 + 1;
            if (required > g_retained_material_capacity_65b9d8) {
                srClass** previous = g_retained_materials_65b9dc;
                srClass** replacement = new srClass*[required];
                if (replacement != 0) {
                    g_retained_material_capacity_65b9d8 = required;
                    for (int retained_index = 0;
                         retained_index < g_retained_material_count_65b9d4;
                         ++retained_index) {
                        replacement[retained_index] = previous[retained_index];
                    }
                    delete[] previous;
                    g_retained_materials_65b9dc = replacement;
                }
            }
            g_retained_materials_65b9dc[
                g_retained_material_count_65b9d4++] = material;
            material->addReference();
        }
    }

    free(meshes);
    return 1;
}

// FUNCTION: WIZ8 0x004881d0
void ReleaseReadMeshScratch004881D0() {
  if (g_read_mesh_materials_65b9e8 != 0) {
    free(g_read_mesh_materials_65b9e8);
    g_read_mesh_materials_65b9e8 = 0;
  }
  if (g_read_mesh_textures_65b9ec != 0) {
    free(g_read_mesh_textures_65b9ec);
    g_read_mesh_textures_65b9ec = 0;
  }
  if (g_read_mesh_render_flags_65b9f0 != 0) {
    free(g_read_mesh_render_flags_65b9f0);
    g_read_mesh_render_flags_65b9f0 = 0;
  }
  if (g_read_mesh_material_records_65b9f4 != 0) {
    free(g_read_mesh_material_records_65b9f4);
    g_read_mesh_material_records_65b9f4 = 0;
  }
  g_read_mesh_scratch_count_65b9f8 = 0;
}

// FUNCTION: WIZ8 0x00487bd0
unsigned char SkipSingleLevelMesh00487BD0(W8ReadLevelInfo *info) {
  int version;
  int vertex_count;
  int face_count;
  unsigned char flags = 0;
  unsigned char count;
  short item_count;
  short index;
  unsigned char success = 1;

  if (info == 0 || info->world == 0 || info->hFile == 0) {
    return 0;
  }
  if (!ReadVirtualFile(info->hFile, &version, 4, 0) ||
      !ReadVirtualFile(info->hFile, &vertex_count, 4, 0) ||
      !ReadVirtualFile(info->hFile, &face_count, 4, 0)) {
    return 0;
  }
  if (vertex_count < 1 || face_count < 1) {
    return 0;
  }
  if (version > 2) {
    success = ReadVirtualFile(info->hFile, &flags, 1, 0);
  }
  if (version > 1) {
    FileSeek(info->hFile, 0x28, FILE_SEEK_FROM_CURRENT);
  }
  if (version > 3) {
    if (success == 0 || !ReadVirtualFile(info->hFile, &count, 1, 0)) {
      success = 0;
    }
    if (count != 0) {
      FileSeek(info->hFile,
               static_cast<int>(static_cast<signed char>(count)) * 4,
               FILE_SEEK_FROM_CURRENT);
    }
  }
  if ((flags & 1) == 0) {
    vertex_count *= 0xc;
  } else {
    unsigned char ignored;
    short group_count;

    ReadVirtualFile(info->hFile, &ignored, 1, 0);
    ReadVirtualFile(info->hFile, &group_count, 2, 0);
    if ((flags & 2) == 0) {
      vertex_count = group_count * vertex_count * 0xc;
    } else {
      vertex_count = group_count * vertex_count * 6;
    }
  }
  FileSeek(info->hFile, vertex_count, FILE_SEEK_FROM_CURRENT);
  if ((flags & 4) == 0) {
    face_count *= 0x29;
  } else {
    face_count *= 0x21;
  }
  FileSeek(info->hFile, face_count, FILE_SEEK_FROM_CURRENT);
  if (ReadVirtualFile(info->hFile, &item_count, 2, 0) && item_count > 0) {
    for (index = 0; index < item_count; ++index) {
      ReadVirtualFile(info->hFile, &count, 1, 0);
      FileSeek(info->hFile, 0x119, FILE_SEEK_FROM_CURRENT);
      if (count > 3) {
        FileSeek(info->hFile, 0x10, FILE_SEEK_FROM_CURRENT);
      }
    }
  }
  if ((flags & 1) != 0) {
    success = 2;
  }
  return success;
}
