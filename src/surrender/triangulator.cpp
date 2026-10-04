#include "surrender/srTriangulator.h"

// FUNCTION: SURRENDER 0x1003c2d0
srTriangulator::CircularList::ListIterator
srTriangulator::CircularList::ListIterator::operator+(int distance) const
{
    ListIterator result = *this;
    while (distance > 0) {
        distance--;
        result.node_00 = result.node_00->next_04;
    }
    return result;
}

// FUNCTION: SURRENDER 0x1003c2b0
srTriangulator::CircularList::ListIterator
srTriangulator::CircularList::ListIterator::operator-(int distance) const
{
    ListIterator result = *this;
    while (distance > 0) {
        distance--;
        result.node_00 = result.node_00->prev;
    }
    return result;
}

// FUNCTION: SURRENDER 0x1003c270
void srTriangulator::CircularList::erase(ListIterator position)
{
    ListIterator next = position + 1;
    ListIterator previous = position - 1;
    next.node_00->prev = previous.node_00;
    previous.node_00->next_04 = next.node_00;
    count_00--;
}

// FUNCTION: SURRENDER 0x1003c1e0
srTriangulator::CircularList::CircularList(int count)
{
    count_00 = count;
    nodes = static_cast<Node*>(operator new(count * sizeof(Node)));
    nodes[0].prev = &nodes[count - 1];
    nodes[0].next_04 = &nodes[1];
    nodes[count - 1].prev = &nodes[count - 2];
    nodes[count - 1].next_04 = nodes;
    for (int i = 1; i < count - 1; i++) {
        nodes[i].prev = &nodes[i - 1];
        nodes[i].next_04 = &nodes[i + 1];
    }
}

// FUNCTION: SURRENDER 0x1003c260
srTriangulator::CircularList::~CircularList()
{
    delete nodes;
}

// FUNCTION: SURRENDER 0x1003bea0
srTriangulator::srTriangulator(srVector2T<float>* points, int count) : list(count)
{
    points_0c = points;
    CircularList::Node* node = list.nodes;
    for (int i = 0; i < count; i++) {
        node->index_00 = i;
        node = node->next_04;
    }
    current.node_00 = list.nodes;
}

// FUNCTION: SURRENDER 0x1003c0e0
srVector3i srTriangulator::next()
{
    srVector3i result;
    if (list.count_00 > 3) {
        CircularList::Node* start = current.node_00;
        while (!satisfyConstraints(current)) {
            current.node_00 = current.node_00->next_04;
            if (current.node_00 == start) {
                result.x = -1;
                result.y = 0;
                result.z = 0;
                return result;
            }
        }
        CircularList::Node* next_node = (current + 1).node_00;
        result.x = (current - 1).node_00->index_00;
        result.y = current.node_00->index_00;
        result.z = next_node->index_00;
        list.erase(current);
        current = current - 1;
        return result;
    }
    if (list.count_00 == 3) {
        CircularList::Node* next_node = current.node_00->next_04;
        result.x = current.node_00->index_00;
        result.y = next_node->index_00;
        result.z = next_node->next_04->index_00;
        list.count_00 = 0;
        return result;
    }
    result.x = -1;
    result.y = 0;
    result.z = 0;
    return result;
}

// FUNCTION: SURRENDER 0x1003bfa0
int srTriangulator::satisfyConstraints(CircularList::ListIterator iterator)
{
    srVector2T<float> a = points_0c[(iterator - 1).node_00->index_00];
    srVector2T<float> b = points_0c[iterator.node_00->index_00];
    srVector2T<float> c = points_0c[(iterator + 1).node_00->index_00];
    if ((c.y - b.y) * (a.x - b.x) - (a.y - b.y) * (c.x - b.x) <= 0.0) {
        return 0;
    }
    CircularList::Node* node = (iterator + 2).node_00;
    while (!isInsideTriangle(points_0c[node->index_00], a, b, c)) {
        node = node->next_04;
        if (node == (iterator - 1).node_00) {
            return 1;
        }
    }
    return 0;
}

// FUNCTION: SURRENDER 0x1003bee0
int srTriangulator::sameSide(const srVector2T<float>& p1, const srVector2T<float>& p2,
                             const srVector2T<float>& a, const srVector2T<float>& b)
{
    float first = (p1.x - a.x) * (b.y - a.y) - (p1.y - a.y) * (b.x - a.x);
    float second = (p2.x - a.x) * (b.y - a.y) - (p2.y - a.y) * (b.x - a.x);
    return first * second > 0.0f;
}

// FUNCTION: SURRENDER 0x1003bf40
int srTriangulator::isInsideTriangle(const srVector2T<float>& p, const srVector2T<float>& a,
                                     const srVector2T<float>& b, const srVector2T<float>& c)
{
    return sameSide(p, a, b, c) && sameSide(p, b, a, c) && sameSide(p, c, a, b);
}
