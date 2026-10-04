#include "surrender/srTriangulator.h"

// FUNCTION: SURRENDER 0x1003c2d0
srTriangulator::CircularList::ListIterator
srTriangulator::CircularList::ListIterator::operator+(int distance) const
{
    ListIterator result = *this;
    while (distance > 0) {
        distance--;
        result.node = result.node->next;
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
        result.node = result.node->prev;
    }
    return result;
}

// FUNCTION: SURRENDER 0x1003c270
void srTriangulator::CircularList::erase(ListIterator position)
{
    ListIterator next = position + 1;
    ListIterator previous = position - 1;
    next.node->prev = previous.node;
    previous.node->next = next.node;
    count--;
}

// FUNCTION: SURRENDER 0x1003c1e0
srTriangulator::CircularList::CircularList(int count)
{
    this->count = count;
    nodes = static_cast<Node*>(operator new(count * sizeof(Node)));
    nodes[0].prev = &nodes[count - 1];
    nodes[0].next = &nodes[1];
    nodes[count - 1].prev = &nodes[count - 2];
    nodes[count - 1].next = nodes;
    for (int i = 1; i < count - 1; i++) {
        nodes[i].prev = &nodes[i - 1];
        nodes[i].next = &nodes[i + 1];
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
    this->points = points;
    CircularList::Node* node = list.nodes;
    for (int i = 0; i < count; i++) {
        node->index = i;
        node = node->next;
    }
    current.node = list.nodes;
}

// FUNCTION: SURRENDER 0x1003c0e0
srVector3i srTriangulator::next()
{
    srVector3i result;
    if (list.count > 3) {
        CircularList::Node* start = current.node;
        while (!satisfyConstraints(current)) {
            current.node = current.node->next;
            if (current.node == start) {
                result.x = -1;
                result.y = 0;
                result.z = 0;
                return result;
            }
        }
        CircularList::Node* next_node = (current + 1).node;
        result.x = (current - 1).node->index;
        result.y = current.node->index;
        result.z = next_node->index;
        list.erase(current);
        current = current - 1;
        return result;
    }
    if (list.count == 3) {
        CircularList::Node* next_node = current.node->next;
        result.x = current.node->index;
        result.y = next_node->index;
        result.z = next_node->next->index;
        list.count = 0;
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
    srVector2T<float> a = points[(iterator - 1).node->index];
    srVector2T<float> b = points[iterator.node->index];
    srVector2T<float> c = points[(iterator + 1).node->index];
    if ((c.y - b.y) * (a.x - b.x) - (a.y - b.y) * (c.x - b.x) <= 0.0) {
        return 0;
    }
    CircularList::Node* node = (iterator + 2).node;
    while (!isInsideTriangle(points[node->index], a, b, c)) {
        node = node->next;
        if (node == (iterator - 1).node) {
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
