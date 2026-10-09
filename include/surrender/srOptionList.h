#pragma once

// The first two words are opaque; the JPEG exporter reads only the option string.
struct srImportOptions {
    w8_ulong unknown_00;
    w8_ulong unknown_04;
    const char* option_string;
};

struct srExportOptions {
    w8_ulong unknown_00;
    w8_ulong unknown_04;
    const char* option_string;
};

W8_ABI_ASSERT((sizeof(srImportOptions) == 0x0c), "srImportOptions_must_be_0x0c");
W8_ABI_ASSERT((sizeof(srExportOptions) == 0x0c), "srExportOptions_must_be_0x0c");
