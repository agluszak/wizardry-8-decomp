#define _CRTIMP
#include <stdio.h>

#include "surrender/srBinIStream.h"
#include "surrender/srBinOStream.h"

#include "codec_adapter.h"

// GLOBAL: SREXT_JPEGIMPORTER 0x1001A22C
srBinIStream* srJPEG_active_input_stream;
// GLOBAL: SREXT_JPEGIMPORTER 0x1001A230
srBinOStream* srJPEG_active_output_stream;

void srJPEG_set_input_stream(srBinIStream* stream)
{
    srJPEG_active_input_stream = stream;
}

void srJPEG_set_output_stream(srBinOStream* stream)
{
    srJPEG_active_output_stream = stream;
}

// These definitions deliberately replace the CRT functions for the IJG object
// files. IJG passes an element size of one; the target returns a byte count and
// ignores its FILE cookie in favor of the active SurRender stream.
// FUNCTION: SREXT_JPEGIMPORTER 0x10014BE0
extern "C" size_t __cdecl fread(void* destination, size_t size, size_t count, FILE*)
{
    if (srJPEG_active_input_stream != 0 && srJPEG_active_input_stream->good()) {
        size_t requested = size * count;
        unsigned long available =
            srJPEG_active_input_stream->getSize() - srJPEG_active_input_stream->tell();
        if (requested > available) {
            requested = available;
        }
        try {
            srJPEG_active_input_stream->read(destination, requested);
        }
        catch (...) {
            throw "srJPEGImporter::importSurface: Input stream is corrupt";
        }
        return requested;
    }
    return 0;
}

// FUNCTION: SREXT_JPEGIMPORTER 0x10014CC0
extern "C" size_t __cdecl fwrite(const void* source, size_t size, size_t count, FILE*)
{
    if (srJPEG_active_output_stream != 0) {
        size_t bytes = size * count;
        try {
            srJPEG_active_output_stream->write(source, bytes);
        }
        catch (...) {
            throw "srJPEGImporter::exportSurface: Output stream is corrupt";
        }
        return bytes;
    }
    return 0;
}
