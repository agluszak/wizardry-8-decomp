#pragma once

#include "input.h"
#include "wiz8/local_code/TextControl.h"

#include <cstddef>

struct W8Region;

/* The eight party-condition buttons beside the portraits, created by
   CreateConditionButtons; its asserts name gpConditionButtonsPanel and
   gpConditionButtons[uiSlot]. W8TextControl is 0xb8; the derived part adds a
   condition-icon selector byte, the catalog image base the refresh copies in
   from the status row, and the party slot the button tracks. */
// VTABLE: WIZ8 0x005eec50
class W8ConditionButton : public W8TextControl {
public:
    /* 0x0059BDB0 shows the whole body inlined at the allocation site: the base
       constructor, the 0xff selector store and the slot store, then the vptr. */
    W8ConditionButton(Controls* panel, unsigned int region, int left, int top, int right,
                      int bottom, int text_40, int text_44, int text_48, int text_4c, int text_54,
                      int text_50, int text_58, int ui_slot)
        : W8TextControl(panel, region, left, top, right, bottom, text_40, text_44, text_48, text_4c,
                        text_54, text_50, text_58),
          m_condition_b8(0xff), m_ui_slot_c0(ui_slot)
    {
    }
    virtual ~W8ConditionButton() override;
    virtual void Redraw(int full_redraw) override;
    virtual void OnMouseLeave(int event) override;
    virtual void OnLeftButtonDown(int event) override;
    virtual void OnLeftButtonUp(int event) override;

    /* 0xb8: 0xff while the slot has no shown condition, else which of the two
       condition icon groups Redraw draws. */
    unsigned char m_condition_b8;
    unsigned char m_pad_b9[3];
    /* 0xbc: catalog image base copied whole from the status row; Redraw adds
       the per-condition offset (0xb6 or 0xc9). */
    int m_image_object_bc;
    int m_ui_slot_c0; /* 0xc0: party slot the button tracks */
};
static_assert(sizeof(W8ConditionButton) == 0xc4, "W8ConditionButton_size");
static_assert(offsetof(W8ConditionButton, m_condition_b8) == 0xb8,
              "W8ConditionButton_condition_b8");
static_assert(offsetof(W8ConditionButton, m_image_object_bc) == 0xbc,
              "W8ConditionButton_image_object_bc");
static_assert(offsetof(W8ConditionButton, m_ui_slot_c0) == 0xc0, "W8ConditionButton_ui_slot_c0");

void ReleasePortraitControls(void);
void ReleaseConditionButtons(void);
/* Create the eight level-up portrait buttons (gpLevelButtons) and their panel. */
void CreateLevelButtons(void);                                   /* 0x0059B940 */
void ToggleNumericHitPoints(void);                               /* 0x0059AA30 */
void CreateConditionButtons(void);                               /* 0x0059BDB0 */
void DisablePortraitControls0059BB40(void);                      /* 0x0059BB40 */
void EnablePortraitAdvanceRegions0059BB70(void);                 /* 0x0059BB70 */
void InvalidatePortraitControl0059BBD0(unsigned int party_slot); /* 0x0059BBD0 */
/* 0x0059C030 / 0x0059BFC0: hide or show the condition-button region set for
   the current layout. SyncMainGameModeRegions picks between them. */
void DisableConditionButtons0059C030(void);
void EnableConditionButtons0059BFC0(void);

/* Region callbacks the eight portrait level-up buttons and condition buttons
   share. */
unsigned char PortraitControlRegionEvent(const InputAtom* event, W8Region* region); /* 0x0059BD20 */
unsigned char ConditionButtonRegionEvent(const InputAtom* event, W8Region* region); /* 0x0059C260 */

/* Main-game portrait overlay helpers used when a party slot refreshes. */
unsigned char PreparePartyPortraitOverlay(unsigned int party_slot, unsigned int flags,
                                          unsigned int top); /* 0x005993A0 */
void RedrawPartyPortraitOverlay(unsigned int party_slot, char highlighted, char overlay_ready,
                                char slot_enabled); /* 0x005994C0 */

/* 0x0059AA60: the slot's menu/portrait anchor table - the keyboard-menu
   panel's origin, the portrait band's two x edges, the grid row and the
   column pixel; the adjust flag applies the compact-display shift. */
void GetPartySlotMenuAnchor(int party_slot, int* menu_x, int* menu_y, int* band_menu_edge,
                            int* band_portrait_edge, int* grid_row, int* column_x, int adjust);
