#pragma once

#include "surrender/srMath.h"

class Trigger;

/* Engine Code\GDFileIO.cpp. The world keeps this loader object opaque; the
   Trigger loader only needs its assertion-backed AddTriggerPlane boundary. */
class GDFileIO {
public:
    void AddTriggerPlane(
        const srVector3T<float>* vertices, Trigger* trigger);

    unsigned char unknown_000[4];
    int trigger_array_mode_004;
    unsigned char unknown_008[0x18];
    int vertex_index_base_020;
    unsigned char unknown_024[4];
    int surface_index_base_028;
    unsigned char unknown_02c[0x10];
    int trigger_surface_count_03c;
    int trigger_vertex_count_040;
    int trigger_count_044;
    void* trigger_surfaces_048;
    srVector3T<float>* trigger_vertices_04c;
    Trigger** triggers_050;
};
