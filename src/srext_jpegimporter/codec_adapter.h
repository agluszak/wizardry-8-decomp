#pragma once

#include "layout.h"

class srBinIStream;
class srBinOStream;

extern srBinIStream* srJPEG_active_input_stream;
extern srBinOStream* srJPEG_active_output_stream;

void srJPEG_set_input_stream(srBinIStream* stream);
void srJPEG_set_output_stream(srBinOStream* stream);

void srJPEG_read_header_adapter(JpegCodecState32* state);
void srJPEG_encode_adapter(JpegCodecState32* state);
void srJPEG_decode_adapter(JpegCodecState32* state);
