#pragma once

#include "srMath.h"

/* Recovered SR provider utility. No known Wizardry/JPEG/ZIP consumer imports
   this class, so provider exports do not justify consumer dllimport codegen. */
class SR_DLL_EXPORT srTriangulator {
public:
    /* Doubly-linked circular vertex list. The constructor allocates the node
       array with a raw scalar operator new and links every node to its
       neighbors; node[0]'s prev is the tail and the tail's next is node[0]. */
    class CircularList {
    public:
        class Node {
        public:
            long index;
            Node* next;
            Node* prev;
        };

        static_assert(sizeof(Node) == 0xc, "CircularList_Node_must_be_0xc");

        /* Value-type iterator; operator+ walks next and operator- walks
           prev the requested number of links. */
        class ListIterator {
        public:
            ListIterator operator+(int distance) const;
            ListIterator operator-(int distance) const;

            Node* node;
        };
        static_assert(sizeof(ListIterator) == 0x04,
                      "srTriangulator_CircularList_ListIterator_must_be_0x04");

        CircularList(int count);
        ~CircularList();

        void erase(ListIterator position);

        long count;
        Node* nodes;
    };

    static_assert(sizeof(CircularList) == 0x8, "CircularList_must_be_0x8");

    srTriangulator(srVector2T<float>* points, int count);

    srVector3i next();

private:
    int sameSide(const srVector2T<float>& p1, const srVector2T<float>& p2,
                 const srVector2T<float>& a, const srVector2T<float>& b);
    int isInsideTriangle(const srVector2T<float>& p, const srVector2T<float>& a,
                         const srVector2T<float>& b, const srVector2T<float>& c);
    int satisfyConstraints(CircularList::ListIterator iterator);

    CircularList::ListIterator current;
    CircularList list;
    srVector2T<float>* points;
};

static_assert((sizeof(srTriangulator) == 0x10), "srTriangulator_must_be_0x10");
