#ifndef WIZ8_FILE_SYSTEM_VIRTUAL_FILE_H
#define WIZ8_FILE_SYSTEM_VIRTUAL_FILE_H

#include <stdint.h>

typedef int32_t W8VirtualFileHandle;

typedef enum W8VirtualFileSeekOrigin {
    W8_SEEK_BEGIN = 1,
    W8_SEEK_END = 2,
    W8_SEEK_CURRENT = 4,
} W8VirtualFileSeekOrigin;

#endif
