#pragma once

#include "wiz8/local_code/ControlsRect.h"
#include "wiz8/local_code/TextBuffer.h"

extern unsigned int g_dialog_text_layout_mask;

// VTABLE: WIZ8 0x005ef894
class W8DialogTextEntry : public W8TextBuffer {
public:
    W8DialogTextEntry(const wchar_t* prefix, const wchar_t* text, unsigned int prefix_palette,
                      unsigned int text_palette, const W8ControlsRect* bounds, int font,
                      unsigned char category, unsigned int layout_mode, unsigned char shorten);
    void Draw(bool force);
    void SetSelected(bool selected);

private:
    int DrawLine(wchar_t* line, size_t span, int prefix_remaining, int y);
    friend class W8DialogTextArea;
    /* The controller reads m_category when it snapshots the transcript. */
    friend class W8NpcDialogueTextController;
    unsigned int m_prefix_palette;
    unsigned int m_text_palette;
    int m_prefix_length; /* includes ": " */
    bool m_selected;
    bool m_entry_highlighted;     /* highlighted keyword palette */
    unsigned char m_category;     /* text-area filter key */
    unsigned char m_shorten_mask; /* raw shortening bit from text-area behavior flags */
    bool m_marked;                /* marked transcript entry palette */
};
static_assert(sizeof(W8DialogTextEntry) == 0x64, "W8DialogTextEntry_size");
