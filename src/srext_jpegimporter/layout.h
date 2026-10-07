#pragma once

struct JpegCodecState {
    unsigned char* pixels;
    unsigned long width;
    unsigned long height;
    void* output_stdio_cookie; // ignored by the SurRender fwrite bridge
    void* input_stdio_cookie;  // ignored by the SurRender fread bridge
    int arithmetic_coding;
    int ccir601_sampling;
    int smoothing_factor;
    int quality;
    unsigned long unknown_24;
    int failed; // nonzero after the IJG error callback
    int components;
};

struct JpegExportOptions {
    unsigned long limit;
    unsigned char quality;
    unsigned char smoothing_factor;
    unsigned char padding_06[2];
    void* pointer;
};

static_assert((sizeof(JpegCodecState) == 0x30), "JpegCodecState_must_be_0x30");
static_assert((sizeof(JpegExportOptions) == 0x0c), "JpegExportOptions_must_be_0x0c");
