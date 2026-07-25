#define _CRTIMP
#include <stdio.h>

#include "surrender/srBinIStream.h"
#include "surrender/srBinOStream.h"

#include "codec_adapter.h"

srBinIStream* srJPEG_active_input_stream;
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
extern "C" size_t __cdecl fread(void* destination, size_t size, size_t count, FILE*)
{
    if (srJPEG_active_input_stream == 0 || !srJPEG_active_input_stream->good()) {
        return 0;
    }

    size_t requested = size * count;
    unsigned long available =
        srJPEG_active_input_stream->getSize() - srJPEG_active_input_stream->tell();
    if (available < requested) {
        requested = available;
    }
    srJPEG_active_input_stream->read(destination, requested);
    return requested;
}

extern "C" size_t __cdecl fwrite(const void* source, size_t size, size_t count, FILE*)
{
    if (srJPEG_active_output_stream == 0) {
        return 0;
    }

    size_t bytes = size * count;
    srJPEG_active_output_stream->write(source, bytes);
    return bytes;
}
