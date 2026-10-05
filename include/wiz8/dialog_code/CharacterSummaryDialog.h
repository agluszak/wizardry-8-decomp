#pragma once

#include "timer.h"
#include "wiz8/layouts/character.h"
#include "wiz8/character_event_queue.h"
#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/MonsterManager.h"

#include "input.h"

class W8DialogNumericInput;
class W8TextBuffer;

/* The character-summary popup temporarily installs the character as party
   slot zero so the ordinary portrait and quote machinery can render it.  Its
   construction, vtable, embedded snapshots and complete lifecycle establish
   a distinct W8DialogBase-derived object, even though retail exposes no
   original class spelling. */
// VTABLE: WIZ8 0x005efdc0
class W8CharacterSummaryDialog : public W8DialogBase {
public:
    explicit W8CharacterSummaryDialog(W8Character* character); /* 0x005E0320 */
    virtual ~W8CharacterSummaryDialog() override;              /* 0x005E0410 */
    virtual int CreateControls() override;                     /* 0x005E04E0 */
    virtual void DestroyControls() override;                   /* 0x005E0590 */
    virtual void Draw() override;                              /* 0x005E07B0 */
    virtual int GetDialogType() override;                      /* 0x005E0C30 */
    virtual unsigned char ProcessInput() override;             /* 0x005E0920 */
    virtual void OnNumericInputChanged(int value) override;    /* 0x005E0860 */

    void DrawPortraitAnimationFrame(); /* 0x005E0830 */

private:
    bool CreateQuoteText();
    unsigned char HandleInputEvent(const InputAtom* input);

    bool m_voice_started;
    unsigned char pad_055[3];
    W8TextBuffer* m_quote_text;
    W8DialogNumericInput* m_numeric_input;
    void* m_field_060;
    int m_remaining;
    int m_taken;
    int m_total;
    int m_field_070;
    W8Character* m_character;
    W8Character m_saved_character;
    W8MonsterManagerEntry m_saved_monster_entry;
    W8PartySlotRow m_saved_party_row;
    unsigned char m_field_1af8;
    bool m_portrait_clock_started;
    unsigned char pad_1afa[2];
    TIMER m_portrait_clock;
};

static_assert(sizeof(W8CharacterSummaryDialog) == 0x1b00,
              "W8CharacterSummaryDialog_must_be_0x1b00");
