#include "wiz8/gameplay_boundaries.h"
#include "wiz8/mesh_model.h"
#include "wiz8/sr_api.h"
#include "surrender/srCore.h"
#include "surrender/srGERD.h"
#include "surrender/srTriangleCuller.h"
#include "surrender/srTriMeshPipeline.h"
#include "surrender/srTypeRegistry.h"
#include "surrender/srVectorProcessor.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>

#define ST_MESH_MODEL_CPP \
    "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp"

/*
 * Engine Code\stMeshModel.cpp.
 *
 * A mesh model and the sibling chain it can be linked into. The shared
 * srTriMeshPipeline singleton's method bodies are emitted in this object as
 * well: its vtable sits immediately after the mesh-model vector vftables.
 */

extern void Function4729F0(void* model);

unsigned long g_mesh_frame_bytes_0065a0e8;
unsigned long g_mesh_frame_budget_00609d34 = 0x00800000;
float g_mesh_normal_values_00659ce8[256];
int g_mesh_model_count_00659cbc;
stMeshModel** g_mesh_models_00659cc4;

unsigned char EnsureMeshFrameMemory00473BF0(unsigned long bytes);

// FUNCTION: WIZ8 0x004712d0
int stMeshModel::FindMappedIndex(short key)
{
    if (key < 0) {
        return -1;
    }
    int index = mapped_keys.IndexOf(key);
    if (index != -1) {
        return *mapped_values.GetAt(index);
    }
    return -1;
}

/* Link one model onto another, setting both ends - so the two pointers are one
   link rather than two independent fields. Unlinking passes nothing. */
// FUNCTION: WIZ8 0x00471d60
void stMeshModel::LinkTo(stMeshModel* other)
{
    next = other;
    if (other != 0) {
        other->previous = this;
    }
}

/* One vertex, bounds-checked against the model's own count and refused
   outright when there is no table at all. */
// FUNCTION: WIZ8 0x00471aa0
void* stMeshModel::GetVertex(unsigned int index)
{
    if (compressed_locations_3e0 != 0 && index < frame_count_3d0) {
        return compressed_locations_3e0[index];
    }
    return 0;
}

// FUNCTION: WIZ8 0x00471720
unsigned char stMeshModel::PrepareFrame00471720(
    unsigned int frame, int channels)
{
    if ((channels & 1) != 0 && frame_locations_3d4[frame] == 0) {
        unsigned long bytes = vertex_location_count_22c * sizeof(srVector3T<float>);
        unsigned long total = g_mesh_frame_bytes_0065a0e8 + bytes;
        if (total >= g_mesh_frame_budget_00609d34) {
            if (!EnsureMeshFrameMemory00473BF0(bytes)) {
                return 0;
            }
            total = g_mesh_frame_bytes_0065a0e8 + bytes;
        }
        g_mesh_frame_bytes_0065a0e8 = total;
        frame_locations_3d4[frame] = static_cast<srVector3T<float>*>(
            srHeap.allocate(bytes));
        if (frame_locations_3d4[frame] == 0) {
            srAssertFail(
                "m_pVertexLoc[uiFrame]", ST_MESH_MODEL_CPP, 0x243, 0);
        }
    }
    if ((channels & 2) != 0 && frame_normals_3d8[frame] == 0) {
        unsigned long bytes = vertex_location_count_22c * sizeof(srVector3T<float>);
        unsigned long total = g_mesh_frame_bytes_0065a0e8 + bytes;
        if (total >= g_mesh_frame_budget_00609d34) {
            if (!EnsureMeshFrameMemory00473BF0(bytes)) {
                return 0;
            }
            total = g_mesh_frame_bytes_0065a0e8 + bytes;
        }
        g_mesh_frame_bytes_0065a0e8 = total;
        frame_normals_3d8[frame] = static_cast<srVector3T<float>*>(
            srHeap.allocate(bytes));
        if (frame_normals_3d8[frame] == 0) {
            srAssertFail(
                "m_pVertexNormal[uiFrame]", ST_MESH_MODEL_CPP, 0x24e, 0);
        }
    }
    if ((channels & 4) != 0 && frame_values_3dc[frame] == 0) {
        unsigned long bytes = polygon_count_230 * sizeof(srVector3T<float>);
        unsigned long total = g_mesh_frame_bytes_0065a0e8 + bytes;
        if (total >= g_mesh_frame_budget_00609d34) {
            if (!EnsureMeshFrameMemory00473BF0(bytes)) {
                return 0;
            }
            total = g_mesh_frame_bytes_0065a0e8 + bytes;
        }
        g_mesh_frame_bytes_0065a0e8 = total;
        frame_values_3dc[frame] = static_cast<srVector3T<float>*>(
            srHeap.allocate(bytes));
        if (frame_values_3dc[frame] == 0) {
            srAssertFail(
                "m_pPolyNormal[uiFrame]", ST_MESH_MODEL_CPP, 0x259, 0);
        }
    }
    return 1;
}

// FUNCTION: WIZ8 0x00471930
unsigned char stMeshModel::FinalizeFrame00471930(
    unsigned int frame, int channels, void* values)
{
    srVector3T<float>* destination =
        static_cast<srVector3T<float>*>(values);
    if ((channels & 1) != 0) {
        short* source = compressed_locations_3e0[frame];
        for (int index = 0; index < vertex_location_count_22c; ++index) {
            destination[index].x = source[index * 3] * compression_scale_444;
            destination[index].y = source[index * 3 + 1] * compression_scale_444;
            destination[index].z = source[index * 3 + 2] * compression_scale_444;
        }
        return 1;
    }
    if ((channels & 2) != 0) {
        unsigned char* source = compressed_normals_3e4[frame];
        for (int index = 0; index < vertex_location_count_22c; ++index) {
            destination[index].x = g_mesh_normal_values_00659ce8[source[index * 3]];
            destination[index].y = g_mesh_normal_values_00659ce8[source[index * 3 + 1]];
            destination[index].z = g_mesh_normal_values_00659ce8[source[index * 3 + 2]];
        }
        return 1;
    }
    if ((channels & 4) != 0) {
        unsigned char* source = compressed_values_3e8[frame];
        for (int index = 0; index < polygon_count_230; ++index) {
            destination[index].x = g_mesh_normal_values_00659ce8[source[index * 3]];
            destination[index].y = g_mesh_normal_values_00659ce8[source[index * 3 + 1]];
            destination[index].z = g_mesh_normal_values_00659ce8[source[index * 3 + 2]];
        }
        return 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x00471AD0
srVector3T<float>* stMeshModel::GetVertexLocations00471AD0(
    unsigned int frame, char load, float interpolation)
{
    if (frame_locations_3d4 == 0) {
        return 0;
    }
    if (interpolation > 0.0f && frame < frame_count_3d0 - 1) {
        unsigned int next_frame = (frame + 1) % frame_count_3d0;
        if (lerp_buffer_448 == 0) {
            lerp_buffer_448 = static_cast<srVector3T<float>*>(
                srHeap.allocate(
                    vertex_location_count_22c * sizeof(srVector3T<float>)));
            if (lerp_buffer_448 == 0) {
                srAssertFail("m_pLerpBuffer", ST_MESH_MODEL_CPP, 0x2d4, 0);
            }
        }
        if (frame_locations_3d4[frame] == 0) {
            PrepareFrame00471720(frame, 1);
            FinalizeFrame00471930(
                frame, 1, frame_locations_3d4[frame]);
        }
        if (frame_locations_3d4[next_frame] == 0) {
            PrepareFrame00471720(next_frame, 1);
            FinalizeFrame00471930(
                next_frame, 1, frame_locations_3d4[next_frame]);
        }
        if (lerp_buffer_448 != 0 &&
            frame_locations_3d4[next_frame] != 0 &&
            frame_locations_3d4[frame] != 0 &&
            vertex_location_count_22c != 0) {
            if (interpolation == 1.0f) {
                if (lerp_buffer_448 != frame_locations_3d4[next_frame]) {
                    srVectorProcessor::memoryCopy(
                        lerp_buffer_448,
                        frame_locations_3d4[next_frame],
                        vertex_location_count_22c * sizeof(srVector3T<float>));
                }
            }
            else {
                srVectorProcessor::lerp(
                    reinterpret_cast<float*>(lerp_buffer_448),
                    reinterpret_cast<const float*>(
                        frame_locations_3d4[next_frame]),
                    reinterpret_cast<const float*>(frame_locations_3d4[frame]),
                    interpolation,
                    vertex_location_count_22c * 3);
            }
        }
        return lerp_buffer_448;
    }
    if (frame_locations_3d4[frame] == 0) {
        PrepareFrame00471720(frame, 1);
        if (load != 0 && frame_locations_3d4[frame] != 0) {
            FinalizeFrame00471930(
                frame, 1, frame_locations_3d4[frame]);
        }
    }
    return frame_locations_3d4[frame];
}

// FUNCTION: WIZ8 0x004739E0
unsigned long stMeshModel::ReleaseFrames004739E0()
{
    unsigned long released = 0;
    if (frame_locations_3d4 != 0) {
        for (unsigned int frame = 0; frame < frame_count_3d0; ++frame) {
            if (frame_locations_3d4[frame] != 0) {
                released +=
                    vertex_location_count_22c * sizeof(srVector3T<float>);
                srHeap.free(frame_locations_3d4[frame]);
                frame_locations_3d4[frame] = 0;
            }
        }
    }
    if (frame_normals_3d8 != 0) {
        for (unsigned int frame = 0; frame < frame_count_3d0; ++frame) {
            if (frame_normals_3d8[frame] != 0) {
                released +=
                    vertex_location_count_22c * sizeof(srVector3T<float>);
                srHeap.free(frame_normals_3d8[frame]);
                frame_normals_3d8[frame] = 0;
            }
        }
    }
    if (frame_values_3dc != 0) {
        for (unsigned int frame = 0; frame < frame_count_3d0; ++frame) {
            if (frame_values_3dc[frame] != 0) {
                released += polygon_count_230 * sizeof(srVector3T<float>);
                srHeap.free(frame_values_3dc[frame]);
                frame_values_3dc[frame] = 0;
            }
        }
    }
    last_frame_use_440 = GetTickCount();
    g_mesh_frame_bytes_0065a0e8 -= released;
    return released;
}

// FUNCTION: WIZ8 0x00473BF0
unsigned char EnsureMeshFrameMemory00473BF0(unsigned long bytes)
{
    unsigned long released = 0;
    bool exhausted = false;
    while (released < bytes && !exhausted) {
        stMeshModel* oldest = 0;
        unsigned long oldest_time = 0xffffffffUL;
        if (g_mesh_model_count_00659cbc < 1) {
            exhausted = true;
            continue;
        }
        for (int index = 0; index < g_mesh_model_count_00659cbc; ++index) {
            stMeshModel* model = g_mesh_models_00659cc4[index];
            if (model == 0) {
                srAssertFail("pstModel", ST_MESH_MODEL_CPP, 0x712, 0);
            }
            if (model->last_frame_use_440 < oldest_time) {
                oldest = model;
                oldest_time = model->last_frame_use_440;
            }
        }
        if (oldest == 0) {
            exhausted = true;
        }
        else {
            released += oldest->ReleaseFrames004739E0();
        }
    }
    return exhausted ? 0 : 1;
}

/* Build the active-polygon table once for either the base texture set or a
   named skin. Polygons whose texture name begins with "blank" are excluded;
   a null result means every polygon remains active and needs no table. */
// FUNCTION: WIZ8 0x00473CD0
unsigned long* stMeshModel::GetPolygonTable00473CD0(
    int* count, int texture_table, char create)
{
    int skin = skin_table_ids.IndexOf(texture_table);
    unsigned long* table;
    unsigned char checked;

    if (skin >= 0) {
        if (skin_blanking_apt_458 == 0) {
            *count = 0;
            return 0;
        }
        table = *skin_blanking_apt_458->GetAt(skin);
        *count = *skin_blanking_apt_number_45c->GetAt(skin);
        checked = *skin_blanking_checked_460->GetAt(skin);
    }
    else {
        table = blanking_apt_44c;
        *count = blanking_apt_number_450;
        checked = blanking_checked_454;
    }

    if (table == 0 && checked == 0 && create != 0) {
        *count = 0;
        table = new unsigned long[polygon_count_230];

        srPtr<srTextureIFace>* textures;
        if (skin < 0) {
            blanking_checked_454 = 1;
            textures = getPolyTexture(0, 0, 0);
        }
        else {
            *skin_blanking_checked_460->GetAt(skin) = 1;
            textures = *skin_texture_tables.GetAt(skin);
        }

        if (textures != 0) {
            for (int polygon = 0; polygon < polygon_count_230; ++polygon) {
                srTextureIFace* texture = textures[polygon].get();
                if (texture == 0) {
                    table[(*count)++] = polygon;
                }
                else {
                    char name[256];
                    strcpy(name, texture->getName());
                    if (_strnicmp(name, "blank", 5) != 0) {
                        table[(*count)++] = polygon;
                    }
                }
            }
            if (*count != polygon_count_230) {
                if (skin >= 0) {
                    *skin_blanking_apt_458->GetAt(skin) = table;
                    *skin_blanking_apt_number_45c->GetAt(skin) = *count;
                }
                else {
                    blanking_apt_44c = table;
                    blanking_apt_number_450 = *count;
                }
                return table;
            }
        }
        delete[] table;
    }

    *count = 0;
    return 0;
}

// FUNCTION: WIZ8 0x00472990
void stMeshModel::SetAmbientColor00472990(
    const srVector3T<float>& color)
{
    if (ambient_color_3a4.x != color.x ||
        ambient_color_3a4.y != color.y ||
        ambient_color_3a4.z != color.z) {
        ambient_color_3a4 = color;
        flags_3a0 |= 2;
    }
}

// FUNCTION: WIZ8 0x00471CA0
srVector3T<float>* stMeshModel::GetVertexNormals00471CA0(
    unsigned int frame, char load)
{
    if (frame_normals_3d8 == 0) {
        return 0;
    }
    if (frame_normals_3d8[frame] == 0) {
        PrepareFrame00471720(frame, 2);
        if (load != 0 && frame_normals_3d8[frame] != 0) {
            FinalizeFrame00471930(frame, 2, frame_normals_3d8[frame]);
        }
    }
    return frame_normals_3d8[frame];
}

// FUNCTION: WIZ8 0x00471D00
void* stMeshModel::GetFrameValues00471D00(unsigned int frame, char load)
{
    if (frame_values_3dc == 0) {
        return 0;
    }
    if (frame_values_3dc[frame] == 0) {
        PrepareFrame00471720(frame, 4);
        if (load != 0 && frame_values_3dc[frame] != 0) {
            FinalizeFrame00471930(frame, 4, frame_values_3dc[frame]);
        }
    }
    return frame_values_3dc[frame];
}

// FUNCTION: WIZ8 0x004736d0
int stMeshModel::FindSkinTable004736D0(const char* name)
{
    for (int index = 0; index < skin_table_names.GetCount(); ++index) {
        if (_stricmp(name, *skin_table_names.GetAt(index)) == 0) {
            return index;
        }
    }
    return -1;
}

// FUNCTION: WIZ8 0x00473720
srPtr<srTextureIFace>* stMeshModel::GetTextureTable00473720(int table)
{
    int index = skin_table_ids.IndexOf(table);

    if (index != -1) {
        return *skin_texture_tables.GetAt(index);
    }
    return 0;
}

/* Clone one polygon-texture table under a new name and allocate the parallel
   skin-blanking state used by the renderer. Table ids are the lowest free
   non-negative integer and remain stable independently of vector position. */
// FUNCTION: WIZ8 0x00473260
int stMeshModel::CreateSkinTable00473260(
    const char* name, int base_table)
{
    int base_index = skin_table_ids.IndexOf(base_table);

    if (FindSkinTable004736D0(name) != -1) {
        return -1;
    }

    srPtr<srTextureIFace>* source;
    if (base_index == -1) {
        source = getPolyTexture(0, 0, 0);
    }
    else {
        source = *skin_texture_tables.GetAt(base_index);
    }
    if (source == 0) {
        return -1;
    }

    int table = 0;
    while (skin_table_ids.IndexOf(table) != -1) {
        ++table;
    }

    srPtr<srTextureIFace>* textures =
        new srPtr<srTextureIFace>[polygon_count_230];
    for (int polygon = 0; polygon < polygon_count_230; ++polygon) {
        textures[polygon] = source[polygon];
    }
    skin_texture_tables.Add(textures);
    skin_table_ids.Add(table);

    char* copied_name = static_cast<char*>(malloc(strlen(name) + 1));
    strcpy(copied_name, name);
    skin_table_names.Add(copied_name);

    if (skin_blanking_apt_458 == 0) {
        skin_blanking_apt_458 = new W8GrowableVector<unsigned long*>;
        if (skin_blanking_apt_458 == 0) {
            srAssertFail(
                "m_plsSkinBlankingAPT",
                "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp",
                0x5cd,
                0);
        }
    }
    skin_blanking_apt_458->Add(0);

    if (skin_blanking_apt_number_45c == 0) {
        skin_blanking_apt_number_45c = new W8GrowableVector<int>;
        if (skin_blanking_apt_number_45c == 0) {
            srAssertFail(
                "m_plsSkinBlankingAPTNum",
                "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp",
                0x5d6,
                0);
        }
    }
    skin_blanking_apt_number_45c->Add(0);

    if (skin_blanking_checked_460 == 0) {
        skin_blanking_checked_460 = new W8GrowableVector<unsigned char>;
        if (skin_blanking_checked_460 == 0) {
            srAssertFail(
                "m_plsSkinBlankingChecked",
                "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp",
                0x5df,
                0);
        }
    }
    skin_blanking_checked_460->Add(0);
    return table;
}

// FUNCTION: WIZ8 0x00473830
void stMeshModel::RemoveSkinTable00473830(int index)
{
    srPtr<srTextureIFace>* textures = *skin_texture_tables.GetAt(index);
    for (int polygon = 0; polygon < polygon_count_230; ++polygon) {
        textures[polygon] = static_cast<srTextureIFace*>(0);
    }
    delete[] textures;
    free(*skin_table_names.GetAt(index));

    skin_texture_tables.RemoveAt(index);
    skin_table_names.RemoveAt(index);
    skin_table_ids.RemoveAt(index);

    unsigned long* apt = *skin_blanking_apt_458->GetAt(index);
    if (apt != 0) {
        delete apt;
    }
    skin_blanking_apt_458->RemoveAt(index);
    skin_blanking_apt_number_45c->RemoveAt(index);
    skin_blanking_checked_460->RemoveAt(index);
}

/* Skin tables are named with the owning cycle plus a one-character suffix.
   Final teardown removes every table whose name has that cycle prefix. */
// FUNCTION: WIZ8 0x00473780
void stMeshModel::RemoveSkinTablesForCycle00473780(const char* cycle_name)
{
    if (cycle_name != 0) {
        for (int index = 0; index < skin_table_names.GetCount(); ++index) {
            char name[200];

            strncpy(name, *skin_table_names.GetAt(index), 199);
            name[199] = '\0';
            name[strlen(name) - 1] = '\0';
            if (strlen(name) != 0 && _stricmp(cycle_name, name) == 0) {
                RemoveSkinTable00473830(index);
                --index;
            }
        }
    }
}

/* Thirteen-byte forwarder onto the model release path. */
// FUNCTION: WIZ8 0x00473180
void ReleaseMeshModel(void* model)
{
    Function4729F0(model);
}

/* The two SurRender registry slots. The literal is the class's own original
   name and the id sits in the Wizardry-registered range at 0x10000 and up,
   which is what separates this class from SurRender's own srMeshModel at
   0x2010. */
// FUNCTION: WIZ8 0x004741f0
const char* stMeshModel::getClassName() const
{
    return "stMeshModel";
}

// FUNCTION: WIZ8 0x004741e0
unsigned long stMeshModel::getClassID() const
{
    return 0x10003;
}

/* Three-level registry builder that names every level by literal - no static
   name getter appears at all, which is what makes it the shortest of the
   three-level forms. The chain is stMeshModel under srMeshModel under srModel
   under srClass. */
// FUNCTION: WIZ8 0x00474820
srRegistry::ClassNode* stMeshModel::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x10003);

    if (!node) {
        srRegistry* mesh_registry = srCore.getRegistry();
        srRegistry::ClassNode* mesh = mesh_registry->getClassNode(0x2010);

        if (!mesh) {
            srRegistry* model_registry = srCore.getRegistry();
            srRegistry::ClassNode* model = model_registry->getClassNode(0x2000);

            if (!model) {
                model = model_registry->registerClass(
                    "srModel", srClass::sGetClassNode(), 0x2000, 1);
            }
            mesh = mesh_registry->registerClass("srMeshModel", model, 0x2010, 0);
        }
        node = registry->registerClass("stMeshModel", mesh, 0x10003, 0);
    }
    return node;
}

/* Ordinary primary-template instantiation emissions. The generic methods live
   inline in srArray.h; there are no per-element authored bodies here. */
// TEMPLATE: WIZ8 0x00474930
// srArray<srTriMeshPipeline::Record>::setCapacity

// TEMPLATE: WIZ8 0x00474A80
// srArray<srTriMeshPipeline::Pass>::setCapacity

// TEMPLATE: WIZ8 0x00475240
// srHeapArray<srVertexProcessor*>::ensure (folded four-byte-element instantiations)

/* Mirror the active shader onto both the pipeline and the current Pass record
   selected at +0x18. */
// FUNCTION: WIZ8 0x004752C0
void srTriMeshPipeline::SetFlags004752C0(srShader shader)
{
    shader_74 = shader;
    current_pass_18->flags_08 = shader;
}

/* Point current_record_14 / current_pass_18 at slot slot_count_84, growing
   either table by (capacity + slot + 8) when needed. */
// FUNCTION: WIZ8 0x00475540
void srTriMeshPipeline::PrepareSlot00475540()
{
    current_record_14 = &records_94[slot_count_84];
    current_pass_18 = &passes_9c[slot_count_84];

    current_record_14->flags_00 = 0;
    current_record_14->value_04 = 0;
    current_record_14->material_08 = material_80;
    current_pass_18->value_00 = value_78;
    current_pass_18->value_04 = value_7c;
    current_pass_18->flags_08.value = shader_74.value;
    current_pass_18->value_0c = 0;
    current_pass_18->value_10 = 0;
    current_pass_18->value_14 = 0;
    current_pass_18->value_18 = 0;
    current_pass_18->value_1c = 0;
}

// FUNCTION: WIZ8 0x00475510
void srTriMeshPipeline::Flush00475510()
{
    flushing_8c = 1;
    if (slot_count_84 > 0) {
        FlushSlots00475600();
    }
    flushing_8c = 0;
}

/* Bind a renderer and rebuild the current slot. Retail duplicates the prepare
   body rather than calling PrepareSlot00475540. */
// FUNCTION: WIZ8 0x004753F0
void srTriMeshPipeline::Reset004753F0(srGERD* renderer)
{
    slot_count_84 = 0;
    renderer_88 = renderer;
    flags_28 = 0;
    flags_28 |= 1;
    flags_28 |= 2;
    value_1c = 0;
    value_2c = 0;
    value_30 = 0;
    value_34 = 0;
    value_20 = 0;
    value_38 = 0;
    value_3c = 0;
    value_6c = 0;
    value_40 = 0;
    shader_74.value = 0x0100241b;
    value_78 = 0;
    value_7c = 0;
    material_80 = srCore.getMaterial();

    current_record_14 = &records_94[slot_count_84];
    current_pass_18 = &passes_9c[slot_count_84];

    current_record_14->flags_00 = 0;
    current_record_14->value_04 = 0;
    current_record_14->material_08 = material_80;
    current_pass_18->value_00 = value_78;
    current_pass_18->value_04 = value_7c;
    current_pass_18->flags_08.value = shader_74.value;
    current_pass_18->value_0c = 0;
    current_pass_18->value_10 = 0;
    current_pass_18->value_14 = 0;
    current_pass_18->value_18 = 0;
    current_pass_18->value_1c = 0;
}

srTriMeshPipeline::srTriMeshPipeline()
{
    flags_28 = 0;
    shader_74.value = 0;
    vertex_pipe_90 = new srVertexPipe();
    flushing_8c = 0;
    Reset004753F0(0);
    Flush00475510();
}

// FUNCTION: WIZ8 0x004752F0
srTriMeshPipeline::~srTriMeshPipeline()
{
    while (flushing_8c != 0) {
    }

    delete vertex_pipe_90;
}

// FUNCTION: WIZ8 0x00475600
void srTriMeshPipeline::FlushSlots00475600()
{
    if (value_1c == 0 || (value_2c != 0 && value_24 == 0)) {
        return;
    }

    if (value_6c != 1) {
        if (value_6c == 0 && value_20 != 0) {
            srVectorProcessor::minMax(
                value_38, value_44, value_50, value_20);
        }

        srVector3T<float> center;
        center.method_00421680(
            (value_44.x + value_50.x) * 0.5,
            (value_44.y + value_50.y) * 0.5,
            (value_44.z + value_50.z) * 0.5);
        value_5c = center;

        float x = value_5c.x - value_44.x;
        float y = value_5c.y - value_44.y;
        float z = value_5c.z - value_44.z;
        value_68 = static_cast<float>(sqrt(x * x + y * y + z * z))
                   * 1.001f;
    }

    srVector3T<float> eye_center;
    float eye_radius;
    srMatrix4T<float> model_view;
    srMatrix4T<float> inverse_model_view;
    srGERD::ClipPlanes clip_planes;
    srMatrix4T<float> project_clip_near;
    srMatrix4T<float> normal_matrix;

    renderer_88->getEyeSpaceBounds(
        eye_center, eye_radius, value_5c, value_68);
    renderer_88->getMatrix(srGERD::MATRIX_MODE_POSITIONAL_0, model_view);
    renderer_88->getInverseModelViewMatrix(inverse_model_view);
    renderer_88->getClipPlanes(clip_planes);
    renderer_88->getProjectClipNearMatrix(project_clip_near);
    renderer_88->getNormalMatrix(normal_matrix);

    srMatrix4T<float>::e_scaleType scale_type =
        renderer_88->getModelViewScaleType();
    srGERD::e_cullMode cull_mode = renderer_88->getCullMode();
    srGERD::e_winding winding = renderer_88->getWinding();

    srTriangleCuller::Input culler_input;
    if (cull_mode == srGERD::CULL_MODE_POSITIONAL_2) {
        culler_input.cull_mode_0c = 2;
    }
    else if (cull_mode == srGERD::CULL_MODE_POSITIONAL_1) {
        culler_input.cull_mode_0c =
            winding == srGERD::WINDING_POSITIONAL_0;
    }
    else {
        culler_input.cull_mode_0c =
            winding != srGERD::WINDING_POSITIONAL_0;
    }

    culler_input.vertex_count_04 = value_20;
    culler_input.vertices_1c = value_38;
    culler_input.clip_planes_20 = clip_planes.planes_000;
    culler_input.model_view_24 = &model_view;
    culler_input.inverse_model_view_28 = &inverse_model_view;
    culler_input.scale_type_2c = scale_type;

    if ((flags_28 & 1) == 0) {
        culler_input.clip_mask_30 = 0;
    }
    else {
        float depth;
        unsigned long clip_mask = srTriangleCuller::getClipMask(
            eye_center,
            eye_radius,
            clip_planes.planes_000,
            clip_planes.mask_200,
            depth);
        int retain_clip_mask =
            clip_mask != 0
            && ((clip_mask & 0xffffffc0UL) != 0
                || (slot_count_84 * value_1c > 45 && depth > 0.23f));
        culler_input.clip_mask_30 = retain_clip_mask ? clip_mask : 0;
    }

    if (value_2c == 0) {
        srCore.getStatisticsManager()->statistics_00
            .triangles_submitted_10 += slot_count_84 * value_1c;
    }
    else {
        srCore.getStatisticsManager()->statistics_00
            .triangles_submitted_10 += slot_count_84 * value_24;
    }
    ++srCore.getStatisticsManager()->statistics_00.meshes_submitted_0c;
    srCore.getStatisticsManager()->statistics_00.vertices_submitted_18 +=
        slot_count_84 * value_20;

    unsigned long total = value_2c == 0 ? value_1c : value_24;
    unsigned long batch_limit = total;
    if ((flags_28 & 2) != 0) {
        double ratio = static_cast<double>(value_20) / value_1c;
        if (ratio > 3.0f) {
            ratio = 3.0f;
        }
        batch_limit = static_cast<unsigned long>(
            1300.0f / (slot_count_84 * ratio));
        if (cull_mode == srGERD::CULL_MODE_POSITIONAL_2) {
            batch_limit >>= 1;
        }
        if (batch_limit > total) {
            batch_limit = total;
        }
    }

    unsigned long* scratch =
        values_0c.ensure(batch_limit + value_20 * 2);
    srTriangleCuller::Output culler_output;
    culler_output.indices_00 = scratch;
    culler_output.avt_04 = scratch + batch_limit;
    culler_output.clip_flags_08 = culler_output.avt_04 + value_20;

    unsigned long processed = 0;
    while (processed < total) {
        unsigned long batch_count = total - processed;
        if (batch_count > batch_limit) {
            batch_count = batch_limit;
        }

        if (value_2c == 0) {
            culler_input.triangle_count_00 = batch_count;
            culler_input.active_triangle_count_08 = 0;
            culler_input.active_triangles_10 = 0;
            culler_input.projected_vertices_14 = value_30 + processed;
            culler_input.triangles_18 = value_34 + processed;
        }
        else {
            culler_input.triangle_count_00 = value_1c;
            culler_input.active_triangle_count_08 = batch_count;
            culler_input.active_triangles_10 = value_2c + processed;
            culler_input.projected_vertices_14 = value_30;
            culler_input.triangles_18 = value_34;
        }

        if (srTriangleCuller::cull(culler_output, culler_input)) {
            srCore.getStatisticsManager()->statistics_00
                .triangles_after_culling_14 +=
                slot_count_84 * culler_output.triangle_count_0c;
            srCore.getStatisticsManager()->statistics_00
                .vertices_after_culling_1c +=
                slot_count_84 * culler_output.vertex_count_10;

            srGERD::Renderer* renderer = renderer_88->lockRenderer();

            (void)vertex_arrays_a4[slot_count_84];
            srVertexArray* vertex_arrays = &vertex_arrays_a4[0];
            renderer->allocVertexArray(
                vertex_arrays[0],
                slot_count_84 * culler_output.vertex_count_10);

            for (unsigned long slot = 1; slot < slot_count_84; ++slot) {
                unsigned long offset =
                    slot * culler_output.vertex_count_10;
                vertex_arrays[slot].values_00 =
                    vertex_arrays[0].values_00 + offset;
                vertex_arrays[slot].values_04 =
                    vertex_arrays[0].values_04 + offset;
                vertex_arrays[slot].values_08 =
                    vertex_arrays[0].values_08 + offset;
                vertex_arrays[slot].values_0c =
                    vertex_arrays[0].values_0c + offset;
                vertex_arrays[slot].values_10 =
                    vertex_arrays[0].values_10 + offset;
                vertex_arrays[slot].values_14 =
                    vertex_arrays[0].values_14 + offset;
                vertex_arrays[slot].values_18 =
                    vertex_arrays[0].values_18 + offset;
                vertex_arrays[slot].values_1c =
                    vertex_arrays[0].values_1c + offset;
            }

            unsigned long processor_count =
                renderer_88->getVertexProcessorCount();
            srVertexProcessor** processors = 0;
            if (processor_count != 0) {
                processors = vertex_processors_04.ensure(processor_count);
                renderer_88->getVertexProcessors(processors);
            }

            srVector4T<float> ambient_light;
            float environment_minimum;
            float environment_maximum;
            float environment_scale;
            float environment_inverse_scale;
            renderer_88->getAmbientLight(ambient_light);
            renderer_88->getEnvironmentRange(
                environment_minimum, environment_maximum);
            renderer_88->getEnvironmentScaleFactor(
                environment_scale, environment_inverse_scale);
            unsigned long exclusion_mask = renderer_88->getExclusionMask();

            srVertexPipe::Input pipe_input;
            pipe_input.record_count_00 = slot_count_84;
            pipe_input.vertex_count_04 = culler_output.vertex_count_10;
            pipe_input.indices_08 = culler_output.avt_04;
            pipe_input.position_is_float3_0c =
                culler_output.linear_14 == 0;
            pipe_input.positions_10 = value_38;
            pipe_input.values_14 = value_3c;
            pipe_input.eye_center_18 = eye_center;
            pipe_input.eye_radius_24 = eye_radius;
            pipe_input.model_view_28 = &model_view;
            pipe_input.normal_matrix_2c = &normal_matrix;
            pipe_input.vertex_arrays_30 = &vertex_arrays_a4[0];
            pipe_input.exclusion_mask_34 = exclusion_mask;
            pipe_input.ambient_light_38 = ambient_light;
            pipe_input.records_48 = &records_94[0];
            pipe_input.processors_4c = processors;
            pipe_input.processor_count_50 = processor_count;
            pipe_input.environment_minimum_54 = environment_minimum;
            pipe_input.environment_maximum_58 = environment_maximum;
            pipe_input.environment_scale_5c = environment_scale;
            pipe_input.environment_inverse_scale_60 =
                environment_inverse_scale;

            if (value_2c == 0 && processed != 0) {
                for (unsigned long index = 0; index < batch_count; ++index) {
                    culler_output.indices_00[index] += processed;
                }
            }

            unsigned long renderer_disable_mask = 0;
            if (renderer_88->getMaxTextureStages() == 1) {
                renderer_disable_mask = 0x140;
            }

            for (unsigned long pass_index = 0;
                 pass_index < slot_count_84;
                 ++pass_index) {
                passes_9c[pass_index].value_18 =
                    records_94[pass_index].value_20;

                unsigned long disable_mask;
                if (passes_9c[pass_index].value_14 != 0) {
                    srFlags<srVertexProcessor::e_channel> flags =
                        srVertexPipe::getShaderDisableMask(
                            passes_9c[pass_index].value_14,
                            culler_output.indices_00,
                            culler_output.triangle_count_0c);
                    disable_mask = flags.value;
                }
                else {
                    srFlags<srVertexProcessor::e_channel> flags =
                        srVertexPipe::getShaderDisableMask(
                            passes_9c[pass_index].flags_08);
                    disable_mask = flags.value;
                }
                records_94[pass_index].value_04 =
                    disable_mask | renderer_disable_mask;
            }

            vertex_pipe_90->process(pipe_input);

            srGERD::Renderer::TriInput render_input;
            render_input.triangle_count_00 =
                culler_output.triangle_count_0c;
            render_input.record_count_04 = slot_count_84;
            render_input.vertex_count_08 = culler_output.vertex_count_10;
            render_input.indices_0c = culler_output.indices_00;
            render_input.triangles_10 = value_34;
            render_input.vertices_14 = culler_output.clip_flags_08;
            render_input.passes_18 = &passes_9c[0];
            render_input.position_is_float3_1c =
                culler_output.linear_14 == 0;
            render_input.project_clip_near_20 = &project_clip_near;
            render_input.value_24 = value_40;
            renderer->render(render_input);
            renderer_88->unlockRenderer(renderer, 0);
        }

        processed += batch_limit;
    }
}

// TEMPLATE: WIZ8 0x00476080
// unresolved generic srHeap allocator instantiation for four-byte elements

// TEMPLATE: WIZ8 0x004760A0
// srArray<srVertexArray>::setCapacity

/* Lazy singleton: construct once against the imported pipe static, then bind
   the caller's renderer and rebuild the current slot. */
// FUNCTION: WIZ8 0x004750A0
srTriMeshPipeline* srTriMeshPipeline::Get004750A0(srGERD* renderer)
{
    srTriMeshPipeline* pipeline;

    if (renderer == 0) {
        return 0;
    }

    if (pipe == 0) {
        pipe = new srTriMeshPipeline();
    }

    pipeline = pipe;
    pipeline->renderer_88 = renderer;
    pipeline->slot_count_84 = 0;
    pipeline->flags_28 = 0;
    pipeline->flags_28 |= 1;
    pipeline->flags_28 |= 2;
    pipeline->value_1c = 0;
    pipeline->value_2c = 0;
    pipeline->value_30 = 0;
    pipeline->value_34 = 0;
    pipeline->value_20 = 0;
    pipeline->value_38 = 0;
    pipeline->value_3c = 0;
    pipeline->value_6c = 0;
    pipeline->value_40 = 0;
    pipeline->shader_74.value = 0x0100241b;
    pipeline->value_78 = 0;
    pipeline->value_7c = 0;
    pipeline->material_80 = srCore.getMaterial();
    pipeline->PrepareSlot00475540();
    return pipe;
}
