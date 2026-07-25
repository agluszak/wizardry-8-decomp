// The original object contributes three functions before the IJG object run.
// Their typed implementations will replace these stubs after the IJG callback
// records are imported.  Keeping them in a separate translation unit already
// reproduces the observed coarse object/link order.

// FUNCTION: SREXT_JPEGIMPORTER 0x10001000
int srJPEG_read_header_adapter(void*)
{
    return 0;
}

// FUNCTION: SREXT_JPEGIMPORTER 0x100010F0
int srJPEG_encode_adapter(void*)
{
    return 0;
}

// FUNCTION: SREXT_JPEGIMPORTER 0x10001290
int srJPEG_decode_adapter(void*)
{
    return 0;
}
