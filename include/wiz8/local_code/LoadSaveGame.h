#pragma once

/* The SHOT record. SaveGame writes 0x2588 bytes; the save-screen producer
   copy-constructs one complete record, including its trailing padding.
   The 80 by 60 16-bit surface begins at offset 6. */
struct W8SaveScreenshot {
    float version;
    unsigned char capture_result;
    unsigned char unknown_05;
    unsigned short pixels[60][80];
};

static_assert(sizeof(W8SaveScreenshot) == 0x2588,
              "W8SaveScreenshot_must_be_0x2588");

unsigned char SaveGame(const char* name, W8SaveScreenshot* screenshot);

unsigned char AutoSaveIfAllowed(char forced);
