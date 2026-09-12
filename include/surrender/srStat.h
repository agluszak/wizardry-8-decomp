#pragma once

/* Channel histogram summary filled by
   srColorSurfaceIFace::getChannelStatistics (0x10059240). The body
   writes a sample count, mean, standard deviation, median and min/max
   bin. The double at +0x08 sits on the natural alignment hole after
   the leading long; nothing in that function stores a field at +0x04. */
class srStat {
public:
    long count_00;       /* 0x00 */
    double mean_08;      /* 0x08 */
    double deviation_10; /* 0x10 */
    long median_18;      /* 0x18 */
    long min_1c;         /* 0x1c */
    long max_20;         /* 0x20 */
};

static_assert(sizeof(srStat) == 0x24, "srStat_must_be_0x24");
