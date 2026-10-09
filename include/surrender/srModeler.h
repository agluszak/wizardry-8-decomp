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
        w8_ulong shade_index;
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
        w8_ulong flags;
        w8_ulong disabled;
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
        w8_ulong flags;
        w8_ulong disabled;
        int capacity;
    };

    srModeler();
    virtual ~srModeler();

    void discard();

    w8_ulong getTriangleCount() const;
    void setTriangleCount(w8_ulong triangles);
    w8_ulong addTriangle(const Triangle& triangle);
    int getTriangle(w8_ulong index, Triangle& triangle);
    void setTriangle(w8_ulong index, const Triangle& triangle);
    void setTriangleVertex(w8_ulong triangle, w8_ulong vertex, const Vertex& value);
    void flipTriangle(w8_ulong triangle);
    void flipTriangles();
    void enableTriangle(w8_ulong triangle);
    void disableTriangle(w8_ulong triangle);
    w8_ulong getEnabledTriangleCount();
    void removeDisabledTriangles();
    void disableDegenerateTriangles();
    void addFromModeler(srModeler& other);

    void scale(const srVector3T<float>& scale);
    void scale(w8_ulong triangle, const srVector3T<float>& scale);
    void move(const srVector3T<float>& delta);
    void move(w8_ulong triangle, const srVector3T<float>& delta);
    void rotate(const srMatrix3T<float>& matrix);
    void rotate(w8_ulong triangle, const srMatrix3T<float>& matrix);
    int findVertex(const srVector3T<float>& position, w8_ulong& triangle, w8_ulong& vertex,
                   w8_ulong start_triangle);
    void findClosestVertex(const srVector3T<float>& position, w8_ulong& triangle, w8_ulong& vertex);
    double getMaxVertexDist();
    void getAxialBounds(e_axis axis, float& minimum, float& maximum);

    w8_long getPassCount() const;
    void setPassCount(w8_long passes);

    w8_ulong getUniqueVertexCount();

    void createSphere(w8_long detail);
    void createTorus(w8_long major_segments, w8_long minor_segments, double radius);
    void createGrid(w8_long columns, w8_long rows);
    void tesselateEdges(double threshold);
    void tesselateEdges(w8_ulong triangle, double threshold);
    /* Delegates to the file-local AutoSmoother worker in modeler.cpp: it
       builds per-shade-vertex triangle adjacency, tests each coincident pair's
       facing/materials, and floods group bits back into flags. */
    void autoSmooth(double threshold, int smooth);

    void addPolygon(const Polygon& polygon);
    void planarMap(w8_long pass, w8_long layer, const MappingInfo& mapping);
    void planarMapAbsolute(w8_long pass, w8_long layer, const MappingInfo& mapping);
    void cylinderMap(w8_long pass, w8_long layer, const MappingInfo& mapping);
    void removeMapping(w8_long pass, w8_long layer);

    void setMaterial(srMaterialIFace* material, w8_long pass, srMeshModel::e_side side);
    void setTexture(srTextureIFace* texture, w8_long pass, w8_long layer);
    void setShader(srShader shader, w8_long pass);
    /* A nonzero remove_degenerate disables and drops triangles with
       coincident corners before building the mesh (the game passes 1). */
    void convert(srMeshModel& model, int remove_degenerate);

    /* getUniqueVertexList's deduplication table: a raw entry pool, 1024
       position-hash buckets chaining entries, and the per-source-vertex
       result table written through during hashing. entries[i].shade_index is
       the representative index convert() copies into the mesh's vertex shade
       table. */
    struct VertexHash {
        struct Entry {
            w8_ulong flags;
            /* Signed: the AutoSmoother worker's max scan in modeler.cpp
               compares it against its long vertex count with a signed JGE. */
            w8_long shade_index;
            Vertex* vertex;
            Entry* next;
            w8_ulong index;
        };

        VertexHash(w8_ulong vertex_count);
        ~VertexHash();

        static w8_ulong hash(double x, double y, double z);

        Entry* entries;
        Entry* buckets[1024];
        Entry** table;
        w8_ulong unique_count;
    };

private:
    VertexHash* getUniqueVertexList();
    int isClockwise(srVector2T<float>* points, int count);

    w8_ulong triangle_count;
    srArray<Triangle> triangles;
    w8_long pass_count;
};

W8_ABI_ASSERT((sizeof(srModeler) == 0x14), "srModeler_must_be_0x14");
static_assert((sizeof(srModeler::MappingInfo) == 0x18), "srModeler_MappingInfo_must_be_0x18");
W8_ABI_ASSERT((sizeof(srModeler::Vertex) == 0x110), "srModeler_Vertex_must_be_0x110");
W8_ABI_ASSERT((sizeof(srModeler::Triangle) == 0x368), "srModeler_Triangle_must_be_0x368");
W8_ABI_ASSERT((sizeof(srModeler::Polygon) == 0x44), "srModeler_Polygon_must_be_0x44");
W8_ABI_ASSERT((sizeof(srModeler::VertexHash) == 0x100c), "srModeler_VertexHash_must_be_0x100c");
W8_ABI_ASSERT((sizeof(srModeler::VertexHash::Entry) == 0x14),
              "srModeler_VertexHash_Entry_must_be_0x14");
