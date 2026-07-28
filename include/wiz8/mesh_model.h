#pragma once

#include "wiz8/vector.h"

/* Engine Code\stMeshModel.cpp. Only fields reached by reviewed bodies are
   modeled. The two short-vector pairs are parallel key/value tables; their
   semantic domain is not established, so the names stay positional. */
class W8MeshModel {
public:
    int FindMappedIndex(short key);       /* 0x004712D0 */
    void LinkTo(W8MeshModel* other);      /* 0x00471D60 */
    void* GetVertex(unsigned int index);  /* 0x00471AA0 */

    unsigned char unknown_000[0x398];
    W8MeshModel* next;                    /* 0x398 */
    W8MeshModel* previous;                /* 0x39c */
    unsigned char unknown_3a0[0x30];
    unsigned int vertex_count;            /* 0x3d0 */
    unsigned char unknown_3d4[0xc];
    void** vertices;                      /* 0x3e0 */
    unsigned char unknown_3e4[0x3c];
    W8GrowableVector<short> mapped_values; /* 0x420 */
    W8GrowableVector<short> mapped_keys;   /* 0x430 */
};

int FindMappedIndexInMeshChain(
    W8MeshModel** mesh, int key);          /* 0x004A8D10 */
