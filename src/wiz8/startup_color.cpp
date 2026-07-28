#include <string.h>

extern "C" {

unsigned int g_color_transform_coefficient_6000ac;
unsigned short g_color_transform_table_6c0da0[0x10000];
unsigned int g_color_transform_sentinels_6e0da0[0x80];

/* The menu bring-up needs the table's ownership and complete range before any
   2D draw can address it.  The coefficient-dependent retail transform is the
   remaining wiz8-1sc scope; identity preserves every renderer-native 16-bit
   colour until that transform is reviewed. */
void RebuildStartupColorTable(void)
{
    unsigned int color;
    for (color = 0; color != 0x10000; ++color) {
        g_color_transform_table_6c0da0[color] =
            static_cast<unsigned short>(color);
    }
    memset(g_color_transform_sentinels_6e0da0, 0xff,
           sizeof(g_color_transform_sentinels_6e0da0));
}

void SetStartupColorTransform(unsigned int coefficient)
{
    g_color_transform_coefficient_6000ac = coefficient;
    RebuildStartupColorTable();
}

}
