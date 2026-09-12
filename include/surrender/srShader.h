#pragma once

/* SurRender treats a shader as a 32-bit flag word. The DLL exports no shader
   methods and no named bit accessors; Wizardry stores the same unsigned long
   and applies masks (0xfffffff7, 0xfffffffb|3, 0xffff7fff, ~0x400|0x800).
   The modeller/mesh default 0x0100241b is likewise unnamed until an SR body
   spells the bits. */
class srShader {
public:
    unsigned long value;
};

static_assert(sizeof(srShader) == 0x04, "srShader_must_be_0x04");
