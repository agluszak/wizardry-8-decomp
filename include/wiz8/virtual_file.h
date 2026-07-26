#pragma once

extern "C" {

unsigned char ReadVirtualFile(
    int handle,
    void* buffer,
    unsigned int size,
    unsigned int* done);
unsigned char WriteVirtualFile(
    int handle,
    const void* buffer,
    unsigned int size,
    unsigned int* done);

}
