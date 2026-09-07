#pragma once

#include "wiz8/local_code/ControlsRect.h"
#include "wiz8/local_code/TextBuffer.h"

extern unsigned int g_dialog_text_layout_mask_69c5d0;

/* Recovered role name. Constructor 0x005D1050 and the allocation in
   0x005D16C0 establish a 0x50-byte text-buffer base plus 0x14 bytes. */
// VTABLE: WIZ8 0x005ef894
class W8DialogTextEntry : public W8TextBuffer {
public:
    W8DialogTextEntry(const wchar_t* prefix, const wchar_t* text,
                      unsigned int prefix_palette, unsigned int text_palette,
                      const W8ControlsRect* bounds, int font,
                      unsigned char category, unsigned int layout_mode,
                      unsigned char shorten);
    virtual ~W8DialogTextEntry() override;
    void Draw(unsigned char force);
    void SetSelected(unsigned char selected);

private:
    friend class W8DialogTextArea;
    unsigned int m_prefix_palette;       /* 0x50 */
    unsigned int m_text_palette;         /* 0x54 */
    int m_prefix_length;                 /* 0x58: includes ": " */
    unsigned char m_selected;            /* 0x5c */
    unsigned char m_state_5d;            /* 0x5d: palette override, separate from selection */
    unsigned char m_category;            /* 0x5e: text-area filter key */
    unsigned char m_shorten;             /* 0x5f */
    unsigned char m_state_60;            /* 0x60: another palette override */
};
static_assert(sizeof(W8DialogTextEntry) == 0x64, "W8DialogTextEntry_size");
