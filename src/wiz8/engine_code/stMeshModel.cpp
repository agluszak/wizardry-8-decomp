#include "wiz8/engine_code/stMeshModel.h"

#include "wiz8/float_constants.h"
#include "wiz8/sr_api.h"
#include "surrender/srCore.h"
#include "surrender/srGERD.h"
#include "surrender/srHeap.h"
#include "wiz8/wiz8_windows.h"
#include "surrender/srMaterial.h"
#include "surrender/srTriangleCuller.h"
#include "surrender/srTriMeshPipeline.h"
#include "surrender/srTypeRegistry.h"
#include "surrender/srVectorProcessor.h"
#include "wiz8/engine_code/Octree.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>

extern srVector3T<float> g_environment_offset_00659cd0;
extern float g_light_scale_0060bfe0;

/*
 * Engine Code\stMeshModel.cpp.
 *
 * A mesh model and the sibling chain it can be linked into. The shared
 * srTriMeshPipeline singleton's method bodies are emitted in this object as
 * well: its vtable sits immediately after the mesh-model vector vftables.
 */

// GLOBAL: WIZ8 0x00659cb8
W8GrowableVector<stMeshModel*> g_mesh_models;

// GLOBAL: WIZ8 0x0065a0e8
int g_decompressed_mesh_bytes;

/* Byte budget for the decompressed per-frame caches; AllocateFrameBuffers
   reclaims least-recently-used frames past it. */
// GLOBAL: WIZ8 0x00609d34
int g_decompressed_mesh_byte_limit_00609d34 = 0x800000;

/* Signed-byte normal components back to floats, indexed by the raw byte. */
// GLOBAL: WIZ8 0x00659ce8
static float s_compressed_normal_table[256];

// GLOBAL: WIZ8 0x0065a0ef
static unsigned char s_compressed_normal_table_ready;

// FUNCTION: WIZ8 0x00470B00
stMeshModel::stMeshModel(long polygons, long vertices)
    : srClassSupport<stMeshModel, srMeshModel, false, 0x10003>(0, 0), next(0), previous(0),
      flags_3a0(0), vertex_light_table_3b0(0), flag_3cc(0), vertex_lighting_ready_3cd(0),
      frame_count(0), frame_vertex_locations(0), frame_vertex_normals(0), frame_polygon_normals(0),
      compressed_vertex_locations(0), compressed_vertex_normals(0), compressed_polygon_normals(0),
      skin_table_ids(5), skin_texture_tables(5), skin_table_names(5), mapped_values(5),
      mapped_keys(5), last_decompress_release_tick_440(0), vertex_compression_scale_444(0.0f),
      lerp_buffer_448(0), automap_polygons(0), automap_polygon_count(0), automap_filter_active(0),
      skin_blanking_apt_458(0), skin_blanking_apt_number_45c(0), skin_blanking_checked_460(0)
{
    memset(&ambient_color_3a4, 0, sizeof(ambient_color_3a4));
    memset(unknown_3ce, 0, sizeof(unknown_3ce));
    memset(unknown_3ec, 0, sizeof(unknown_3ec));

    if (!s_compressed_normal_table_ready) {
        for (int value = -128; value < 128; ++value) {
            float component = (float)value * (1.0f / 127.0f);
            if (component < -1.0f) {
                component = -1.0f;
            } else if (component > 1.0f) {
                component = 1.0f;
            }
            s_compressed_normal_table[(unsigned char)value] = component;
        }
        s_compressed_normal_table_ready = 1;
    }

    srCore.getRegistry()->registerInstance(
        srClassSupport<stMeshModel, srMeshModel, false, 0x10003>::sGetClassNode(), this);

    srMeshModel::reset(polygons, vertices);

    for (int pass = 0; pass < 4; ++pass) {
        for (int side = 0; side < 2; ++side) {
            materials_1c[pass][side] = 0;
            textures_3c[pass][side] = 0;
        }
    }
    control_state_394 &= ~0x10UL;
    if ((control_state_390 & 1) == 0) {
        control_state_390 |= 9;
    }
}

// SYNTHETIC: WIZ8 0x00470e90
// stMeshModel::`scalar deleting destructor'

// FUNCTION: WIZ8 0x004748c0
srClass* stMeshModel::vInstance()
{
    return new stMeshModel(0, 0);
}

// FUNCTION: WIZ8 0x00471dd0
int stMeshModel::getBoundingSphere(srVector3T<float>& center, float& radius)
{
    if ((control_state_390 & 1) != 0) {
        calculateBounds();
    }
    center = bounds_center_218;
    radius = bounds_radius_224;
    return 1;
}

// FUNCTION: WIZ8 0x00471d80
int stMeshModel::getBoundingBox(srVector3T<float>& minimum, srVector3T<float>& maximum)
{
    if ((control_state_390 & 1) != 0) {
        calculateBounds();
    }
    minimum = bounds_minimum_200;
    maximum = bounds_maximum_20c;
    return 1;
}

/* Apply pending vertex DIG lighting when flags_3a0 bit 1 is set, then return
   the SurRender TriMesh cache. */
// FUNCTION: WIZ8 0x00472270
const srMeshModel::TriMesh& stMeshModel::getTriMesh()
{
    float light_scale = g_light_scale_0060bfe0;
    srVector3T<float>* dig;
    srVector3T<float>* lights;
    float* sunlight;
    srPtr<srMaterialIFace>* vertex_materials;
    int count;
    int index;
    int run;
    int next_index;
    srMaterialIFace* material_iface;
    srMaterial* material;
    srVector3T<float> scaled;
    srVector3T<float> ambient_rgb;

    if ((flags_3a0 & 2) != 0 && g_flag_0065a0ec == 0) {
        lights = vertex_lights_3b4[vertex_light_table_3b0].data;
        sunlight = vertex_sunlight_3c4.data;
        if (lights != 0 && sunlight != 0) {
            dig = getVertexDIG(0, 1);
            vertex_materials = getVertexMaterial(0, static_cast<e_side>(0), 0);
            if (vertex_materials == 0) {
                if ((ambient_color_3a4.x == g_float_005ebb34 &&
                     ambient_color_3a4.y == g_float_005ebb34 &&
                     ambient_color_3a4.z == g_float_005ebb34) ||
                    vertex_light_table_3b0 == 1) {
                    CopyDwordBuffer00470180(dig, lights, vertex_location_count_22c * 3);
                    if ((g_environment_offset_00659cd0.x != g_float_005ebb34 ||
                         g_environment_offset_00659cd0.y != g_float_005ebb34 ||
                         g_environment_offset_00659cd0.z != g_float_005ebb34) &&
                        vertex_light_table_3b0 != 1 &&
                        (count = vertex_location_count_22c, count != 0) &&
                        IsZeroVector0046FFA0(&g_environment_offset_00659cd0) == 0) {
                        srVectorProcessor::add(dig, g_environment_offset_00659cd0, dig,
                                               static_cast<SRDWORD>(count));
                    }
                } else {
                    /* Retail indexes material ambient at +0x28; that is
                       srMaterial::parms_18.ambient on the concrete type. */
                    material = static_cast<srMaterial*>(getMaterial(0, static_cast<e_side>(0)));
                    ambient_rgb.x = material->parms_18.ambient.x;
                    ambient_rgb.y = material->parms_18.ambient.y;
                    ambient_rgb.z = material->parms_18.ambient.z;
                    count = vertex_location_count_22c;
                    scaled.x = ambient_rgb.x * ambient_color_3a4.x;
                    scaled.y = ambient_rgb.y * ambient_color_3a4.y;
                    scaled.z = ambient_rgb.z * ambient_color_3a4.z;
                    if (count != 0) {
                        if (scaled.x == scaled.y && scaled.x == scaled.z) {
                            unsigned int bits;
                            // reinterpret-ok: FillDwordBuffer takes the float bit pattern
                            bits = *reinterpret_cast<unsigned int*>(&scaled.x);
                            FillDwordBuffer00474700(dig, bits, count * 3);
                        } else {
                            srVectorProcessor::copy(dig, scaled, static_cast<SRDWORD>(count));
                        }
                    }
                    if (vertex_location_count_22c != 0) {
                        srVectorProcessor::mul(dig, dig, sunlight,
                                               static_cast<SRDWORD>(vertex_location_count_22c));
                    }
                    AddFloatBuffer00474730(
                        reinterpret_cast<float*>(dig), // reinterpret-ok: packed DIG as float*
                        reinterpret_cast<float*>(lights), // reinterpret-ok: packed lights as float*
                        vertex_location_count_22c * 3);
                    if ((g_environment_offset_00659cd0.x != g_float_005ebb34 ||
                         g_environment_offset_00659cd0.y != g_float_005ebb34 ||
                         g_environment_offset_00659cd0.z != g_float_005ebb34) &&
                        (count = vertex_location_count_22c, count != 0) &&
                        IsZeroVector0046FFA0(&g_environment_offset_00659cd0) == 0) {
                        srVectorProcessor::add(dig, g_environment_offset_00659cd0, dig,
                                               static_cast<SRDWORD>(count));
                    }
                }
            } else {
                if ((ambient_color_3a4.x == g_float_005ebb34 &&
                     ambient_color_3a4.y == g_float_005ebb34 &&
                     ambient_color_3a4.z == g_float_005ebb34) ||
                    vertex_light_table_3b0 == 1) {
                    if (vertex_location_count_22c != 0) {
                        FillDwordBuffer00474700(dig, 0, vertex_location_count_22c * 3);
                    }
                } else {
                    count = vertex_location_count_22c;
                    if (count != 0) {
                        if (ambient_color_3a4.x == ambient_color_3a4.y &&
                            ambient_color_3a4.x == ambient_color_3a4.z) {
                            unsigned int bits;
                            // reinterpret-ok: FillDwordBuffer takes the float bit pattern
                            bits = *reinterpret_cast<unsigned int*>(&ambient_color_3a4.x);
                            FillDwordBuffer00474700(dig, bits, count * 3);
                        } else {
                            srVectorProcessor::copy(dig, ambient_color_3a4,
                                                    static_cast<SRDWORD>(count));
                        }
                    }
                    if (vertex_location_count_22c != 0) {
                        srVectorProcessor::mul(dig, dig, sunlight,
                                               static_cast<SRDWORD>(vertex_location_count_22c));
                    }
                }
                index = 0;
                do {
                    next_index = index + 1;
                    run = 1;
                    if (next_index < vertex_location_count_22c) {
                        do {
                            if (static_cast<srMaterialIFace*>(vertex_materials[index]) !=
                                static_cast<srMaterialIFace*>(vertex_materials[next_index])) {
                                break;
                            }
                            ++run;
                            ++next_index;
                        } while (next_index < vertex_location_count_22c);
                    }
                    material_iface = vertex_materials[index];
                    if (material_iface != 0) {
                        material = static_cast<srMaterial*>(material_iface);
                        scaled.x = material->parms_18.ambient.x;
                        scaled.y = material->parms_18.ambient.y;
                        scaled.z = material->parms_18.ambient.z;
                        if (run != 0) {
                            if (IsZeroVector0046FFA0(&scaled) == 0) {
                                srVectorProcessor::mul(dig + index, scaled, dig + index,
                                                       static_cast<SRDWORD>(run));
                            } else {
                                FillDwordBuffer00474700(dig + index, 0, run * 3);
                            }
                        }
                    }
                    index += run;
                } while (index < vertex_location_count_22c);
                AddFloatBuffer00474730(
                    reinterpret_cast<float*>(dig), // reinterpret-ok: packed DIG as float*
                    reinterpret_cast<float*>(lights), // reinterpret-ok: packed lights as float*
                    vertex_location_count_22c * 3);
                if ((g_environment_offset_00659cd0.x != g_float_005ebb34 ||
                     g_environment_offset_00659cd0.y != g_float_005ebb34 ||
                     g_environment_offset_00659cd0.z != g_float_005ebb34) &&
                    vertex_light_table_3b0 != 1 &&
                    (count = vertex_location_count_22c, count != 0) &&
                    IsZeroVector0046FFA0(&g_environment_offset_00659cd0) == 0) {
                    srVectorProcessor::add(dig, g_environment_offset_00659cd0, dig,
                                           static_cast<SRDWORD>(count));
                }
            }
            if (light_scale != g_float_005ebb38 &&
                (count = vertex_location_count_22c, count != 0)) {
                if (light_scale == g_float_005ebb34) {
                    FillDwordBuffer00474700(dig, 0, count * 3);
                } else {
                    srVectorProcessor::mul(
                        reinterpret_cast<float*>(dig), // reinterpret-ok: packed DIG as float*
                        light_scale,
                        reinterpret_cast<float*>(dig), // reinterpret-ok: packed DIG as float*
                        static_cast<SRDWORD>(count) * 3);
                }
            }
        }
        flags_3a0 &= ~2u;
    }
    return srMeshModel::getTriMesh();
}

// FUNCTION: WIZ8 0x004727e0
void stMeshModel::getTriMesh(TriMesh& mesh)
{
    mesh = getTriMesh();
}

/* Retail wrapper pushes a zero flag into the shared renderer at 0x00470380.
   The helper body is still unrecovered; forward to the SurRender import so the
   vtable slot stays local while the full path is recovered. */
// FUNCTION: WIZ8 0x00470360
void stMeshModel::renderTriMesh(srGERD& renderer, const TriMesh& mesh)
{
    srMeshModel::renderTriMesh(renderer, mesh);
}

/* Copy `count` dwords between distinct buffers through the imported vp. */
// FUNCTION: WIZ8 0x00470180
void CopyDwordBuffer00470180(void* destination, const void* source, int count)
{
    if (count != 0 && destination != source) {
        srVectorProcessor::memcopy(destination, source, count << 2);
    }
}

// FUNCTION: WIZ8 0x0046ffa0
int __fastcall IsZeroVector0046FFA0(const srVector3T<float>* vector)
{
    if (vector->x == g_float_005ebb34 && vector->y == g_float_005ebb34 &&
        vector->z == g_float_005ebb34) {
        return 1;
    }
    return 0;
}

/* Fill `count` dwords through vp->_copy(SRDWORD*, SRDWORD, SRDWORD). */
// FUNCTION: WIZ8 0x00474700
void FillDwordBuffer00474700(void* destination, unsigned int value, int count)
{
    if (count != 0) {
        srVectorProcessor::copy(static_cast<SRDWORD*>(destination), value,
                                static_cast<SRDWORD>(count));
    }
}

/* dest[i] += source[i] through vp->_add(float*, dest, source, count). */
// FUNCTION: WIZ8 0x00474730
void AddFloatBuffer00474730(float* destination, const float* source, int count)
{
    if (count != 0) {
        srVectorProcessor::add(destination, destination, source, static_cast<SRDWORD>(count));
    }
}

/* Copy or translate `count` vertices: a zero offset is a plain copy and a
   nonzero one goes through the vp constant-vector add. */
// FUNCTION: WIZ8 0x00470040
void OffsetVertices00470040(srVector3T<float>* destination, const srVector3T<float>* source,
                            const srVector3T<float>* offset, int count)
{
    if (count != 0) {
        if (offset->x == g_float_005ebb34 && offset->y == g_float_005ebb34 &&
            offset->z == g_float_005ebb34) {
            CopyDwordBuffer00470180(destination, source, count * 3);
        } else {
            srVectorProcessor::add(destination, *offset, source, static_cast<SRDWORD>(count));
        }
    }
}

// FUNCTION: WIZ8 0x00473fa0
void stMeshModel::ApplyAutomapPolygonFilter(const W8GrowableVector<char*>* excluded_textures)
{
    if (automap_polygons) {
        delete[] automap_polygons;
        automap_polygons = 0;
    }
    automap_polygon_count = 0;
    automap_polygons = new unsigned int[polygon_count_230];
    automap_filter_active = 1;
    srPtr<srTextureIFace>* texture = getPolyTexture(0, 0, 0);
    if (texture) {
        for (unsigned int polygon = 0; polygon < static_cast<unsigned int>(polygon_count_230);
             ++polygon, texture += 4) {
            if (!*texture) {
                automap_polygons[automap_polygon_count++] = polygon;
            } else {
                char name[256];
                strcpy(name, (*texture)->getName());
                bool include = true;
                for (int index = 0; index < excluded_textures->count; ++index) {
                    if (_stricmp(name, *excluded_textures->GetAt(index)) == 0)
                        include = false;
                }
                if (include)
                    automap_polygons[automap_polygon_count++] = polygon;
            }
        }
    }
    if (automap_polygon_count >= static_cast<unsigned int>(polygon_count_230)) {
        delete[] automap_polygons;
        automap_polygons = 0;
        automap_polygon_count = 0;
    }
}

// FUNCTION: WIZ8 0x00474120
void stMeshModel::ClearAutomapPolygonFilter()
{
    if (automap_polygons) {
        delete[] automap_polygons;
        automap_polygons = 0;
        automap_polygon_count = 0;
    }
}

// FUNCTION: WIZ8 0x00471160
void stMeshModel::SetMappedVertex00471160(short vertex, short key)
{
    int index = mapped_keys.IndexOf(key);
    if (index != -1) {
        mapped_values.SetAt(index, vertex);
        return;
    }
    mapped_values.Add(vertex);
    mapped_keys.Add(key);
}

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
   link rather than two independent  Unlinking passes nothing. */
// FUNCTION: WIZ8 0x00471d60
void stMeshModel::LinkTo(stMeshModel* other)
{
    next = other;
    if (other != 0) {
        other->previous = this;
    }
}

/* One frame's compressed vertex table, refused outright when there is no
   table at all. */
// FUNCTION: WIZ8 0x00471aa0
short* stMeshModel::GetVertex(unsigned int frame)
{
    if (compressed_vertex_locations != 0 && frame < frame_count) {
        return compressed_vertex_locations[frame];
    }
    return 0;
}

// FUNCTION: WIZ8 0x00473b00
void stMeshModel::InitializeVertexFrames(int frames)
{
    if (frames == 0) {
        srAssertFail("uiFrames", "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp", 0x6e8,
                     0);
    }
    if ((flags_3a0 & 4) == 0) {
        FreeFrameStorage();
        frame_count = frames;
        flags_3a0 |= 4;
        AllocateFrameStorage();
        g_mesh_models.Add(this);
    }
}

/* Zero-filled pointer tables for every frame, then one compressed table per
   frame: three shorts per vertex location and three bytes per vertex and
   polygon normal. Any failed allocation releases everything again. */
// FUNCTION: WIZ8 0x00471340
unsigned char stMeshModel::AllocateFrameStorage()
{
    if (frame_count == 0) {
        return 0;
    }
    frame_vertex_locations = new srVector3T<float>*[frame_count];
    if (frame_vertex_locations == 0) {
        FreeFrameStorage();
        return 0;
    }
    memset(frame_vertex_locations, 0, frame_count * sizeof(srVector3T<float>*));
    frame_vertex_normals = new srVector3T<float>*[frame_count];
    if (frame_vertex_normals == 0) {
        FreeFrameStorage();
        return 0;
    }
    memset(frame_vertex_normals, 0, frame_count * sizeof(srVector3T<float>*));
    frame_polygon_normals = new srVector3T<float>*[frame_count];
    if (frame_polygon_normals == 0) {
        FreeFrameStorage();
        return 0;
    }
    memset(frame_polygon_normals, 0, frame_count * sizeof(srVector3T<float>*));
    compressed_vertex_locations = new short*[frame_count];
    if (compressed_vertex_locations == 0) {
        FreeFrameStorage();
        return 0;
    }
    compressed_vertex_normals = new unsigned char*[frame_count];
    if (compressed_vertex_normals == 0) {
        FreeFrameStorage();
        return 0;
    }
    compressed_polygon_normals = new unsigned char*[frame_count];
    if (compressed_polygon_normals == 0) {
        FreeFrameStorage();
        return 0;
    }
    for (unsigned int frame = 0; frame < frame_count; ++frame) {
        compressed_vertex_locations[frame] = new short[vertex_location_count_22c * 3];
        if (compressed_vertex_locations[frame] == 0) {
            srAssertFail("m_psCompVertexLoc[uiCount]",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp", 0x1ce, 0);
        }
        memset(compressed_vertex_locations[frame], 0,
               vertex_location_count_22c * 3 * sizeof(short));
        compressed_vertex_normals[frame] = new unsigned char[vertex_location_count_22c * 3];
        if (compressed_vertex_normals[frame] == 0) {
            srAssertFail("m_pbCompVertexNormal[uiCount]",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp", 0x1d3, 0);
        }
        memset(compressed_vertex_normals[frame], 0, vertex_location_count_22c * 3);
        compressed_polygon_normals[frame] = new unsigned char[polygon_count_230 * 3];
        if (compressed_polygon_normals[frame] == 0) {
            srAssertFail("m_pbCompPolyNormal[uiCount]",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp", 0x1d8, 0);
        }
        memset(compressed_polygon_normals[frame], 0, polygon_count_230 * 3);
    }
    return 1;
}

// FUNCTION: WIZ8 0x004715e0
void stMeshModel::FreeFrameStorage()
{
    unsigned int frame;

    ReleaseDecompressedFrames();
    if (frame_vertex_locations != 0) {
        delete[] frame_vertex_locations;
        frame_vertex_locations = 0;
    }
    if (frame_vertex_normals != 0) {
        delete[] frame_vertex_normals;
        frame_vertex_normals = 0;
    }
    if (frame_polygon_normals != 0) {
        delete[] frame_polygon_normals;
        frame_polygon_normals = 0;
    }
    if (compressed_vertex_locations != 0) {
        for (frame = 0; frame < frame_count; ++frame) {
            if (compressed_vertex_locations[frame] != 0) {
                delete[] compressed_vertex_locations[frame];
            }
        }
        delete[] compressed_vertex_locations;
        compressed_vertex_locations = 0;
    }
    if (compressed_vertex_normals != 0) {
        for (frame = 0; frame < frame_count; ++frame) {
            if (compressed_vertex_normals[frame] != 0) {
                delete[] compressed_vertex_normals[frame];
            }
        }
        delete[] compressed_vertex_normals;
        compressed_vertex_normals = 0;
    }
    if (compressed_polygon_normals != 0) {
        for (frame = 0; frame < frame_count; ++frame) {
            if (compressed_polygon_normals[frame] != 0) {
                delete[] compressed_polygon_normals[frame];
            }
        }
        delete[] compressed_polygon_normals;
        compressed_polygon_normals = 0;
    }
}

/* Drop every decompressed float cache, returning the bytes released. */
// FUNCTION: WIZ8 0x004739e0
int stMeshModel::ReleaseDecompressedFrames()
{
    int released = 0;
    unsigned int frame;

    if (frame_vertex_locations != 0) {
        for (frame = 0; frame < frame_count; ++frame) {
            if (frame_vertex_locations[frame] != 0) {
                released += vertex_location_count_22c * sizeof(srVector3T<float>);
                srHeap.free(frame_vertex_locations[frame]);
                frame_vertex_locations[frame] = 0;
            }
        }
    }
    if (frame_vertex_normals != 0) {
        for (frame = 0; frame < frame_count; ++frame) {
            if (frame_vertex_normals[frame] != 0) {
                released += vertex_location_count_22c * sizeof(srVector3T<float>);
                srHeap.free(frame_vertex_normals[frame]);
                frame_vertex_normals[frame] = 0;
            }
        }
    }
    if (frame_polygon_normals != 0) {
        for (frame = 0; frame < frame_count; ++frame) {
            if (frame_polygon_normals[frame] != 0) {
                released += polygon_count_230 * sizeof(srVector3T<float>);
                srHeap.free(frame_polygon_normals[frame]);
                frame_polygon_normals[frame] = 0;
            }
        }
    }
    last_decompress_release_tick_440 = GetTickCount();
    g_decompressed_mesh_bytes -= released;
    return released;
}

/* Evict least-recently-used decompressed frames until `needed` bytes are
   available, or give up when every registered model has been drained. */
// FUNCTION: WIZ8 0x00473BF0
unsigned char ReclaimDecompressedBytes00473BF0(unsigned int needed)
{
    unsigned int released = 0;
    while (released < needed) {
        stMeshModel* oldest = 0;
        unsigned long oldest_tick = 0xffffffff;
        for (int index = 0; index < g_mesh_models.count; ++index) {
            stMeshModel* model = *g_mesh_models.GetAt(index);
            if (model == 0) {
                srAssertFail("pstModel", "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp",
                             0x712, 0);
            }
            if (model->last_decompress_release_tick_440 < oldest_tick) {
                oldest = model;
                oldest_tick = model->last_decompress_release_tick_440;
            }
        }
        if (oldest == 0) {
            return 0;
        }
        released += oldest->ReleaseDecompressedFrames();
    }
    return 1;
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
int stMeshModel::CreateSkinTable00473260(const char* name, int base_table)
{
    int base_index = skin_table_ids.IndexOf(base_table);

    if (FindSkinTable004736D0(name) != -1) {
        return -1;
    }

    srPtr<srTextureIFace>* source;
    if (base_index == -1) {
        source = getPolyTexture(0, 0, 0);
    } else {
        source = *skin_texture_tables.GetAt(base_index);
    }
    if (source == 0) {
        return -1;
    }

    int table = 0;
    while (skin_table_ids.IndexOf(table) != -1) {
        ++table;
    }

    srPtr<srTextureIFace>* textures = new srPtr<srTextureIFace>[polygon_count_230];
    for (int polygon = 0; polygon < polygon_count_230; ++polygon) {
        textures[polygon] = source[polygon];
    }
    skin_texture_tables.Add(textures);
    skin_table_ids.Add(table);

    char* copied_name = static_cast<char*>(malloc(strlen(name) + 1));
    strcpy(copied_name, name);
    skin_table_names.Add(copied_name);

    if (skin_blanking_apt_458 == 0) {
        skin_blanking_apt_458 = new W8GrowableVector<int*>;
        if (skin_blanking_apt_458 == 0) {
            srAssertFail("m_plsSkinBlankingAPT",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp", 0x5cd, 0);
        }
    }
    skin_blanking_apt_458->Add(0);

    if (skin_blanking_apt_number_45c == 0) {
        skin_blanking_apt_number_45c = new W8GrowableVector<int>;
        if (skin_blanking_apt_number_45c == 0) {
            srAssertFail("m_plsSkinBlankingAPTNum",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp", 0x5d6, 0);
        }
    }
    skin_blanking_apt_number_45c->Add(0);

    if (skin_blanking_checked_460 == 0) {
        skin_blanking_checked_460 = new W8GrowableVector<unsigned char>;
        if (skin_blanking_checked_460 == 0) {
            srAssertFail("m_plsSkinBlankingChecked",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp", 0x5df, 0);
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

    int* apt = *skin_blanking_apt_458->GetAt(index);
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

/* Expand one frame's compressed table into `destination`: bit 1 the vertex
   locations, bit 2 the vertex normals, bit 4 the polygon normals. */
// FUNCTION: WIZ8 0x00471930
unsigned char stMeshModel::DecompressFrame(int frame, unsigned char flags,
                                           srVector3T<float>* destination)
{
    if (flags & 1) {
        for (int index = 0; index < vertex_location_count_22c; ++index) {
            const short* source = &compressed_vertex_locations[frame][index * 3];
            destination[index].x = (float)source[0] * vertex_compression_scale_444;
            destination[index].y = (float)source[1] * vertex_compression_scale_444;
            destination[index].z = (float)source[2] * vertex_compression_scale_444;
        }
        return 1;
    }
    if (flags & 2) {
        for (int index = 0; index < vertex_location_count_22c; ++index) {
            const unsigned char* source = &compressed_vertex_normals[frame][index * 3];
            destination[index].x = s_compressed_normal_table[source[0]];
            destination[index].y = s_compressed_normal_table[source[1]];
            destination[index].z = s_compressed_normal_table[source[2]];
        }
        return 1;
    }
    if (flags & 4) {
        for (int index = 0; index < polygon_count_230; ++index) {
            const unsigned char* source = &compressed_polygon_normals[frame][index * 3];
            destination[index].x = s_compressed_normal_table[source[0]];
            destination[index].y = s_compressed_normal_table[source[1]];
            destination[index].z = s_compressed_normal_table[source[2]];
        }
        return 1;
    }
    return 0;
}

/* Allocate one frame's decompressed caches for the tables named by `flags`
   (bit 0 locations, bit 1 vertex normals, bit 2 polygon normals), reclaiming
   least-recently-used frames when the byte budget would overflow. */
// FUNCTION: WIZ8 0x00471720
unsigned char stMeshModel::AllocateFrameBuffers00471720(unsigned int frame, unsigned char flags)
{
    if ((flags & 1) != 0 && frame_vertex_locations[frame] == 0) {
        int needed = vertex_location_count_22c * sizeof(srVector3T<float>);
        if (g_decompressed_mesh_byte_limit_00609d34 <= g_decompressed_mesh_bytes + needed) {
            if (ReclaimDecompressedBytes00473BF0(needed) == 0) {
                return 0;
            }
        }
        g_decompressed_mesh_bytes += needed;
        frame_vertex_locations[frame] = static_cast<srVector3T<float>*>(
            srHeap.allocate(vertex_location_count_22c * sizeof(srVector3T<float>)));
        if (frame_vertex_locations[frame] == 0) {
            srAssertFail("m_pVertexLoc[uiFrame]",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp", 0x243, 0);
        }
    }
    if ((flags & 2) != 0 && frame_vertex_normals[frame] == 0) {
        int needed = vertex_location_count_22c * sizeof(srVector3T<float>);
        if (g_decompressed_mesh_byte_limit_00609d34 <= g_decompressed_mesh_bytes + needed) {
            if (ReclaimDecompressedBytes00473BF0(needed) == 0) {
                return 0;
            }
        }
        g_decompressed_mesh_bytes += needed;
        frame_vertex_normals[frame] = static_cast<srVector3T<float>*>(
            srHeap.allocate(vertex_location_count_22c * sizeof(srVector3T<float>)));
        if (frame_vertex_normals[frame] == 0) {
            srAssertFail("m_pVertexNormal[uiFrame]",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp", 0x24e, 0);
        }
    }
    if ((flags & 4) != 0 && frame_polygon_normals[frame] == 0) {
        int needed = polygon_count_230 * sizeof(srVector3T<float>);
        if (g_decompressed_mesh_byte_limit_00609d34 <= g_decompressed_mesh_bytes + needed) {
            if (ReclaimDecompressedBytes00473BF0(needed) == 0) {
                return 0;
            }
        }
        g_decompressed_mesh_bytes += needed;
        frame_polygon_normals[frame] = static_cast<srVector3T<float>*>(
            srHeap.allocate(polygon_count_230 * sizeof(srVector3T<float>)));
        if (frame_polygon_normals[frame] == 0) {
            srAssertFail("m_pPolyNormal[uiFrame]",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp", 0x259, 0);
        }
    }
    return 1;
}

/* Return frame `frame`'s vertex locations, decompressing on demand. When
   `interpolation` is positive and another frame follows, both frames are
   decompressed and lerped into lerp_buffer_448 (m_pLerpBuffer). */
// FUNCTION: WIZ8 0x00471AD0
srVector3T<float>* stMeshModel::GetVertexLocations00471AD0(unsigned int frame, char load,
                                                           float interpolation)
{
    if (frame_vertex_locations == 0) {
        return 0;
    }
    if (g_float_005ebb34 < interpolation && frame < frame_count - 1) {
        unsigned int next_frame = (frame + 1) % frame_count;
        if (lerp_buffer_448 == 0) {
            lerp_buffer_448 = static_cast<srVector3T<float>*>(
                srHeap.allocate(vertex_location_count_22c * sizeof(srVector3T<float>)));
            if (lerp_buffer_448 == 0) {
                srAssertFail("m_pLerpBuffer",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp", 0x2d4, 0);
            }
        }
        if (frame_vertex_locations[frame] == 0) {
            AllocateFrameBuffers00471720(frame, 1);
            DecompressFrame(frame, 1, frame_vertex_locations[frame]);
        }
        if (frame_vertex_locations[next_frame] == 0) {
            AllocateFrameBuffers00471720(next_frame, 1);
            DecompressFrame(next_frame, 1, frame_vertex_locations[next_frame]);
        }
        if (lerp_buffer_448 != 0) {
            srVector3T<float>* next = frame_vertex_locations[next_frame];
            srVector3T<float>* current = frame_vertex_locations[frame];
            if (next != 0 && current != 0 && vertex_location_count_22c != 0) {
                if (interpolation == g_float_005ebb38) {
                    CopyDwordBuffer00470180(lerp_buffer_448, next, vertex_location_count_22c * 3);
                } else {
                    srVectorProcessor::lerp(&lerp_buffer_448->x, &next->x, &current->x,
                                            interpolation, vertex_location_count_22c * 3);
                }
            }
        }
        return lerp_buffer_448;
    }
    if (frame_vertex_locations[frame] == 0) {
        AllocateFrameBuffers00471720(frame, 1);
        if (load != 0 && frame_vertex_locations[frame] != 0) {
            DecompressFrame(frame, 1, frame_vertex_locations[frame]);
        }
    }
    return frame_vertex_locations[frame];
}

/* Return frame `frame`'s vertex normals, decompressing on demand when `load`
   is set. */
// FUNCTION: WIZ8 0x00471CA0
srVector3T<float>* stMeshModel::GetVertexNormals00471CA0(unsigned int frame, char load)
{
    if (frame_vertex_normals == 0) {
        return 0;
    }
    if (frame_vertex_normals[frame] == 0) {
        AllocateFrameBuffers00471720(frame, 2);
        if (load != 0 && frame_vertex_normals[frame] != 0) {
            DecompressFrame(frame, 2, frame_vertex_normals[frame]);
        }
    }
    return frame_vertex_normals[frame];
}

// FUNCTION: WIZ8 0x00471D00
srVector3T<float>* stMeshModel::GetPolygonNormals00471D00(unsigned int frame, char load)
{
    if (frame_polygon_normals == 0) {
        return 0;
    }
    if (frame_polygon_normals[frame] == 0) {
        AllocateFrameBuffers00471720(frame, 4);
        if (load != 0 && frame_polygon_normals[frame] != 0) {
            DecompressFrame(frame, 4, frame_polygon_normals[frame]);
        }
    }
    return frame_polygon_normals[frame];
}

// FUNCTION: WIZ8 0x00472990
void stMeshModel::SetAmbientColor00472990(const srVector3T<float>& color)
{
    if (ambient_color_3a4.x != color.x || ambient_color_3a4.y != color.y ||
        ambient_color_3a4.z != color.z) {
        ambient_color_3a4 = color;
        flags_3a0 |= 2;
    }
}

/* Build one frame's compressed polygon and vertex normals from its vertex
   locations. Vertex normals are summed per polygon corner, remapped through
   the shade index table when there is one, unitized, and stored as signed
   bytes scaled by 127. */
// FUNCTION: WIZ8 0x004729F0
void stMeshModel::ComputeFrameNormals(int frame)
{
    if (polygon_count_230 == 0 || vertex_location_count_22c == 0) {
        return;
    }

    srVector3i* poly_vertex = getPolyVertex();
    srVector3T<float>* pnorm = new srVector3T<float>[polygon_count_230];
    if (pnorm == 0 || poly_vertex == 0) {
        return;
    }

    srVector3T<float>* l = 0;
    unsigned char decompressed = 0;
    if (frame_vertex_locations != 0) {
        l = frame_vertex_locations[frame];
    }
    if (l == 0) {
        l = new srVector3T<float>[vertex_location_count_22c];
        if (l == 0) {
            srAssertFail("l", "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp", 0x4ca, 0);
        }
        DecompressFrame(frame, 1, l);
        decompressed = 1;
    }

    for (int poly = 0; poly < polygon_count_230; ++poly) {
        const srVector3T<float>& origin = l[poly_vertex[poly].x];
        srVector3T<float> edge_0 = l[poly_vertex[poly].y] - origin;
        srVector3T<float> edge_1 = l[poly_vertex[poly].z] - origin;
        pnorm[poly] = CrossProduct(edge_0, edge_1);
    }

    srVector3T<float>* vnorm = new srVector3T<float>[vertex_location_count_22c];
    unsigned long* shade_index = getVertexShadeIndex(0);
    if (vnorm == 0) {
        srAssertFail("vnorm", "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp", 0x4db, 0);
    }
    if (shade_index == 0) {
        FillDwordBuffer00474700(vnorm, 0, vertex_location_count_22c * 3);
        for (int corner_poly = 0; corner_poly < polygon_count_230; ++corner_poly) {
            vnorm[poly_vertex[corner_poly].x] += pnorm[corner_poly];
            vnorm[poly_vertex[corner_poly].y] += pnorm[corner_poly];
            vnorm[poly_vertex[corner_poly].z] += pnorm[corner_poly];
        }
    } else {
        srVector3T<float>* shaded = new srVector3T<float>[vertex_location_count_22c];
        FillDwordBuffer00474700(shaded, 0, vertex_location_count_22c * 3);
        for (int corner_poly = 0; corner_poly < polygon_count_230; ++corner_poly) {
            shaded[shade_index[poly_vertex[corner_poly].x]] += pnorm[corner_poly];
            shaded[shade_index[poly_vertex[corner_poly].y]] += pnorm[corner_poly];
            shaded[shade_index[poly_vertex[corner_poly].z]] += pnorm[corner_poly];
        }
        srVectorProcessor::copyIndexed(vnorm, shaded, shade_index, vertex_location_count_22c);
        delete[] shaded;
    }

    srVectorProcessor::normalize(vnorm, vnorm, 1.0f, vertex_location_count_22c);
    for (int vertex = 0; vertex < vertex_location_count_22c; ++vertex) {
        if (vnorm[vertex].x == 0.0f && vnorm[vertex].y == 0.0f && vnorm[vertex].z == 0.0f) {
            vnorm[vertex].x = 1e-6f;
            vnorm[vertex].y = 1e-6f;
            vnorm[vertex].z = 1e-6f;
        }
    }
    srVectorProcessor::mul(&vnorm->x, 127.0f, &vnorm->x, vertex_location_count_22c * 3);
    for (int index = 0; index < vertex_location_count_22c; ++index) {
        compressed_vertex_normals[frame][index * 3] = (char)vnorm[index].x;
        compressed_vertex_normals[frame][index * 3 + 1] = (char)vnorm[index].y;
        compressed_vertex_normals[frame][index * 3 + 2] = (char)vnorm[index].z;
    }

    srVectorProcessor::normalize(pnorm, pnorm, 1.0f, polygon_count_230);
    srVectorProcessor::mul(&pnorm->x, 127.0f, &pnorm->x, polygon_count_230 * 3);
    for (int polygon = 0; polygon < polygon_count_230; ++polygon) {
        compressed_polygon_normals[frame][polygon * 3] = (char)pnorm[polygon].x;
        compressed_polygon_normals[frame][polygon * 3 + 1] = (char)pnorm[polygon].y;
        compressed_polygon_normals[frame][polygon * 3 + 2] = (char)pnorm[polygon].z;
    }

    delete[] pnorm;
    delete[] vnorm;
    if (decompressed) {
        delete[] l;
    }
}

/* Thirteen-byte forwarder onto the per-frame normal builder. */
// FUNCTION: WIZ8 0x00472100
srVector3T<float>* stMeshModel::GetVertexLights(char initialize, int table)
{
    if (table == -1) {
        table = vertex_light_table_3b0;
    }
    srHeapArray<srVector3T<float> >& lights = vertex_lights_3b4[table];
    if (lights.data == 0 && initialize) {
        lights.setCapacity(vertex_location_count_22c, 0);
        srVector3T<float> zero(0.0f, 0.0f, 0.0f);
        for (unsigned int index = 0; index < lights.capacity; ++index) {
            lights.data[index] = zero;
        }
        if (vertex_sunlight_3c4.data != 0) {
            vertex_lighting_ready_3cd = 1;
        }
    }
    return lights.data;
}

// FUNCTION: WIZ8 0x004721E0
float* stMeshModel::GetVertexSunlight(char initialize)
{
    if (vertex_sunlight_3c4.data == 0 && initialize) {
        vertex_sunlight_3c4.setCapacity(vertex_location_count_22c, 0);
        for (unsigned int index = 0; index < vertex_sunlight_3c4.capacity; ++index) {
            vertex_sunlight_3c4.data[index] = 1.0f;
        }
        vertex_lighting_ready_3cd = 1;
    }
    return vertex_sunlight_3c4.data;
}

// FUNCTION: WIZ8 0x00473180
void stMeshModel::FinalizeVertexFrame00473180(int frame)
{
    ComputeFrameNormals(frame);
}

// TEMPLATE: WIZ8 0x004741E0
// srClassSupport<stMeshModel,srMeshModel,0,65539>::getClassID

// TEMPLATE: WIZ8 0x004741F0
// srClassSupport<stMeshModel,srMeshModel,0,65539>::getClassName

// TEMPLATE: WIZ8 0x00474200
// srClassSupport<stMeshModel,srMeshModel,0,65539>::clone

// TEMPLATE: WIZ8 0x00474820
// srClassSupport<stMeshModel,srMeshModel,0,65539>::getClassNode

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
    current_record_14->disable_mask_04 = 0;
    current_record_14->material_08 = material_80;
    current_pass_18->texture_00 = texture_78;
    current_pass_18->pass_value_04 = pass_value_7c;
    current_pass_18->flags_08.value = shader_74.value;
    current_pass_18->texture_array_0c = 0;
    current_pass_18->value_10 = 0;
    current_pass_18->shader_14 = 0;
    current_pass_18->st_18 = 0;
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
    triangle_count_1c = 0;
    active_triangles_2c = 0;
    projected_vertices_30 = 0;
    triangles_34 = 0;
    vertex_count_20 = 0;
    positions_38 = 0;
    vertex_extras_3c = 0;
    bounds_state_6c = 0;
    extra_40 = 0;
    shader_74.value = 0x0100241b;
    texture_78 = 0;
    pass_value_7c = 0;
    material_80 = srCore.getMaterial();

    current_record_14 = &records_94[slot_count_84];
    current_pass_18 = &passes_9c[slot_count_84];

    current_record_14->flags_00 = 0;
    current_record_14->disable_mask_04 = 0;
    current_record_14->material_08 = material_80;
    current_pass_18->texture_00 = texture_78;
    current_pass_18->pass_value_04 = pass_value_7c;
    current_pass_18->flags_08.value = shader_74.value;
    current_pass_18->texture_array_0c = 0;
    current_pass_18->value_10 = 0;
    current_pass_18->shader_14 = 0;
    current_pass_18->st_18 = 0;
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
    if (triangle_count_1c == 0 || (active_triangles_2c != 0 && active_triangle_count_24 == 0)) {
        return;
    }

    if (bounds_state_6c != 1) {
        if (bounds_state_6c == 0 && vertex_count_20 != 0) {
            srVectorProcessor::minMax(positions_38, bounds_minimum_44, bounds_maximum_50,
                                      vertex_count_20);
        }

        srVector3T<float> center;
        center.Set((bounds_minimum_44.x + bounds_maximum_50.x) * 0.5,
                   (bounds_minimum_44.y + bounds_maximum_50.y) * 0.5,
                   (bounds_minimum_44.z + bounds_maximum_50.z) * 0.5);
        bounds_center_5c = center;

        bounds_radius_68 = (bounds_center_5c - bounds_minimum_44).Length() * 1.001f;
    }

    srVector3T<float> eye_center;
    float eye_radius;
    srMatrix4T<float> model_view;
    srMatrix4T<float> inverse_model_view;
    srGERD::ClipPlanes clip_planes;
    srMatrix4T<float> project_clip_near;
    srMatrix4T<float> normal_matrix;

    renderer_88->getEyeSpaceBounds(eye_center, eye_radius, bounds_center_5c, bounds_radius_68);
    renderer_88->getMatrix(srGERD::MATRIX_MODELVIEW, model_view);
    renderer_88->getInverseModelViewMatrix(inverse_model_view);
    renderer_88->getClipPlanes(clip_planes);
    renderer_88->getProjectClipNearMatrix(project_clip_near);
    renderer_88->getNormalMatrix(normal_matrix);

    srMatrix4T<float>::e_scaleType scale_type = renderer_88->getModelViewScaleType();
    srGERD::e_cullMode cull_mode = renderer_88->getCullMode();
    srGERD::e_winding winding = renderer_88->getWinding();

    srTriangleCuller::Input culler_input;
    if (cull_mode == srGERD::CULL_FRONT) {
        culler_input.cull_mode_0c = 2;
    } else if (cull_mode == srGERD::CULL_BACK) {
        culler_input.cull_mode_0c = winding == srGERD::WINDING_POSITIONAL_0;
    } else {
        culler_input.cull_mode_0c = winding != srGERD::WINDING_POSITIONAL_0;
    }

    culler_input.vertex_count_04 = vertex_count_20;
    culler_input.vertices_1c = positions_38;
    culler_input.clip_planes_20 = clip_planes.planes_000;
    culler_input.model_view_24 = &model_view;
    culler_input.inverse_model_view_28 = &inverse_model_view;
    culler_input.scale_type_2c = scale_type;

    if ((flags_28 & 1) == 0) {
        culler_input.clip_mask_30 = 0;
    } else {
        float depth;
        unsigned long clip_mask = srTriangleCuller::getClipMask(
            eye_center, eye_radius, clip_planes.planes_000, clip_planes.mask_200, depth);
        int retain_clip_mask =
            clip_mask != 0 && ((clip_mask & 0xffffffc0UL) != 0 ||
                               (slot_count_84 * triangle_count_1c > 45 && depth > 0.23f));
        culler_input.clip_mask_30 = retain_clip_mask ? clip_mask : 0;
    }

    if (active_triangles_2c == 0) {
        srCore.getStatisticsManager()->statistics_00.triangles_submitted_10 +=
            slot_count_84 * triangle_count_1c;
    } else {
        srCore.getStatisticsManager()->statistics_00.triangles_submitted_10 +=
            slot_count_84 * active_triangle_count_24;
    }
    ++srCore.getStatisticsManager()->statistics_00.meshes_submitted_0c;
    srCore.getStatisticsManager()->statistics_00.vertices_submitted_18 +=
        slot_count_84 * vertex_count_20;

    unsigned long total = active_triangles_2c == 0 ? triangle_count_1c : active_triangle_count_24;
    unsigned long batch_limit = total;
    if ((flags_28 & 2) != 0) {
        double ratio = static_cast<double>(vertex_count_20) / triangle_count_1c;
        if (ratio > 3.0f) {
            ratio = 3.0f;
        }
        batch_limit = static_cast<unsigned long>(1300.0f / (slot_count_84 * ratio));
        if (cull_mode == srGERD::CULL_FRONT) {
            batch_limit >>= 1;
        }
        if (batch_limit > total) {
            batch_limit = total;
        }
    }

    unsigned long* scratch = culler_scratch_0c.ensure(batch_limit + vertex_count_20 * 2);
    srTriangleCuller::Output culler_output;
    culler_output.indices_00 = scratch;
    culler_output.avt_04 = scratch + batch_limit;
    culler_output.clip_flags_08 = culler_output.avt_04 + vertex_count_20;

    unsigned long processed = 0;
    while (processed < total) {
        unsigned long batch_count = total - processed;
        if (batch_count > batch_limit) {
            batch_count = batch_limit;
        }

        if (active_triangles_2c == 0) {
            culler_input.triangle_count_00 = batch_count;
            culler_input.active_triangle_count_08 = 0;
            culler_input.active_triangles_10 = 0;
            culler_input.projected_vertices_14 = projected_vertices_30 + processed;
            culler_input.triangles_18 = triangles_34 + processed;
        } else {
            culler_input.triangle_count_00 = triangle_count_1c;
            culler_input.active_triangle_count_08 = batch_count;
            culler_input.active_triangles_10 = active_triangles_2c + processed;
            culler_input.projected_vertices_14 = projected_vertices_30;
            culler_input.triangles_18 = triangles_34;
        }

        if (srTriangleCuller::cull(culler_output, culler_input)) {
            srCore.getStatisticsManager()->statistics_00.triangles_after_culling_14 +=
                slot_count_84 * culler_output.triangle_count_0c;
            srCore.getStatisticsManager()->statistics_00.vertices_after_culling_1c +=
                slot_count_84 * culler_output.vertex_count_10;

            srGERD::Renderer* renderer = renderer_88->lockRenderer();

            (void)vertex_arrays_a4[slot_count_84];
            srVertexArray* vertex_arrays = &vertex_arrays_a4[0];
            renderer->allocVertexArray(vertex_arrays[0],
                                       slot_count_84 * culler_output.vertex_count_10);

            for (unsigned long slot = 1; slot < slot_count_84; ++slot) {
                unsigned long offset = slot * culler_output.vertex_count_10;
                vertex_arrays[slot].eye_locations_00 = vertex_arrays[0].eye_locations_00 + offset;
                vertex_arrays[slot].diffuse_04 = vertex_arrays[0].diffuse_04 + offset;
                vertex_arrays[slot].specular_08 = vertex_arrays[0].specular_08 + offset;
                vertex_arrays[slot].st0_0c = vertex_arrays[0].st0_0c + offset;
                vertex_arrays[slot].st1_10 = vertex_arrays[0].st1_10 + offset;
                vertex_arrays[slot].q0_14 = vertex_arrays[0].q0_14 + offset;
                vertex_arrays[slot].q1_18 = vertex_arrays[0].q1_18 + offset;
                vertex_arrays[slot].packed_1c = vertex_arrays[0].packed_1c + offset;
            }

            unsigned long processor_count = renderer_88->getVertexProcessorCount();
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
            renderer_88->getEnvironmentRange(environment_minimum, environment_maximum);
            renderer_88->getEnvironmentScaleFactor(environment_scale, environment_inverse_scale);
            unsigned long exclusion_mask = renderer_88->getExclusionMask();

            srVertexPipe::Input pipe_input;
            pipe_input.record_count_00 = slot_count_84;
            pipe_input.vertex_count_04 = culler_output.vertex_count_10;
            pipe_input.indices_08 = culler_output.avt_04;
            pipe_input.position_is_float3_0c = culler_output.linear_14 == 0;
            pipe_input.positions_10 = positions_38;
            pipe_input.values_14 = vertex_extras_3c;
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
            pipe_input.environment_inverse_scale_60 = environment_inverse_scale;

            if (active_triangles_2c == 0 && processed != 0) {
                for (unsigned long index = 0; index < batch_count; ++index) {
                    culler_output.indices_00[index] += processed;
                }
            }

            unsigned long renderer_disable_mask = 0;
            if (renderer_88->getMaxTextureStages() == 1) {
                renderer_disable_mask = 0x140;
            }

            for (unsigned long pass_index = 0; pass_index < slot_count_84; ++pass_index) {
                passes_9c[pass_index].st_18 = records_94[pass_index].st0_20;

                unsigned long disable_mask;
                if (passes_9c[pass_index].shader_14 != 0) {
                    srFlags<srVertexProcessor::e_channel> flags =
                        srVertexPipe::getShaderDisableMask(passes_9c[pass_index].shader_14,
                                                           culler_output.indices_00,
                                                           culler_output.triangle_count_0c);
                    disable_mask = flags.value;
                } else {
                    srFlags<srVertexProcessor::e_channel> flags =
                        srVertexPipe::getShaderDisableMask(passes_9c[pass_index].flags_08);
                    disable_mask = flags.value;
                }
                records_94[pass_index].disable_mask_04 = disable_mask | renderer_disable_mask;
            }

            vertex_pipe_90->process(pipe_input);

            srGERD::Renderer::TriInput render_input;
            render_input.triangle_count_00 = culler_output.triangle_count_0c;
            render_input.record_count_04 = slot_count_84;
            render_input.vertex_count_08 = culler_output.vertex_count_10;
            render_input.indices_0c = culler_output.indices_00;
            render_input.triangles_10 = triangles_34;
            render_input.vertices_14 = culler_output.clip_flags_08;
            render_input.passes_18 = &passes_9c[0];
            render_input.position_is_float3_1c = culler_output.linear_14 == 0;
            render_input.project_clip_near_20 = &project_clip_near;
            render_input.value_24 = extra_40;
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
    pipeline->triangle_count_1c = 0;
    pipeline->active_triangles_2c = 0;
    pipeline->projected_vertices_30 = 0;
    pipeline->triangles_34 = 0;
    pipeline->vertex_count_20 = 0;
    pipeline->positions_38 = 0;
    pipeline->vertex_extras_3c = 0;
    pipeline->bounds_state_6c = 0;
    pipeline->extra_40 = 0;
    pipeline->shader_74.value = 0x0100241b;
    pipeline->texture_78 = 0;
    pipeline->pass_value_7c = 0;
    pipeline->material_80 = srCore.getMaterial();
    pipeline->PrepareSlot00475540();
    return pipe;
}
// FUNCTION: WIZ8 0x005aa400
void stMeshModel::NotifyLinkedModel005AA400(stMeshModel*) {}
