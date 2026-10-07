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

extern srVector3T<float> g_environment_offset;
extern float g_monster_light_scale;

// GLOBAL: WIZ8 0x00659cb8
W8GrowableVector<stMeshModel*> g_mesh_models;

// GLOBAL: WIZ8 0x0065a0e8
int g_decompressed_mesh_bytes;

/* Active-polygon scratch for the optional software backface pass in
   RenderTriMeshWithEquations. Layout matches srHeapBuffer<ulong>. */
// GLOBAL: WIZ8 0x00659ce0
srHeapBuffer<unsigned long> g_software_cull_active_polygons;

/* Byte budget for the decompressed per-frame caches; AllocateFrameBuffers
   reclaims least-recently-used frames past it. */
// GLOBAL: WIZ8 0x00609d34
static int g_decompressed_mesh_byte_limit = 0x800000;

/* Signed-byte normal components back to floats, indexed by the raw byte. */
// GLOBAL: WIZ8 0x00659ce8
static float s_compressed_normal_table[256];

// GLOBAL: WIZ8 0x0065a0ef
static bool s_compressed_normal_table_ready;

// FUNCTION: WIZ8 0x00470B00
stMeshModel::stMeshModel(long polygons, long vertices)
    : srClassSupport<stMeshModel, srMeshModel, false, 0x10003>(0, 0), skin_table_ids(5),
      skin_texture_tables(5), skin_table_names(5), mapped_values(5), mapped_keys(5)
{
    if (!s_compressed_normal_table_ready) {
        for (int value = -128; value < 128; ++value) {
            float component = value * (1.0f / 127.0f);
            if (component < -1.0f) {
                component = -1.0f;
            } else if (component > 1.0f) {
                component = 1.0f;
            }
            s_compressed_normal_table[static_cast<unsigned char>(value)] = component;
        }
        s_compressed_normal_table_ready = true;
    }

    memset(&tri_mesh, 0, sizeof(tri_mesh));
    srMeshModel::reset(polygons, vertices);
    sort_bias = 0.0f;

    for (int pass = 0; pass < 4; ++pass) {
        int side;
        for (side = 0; side < 2; ++side) {
            materials[pass][side] = 0;
        }
        for (side = 0; side < 2; ++side) {
            textures[pass][side] = 0;
        }
        shaders[pass] = srShader();
    }
    duplicate_on_reuse = 1;
    vertex_lighting_ready = false;
    previous = 0;
    next = 0;
    flags = W8_MESH_VERTEX_LIGHTING_DIRTY;
    ambient_color = -1.0f;
    frame_count = 1;
    vertex_light_table = 0;
    compressed_vertex_locations = 0;
    compressed_vertex_normals = 0;
    compressed_polygon_normals = 0;
    m_pVertexLoc = 0;
    m_pVertexNormal = 0;
    m_pPolyNormal = 0;
    render_control.value &= ~0x10UL;
    setDirty(DIRTY_BOUNDS);
    lerp_buffer = 0;
    automap_polygons = 0;
    automap_polygon_count = 0;
    automap_filter_active = false;
    skin_blanking_apt = 0;
    skin_blanking_apt_number = 0;
    skin_blanking_checked = 0;
}

// FUNCTION: WIZ8 0x00470ED0
stMeshModel::~stMeshModel()
{
    if (next != 0) {
        stMeshModel* linked = next;
        next = 0;
        delete linked;
    }
    FreeFrameStorage();
    while (skin_table_names.GetCount() != 0) {
        RemoveSkinTable(0);
    }
    if ((flags & W8_MESH_HAS_FRAME_STORAGE) != 0) {
        g_mesh_models.Remove(this);
    }
    if (lerp_buffer != 0) {
        srHeap.free(lerp_buffer);
        lerp_buffer = 0;
    }
    if (automap_polygons != 0) {
        delete[] automap_polygons;
        automap_polygons = 0;
    }
    if (skin_blanking_apt != 0) {
        delete skin_blanking_apt;
    }
    if (skin_blanking_apt_number != 0) {
        delete skin_blanking_apt_number;
    }
    if (skin_blanking_checked != 0) {
        delete skin_blanking_checked;
    }
}

// FUNCTION: WIZ8 0x004748c0
srClass* stMeshModel::vInstance()
{
    return new stMeshModel(0, 0);
}

// FUNCTION: WIZ8 0x00471dd0
int stMeshModel::getBoundingSphere(srVector3T<float>& center, float& radius)
{
    if ((dirty_flags.value & 1) != 0) {
        CalculateLinkedBounds();
    }
    center = bounds_center;
    radius = bounds_radius;
    return 1;
}

// FUNCTION: WIZ8 0x00471d80
int stMeshModel::getBoundingBox(srVector3T<float>& minimum, srVector3T<float>& maximum)
{
    if ((dirty_flags.value & 1) != 0) {
        CalculateLinkedBounds();
    }
    minimum = bounds_minimum;
    maximum = bounds_maximum;
    return 1;
}

/* Recompute the linked group's union bounds: clear this model's cached box,
   accumulate every frame's bounds from the whole previous/next chain, then
   push the resulting box, center and radius to each member. Models whose
   flags bit 2 is clear answer from the base-class bounding box; the rest
   decompress each frame's vertex table. */
// FUNCTION: WIZ8 0x00471E10
void stMeshModel::CalculateLinkedBounds()
{
    bounds_minimum.SetZero();
    bounds_maximum.SetZero();
    bounds_center.SetZero();
    bounds_radius = 0;

    stMeshModel* head = this;
    while (head->previous != 0) {
        head = head->previous;
    }

    stMeshModel* model;
    srVector3T<float> minimum;
    srVector3T<float> maximum;
    minimum.SetZero();
    maximum.SetZero();
    for (model = head; model != 0; model = model->next) {
        for (unsigned int frame = 0; frame < model->frame_count; ++frame) {
            srVector3T<float> frame_minimum;
            srVector3T<float> frame_maximum;
            if ((model->flags & W8_MESH_HAS_FRAME_STORAGE) == 0) {
                model->srMeshModel::getBoundingBox(frame_minimum, frame_maximum);
            } else {
                model->GetFrameBounds(frame, &frame_minimum, &frame_maximum);
            }
            if (frame_minimum.x < minimum.x) {
                minimum.x = frame_minimum.x;
            }
            if (frame_minimum.y < minimum.y) {
                minimum.y = frame_minimum.y;
            }
            if (frame_minimum.z < minimum.z) {
                minimum.z = frame_minimum.z;
            }
            if (maximum.x < frame_maximum.x) {
                maximum.x = frame_maximum.x;
            }
            if (maximum.y < frame_maximum.y) {
                maximum.y = frame_maximum.y;
            }
            if (maximum.z < frame_maximum.z) {
                maximum.z = frame_maximum.z;
            }
        }
    }

    srVector3T<float> center;
    center.Set((maximum.x + minimum.x) * g_double_half, (maximum.y + minimum.y) * g_double_half,
               (maximum.z + minimum.z) * g_double_half);
    float radius = static_cast<float>(sqrt((maximum.x - center.x) * (maximum.x - center.x) +
                                           (maximum.y - center.y) * (maximum.y - center.y) +
                                           (maximum.z - center.z) * (maximum.z - center.z)));
    float minimum_distance =
        static_cast<float>(sqrt((minimum.x - center.x) * (minimum.x - center.x) +
                                (minimum.y - center.y) * (minimum.y - center.y) +
                                (minimum.z - center.z) * (minimum.z - center.z)));
    if (radius < minimum_distance) {
        radius = minimum_distance;
    }
    head->setBounds(minimum, maximum, center, radius);
    for (model = head->next; model != 0; model = model->next) {
        model->setBounds(minimum, maximum, center, radius);
    }
}

/* Bounds `frame`'s vertex table into `minimum`/`maximum`. When the frame is
   not resident it is decompressed into a scratch array that is released
   afterward; empty tables produce the zero vector on both outputs. */
// FUNCTION: WIZ8 0x00473190
void stMeshModel::GetFrameBounds(int frame, srVector3T<float>* minimum, srVector3T<float>* maximum)
{
    minimum->SetZero();
    maximum->SetZero();
    if (m_pVertexLoc != 0 && vertex_location_count != 0 &&
        static_cast<unsigned int>(frame) < frame_count) {
        srVector3T<float>* vertices = m_pVertexLoc[frame];
        if (vertices == 0) {
            vertices = new srVector3T<float>[vertex_location_count];
            if (vertices == 0) {
                return;
            }
            DecompressFrame(frame, W8_MESH_FRAME_LOCATIONS, vertices);
        }
        if (vertex_location_count != 0) {
            srVectorProcessor::minMax(vertices, *minimum, *maximum,
                                      static_cast<SRDWORD>(vertex_location_count));
        }
        if (m_pVertexLoc[frame] == 0) {
            delete[] vertices;
        }
    }
}

/* Apply pending vertex DIG lighting when flags bit 1 is set, then return
   the SurRender TriMesh cache. */
// FUNCTION: WIZ8 0x00472270
const srMeshModel::TriMesh& stMeshModel::getTriMesh()
{
    float light_scale = g_monster_light_scale;
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

    if ((flags & W8_MESH_VERTEX_LIGHTING_DIRTY) != 0 && !g_render_unlit) {
        lights = vertex_lights[vertex_light_table].data;
        sunlight = vertex_sunlight.data;
        if (lights != 0 && sunlight != 0) {
            dig = getVertexDIG(0, 1);
            vertex_materials = getVertexMaterial(0, SIDE_FRONT, 0);
            if (vertex_materials == 0) {
                if ((IsZeroVector(&ambient_color) != 0) || vertex_light_table == 1) {
                    CopyDwordBuffer(dig, lights, vertex_location_count * 3);
                    if ((g_environment_offset.x != g_float_zero ||
                         g_environment_offset.y != g_float_zero ||
                         g_environment_offset.z != g_float_zero) &&
                        vertex_light_table != 1 && (count = vertex_location_count, count != 0) &&
                        IsZeroVector(&g_environment_offset) == 0) {
                        srVectorProcessor::add(dig, g_environment_offset, dig,
                                               static_cast<SRDWORD>(count));
                    }
                } else {
                    /* Retail indexes material ambient at +0x28; that is
                       srMaterial::parms.ambient on the concrete type. */
                    material = static_cast<srMaterial*>(getMaterial(0, SIDE_FRONT));
                    ambient_rgb = material->parms.ambient.xyz();
                    count = vertex_location_count;
                    scaled = ambient_color;
                    scaled *= ambient_rgb;
                    if (count != 0) {
                        if (scaled.x == scaled.y && scaled.x == scaled.z) {
                            unsigned int bits;
                            // reinterpret-ok: FillDwordBuffer takes the float bit pattern
                            bits = *reinterpret_cast<unsigned int*>(&scaled.x);
                            FillDwordBuffer(dig, bits, count * 3);
                        } else {
                            srVectorProcessor::copy(dig, scaled, static_cast<SRDWORD>(count));
                        }
                    }
                    if (vertex_location_count != 0) {
                        srVectorProcessor::mul(dig, dig, sunlight,
                                               static_cast<SRDWORD>(vertex_location_count));
                    }
                    AddFloatBuffer(
                        reinterpret_cast<float*>(dig),    // reinterpret-ok: packed DIG as float*
                        reinterpret_cast<float*>(lights), // reinterpret-ok: packed lights as float*
                        vertex_location_count * 3);
                    if ((g_environment_offset.x != g_float_zero ||
                         g_environment_offset.y != g_float_zero ||
                         g_environment_offset.z != g_float_zero) &&
                        (count = vertex_location_count, count != 0) &&
                        IsZeroVector(&g_environment_offset) == 0) {
                        srVectorProcessor::add(dig, g_environment_offset, dig,
                                               static_cast<SRDWORD>(count));
                    }
                }
            } else {
                if ((IsZeroVector(&ambient_color) != 0) || vertex_light_table == 1) {
                    if (vertex_location_count != 0) {
                        FillDwordBuffer(dig, 0, vertex_location_count * 3);
                    }
                } else {
                    count = vertex_location_count;
                    if (count != 0) {
                        if (ambient_color.x == ambient_color.y &&
                            ambient_color.x == ambient_color.z) {
                            unsigned int bits;
                            // reinterpret-ok: FillDwordBuffer takes the float bit pattern
                            bits = *reinterpret_cast<unsigned int*>(&ambient_color.x);
                            FillDwordBuffer(dig, bits, count * 3);
                        } else {
                            srVectorProcessor::copy(dig, ambient_color,
                                                    static_cast<SRDWORD>(count));
                        }
                    }
                    if (vertex_location_count != 0) {
                        srVectorProcessor::mul(dig, dig, sunlight,
                                               static_cast<SRDWORD>(vertex_location_count));
                    }
                }
                index = 0;
                do {
                    next_index = index + 1;
                    run = 1;
                    if (next_index < vertex_location_count) {
                        do {
                            if (static_cast<srMaterialIFace*>(vertex_materials[index]) !=
                                static_cast<srMaterialIFace*>(vertex_materials[next_index])) {
                                break;
                            }
                            ++run;
                            ++next_index;
                        } while (next_index < vertex_location_count);
                    }
                    material_iface = vertex_materials[index];
                    if (material_iface != 0) {
                        material = static_cast<srMaterial*>(material_iface);
                        scaled = material->parms.ambient.xyz();
                        if (run != 0) {
                            if (IsZeroVector(&scaled) == 0) {
                                srVectorProcessor::mul(dig + index, scaled, dig + index,
                                                       static_cast<SRDWORD>(run));
                            } else {
                                FillDwordBuffer(dig + index, 0, run * 3);
                            }
                        }
                    }
                    index += run;
                } while (index < vertex_location_count);
                AddFloatBuffer(
                    reinterpret_cast<float*>(dig),    // reinterpret-ok: packed DIG as float*
                    reinterpret_cast<float*>(lights), // reinterpret-ok: packed lights as float*
                    vertex_location_count * 3);
                if ((g_environment_offset.x != g_float_zero ||
                     g_environment_offset.y != g_float_zero ||
                     g_environment_offset.z != g_float_zero) &&
                    vertex_light_table != 1 && (count = vertex_location_count, count != 0) &&
                    IsZeroVector(&g_environment_offset) == 0) {
                    srVectorProcessor::add(dig, g_environment_offset, dig,
                                           static_cast<SRDWORD>(count));
                }
            }
            if (light_scale != g_float_one && (count = vertex_location_count, count != 0)) {
                if (light_scale == g_float_zero) {
                    FillDwordBuffer(dig, 0, count * 3);
                } else {
                    srVectorProcessor::mul(
                        reinterpret_cast<float*>(dig), // reinterpret-ok: packed DIG as float*
                        light_scale,
                        reinterpret_cast<float*>(dig), // reinterpret-ok: packed DIG as float*
                        static_cast<SRDWORD>(count) * 3);
                }
            }
        }
        flags &= ~static_cast<unsigned int>(W8_MESH_VERTEX_LIGHTING_DIRTY);
    }
    return srMeshModel::getTriMesh();
}

// FUNCTION: WIZ8 0x004727e0
void stMeshModel::getTriMesh(TriMesh& mesh)
{
    mesh = getTriMesh();
}

/* Thin vtable wrapper: submit with no external poly-equation table. */
// FUNCTION: WIZ8 0x00470360
void stMeshModel::renderTriMesh(srGERD& renderer, const TriMesh& mesh)
{
    RenderTriMeshWithEquations(renderer, mesh, 0);
}

/* Wizardry-extended srMeshModel::renderTriMesh. Optional polygon normals enable
   a software backface cull into g_software_cull_active_polygons and
   forces CULL_NONE; a null table leaves hardware cull at CULL_BACK unless
   g_render_cull_front already requested front culling. */
// FUNCTION: WIZ8 0x00470380
void stMeshModel::RenderTriMeshWithEquations(srGERD& renderer, const TriMesh& mesh,
                                             const srVector3T<float>* arg_poly_equations)
{
    unsigned long active_count = 0;
    srShader shader;

    if (mesh.polygon_count != 0 && mesh.vertex_count != 0) {
        renderer.pushEnable();
        if ((g_inverted_depth_render ||
             (mesh.control_flags & (1UL << srMeshModel::CONTROL_SORTED_RENDERING)) != 0) &&
            !renderer.isEnabled(srGERD::ENABLE_SORTED_RENDERING)) {
            renderer.toggle(srGERD::ENABLE_SORTED_RENDERING);
        }

        if (g_render_cull_front) {
            renderer.setCullMode(srGERD::CULL_NONE);
        } else if (arg_poly_equations != 0) {
            renderer.setCullMode(srGERD::CULL_NONE);

            g_software_cull_active_polygons.setCapacity(
                static_cast<unsigned long>(mesh.polygon_count), 1);

            srMatrix4T<float> inverse_model_view;
            renderer.getInverseModelViewMatrix(inverse_model_view);
            srVector3T<float> eye(inverse_model_view.vectors[0].w, inverse_model_view.vectors[1].w,
                                  inverse_model_view.vectors[2].w);

            if (mesh.active_polygons == 0) {
                if (renderer.getWinding() == srGERD::WINDING_POSITIONAL_0) {
                    for (long polygon = 0; polygon < mesh.polygon_count; ++polygon) {
                        int vertex = mesh.poly_vertices[polygon].y;
                        const srVector3T<float>& position = mesh.positions[vertex];
                        const srVector3T<float>& equation = arg_poly_equations[polygon];
                        float facing = (eye.x - position.x) * equation.x +
                                       (eye.y - position.y) * equation.y +
                                       (eye.z - position.z) * equation.z;
                        if (static_cast<float>(g_double_zero) <= facing) {
                            g_software_cull_active_polygons.data[active_count] =
                                static_cast<unsigned long>(polygon);
                            ++active_count;
                        }
                    }
                } else {
                    for (long polygon = 0; polygon < mesh.polygon_count; ++polygon) {
                        int vertex = mesh.poly_vertices[polygon].y;
                        const srVector3T<float>& position = mesh.positions[vertex];
                        const srVector3T<float>& equation = arg_poly_equations[polygon];
                        float facing = (eye.x - position.x) * equation.x +
                                       (eye.y - position.y) * equation.y +
                                       (eye.z - position.z) * equation.z;
                        if (facing <= static_cast<float>(g_double_zero)) {
                            g_software_cull_active_polygons.data[active_count] =
                                static_cast<unsigned long>(polygon);
                            ++active_count;
                        }
                    }
                }
            } else if (renderer.getWinding() == srGERD::WINDING_POSITIONAL_0) {
                for (long index = 0; index < mesh.active_polygon_count; ++index) {
                    unsigned long polygon = mesh.active_polygons[index];
                    int vertex = mesh.poly_vertices[polygon].y;
                    const srVector3T<float>& position = mesh.positions[vertex];
                    const srVector3T<float>& equation = arg_poly_equations[polygon];
                    float facing = (eye.x - position.x) * equation.x +
                                   (eye.y - position.y) * equation.y +
                                   (eye.z - position.z) * equation.z;
                    if (static_cast<float>(g_double_zero) <= facing) {
                        g_software_cull_active_polygons.data[active_count] = polygon;
                        ++active_count;
                    }
                }
            } else {
                for (long index = 0; index < mesh.active_polygon_count; ++index) {
                    unsigned long polygon = mesh.active_polygons[index];
                    int vertex = mesh.poly_vertices[polygon].y;
                    const srVector3T<float>& position = mesh.positions[vertex];
                    const srVector3T<float>& equation = arg_poly_equations[polygon];
                    float facing = (eye.x - position.x) * equation.x +
                                   (eye.y - position.y) * equation.y +
                                   (eye.z - position.z) * equation.z;
                    if (facing <= static_cast<float>(g_double_zero)) {
                        g_software_cull_active_polygons.data[active_count] = polygon;
                        ++active_count;
                    }
                }
            }
        } else {
            renderer.setCullMode(srGERD::CULL_BACK);
        }

        for (int side = 1; side >= 0; --side) {
            if ((mesh.control_flags & (1u << side)) != 0) {
                srTriMeshPipeline* pipeline = srTriMeshPipeline::Get(&renderer);
                pipeline->sort_bias = mesh.sort_bias;
                pipeline->triangles = mesh.poly_vertices;
                pipeline->triangle_count = static_cast<unsigned long>(mesh.polygon_count);
                pipeline->positions = mesh.positions;
                pipeline->vertex_count = static_cast<unsigned long>(mesh.vertex_count);
                pipeline->vertex_extras = mesh.normals;

                if (arg_poly_equations != 0) {
                    pipeline->projected_vertices = 0;
                } else {
                    pipeline->projected_vertices = mesh.poly_equations;
                }

                if (arg_poly_equations != 0) {
                    pipeline->active_triangles = g_software_cull_active_polygons.data;
                    pipeline->active_triangle_count = active_count;
                } else if (mesh.active_polygons != 0) {
                    pipeline->active_triangles = mesh.active_polygons;
                    pipeline->active_triangle_count = mesh.active_polygon_count;
                }

                if ((mesh.control_flags & (1UL << srMeshModel::CONTROL_SKIP_AUTO_BOX)) == 0) {
                    pipeline->bounds_minimum = mesh.bounds_minimum;
                    pipeline->bounds_maximum = mesh.bounds_maximum;
                    if (pipeline->bounds_source == srTriMeshPipeline::BOUNDS_FROM_VERTICES) {
                        pipeline->bounds_source = srTriMeshPipeline::BOUNDS_BOX;
                    }
                }
                if ((mesh.control_flags & (1UL << srMeshModel::CONTROL_SKIP_AUTO_SPHERE)) == 0) {
                    pipeline->bounds_center = mesh.bounds_center;
                    pipeline->bounds_radius = mesh.bounds_radius;
                    pipeline->bounds_source = srTriMeshPipeline::BOUNDS_SPHERE;
                }

                for (long pass = 0; pass < mesh.pass_count; ++pass) {
                    pipeline->current_record->flags = 0;
                    pipeline->current_pass->shaders = 0;
                    pipeline->current_pass->texture_tables[0] = 0;
                    pipeline->current_pass->texture_tables[1] = 0;

                    if (mesh.dig[pass] != 0) {
                        pipeline->current_record->colors = mesh.dig[pass];
                        pipeline->current_record->color_format =
                            srVertexPipe::Record::ColorSource::FORMAT_VECTOR3;
                        pipeline->current_record->flags |= srVertexPipe::Record::HAS_COLORS;
                    }
                    if (mesh.dcg[pass] != 0) {
                        pipeline->current_record->dcg = mesh.dcg[pass];
                        pipeline->current_record->flags |=
                            srVertexPipe::Record::HAS_DIFFUSE_MULTIPLIERS;
                    }
                    if (mesh.scg[pass] != 0) {
                        pipeline->current_record->scg = mesh.scg[pass];
                        pipeline->current_record->flags |=
                            srVertexPipe::Record::HAS_SPECULAR_MULTIPLIERS;
                    }

                    if (mesh.vertex_materials[pass][side] == 0) {
                        srMaterialIFace* material = mesh.materials[pass][side];
                        pipeline->material = material;
                        pipeline->current_record->material = material;
                    } else {
                        pipeline->current_record->vertex_materials =
                            mesh.vertex_materials[pass][side];
                        pipeline->current_record->flags |=
                            srVertexPipe::Record::HAS_VERTEX_MATERIALS;
                    }

                    if (mesh.poly_uv[pass] != 0) {
                        pipeline->current_pass->poly_uv = mesh.poly_uv[pass];
                    }

                    if (mesh.poly_shaders[pass] == 0) {
                        shader.value = mesh.shaders[pass].value;
                        if (g_inverted_depth_render) {
                            shader.value = (shader.value & 0xfffffffeUL) | 6UL;
                        }
                        pipeline->SetFlags(shader);
                    } else {
                        pipeline->current_pass->shaders = mesh.poly_shaders[pass];
                    }

                    if (mesh.texcoords[pass][0] != 0) {
                        pipeline->current_record->st0 = mesh.texcoords[pass][0];
                        pipeline->current_record->flags |= srVertexPipe::Record::HAS_TEXCOORD0;
                    }
                    if (mesh.texcoords[pass][1] != 0) {
                        pipeline->current_record->flags |= srVertexPipe::Record::HAS_TEXCOORD1;
                        pipeline->current_record->st1 = mesh.texcoords[pass][1];
                    }

                    for (int layer = 0; layer < 2; ++layer) {
                        if (mesh.poly_textures[pass][layer] == 0) {
                            srTextureIFace* texture = mesh.textures[pass][layer];
                            (&pipeline->texture0)[layer] = texture;
                            pipeline->current_pass->textures[layer] = texture;
                        } else {
                            pipeline->current_pass->texture_tables[layer] =
                                mesh.poly_textures[pass][layer];
                        }
                    }

                    ++pipeline->slot_count;
                    pipeline->PrepareSlot();
                }

                pipeline->FlushIfCurrent();
            }
        }

        renderer.popEnable();
        last_decompress_release_tick = GetTickCount();
    }
}

/* Copy `count` dwords with a plain pointer walk. Distinct from
   CopyDwordBuffer (vp memcopy + self-copy guard). Retail tests the
   count before the loop's own guard. */
// FUNCTION: WIZ8 0x004747f0
void CopyUlongBuffer(unsigned long* destination, const unsigned long* source, unsigned long count)
{
    if (count != 0) {
        for (unsigned long index = 0; index < count; ++index) {
            destination[index] = source[index];
        }
    }
}

/* Copy `count` dwords between distinct buffers through the imported vp. */
// FUNCTION: WIZ8 0x00470180
void CopyDwordBuffer(void* destination, const void* source, int count)
{
    if (count != 0 && destination != source) {
        srVectorProcessor::memcopy(destination, source, count << 2);
    }
}

// FUNCTION: WIZ8 0x0046ffa0
int __fastcall IsZeroVector(const srVector3T<float>* vector)
{
    if (vector->x == g_float_zero && vector->y == g_float_zero && vector->z == g_float_zero) {
        return 1;
    }
    return 0;
}

/* Fill `count` dwords through vp->_copy(SRDWORD*, SRDWORD, SRDWORD). */
// FUNCTION: WIZ8 0x00474700
void FillDwordBuffer(void* destination, unsigned int value, int count)
{
    if (count != 0) {
        srVectorProcessor::copy(static_cast<SRDWORD*>(destination), value,
                                static_cast<SRDWORD>(count));
    }
}

/* dest[i] += source[i] through vp->_add(float*, dest, source, count). */
// FUNCTION: WIZ8 0x00474730
void AddFloatBuffer(float* destination, const float* source, int count)
{
    if (count != 0) {
        srVectorProcessor::add(destination, destination, source, static_cast<SRDWORD>(count));
    }
}

/* Copy or translate `count` vertices: a zero offset is a plain copy and a
   nonzero one goes through the vp constant-vector add. */
// FUNCTION: WIZ8 0x00470040
void OffsetVertices(srVector3T<float>* destination, const srVector3T<float>* source,
                    const srVector3T<float>* offset, int count)
{
    if (count != 0) {
        if (IsZeroVector(offset) != 0) {
            CopyDwordBuffer(destination, source, count * 3);
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
    automap_polygons = new unsigned long[polygon_count];
    automap_filter_active = true;
    srPtr<srTextureIFace>* texture = getPolyTexture(0, 0, 0);
    if (texture) {
        for (unsigned int polygon = 0; polygon < static_cast<unsigned int>(polygon_count);
             ++polygon, ++texture) {
            if (*texture) {
                char name[256];
                strcpy(name, (*texture)->getName());
                bool include = true;
                for (int index = 0; index < excluded_textures->GetCount(); ++index) {
                    if (_stricmp(name, *excluded_textures->GetAt(index)) == 0)
                        include = false;
                }
                if (include)
                    automap_polygons[automap_polygon_count++] = polygon;
            } else {
                automap_polygons[automap_polygon_count++] = polygon;
            }
        }
    }
    if (automap_polygon_count >= static_cast<unsigned int>(polygon_count)) {
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

/* The active-polygon table selects which polygons a texture-table draw
   submits: per skin table the list is cached in the blanking vectors and
   built on first use by dropping polygons whose texture name starts with
   "blank"; the -1 table is the automap filter built by
   ApplyAutomapPolygonFilter. */
// FUNCTION: WIZ8 0x00473CD0
unsigned long* stMeshModel::GetActivePolygons(long* count_out, int table, bool flag)
{
    int index = -1;
    for (int i = 0; i < skin_table_ids.GetCount(); ++i) {
        if (*skin_table_ids.GetAt(i) == table) {
            index = i;
            break;
        }
    }

    unsigned long* list;
    unsigned char checked;
    if (index >= 0) {
        if (skin_blanking_apt == 0) {
            *count_out = 0;
            return 0;
        }
        list = *skin_blanking_apt->GetAt(index);
        *count_out = *skin_blanking_apt_number->GetAt(index);
        checked = *skin_blanking_checked->GetAt(index);
    } else {
        *count_out = automap_polygon_count;
        list = automap_polygons;
        checked = automap_filter_active;
    }
    if (list != 0) {
        return list;
    }
    if (checked != 0 || !flag) {
        *count_out = 0;
        return 0;
    }

    *count_out = 0;
    unsigned long* fresh = new unsigned long[polygon_count];
    srPtr<srTextureIFace>* textures = 0;
    if (index < 0) {
        automap_filter_active = true;
        textures = getPolyTexture(0, 0, 0);
    } else {
        skin_blanking_checked->SetAt(index, 1);
        int skin = -1;
        for (int i = 0; i < skin_table_ids.GetCount(); ++i) {
            if (*skin_table_ids.GetAt(i) == table) {
                skin = i;
                break;
            }
        }
        if (skin != -1) {
            textures = *skin_texture_tables.GetAt(skin);
        }
    }
    if (textures != 0) {
        for (unsigned int polygon = 0; polygon < static_cast<unsigned int>(polygon_count);
             ++polygon) {
            if (textures[polygon] == 0) {
                fresh[*count_out] = polygon;
                ++*count_out;
            } else {
                char name[260];
                strcpy(name, textures[polygon]->getName());
                if (_strnicmp(name, "blank", 5) != 0) {
                    fresh[*count_out] = polygon;
                    ++*count_out;
                }
            }
        }
        if (*count_out != polygon_count) {
            if (index < 0) {
                automap_polygons = fresh;
                automap_polygon_count = *count_out;
            } else {
                skin_blanking_apt->SetAt(index, fresh);
                skin_blanking_apt_number->SetAt(index, *count_out);
            }
            return fresh;
        }
    }
    delete[] fresh;
    *count_out = 0;
    return 0;
}

// FUNCTION: WIZ8 0x00471160
void stMeshModel::SetMappedVertex(short vertex, short key)
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
    if ((flags & W8_MESH_HAS_FRAME_STORAGE) == 0) {
        FreeFrameStorage();
        frame_count = frames;
        flags |= W8_MESH_HAS_FRAME_STORAGE;
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
    m_pVertexLoc = new srVector3T<float>*[frame_count];
    if (m_pVertexLoc == 0) {
        FreeFrameStorage();
        return 0;
    }
    memset(m_pVertexLoc, 0, frame_count * sizeof(srVector3T<float>*));
    m_pVertexNormal = new srVector3T<float>*[frame_count];
    if (m_pVertexNormal == 0) {
        FreeFrameStorage();
        return 0;
    }
    memset(m_pVertexNormal, 0, frame_count * sizeof(srVector3T<float>*));
    m_pPolyNormal = new srVector3T<float>*[frame_count];
    if (m_pPolyNormal == 0) {
        FreeFrameStorage();
        return 0;
    }
    memset(m_pPolyNormal, 0, frame_count * sizeof(srVector3T<float>*));
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
        compressed_vertex_locations[frame] = new short[vertex_location_count * 3];
        if (compressed_vertex_locations[frame] == 0) {
            srAssertFail("m_psCompVertexLoc[uiCount]",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp", 0x1ce, 0);
        }
        memset(compressed_vertex_locations[frame], 0, vertex_location_count * 3 * sizeof(short));
        compressed_vertex_normals[frame] = new unsigned char[vertex_location_count * 3];
        if (compressed_vertex_normals[frame] == 0) {
            srAssertFail("m_pbCompVertexNormal[uiCount]",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp", 0x1d3, 0);
        }
        memset(compressed_vertex_normals[frame], 0, vertex_location_count * 3);
        compressed_polygon_normals[frame] = new unsigned char[polygon_count * 3];
        if (compressed_polygon_normals[frame] == 0) {
            srAssertFail("m_pbCompPolyNormal[uiCount]",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp", 0x1d8, 0);
        }
        memset(compressed_polygon_normals[frame], 0, polygon_count * 3);
    }
    return 1;
}

// FUNCTION: WIZ8 0x004715e0
void stMeshModel::FreeFrameStorage()
{
    unsigned int frame;

    ReleaseDecompressedFrames();
    if (m_pVertexLoc != 0) {
        delete[] m_pVertexLoc;
        m_pVertexLoc = 0;
    }
    if (m_pVertexNormal != 0) {
        delete[] m_pVertexNormal;
        m_pVertexNormal = 0;
    }
    if (m_pPolyNormal != 0) {
        delete[] m_pPolyNormal;
        m_pPolyNormal = 0;
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

static unsigned int ReleaseDecompressedFrameTable(srVector3T<float>** frames,
                                                  unsigned int frame_count, int element_count)
{
    unsigned int released = 0;
    if (frames != 0) {
        for (unsigned int frame = 0; frame < frame_count; ++frame) {
            if (frames[frame] != 0) {
                released += element_count * sizeof(srVector3T<float>);
                srHeap.free(frames[frame]);
                frames[frame] = 0;
            }
        }
    }
    return released;
}

/* Drop every decompressed float cache, returning the bytes released. */
// FUNCTION: WIZ8 0x004739e0
int stMeshModel::ReleaseDecompressedFrames()
{
    int released = 0;
    released += ReleaseDecompressedFrameTable(m_pVertexLoc, frame_count, vertex_location_count);
    released += ReleaseDecompressedFrameTable(m_pVertexNormal, frame_count, vertex_location_count);
    released += ReleaseDecompressedFrameTable(m_pPolyNormal, frame_count, polygon_count);
    last_decompress_release_tick = GetTickCount();
    g_decompressed_mesh_bytes -= released;
    return released;
}

/* Evict least-recently-used decompressed frames until `needed` bytes are
   available, or give up when every registered model has been drained. */
// FUNCTION: WIZ8 0x00473BF0
unsigned char ReclaimDecompressedBytes(unsigned int needed)
{
    unsigned int released = 0;
    while (released < needed) {
        stMeshModel* oldest = 0;
        unsigned long oldest_tick = 0xffffffff;
        for (int index = 0; index < g_mesh_models.GetCount(); ++index) {
            stMeshModel* model = *g_mesh_models.GetAt(index);
            if (model == 0) {
                srAssertFail("pstModel", "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp",
                             0x712, 0);
            }
            if (model->last_decompress_release_tick < oldest_tick) {
                oldest = model;
                oldest_tick = model->last_decompress_release_tick;
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
int stMeshModel::FindSkinTable(const char* name)
{
    for (int index = 0; index < skin_table_names.GetCount(); ++index) {
        if (_stricmp(name, *skin_table_names.GetAt(index)) == 0) {
            return index;
        }
    }
    return -1;
}

// FUNCTION: WIZ8 0x00473720
srPtr<srTextureIFace>* stMeshModel::GetTextureTable(int table)
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
int stMeshModel::CreateSkinTable(const char* name, int base_table)
{
    int base_index = skin_table_ids.IndexOf(base_table);

    if (FindSkinTable(name) != -1) {
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

    srPtr<srTextureIFace>* textures = new srPtr<srTextureIFace>[polygon_count];
    for (int polygon = 0; polygon < polygon_count; ++polygon) {
        textures[polygon] = source[polygon];
    }
    skin_texture_tables.Add(textures);
    skin_table_ids.Add(table);

    char* copied_name = static_cast<char*>(malloc(strlen(name) + 1));
    strcpy(copied_name, name);
    skin_table_names.Add(copied_name);

    if (skin_blanking_apt == 0) {
        skin_blanking_apt = new W8GrowableVector<unsigned long*>;
        if (skin_blanking_apt == 0) {
            srAssertFail("m_plsSkinBlankingAPT",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp", 0x5cd, 0);
        }
    }
    skin_blanking_apt->Add(0);

    if (skin_blanking_apt_number == 0) {
        skin_blanking_apt_number = new W8GrowableVector<int>;
        if (skin_blanking_apt_number == 0) {
            srAssertFail("m_plsSkinBlankingAPTNum",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp", 0x5d6, 0);
        }
    }
    skin_blanking_apt_number->Add(0);

    if (skin_blanking_checked == 0) {
        skin_blanking_checked = new W8GrowableVector<unsigned char>;
        if (skin_blanking_checked == 0) {
            srAssertFail("m_plsSkinBlankingChecked",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp", 0x5df, 0);
        }
    }
    skin_blanking_checked->Add(0);
    return table;
}

// FUNCTION: WIZ8 0x00473830
void stMeshModel::RemoveSkinTable(int index)
{
    srPtr<srTextureIFace>* textures = *skin_texture_tables.GetAt(index);
    for (int polygon = 0; polygon < polygon_count; ++polygon) {
        textures[polygon] = static_cast<srTextureIFace*>(0);
    }
    delete[] textures;
    free(*skin_table_names.GetAt(index));

    skin_texture_tables.RemoveAt(index);
    skin_table_names.RemoveAt(index);
    skin_table_ids.RemoveAt(index);

    unsigned long* apt = *skin_blanking_apt->GetAt(index);
    if (apt != 0) {
        delete apt;
    }
    skin_blanking_apt->RemoveAt(index);
    skin_blanking_apt_number->RemoveAt(index);
    skin_blanking_checked->RemoveAt(index);
}

/* Skin tables are named with the owning cycle plus a one-character suffix.
   Final teardown removes every table whose name has that cycle prefix. */
// FUNCTION: WIZ8 0x00473780
void stMeshModel::RemoveSkinTablesForCycle(const char* cycle_name)
{
    if (cycle_name != 0) {
        for (int index = 0; index < skin_table_names.GetCount(); ++index) {
            char name[200];

            strncpy(name, *skin_table_names.GetAt(index), 199);
            name[199] = '\0';
            name[strlen(name) - 1] = '\0';
            if (strlen(name) != 0 && _stricmp(cycle_name, name) == 0) {
                RemoveSkinTable(index);
                --index;
            }
        }
    }
}

static void DecompressMeshNormals(const unsigned char* normals, int count,
                                  srVector3T<float>* destination)
{
    for (int index = 0; index < count; ++index) {
        const unsigned char* source = &normals[index * 3];
        destination[index].Set(s_compressed_normal_table[source[0]],
                               s_compressed_normal_table[source[1]],
                               s_compressed_normal_table[source[2]]);
    }
}

/* Expand one frame's compressed table into `destination`: bit 1 the vertex
   locations, bit 2 the vertex normals, bit 4 the polygon normals. */
// FUNCTION: WIZ8 0x00471930
unsigned char stMeshModel::DecompressFrame(int frame, unsigned char flags,
                                           srVector3T<float>* destination)
{
    if (flags & W8_MESH_FRAME_LOCATIONS) {
        for (int index = 0; index < vertex_location_count; ++index) {
            const short* source = &compressed_vertex_locations[frame][index * 3];
            destination[index].Set(source[0] * vertex_compression_scale,
                                   source[1] * vertex_compression_scale,
                                   source[2] * vertex_compression_scale);
        }
        return 1;
    }
    if (flags & W8_MESH_FRAME_VERTEX_NORMALS) {
        DecompressMeshNormals(compressed_vertex_normals[frame], vertex_location_count, destination);
        return 1;
    }
    if (flags & W8_MESH_FRAME_POLYGON_NORMALS) {
        DecompressMeshNormals(compressed_polygon_normals[frame], polygon_count, destination);
        return 1;
    }
    return 0;
}

/* Allocate one frame's decompressed caches for the tables named by `flags`
   (bit 0 locations, bit 1 vertex normals, bit 2 polygon normals), reclaiming
   least-recently-used frames when the byte budget would overflow. */
// FUNCTION: WIZ8 0x00471720
unsigned char stMeshModel::AllocateFrameBuffers(unsigned int uiFrame, unsigned char flags)
{
    if ((flags & W8_MESH_FRAME_LOCATIONS) != 0 && m_pVertexLoc[uiFrame] == 0) {
        int needed = vertex_location_count * sizeof(srVector3T<float>);
        if (g_decompressed_mesh_byte_limit <= g_decompressed_mesh_bytes + needed) {
            if (ReclaimDecompressedBytes(needed) == 0) {
                return 0;
            }
        }
        g_decompressed_mesh_bytes += needed;
        m_pVertexLoc[uiFrame] = static_cast<srVector3T<float>*>(
            srHeap.allocate(vertex_location_count * sizeof(srVector3T<float>)));
        if (m_pVertexLoc[uiFrame] == 0) {
            srAssertFail("m_pVertexLoc[uiFrame]",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp", 0x243, 0);
        }
    }
    if ((flags & W8_MESH_FRAME_VERTEX_NORMALS) != 0 && m_pVertexNormal[uiFrame] == 0) {
        int needed = vertex_location_count * sizeof(srVector3T<float>);
        if (g_decompressed_mesh_byte_limit <= g_decompressed_mesh_bytes + needed) {
            if (ReclaimDecompressedBytes(needed) == 0) {
                return 0;
            }
        }
        g_decompressed_mesh_bytes += needed;
        m_pVertexNormal[uiFrame] = static_cast<srVector3T<float>*>(
            srHeap.allocate(vertex_location_count * sizeof(srVector3T<float>)));
        if (m_pVertexNormal[uiFrame] == 0) {
            srAssertFail("m_pVertexNormal[uiFrame]",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp", 0x24e, 0);
        }
    }
    if ((flags & W8_MESH_FRAME_POLYGON_NORMALS) != 0 && m_pPolyNormal[uiFrame] == 0) {
        int needed = polygon_count * sizeof(srVector3T<float>);
        if (g_decompressed_mesh_byte_limit <= g_decompressed_mesh_bytes + needed) {
            if (ReclaimDecompressedBytes(needed) == 0) {
                return 0;
            }
        }
        g_decompressed_mesh_bytes += needed;
        m_pPolyNormal[uiFrame] = static_cast<srVector3T<float>*>(
            srHeap.allocate(polygon_count * sizeof(srVector3T<float>)));
        if (m_pPolyNormal[uiFrame] == 0) {
            srAssertFail("m_pPolyNormal[uiFrame]",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp", 0x259, 0);
        }
    }
    return 1;
}

/* Return frame `frame`'s vertex locations, decompressing on demand. When
   `interpolation` is positive and another frame follows, both frames are
   decompressed and lerped into lerp_buffer (m_pLerpBuffer). */
// FUNCTION: WIZ8 0x00471AD0
srVector3T<float>* stMeshModel::GetVertexLocations(unsigned int frame, bool load,
                                                   float interpolation)
{
    if (m_pVertexLoc == 0) {
        return 0;
    }
    if (g_float_zero < interpolation && frame < frame_count - 1) {
        unsigned int next_frame = (frame + 1) % frame_count;
        if (lerp_buffer == 0) {
            lerp_buffer = static_cast<srVector3T<float>*>(
                srHeap.allocate(vertex_location_count * sizeof(srVector3T<float>)));
            if (lerp_buffer == 0) {
                srAssertFail("m_pLerpBuffer",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp", 0x2d4, 0);
            }
        }
        if (m_pVertexLoc[frame] == 0) {
            AllocateFrameBuffers(frame, W8_MESH_FRAME_LOCATIONS);
            DecompressFrame(frame, W8_MESH_FRAME_LOCATIONS, m_pVertexLoc[frame]);
        }
        if (m_pVertexLoc[next_frame] == 0) {
            AllocateFrameBuffers(next_frame, W8_MESH_FRAME_LOCATIONS);
            DecompressFrame(next_frame, W8_MESH_FRAME_LOCATIONS, m_pVertexLoc[next_frame]);
        }
        if (lerp_buffer != 0) {
            srVector3T<float>* next = m_pVertexLoc[next_frame];
            srVector3T<float>* current = m_pVertexLoc[frame];
            if (next != 0 && current != 0 && vertex_location_count != 0) {
                if (interpolation == g_float_one) {
                    CopyDwordBuffer(lerp_buffer, next, vertex_location_count * 3);
                } else {
                    srVectorProcessor::lerp(&lerp_buffer->x, &next->x, &current->x, interpolation,
                                            vertex_location_count * 3);
                }
            }
        }
        return lerp_buffer;
    }
    if (m_pVertexLoc[frame] == 0) {
        AllocateFrameBuffers(frame, W8_MESH_FRAME_LOCATIONS);
        if (load && m_pVertexLoc[frame] != 0) {
            DecompressFrame(frame, W8_MESH_FRAME_LOCATIONS, m_pVertexLoc[frame]);
        }
    }
    return m_pVertexLoc[frame];
}

/* Return frame `frame`'s vertex normals, decompressing on demand when `load`
   is set. */
// FUNCTION: WIZ8 0x00471CA0
srVector3T<float>* stMeshModel::GetVertexNormals(unsigned int frame, bool load)
{
    if (m_pVertexNormal == 0) {
        return 0;
    }
    if (m_pVertexNormal[frame] == 0) {
        AllocateFrameBuffers(frame, W8_MESH_FRAME_VERTEX_NORMALS);
        if (load && m_pVertexNormal[frame] != 0) {
            DecompressFrame(frame, W8_MESH_FRAME_VERTEX_NORMALS, m_pVertexNormal[frame]);
        }
    }
    return m_pVertexNormal[frame];
}

// FUNCTION: WIZ8 0x00471D00
srVector3T<float>* stMeshModel::GetPolygonNormals(unsigned int frame, bool load)
{
    if (m_pPolyNormal == 0) {
        return 0;
    }
    if (m_pPolyNormal[frame] == 0) {
        AllocateFrameBuffers(frame, W8_MESH_FRAME_POLYGON_NORMALS);
        if (load && m_pPolyNormal[frame] != 0) {
            DecompressFrame(frame, W8_MESH_FRAME_POLYGON_NORMALS, m_pPolyNormal[frame]);
        }
    }
    return m_pPolyNormal[frame];
}

void stMeshModel::ClearVertexLights()
{
    srVector3T<float>* lights = GetVertexLights(true, 1);
    int count = vertex_location_count * 3;
    if (count != 0) {
        FillDwordBuffer(lights, 0, count);
    }
    flags |= W8_MESH_VERTEX_LIGHTING_DIRTY;
}

// FUNCTION: WIZ8 0x00472990
void stMeshModel::SetAmbientColor(const srVector3T<float>& color)
{
    if (!(ambient_color == color)) {
        ambient_color = color;
        flags |= W8_MESH_VERTEX_LIGHTING_DIRTY;
    }
}

/* Build one frame's compressed polygon and vertex normals from its vertex
   locations. Vertex normals are summed per polygon corner, remapped through
   the shade index table when there is one, unitized, and stored as signed
   bytes scaled by 127. */
// FUNCTION: WIZ8 0x004729F0
void stMeshModel::ComputeFrameNormals(int frame)
{
    if (polygon_count == 0 || vertex_location_count == 0) {
        return;
    }

    srVector3i* poly_vertex = getPolyVertex();
    srVector3T<float>* pnorm = new srVector3T<float>[polygon_count];
    if (pnorm == 0 || poly_vertex == 0) {
        return;
    }

    srVector3T<float>* l = 0;
    bool decompressed = false;
    if (m_pVertexLoc != 0) {
        l = m_pVertexLoc[frame];
    }
    if (l == 0) {
        l = new srVector3T<float>[vertex_location_count];
        if (l == 0) {
            srAssertFail("l", "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp", 0x4ca, 0);
        }
        DecompressFrame(frame, W8_MESH_FRAME_LOCATIONS, l);
        decompressed = true;
    }

    for (int poly = 0; poly < polygon_count; ++poly) {
        const srVector3T<float>& origin = l[poly_vertex[poly].x];
        srVector3T<float> edge_0 = l[poly_vertex[poly].y] - origin;
        srVector3T<float> edge_1 = l[poly_vertex[poly].z] - origin;
        pnorm[poly] = CrossProduct(edge_0, edge_1);
    }

    srVector3T<float>* vnorm = new srVector3T<float>[vertex_location_count];
    unsigned long* shade_index = getVertexShadeIndex(0);
    if (vnorm == 0) {
        srAssertFail("vnorm", "C:\\Projects\\Wizardry 8\\Engine Code\\stMeshModel.cpp", 0x4db, 0);
    }
    if (shade_index == 0) {
        FillDwordBuffer(vnorm, 0, vertex_location_count * 3);
        for (int corner_poly = 0; corner_poly < polygon_count; ++corner_poly) {
            vnorm[poly_vertex[corner_poly].x] += pnorm[corner_poly];
            vnorm[poly_vertex[corner_poly].y] += pnorm[corner_poly];
            vnorm[poly_vertex[corner_poly].z] += pnorm[corner_poly];
        }
    } else {
        srVector3T<float>* shaded = new srVector3T<float>[vertex_location_count];
        FillDwordBuffer(shaded, 0, vertex_location_count * 3);
        for (int corner_poly = 0; corner_poly < polygon_count; ++corner_poly) {
            shaded[shade_index[poly_vertex[corner_poly].x]] += pnorm[corner_poly];
            shaded[shade_index[poly_vertex[corner_poly].y]] += pnorm[corner_poly];
            shaded[shade_index[poly_vertex[corner_poly].z]] += pnorm[corner_poly];
        }
        srVectorProcessor::copyIndexed(vnorm, shaded, shade_index, vertex_location_count);
        delete[] shaded;
    }

    srVectorProcessor::normalize(vnorm, vnorm, 1.0f, vertex_location_count);
    for (int vertex = 0; vertex < vertex_location_count; ++vertex) {
        if (IsZeroVector(vnorm + vertex) != 0) {
            vnorm[vertex] = 1e-6f;
        }
    }
    srVectorProcessor::mul(&vnorm->x, 127.0f, &vnorm->x, vertex_location_count * 3);
    for (int index = 0; index < vertex_location_count; ++index) {
        compressed_vertex_normals[frame][index * 3] = static_cast<char>(vnorm[index].x);
        compressed_vertex_normals[frame][index * 3 + 1] = static_cast<char>(vnorm[index].y);
        compressed_vertex_normals[frame][index * 3 + 2] = static_cast<char>(vnorm[index].z);
    }

    srVectorProcessor::normalize(pnorm, pnorm, 1.0f, polygon_count);
    srVectorProcessor::mul(&pnorm->x, 127.0f, &pnorm->x, polygon_count * 3);
    for (int polygon = 0; polygon < polygon_count; ++polygon) {
        compressed_polygon_normals[frame][polygon * 3] = static_cast<char>(pnorm[polygon].x);
        compressed_polygon_normals[frame][polygon * 3 + 1] = static_cast<char>(pnorm[polygon].y);
        compressed_polygon_normals[frame][polygon * 3 + 2] = static_cast<char>(pnorm[polygon].z);
    }

    delete[] pnorm;
    delete[] vnorm;
    if (decompressed) {
        delete[] l;
    }
}

/* Thirteen-byte forwarder onto the per-frame normal builder. */
// FUNCTION: WIZ8 0x00472100
srVector3T<float>* stMeshModel::GetVertexLights(bool initialize, int table)
{
    if (table == -1) {
        table = vertex_light_table;
    }
    srHeapBuffer<srVector3T<float> >& lights = vertex_lights[table];
    if (lights.data == 0 && initialize) {
        lights.setCapacity(vertex_location_count, 0);
        srVector3T<float> zero(0.0f, 0.0f, 0.0f);
        for (unsigned int index = 0; index < lights.capacity; ++index) {
            lights.data[index] = zero;
        }
        if (vertex_sunlight.data != 0) {
            vertex_lighting_ready = true;
        }
    }
    return lights.data;
}

// FUNCTION: WIZ8 0x004721E0
float* stMeshModel::GetVertexSunlight(bool initialize)
{
    if (vertex_sunlight.data == 0 && initialize) {
        vertex_sunlight.setCapacity(vertex_location_count, 0);
        for (unsigned int index = 0; index < vertex_sunlight.capacity; ++index) {
            vertex_sunlight.data[index] = 1.0f;
        }
        vertex_lighting_ready = true;
    }
    return vertex_sunlight.data;
}

// FUNCTION: WIZ8 0x00473180
void stMeshModel::FinalizeVertexFrame(int frame)
{
    ComputeFrameNormals(frame);
}

/* Ordinary primary-template instantiation emissions. The generic methods live
   in srArray.h; there are no per-element authored bodies here. */

/* Further primary-template emissions in this TU: the preserving setCapacity
   overloads, the unconditional release for the twelve-byte-element vector
   array, member vector dtors/deleting destructors, and the copy machinery the
   srClassSupport clone reaches. */

/* Stores table 0x005EC518, the W8GrowableVector<W8VectorElement005EC514*>
   specialization's one-slot table - not 0x005EC514 of W8GrowableVector<stMeshModel*>. */

/* Mirror the active shader onto both the pipeline and the current Pass record
   selected at +0x18. */
// FUNCTION: WIZ8 0x004752C0
void srTriMeshPipeline::SetFlags(srShader shader)
{
    this->shader = shader;
    current_pass->shader = shader;
}

/* Point current_record / current_pass at slot slot_count, growing
   either table by (capacity + slot + 8) when needed. */
// FUNCTION: WIZ8 0x00475540
void srTriMeshPipeline::PrepareSlot()
{
    current_record = &records[slot_count];
    current_pass = &passes[slot_count];

    current_record->flags = 0;
    current_record->disable_mask = 0;
    current_record->material = material;
    current_pass->textures[0] = texture0;
    current_pass->textures[1] = texture1;
    current_pass->shader.value = shader.value;
    current_pass->texture_tables[0] = 0;
    current_pass->texture_tables[1] = 0;
    current_pass->shaders = 0;
    current_pass->texcoords = 0;
    current_pass->poly_uv = 0;
}

// FUNCTION: WIZ8 0x00475510
void srTriMeshPipeline::Flush()
{
    flushing = 1;
    if (slot_count > 0) {
        FlushSlots();
    }
    flushing = 0;
}

/* Bind a renderer and rebuild the current slot through PrepareSlot. */
// FUNCTION: WIZ8 0x004753F0
void srTriMeshPipeline::Reset(srGERD* renderer)
{
    slot_count = 0;
    this->renderer = renderer;
    flags = 0;
    flags |= FRUSTUM_CLIPPING;
    flags |= LIMIT_VERTEX_BATCHES;
    triangle_count = 0;
    active_triangles = 0;
    projected_vertices = 0;
    triangles = 0;
    vertex_count = 0;
    positions = 0;
    vertex_extras = 0;
    bounds_source = srTriMeshPipeline::BOUNDS_FROM_VERTICES;
    sort_bias = 0.0f;
    shader.value = 0x0100241b;
    texture0 = 0;
    texture1 = 0;
    material = srCore.getMaterial();

    PrepareSlot();
}

srTriMeshPipeline::srTriMeshPipeline()
{
    flags = 0;
    shader.value = 0;
    vertex_pipe = new srVertexPipe();
    flushing = 0;
    Reset(0);
    Flush();
}

// FUNCTION: WIZ8 0x004752F0
srTriMeshPipeline::~srTriMeshPipeline()
{
    while (flushing != 0) {
    }

    delete vertex_pipe;
}

// FUNCTION: WIZ8 0x00475600
void srTriMeshPipeline::FlushSlots()
{
    if (triangle_count == 0 || (active_triangles != 0 && active_triangle_count == 0)) {
        return;
    }

    if (bounds_source != srTriMeshPipeline::BOUNDS_SPHERE) {
        if (bounds_source == srTriMeshPipeline::BOUNDS_FROM_VERTICES && vertex_count != 0) {
            srVectorProcessor::minMax(positions, bounds_minimum, bounds_maximum, vertex_count);
        }

        srVector3T<float> center;
        center.Set((bounds_minimum.x + bounds_maximum.x) * 0.5,
                   (bounds_minimum.y + bounds_maximum.y) * 0.5,
                   (bounds_minimum.z + bounds_maximum.z) * 0.5);
        bounds_center = center;

        bounds_radius = (bounds_center - bounds_minimum).Length() * 1.001f;
    }

    srVector3T<float> eye_center;
    float eye_radius;
    srMatrix4T<float> model_view;
    srMatrix4T<float> inverse_model_view;
    srGERD::ClipPlanes clip_planes;
    srMatrix4T<float> project_clip_near;
    srMatrix4T<float> normal_matrix;

    this->renderer->getEyeSpaceBounds(eye_center, eye_radius, bounds_center, bounds_radius);
    this->renderer->getMatrix(srGERD::MATRIX_MODELVIEW, model_view);
    this->renderer->getInverseModelViewMatrix(inverse_model_view);
    this->renderer->getClipPlanes(clip_planes);
    this->renderer->getProjectClipNearMatrix(project_clip_near);
    this->renderer->getNormalMatrix(normal_matrix);

    srMatrix4T<float>::e_scaleType scale_type = this->renderer->getModelViewScaleType();
    srGERD::e_cullMode cull_mode = this->renderer->getCullMode();
    srGERD::e_winding winding = this->renderer->getWinding();

    srTriangleCuller::Input culler_input;
    if (cull_mode == srGERD::CULL_NONE) {
        culler_input.cull_mode = 2;
    } else if (cull_mode == srGERD::CULL_FRONT) {
        culler_input.cull_mode = winding == srGERD::WINDING_POSITIONAL_0;
    } else {
        culler_input.cull_mode = winding != srGERD::WINDING_POSITIONAL_0;
    }

    culler_input.vertex_count = vertex_count;
    culler_input.vertices = positions;
    culler_input.clip_planes = clip_planes.planes;
    culler_input.model_view = &model_view;
    culler_input.inverse_model_view = &inverse_model_view;
    culler_input.scale_type = scale_type;

    if ((this->flags & FRUSTUM_CLIPPING) == 0) {
        culler_input.clip_mask = 0;
    } else {
        float depth;
        unsigned long clip_mask = srTriangleCuller::getClipMask(
            eye_center, eye_radius, clip_planes.planes, clip_planes.mask, depth);
        int retain_clip_mask =
            clip_mask != 0 && ((clip_mask & 0xffffffc0UL) != 0 ||
                               (slot_count * triangle_count > 45 && depth > 0.23f));
        culler_input.clip_mask = retain_clip_mask ? clip_mask : 0;
    }

    if (active_triangles == 0) {
        srCore.getStatisticsManager()->statistics.triangles_submitted +=
            slot_count * triangle_count;
    } else {
        srCore.getStatisticsManager()->statistics.triangles_submitted +=
            slot_count * active_triangle_count;
    }
    ++srCore.getStatisticsManager()->statistics.meshes_submitted;
    srCore.getStatisticsManager()->statistics.vertices_submitted += slot_count * vertex_count;

    unsigned long total = active_triangles == 0 ? triangle_count : active_triangle_count;
    unsigned long batch_limit = total;
    if ((this->flags & LIMIT_VERTEX_BATCHES) != 0) {
        double ratio = static_cast<double>(vertex_count) / triangle_count;
        if (ratio > 3.0f) {
            ratio = 3.0f;
        }
        batch_limit = static_cast<unsigned long>(1300.0f / (slot_count * ratio));
        if (cull_mode == srGERD::CULL_NONE) {
            batch_limit >>= 1;
        }
        if (batch_limit > total) {
            batch_limit = total;
        }
    }

    unsigned long* scratch = culler_scratch.ensure(batch_limit + vertex_count * 2);
    srTriangleCuller::Output culler_output;
    culler_output.indices = scratch;
    culler_output.avt = scratch + batch_limit;
    culler_output.vertex_remap = culler_output.avt + vertex_count;

    unsigned long processed = 0;
    while (processed < total) {
        unsigned long batch_count = total - processed;
        if (batch_count > batch_limit) {
            batch_count = batch_limit;
        }

        if (active_triangles == 0) {
            culler_input.triangle_count = batch_count;
            culler_input.active_triangle_count = 0;
            culler_input.active_triangles = 0;
            culler_input.projected_vertices = projected_vertices + processed;
            culler_input.triangles = triangles + processed;
        } else {
            culler_input.triangle_count = triangle_count;
            culler_input.active_triangle_count = batch_count;
            culler_input.active_triangles = active_triangles + processed;
            culler_input.projected_vertices = projected_vertices;
            culler_input.triangles = triangles;
        }

        if (srTriangleCuller::cull(culler_output, culler_input)) {
            srCore.getStatisticsManager()->statistics.triangles_after_culling +=
                slot_count * culler_output.triangle_count;
            srCore.getStatisticsManager()->statistics.vertices_after_culling +=
                slot_count * culler_output.vertex_count;

            srGERD::Renderer* renderer = this->renderer->lockRenderer();

            (void)this->vertex_arrays[slot_count];
            srVertexArray* vertex_arrays = &this->vertex_arrays[0];
            renderer->allocVertexArray(vertex_arrays[0], slot_count * culler_output.vertex_count);

            for (unsigned long slot = 1; slot < slot_count; ++slot) {
                unsigned long offset = slot * culler_output.vertex_count;
                vertex_arrays[slot].eye_locations = vertex_arrays[0].eye_locations + offset;
                vertex_arrays[slot].diffuse = vertex_arrays[0].diffuse + offset;
                vertex_arrays[slot].specular = vertex_arrays[0].specular + offset;
                vertex_arrays[slot].st0 = vertex_arrays[0].st0 + offset;
                vertex_arrays[slot].st1 = vertex_arrays[0].st1 + offset;
                vertex_arrays[slot].q0 = vertex_arrays[0].q0 + offset;
                vertex_arrays[slot].q1 = vertex_arrays[0].q1 + offset;
                vertex_arrays[slot].attributes = vertex_arrays[0].attributes + offset;
            }

            unsigned long processor_count = this->renderer->getVertexProcessorCount();
            srVertexProcessor** processors = 0;
            if (processor_count != 0) {
                processors = vertex_processors.ensure(processor_count);
                this->renderer->getVertexProcessors(processors);
            }

            srVector4T<float> ambient_light;
            float environment_minimum;
            float environment_maximum;
            float environment_scale;
            float environment_inverse_scale;
            this->renderer->getAmbientLight(ambient_light);
            this->renderer->getEnvironmentRange(environment_minimum, environment_maximum);
            this->renderer->getEnvironmentScaleFactor(environment_scale, environment_inverse_scale);
            unsigned long exclusion_mask = this->renderer->getExclusionMask();

            srVertexPipe::Input pipe_input;
            pipe_input.record_count = slot_count;
            pipe_input.vertex_count = culler_output.vertex_count;
            pipe_input.active_vertices = culler_output.avt;
            pipe_input.direct_vertex_indices = culler_output.linear == 0;
            pipe_input.positions = positions;
            pipe_input.normals = vertex_extras;
            pipe_input.eye_center = eye_center;
            pipe_input.eye_radius = eye_radius;
            pipe_input.model_view = &model_view;
            pipe_input.normal_matrix = &normal_matrix;
            pipe_input.vertex_arrays = &this->vertex_arrays[0];
            pipe_input.exclusion_mask = exclusion_mask;
            pipe_input.ambient_light = ambient_light;
            pipe_input.records = &records[0];
            pipe_input.processors = processors;
            pipe_input.processor_count = processor_count;
            pipe_input.environment_minimum = environment_minimum;
            pipe_input.environment_maximum = environment_maximum;
            pipe_input.environment_scale = environment_scale;
            pipe_input.environment_inverse_scale = environment_inverse_scale;

            if (active_triangles == 0 && processed != 0) {
                for (unsigned long index = 0; index < batch_count; ++index) {
                    culler_output.indices[index] += processed;
                }
            }

            unsigned long renderer_disable_mask = 0;
            if (this->renderer->getMaxTextureStages() == 1) {
                renderer_disable_mask = (1UL << srVertexProcessor::CHANNEL_ST1) |
                                        (1UL << srVertexProcessor::CHANNEL_Q1);
            }

            for (unsigned long pass_index = 0; pass_index < slot_count; ++pass_index) {
                passes[pass_index].texcoords = records[pass_index].st0;

                unsigned long disable_mask;
                if (passes[pass_index].shaders != 0) {
                    srFlags<srVertexProcessor::e_channel> flags =
                        srVertexPipe::getShaderDisableMask(passes[pass_index].shaders,
                                                           culler_output.indices,
                                                           culler_output.triangle_count);
                    disable_mask = flags.value;
                } else {
                    srFlags<srVertexProcessor::e_channel> flags =
                        srVertexPipe::getShaderDisableMask(passes[pass_index].shader);
                    disable_mask = flags.value;
                }
                records[pass_index].disable_mask = disable_mask | renderer_disable_mask;
            }

            vertex_pipe->process(pipe_input);

            srGERD::Renderer::TriInput render_input;
            render_input.triangle_count = culler_output.triangle_count;
            render_input.record_count = slot_count;
            render_input.vertex_count = culler_output.vertex_count;
            render_input.indices = culler_output.indices;
            render_input.triangles = triangles;
            render_input.vertices = culler_output.vertex_remap;
            render_input.passes = &passes[0];
            render_input.direct_vertex_indices = culler_output.linear == 0;
            render_input.project_clip_near = &project_clip_near;
            render_input.sort_bias = sort_bias;
            renderer->render(render_input);
            this->renderer->unlockRenderer(renderer, 0);
        }

        processed += batch_limit;
    }
}

/* Lazy singleton: construct once against the imported pipe static, then bind
   the caller's renderer and rebuild the current slot. */
// FUNCTION: WIZ8 0x004750A0
srTriMeshPipeline* srTriMeshPipeline::Get(srGERD* renderer)
{
    srTriMeshPipeline* pipeline;

    if (renderer == 0) {
        return 0;
    }

    if (pipe == 0) {
        pipe = new srTriMeshPipeline();
    }

    pipeline = pipe;
    pipeline->Reset(renderer);
    return pipe;
}
/* Retail ICF folds this empty thiscall onto W8OptionsGraphicsPanel::OnDragEnd
   at 0x005AA400 (OptionsScreen.cpp). This source function has no separately
   retained retail address, so it intentionally has no FUNCTION marker. */
void stMeshModel::NotifyLinkedModel(stMeshModel*) {}
