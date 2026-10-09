#pragma once

#include "timer.h"
#include <stddef.h>

#include "surrender/srMath.h"
#include "surrender/srModelInstance.h"
#include "wiz8/engine_code/AnimRep.hpp"
#include "wiz8/engine_code/GrObject.h"

struct W8World;
struct W8ReadLevelInfo;
struct W8Item;
class Trigger;

/* Bit masks in W8ItemRep::flags and the saved W8WorldItem::entity_flags.
   The world item's own flags are a separate domain. */
enum {
    W8_ITEM_ENTITY_PULSE = 0x01u,
    W8_ITEM_ENTITY_ROTATE = 0x02u,
    W8_ITEM_ENTITY_NO_PICKUP = 0x04u,
    W8_ITEM_ENTITY_RADAR_SEEN = 0x08u,
    W8_ITEM_ENTITY_HIGHLIGHT_GREEN = 0x10u,
    W8_ITEM_ENTITY_PULSE_INCREASING = 0x20u,
    W8_ITEM_ENTITY_MESH_LOADED = 0x40u,
    W8_ITEM_ENTITY_HIGHLIGHT_BLUE = 0x80u
};

/* Engine Code\Item.cpp. The assertion expressions establish the original
   m_pRep and m_psrMesh names; the bodies establish their offsets. */
struct W8ItemRep : public W8AnimRepBase {
    W8ItemRep();
    virtual ~W8ItemRep() override;

    srModelInstance* m_psrMesh;       /* 0x64 */
    srVector3T<float> bounds_minimum; /* 0x68 */
    srVector3T<float> bounds_maximum; /* 0x74 */
    float bounds_radius;              /* 0x80 */
    unsigned char padding_84[4];
    float pulse_level; /* 0x88 */
    unsigned char padding_8c[4];
    unsigned int flags; /* 0x90 */

    bool ReadFromFile(W8ReadLevelInfo* info, W8Item* item, bool anonymous_mesh);
    void RefreshBounds();
    unsigned int SetFlags(unsigned int mask, bool enabled); /* 0x0049F310 */
};

struct W8Item : public W8GrObject {
    W8Item();
    virtual ~W8Item() override;

    Trigger* trigger;
    /* 0x1c: countdown clock; IsTicking reports whether it is running. */
    TIMER countdown;

    void DetachMesh(W8World* world);
    void ApplyRepTransform();
    void AttachMesh(W8World* world);
    void UpdateAnimation();
    void SetLocation(const srVector3T<float>* location);
    srNode* GetMesh();
    /* Model-local cached bounds. GetItemWorldBounds translates them by the
       representation's location without rotating them. */
    unsigned char GetCachedLocalBounds(srVector3T<float>* lower, srVector3T<float>* upper);
    bool GetBoundsRadius(float* radius);
    /* Live search/interaction point: representation location raised by 0.66 of
       the cached local Y extent. */
    unsigned char GetSearchPosition(srVector3T<float>* location);
    void SetYaw(float angle);
    void SetHighlight(bool enabled);
    bool IsSelected();
    float DistanceToCamera(W8World* world);
    void LightRadarBlip();
    bool IsRadarBlipLit();
};

/* 0x0049F350 allocates the 0x94-byte representation; 0x0049F5D0 allocates
   the 0x20-byte item. */
W8_ABI_ASSERT(sizeof(W8ItemRep) == 0x94, "W8ItemRep_size");
W8_ABI_ASSERT(sizeof(W8Item) == 0x20, "W8Item_size");
W8_ABI_ASSERT(offsetof(W8ItemRep, m_psrMesh) == 0x64, "W8ItemRep_m_psrMesh_offset");
W8_ABI_ASSERT(offsetof(W8ItemRep, bounds_minimum) == 0x68, "W8ItemRep_bounds_minimum_offset");
W8_ABI_ASSERT(offsetof(W8ItemRep, bounds_maximum) == 0x74, "W8ItemRep_bounds_maximum_offset");
W8_ABI_ASSERT(offsetof(W8ItemRep, bounds_radius) == 0x80, "W8ItemRep_bounds_radius_offset");
W8_ABI_ASSERT(offsetof(W8ItemRep, pulse_level) == 0x88, "W8ItemRep_pulse_level_offset");
W8_ABI_ASSERT(offsetof(W8ItemRep, flags) == 0x90, "W8ItemRep_flags_offset");
W8_ABI_ASSERT(offsetof(W8Item, trigger) == 0x18, "W8Item_trigger_offset");
W8_ABI_ASSERT(offsetof(W8Item, countdown) == 0x1c, "W8Item_countdown_01c_offset");

bool ReadItemFromFile(W8ReadLevelInfo* info, W8Item** item, bool anonymous_mesh);
/* Run the item's trigger, if any, and report its action state. */
unsigned char RunItemTrigger(W8Item* item); /* 0x004A0070 */
bool GetItemWorldBounds(W8Item* item, srVector3T<float>* lower, srVector3T<float>* upper);

bool LoadItemFromFile(const W8ReadLevelInfo* context, const char* name, W8Item** item,
                      bool anonymous_mesh);
