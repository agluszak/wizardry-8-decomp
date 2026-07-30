#pragma once

/* Dialog Code\DialogInterface.cpp proves slots three and nine through its
   forwarding API. The remaining slots and storage stay positional. */
class W8DialogInterface {
public:
    virtual void Method0();
    virtual void Method1();
    virtual void Method2();
    virtual void Method3();
    virtual void Method4();
    virtual void Method5();
    virtual void Method6();
    virtual void Method7();
    virtual void Method8();
    virtual void Method9();

    unsigned char unknown_04[0x40];
    int value_44;                         /* 0x44 */
};

static_assert(sizeof(W8DialogInterface) == 0x48,
              "W8DialogInterface_must_be_0x48");

extern "C" {
void Function5CF250(
    int font,
    unsigned char enabled,
    unsigned char foreground,
    unsigned char background);
void Function5CF520(W8DialogInterface* dialog);
void Function5CF550(W8DialogInterface* dialog);
void Function5CF580(W8DialogInterface* dialog, int value);
}
