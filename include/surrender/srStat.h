#pragma once

/* Channel histogram summary filled by
   srColorSurfaceIFace::getChannelStatistics (0x10059240). The body
   writes a sample count, mean, standard deviation, median and min/max
   bin. The double at +0x08 sits on the natural alignment hole after
   the leading long; nothing in that function stores a field at +0x04.

   Default 8-byte class alignment would pad the trailing longs to 0x28.
   pack(4) plus the explicit hole keeps the recovered 0x24 size and
   leaves the first double at +0x08. */
#pragma pack(push, 4)
class srStat {
public:
    long count_00;                /* 0x00 */
    unsigned char unknown_04_[4]; /* 0x04: alignment hole, never written */
    double mean_08;               /* 0x08 */
    double deviation_10;          /* 0x10 */
    long median_18;               /* 0x18 */
    long min_1c;                  /* 0x1c */
    long max_20;                  /* 0x20 */
};
#pragma pack(pop)

static_assert(sizeof(srStat) == 0x24, "srStat_must_be_0x24");
