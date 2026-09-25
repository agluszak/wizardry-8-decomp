/* D:\srsdk1x\sources\corelib\srMeshModel.cpp */

#include "surrender/srMeshModel.h"

#include "surrender/srCore.h"
#include "surrender/srDebug.h"
#include "surrender/srGERD.h"
#include "surrender/srMaterial.h"
#include "surrender/srTriMeshPipeline.h"
#include "surrender/srTriangleCuller.h"
#include "surrender/srVectorProcessor.h"
#include "surrender/srVertexPipe.h"

#include <float.h>
#include <math.h>
#include <ostream>
#include <stdio.h>
#include <string.h>
#pragma intrinsic(memset)

/* Comma-separated flag-name tables dump walks while printing the
   control_state_394 and control_state_390 bits. Retail .bss holds
   zero-initialized pointers here; no in-range provider code ever stores to
   them, so dump prints numeric bit indices. */
// GLOBAL: SURRENDER 0x100A4998
static const char* s_control_names_100a4998;

// GLOBAL: SURRENDER 0x100A499C
static const char* s_flag_names_100a499c;

/* Retail's guarded dword fill — identical emission to renderer.cpp's
   file-local fillConstant; the linker folds the two copies. */
static void fillConstant(unsigned long* destination, unsigned long value, unsigned long count)
{
    if (count != 0) {
        srVectorProcessor::copy(destination, value, count);
    }
}

/* POD table permutation: scratch through srHeap, straight copy, then
   reordered copy back. Retail emits one instantiation per element type. */
template <class T> static void permuteTable(T* table, const unsigned long* indices, long count)
{
    T* scratch = static_cast<T*>(srHeap.allocate(count * sizeof(T)));
    if (scratch == 0) {
        scratch = 0;
    }
    if (count != 0) {
        long index;
        for (index = 0; index < count; ++index) {
            scratch[index] = table[index];
        }
        for (index = 0; index < count; ++index) {
            table[index] = scratch[indices[index]];
        }
    }
    srHeap.free(scratch);
}

/* Object-table permutation: `new T[count]` scratch so each element's ctor and
   copy-assign run (srPtr reference counts, srShader words). Retail emits one
   instantiation per element type. */
template <class T> static void permuteObjects(T* table, const unsigned long* indices, long count)
{
    T* scratch = new T[count];
    if (count != 0) {
        long index;
        for (index = 0; index < count; ++index) {
            scratch[index] = table[index];
        }
        for (index = 0; index < count; ++index) {
            table[index] = scratch[indices[index]];
        }
    }
    delete[] scratch;
}

/* Bounds-checked pass/side slots. Retail rejects out-of-range indices and
   leaves the slot untouched; setMaterial/setTexture keep the reference
   counts balanced through addReference/release. */
// FUNCTION: SURRENDER 0x100403E0
void srMeshModel::setShader(srShader shader, long pass)
{
    if (pass >= 0 && pass < 4) {
        shaders_5c[pass] = shader;
    }
}

// FUNCTION: SURRENDER 0x10040400
srShader srMeshModel::getShader(long pass) const
{
    if (pass >= 0 && pass < 4) {
        return shaders_5c[pass];
    }
    srShader shader;
    shader.value = 0x0100241b;
    return shader;
}

// FUNCTION: SURRENDER 0x10040330
srMaterialIFace* srMeshModel::getMaterial(long pass, e_side side) const
{
    if (pass >= 0 && pass < 4 && (int)side >= 0 && (int)side < 2) {
        return materials_1c[pass][side];
    }
    return 0;
}

// FUNCTION: SURRENDER 0x100402E0
void srMeshModel::setMaterial(srMaterialIFace* material, long pass, e_side side)
{
    if (pass >= 0 && pass < 4 && (int)side >= 0 && (int)side < 2) {
        materials_1c[pass][side] = material;
    }
}

// FUNCTION: SURRENDER 0x100403B0
srTextureIFace* srMeshModel::getTexture(long pass, long layer) const
{
    if (pass >= 0 && pass < 4 && layer >= 0 && layer < 2) {
        return textures_3c[pass][layer];
    }
    return 0;
}

// FUNCTION: SURRENDER 0x10040360
void srMeshModel::setTexture(srTextureIFace* texture, long pass, long layer)
{
    if (pass >= 0 && pass < 4 && layer >= 0 && layer < 2) {
        textures_3c[pass][layer] = texture;
    }
}

// FUNCTION: SURRENDER 0x10042800
long srMeshModel::getActivePolygonCount()
{
    return active_polygon_count_1fc;
}

// FUNCTION: SURRENDER 0x10042810
void srMeshModel::setActivePolygonCount(long count)
{
    active_polygon_count_1fc = count;
}

// FUNCTION: SURRENDER 0x10042820
void srMeshModel::setUVCount(long count)
{
    if (count < vertex_location_count_22c) {
        count = vertex_location_count_22c;
    }
    if (count != uv_count_234) {
        uv_count_234 = count;
        MeshTable<srVector2T<float> >* table = &texcoords_13c[0][0];
        for (long pass = 4; pass != 0; --pass) {
            for (long side = 2; side != 0; --side, ++table) {
                if (table->data != 0) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
                    /* Retail compares the count operands with unsigned
                       branches (JBE/JNC); the unsigned locals reproduce that
                       lowering against the signed fields. */
                    unsigned long old_count = table->count;
                    unsigned long uv = uv_count_234;
                    srVector2T<float> empty(0.0f, 0.0f);
                    if (old_count != uv) {
                        if (uv == 0) {
                            table->Release();
                        } else {
                            srVector2T<float>* replacement = table->Allocate(uv);
                            if (table->data != 0 && table->count != 0) {
                                unsigned long copy = table->count;
                                if (uv < copy) {
                                    copy = uv;
                                }
                                for (unsigned long index = 0; index < copy; ++index) {
                                    replacement[index] = table->data[index];
                                }
                            }
                            if (table->data != 0) {
                                srHeap.free(table->data);
                            }
                            table->data = replacement;
                            table->count = uv;
                        }
                    }
                    if (uv != 0) {
                        for (unsigned long index = old_count; index < table->count; ++index) {
                            table->data[index] = empty;
                        }
                    }
#pragma clang diagnostic pop
                }
            }
        }
        setDirty(static_cast<e_flags>(3));
    }
}

// FUNCTION: SURRENDER 0x10042980
long srMeshModel::getUVCount() const
{
    return uv_count_234;
}

/* The lazily grown table accessors below share one retail shape: a table is
   supplied only when its governing count is nonzero; on first use the pair
   reallocs through srHeap, copies the shorter extent when the element type
   preserves old data, and then clears or seeds the whole table. */
// FUNCTION: SURRENDER 0x1003FFD0
srVector3T<float>* srMeshModel::getVertexLoc()
{
    if (vertex_location_count_22c == 0) {
        return 0;
    }
    if (vertex_locations_1dc.data == 0) {
        if (vertex_locations_1dc.count != vertex_location_count_22c) {
            if (vertex_location_count_22c == 0) {
                vertex_locations_1dc.Release();
            } else {
                srVector3T<float>* replacement =
                    vertex_locations_1dc.Allocate(vertex_location_count_22c);
                if (vertex_locations_1dc.data != 0 && vertex_locations_1dc.count != 0) {
                    long copy = vertex_locations_1dc.count;
                    if (vertex_location_count_22c < copy) {
                        copy = vertex_location_count_22c;
                    }
                    for (long index = 0; index < copy; ++index) {
                        replacement[index] = vertex_locations_1dc.data[index];
                    }
                }
                if (vertex_locations_1dc.data != 0) {
                    srHeap.free(vertex_locations_1dc.data);
                }
                vertex_locations_1dc.data = replacement;
                vertex_locations_1dc.count = vertex_location_count_22c;
            }
        }
        for (long index = 0; index < vertex_locations_1dc.count; ++index) {
            vertex_locations_1dc.data[index].x = 0.0f;
            vertex_locations_1dc.data[index].y = 0.0f;
            vertex_locations_1dc.data[index].z = 0.0f;
        }
    }
    return vertex_locations_1dc.data;
}

// FUNCTION: SURRENDER 0x100400D0
srVector3T<float>* srMeshModel::getVertexNormal()
{
    if (vertex_location_count_22c == 0) {
        return 0;
    }
    if (vertex_normals_1e4.data == 0 && vertex_normals_1e4.count != vertex_location_count_22c) {
        if (vertex_location_count_22c == 0) {
            vertex_normals_1e4.Release();
        } else {
            srVector3T<float>* replacement = vertex_normals_1e4.Allocate(vertex_location_count_22c);
            if (vertex_normals_1e4.data != 0) {
                srHeap.free(vertex_normals_1e4.data);
            }
            vertex_normals_1e4.data = replacement;
            vertex_normals_1e4.count = vertex_location_count_22c;
        }
    }
    if ((control_state_390 & 4) != 0) {
        calculateVertexNormals();
    }
    return vertex_normals_1e4.data;
}

// FUNCTION: SURRENDER 0x10040160
srVector4T<float>* srMeshModel::getPolyEq()
{
    if (polygon_count_230 == 0) {
        return 0;
    }
    if (poly_equations_134.data == 0 && poly_equations_134.count != polygon_count_230) {
        if (polygon_count_230 == 0) {
            poly_equations_134.Release();
        } else {
            srVector4T<float>* replacement = poly_equations_134.Allocate(polygon_count_230);
            if (poly_equations_134.data != 0 && poly_equations_134.count != 0) {
                long copy = poly_equations_134.count;
                if (polygon_count_230 < copy) {
                    copy = polygon_count_230;
                }
                for (long index = 0; index < copy; ++index) {
                    replacement[index] = poly_equations_134.data[index];
                }
            }
            if (poly_equations_134.data != 0) {
                srHeap.free(poly_equations_134.data);
            }
            poly_equations_134.data = replacement;
            poly_equations_134.count = polygon_count_230;
        }
    }
    if ((control_state_390 & 2) != 0) {
        calculatePolygonNormals();
    }
    return poly_equations_134.data;
}

// FUNCTION: SURRENDER 0x10040250
unsigned long* srMeshModel::getActivePolygonTable(int table)
{
    if (active_polygons_1f4.data == 0) {
        if (table != 0) {
            if (active_polygons_1f4.count != polygon_count_230) {
                if (polygon_count_230 == 0) {
                    active_polygons_1f4.Release();
                } else {
                    unsigned long* replacement = active_polygons_1f4.Allocate(polygon_count_230);
                    if (active_polygons_1f4.data != 0) {
                        srHeap.free(active_polygons_1f4.data);
                    }
                    active_polygons_1f4.data = replacement;
                    active_polygons_1f4.count = polygon_count_230;
                }
            }
            for (long index = 0; index < polygon_count_230; ++index) {
                active_polygons_1f4.data[index] = index;
            }
        }
    }
    return active_polygons_1f4.data;
}

// FUNCTION: SURRENDER 0x1003FED0
srVector3i* srMeshModel::getPolyVertex()
{
    if (polygon_count_230 == 0) {
        return 0;
    }
    if (poly_vertices_10c.data == 0) {
        if (poly_vertices_10c.count != polygon_count_230) {
            if (polygon_count_230 == 0) {
                poly_vertices_10c.Release();
            } else {
                srVector3i* replacement = poly_vertices_10c.Allocate(polygon_count_230);
                if (poly_vertices_10c.data != 0 && poly_vertices_10c.count != 0) {
                    long copy = poly_vertices_10c.count;
                    if (polygon_count_230 < copy) {
                        copy = polygon_count_230;
                    }
                    for (long index = 0; index < copy; ++index) {
                        replacement[index] = poly_vertices_10c.data[index];
                    }
                }
                if (poly_vertices_10c.data != 0) {
                    srHeap.free(poly_vertices_10c.data);
                }
                poly_vertices_10c.data = replacement;
                poly_vertices_10c.count = polygon_count_230;
            }
        }
        for (long index = 0; index < poly_vertices_10c.count; ++index) {
            poly_vertices_10c.data[index].x = 0;
            poly_vertices_10c.data[index].y = 0;
            poly_vertices_10c.data[index].z = 0;
        }
    }
    return poly_vertices_10c.data;
}

// FUNCTION: SURRENDER 0x10040430
srPtr<srMaterialIFace>* srMeshModel::getVertexMaterial(long vertex, e_side side, int table)
{
    if (vertex < 0 || vertex > 3 || (int)side < 0 || (int)side > 1) {
        return 0;
    }
    MeshTable<srPtr<srMaterialIFace> >& slot = vertex_materials_cc[vertex][side];
    if (slot.data == 0) {
        if (table != 0) {
            if (slot.count != vertex_location_count_22c) {
                if (vertex_location_count_22c == 0) {
                    slot.Release();
                } else {
                    srPtr<srMaterialIFace>* replacement = slot.Allocate(vertex_location_count_22c);
                    slot.Release();
                    slot.data = replacement;
                    slot.count = vertex_location_count_22c;
                }
            }
            for (long index = 0; index < slot.count; ++index) {
                slot.data[index] = srPtr<srMaterialIFace>();
            }
        }
    }
    return slot.data;
}

// FUNCTION: SURRENDER 0x10040560
srPtr<srTextureIFace>* srMeshModel::getPolyTexture(long polygon, long layer, int table)
{
    if (polygon < 0 || polygon > 3 || layer < 0 || layer > 1) {
        return 0;
    }
    MeshTable<srPtr<srTextureIFace> >& slot = poly_textures_6c[polygon][layer];
    if (slot.data == 0) {
        if (table != 0) {
            if (slot.count != polygon_count_230) {
                if (polygon_count_230 == 0) {
                    slot.Release();
                } else {
                    srPtr<srTextureIFace>* replacement = slot.Allocate(polygon_count_230);
                    slot.Release();
                    slot.data = replacement;
                    slot.count = polygon_count_230;
                }
            }
            for (long index = 0; index < slot.count; ++index) {
                slot.data[index] = srPtr<srTextureIFace>();
            }
        }
    }
    return slot.data;
}

// FUNCTION: SURRENDER 0x10040690
srShader* srMeshModel::getPolyShader(long polygon, int layer)
{
    if (polygon < 0 || polygon > 3) {
        return 0;
    }
    MeshTable<srShader>& slot = poly_shaders_ac[polygon];
    if (slot.data == 0) {
        if (layer != 0 && slot.count != polygon_count_230) {
            if (polygon_count_230 != 0) {
                srShader* replacement = slot.Allocate(polygon_count_230);
                if (slot.data != 0) {
                    srHeap.free(slot.data);
                }
                slot.data = replacement;
                slot.count = polygon_count_230;
            } else {
                slot.Release();
            }
        }
    }
    return slot.data;
}

// FUNCTION: SURRENDER 0x10040720
srVector3i* srMeshModel::getPolyUVIndex(long layer, int table)
{
    if (layer < 0 || layer > 3) {
        return 0;
    }
    MeshTable<srVector3i>& slot = poly_uv_indices_114[layer];
    if (slot.data == 0) {
        if (table != 0) {
            if (slot.count != polygon_count_230) {
                if (polygon_count_230 == 0) {
                    slot.Release();
                } else {
                    srVector3i* replacement = slot.Allocate(polygon_count_230);
                    if (slot.data != 0) {
                        srHeap.free(slot.data);
                    }
                    slot.data = replacement;
                    slot.count = polygon_count_230;
                }
            }
            srVector3i* source = getPolyVertex();
            for (long index = 0; index < polygon_count_230; ++index) {
                slot.data[index] = source[index];
            }
        }
    }
    return slot.data;
}

// FUNCTION: SURRENDER 0x100407E0
srVector3T<float>* srMeshModel::getVertexDIG(long vertex, int table)
{
    if (vertex < 0 || vertex > 3) {
        return 0;
    }
    MeshTable<srVector3T<float> >& slot = dig_17c[vertex];
    if (slot.data == 0) {
        if (table != 0) {
            if (slot.count != vertex_location_count_22c) {
                if (vertex_location_count_22c == 0) {
                    slot.Release();
                } else {
                    srVector3T<float>* replacement = slot.Allocate(vertex_location_count_22c);
                    if (slot.data != 0) {
                        srHeap.free(slot.data);
                    }
                    slot.data = replacement;
                    slot.count = vertex_location_count_22c;
                }
            }
            for (long index = 0; index < slot.count; ++index) {
                slot.data[index].x = 0.0f;
                slot.data[index].y = 0.0f;
                slot.data[index].z = 0.0f;
            }
        }
    }
    return slot.data;
}

// FUNCTION: SURRENDER 0x10040A70
srVector2T<float>* srMeshModel::getVertexTexCoords(long vertex, long layer, int table)
{
    if (vertex < 0 || vertex > 3 || layer < 0 || layer > 1) {
        return 0;
    }
    MeshTable<srVector2T<float> >& slot = texcoords_13c[vertex][layer];
    if (slot.data == 0) {
        if (table != 0) {
            if (slot.count != uv_count_234) {
                if (uv_count_234 == 0) {
                    slot.Release();
                } else {
                    srVector2T<float>* replacement = slot.Allocate(uv_count_234);
                    if (slot.data != 0 && slot.count != 0) {
                        long copy = slot.count;
                        if (uv_count_234 < copy) {
                            copy = uv_count_234;
                        }
                        for (long index = 0; index < copy; ++index) {
                            replacement[index] = slot.data[index];
                        }
                    }
                    if (slot.data != 0) {
                        srHeap.free(slot.data);
                    }
                    slot.data = replacement;
                    slot.count = uv_count_234;
                }
            }
            for (long index = 0; index < slot.count; ++index) {
                slot.data[index].x = 0.0f;
                slot.data[index].y = 0.0f;
            }
        }
    }
    return slot.data;
}

// FUNCTION: SURRENDER 0x10040B80
unsigned long* srMeshModel::getVertexShadeIndex(int table)
{
    if (vertex_shade_indices_1ec.data == 0) {
        if (table != 0) {
            if (vertex_shade_indices_1ec.count != vertex_location_count_22c) {
                if (vertex_location_count_22c == 0) {
                    vertex_shade_indices_1ec.Release();
                } else {
                    unsigned long* replacement =
                        vertex_shade_indices_1ec.Allocate(vertex_location_count_22c);
                    if (vertex_shade_indices_1ec.data != 0 && vertex_shade_indices_1ec.count != 0) {
                        long copy = vertex_shade_indices_1ec.count;
                        if (vertex_location_count_22c < copy) {
                            copy = vertex_location_count_22c;
                        }
                        for (long index = 0; index < copy; ++index) {
                            replacement[index] = vertex_shade_indices_1ec.data[index];
                        }
                    }
                    if (vertex_shade_indices_1ec.data != 0) {
                        srHeap.free(vertex_shade_indices_1ec.data);
                    }
                    vertex_shade_indices_1ec.data = replacement;
                    vertex_shade_indices_1ec.count = vertex_location_count_22c;
                }
            }
            for (long index = 0; index < vertex_shade_indices_1ec.count; ++index) {
                vertex_shade_indices_1ec.data[index] = 0;
            }
        }
    }
    return vertex_shade_indices_1ec.data;
}

// FUNCTION: SURRENDER 0x10041B60
void srMeshModel::setDirtyBounds()
{
    setDirty(static_cast<e_flags>(0));
}

// FUNCTION: SURRENDER 0x10041B90
void srMeshModel::setDirtyNormals()
{
    setDirty(static_cast<e_flags>(1));
    setDirty(static_cast<e_flags>(2));
}

// FUNCTION: SURRENDER 0x1003DEA0
void srMeshModel::freeAll()
{
    bounds_minimum_200.x = 0.0f;
    bounds_minimum_200.y = 0.0f;
    bounds_minimum_200.z = 0.0f;
    bounds_maximum_20c.x = 0.0f;
    bounds_maximum_20c.y = 0.0f;
    bounds_maximum_20c.z = 0.0f;
    bounds_center_218.x = 0.0f;
    bounds_center_218.y = 0.0f;
    bounds_center_218.z = 0.0f;
    bounds_radius_224 = 0.0f;
    uv_count_234 = 0;
    vertex_location_count_22c = 0;
    polygon_count_230 = 0;
    active_polygon_count_1fc = 0;
    setDirty(static_cast<e_flags>(0));
    setDirty(static_cast<e_flags>(1));
    setDirty(static_cast<e_flags>(2));
    setDirty(static_cast<e_flags>(3));
    active_polygons_1f4.Release();
    poly_vertices_10c.Release();
    poly_equations_134.Release();
    vertex_locations_1dc.Release();
    vertex_normals_1e4.Release();
    vertex_shade_indices_1ec.Release();
    for (long pass = 0; pass < 4; ++pass) {
        vertex_materials_cc[pass][0].Release();
        vertex_materials_cc[pass][1].Release();
        poly_textures_6c[pass][0].Release();
        poly_textures_6c[pass][1].Release();
        texcoords_13c[pass][0].Release();
        texcoords_13c[pass][1].Release();
        poly_uv_indices_114[pass].Release();
        poly_shaders_ac[pass].Release();
        dig_17c[pass].Release();
        dcg_19c[pass].Release();
        scg_1bc[pass].Release();
    }
}

// FUNCTION: SURRENDER 0x1003CF30
srMeshModel::srMeshModel(long polygons, long vertices)
{
    control_state_390 = 0;
    control_state_394 = 0;
    memset(&tri_mesh_23c, 0, sizeof(tri_mesh_23c));
    reset(polygons, vertices);
    sort_bias_238 = 0.0f;
    for (long pass = 0; pass < 4; ++pass) {
        materials_1c[pass][0] = 0;
        materials_1c[pass][1] = 0;
        textures_3c[pass][0] = 0;
        textures_3c[pass][1] = 0;
        shaders_5c[pass] = srShader();
    }
}

// FUNCTION: SURRENDER 0x10041BF0
srMeshModel::srMeshModel(const srMeshModel& other)
{
    *this = other;
}

// FUNCTION: SURRENDER 0x1003D2C0
void srMeshModel::reset(long polygons, long vertices)
{
    freeAll();
    vertex_location_count_22c = vertices;
    uv_count_234 = vertices;
    polygon_count_230 = polygons;
    active_polygon_count_1fc = polygons;
    pass_count_228 = 1;
    control_state_394 = 0;
    control_state_394 |= 1;
    control_state_394 |= 0x10;
}

// FUNCTION: SURRENDER 0x1003D320
srMeshModel::~srMeshModel()
{
    freeAll();
}

/* Deep copy: reset re-allocates the destination to the source's polygon and
   vertex counts, then every table, pass slot and scalar is copied over. The
   changed bit is set in control_state_390 after the state words transfer. */
// FUNCTION: SURRENDER 0x1003D5D0
srMeshModel& srMeshModel::operator=(const srMeshModel& other)
{
    if (this != &other) {
        srModel::operator=(other);
        reset(other.polygon_count_230, other.vertex_location_count_22c);
        control_state_390 = other.control_state_390;
        control_state_394 = other.control_state_394;
        pass_count_228 = other.pass_count_228;
        control_state_390 |= 8;
        bounds_minimum_200 = other.bounds_minimum_200;
        bounds_maximum_20c = other.bounds_maximum_20c;
        bounds_center_218 = other.bounds_center_218;
        bounds_radius_224 = other.bounds_radius_224;
        active_polygon_count_1fc = other.active_polygon_count_1fc;
        active_polygons_1f4 = other.active_polygons_1f4;
        poly_vertices_10c = other.poly_vertices_10c;
        poly_equations_134 = other.poly_equations_134;
        vertex_locations_1dc = other.vertex_locations_1dc;
        vertex_normals_1e4 = other.vertex_normals_1e4;
        vertex_shade_indices_1ec = other.vertex_shade_indices_1ec;
        for (long pass = 0; pass < 4; ++pass) {
            long side;
            for (side = 0; side < 2; ++side) {
                materials_1c[pass][side] = other.materials_1c[pass][side];
                vertex_materials_cc[pass][side] = other.vertex_materials_cc[pass][side];
            }
            for (side = 0; side < 2; ++side) {
                textures_3c[pass][side] = other.textures_3c[pass][side];
                poly_textures_6c[pass][side] = other.poly_textures_6c[pass][side];
                texcoords_13c[pass][side] = other.texcoords_13c[pass][side];
            }
            shaders_5c[pass] = other.shaders_5c[pass];
            poly_shaders_ac[pass] = other.poly_shaders_ac[pass];
            poly_uv_indices_114[pass] = other.poly_uv_indices_114[pass];
            dig_17c[pass] = other.dig_17c[pass];
            dcg_19c[pass] = other.dcg_19c[pass];
            scg_1bc[pass] = other.scg_1bc[pass];
        }
    }
    return *this;
}

// FUNCTION: SURRENDER 0x1003E120
void srMeshModel::calculateBounds()
{
    bounds_minimum_200.x = 0.0f;
    bounds_minimum_200.y = 0.0f;
    bounds_minimum_200.z = 0.0f;
    bounds_maximum_20c.x = 0.0f;
    bounds_maximum_20c.y = 0.0f;
    bounds_maximum_20c.z = 0.0f;
    bounds_center_218.x = 0.0f;
    bounds_center_218.y = 0.0f;
    bounds_center_218.z = 0.0f;
    bounds_radius_224 = 0.0f;
    if (vertex_location_count_22c != 0) {
        srVector3T<float>* vertices = getVertexLoc();
        if (vertex_location_count_22c != 0) {
            srVectorProcessor::minMax(vertices, bounds_minimum_200, bounds_maximum_20c,
                                      vertex_location_count_22c);
        }
        bounds_center_218.x = (bounds_minimum_200.x + bounds_maximum_20c.x) * 0.5f;
        bounds_center_218.y = (bounds_minimum_200.y + bounds_maximum_20c.y) * 0.5f;
        bounds_center_218.z = (bounds_minimum_200.z + bounds_maximum_20c.z) * 0.5f;
        long count = vertex_location_count_22c;
        if (0 < count) {
            do {
                float dy = vertices->y - bounds_center_218.y;
                float dz = vertices->z - bounds_center_218.z;
                float radius =
                    (vertices->x - bounds_center_218.x) * (vertices->x - bounds_center_218.x) +
                    dy * dy + dz * dz;
                if (bounds_radius_224 < radius) {
                    bounds_radius_224 = radius;
                }
                vertices = vertices + 1;
                count = count - 1;
            } while (count != 0);
        }
        bounds_radius_224 = sqrtf(bounds_radius_224) * 1.00001f;
        updateAllClients(static_cast<Client::e_update>(0));
        control_state_390 &= 0xfffffffe;
    }
}

// FUNCTION: SURRENDER 0x1003E280
void srMeshModel::calculatePolygonNormals()
{
    control_state_390 &= 0xfffffffd;
    srVector4T<float>* equations = getPolyEq();
    srVector3i* polygons = getPolyVertex();
    srVector3T<float>* vertices = getVertexLoc();
    for (long polygon = 0; polygon < polygon_count_230; polygon++) {
        srVector3T<float>* v0 = vertices + polygons[polygon].x;
        srVector3T<float>* v1 = vertices + polygons[polygon].y;
        srVector3T<float>* v2 = vertices + polygons[polygon].z;
        float a = (v2->z - v0->z) * (v1->y - v0->y) - (v2->y - v0->y) * (v1->z - v0->z);
        float b = (v2->x - v0->x) * (v1->z - v0->z) - (v2->z - v0->z) * (v1->x - v0->x);
        float c = (v2->y - v0->y) * (v1->x - v0->x) - (v2->x - v0->x) * (v1->y - v0->y);
        equations[polygon].x = a;
        equations[polygon].y = b;
        equations[polygon].z = c;
        equations[polygon].w = -(a * v0->x + b * v0->y + c * v0->z);
    }
}

// FUNCTION: SURRENDER 0x1003E390
void srMeshModel::calculateVertexNormals()
{
    if ((polygon_count_230 != 0) && (vertex_location_count_22c != 0)) {
        if ((control_state_390 & 2) != 0) {
            calculatePolygonNormals();
        }
        control_state_390 &= 0xfffffffb;
        srVector4T<float>* equations = getPolyEq();
        srVector3T<float>* normals = getVertexNormal();
        srVector3i* polygons = getPolyVertex();
        unsigned long* shade_indices = getVertexShadeIndex(0);
        if (shade_indices == 0) {
            srVectorProcessor::copy((SRDWORD*)normals, 0, vertex_location_count_22c * 3);
            for (long polygon = 0; polygon < polygon_count_230; polygon++) {
                for (long corner = 0; corner < 3; corner++) {
                    srVector3T<float>* normal = normals + ((long*)polygons)[polygon * 3 + corner];
                    normal->x = equations[polygon].x + normal->x;
                    normal->y = equations[polygon].y + normal->y;
                    normal->z = equations[polygon].z + normal->z;
                }
            }
        } else {
            srVector3T<float>* smooth =
                (srVector3T<float>*)srHeap.allocate(vertex_location_count_22c * 0xc);
            long count = vertex_location_count_22c * 3;
            if (count != 0) {
                srVectorProcessor::copy((SRDWORD*)smooth, 0, count);
            }
            for (long polygon = 0; polygon < polygon_count_230; polygon++) {
                for (long corner = 0; corner < 3; corner++) {
                    srVector3T<float>* normal =
                        smooth + shade_indices[((long*)polygons)[polygon * 3 + corner]];
                    normal->x = normal->x + equations[polygon].x;
                    normal->y = normal->y + equations[polygon].y;
                    normal->z = normal->z + equations[polygon].z;
                }
            }
            if (vertex_location_count_22c != 0) {
                srVectorProcessor::copyIndexed(normals, smooth, shade_indices,
                                               vertex_location_count_22c);
            }
            srHeap.free(smooth);
        }
        if (vertex_location_count_22c != 0) {
            srVectorProcessor::normalize(normals, normals, 1.0f, vertex_location_count_22c);
        }
    }
}

// FUNCTION: SURRENDER 0x1003E690
void srMeshModel::scale(const srVector3T<float>& scale)
{
    srVector3T<float>* vertices = getVertexLoc();
    for (long index = 0; index < vertex_location_count_22c; index++) {
        vertices[index].x = vertices[index].x * scale.x;
        vertices[index].y = vertices[index].y * scale.y;
        vertices[index].z = scale.z * vertices[index].z;
    }
    setDirty(static_cast<e_flags>(0));
    setDirty(static_cast<e_flags>(1));
    setDirty(static_cast<e_flags>(2));
    setDirty(static_cast<e_flags>(3));
}

// FUNCTION: SURRENDER 0x1003E780
void srMeshModel::applyMatrix(const srMatrix3T<float>& matrix)
{
    srVector3T<float>* vertices = getVertexLoc();
    for (long index = 0; index < vertex_location_count_22c; index++) {
        float x = vertices[index].x;
        float y = vertices[index].y;
        float z = vertices[index].z;
        vertices[index].x =
            matrix.vectors[0].x * x + matrix.vectors[0].y * y + matrix.vectors[0].z * z;
        vertices[index].y =
            matrix.vectors[1].x * x + matrix.vectors[1].y * y + matrix.vectors[1].z * z;
        vertices[index].z =
            matrix.vectors[2].x * x + matrix.vectors[2].y * y + matrix.vectors[2].z * z;
    }
    setDirty(static_cast<e_flags>(0));
    setDirty(static_cast<e_flags>(1));
    setDirty(static_cast<e_flags>(2));
    setDirty(static_cast<e_flags>(3));
}

// FUNCTION: SURRENDER 0x1003E8C0
void srMeshModel::relocateVertices(const srVector3T<float>& offset)
{
    srVector3T<float>* vertices = getVertexLoc();
    for (long index = 0; index < vertex_location_count_22c; index++) {
        vertices[index].x = vertices[index].x + offset.x;
        vertices[index].y = vertices[index].y + offset.y;
        vertices[index].z = offset.z + vertices[index].z;
    }
    setDirty(static_cast<e_flags>(0));
    setDirty(static_cast<e_flags>(1));
    setDirty(static_cast<e_flags>(2));
    setDirty(static_cast<e_flags>(3));
}

// FUNCTION: SURRENDER 0x1003E9B0
void srMeshModel::centerVertices()
{
    if (vertex_location_count_22c != 0) {
        srVector3T<float> sum;
        sum.x = 0.0f;
        sum.y = 0.0f;
        sum.z = 0.0f;
        srVector3T<float>* vertices = getVertexLoc();
        long count = vertex_location_count_22c;
        if (0 < count) {
            for (long index = 0; index < count; index++) {
                sum.x = sum.x + vertices[index].x;
                sum.y = sum.y + vertices[index].y;
                sum.z = sum.z + vertices[index].z;
            }
        }
        float inverse = 1.0f / count;
        srVector3T<float> offset;
        offset.x = -(sum.x * inverse);
        offset.y = -(sum.y * inverse);
        offset.z = -(sum.z * inverse);
        relocateVertices(offset);
    }
}

// FUNCTION: SURRENDER 0x1003EA90
double srMeshModel::getAverageRadius()
{
    if (vertex_location_count_22c == 0) {
        return 0.0;
    }
    srVector3T<float>* vertices = getVertexLoc();
    long count = vertex_location_count_22c;
    double radius = 0.0;
    if (0 < count) {
        for (long index = 0; index < count; index++) {
            radius =
                sqrt(vertices[index].x * vertices[index].x + vertices[index].z * vertices[index].z +
                     vertices[index].y * vertices[index].y) +
                radius;
        }
        return radius / count;
    }
    return 0.0 / count;
}

// FUNCTION: SURRENDER 0x1003EB20
double srMeshModel::getMaxRadius()
{
    double maximum = 0.0;
    if (vertex_location_count_22c != 0) {
        srVector3T<float>* vertices = getVertexLoc();
        long count = vertex_location_count_22c;
        if (0 < count) {
            for (long index = 0; index < count; index++) {
                float radius = vertices[index].y * vertices[index].y +
                               vertices[index].z * vertices[index].z +
                               vertices[index].x * vertices[index].x;
                if ((float)maximum <= radius) {
                    maximum = radius;
                }
            }
        }
        return sqrt(maximum);
    }
    return 0.0;
}

// FUNCTION: SURRENDER 0x1003EBB0
void srMeshModel::scaleToAverageRadius(double radius)
{
    if (polygon_count_230 != 0) {
        float factor = (float)(radius / getAverageRadius());
        srVector3T<float> scale;
        scale.x = factor;
        scale.y = factor;
        scale.z = factor;
        this->scale(scale);
    }
}

// FUNCTION: SURRENDER 0x1003EBF0
void srMeshModel::scaleToMaxRadius(double radius)
{
    if (polygon_count_230 != 0) {
        float factor = (float)(radius / getMaxRadius());
        srVector3T<float> scale;
        scale.x = factor;
        scale.y = factor;
        scale.z = factor;
        this->scale(scale);
    }
}

// FUNCTION: SURRENDER 0x1003EC30
void srMeshModel::flipFaces()
{
    if (polygon_count_230 != 0) {
        srVector3i* polygons = getPolyVertex();
        for (long polygon = 0; polygon < polygon_count_230; polygon++) {
            long first = polygons[polygon].x;
            polygons[polygon].x = polygons[polygon].y;
            polygons[polygon].y = first;
        }
        setDirty(static_cast<e_flags>(0));
        setDirty(static_cast<e_flags>(1));
        setDirty(static_cast<e_flags>(2));
        setDirty(static_cast<e_flags>(3));
    }
}

// FUNCTION: SURRENDER 0x1003ED20
long srMeshModel::findClosestVertex(const srVector3T<float>& position)
{
    if (vertex_location_count_22c != 0) {
        srVector3T<float>* vertices = getVertexLoc();
        long index = 1;
        long closest = 0;
        float minimum = (vertices[0].x - position.x) * (vertices[0].x - position.x) +
                        (vertices[0].y - position.y) * (vertices[0].y - position.y) +
                        (vertices[0].z - position.z) * (vertices[0].z - position.z);
        if (1 < vertex_location_count_22c) {
            do {
                float x = vertices[index].x - position.x;
                float y = vertices[index].y - position.y;
                float z = vertices[index].z - position.z;
                if (x * x + y * y + z * z < minimum) {
                    minimum = x * x + y * y + z * z;
                    closest = index;
                }
                index = index + 1;
            } while (index < vertex_location_count_22c);
        }
        return closest;
    }
    return 0;
}

// FUNCTION: SURRENDER 0x1003DAC0
void srMeshModel::getTriMesh(TriMesh& mesh)
{
    mesh = getTriMesh();
}

// FUNCTION: SURRENDER 0x1003DC70
const srMeshModel::TriMesh& srMeshModel::getTriMesh()
{
    if ((control_state_390 & 8) != 0) {
        updateTriMesh();
    }
    return tri_mesh_23c;
}

// FUNCTION: SURRENDER 0x1003DC90
void srMeshModel::updateTriMesh()
{
    memset(&tri_mesh_23c, 0, sizeof(TriMesh));
    tri_mesh_23c.vertex_count_00 = vertex_location_count_22c;
    tri_mesh_23c.polygon_count_04 = polygon_count_230;
    tri_mesh_23c.pass_count_08 = pass_count_228;
    tri_mesh_23c.control_flags_0c = control_state_394;
    tri_mesh_23c.sort_bias_148 = sort_bias_238;
    tri_mesh_23c.poly_vertices_10 = getPolyVertex();
    tri_mesh_23c.poly_equations_14 = getPolyEq();
    tri_mesh_23c.positions_38 = getVertexLoc();
    tri_mesh_23c.normals_3c = getVertexNormal();
    for (long pass = 0; pass < 4; pass++) {
        for (long side = 0; side < 2; side++) {
            tri_mesh_23c.materials_70[pass][side] =
                (srMaterial*)(srMaterialIFace*)materials_1c[pass][side];
            tri_mesh_23c.vertex_materials_c0[pass][side] =
                getVertexMaterial(pass, static_cast<e_side>(side), 0);
        }
        for (long layer = 0; layer < 2; layer++) {
            tri_mesh_23c.textures_90[pass][layer] = (srTextureIFace*)textures_3c[pass][layer];
            tri_mesh_23c.poly_textures_e0[pass][layer] = getPolyTexture(pass, layer, 0);
            tri_mesh_23c.texcoords_18[pass][layer] = getVertexTexCoords(pass, layer, 0);
        }
        tri_mesh_23c.shaders_b0[pass] = shaders_5c[pass];
        tri_mesh_23c.poly_uv_110[pass] = getPolyUVIndex(pass, 0);
        tri_mesh_23c.poly_shaders_100[pass] = getPolyShader(pass, 0);
        tri_mesh_23c.dig_40[pass] = getVertexDIG(pass, 0);
        tri_mesh_23c.dcg_50[pass] = getVertexDCG(pass, 0);
        tri_mesh_23c.scg_60[pass] = getVertexSCG(pass, 0);
    }
    if ((tri_mesh_23c.control_flags_0c & 0x10) == 0) {
        getBoundingBox(tri_mesh_23c.bounds_minimum_120, tri_mesh_23c.bounds_maximum_12c);
    }
    if ((tri_mesh_23c.control_flags_0c & 0x20) == 0) {
        getBoundingSphere(tri_mesh_23c.bounds_center_138, tri_mesh_23c.bounds_radius_144);
    }
    tri_mesh_23c.active_polygons_14c = getActivePolygonTable(0);
    tri_mesh_23c.active_polygon_count_150 = active_polygon_count_1fc;
    control_state_390 &= 0xfffffff7;
}

// FUNCTION: SURRENDER 0x1003FDA0
void srMeshModel::setBounds(const srVector3T<float>& minimum, const srVector3T<float>& maximum,
                            const srVector3T<float>& center, float radius)
{
    bounds_minimum_200 = minimum;
    bounds_maximum_20c = maximum;
    bounds_center_218 = center;
    bounds_radius_224 = radius;
    clearDirty(static_cast<e_flags>(0));
    updateAllClients(static_cast<Client::e_update>(0));
    setDirty(static_cast<e_flags>(3));
}

// FUNCTION: SURRENDER 0x1003FE40
int srMeshModel::getBoundingBox(srVector3T<float>& minimum, srVector3T<float>& maximum)
{
    if ((control_state_390 & 1) != 0) {
        calculateBounds();
    }
    minimum = bounds_minimum_200;
    maximum = bounds_maximum_20c;
    return 1;
}

// FUNCTION: SURRENDER 0x1003FE90
int srMeshModel::getBoundingSphere(srVector3T<float>& center, float& radius)
{
    if ((control_state_390 & 1) != 0) {
        calculateBounds();
    }
    center = bounds_center_218;
    radius = bounds_radius_224;
    return 1;
}

// FUNCTION: SURRENDER 0x100408B0
srVector4T<float>* srMeshModel::getVertexSCG(long vertex, int table)
{
    if (vertex < 0 || vertex > 3) {
        return 0;
    }
    MeshTable<srVector4T<float> >& slot = scg_1bc[vertex];
    if (slot.data == 0) {
        if (table != 0) {
            if (slot.count != vertex_location_count_22c) {
                if (vertex_location_count_22c == 0) {
                    slot.Release();
                } else {
                    srVector4T<float>* replacement = slot.Allocate(vertex_location_count_22c);
                    if (slot.data != 0) {
                        srHeap.free(slot.data);
                    }
                    slot.data = replacement;
                    slot.count = vertex_location_count_22c;
                }
            }
            for (long index = 0; index < slot.count; ++index) {
                slot.data[index].x = 1.0f;
                slot.data[index].y = 1.0f;
                slot.data[index].z = 1.0f;
                slot.data[index].w = 1.0f;
            }
        }
        return slot.data;
    }
    return slot.data;
}

// FUNCTION: SURRENDER 0x10040990
srVector4T<float>* srMeshModel::getVertexDCG(long vertex, int table)
{
    if (vertex < 0 || vertex > 3) {
        return 0;
    }
    MeshTable<srVector4T<float> >& slot = dcg_19c[vertex];
    if (slot.data == 0) {
        if (table != 0) {
            if (slot.count != vertex_location_count_22c) {
                if (vertex_location_count_22c == 0) {
                    slot.Release();
                } else {
                    srVector4T<float>* replacement = slot.Allocate(vertex_location_count_22c);
                    if (slot.data != 0) {
                        srHeap.free(slot.data);
                    }
                    slot.data = replacement;
                    slot.count = vertex_location_count_22c;
                }
            }
            for (long index = 0; index < slot.count; ++index) {
                slot.data[index].x = 1.0f;
                slot.data[index].y = 1.0f;
                slot.data[index].z = 1.0f;
                slot.data[index].w = 1.0f;
            }
        }
        return slot.data;
    }
    return slot.data;
}

// FUNCTION: SURRENDER 0x10041B00
srClass* srMeshModel::vInstance()
{
    return new srMeshModel(0, 0);
}

// FUNCTION: SURRENDER 0x1003CEA0
void srMeshModel::render(srGERD& renderer)
{
    if ((control_state_394 & 0x20) == 0) {
        srVector3T<float> center;
        float radius;
        getBoundingSphere(center, radius);
        if (renderer.testBoundingSphere(center, radius) == srGERD::VISIBILITY_POSITIONAL_0) {
            return;
        }
    }
    if ((control_state_394 & 0x10) == 0 && vertex_location_count_22c >= 8) {
        srVector3T<float> minimum;
        srVector3T<float> maximum;
        getBoundingBox(minimum, maximum);
        if (renderer.testBoundingBox(minimum, maximum) == srGERD::VISIBILITY_POSITIONAL_0) {
            return;
        }
    }
    renderTriMesh(renderer, getTriMesh());
}

// TEMPLATE: SURRENDER 0x10045130
// permuteTable<srVector4T<float> >

// TEMPLATE: SURRENDER 0x100451E0
// permuteTable<srVector3i>

// TEMPLATE: SURRENDER 0x10045280
// permuteObjects<srShader>

// TEMPLATE: SURRENDER 0x10045300
// permuteObjects<srPtr<srTextureIFace> >

// TEMPLATE: SURRENDER 0x10045450
// permuteTable<srVector3T<float> >

// TEMPLATE: SURRENDER 0x100454F0
// permuteObjects<srPtr<srMaterialIFace> >

// TEMPLATE: SURRENDER 0x10045640
// permuteTable<srVector2T<float> >

// FUNCTION: SURRENDER 0x100418C0
void srMeshModel::reindexPolygons(const unsigned long* indices)
{
    long index;
    long pass;
    if (polygon_count_230 != 0) {
        permuteTable(getPolyEq(), indices, polygon_count_230);
        permuteTable(getPolyVertex(), indices, polygon_count_230);
        for (pass = 0; pass < pass_count_228; ++pass) {
            if (getPolyUVIndex(pass, 0) != 0) {
                permuteTable(getPolyUVIndex(pass, 0), indices, polygon_count_230);
            }
            if (getPolyShader(pass, 0) != 0) {
                permuteObjects(getPolyShader(pass, 0), indices, polygon_count_230);
            }
            for (long layer = 0; layer < 2; ++layer) {
                if (getPolyTexture(pass, layer, 0) != 0) {
                    permuteObjects(getPolyTexture(pass, layer, 0), indices, polygon_count_230);
                }
            }
        }
        if (active_polygon_count_1fc != 0 && getActivePolygonTable(0) != 0) {
            unsigned long* table = getActivePolygonTable(1);
            unsigned long* forward = new unsigned long[polygon_count_230];
            for (index = 0; index < polygon_count_230; ++index) {
                forward[indices[index]] = index;
            }
            for (index = 0; index < active_polygon_count_1fc; ++index) {
                table[index] = forward[table[index]];
            }
            delete[] forward;
        }
        setDirty(static_cast<e_flags>(0));
        setDirty(static_cast<e_flags>(1));
        setDirty(static_cast<e_flags>(2));
        setDirty(static_cast<e_flags>(3));
    }
}

// FUNCTION: SURRENDER 0x10042230
void srMeshModel::reindexVertices(const unsigned long* indices)
{
    long component;
    long pass;
    long polygon;
    long side;
    long table;
    long index;
    if (vertex_location_count_22c != 0) {
        unsigned long* forward = new unsigned long[vertex_location_count_22c];
        for (index = 0; index < vertex_location_count_22c; ++index) {
            forward[indices[index]] = index;
        }
        permuteTable(getVertexNormal(), indices, vertex_location_count_22c);
        permuteTable(getVertexLoc(), indices, vertex_location_count_22c);
        unsigned long* shade = getVertexShadeIndex(0);
        if (shade != 0) {
            unsigned long* copy = new unsigned long[vertex_location_count_22c];
            if (vertex_location_count_22c != 0 && copy != shade) {
                srVectorProcessor::memcopy(copy, shade, vertex_location_count_22c * 4);
            }
            for (index = 0; index < vertex_location_count_22c; ++index) {
                shade[index] = forward[copy[indices[index]]];
            }
            for (index = 0; index < vertex_location_count_22c; ++index) {
                unsigned long value = shade[index];
                if (value < (unsigned long)index) {
                    shade[index] = shade[value];
                    shade[value] = value;
                }
            }
            delete[] copy;
        }
        for (pass = 0; pass < pass_count_228; ++pass) {
            for (side = 0; side < 2; ++side) {
                if (getVertexMaterial(pass, static_cast<e_side>(side), 0) != 0) {
                    permuteObjects(getVertexMaterial(pass, static_cast<e_side>(side), 0), indices,
                                   vertex_location_count_22c);
                }
            }
            for (table = 0; table < 2; ++table) {
                if (getVertexTexCoords(pass, table, 0) != 0) {
                    permuteTable(getVertexTexCoords(pass, table, 0), indices,
                                 vertex_location_count_22c);
                }
            }
            if (getVertexDIG(pass, 0) != 0) {
                permuteTable(getVertexDIG(pass, 0), indices, vertex_location_count_22c);
            }
            if (getVertexSCG(pass, 0) != 0) {
                permuteTable(getVertexSCG(pass, 0), indices, vertex_location_count_22c);
            }
            if (getVertexDCG(pass, 0) != 0) {
                permuteTable(getVertexDCG(pass, 0), indices, vertex_location_count_22c);
            }
            if (getPolyUVIndex(pass, 0) != 0) {
                srVector3i* uv = getPolyUVIndex(pass, 1);
                for (polygon = 0; polygon < polygon_count_230; ++polygon, ++uv) {
                    int* corner = &uv->x;
                    for (component = 0; component < 3; ++component, ++corner) {
                        int index = *corner;
                        if (index < vertex_location_count_22c) {
                            index = forward[index];
                        }
                        *corner = index;
                    }
                }
            }
        }
        srVector3i* vertices = getPolyVertex();
        for (polygon = 0; polygon < polygon_count_230; ++polygon, ++vertices) {
            int* corner = &vertices->x;
            for (component = 0; component < 3; ++component, ++corner) {
                *corner = forward[*corner];
            }
        }
        setDirty(static_cast<e_flags>(0));
        setDirty(static_cast<e_flags>(1));
        setDirty(static_cast<e_flags>(2));
        setDirty(static_cast<e_flags>(3));
        delete[] forward;
    }
}

// FUNCTION: SURRENDER 0x10040C40
void srMeshModel::verify(srRuntimeClass::e_verify mode)
{
    long i;
    long j;
    long p;
    long s;
    long v;

    srClass::verify(mode);
    TriMesh t;
    getTriMesh(t);
    if (t.vertex_count_00 <= 0) {
        srAssertFail("t.vnum > 0", "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x5cf, 0);
    }
    if (t.polygon_count_04 <= 0) {
        srAssertFail("t.pnum > 0", "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x5d0, 0);
    }
    if (t.pass_count_08 > 0 && t.pass_count_08 <= MAX_PASSES) {
        if (!srFinite(t.sort_bias_148)) {
            srAssertFail("srFinite(t.sortBias)", "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp",
                         0x5d2, 0);
        }
        if (t.active_polygons_14c != 0) {
            if (t.polygon_count_04 < t.active_polygon_count_150) {
                srAssertFail("t.aPnum <= t.pnum", "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp",
                             0x5da, 0);
            }
            for (i = 0; i < t.active_polygon_count_150; ++i) {
                /* c-style-cast-ok: the assert text spells (SRDWORD)(t.pnum). */
                if ((SRDWORD)t.polygon_count_04 <= t.active_polygons_14c[i]) {
                    srAssertFail("t.APT[i] < (SRDWORD)(t.pnum)",
                                 "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x5dd, 0);
                }
            }
            for (i = 0; i < t.active_polygon_count_150; ++i) {
                for (j = i + 1; j < t.active_polygon_count_150; ++j) {
                    if (t.active_polygons_14c[i] == t.active_polygons_14c[j]) {
                        srAssertFail("t.APT[i] != t.APT[j]",
                                     "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x5e1, 0);
                    }
                }
            }
        }
        if (t.poly_vertices_10 == 0) {
            srAssertFail("t.pVertex", "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x5e5, 0);
        }
        for (i = 0; i < t.polygon_count_04; ++i) {
            int* component = &t.poly_vertices_10[i].x;
            for (j = 0; j < 3; ++j, ++component) {
                if (*component < 0 || t.vertex_count_00 <= *component) {
                    srAssertFail("t.pVertex[i][j] >= 0 && t.pVertex[i][j] < t.vnum",
                                 "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x5ed, 0);
                }
            }
        }
        if (t.poly_equations_14 == 0) {
            srAssertFail("t.pEq", "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x5f1, 0);
        }
        for (i = 0; i < t.polygon_count_04; ++i) {
            if (!t.poly_equations_14[i].isValid()) {
                srAssertFail("t.pEq[i].isValid()", "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp",
                             0x5f6, 0);
            }
        }
        if (t.positions_38 == 0) {
            srAssertFail("t.vLoc", "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x5fa, 0);
        }
        for (i = 0; i < t.vertex_count_00; ++i) {
            if (!t.positions_38[i].isValid()) {
                srAssertFail("t.vLoc[i].isValid()",
                             "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x5fe, 0);
            }
        }
        if (t.normals_3c == 0) {
            srAssertFail("t.vNorm", "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x602, 0);
        }
        for (i = 0; i < t.vertex_count_00; ++i) {
            if (fabs(t.normals_3c[i].length() - 1.0) >= 0.01) {
                srAssertFail("fabs(t.vNorm[i].length()-1.0) < 0.01",
                             "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x606, 0);
            }
        }
        for (p = 0; p < pass_count_228; ++p) {
            if (t.dig_40[p] != 0) {
                for (i = 0; i < t.vertex_count_00; ++i) {
                    if (!t.dig_40[p][i].isValid()) {
                        srAssertFail("t.DIG[p][i].isValid()",
                                     "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x611, 0);
                    }
                }
            }
            if (t.dcg_50[p] != 0) {
                for (i = 0; i < t.vertex_count_00; ++i) {
                    if (!t.dcg_50[p][i].isValid()) {
                        srAssertFail("t.DCG[p][i].isValid()",
                                     "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x615, 0);
                    }
                }
            }
            if (t.scg_60[p] != 0) {
                for (i = 0; i < t.vertex_count_00; ++i) {
                    if (!t.scg_60[p][i].isValid()) {
                        srAssertFail("t.SCG[p][i].isValid()",
                                     "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x619, 0);
                    }
                }
            }
            for (j = 0; j < 2; ++j) {
                if (t.texcoords_18[p][j] != 0) {
                    for (i = 0; i < t.vertex_count_00; ++i) {
                        if (!t.texcoords_18[p][j][i].isValid()) {
                            srAssertFail("t.vUV[p][j][i].isValid()",
                                         "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x620,
                                         0);
                        }
                    }
                }
                if (t.poly_textures_e0[p][j] != 0) {
                    for (i = 0; i < t.polygon_count_04; ++i) {
                        if (!t.poly_textures_e0[p][j][i]) {
                            srAssertFail("t.pTexture[p][j][i]",
                                         "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x625,
                                         0);
                        }
                    }
                }
            }
            for (s = 0; s < 2; ++s) {
                if (t.vertex_materials_c0[p][s] != 0) {
                    for (v = 0; v < t.vertex_count_00; ++v) {
                        if (!t.vertex_materials_c0[p][s][v]) {
                            srAssertFail("t.vMaterial[p][s][v]",
                                         "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x62f,
                                         0);
                        }
                    }
                }
            }
            if (t.poly_shaders_100[p] == 0) {
                if (!t.shaders_b0[p].isValid()) {
                    srAssertFail("t.shader[p].isValid()",
                                 "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x639, 0);
                }
            } else {
                for (v = 0; v < t.polygon_count_04; ++v) {
                    if (!t.poly_shaders_100[p][v].isValid()) {
                        srAssertFail("t.pShader[p][v].isValid()",
                                     "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x637, 0);
                    }
                }
            }
        }
        return;
    }
    srAssertFail("t.passes > 0 && t.passes <= MAX_PASSES",
                 "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x5d1, 0);
}

// FUNCTION: SURRENDER 0x1003EE00
void srMeshModel::dump(std::ostream& stream)
{
    long pass;
    int stage;
    long polygon;
    long vertex;
    long side;
    long changes;

    srModel::dump(stream);
    long flags = stream.flags();
    stream.flags((flags & 0xfffffe7fL) | 0x40);
    stream.width(0x20);
    stream << "  Polygons: " << polygon_count_230 << '\n';
    stream.width(0x20);
    stream << "  Vertices: " << vertex_location_count_22c << '\n';
    stream.width(0x20);
    stream << "  Passes: " << pass_count_228 << '\n';
    stream.width(0x20);
    srVector3T<float> minimum;
    srVector3T<float> maximum;
    getBoundingBox(minimum, maximum);
    stream << "  Bounding box min: ";
    stream << '{' << minimum.x << ',' << minimum.y << ',' << minimum.z << '}' << '\n';
    stream.width(0x20);
    stream << "  Bounding box max: ";
    stream << '{' << maximum.x << ',' << maximum.y << ',' << maximum.z << '}' << '\n';
    srVector3T<float> center;
    float radius;
    getBoundingSphere(center, radius);
    stream.width(0x20);
    stream << "  Bounding sphere origin: ";
    stream << '{' << center.x << ',' << center.y << ',' << center.z << '}' << '\n';
    stream.width(0x20);
    stream << "  Bounding sphere radius: " << radius << '\n';
    stream.width(0x20);
    stream << "  Sort bias: " << sort_bias_238 << '\n';
    stream.width(0x20);
    stream << "  Flags: ";
    if (control_state_390 == 0) {
        stream << "[NONE]";
    } else {
        stream << '[';
        bool first = true;
        const char* names = s_flag_names_100a499c;
        const char* name = names;
        for (unsigned long bit = 0; bit < 0x20; ++bit) {
            if ((control_state_390 & (1 << bit)) == 0) {
                if (name != 0) {
                    while (*name != 0 && *name != ',') {
                        ++name;
                    }
                    if (*name == ',') {
                        ++name;
                    }
                }
            } else {
                if (first) {
                    first = false;
                } else {
                    stream << ',';
                }
                if (name == 0 || *name == 0) {
                    stream << bit;
                } else {
                    while (*name != 0 && *name != ',') {
                        stream << *name;
                        ++name;
                    }
                    if (*name == ',') {
                        ++name;
                    }
                }
            }
        }
        stream << ']';
    }
    stream << '\n';
    stream.width(0x20);
    stream << "  Control flags: ";
    if (control_state_394 == 0) {
        stream << "[NONE]";
    } else {
        stream << '[';
        bool first = true;
        const char* names = s_control_names_100a4998;
        const char* name = names;
        for (unsigned long bit = 0; bit < 0x20; ++bit) {
            if ((control_state_394 & (1 << bit)) == 0) {
                if (name != 0) {
                    while (*name != 0 && *name != ',') {
                        ++name;
                    }
                    if (*name == ',') {
                        ++name;
                    }
                }
            } else {
                if (first) {
                    first = false;
                } else {
                    stream << ',';
                }
                if (name == 0 || *name == 0) {
                    stream << bit;
                } else {
                    while (*name != 0 && *name != ',') {
                        stream << *name;
                        ++name;
                    }
                    if (*name == ',') {
                        ++name;
                    }
                }
            }
        }
        stream << ']';
    }
    stream << "\n\n";
    stream << "  Materials, textures, shaders\n";
    stream << "  ----------------------------\n";
    for (pass = 0; pass < 4; ++pass) {
        bool defined = (materials_1c[pass][0] != 0) || (materials_1c[pass][1] != 0);
        for (stage = 0; stage < 2; ++stage) {
            if (textures_3c[pass][stage] != 0) {
                defined = true;
            }
        }
        if (defined) {
            stream << "    Pass " << pass << '\n';
        }
        if (materials_1c[pass][0] != 0) {
            stream.width(0x20);
            stream << "      Front material: " << materials_1c[pass][0]->getName() << '\n';
        }
        if (materials_1c[pass][1] != 0) {
            stream.width(0x20);
            stream << "      Back material: " << materials_1c[pass][1]->getName() << '\n';
        }
        if (defined) {
            stream.width(0x20);
            stream << "      Shader: " << shaders_5c[pass] << '\n';
        }
        for (stage = 0; stage < 2; ++stage) {
            if (textures_3c[pass][stage] != 0) {
                char label[36];
                sprintf(label, "      Texture %d", stage);
                stream.width(0x20);
                stream << label << textures_3c[pass][stage]->getName() << '\n';
            }
        }
    }
    stream << '\n' << "  Polygon fields defined\n";
    stream << "  ----------------------\n";
    stream.width(0x20);
    if (getPolyEq() != 0) {
        stream << "    Polygon plane equations" << '\n';
    }
    stream.width(0x20);
    if (getPolyVertex() != 0) {
        stream << "    Polygon vertex indices" << '\n' << '\n';
    }
    for (pass = 0; pass < 4; ++pass) {
        srShader* shaders = getPolyShader(pass, 0);
        srVector3i* uv = getPolyUVIndex(pass, 0);
        srPtr<srTextureIFace>* textures[2];
        bool defined = false;
        for (stage = 0; stage < 2; ++stage) {
            textures[stage] = getPolyTexture(pass, stage, 0);
            if (textures[stage] != 0) {
                defined = true;
            }
        }
        if (defined || shaders != 0 || uv != 0) {
            stream << "    Pass " << pass << '\n';
            if (shaders != 0) {
                stream << "      Shader override array" << '\n';
            }
            if (uv != 0) {
                stream << "      Polygon UV override array" << '\n';
            }
            for (stage = 0; stage < 2; ++stage) {
                if (textures[stage] != 0) {
                    stream << "      Texture override array (stage " << stage << ")\n";
                }
            }
        }
    }
    stream << "\n  Vertex fields defined\n";
    stream << "  ---------------------\n";
    stream.width(0x20);
    if (getVertexLoc() != 0) {
        stream << "    Vertex locations" << '\n';
    }
    stream.width(0x20);
    if (getVertexNormal() != 0) {
        stream << "    Vertex normals" << '\n';
    }
    stream.width(0x20);
    if (getVertexShadeIndex(0) != 0) {
        stream << "    Vertex normal remapping indices" << '\n';
    }
    for (pass = 0; pass < 4; ++pass) {
        srVector3T<float>* dig = getVertexDIG(pass, 0);
        srVector4T<float>* scg = getVertexSCG(pass, 0);
        srVector4T<float>* dcg = getVertexDCG(pass, 0);
        srVector2T<float>* uvs[2];
        for (stage = 0; stage < 2; ++stage) {
            uvs[stage] = getVertexTexCoords(pass, stage, 0);
        }
        srPtr<srMaterialIFace>* front = getVertexMaterial(pass, static_cast<e_side>(0), 0);
        srPtr<srMaterialIFace>* back = getVertexMaterial(pass, static_cast<e_side>(1), 0);
        bool defined = dig != 0 || scg != 0 || dcg != 0 || front != 0 || back != 0;
        for (stage = 0; stage < 2; ++stage) {
            if (uvs[stage] != 0) {
                defined = true;
            }
        }
        if (defined) {
            stream << "    Pass " << pass << '\n';
        }
        if (dig != 0) {
            stream << "      DIG array\n";
        }
        if (scg != 0) {
            stream << "      SCG array\n";
        }
        if (dcg != 0) {
            stream << "      DCG array\n";
        }
        for (stage = 0; stage < 2; ++stage) {
            if (uvs[stage] != 0) {
                stream << "      Texture coordinate array (stage " << stage << ")";
                if (stage == 0) {
                    stream << " (" << getUVCount() << " UV entries)";
                }
                stream << '\n';
            }
        }
        if (front != 0) {
            stream << "      Front material override array\n";
        }
        if (back != 0) {
            stream << "      Back material override array\n";
        }
    }
    stream << '\n';
    srVector3i* vertices = getPolyVertex();
    unsigned long strips = 1;
    for (pass = 0; pass < pass_count_228; ++pass) {
        for (polygon = 1; polygon < polygon_count_230; ++polygon) {
            int* next = &vertices[polygon].x;
            int* previous = &vertices[polygon - 1].x;
            long shared = 0;
            for (long a = 0; a < 3; ++a) {
                for (long b = 0; b < 3; ++b) {
                    if (next[a] == previous[b]) {
                        ++shared;
                    }
                }
            }
            if (getPolyShader(pass, 0) != 0) {
                srShader* shaders = getPolyShader(pass, 1);
                if (shaders[polygon].value != shaders[polygon - 1].value) {
                    shared = 0;
                }
            }
            for (stage = 0; stage < 2; ++stage) {
                if (getPolyTexture(pass, stage, 0) != 0) {
                    srPtr<srTextureIFace>* textures = getPolyTexture(pass, stage, 1);
                    if (textures[polygon] != textures[polygon - 1]) {
                        shared = 0;
                    }
                }
            }
            if (shared < 2) {
                ++strips;
            }
        }
    }
    unsigned long shader_changes = 0;
    unsigned long texture_changes = 0;
    unsigned long material_changes = 0;
    for (pass = 0; pass < pass_count_228; ++pass) {
        if (getPolyShader(pass, 0) != 0) {
            srShader* shaders = getPolyShader(pass, 1);
            changes = 1;
            for (polygon = 1; polygon < polygon_count_230; ++polygon) {
                if (shaders[polygon].value != shaders[polygon - 1].value) {
                    ++changes;
                }
            }
            shader_changes += changes;
        }
        for (stage = 0; stage < 2; ++stage) {
            if (getPolyTexture(pass, stage, 0) != 0) {
                srPtr<srTextureIFace>* textures = getPolyTexture(pass, stage, 1);
                changes = 1;
                for (polygon = 1; polygon < polygon_count_230; ++polygon) {
                    if (textures[polygon] != textures[polygon - 1]) {
                        ++changes;
                    }
                }
                texture_changes += changes;
            }
        }
        for (side = 0; side < 2; ++side) {
            if (getVertexMaterial(pass, static_cast<e_side>(side), 0) != 0) {
                srPtr<srMaterialIFace>* materials =
                    getVertexMaterial(pass, static_cast<e_side>(side), 1);
                changes = 1;
                for (vertex = 1; vertex < vertex_location_count_22c; ++vertex) {
                    if (materials[vertex] != materials[vertex - 1]) {
                        ++changes;
                    }
                }
                material_changes += changes;
            }
        }
    }
    if (texture_changes == 0) {
        texture_changes = 1;
    }
    if (shader_changes == 0) {
        shader_changes = 1;
    }
    if (material_changes == 0) {
        material_changes = 1;
    }
    stream.width(0x20);
    stream << "  Total triangle strips:" << strips << '\n';
    stream.width(0x20);
    stream << "  Texture changes:" << texture_changes << '\n';
    stream.width(0x20);
    stream << "  Shader changes:" << shader_changes << '\n';
    stream.width(0x20);
    stream << "  Material changes:" << material_changes << '\n';
    stream << '\n';
    stream.flags(flags & 0x7fff);
}

/* SR.DLL's own copy of the shared pipeline singleton; the consumer build
   carries a parallel copy in stMeshModel.cpp. Retail keeps the provider
   bodies inside this TU: the pipeline block (0x10043D50-0x10045030) sits
   between this file's functions and its permuteTable template emissions
   (0x10045130+), so same-TU /Ob2 inlining reproduces the call/inline split
   retail shows (PrepareSlot expands inside renderTriMesh and Reset, while Get
   calls it). */

// GLOBAL: SURRENDER 0x100A4790
srTriMeshPipeline* srTriMeshPipeline::pipe = 0;

/* The constructor has no standalone retail emission; Get expands it inline. */
inline srTriMeshPipeline::srTriMeshPipeline()
{
    flags_28 = 0;
    vertex_pipe_90 = new srVertexPipe();
    flushing_8c = 0;
    Reset004753F0(0);
    Flush00475510();
}

// FUNCTION: SURRENDER 0x100440A0
srTriMeshPipeline::~srTriMeshPipeline()
{
    while (flushing_8c != 0) {
    }

    delete vertex_pipe_90;
}

// SYNTHETIC: SURRENDER 0x10044080
// srTriMeshPipeline::`scalar deleting destructor'

// FUNCTION: SURRENDER 0x10044070
void srTriMeshPipeline::SetFlags004752C0(srShader shader)
{
    shader_74 = shader;
    current_pass_18->flags_08 = shader;
}

/* Bind a renderer and rebuild the current slot. Retail duplicates the prepare
   body rather than calling PrepareSlot00475540. */
// FUNCTION: SURRENDER 0x100441A0
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
    current_pass_18->texture_array_10 = 0;
    current_pass_18->shader_14 = 0;
    current_pass_18->st_18 = 0;
    current_pass_18->poly_uv_1c = 0;
}

// FUNCTION: SURRENDER 0x100442B0
void srTriMeshPipeline::Flush00475510()
{
    flushing_8c = 1;
    if (slot_count_84 > 0) {
        FlushSlots();
    }
    flushing_8c = 0;
}

/* Point current_record_14 / current_pass_18 at slot slot_count_84, growing
   either table by (capacity + slot + 8) when needed. */
// FUNCTION: SURRENDER 0x100442E0
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
    current_pass_18->texture_array_10 = 0;
    current_pass_18->shader_14 = 0;
    current_pass_18->st_18 = 0;
    current_pass_18->poly_uv_1c = 0;
}

// FUNCTION: SURRENDER 0x100443A0
void srTriMeshPipeline::FlushSlots()
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

            /* Retail grows and re-reads the member array through
               vertex_arrays_a4[slot] on the left while the right side keeps
               the vertex_arrays snapshot taken before the loop. */
            for (unsigned long slot = 1; slot < slot_count_84; ++slot) {
                unsigned long offset = slot * culler_output.vertex_count_10;
                vertex_arrays_a4[slot].eye_locations_00 =
                    vertex_arrays[0].eye_locations_00 + offset;
                vertex_arrays_a4[slot].diffuse_04 = vertex_arrays[0].diffuse_04 + offset;
                vertex_arrays_a4[slot].specular_08 = vertex_arrays[0].specular_08 + offset;
                vertex_arrays_a4[slot].st0_0c = vertex_arrays[0].st0_0c + offset;
                vertex_arrays_a4[slot].st1_10 = vertex_arrays[0].st1_10 + offset;
                vertex_arrays_a4[slot].q0_14 = vertex_arrays[0].q0_14 + offset;
                vertex_arrays_a4[slot].q1_18 = vertex_arrays[0].q1_18 + offset;
                vertex_arrays_a4[slot].packed_1c = vertex_arrays[0].packed_1c + offset;
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

/* Lazy singleton: construct once against the imported pipe static, then bind
   the caller's renderer and rebuild the current slot. */
// FUNCTION: SURRENDER 0x10043E00
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

/* Provider-side renderTriMesh: the consumer's stMeshModel override extends
   this same shape with its software-cull and inverted-depth paths. The
   pipeline calls expand PrepareSlot inline here while SetFlags, Get and the
   pipeline vtable call stay out-of-line, matching retail's emissions. */
// FUNCTION: SURRENDER 0x1003CA80
void srMeshModel::renderTriMesh(srGERD& renderer, const TriMesh& mesh)
{
    if (mesh.polygon_count_04 != 0 && mesh.vertex_count_00 != 0) {
        renderer.pushEnable();

        if ((mesh.control_flags_0c & 0x40) != 0 &&
            !renderer.isEnabled(srGERD::ENABLE_POSITIONAL_1)) {
            renderer.toggle(srGERD::ENABLE_POSITIONAL_1);
        }

        long material_side;
        for (long side = 1; side >= 0; --side) {
            if ((mesh.control_flags_0c & (1u << side)) != 0) {
                srTriMeshPipeline* pipeline = srTriMeshPipeline::Get004750A0(&renderer);
                // reinterpret-ok: sort bias is stored as float bits in extra_40
                pipeline->extra_40 = *reinterpret_cast<const unsigned long*>(&mesh.sort_bias_148);
                pipeline->triangles_34 = mesh.poly_vertices_10;
                pipeline->triangle_count_1c = static_cast<unsigned long>(mesh.polygon_count_04);
                pipeline->positions_38 = mesh.positions_38;
                pipeline->vertex_count_20 = static_cast<unsigned long>(mesh.vertex_count_00);
                pipeline->vertex_extras_3c = mesh.normals_3c;
                pipeline->projected_vertices_30 = mesh.poly_equations_14;

                if (mesh.active_polygons_14c != 0) {
                    pipeline->active_triangles_2c = mesh.active_polygons_14c;
                    pipeline->active_triangle_count_24 =
                        static_cast<unsigned long>(mesh.active_polygon_count_150);
                }

                if ((mesh.control_flags_0c & 0x10) == 0) {
                    pipeline->bounds_minimum_44 = mesh.bounds_minimum_120;
                    pipeline->bounds_maximum_50 = mesh.bounds_maximum_12c;
                    if (pipeline->bounds_state_6c == 0) {
                        pipeline->bounds_state_6c = 2;
                    }
                }
                if ((mesh.control_flags_0c & 0x20) == 0) {
                    pipeline->bounds_center_5c = mesh.bounds_center_138;
                    pipeline->bounds_radius_68 = mesh.bounds_radius_144;
                    pipeline->bounds_state_6c = 1;
                }

                material_side = side;
                if (side == 1) {
                    renderer.setCullMode(srGERD::CULL_BACK);
                    if (!renderer.isEnabled(static_cast<srGERD::e_enable>(3))) {
                        renderer.toggle(static_cast<srGERD::e_enable>(3));
                    }
                    if ((mesh.control_flags_0c & 4) != 0) {
                        material_side = 0;
                    }
                } else {
                    if ((mesh.control_flags_0c & 8) == 0) {
                        renderer.setCullMode(srGERD::CULL_NONE);
                    } else {
                        renderer.setCullMode(srGERD::CULL_FRONT);
                    }
                    if (renderer.isEnabled(static_cast<srGERD::e_enable>(3))) {
                        renderer.toggle(static_cast<srGERD::e_enable>(3));
                    }
                }

                for (long pass = 0; pass < mesh.pass_count_08; ++pass) {
                    pipeline->current_record_14->flags_00 = 0;
                    pipeline->current_pass_18->shader_14 = 0;
                    pipeline->current_pass_18->texture_array_0c = 0;
                    pipeline->current_pass_18->texture_array_10 = 0;

                    if (mesh.dig_40[pass] != 0) {
                        pipeline->current_record_14->colors_0c = mesh.dig_40[pass];
                        pipeline->current_record_14->color_format_10 = 1;
                        pipeline->current_record_14->flags_00 |= 1;
                    }
                    if (mesh.dcg_50[pass] != 0) {
                        pipeline->current_record_14->dcg_14 = mesh.dcg_50[pass];
                        pipeline->current_record_14->flags_00 |= 2;
                    }
                    if (mesh.scg_60[pass] != 0) {
                        pipeline->current_record_14->scg_18 = mesh.scg_60[pass];
                        pipeline->current_record_14->flags_00 |= 4;
                    }

                    if (mesh.vertex_materials_c0[pass][material_side] == 0) {
                        srMaterialIFace* material = mesh.materials_70[pass][material_side];
                        pipeline->material_80 = material;
                        pipeline->current_record_14->material_08 = material;
                    } else {
                        pipeline->current_record_14->vertex_materials_28 =
                            mesh.vertex_materials_c0[pass][material_side];
                        pipeline->current_record_14->flags_00 |= 0x40;
                    }

                    if (mesh.poly_uv_110[pass] != 0) {
                        pipeline->current_pass_18->poly_uv_1c = mesh.poly_uv_110[pass];
                    }

                    if (mesh.poly_shaders_100[pass] == 0) {
                        pipeline->SetFlags004752C0(mesh.shaders_b0[pass]);
                    } else {
                        pipeline->current_pass_18->shader_14 = mesh.poly_shaders_100[pass];
                    }

                    if (mesh.texcoords_18[pass][0] != 0) {
                        pipeline->current_record_14->st0_20 = mesh.texcoords_18[pass][0];
                        pipeline->current_record_14->flags_00 |= 0x10;
                    }
                    if (mesh.texcoords_18[pass][1] != 0) {
                        pipeline->current_record_14->flags_00 |= 0x20;
                        pipeline->current_record_14->st1_24 = mesh.texcoords_18[pass][1];
                    }

                    for (long layer = 0; layer < 2; ++layer) {
                        if (mesh.poly_textures_e0[pass][layer] == 0) {
                            srTextureIFace* texture = mesh.textures_90[pass][layer];
                            (&pipeline->texture_78)[layer] = texture;
                            (&pipeline->current_pass_18->texture_00)[layer] = texture;
                        } else {
                            (&pipeline->current_pass_18->texture_array_0c)[layer] =
                                mesh.poly_textures_e0[pass][layer];
                        }
                    }

                    ++pipeline->slot_count_84;
                    pipeline->PrepareSlot00475540();
                }

                pipeline->FlushIfCurrent();
            }
        }

        renderer.popEnable();
    }
}

// FUNCTION: SURRENDER 0x100417D0
float srMeshModel::getSortBias() const
{
    return sort_bias_238;
}

// FUNCTION: SURRENDER 0x10041790
void srMeshModel::setSortBias(float bias)
{
    sort_bias_238 = bias;
    if ((control_state_390 & 8) == 0) {
        control_state_390 |= 8;
        control_state_390 |= 8;
    }
}

// FUNCTION: SURRENDER 0x100417E0
void srMeshModel::disable(e_control control)
{
    control_state_394 &= ~(1 << control);
    if ((control_state_390 & 8) == 0) {
        control_state_390 |= 8;
        control_state_390 |= 8;
    }
}

// FUNCTION: SURRENDER 0x10041830
void srMeshModel::enable(e_control control)
{
    control_state_394 |= 1 << control;
    if ((control_state_390 & 8) == 0) {
        control_state_390 |= 8;
        control_state_390 |= 8;
    }
}

// FUNCTION: SURRENDER 0x10041870
int srMeshModel::isEnabled(e_control control) const
{
    return (control_state_394 & (1 << control)) != 0;
}

// FUNCTION: SURRENDER 0x10041890
void srMeshModel::setPassCount(long count)
{
    pass_count_228 = count;
    if (count < 1) {
        pass_count_228 = 1;
        return;
    }
    if (count > MAX_PASSES) {
        pass_count_228 = MAX_PASSES;
    }
}

// FUNCTION: SURRENDER 0x10041AC0
long srMeshModel::getPassCount() const
{
    return pass_count_228;
}

// FUNCTION: SURRENDER 0x10041AD0
long srMeshModel::getPolygonCount() const
{
    return polygon_count_230;
}

// FUNCTION: SURRENDER 0x10041AE0
long srMeshModel::getVertexCount() const
{
    return vertex_location_count_22c;
}

/* Retail expands setDirty(0..3) inline; flag 0 also runs updateAllClients. */
// FUNCTION: SURRENDER 0x10041660
void srMeshModel::setDirtyAll()
{
    setDirty(static_cast<e_flags>(0));
    setDirty(static_cast<e_flags>(1));
    setDirty(static_cast<e_flags>(2));
    setDirty(static_cast<e_flags>(3));
}

// FUNCTION: SURRENDER 0x10041AF0
const char* srMeshModel::sGetClassName()
{
    return "srMeshModel";
}

// SYNTHETIC: SURRENDER 0x1003FFB0
// srVector3i implicit copy-assignment emission

// TEMPLATE: SURRENDER 0x10042A30
// srClassSupport<srMeshModel, srModel, 0, 0x2010>::~srClassSupport

// SYNTHETIC: SURRENDER 0x10043970
// std::ios_base::Init global static-init block

// LIBRARY: SURRENDER 0x10043980
// std::ios_base::Init::Init

// SYNTHETIC: SURRENDER 0x10043990
// std::ios_base::Init global atexit registrar

// LIBRARY: SURRENDER 0x100439A0
// std::ios_base::Init::~Init

// SYNTHETIC: SURRENDER 0x100439B0
// std::_Winit global static-init block

// LIBRARY: SURRENDER 0x100439C0
// std::_Winit::_Winit

// SYNTHETIC: SURRENDER 0x100439D0
// std::_Winit global atexit registrar

// LIBRARY: SURRENDER 0x100439E0
// std::_Winit::~_Winit

// SYNTHETIC: SURRENDER 0x100439F0
// srClassSupport<srMeshModel, srModel, 0, 0x2010> scalar deleting destructor

// SYNTHETIC: SURRENDER 0x10043A10
// srPtr<srTextureIFace> element destructor emission

// SYNTHETIC: SURRENDER 0x10043A40
// srPtr<srMaterialIFace> element destructor emission

// TEMPLATE: SURRENDER 0x10043A70
// srClassSupport<srModel, srClass, true, 0x2000>::sGetClassNode

// SYNTHETIC: SURRENDER 0x100425A0
// srMeshModel default constructor closure

// SYNTHETIC: SURRENDER 0X100425B0
// srMeshModel scalar deleting destructor

// TEMPLATE: SURRENDER 0X10042A10
// srClassSupport<srMeshModel, srModel, 0, 0x2010>::clone

// TEMPLATE: SURRENDER 0X10044010
// srArray<T>::release emission

// TEMPLATE: SURRENDER 0X10044030
// srArray<T>::release emission

// TEMPLATE: SURRENDER 0X10044050
// srArray<T>::release emission
