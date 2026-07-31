#pragma once

/* The plain scene node the world's dynamic branch and every W8Navigator
   allocate. Split out of registry_classes.h so units that need only this one
   class - Navigator.cpp allocates one per navigator - do not have to include
   that aggregate. */

#include "surrender/srNode.h"

// VTABLE: WIZ8 0x005ec208
class W8Node005EC208 : public srNode {
public:
    explicit W8Node005EC208(srNode* parent) : srNode(parent) {}
    virtual ~W8Node005EC208() override;

};

static_assert(sizeof(W8Node005EC208) == 0x138,
              "W8Node005EC208_must_be_0x138");
