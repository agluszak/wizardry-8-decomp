#pragma once

#include "srArray.h"
#include "srMaterialIFace.h"
#include "srMath.h"
#include "srMeshModel.h"
#include "srShader.h"
#include "srTextureIFace.h"

/* Retail exports the private getUniqueVertexList/isClockwise members and the
   implicit copy constructor/assignment, so the class is dllexport under
   SURRENDER_BUILD; consumer TUs stay member-level imported. */
#if defined(SURRENDER_BUILD)
class __declspec(dllexport) srModeler {
#else
class SR_DLL_IMPORT srModeler {
#endif
public:
    /* Axis selector indexing the position components: getAxialBounds and the
       mapping functions read (&vertex.position_00.x)[axis], proving X=0,
       Y=1, Z=2. */
    enum e_axis { AXIS_X = 0, AXIS_Y = 1, AXIS_Z = 2 };

    /* The exported constructor (0x10037BD0) takes two e_axis values and four
       floats; the ??_F default-constructor closure (0x10037DC0) proves all six
       arguments carry defaults. In-class inline: retail Wiz8.exe imports only
       planarMap and never this constructor, so the consumer TUs inlined the
       six-field store while the dllexport standalone emission stays at
       0x10037BD0. */
    /* Retail exports the MappingInfo constructor, implicit assignment and the
       default-constructor closure, so the declaration is dllexport under
       SURRENDER_BUILD. */
    struct
#if defined(SURRENDER_BUILD)
        __declspec(dllexport)
#endif
        MappingInfo {
        // FUNCTION: SURRENDER 0x10037BD0
        // ??0MappingInfo@srModeler@@QAE@W4e_axis@1@0MMMM@Z
        MappingInfo(e_axis axis_u = AXIS_X, e_axis axis_v = AXIS_Y, float u_scale = 1.0f,
                    float v_scale = 1.0f, float u_offset = 0.0f, float v_offset = 0.0f)
            : axis_u_00(axis_u), axis_v_04(axis_v), u_scale_08(u_scale), v_scale_0c(v_scale),
              u_offset_10(u_offset), v_offset_14(v_offset)
        {
        }

        /* Implicit assignment emitted via the class-level dllexport as a
           memberwise copy of the six fields. */
        // SYNTHETIC: SURRENDER 0x10037DA0
        // ??4MappingInfo@srModeler@@QAEAAU01@ABU01@@Z

        e_axis axis_u_00;
        e_axis axis_v_04;
        float u_scale_08;
        float v_scale_0c;
        float u_offset_10;
        float v_offset_14;
    };

    /* A triangle vertex: position, the per-pass material pair (side-indexed),
       the three per-pass attribute vectors convert() feeds into the mesh's
       DCG/DIG/SCG streams, the eight UV slots (pass*2 + layer), and the
       per-pass weights convert() writes as the DCG alpha. */
    /* Retail exports the full Vertex lifecycle sweep including the implicit
       copy/assignment bodies, so the declaration is dllexport under
       SURRENDER_BUILD. */
    class
#if defined(SURRENDER_BUILD)
        __declspec(dllexport)
#endif
        Vertex {
    public:
        /* Retail inlines the reset() call into the modeler-TU
           new Vertex[]/vertices_30[3] array-construction loops
           (Polygon::Polygon 0x10038890, Triangle::Triangle 0x10038B50) while
           Wiz8.exe imports the standalone copy - so the body is an inline
           definition in modeler.cpp, not visible to consumers. */
        // FUNCTION: SURRENDER 0x10037BC0
        // ??0Vertex@srModeler@@QAE@XZ
        Vertex();
        void reset();
        void interpolate(const Vertex& first, const Vertex& second, float fraction);
        int operator==(const Vertex& other) const;
        int operator!=(const Vertex& other) const;
        Vertex& operator=(const Vertex& other);

        /* stCube.cpp writes the modelled position and the first of the eight
           UV slots the Polygon constructor layout-initializes at +0xC0. */
        srVector3T<float> position_00;
        unsigned long shade_index_0c;
        srMaterialIFace* materials_10[4][2];
        srVector3T<float> dcg_30[4];
        srVector3T<float> dig_60[4];
        srVector3T<float> scg_90[4];
        srVector2T<float> uv_c0[8];
        float weights_100[4];
    };

    /* Retail exports the full Triangle lifecycle sweep including the implicit
       copy/assignment bodies, so the declaration is dllexport under
       SURRENDER_BUILD. */
    class
#if defined(SURRENDER_BUILD)
        __declspec(dllexport)
#endif
        Triangle {
    public:
        Triangle();
        void reset();
        void flipFacing();

        /* Implicit copy constructor/assignment emitted via the class-level
           dllexport as memberwise copies. The assignment reaches the Vertex
           members through srModeler::Vertex::operator= rather than a block
           move; the copy constructor block-copies them.

           The copy constructor's export ordinal addresses inter-function
           padding: retail and the recompiled provider both place nops at that
           RVA, so no body exists in either image to compare, and the
           recompiled export reproduces retail's ordinal and RVA. */
        // SYNTHETIC: SURRENDER 0x10037C10
        // ??0Triangle@srModeler@@QAE@ABV01@@Z
        // SYNTHETIC: SURRENDER 0x10037C70
        // ??4Triangle@srModeler@@QAEAAV01@ABV01@@Z

        srTextureIFace* textures_00[4][2];
        srShader shaders_20[4];
        Vertex vertices_30[3];
        unsigned long flags_360;
        unsigned long disabled_364;
    };

    /* Retail exports the full Polygon lifecycle sweep including the implicit
       copy/assignment bodies, so the declaration is dllexport under
       SURRENDER_BUILD. */
    class
#if defined(SURRENDER_BUILD)
        __declspec(dllexport)
#endif
        Polygon {
    public:
        Polygon(int vertices);
        ~Polygon();
        void reset();
        void reAllocate(int vertices);

        /* Implicit copy constructor/assignment emitted via the class-level
           dllexport as memberwise copies. The copy constructor's export
           ordinal addresses inter-function padding, as for Triangle above. */
        // SYNTHETIC: SURRENDER 0x10037CF0
        // ??0Polygon@srModeler@@QAE@ABV01@@Z
        // SYNTHETIC: SURRENDER 0x10037D40
        // ??4Polygon@srModeler@@QAEAAV01@ABV01@@Z

        srTextureIFace* textures_00[4][2];
        srShader shaders_20[4];
        /* Engine Code\stCube.cpp assigns positions and UVs through this table
           after Polygon(4) allocates it. */
        Vertex* vertices_30;
        int vertex_count_34;
        unsigned long flags_38;
        unsigned long disabled_3c;
        int capacity_40;
    };

    srModeler();
    virtual ~srModeler();

    /* Implicit copy constructor/assignment: emitted via the class-level
       dllexport as memberwise copies (the srArray<Triangle> member owns the
       triangle storage clone). */
    // SYNTHETIC: SURRENDER 0x10037DE0
    // ??0srModeler@@QAE@ABV0@@Z
    // SYNTHETIC: SURRENDER 0x10037F90
    // srModeler::operator=

    void discard();

    unsigned long getTriangleCount() const;
    void setTriangleCount(unsigned long triangles);
    unsigned long addTriangle(const Triangle& triangle);
    int getTriangle(unsigned long index, Triangle& triangle);
    void setTriangle(unsigned long index, const Triangle& triangle);
    void setTriangleVertex(unsigned long triangle, unsigned long vertex, const Vertex& value);
    void flipTriangle(unsigned long triangle);
    void flipTriangles();
    void enableTriangle(unsigned long triangle);
    void disableTriangle(unsigned long triangle);
    unsigned long getEnabledTriangleCount();
    void removeDisabledTriangles();
    void disableDegenerateTriangles();
    void addFromModeler(srModeler& other);

    void scale(const srVector3T<float>& scale);
    void scale(unsigned long triangle, const srVector3T<float>& scale);
    void move(const srVector3T<float>& delta);
    void move(unsigned long triangle, const srVector3T<float>& delta);
    void rotate(const srMatrix3T<float>& matrix);
    void rotate(unsigned long triangle, const srMatrix3T<float>& matrix);
    int findVertex(const srVector3T<float>& position, unsigned long& triangle,
                   unsigned long& vertex, unsigned long start_triangle);
    void findClosestVertex(const srVector3T<float>& position, unsigned long& triangle,
                           unsigned long& vertex);
    double getMaxVertexDist();
    void getAxialBounds(e_axis axis, float& minimum, float& maximum);

    long getPassCount() const;
    void setPassCount(long passes);

    unsigned long getUniqueVertexCount();

    void createSphere(long detail);
    void createTorus(long major_segments, long minor_segments, double radius);
    void createGrid(long columns, long rows);
    void tesselateEdges(double threshold);
    void tesselateEdges(unsigned long triangle, double threshold);
    /* Deferred: the retail body delegates to a separate 0x20-byte worker
       object; that worker is its own recovery lane. */
    void autoSmooth(double threshold, int smooth);

    void addPolygon(const Polygon& polygon);
    void planarMap(long pass, long layer, const MappingInfo& mapping);
    void planarMapAbsolute(long pass, long layer, const MappingInfo& mapping);
    void cylinderMap(long pass, long layer, const MappingInfo& mapping);
    void removeMapping(long pass, long layer);

    void setMaterial(srMaterialIFace* material, long pass, srMeshModel::e_side side);
    void setTexture(srTextureIFace* texture, long pass, long layer);
    void setShader(srShader shader, long pass);
    void convert(srMeshModel& model, int preserve);

    /* getUniqueVertexList's deduplication table: a raw entry pool, 1024
       position-hash buckets chaining entries, and the per-source-vertex
       result table written through during hashing. entries_00[i].shade_04 is
       the representative index convert() copies into the mesh's vertex shade
       table. */
    struct VertexHash {
        struct Entry {
            unsigned long flags_00;
            unsigned long shade_index_04;
            Vertex* vertex_08;
            Entry* next_0c;
            unsigned long index_10;
        };

        VertexHash(unsigned long vertex_count);
        ~VertexHash();

        static unsigned long hash(double x, double y, double z);

        Entry* entries_00;
        Entry* buckets_04[1024];
        Entry** table_1004;
        unsigned long unique_count_1008;
    };

private:
    VertexHash* getUniqueVertexList();
    int isClockwise(srVector2T<float>* points, int count);

    unsigned long triangle_count_04;
    srArray<Triangle> triangles_08;
    long pass_count_10;
};

static_assert((sizeof(srModeler) == 0x14), "srModeler_must_be_0x14");
static_assert((sizeof(srModeler::MappingInfo) == 0x18), "srModeler_MappingInfo_must_be_0x18");
static_assert((sizeof(srModeler::Vertex) == 0x110), "srModeler_Vertex_must_be_0x110");
static_assert((sizeof(srModeler::Triangle) == 0x368), "srModeler_Triangle_must_be_0x368");
static_assert((sizeof(srModeler::Polygon) == 0x44), "srModeler_Polygon_must_be_0x44");
static_assert((sizeof(srModeler::VertexHash) == 0x100c), "srModeler_VertexHash_must_be_0x100c");
static_assert((sizeof(srModeler::VertexHash::Entry) == 0x14),
              "srModeler_VertexHash_Entry_must_be_0x14");
