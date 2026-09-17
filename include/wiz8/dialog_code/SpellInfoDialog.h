#pragma once

#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/dialog_code/DialogButton.h"
#include "wiz8/dialog_code/DialogScrollBar.h"
#include "wiz8/dialog_code/DialogTextArea.h"
#include "wiz8/engine_code/game_timer.h"

/* Dialog Code\SpellInfoDialog.cpp. The constructor stores the spell id; Draw
   and DrawLabels index g_spell_records with it. CreateControls, the close
   callback and OnMouseWheel sit with the other popup-info dialogs. */
// VTABLE: WIZ8 0x005efab0
class W8SpellInfoDialog : public W8DialogBase {
public:
    W8SpellInfoDialog(unsigned int spell); /* 0x005DBB60 */
    virtual ~W8SpellInfoDialog() override;
    virtual int CreateControls() override;
    virtual void DestroyControls() override;
    virtual void Draw() override;
    virtual void OnRightButtonUp() override;
    virtual void OnMouseWheel(int delta) override;

private:
    unsigned char PopulateText(); /* 0x005DBEE0 */
    void DrawLabels();            /* 0x005DC490 */
    static void ScrollCallback(W8DialogScrollBar* scroll_bar, int first_visible_entry);

    unsigned int m_spell_054;           /* 0x054 */
    W8DialogScrollBar m_scroll_bar_058; /* 0x058 */
    W8DialogButton m_button_0a4;        /* 0x0a4 */
    W8DialogTextArea m_text_area_0ec;   /* 0x0ec */
    W8GameTimer m_timer_144;            /* 0x144 */
    unsigned int m_animation_frame;     /* 0x168 */
};
static_assert(sizeof(W8SpellInfoDialog) == 0x16c, "W8SpellInfoDialog_size");

/* 0x0060D4E0: the "(on ...)" parenthetical per spell target type. */
extern const wchar_t* g_spell_target_parentheticals_60d4e0[11];

/* 0x00619794: the ", " separator join lists of notices are built with. */
extern const wchar_t g_comma_space_00619794[];

/* 0x0061E9A0: gppStringList indices naming each W8RangeCategory band. */
extern const unsigned short g_spell_range_name_ids_61e9a0[4];
