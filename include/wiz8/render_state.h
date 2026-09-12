#ifndef WIZ8_RENDER_STATE_H
#define WIZ8_RENDER_STATE_H

#include "wiz8/wiz8_windows.h"
#include "wiz8/engine_code/Video2.h"

class srColorSurface;
class srCamera;
class srClass;
class srModeler;
class srGERD;
class srMaterial;
class srModelInstance;
class srNode;
class srScene;
class stSurface2D;
struct EnvironmentColour;
template <class T> class srVector3T;

extern int g_pixel_format_603c48;
extern unsigned char g_fullscreen_603c39;
extern unsigned char g_flag_659711;
extern unsigned char g_flag_65970f;
extern unsigned char g_flag_603c60;
extern unsigned char g_flag_603c4c;
extern const int* g_value_659668;
extern srModeler* g_modeler_65963c;
extern srScene* g_scene_user_659640;
extern srScene* g_scene_fullscreen_659644;
extern srScene* g_scene_permanent_659648;
extern srScene* g_scene_prerender0_65964c;
extern srScene* g_scene_prerender1_659650;
extern srScene* g_scene_overlay0_659654;
extern srScene* g_scene_overlay1_659658;
extern srScene* g_scene_square_65965c;
extern srColorSurface* g_primary_color_surface_659660;
void DrawColorSurface00425590(srColorSurface* surface, int x, int y);
extern srCamera* g_overlay_camera_659670;
extern srCamera* g_square_camera_659674;

extern unsigned char* g_render_options_65a118;
extern unsigned char g_flag_65beaf;
extern srGERD* g_gerd_659634;
extern LPDIRECTDRAWSURFACE2 g_primary_surface_6596a8;
extern stSurface2D* g_surface_node_659664;
extern srMaterial* g_blit_material_65967c;
extern srColorSurface* g_mouse_surface_659688;
extern srNode* g_surface_nodes_654adc[0x12c0];
extern unsigned char g_block_652ddc[0x12c0];
/* Globals the accessor bodies below reach and that more than one recovered
   unit needs. They were each declared locally in the unit that first used
   them; holding them here instead is what stops a second unit from spelling
   the same object a second, divergent way. */
extern IDirectDraw2* g_direct_draw2_6596a0;
extern IDirectDrawSurface* g_video_primary_surface1_6596ac;
extern IDirectDrawSurface2* g_video_primary_surface2_6596b0;
extern srModelInstance* g_current_model_instance_65962c;
extern int g_renderer_mode_603d74;
/* The two detail-slider values GrCycle.cpp's LOD selector reads. */
extern float g_render_brightness_60a210;
extern float g_render_fog_distance_60e610;
extern unsigned char g_render_flag_60a20c;
/* Set by render-option 10; the frame body gates world rendering on it. */
extern unsigned char g_render_flag_603c6c;
/* The pair the mode-select bodies write together, promoted here for the same
   reason: Video2.cpp defines them and the recovered setters read them
   from outside it. */
extern int g_dword_6596ec;
extern int g_dword_6596f0;
/* The cursor mapping scale Video2.cpp owns; render-option scaling writes it. */
extern float g_surface_scale_659680;

/* Engine Code\Quality.cpp seeds the clock, the mapper constructor reseeds it,
   and the texture scrollers read the shared scaled delta. */
extern unsigned int g_frame_tick_65a154;
extern float g_frame_elapsed_65a158;

extern int g_surface_state_6595dc;
extern int g_surface_state_654ad8;
extern int g_viewport_left_6595e8;
extern int g_viewport_top_6595ec;
extern int g_viewport_right_6595f0;
extern int g_viewport_bottom_6595f4;
extern int g_dword_6596d8;
extern int g_resident_texture_policy_659714;
extern unsigned char g_monster_shadow_updates_enabled_0065970c;
extern unsigned char g_flag_65970d;

void SetRenderOption(int option, int enabled);
unsigned char LoadRenderOptions0047B890(int handle);
bool SaveRenderOptions0047B920(int handle);
void SetResidentTexturePolicy(int policy);
void DisableRenderOption(int option);
void EnableRenderOption(int option);
unsigned char GetRendererModeByte(void);
unsigned char GetRenderOptionState(int option);
void EnableAllRenderOptions(void);
unsigned char InitializeEnvironmentColours(void);
void SetViewport(int left, int top, int right, int bottom);
unsigned char InitializeRendererSceneObjects(void);
void PurgeInactiveSceneInstances(srScene* scene);
void Function422B10(void);
void SetPrimarySurfaceTextureHint2Enabled(unsigned char enabled);
unsigned char ClearPrimarySurface(void);
void ResetTransientRenderScenes(void);
void RenderScene(srScene* scene, srCamera* camera, const int* viewport, char preserve_fog);
void RenderFrame(void);
IDirectDrawSurface2* BeginVideoPresentation(void);
unsigned char FinishVideoPresentation(void);

/* Renderer helpers Video2.cpp defines and other recovered units call. They are
   product internals, not part of the released SGP video interface, so they are
   declared here rather than in Video2.h. */
void PublishLightDirection(const EnvironmentColour* direction);
/* Clamp a colour triple to the unit range in place and return it. No
   srVector3T saturation method survives in the SurRender headers, so this stays
   the product's free fastcall. */
srVector3T<float>* __fastcall SaturateColor004299B0(srVector3T<float>* color);
/* Release an srClass whose +0x160 flag selects the renderer mode pair.
   stModelInstance and stModelInstance2D both store that flag as state_160,
   but recovered code never constructs these four slots, so the parameter
   stays the release() ancestor. */
void ReleaseObject004257F0(srClass* object);

void Initialize16BitPixelFormatMasks(void);
unsigned char CreateWizardryWindow(void);
unsigned char InitializePrimaryDirectDrawSurface(void);
unsigned char InitializeVideoDevice(void);
unsigned char Function422800(void);
void Function427440(void);

void AssertFailureHandler(const char* expression, const char* file, long line, const char* message);
unsigned char ClearFlag603C60(void);
unsigned char SetFlag603C60(void);
void SetValue659668(const int* value);
void Function427830(char enabled);
unsigned char Function427260(void);
void SetRendererOption4Enabled(char enabled);
unsigned char HasEnoughFreeDiskSpace(void);
int Function428E20(void);
srModelInstance* GetValue65962C(void);
void SetValue65962C(srModelInstance* value);

#endif
