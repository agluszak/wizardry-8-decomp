#pragma once

// The JPEG exporter reads the option string at +0x08.  The first two words
// remain intentionally opaque until another extension gives them semantics.
struct srImportOptions {
    unsigned long unknown_00;
    unsigned long unknown_04;
    const char* option_string;
};

struct srExportOptions {
    unsigned long unknown_00;
    unsigned long unknown_04;
    const char* option_string;
};

static_assert((sizeof(srImportOptions) == 0x0c), "srImportOptions_must_be_0x0c");
static_assert((sizeof(srExportOptions) == 0x0c), "srExportOptions_must_be_0x0c");
