#include <stdlib.h>

#include "zlib.h"

// FUNCTION: WIZ8 0x00415820
voidpf wiz8_zalloc(voidpf opaque, uInt items, uInt size)
{
    (void)opaque;
    return malloc(items * size);
}

// FUNCTION: WIZ8 0x00415840
void wiz8_zfree(voidpf opaque, voidpf allocation)
{
    (void)opaque;
    free(allocation);
}

// FUNCTION: WIZ8 0x00415850
z_streamp wiz8_inflate_create(Bytef* input, uInt input_size)
{
    z_streamp stream = (z_streamp)malloc(sizeof(z_stream));
    if (stream == Z_NULL) {
        return Z_NULL;
    }

    stream->zalloc = wiz8_zalloc;
    stream->zfree = wiz8_zfree;
    stream->opaque = Z_NULL;
    if (inflateInit(stream) != Z_OK) {
        free(stream);
        return Z_NULL;
    }

    stream->next_in = input;
    stream->avail_in = input_size;
    return stream;
}

// FUNCTION: WIZ8 0x004158B0
uInt wiz8_inflate_read(z_streamp stream, Bytef* output, uInt output_size)
{
    if (stream->avail_in == 0) {
        return 0;
    }

    stream->next_out = output;
    stream->avail_out = output_size;
    inflate(stream, Z_PARTIAL_FLUSH);
    return output_size - stream->avail_out;
}

// FUNCTION: WIZ8 0x004158F0
void wiz8_inflate_destroy(z_streamp stream)
{
    inflateEnd(stream);
    free(stream);
}
