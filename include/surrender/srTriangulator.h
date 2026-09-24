#pragma once

#include "srMath.h"

/* Recovered SR provider utility. No known Wizardry/JPEG/ZIP consumer imports
   this class, so provider exports do not justify consumer dllimport codegen. */
class srTriangulator {
public:
    /* Doubly-linked circular vertex list. The constructor allocates the node
       array with a raw scalar operator new and links every node to its
       neighbors; node[0]'s prev is the tail and the tail's next is node[0]. */
    class CircularList {
    public:
        class Node {
        public:
            long index_00;
            Node* next_04;
            Node* prev_08;
        };

        static_assert(sizeof(Node) == 0xc, "CircularList_Node_must_be_0xc");

        /* Value-type iterator; operator+ walks next_04 and operator- walks
           prev_08 the requested number of links. */
        class ListIterator {
        public:
            ListIterator operator+(int distance) const;
            ListIterator operator-(int distance) const;

            Node* node_00;
        };

        CircularList(int count);
        ~CircularList();

        void erase(ListIterator position);

        long count_00;
        Node* nodes_04;
    };

    static_assert(sizeof(CircularList) == 0x8, "CircularList_must_be_0x8");

    srTriangulator(srVector2T<float>* points, int count);
    ~srTriangulator();
    srTriangulator& operator=(const srTriangulator& other);

    srVector3i next();

private:
    int sameSide(const srVector2T<float>& p1, const srVector2T<float>& p2,
                 const srVector2T<float>& a, const srVector2T<float>& b);
    int isInsideTriangle(const srVector2T<float>& p, const srVector2T<float>& a,
                         const srVector2T<float>& b, const srVector2T<float>& c);
    int satisfyConstraints(CircularList::ListIterator iterator);

    CircularList::ListIterator current_00;
    CircularList list_04;
    srVector2T<float>* points_0c;
};

static_assert((sizeof(srTriangulator) == 0x10), "srTriangulator_must_be_0x10");
