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
    enum e_side {};
    /* Bit indices into control_state_390; setDirty(0..3) marks per-pass dirty
       flags and updateAllClients(0) runs when flag 0 is newly raised. */
    enum e_flags {};
    /* Bit indices into control_state_394. renderTriMesh tests bits 0/1 as
       front/back sides. updateTriMesh skips auto box when bit 4 is set and
       auto sphere when bit 5 is set. */
    enum e_control {
        CONTROL_FRONT = 0,
        CONTROL_BACK = 1,
        CONTROL_SKIP_AUTO_BOX = 4,
        CONTROL_SKIP_AUTO_SPHERE = 5,
        CONTROL_STARTUP = 6
    };
    /* The four per-pass table slots cap t.passes, as verify() asserts. */
    enum { MAX_PASSES = 4 };
    /* Detached 0x154-byte value at srMeshModel+0x23c. updateTriMesh fills it
       from the live tables; getTriMesh copies or returns it; renderTriMesh
       feeds srTriMeshPipeline from these slots. */
    struct TriMesh {
        /* verify()'s emission zeroes only poly_vertices_10 before the
           getTriMesh fill. */
        TriMesh() : poly_vertices_10(0) {}

        long vertex_count_00;
        long polygon_count_04;
        long pass_count_08;
        unsigned long control_flags;
        srVector3i* poly_vertices_10;
        srVector4T<float>* poly_equations_14;
        srVector2T<float>* texcoords_18[4][2];
        srVector3T<float>* positions_38;
        srVector3T<float>* normals_3c;
        srVector3T<float>* dig_40[4];
        srVector4T<float>* dcg_50[4];
        srVector4T<float>* scg_60[4];
        srMaterial* materials_70[4][2];
        srTextureIFace* textures_90[4][2];
        srShader shaders_b0[4];
        srPtr<srMaterialIFace>* vertex_materials_c0[4][2];
        srPtr<srTextureIFace>* poly_textures_e0[4][2];
        srShader* poly_shaders[4];
        srVector3i* poly_uv_110[4];
        srVector3T<float> bounds_minimum_120;
        srVector3T<float> bounds_maximum_12c;
        srVector3T<float> bounds_center_138;
        float bounds_radius_144;
        float sort_bias_148;
        unsigned long* active_polygons_14c;
        long active_polygon_count_150;
    };

    /* The default-constructor closure 0x100425A0 proves both arguments
       default to zero for paren-less new expressions. */
    SR_DLL_IMPORT srMeshModel(long polygons = 0, long vertices = 0);

    SR_DLL_IMPORT void reset(long polygons, long vertices);
    SR_DLL_IMPORT void scale(const srVector3T<float>& scale);
    void applyMatrix(const srMatrix3T<float>& matrix);
    SR_DLL_IMPORT void relocateVertices(const srVector3T<float>& offset);
    void centerVertices();
    double getAverageRadius();
    double getMaxRadius();
    void scaleToAverageRadius(double radius);
    void scaleToMaxRadius(double radius);
    void flipFaces();
    long findClosestVertex(const srVector3T<float>& point);
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
    SR_DLL_IMPORT virtual void reindexPolygons(const unsigned long* indices);
    SR_DLL_IMPORT virtual void reindexVertices(const unsigned long* indices);
    SR_DLL_IMPORT virtual void getTriMesh(TriMesh& mesh);
    SR_DLL_IMPORT virtual const TriMesh& getTriMesh();
    SR_DLL_IMPORT virtual void renderTriMesh(class srGERD& renderer, const TriMesh& mesh);
    SR_DLL_IMPORT srPtr<srTextureIFace>* getPolyTexture(long polygon, long layer, int table);
    SR_DLL_IMPORT srVector3i* getPolyVertex();
    SR_DLL_IMPORT srVector3i* getPolyUVIndex(long layer, int table);
    SR_DLL_IMPORT srVector2T<float>* getVertexTexCoords(long vertex, long layer, int table);
    SR_DLL_IMPORT srPtr<srMaterialIFace>* getVertexMaterial(long vertex, e_side side, int table);
    SR_DLL_IMPORT unsigned long* getVertexShadeIndex(int table);
    SR_DLL_IMPORT srVector3T<float>* getVertexNormal();
    SR_DLL_IMPORT srVector4T<float>* getPolyEq();
    SR_DLL_IMPORT srVector3T<float>* getVertexDIG(long vertex, int table);
    srVector4T<float>* getVertexSCG(long vertex, int table);
    srVector4T<float>* getVertexDCG(long vertex, int table);
    SR_DLL_IMPORT srMaterialIFace* getMaterial(long polygon, e_side side) const;
    SR_DLL_IMPORT srTextureIFace* getTexture(long polygon, long layer) const;
    SR_DLL_IMPORT void setMaterial(srMaterialIFace* material, long polygon, e_side side);
    SR_DLL_IMPORT void setTexture(srTextureIFace* texture, long polygon, long layer);
    /* In-class inlines: srModeler::convert expands these bodies inside the
       srModeler TU. */
    // FUNCTION: SURRENDER 0x10041710 SYMBOL
    // ?setDirty@srMeshModel@@QAEXW4e_flags@1@@Z
    void setDirty(e_flags flag)
    {
        unsigned long mask = 1 << flag;
        if ((control_state_390.value & mask) == 0) {
            control_state_390.set(flag, 1);
            control_state_390.set(3, 1);
            if (flag == 0) {
                updateAllClients(static_cast<Client::e_update>(0));
            }
        }
    }
    // FUNCTION: SURRENDER 0x10041750 SYMBOL
    // ?clearDirty@srMeshModel@@QAEXW4e_flags@1@@Z
    void clearDirty(e_flags flag)
    {
        control_state_390.set(flag, 0);
    }
    // FUNCTION: SURRENDER 0x10041770 SYMBOL
    // ?testDirty@srMeshModel@@QBEHW4e_flags@1@@Z
    int testDirty(e_flags flag) const
    {
        return (control_state_390.value & (1 << flag)) != 0;
    }
    SR_DLL_IMPORT srShader* getPolyShader(long polygon, int layer);
    SR_DLL_IMPORT srShader getShader(long polygon) const;
    SR_DLL_IMPORT void setShader(srShader shader, long pass);
    SR_DLL_IMPORT void setUVCount(long count);
    long getUVCount() const;
    SR_DLL_IMPORT void setActivePolygonCount(long count);
    SR_DLL_IMPORT long getActivePolygonCount();
    long getPassCount() const;
    long getPolygonCount() const;
    long getVertexCount() const;
    void setPassCount(long count);
    // FUNCTION: SURRENDER 0x10041790
    void setSortBias(float bias)
    {
        sort_bias_238 = bias;
        setDirty(static_cast<e_flags>(3));
    }
    float getSortBias() const;
    void disable(e_control control);
    // FUNCTION: SURRENDER 0x10041830
    void enable(e_control control)
    {
        control_state_394.set(control, 1);
        setDirty(static_cast<e_flags>(3));
    }
    int isEnabled(e_control control) const;
    // FUNCTION: SURRENDER 0x10041660
    void setDirtyAll()
    {
        setDirty(static_cast<e_flags>(0));
        setDirty(static_cast<e_flags>(1));
        setDirty(static_cast<e_flags>(2));
        setDirty(static_cast<e_flags>(3));
    }
    void setDirtyBounds();
    void setDirtyNormals();
    SR_DLL_IMPORT unsigned long* getActivePolygonTable(int table);
    SR_DLL_IMPORT srVector3T<float>* getVertexLoc();
    /* Fills the cached AABB/sphere (bounds_minimum_200..bounds_radius_224)
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
    /* setMaterial indexes [pass][side]; ctor default-constructs eight slots.
       The ctor/dtor array emissions prove srPtr elements (4 x 8 bytes via
       __eharray, single srPtr ctor/dtor each). */
    srPtr<srMaterialIFace> materials_1c[4][2];
    srPtr<srTextureIFace> textures_3c[4][2];
    srShader shaders_5c[4];
    /* Lazily grown mesh table pair. Retail's constructor/destructor emit the
       pair records through array ctors/dtors; every table accessor resizes
       `data` to its governing count on first use. POD elements zero-fill;
       srPtr elements release through their own destructor. Concrete marker
       names are recomp selectors unless independently bound to typed owners;
       equal-width POD copies and bare frees do not distinguish exact T. */
    template <class T> struct MeshTable {
        MeshTable() : data(0), count(0) {}
        MeshTable(const MeshTable& other) : data(0), count(0)
        {
            *this = other;
        }
        /* The member-array destructor emissions: the srPtr copies run the
           per-element release loop while the POD copies fold to a bare
           free + zero. */
        ~MeshTable()
        {
            Release();
        }
        /* Release, then the preserving resize and the element copy:
           srMeshModel::operator= inlines this member as the three separate
           calls while the standalone emissions inline the member bodies. */
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

        /* Retail emits one allocation emission per element type, each a
           thiscall on the table (the member never reads it): the srPtr/srShader
           copies default-construct every element while the POD copies allocate
           only, exactly as VC6 lowers an array new through srHeap. */
        T* Allocate(unsigned long elements)
        {
            T* replacement = static_cast<T*>(srHeap.allocate(elements * sizeof(T)));
            for (unsigned long index = 0; index < elements; ++index) {
                new (&replacement[index]) T;
            }
            return replacement;
        }

        /* Release each element, free the allocation, and zero the pair; the
           srPtr copies emit per-element releases while POD copies fold to a
           bare free. */
        void Release()
        {
            if (data != 0) {
                for (unsigned long index = 0; index < count; ++index) {
                    data[index].~T();
                }
                srHeap.free(data);
            }
            data = 0;
            count = 0;
        }

        /* The shared resize used by the accessors: fresh
           Allocate() storage, a min(old,new) prefix copy only when
           `preserve` is set, the old table's full Release(), then the pair
           retargets. operator= and the copy-ctor pass preserve=1 on an
           empty table where the prefix copy is dead. */
        void Resize(unsigned long elements, int preserve)
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

        /* The elementwise copy operator= and Resize share; the srPtr
           instantiations run the addref/release handoff through each
           element's own assignment. */
        static void Copy(T* destination, const T* source, unsigned long count)
        {
            for (unsigned long index = 0; index < count; ++index) {
                destination[index] = source[index];
            }
        }

        T* data;
        unsigned long count;
    };

    MeshTable<srPtr<srTextureIFace> > poly_textures_6c[4][2];
    MeshTable<srShader> poly_shaders_ac[4];
    MeshTable<srPtr<srMaterialIFace> > vertex_materials_cc[4][2];
    MeshTable<srVector3i> poly_vertices_10c;
    MeshTable<srVector3i> poly_uv_indices[4];
    MeshTable<srVector4T<float> > poly_equations_134;
    MeshTable<srVector2T<float> > texcoords_13c[4][2];
    MeshTable<srVector3T<float> > dig_17c[4];
    MeshTable<srVector4T<float> > dcg_19c[4];
    MeshTable<srVector4T<float> > scg_1bc[4];
    MeshTable<srVector3T<float> > vertex_locations_1dc;
    MeshTable<srVector3T<float> > vertex_normals;
    MeshTable<unsigned long> vertex_shade_indices;
    MeshTable<unsigned long> active_polygons_1f4;
    long active_polygon_count_1fc;
    srVector3T<float> bounds_minimum_200;
    srVector3T<float> bounds_maximum_20c;
    srVector3T<float> bounds_center_218;
    float bounds_radius_224;
    long pass_count_228;
    /* GrCycle.cpp's 0x004A7E50 clamps a vertex index against this before
       indexing the location array, which is what makes it that array's
       length rather than one more opaque dword. */
    long vertex_location_count_22c;
    long polygon_count_230;
    long uv_count_234;
    float sort_bias_238;
    TriMesh tri_mesh_23c;
    srFlags<e_flags> control_state_390;
    srFlags<e_control> control_state_394;
};

static_assert((sizeof(srMeshModel::TriMesh) == 0x154), "srMeshModel_TriMesh_must_be_0x154");
static_assert((sizeof(srMeshModel) == 0x398), "srMeshModel_must_be_0x398");
