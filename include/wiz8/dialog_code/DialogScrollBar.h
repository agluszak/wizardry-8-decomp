#pragma once

#include "Button System.h"

class W8DialogBase;

class W8DialogScrollBar {
public:
    /* Four-word record passed by the monster/profession dialogs and the
       static record at 0x0064F908. */
    struct Resources {
        const char* arrows_path;
        const char* track_path;
        int track_frame;
        void (*on_scroll)(W8DialogScrollBar*, int first_visible_entry);
    };
    W8DialogScrollBar(); /* 0x005E0C40 */
    ~W8DialogScrollBar();
    unsigned char CreateControls(const Resources* resources); /* 0x005E0CA0 */
    void DestroyControls();                                   /* 0x005E0E00 */
    void SetLayout(int x, int y, int entry_count, int first_visible_entry, int entry_height,
                   int view_height); /* 0x005E0EB0 */
    void UpdateThumb();              /* 0x005E1000 */
    void Draw(unsigned char force);  /* 0x005E10B0 */
    void ScrollUp();                 /* 0x005E1170 */
    void ScrollDown();               /* 0x005E11A0 */
    void ScrollToMouse();            /* 0x005E11E0 */

private:
    static void UpButtonCallback(GUI_BUTTON* button, INT32 reason);
    static void DownButtonCallback(GUI_BUTTON* button, INT32 reason);
    static void TrackButtonCallback(GUI_BUTTON* button, INT32 reason);
    unsigned char m_initialized; /* 0x00 */
    unsigned char m_visible;     /* 0x01 */
public:
    unsigned char m_dirty; /* 0x02: owning dialogs set this before Draw */
private:
    unsigned char unknown_003;
    int m_entry_count;         /* 0x04 */
    int m_first_visible_entry; /* 0x08 */
    int m_entry_height;        /* 0x0c */
    int m_view_height;         /* 0x10 */
    int m_track_bounds[4];     /* 0x14: left, top, right, bottom */
public:
    /* Owning dialog; MonsterInfoDialog stores this so the scroll callback can
       retarget the text area. */
    W8DialogBase* m_owner; /* 0x24 */
private:
    int m_up_image;                                                              /* 0x28 */
    int m_up_button;                                                             /* 0x2c */
    int m_down_image;                                                            /* 0x30 */
    int m_down_button;                                                           /* 0x34 */
    int m_thumb_image;                                                           /* 0x38 */
    int m_thumb_button;                                                          /* 0x3c */
    int m_track_image;                                                           /* 0x40 */
    int m_track_button;                                                          /* 0x44 */
    void (*m_on_scroll)(W8DialogScrollBar* scroll_bar, int first_visible_entry); /* 0x48 */
}; /* 0x4c */
static_assert(sizeof(W8DialogScrollBar::Resources) == 0x10, "W8DialogScrollBar_Resources_size");
static_assert(sizeof(W8DialogScrollBar) == 0x4c, "W8DialogScrollBar_size");
