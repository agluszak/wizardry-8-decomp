#include "surrender/srTriangulator.h"

// FUNCTION: SURRENDER 0x1003C1E0
srTriangulator::CircularList::CircularList(int count)
{
    count_00 = count;
    nodes_04 = new ListNode[count];
    nodes_04->prev_08 = nodes_04 + count - 1;
    nodes_04->next_04 = nodes_04 + 1;
    nodes_04[count - 1].prev_08 = nodes_04 + count - 2;
    nodes_04[count - 1].next_04 = nodes_04;
    for (int index = 1; index < count - 1; ++index) {
        nodes_04[index].prev_08 = nodes_04 + index - 1;
        nodes_04[index].next_04 = nodes_04 + index + 1;
    }
}

// FUNCTION: SURRENDER 0x1003C260
srTriangulator::CircularList::~CircularList()
{
    delete[] nodes_04;
}

// FUNCTION: SURRENDER 0x1003C270
void srTriangulator::CircularList::erase(ListIterator iterator)
{
    ListNode* next = (iterator + 1).node_00;
    ListNode* previous = (iterator - 1).node_00;
    next->prev_08 = previous;
    previous->next_04 = next;
    --count_00;
}

// FUNCTION: SURRENDER 0x1003C2D0
srTriangulator::CircularList::ListIterator
srTriangulator::CircularList::ListIterator::operator+(int distance)
{
    ListNode* node = node_00;
    while (distance > 0) {
        --distance;
        node = node->next_04;
    }
    ListIterator result;
    result.node_00 = node;
    return result;
}

// FUNCTION: SURRENDER 0x1003C2B0
srTriangulator::CircularList::ListIterator
srTriangulator::CircularList::ListIterator::operator-(int distance)
{
    ListNode* node = node_00;
    while (distance > 0) {
        --distance;
        node = node->prev_08;
    }
    ListIterator result;
    result.node_00 = node;
    return result;
}

// FUNCTION: SURRENDER 0x1003BEA0
srTriangulator::srTriangulator(srVector2T<float>* points, int count)
    : list_04(count), points_0c(points)
{
    int index = 0;
    for (CircularList::ListNode* node = list_04.nodes_04; index < count; ++index) {
        node->vertex_00 = index;
        node = node->next_04;
    }
    current_00.node_00 = list_04.nodes_04;
}

srTriangulator::~srTriangulator() {}

// FUNCTION: SURRENDER 0x1003BEE0
int srTriangulator::sameSide(const srVector2T<float>& p1, const srVector2T<float>& p2,
                             const srVector2T<float>& a, const srVector2T<float>& b)
{
    float first = (p1.x - a.x) * (b.y - a.y) - (p1.y - a.y) * (b.x - a.x);
    float second = (p2.x - a.x) * (b.y - a.y) - (p2.y - a.y) * (b.x - a.x);
    return first * second > 0.0f;
}

// FUNCTION: SURRENDER 0x1003BF40
int srTriangulator::isInsideTriangle(const srVector2T<float>& p, const srVector2T<float>& a,
                                     const srVector2T<float>& b, const srVector2T<float>& c)
{
    return sameSide(p, a, b, c) && sameSide(p, b, a, c) && sameSide(p, c, a, b);
}

// FUNCTION: SURRENDER 0x1003BFA0
int srTriangulator::satisfyConstraints(CircularList::ListIterator iterator)
{
    srVector2T<float> previous = points_0c[*(iterator - 1)];
    srVector2T<float> vertex = points_0c[*iterator];
    srVector2T<float> next = points_0c[*(iterator + 1)];

    if ((next.y - vertex.y) * (previous.x - vertex.x) -
            (previous.y - vertex.y) * (next.x - vertex.x) <=
        0.0) {
        return 0;
    }

    CircularList::ListNode* node = (iterator + 2).node_00;
    do {
        if (isInsideTriangle(points_0c[node->vertex_00], previous, vertex, next)) {
            return 0;
        }
        node = node->next_04;
    } while (node != (iterator - 1).node_00);
    return 1;
}

// FUNCTION: SURRENDER 0x1003C0E0
srVector3i srTriangulator::next()
{
    srVector3i triangle;
    if (list_04.count_00 < 4) {
        if (list_04.count_00 == 3) {
            CircularList::ListNode* node = current_00.node_00->next_04;
            triangle.x = current_00.node_00->vertex_00;
            triangle.y = node->vertex_00;
            triangle.z = node->next_04->vertex_00;
            list_04.count_00 = 0;
            return triangle;
        }
    } else {
        CircularList::ListNode* first = current_00.node_00;
        int satisfied = satisfyConstraints(current_00);
        while (true) {
            if (satisfied != 0) {
                CircularList::ListIterator forward = current_00 + 1;
                CircularList::ListIterator backward = current_00 - 1;
                triangle.x = *backward;
                triangle.y = *current_00;
                triangle.z = *forward;
                CircularList::ListIterator previous = current_00 - 1;
                list_04.erase(current_00);
                current_00 = previous;
                return triangle;
            }
            current_00.node_00 = current_00.node_00->next_04;
            if (current_00.node_00 == first) {
                break;
            }
            satisfied = satisfyConstraints(current_00);
        }
    }
    triangle.x = -1;
    triangle.y = 0;
    triangle.z = 0;
    return triangle;
}
