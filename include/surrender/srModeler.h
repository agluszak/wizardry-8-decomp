#pragma once

#include "srArray.h"
#include "srMaterialIFace.h"
#include "srMath.h"
#include "srMeshModel.h"
#include "srShader.h"
#include "srTextureIFace.h"

// VTABLE: SURRENDER 0x10076C88 srModeler
#if defined(SURRENDER_BUILD)
class __declspec(dllexport) srModeler {
#else
class SR_DLL_IMPORT srModeler {
#endif
public:
    /* Axis selector indexing the position components. */
    enum e_axis { AXIS_X = 0, AXIS_Y = 1, AXIS_Z = 2 };

    struct
#if defined(SURRENDER_BUILD)
        __declspec(dllexport)
#endif
        MappingInfo {
        // FUNCTION: SURRENDER 0x10037BD0
        // RECOMP: ??0MappingInfo@srModeler@@QAE@W4e_axis@1@0MMMM@Z
        MappingInfo(e_axis axis_u = AXIS_X, e_axis axis_v = AXIS_Y, float u_scale = 1.0f,
                    float v_scale = 1.0f, float u_offset = 0.0f, float v_offset = 0.0f)
            : axis_u(axis_u), axis_v(axis_v), u_scale(u_scale), v_scale(v_scale),
              u_offset(u_offset), v_offset(v_offset)
        {
        }

        e_axis axis_u;
        e_axis axis_v;
        float u_scale;
        float v_scale;
        float u_offset;
        float v_offset;
    };

    /* A triangle vertex: position, the per-pass material pair (side-indexed), the three per-pass
       attribute vectors convert() feeds into the mesh's DCG/DIG/SCG streams, the eight UV slots
       (pass*2 + layer), and the per-pass weights convert() writes as the DCG alpha. */
    class
#if defined(SURRENDER_BUILD)
        __declspec(dllexport)
#endif
        Vertex {
    public:
        Vertex();
        void reset();
        void interpolate(const Vertex& first, const Vertex& second, float fraction);
        int operator==(const Vertex& other) const;
        int operator!=(const Vertex& other) const;

        srVector3T<float> position;
        unsigned long shade_index;
        srMaterialIFace* materials[4][2];
        srVector3T<float> dcg[4];
        srVector3T<float> dig[4];
        srVector3T<float> scg[4];
        srVector2T<float> uv[8];
        float weights[4];
    };

    class
#if defined(SURRENDER_BUILD)
        __declspec(dllexport)
#endif
        Triangle {
    public:
        Triangle();
        void reset();
        void flipFacing();

        srTextureIFace* textures[4][2];
        srShader shaders[4];
        Vertex vertices[3];
        unsigned long flags;
        unsigned long disabled;
    };

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

        srTextureIFace* textures[4][2];
        srShader shaders[4];
        /* Engine Code\stCube.cpp assigns positions and UVs through this table
           after Polygon(4) allocates it. */
        Vertex* vertices;
        int vertex_count;
        unsigned long flags;
        unsigned long disabled;
        int capacity;
    };

    srModeler();
    virtual ~srModeler();

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
    /* Delegates to the file-local AutoSmoother worker in modeler.cpp: it
       builds per-shade-vertex triangle adjacency, tests each coincident pair's
       facing/materials, and floods group bits back into flags. */
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
       result table written through during hashing. entries[i].shade_index is
       the representative index convert() copies into the mesh's vertex shade
       table. */
    struct VertexHash {
        struct Entry {
            unsigned long flags;
            /* Signed: the AutoSmoother worker's max scan in modeler.cpp
               compares it against its long vertex count with a signed JGE. */
            long shade_index;
            Vertex* vertex;
            Entry* next;
            unsigned long index;
        };

        VertexHash(unsigned long vertex_count);
        ~VertexHash();

        static unsigned long hash(double x, double y, double z);

        Entry* entries;
        Entry* buckets[1024];
        Entry** table;
        unsigned long unique_count;
    };

private:
    VertexHash* getUniqueVertexList();
    int isClockwise(srVector2T<float>* points, int count);

    unsigned long triangle_count;
    srArray<Triangle> triangles;
    long pass_count;
};

static_assert((sizeof(srModeler) == 0x14), "srModeler_must_be_0x14");
static_assert((sizeof(srModeler::MappingInfo) == 0x18), "srModeler_MappingInfo_must_be_0x18");
static_assert((sizeof(srModeler::Vertex) == 0x110), "srModeler_Vertex_must_be_0x110");
static_assert((sizeof(srModeler::Triangle) == 0x368), "srModeler_Triangle_must_be_0x368");
static_assert((sizeof(srModeler::Polygon) == 0x44), "srModeler_Polygon_must_be_0x44");
static_assert((sizeof(srModeler::VertexHash) == 0x100c), "srModeler_VertexHash_must_be_0x100c");
static_assert((sizeof(srModeler::VertexHash::Entry) == 0x14),
              "srModeler_VertexHash_Entry_must_be_0x14");
