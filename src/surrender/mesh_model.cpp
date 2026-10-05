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
   render_control and dirty_flags bits. Retail .bss holds
   zero-initialized pointers here; no in-range provider code ever stores to
   them, so dump prints numeric bit indices. */
// GLOBAL: SURRENDER 0x100A4998
static const char* s_control_names;

// GLOBAL: SURRENDER 0x100A499C
static const char* s_flag_names0;

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
        shaders[pass] = shader;
    }
}

// FUNCTION: SURRENDER 0x10040400
srShader srMeshModel::getShader(long pass) const
{
    if (pass >= 0 && pass < 4) {
        return shaders[pass];
    }
    srShader shader;
    shader.value = 0x0100241b;
    return shader;
}

// FUNCTION: SURRENDER 0x10040330
srMaterialIFace* srMeshModel::getMaterial(long pass, e_side side) const
{
    if (pass >= 0 && pass < 4 && (int)side >= 0 && (int)side < 2) {
        return materials[pass][side];
    }
    return 0;
}

// FUNCTION: SURRENDER 0x100402E0
void srMeshModel::setMaterial(srMaterialIFace* material, long pass, e_side side)
{
    if (pass >= 0 && pass < 4 && (int)side >= 0 && (int)side < 2) {
        materials[pass][side] = material;
    }
}

// FUNCTION: SURRENDER 0x100403B0
srTextureIFace* srMeshModel::getTexture(long pass, long layer) const
{
    if (pass >= 0 && pass < 4 && layer >= 0 && layer < 2) {
        return textures[pass][layer];
    }
    return 0;
}

// FUNCTION: SURRENDER 0x10040360
void srMeshModel::setTexture(srTextureIFace* texture, long pass, long layer)
{
    if (pass >= 0 && pass < 4 && layer >= 0 && layer < 2) {
        textures[pass][layer] = texture;
    }
}

// FUNCTION: SURRENDER 0x10042800
long srMeshModel::getActivePolygonCount()
{
    return active_polygon_count;
}

// FUNCTION: SURRENDER 0x10042810
void srMeshModel::setActivePolygonCount(long count)
{
    active_polygon_count = count;
}

// FUNCTION: SURRENDER 0x10042820
void srMeshModel::setUVCount(long count)
{
    if (count < vertex_location_count) {
        count = vertex_location_count;
    }
    if (count != uv_count) {
        uv_count = count;
        MeshTable<srVector2T<float> >* table = &texcoords[0][0];
        for (long pass = 0; pass < 4; ++pass) {
            for (long side = 0; side < 2; ++side, ++table) {
                if (table->data != 0) {
                    unsigned long old_count = table->count;
                    unsigned long uv = uv_count;
                    srVector2T<float> empty(0.0f, 0.0f);
                    table->Resize(uv, 1);
                    if (uv != 0) {
                        for (unsigned long index = old_count; index < table->count; ++index) {
                            table->data[index] = empty;
                        }
                    }
                }
            }
        }
        setDirty(DIRTY_TRI_MESH);
    }
}

// FUNCTION: SURRENDER 0x10042980
long srMeshModel::getUVCount() const
{
    return uv_count;
}

/* The lazily grown table accessors below share one retail shape: a table is
   supplied only when its governing count is nonzero; on first use the pair
   reallocs through srHeap, copies the shorter extent when the element type
   preserves old data, and then clears or seeds the whole table. */
// FUNCTION: SURRENDER 0x1003FFD0
srVector3T<float>* srMeshModel::getVertexLoc()
{
    if (vertex_location_count == 0) {
        return 0;
    }
    if (vertex_locations.data == 0) {
        vertex_locations.Resize(vertex_location_count, 1);
        for (unsigned long index = 0; index < vertex_locations.count; ++index) {
            vertex_locations.data[index].SetZero();
        }
    }
    return vertex_locations.data;
}

// FUNCTION: SURRENDER 0x100400D0
srVector3T<float>* srMeshModel::getVertexNormal()
{
    if (vertex_location_count == 0) {
        return 0;
    }
    if (vertex_normals.data == 0) {
        vertex_normals.Resize(vertex_location_count, 0);
    }
    if ((dirty_flags.value & (1UL << DIRTY_VERTEX_NORMALS)) != 0) {
        calculateVertexNormals();
    }
    return vertex_normals.data;
}

// FUNCTION: SURRENDER 0x10040160
srVector4T<float>* srMeshModel::getPolyEq()
{
    if (polygon_count == 0) {
        return 0;
    }
    if (poly_equations.data == 0) {
        poly_equations.Resize(polygon_count, 1);
    }
    if ((dirty_flags.value & (1UL << DIRTY_POLYGON_NORMALS)) != 0) {
        calculatePolygonNormals();
    }
    return poly_equations.data;
}

// FUNCTION: SURRENDER 0x10040250
unsigned long* srMeshModel::getActivePolygonTable(int table)
{
    if (active_polygons.data == 0) {
        if (table != 0) {
            active_polygons.Resize(polygon_count, 0);
            for (long index = 0; index < polygon_count; ++index) {
                active_polygons.data[index] = index;
            }
        }
    }
    return active_polygons.data;
}

// FUNCTION: SURRENDER 0x1003FED0
srVector3i* srMeshModel::getPolyVertex()
{
    if (polygon_count == 0) {
        return 0;
    }
    if (poly_vertices.data == 0) {
        poly_vertices.Resize(polygon_count, 1);
        for (unsigned long index = 0; index < poly_vertices.count; ++index) {
            poly_vertices.data[index].x = 0;
            poly_vertices.data[index].y = 0;
            poly_vertices.data[index].z = 0;
        }
    }
    return poly_vertices.data;
}

// FUNCTION: SURRENDER 0x10040430
srPtr<srMaterialIFace>* srMeshModel::getVertexMaterial(long vertex, e_side side, int table)
{
    if (vertex < 0 || vertex > 3 || (int)side < 0 || (int)side > 1) {
        return 0;
    }
    MeshTable<srPtr<srMaterialIFace> >& slot = vertex_materials[vertex][side];
    if (slot.data == 0) {
        if (table != 0) {
            slot.Resize(vertex_location_count, 0);
            for (unsigned long index = 0; index < slot.count; ++index) {
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
    MeshTable<srPtr<srTextureIFace> >& slot = poly_textures[polygon][layer];
    if (slot.data == 0) {
        if (table != 0) {
            slot.Resize(polygon_count, 0);
            for (unsigned long index = 0; index < slot.count; ++index) {
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
    MeshTable<srShader>& slot = poly_shaders[polygon];
    if (slot.data == 0) {
        if (layer != 0) {
            slot.Resize(polygon_count, 0);
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
    MeshTable<srVector3i>& slot = poly_uv_indices[layer];
    if (slot.data == 0) {
        if (table != 0) {
            slot.Resize(polygon_count, 0);
            srVector3i* source = getPolyVertex();
            for (long index = 0; index < polygon_count; ++index) {
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
    MeshTable<srVector3T<float> >& slot = dig[vertex];
    if (slot.data == 0) {
        if (table != 0) {
            slot.Resize(vertex_location_count, 0);
            for (unsigned long index = 0; index < slot.count; ++index) {
                slot.data[index].SetZero();
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
    MeshTable<srVector2T<float> >& slot = texcoords[vertex][layer];
    if (slot.data == 0) {
        if (table != 0) {
            slot.Resize(uv_count, 1);
            for (unsigned long index = 0; index < slot.count; ++index) {
                slot.data[index].SetZero();
            }
        }
    }
    return slot.data;
}

// FUNCTION: SURRENDER 0x10040B80
unsigned long* srMeshModel::getVertexShadeIndex(int table)
{
    if (vertex_shade_indices.data == 0) {
        if (table != 0) {
            vertex_shade_indices.Resize(vertex_location_count, 1);
            for (unsigned long index = 0; index < vertex_shade_indices.count; ++index) {
                vertex_shade_indices.data[index] = 0;
            }
        }
    }
    return vertex_shade_indices.data;
}

// FUNCTION: SURRENDER 0x10041B60
void srMeshModel::setDirtyBounds()
{
    setDirty(DIRTY_BOUNDS);
}

// FUNCTION: SURRENDER 0x10041B90
void srMeshModel::setDirtyNormals()
{
    setDirty(DIRTY_POLYGON_NORMALS);
    setDirty(DIRTY_VERTEX_NORMALS);
}

// FUNCTION: SURRENDER 0x1003DEA0
void srMeshModel::freeAll()
{
    bounds_minimum.SetZero();
    bounds_maximum.SetZero();
    bounds_center.SetZero();
    bounds_radius = 0.0f;
    uv_count = 0;
    vertex_location_count = 0;
    polygon_count = 0;
    active_polygon_count = 0;
    setDirty(DIRTY_BOUNDS);
    setDirty(DIRTY_POLYGON_NORMALS);
    setDirty(DIRTY_VERTEX_NORMALS);
    setDirty(DIRTY_TRI_MESH);
    active_polygons.Release();
    poly_vertices.Release();
    poly_equations.Release();
    vertex_locations.Release();
    vertex_normals.Release();
    vertex_shade_indices.Release();
    for (long pass = 0; pass < 4; ++pass) {
        vertex_materials[pass][0].Release();
        vertex_materials[pass][1].Release();
        poly_textures[pass][0].Release();
        poly_textures[pass][1].Release();
        texcoords[pass][0].Release();
        texcoords[pass][1].Release();
        poly_uv_indices[pass].Release();
        poly_shaders[pass].Release();
        dig[pass].Release();
        dcg[pass].Release();
        scg[pass].Release();
    }
}

// FUNCTION: SURRENDER 0x1003CF30
srMeshModel::srMeshModel(long polygons, long vertices)
{
    memset(&tri_mesh, 0, sizeof(tri_mesh));
    reset(polygons, vertices);
    sort_bias = 0.0f;
    for (long pass = 0; pass < 4; ++pass) {
        materials[pass][0] = 0;
        materials[pass][1] = 0;
        textures[pass][0] = 0;
        textures[pass][1] = 0;
        shaders[pass] = srShader();
    }
}

// FUNCTION: SURRENDER 0x1003D2C0
void srMeshModel::reset(long polygons, long vertices)
{
    freeAll();
    vertex_location_count = vertices;
    uv_count = vertices;
    polygon_count = polygons;
    active_polygon_count = polygons;
    pass_count = 1;
    render_control.value = 0;
    render_control.value |= 1UL << CONTROL_FRONT;
    render_control.value |= 1UL << CONTROL_SKIP_AUTO_BOX;
}

// FUNCTION: SURRENDER 0x1003D320
srMeshModel::~srMeshModel()
{
    freeAll();
}

/* Deep copy: reset re-allocates the destination to the source's polygon and
   vertex counts, then every table, pass slot and scalar is copied over. The
   changed bit is set in dirty_flags after the state words transfer. */
// FUNCTION: SURRENDER 0x1003D5D0
srMeshModel& srMeshModel::operator=(const srMeshModel& other)
{
    if (this != &other) {
        srModel::operator=(other);
        reset(other.polygon_count, other.vertex_location_count);
        dirty_flags = other.dirty_flags;
        render_control = other.render_control;
        pass_count = other.pass_count;
        dirty_flags.value |= (1UL << DIRTY_TRI_MESH);
        bounds_minimum = other.bounds_minimum;
        bounds_maximum = other.bounds_maximum;
        bounds_center = other.bounds_center;
        bounds_radius = other.bounds_radius;
        active_polygon_count = other.active_polygon_count;
        active_polygons = other.active_polygons;
        poly_vertices = other.poly_vertices;
        poly_equations = other.poly_equations;
        vertex_locations = other.vertex_locations;
        vertex_normals = other.vertex_normals;
        vertex_shade_indices = other.vertex_shade_indices;
        for (long pass = 0; pass < 4; ++pass) {
            long side;
            for (side = 0; side < 2; ++side) {
                materials[pass][side] = other.materials[pass][side];
                vertex_materials[pass][side] = other.vertex_materials[pass][side];
            }
            for (side = 0; side < 2; ++side) {
                textures[pass][side] = other.textures[pass][side];
                poly_textures[pass][side] = other.poly_textures[pass][side];
                texcoords[pass][side] = other.texcoords[pass][side];
            }
            shaders[pass] = other.shaders[pass];
            poly_shaders[pass] = other.poly_shaders[pass];
            poly_uv_indices[pass] = other.poly_uv_indices[pass];
            dig[pass] = other.dig[pass];
            dcg[pass] = other.dcg[pass];
            scg[pass] = other.scg[pass];
        }
    }
    return *this;
}

// FUNCTION: SURRENDER 0x1003E120
void srMeshModel::calculateBounds()
{
    bounds_minimum.SetZero();
    bounds_maximum.SetZero();
    bounds_center.SetZero();
    bounds_radius = 0.0f;
    if (vertex_location_count != 0) {
        srVector3T<float>* vertices = getVertexLoc();
        if (vertex_location_count != 0) {
            srVectorProcessor::minMax(vertices, bounds_minimum, bounds_maximum,
                                      vertex_location_count);
        }
        bounds_center.x = (bounds_minimum.x + bounds_maximum.x) * 0.5;
        bounds_center.y = (bounds_minimum.y + bounds_maximum.y) * 0.5;
        bounds_center.z = (bounds_minimum.z + bounds_maximum.z) * 0.5;
        long count = vertex_location_count;
        if (0 < count) {
            do {
                float dy = vertices->y - bounds_center.y;
                float dz = vertices->z - bounds_center.z;
                float radius =
                    (vertices->x - bounds_center.x) * (vertices->x - bounds_center.x) +
                    dy * dy + dz * dz;
                if (bounds_radius < radius) {
                    bounds_radius = radius;
                }
                vertices = vertices + 1;
                count = count - 1;
            } while (count != 0);
        }
        bounds_radius = sqrtf(bounds_radius) * 1.00001f;
        updateAllClients(Client::UPDATE_BOUNDS);
        dirty_flags.value &= ~(1UL << DIRTY_BOUNDS);
    }
}

// FUNCTION: SURRENDER 0x1003E280
void srMeshModel::calculatePolygonNormals()
{
    dirty_flags.value &= ~(1UL << DIRTY_POLYGON_NORMALS);
    srVector4T<float>* equations = getPolyEq();
    srVector3i* polygons = getPolyVertex();
    srVector3T<float>* vertices = getVertexLoc();
    for (long polygon = 0; polygon < polygon_count; polygon++) {
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
    if ((polygon_count != 0) && (vertex_location_count != 0)) {
        if ((dirty_flags.value & (1UL << DIRTY_POLYGON_NORMALS)) != 0) {
            calculatePolygonNormals();
        }
        dirty_flags.value &= ~(1UL << DIRTY_VERTEX_NORMALS);
        srVector4T<float>* equations = getPolyEq();
        srVector3T<float>* normals = getVertexNormal();
        srVector3i* polygons = getPolyVertex();
        unsigned long* shade_indices = getVertexShadeIndex(0);
        if (shade_indices == 0) {
            fillConstant((unsigned long*)normals, 0, vertex_location_count * 3);
            for (long polygon = 0; polygon < polygon_count; polygon++) {
                normals[polygons[polygon].x].x += equations[polygon].x;
                normals[polygons[polygon].x].y += equations[polygon].y;
                normals[polygons[polygon].x].z += equations[polygon].z;
                normals[polygons[polygon].y].x += equations[polygon].x;
                normals[polygons[polygon].y].y += equations[polygon].y;
                normals[polygons[polygon].y].z += equations[polygon].z;
                normals[polygons[polygon].z].x += equations[polygon].x;
                normals[polygons[polygon].z].y += equations[polygon].y;
                normals[polygons[polygon].z].z += equations[polygon].z;
            }
        } else {
            srVector3T<float>* smooth =
                (srVector3T<float>*)srHeap.allocate(vertex_location_count * 0xc);
            long count = vertex_location_count * 3;
            if (count != 0) {
                srVectorProcessor::copy((SRDWORD*)smooth, 0, count);
            }
            for (long polygon = 0; polygon < polygon_count; polygon++) {
                smooth[shade_indices[polygons[polygon].x]].x += equations[polygon].x;
                smooth[shade_indices[polygons[polygon].x]].y += equations[polygon].y;
                smooth[shade_indices[polygons[polygon].x]].z += equations[polygon].z;
                smooth[shade_indices[polygons[polygon].y]].x += equations[polygon].x;
                smooth[shade_indices[polygons[polygon].y]].y += equations[polygon].y;
                smooth[shade_indices[polygons[polygon].y]].z += equations[polygon].z;
                smooth[shade_indices[polygons[polygon].z]].x += equations[polygon].x;
                smooth[shade_indices[polygons[polygon].z]].y += equations[polygon].y;
                smooth[shade_indices[polygons[polygon].z]].z += equations[polygon].z;
            }
            if (vertex_location_count != 0) {
                srVectorProcessor::copyIndexed(normals, smooth, shade_indices,
                                               vertex_location_count);
            }
            srHeap.free(smooth);
        }
        if (vertex_location_count != 0) {
            srVectorProcessor::normalize(normals, normals, 1.0f, vertex_location_count);
        }
    }
}

// FUNCTION: SURRENDER 0x1003E690
void srMeshModel::scale(const srVector3T<float>& scale)
{
    srVector3T<float>* vertices = getVertexLoc();
    for (long index = 0; index < vertex_location_count; index++) {
        vertices[index].x = vertices[index].x * scale.x;
        vertices[index].y = vertices[index].y * scale.y;
        vertices[index].z = scale.z * vertices[index].z;
    }
    setDirty(DIRTY_BOUNDS);
    setDirty(DIRTY_POLYGON_NORMALS);
    setDirty(DIRTY_VERTEX_NORMALS);
    setDirty(DIRTY_TRI_MESH);
}

// FUNCTION: SURRENDER 0x1003E780
void srMeshModel::applyMatrix(const srMatrix3T<float>& matrix)
{
    srVector3T<float>* vertices = getVertexLoc();
    for (long index = 0; index < vertex_location_count; index++) {
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
    setDirty(DIRTY_BOUNDS);
    setDirty(DIRTY_POLYGON_NORMALS);
    setDirty(DIRTY_VERTEX_NORMALS);
    setDirty(DIRTY_TRI_MESH);
}

// FUNCTION: SURRENDER 0x1003E8C0
void srMeshModel::relocateVertices(const srVector3T<float>& offset)
{
    srVector3T<float>* vertices = getVertexLoc();
    for (long index = 0; index < vertex_location_count; index++) {
        vertices[index].x = vertices[index].x + offset.x;
        vertices[index].y = vertices[index].y + offset.y;
        vertices[index].z = offset.z + vertices[index].z;
    }
    setDirty(DIRTY_BOUNDS);
    setDirty(DIRTY_POLYGON_NORMALS);
    setDirty(DIRTY_VERTEX_NORMALS);
    setDirty(DIRTY_TRI_MESH);
}

// FUNCTION: SURRENDER 0x1003E9B0
void srMeshModel::centerVertices()
{
    if (vertex_location_count != 0) {
        srVector3T<float> sum;
        sum.SetZero();
        srVector3T<float>* vertices = getVertexLoc();
        long count = vertex_location_count;
        if (0 < count) {
            for (long index = 0; index < count; index++) {
                sum += vertices[index];
            }
        }
        double inverse = 1.0 / count;
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
    if (vertex_location_count == 0) {
        return 0.0;
    }
    srVector3T<float>* vertices = getVertexLoc();
    long count = vertex_location_count;
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
    if (vertex_location_count != 0) {
        srVector3T<float>* vertices = getVertexLoc();
        long count = vertex_location_count;
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
    if (polygon_count != 0) {
        float factor = (float)(radius / getAverageRadius());
        srVector3T<float> scale;
        scale = factor;
        this->scale(scale);
    }
}

// FUNCTION: SURRENDER 0x1003EBF0
void srMeshModel::scaleToMaxRadius(double radius)
{
    if (polygon_count != 0) {
        float factor = (float)(radius / getMaxRadius());
        srVector3T<float> scale;
        scale = factor;
        this->scale(scale);
    }
}

// FUNCTION: SURRENDER 0x1003EC30
void srMeshModel::flipFaces()
{
    if (polygon_count != 0) {
        srVector3i* polygons = getPolyVertex();
        for (long polygon = 0; polygon < polygon_count; polygon++) {
            long first = polygons[polygon].x;
            polygons[polygon].x = polygons[polygon].y;
            polygons[polygon].y = first;
        }
        setDirty(DIRTY_BOUNDS);
        setDirty(DIRTY_POLYGON_NORMALS);
        setDirty(DIRTY_VERTEX_NORMALS);
        setDirty(DIRTY_TRI_MESH);
    }
}

// FUNCTION: SURRENDER 0x1003ED20
long srMeshModel::findClosestVertex(const srVector3T<float>& position)
{
    if (vertex_location_count != 0) {
        srVector3T<float>* vertices = getVertexLoc();
        long index = 1;
        long closest = 0;
        float minimum = (vertices[0].x - position.x) * (vertices[0].x - position.x) +
                        (vertices[0].y - position.y) * (vertices[0].y - position.y) +
                        (vertices[0].z - position.z) * (vertices[0].z - position.z);
        if (1 < vertex_location_count) {
            do {
                float x = vertices[index].x - position.x;
                float y = vertices[index].y - position.y;
                float z = vertices[index].z - position.z;
                if (x * x + y * y + z * z < minimum) {
                    minimum = x * x + y * y + z * z;
                    closest = index;
                }
                index = index + 1;
            } while (index < vertex_location_count);
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
    if ((dirty_flags.value & (1UL << DIRTY_TRI_MESH)) != 0) {
        updateTriMesh();
    }
    return tri_mesh;
}

// FUNCTION: SURRENDER 0x1003DC90
void srMeshModel::updateTriMesh()
{
    memset(&tri_mesh, 0, sizeof(TriMesh));
    tri_mesh.vertex_count = vertex_location_count;
    tri_mesh.polygon_count = polygon_count;
    tri_mesh.pass_count = pass_count;
    tri_mesh.control_flags = render_control.value;
    tri_mesh.sort_bias = sort_bias;
    tri_mesh.poly_vertices = getPolyVertex();
    tri_mesh.poly_equations = getPolyEq();
    tri_mesh.positions = getVertexLoc();
    tri_mesh.normals = getVertexNormal();
    for (long pass = 0; pass < 4; pass++) {
        for (long side = 0; side < 2; side++) {
            tri_mesh.materials[pass][side] =
                static_cast<srMaterial*>(materials[pass][side].get());
            tri_mesh.vertex_materials[pass][side] =
                getVertexMaterial(pass, static_cast<e_side>(side), 0);
        }
        for (long layer = 0; layer < 2; layer++) {
            tri_mesh.textures[pass][layer] = (srTextureIFace*)textures[pass][layer];
            tri_mesh.poly_textures[pass][layer] = getPolyTexture(pass, layer, 0);
            tri_mesh.texcoords[pass][layer] = getVertexTexCoords(pass, layer, 0);
        }
        tri_mesh.shaders[pass] = shaders[pass];
        tri_mesh.poly_uv[pass] = getPolyUVIndex(pass, 0);
        tri_mesh.poly_shaders[pass] = getPolyShader(pass, 0);
        tri_mesh.dig[pass] = getVertexDIG(pass, 0);
        tri_mesh.dcg[pass] = getVertexDCG(pass, 0);
        tri_mesh.scg[pass] = getVertexSCG(pass, 0);
    }
    if ((tri_mesh.control_flags & (1UL << srMeshModel::CONTROL_SKIP_AUTO_BOX)) == 0) {
        getBoundingBox(tri_mesh.bounds_minimum, tri_mesh.bounds_maximum);
    }
    if ((tri_mesh.control_flags & (1UL << srMeshModel::CONTROL_SKIP_AUTO_SPHERE)) == 0) {
        getBoundingSphere(tri_mesh.bounds_center, tri_mesh.bounds_radius);
    }
    tri_mesh.active_polygons = getActivePolygonTable(0);
    tri_mesh.active_polygon_count = active_polygon_count;
    dirty_flags.value &= ~(1UL << DIRTY_TRI_MESH);
}

// FUNCTION: SURRENDER 0x1003FDA0
void srMeshModel::setBounds(const srVector3T<float>& minimum, const srVector3T<float>& maximum,
                            const srVector3T<float>& center, float radius)
{
    bounds_minimum = minimum;
    bounds_maximum = maximum;
    bounds_center = center;
    bounds_radius = radius;
    clearDirty(DIRTY_BOUNDS);
    updateAllClients(Client::UPDATE_BOUNDS);
    setDirty(DIRTY_TRI_MESH);
}

// FUNCTION: SURRENDER 0x1003FE40
int srMeshModel::getBoundingBox(srVector3T<float>& minimum, srVector3T<float>& maximum)
{
    if ((dirty_flags.value & (1UL << DIRTY_BOUNDS)) != 0) {
        calculateBounds();
    }
    minimum = bounds_minimum;
    maximum = bounds_maximum;
    return 1;
}

// FUNCTION: SURRENDER 0x1003FE90
int srMeshModel::getBoundingSphere(srVector3T<float>& center, float& radius)
{
    if ((dirty_flags.value & (1UL << DIRTY_BOUNDS)) != 0) {
        calculateBounds();
    }
    center = bounds_center;
    radius = bounds_radius;
    return 1;
}

// FUNCTION: SURRENDER 0x100408B0
srVector4T<float>* srMeshModel::getVertexSCG(long vertex, int table)
{
    if (vertex < 0 || vertex > 3) {
        return 0;
    }
    MeshTable<srVector4T<float> >& slot = scg[vertex];
    if (slot.data == 0) {
        if (table != 0) {
            slot.Resize(vertex_location_count, 0);
            for (unsigned long index = 0; index < slot.count; ++index) {
                slot.data[index] = 1.0f;
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
    MeshTable<srVector4T<float> >& slot = dcg[vertex];
    if (slot.data == 0) {
        if (table != 0) {
            slot.Resize(vertex_location_count, 0);
            for (unsigned long index = 0; index < slot.count; ++index) {
                slot.data[index] = 1.0f;
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
    if ((render_control.value & (1UL << CONTROL_SKIP_AUTO_SPHERE)) == 0) {
        srVector3T<float> center;
        float radius;
        getBoundingSphere(center, radius);
        if (renderer.testBoundingSphere(center, radius) == srGERD::VISIBILITY_OUTSIDE) {
            return;
        }
    }
    if ((render_control.value & (1UL << CONTROL_SKIP_AUTO_BOX)) == 0 &&
        vertex_location_count >= 8) {
        srVector3T<float> minimum;
        srVector3T<float> maximum;
        getBoundingBox(minimum, maximum);
        if (renderer.testBoundingBox(minimum, maximum) == srGERD::VISIBILITY_OUTSIDE) {
            return;
        }
    }
    renderTriMesh(renderer, getTriMesh());
}

// FUNCTION: SURRENDER 0x100418C0
void srMeshModel::reindexPolygons(const unsigned long* indices)
{
    long index;
    long pass;
    if (polygon_count != 0) {
        permuteTable(getPolyEq(), indices, polygon_count);
        permuteTable(getPolyVertex(), indices, polygon_count);
        for (pass = 0; pass < pass_count; ++pass) {
            if (getPolyUVIndex(pass, 0) != 0) {
                permuteTable(getPolyUVIndex(pass, 0), indices, polygon_count);
            }
            if (getPolyShader(pass, 0) != 0) {
                permuteObjects(getPolyShader(pass, 0), indices, polygon_count);
            }
            for (long layer = 0; layer < 2; ++layer) {
                if (getPolyTexture(pass, layer, 0) != 0) {
                    permuteObjects(getPolyTexture(pass, layer, 0), indices, polygon_count);
                }
            }
        }
        if (active_polygon_count != 0 && getActivePolygonTable(0) != 0) {
            unsigned long* table = getActivePolygonTable(1);
            unsigned long* forward = new unsigned long[polygon_count];
            for (index = 0; index < polygon_count; ++index) {
                forward[indices[index]] = index;
            }
            for (index = 0; index < active_polygon_count; ++index) {
                table[index] = forward[table[index]];
            }
            delete[] forward;
        }
        setDirty(DIRTY_BOUNDS);
        setDirty(DIRTY_POLYGON_NORMALS);
        setDirty(DIRTY_VERTEX_NORMALS);
        setDirty(DIRTY_TRI_MESH);
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
    if (vertex_location_count != 0) {
        unsigned long* forward = new unsigned long[vertex_location_count];
        for (index = 0; index < vertex_location_count; ++index) {
            forward[indices[index]] = index;
        }
        permuteTable(getVertexNormal(), indices, vertex_location_count);
        permuteTable(getVertexLoc(), indices, vertex_location_count);
        unsigned long* shade = getVertexShadeIndex(0);
        if (shade != 0) {
            unsigned long* copy = new unsigned long[vertex_location_count];
            if (vertex_location_count != 0 && copy != shade) {
                srVectorProcessor::memcopy(copy, shade, vertex_location_count * 4);
            }
            for (index = 0; index < vertex_location_count; ++index) {
                shade[index] = forward[copy[indices[index]]];
            }
            for (index = 0; index < vertex_location_count; ++index) {
                unsigned long value = shade[index];
                if (value < (unsigned long)index) {
                    shade[index] = shade[value];
                    shade[value] = value;
                }
            }
            delete[] copy;
        }
        for (pass = 0; pass < pass_count; ++pass) {
            for (side = 0; side < 2; ++side) {
                if (getVertexMaterial(pass, static_cast<e_side>(side), 0) != 0) {
                    permuteObjects(getVertexMaterial(pass, static_cast<e_side>(side), 0), indices,
                                   vertex_location_count);
                }
            }
            for (table = 0; table < 2; ++table) {
                if (getVertexTexCoords(pass, table, 0) != 0) {
                    permuteTable(getVertexTexCoords(pass, table, 0), indices,
                                 vertex_location_count);
                }
            }
            if (getVertexDIG(pass, 0) != 0) {
                permuteTable(getVertexDIG(pass, 0), indices, vertex_location_count);
            }
            if (getVertexSCG(pass, 0) != 0) {
                permuteTable(getVertexSCG(pass, 0), indices, vertex_location_count);
            }
            if (getVertexDCG(pass, 0) != 0) {
                permuteTable(getVertexDCG(pass, 0), indices, vertex_location_count);
            }
            if (getPolyUVIndex(pass, 0) != 0) {
                srVector3i* uv = getPolyUVIndex(pass, 1);
                for (polygon = 0; polygon < polygon_count; ++polygon, ++uv) {
                    int* corner = &uv->x;
                    for (component = 0; component < 3; ++component, ++corner) {
                        int index = *corner;
                        if (index < vertex_location_count) {
                            index = forward[index];
                        }
                        *corner = index;
                    }
                }
            }
        }
        srVector3i* vertices = getPolyVertex();
        for (polygon = 0; polygon < polygon_count; ++polygon, ++vertices) {
            int* corner = &vertices->x;
            for (component = 0; component < 3; ++component, ++corner) {
                *corner = forward[*corner];
            }
        }
        setDirty(DIRTY_BOUNDS);
        setDirty(DIRTY_POLYGON_NORMALS);
        setDirty(DIRTY_VERTEX_NORMALS);
        setDirty(DIRTY_TRI_MESH);
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
    if (t.vertex_count <= 0) {
        srAssertFail("t.vnum > 0", "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x5cf, 0);
    }
    if (t.polygon_count <= 0) {
        srAssertFail("t.pnum > 0", "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x5d0, 0);
    }
    if (t.pass_count > 0 && t.pass_count <= MAX_PASSES) {
        if (!srFinite(t.sort_bias)) {
            srAssertFail("srFinite(t.sortBias)", "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp",
                         0x5d2, 0);
        }
        if (t.active_polygons != 0) {
            if (t.polygon_count < t.active_polygon_count) {
                srAssertFail("t.aPnum <= t.pnum", "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp",
                             0x5da, 0);
            }
            for (i = 0; i < t.active_polygon_count; ++i) {
                /* c-style-cast-ok: the assert text spells (SRDWORD)(t.pnum). */
                if ((SRDWORD)t.polygon_count <= t.active_polygons[i]) {
                    srAssertFail("t.APT[i] < (SRDWORD)(t.pnum)",
                                 "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x5dd, 0);
                }
            }
            for (i = 0; i < t.active_polygon_count; ++i) {
                for (j = i + 1; j < t.active_polygon_count; ++j) {
                    if (t.active_polygons[i] == t.active_polygons[j]) {
                        srAssertFail("t.APT[i] != t.APT[j]",
                                     "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x5e1, 0);
                    }
                }
            }
        }
        if (t.poly_vertices == 0) {
            srAssertFail("t.pVertex", "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x5e5, 0);
        }
        for (i = 0; i < t.polygon_count; ++i) {
            int* component = &t.poly_vertices[i].x;
            for (j = 0; j < 3; ++j, ++component) {
                if (*component < 0 || t.vertex_count <= *component) {
                    srAssertFail("t.pVertex[i][j] >= 0 && t.pVertex[i][j] < t.vnum",
                                 "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x5ed, 0);
                }
            }
        }
        if (t.poly_equations == 0) {
            srAssertFail("t.pEq", "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x5f1, 0);
        }
        for (i = 0; i < t.polygon_count; ++i) {
            if (!t.poly_equations[i].isValid()) {
                srAssertFail("t.pEq[i].isValid()", "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp",
                             0x5f6, 0);
            }
        }
        if (t.positions == 0) {
            srAssertFail("t.vLoc", "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x5fa, 0);
        }
        for (i = 0; i < t.vertex_count; ++i) {
            if (!t.positions[i].isValid()) {
                srAssertFail("t.vLoc[i].isValid()",
                             "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x5fe, 0);
            }
        }
        if (t.normals == 0) {
            srAssertFail("t.vNorm", "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x602, 0);
        }
        for (i = 0; i < t.vertex_count; ++i) {
            if (fabs(t.normals[i].length() - 1.0) >= 0.01) {
                srAssertFail("fabs(t.vNorm[i].length()-1.0) < 0.01",
                             "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x606, 0);
            }
        }
        for (p = 0; p < pass_count; ++p) {
            if (t.dig[p] != 0) {
                for (i = 0; i < t.vertex_count; ++i) {
                    if (!t.dig[p][i].isValid()) {
                        srAssertFail("t.DIG[p][i].isValid()",
                                     "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x611, 0);
                    }
                }
            }
            if (t.dcg[p] != 0) {
                for (i = 0; i < t.vertex_count; ++i) {
                    if (!t.dcg[p][i].isValid()) {
                        srAssertFail("t.DCG[p][i].isValid()",
                                     "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x615, 0);
                    }
                }
            }
            if (t.scg[p] != 0) {
                for (i = 0; i < t.vertex_count; ++i) {
                    if (!t.scg[p][i].isValid()) {
                        srAssertFail("t.SCG[p][i].isValid()",
                                     "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x619, 0);
                    }
                }
            }
            for (j = 0; j < 2; ++j) {
                if (t.texcoords[p][j] != 0) {
                    for (i = 0; i < t.vertex_count; ++i) {
                        if (!t.texcoords[p][j][i].isValid()) {
                            srAssertFail("t.vUV[p][j][i].isValid()",
                                         "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x620,
                                         0);
                        }
                    }
                }
                if (t.poly_textures[p][j] != 0) {
                    for (i = 0; i < t.polygon_count; ++i) {
                        if (!t.poly_textures[p][j][i]) {
                            srAssertFail("t.pTexture[p][j][i]",
                                         "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x625,
                                         0);
                        }
                    }
                }
            }
            for (s = 0; s < 2; ++s) {
                if (t.vertex_materials[p][s] != 0) {
                    for (v = 0; v < t.vertex_count; ++v) {
                        if (!t.vertex_materials[p][s][v]) {
                            srAssertFail("t.vMaterial[p][s][v]",
                                         "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x62f,
                                         0);
                        }
                    }
                }
            }
            if (t.poly_shaders[p] == 0) {
                if (!t.shaders[p].isValid()) {
                    srAssertFail("t.shader[p].isValid()",
                                 "D:\\srsdk1x\\sources\\corelib\\srMeshModel.cpp", 0x639, 0);
                }
            } else {
                for (v = 0; v < t.polygon_count; ++v) {
                    if (!t.poly_shaders[p][v].isValid()) {
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
    stream << "  Polygons: " << polygon_count << '\n';
    stream.width(0x20);
    stream << "  Vertices: " << vertex_location_count << '\n';
    stream.width(0x20);
    stream << "  Passes: " << pass_count << '\n';
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
    stream << "  Sort bias: " << sort_bias << '\n';
    stream.width(0x20);
    stream << "  Flags: ";
    if (dirty_flags.value == 0) {
        stream << "[NONE]";
    } else {
        stream << '[';
        bool first = true;
        const char* names = s_flag_names0;
        const char* name = names;
        for (unsigned long bit = 0; bit < 0x20; ++bit) {
            if ((dirty_flags.value & (1 << bit)) == 0) {
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
    if (render_control.value == 0) {
        stream << "[NONE]";
    } else {
        stream << '[';
        bool first = true;
        const char* names = s_control_names;
        const char* name = names;
        for (unsigned long bit = 0; bit < 0x20; ++bit) {
            if ((render_control.value & (1 << bit)) == 0) {
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
        bool defined = (this->materials[pass][0] != 0) || (this->materials[pass][1] != 0);
        for (stage = 0; stage < 2; ++stage) {
            if (this->textures[pass][stage] != 0) {
                defined = true;
            }
        }
        if (defined) {
            stream << "    Pass " << pass << '\n';
        }
        if (this->materials[pass][0] != 0) {
            stream.width(0x20);
            stream << "      Front material: " << this->materials[pass][0]->getName() << '\n';
        }
        if (this->materials[pass][1] != 0) {
            stream.width(0x20);
            stream << "      Back material: " << this->materials[pass][1]->getName() << '\n';
        }
        if (defined) {
            stream.width(0x20);
            stream << "      Shader: " << this->shaders[pass] << '\n';
        }
        for (stage = 0; stage < 2; ++stage) {
            if (this->textures[pass][stage] != 0) {
                char label[36];
                sprintf(label, "      Texture %d", stage);
                stream.width(0x20);
                stream << label << this->textures[pass][stage]->getName() << '\n';
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
        srPtr<srMaterialIFace>* front = getVertexMaterial(pass, SIDE_FRONT, 0);
        srPtr<srMaterialIFace>* back = getVertexMaterial(pass, SIDE_BACK, 0);
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
    for (pass = 0; pass < pass_count; ++pass) {
        for (polygon = 1; polygon < polygon_count; ++polygon) {
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
    for (pass = 0; pass < pass_count; ++pass) {
        if (getPolyShader(pass, 0) != 0) {
            srShader* shaders = getPolyShader(pass, 1);
            changes = 1;
            for (polygon = 1; polygon < polygon_count; ++polygon) {
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
                for (polygon = 1; polygon < polygon_count; ++polygon) {
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
                for (vertex = 1; vertex < vertex_location_count; ++vertex) {
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

/* Provider pipeline ownership remains provisional; linked address order does
   not establish original TU placement. */

// GLOBAL: SURRENDER 0x100A4790
srTriMeshPipeline* srTriMeshPipeline::pipe = 0;

srTriMeshPipeline::srTriMeshPipeline()
{
    flags = 0;
    vertex_pipe = new srVertexPipe();
    flushing = 0;
    Reset(0);
    Flush();
}

// FUNCTION: SURRENDER 0x100440A0
srTriMeshPipeline::~srTriMeshPipeline()
{
    while (flushing != 0) {
    }

    delete vertex_pipe;
}

// FUNCTION: SURRENDER 0x10044070
void srTriMeshPipeline::SetFlags(srShader shader)
{
    this->shader = shader;
    current_pass->shader = shader;
}

/* Bind a renderer and rebuild the current slot through PrepareSlot. */
// FUNCTION: SURRENDER 0x100441A0
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

// FUNCTION: SURRENDER 0x100442B0
void srTriMeshPipeline::Flush()
{
    flushing = 1;
    if (slot_count > 0) {
        FlushSlots();
    }
    flushing = 0;
}

/* Point current_record / current_pass at slot slot_count, growing
   either table by (capacity + slot + 8) when needed. */
// FUNCTION: SURRENDER 0x100442E0
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

// FUNCTION: SURRENDER 0x100443A0
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
    srCore.getStatisticsManager()->statistics.vertices_submitted +=
        slot_count * vertex_count;

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
            renderer->allocVertexArray(vertex_arrays[0],
                                       slot_count * culler_output.vertex_count);

            /* Retail grows and re-reads the member array through
               vertex_arrays[slot] on the left while the right side keeps
               the vertex_arrays snapshot taken before the loop. */
            for (unsigned long slot = 1; slot < slot_count; ++slot) {
                unsigned long offset = slot * culler_output.vertex_count;
                this->vertex_arrays[slot].eye_locations =
                    vertex_arrays[0].eye_locations + offset;
                this->vertex_arrays[slot].diffuse = vertex_arrays[0].diffuse + offset;
                this->vertex_arrays[slot].specular = vertex_arrays[0].specular + offset;
                this->vertex_arrays[slot].st0 = vertex_arrays[0].st0 + offset;
                this->vertex_arrays[slot].st1 = vertex_arrays[0].st1 + offset;
                this->vertex_arrays[slot].q0 = vertex_arrays[0].q0 + offset;
                this->vertex_arrays[slot].q1 = vertex_arrays[0].q1 + offset;
                this->vertex_arrays[slot].attributes = vertex_arrays[0].attributes + offset;
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
// FUNCTION: SURRENDER 0x10043E00
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

/* Provider-side renderTriMesh: the consumer's stMeshModel override extends
   this same shape with its software-cull and inverted-depth paths. */
// FUNCTION: SURRENDER 0x1003CA80
void srMeshModel::renderTriMesh(srGERD& renderer, const TriMesh& mesh)
{
    if (mesh.polygon_count != 0 && mesh.vertex_count != 0) {
        renderer.pushEnable();

        if ((mesh.control_flags & (1UL << srMeshModel::CONTROL_SORTED_RENDERING)) != 0 &&
            !renderer.isEnabled(srGERD::ENABLE_SORTED_RENDERING)) {
            renderer.toggle(srGERD::ENABLE_SORTED_RENDERING);
        }

        long material_side;
        for (long side = 1; side >= 0; --side) {
            if ((mesh.control_flags & (1u << side)) != 0) {
                srTriMeshPipeline* pipeline = srTriMeshPipeline::Get(&renderer);
                pipeline->sort_bias = mesh.sort_bias;
                pipeline->triangles = mesh.poly_vertices;
                pipeline->triangle_count = static_cast<unsigned long>(mesh.polygon_count);
                pipeline->positions = mesh.positions;
                pipeline->vertex_count = static_cast<unsigned long>(mesh.vertex_count);
                pipeline->vertex_extras = mesh.normals;
                pipeline->projected_vertices = mesh.poly_equations;

                if (mesh.active_polygons != 0) {
                    pipeline->active_triangles = mesh.active_polygons;
                    pipeline->active_triangle_count =
                        static_cast<unsigned long>(mesh.active_polygon_count);
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

                material_side = side;
                if (side == 1) {
                    renderer.setCullMode(srGERD::CULL_FRONT);
                    if (!renderer.isEnabled(srGERD::ENABLE_REVERSE_NORMALS)) {
                        renderer.toggle(srGERD::ENABLE_REVERSE_NORMALS);
                    }
                    if ((mesh.control_flags & (1UL << srMeshModel::CONTROL_REUSE_FRONT_MATERIAL)) !=
                        0) {
                        material_side = 0;
                    }
                } else {
                    if ((mesh.control_flags & (1UL << srMeshModel::CONTROL_NO_FRONT_CULL)) == 0) {
                        renderer.setCullMode(srGERD::CULL_BACK);
                    } else {
                        renderer.setCullMode(srGERD::CULL_NONE);
                    }
                    if (renderer.isEnabled(srGERD::ENABLE_REVERSE_NORMALS)) {
                        renderer.toggle(srGERD::ENABLE_REVERSE_NORMALS);
                    }
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

                    if (mesh.vertex_materials[pass][material_side] == 0) {
                        srMaterialIFace* material = mesh.materials[pass][material_side];
                        pipeline->material = material;
                        pipeline->current_record->material = material;
                    } else {
                        pipeline->current_record->vertex_materials =
                            mesh.vertex_materials[pass][material_side];
                        pipeline->current_record->flags |=
                            srVertexPipe::Record::HAS_VERTEX_MATERIALS;
                    }

                    if (mesh.poly_uv[pass] != 0) {
                        pipeline->current_pass->poly_uv = mesh.poly_uv[pass];
                    }

                    if (mesh.poly_shaders[pass] == 0) {
                        pipeline->SetFlags(mesh.shaders[pass]);
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

                    for (long layer = 0; layer < 2; ++layer) {
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
    }
}

// FUNCTION: SURRENDER 0x100417D0
float srMeshModel::getSortBias() const
{
    return sort_bias;
}

// FUNCTION: SURRENDER 0x100417E0
void srMeshModel::disable(e_control control)
{
    render_control.set(control, 0);
    setDirty(DIRTY_TRI_MESH);
}

// FUNCTION: SURRENDER 0x10041870
int srMeshModel::isEnabled(e_control control) const
{
    return (render_control.value & (1 << control)) != 0;
}

// FUNCTION: SURRENDER 0x10041890
void srMeshModel::setPassCount(long count)
{
    pass_count = count;
    if (count < 1) {
        pass_count = 1;
        return;
    }
    if (count > MAX_PASSES) {
        pass_count = MAX_PASSES;
    }
}

// FUNCTION: SURRENDER 0x10041AC0
long srMeshModel::getPassCount() const
{
    return pass_count;
}

// FUNCTION: SURRENDER 0x10041AD0
long srMeshModel::getPolygonCount() const
{
    return polygon_count;
}

// FUNCTION: SURRENDER 0x10041AE0
long srMeshModel::getVertexCount() const
{
    return vertex_location_count;
}
