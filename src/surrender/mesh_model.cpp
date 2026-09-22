#include "surrender/srMeshModel.h"
#include <string.h>
#pragma intrinsic(memset)

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
                    MeshTable<srVector3T<float> >::Allocate(vertex_location_count_22c);
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
            srVector3T<float>* replacement =
                MeshTable<srVector3T<float> >::Allocate(vertex_location_count_22c);
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
            srVector4T<float>* replacement =
                MeshTable<srVector4T<float> >::Allocate(polygon_count_230);
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
                    unsigned long* replacement =
                        MeshTable<unsigned long>::Allocate(polygon_count_230);
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
                srVector3i* replacement = MeshTable<srVector3i>::Allocate(polygon_count_230);
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
                    srPtr<srMaterialIFace>* replacement =
                        MeshTable<srPtr<srMaterialIFace> >::Allocate(vertex_location_count_22c);
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
                    srPtr<srTextureIFace>* replacement =
                        MeshTable<srPtr<srTextureIFace> >::Allocate(polygon_count_230);
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
                srShader* replacement = MeshTable<srShader>::Allocate(polygon_count_230);
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
                    srVector3i* replacement = MeshTable<srVector3i>::Allocate(polygon_count_230);
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
                    srVector3T<float>* replacement =
                        MeshTable<srVector3T<float> >::Allocate(vertex_location_count_22c);
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
                    srVector2T<float>* replacement =
                        MeshTable<srVector2T<float> >::Allocate(uv_count_234);
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
                        MeshTable<unsigned long>::Allocate(vertex_location_count_22c);
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

// FUNCTION: SURRENDER 0x10041710
void srMeshModel::setDirty(e_flags flag)
{
    unsigned long mask = 1 << (flag & 0x1f);
    if ((control_state_390 & mask) == 0) {
        control_state_390 |= mask;
        control_state_390 |= 8;
        if (flag == 0) {
            updateAllClients(static_cast<Client::e_update>(0));
        }
    }
}

// FUNCTION: SURRENDER 0x10041750
void srMeshModel::clearDirty(e_flags flag)
{
    control_state_390 &= ~(1 << (flag & 0x1f));
}

// FUNCTION: SURRENDER 0x10041770
int srMeshModel::testDirty(e_flags flag) const
{
    return (control_state_390 & (1 << (flag & 0x1f))) != 0;
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
