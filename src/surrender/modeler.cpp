#include "surrender/srModeler.h"

#include "surrender/srDebug.h"
#include "surrender/srHeap.h"
#include "surrender/srTriangulator.h"

#include <math.h>
#include <string.h>

/* autoSmooth's worker: buckets triangle indices by deduplicated shade vertex, turns every triangle
   pair coincident at a vertex into a candidate edge weighted by the facing/material test, floods
   smooth-group bits through the smooth edges, then copies the assigned mask back into
   Triangle::flags. */
namespace {

class AutoSmoother {
public:
    AutoSmoother(srModeler::Triangle* triangles, unsigned long triangle_count,
                 srModeler::VertexHash* hash, double threshold, int smooth);
    ~AutoSmoother();
    void smooth();

    /* Per shade-group vertex: the triangles sharing that vertex, filled by a
       count/allocate/fill pass. */
    struct VertexEntry {
        long count;
        long fill;
        unsigned long* triangles;
    };

    /* One triangle-pair coincidence at a shared vertex: smooth is the edge
       test result, group the assigned smooth group (-1 until assigned). */
    struct Edge {
        int smooth;
        long group;
        unsigned long first;
        unsigned long second;
    };

    /* Per triangle: its edge list plus the assigned/blocked group masks;
       groups becomes the triangle's new flags. */
    struct TriangleEntry {
        TriangleEntry() : blocked(0), groups(0) {}

        long count;
        unsigned long* edges;
        long fill;
        unsigned long blocked;
        unsigned long groups;
    };

    int isSmooth(unsigned long first, unsigned long second, double cosine, int smooth);
    void markEdge(Edge* edge, long group);
    void assignGroups(long group);

    srModeler::Triangle* triangles;
    unsigned long triangle_count;
    srModeler::VertexHash* hash;
    long vertex_count;
    unsigned long edge_count;
    VertexEntry* vertices;
    Edge* edges;
    TriangleEntry* entries;
};

// FUNCTION: SURRENDER 0x100370B0
AutoSmoother::AutoSmoother(srModeler::Triangle* triangles, unsigned long triangle_count,
                           srModeler::VertexHash* hash, double threshold, int smooth)
    : triangles(triangles), triangle_count(triangle_count), hash(hash), vertex_count(0)
{
    unsigned long index;
    unsigned long vertex;
    /* The adjacency table is indexed by the representative shade index, not
       the unique ordinal: vertices duplicated across triangles share one. */
    for (index = 0; index < this->hash->unique_count; ++index) {
        if (vertex_count < this->hash->entries[index].shade_index) {
            vertex_count = this->hash->entries[index].shade_index;
        }
    }
    ++vertex_count;
    vertices = new VertexEntry[vertex_count];
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
    for (vertex = 0; vertex < vertex_count; ++vertex) {
        vertices[vertex].count = 0;
        vertices[vertex].fill = 0;
    }
    srModeler::VertexHash::Entry** entries = this->hash->table;
    for (index = 0; index < this->triangle_count; ++index) {
        ++vertices[entries[0]->shade_index].count;
        ++vertices[entries[1]->shade_index].count;
        ++vertices[entries[2]->shade_index].count;
        entries += 3;
    }
    for (vertex = 0; vertex < vertex_count; ++vertex) {
        vertices[vertex].triangles = new unsigned long[vertices[vertex].count];
    }
    entries = this->hash->table;
    for (index = 0; index < this->triangle_count; ++index) {
        VertexEntry* group = &vertices[entries[0]->shade_index];
        group->triangles[group->fill++] = index;
        group = &vertices[entries[1]->shade_index];
        group->triangles[group->fill++] = index;
        group = &vertices[entries[2]->shade_index];
        group->triangles[group->fill++] = index;
        entries += 3;
    }
    edge_count = 0;
    for (vertex = 0; vertex < vertex_count; ++vertex) {
        edge_count += (vertices[vertex].count - 1) * vertices[vertex].count / 2;
    }
    edges = new Edge[edge_count];
    double cosine = cos(threshold);
    unsigned long edge = 0;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
    /* Counted but never read. */
    long smooth_edges = 0;
#pragma clang diagnostic pop
    for (vertex = 0; vertex < vertex_count; ++vertex) {
        for (long i = 0; i < vertices[vertex].count - 1; ++i) {
            for (long j = i + 1; j < vertices[vertex].count; ++j) {
                edges[edge].group = -1;
                edges[edge].first = vertices[vertex].triangles[i];
                edges[edge].second = vertices[vertex].triangles[j];
                edges[edge].smooth =
                    isSmooth(edges[edge].first, edges[edge].second, cosine, smooth);
                if (edges[edge].smooth != 0) {
                    ++smooth_edges;
                }
                ++edge;
            }
        }
    }
#pragma clang diagnostic pop
    this->entries = new TriangleEntry[this->triangle_count];
    for (index = 0; index < this->triangle_count; ++index) {
        this->entries[index].count = 0;
    }
    for (edge = 0; edge < edge_count; ++edge) {
        ++this->entries[edges[edge].first].count;
        ++this->entries[edges[edge].second].count;
    }
    for (index = 0; index < this->triangle_count; ++index) {
        this->entries[index].groups = 0;
        this->entries[index].blocked = 0;
        this->entries[index].edges = new unsigned long[this->entries[index].count];
        this->entries[index].fill = 0;
    }
    for (edge = 0; edge < edge_count; ++edge) {
        TriangleEntry* first = &this->entries[edges[edge].first];
        first->edges[first->fill++] = edge;
        TriangleEntry* second = &this->entries[edges[edge].second];
        second->edges[second->fill++] = edge;
    }
}

// FUNCTION: SURRENDER 0x10037550
AutoSmoother::~AutoSmoother()
{
    for (unsigned long index = 0; index < triangle_count; ++index) {
        delete[] entries[index].edges;
    }
    delete[] entries;
    delete[] edges;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
    for (unsigned long vertex = 0; vertex < vertex_count; ++vertex) {
        delete[] vertices[vertex].triangles;
    }
#pragma clang diagnostic pop
    delete[] vertices;
}

// FUNCTION: SURRENDER 0x100375D0
int AutoSmoother::isSmooth(unsigned long first, unsigned long second, double cosine, int smooth)
{
    const srModeler::Triangle* first_triangle = &triangles[first];
    const srModeler::Triangle* second_triangle = &triangles[second];
    if (smooth == 0) {
        for (long pass = 0; pass < 4; ++pass) {
            if (first_triangle->shaders[pass].value != second_triangle->shaders[pass].value) {
                return 0;
            }
            for (long layer = 0; layer < 2; ++layer) {
                if (first_triangle->textures[pass][layer] !=
                    second_triangle->textures[pass][layer]) {
                    return 0;
                }
            }
        }
    }
    srVector3T<float> first_normal =
        CrossProduct(first_triangle->vertices[0].position - first_triangle->vertices[1].position,
                     first_triangle->vertices[2].position - first_triangle->vertices[1].position);
    srVector3T<float> second_normal =
        CrossProduct(second_triangle->vertices[0].position - second_triangle->vertices[1].position,
                     second_triangle->vertices[2].position - second_triangle->vertices[1].position);
    float magnitude = first_normal.Length() * second_normal.Length();
    if (0.0 < magnitude) {
        if (DotProduct(first_normal, second_normal) / magnitude <= cosine) {
            return 0;
        }
        return 1;
    }
    return 1;
}

// FUNCTION: SURRENDER 0x100378B0
void AutoSmoother::markEdge(Edge* edge, long group)
{
    edge->group = group;
    unsigned long bit = 1 << (group & 0x1f);
    entries[edge->first].groups |= bit;
    entries[edge->second].groups |= bit;
    long index;
    for (index = 0; index < entries[edge->first].count; ++index) {
        Edge* other = &edges[entries[edge->first].edges[index]];
        if (other->smooth == 0) {
            unsigned long triangle = other->first == edge->first ? other->second : other->first;
            entries[triangle].blocked |= bit;
        }
    }
    for (index = 0; index < entries[edge->second].count; ++index) {
        Edge* other = &edges[entries[edge->second].edges[index]];
        if (other->smooth == 0) {
            unsigned long triangle = other->first == edge->second ? other->second : other->first;
            entries[triangle].blocked |= bit;
        }
    }
}

// FUNCTION: SURRENDER 0x100379C0
void AutoSmoother::assignGroups(long group)
{
    if (group >= 0x1f) {
        srErr << "Warning: srModeler::autoSmooth() ran out of groups." << std::endl;
        for (unsigned long index = 0; index < edge_count; ++index) {
            if (edges[index].group == -1) {
                entries[edges[index].first].groups |= 0x80000000;
                entries[edges[index].second].groups |= 0x80000000;
                edges[index].group = 0x1f;
            }
        }
        return;
    }
    for (unsigned long index = 0; index < edge_count; ++index) {
        Edge* edge = &edges[index];
        if (edge->smooth != 0 && edge->group == -1) {
            ++group;
            markEdge(edge, group);
            /* Rescan from the seed edge: a smooth edge with no hard-edge
               neighbour blocking this group joins it immediately. */
            for (unsigned long scan = index; scan < edge_count; ++scan) {
                Edge* other = &edges[scan];
                if (other->smooth != 0 && other->group == -1) {
                    unsigned long bit = 1 << (group & 0x1f);
                    if ((entries[other->first].blocked & bit) == 0 &&
                        (entries[other->second].blocked & bit) == 0) {
                        markEdge(other, group);
                    }
                }
            }
            assignGroups(group);
        }
    }
}

// FUNCTION: SURRENDER 0x10037B20
void AutoSmoother::smooth()
{
    unsigned long flags = 0;
    for (unsigned long index = 0; index < triangle_count; ++index) {
        flags |= triangles[index].flags;
    }
    if (flags != 0) {
        /* The flood starts one below the highest set flag bit so the first
           smooth group reuses that slot. */
        unsigned long bit = 0;
        if ((flags & 0xffff0000) != 0) {
            bit = 0x10;
            flags >>= 0x10;
        }
        if ((flags & 0xff00) != 0) {
            bit += 8;
            flags >>= 8;
        }
        if ((flags & 0xf0) != 0) {
            bit += 4;
            flags >>= 4;
        }
        if ((flags & 0xc) != 0) {
            bit += 2;
            flags >>= 2;
        }
        if ((flags & 0x2) != 0) {
            bit += 1;
        }
        if (bit < 0x1f) {
            assignGroups(bit - 1);
            for (unsigned long index = 0; index < triangle_count; ++index) {
                triangles[index].flags = entries[index].groups;
            }
        }
    }
}

} // namespace

static const double pi = 3.141592653589793;

// FUNCTION: SURRENDER 0x10037BC0
// RECOMP: ??0Vertex@srModeler@@QAE@XZ
srModeler::Vertex::Vertex()
{
    reset();
}

// FUNCTION: SURRENDER 0x100386A0
void srModeler::Vertex::reset()
{
    position.SetZero();
    for (int pass = 0; pass < 4; ++pass) {
        dig[pass].SetZero();
        dcg[pass] = 1.0f;
        scg[pass] = 1.0f;
        weights[pass] = 1.0f;
        materials[pass][0] = 0;
        materials[pass][1] = 0;
        for (int layer = 0; layer < 2; ++layer) {
            uv[pass * 2 + layer].SetZero();
        }
    }
    shade_index = 0;
}

// FUNCTION: SURRENDER 0x10038240
int srModeler::Vertex::operator==(const Vertex& other) const
{
    if (!(position == other.position)) {
        return 0;
    }
    if (shade_index == other.shade_index) {
        for (int pass = 0; pass < 4; ++pass) {
            if (!(dcg[pass] == other.dcg[pass] && dig[pass] == other.dig[pass] &&
                  scg[pass] == other.scg[pass] && weights[pass] == other.weights[pass])) {
                return 0;
            }
            for (int side = 0; side < 2; ++side) {
                if (materials[pass][side] != other.materials[pass][side]) {
                    return 0;
                }
            }
            for (int layer = 0; layer < 2; ++layer) {
                if (uv[pass * 2 + layer].x != other.uv[pass * 2 + layer].x ||
                    uv[pass * 2 + layer].y != other.uv[pass * 2 + layer].y) {
                    return 0;
                }
            }
        }
        return 1;
    }
    return 0;
}

// FUNCTION: SURRENDER 0x10038680
int srModeler::Vertex::operator!=(const Vertex& other) const
{
    return !(*this == other);
}

// FUNCTION: SURRENDER 0x10038420
void srModeler::Vertex::interpolate(const Vertex& first, const Vertex& second, float fraction)
{
    position.x = (second.position.x - first.position.x) * fraction + first.position.x;
    position.y = (second.position.y - first.position.y) * fraction + first.position.y;
    position.z = (second.position.z - first.position.z) * fraction + first.position.z;
    for (int pass = 0; pass < 4; ++pass) {
        dig[pass].x = (second.dig[pass].x - first.dig[pass].x) * fraction + first.dig[pass].x;
        dig[pass].y = (second.dig[pass].y - first.dig[pass].y) * fraction + first.dig[pass].y;
        dig[pass].z = (second.dig[pass].z - first.dig[pass].z) * fraction + first.dig[pass].z;
        scg[pass].x = (second.scg[pass].x - first.scg[pass].x) * fraction + first.scg[pass].x;
        scg[pass].y = (second.scg[pass].y - first.scg[pass].y) * fraction + first.scg[pass].y;
        scg[pass].z = (second.scg[pass].z - first.scg[pass].z) * fraction + first.scg[pass].z;
        dcg[pass].x = (second.dcg[pass].x - first.dcg[pass].x) * fraction + first.dcg[pass].x;
        dcg[pass].y = (second.dcg[pass].y - first.dcg[pass].y) * fraction + first.dcg[pass].y;
        dcg[pass].z = (second.dcg[pass].z - first.dcg[pass].z) * fraction + first.dcg[pass].z;
        weights[pass] =
            (second.weights[pass] - first.weights[pass]) * fraction + first.weights[pass];
        for (int layer = 0; layer < 2; ++layer) {
            uv[pass * 2 + layer].x =
                (second.uv[pass * 2 + layer].x - first.uv[pass * 2 + layer].x) * fraction +
                first.uv[pass * 2 + layer].x;
            uv[pass * 2 + layer].y =
                (second.uv[pass * 2 + layer].y - first.uv[pass * 2 + layer].y) * fraction +
                first.uv[pass * 2 + layer].y;
        }
    }
    shade_index = 0;
}

// FUNCTION: SURRENDER 0x10038B50
srModeler::Triangle::Triangle() : flags(0)
{
    reset();
}

// FUNCTION: SURRENDER 0x10038AF0
void srModeler::Triangle::reset()
{
    for (int vertex = 0; vertex < 3; ++vertex) {
        vertices[vertex].reset();
    }
    for (int pass = 0; pass < 4; ++pass) {
        shaders[pass] = srShader();
        textures[pass][0] = 0;
        textures[pass][1] = 0;
    }
    flags = 0;
    flags |= 1;
    disabled = 0;
}

// FUNCTION: SURRENDER 0x10038840
void srModeler::Triangle::flipFacing()
{
    Vertex temporary;
    temporary = vertices[1];
    vertices[1] = vertices[2];
    vertices[2] = temporary;
}

// FUNCTION: SURRENDER 0x10038890
srModeler::Polygon::Polygon(int vertices)
{
    vertex_count = vertices;
    flags = 0;
    this->vertices = new Vertex[vertices];
    capacity = vertices;
    reset();
}

// FUNCTION: SURRENDER 0x10038990
srModeler::Polygon::~Polygon()
{
    delete[] vertices;
}

// FUNCTION: SURRENDER 0x100389A0
void srModeler::Polygon::reset()
{
    for (int vertex = 0; vertex < vertex_count; ++vertex) {
        vertices[vertex].reset();
    }
    for (int pass = 0; pass < 4; ++pass) {
        shaders[pass] = srShader();
        textures[pass][0] = 0;
        textures[pass][1] = 0;
    }
    flags = 0;
    flags |= 1;
    disabled = 0;
}

// FUNCTION: SURRENDER 0x10038A00
void srModeler::Polygon::reAllocate(int vertices)
{
    if (capacity < vertices) {
        delete[] this->vertices;
        this->vertices = new Vertex[vertices];
        capacity = vertices;
        reset();
    }
    vertex_count = vertices;
}

// FUNCTION: SURRENDER 0x1003BAE0
srModeler::srModeler()
{
    triangle_count = 0;
    pass_count = 1;
}

// FUNCTION: SURRENDER 0x1003BC70
srModeler::~srModeler() {}

// FUNCTION: SURRENDER 0x10039A70
void srModeler::discard()
{
    setTriangleCount(0);
    pass_count = 1;
}

// FUNCTION: SURRENDER 0x10037C00
unsigned long srModeler::getTriangleCount() const
{
    return triangle_count;
}

// FUNCTION: SURRENDER 0x1003A460
void srModeler::setTriangleCount(unsigned long triangles)
{
    triangle_count = triangles;
    this->triangles.setCapacity(triangles);
}

// FUNCTION: SURRENDER 0x1003A480
unsigned long srModeler::addTriangle(const Triangle& triangle)
{
    setTriangle(++triangle_count - 1, triangle);
    return triangle_count - 1;
}

// FUNCTION: SURRENDER 0x1003A330
int srModeler::getTriangle(unsigned long index, Triangle& triangle)
{
    if (triangle_count <= index) {
        return 0;
    }
    triangle = triangles[index];
    return 1;
}

// FUNCTION: SURRENDER 0x1003B0B0
void srModeler::setTriangle(unsigned long index, const Triangle& triangle)
{
    if (index < triangle_count) {
        triangles[index] = triangle;
    }
}

// FUNCTION: SURRENDER 0x10039CC0
void srModeler::setTriangleVertex(unsigned long triangle, unsigned long vertex, const Vertex& value)
{
    if (triangle < triangle_count && vertex < 3) {
        triangles[triangle].vertices[vertex] = value;
    }
}

// FUNCTION: SURRENDER 0x10039C80
void srModeler::flipTriangle(unsigned long triangle)
{
    if (triangle < triangle_count) {
        triangles[triangle].flipFacing();
    }
}

// FUNCTION: SURRENDER 0x10039C40
void srModeler::flipTriangles()
{
    Triangle* triangle = &triangles[0];
    for (unsigned long index = 0; index < triangle_count; ++index) {
        triangle->flipFacing();
        ++triangle;
    }
}

// FUNCTION: SURRENDER 0x1003A420
void srModeler::enableTriangle(unsigned long triangle)
{
    if (triangle < triangle_count) {
        triangles[triangle].disabled = 0;
    }
}

// FUNCTION: SURRENDER 0x1003A3E0
void srModeler::disableTriangle(unsigned long triangle)
{
    if (triangle < triangle_count) {
        triangles[triangle].disabled = 1;
    }
}

// FUNCTION: SURRENDER 0x1003A0F0
unsigned long srModeler::getEnabledTriangleCount()
{
    unsigned long enabled = 0;
    Triangle* triangle = &triangles[0];
    for (unsigned long index = triangle_count; index != 0; --index) {
        if (triangle->disabled == 0) {
            ++enabled;
        }
        ++triangle;
    }
    return enabled;
}

// FUNCTION: SURRENDER 0x1003A130
void srModeler::removeDisabledTriangles()
{
    unsigned long enabled = getEnabledTriangleCount();
    if (enabled != triangle_count) {
        unsigned long destination_index = 0;
        for (unsigned long index = 0; index < triangle_count; ++index) {
            if (triangles[index].disabled == 0) {
                if (destination_index != index) {
                    triangles[destination_index] = triangles[index];
                }
                ++destination_index;
            }
        }
        triangle_count = enabled;
    }
}

// FUNCTION: SURRENDER 0x1003A240
void srModeler::disableDegenerateTriangles()
{
    Triangle* triangle = &triangles[0];
    for (unsigned long index = 0; index < triangle_count; ++index) {
        if (triangle->vertices[0].position == triangle->vertices[1].position ||
            triangle->vertices[1].position == triangle->vertices[2].position ||
            triangle->vertices[0].position == triangle->vertices[2].position) {
            triangle->disabled = 1;
        }
        ++triangle;
    }
}

// FUNCTION: SURRENDER 0x10038D60
void srModeler::addFromModeler(srModeler& other)
{
    Triangle triangle;
    long count = other.triangle_count;
    for (long index = 0; index < count; ++index) {
        other.getTriangle(index, triangle);
        addTriangle(triangle);
    }
}

// FUNCTION: SURRENDER 0x10039E10
void srModeler::scale(const srVector3T<float>& scale)
{
    Triangle* triangle = &triangles[0];
    for (unsigned long index = 0; index < triangle_count; ++index) {
        for (int vertex = 0; vertex < 3; ++vertex) {
            triangle->vertices[vertex].position *= scale;
        }
        ++triangle;
    }
}

// FUNCTION: SURRENDER 0x10039F90
void srModeler::scale(unsigned long triangle, const srVector3T<float>& scale)
{
    if (triangle < triangle_count) {
        Triangle* element = &triangles[triangle];
        for (int index = 0; index < 3; ++index) {
            element->vertices[index].position *= scale;
        }
    }
}

// FUNCTION: SURRENDER 0x10039E90
void srModeler::move(const srVector3T<float>& delta)
{
    Triangle* triangle = &triangles[0];
    for (unsigned long index = 0; index < triangle_count; ++index) {
        for (int vertex = 0; vertex < 3; ++vertex) {
            triangle->vertices[vertex].position += delta;
        }
        ++triangle;
    }
}

// FUNCTION: SURRENDER 0x10039F10
void srModeler::move(unsigned long triangle, const srVector3T<float>& delta)
{
    if (triangle < triangle_count) {
        Triangle* element = &triangles[triangle];
        for (int index = 0; index < 3; ++index) {
            element->vertices[index].position += delta;
        }
    }
}

// FUNCTION: SURRENDER 0x10039D20
void srModeler::rotate(const srMatrix3T<float>& matrix)
{
    Triangle* triangle = &triangles[0];
    for (unsigned long index = 0; index < triangle_count; ++index) {
        for (int vertex = 0; vertex < 3; ++vertex) {
            triangle->vertices[vertex].position.Transform(matrix);
        }
        ++triangle;
    }
}

// FUNCTION: SURRENDER 0x1003A010
void srModeler::rotate(unsigned long triangle, const srMatrix3T<float>& matrix)
{
    if (triangle < triangle_count) {
        Triangle* element = &triangles[triangle];
        for (int index = 0; index < 3; ++index) {
            element->vertices[index].position = matrix.Transform(element->vertices[index].position);
        }
    }
}

// FUNCTION: SURRENDER 0x10039A90
int srModeler::findVertex(const srVector3T<float>& position, unsigned long& triangle,
                          unsigned long& vertex, unsigned long start_triangle)
{
    if (start_triangle < triangle_count) {
        Triangle* current = &triangles[0] + start_triangle;
        for (unsigned long index = start_triangle; index < triangle_count; ++index) {
            for (unsigned long slot = 0; slot < 3; ++slot) {
                if (current->vertices[slot].position == position) {
                    triangle = index;
                    vertex = slot;
                    return 1;
                }
            }
            ++current;
        }
    }
    return 0;
}

// FUNCTION: SURRENDER 0x10039B40
void srModeler::findClosestVertex(const srVector3T<float>& position, unsigned long& triangle,
                                  unsigned long& vertex)
{
    triangle = 0;
    vertex = 0;
    if (triangle_count != 0) {
        Triangle* current = &triangles[0];
        float best = (current->vertices[0].position - position).LengthSquared();
        for (unsigned long index = 0; index < triangle_count; ++index) {
            for (unsigned long slot = 0; slot < 3; ++slot) {
                float distance = (current->vertices[slot].position - position).LengthSquared();
                if (distance < best) {
                    triangle = index;
                    vertex = slot;
                    best = distance;
                }
            }
            ++current;
        }
    }
}

// FUNCTION: SURRENDER 0x1003AA80
double srModeler::getMaxVertexDist()
{
    if (triangle_count == 0) {
        return 0.0;
    }
    Triangle* triangle = &triangles[0];
    double maximum = 0.0;
    for (unsigned long index = 0; index < triangle_count; ++index) {
        for (int vertex = 0; vertex < 3; ++vertex) {
            /* Retail compares and keeps the unrounded register sum. */
            double distance = triangle->vertices[vertex].position.LengthSquared();
            if (distance > maximum) {
                maximum = distance;
            }
        }
        ++triangle;
    }
    return sqrt(maximum);
}

// FUNCTION: SURRENDER 0x1003AB30
void srModeler::getAxialBounds(e_axis axis, float& minimum, float& maximum)
{
    if (0 <= axis && axis < 3) {
        if (triangle_count == 0) {
            minimum = 0.0f;
            maximum = 0.0f;
            return;
        }
        float* component = &(&triangles[0].vertices[0].position.x)[axis];
        minimum = *component;
        maximum = *component;
        for (unsigned long index = 0; index < triangle_count; ++index) {
            float* vertex_component = component;
            for (int vertex = 0; vertex < 3; ++vertex) {
                if (*vertex_component < minimum) {
                    minimum = *vertex_component;
                }
                if (maximum < *vertex_component) {
                    maximum = *vertex_component;
                }
                vertex_component += 0x44;
            }
            component += 0xda;
        }
    }
}

// FUNCTION: SURRENDER 0x1003BB00
void srModeler::setPassCount(long passes)
{
    pass_count = passes;
    if (passes < 0) {
        pass_count = 0;
    }
    if (pass_count > 4) {
        pass_count = 4;
    }
}

// FUNCTION: SURRENDER 0x1003BB30
long srModeler::getPassCount() const
{
    return pass_count;
}

// FUNCTION: SURRENDER 0x10038BF0
srModeler::VertexHash::VertexHash(unsigned long vertex_count)
{
    entries = new Entry[vertex_count];
    table = new Entry*[vertex_count];
    unique_count = 0;
    memset(entries, 0, vertex_count * sizeof(Entry));
    memset(buckets, 0, sizeof(buckets));
    memset(table, 0, vertex_count * sizeof(Entry*));
}

// FUNCTION: SURRENDER 0x10038D40
srModeler::VertexHash::~VertexHash()
{
    delete[] entries;
    delete[] table;
}

// FUNCTION: SURRENDER 0x100390A0
unsigned long srModeler::VertexHash::hash(double x, double y, double z)
{
    return static_cast<int>(x * 12345.6f + y * 1714.3849f + z * 27561.3f) & 0x3ff;
}

// FUNCTION: SURRENDER 0x10038DB0
srModeler::VertexHash* srModeler::getUniqueVertexList()
{
    if (triangle_count == 0) {
        return 0;
    }
    VertexHash* hash = new VertexHash(triangle_count * 3);
    Triangle* triangle = &triangles[0];
    unsigned long unique = 0;
    unsigned long slot = 0;
    double scale = 1.0 / getMaxVertexDist();
    for (unsigned long index = 0; index < triangle_count; ++index) {
        unsigned long flags = triangle->flags;
        for (int vertex = 0; vertex < 3; ++vertex) {
            Vertex* source = &triangle->vertices[vertex];
            unsigned long group = 0xffffffff;
            unsigned long bucket = VertexHash::hash(
                source->position.x * scale, source->position.y * scale, source->position.z * scale);
            VertexHash::Entry* entry;
            for (entry = hash->buckets[bucket]; entry != 0; entry = entry->next) {
                Vertex* other = entry->vertex;
                if (fabs((source->position.x - other->position.x) * scale) < 0.0001f &&
                    fabs((source->position.y - other->position.y) * scale) < 0.0001f &&
                    fabs((source->position.z - other->position.z) * scale) < 0.0001f &&
                    (entry->flags & flags) != 0 && source->shade_index == other->shade_index) {
                    group = entry->shade_index;
                }
                if (*source == *other && (entry->flags & flags) != 0) {
                    hash->table[slot] = entry;
                    break;
                }
            }
            if (entry == 0) {
                VertexHash::Entry* created = hash->entries + unique;
                hash->table[slot] = created;
                created->flags = flags;
                created->vertex = source;
                created->index = unique;
                if (group == 0xffffffff) {
                    created->shade_index = unique;
                } else {
                    created->shade_index = group;
                }
                created->next = hash->buckets[bucket];
                ++unique;
                hash->buckets[bucket] = created;
            }
            ++slot;
        }
        ++triangle;
    }
    hash->unique_count = unique;
    return hash;
}

// FUNCTION: SURRENDER 0x100390D0
unsigned long srModeler::getUniqueVertexCount()
{
    VertexHash* hash = getUniqueVertexList();
    unsigned long count = hash->unique_count;
    delete hash;
    return count;
}

// FUNCTION: SURRENDER 0x1003A4A0
int srModeler::isClockwise(srVector2T<float>* points, int count)
{
    double accumulated = 0.0;
    int index = 1;
    srVector2T<float>* current = points;
    if (1 < count) {
        do {
            float dx1 = current->x - current[1].x;
            float dy1 = current->y - current[1].y;
            ++index;
            float dx2 = points[index % count].x - current[1].x;
            float dy2 = points[index % count].y - current[1].y;
            double argument = 0.0;
            float magnitude = sqrt(dx2 * dx2 + dy2 * dy2) * sqrt(dx1 * dx1 + dy1 * dy1);
            if (magnitude > 0.0) {
                argument = (dx1 * dx2 + dy1 * dy2) / magnitude;
            }
            if (argument > 1.0) {
                argument = 1.0;
            }
            if (argument < -1.0) {
                argument = -1.0;
            }
            if (0.0 < dx1 * dy2 - dx2 * dy1) {
                accumulated -= acos(argument);
            } else {
                accumulated += acos(argument);
            }
            ++current;
        } while (index < count);
        if (accumulated < 0.0) {
            return 0;
        }
    }
    return 1;
}

// FUNCTION: SURRENDER 0x1003A650
void srModeler::addPolygon(const Polygon& polygon)
{
    Triangle triangle;
    int count = polygon.vertex_count;
    int index;
    if (count < 3) {
        return;
    }
    triangle.flags = polygon.flags;
    triangle.disabled = polygon.disabled;
    for (int pass = 0; pass < 4; ++pass) {
        triangle.shaders[pass] = polygon.shaders[pass];
        triangle.textures[pass][0] = polygon.textures[pass][0];
        triangle.textures[pass][1] = polygon.textures[pass][1];
    }
    if (count == 3) {
        triangle.vertices[0] = polygon.vertices[0];
        triangle.vertices[1] = polygon.vertices[1];
        triangle.vertices[2] = polygon.vertices[2];
        addTriangle(triangle);
    } else {
        float abs_x = 0.0f;
        float abs_y = 0.0f;
        float abs_z = 0.0f;
        for (int index = 1; index < count; ++index) {
            Vertex* previous = &polygon.vertices[index - 1];
            Vertex* current = &polygon.vertices[index];
            Vertex* next = &polygon.vertices[(index + 1) % count];
            float first_x = previous->position.x - current->position.x;
            float first_y = previous->position.y - current->position.y;
            float first_z = previous->position.z - current->position.z;
            float second_x = next->position.x - current->position.x;
            float second_y = next->position.y - current->position.y;
            float second_z = next->position.z - current->position.z;
            abs_x += (float)fabs(first_y * second_z - first_z * second_y);
            abs_y += (float)fabs(first_z * second_x - first_x * second_z);
            abs_z += (float)fabs(first_x * second_y - first_y * second_x);
        }
        srVector2T<float>* points = static_cast<srVector2T<float>*>(srHeap.allocate(count * 8));
        if (abs_x <= abs_y) {
            if (abs_y <= abs_z) {
                for (int index = 0; index < count; ++index) {
                    points[index].x = polygon.vertices[index].position.x;
                    points[index].y = polygon.vertices[index].position.y;
                }
            } else {
                for (int index = 0; index < count; ++index) {
                    points[index].x = polygon.vertices[index].position.x;
                    points[index].y = polygon.vertices[index].position.z;
                }
            }
        } else {
            if (abs_x <= abs_z) {
                for (int index = 0; index < count; ++index) {
                    points[index].x = polygon.vertices[index].position.x;
                    points[index].y = polygon.vertices[index].position.y;
                }
            } else {
                for (int index = 0; index < count; ++index) {
                    points[index].x = polygon.vertices[index].position.y;
                    points[index].y = polygon.vertices[index].position.z;
                }
            }
        }
        if (isClockwise(points, count)) {
            for (int index = 0; index < count; ++index) {
                points[index].y *= -1.0;
            }
        }
        srTriangulator triangulator(points, count);
        srVector3i indices = triangulator.next();
        while (indices.x != -1) {
            triangle.vertices[0] = polygon.vertices[indices.x];
            triangle.vertices[1] = polygon.vertices[indices.y];
            triangle.vertices[2] = polygon.vertices[indices.z];
            addTriangle(triangle);
            indices = triangulator.next();
        }
        srHeap.free(points);
    }
}

// FUNCTION: SURRENDER 0x1003AC00
void srModeler::planarMap(long pass, long layer, const MappingInfo& mapping)
{
    if (0 <= pass && pass < 4 && 0 <= layer && layer < 2 && triangle_count != 0) {
        float u_minimum, u_maximum, v_minimum, v_maximum;
        getAxialBounds(mapping.axis_u, u_minimum, u_maximum);
        getAxialBounds(mapping.axis_v, v_minimum, v_maximum);
        float u_scale = 0.0f;
        if (u_maximum - u_minimum != 0.0f) {
            u_scale = mapping.u_scale / (u_maximum - u_minimum);
        }
        float v_scale = 0.0f;
        if (v_maximum - v_minimum != 0.0f) {
            v_scale = mapping.v_scale / (v_maximum - v_minimum);
        }
        Triangle* triangle = &triangles[0];
        for (unsigned long index = 0; index < triangle_count; ++index) {
            for (int vertex = 0; vertex < 3; ++vertex) {
                float* position = &triangle->vertices[vertex].position.x;
                triangle->vertices[vertex].uv[pass * 2 + layer].x =
                    (position[mapping.axis_u] - u_minimum) * u_scale + mapping.u_offset;
                triangle->vertices[vertex].uv[pass * 2 + layer].y =
                    (-position[mapping.axis_v] - v_minimum) * v_scale + mapping.v_offset;
            }
            ++triangle;
        }
    }
}

// FUNCTION: SURRENDER 0x1003AD60
void srModeler::planarMapAbsolute(long pass, long layer, const MappingInfo& mapping)
{
    if (0 <= pass && pass < 4 && 0 <= layer && layer < 2 && triangle_count != 0) {
        Triangle* triangle = &triangles[0];
        for (unsigned long index = 0; index < triangle_count; ++index) {
            for (int vertex = 0; vertex < 3; ++vertex) {
                float* position = &triangle->vertices[vertex].position.x;
                triangle->vertices[vertex].uv[pass * 2 + layer].x =
                    position[mapping.axis_u] * mapping.u_scale + mapping.u_offset;
                /* Retail scales v by u_scale too (both fmuls read
                   MappingInfo+8); v_scale is unused here. */
                triangle->vertices[vertex].uv[pass * 2 + layer].y =
                    -(position[mapping.axis_v] * mapping.u_scale) + mapping.v_offset;
            }
            ++triangle;
        }
    }
}

// FUNCTION: SURRENDER 0x1003AE30
void srModeler::removeMapping(long pass, long layer)
{
    if (0 <= pass && pass < 4 && 0 <= layer && layer < 2 && triangle_count != 0) {
        Triangle* triangle = &triangles[0];
        for (unsigned long index = 0; index < triangle_count; ++index) {
            for (int vertex = 0; vertex < 3; ++vertex) {
                triangle->vertices[vertex].uv[pass * 2 + layer].SetZero();
            }
            ++triangle;
        }
    }
}

// FUNCTION: SURRENDER 0x1003AEC0
void srModeler::cylinderMap(long pass, long layer, const MappingInfo& mapping)
{
    if (pass < 0 || pass > 3 || layer < 0 || layer > 1 || triangle_count == 0) {
        return;
    }
    e_axis axis = mapping.axis_u;
    e_axis second_axis;
    e_axis third_axis;
    switch (axis) {
    case AXIS_X:
        second_axis = AXIS_Y;
        third_axis = AXIS_Z;
        break;
    case AXIS_Y:
        second_axis = AXIS_X;
        third_axis = AXIS_Z;
        break;
    default:
        second_axis = AXIS_X;
        third_axis = AXIS_Y;
        break;
    }
    float u_minimum, u_maximum;
    getAxialBounds(axis, u_minimum, u_maximum);
    float u_scale;
    if (u_maximum - u_minimum == 0.0f) {
        u_scale = 0.0f;
    } else {
        u_scale = mapping.u_scale / (u_maximum - u_minimum);
    }
    Triangle* triangle = &triangles[0];
    int vertex;
    for (unsigned long index = 0; index < triangle_count; ++index) {
        for (vertex = 0; vertex < 3; ++vertex) {
            float* position = &triangle->vertices[vertex].position.x;
            srVector2T<float>* uv = &triangle->vertices[vertex].uv[pass * 2 + layer];
            /* fpatan with the third axis in ST(1): atan2(third, second),
               kept at register precision. */
            double angle = atan2(position[third_axis], position[second_axis]);
            uv->x = (position[axis] - u_minimum) * u_scale + mapping.u_offset;
            uv->y = -(angle / (pi * 2.0)) * mapping.v_scale + mapping.v_offset;
        }
        float* previous = &triangle->vertices[0].uv[pass * 2 + layer].y;
        /* Edges 0-1, 1-2 and the closing edge 2-0. */
        for (vertex = 1; vertex < 4; ++vertex) {
            float* current = &triangle->vertices[vertex % 3].uv[pass * 2 + layer].y;
            if (mapping.v_scale * 0.8f < fabs(*current - *previous)) {
                if (*previous <= *current) {
                    *previous += mapping.v_scale;
                } else {
                    *current += mapping.v_scale;
                }
            }
            previous = current;
        }
        ++triangle;
    }
}

// FUNCTION: SURRENDER 0x100398E0
void srModeler::createGrid(long columns, long rows)
{
    discard();
    if (columns < 1) {
        columns = 1;
    }
    if (rows < 1) {
        rows = 1;
    }
    int row;
    for (row = 0; row < rows; ++row) {
        int column = 0;
        if (0 < columns) {
            float top = (row + 1) / static_cast<double>(rows) - 0.5;
            float bottom = row / static_cast<double>(rows) - 0.5;
            do {
                Triangle triangle;
                long column_position = column;
                ++column;
                float left = column_position / static_cast<double>(columns) - 0.5;
                float right = column / static_cast<double>(columns) - 0.5;
                triangle.vertices[0].position.Set(left, bottom, 0.0);
                triangle.vertices[1].position.Set(left, top, 0.0);
                triangle.vertices[2].position.Set(right, top, 0.0);
                addTriangle(triangle);
                triangle.vertices[1].position.Set(right, bottom, 0.0);
                triangle.flipFacing();
                addTriangle(triangle);
            } while (column < columns);
        }
    }
}

// FUNCTION: SURRENDER 0x10039580
void srModeler::createSphere(long detail)
{
    Triangle triangle;
    discard();
    if (detail < 3) {
        detail = 3;
    }
    int bands = detail / 2;
    double fraction = 0.0;
    double band_count = (double)bands;
    if (0 < bands) {
        do {
            double angle = 0.0;
            double height1 = sin(pi * fraction * 0.5);
            double next_fraction = 1.0 / band_count + fraction;
            double height2 = sin(next_fraction * pi * 0.5);
            double radius1 = 1.0 - height1 * height1;
            if (0.0 <= radius1) {
                radius1 = sqrt(radius1);
            }
            double radius2 = 1.0 - height2 * height2;
            if (0.0 <= radius2) {
                radius2 = sqrt(radius2);
            }
            radius2 = radius2 * 0.5;
            if (0 < detail) {
                float z1 = (float)(height1 * 0.5);
                float z2 = (float)(height2 * 0.5);
                float nz1 = -z1;
                float nz2 = -z2;
                long segment = detail;
                do {
                    double cosine = cos(angle);
                    double sine = sin(angle);
                    angle = angle + (pi * 2.0) / detail;
                    double next_cosine = cos(angle);
                    double next_sine = sin(angle);
                    double ring = radius1 * 0.5;
                    triangle.vertices[0].position.Set(cosine * ring, sine * ring, z1);
                    triangle.vertices[1].position.Set(next_cosine * ring, next_sine * ring, z1);
                    triangle.vertices[2].position.Set(cosine * radius2, sine * radius2, z2);
                    addTriangle(triangle);
                    triangle.vertices[0].position.Set(next_cosine * radius2, next_sine * radius2,
                                                      z2);
                    triangle.flipFacing();
                    addTriangle(triangle);
                    triangle.vertices[0].position.Set(cosine * ring, sine * ring, nz1);
                    triangle.vertices[1].position.Set(next_cosine * ring, next_sine * ring, nz1);
                    triangle.vertices[2].position.Set(cosine * radius2, sine * radius2, nz2);
                    triangle.flipFacing();
                    addTriangle(triangle);
                    triangle.vertices[0].position.Set(next_cosine * radius2, next_sine * radius2,
                                                      nz2);
                    triangle.flipFacing();
                    addTriangle(triangle);
                    --segment;
                } while (segment != 0);
            }
            fraction = next_fraction;
            --bands;
        } while (bands != 0);
    }
}

// FUNCTION: SURRENDER 0x10039360
void srModeler::createTorus(long major_segments, long minor_segments, double radius)
{
    Triangle triangle;
    discard();
    if (major_segments < 3) {
        major_segments = 3;
    }
    if (minor_segments < 3) {
        minor_segments = 3;
    }
    radius = fabs(radius);
    if (1.0 < radius) {
        radius = 1.0;
    }
    radius = radius * 0.5;
    double major_angle = 0.0;
    if (0 < major_segments) {
        long major_count = major_segments;
        do {
            double cosine = cos(major_angle);
            double minor_angle = 0.0;
            double sine = sin(major_angle);
            major_angle = major_angle + (pi * 2.0) / major_segments;
            double next_cosine = cos(major_angle);
            double next_sine = sin(major_angle);
            long minor_count = minor_segments;
            if (0 < minor_segments) {
                do {
                    double tube = cos(minor_angle) * radius + 0.5;
                    double next_minor = (pi * 2.0) / minor_segments + minor_angle;
                    double next_tube = cos(next_minor) * radius + 0.5;
                    double depth = sin(minor_angle) * radius;
                    double next_depth = sin(next_minor) * radius;
                    triangle.vertices[0].position.Set(tube * cosine, tube * sine, depth);
                    triangle.vertices[1].position.Set(tube * next_cosine, tube * next_sine, depth);
                    triangle.vertices[2].position.Set(next_tube * cosine, next_tube * sine,
                                                      next_depth);
                    addTriangle(triangle);
                    triangle.vertices[0].position.Set(next_tube * next_cosine,
                                                      next_tube * next_sine, next_depth);
                    triangle.flipFacing();
                    addTriangle(triangle);
                    minor_angle = next_minor;
                    --minor_count;
                } while (minor_count != 0);
            }
            --major_count;
        } while (major_count != 0);
    }
}

// FUNCTION: SURRENDER 0x10039130
void srModeler::tesselateEdges(unsigned long triangle, double threshold)
{
    if (triangle < triangle_count && 0.0 < threshold) {
        Triangle* source = &triangles[triangle];
        double longest = 0.0;
        int edge = 0;
        int vertex;
        for (vertex = 0; vertex < 3; ++vertex) {
            int next = (vertex + 1) % 3;
            double dx =
                source->vertices[vertex].position.x - (double)source->vertices[next].position.x;
            double dy =
                source->vertices[vertex].position.y - (double)source->vertices[next].position.y;
            double dz =
                source->vertices[vertex].position.z - (double)source->vertices[next].position.z;
            double distance = sqrt(dx * dx + dy * dy + dz * dz);
            if (longest < distance) {
                longest = distance;
                edge = vertex;
            }
        }
        if (threshold < longest && longest != 0.0) {
            Triangle child;
            for (int pass = 0; pass < 4; ++pass) {
                child.shaders[pass] = source->shaders[pass];
                for (int layer = 0; layer < 2; ++layer) {
                    child.textures[pass][layer] = source->textures[pass][layer];
                }
            }
            child.flags = source->flags;
            child.vertices[0] = source->vertices[(edge + 2) % 3];
            child.vertices[1] = source->vertices[edge];
            child.vertices[2].interpolate(source->vertices[edge], source->vertices[(edge + 1) % 3],
                                          0.5f);
            source->vertices[edge] = child.vertices[2];
            unsigned long added = addTriangle(child);
            tesselateEdges(triangle, threshold);
            tesselateEdges(added, threshold);
        }
    }
}

// FUNCTION: SURRENDER 0x10039100
void srModeler::tesselateEdges(double threshold)
{
    unsigned long count = triangle_count;
    for (unsigned long index = 0; index < count; ++index) {
        tesselateEdges(index, threshold);
    }
}

// FUNCTION: SURRENDER 0x1003B940
void srModeler::autoSmooth(double threshold, int smooth)
{
    VertexHash* hash = getUniqueVertexList();
    AutoSmoother* smoother =
        new AutoSmoother(&triangles[0], triangle_count, hash, threshold, smooth);
    smoother->smooth();
    delete hash;
    delete smoother;
}

// FUNCTION: SURRENDER 0x1003BB40
void srModeler::setMaterial(srMaterialIFace* material, long pass, srMeshModel::e_side side)
{
    if (0 <= pass && pass < 4 &&
        (side == srMeshModel::SIDE_FRONT || side == srMeshModel::SIDE_BACK)) {
        Triangle* triangle = &triangles[0];
        for (long index = 0; index < (long)triangle_count; ++index) {
            for (int vertex = 0; vertex < 3; ++vertex) {
                triangle->vertices[vertex].materials[pass][side] = material;
            }
            ++triangle;
        }
    }
}

// FUNCTION: SURRENDER 0x1003BBC0
void srModeler::setTexture(srTextureIFace* texture, long pass, long layer)
{
    if (0 <= pass && pass < 4 && 0 <= layer && layer < 2) {
        Triangle* triangle = &triangles[0];
        for (long index = 0; index < (long)triangle_count; ++index) {
            triangle->textures[pass][layer] = texture;
            ++triangle;
        }
    }
}

// FUNCTION: SURRENDER 0x1003BC20
void srModeler::setShader(srShader shader, long pass)
{
    if (0 <= pass && pass < 4) {
        Triangle* triangle = &triangles[0];
        for (long index = 0; index < (long)triangle_count; ++index) {
            triangle->shaders[pass] = shader;
            ++triangle;
        }
    }
}

// FUNCTION: SURRENDER 0x1003B160
void srModeler::convert(srMeshModel& model, int remove_degenerate)
{
    if (remove_degenerate != 0) {
        disableDegenerateTriangles();
        removeDisabledTriangles();
    }
    if (triangle_count == 0 || pass_count == 0) {
        model.reset(0, 0);
        return;
    }
    VertexHash* hash = getUniqueVertexList();
    model.reset(triangle_count, hash->unique_count);
    model.clearDirty(srMeshModel::DIRTY_POLYGON_NORMALS);
    model.clearDirty(srMeshModel::DIRTY_VERTEX_NORMALS);
    model.pass_count = pass_count;
    if (pass_count < 1) {
        model.pass_count = 1;
    } else if (4 < pass_count) {
        model.pass_count = 4;
    }
    unsigned long unique = hash->unique_count;
    unsigned long count = triangle_count;
    unsigned long index;
    long layer;
    long side;
    Vertex* vertex;
    VertexHash::Entry* entry;
    for (long pass = 0; pass < pass_count; ++pass) {
        char same_shader = 1;
        char same_material[2];
        char same_texture[2];
        char same_uv[2];
        bool same_dcg = true;
        bool same_scg = true;
        bool same_dig = true;
        same_material[0] = 1;
        same_material[1] = 1;
        same_texture[0] = 1;
        same_texture[1] = 1;
        srShader shader;
        Triangle* triangle = &triangles[0];
        shader = triangle->shaders[pass];
        srTextureIFace* texture[2];
        texture[0] = triangle->textures[pass][0];
        texture[1] = triangle->textures[pass][1];
        if (1 < count) {
            triangle = &triangles[1];
            for (index = count - 1; index != 0; --index) {
                for (layer = 0; layer < 2; ++layer) {
                    if (triangle->textures[pass][layer] != texture[layer]) {
                        same_texture[layer] = 0;
                    }
                }
                if (triangle->shaders[pass].value != shader.value) {
                    same_shader = 0;
                }
                ++triangle;
            }
        }
        for (layer = 0; layer < 2; ++layer) {
            model.setTexture(texture[layer], pass, layer);
        }
        model.setShader(shader, pass);
        if (same_shader == 0) {
            srShader* table = model.getPolyShader(pass, 1);
            triangle = &triangles[0];
            for (index = count; index != 0; --index) {
                *table = triangle->shaders[pass];
                ++table;
                ++triangle;
            }
        }
        for (layer = 0; layer < 2; ++layer) {
            if (same_texture[layer] == 0) {
                srPtr<srTextureIFace>* table = model.getPolyTexture(pass, layer, 1);
                triangle = &triangles[0];
                for (index = count; index != 0; --index) {
                    *table = triangle->textures[pass][layer];
                    ++table;
                    ++triangle;
                }
            }
        }
        same_uv[0] = 1;
        same_uv[1] = 1;
        srMaterialIFace* material_front = 0;
        srMaterialIFace* material_back = 0;
        for (index = 0; index < unique; ++index) {
            vertex = hash->entries[index].vertex;
            if (index == 0) {
                material_front = vertex->materials[pass][0];
                material_back = vertex->materials[pass][1];
            }
            if (vertex->dcg[pass].x != 1.0f || vertex->dcg[pass].y != 1.0f ||
                vertex->dcg[pass].z != 1.0f || vertex->weights[pass] != 1.0f) {
                same_dcg = false;
            }
            if (vertex->dig[pass].x != 0.0f || vertex->dig[pass].y != 0.0f ||
                vertex->dig[pass].z != 0.0f) {
                same_dig = false;
            }
            if (vertex->scg[pass].x != 1.0f || vertex->scg[pass].y != 1.0f ||
                vertex->scg[pass].z != 1.0f) {
                same_scg = false;
            }
            if (vertex->materials[pass][0] != material_front) {
                same_material[0] = 0;
            }
            if (vertex->materials[pass][1] != material_back) {
                same_material[1] = 0;
            }
            for (layer = 0; layer < 2; ++layer) {
                if (vertex->uv[pass * 2 + layer].x != 0.0f ||
                    vertex->uv[pass * 2 + layer].y != 0.0f) {
                    same_uv[layer] = 0;
                }
            }
        }
        if (!same_dcg) {
            srVector4T<float>* table = model.getVertexDCG(pass, 1);
            entry = hash->entries;
            for (index = unique; index != 0; --index) {
                vertex = entry->vertex;
                table->x = vertex->dcg[pass].x;
                table->y = vertex->dcg[pass].y;
                table->z = vertex->dcg[pass].z;
                table->w = vertex->weights[pass];
                ++table;
                ++entry;
            }
        }
        if (!same_scg) {
            srVector4T<float>* table = model.getVertexSCG(pass, 1);
            entry = hash->entries;
            for (index = unique; index != 0; --index) {
                vertex = entry->vertex;
                table->x = vertex->scg[pass].x;
                table->y = vertex->scg[pass].y;
                table->z = vertex->scg[pass].z;
                table->w = 1.0f;
                ++table;
                ++entry;
            }
        }
        if (!same_dig) {
            srVector3T<float>* table = model.getVertexDIG(pass, 1);
            entry = hash->entries;
            for (index = unique; index != 0; --index) {
                vertex = entry->vertex;
                table->x = vertex->dig[pass].x;
                table->y = vertex->dig[pass].y;
                table->z = vertex->dig[pass].z;
                ++table;
                ++entry;
            }
        }
        model.setMaterial(material_front, pass, srMeshModel::SIDE_FRONT);
        model.setMaterial(material_back, pass, srMeshModel::SIDE_BACK);
        for (side = 0; side < 2; ++side) {
            if (same_material[side] == 0) {
                srPtr<srMaterialIFace>* table =
                    model.getVertexMaterial(pass, static_cast<srMeshModel::e_side>(side), 1);
                if (unique != 0) {
                    entry = hash->entries;
                    for (index = unique; index != 0; --index) {
                        *table = entry->vertex->materials[pass][side];
                        ++table;
                        ++entry;
                    }
                }
            }
        }
        for (layer = 0; layer < 2; ++layer) {
            if (same_uv[layer] == 0) {
                srVector2T<float>* table = model.getVertexTexCoords(pass, layer, 1);
                if (unique != 0) {
                    entry = hash->entries;
                    for (index = unique; index != 0; --index) {
                        *table = entry->vertex->uv[pass * 2 + layer];
                        ++table;
                        ++entry;
                    }
                }
            }
        }
    }
    srVector3T<float>* locations = model.getVertexLoc();
    for (index = 0; index < unique; ++index) {
        locations[index] = hash->entries[index].vertex->position;
    }
    srVector3i* polygons = model.getPolyVertex();
    for (index = 0; index < count; ++index) {
        polygons[index].x = hash->table[index * 3]->index;
        polygons[index].y = hash->table[index * 3 + 1]->index;
        polygons[index].z = hash->table[index * 3 + 2]->index;
    }
    unsigned long* shades = model.getVertexShadeIndex(1);
    for (index = 0; index < unique; ++index) {
        shades[index] = hash->entries[index].shade_index;
    }
    model.setDirty(srMeshModel::DIRTY_BOUNDS);
    model.setDirty(srMeshModel::DIRTY_POLYGON_NORMALS);
    model.setDirty(srMeshModel::DIRTY_VERTEX_NORMALS);
    model.setDirty(srMeshModel::DIRTY_TRI_MESH);
    delete hash;
}
