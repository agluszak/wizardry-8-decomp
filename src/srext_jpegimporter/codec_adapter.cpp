#include <setjmp.h>
#include <string.h>

#include <stdio.h>

extern "C" {
#include "jpeglib.h"
}

#include "codec_adapter.h"

namespace {

struct JpegErrorManager {
    jpeg_error_mgr manager;
    jmp_buf jump_buffer;
};

void jpegErrorExit(j_common_ptr common);

} // namespace

// FUNCTION: SREXT_JPEGIMPORTER 0x10001000
void srJPEG_read_header_adapter(JpegCodecState32* state)
{
    jpeg_decompress_struct decompress;
    JpegErrorManager error;

    decompress.err = jpeg_std_error(&error.manager);
    error.manager.error_exit = jpegErrorExit;
    if (setjmp(error.jump_buffer) != 0) {
        jpeg_destroy_decompress(&decompress);
        state->failed = 1;
        return;
    }

    jpeg_create_decompress(&decompress);
    jpeg_stdio_src(&decompress, reinterpret_cast<FILE*>(state->input_stdio_cookie));
    jpeg_read_header(&decompress, TRUE);
    jpeg_start_decompress(&decompress);
    state->width = decompress.output_width;
    state->height = decompress.output_height;
    state->components = decompress.num_components;
    jpeg_destroy_decompress(&decompress);
    state->failed = 0;
}

namespace {

// FUNCTION: SREXT_JPEGIMPORTER 0x100010D0
void jpegErrorExit(j_common_ptr common)
{
    JpegErrorManager* error = reinterpret_cast<JpegErrorManager*>(common->err);
    longjmp(error->jump_buffer, 1);
}

} // namespace

// FUNCTION: SREXT_JPEGIMPORTER 0x100010F0
void srJPEG_encode_adapter(JpegCodecState32* state)
{
    jpeg_compress_struct compress;
    jpeg_error_mgr error;
    jmp_buf jump_buffer;

    if (setjmp(jump_buffer) != 0) {
        state->failed = 1;
        jpeg_destroy_compress(&compress);
        return;
    }

    memset(&compress, 0, sizeof(compress));
    memset(&error, 0, sizeof(error));
    compress.err = jpeg_std_error(&error);
    error.error_exit = jpegErrorExit;
    jpeg_create_compress(&compress);
    jpeg_stdio_dest(&compress, reinterpret_cast<FILE*>(state->output_stdio_cookie));
    compress.in_color_space = JCS_RGB;
    compress.image_width = state->width;
    compress.image_height = state->height;
    compress.input_components = 3;
    jpeg_set_defaults(&compress);
    compress.arith_code = state->arithmetic_coding;
    compress.CCIR601_sampling = state->ccir601_sampling;
    compress.data_precision = 8;
    compress.optimize_coding = TRUE;
    compress.smoothing_factor = state->smoothing_factor;
    jpeg_set_quality(&compress, state->quality, TRUE);
    jpeg_default_colorspace(&compress);
    jpeg_start_compress(&compress, TRUE);

    const unsigned long row_stride = state->width * 3;
    while (compress.next_scanline < compress.image_height) {
        JSAMPROW row = state->pixels + compress.next_scanline * row_stride;
        jpeg_write_scanlines(&compress, &row, 1);
    }
    jpeg_finish_compress(&compress);
    jpeg_destroy_compress(&compress);
    state->failed = 0;
}

// FUNCTION: SREXT_JPEGIMPORTER 0x10001290
void srJPEG_decode_adapter(JpegCodecState32* state)
{
    jpeg_decompress_struct decompress;
    JpegErrorManager error;

    decompress.err = jpeg_std_error(&error.manager);
    error.manager.error_exit = jpegErrorExit;
    if (setjmp(error.jump_buffer) != 0) {
        state->failed = 1;
        jpeg_destroy_decompress(&decompress);
        return;
    }

    jpeg_create_decompress(&decompress);
    jpeg_stdio_src(&decompress, reinterpret_cast<FILE*>(state->input_stdio_cookie));
    jpeg_read_header(&decompress, TRUE);
    jpeg_start_decompress(&decompress);
    const unsigned long row_stride = decompress.output_width * decompress.output_components;
    state->components = decompress.num_components;
    JSAMPARRAY scanline = (*decompress.mem->alloc_sarray)(
        reinterpret_cast<j_common_ptr>(&decompress), JPOOL_IMAGE, row_stride, 1);
    unsigned char* output = state->pixels;
    while (decompress.output_scanline < decompress.output_height) {
        jpeg_read_scanlines(&decompress, scanline, 1);
        memcpy(output, scanline[0], row_stride);
        output += row_stride;
    }
    jpeg_finish_decompress(&decompress);
    jpeg_destroy_decompress(&decompress);
    state->failed = 0;
}
