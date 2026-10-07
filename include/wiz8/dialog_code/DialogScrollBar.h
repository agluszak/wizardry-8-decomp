#pragma once

#include "Button System.h"
#include "wiz8/local_code/ControlsRect.h"

class W8DialogBase;

class W8DialogScrollBar {
public:
    struct Resources {
        const char* arrows_path;
        const char* track_path;
        int track_frame;
        void (*on_scroll)(W8DialogScrollBar*, int first_visible_entry);
    };
    W8DialogScrollBar();
    ~W8DialogScrollBar();
    bool CreateControls(const Resources* resources);
    void DestroyControls();
    void SetLayout(int x, int y, int entry_count, int first_visible_entry, int entry_height,
                   int view_height);
    void UpdateThumb();
    void Draw(bool force);
    void ScrollUp();
    void ScrollBy(int delta);
    void ScrollDown();
    void ScrollToMouse();

private:
    static void UpButtonCallback(GUI_BUTTON* button, INT32 reason);
    static void DownButtonCallback(GUI_BUTTON* button, INT32 reason);
    static void TrackButtonCallback(GUI_BUTTON* button, INT32 reason);
    bool m_initialized;
    bool m_visible;

public:
    bool m_dirty; /* owning dialogs set this before Draw */
private:
    unsigned char unknown_003;
    int m_entry_count;
    int m_first_visible_entry;
    int m_entry_height;
    int m_view_height;
    W8ControlsRect m_track_bounds; /* left, top, right, bottom */
public:
    /* Owning dialog; MonsterInfoDialog stores this so the scroll callback can
       retarget the text area. */
    W8DialogBase* m_owner;

private:
    int m_up_image;
    int m_up_button;
    int m_down_image;
    int m_down_button;
    int m_thumb_image;
    int m_thumb_button;
    int m_track_image;
    int m_track_button;
    void (*m_on_scroll)(W8DialogScrollBar* scroll_bar, int first_visible_entry);
};
static_assert(sizeof(W8DialogScrollBar::Resources) == 0x10, "W8DialogScrollBar_Resources_size");
static_assert(sizeof(W8DialogScrollBar) == 0x4c, "W8DialogScrollBar_size");
