#pragma once

#include "wiz8/regions.h"

class W8ModalDialogBase;

extern unsigned short g_main_menu_selected_item;
extern unsigned char g_main_menu_has_save_games;
extern unsigned char g_main_menu_redraw;
extern unsigned char g_main_menu_warning_shown;
extern unsigned char g_main_menu_overlay_enabled;
extern unsigned int g_main_menu_overlay_surface;
extern unsigned int g_main_menu_hover_region;
extern W8ModalDialogBase* g_main_menu_dialog;
extern wchar_t* g_pending_main_menu_message;

unsigned char DrawMainMenuItem(short item, short state);
void SetMainMenuMessage(const wchar_t* message);
unsigned char MainMenuIntroduction(const W8RegionEvent* event, W8Region* region);
unsigned char MainMenuNewGame(const W8RegionEvent* event, W8Region* region);
unsigned char MainMenuLoadGame(const W8RegionEvent* event, W8Region* region);
unsigned char MainMenuCredits(const W8RegionEvent* event, W8Region* region);
unsigned char MainMenuOptions(const W8RegionEvent* event, W8Region* region);
unsigned char MainMenuExit(const W8RegionEvent* event, W8Region* region);
unsigned char MainMenuScreenInitialize(void);
unsigned char MainMenuScreenEnter(void);
void MainMenuScreenFrame(void);
unsigned char MainMenuScreenLeave(int leaving);
