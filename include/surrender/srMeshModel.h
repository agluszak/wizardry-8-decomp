#pragma once

#include <new>

#include "srHeap.h"
#include "srMaterial.h"
#include "srMath.h"
#include "srModel.h"
#include "srPtr.h"
#include "srShader.h"
#include "srTexture.h"
#include "srTypeRegistry.h"

// VTABLE: SURRENDER 0x10076D48 srMeshModel
class SR_DLL_IMPORT srMeshModel : public srClassSupport<srMeshModel, srModel, 0, 0x2010> {
public:
    enum e_side {};
    /* Bit indices into control_state_390; setDirty(0..3) marks per-pass dirty
       flags and updateAllClients(0) runs when flag 0 is newly raised. */
    enum e_flags {};
    /* Bit indices into control_state_394. renderTriMesh tests bits 0/1 as
       front/back sides. updateTriMesh skips auto box when bit 4 is set and
       auto sphere when bit 5 is set. enableStartupControls ORs bits 4–6. */
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
        unsigned long control_flags_0c;
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
        srShader* poly_shaders_100[4];
        srVector3i* poly_uv_110[4];
        srVector3T<float> bounds_minimum_120;
        srVector3T<float> bounds_maximum_12c;
        srVector3T<float> bounds_center_138;
        float bounds_radius_144;
        float sort_bias_148;
        unsigned long* active_polygons_14c;
        long active_polygon_count_150;
    };

    srMeshModel(long polygons, long vertices);
    srMeshModel(const srMeshModel& other);
    void reset(long polygons, long vertices);
    void scale(const srVector3T<float>& scale);
    void applyMatrix(const srMatrix3T<float>& matrix);
    void relocateVertices(const srVector3T<float>& offset);
    void centerVertices();
    double getAverageRadius();
    double getMaxRadius();
    void scaleToAverageRadius(double radius);
    void scaleToMaxRadius(double radius);
    void flipFaces();
    long findClosestVertex(const srVector3T<float>& point);
    srMeshModel& operator=(const srMeshModel& other);

#if defined(SURRENDER_BUILD)
    static const char* sGetClassName();
#else
    static const char* sGetClassName()
    {
        return "srMeshModel";
    }
#endif

    virtual void dump(std::ostream& stream) override;
    virtual void verify(srRuntimeClass::e_verify mode) override;
    virtual srClass* vInstance() override;
    virtual int getBoundingSphere(srVector3T<float>& center, float& radius) override;
    virtual int getBoundingBox(srVector3T<float>& minimum, srVector3T<float>& maximum) override;
    virtual void render(class srGERD& renderer) override;
    virtual void reindexPolygons(const unsigned long* indices);
    virtual void reindexVertices(const unsigned long* indices);
    virtual void getTriMesh(TriMesh& mesh);
    virtual const TriMesh& getTriMesh();
    virtual void renderTriMesh(class srGERD& renderer, const TriMesh& mesh);
    srPtr<srTextureIFace>* getPolyTexture(long polygon, long layer, int table);
    srVector3i* getPolyVertex();
    srVector3i* getPolyUVIndex(long layer, int table);
    srVector2T<float>* getVertexTexCoords(long vertex, long layer, int table);
    srPtr<srMaterialIFace>* getVertexMaterial(long vertex, e_side side, int table);
    unsigned long* getVertexShadeIndex(int table);
    srVector3T<float>* getVertexNormal();
    srVector4T<float>* getPolyEq();
    srVector3T<float>* getVertexDIG(long vertex, int table);
    srVector4T<float>* getVertexSCG(long vertex, int table);
    srVector4T<float>* getVertexDCG(long vertex, int table);
    srMaterialIFace* getMaterial(long polygon, e_side side) const;
    srTextureIFace* getTexture(long polygon, long layer) const;
    void setMaterial(srMaterialIFace* material, long polygon, e_side side);
    void setTexture(srTextureIFace* texture, long polygon, long layer);
    /* In-class inlines: srModeler::convert expands these bodies inside the
       srModeler TU; dllexport still emits the standalone copies below. */
    // FUNCTION: SURRENDER 0x10041710 SYMBOL
    // ?setDirty@srMeshModel@@QAEXW4e_flags@1@@Z
    void setDirty(e_flags flag)
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
    // FUNCTION: SURRENDER 0x10041750 SYMBOL
    // ?clearDirty@srMeshModel@@QAEXW4e_flags@1@@Z
    void clearDirty(e_flags flag)
    {
        control_state_390 &= ~(1 << (flag & 0x1f));
    }
    // FUNCTION: SURRENDER 0x10041770 SYMBOL
    // ?testDirty@srMeshModel@@QBEHW4e_flags@1@@Z
    int testDirty(e_flags flag) const
    {
        return (control_state_390 & (1 << (flag & 0x1f))) != 0;
    }
    srShader* getPolyShader(long polygon, int layer);
    srShader getShader(long polygon) const;
    void setShader(srShader shader, long pass);
    void setUVCount(long count);
    long getUVCount() const;
    void setActivePolygonCount(long count);
    long getActivePolygonCount();
    long getPassCount() const;
    long getPolygonCount() const;
    long getVertexCount() const;
    void setPassCount(long count);
    void setSortBias(float bias);
    float getSortBias() const;
    void disable(e_control control);
    void enable(e_control control);
    int isEnabled(e_control control) const;
    void setDirtyAll();
    void setDirtyBounds();
    void setDirtyNormals();
    unsigned long* getActivePolygonTable(int table);
    srVector3T<float>* getVertexLoc();
    void enableStartupControls()
    {
        control_state_394 |= 0x40;
        control_state_390 |= 8;
        control_state_394 |= 0x30;
    }
    /* Raise one 0x394 control bit and mark the 0x390 changed bit when it is
       clear. The original stores the changed bit twice; VC6 emits that pair
       at every expansion site, so the body keeps both stores. */
    void setControlMask(unsigned long mask)
    {
        control_state_394 |= mask;
        if ((control_state_390 & 8) == 0) {
            unsigned long state = control_state_390;
            control_state_390 = state | 8;
            control_state_390 = state | 8;
        }
    }
    /* Fills the cached AABB/sphere (bounds_minimum_200..bounds_radius_224)
       from the supplied box and center/radius. */
    void setBounds(const srVector3T<float>& minimum, const srVector3T<float>& maximum,
                   const srVector3T<float>& center, float radius);

protected:
    virtual ~srMeshModel() override;
    void freeAll();
    virtual void updateTriMesh();
    virtual void calculateBounds();
    virtual void calculatePolygonNormals();
    virtual void calculateVertexNormals();

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
       srPtr elements release through their own destructor. */
    template <class T> struct MeshTable {
        MeshTable() : data(0), count(0) {}
        MeshTable(const MeshTable& other) : data(0), count(0)
        {
            *this = other;
        }
        /* The member-array destructor emissions: the srPtr copies run the
           per-element release loop while the POD copies fold to a bare
           free + zero. */
        // TEMPLATE: SURRENDER 0x10042B10
        // srMeshModel::MeshTable<srPtr<srTextureIFace> >::~MeshTable
        // TEMPLATE: SURRENDER 0x10042D00
        // srMeshModel::MeshTable<srShader>::~MeshTable
        // TEMPLATE: SURRENDER 0x10042DF0
        // srMeshModel::MeshTable<srPtr<srMaterialIFace> >::~MeshTable
        // TEMPLATE: SURRENDER 0x10042FE0
        // srMeshModel::MeshTable<srVector3i>::~MeshTable
        // TEMPLATE: SURRENDER 0x100431E0
        // srMeshModel::MeshTable<srVector4T<float> >::~MeshTable
        // TEMPLATE: SURRENDER 0x100433F0
        // srMeshModel::MeshTable<srVector2T<float> >::~MeshTable
        // TEMPLATE: SURRENDER 0x100435B0
        // srMeshModel::MeshTable<srVector3T<float> >::~MeshTable
        // TEMPLATE: SURRENDER 0x100437B0
        // srMeshModel::MeshTable<unsigned long>::~MeshTable
        ~MeshTable()
        {
            Release();
        }
        /* Release, then the preserving resize and the element copy:
           srMeshModel::operator= inlines this member as the three separate
           calls while the standalone emissions inline the member bodies. */
        // TEMPLATE: SURRENDER 0x10043010
        // srMeshModel::MeshTable<srVector3i>::operator=
        // TEMPLATE: SURRENDER 0x10043210
        // srMeshModel::MeshTable<srVector4T<float> >::operator=
        // TEMPLATE: SURRENDER 0x10043420
        // srMeshModel::MeshTable<srVector2T<float> >::operator=
        // TEMPLATE: SURRENDER 0x100435E0
        // srMeshModel::MeshTable<srVector3T<float> >::operator=
        // TEMPLATE: SURRENDER 0x100437E0
        // srMeshModel::MeshTable<unsigned long>::operator=
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
        // TEMPLATE: SURRENDER 0x10043B10
        // srMeshModel::MeshTable<srPtr<srTextureIFace> >::Allocate
        // TEMPLATE: SURRENDER 0x10043B80
        // srMeshModel::MeshTable<srShader>::Allocate
        // TEMPLATE: SURRENDER 0x10043C20
        // srMeshModel::MeshTable<srPtr<srMaterialIFace> >::Allocate
        // TEMPLATE: SURRENDER 0x10043CA0
        // srMeshModel::MeshTable<srVector3i>::Allocate
        // TEMPLATE: SURRENDER 0x10043D00
        // srMeshModel::MeshTable<srVector4T<float> >::Allocate
        // TEMPLATE: SURRENDER 0x10043D50
        // srMeshModel::MeshTable<srVector2T<float> >::Allocate
        // TEMPLATE: SURRENDER 0x10043DB0
        // srMeshModel::MeshTable<srVector3T<float> >::Allocate
        // TEMPLATE: SURRENDER 0x10044E10
        // srMeshModel::MeshTable<unsigned long>::Allocate
        T* Allocate(long elements)
        {
            T* replacement = static_cast<T*>(srHeap.allocate(elements * sizeof(T)));
            for (long index = 0; index < elements; ++index) {
                new (&replacement[index]) T;
            }
            return replacement;
        }

        /* Release each element, free the allocation, and zero the pair; the
           srPtr copies emit per-element releases while POD copies fold to a
           bare free. */
        // TEMPLATE: SURRENDER 0x10042B70
        // srMeshModel::MeshTable<srPtr<srTextureIFace> >::Release
        // TEMPLATE: SURRENDER 0x10042D30
        // srMeshModel::MeshTable<srShader>::Release
        // TEMPLATE: SURRENDER 0x10042E50
        // srMeshModel::MeshTable<srPtr<srMaterialIFace> >::Release
        // TEMPLATE: SURRENDER 0x10043100
        // srMeshModel::MeshTable<srVector3i>::Release
        // TEMPLATE: SURRENDER 0x10043300
        // srMeshModel::MeshTable<srVector4T<float> >::Release
        // TEMPLATE: SURRENDER 0x100434E0
        // srMeshModel::MeshTable<srVector2T<float> >::Release
        // TEMPLATE: SURRENDER 0x100436D0
        // srMeshModel::MeshTable<srVector3T<float> >::Release
        // TEMPLATE: SURRENDER 0x100438A0
        // srMeshModel::MeshTable<unsigned long>::Release
        void Release()
        {
            for (long index = 0; index < count; ++index) {
                data[index].~T();
            }
            if (data != 0) {
                srHeap.free(data);
            }
            data = 0;
            count = 0;
        }

        /* The preserving resize every accessor's grow path reaches: fresh
           Allocate() storage, a min(old,new) prefix copy only when
           `preserve` is set, the old table's full Release(), then the pair
           retargets. operator= and the copy-ctor pass preserve=1 on an
           empty table where the prefix copy is dead. */
        // TEMPLATE: SURRENDER 0x10042BD0
        // srMeshModel::MeshTable<srPtr<srTextureIFace> >::Resize
        // TEMPLATE: SURRENDER 0x10042D60
        // srMeshModel::MeshTable<srShader>::Resize
        // TEMPLATE: SURRENDER 0x10042EB0
        // srMeshModel::MeshTable<srPtr<srMaterialIFace> >::Resize
        // TEMPLATE: SURRENDER 0x10043130
        // srMeshModel::MeshTable<srVector3i>::Resize
        // TEMPLATE: SURRENDER 0x10043330
        // srMeshModel::MeshTable<srVector4T<float> >::Resize
        // TEMPLATE: SURRENDER 0x10043510
        // srMeshModel::MeshTable<srVector2T<float> >::Resize
        // TEMPLATE: SURRENDER 0x10043700
        // srMeshModel::MeshTable<srVector3T<float> >::Resize
        // TEMPLATE: SURRENDER 0x100438D0
        // srMeshModel::MeshTable<unsigned long>::Resize
        void Resize(long elements, int preserve)
        {
            if (count != elements) {
                T* replacement = 0;
                if (elements != 0) {
                    replacement = Allocate(elements);
                    if (data != 0 && count != 0 && preserve != 0) {
                        Copy(replacement, data, elements < count ? elements : count);
                    }
                }
                Release();
                data = replacement;
                count = elements;
            }
        }

        /* The elementwise copy operator= and Resize share; the srPtr
           instantiations run the addref/release handoff through each
           element's own assignment. */
        // TEMPLATE: SURRENDER 0x10043AB0
        // srMeshModel::MeshTable<srPtr<srTextureIFace> >::Copy
        // TEMPLATE: SURRENDER 0x10043B50
        // srMeshModel::MeshTable<srShader>::Copy
        // TEMPLATE: SURRENDER 0x10043BC0
        // srMeshModel::MeshTable<srPtr<srMaterialIFace> >::Copy
        // TEMPLATE: SURRENDER 0x10043C60
        // srMeshModel::MeshTable<srVector3i>::Copy
        // TEMPLATE: SURRENDER 0x10043CC0
        // srMeshModel::MeshTable<srVector4T<float> >::Copy
        // TEMPLATE: SURRENDER 0x10043D20
        // srMeshModel::MeshTable<srVector2T<float> >::Copy
        // TEMPLATE: SURRENDER 0x10043D70
        // srMeshModel::MeshTable<srVector3T<float> >::Copy
        // TEMPLATE: SURRENDER 0x10043DD0
        // srMeshModel::MeshTable<unsigned long>::Copy
        static void Copy(T* destination, const T* source, long count)
        {
            for (long index = 0; index < count; ++index) {
                destination[index] = source[index];
            }
        }

        T* data;
        long count;
    };

    MeshTable<srPtr<srTextureIFace> > poly_textures_6c[4][2];
    MeshTable<srShader> poly_shaders_ac[4];
    MeshTable<srPtr<srMaterialIFace> > vertex_materials_cc[4][2];
    MeshTable<srVector3i> poly_vertices_10c;
    MeshTable<srVector3i> poly_uv_indices_114[4];
    MeshTable<srVector4T<float> > poly_equations_134;
    MeshTable<srVector2T<float> > texcoords_13c[4][2];
    MeshTable<srVector3T<float> > dig_17c[4];
    MeshTable<srVector4T<float> > dcg_19c[4];
    MeshTable<srVector4T<float> > scg_1bc[4];
    MeshTable<srVector3T<float> > vertex_locations_1dc;
    MeshTable<srVector3T<float> > vertex_normals_1e4;
    MeshTable<unsigned long> vertex_shade_indices_1ec;
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
    unsigned long control_state_390;
    unsigned long control_state_394;
};

static_assert((sizeof(srMeshModel::TriMesh) == 0x154), "srMeshModel_TriMesh_must_be_0x154");
static_assert((sizeof(srMeshModel) == 0x398), "srMeshModel_must_be_0x398");
