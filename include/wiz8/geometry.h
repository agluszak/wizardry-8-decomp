#ifndef WIZ8_GEOMETRY_H
#define WIZ8_GEOMETRY_H

#pragma pack(push, 1)
/* A world position as the packed records carry it: three floats, C-compatible,
   distinct from srVector3T<float> which only exists for C++ consumers. */
typedef struct W8Position {
    float x;
    float y;
    float z;
} W8Position;
#pragma pack(pop)

#endif

