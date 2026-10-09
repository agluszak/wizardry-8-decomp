#pragma once

#include <new>

#include "srHeap.h"
#include "srFlags.h"
#include "srMaterial.h"
#include "srMath.h"
#include "srModel.h"
#include "srPtr.h"
#include "srShader.h"
#include "srTexture.h"
#include "srTypeRegistry.h"

// VTABLE: SURRENDER 0x10076D9C
// class srClassSupport<srMeshModel, srModel, 0, 8208>

// VTABLE: SURRENDER 0x10076D48 srMeshModel
class SR_DLL_EXPORT srMeshModel : public srClassSupport<srMeshModel, srModel, 0, 0x2010> {
public:
    /* Front/back table indices: dump labels materials[pass][0/1] and
       renderTriMesh uses the corresponding CONTROL_FRONT/BACK bits. */
    enum e_side { SIDE_FRONT = 0, SIDE_BACK = 1 };
    /* Bit indices into dirty_flags. calculateBounds, calculatePolygonNormals,
       calculateVertexNormals and updateTriMesh clear bits 0, 1, 2 and 3.
       Raising the bounds bit notifies model clients. */
    enum e_flags {
        DIRTY_BOUNDS = 0,
        DIRTY_POLYGON_NORMALS = 1,
        DIRTY_VERTEX_NORMALS = 2,
        DIRTY_TRI_MESH = 3
    };
    /* Bit indices into render_control. renderTriMesh tests bits 0/1 as
       front/back sides. updateTriMesh skips auto box when bit 4 is set and
       auto sphere when bit 5 is set. */
    enum e_control {
        CONTROL_FRONT = 0,
        CONTROL_BACK = 1,
        CONTROL_REUSE_FRONT_MATERIAL = 2,
        CONTROL_NO_FRONT_CULL = 3,
        CONTROL_SKIP_AUTO_BOX = 4,
        CONTROL_SKIP_AUTO_SPHERE = 5,
        CONTROL_SORTED_RENDERING = 6
    };
    /* The four per-pass table slots cap t.passes, as verify() asserts. */
    enum { MAX_PASSES = 4 };
    /* Detached triangle-mesh view: updateTriMesh fills it from the live tables, getTriMesh copies
       or returns it and renderTriMesh feeds srTriMeshPipeline from it. */
    struct TriMesh {
        TriMesh() : control_flags(0) {}

        w8_long vertex_count;
        w8_long polygon_count;
        w8_long pass_count;
        w8_ulong control_flags;
        srVector3i* poly_vertices;
        srVector4T<float>* poly_equations;
        srVector2T<float>* texcoords[4][2];
        srVector3T<float>* positions;
        srVector3T<float>* normals;
        srVector3T<float>* dig[4];
        srVector4T<float>* dcg[4];
        srVector4T<float>* scg[4];
        srMaterial* materials[4][2];
        srTextureIFace* textures[4][2];
        srShader shaders[4];
        srPtr<srMaterialIFace>* vertex_materials[4][2];
        srPtr<srTextureIFace>* poly_textures[4][2];
        srShader* poly_shaders[4];
        srVector3i* poly_uv[4];
        srVector3T<float> bounds_minimum;
        srVector3T<float> bounds_maximum;
        srVector3T<float> bounds_center;
        float bounds_radius;
        float sort_bias;
        w8_ulong* active_polygons;
        w8_long active_polygon_count;
    };

    SR_DLL_IMPORT srMeshModel(w8_long polygons = 0, w8_long vertices = 0);

    SR_DLL_IMPORT void reset(w8_long polygons, w8_long vertices);
    SR_DLL_IMPORT void scale(const srVector3T<float>& scale);
    void applyMatrix(const srMatrix3T<float>& matrix);
    SR_DLL_IMPORT void relocateVertices(const srVector3T<float>& offset);
    void centerVertices();
    double getAverageRadius();
    double getMaxRadius();
    void scaleToAverageRadius(double radius);
    void scaleToMaxRadius(double radius);
    void flipFaces();
    w8_long findClosestVertex(const srVector3T<float>& point);
    SR_DLL_IMPORT srMeshModel& operator=(const srMeshModel& other);

    // FUNCTION: SURRENDER 0x10041AF0
    static const char* sGetClassName()
    {
        return "srMeshModel";
    }

    SR_DLL_IMPORT virtual void dump(std::ostream& stream) override;
    SR_DLL_IMPORT virtual void verify(srRuntimeClass::e_verify mode) override;
    SR_DLL_IMPORT virtual srClass* vInstance() override;
    SR_DLL_IMPORT virtual int getBoundingSphere(srVector3T<float>& center, float& radius) override;
    SR_DLL_IMPORT virtual int getBoundingBox(srVector3T<float>& minimum,
                                             srVector3T<float>& maximum) override;
    SR_DLL_IMPORT virtual void render(class srGERD& renderer) override;
    SR_DLL_IMPORT virtual void reindexPolygons(const w8_ulong* indices);
    SR_DLL_IMPORT virtual void reindexVertices(const w8_ulong* indices);
    SR_DLL_IMPORT virtual void getTriMesh(TriMesh& mesh);
    SR_DLL_IMPORT virtual const TriMesh& getTriMesh();
    SR_DLL_IMPORT virtual void renderTriMesh(class srGERD& renderer, const TriMesh& mesh);
    SR_DLL_IMPORT srPtr<srTextureIFace>* getPolyTexture(w8_long polygon, w8_long layer, int table);
    SR_DLL_IMPORT srVector3i* getPolyVertex();
    SR_DLL_IMPORT srVector3i* getPolyUVIndex(w8_long layer, int table);
    SR_DLL_IMPORT srVector2T<float>* getVertexTexCoords(w8_long vertex, w8_long layer, int table);
    SR_DLL_IMPORT srPtr<srMaterialIFace>* getVertexMaterial(w8_long vertex, e_side side, int table);
    SR_DLL_IMPORT w8_ulong* getVertexShadeIndex(int table);
    SR_DLL_IMPORT srVector3T<float>* getVertexNormal();
    SR_DLL_IMPORT srVector4T<float>* getPolyEq();
    SR_DLL_IMPORT srVector3T<float>* getVertexDIG(w8_long vertex, int table);
    srVector4T<float>* getVertexSCG(w8_long vertex, int table);
    srVector4T<float>* getVertexDCG(w8_long vertex, int table);
    SR_DLL_IMPORT srMaterialIFace* getMaterial(w8_long polygon, e_side side) const;
    SR_DLL_IMPORT srTextureIFace* getTexture(w8_long polygon, w8_long layer) const;
    SR_DLL_IMPORT void setMaterial(srMaterialIFace* material, w8_long polygon, e_side side);
    SR_DLL_IMPORT void setTexture(srTextureIFace* texture, w8_long polygon, w8_long layer);
    // FUNCTION: SURRENDER 0x10041710 SYMBOL
    // RECOMP: ?setDirty@srMeshModel@@QAEXW4e_flags@1@@Z
    void setDirty(e_flags flag)
    {
        w8_ulong mask = 1 << flag;
        if ((dirty_flags.value & mask) == 0) {
            dirty_flags.set(flag, 1);
            dirty_flags.set(DIRTY_TRI_MESH, 1);
            if (flag == DIRTY_BOUNDS) {
                updateAllClients(Client::UPDATE_BOUNDS);
            }
        }
    }
    // FUNCTION: SURRENDER 0x10041750 SYMBOL
    // RECOMP: ?clearDirty@srMeshModel@@QAEXW4e_flags@1@@Z
    void clearDirty(e_flags flag)
    {
        dirty_flags.set(flag, 0);
    }
    // FUNCTION: SURRENDER 0x10041770 SYMBOL
    // RECOMP: ?testDirty@srMeshModel@@QBEHW4e_flags@1@@Z
    int testDirty(e_flags flag) const
    {
        return (dirty_flags.value & (1 << flag)) != 0;
    }
    SR_DLL_IMPORT srShader* getPolyShader(w8_long polygon, int layer);
    SR_DLL_IMPORT srShader getShader(w8_long polygon) const;
    SR_DLL_IMPORT void setShader(srShader shader, w8_long pass);
    SR_DLL_IMPORT void setUVCount(w8_long count);
    w8_long getUVCount() const;
    SR_DLL_IMPORT void setActivePolygonCount(w8_long count);
    SR_DLL_IMPORT w8_long getActivePolygonCount();
    w8_long getPassCount() const;
    w8_long getPolygonCount() const;
    w8_long getVertexCount() const;
    void setPassCount(w8_long count);
    // FUNCTION: SURRENDER 0x10041790
    void setSortBias(float bias)
    {
        sort_bias = bias;
        setDirty(DIRTY_TRI_MESH);
    }
    float getSortBias() const;
    void disable(e_control control);
    // FUNCTION: SURRENDER 0x10041830
    void enable(e_control control)
    {
        render_control.set(control, 1);
        setDirty(DIRTY_TRI_MESH);
    }
    int isEnabled(e_control control) const;
    // FUNCTION: SURRENDER 0x10041660
    void setDirtyAll()
    {
        setDirty(DIRTY_BOUNDS);
        setDirty(DIRTY_POLYGON_NORMALS);
        setDirty(DIRTY_VERTEX_NORMALS);
        setDirty(DIRTY_TRI_MESH);
    }
    void setDirtyBounds();
    void setDirtyNormals();
    SR_DLL_IMPORT w8_ulong* getActivePolygonTable(int table);
    SR_DLL_IMPORT srVector3T<float>* getVertexLoc();
    /* Fills the cached AABB/sphere (bounds_minimum..bounds_radius)
       from the supplied box and center/radius. */
    SR_DLL_IMPORT void setBounds(const srVector3T<float>& minimum, const srVector3T<float>& maximum,
                                 const srVector3T<float>& center, float radius);

protected:
    SR_DLL_IMPORT virtual ~srMeshModel() override;
    void freeAll();
    SR_DLL_IMPORT virtual void updateTriMesh();
    SR_DLL_IMPORT virtual void calculateBounds();
    SR_DLL_IMPORT virtual void calculatePolygonNormals();
    SR_DLL_IMPORT virtual void calculateVertexNormals();

public:
    /* setMaterial indexes [pass][side]. */
    srPtr<srMaterialIFace> materials[4][2];
    srPtr<srTextureIFace> textures[4][2];
    srShader shaders[4];
    /* Lazily grown mesh table; every table accessor resizes it to its governing count on first use. */
    template <class T> struct MeshTable {
        MeshTable() : data(0), count(0) {}
        MeshTable(const MeshTable& other) : data(0), count(0)
        {
            *this = other;
        }
        ~MeshTable()
        {
            Release();
        }
        MeshTable& operator=(const MeshTable& other)
        {
            if (this != &other) {
                Release();
                if (other.count != 0) {
                    Resize(other.count, 1);
                    Copy(data, other.data, count);
                }
            }
            return *this;
        }

        T* Allocate(w8_ulong elements)
        {
            T* replacement = static_cast<T*>(srHeap.allocate(elements * sizeof(T)));
            for (w8_ulong index = 0; index < elements; ++index) {
                new (&replacement[index]) T;
            }
            return replacement;
        }

        /* Release each element, free the allocation, and zero the pair. */
        void Release()
        {
            if (data != 0) {
                for (w8_ulong index = 0; index < count; ++index) {
                    data[index].~T();
                }
                srHeap.free(data);
            }
            data = 0;
            count = 0;
        }

        /* Fresh storage, a min(old,new) prefix copy when preserve is set, then the old table's
           Release(). */
        void Resize(w8_ulong elements, int preserve)
        {
            if (count != elements) {
                if (elements == 0) {
                    Release();
                    return;
                }
                T* replacement = Allocate(elements);
                if (data != 0 && count != 0 && preserve != 0) {
                    Copy(replacement, data, elements < count ? elements : count);
                }
                Release();
                data = replacement;
                count = elements;
            }
        }

        static void Copy(T* destination, const T* source, w8_ulong count)
        {
            for (w8_ulong index = 0; index < count; ++index) {
                destination[index] = source[index];
            }
        }

        T* data;
        w8_ulong count;
    };

    MeshTable<srPtr<srTextureIFace> > poly_textures[4][2];
    MeshTable<srShader> poly_shaders[4];
    MeshTable<srPtr<srMaterialIFace> > vertex_materials[4][2];
    MeshTable<srVector3i> poly_vertices;
    MeshTable<srVector3i> poly_uv_indices[4];
    MeshTable<srVector4T<float> > poly_equations;
    MeshTable<srVector2T<float> > texcoords[4][2];
    MeshTable<srVector3T<float> > dig[4];
    MeshTable<srVector4T<float> > dcg[4];
    MeshTable<srVector4T<float> > scg[4];
    MeshTable<srVector3T<float> > vertex_locations;
    MeshTable<srVector3T<float> > vertex_normals;
    MeshTable<w8_ulong> vertex_shade_indices;
    MeshTable<w8_ulong> active_polygons;
    w8_long active_polygon_count;
    srVector3T<float> bounds_minimum;
    srVector3T<float> bounds_maximum;
    srVector3T<float> bounds_center;
    float bounds_radius;
    w8_long pass_count;
    w8_long vertex_location_count;
    w8_long polygon_count;
    w8_long uv_count;
    float sort_bias;
    TriMesh tri_mesh;
    srFlags<e_flags> dirty_flags;
    srFlags<e_control> render_control;
};

W8_ABI_ASSERT((sizeof(srMeshModel::TriMesh) == 0x154), "srMeshModel_TriMesh_must_be_0x154");
W8_ABI_ASSERT((sizeof(srMeshModel) == 0x398), "srMeshModel_must_be_0x398");
