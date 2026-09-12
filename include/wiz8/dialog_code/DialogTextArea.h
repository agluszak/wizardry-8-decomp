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
    W8ControlsRect m_bounds; /* 0x00: passed to entry construction */
    int unknown_010;
    int unknown_014;
    int unknown_018;
    W8GrowableVector<W8DialogTextEntry*> m_all_lines_01c;     /* owns entries */
    W8GrowableVector<W8DialogTextEntry*> m_visible_lines_02c; /* non-owning view */
    unsigned char unknown_03c;
    unsigned char unknown_03d;
    unsigned char unknown_03e;
    unsigned char unknown_03f;
    int unknown_040;
    int unknown_044;
    int unknown_048;
    int unknown_04c;
    int unknown_050;
    unsigned char unknown_054;
    signed char unknown_055;
    unsigned char unknown_056;
    unsigned char unknown_057;
}; /* modeled minimum 0x58 */
