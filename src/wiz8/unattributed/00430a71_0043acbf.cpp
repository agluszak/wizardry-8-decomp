#include "wiz8/virtual_file.h"
#include "surrender/srMath.h"

// FUNCTION: WIZ8 0x004374C0
unsigned char ReadVector4Array004374C0(
    int file, srVector4T<float>* values, int count)
{
    return ReadVirtualFile(file, values,
        count * sizeof(srVector4T<float>), 0) & 1;
}

// FUNCTION: WIZ8 0x004374E0
unsigned char ReadVector3Array004374E0(
    int file, srVector3T<float>* values, int count)
{
    return ReadVirtualFile(file, values,
        count * sizeof(srVector3T<float>), 0) & 1;
}

// FUNCTION: WIZ8 0x00437510
unsigned char ReadVector2Array00437510(
    int file, srVector2T<float>* values, int count)
{
    return ReadVirtualFile(file, values,
        count * sizeof(srVector2T<float>), 0) & 1;
}
