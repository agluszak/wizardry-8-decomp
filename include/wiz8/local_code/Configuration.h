#pragma once

void LoadGameConfiguration(void);
unsigned char SaveGameConfiguration(void);
void SetDisplayGamma(float value);
void SetMusicVolume(unsigned char volume);
bool IsMusicMuted(void);
void SetMusicMuted(unsigned char muted);
unsigned int GetTotalPhysicalMemory(void);

/* Local Code\Configuration.cpp owns the persisted 0xa4-byte configuration
   block at 0x006850C8. */

#pragma pack(push, 1)
struct W8GameSettings {
    unsigned char field_000;
    unsigned char numeric_hit_points;
    unsigned char unknown_002[0x4];
    int field_006;
    unsigned char continuous_combat;
    unsigned char auto_advance_character;
    unsigned char tooltips_enabled;
    int difficulty;
    unsigned int text_display_delay_ms;
    int combat_delay_ms;
    int field_019;
    int camera_rotation_mode;
    int camera_rotation_style;
    int tooltip_delay_ms;
    unsigned char field_029;
    unsigned char field_02a;
    unsigned char field_02b;
    unsigned char field_02c;
    unsigned char unknown_02d[0x1];
    unsigned char sound_effects_volume;
    unsigned char music_volume;
    unsigned char voice_volume;
    unsigned char footstep_volume;
    unsigned char muted_sound_effects_volume;
    unsigned char muted_music_volume;
    unsigned char muted_voice_volume;
    unsigned char field_035;
    unsigned char invert_mouse_y;
    float monster_movement_speed;
    unsigned char field_03b;
    float gamma;
    unsigned char field_040;
    unsigned char mouselook_toggle;
    unsigned char field_042;
    unsigned char mouselook_smoothing;
    unsigned char verbose_combat_messages;
    unsigned char auto_save;
    unsigned char intro_seen;
    unsigned char field_047;
    unsigned char field_048;
    unsigned char field_049;
    unsigned char field_04a;
    unsigned char skill_increase_messages;
    unsigned char ctrl_right_click_info;
    unsigned char autoswap_weapons;
    unsigned char autotarget_spells;
    unsigned char autoscroll_combat_messages;
    unsigned char simplified_npc_interaction;
    unsigned char unknown_051[0x53];
};
#pragma pack(pop)

static_assert(sizeof(W8GameSettings) == 0xa4, "W8GameSettings_must_be_0xa4");

extern W8GameSettings g_settings_6850c8;

int GetRendererFamily(void);
