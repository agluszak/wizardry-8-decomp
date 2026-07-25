#pragma once

// These are 32-bit target layouts. Names are intentionally limited to fields whose use is
// visible in srEXT_JPEGImporter.dll; the remaining storage stays explicit until its callers are
// typed.

struct JpegCodecState32 {
    unsigned char* pixels;        // +0x00
    unsigned long width;          // +0x04: IJG output/image width
    unsigned long height;         // +0x08: IJG output/image height
    void* output_stdio_cookie;    // +0x0c: ignored by the SurRender fwrite bridge
    void* input_stdio_cookie;     // +0x10: ignored by the SurRender fread bridge
    int arithmetic_coding;        // +0x14 -> jpeg_compress_struct::arith_code
    int ccir601_sampling;         // +0x18 -> jpeg_compress_struct::CCIR601_sampling
    int smoothing_factor;         // +0x1c -> jpeg_compress_struct::smoothing_factor
    int quality;                  // +0x20 -> jpeg_set_quality
    unsigned long unknown_24;     // +0x24: not read by any of the three codec adapters
    int failed;                   // +0x28: nonzero after the IJG error callback
    int components;               // +0x2c: IJG output_components
};

struct JpegExportOptions32 {
    unsigned long limit_200;      // +0x00: initialized to 200
    unsigned char quality;        // +0x04: 75 at construction, 100 per export
    unsigned char smoothing_factor; // +0x05 -> jpeg_compress_struct::smoothing_factor
    unsigned char padding_06[2];
    void* pointer_08;             // +0x08
};

struct srJPEGImporterLayout32 {
    void* importer_vftable;       // +0x00 -> 0x10016d7c
    void* exporter_vftable;       // +0x04 -> 0x10016d70
    JpegCodecState32 codec;       // +0x08
    JpegExportOptions32 options;  // +0x38
};

struct srJPEGPluginLayout32 {
    void* plugin_vftable;         // +0x00 -> 0x10016d68
    srJPEGImporterLayout32 jpeg;  // +0x04
};

typedef char JpegCodecState32_must_be_0x30[(sizeof(JpegCodecState32) == 0x30) ? 1 : -1];
typedef char JpegExportOptions32_must_be_0x0c[(sizeof(JpegExportOptions32) == 0x0c) ? 1 : -1];
typedef char srJPEGImporterLayout32_must_be_0x44[(sizeof(srJPEGImporterLayout32) == 0x44) ? 1 : -1];
typedef char srJPEGPluginLayout32_must_be_0x48[(sizeof(srJPEGPluginLayout32) == 0x48) ? 1 : -1];
