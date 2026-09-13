#include "wiz8/dialog_code/DialogScrollBar.h"
#include "wiz8/dialog_code/ButtonUserData.h"
#include "wiz8/cursor.h"
#include "wiz8/utility.h"

/* Reconstructed logical owner; original translation-unit identity is unproven.
   Live query: 0x005E0C40 sits in the gap after StatInfoDialogs.cpp (upper
   0x005E0180) and before 3D Code\PList.cpp (lower 0x005E22C0). It is not
   stListBox.cpp. */

/* The owning dialog performs DestroyControls separately. Its member teardown
   (0x005DBCC4, also 0x005D5F00's teardown) calls the shared ret at 0x004023A0
   with the scrollbar receiver; the scrollbar destructor itself is empty. */
W8DialogScrollBar::~W8DialogScrollBar() {}

// FUNCTION: WIZ8 0x005e0e00
void W8DialogScrollBar::DestroyControls()
{
    if (m_up_button != -1) {
        RemoveButton(m_up_button);
        m_up_button = -1;
    }
    if (m_up_image != -1) {
        UnloadButtonImage(m_up_image);
        m_up_image = -1;
    }
    if (m_down_button != -1) {
        RemoveButton(m_down_button);
        m_down_button = -1;
    }
    if (m_down_image != -1) {
        UnloadButtonImage(m_down_image);
        m_down_image = -1;
    }
    if (m_thumb_button != -1) {
        RemoveButton(m_thumb_button);
        m_thumb_button = -1;
    }
    if (m_thumb_image != -1) {
        UnloadButtonImage(m_thumb_image);
        m_thumb_image = -1;
    }
    if (m_track_button != -1) {
        RemoveButton(m_track_button);
        m_track_button = -1;
    }
    if (m_track_image != -1) {
        UnloadButtonImage(m_track_image);
        m_track_image = -1;
    }
    m_on_scroll = 0;
    m_initialized = 0;
}

// FUNCTION: WIZ8 0x005e0ca0
unsigned char W8DialogScrollBar::CreateControls(const Resources* resources)
{
    m_up_image =
        LoadButtonImage(
            reinterpret_cast<UINT8*>( // reinterpret-ok: SGP API declared UINT8* for text
                const_cast<char*>(resources->arrows_path)),
                        3, 0, 1, 2, 2);
    if (m_up_image != -1) {
        m_up_button = QuickCreateButton(m_up_image, 0, 0, BUTTON_NO_TOGGLE, 126, UpButtonCallback,
                                        UpButtonCallback);
    }
    m_down_image =
        LoadButtonImage(
            reinterpret_cast<UINT8*>( // reinterpret-ok: SGP API declared UINT8* for text
                const_cast<char*>(resources->arrows_path)),
                        11, 8, 9, 10, 10);
    if (m_down_image != -1) {
        m_down_button = QuickCreateButton(m_down_image, 0, 0, BUTTON_NO_TOGGLE, 126,
                                          DownButtonCallback, DownButtonCallback);
    }
    m_thumb_image =
        LoadButtonImage(
            reinterpret_cast<UINT8*>( // reinterpret-ok: SGP API declared UINT8* for text
                const_cast<char*>(resources->arrows_path)),
                        7, 4, 5, 6, 6);
    if (m_thumb_image != -1) {
        m_thumb_button = QuickCreateButton(m_thumb_image, 0, 0, BUTTON_NO_TOGGLE, 125, 0, 0);
    }
    m_track_image =
        LoadButtonImage(
            reinterpret_cast<UINT8*>( // reinterpret-ok: SGP API declared UINT8* for text
                const_cast<char*>(resources->track_path)),
                        -1, resources->track_frame, -1, resources->track_frame, -1);
    if (m_track_image != -1) {
        m_track_button = QuickCreateButton(m_track_image, 0, 0, BUTTON_NO_TOGGLE, 125,
                                           TrackButtonCallback, TrackButtonCallback);
    }
    if (m_track_button != -1 && m_thumb_button != -1 && m_down_button != -1 && m_up_button != -1) {
        SetButtonUserDataPointer(m_up_button, this);
        SetButtonUserDataPointer(m_down_button, this);
        SetButtonUserDataPointer(m_thumb_button, this);
        SetButtonUserDataPointer(m_track_button, this);
        m_on_scroll = resources->on_scroll;
        m_initialized = 1;
        m_visible = 0;
        m_dirty = 1;
        return 1;
    }
    DestroyControls();
    return 0;
}

// FUNCTION: WIZ8 0x005e0eb0
void W8DialogScrollBar::SetLayout(int x, int y, int entry_count, int first_visible_entry,
                                  int entry_height, int view_height)
{
    if (m_initialized) {
        int track_width = GetButtonWidth(m_track_button);
        int arrow_width = GetButtonWidth(m_up_button);
        SetButtonPosition(m_track_button, static_cast<short>(x), static_cast<short>(y));
        int arrow_x = x + (track_width - arrow_width) / 2;
        SetButtonPosition(m_up_button, static_cast<short>(arrow_x), static_cast<short>(y));
        SetButtonPosition(m_down_button, static_cast<short>(arrow_x),
                          static_cast<short>(y - GetButtonHeight(m_down_button) +
                                             GetButtonHeight(m_track_button)));
        m_track_bounds[0] = x;
        m_track_bounds[1] = y + GetButtonHeight(m_up_button);
        m_track_bounds[2] = x + GetButtonWidth(m_track_button);
        m_track_bounds[3] = y - GetButtonHeight(m_down_button) + GetButtonHeight(m_track_button);
        m_entry_count = entry_count;
        m_first_visible_entry = first_visible_entry;
        m_entry_height = entry_height;
        m_view_height = view_height;
        UpdateThumb();
        if (m_entry_count <= m_view_height / m_entry_height) {
            GUI_BUTTON* button = GetButtonPtr(m_up_button);
            if (button && (button->uiFlags & BUTTON_ENABLED)) {
                button->uiFlags &= ~BUTTON_ENABLED;
            }
            button = GetButtonPtr(m_down_button);
            if (button && (button->uiFlags & BUTTON_ENABLED)) {
                button->uiFlags &= ~BUTTON_ENABLED;
            }
            HideButton(m_thumb_button);
        }
        m_visible = 1;
        m_dirty = 1;
    }
}

// FUNCTION: WIZ8 0x005e1000
void W8DialogScrollBar::UpdateThumb()
{
    if (m_initialized && m_entry_count != -1 && m_first_visible_entry != -1) {
        int x = m_track_bounds[0] +
                (m_track_bounds[2] - m_track_bounds[0] - GetButtonWidth(m_thumb_button)) / 2;
        int offset = 0;
        if (m_entry_count > m_view_height / m_entry_height) {
            offset = ((m_track_bounds[3] - m_track_bounds[1] - GetButtonHeight(m_thumb_button)) *
                      m_first_visible_entry) /
                     (m_entry_count - m_view_height / m_entry_height);
        }
        SetButtonPosition(m_thumb_button, static_cast<short>(x),
                          static_cast<short>(m_track_bounds[1] + offset));
        m_dirty = 1;
    }
}

// FUNCTION: WIZ8 0x005e10b0
void W8DialogScrollBar::Draw(unsigned char force)
{
    if (m_initialized && m_visible && (force || m_dirty)) {
        DrawButton(m_track_button);
        DrawButton(m_up_button);
        DrawButton(m_down_button);
        DrawButton(m_thumb_button);
        InvalidateRegion(GetButtonX(m_track_button), GetButtonY(m_track_button),
                         GetButtonX(m_track_button) + GetButtonWidth(m_track_button),
                         GetButtonY(m_track_button) + GetButtonHeight(m_track_button), 0);
        m_dirty = 0;
    }
}

// FUNCTION: WIZ8 0x005e1170
void W8DialogScrollBar::ScrollUp()
{
    if (m_first_visible_entry != 0) {
        --m_first_visible_entry;
        if (m_on_scroll) {
            m_on_scroll(this, m_first_visible_entry);
        }
        UpdateThumb();
    }
}

// FUNCTION: WIZ8 0x005e11a0
void W8DialogScrollBar::ScrollDown()
{
    int visible_entries = m_view_height / m_entry_height;
    if (visible_entries < m_entry_count &&
        m_first_visible_entry < m_entry_count - visible_entries) {
        ++m_first_visible_entry;
        if (m_on_scroll) {
            m_on_scroll(this, m_first_visible_entry);
        }
        UpdateThumb();
    }
}

// FUNCTION: WIZ8 0x005e11e0
void W8DialogScrollBar::ScrollToMouse()
{
    if (m_view_height / m_entry_height < m_entry_count && m_entry_count != -1 &&
        m_first_visible_entry != -1) {
        POINT mouse;
        SGPMouseGetPos(&mouse);
        if (mouse.y < m_track_bounds[1])
            mouse.y = m_track_bounds[1];
        if (mouse.y > m_track_bounds[3])
            mouse.y = m_track_bounds[3];
        int top = m_track_bounds[1];
        int range = m_track_bounds[3] - top - GetButtonHeight(m_thumb_button);
        m_first_visible_entry =
            (m_entry_count - m_view_height / m_entry_height) * (mouse.y - top) / range;
        if (m_on_scroll) {
            m_on_scroll(this, m_first_visible_entry);
        }
        UpdateThumb();
    }
}

// FUNCTION: WIZ8 0x005e1280
void W8DialogScrollBar::UpButtonCallback(GUI_BUTTON* button, INT32 reason)
{
    W8DialogScrollBar* bar = GetButtonUserDataPointer<W8DialogScrollBar>(button);
    if (bar) {
        if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
            bar->ScrollUp();
            if (!(button->uiFlags & BUTTON_CLICKED_ON)) {
                button->uiFlags |= BUTTON_CLICKED_ON;
                bar->m_dirty = 1;
            }
        } else if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
            if (button->uiFlags & BUTTON_CLICKED_ON) {
                button->uiFlags &= ~BUTTON_CLICKED_ON;
                bar->m_dirty = 1;
            }
        } else if (reason & MSYS_CALLBACK_REASON_GAIN_MOUSE) {
            button->Area.uiFlags |= MSYS_MOUSE_IN_AREA;
            bar->m_dirty = 1;
        } else if (reason & MSYS_CALLBACK_REASON_LOST_MOUSE) {
            button->Area.uiFlags &= ~MSYS_MOUSE_IN_AREA;
            bar->m_dirty = 1;
        }
    }
}

// FUNCTION: WIZ8 0x005e1320
void W8DialogScrollBar::DownButtonCallback(GUI_BUTTON* button, INT32 reason)
{
    W8DialogScrollBar* bar = GetButtonUserDataPointer<W8DialogScrollBar>(button);
    if (bar) {
        if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
            bar->ScrollDown();
            if (!(button->uiFlags & BUTTON_CLICKED_ON)) {
                button->uiFlags |= BUTTON_CLICKED_ON;
                bar->m_dirty = 1;
            }
        } else if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
            if (button->uiFlags & BUTTON_CLICKED_ON) {
                button->uiFlags &= ~BUTTON_CLICKED_ON;
                bar->m_dirty = 1;
            }
        } else if (reason & MSYS_CALLBACK_REASON_GAIN_MOUSE) {
            button->Area.uiFlags |= MSYS_MOUSE_IN_AREA;
            bar->m_dirty = 1;
        } else if (reason & MSYS_CALLBACK_REASON_LOST_MOUSE) {
            button->Area.uiFlags &= ~MSYS_MOUSE_IN_AREA;
            bar->m_dirty = 1;
        }
    }
}

// FUNCTION: WIZ8 0x005e13d0
void W8DialogScrollBar::TrackButtonCallback(GUI_BUTTON* button, INT32 reason)
{
    W8DialogScrollBar* bar = GetButtonUserDataPointer<W8DialogScrollBar>(button);
    if (bar && (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN)) {
        bar->ScrollToMouse();
    }
}

// FUNCTION: WIZ8 0x005e0c40
W8DialogScrollBar::W8DialogScrollBar()
{
    m_initialized = 0;
    m_visible = 0;
    m_owner = 0;
    m_entry_count = 1;
    m_first_visible_entry = 0;
    m_entry_height = -1;
    m_view_height = -1;
    m_dirty = 0;
    m_track_bounds[0] = 0;
    m_track_bounds[1] = 0;
    m_track_bounds[2] = 0;
    m_track_bounds[3] = 0;
    m_up_image = -1;
    m_up_button = -1;
    m_down_image = -1;
    m_down_button = -1;
    m_thumb_image = -1;
    m_thumb_button = -1;
    m_track_image = -1;
    m_track_button = -1;
    m_on_scroll = 0;
}
