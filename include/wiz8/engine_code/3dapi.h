#pragma once

struct W8World;
class srScene;

void SetSceneAmbientLightWhite(srScene* scene);
void DestroyAllWorldTriggers(W8World* world);

extern unsigned char g_renderer_ready_00607d7c;
extern unsigned char g_world_cleanup_flag_00659757;
extern unsigned char g_monster_combat_timer_enabled_006f0531;
extern unsigned char g_navigator_vertical_enabled_006081f8;
extern unsigned char g_world_mesh_update_enabled_00607d7d;
