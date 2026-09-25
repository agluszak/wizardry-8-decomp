#pragma once

#include "surrender/srArray.h"
#include "surrender/srMeshModel.h"
#include "wiz8/vector.h"
#include "surrender/srTypeRegistry.h"

/* Engine Code\stMeshModel.cpp. Only fields reached by reviewed bodies are
   modeled. The two short-vector pairs are parallel key/value tables; their
   semantic domain is not established, so the names stay positional. */
// VTABLE: WIZ8 0x005ec454 stMeshModel
// VTABLE: WIZ8 0x005ec4b0 srClassSupport<stMeshModel, srMeshModel, 0, 65539>
class stMeshModel : public srClassSupport<stMeshModel, srMeshModel, false, 0x10003> {
public:
    static const char* sGetClassName()
    {
        return "stMeshModel";
    }

    stMeshModel(long polygons, long vertices);
    virtual ~stMeshModel() override;       /* 0x00470ED0 */
    virtual srClass* vInstance() override; /* 0x004748c0 */
    virtual int getBoundingSphere(srVector3T<float>& center,
                                  float& radius) override; /* 0x00471dd0 */
    virtual int getBoundingBox(srVector3T<float>& minimum,
                               srVector3T<float>& maximum) override; /* 0x00471d80 */
    virtual void getTriMesh(TriMesh& mesh) override;                 /* 0x004727e0 */
    /* Recomputes the union bounds over every model in the previous/next chain
       and pushes them to each member via srMeshModel::setBounds. */
    void CalculateLinkedBounds();                 /* 0x00471e10 */
    virtual const TriMesh& getTriMesh() override; /* 0x00472270 */
    virtual void renderTriMesh(class srGERD& renderer,
                               const TriMesh& mesh) override; /* 0x00470360 */
    /* Shared Wizardry-extended tri-mesh submit. `poly_equations` null skips
       the software backface pass; non-null callers supply polygon normals used
       to build the active-polygon scratch at 0x00659ce0. */
    void RenderTriMeshWithEquations(class srGERD& renderer, const TriMesh& mesh,
                                    const srVector3T<float>* poly_equations); /* 0x00470380 */

    int FindMappedIndex(short key); /* 0x004712D0 */
    void SetMappedVertex(short vertex, short key);
    void LinkTo(stMeshModel* other);      /* 0x00471D60 */
    short* GetVertex(unsigned int frame); /* 0x00471AA0 */
    int FindSkinTable(const char* name);
    int CreateSkinTable(const char* name, int base_table);
    srPtr<srTextureIFace>* GetTextureTable(int table); /* 0x00473720 */
    /* Active-polygon index list for a texture table; writes the count through
       `count_out` and returns 0 when the table selects no polygons. */
    unsigned long* GetActivePolygons(long* count_out, int table, bool flag);
    void RemoveSkinTable(int index);
    void RemoveSkinTablesForCycle(const char* cycle_name);
    srVector3T<float>* GetVertexLocations00471AD0(unsigned int frame, char load,
                                                  float interpolation);
    srVector3T<float>* GetVertexNormals(unsigned int frame, char load); /* 0x00471CA0 */
    srVector3T<float>* GetPolygonNormals(unsigned int frame, char load);
    void SetAmbientColor(const srVector3T<float>& color);
    unsigned char AllocateFrameBuffers00471720(unsigned int uiFrame,
                                               unsigned char flags); /* 0x00471720 */
    srVector3T<float>* GetVertexLights(char initialize, int table);  /* 0x00472100 */
    float* GetVertexSunlight(char initialize);                       /* 0x004721E0 */
    void NotifyLinkedModel005AA400(stMeshModel* previous_model);
    void InitializeVertexFrames(int frames); /* 0x00473B00 */
    unsigned char AllocateFrameStorage();    /* 0x00471340 */
    void FreeFrameStorage();                 /* 0x004715E0 */
    int ReleaseDecompressedFrames();         /* 0x004739E0 */
    void FinalizeVertexFrame(int frame);
    /* Bounds `frame`'s vertex table into `minimum`/`maximum`, decompressing a
       scratch copy when the frame is not resident. */
    void GetFrameBounds(int frame, srVector3T<float>* minimum, srVector3T<float>* maximum);
    unsigned char DecompressFrame(int frame, unsigned char flags,
                                  srVector3T<float>* destination); /* 0x00471930 */
    void ComputeFrameNormals(int frame);                           /* 0x004729F0 */
    void ClearAutomapPolygonFilter();
    void ApplyAutomapPolygonFilter(const W8GrowableVector<char*>* excluded_textures);

    stMeshModel* next;     /* 0x398 */
    stMeshModel* previous; /* 0x39c */
    unsigned int flags_3a0;
    srVector3T<float> ambient_color_3a4;
    int vertex_light_table_3b0;
    /* m_pVertLights: per-vertex static lighting, zero-filled on demand; table
       -1 selects vertex_light_table_3b0. */
    srHeapArray<srVector3T<float> > vertex_lights_3b4[2];
    /* Per-vertex sunlight intensity, filled with 1.0f on demand. */
    srHeapArray<float> vertex_sunlight_3c4;
    unsigned char duplicate_on_reuse_3cc;
    /* Set once both vertex lights and sunlight exist. */
    bool vertex_lighting_ready_3cd;
    unsigned char padding_3ce[2];
    /* uiFrames: per-frame tables below hold one pointer per frame. The
       decompressed float caches are srHeap allocations and are counted in
       g_decompressed_mesh_bytes; the compressed tables are operator new. */
    unsigned int frame_count;                   /* 0x3d0 */
    srVector3T<float>** m_pVertexLoc;           /* 0x3d4 */
    srVector3T<float>** m_pVertexNormal;        /* 0x3d8 */
    srVector3T<float>** m_pPolyNormal;          /* 0x3dc */
    short** compressed_vertex_locations;        /* 0x3e0 m_psCompVertexLoc */
    unsigned char** compressed_vertex_normals;  /* 0x3e4 m_pbCompVertexNormal */
    unsigned char** compressed_polygon_normals; /* 0x3e8 m_pbCompPolyNormal */
    unsigned char padding_3ec[4];
    W8GrowableVector<int> skin_table_ids;                         /* 0x3f0; count at 0x3f4 */
    W8GrowableVector<srPtr<srTextureIFace>*> skin_texture_tables; /* 0x400 */
    W8GrowableVector<char*> skin_table_names;                     /* 0x410 */
    W8GrowableVector<short> mapped_values;                        /* 0x420 */
    W8GrowableVector<short> mapped_keys;                          /* 0x430 */
    unsigned long last_decompress_release_tick_440;
    float vertex_compression_scale_444;
    /* m_pLerpBuffer: interpolation scratch for GetVertexLocations; an srHeap
       allocation that is not counted in g_decompressed_mesh_bytes. */
    srVector3T<float>* lerp_buffer_448;
    unsigned int* automap_polygons;     /* 0x44c */
    unsigned int automap_polygon_count; /* 0x450 */
    bool automap_filter_active;         /* 0x454 */
    unsigned char padding_455[3];
    W8GrowableVector<int*>* skin_blanking_apt_458;
    W8GrowableVector<int>* skin_blanking_apt_number_45c;
    W8GrowableVector<unsigned char>* skin_blanking_checked_460;
};

static_assert(sizeof(stMeshModel) == 0x464, "stMeshModel_size_must_be_0x464");

/* Every mesh model whose frame storage has been initialized. */
extern W8GrowableVector<stMeshModel*> g_mesh_models; /* 0x00659CB8 */
/* Bytes currently held by decompressed per-frame float caches. */
extern int g_decompressed_mesh_bytes; /* 0x0065A0E8 */
/* Scratch active-polygon indices filled by software backface cull in
   RenderTriMeshWithEquations when an equation table is supplied. */
extern srHeapArray<unsigned long> g_software_cull_active_polygons; /* 0x00659CE0 */

/* True when all three components of the vector are zero; the vertex-lighting
   code uses it to decide between a plain copy and a per-vertex offset. */
int __fastcall IsZeroVector(const srVector3T<float>* vector);
/* Copy `count` dwords when the buffers differ. Callers pass 3*n for vec3
   arrays. */
void CopyDwordBuffer(void* destination, const void* source, int count);
/* Fill `count` dwords with `value`. FUN_00472270 uses this when a vec3's
   components are equal, passing vertex_count*3. */
void FillDwordBuffer00474700(void* destination, unsigned int value, int count);
/* Plain dword walk used by RenderTriMeshWithEquations's active-poly
   scratch resize and by stMeshModel clone's sunlight table copy. */
void CopyUlongBuffer(unsigned long* destination, const unsigned long* source, int count);
/* dest[i] += source[i] for `count` floats. Callers pass vertex_count*3. */
void AddFloatBuffer(float* destination, const float* source, int count);
/* dest[i] = source[i] + offset for `count` vectors, or a plain copy when the
   offset is zero. */
void OffsetVertices(srVector3T<float>* destination, const srVector3T<float>* source,
                    const srVector3T<float>* offset, int count);
/* Release least-recently-used decompressed frame caches until `needed` bytes
   have been freed; 0 when the registry cannot supply them. */
unsigned char ReclaimDecompressedBytes(unsigned int needed);
