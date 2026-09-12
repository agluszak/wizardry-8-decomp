#pragma once

#include "wiz8/local_code/ControlsRect.h"
#include "wiz8/vector.h"
#include "wiz8/dialog_code/DialogTextEntry.h"

/* Two instances of this pointer-vector specialization are embedded in
   W8DialogTextArea. */
// VTABLE: WIZ8 0x005ef898
// class W8GrowableVector<W8DialogTextEntry*>

/* Nonpolymorphic scrolling-text helper contained by dialogs, not a widget or
   text-buffer base. Both vectors belong to this object; only all_lines owns
   the entries. visible_lines references entries selected/ordered from it. */
class W8DialogTextArea {
public:
    W8DialogTextArea();  /* 0x005D14D0 */
    ~W8DialogTextArea(); /* 0x005D1590 */
    void Configure(const W8ControlsRect* bounds, int font, unsigned int flags);
    void Draw(unsigned char force);
    void SetFirstVisibleLine(int line);
    int GetTotalLineCount();
    unsigned int GetLineHeight();
    void SetEntrySpacing(int lines);
    void SetLineHeight(unsigned int height);
    unsigned char SelectEntry(int index);
    unsigned char ClearSelection();
    unsigned char CopyEntryText(unsigned int index, wchar_t* output);
    unsigned int HitTestEntry(int x, int y);
    unsigned char UpdateSelectionFromPoint(int x, int y);
    unsigned char ClearPointSelection();
    unsigned char CopyVisibleEntryText(unsigned int index, wchar_t* output);
    unsigned char SetEntryState5D(int index);
    unsigned char ClearEntryState5D();
    W8DialogTextEntry* GetEntry(unsigned int index);
    int GetOwningEntryIndex(int visible_index);
    void SetEntryState60(int index, unsigned char state);
    int AddEntry(const wchar_t* prefix, const wchar_t* text, unsigned int prefix_palette,
                 unsigned int text_palette, unsigned char category);
    void RemoveEntry(unsigned int index);
    void RebuildVisibleEntries();
    void SetCategoryFilter(signed char category);
    void SetSorted(unsigned char sorted);
    unsigned char ScrollDown(unsigned char check_only);
    unsigned char ScrollUp(unsigned char check_only);
    void SetFirstVisibleEntry(unsigned int index);

private:
    W8ControlsRect m_bounds;                              /* 0x00: passed to entry construction */
    int m_font;                                           /* 0x10 */
    int m_first_visible_entry;                            /* 0x14 */
    int m_first_visible_line;                             /* 0x18 */
    W8GrowableVector<W8DialogTextEntry*> m_all_lines_01c; /* owns entries */
    W8GrowableVector<W8DialogTextEntry*> m_visible_lines_02c; /* non-owning view */
    unsigned char m_layout_initialized;                       /* 0x3c */
    unsigned char unknown_03d;
    unsigned char unknown_03e;
    unsigned char unknown_03f;
    int m_entry_spacing;             /* 0x40 */
    int m_behavior_flags;            /* 0x44 */
    int m_line_height_override;      /* 0x48 */
    int m_selected_visible_entry;    /* 0x4c */
    int m_state_5d_entry;            /* 0x50 */
    unsigned char m_relayout_needed; /* 0x54 */
    signed char m_category_filter;   /* 0x55 */
    unsigned char m_sorted;          /* 0x56 */
    unsigned char unknown_057;
}; /* modeled minimum 0x58 */
