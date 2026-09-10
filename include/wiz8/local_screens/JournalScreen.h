#pragma once

extern wchar_t g_default_level_0064d7b8[];
#include "wiz8/screen_state.h"

#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/local_code/Controls.h"

/* Local Screens\JournalScreen.cpp's live panel.  Construction starts with a
   complete Controls at +0, then installs the independently evidenced callback
   base at +0x4c before the five owned display objects. */
class W8JournalPanel005EF340
    : public Controls,
      public W8TextControl::Listener {
public:
    explicit W8JournalPanel005EF340(unsigned int* region_set);
    virtual ~W8JournalPanel005EF340();
    virtual void Redraw() override;
    virtual void OnPrimary(W8TextControl* control) override;

    W8TextControl* m_next_050;
    W8TextControl* m_previous_054;
    W8TextControl* m_close_058;
    W8TextControl* m_mode_05c;
    W8TextBuffer* m_page_text_060;
    unsigned char m_alternate_mode_064;
    unsigned char m_pad_065[3];
};

static_assert(sizeof(W8JournalPanel005EF340) == 0x68,
              "W8JournalPanel005EF340_size");

struct W8JournalEntry {
    int level;
    int fact;
    int alternate_text;
};
static_assert(sizeof(W8JournalEntry) == 0x0c, "W8JournalEntry_size");

// SYNTHETIC: WIZ8 0x005bd7d0
// W8JournalPanel005EF340::`scalar deleting destructor'

void RefreshJournalPanel005BD860(void);
