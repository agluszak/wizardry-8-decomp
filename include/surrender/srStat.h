#pragma once

/* Channel histogram summary filled by srColorSurfaceIFace::getChannelStatistics. */
#pragma pack(push, 4)
class srStat {
public:
    long count;                   /* 0x00 */
    unsigned char unknown_04_[4]; /* 0x04: alignment hole, never written */
    double mean;                  /* 0x08 */
    double deviation;             /* 0x10 */
    long median;                  /* 0x18 */
    long min;                     /* 0x1c */
    long max;                     /* 0x20 */
};
#pragma pack(pop)

static_assert(sizeof(srStat) == 0x24, "srStat_must_be_0x24");
