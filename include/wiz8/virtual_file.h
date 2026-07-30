#pragma once

extern "C" {

void CloseVirtualFile(int handle);

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

unsigned char ReadTextLine004CEE40(
    int handle, char* destination, int capacity, unsigned char* more);
