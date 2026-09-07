#pragma once

#include <stddef.h>

#include "wiz8/character.h"
#include "wiz8/compat/compiler.h"
#include "wiz8/local_code/Controls.h"

extern "C" {
#include "input.h"
}

class W8DialogBase005DC7A0;
class W8CharacterScreen;

/* The page's secondary subobject begins at +0x4c.  Its two deleting-wrapper
   targets are compiler-generated; the interface itself is independently
   established by construction and adjusted receivers. */
class W8CharacterPageOwner005EF214 {
public:
    virtual void OwnerMethod0() = 0;
    virtual void OwnerMethod1() = 0;
};
static_assert(sizeof(W8CharacterPageOwner005EF214) == 0x4, "W8CharacterPageOwner_size");

/* Common 0x70-byte base constructed by 0x005AFD90.  The four page constructors
   below all call it and SelectPage dispatches these primary slots. */
class W8CharacterPage : public Controls, public W8CharacterPageOwner005EF214 {
public:
    W8CharacterPage(int render_target);                /* 0x005AFD90 */
    virtual ~W8CharacterPage();                        /* 0x005AFE40 */
    virtual void Invalidate(const W8ControlsRect* rect) override; /* 0x005AFF50 */
    virtual void Redraw() override;                    /* 0x005AFF20 */
    virtual void SetCharacter(W8Character* character, void* creation_state,
                              int mode);                /* 0x005AFF00 */
    virtual void Activate() = 0;
    virtual void Deactivate() = 0;
    virtual void Accept() = 0;
    virtual void GetNavigationState(unsigned char* next_enabled,
                                    unsigned char* exit_enabled) = 0;
    virtual void HandleInput(InputAtom* input);         /* 0x005B1BE0 */
    virtual void Refresh();                            /* 0x005B1BF0 */
    virtual void Prepare();                            /* 0x005AFFA0 */
    virtual void OwnerMethod0() override;              /* 0x005B1BC0 */
    virtual void OwnerMethod1() override;              /* 0x005B1B90 */

    int m_entry_count_050;
    int m_entry_capacity_054;
    void** m_entries_058;
    W8CharacterScreen* m_screen_05c;
    W8Character* m_character_060;
    void* m_creation_state_064;
    int m_mode_068;
    unsigned char m_prepared_06c;
    unsigned char m_dirty_06d;
    unsigned char pad_06e[2];
};
static_assert(sizeof(W8CharacterPage) == 0x70, "W8CharacterPage_size");

class W8CharacterPage005EF778 : public W8CharacterPage {
public:
    W8CharacterPage005EF778();                         /* 0x005CBA90 */
    virtual void Activate() override;
    virtual void Deactivate() override;
    virtual void Accept() override;
    virtual void GetNavigationState(unsigned char*, unsigned char*) override;
private:
    unsigned char unknown_070[0x30];
};
static_assert(sizeof(W8CharacterPage005EF778) == 0xa0, "W8CharacterPage005EF778_size");

class W8CharacterPage005EF664 : public W8CharacterPage {
public:
    W8CharacterPage005EF664();                         /* 0x005C8DE0 */
    virtual void Activate() override;
    virtual void Deactivate() override;
    virtual void Accept() override;
    virtual void GetNavigationState(unsigned char*, unsigned char*) override;
private:
    unsigned char unknown_070[0x5b4];
};
static_assert(sizeof(W8CharacterPage005EF664) == 0x624, "W8CharacterPage005EF664_size");

class W8CharacterPage005EF5C8 : public W8CharacterPage {
public:
    W8CharacterPage005EF5C8();                         /* 0x005C7CC0 */
    virtual void Activate() override;
    virtual void Deactivate() override;
    virtual void Accept() override;
    virtual void GetNavigationState(unsigned char*, unsigned char*) override;
private:
    unsigned char unknown_070[8];
};
static_assert(sizeof(W8CharacterPage005EF5C8) == 0x78, "W8CharacterPage005EF5C8_size");

class W8CharacterPage005EF57C : public W8CharacterPage {
public:
    W8CharacterPage005EF57C();                         /* 0x005C73F0 */
    virtual void Activate() override;
    virtual void Deactivate() override;
    virtual void Accept() override;
    virtual void GetNavigationState(unsigned char*, unsigned char*) override;
private:
    unsigned char unknown_070[0x90];
};
static_assert(sizeof(W8CharacterPage005EF57C) == 0x100, "W8CharacterPage005EF57C_size");

/* Primary interface at 0x005EF224, used by the pages to raise the screen-owned
   dialogs and to query the current character. */
class W8CharacterPageHost {
public:
    virtual void UpdateNavigation(W8CharacterPage* page) = 0; /* 0x005B0580 */
    virtual void ShowDialog005B0610(int value) = 0;
    virtual void ShowProfessionInfo(unsigned int profession) = 0;
    virtual void ShowRaceInfo(unsigned int race) = 0;
    virtual void ShowAttributeInfo005B07C0(unsigned int attribute) = 0;
    virtual void ShowAttributeInfo005B0850(unsigned int attribute) = 0;
    virtual void ShowDialog005B08E0(int value) = 0;
    virtual void ShowDescription(int first, int second) = 0;
    virtual void ShowCharacterSummary() = 0;
    virtual unsigned char HasDialog() = 0;
    virtual W8Character* GetOriginalCharacter() = 0;
};
static_assert(sizeof(W8CharacterPageHost) == 0x4, "W8CharacterPageHost_size");

class W8CharacterScreen : public W8CharacterPageHost,
                          public W8TextControl005ED604::Listener {
public:
    W8CharacterScreen(int mode, W8Character* character); /* 0x005B0040 */
    void BuildControls();                                /* 0x005B0140 */
    void UpdateDialog();                                 /* 0x005B04B0 */
    void AdvancePage(unsigned char forward);             /* 0x005B0B50 */
    void SelectPage(int index);                          /* 0x005B0D50 */
    void SyncCharacterForPage(int index);                /* 0x005B0F30 */
    unsigned char CommitCharacter();                     /* 0x005B0FD0 */
    void DrawHeader();                                   /* 0x005B1110 */
    void ShowMessage(wchar_t* text, int confirmation,
                     int response);                      /* 0x005B1430 */
    void HandleDialogResult(int response, unsigned char accepted); /* 0x005B1520 */
    unsigned char ValidateName();                        /* 0x005B1670 */

    virtual void UpdateNavigation(W8CharacterPage* page) override;
    virtual void ShowDialog005B0610(int value) override;
    virtual void ShowProfessionInfo(unsigned int profession) override;
    virtual void ShowRaceInfo(unsigned int race) override;
    virtual void ShowAttributeInfo005B07C0(unsigned int attribute) override;
    virtual void ShowAttributeInfo005B0850(unsigned int attribute) override;
    virtual void ShowDialog005B08E0(int value) override;
    virtual void ShowDescription(int first, int second) override;
    virtual void ShowCharacterSummary() override;
    virtual unsigned char HasDialog() override;
    virtual W8Character* GetOriginalCharacter() override;
    virtual void OnPrimary(W8TextControl005ED604* control) override;
    virtual void OnSecondary(W8TextControl005ED604* control) override;

    int m_mode_008;
    int m_page_index_00c;
    unsigned char m_header_dirty_010;
    unsigned char pad_011[3];
    W8Character* m_original_014;
    W8Character m_character_018;
    unsigned char pad_187a[2];
    unsigned char m_creation_state_187c[0x260];
    int m_pending_page_one_1adc;
    int m_pending_page_zero_1ae0;
    unsigned char m_creation_state_1ae4[8];
    unsigned char m_block_advance_1aec;
    unsigned char m_confirm_profession_1aed;
    unsigned char m_force_transition_1aee;
    unsigned char pad_1aef;
    Controls* m_controls_1af0;
    W8TextControl005ED604* m_previous_1af4;
    W8TextControl005ED604* m_next_1af8;
    W8TextControl005ED604* m_exit_1afc;
    W8TextControl005ED604* m_accept_1b00;
    W8TextControl005ED604* m_reset_1b04;
    unsigned char m_page_enabled_1b08[4];
    W8CharacterPage* m_pages_1b0c[4];
    W8DialogBase005DC7A0* m_dialog_1b1c;
    unsigned int m_dialog_response_1b20;
    unsigned char m_capture_dialog_result_1b24;
    unsigned char pad_1b25[3];
};
static_assert(sizeof(W8CharacterScreen) == 0x1b28, "W8CharacterScreen_size");

extern W8CharacterScreen* g_character_screen_0069c2e8;
unsigned char CharacterScreenEnter005B1750(void);
unsigned char CharacterScreenLeave005B1840(int leaving);
void CharacterScreenFrame005B18E0(void);
