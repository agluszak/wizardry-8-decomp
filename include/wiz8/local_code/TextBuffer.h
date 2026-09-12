#pragma once

#include "wiz8/local_code/ControlsRect.h"
#include <wchar.h>

/* Shared declaration owner; implementation remains in Local Code\Controls.cpp. */

extern const unsigned int g_W8TextBufferLayoutMask005ED548;
extern const unsigned int g_W8TextBufferLayoutMask005ED54C;
extern const unsigned int g_W8TextBufferLayoutMask005ED550;
extern const unsigned int g_W8TextBufferLayoutMask005ED554;
extern const unsigned int g_W8TextBufferLayoutMask005ED558;
extern const unsigned int g_W8TextBufferLayoutMask005ED55C;
extern const unsigned int g_W8TextBufferLayoutMask005ED560;
extern const wchar_t g_W8LineBreakCharacters00617C90[];

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
    void RenderText(int a, int b, int x_offset, int y_offset, unsigned char force);
    void RenderToTarget(int offset, unsigned char force, int target);
    void UpdateLayout(); /* 0x004F35B0 */
    void SetLayoutMode(unsigned int layout_mode);
    void SetText(const wchar_t* text, int font);
    void SetLayoutBounds(const W8ControlsRect* bounds, unsigned char copy_pending,
                         unsigned char update_layout);

    __forceinline void SetLayoutBounds(int left, int top, int right, int bottom)
    {
        m_layoutBounds.left = left;
        m_layoutBounds.top = top;
        m_layoutBounds.right = right;
        m_layoutBounds.bottom = bottom;
        m_pendingBounds = m_layoutBounds;
    }

    __forceinline int HasBuffer() const
    {
        return m_buffer != 0;
    }
    __forceinline void SetGeometryDirty()
    {
        m_geometryDirty = 1;
    }
    __forceinline void SetRenderMode(int mode)
    {
        m_renderMode = mode;
    }
    __forceinline void SetFontStateIndex(int index)
    {
        m_fontStateIndex = index;
    }
    __forceinline void MarkGeometryDirty(int mode)
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
    /* State-5's controller raises the alternate-renderer byte directly when
       it changes modes; retain that observed public storage access. */
    unsigned char m_geometryDirty;
    unsigned char m_alternateRenderer;
    unsigned char pad_42[2];
    int m_renderMode;     /* 0x44: 4 initially */
    int m_fontStateIndex; /* 0x48: -1 skips the state-table override */
    unsigned char m_flag_4c;
};
