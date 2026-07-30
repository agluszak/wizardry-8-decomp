#include "wiz8/grcycle.h"

#include "surrender/srNode.h"

#include <string.h>

namespace {

unsigned int float_bits(float value)
{
    union {
        float floating;
        unsigned int bits;
    } representation;
    representation.floating = value;
    return representation.bits;
}

W8Navigator** g_navigators_659b3c;
int g_navigator_count_659b34;
int g_navigator_capacity_659b38;

void RegisterNavigator(W8Navigator* navigator)
{
    W8Navigator** replacement;
    int index;
    int new_count = g_navigator_count_659b34 + 1;

    if (g_navigator_capacity_659b38 < new_count) {
        replacement = static_cast<W8Navigator**>(
            ::operator new(new_count * sizeof(*replacement)));
        if (replacement == 0) {
            return;
        }
        for (index = 0; index != g_navigator_count_659b34; ++index) {
            replacement[index] = g_navigators_659b3c[index];
        }
        ::operator delete(g_navigators_659b3c);
        g_navigators_659b3c = replacement;
        g_navigator_capacity_659b38 = new_count;
    }
    g_navigators_659b3c[g_navigator_count_659b34++] = navigator;
}

}

// FUNCTION: WIZ8 0x00451ec0
W8Navigator::W8Navigator()
{
    memset(unknown_004, 0, sizeof(unknown_004));
    unknown_004[0] = 0;
    unknown_004[3] = 0xffffffff;
    unknown_004[8] = 0x00010001;
    unknown_004[12] = 0x461c4000;
    unknown_004[13] = 0x469c4000;
    unknown_004[26] = 0xc3fa0000;
    unknown_004[27] = 0xc3fa0000;
    unknown_004[28] = 0xc3fa0000;
    unknown_004[29] = 0x43fa0000;
    unknown_004[30] = 0x43fa0000;
    unknown_004[31] = 0x43fa0000;
    unknown_004[32] = 0x43fa0000;
    unknown_004[33] = 1;
    unknown_004[43] = 0x43fa0000;
    unknown_004[91] = 0x43fa0000;
    unknown_004[92] = 0x43fa0000;
    unknown_004[93] = 0x43fa0000;
    unknown_004[94] = 0x43fa0000;
    unknown_004[95] = 0;
    unknown_004[96] = 0x3f800000;
    node_18c = new srNode(0);
    RegisterNavigator(this);
}

// FUNCTION: WIZ8 0x004534c0
srVector3T<float> W8Navigator::GetPosition()
{
    return fields.position_100;
}

// FUNCTION: WIZ8 0x004538b0
void W8Navigator::SetPathAI(W8PathAI* path_ai)
{
    fields.path_ai_068 = path_ai;
}

// FUNCTION: WIZ8 0x004538c0
W8PathAI* W8Navigator::GetPathAI()
{
    return fields.path_ai_068;
}

void W8Navigator::configureStartupRange(float range)
{
    unknown_004[32] = float_bits(range);
    unknown_004[35] = 1;
    unknown_004[91] = float_bits(range);
    unknown_004[92] = float_bits(range);
}

void W8Navigator::configureStartupDepth(float near_depth, float far_depth)
{
    unknown_004[93] = float_bits(near_depth);
    unknown_004[94] = float_bits(far_depth);
}

extern "C" {
extern void* g_startup_world_659c0c;
extern unsigned char g_navigator_position_changed_659c11;
}

extern void Function454780(int changed);

// FUNCTION: WIZ8 0x00453590
void W8Navigator::SetPositionInternal00453590(const W8Position* position)
{
    srVector3T<double> widened;

    if (position->x != fields.position_100.x ||
        position->y != fields.position_100.y ||
        position->z != fields.position_100.z) {
        fields.position_100.x = position->x;
        fields.position_100.y = position->y;
        fields.position_100.z = position->z;
        widened.x = position->x;
        widened.y = position->y;
        widened.z = position->z;
        node_18c->setLocation(widened);
        if (fields.location_id_0c4 != 0 || this == g_startup_world_659c0c) {
            g_navigator_position_changed_659c11 = 1;
        }
        Function454780(1);
        if (fields.attachment_16c != 0) {
            *fields.attachment_16c->position_4c = fields.position_100;
            fields.attachment_16c->position_34 =
                *fields.attachment_16c->position_4c;
            fields.attachment_16c->position_10 =
                *fields.attachment_16c->position_4c;
        }
    }
}
