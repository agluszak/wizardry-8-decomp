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

W8GrCycleBase00451EC0** g_cycle_bases_659b3c;
int g_cycle_base_count_659b34;
int g_cycle_base_capacity_659b38;

void register_cycle_base(W8GrCycleBase00451EC0* value)
{
    W8GrCycleBase00451EC0** replacement;
    int index;
    int new_count = g_cycle_base_count_659b34 + 1;

    if (g_cycle_base_capacity_659b38 < new_count) {
        replacement = static_cast<W8GrCycleBase00451EC0**>(
            ::operator new(new_count * sizeof(*replacement)));
        if (!replacement) {
            return;
        }
        for (index = 0; index != g_cycle_base_count_659b34; ++index) {
            replacement[index] = g_cycle_bases_659b3c[index];
        }
        ::operator delete(g_cycle_bases_659b3c);
        g_cycle_bases_659b3c = replacement;
        g_cycle_base_capacity_659b38 = new_count;
    }
    g_cycle_bases_659b3c[g_cycle_base_count_659b34++] = value;
}

}

// FUNCTION: WIZ8 0x004b6900
W8GrCycleBase004B6900::W8GrCycleBase004B6900()
{
    unknown_004 = 0;
    unknown_008 = -1;
    unknown_00c = 0;
    unknown_010 = 0;
}

/* The complete constructor initializes the observed 99-dword payload, then
   registers the instance and owns an ordinary srNode at +0x18c.  Its two
   internal helper constructors only establish these same scalar defaults; the
   source-visible identity of those helpers is not known. */
// FUNCTION: WIZ8 0x00451ec0
W8GrCycleBase00451EC0::W8GrCycleBase00451EC0()
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
    register_cycle_base(this);
}

void W8GrCycleBase00451EC0::configureStartupRange(float range)
{
    unknown_004[32] = float_bits(range);
    unknown_004[35] = 1;
    unknown_004[91] = float_bits(range);
    unknown_004[92] = float_bits(range);
}

void W8GrCycleBase00451EC0::configureStartupDepth(float near_depth, float far_depth)
{
    unknown_004[93] = float_bits(near_depth);
    unknown_004[94] = float_bits(far_depth);
}
