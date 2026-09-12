#pragma once

extern wchar_t g_default_level_0064d7b8[];
extern int g_value_006850d5;
#include "wiz8/screen_state.h"
#include "wiz8/vector.h"

#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/local_code/Controls.h"

/* Local Screens\JournalScreen.cpp's live panel.  Construction starts with a
   complete Controls at +0, then installs the independently evidenced callback
   base at +0x4c before the five owned display objects. */
class W8JournalPanel005EF340 : public Controls, public W8TextControl::Listener {
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

static_assert(sizeof(W8JournalPanel005EF340) == 0x68, "W8JournalPanel005EF340_size");

struct W8JournalEntry {
    int level;
    int fact;
    int alternate_text;
};
static_assert(sizeof(W8JournalEntry) == 0x0c, "W8JournalEntry_size");

// SYNTHETIC: WIZ8 0x005bd7d0
// W8JournalPanel005EF340::`scalar deleting destructor'

void RefreshJournalPanel005BD860(void);

/* 0x0068de40: the fact journal, created lazily by the initializer below and
   appended to whenever a fact changes. */
extern W8GrowableVector<W8JournalEntry>* g_fact_journal_entries_0068de40;
void InitializeFactJournal(void);
/* 0x005588F0: append one changed fact to the journal and, unless notices are
   suppressed, post the fact's own journal entry. */
void RecordFactChangeForJournal(int fact_id);
