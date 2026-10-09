#pragma once

#include "wiz8/compat/compiler.h"

struct JpegCodecState {
    unsigned char* pixels;
    w8_ulong width;
    w8_ulong height;
    void* output_stdio_cookie; // ignored by the SurRender fwrite bridge
    void* input_stdio_cookie;  // ignored by the SurRender fread bridge
    int arithmetic_coding;
    int ccir601_sampling;
    int smoothing_factor;
    int quality;
    w8_ulong unknown_24;
    int failed; // nonzero after the IJG error callback
    int components;
};

struct JpegExportOptions {
    w8_ulong limit;
    unsigned char quality;
    unsigned char smoothing_factor;
    unsigned char padding_06[2];
    void* pointer;
};

W8_ABI_ASSERT((sizeof(JpegCodecState) == 0x30), "JpegCodecState_must_be_0x30");
W8_ABI_ASSERT((sizeof(JpegExportOptions) == 0x0c), "JpegExportOptions_must_be_0x0c");
