#pragma once

#include "wiz8/local_code/Controls.h"

/* Local Screens\JournalScreen.cpp's live panel.  Construction starts with a
   complete Controls at +0, then installs the independently evidenced callback
   base at +0x4c before the five owned display objects. */
// VTABLE: WIZ8 0x005ef340
class W8JournalPanel005EF340
    : public Controls,
      public W8TextControlActionListener005ED664 {
public:
    explicit W8JournalPanel005EF340(unsigned int* region_set);
    virtual ~W8JournalPanel005EF340();
    virtual void Redraw() override;
    virtual void OnControlAction(W8TextControl005ED604* control) override;

    W8TextControl005ED604* m_next_050;
    W8TextControl005ED604* m_previous_054;
    W8TextControl005ED604* m_close_058;
    W8TextControl005ED604* m_mode_05c;
    W8TextBuffer005ED5B8* m_page_text_060;
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

unsigned char JournalScreenEnter005BDE40(void);
unsigned char JournalScreenLeave005BE0B0(int leaving);
void JournalScreenFrame005BE110(void);
