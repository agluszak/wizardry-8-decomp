#ifndef WIZ8_LOCAL_CODE_BUTTON_SOUND_H
#define WIZ8_LOCAL_CODE_BUTTON_SOUND_H

extern char g_button_click_1_62a51c[39];
extern char g_button_click_2_62a544[39];
extern char g_button_whoosh_62a56c[45];

void PlayButtonSound(int sound_id);
void PushButtonSoundScheme005587C0(int scheme, char replace_current);
void ResetButtonSoundScheme(void);

#endif
