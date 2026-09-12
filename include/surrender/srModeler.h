#pragma once

#include "srMaterialIFace.h"
#include "srMath.h"
#include "srMeshModel.h"
#include "srShader.h"

class SR_DLL_IMPORT srModeler {
public:
    struct MappingInfo {
        unsigned long unknown_00;
        unsigned long unknown_04;
        unsigned long unknown_08;
        unsigned long unknown_0c;
        unsigned long unknown_10;
        unsigned long unknown_14;
    };

    class Vertex {
    public:
        Vertex();

    private:
        unsigned char unknown_00_[0x110];
    };

    class Polygon {
    public:
        Polygon(int vertices);
        ~Polygon();

    private:
        unsigned char unknown_00_[0x20];
        srShader shaders_20[4];
        Vertex* vertices_30;
        int vertex_count_34;
        unsigned long unknown_38_;
        unsigned long unknown_3c_;
        int capacity_40;
    };

    srModeler();
    virtual ~srModeler();
    void createGrid(long columns, long rows);
    void planarMap(long polygon, long layer, const MappingInfo& mapping);
    void scale(const srVector3T<float>& scale);
    void convert(srMeshModel& model, int preserve);
    void discard();
    void addPolygon(const Polygon& polygon);
    void setMaterial(srMaterialIFace* material, long polygon, srMeshModel::e_side side);
    void setShader(srShader shader, long pass);

private:
    unsigned char unknown_04_[0x10];
};

static_assert((sizeof(srModeler) == 0x14), "srModeler_must_be_0x14");
static_assert((sizeof(srModeler::MappingInfo) == 0x18), "srModeler_MappingInfo_must_be_0x18");
static_assert((sizeof(srModeler::Vertex) == 0x110), "srModeler_Vertex_must_be_0x110");
static_assert((sizeof(srModeler::Polygon) == 0x44), "srModeler_Polygon_must_be_0x44");
