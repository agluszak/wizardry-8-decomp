#pragma once

#include "wiz8/local_code/TextControl.h"
#include "wiz8/vector.h"

/* Shared declaration owner; implementation remains in Local Code\Controls.cpp. */

class W8ControlSelection;

class W8ControlSelectionListener {
public:
    virtual void OnSelectionChanged(W8ControlSelection* control, int selected) = 0;
};

// VTABLE: WIZ8 0x005ed65c
// class W8Vector<W8TextControl*>

/* A radio group of text controls: selecting one deselects the rest. */
// VTABLE: WIZ8 0x005ed654
class W8ControlSelection : public W8TextControl::Listener {
public:
    W8ControlSelection();
    int AddEntry(W8TextControl* entry);
    void SetSelected(int iSelected);
    virtual void OnPrimary(W8TextControl* entry) override;
    virtual void OnSecondary(W8TextControl*) override {}

public:
    int m_value_4;
    int m_value_8;
    int m_selectedIndex;
    W8Vector<W8TextControl*> m_lsButtons;
    W8ControlSelectionListener* m_selectionListener;
};
static_assert(sizeof(W8ControlSelection) == 0x24, "W8ControlSelection_size");
