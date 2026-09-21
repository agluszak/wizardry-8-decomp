#include <stdio.h>
#include "wiz8/local_screens/MGSKeyboard.h"
#include "input.h"
#include "wiz8/character_event_queue.h"
#include "wiz8/cursor.h"
#include "wiz8/engine_code/Cursor3d.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/3dapi.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/layouts/character.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/Search.h"
#include "wiz8/local_code/CombatPartyMovement.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/local_code/GameplayTime.h"
#include "wiz8/local_code/LoadSaveGame.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/local_code/Traps.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/local_screens/CreditsScreen.h"
#include "wiz8/local_screens/MGSButtons.h"
#include "wiz8/local_screens/MGSPortraitCombat.h"
#include "wiz8/local_screens/MGSFormation.h"
#include "wiz8/local_screens/mipe.h"
#include "wiz8/local_screens/MGSPortraits.h"
#include "wiz8/local_screens/MGSRadarMap.h"
#include "wiz8/local_screens/MGSSpellCasting.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_screens/MGSUseItemSelect.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/NPCInteractionSubscreen.h"
#include "wiz8/local_screens/OptionsScreen.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/npc_interaction.h"
#include "wiz8/regions.h"
#include "wiz8/sr_api.h"
#include "wiz8/text_input.h"
#include "wiz8/utility.h"
#include "wiz8/version.h"
#include "wiz8/virtual_file.h"
#include "wiz8/world_cursor.h"
#include "wiz8/xstatus.h"

#include "FileMan.h"
#include "input.h"

#include <string.h>
#include <wchar.h>
#include <wctype.h>

/* Local Screens\MGSKeyboard.cpp owns the binding vector, its command-keyed
   lookup and the singleton ResetMGSKeyboardBindings installs.
   MGSKeyboard::LoadDefaults moved to Local Code\InputMapper.cpp. */

// GLOBAL: WIZ8 0x0069b7e4
MGSKeyboard* g_mgs_keyboard;

// FUNCTION: WIZ8 0x0055CFD0
MGSKeyboard::MGSKeyboard() {}

// SYNTHETIC: WIZ8 0x0055D160
// MGSKeyboard::`scalar deleting destructor'

// FUNCTION: WIZ8 0x0055D180
MGSKeyboard::~MGSKeyboard()
{
    Clear();
}

// FUNCTION: WIZ8 0x0055d260
int MGSKeyboard::FindBinding(int command) const
{
    int count = m_bindings.GetCount();
    int index = 0;
    if (count > 0) {
        do {
            MGSKeyBinding* binding = *m_bindings.GetAt(index);
            if (binding->command == command) {
                return index;
            }
            ++index;
        } while (index < count);
    }
    return -1;
}

// FUNCTION: WIZ8 0x0055d2a0
int MGSKeyboard::FindCommandForEvent(const InputAtom* event) const
{
    int count = m_bindings.GetCount();
    int index = 0;
    if (count > 0) {
        do {
            MGSKeyBinding* binding = *m_bindings.GetAt(index);
            if (binding->active == event->usEvent && binding->modifiers == event->usKeyState &&
                binding->key == event->usParam) {
                return binding->command;
            }
            ++index;
        } while (index < count);
    }
    return -1;
}

// FUNCTION: WIZ8 0x0055d300
MGSKeyBinding* MGSKeyboard::GetBinding(int index) const
{
    if (index >= 0) {
        MGSKeyBinding** binding = m_bindings.data;
        if (index < m_bindings.count) {
            binding += index;
        }
        return *binding;
    }
    return 0;
}

// FUNCTION: WIZ8 0x0055d320
unsigned char MGSKeyboard::IsCommandPressed(unsigned int command) const
{
    MGSKeyBinding* binding = m_command_index.Lookup(&command);
    if (binding != 0 && gfKeyState[binding->key] != 0) {
        unsigned short modifiers = 0;
        if (gfKeyState[VK_SHIFT] != 0) {
            modifiers |= SHIFT_DOWN;
        }
        if (gfKeyState[VK_MENU] != 0) {
            modifiers |= ALT_DOWN;
        }
        if (gfKeyState[VK_CONTROL] != 0) {
            modifiers |= CTRL_DOWN;
        }
        if (modifiers == binding->modifiers) {
            return 1;
        }
    }
    return 0;
}

/* Discard every queued input atom. MainGameScreenEnter calls this so stale
   presses do not fire against the freshly entered screen state. */
// FUNCTION: WIZ8 0x0055D3C0
void DrainInputEventQueue(void)
{
    InputAtom event;

    while (DequeueEvent(&event)) {
    }
}

// FUNCTION: WIZ8 0x0055D3F0
void MGSKeyboard::Clear()
{
    for (int index = m_bindings.count - 1; index >= 0; --index) {
        delete m_bindings.RemoveAt(index);
    }
    m_command_index.Clear();
}

// FUNCTION: WIZ8 0x0055d590
unsigned char MGSKeyboard::Load(int handle, unsigned char clear)
{
    int count;

    if (clear != 0) {
        Clear();
    }
    FileRead(handle, &count, sizeof(count), 0);
    for (int index = 0; index < count; ++index) {
        MGSKeyBinding* binding = new MGSKeyBinding;
        FileRead(handle, binding, sizeof(*binding), 0);

        int old_index = FindBinding(binding->command);
        if (old_index != -1) {
            unsigned int command = binding->command;
            delete m_bindings.RemoveAt(old_index);
            m_command_index.Remove(&command);
        }
        if (m_bindings.Add(binding) != -1) {
            unsigned int command = binding->command;
            m_command_index.Insert(&command, &binding);
        }
    }
    return 1;
}

// FUNCTION: WIZ8 0x0055d7a0
unsigned char MGSKeyboard::Save(int handle) const
{
    int count = m_bindings.GetCount();
    FileWrite(handle, &count, sizeof(count), 0);
    for (int index = 0; index < count; ++index) {
        FileWrite(handle, *m_bindings.GetAt(index), sizeof(MGSKeyBinding), 0);
    }
    return 1;
}

// FUNCTION: WIZ8 0x00592BE0
void ResetMGSKeyboardBindings()
{
    if (g_mgs_keyboard == 0) {
        g_mgs_keyboard = new MGSKeyboard;
    } else {
        g_mgs_keyboard->Clear();
    }
    g_mgs_keyboard->LoadDefaults("Data\\Strings\\MGSKeyboard.ini");
}

#define MGSKEYBOARD_CPP "C:\\Projects\\Wizardry 8\\Local Screens\\MGSKeyboard.cpp"

/* Route one non-mouse input atom: text entry, NPC dialogue and the trap text
   box consume key presses first; the MIPE editor and the record-mode console
   gate on their flags; everything else resolves to a bound MGS command.
   Attribution-gap body leading the MGSKeyboard.cpp hull at 0x00591960. */
// FUNCTION: WIZ8 0x00591890
unsigned char HandleMainGameInputEvent(const InputAtom* input)
{
    if ((input->usEvent == KEY_DOWN || input->usEvent == KEY_REPEAT) &&
        static_cast<char>(HandleTextInput(input)) != 0) {
        return 1;
    }
    if ((input->usEvent == KEY_DOWN || input->usEvent == KEY_REPEAT) &&
        gXStatus.fNpcDialogueMode != 0) {
        HandleNpcDialogueKeyEvent00574BB0(input);
        return 1;
    }
    if ((input->usEvent == KEY_DOWN || input->usEvent == KEY_REPEAT) &&
        gXStatus.fTrapInteractMode != 0 && TextBoxHandleKey(input) != 0) {
        return 1;
    }
    if (GetFlag68F105() != 0 && HandleMipeKey0057C230(input) != 0) {
        return 1;
    }
    if (GetFlag69DA6C() != 0) {
        if (g_monster_combat_timer_enabled_006f0531 == 0 &&
            HandleRecordModeKey005E3610(input, PromptRecordModeEntry005E35A0) == 1) {
            ApplyRecordModeLine005E34B0();
            return 1;
        }
    } else {
        DispatchMGSCommand(g_mgs_keyboard->FindCommandForEvent(input));
    }
    return 1;
}

// FUNCTION: WIZ8 0x00591960
void DispatchMGSCommand(int command)
{
    switch (command) {
    case W8_MGS_COMMAND_CANCEL:
        if (IsWorldCursorVisible() != 0) {
            ToggleWorldCursor();
        } else if (gXStatus.fSurprisePossible != 0) {
            AcknowledgeSurprise00502790();
        } else if (gXStatus.fSpellCastMode != 0) {
            ResetSpellCastingSelection005A0B90();
        } else if (gXStatus.fLockInteractMode != 0) {
            EndLockInteractMode(0);
        } else if (gXStatus.fItemSelectMode != 0) {
            CloseUseItemSelection0059D950();
        } else if (gXStatus.iTargetingMode != 0) {
            SetTargetingMode(0);
        } else if (gXStatus.fReviewCharacterMode != 0) {
            CloseFormationPanel();
        } else if (IsNpcScriptSessionActive() != 0) {
            TryFinishNpcVoicePlayback(1);
        } else if (gXStatus.character_event_queue->HasActiveEvents() != 0) {
            gXStatus.character_event_queue->CompleteFirstActiveEvent();
        } else {
            if (gXStatus.fCombatMode != 0) {
                ToggleCombatMode();
                if (gXStatus.fCombatMode == 0) {
                    InvalidateRegion(0xa8, 0x16e, 0x1c4, 0x1ba, 0);
                    return;
                }
            }
            ClearValue69DA68();
            ShowMainGameNoticeLine(gppStringList[0x1de4 / 4], OnLeaveGameConfirmClosed00560A70, 1,
                                   1);
        }
        InvalidateRegion(0xa8, 0x16e, 0x1c4, 0x1ba, 0);
        break;
    case W8_MGS_COMMAND_QUIT_GAME:
        ShowMainGameNoticeLine(gppStringList[0x20c8 / 4], OnQuitGameDialogClosed, 1, 1);
        break;
    case W8_MGS_COMMAND_TOGGLE_FULLSCREEN:
        ResetTransientRenderScenes();
        SetRendererMode6596EC();
        SetRendererModePair();
        VideoFullScreen(VideoIsFullScreen() == 0);
        break;
    case W8_MGS_COMMAND_LOAD_OPTIONS:
        if (IsScreenInputBlocked() == 0 && IsLevelDataFlag4EffectivelySet() != 0) {
            g_pending_screen_state.mode = 1;
            SetPendingScreenState(W8_SCREEN_OPTIONS);
        }
        break;
    case W8_MGS_COMMAND_SAVE_OPTIONS:
        if (IsScreenInputBlocked() == 0 && IsLevelDataFlag4EffectivelySet() != 0) {
            g_pending_screen_state.mode = 2;
            SetPendingScreenState(W8_SCREEN_OPTIONS);
        }
        break;
    case W8_MGS_COMMAND_COMBAT_DELAY_UP: {
        unsigned int delay = g_settings_6850c8.combat_delay_ms;
        if (delay < 0x1388) {
            if (delay % 0xfa == 0) {
                delay += 0xfa;
            } else {
                delay += 0xfa - delay % 0xfa;
            }
            g_settings_6850c8.combat_delay_ms = delay;
            ShowNoticef(0xc, L"%s %d", gppStringList[0x1ee8 / 4], 0x14 - delay / 0xfa);
        } else {
            ShowNoticef(0xc, L"%s (%d)", gppStringList[0x1eec / 4], 0x14 - delay / 0xfa);
        }
        break;
    }
    case W8_MGS_COMMAND_COMBAT_DELAY_DOWN: {
        unsigned int delay = g_settings_6850c8.combat_delay_ms;
        if (delay > 0) {
            if (delay % 0xfa == 0) {
                delay -= 0xfa;
            } else {
                delay -= delay % 0xfa;
            }
            g_settings_6850c8.combat_delay_ms = delay;
            ShowNoticef(0xc, L"%s %d", gppStringList[0x1ee8 / 4], 0x14 - delay / 0xfa);
        } else {
            ShowNoticef(0xc, L"%s (%d)", gppStringList[0x1ef0 / 4], 0x14 - delay / 0xfa);
        }
        break;
    }
    case W8_MGS_COMMAND_TEXT_DELAY_UP: {
        unsigned int delay = g_settings_6850c8.text_display_delay_ms;
        if (delay < 0x1388) {
            if (delay % 0xfa == 0) {
                delay += 0xfa;
            } else {
                delay += 0xfa - delay % 0xfa;
            }
            g_settings_6850c8.text_display_delay_ms = delay;
            ShowNoticef(0xc, L"%s %d", gppStringList[0x1edc / 4], 0x14 - delay / 0xfa);
        } else {
            ShowNoticef(0xc, L"%s (%d)", gppStringList[0x1ee0 / 4], 0x14 - delay / 0xfa);
        }
        break;
    }
    case W8_MGS_COMMAND_TEXT_DELAY_DOWN: {
        unsigned int delay = g_settings_6850c8.text_display_delay_ms;
        if (delay > 0) {
            if (delay % 0xfa == 0) {
                delay -= 0xfa;
            } else {
                delay -= delay % 0xfa;
            }
            g_settings_6850c8.text_display_delay_ms = delay;
            ShowNoticef(0xc, L"%s %d", gppStringList[0x1edc / 4], 0x14 - delay / 0xfa);
        } else {
            ShowNoticef(0xc, L"%s (%d)", gppStringList[0x1ee4 / 4], 0x14 - delay / 0xfa);
        }
        break;
    }
    case W8_MGS_COMMAND_SHOW_VERSION: {
        char version_text[0x40];
        FormatVersionBanner004E3620(version_text, 1, 1, 1);
        ShowNotice(0xc, ConvertStringToWide(version_text), -1, -1, 0);
        break;
    }
    case W8_MGS_COMMAND_LOOK_LEVEL:
        if (IsWorldCursorVisible() != 0) {
            UpdateWorldCursorPlacement00491EC0();
        } else {
            LevelCamera();
        }
        break;
    case W8_MGS_COMMAND_PAUSE_GAME:
        ToggleMainGamePause();
        break;
    case W8_MGS_COMMAND_NEXT_LAYOUT:
        if (gXStatus.fNpcDialogueMode != 0 || gXStatus.fCampMode != 0) {
            break;
        }
        if (IsScreenInputBlocked() != 0) {
            ApplyMainGameModeFlag(static_cast<W8MainUiMode>((g_level_block->main_ui_mode - 1) & 1),
                                  1);
        } else {
            ApplyMainGameModeFlag(static_cast<W8MainUiMode>((g_level_block->main_ui_mode + 1) % 3),
                                  1);
        }
        break;
    case W8_MGS_COMMAND_PREV_LAYOUT:
        if (IsScreenInputBlocked() != 0) {
            ApplyMainGameModeFlag(static_cast<W8MainUiMode>((g_level_block->main_ui_mode - 1) & 1),
                                  1);
        } else {
            ApplyMainGameModeFlag(static_cast<W8MainUiMode>((g_level_block->main_ui_mode + 2) % 3),
                                  1);
        }
        break;
    case W8_MGS_COMMAND_AUTOMAP:
        if (IsScreenInputBlocked() == 0) {
            OpenAutomapScreen();
        }
        break;
    case W8_MGS_COMMAND_OPTIONS:
        if (gXStatus.fNpcDialogueMode != 0 || CanOpenNpcDialogue() != 0 ||
            gXStatus.fLockInteractMode != 0 || gXStatus.fTrapInteractMode != 0 ||
            gXStatus.fCampMode != 0 || gXStatus.fLockInteract != 0 || gXStatus.fTrapInteract != 0) {
            break;
        }
        if (IsLevelDataFlag4EffectivelySet() == 0) {
            break;
        }
        ClearScreenWait();
        break;
    case W8_MGS_COMMAND_JOURNAL:
        TryMGSActionKey(W8_MGS_ACTION_JOURNAL);
        break;
    case W8_MGS_COMMAND_INVENTORY:
        if (CanOpenNpcDialogue() != 0 || gXStatus.fCampMode != 0 ||
            g_status_685170.selected_character == -1 ||
            (gXStatus.fNpcDialogueMode != 0 &&
             g_screen_state_00649f1c->value_fc == W8_DIALOGUE_LAYOUT_TRANSCRIPT)) {
            break;
        }
        OpenCharacterScreenForPartySlot(g_status_685170.selected_character, 0);
        break;
    case W8_MGS_COMMAND_USE_ITEM:
        TryMGSActionKey(W8_MGS_ACTION_USE_ITEM_VIEW);
        break;
    case W8_MGS_COMMAND_USE_LAST_ITEM:
        TryMGSActionKey(W8_MGS_ACTION_USE_RECORDED_ITEM);
        break;
    case W8_MGS_COMMAND_CAST_SPELL:
        TryMGSActionKey(W8_MGS_ACTION_SPELL_VIEW);
        break;
    case W8_MGS_COMMAND_CAST_LAST_SPELL:
        TryMGSActionKey(W8_MGS_ACTION_CAST_RECORDED_SPELL);
        break;
    case W8_MGS_COMMAND_CAMP:
        RequestCamp00502460();
        break;
    case W8_MGS_COMMAND_TOGGLE_SEARCH:
        if (IsScreenInputBlocked() == 0) {
            ToggleSearchMode();
        }
        break;
    case W8_MGS_COMMAND_TOGGLE_COMBAT:
        if (IsScreenInputBlocked() == 0) {
            ToggleCombatMode();
        }
        break;
    case W8_MGS_COMMAND_QUICK_SAVE: {
        char slot_name[0x100];
        if (IsScreenInputBlocked() != 0 || IsLevelDataFlag4EffectivelySet() == 0) {
            break;
        }
        if (g_status_685170.iron_man != 0 && g_flag_689b32 == 0) {
            ShowNotice(0xc, gppStringList[0x20b0 / 4], -1, -1, 0);
            break;
        }
        if (gXStatus.fCombatMode != 0) {
            if (g_flag_689b32 != 0) {
                EndCombat004EA310(0);
            } else {
                ShowNotice(0xc, gppStringList[0x1dd0 / 4], -1, -1, 0);
                break;
            }
        }
        SelectQuickSaveSlotForWrite(slot_name);
        if (SaveGame(slot_name, 0)) {
            SetLastSaveName(ConvertStringToWide(slot_name));
            ShowNotice(0xc, gppStringList[0x1bd4 / 4], -1, -1, 0);
        } else {
            ShowNotice(0xc, gppStringList[0x1bd8 / 4], -1, -1, 0);
        }
        break;
    }
    case W8_MGS_COMMAND_QUICK_LOAD: {
        char slot_name[0x100];
        if (IsScreenInputBlocked() != 0) {
            break;
        }
        if (g_status_685170.iron_man != 0 && g_flag_689b32 == 0) {
            ShowNotice(0xc, gppStringList[0x20b0 / 4], -1, -1, 0);
            break;
        }
        if (FindStartupQuickSave(slot_name)) {
            SetLastSaveName(ConvertStringToWide(slot_name));
            ClearHeldItemDisplay();
            g_pending_screen_state.mode = 1;
            strcpy(g_pending_screen_state.name, slot_name);
            g_pending_screen_state.parameter = GetSaveGameLevel(g_pending_screen_state.name);
            CloseMainGameOverlays();
            SetMainGameMode00568390(0);
            SetPendingScreenState(W8_SCREEN_PLEASE_WAIT);
        } else {
            ShowNotice(0xc, gppStringList[0x1e18 / 4], -1, -1, 0);
        }
        break;
    }
    case W8_MGS_COMMAND_RADAR_ZOOM:
        if (g_level_block->radar_map_visible != 0) {
            ToggleRadarMapZoom();
        }
        break;
    case W8_MGS_COMMAND_REPLAY_QUOTE:
        RequeueSelectedPortraitEvent();
        break;
    case W8_MGS_COMMAND_SWAP_WEAPONS:
        if (IsScreenInputBlocked() == 0 && g_status_685170.selected_character != -1) {
            BindCharacterItems(g_status_685170.selected_character, 1);
        }
        break;
    case W8_MGS_COMMAND_SWAP_ALL_WEAPONS:
        if (IsScreenInputBlocked() == 0) {
            BindEveryPartyItem();
        }
        break;
    case W8_MGS_COMMAND_SELECT_RECRUITED_1:
    case W8_MGS_COMMAND_SELECT_RECRUITED_2:
    case W8_MGS_COMMAND_SELECT_PC_1:
    case W8_MGS_COMMAND_SELECT_PC_2:
    case W8_MGS_COMMAND_SELECT_PC_3:
    case W8_MGS_COMMAND_SELECT_PC_4:
    case W8_MGS_COMMAND_SELECT_PC_5:
    case W8_MGS_COMMAND_SELECT_PC_6: {
        int slot = command - W8_MGS_COMMAND_SELECT_RECRUITED_1;
        if (g_status_685170.buffers.party_rows[slot].occupied == 0) {
            break;
        }
        if (g_status_685170.selected_character != slot) {
            SelectPartyCharacter(slot);
            break;
        }
        if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_PORTRAITS) {
            break;
        }
        if (g_level_block->portrait_refresh_pending[slot] != 0) {
            ClearPortraitRefreshSlot(slot);
        } else {
            RefreshSelectedPartyPortrait(slot);
            gXStatus.monster_manager_entries[slot].portrait_refresh_pinned = 1;
        }
        break;
    }
    case W8_MGS_COMMAND_TEXTBOX_PAGE_UP:
        ScrollTextBoxUp(7);
        break;
    case W8_MGS_COMMAND_TEXTBOX_PAGE_DOWN:
        ScrollTextBoxDown(7);
        break;
    case W8_MGS_COMMAND_TEXTBOX_TOP:
        ScrollTextBoxTo(0);
        break;
    case W8_MGS_COMMAND_TEXTBOX_BOTTOM:
        ScrollDialogueTextBoxToLine0058BA60();
        break;
    case W8_MGS_COMMAND_TEXTBOX_CLEAR:
        if (gXStatus.fSpellCastMode == 0 && gXStatus.fItemSelectMode == 0) {
            ResetEditorStatusLine0058AA20(-1);
        }
        break;
    case W8_MGS_COMMAND_START_COMBAT_ROUND:
        if (gXStatus.fCombatMode == 0 || gXStatus.fSpellCastMode != 0 ||
            gXStatus.fItemSelectMode != 0) {
            break;
        }
        if (g_combat_state->flag_000 == 0) {
            BeginCombatExecution004E8370();
        } else if (gXStatus.fPartyMovementUi != 0 && CanPartyMove() == 0 &&
                   GetLevelDataFlag6() == 0) {
            BeginFreeTurnPhase();
        }
        break;
    case W8_MGS_COMMAND_CONTINUOUS_COMBAT:
        if (IsScreenInputBlocked() == 0) {
            TogglePartyCombatStance();
        }
        break;
    case W8_MGS_COMMAND_CYCLE_TARGET:
        if (g_status_685170.selected_character != -1) {
            CycleToNextTarget(g_status_685170.selected_character);
        }
        break;
    case W8_MGS_COMMAND_ATTACK:
        TryMGSActionKey(W8_MGS_ACTION_ATTACK);
        break;
    case W8_MGS_COMMAND_BERSERK:
        TryMGSActionKey(W8_MGS_ACTION_BERSERK);
        break;
    case W8_MGS_COMMAND_BREATHE:
        TryMGSActionKey(W8_MGS_ACTION_BREATHE);
        break;
    case W8_MGS_COMMAND_TURN_UNDEAD:
        TryMGSActionKey(W8_MGS_ACTION_TURN_UNDEAD);
        break;
    case W8_MGS_COMMAND_PRAY:
        TryMGSActionKey(W8_MGS_ACTION_PRAY);
        break;
    case W8_MGS_COMMAND_DEFEND:
        TryMGSActionKey(W8_MGS_ACTION_DEFEND);
        break;
    case W8_MGS_COMMAND_PROTECT:
        TryMGSActionKey(W8_MGS_ACTION_PROTECT);
        break;
    case W8_MGS_COMMAND_EQUIP:
        TryMGSActionKey(W8_MGS_ACTION_EQUIP);
        break;
    case W8_MGS_COMMAND_PARTY_WALK:
        TryMGSActionKey(W8_MGS_ACTION_WALK);
        break;
    case W8_MGS_COMMAND_PARTY_RUN:
        TryMGSActionKey(W8_MGS_ACTION_RUN);
        break;
    case W8_MGS_COMMAND_REPEAT_ACTION:
        TryMGSActionKey(W8_MGS_ACTION_REPEAT);
        break;
    case W8_MGS_COMMAND_DEBUG_AUDIT_QUOTES:
        AuditNpcScriptQuotes00529660();
        break;
    case W8_MGS_COMMAND_DEBUG_NUMERIC_HP:
        ToggleNumericHitPoints();
        break;
    case W8_MGS_COMMAND_DEBUG_INJECT_CLICK:
        if (IsCursorInsideViewport() != 0) {
            unsigned int position = (static_cast<unsigned int>(gusMouseYPos) << 16) | gusMouseXPos;
            QueueEvent(LEFT_BUTTON_DOWN, 0, position);
            QueueEvent(LEFT_BUTTON_UP, 0, position);
            gfRecordedLeftButtonUp = 0;
        }
        break;
    case W8_MGS_COMMAND_TOGGLE_AUTO_ADVANCE:
        g_settings_6850c8.auto_advance_character ^= 1;
        ShowNotice(
            0xc,
            gppStringList[g_settings_6850c8.auto_advance_character != 0 ? 0x1f68 / 4 : 0x1f6c / 4],
            -1, -1, 0);
        break;
    case W8_MGS_COMMAND_TEXTBOX_SCROLL_UP:
        ScrollTextBoxUp(1);
        break;
    case W8_MGS_COMMAND_TEXTBOX_SCROLL_DOWN:
        ScrollTextBoxDown(1);
        break;
    case W8_MGS_COMMAND_DEBUG_TOGGLE_FLAG_271:
        g_level_block->flag_271 ^= 1;
        RequestRedraw(0x800);
        break;
    case W8_MGS_COMMAND_DEBUG_MONSTER_SCRIPT:
        if (g_level_block->highlighted_item != -1) {
            MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                0x1b9, MGSKEYBOARD_CPP, g_level_block->highlighted_item, 1));
        }
        break;
    default:
        break;
    }
}

// GLOBAL: WIZ8 0x0064c1cc
const int g_keyboard_row_positions_64c1cc[13][2] = {
    {5, 55}, {23, 55}, {41, 55}, {59, 55}, {59, 37}, {5, 37},  {5, 19},
    {5, 1},  {23, 1},  {41, 1},  {59, 1},  {59, 19}, {32, 28},
};

// GLOBAL: WIZ8 0x0069b7ec
short g_keyboard_menu_items_69b7ec[12];
// GLOBAL: WIZ8 0x0069b804
Controls* g_keyboard_menu_panel_69b804;
// GLOBAL: WIZ8 0x0069b808
short g_keyboard_menu_pages_69b808[12];
// GLOBAL: WIZ8 0x0069b820
W8TextControl* g_keyboard_menu_rows_69b820[13];

void KeyboardMenuSelectAttack(void);
void KeyboardMenuSelectBerserk(void);
void KeyboardMenuSelectBreathe(void);
void KeyboardMenuSelectPray(void);
void KeyboardMenuSelectTurnUndead(void);
void KeyboardMenuSelectDefend(void);
void KeyboardMenuSelectProtect(void);
void KeyboardMenuOpenUseItemView(void);
void KeyboardMenuSelectEquip(void);
void KeyboardMenuOpenSpellView(void);
void KeyboardMenuCastRecordedSpell(void);
void KeyboardMenuUseRecordedItem(void);

// FUNCTION: WIZ8 0x00592C70
void OpenKeyboardMenuForSlot(int slot)
{
    memset(g_keyboard_menu_rows_69b820, 0, sizeof(g_keyboard_menu_rows_69b820));
    g_value_64c1c8 = slot;
    g_keyboard_menu_panel_69b804 = 0;
    g_level_block->keyboard_menu_open = 1;
    SelectPartyCharacter(g_value_64c1c8);
    if (BuildKeyboardMenu() == 0) {
        CloseKeyboardMenu();
        return;
    }
    UpdateScreenOverlays(0);
    gXStatus.monster_manager_entries[slot].keyboard_menu_open = 1;
    RegionSetDisable(slot + 7);
    DisableRegionSetInput(slot + 7);
    DisableRegionInput(slot + 0x5a);
    DisableRegionInput(slot + 0xa);
    EnableKeyboardMenuInput();
    RequestRedraw(1 << slot);
}

// FUNCTION: WIZ8 0x00592E60
__forceinline void CloseKeyboardMenu(void)
{
    int index;

    gXStatus.monster_manager_entries[g_value_64c1c8].keyboard_menu_open = 0;
    g_level_block->keyboard_menu_open = 0;
    g_level_block->combat_slot = -1;
    g_level_block->hover_combat_slot = g_value_64c1c8;
    g_level_block->flag_31c = 0;
    RefreshPartySlotRegions();
    if (gXStatus.fCombatMode != 0) {
        EnableMainRegionSet();
    } else {
        DisableMainRegionSet();
    }
    RegionSetDisable(0x26);
    DisableRegionSetInput(0x26);
    if (g_level_block->portrait_refresh_pending[g_value_64c1c8] == 0) {
        ClearSurfaceRect(g_keyboard_menu_panel_69b804->origin_x,
                         g_keyboard_menu_panel_69b804->origin_y,
                         g_keyboard_menu_panel_69b804->origin_x + 0x52,
                         g_keyboard_menu_panel_69b804->origin_y + 0x4a);
        InvalidateRegion(g_keyboard_menu_panel_69b804->origin_x,
                         g_keyboard_menu_panel_69b804->origin_y,
                         g_keyboard_menu_panel_69b804->origin_x + 0x52,
                         g_keyboard_menu_panel_69b804->origin_y + 0x4a, 0);
    }
    if (g_keyboard_menu_panel_69b804 != 0) {
        delete g_keyboard_menu_panel_69b804;
        g_keyboard_menu_panel_69b804 = 0;
    }
    for (index = 0; index < 13; ++index) {
        if (g_keyboard_menu_rows_69b820[index] != 0) {
            delete g_keyboard_menu_rows_69b820[index];
            g_keyboard_menu_rows_69b820[index] = 0;
        }
    }
    RequestRedraw(1 << g_value_64c1c8);
}

// FUNCTION: WIZ8 0x00592F90
unsigned char BuildKeyboardMenu(void)
{
    int left;
    int top;
    int unused_4;
    int unused_5;
    int unused_6;
    int unused_7;
    short menu;
    short item;
    short row;
    W8TextControl* control;
    short message;
    int index;

    GetPartySlotMenuAnchor(g_value_64c1c8, &left, &top, &unused_4, &unused_5, &unused_6, &unused_7,
                           0);
    left += 0x17;
    g_keyboard_menu_panel_69b804 = new Controls(left, top, left + 0x52, top + 0x4a, 0xa6, 0, 0);
    if (g_keyboard_menu_panel_69b804 == 0) {
        return 0;
    }
    SetRegionBounds(0xc1, left, top, left + 0x52, top + 0x4a);
    row = 0;
    for (menu = 0; menu < 4; ++menu) {
        for (item = 0; item < 5; ++item) {
            message = g_submenu_entry_message_ids_64c548[menu * 5 + item];
            if (message == -1) {
                continue;
            }
            control = new W8TextControl(g_keyboard_menu_panel_69b804, row + 0xb4,
                                        g_keyboard_row_positions_64c1cc[row][0],
                                        g_keyboard_row_positions_64c1cc[row][1],
                                        g_keyboard_row_positions_64c1cc[row][0] + 0x12,
                                        g_keyboard_row_positions_64c1cc[row][1] + 0x12, 0x89, 0, -1,
                                        -1, -1, -1, message + 6);
            g_keyboard_menu_rows_69b820[row] = control;
            if (control == 0) {
                if (g_keyboard_menu_panel_69b804 != 0) {
                    delete g_keyboard_menu_panel_69b804;
                    g_keyboard_menu_panel_69b804 = 0;
                }
                for (index = 0; index < 13; ++index) {
                    if (g_keyboard_menu_rows_69b820[index] != 0) {
                        delete g_keyboard_menu_rows_69b820[index];
                        g_keyboard_menu_rows_69b820[index] = 0;
                    }
                }
                return 0;
            }
            AssignKeyboardMenuCallback(menu, item, control);
            control->m_secondaryActivationCallback = CloseKeyboardMenu;
            g_keyboard_menu_pages_69b808[row] = menu;
            g_keyboard_menu_items_69b7ec[row] = item;
            SetRegionHelp(row + 0xb4, 1, g_submenu_entry_help_ids_64c57c[menu * 5 + item]);
            ++row;
        }
    }
    g_keyboard_menu_rows_69b820[row] = new W8TextControl(
        g_keyboard_menu_panel_69b804, row + 0xb4, g_keyboard_row_positions_64c1cc[row][0],
        g_keyboard_row_positions_64c1cc[row][1], g_keyboard_row_positions_64c1cc[row][0] + 0x12,
        g_keyboard_row_positions_64c1cc[row][1] + 0x12, 0x89, 0, 0xbd, 0xbd, 0xbf, 0xbf, -1);
    g_keyboard_menu_rows_69b820[row]->m_primaryActivationCallback = CloseKeyboardMenu;
    g_keyboard_menu_rows_69b820[row]->m_secondaryActivationCallback = CloseKeyboardMenu;
    SetRegionHelp(row + 0xb4, 1, 0x11);
    g_keyboard_menu_panel_69b804->SetEnabled(1);
    RefreshKeyboardMenuRows();
    return 1;
}

// FUNCTION: WIZ8 0x005932D0
void EnableKeyboardMenuInput(void)
{
    int index;

    RegionSetEnable(0x26);
    EnableRegionInput(0xc1);
    for (index = 0; index < 13; ++index) {
        EnableRegionInput(index + 0xb4);
    }
}

// FUNCTION: WIZ8 0x00593300
bool KeyboardMenuContainsCursor(void)
{
    return IsCursorInRectangle(g_keyboard_menu_panel_69b804->origin_x,
                               g_keyboard_menu_panel_69b804->origin_y,
                               g_keyboard_menu_panel_69b804->origin_x + 0x52,
                               g_keyboard_menu_panel_69b804->origin_y + 0x4a);
}

// FUNCTION: WIZ8 0x00593360
void RefreshKeyboardMenuRows(void)
{
    int remaining;
    short item;
    short menu;
    short message;
    short icon;
    W8TextControl* row;
    int adjust;
    int index;

    index = 0;
    remaining = 12;
    row = g_keyboard_menu_rows_69b820[0];
    do {
        item = g_keyboard_menu_items_69b7ec[index];
        menu = g_keyboard_menu_pages_69b808[index];
        message = g_submenu_entry_message_ids_64c548[menu * 5 + item];
        switch (GetSubMenuEntryState(menu, item, g_value_64c1c8)) {
        case W8_SUBMENU_ENTRY_USABLE:
            icon = message + 2;
            break;
        case W8_SUBMENU_ENTRY_USABLE_SELECTED:
            message += 1;
            icon = message + 1;
            break;
        case W8_SUBMENU_ENTRY_UNUSABLE:
            message += 3;
            icon = message + 2;
            break;
        case W8_SUBMENU_ENTRY_UNUSABLE_SELECTED:
            message += 4;
            icon = message + 1;
            break;
        case W8_SUBMENU_ENTRY_UNAVAILABLE:
            message = -1;
            break;
        }
        if (message == -1) {
            if (menu == W8_SUBMENU_SPELLS && item == 1) {
                icon += g_spell_records[g_status_685170.buffers.party_rows[g_value_64c1c8].spell_id]
                            .realm *
                        7;
            }
            row->SetEnabled(0);
        } else {
            if (menu == W8_SUBMENU_SPELLS && item == 1) {
                adjust =
                    g_spell_records[g_status_685170.buffers.party_rows[g_value_64c1c8].spell_id]
                        .realm *
                    7;
                message += adjust;
                icon += adjust;
            } else if (menu == W8_SUBMENU_ATTACK && item == 0) {
                switch (g_status_685170.buffers.characters[g_value_64c1c8]
                            .hand_attacks[0]
                            .weapon_skill) {
                case 1:
                    adjust = 4;
                    break;
                case 2:
                    adjust = 7;
                    break;
                case 3:
                    adjust = 3;
                    break;
                case 5:
                    adjust = 6;
                    break;
                case 7:
                    adjust = 8;
                    break;
                case 8:
                    adjust = 1;
                    break;
                case 9:
                    adjust = 2;
                    break;
                case 14:
                    adjust = 5;
                    break;
                default:
                    adjust = 0;
                    break;
                }
                adjust *= 7;
                message += adjust;
                icon += adjust;
            }
            row->m_normalSprite = message;
            row->m_pressedSprite = message;
            row->m_alternateNormalSprite = icon;
            row->m_alternatePressedSprite = icon;
            row->SetActive(1);
        }
        row->Invalidate(0);
        ++index;
        row = g_keyboard_menu_rows_69b820[index];
        --remaining;
    } while (remaining != 0);
}

// FUNCTION: WIZ8 0x005935E0
void AssignKeyboardMenuCallback(short menu, short item, W8TextControl* row)
{
    switch (menu) {
    case W8_SUBMENU_ATTACK:
        switch (item) {
        case 0:
            row->m_primaryActivationCallback = KeyboardMenuSelectAttack;
            break;
        case 1:
            row->m_primaryActivationCallback = KeyboardMenuSelectBerserk;
            break;
        case 2:
            row->m_primaryActivationCallback = KeyboardMenuSelectBreathe;
            break;
        case 3:
            row->m_primaryActivationCallback = KeyboardMenuSelectTurnUndead;
            break;
        case 4:
            row->m_primaryActivationCallback = KeyboardMenuSelectPray;
            break;
        }
        break;
    case W8_SUBMENU_DEFEND:
        if (item == 0) {
            row->m_primaryActivationCallback = KeyboardMenuSelectDefend;
        } else if (item == 1) {
            row->m_primaryActivationCallback = KeyboardMenuSelectProtect;
        }
        break;
    case W8_SUBMENU_ITEMS:
        if (item == 0) {
            row->m_primaryActivationCallback = KeyboardMenuSelectEquip;
        } else if (item == 1) {
            row->m_primaryActivationCallback = KeyboardMenuOpenUseItemView;
        } else if (item == 2) {
            row->m_primaryActivationCallback = KeyboardMenuUseRecordedItem;
        }
        break;
    case W8_SUBMENU_SPELLS:
        if (item == 0) {
            row->m_primaryActivationCallback = KeyboardMenuOpenSpellView;
        } else if (item == 1) {
            row->m_primaryActivationCallback = KeyboardMenuCastRecordedSpell;
        }
        break;
    }
}

// FUNCTION: WIZ8 0x005936F0
void RedrawKeyboardMenuPanel(unsigned char invalidate)
{
    if (invalidate != 0) {
        g_keyboard_menu_panel_69b804->Invalidate(0);
    }
    g_keyboard_menu_panel_69b804->Redraw();
}

// FUNCTION: WIZ8 0x00593710
void KeyboardMenuSelectAttack(void)
{

    ChooseAction(g_value_64c1c8, W8_ACTION_ATTACK, -1, 0, 0, 1);
    RequestRedraw(0x200000);
    CloseKeyboardMenu();
}

// FUNCTION: WIZ8 0x00593860
void KeyboardMenuSelectBerserk(void)
{

    ChooseAction(g_value_64c1c8, W8_ACTION_BERSERK, -1, 0, 0, 1);
    RequestRedraw(0x200000);
    CloseKeyboardMenu();
}

// FUNCTION: WIZ8 0x005939B0
void KeyboardMenuSelectBreathe(void)
{

    ChooseAction(g_value_64c1c8, W8_ACTION_BREATHE, -1, 0, 0, 1);
    RequestRedraw(0x200000);
    CloseKeyboardMenu();
}

// FUNCTION: WIZ8 0x00593B00
void KeyboardMenuSelectTurnUndead(void)
{

    ChooseAction(g_value_64c1c8, W8_ACTION_TURN_UNDEAD, -1, 0, 0, 1);
    RequestRedraw(0x200000);
    CloseKeyboardMenu();
}

// FUNCTION: WIZ8 0x00593C50
void KeyboardMenuSelectPray(void)
{

    ChooseAction(g_value_64c1c8, W8_ACTION_PRAY, -1, 0, 0, 1);
    RequestRedraw(0x200000);
    CloseKeyboardMenu();
}

// FUNCTION: WIZ8 0x00593DA0
void KeyboardMenuSelectDefend(void)
{

    ChooseAction(g_value_64c1c8, W8_ACTION_DEFEND, -1, 0, 0, 1);
    RequestRedraw(0x200000);
    CloseKeyboardMenu();
}

// FUNCTION: WIZ8 0x00593EF0
void KeyboardMenuSelectProtect(void)
{

    ChooseAction(g_value_64c1c8, W8_ACTION_PROTECT, -1, 0, 0, 1);
    RequestRedraw(0x200000);
    CloseKeyboardMenu();
}

// FUNCTION: WIZ8 0x00594040
void KeyboardMenuOpenSpellView(void)
{

    CloseKeyboardMenu();
    if (gXStatus.fSpellCastMode == 0) {
        OpenSpellCastingView(g_value_64c1c8);
    }
}

// FUNCTION: WIZ8 0x00594180
void KeyboardMenuCastRecordedSpell(void)
{

    CloseKeyboardMenu();
    if (CanPartySlotCastRecordedSpell(g_value_64c1c8) != 0) {
        StartCharacterSpellCast(g_value_64c1c8, 0);
        RequestRedraw(0x200000);
    }
}

// FUNCTION: WIZ8 0x00594390
void KeyboardMenuSelectEquip(void)
{

    ChooseAction(g_value_64c1c8, W8_ACTION_EQUIP, -1, 0, 0, 1);
    RequestRedraw(0x200000);
    CloseKeyboardMenu();
}

// FUNCTION: WIZ8 0x005944E0
void KeyboardMenuOpenUseItemView(void)
{

    CloseKeyboardMenu();
    if (gXStatus.fItemSelectMode == 0) {
        OpenUseItemSelectView(g_value_64c1c8);
    }
}

// FUNCTION: WIZ8 0x00594620
void KeyboardMenuUseRecordedItem(void)
{

    CloseKeyboardMenu();
    StartCharacterItemUse(g_value_64c1c8);
    RequestRedraw(0x200000);
}

// FUNCTION: WIZ8 0x00594760
unsigned char KeyboardMenuRowRegionEvent(const InputAtom* event, W8Region* region)
{
    W8TextControl* row = g_keyboard_menu_rows_69b820[region->callback_id];
    unsigned short row_id;
    W8PartySlotRow* party_row;
    W8ItemInstance* item;
    wchar_t* name;
    int power;

    if (row == 0) {
        return 0;
    }
    switch (event->usEvent) {
    case RIGHT_BUTTON_DOWN:
        row->OnRightButtonDown(0);
        region->flags |= W8_REGION_RIGHT_BUTTON_HELD;
        return 1;
    case LEFT_BUTTON_DOWN:
    case LEFT_BUTTON_REPEAT:
        row->OnLeftButtonDown(0);
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        return 1;
    case LEFT_BUTTON_UP:
        row->OnLeftButtonUp(0);
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) == 0) {
            return 1;
        }
        region->flags &= ~W8_REGION_LEFT_BUTTON_HELD;
        return 1;
    case RIGHT_BUTTON_UP:
        row->OnRightButtonUp(0);
        if ((region->flags & W8_REGION_RIGHT_BUTTON_HELD) != 0) {
            region->flags &= ~W8_REGION_RIGHT_BUTTON_HELD;
        }
        return 1;
    case MOUSE_POS:
        if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
            row->OnMouseLeave(0);
            return 1;
        }
        if ((region->flags & W8_REGION_MOUSE_ENTER) == 0) {
            return 0;
        }
        row->OnMouseEnter(0);
        row_id = region->callback_id;
        if (g_keyboard_menu_pages_69b808[row_id] == W8_SUBMENU_SPELLS &&
            g_keyboard_menu_items_69b7ec[row_id] == 1) {
            SetRegionHelpForceEnabled004F27C0(1);
            if (g_keyboard_menu_rows_69b820[region->callback_id]->m_enabled == 0) {
                SetRegionHelpText(gppStringList[0x168 / 4]);
                return 1;
            }
            name = g_spell_records[g_status_685170.buffers.party_rows[g_value_64c1c8].spell_id]
                       .display_name;
            power = GetAffordableSpellPowerLevel(g_value_64c1c8);
            SetRegionHelpText(FormatWideString(g_format_s_colon_s_paren_d_006481b4,
                                               gppStringList[0x168 / 4], name, power));
            return 1;
        }
        if (g_keyboard_menu_pages_69b808[row_id] != W8_SUBMENU_ITEMS) {
            return 1;
        }
        if (g_keyboard_menu_items_69b7ec[row_id] != 2) {
            return 1;
        }
        SetRegionHelpForceEnabled004F27C0(1);
        if (g_keyboard_menu_rows_69b820[region->callback_id]->m_enabled == 0) {
            SetRegionHelpText(gppStringList[0x174 / 4]);
            return 1;
        }
        party_row = &g_status_685170.buffers.party_rows[g_value_64c1c8];
        item = FindCharacterItemAt(g_value_64c1c8, party_row->item_origin, party_row->item_slot);
        name = FormatItemDisplayName(item, 0);
        SetRegionHelpText(
            FormatWideString(g_format_s_colon_s_0061c3e0, gppStringList[0x174 / 4], name));
        return 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x005949A0
unsigned char KeyboardMenuBackgroundRegionEvent(const InputAtom* event, W8Region*)
{
    if (g_level_block->keyboard_menu_open == 0) {
        return 0;
    }
    if (event->usEvent != RIGHT_BUTTON_UP) {
        return 0;
    }
    CloseKeyboardMenu();
    return 1;
}
