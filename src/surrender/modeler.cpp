#include "surrender/srModeler.h"

#include "surrender/srHeap.h"
#include "surrender/srTriangulator.h"

#include <math.h>
#include <string.h>

/* TU-scope constant the geometry generators and cylinderMap load from rdata
   0x10076C90 (pi + pi / pi * 2.0 in retail emission). */
static const double pi = 3.141592653589793;

/* Inline so the Polygon/Triangle array-construction loops in this TU inline
   the reset() call like retail; consumers see the declaration only and import
   the standalone copy (Wiz8.exe imports ??0Vertex@srModeler@@QAE@XZ). */
inline srModeler::Vertex::Vertex()
{
    reset();
}

// FUNCTION: SURRENDER 0x100386A0
void srModeler::Vertex::reset()
{
    position_00.x = 0.0f;
    position_00.y = 0.0f;
    position_00.z = 0.0f;
    for (int pass = 0; pass < 4; ++pass) {
        dig_60[pass].x = 0.0f;
        dig_60[pass].y = 0.0f;
        dig_60[pass].z = 0.0f;
        dcg_30[pass].x = 1.0f;
        dcg_30[pass].y = 1.0f;
        dcg_30[pass].z = 1.0f;
        scg_90[pass].x = 1.0f;
        scg_90[pass].y = 1.0f;
        scg_90[pass].z = 1.0f;
        weights_100[pass] = 1.0f;
        materials_10[pass][0] = 0;
        materials_10[pass][1] = 0;
        for (int layer = 0; layer < 2; ++layer) {
            uv_c0[pass * 2 + layer].x = 0.0f;
            uv_c0[pass * 2 + layer].y = 0.0f;
        }
    }
    shade_index_0c = 0;
}

// FUNCTION: SURRENDER 0x10038240
int srModeler::Vertex::operator==(const Vertex& other) const
{
    if (!(position_00 == other.position_00)) {
        return 0;
    }
    if (shade_index_0c == other.shade_index_0c) {
        for (int pass = 0; pass < 4; ++pass) {
            if (!(dcg_30[pass] == other.dcg_30[pass] && dig_60[pass] == other.dig_60[pass] &&
                  scg_90[pass] == other.scg_90[pass] &&
                  weights_100[pass] == other.weights_100[pass])) {
                return 0;
            }
            for (int side = 0; side < 2; ++side) {
                if (materials_10[pass][side] != other.materials_10[pass][side]) {
                    return 0;
                }
            }
            for (int layer = 0; layer < 2; ++layer) {
                if (uv_c0[pass * 2 + layer].x != other.uv_c0[pass * 2 + layer].x ||
                    uv_c0[pass * 2 + layer].y != other.uv_c0[pass * 2 + layer].y) {
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
    position_00.x = (second.position_00.x - first.position_00.x) * fraction + first.position_00.x;
    position_00.y = (second.position_00.y - first.position_00.y) * fraction + first.position_00.y;
    position_00.z = (second.position_00.z - first.position_00.z) * fraction + first.position_00.z;
    for (int pass = 0; pass < 4; ++pass) {
        dig_60[pass].x =
            (second.dig_60[pass].x - first.dig_60[pass].x) * fraction + first.dig_60[pass].x;
        dig_60[pass].y =
            (second.dig_60[pass].y - first.dig_60[pass].y) * fraction + first.dig_60[pass].y;
        dig_60[pass].z =
            (second.dig_60[pass].z - first.dig_60[pass].z) * fraction + first.dig_60[pass].z;
        scg_90[pass].x =
            (second.scg_90[pass].x - first.scg_90[pass].x) * fraction + first.scg_90[pass].x;
        scg_90[pass].y =
            (second.scg_90[pass].y - first.scg_90[pass].y) * fraction + first.scg_90[pass].y;
        scg_90[pass].z =
            (second.scg_90[pass].z - first.scg_90[pass].z) * fraction + first.scg_90[pass].z;
        dcg_30[pass].x =
            (second.dcg_30[pass].x - first.dcg_30[pass].x) * fraction + first.dcg_30[pass].x;
        dcg_30[pass].y =
            (second.dcg_30[pass].y - first.dcg_30[pass].y) * fraction + first.dcg_30[pass].y;
        dcg_30[pass].z =
            (second.dcg_30[pass].z - first.dcg_30[pass].z) * fraction + first.dcg_30[pass].z;
        weights_100[pass] = (second.weights_100[pass] - first.weights_100[pass]) * fraction +
                            first.weights_100[pass];
        for (int layer = 0; layer < 2; ++layer) {
            uv_c0[pass * 2 + layer].x =
                (second.uv_c0[pass * 2 + layer].x - first.uv_c0[pass * 2 + layer].x) * fraction +
                first.uv_c0[pass * 2 + layer].x;
            uv_c0[pass * 2 + layer].y =
                (second.uv_c0[pass * 2 + layer].y - first.uv_c0[pass * 2 + layer].y) * fraction +
                first.uv_c0[pass * 2 + layer].y;
        }
    }
    shade_index_0c = 0;
}

// FUNCTION: SURRENDER 0x10038730
srModeler::Vertex& srModeler::Vertex::operator=(const Vertex& other)
{
    if (this != &other) {
        for (int pass = 0; pass < 4; ++pass) {
            dig_60[pass] = other.dig_60[pass];
            dcg_30[pass] = other.dcg_30[pass];
            scg_90[pass] = other.scg_90[pass];
            weights_100[pass] = other.weights_100[pass];
            for (int side = 0; side < 2; ++side) {
                materials_10[pass][side] = other.materials_10[pass][side];
            }
            for (int layer = 0; layer < 2; ++layer) {
                uv_c0[pass * 2 + layer] = other.uv_c0[pass * 2 + layer];
            }
        }
        shade_index_0c = other.shade_index_0c;
        position_00 = other.position_00;
    }
    return *this;
}

// FUNCTION: SURRENDER 0x10038B50
srModeler::Triangle::Triangle() : flags_360(0)
{
    reset();
}

// FUNCTION: SURRENDER 0x10038AF0
void srModeler::Triangle::reset()
{
    for (int vertex = 0; vertex < 3; ++vertex) {
        vertices_30[vertex].reset();
    }
    for (int pass = 0; pass < 4; ++pass) {
        shaders_20[pass] = srShader();
        textures_00[pass][0] = 0;
        textures_00[pass][1] = 0;
    }
    flags_360 = 0;
    flags_360 |= 1;
    disabled_364 = 0;
}

// FUNCTION: SURRENDER 0x10038840
void srModeler::Triangle::flipFacing()
{
    Vertex temporary;
    temporary = vertices_30[1];
    vertices_30[1] = vertices_30[2];
    vertices_30[2] = temporary;
}

// FUNCTION: SURRENDER 0x10038890
srModeler::Polygon::Polygon(int vertices)
{
    vertex_count_34 = vertices;
    flags_38 = 0;
    vertices_30 = new Vertex[vertices];
    capacity_40 = vertices;
    reset();
}

// FUNCTION: SURRENDER 0x10038990
srModeler::Polygon::~Polygon()
{
    delete[] vertices_30;
}

// FUNCTION: SURRENDER 0x100389A0
void srModeler::Polygon::reset()
{
    for (int vertex = 0; vertex < vertex_count_34; ++vertex) {
        vertices_30[vertex].reset();
    }
    for (int pass = 0; pass < 4; ++pass) {
        shaders_20[pass] = srShader();
        textures_00[pass][0] = 0;
        textures_00[pass][1] = 0;
    }
    flags_38 = 0;
    flags_38 |= 1;
    disabled_3c = 0;
}

// FUNCTION: SURRENDER 0x10038A00
void srModeler::Polygon::reAllocate(int vertices)
{
    if (capacity_40 < vertices) {
        delete[] vertices_30;
        vertices_30 = new Vertex[vertices];
        capacity_40 = vertices;
        reset();
    }
    vertex_count_34 = vertices;
}

// FUNCTION: SURRENDER 0x1003BAE0
srModeler::srModeler()
{
    triangle_count_04 = 0;
    pass_count_10 = 1;
}

// FUNCTION: SURRENDER 0x1003BC70
srModeler::~srModeler() {}

// FUNCTION: SURRENDER 0x10039A70
void srModeler::discard()
{
    setTriangleCount(0);
    pass_count_10 = 1;
}

// FUNCTION: SURRENDER 0x10037C00
unsigned long srModeler::getTriangleCount() const
{
    return triangle_count_04;
}

// FUNCTION: SURRENDER 0x1003A460
void srModeler::setTriangleCount(unsigned long triangles)
{
    triangle_count_04 = triangles;
    triangles_08.setCapacity(triangles);
}

// FUNCTION: SURRENDER 0x1003A480
unsigned long srModeler::addTriangle(const Triangle& triangle)
{
    setTriangle(++triangle_count_04 - 1, triangle);
    return triangle_count_04 - 1;
}

// FUNCTION: SURRENDER 0x1003A330
int srModeler::getTriangle(unsigned long index, Triangle& triangle)
{
    if (triangle_count_04 <= index) {
        return 0;
    }
    triangle = triangles_08[index];
    return 1;
}

// FUNCTION: SURRENDER 0x1003B0B0
void srModeler::setTriangle(unsigned long index, const Triangle& triangle)
{
    if (index < triangle_count_04) {
        triangles_08[index] = triangle;
    }
}

// FUNCTION: SURRENDER 0x10039CC0
void srModeler::setTriangleVertex(unsigned long triangle, unsigned long vertex, const Vertex& value)
{
    if (triangle < triangle_count_04 && vertex < 3) {
        triangles_08[triangle].vertices_30[vertex] = value;
    }
}

// FUNCTION: SURRENDER 0x10039C80
void srModeler::flipTriangle(unsigned long triangle)
{
    if (triangle < triangle_count_04) {
        triangles_08[triangle].flipFacing();
    }
}

// FUNCTION: SURRENDER 0x10039C40
void srModeler::flipTriangles()
{
    Triangle* triangle = &triangles_08[0];
    for (unsigned long index = 0; index < triangle_count_04; ++index) {
        triangle->flipFacing();
        ++triangle;
    }
}

// FUNCTION: SURRENDER 0x1003A420
void srModeler::enableTriangle(unsigned long triangle)
{
    if (triangle < triangle_count_04) {
        triangles_08[triangle].disabled_364 = 0;
    }
}

// FUNCTION: SURRENDER 0x1003A3E0
void srModeler::disableTriangle(unsigned long triangle)
{
    if (triangle < triangle_count_04) {
        triangles_08[triangle].disabled_364 = 1;
    }
}

// FUNCTION: SURRENDER 0x1003A0F0
unsigned long srModeler::getEnabledTriangleCount()
{
    unsigned long enabled = 0;
    Triangle* triangle = &triangles_08[0];
    for (unsigned long index = triangle_count_04; index != 0; --index) {
        if (triangle->disabled_364 == 0) {
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
    if (enabled != triangle_count_04) {
        Triangle* destination = &triangles_08[0];
        unsigned long destination_index = 0;
        unsigned long index = 0;
        Triangle* source = destination;
        if (triangle_count_04 != 0) {
            do {
                if (source->disabled_364 == 0) {
                    if (destination_index != index) {
                        *destination = *source;
                    }
                    ++destination_index;
                    ++destination;
                }
                ++index;
                ++source;
            } while (index < triangle_count_04);
        }
        triangle_count_04 = enabled;
    }
}

// FUNCTION: SURRENDER 0x1003A240
void srModeler::disableDegenerateTriangles()
{
    Triangle* triangle = &triangles_08[0];
    for (unsigned long index = 0; index < triangle_count_04; ++index) {
        if (triangle->vertices_30[0].position_00 == triangle->vertices_30[1].position_00 ||
            triangle->vertices_30[1].position_00 == triangle->vertices_30[2].position_00 ||
            triangle->vertices_30[0].position_00 == triangle->vertices_30[2].position_00) {
            triangle->disabled_364 = 1;
        }
        ++triangle;
    }
}

// FUNCTION: SURRENDER 0x10038D60
void srModeler::addFromModeler(srModeler& other)
{
    Triangle triangle;
    long count = other.triangle_count_04;
    for (long index = 0; index < count; ++index) {
        other.getTriangle(index, triangle);
        addTriangle(triangle);
    }
}

// FUNCTION: SURRENDER 0x10039E10
void srModeler::scale(const srVector3T<float>& scale)
{
    Triangle* triangle = &triangles_08[0];
    for (unsigned long index = 0; index < triangle_count_04; ++index) {
        for (int vertex = 0; vertex < 3; ++vertex) {
            triangle->vertices_30[vertex].position_00 *= scale;
        }
        ++triangle;
    }
}

// FUNCTION: SURRENDER 0x10039F90
void srModeler::scale(unsigned long triangle, const srVector3T<float>& scale)
{
    if (triangle < triangle_count_04) {
        Triangle* element = &triangles_08[triangle];
        for (int index = 0; index < 3; ++index) {
            element->vertices_30[index].position_00 *= scale;
        }
    }
}

// FUNCTION: SURRENDER 0x10039E90
void srModeler::move(const srVector3T<float>& delta)
{
    Triangle* triangle = &triangles_08[0];
    for (unsigned long index = 0; index < triangle_count_04; ++index) {
        for (int vertex = 0; vertex < 3; ++vertex) {
            triangle->vertices_30[vertex].position_00 += delta;
        }
        ++triangle;
    }
}

// FUNCTION: SURRENDER 0x10039F10
void srModeler::move(unsigned long triangle, const srVector3T<float>& delta)
{
    if (triangle < triangle_count_04) {
        Triangle* element = &triangles_08[triangle];
        for (int index = 0; index < 3; ++index) {
            element->vertices_30[index].position_00 += delta;
        }
    }
}

// FUNCTION: SURRENDER 0x10039D20
void srModeler::rotate(const srMatrix3T<float>& matrix)
{
    Triangle* triangle = &triangles_08[0];
    for (unsigned long index = 0; index < triangle_count_04; ++index) {
        for (int vertex = 0; vertex < 3; ++vertex) {
            triangle->vertices_30[vertex].position_00.Transform(matrix);
        }
        ++triangle;
    }
}

// FUNCTION: SURRENDER 0x1003A010
void srModeler::rotate(unsigned long triangle, const srMatrix3T<float>& matrix)
{
    if (triangle < triangle_count_04) {
        Triangle* element = &triangles_08[triangle];
        for (int index = 0; index < 3; ++index) {
            element->vertices_30[index].position_00 =
                matrix.Transform(element->vertices_30[index].position_00);
        }
    }
}

// FUNCTION: SURRENDER 0x10039A90
int srModeler::findVertex(const srVector3T<float>& position, unsigned long& triangle,
                          unsigned long& vertex, unsigned long start_triangle)
{
    if (start_triangle < triangle_count_04) {
        Triangle* current = &triangles_08[0] + start_triangle;
        for (unsigned long index = start_triangle; index < triangle_count_04; ++index) {
            for (unsigned long slot = 0; slot < 3; ++slot) {
                if (current->vertices_30[slot].position_00 == position) {
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
    if (triangle_count_04 != 0) {
        Triangle* current = &triangles_08[0];
        float best = (current->vertices_30[0].position_00 - position).LengthSquared();
        for (unsigned long index = 0; index < triangle_count_04; ++index) {
            for (unsigned long slot = 0; slot < 3; ++slot) {
                float distance =
                    (current->vertices_30[slot].position_00 - position).LengthSquared();
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
    if (triangle_count_04 != 0) {
        double maximum = 0.0;
        Triangle* triangle = &triangles_08[0];
        for (unsigned long index = 0; index < triangle_count_04; ++index) {
            for (int vertex = 0; vertex < 3; ++vertex) {
                float distance = triangle->vertices_30[vertex].position_00.LengthSquared();
                if (maximum < distance) {
                    maximum = distance;
                }
            }
            ++triangle;
        }
        return sqrt(maximum);
    }
    return 0.0;
}

// FUNCTION: SURRENDER 0x1003AB30
void srModeler::getAxialBounds(e_axis axis, float& minimum, float& maximum)
{
    if (0 <= axis && axis < 3) {
        if (triangle_count_04 == 0) {
            minimum = 0.0f;
            maximum = 0.0f;
            return;
        }
        float* component = &(&triangles_08[0].vertices_30[0].position_00.x)[axis];
        minimum = *component;
        maximum = *component;
        for (unsigned long index = 0; index < triangle_count_04; ++index) {
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
    pass_count_10 = passes;
    if (passes < 0) {
        pass_count_10 = 0;
    }
    if (pass_count_10 > 4) {
        pass_count_10 = 4;
    }
}

// FUNCTION: SURRENDER 0x1003BB30
long srModeler::getPassCount() const
{
    return pass_count_10;
}

// FUNCTION: SURRENDER 0x10038BF0
srModeler::VertexHash::VertexHash(unsigned long vertex_count)
{
    entries_00 = new Entry[vertex_count];
    table_1004 = new Entry*[vertex_count];
    unique_count_1008 = 0;
    memset(entries_00, 0, vertex_count * sizeof(Entry));
    memset(buckets_04, 0, sizeof(buckets_04));
    memset(table_1004, 0, vertex_count * sizeof(Entry*));
}

// FUNCTION: SURRENDER 0x10038D40
srModeler::VertexHash::~VertexHash()
{
    delete[] entries_00;
    delete[] table_1004;
}

// FUNCTION: SURRENDER 0x100390A0
unsigned long srModeler::VertexHash::hash(double x, double y, double z)
{
    return static_cast<int>(x * 12345.6f + y * 1714.3849f + z * 27561.3f) & 0x3ff;
}

// FUNCTION: SURRENDER 0x10038DB0
srModeler::VertexHash* srModeler::getUniqueVertexList()
{
    if (triangle_count_04 == 0) {
        return 0;
    }
    VertexHash* hash = new VertexHash(triangle_count_04 * 3);
    Triangle* triangle = &triangles_08[0];
    unsigned long unique = 0;
    unsigned long slot = 0;
    double scale = 1.0 / getMaxVertexDist();
    for (unsigned long index = 0; index < triangle_count_04; ++index) {
        unsigned long flags = triangle->flags_360;
        for (int vertex = 0; vertex < 3; ++vertex) {
            Vertex* source = &triangle->vertices_30[vertex];
            unsigned long group = 0xffffffff;
            unsigned long bucket =
                VertexHash::hash(source->position_00.x * scale, source->position_00.y * scale,
                                 source->position_00.z * scale);
            VertexHash::Entry* entry;
            for (entry = hash->buckets_04[bucket]; entry != 0; entry = entry->next_0c) {
                Vertex* other = entry->vertex_08;
                if (fabs((source->position_00.x - other->position_00.x) * scale) < 0.0001 &&
                    fabs((source->position_00.y - other->position_00.y) * scale) < 0.0001 &&
                    fabs((source->position_00.z - other->position_00.z) * scale) < 0.0001 &&
                    (entry->flags_00 & flags) != 0 &&
                    source->shade_index_0c == other->shade_index_0c) {
                    group = entry->shade_index_04;
                }
                if (*source == *other && (entry->flags_00 & flags) != 0) {
                    hash->table_1004[slot] = entry;
                    break;
                }
            }
            if (entry == 0) {
                VertexHash::Entry* created = hash->entries_00 + unique;
                hash->table_1004[slot] = created;
                created->flags_00 = flags;
                created->vertex_08 = source;
                created->index_10 = unique;
                if (group == 0xffffffff) {
                    created->shade_index_04 = unique;
                } else {
                    created->shade_index_04 = group;
                }
                created->next_0c = hash->buckets_04[bucket];
                ++unique;
                hash->buckets_04[bucket] = created;
            }
            ++slot;
        }
        ++triangle;
    }
    hash->unique_count_1008 = unique;
    return hash;
}

// FUNCTION: SURRENDER 0x100390D0
unsigned long srModeler::getUniqueVertexCount()
{
    VertexHash* hash = getUniqueVertexList();
    unsigned long count = hash->unique_count_1008;
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
    int count = polygon.vertex_count_34;
    int index;
    if (count < 3) {
        return;
    }
    triangle.flags_360 = polygon.flags_38;
    triangle.disabled_364 = polygon.disabled_3c;
    for (int pass = 0; pass < 4; ++pass) {
        triangle.shaders_20[pass] = polygon.shaders_20[pass];
        triangle.textures_00[pass][0] = polygon.textures_00[pass][0];
        triangle.textures_00[pass][1] = polygon.textures_00[pass][1];
    }
    if (count == 3) {
        triangle.vertices_30[0] = polygon.vertices_30[0];
        triangle.vertices_30[1] = polygon.vertices_30[1];
        triangle.vertices_30[2] = polygon.vertices_30[2];
        addTriangle(triangle);
    } else {
        float abs_x = 0.0f;
        float abs_y = 0.0f;
        float abs_z = 0.0f;
        for (int index = 1; index < count; ++index) {
            Vertex* previous = &polygon.vertices_30[index - 1];
            Vertex* current = &polygon.vertices_30[index];
            Vertex* next = &polygon.vertices_30[(index + 1) % count];
            float first_x = previous->position_00.x - current->position_00.x;
            float first_y = previous->position_00.y - current->position_00.y;
            float first_z = previous->position_00.z - current->position_00.z;
            float second_x = next->position_00.x - current->position_00.x;
            float second_y = next->position_00.y - current->position_00.y;
            float second_z = next->position_00.z - current->position_00.z;
            abs_x += (float)fabs(first_y * second_z - first_z * second_y);
            abs_y += (float)fabs(first_z * second_x - first_x * second_z);
            abs_z += (float)fabs(first_x * second_y - first_y * second_x);
        }
        srVector2T<float>* points = static_cast<srVector2T<float>*>(srHeap.allocate(count * 8));
        if (abs_x <= abs_y) {
            if (abs_y <= abs_z) {
                for (int index = 0; index < count; ++index) {
                    points[index].x = polygon.vertices_30[index].position_00.x;
                    points[index].y = polygon.vertices_30[index].position_00.y;
                }
            } else {
                for (int index = 0; index < count; ++index) {
                    points[index].x = polygon.vertices_30[index].position_00.x;
                    points[index].y = polygon.vertices_30[index].position_00.z;
                }
            }
        } else {
            if (abs_x <= abs_z) {
                for (int index = 0; index < count; ++index) {
                    points[index].x = polygon.vertices_30[index].position_00.x;
                    points[index].y = polygon.vertices_30[index].position_00.y;
                }
            } else {
                for (int index = 0; index < count; ++index) {
                    points[index].x = polygon.vertices_30[index].position_00.y;
                    points[index].y = polygon.vertices_30[index].position_00.z;
                }
            }
        }
        if (isClockwise(points, count)) {
            for (int index = 0; index < count; ++index) {
                points[index].y *= -1.0f;
            }
        }
        srTriangulator triangulator(points, count);
        srVector3i indices = triangulator.next();
        while (indices.x != -1) {
            triangle.vertices_30[0] = polygon.vertices_30[indices.x];
            triangle.vertices_30[1] = polygon.vertices_30[indices.y];
            triangle.vertices_30[2] = polygon.vertices_30[indices.z];
            addTriangle(triangle);
            indices = triangulator.next();
        }
        srHeap.free(points);
    }
}

// FUNCTION: SURRENDER 0x1003AC00
void srModeler::planarMap(long pass, long layer, const MappingInfo& mapping)
{
    if (0 <= pass && pass < 4 && 0 <= layer && layer < 2 && triangle_count_04 != 0) {
        float u_minimum, u_maximum, v_minimum, v_maximum;
        getAxialBounds(mapping.axis_u_00, u_minimum, u_maximum);
        getAxialBounds(mapping.axis_v_04, v_minimum, v_maximum);
        float u_scale = 0.0f;
        if (u_maximum - u_minimum != 0.0f) {
            u_scale = mapping.u_scale_08 / (u_maximum - u_minimum);
        }
        float v_scale = 0.0f;
        if (v_maximum - v_minimum != 0.0f) {
            v_scale = mapping.v_scale_0c / (v_maximum - v_minimum);
        }
        Triangle* triangle = &triangles_08[0];
        for (unsigned long index = 0; index < triangle_count_04; ++index) {
            for (int vertex = 0; vertex < 3; ++vertex) {
                float* position = &triangle->vertices_30[vertex].position_00.x;
                triangle->vertices_30[vertex].uv_c0[pass * 2 + layer].x =
                    (position[mapping.axis_u_00] - u_minimum) * u_scale + mapping.u_offset_10;
                triangle->vertices_30[vertex].uv_c0[pass * 2 + layer].y =
                    (-position[mapping.axis_v_04] - v_minimum) * v_scale + mapping.v_offset_14;
            }
            ++triangle;
        }
    }
}

// FUNCTION: SURRENDER 0x1003AD60
void srModeler::planarMapAbsolute(long pass, long layer, const MappingInfo& mapping)
{
    if (0 <= pass && pass < 4 && 0 <= layer && layer < 2 && triangle_count_04 != 0) {
        Triangle* triangle = &triangles_08[0];
        for (unsigned long index = 0; index < triangle_count_04; ++index) {
            for (int vertex = 0; vertex < 3; ++vertex) {
                float* position = &triangle->vertices_30[vertex].position_00.x;
                triangle->vertices_30[vertex].uv_c0[pass * 2 + layer].x =
                    position[mapping.axis_u_00] * mapping.u_scale_08 + mapping.u_offset_10;
                triangle->vertices_30[vertex].uv_c0[pass * 2 + layer].y =
                    -(position[mapping.axis_v_04] * mapping.v_scale_0c) + mapping.v_offset_14;
            }
            ++triangle;
        }
    }
}

// FUNCTION: SURRENDER 0x1003AE30
void srModeler::removeMapping(long pass, long layer)
{
    if (0 <= pass && pass < 4 && 0 <= layer && layer < 2 && triangle_count_04 != 0) {
        Triangle* triangle = &triangles_08[0];
        for (unsigned long index = 0; index < triangle_count_04; ++index) {
            for (int vertex = 0; vertex < 3; ++vertex) {
                triangle->vertices_30[vertex].uv_c0[pass * 2 + layer].x = 0.0f;
                triangle->vertices_30[vertex].uv_c0[pass * 2 + layer].y = 0.0f;
            }
            ++triangle;
        }
    }
}

// FUNCTION: SURRENDER 0x1003AEC0
void srModeler::cylinderMap(long pass, long layer, const MappingInfo& mapping)
{
    if (pass < 0 || pass > 3 || layer < 0 || layer > 1 || triangle_count_04 == 0) {
        return;
    }
    e_axis axis = mapping.axis_u_00;
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
        u_scale = mapping.u_scale_08 / (u_maximum - u_minimum);
    }
    Triangle* triangle = &triangles_08[0];
    int vertex;
    for (unsigned long index = 0; index < triangle_count_04; ++index) {
        for (vertex = 0; vertex < 3; ++vertex) {
            float* position = &triangle->vertices_30[vertex].position_00.x;
            srVector2T<float>* uv = &triangle->vertices_30[vertex].uv_c0[pass * 2 + layer];
            float angle = (float)atan2(position[second_axis], position[third_axis]);
            uv->x = (position[axis] - u_minimum) * u_scale + mapping.u_offset_10;
            uv->y = -(angle / (float)(pi * 2.0)) * mapping.v_scale_0c + mapping.v_offset_14;
        }
        float* previous = &triangle->vertices_30[0].uv_c0[pass * 2 + layer].y;
        for (vertex = 1; vertex < 3; ++vertex) {
            float* current = &triangle->vertices_30[vertex % 3].uv_c0[pass * 2 + layer].y;
            if (mapping.v_scale_0c * 0.8f < fabs(*current - *previous)) {
                if (*previous <= *current) {
                    *previous += mapping.v_scale_0c;
                } else {
                    *current += mapping.v_scale_0c;
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
    int row = 0;
    if (0 < rows) {
        do {
            int column = 0;
            if (0 < columns) {
                float top = (row + 1) / (float)rows - 0.5f;
                float bottom = row / (float)rows - 0.5f;
                do {
                    Triangle triangle;
                    float column_position = (float)column;
                    ++column;
                    float left = column_position / columns - 0.5f;
                    float right = column / (float)columns - 0.5f;
                    triangle.vertices_30[0].position_00.Set(left, bottom, 0.0);
                    triangle.vertices_30[1].position_00.Set(left, top, 0.0);
                    triangle.vertices_30[2].position_00.Set(right, top, 0.0);
                    addTriangle(triangle);
                    triangle.vertices_30[1].position_00.Set(right, bottom, 0.0);
                    triangle.flipFacing();
                    addTriangle(triangle);
                } while (column < columns);
            }
            ++row;
        } while (row < rows);
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
                    triangle.vertices_30[0].position_00.Set(cosine * ring, sine * ring, z1);
                    triangle.vertices_30[1].position_00.Set(next_cosine * ring, next_sine * ring,
                                                            z1);
                    triangle.vertices_30[2].position_00.Set(cosine * radius2, sine * radius2, z2);
                    addTriangle(triangle);
                    triangle.vertices_30[0].position_00.Set(next_cosine * radius2,
                                                            next_sine * radius2, z2);
                    triangle.flipFacing();
                    addTriangle(triangle);
                    triangle.vertices_30[0].position_00.Set(cosine * ring, sine * ring, nz1);
                    triangle.vertices_30[1].position_00.Set(next_cosine * ring, next_sine * ring,
                                                            nz1);
                    triangle.vertices_30[2].position_00.Set(cosine * radius2, sine * radius2, nz2);
                    triangle.flipFacing();
                    addTriangle(triangle);
                    triangle.vertices_30[0].position_00.Set(next_cosine * radius2,
                                                            next_sine * radius2, nz2);
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
                    triangle.vertices_30[0].position_00.Set(tube * cosine, tube * sine, depth);
                    triangle.vertices_30[1].position_00.Set(tube * next_cosine, tube * next_sine,
                                                            depth);
                    triangle.vertices_30[2].position_00.Set(next_tube * cosine, next_tube * sine,
                                                            next_depth);
                    addTriangle(triangle);
                    triangle.vertices_30[0].position_00.Set(next_tube * next_cosine,
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
    if (triangle < triangle_count_04 && 0.0 < threshold) {
        Triangle* source = &triangles_08[triangle];
        double longest = 0.0;
        int edge = 0;
        int vertex = 0;
        do {
            int next = (vertex + 1) % 3;
            double dx = source->vertices_30[vertex].position_00.x -
                        (double)source->vertices_30[next].position_00.x;
            double dy = source->vertices_30[vertex].position_00.y -
                        (double)source->vertices_30[next].position_00.y;
            double dz = source->vertices_30[vertex].position_00.z -
                        (double)source->vertices_30[next].position_00.z;
            double distance = sqrt(dx * dx + dy * dy + dz * dz);
            if (longest < distance) {
                longest = distance;
                edge = vertex;
            }
            ++vertex;
        } while (vertex < 3);
        if (threshold < longest && longest != 0.0) {
            Triangle child;
            for (int pass = 0; pass < 4; ++pass) {
                child.shaders_20[pass] = source->shaders_20[pass];
                for (int layer = 0; layer < 2; ++layer) {
                    child.textures_00[pass][layer] = source->textures_00[pass][layer];
                }
            }
            child.flags_360 = source->flags_360;
            child.vertices_30[0] = source->vertices_30[(edge + 2) % 3];
            child.vertices_30[1] = source->vertices_30[edge];
            child.vertices_30[2].interpolate(source->vertices_30[edge],
                                             source->vertices_30[(edge + 1) % 3], 0.5f);
            source->vertices_30[edge] = child.vertices_30[2];
            unsigned long added = addTriangle(child);
            tesselateEdges(triangle, threshold);
            tesselateEdges(added, threshold);
        }
    }
}

// FUNCTION: SURRENDER 0x10039100
void srModeler::tesselateEdges(double threshold)
{
    unsigned long count = triangle_count_04;
    for (unsigned long index = 0; index < count; ++index) {
        tesselateEdges(index, threshold);
    }
}

// FUNCTION: SURRENDER 0x1003BB40
void srModeler::setMaterial(srMaterialIFace* material, long pass, srMeshModel::e_side side)
{
    if (0 <= pass && pass < 4 &&
        (side == static_cast<srMeshModel::e_side>(0) ||
         side == static_cast<srMeshModel::e_side>(1))) {
        Triangle* triangle = &triangles_08[0];
        for (long index = 0; index < (long)triangle_count_04; ++index) {
            for (int vertex = 0; vertex < 3; ++vertex) {
                triangle->vertices_30[vertex].materials_10[pass][side] = material;
            }
            ++triangle;
        }
    }
}

// FUNCTION: SURRENDER 0x1003BBC0
void srModeler::setTexture(srTextureIFace* texture, long pass, long layer)
{
    if (0 <= pass && pass < 4 && 0 <= layer && layer < 2) {
        Triangle* triangle = &triangles_08[0];
        for (long index = 0; index < (long)triangle_count_04; ++index) {
            triangle->textures_00[pass][layer] = texture;
            ++triangle;
        }
    }
}

// FUNCTION: SURRENDER 0x1003BC20
void srModeler::setShader(srShader shader, long pass)
{
    if (0 <= pass && pass < 4) {
        Triangle* triangle = &triangles_08[0];
        for (long index = 0; index < (long)triangle_count_04; ++index) {
            triangle->shaders_20[pass] = shader;
            ++triangle;
        }
    }
}

// FUNCTION: SURRENDER 0x1003B160
void srModeler::convert(srMeshModel& model, int preserve)
{
    if (preserve == 0) {
        disableDegenerateTriangles();
        removeDisabledTriangles();
    }
    if (triangle_count_04 == 0 || pass_count_10 == 0) {
        model.reset(0, 0);
        return;
    }
    VertexHash* hash = getUniqueVertexList();
    model.reset(triangle_count_04, hash->unique_count_1008);
    model.clearDirty(static_cast<srMeshModel::e_flags>(1));
    model.clearDirty(static_cast<srMeshModel::e_flags>(2));
    model.pass_count_228 = pass_count_10;
    if (pass_count_10 < 1) {
        model.pass_count_228 = 1;
    } else if (4 < pass_count_10) {
        model.pass_count_228 = 4;
    }
    unsigned long unique = hash->unique_count_1008;
    unsigned long count = triangle_count_04;
    unsigned long index;
    long layer;
    long side;
    Vertex* vertex;
    VertexHash::Entry* entry;
    for (long pass = 0; pass < pass_count_10; ++pass) {
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
        Triangle* triangle = &triangles_08[0];
        shader = triangle->shaders_20[pass];
        srTextureIFace* texture[2];
        texture[0] = triangle->textures_00[pass][0];
        texture[1] = triangle->textures_00[pass][1];
        if (1 < count) {
            triangle = &triangles_08[1];
            for (index = count - 1; index != 0; --index) {
                for (layer = 0; layer < 2; ++layer) {
                    if (triangle->textures_00[pass][layer] != texture[layer]) {
                        same_texture[layer] = 0;
                    }
                }
                if (triangle->shaders_20[pass].value != shader.value) {
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
            triangle = &triangles_08[0];
            for (index = count; index != 0; --index) {
                *table = triangle->shaders_20[pass];
                ++table;
                ++triangle;
            }
        }
        for (layer = 0; layer < 2; ++layer) {
            if (same_texture[layer] == 0) {
                srPtr<srTextureIFace>* table = model.getPolyTexture(pass, layer, 1);
                triangle = &triangles_08[0];
                for (index = count; index != 0; --index) {
                    *table = triangle->textures_00[pass][layer];
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
            vertex = hash->entries_00[index].vertex_08;
            if (index == 0) {
                material_front = vertex->materials_10[pass][0];
                material_back = vertex->materials_10[pass][1];
            }
            if (vertex->dcg_30[pass].x != 1.0f || vertex->dcg_30[pass].y != 1.0f ||
                vertex->dcg_30[pass].z != 1.0f || vertex->weights_100[pass] != 1.0f) {
                same_dcg = false;
            }
            if (vertex->dig_60[pass].x != 0.0f || vertex->dig_60[pass].y != 0.0f ||
                vertex->dig_60[pass].z != 0.0f) {
                same_dig = false;
            }
            if (vertex->scg_90[pass].x != 1.0f || vertex->scg_90[pass].y != 1.0f ||
                vertex->scg_90[pass].z != 1.0f) {
                same_scg = false;
            }
            if (vertex->materials_10[pass][0] != material_front) {
                same_material[0] = 0;
            }
            if (vertex->materials_10[pass][1] != material_back) {
                same_material[1] = 0;
            }
            for (layer = 0; layer < 2; ++layer) {
                if (vertex->uv_c0[pass * 2 + layer].x != 0.0f ||
                    vertex->uv_c0[pass * 2 + layer].y != 0.0f) {
                    same_uv[layer] = 0;
                }
            }
        }
        if (!same_dcg) {
            srVector4T<float>* table = model.getVertexDCG(pass, 1);
            entry = hash->entries_00;
            for (index = unique; index != 0; --index) {
                vertex = entry->vertex_08;
                table->x = vertex->dcg_30[pass].x;
                table->y = vertex->dcg_30[pass].y;
                table->z = vertex->dcg_30[pass].z;
                table->w = vertex->weights_100[pass];
                ++table;
                ++entry;
            }
        }
        if (!same_scg) {
            srVector4T<float>* table = model.getVertexSCG(pass, 1);
            entry = hash->entries_00;
            for (index = unique; index != 0; --index) {
                vertex = entry->vertex_08;
                table->x = vertex->scg_90[pass].x;
                table->y = vertex->scg_90[pass].y;
                table->z = vertex->scg_90[pass].z;
                table->w = 1.0f;
                ++table;
                ++entry;
            }
        }
        if (!same_dig) {
            srVector3T<float>* table = model.getVertexDIG(pass, 1);
            entry = hash->entries_00;
            for (index = unique; index != 0; --index) {
                vertex = entry->vertex_08;
                table->x = vertex->dig_60[pass].x;
                table->y = vertex->dig_60[pass].y;
                table->z = vertex->dig_60[pass].z;
                ++table;
                ++entry;
            }
        }
        model.setMaterial(material_front, pass, static_cast<srMeshModel::e_side>(0));
        model.setMaterial(material_back, pass, static_cast<srMeshModel::e_side>(1));
        for (side = 0; side < 2; ++side) {
            if (same_material[side] == 0) {
                srPtr<srMaterialIFace>* table =
                    model.getVertexMaterial(pass, static_cast<srMeshModel::e_side>(side), 1);
                if (unique != 0) {
                    entry = hash->entries_00;
                    for (index = unique; index != 0; --index) {
                        *table = entry->vertex_08->materials_10[pass][side];
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
                    entry = hash->entries_00;
                    for (index = unique; index != 0; --index) {
                        *table = entry->vertex_08->uv_c0[pass * 2 + layer];
                        ++table;
                        ++entry;
                    }
                }
            }
        }
    }
    srVector3T<float>* locations = model.getVertexLoc();
    for (index = 0; index < unique; ++index) {
        locations[index] = hash->entries_00[index].vertex_08->position_00;
    }
    srVector3i* polygons = model.getPolyVertex();
    for (index = 0; index < count; ++index) {
        polygons[index].x = hash->table_1004[index * 3]->index_10;
        polygons[index].y = hash->table_1004[index * 3 + 1]->index_10;
        polygons[index].z = hash->table_1004[index * 3 + 2]->index_10;
    }
    unsigned long* shades = model.getVertexShadeIndex(1);
    for (index = 0; index < unique; ++index) {
        shades[index] = hash->entries_00[index].shade_index_04;
    }
    model.setDirty(static_cast<srMeshModel::e_flags>(0));
    model.setDirty(static_cast<srMeshModel::e_flags>(1));
    model.setDirty(static_cast<srMeshModel::e_flags>(2));
    model.setDirty(static_cast<srMeshModel::e_flags>(3));
    delete hash;
}

// SYNTHETIC: SURRENDER 0x10037DC0
// srModeler::MappingInfo default constructor closure
