#pragma once

void ReleasePortraitControls(void);
void ReleaseConditionButtons(void);
unsigned char ReleasePortraitQuoteBubble(int quote_handle);
int LayoutPortraitQuoteBubble(int surface_handle, unsigned char param_2, unsigned char param_3,
                              const wchar_t* text, unsigned int max_width, int param_6, int param_7,
                              int param_8, unsigned short* out_width, unsigned short* out_height,
                              unsigned int font_palette);
const wchar_t* FormatPortraitQuoteNoticeText(int quote_handle, int channel, int scroll_range,
                                             const void* unused);
void RefreshSelectedPartyPortrait(unsigned int party_slot);
void Function59B940(void);
void Function59BDB0(void);
void EnablePortraitAdvanceRegions0059BB70(void);                 /* 0x0059BB70 */
void InvalidatePortraitControl0059BBD0(unsigned int party_slot); /* 0x0059BBD0 */

/* Main-game portrait overlay helpers used when a party slot refreshes. */
unsigned char PreparePartyPortraitOverlay(unsigned int party_slot, unsigned int flags,
                                          unsigned int top); /* 0x005993A0 */
void RedrawPartyPortraitOverlay(unsigned int party_slot, char highlighted, char overlay_ready,
                                char slot_enabled); /* 0x005994C0 */
