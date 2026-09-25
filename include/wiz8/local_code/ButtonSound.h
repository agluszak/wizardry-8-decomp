#ifndef WIZ8_LOCAL_CODE_BUTTON_SOUND_H
#define WIZ8_LOCAL_CODE_BUTTON_SOUND_H

extern char g_button_click_1[39];
extern char g_button_click_2[39];
extern char g_button_whoosh[45];

void PlayButtonSound(int sound_id);
void PushButtonSoundScheme(int scheme, char replace_current);
void ResetButtonSoundScheme(void);

#endif
