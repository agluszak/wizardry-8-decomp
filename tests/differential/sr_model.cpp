/* Differential cases for the geometry classes: srModeler (shape builders,
   transforms, tesselation, smoothing, UV mapping, polygon triangulation,
   convert), srMeshModel (tri-mesh normals, bounds, radius and transform
   helpers), srNode transforms and srBounder over srModelInstance children. */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "surrender/srBounder.h"
#include "surrender/srMaterial.h"
#include "surrender/srMath.h"
#include "surrender/srMeshModel.h"
#include "surrender/srModelInstance.h"
#include "surrender/srModeler.h"
#include "surrender/srNode.h"
#include "surrender/srVertexPipe.h"
#include "surrender/srVectorProcessor.h"
#include "surrender/srVertexProcessor.h"

void runProbe(const char* name, void (*probe)());

static unsigned long g_model_rng = 1;
static char g_model_name[96];
static int g_model_variant;

static unsigned long modelRandom()
{
    g_model_rng ^= g_model_rng << 13;
    g_model_rng ^= g_model_rng >> 17;
    g_model_rng ^= g_model_rng << 5;
    return g_model_rng;
}

/* Uniform in [-range, range) with 1/4096 steps so values are exact floats. */
static float modelFloat(float range)
{
    long step = static_cast<long>(modelRandom() & 0x1fff) - 0x1000;
    return range * static_cast<float>(step) / 4096.0f;
}

static unsigned long floatBits(float value)
{
    unsigned long bits;
    memcpy(&bits, &value, 4);
    return bits;
}

static void doubleBits(double value, unsigned long words[2])
{
    memcpy(words, &value, 8);
}

static void runModel(const char* family, const char* label, int variant, void (*probe)())
{
    const char* cursor;
    sprintf(g_model_name, "%s.%s", family, label);
    g_model_rng = 0x811c9dc5UL;
    for (cursor = g_model_name; *cursor != 0; ++cursor) {
        g_model_rng = (g_model_rng ^ static_cast<unsigned char>(*cursor)) * 0x01000193UL;
    }
    if (g_model_rng == 0) {
        g_model_rng = 1;
    }
    g_model_variant = variant;
    runProbe(g_model_name, probe);
}

static unsigned long fnv(unsigned long sum, const void* data, unsigned long bytes)
{
    const unsigned char* cursor = static_cast<const unsigned char*>(data);
    unsigned long index;
    for (index = 0; index < bytes; ++index) {
        sum = (sum ^ cursor[index]) * 16777619UL;
    }
    return sum;
}

static void printDouble(const char* label, double value)
{
    unsigned long words[2];
    doubleBits(value, words);
    printf("%s %08lx%08lx %.17g\n", label, words[1], words[0], value);
}

static void printVector3(const char* label, const srVector3T<float>& value)
{
    printf("%s %08lx %08lx %08lx\n", label, floatBits(value.x), floatBits(value.y),
           floatBits(value.z));
}

/* Arrays print one line per element up to a cap, then a hash of the whole. */
static void printFloats(const char* label, const float* values, unsigned long elements,
                        unsigned long width)
{
    unsigned long index;
    unsigned long lines = elements / width;
    if (values == 0) {
        printf("%s null\n", label);
        return;
    }
    for (index = 0; index < lines && index < 48; ++index) {
        unsigned long column;
        printf("%s[%lu]", label, index);
        for (column = 0; column < width; ++column) {
            printf(" %08lx", floatBits(values[index * width + column]));
        }
        printf("\n");
    }
    printf("%s count %lu fnv %08lx\n", label, lines, fnv(2166136261UL, values, elements * 4));
}

static srVector3T<float> randomVector(float range)
{
    srVector3T<float> value(modelFloat(range), modelFloat(range), modelFloat(range));
    return value;
}

static void randomRotation(srMatrix3T<float>& matrix)
{
    srVector3T<float> axis(modelFloat(1.0f), modelFloat(1.0f), modelFloat(1.0f) + 1.5f);
    double angle = modelFloat(3.0f);
    matrix.SetIdentity();
    matrix.RotateAroundAxis(angle, axis);
}

/* ---- srModeler ---- */

static void vertexHash(const srModeler::Vertex& vertex, unsigned long& sum)
{
    sum = fnv(sum, &vertex.position, sizeof(vertex.position));
    sum = fnv(sum, &vertex.shade_index, sizeof(vertex.shade_index));
    sum = fnv(sum, vertex.dcg, sizeof(vertex.dcg));
    sum = fnv(sum, vertex.dig, sizeof(vertex.dig));
    sum = fnv(sum, vertex.scg, sizeof(vertex.scg));
    sum = fnv(sum, vertex.uv, sizeof(vertex.uv));
    sum = fnv(sum, vertex.weights, sizeof(vertex.weights));
}

static void dumpModeler(const char* label, srModeler& modeler)
{
    unsigned long count = modeler.getTriangleCount();
    unsigned long index;
    unsigned long total = 2166136261UL;
    srModeler::Triangle* triangle = new srModeler::Triangle;
    printf("%s triangles %lu enabled %lu passes %ld\n", label, count,
           modeler.getEnabledTriangleCount(), modeler.getPassCount());
    for (index = 0; index < count; ++index) {
        unsigned long sum = 2166136261UL;
        int vertex;
        int found = modeler.getTriangle(index, *triangle);
        for (vertex = 0; vertex < 3; ++vertex) {
            vertexHash(triangle->vertices[vertex], sum);
        }
        sum = fnv(sum, &triangle->flags, sizeof(triangle->flags));
        sum = fnv(sum, &triangle->disabled, sizeof(triangle->disabled));
        sum = fnv(sum, triangle->shaders, sizeof(triangle->shaders));
        total = fnv(total, &sum, sizeof(sum));
        if (index < 40) {
            const srModeler::Vertex* v = triangle->vertices;
            printf("%s t%lu %d %08lx flags %08lx dis %lu p %08lx %08lx %08lx | %08lx %08lx %08lx | "
                   "%08lx %08lx %08lx uv %08lx %08lx\n",
                   label, index, found, sum, triangle->flags, triangle->disabled,
                   floatBits(v[0].position.x), floatBits(v[0].position.y),
                   floatBits(v[0].position.z), floatBits(v[1].position.x),
                   floatBits(v[1].position.y), floatBits(v[1].position.z),
                   floatBits(v[2].position.x), floatBits(v[2].position.y),
                   floatBits(v[2].position.z), floatBits(v[0].uv[0].x), floatBits(v[0].uv[0].y));
        }
    }
    printf("%s all %08lx\n", label, total);
    delete triangle;
}

static void dumpModelerQueries(const char* label, srModeler& modeler)
{
    char text[64];
    int axis;
    printf("%s unique %lu\n", label, modeler.getUniqueVertexCount());
    sprintf(text, "%s max-dist", label);
    printDouble(text, modeler.getMaxVertexDist());
    for (axis = 0; axis < 3; ++axis) {
        float minimum = 12345.0f;
        float maximum = -12345.0f;
        modeler.getAxialBounds(static_cast<srModeler::e_axis>(axis), minimum, maximum);
        printf("%s axis %d %08lx %08lx\n", label, axis, floatBits(minimum), floatBits(maximum));
    }
}

static void buildShape(srModeler& modeler, int shape)
{
    switch (shape % 4) {
    case 0:
        modeler.createSphere(3 + (shape / 4) % 8);
        break;
    case 1:
        modeler.createTorus(3 + (shape / 4) % 6, 3 + (shape / 8) % 5, 0.15 + 0.1 * (shape % 3));
        break;
    case 2:
        modeler.createGrid(1 + (shape / 4) % 5, 1 + (shape / 8) % 4);
        break;
    default:
        modeler.createSphere(4);
        modeler.scale(srVector3T<float>(1.0f, 0.5f, 2.0f));
        break;
    }
}

static void modelerShapes()
{
    srModeler modeler;
    int detail = g_model_variant;
    if (detail < 40) {
        modeler.createSphere(detail % 20);
    } else if (detail < 70) {
        int index = detail - 40;
        modeler.createTorus(index % 6 + 1, index / 6 + 1, 0.05 * (index % 7) + 0.1);
    } else {
        int index = detail - 70;
        modeler.createGrid(index % 5, index / 5);
    }
    dumpModeler("shape", modeler);
    dumpModelerQueries("shape", modeler);
}

static void modelerTransforms()
{
    srModeler modeler;
    srMatrix3T<float> rotation;
    unsigned long count;
    int step;
    buildShape(modeler, g_model_variant);
    count = modeler.getTriangleCount();
    for (step = 0; step < 6; ++step) {
        unsigned long which = count != 0 ? modelRandom() % count : 0;
        switch (modelRandom() % 6) {
        case 0:
            modeler.scale(randomVector(3.0f));
            break;
        case 1:
            modeler.scale(which, randomVector(3.0f));
            break;
        case 2:
            modeler.move(randomVector(4.0f));
            break;
        case 3:
            modeler.move(which, randomVector(4.0f));
            break;
        case 4:
            randomRotation(rotation);
            modeler.rotate(rotation);
            break;
        default:
            randomRotation(rotation);
            modeler.rotate(which, rotation);
            break;
        }
    }
    dumpModeler("xform", modeler);
    dumpModelerQueries("xform", modeler);
    for (step = 0; step < 6; ++step) {
        srVector3T<float> point = randomVector(2.0f);
        unsigned long triangle = 0xdead;
        unsigned long vertex = 0xbeef;
        srModeler::Triangle* existing = new srModeler::Triangle;
        int found;
        modeler.findClosestVertex(point, triangle, vertex);
        printf("closest %lu %lu\n", triangle, vertex);
        if (count != 0 && modeler.getTriangle(triangle, *existing) && vertex < 3) {
            unsigned long t2 = 0xdead;
            unsigned long v2 = 0xbeef;
            found = modeler.findVertex(existing->vertices[vertex].position, t2, v2,
                                       modelRandom() % (triangle + 1));
            printf("find %d %lu %lu\n", found, t2, v2);
        }
        found = modeler.findVertex(point, triangle, vertex, 0);
        printf("find-random %d\n", found);
        delete existing;
    }
}

/* Repeated rotations isolate the 3x3 transform's summation order. */
static void modelerRotate()
{
    srModeler modeler;
    srMatrix3T<float> rotation;
    int step;
    buildShape(modeler, g_model_variant);
    modeler.scale(randomVector(4.0f));
    for (step = 0; step < 4; ++step) {
        randomRotation(rotation);
        rotation.vectors[step % 3].y *= 1.25f;
        if ((g_model_variant & 1) != 0 && modeler.getTriangleCount() != 0) {
            unsigned long index;
            for (index = 0; index < modeler.getTriangleCount(); ++index) {
                modeler.rotate(index, rotation);
            }
        } else {
            modeler.rotate(rotation);
        }
    }
    dumpModeler("rotate", modeler);
}

static void meshApply()
{
    srModeler modeler;
    srMeshModel* mesh = new srMeshModel(0, 0);
    srMatrix3T<float> rotation;
    int step;
    buildShape(modeler, g_model_variant);
    modeler.scale(randomVector(4.0f));
    modeler.convert(*mesh, 0);
    for (step = 0; step < 4; ++step) {
        randomRotation(rotation);
        rotation.vectors[step % 3].z *= 0.75f;
        mesh->applyMatrix(rotation);
    }
    printFloats("apply loc", &mesh->getVertexLoc()->x, mesh->getVertexCount() * 3, 3);
    mesh->release();
}

static void modelerTesselate()
{
    srModeler modeler;
    double threshold;
    buildShape(modeler, g_model_variant / 3);
    threshold = 0.05 + 0.15 * (g_model_variant % 3) + modelFloat(0.02f);
    if ((g_model_variant & 4) != 0 && modeler.getTriangleCount() != 0) {
        modeler.tesselateEdges(modelRandom() % modeler.getTriangleCount(), threshold);
    } else {
        modeler.tesselateEdges(threshold);
    }
    dumpModeler("tess", modeler);
    dumpModelerQueries("tess", modeler);
}

static void dumpMesh(const char* label, srMeshModel& mesh);

static void modelerSmooth()
{
    srModeler modeler;
    srMeshModel* mesh;
    double threshold = 0.2 + 0.3 * (g_model_variant % 4);
    buildShape(modeler, g_model_variant / 2);
    if ((g_model_variant & 8) != 0) {
        srMatrix3T<float> rotation;
        randomRotation(rotation);
        modeler.rotate(rotation);
        modeler.scale(randomVector(2.0f));
    }
    dumpModeler("pre-smooth", modeler);
    modeler.autoSmooth(threshold, g_model_variant & 1);
    dumpModeler("smooth", modeler);
    mesh = new srMeshModel(0, 0);
    modeler.convert(*mesh, (g_model_variant >> 4) & 1);
    dumpMesh("smooth-mesh", *mesh);
    mesh->release();
}

static void modelerMapping()
{
    srModeler modeler;
    int kind = g_model_variant % 3;
    long pass = (g_model_variant / 3) % 2;
    long layer = (g_model_variant / 6) % 2;
    srModeler::MappingInfo mapping(static_cast<srModeler::e_axis>(modelRandom() % 3),
                                   static_cast<srModeler::e_axis>(modelRandom() % 3),
                                   modelFloat(4.0f), modelFloat(4.0f), modelFloat(1.0f),
                                   modelFloat(1.0f));
    buildShape(modeler, g_model_variant / 2);
    modeler.setPassCount(2);
    modeler.move(randomVector(1.0f));
    if (kind == 0) {
        modeler.planarMap(pass, layer, mapping);
    } else if (kind == 1) {
        modeler.planarMapAbsolute(pass, layer, mapping);
    } else {
        modeler.cylinderMap(pass, layer, mapping);
    }
    if (modeler.getTriangleCount() != 0) {
        srModeler::Triangle* triangle = new srModeler::Triangle;
        unsigned long index;
        unsigned long count = modeler.getTriangleCount();
        unsigned long sum = 2166136261UL;
        for (index = 0; index < count; ++index) {
            int vertex;
            modeler.getTriangle(index, *triangle);
            for (vertex = 0; vertex < 3; ++vertex) {
                const srVector2T<float>& uv = triangle->vertices[vertex].uv[pass * 2 + layer];
                sum = fnv(sum, &uv, sizeof(uv));
                if (index < 24) {
                    printf("uv t%lu v%d %08lx %08lx\n", index, vertex, floatBits(uv.x),
                           floatBits(uv.y));
                }
            }
        }
        printf("uv all %08lx\n", sum);
        delete triangle;
    }
    dumpModeler("map", modeler);
    modeler.removeMapping(pass, layer);
    dumpModeler("unmapped", modeler);
}

static void modelerPolygon()
{
    srModeler modeler;
    int corners = 3 + g_model_variant % 10;
    srModeler::Polygon polygon(corners);
    int index;
    int shape = g_model_variant / 10;
    for (index = 0; index < corners; ++index) {
        double angle = 6.283185307179586 * index / corners;
        double radius = 1.0;
        if (shape == 1) {
            radius = (index & 1) != 0 ? 0.4 : 1.0; /* star: concave */
        } else if (shape == 2) {
            radius = 0.5 + (modelRandom() % 100) / 100.0;
        }
        srModeler::Vertex& vertex = polygon.vertices[index];
        vertex.position.x = static_cast<float>(cos(angle) * radius);
        vertex.position.y = static_cast<float>(sin(angle) * radius);
        vertex.position.z = shape == 3 ? modelFloat(0.25f) : 0.0f;
        vertex.uv[0].x = static_cast<float>(index);
        vertex.uv[0].y = static_cast<float>(corners - index);
    }
    if ((g_model_variant & 1) != 0) {
        /* Reverse the winding. */
        for (index = 0; index < corners / 2; ++index) {
            srVector3T<float> swap = polygon.vertices[index].position;
            polygon.vertices[index].position = polygon.vertices[corners - 1 - index].position;
            polygon.vertices[corners - 1 - index].position = swap;
        }
    }
    modeler.addPolygon(polygon);
    dumpModeler("polygon", modeler);
}

static void modelerManage()
{
    srModeler modeler;
    srModeler other;
    srModeler::Triangle* triangle = new srModeler::Triangle;
    srModeler::Vertex vertex;
    unsigned long count;
    unsigned long index;
    buildShape(modeler, g_model_variant);
    count = modeler.getTriangleCount();
    /* Collapse some triangles onto a shared corner or a shared position. */
    for (index = 0; index < count; index += 3 + g_model_variant % 4) {
        if (modeler.getTriangle(index, *triangle)) {
            vertex = triangle->vertices[0];
            modeler.setTriangleVertex(index, 1 + (index & 1), vertex);
        }
    }
    for (index = 1; index < count; index += 5) {
        modeler.disableTriangle(index);
    }
    if (count > 2) {
        modeler.enableTriangle(1);
        modeler.flipTriangle(2);
    }
    dumpModeler("edited", modeler);
    modeler.disableDegenerateTriangles();
    printf("enabled-after-degenerate %lu\n", modeler.getEnabledTriangleCount());
    modeler.flipTriangles();
    modeler.removeDisabledTriangles();
    dumpModeler("cleaned", modeler);
    buildShape(other, g_model_variant + 1);
    other.move(srVector3T<float>(3.0f, 0.0f, 0.0f));
    modeler.addFromModeler(other);
    dumpModeler("merged", modeler);
    dumpModelerQueries("merged", modeler);
    triangle->reset();
    triangle->vertices[1].position.x = 1.0f;
    triangle->vertices[2].position.y = 1.0f;
    index = modeler.addTriangle(*triangle);
    printf("added %lu\n", index);
    triangle->flipFacing();
    modeler.setTriangle(0, *triangle);
    modeler.setTriangleCount(modeler.getTriangleCount() / 2 + 1);
    dumpModeler("final", modeler);
    delete triangle;
}

/* ---- srMeshModel ---- */

static void dumpMesh(const char* label, srMeshModel& mesh)
{
    char text[64];
    long vertices = mesh.getVertexCount();
    long polygons = mesh.getPolygonCount();
    srVector3T<float> minimum(0.0f, 0.0f, 0.0f);
    srVector3T<float> maximum(0.0f, 0.0f, 0.0f);
    srVector3T<float> center(0.0f, 0.0f, 0.0f);
    float radius = 0.0f;
    int result;
    printf("%s vertices %ld polygons %ld passes %ld uv %ld active %ld\n", label, vertices, polygons,
           mesh.getPassCount(), mesh.getUVCount(), mesh.getActivePolygonCount());
    sprintf(text, "%s loc", label);
    printFloats(text, &mesh.getVertexLoc()->x, vertices * 3, 3);
    if (mesh.getPolyVertex() != 0) {
        const srVector3i* indices = mesh.getPolyVertex();
        long index;
        for (index = 0; index < polygons && index < 32; ++index) {
            printf("%s poly[%ld] %d %d %d\n", label, index, indices[index].x, indices[index].y,
                   indices[index].z);
        }
        printf("%s poly fnv %08lx\n", label,
               fnv(2166136261UL, indices, polygons * sizeof(srVector3i)));
    }
    if (mesh.getVertexShadeIndex(0) != 0) {
        printf("%s shade fnv %08lx\n", label,
               fnv(2166136261UL, mesh.getVertexShadeIndex(0), vertices * 4));
    }
    result = mesh.getBoundingBox(minimum, maximum);
    printf("%s box %d\n", label, result);
    sprintf(text, "%s box-min", label);
    printVector3(text, minimum);
    sprintf(text, "%s box-max", label);
    printVector3(text, maximum);
    result = mesh.getBoundingSphere(center, radius);
    sprintf(text, "%s sphere", label);
    printf("%s %d radius %08lx\n", text, result, floatBits(radius));
    printVector3(text, center);
    {
        const srMeshModel::TriMesh& tri = mesh.getTriMesh();
        printf("%s tri %ld %ld %ld control %08lx active %ld\n", label, tri.vertex_count,
               tri.polygon_count, tri.pass_count, tri.control_flags, tri.active_polygon_count);
        sprintf(text, "%s normal", label);
        printFloats(text, tri.normals != 0 ? &tri.normals->x : 0, tri.vertex_count * 3, 3);
        sprintf(text, "%s poly-eq", label);
        printFloats(text, tri.poly_equations != 0 ? &tri.poly_equations->x : 0,
                    tri.polygon_count * 4, 4);
        sprintf(text, "%s tri-bounds", label);
        printFloats(text, &tri.bounds_minimum.x, 10, 10);
        if (tri.texcoords[0][0] != 0) {
            sprintf(text, "%s uv00", label);
            printFloats(text, &tri.texcoords[0][0]->x, tri.vertex_count * 2, 2);
        }
        if (tri.dcg[0] != 0) {
            sprintf(text, "%s dcg0", label);
            printFloats(text, &tri.dcg[0]->x, tri.vertex_count * 4, 4);
        }
    }
    sprintf(text, "%s avg-radius", label);
    printDouble(text, mesh.getAverageRadius());
    sprintf(text, "%s max-radius", label);
    printDouble(text, mesh.getMaxRadius());
}

static void meshConvert()
{
    srModeler modeler;
    srMeshModel* mesh = new srMeshModel(0, 0);
    int remove_degenerate = g_model_variant & 1;
    buildShape(modeler, g_model_variant / 2);
    modeler.setPassCount(1 + (g_model_variant / 2) % 2);
    modeler.planarMap(0, 0, srModeler::MappingInfo());
    if ((g_model_variant & 2) != 0) {
        modeler.autoSmooth(0.5, 1);
    }
    modeler.move(randomVector(1.0f));
    modeler.convert(*mesh, remove_degenerate);
    dumpMesh("mesh", *mesh);
    /* Convert again into the populated mesh with the other flag. */
    modeler.scale(srVector3T<float>(0.5f, 0.5f, 0.5f));
    modeler.convert(*mesh, !remove_degenerate);
    dumpMesh("mesh-again", *mesh);
    mesh->release();
}

static void meshOperations()
{
    srModeler modeler;
    srMeshModel* mesh = new srMeshModel(0, 0);
    srMatrix3T<float> rotation;
    int step;
    buildShape(modeler, g_model_variant);
    modeler.move(randomVector(2.0f));
    modeler.convert(*mesh, 0);
    for (step = 0; step < 5; ++step) {
        int operation = static_cast<int>(modelRandom() % 8);
        printf("op %d\n", operation);
        switch (operation) {
        case 0:
            mesh->centerVertices();
            break;
        case 1:
            randomRotation(rotation);
            rotation.vectors[0].x *= 1.5f;
            mesh->applyMatrix(rotation);
            break;
        case 2:
            mesh->scale(randomVector(3.0f));
            break;
        case 3:
            mesh->relocateVertices(randomVector(2.0f));
            break;
        case 4:
            mesh->flipFaces();
            break;
        case 5:
            mesh->scaleToMaxRadius(0.5 + (modelRandom() % 100) / 50.0);
            break;
        case 6:
            mesh->scaleToAverageRadius(0.5 + (modelRandom() % 100) / 50.0);
            break;
        default: {
            srVector3T<float> point = randomVector(2.0f);
            printf("closest %ld\n", mesh->findClosestVertex(point));
            break;
        }
        }
        printf("after-op loc fnv %08lx\n",
               fnv(2166136261UL, mesh->getVertexLoc(), mesh->getVertexCount() * 12));
    }
    dumpMesh("ops", *mesh);
    mesh->release();
}

/* ---- srNode ---- */

static void printNode(const char* label, srNode& node)
{
    srMatrix4T<double> world;
    srMatrix4T<float> world_float;
    srMatrix3T<double> rotation;
    srVector3T<double> vector;
    int row;
    node.getWorldSpaceMatrix(world);
    for (row = 0; row < 4; ++row) {
        unsigned long words[8];
        memcpy(words, &world.vectors[row], 32);
        printf("%s world[%d] %08lx%08lx %08lx%08lx %08lx%08lx %08lx%08lx\n", label, row, words[1],
               words[0], words[3], words[2], words[5], words[4], words[7], words[6]);
    }
    node.getWorldSpaceMatrix(world_float);
    printf("%s world-float fnv %08lx\n", label,
           fnv(2166136261UL, &world_float, sizeof(world_float)));
    node.getRotation(rotation);
    printf("%s rotation fnv %08lx\n", label, fnv(2166136261UL, &rotation, sizeof(rotation)));
    vector = node.getLocation();
    printf("%s location fnv %08lx\n", label, fnv(2166136261UL, &vector, sizeof(vector)));
    vector = node.getScale();
    printf("%s scale fnv %08lx\n", label, fnv(2166136261UL, &vector, sizeof(vector)));
    vector = node.getWorldSpaceLocation();
    printf("%s ws-location fnv %08lx\n", label, fnv(2166136261UL, &vector, sizeof(vector)));
    vector = node.getWorldSpaceScale();
    printf("%s ws-scale fnv %08lx\n", label, fnv(2166136261UL, &vector, sizeof(vector)));
    vector = node.getWorldSpaceDOF();
    printf("%s ws-dof fnv %08lx\n", label, fnv(2166136261UL, &vector, sizeof(vector)));
    node.getWorldSpaceRotation(rotation);
    printf("%s ws-rotation fnv %08lx\n", label, fnv(2166136261UL, &rotation, sizeof(rotation)));
}

static srVector3T<double> randomDouble3(float range)
{
    srVector3T<double> value(modelFloat(range), modelFloat(range), modelFloat(range));
    return value;
}

static void nodeOperation(srNode& node, srNode& other, int operation)
{
    double amount = modelFloat(2.0f);
    srVector3T<double> vector = randomDouble3(3.0f);
    printf("node-op %d\n", operation);
    switch (operation) {
    case 0:
        node.setLocation(vector);
        break;
    case 1:
        node.move(vector);
        break;
    case 2:
        node.rotate(amount, srVector3T<double>(vector.x, vector.y, vector.z + 4.0));
        break;
    case 3:
        node.rotateX(amount);
        break;
    case 4:
        node.rotateY(amount);
        break;
    case 5:
        node.rotateZ(amount);
        break;
    case 6:
        node.setScale(srVector3T<double>(1.0 + amount * 0.25, 1.5, 0.75));
        break;
    case 7:
        node.setScale(1.0 + amount * 0.25);
        break;
    case 8:
        node.moveForward(amount);
        node.moveLeft(amount * 0.5);
        node.moveUp(amount * 0.25);
        break;
    case 9:
        node.moveBackward(amount);
        node.moveRight(amount * 0.5);
        node.moveDown(amount * 0.25);
        break;
    case 10:
        node.pitchAt(vector, 0.25 + 0.25 * (modelRandom() % 4));
        break;
    case 11:
        node.yawAt(vector, 0.25 + 0.25 * (modelRandom() % 4));
        break;
    case 12:
        node.rollAt(vector, 0.25 + 0.25 * (modelRandom() % 4));
        break;
    case 13:
        node.setRotation(vector.x, vector.y, vector.z);
        break;
    case 14:
        node.setRotation(srVector3T<double>(vector.x, vector.y, vector.z + 4.0), amount);
        break;
    case 15:
        node.setRotation(amount, srVector3T<double>(vector.x + 4.0, vector.y, vector.z));
        break;
    case 16:
        node.offsetLocation(vector.x, vector.y, vector.z);
        break;
    case 17:
        node.setWorldSpaceLocation(vector);
        break;
    case 18:
        node.rollUp(amount);
        break;
    default:
        node.yawAt(&other, 0.5);
        break;
    }
}

static void nodeStep(srNode& node, srNode& other)
{
    nodeOperation(node, other, static_cast<int>(modelRandom() % 20));
    {
        srMatrix3T<double> rotation;
        srVector3T<double> location = node.getLocation();
        srVector3T<double> scale = node.getScale();
        node.getRotation(rotation);
        printf("after node-op rotation %08lx location %08lx scale %08lx\n",
               fnv(2166136261UL, &rotation, sizeof(rotation)),
               fnv(2166136261UL, &location, sizeof(location)),
               fnv(2166136261UL, &scale, sizeof(scale)));
    }
}

/* One operation on a node whose parent and rotation are already non-trivial;
   prints the full local rotation and location afterwards. */
static void nodeSingleOperation()
{
    srNode* parent = new srNode(0);
    srNode* node = new srNode(parent);
    srNode* other = new srNode(0);
    srMatrix3T<double> rotation;
    srVector3T<double> location;
    int row;
    parent->setRotation(modelFloat(2.0f), modelFloat(2.0f), modelFloat(2.0f));
    parent->setLocation(modelFloat(3.0f), modelFloat(3.0f), modelFloat(3.0f));
    node->setRotation(modelFloat(2.0f), modelFloat(2.0f), modelFloat(2.0f));
    node->setLocation(modelFloat(3.0f), modelFloat(3.0f), modelFloat(3.0f));
    other->setLocation(modelFloat(3.0f), modelFloat(3.0f), modelFloat(3.0f));
    nodeOperation(*node, *other, g_model_variant % 20);
    node->getRotation(rotation);
    for (row = 0; row < 3; ++row) {
        unsigned long words[6];
        memcpy(words, &rotation.vectors[row], 24);
        printf("rotation[%d] %08lx%08lx %08lx%08lx %08lx%08lx\n", row, words[1], words[0], words[3],
               words[2], words[5], words[4]);
    }
    location = node->getLocation();
    {
        unsigned long words[6];
        memcpy(words, &location, 24);
        printf("location %08lx%08lx %08lx%08lx %08lx%08lx\n", words[1], words[0], words[3],
               words[2], words[5], words[4]);
    }
}

static void printDoubleRow(const char* label, int row, const double* values, int count)
{
    int index;
    printf("%s[%d]", label, row);
    for (index = 0; index < count; ++index) {
        unsigned long words[2];
        memcpy(words, &values[index], 8);
        printf(" %08lx%08lx", words[1], words[0]);
    }
    printf("\n");
}

/* World-space rotation and matrix accessors on a node under a rotated,
   translated and (for odd variants) scaled parent. */
static void nodeWorldSpace()
{
    srNode* parent = new srNode(0);
    srNode* node = new srNode(parent);
    srNode* other = new srNode(0);
    srMatrix3T<double> rotation;
    srMatrix4T<double> matrix;
    int row;
    parent->setRotation(modelFloat(2.0f), modelFloat(2.0f), modelFloat(2.0f));
    parent->setLocation(modelFloat(3.0f), modelFloat(3.0f), modelFloat(3.0f));
    if (g_model_variant & 1) {
        parent->setScale(srVector3T<double>(1.25, 0.75, 1.5));
    }
    node->setRotation(modelFloat(2.0f), modelFloat(2.0f), modelFloat(2.0f));
    node->setLocation(modelFloat(3.0f), modelFloat(3.0f), modelFloat(3.0f));
    other->setRotation(modelFloat(2.0f), modelFloat(2.0f), modelFloat(2.0f));
    node->getWorldSpaceRotation(rotation);
    for (row = 0; row < 3; ++row) {
        printDoubleRow("ws-rotation", row, &rotation.vectors[row].x, 3);
    }
    node->getWorldSpaceMatrix(matrix);
    for (row = 0; row < 4; ++row) {
        printDoubleRow("ws-matrix", row, &matrix.vectors[row].x, 4);
    }
    node->setWorldSpaceRotation(rotation);
    node->getRotation(rotation);
    for (row = 0; row < 3; ++row) {
        printDoubleRow("roundtrip-rotation", row, &rotation.vectors[row].x, 3);
    }
    other->getRotation(rotation);
    node->setWorldSpaceRotation(rotation);
    node->getRotation(rotation);
    for (row = 0; row < 3; ++row) {
        printDoubleRow("set-rotation", row, &rotation.vectors[row].x, 3);
    }
    other->setLocation(modelFloat(3.0f), modelFloat(3.0f), modelFloat(3.0f));
    other->setScale(srVector3T<double>(0.5 + (g_model_variant & 3) * 0.25, 1.0, 1.25));
    other->getWorldSpaceMatrix(matrix);
    node->setWorldSpaceMatrix(matrix);
    node->getRotation(rotation);
    for (row = 0; row < 3; ++row) {
        printDoubleRow("set-matrix-rotation", row, &rotation.vectors[row].x, 3);
    }
    {
        srVector3T<double> location = node->getLocation();
        srVector3T<double> scale = node->getScale();
        printDoubleRow("set-matrix-location", 0, &location.x, 3);
        printDoubleRow("set-matrix-scale", 0, &scale.x, 3);
    }
}

static void nodeTransforms()
{
    srNode* root = new srNode(0);
    srNode* child = new srNode(root);
    srNode* grandchild = new srNode(child);
    srNode* loose = new srNode(0);
    int step;
    char text[32];
    root->setLocation(modelFloat(2.0f), modelFloat(2.0f), modelFloat(2.0f));
    for (step = 0; step < 4; ++step) {
        nodeStep(*root, *loose);
        nodeStep(*child, *loose);
        nodeStep(*grandchild, *root);
        nodeStep(*loose, *grandchild);
    }
    printNode("root", *root);
    printNode("child", *child);
    printNode("grandchild", *grandchild);
    sprintf(text, "distance");
    printDouble(text, grandchild->getDistance(*loose));
    printf("level %ld child-of %d\n", grandchild->getHierarchyLevel(),
           grandchild->isChildOf(*root));
    /* Reparent with and without preserving the world transform. */
    printf("setParent %d\n", grandchild->setParent(loose, g_model_variant & 1));
    printNode("reparented", *grandchild);
    {
        srMatrix4T<double> world;
        child->getWorldSpaceMatrix(world);
        loose->setWorldSpaceMatrix(world);
        printNode("ws-set", *loose);
    }
}

/* ---- srBounder / srModelInstance ---- */

static void printBounds(const char* label, const srNode::BoundInfo& bounds)
{
    printf("%s state %d min %08lx %08lx %08lx max %08lx %08lx %08lx c %08lx %08lx %08lx r %08lx\n",
           label, bounds.state, floatBits(bounds.minimum.x), floatBits(bounds.minimum.y),
           floatBits(bounds.minimum.z), floatBits(bounds.maximum.x), floatBits(bounds.maximum.y),
           floatBits(bounds.maximum.z), floatBits(bounds.center.x), floatBits(bounds.center.y),
           floatBits(bounds.center.z), floatBits(bounds.radius));
}

static void bounderCase()
{
    srBounder* bounder = new srBounder(0);
    srMeshModel* meshes[3];
    srModelInstance* instances[4];
    srNode* group = new srNode(bounder);
    srNode::BoundInfo bounds;
    int index;
    int count = 1 + g_model_variant % 4;
    for (index = 0; index < 3; ++index) {
        srModeler modeler;
        buildShape(modeler, g_model_variant + index);
        modeler.scale(randomVector(2.0f));
        modeler.move(randomVector(1.0f));
        meshes[index] = new srMeshModel(0, 0);
        modeler.convert(*meshes[index], 0);
    }
    bounder->setLocation(modelFloat(1.0f), modelFloat(1.0f), modelFloat(1.0f));
    group->rotate(modelFloat(3.0f), srVector3T<double>(modelFloat(1.0f), 1.0, modelFloat(1.0f)));
    group->setScale(1.0 + modelFloat(0.5f));
    for (index = 0; index < count; ++index) {
        srNode* parent = (index & 1) != 0 ? group : static_cast<srNode*>(bounder);
        instances[index] = new srModelInstance(parent);
        instances[index]->setModel(meshes[index % 3]);
        instances[index]->setLocation(randomDouble3(3.0f));
        instances[index]->rotate(modelFloat(3.0f), srVector3T<double>(1.0, modelFloat(1.0f), 0.5));
        if ((g_model_variant & 4) != 0) {
            instances[index]->setScale(srVector3T<double>(0.5, 1.25, 2.0));
        }
        memset(&bounds, 0, sizeof(bounds));
        instances[index]->getLocalBounds(bounds);
        printBounds("local", bounds);
    }
    memset(&bounds, 0, sizeof(bounds));
    bounder->forceUpdateBounds();
    bounder->getBounds(bounds);
    printBounds("forced", bounds);
    /* Move one child; updateBounds picks up the change. */
    instances[0]->move(randomDouble3(2.0f));
    bounder->updateBounds();
    memset(&bounds, 0, sizeof(bounds));
    bounder->getBounds(bounds);
    printBounds("updated", bounds);
    memset(&bounds, 0, sizeof(bounds));
    bounder->getLocalBounds(bounds);
    printBounds("bounder-local", bounds);
}

/* ---- srMaterial ---- */

/* srMaterial's constructor runs reset(); the probe builds the material over
   storage pre-filled with a pattern, so any field reset leaves alone shows up
   as the fill (0x00 zeroes fog_scale, which makes updateParms disable
   CHANNEL_FOG). Odd variants change the colours first so getMaterialInfo runs
   updateParms on edited parameters. */
static void materialResetCase()
{
    static const unsigned char fills[4] = {0x00, 0xcd, 0x3f, 0xff};
    static double storage[(sizeof(srMaterial) + 7) / 8];
    srVertexProcessor::MaterialInfo info;
    srMaterial* material;
    memset(storage, fills[(g_model_variant / 2) % 4], sizeof(storage));
    material = new (storage) srMaterial;
    if ((g_model_variant & 1) != 0) {
        srVector4T<float> color;
        color.Set(modelFloat(1.0f), modelFloat(1.0f), modelFloat(1.0f), modelFloat(1.0f));
        material->setDiffuse(color);
        color.Set(modelFloat(1.0f), modelFloat(1.0f), modelFloat(1.0f), modelFloat(1.0f));
        material->setAmbient(color);
    }
    memset(&info, 0xa5, sizeof(info));
    material->getMaterialInfo(info);
    printFloats("diffuse", &info.diffuse.x, 4, 4);
    printFloats("ambient", &info.ambient.x, 4, 4);
    printFloats("specular", &info.specular.x, 4, 4);
    printFloats("emissive", &info.emissive.x, 4, 4);
    printf("translucency %08lx shininess %08lx value_38 %08lx fog_scale %08lx disabled %08lx\n",
           floatBits(info.translucency), floatBits(info.shininess), floatBits(info.value_38),
           floatBits(info.fog_scale), info.disabled_channels);
}

/* ---- srEnvironmentMapper ---- */

/* process() reads only these srVertexPipe members (private; the mapper is a
   friend), so the probe fills a raw pipe image at the header offsets. */
static void callPipe(void* function, void* pipe, unsigned long* result)
{
    unsigned long value;
    __asm {
        push pipe
        xor ecx, ecx
        call function
        mov value, eax
    }
    *result = value;
}

/* Mirror of the private srVertexPipe::Scratch layout. */
struct PipeScratch {
    srVector3T<float> dir[0x40];
    srVector3T<float> normals[0x40];
    float dist[0x40];
    float z_dist[0x40];
    float depth_cue[0x40];
    float alpha[0x40];
    float fog[0x40];
    unsigned long flags;
};

static void environmentMapperCase()
{
    static PipeScratch scratch;
    static srVector2T<float> st[0x100];
    srVertexArray array;
    unsigned char pipe[sizeof(srVertexPipe)];
    unsigned long count = 1 + modelRandom() % 0x20;
    unsigned long sub_offset = modelRandom() % (0x40 - count + 1);
    unsigned long batch_base = modelRandom() % 0x40;
    unsigned long index;
    unsigned long result = 0;
    unsigned long sum = 2166136261UL;
    HMODULE module = GetModuleHandleA("sr.dll");
    void* process = reinterpret_cast<void*>(
        GetProcAddress(module, "?process@srEnvironmentMapper@@UAEXAAVsrVertexPipe@@@Z"));
    void* is_active = reinterpret_cast<void*>(
        GetProcAddress(module, "?isActive@srEnvironmentMapper@@UAEHAAVsrVertexPipe@@@Z"));
    memset(&scratch, 0, sizeof(scratch));
    memset(st, 0xa7, sizeof(st));
    memset(&array, 0, sizeof(array));
    memset(pipe, 0, sizeof(pipe));
    for (index = 0; index < 0x40; ++index) {
        srVector3T<float> normal = randomVector(1.0f);
        srVector3T<float> direction = randomVector(1.0f);
        int special = static_cast<int>(modelRandom() % 8);
        if ((g_model_variant & 1) != 0) {
            double length = sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
            if (length > 0.0) {
                normal.x = static_cast<float>(normal.x / length);
                normal.y = static_cast<float>(normal.y / length);
                normal.z = static_cast<float>(normal.z / length);
            }
        }
        if (special == 0) {
            /* Reflection lands on (0, 0, -1): rz is zero. */
            normal = srVector3T<float>(0.0f, 0.0f, 1.0f);
            direction = srVector3T<float>(0.0f, 0.0f, 1.0f);
        } else if (special == 1) {
            direction = normal;
        }
        scratch.dir[index] = direction;
        scratch.normals[index] = normal;
    }
    scratch.flags = 0x01 | 0x08; /* READY_EYE_DIRECTION | READY_EYE_NORMALS */
    array.st0 = st;
    *reinterpret_cast<PipeScratch**>(pipe + 0x00) = &scratch;
    *reinterpret_cast<srVertexArray**>(pipe + 0x78) = &array;
    *reinterpret_cast<unsigned long*>(pipe + 0x80) = batch_base;
    *reinterpret_cast<unsigned long*>(pipe + 0x84) = sub_offset;
    *reinterpret_cast<unsigned long*>(pipe + 0x88) = count;
    callPipe(is_active, pipe, &result);
    printf("active %lu count %lu offset %lu base %lu\n", result, count, sub_offset, batch_base);
    callPipe(process, pipe, &result);
    printf("lazy %08lx flags %08lx\n", *reinterpret_cast<unsigned long*>(pipe + 0x10),
           scratch.flags);
    for (index = 0; index < 0x100; ++index) {
        sum = fnv(sum, &st[index], sizeof(st[index]));
        if (index >= batch_base + sub_offset && index < batch_base + sub_offset + count) {
            printf("st[%lu] %08lx %08lx\n", index, floatBits(st[index].x), floatBits(st[index].y));
        }
    }
    printf("st fnv %08lx\n", sum);
}

/* srEnvironmentMapper::process with the eye-space scratch not ready, so it
   goes through srVertexPipe::setupEyeSpaceDirAndDist (eye-space locations ->
   direction and distance through the base vector processor) and/or
   setupEyeSpaceNormal (no normals -> (0, 0, -1); direct normals through
   _transform; indexed normals through _transformIndexed with the AVT). */
static void environmentMapperSetupCase()
{
    static PipeScratch scratch;
    static srVector2T<float> st[0x100];
    static srVector4T<float> locations[0x80];
    static srVector3T<float> normals[0x80];
    static unsigned long avt[0x40];
    srVertexArray array;
    srVertexPipe::Input input;
    srMatrix4T<float> normal_matrix;
    unsigned char pipe[sizeof(srVertexPipe)];
    int normal_mode = g_model_variant % 3;
    int ready_mode = (g_model_variant / 3) % 4;
    unsigned long count = 1 + modelRandom() % 0x20;
    unsigned long sub_offset = modelRandom() % (0x40 - count + 1);
    unsigned long batch_base = modelRandom() % 0x40;
    unsigned long batch_count = sub_offset + count;
    unsigned long index;
    unsigned long result = 0;
    unsigned long sum = 2166136261UL;
    HMODULE module = GetModuleHandleA("sr.dll");
    void* process = reinterpret_cast<void*>(
        GetProcAddress(module, "?process@srEnvironmentMapper@@UAEXAAVsrVertexPipe@@@Z"));
    srVP** exported_vp =
        reinterpret_cast<srVP**>(GetProcAddress(module, "?vp@srVectorProcessor@@0PAVsrVP@@A"));
    srVectorProcessor::initBaseVP();
    memset(&scratch, 0, sizeof(scratch));
    memset(st, 0xa7, sizeof(st));
    memset(&array, 0, sizeof(array));
    memset(&input, 0, sizeof(input));
    memset(pipe, 0, sizeof(pipe));
    for (index = 0; index < 0x80; ++index) {
        srVector3T<float> position = randomVector(4.0f);
        locations[index].Set(position.x, position.y, position.z - 6.0f, 1.0f);
        normals[index] = randomVector(1.0f);
    }
    for (index = 0; index < 0x40; ++index) {
        avt[index] = modelRandom() % 0x80;
        scratch.dir[index] = randomVector(1.0f);
        scratch.normals[index] = randomVector(1.0f);
    }
    for (index = 0; index < 4; ++index) {
        srVector3T<float> row = randomVector(1.5f);
        normal_matrix.vectors[index].Set(row.x, row.y, row.z, index == 3 ? 1.0f : 0.0f);
    }
    input.normals = normal_mode == 0 ? 0 : normals;
    input.direct_vertex_indices = normal_mode == 1;
    input.normal_matrix = &normal_matrix;
    scratch.flags = ready_mode == 1 ? 0x01 : ready_mode == 2 ? 0x08 : 0;
    array.st0 = st;
    *reinterpret_cast<PipeScratch**>(pipe + 0x00) = &scratch;
    *reinterpret_cast<srVertexPipe::Input**>(pipe + 0x6c) = &input;
    *reinterpret_cast<unsigned long**>(pipe + 0x70) = avt;
    *reinterpret_cast<srVertexArray**>(pipe + 0x78) = &array;
    *reinterpret_cast<srVector4T<float>**>(pipe + 0x7c) = locations;
    *reinterpret_cast<unsigned long*>(pipe + 0x80) = batch_base;
    *reinterpret_cast<unsigned long*>(pipe + 0x84) = sub_offset;
    *reinterpret_cast<unsigned long*>(pipe + 0x88) = count;
    *reinterpret_cast<unsigned long*>(pipe + 0x8c) = batch_count;
    *reinterpret_cast<srVP**>(pipe + 0x98) = exported_vp != 0 ? *exported_vp : 0;
    printf("setup normals %d ready %d count %lu offset %lu base %lu\n", normal_mode, ready_mode,
           count, sub_offset, batch_base);
    callPipe(process, pipe, &result);
    printf("lazy %08lx flags %08lx\n", *reinterpret_cast<unsigned long*>(pipe + 0x10),
           scratch.flags);
    for (index = 0; index < batch_count; ++index) {
        printf("eye[%lu] dir %08lx %08lx %08lx dist %08lx n %08lx %08lx %08lx\n", index,
               floatBits(scratch.dir[index].x), floatBits(scratch.dir[index].y),
               floatBits(scratch.dir[index].z), floatBits(scratch.dist[index]),
               floatBits(scratch.normals[index].x), floatBits(scratch.normals[index].y),
               floatBits(scratch.normals[index].z));
    }
    for (index = 0; index < 0x100; ++index) {
        sum = fnv(sum, &st[index], sizeof(st[index]));
        if (index >= batch_base + sub_offset && index < batch_base + sub_offset + count) {
            printf("st[%lu] %08lx %08lx\n", index, floatBits(st[index].x), floatBits(st[index].y));
        }
    }
    printf("st fnv %08lx\n", sum);
    srVectorProcessor::release();
}

void modelCases()
{
    int variant;
    char label[32];
    for (variant = 0; variant < 90; ++variant) {
        sprintf(label, "v%d", variant);
        runModel("modeler.shape", label, variant, modelerShapes);
    }
    for (variant = 0; variant < 32; ++variant) {
        sprintf(label, "v%d", variant);
        runModel("modeler.xform", label, variant, modelerTransforms);
    }
    for (variant = 0; variant < 32; ++variant) {
        sprintf(label, "v%d", variant);
        runModel("modeler.rotate", label, variant, modelerRotate);
    }
    for (variant = 0; variant < 48; ++variant) {
        sprintf(label, "v%d", variant);
        runModel("modeler.tesselate", label, variant, modelerTesselate);
    }
    for (variant = 0; variant < 32; ++variant) {
        sprintf(label, "v%d", variant);
        runModel("modeler.smooth", label, variant, modelerSmooth);
    }
    for (variant = 0; variant < 36; ++variant) {
        sprintf(label, "v%d", variant);
        runModel("modeler.map", label, variant, modelerMapping);
    }
    for (variant = 0; variant < 40; ++variant) {
        sprintf(label, "v%d", variant);
        runModel("modeler.polygon", label, variant, modelerPolygon);
    }
    for (variant = 0; variant < 16; ++variant) {
        sprintf(label, "v%d", variant);
        runModel("modeler.manage", label, variant, modelerManage);
    }
    for (variant = 0; variant < 24; ++variant) {
        sprintf(label, "v%d", variant);
        runModel("mesh.convert", label, variant, meshConvert);
    }
    for (variant = 0; variant < 32; ++variant) {
        sprintf(label, "v%d", variant);
        runModel("mesh.ops", label, variant, meshOperations);
    }
    for (variant = 0; variant < 24; ++variant) {
        sprintf(label, "v%d", variant);
        runModel("mesh.apply", label, variant, meshApply);
    }
    for (variant = 0; variant < 48; ++variant) {
        sprintf(label, "v%d", variant);
        runModel("node.xform", label, variant, nodeTransforms);
    }
    for (variant = 0; variant < 120; ++variant) {
        sprintf(label, "op%d.v%d", variant % 20, variant / 20);
        runModel("node.single", label, variant, nodeSingleOperation);
    }
    for (variant = 0; variant < 16; ++variant) {
        sprintf(label, "v%d", variant);
        runModel("node.worldspace", label, variant, nodeWorldSpace);
    }
    for (variant = 0; variant < 24; ++variant) {
        sprintf(label, "v%d", variant);
        runModel("bounder", label, variant, bounderCase);
    }
    for (variant = 0; variant < 8; ++variant) {
        sprintf(label, "v%d", variant);
        runModel("material.reset", label, variant, materialResetCase);
    }
    for (variant = 0; variant < 16; ++variant) {
        sprintf(label, "v%d", variant);
        runModel("envmap", label, variant, environmentMapperCase);
    }
    for (variant = 0; variant < 24; ++variant) {
        sprintf(label, "v%d", variant);
        runModel("envmap.setup", label, variant, environmentMapperSetupCase);
    }
}
