#pragma once

#include "wiz8/vector.h"
#include "surrender/srTypeRegistry.h"

/* Engine Code\stMeshModel.cpp. Only fields reached by reviewed bodies are
   modeled. The two short-vector pairs are parallel key/value tables; their
   semantic domain is not established, so the names stay positional. */
class W8MeshModel {
public:
    /* The two SurRender registry slots. The class registers itself as
       "stMeshModel" under id 0x10003, which is the original class name and
       puts it in the Wizardry-registered id range rather than SurRender's own.
       They are members but not spelled `virtual` here for the same reason the
       vptr is not: the vtable is inside unknown_000, and declaring virtuals
       would have VC6 synthesize a second one and shift every offset below. */
    const char* getClassName() const;     /* 0x004741F0 */
    unsigned long getClassID() const;     /* 0x004741E0 */
    class srRegistry::ClassNode* getClassNode() const;  /* 0x00474820 */

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
