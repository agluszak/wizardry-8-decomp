#pragma once

#include "wiz8/local_code/ControlsRect.h"
#include "wiz8/vector.h"
#include "wiz8/dialog_code/DialogTextEntry.h"

extern const unsigned int g_W8DialogTextAreaNoWrap;
extern const unsigned int g_W8DialogTextAreaPointSelection;
extern const unsigned int g_W8DialogTextAreaShortenText;

/* Two instances of this pointer-vector specialization are embedded in
   W8DialogTextArea. */
// VTABLE: WIZ8 0x005ef89c
// class W8GrowableVector<W8DialogTextEntry*>

// VTABLE: WIZ8 0x005ef898
// class W8Vector<W8DialogTextEntry*>

/* Nonpolymorphic scrolling-text helper contained by dialogs, not a widget or
   text-buffer base. Both vectors belong to this object; only all_lines owns
   the entries. visible_lines references entries selected/ordered from it. */
class W8DialogTextArea {
    friend class W8NpcDialogueScrollWidget;
    friend class W8NpcDialogueTextController;

public:
    W8DialogTextArea();
    ~W8DialogTextArea();
    void Configure(const W8ControlsRect* bounds, int font, unsigned int flags);
    void Draw(bool force);
    void SetFirstVisibleLine(int line);
    int GetTotalLineCount();
    unsigned int GetLineHeight();
    void SetEntrySpacing(int lines);
    void SetLineHeight(unsigned int height);
    bool SelectEntry(int index);
    bool ClearSelection();
    bool CopyEntryText(unsigned int index, wchar_t* output);
    unsigned int HitTestEntry(int x, int y);
    bool UpdateSelectionFromPoint(int x, int y);
    bool ClearPointSelection();
    bool CopyVisibleEntryText(unsigned int index, wchar_t* output);
    bool HighlightVisibleEntry(int index);
    bool ClearEntryHighlight();
    W8DialogTextEntry* GetEntry(unsigned int index);
    int GetOwningEntryIndex(int visible_index);
    void SetEntryMarked(int index, bool state);
    int AddEntry(const wchar_t* prefix, const wchar_t* text, unsigned int prefix_palette,
                 unsigned int text_palette, unsigned char category);
    void RemoveEntry(unsigned int index);
    void RebuildVisibleEntries();
    void SetCategoryFilter(signed char category);
    void SetSorted(bool sorted);
    bool ScrollDown(bool check_only);
    bool ScrollUp(bool check_only);
    void SetFirstVisibleEntry(unsigned int index);

private:
    unsigned int FindVisibleEntryAtLine(unsigned int target, unsigned int spacing);

    W8ControlsRect m_bounds; /* passed to entry construction */
    int m_font;
    int m_first_visible_entry;
    int m_first_visible_line;
    W8Vector<W8DialogTextEntry*> m_all_lines;     /* owns entries */
    W8Vector<W8DialogTextEntry*> m_visible_lines; /* non-owning view */
    bool m_layout_initialized;

public:
    /* Owning dialogs raise this before Draw, the same way they dirty the
       contained button and scrollbar. */
    bool m_dirty;

private:
    bool m_selection_dirty; /* repaint after a selection change */
    unsigned char unknown_03f;
    int m_entry_spacing;
    int m_behavior_flags;
    int m_line_height_override;
    int m_selected_visible_entry;
    int m_highlighted_entry; /* visible entry index, -1 when clear */
    bool m_relayout_needed;
    signed char m_category_filter;
    bool m_sorted;
    unsigned char unknown_057;
};
W8_ABI_ASSERT(sizeof(W8DialogTextArea) == 0x58, "W8DialogTextArea_size");
