#pragma once

#include "wiz8/local_code/ControlsRect.h"
#include <wchar.h>

/* Shared declaration owner; implementation remains in Local Code\Controls.cpp. */

extern const unsigned int g_W8TextBufferAlignLeft;
extern const unsigned int g_W8TextBufferAlignCenter;
extern const unsigned int g_W8TextBufferAlignRight;
extern const unsigned int g_W8TextBufferAlignMiddle;
extern const unsigned int g_W8TextBufferAlignTop;
extern const unsigned int g_W8TextBufferAlignBottom;
extern const unsigned int g_W8TextBufferNoWrap;
/* The 0x55DE40 translation unit's own copies of the 1/2/4 layout masks; the
   Controls.cpp definitions above carry the same values but retail bound the
   NPC dialogue text controller to its own emission. */
extern const unsigned int g_W8DialogTextAreaAlignLeft;
extern const unsigned int g_W8DialogTextAreaAlignCenter;
extern const unsigned int g_W8DialogTextAreaAlignRight;
extern const wchar_t g_W8LineBreakCharacters[];

// VTABLE: WIZ8 0x005ed5b8
class W8TextBuffer {
public:
    friend class W8DialogTextArea;
    W8TextBuffer();
    W8TextBuffer(const W8ControlsRect* bounds, const wchar_t* text, int font,
                 unsigned int layout_mode, int render_mode);
    void CopyTextTo(wchar_t* destination);
    unsigned int GetLineHeight();
    int GetHorizontalPosition(int width);
    int GetVerticalPosition();
    void SetLineHeight(unsigned int height);
    void FillBounds(int colour);
    void RenderText(unsigned char* buffer, unsigned int pitch, int x_offset, int y_offset,
                    unsigned char force);
    void RenderToTarget(int offset, unsigned char force, int target);
    void UpdateLayout(); /* 0x004F35B0 */
    void SetLayoutMode(unsigned int layout_mode);
    void SetText(const wchar_t* text, int font);
    void SetLayoutBounds(const W8ControlsRect* bounds, unsigned char copy_pending,
                         unsigned char update_layout);

    void SetLayoutBounds(int left, int top, int right, int bottom)
    {
        m_layoutBounds.left = left;
        m_layoutBounds.top = top;
        m_layoutBounds.right = right;
        m_layoutBounds.bottom = bottom;
        m_pendingBounds = m_layoutBounds;
    }

    int HasBuffer() const
    {
        return m_buffer != 0;
    }
    void SetGeometryDirty()
    {
        m_geometryDirty = 1;
    }
    void SetRenderMode(int mode)
    {
        m_renderMode = mode;
    }
    void SetFontStateIndex(int index)
    {
        m_fontStateIndex = index;
    }
    void MarkGeometryDirty(int mode)
    {
        m_geometryDirty = 1;
        m_layoutMode = mode;
    }

    virtual ~W8TextBuffer();

protected:
    W8ControlsRect m_layoutBounds;  /* 0x04: current absolute layout bounds */
    W8ControlsRect m_pendingBounds; /* 0x14: mirrored pending bounds */
    int m_field_24;                 /* 0x24: the constructor steps over this one */
    int m_font;                     /* 0x28: font used for uncached line height */
public:
    /* The tooltip builder in Video2.cpp reads the finished layout directly. */
    int m_lineCount; /* 0x2c */
protected:
    unsigned int m_lineHeight; /* 0x30: cached height, zero means query font */
    wchar_t* m_buffer;         /* 0x34: freed on teardown */
    int m_layoutMode;          /* 0x38: 10 initially */
public:
    unsigned int m_maxLineWidth; /* 0x3c */
public:
    /* The party-selection controller raises the geometry-dirty byte directly
       when it changes modes; retain that observed public storage access. */
    bool m_geometryDirty;              /* 0x40 */
    unsigned char m_alternateRenderer; /* 0x41 */
    unsigned char pad_42[2];
    int m_renderMode;     /* 0x44: 4 initially */
    int m_fontStateIndex; /* 0x48: -1 skips the state-table override */
    bool m_flag_4c;
};
