#include "line.h"
#include "wiz8/integer_constants.h"
#include "wiz8/local_screens/MGSFormation.h"
#include "wiz8/local_screens/MGSPortraits.h"
#include "soundman.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/Search.h"
#include "wiz8/local_code/Sight.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/local_code/FormationAndFacing.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/npc_interaction.h"
#include "wiz8/text_input.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/engine_code/Level.h"
#include "wiz8/local_screens/MGSSpellCasting.h"
#include "wiz8/local_screens/MGSButtons.h"
#include "wiz8/local_screens/AutomapScreen.h"
#include "wiz8/local_screens/mipe.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/MGSPortraitCombat.h"
#include "wiz8/local_screens/NPCInteractionSubscreen.h"
#include "wiz8/local_screens/MGSRadarMap.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/layouts/npc_state.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/npc_script_file.h"
#include "wiz8/npc_items.h"
#include "wiz8/item_video_object_vector.h"
#include "wiz8/dialog_code/DialogFactoryDialogs.h"
#include "wiz8/layouts/item_instance.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/fact_state.h"
#include "wiz8/local_screens/MGSKeyboard.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/engine_code/3dapi.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/engine_code/stTextureAnim.h"
#include "vobject.h"
#include "wiz8/local_screens/RCSCommon.h"
#include "wiz8/local_code/LoadSaveGame.h"
#include "wiz8/local_code/MonsterAI.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/dialog_code/MessageDialogBase.h"
#include "wiz8/dialog_code/NpcDialog.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/cursor.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/engine_code/3d.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayTime.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/music_playlist.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/notices.h"
#include "wiz8/regions.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/fonts.h"
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_screens/MGSUseItemSelect.h"
#include "wiz8/local_screens/RCSItemsPage.h"
#include "wiz8/dialog_code/AssayDialog.h"
#include "wiz8/dialog_code/PortraitQuote.h"
#include "wiz8/character_event_queue.h"
#include "wiz8/xstatus.h"
#include "wiz8/wiz8_windows.h"
#include "wiz8/world_cursor.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "font.h"
#include "FileMan.h"
#include "input.h"
#include "timer.h"
#include "Types.h"
#include "mousesystem.h"
#include "mousesystem_macros.h"
#include "surrender/srTypeRegistry.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/3dapi.h"
#include "wiz8/local_code/ItemManager.h"
#include "wiz8/item_spawning.h"
#include "wiz8/local_screens/MGSPartyMovement.h"
#include "wiz8/engine_code/Cursor3d.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/local_screens/MGSPortraits.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/string_database.h"
#include "wiz8/local_screens/MGSSpellIcons.h"
#include "wiz8/local_screens/JournalScreen.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/layouts/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/float_constants.h"
#include "wiz8/monster_generators.h"
#include "wiz8/local_code/Traps.h"
#include "wiz8/local_code/ButtonSound.h"
#include "wiz8/local_code/GameplayInit.h"
#include "vobject_blitters.h"
#include "random.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_screens/OptionsScreen.h"

// GLOBAL: WIZ8 0x0068ee90
static W8NpcInteractionState g_npc_interaction_storage;
/* The global object's implicit constructor and destructor; 0x0056B930 is the
   retail static initializer that runs the former and registers the latter. */

// GLOBAL: WIZ8 0x00649f1c
W8NpcInteractionState* g_npc_interaction_state = &g_npc_interaction_storage;
/* 0x0068EE80: the dialogue keyword tables. Element zero is the English file
   list and element one the translated one; each file list holds one line list
   per line and each line list one word per field. */
// GLOBAL: WIZ8 0x0068EE80
W8GrowableVector<W8GrowableVector<W8GrowableVector<wchar_t*>*>*> g_keyword_lists;
/* 0x0068F0F8: both keyword files are loaded and the tables are usable. */
// GLOBAL: WIZ8 0x0068F0F8
bool g_keyword_lists_loaded;
/* 0x0068F0F9: the keyword subsystem's active flag, written absolutely by the
   screen reset and by the keyword panel helpers. */
// GLOBAL: WIZ8 0x0068F0F9
bool g_pending_notice_queued;
/* 0x0068EE58: empty wide string used to clear dialogue editor text. */
// GLOBAL: WIZ8 0x0068EE58
wchar_t g_dialogue_empty_text[4];
/* 0x0068EE60: the queued NPC script notice; see the type comment in the
   header. */
// GLOBAL: WIZ8 0x0068EE60
W8PendingNotice g_pending_notice;
/* 0x0068EE78: GetTickCount sample for the trade-item highlight timeout. */
// GLOBAL: WIZ8 0x0068EE78
static unsigned int g_trade_highlight_tick;

// GLOBAL: WIZ8 0x00649f20
static int g_dialogue_place_keyword_count = 15;
// GLOBAL: WIZ8 0x00649f24
static int g_dialogue_place_keyword_ids[15] = {0x751, 0x752, 0x753, 0x754, 0x755,
                                               0x756, 0x757, 0x758, 0x759, 0x75a,
                                               0x75b, 0x75c, 0x75d, 0x75e, 0x75f};
// GLOBAL: WIZ8 0x00649F64
static int g_dialogue_fallback_ids0[5] = {0x760, 0x761, 0x762, 0x763, 0x764};
// GLOBAL: WIZ8 0x00649F78
static int g_dialogue_fallback_ids1[5] = {0x765, 0x766, 0x767, 0x768, 0x769};
// GLOBAL: WIZ8 0x00649f8c
const wchar_t* g_dialogue_person_keywords[] = {L"BALBRAK", L"BILDUBLU", L"EWAXX",  L"KUNAR",
                                               L"PANRACK", L"RODAN",    L"RUBBLE", L"SAXX",
                                               L"SPARKLE", L"YAMIR",    L""};

/* Enabling starts text-input scheme 1 and installs the typed-dialogue field;
   disabling removes it. An already-enabled panel does none of this. */
// FUNCTION: WIZ8 0x0056BAC0
void W8NpcTypedDialoguePanel::SetEnabled(bool enable)
{
    if (!enable || !m_fEnabled) {
        Controls::SetEnabled(enable);
        if (enable) {
            InitTextInputModeWithScheme(1);
            AddTextInputField(0x1e5, 0x170, 0x7a, 0x12, 0x7f, &g_empty_wide_string, 0xbe, 0xf, 1);
        } else {
            RemoveTextInputField(0);
        }
    }
}

/* Base redraw plus the input-frame image: drawn at y 0x19b while where_is_query is
   raised, else 0x18b. */
// FUNCTION: WIZ8 0x0056BB20
void W8NpcTypedDialoguePanel::Redraw()
{
    int redrawn = 0;

    if (!m_fEnabled) {
        return;
    }
    if (m_fDirty) {
        if (m_renderTarget != -1) {
            DrawCatalogImage(-14, m_renderTarget, m_renderArg0, m_renderArg1, m_bounds.left,
                             m_bounds.top, 2, 0);
        }
        DrawCatalogImage(-14, 0x1a9, 0, 0x10, 0x1df,
                         g_npc_interaction_state->where_is_query ? 0x19b : 0x18b, 2, 0);
        redrawn = 1;
    } else if (!m_fLayoutDirty) {
        return;
    }
    RedrawControls(redrawn != 0);
}
/* Unlike the base, the six option buttons stay inactive while the expanded
   NPC dialogue layout (dialogue_layout == W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX) is not up. */
// FUNCTION: WIZ8 0x0056BC50
void W8NpcDialogueOptionsPanel::SetEnabled(bool enable)
{
    int index;

    m_fEnabled = enable;
    for (index = 0; index < m_controls.count; ++index) {
        if (enable &&
            (ControlAt(index) == static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 0]) ||
             ControlAt(index) == static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 1]) ||
             ControlAt(index) == static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 2]) ||
             ControlAt(index) == static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 3]) ||
             ControlAt(index) == static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 4]) ||
             ControlAt(index) == static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 5])) &&
            g_npc_interaction_state->dialogue_layout != W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX) {
            continue;
        }
        ControlAt(index)->SetActive(enable);
    }
}

/* Base redraw except the foreground catalog image is m_main_text_box_image rather than
   m_renderArg1 while the expanded dialogue layout is up. */
// FUNCTION: WIZ8 0x0056BD30
void W8NpcDialogueOptionsPanel::Redraw()
{
    int redrawn = 0;

    if (!m_fEnabled) {
        return;
    }
    if (m_fDirty) {
        if (m_renderTarget != -1) {
            DrawCatalogImage(-14, m_renderTarget, m_renderArg0,
                             g_npc_interaction_state->dialogue_layout ==
                                     W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX
                                 ? m_main_text_box_image
                                 : m_renderArg1,
                             m_bounds.left, m_bounds.top, 2, 0);
        }
        redrawn = 1;
    } else if (!m_fLayoutDirty) {
        return;
    }
    RedrawControls(redrawn != 0);
}

/* Copy the next '/'-terminated field of a keyword line into the caller's
   buffer, trimming the leading and trailing spaces and stopping at a newline
   or at the end of the line. A field reached at a slash position, or a line
   that ends before any field, answers null; otherwise the returned cursor
   sits past the terminating slash so the next call continues the line. */
// FUNCTION: WIZ8 0x0056be40
wchar_t* ParseKeywordToken(wchar_t* line, wchar_t* field)
{
    wchar_t* cursor = line;
    wchar_t* out = field;
    int length = 0;

    *out = 0;
    while (*cursor == L' ' && *cursor != 0) {
        ++cursor;
    }
    if (*cursor == L'/') {
        return 0;
    }
    while (*cursor != 0 && *cursor != L'\n' && *cursor != L'\r') {
        *out = *cursor;
        ++cursor;
        ++length;
        ++out;
        if (*cursor == L'/') {
            break;
        }
    }
    if (length == 0) {
        return 0;
    }
    while (field[length - 1] == L' ') {
        --length;
        if (length < 1) {
            return 0;
        }
    }
    field[length] = 0;
    if (*cursor == L'/') {
        ++cursor;
    }
    return cursor;
}

/* Load one keyword file into a file list: a fresh line list per line and a
   malloc'd wide copy of every '/'-separated field. The first line is read only
   to prime the end-of-file test, and parsing starts eleven wide characters
   into every line - retail's own offset, whose prefix meaning is not
   resolved. A file that cannot be opened answers zero; otherwise every line
   adds a list, an empty one included, and the loader answers one. */
// FUNCTION: WIZ8 0x0056bed0
unsigned char LoadKeywordFile(const char* path, W8GrowableVector<W8GrowableVector<wchar_t*>*>* file)
{
    wchar_t line[1000];
    wchar_t field[1000];
    W8GrowableVector<wchar_t*>* entry;
    wchar_t* cursor;
    wchar_t* word;
    FILE* stream;
    size_t length;

    stream = fopen(path, "rb");
    if (stream == 0) {
        return 0;
    }
    memset(line, 0, sizeof(line));
    fgetws(line, 1000, stream);
    while (!feof(stream)) {
        memset(line, 0, sizeof(line));
        fgetws(line, 1000, stream);
        entry = new W8GrowableVector<wchar_t*>;
        cursor = line + 11;
        while ((cursor = ParseKeywordToken(cursor, field)) != 0) {
            length = wcslen(field);
            word = static_cast<wchar_t*>(malloc(length * 2 + 2));
            wcscpy(word, field);
            entry->Add(word);
        }
        file->Add(entry);
    }
    fclose(stream);
    return 1;
}

/* Release every keyword file list: its words through free, each line list and
   each file list through its deleting destructor. The loaded flag is lowered
   either way and the outer count is cleared. */
// FUNCTION: WIZ8 0x0056c130
void ClearKeywordLists(void)
{
    W8GrowableVector<W8GrowableVector<wchar_t*>*>* file;
    W8GrowableVector<wchar_t*>* entry;
    int file_index;
    int entry_index;
    int word_index;

    for (file_index = 0; file_index < g_keyword_lists.count; ++file_index) {
        file = *g_keyword_lists.GetAt(file_index);
        for (entry_index = 0; entry_index < file->count; ++entry_index) {
            entry = *file->GetAt(entry_index);
            for (word_index = 0; word_index < entry->count; ++word_index) {
                free(*entry->GetAt(word_index));
            }
            entry->Clear();
            delete entry;
        }
        delete file;
    }
    g_keyword_lists.Clear();
    g_keyword_lists_loaded = false;
}

/* Replace the keyword tables: release the current pair, then load the English
   file into element zero and the translated file into element one. A failed
   first load leaves an empty table; a failed second load releases the first
   list again. Only a complete pair raises the loaded flag. */
// FUNCTION: WIZ8 0x0056c200
void ReloadKeywordLists(void)
{
    W8GrowableVector<W8GrowableVector<wchar_t*>*>* english;
    W8GrowableVector<W8GrowableVector<wchar_t*>*>* translated;

    ClearKeywordLists();
    english = new W8GrowableVector<W8GrowableVector<wchar_t*>*>;
    if (!LoadKeywordFile("Data\\Strings\\English_Keywords.txt", english)) {
        return;
    }
    g_keyword_lists.Add(english);
    translated = new W8GrowableVector<W8GrowableVector<wchar_t*>*>;
    if (!LoadKeywordFile("Data\\Strings\\translated_Keywords.txt", translated)) {
        ClearKeywordLists();
        return;
    }
    g_keyword_lists.Add(translated);
    g_keyword_lists_loaded = true;
}

/* Translate a typed dialogue keyword through the loaded tables. With no
   tables loaded the input passes through verbatim; otherwise the active
   language's list is scanned and the matching English field is copied out.
   Element one holds the translated file when two loaded, falling back to the
   English list through GetAt's clamped read. */
// FUNCTION: WIZ8 0x0056c440
void TranslateDialogueKeyword(const wchar_t* source, wchar_t* destination)
{
    W8GrowableVector<W8GrowableVector<wchar_t*>*>* file;
    W8GrowableVector<wchar_t*>* entry;
    W8GrowableVector<wchar_t*>* english;
    int entry_index;
    int word_index;

    if (!g_keyword_lists_loaded) {
        wcscpy(destination, source);
        return;
    }
    file = *g_keyword_lists.GetAt(1);
    for (entry_index = 0; entry_index < file->count; ++entry_index) {
        entry = *file->GetAt(entry_index);
        for (word_index = 0; word_index < entry->count; ++word_index) {
            if (CompareWideTextIgnoreAsciiCase(source, *entry->GetAt(word_index)) == 0) {
                english = *(*g_keyword_lists.GetAt(0))->GetAt(entry_index);
                wcscpy(destination, *english->GetAt(word_index));
                return;
            }
        }
    }
}

/* Reset the screen state block: zero its 0x268 bytes, write its reset values,
   clear the keyword status byte, and reload the keyword lists. */
// FUNCTION: WIZ8 0x0056c520
void ResetMainScreenStateBlock(void)
{
    int unset = -1;

    memset(g_npc_interaction_state, 0, sizeof(W8NpcInteractionState));
    g_npc_interaction_state->dialogue_category_filter = unset;
    g_npc_interaction_state->transcript_sorted = 0;
    g_npc_interaction_state->pending_trade_toggle = 0;
    g_npc_interaction_state->selected_trade_row = unset;
    g_npc_interaction_state->trade_pc_items = 1;
    g_status.selected_party_member = 0xff;
    g_pending_notice_queued = false;
    ReloadKeywordLists();
}

/* Forward a monster-script notice to the targeting layer unless the screen is
   busy or this NPC kind suppresses it. */
// FUNCTION: WIZ8 0x0056C590
void ForwardNpcScriptNotice(W8NpcState* npc, W8ItemInstance* item, int line, bool suppress)
{
    if (!gXStatus.fNpcDialogueMode && !gXStatus.fCombatMode &&
        (npc->record->kind != 7 || GetFact(W8_FACT_ARNIKA_MYLES_MEET_ONCE) != 1)) {
        QueueNpcScriptNotice(npc, item, line, suppress, 0);
    }
}

/* Queue one NPC script notice for the dispatch pass. A second NPC of the same
   kind already in the world suppresses the notice unless the record carries
   the 0x054 binding flag, a live monster with a condition at or above 0xf
   takes none, and a pending notice blocks the next until it drains. Kind
   0x10/0x11 NPCs with fact 0xbf substitute their own notice line and raise
   the flag byte. */
// FUNCTION: WIZ8 0x0056C5E0
void QueueNpcScriptNotice(W8NpcState* npc, W8ItemInstance* item, int line, bool suppress,
                          unsigned char arg)
{
    W8MonsterInfo* info;
    bool flag;

    if (FindNpcOfKind(npc->name_style) != 0 && npc->record->monster_bound == 0) {
        return;
    }
    info = GetNpcMonsterInfo(npc);
    if (info != 0 && info->highest_condition >= W8_CONDITION_ASLEEP) {
        return;
    }
    flag = suppress;
    if ((npc->name_style == W8_NPC_DRAZIC || npc->name_style == W8_NPC_RODAN) &&
        GetFact(W8_FACT_PEACE_ACHIEVED) != 0) {
        flag = 1;
        line = npc->name_style == W8_NPC_DRAZIC ? 0x23 : 0x1d;
    }
    if (g_pending_notice_queued) {
        return;
    }
    g_pending_notice.flag = flag;
    g_pending_notice.npc = npc;
    g_pending_notice.line = line;
    g_pending_notice.force = arg;
    if (item != 0) {
        g_pending_notice.item = *item;
    } else {
        EmptyItemRecord(&g_pending_notice.item, 0, 1);
    }
    QueueNpcMessageLine(W8_NPC_MSG_DISPATCH_PENDING_NOTICE, 0);
    g_pending_notice_queued = true;
}

/* The NPC notice and dialogue dispatcher. Talking to a healer NPC (name
   styles 0x0f and 0x12) first lifts every active condition off the two party
   rows bound to RPC NPCs (name styles 0x10 and 0x11) whose condition set has
   reached 0x0f; clearing condition 0x12 - death - leaves them on ten hit
   points. A completed character event is finished off, the dialogue NPC is
   staged outside camp mode, and then the record's 0x054/0x2ea flags choose
   between the item/quote branch and the plain quote branch; both end by
   pausing the world, raising the dialogue flags and pointing the camera at
   the NPC's monster. */
// FUNCTION: WIZ8 0x0056C6D0
void BeginNpcDialogueInternal(W8NpcState* npc, W8ItemInstance* item, int quote, unsigned char flags,
                              unsigned char force)
{
    W8NpcInteractionState* state;
    W8MonsterInfo* info;
    W8NpcState* bound;
    W8NpcState* selected;
    W8Character* characters;
    int slot;
    int condition;
    srVector3T<float> position;

    g_pending_notice_queued = false;
    g_npc_interaction_state->transcript_open_count = 0;
    if (npc->name_style == 0xf || npc->name_style == 0x12) {
        characters = g_status.buffers.Char;
        for (slot = 0; slot < 2; ++slot) {
            if (g_status.buffers.XChar[slot].fOccupied) {
                bound = GetNpcState(g_status.buffers.XChar[slot].npc_index);
                if ((bound->name_style == 0x11 || bound->name_style == 0x10) &&
                    characters[slot].highest_condition >= W8_CONDITION_ASLEEP) {
                    for (condition = 0; condition <= 0x12; ++condition) {
                        if (characters[slot].uiCondition[condition] != 0) {
                            RemoveCharacterCondition(slot, static_cast<W8Condition>(condition), 0);
                            if (condition == 0x12) {
                                characters[slot].hp_current = 10;
                            }
                        }
                    }
                }
            }
        }
    }
    if (gXStatus.character_event_queue->HasActiveEvents()) {
        gXStatus.character_event_queue->CompleteFirstActiveEvent();
    }
    if (!gXStatus.fCampMode) {
        SelectNpcDialogueSpeaker(npc, flags);
    }
    if (force != 0) {
        OpenNpcDialoguePanel(npc, item, 1);
        return;
    }
    if (npc->record->monster_bound == 0 && npc->record->voice_script == 0) {
        if (GetNpcDispositionBand(npc) == 2 && npc->record->merchant == 0) {
            QueueNpcScriptLine(0x18, 0, 0, 0);
            return;
        }
        if (flags == 0) {
            OpenNpcDialoguePanel(npc, item, 0);
            return;
        }
    }
    if (npc->record->voice_script != 0) {
        if (item != 0) {
            state = g_npc_interaction_state;
            state->pending_item = *item;
            if (g_status.item_in_cursor) {
                state->held_item_pending = 1;
            }
            HandleNpcDialogueItem(&state->pending_item);
        } else if (quote == -1) {
            HandleNpcDialogueDeparture(1);
        } else {
            QueueNpcScriptLine(quote, 0, 0, 0);
        }
    } else {
        if (quote == -1) {
            quote = 0;
        }
        QueueNpcScriptLine(quote, 0, 0, 0);
    }
    selected = g_npc_interaction_state->dialogue_npc;
    if (selected->name_style != 0x84 && selected->name_style != 0x85) {
        info = GetNpcMonsterInfo(selected);
        if (info != 0) {
            MonsterForwardReferencePosition(info->p3D, 0);
        }
    }
    PauseMainGameWorld();
    state = g_npc_interaction_state;
    state->scripted_dialogue = 1;
    gXStatus.fNpcDialogueMode = true;
    state->transcript_open_count = 0;
    info = GetNpcMonsterInfo(state->dialogue_npc);
    if (info == 0) {
        return;
    }
    position = info->p3D->movement.position;
    position.y += info->p3D->movement.height_offset;
    g_gd_camera->LookAt(&position, 0);
}

// FUNCTION: WIZ8 0x0056CA60
void BeginNpcDialogue(W8NpcState* npc, W8ItemInstance* item, int quote, unsigned char flags,
                      unsigned char force)
{
    BeginNpcDialogueInternal(npc, item, quote, flags, force);
}

/* Dispatch the queued NPC script notice: the item goes across only while it
   still carries an id. The two notice flags remain independent byte values. */
// FUNCTION: WIZ8 0x0056CA90
void DispatchPendingNpcScriptNotice(void)
{
    W8ItemInstance* item;

    item = 0;
    if (g_pending_notice.item.iItemNo != -1) {
        item = &g_pending_notice.item;
    }
    BeginNpcDialogue(g_pending_notice.npc, item, g_pending_notice.line, g_pending_notice.flag,
                     g_pending_notice.force);
}

/* Open the NPC dialogue panel. After the shared screen reset and the
   dialogue-UI build, a carried item goes through the pending-item path -
   inspecting it decides between the trade switch and a disposition check -
   while everything else falls to the force gate and then the
   dispatch: a record-0x056 NPC takes the plain quote, otherwise the
   disposition band picks the hostile or friendly entry. */
// FUNCTION: WIZ8 0x0056CAD0
unsigned char OpenNpcDialoguePanel(W8NpcState* npc, W8ItemInstance* item, bool force)
{
    W8NpcInteractionState* state;
    W8MonsterInfo* info;
    W8MonsterInfo* dialogue_info;
    unsigned char band;
    bool greet;
    wchar_t space[2];
    srVector3T<float> position;

    UpdateScreenOverlays(0);
    gXStatus.fNpcDialogueMode = true;
    CloseMainGameOverlays();
    if (npc->record->owns_stock != 0) {
        RestockNpcInventory(npc);
    }
    state = g_npc_interaction_state;
    state->dialogue_layout = W8_DIALOGUE_LAYOUT_NONE;
    if (!gXStatus.fCampMode) {
        state->previous_dialogue_layout = W8_DIALOGUE_LAYOUT_NONE;
        state->trade_mode = W8_NPC_TRADE_NONE;
    }
    state->dialogue_panel_hidden = 0;
    state->scripted_dialogue = 0;
    state->trade_item = 0;
    state->last_mouse_x = -1;
    state->last_mouse_y = -1;
    state->hovered_text_line = -1;
    state->trade_filter = 0;
    state->value_000 = 0;
    state->where_is_query = 0;
    state->held_item_pending = 0;
    state->script_busy = 0;
    state->price_check_pending = 0;
    state->price_check_skip_fact = 0;
    state->dialogue_hidden = 0;
    state->modal_dialog_open = 0;
    state->farewell_queued = false;
    state->reopen_topics = 0;
    state->trade_gold = 0;
    state->pending_layout = 0;
    state->suppress_parting_reaction = 1;
    state->selected_trade_row = -1;
    state->transcript_open_count = 0;
    if (g_settings.main_ui_mode != W8_MAIN_UI_MODE_PORTRAITS) {
        ApplyMainGameModeFlag(W8_MAIN_UI_MODE_PORTRAITS, 0);
    } else {
        SetViewportMode(GetMainGameViewportMode());
    }
    if (!gXStatus.fCampMode) {
        g_npc_interaction_state->saved_main_ui_mode = g_settings.main_ui_mode;
    }
    CreateNpcDialogueControls();
    SetRegionBounds(0x8a, 0x17, 0x166, 0x269, 0x1c2);
    g_level_block->text_box_visible = 0;
    RegionSetEnable(0x15);
    EnableRegionInput(0x52);
    EnableRegionInput(0x53);
    EnableRegionInput(0x54);
    EnableRegionInput(0x55);
    g_level_block->action_panel_visible = 1;
    gXStatus.fCampMode = false;
    greet = !force;
    if (item != 0) {
        state = g_npc_interaction_state;
        state->pending_item = *item;
        if (g_status.item_in_cursor) {
            state->held_item_pending = 1;
            ClearHeldItemDisplay();
        }
        if (npc->record->merchant != 0) {
            if (HandleNpcDialogueItem(&state->pending_item) == 0) {
                OpenNpcDialogueTranscriptLayout();
                greet = 1;
            }
        } else if (HandleNpcDialogueItem(&state->pending_item) == 0) {
            switch (g_npc_interaction_state->dialogue_layout) {
            case W8_DIALOGUE_LAYOUT_SERVICES:
                CloseNpcDialogueMode1Layout();
                break;
            case W8_DIALOGUE_LAYOUT_TOPIC_MENU:
                RegionSetDisable(0x18);
                static_cast<W8NpcDialogueOptionsPanel*>(g_npc_interaction_state->dialogue_panels[0])->SetEnabled(0);
                g_npc_interaction_state->dialogue_panels[1]->SetEnabled(0);
                g_npc_interaction_state->dialogue_panels[4]->SetEnabled(0);
                static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_38])->SetActive(0);
                static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_38])->m_textBuffer.SetText(
                    &g_empty_wide_string, g_wiz_text_bold_font);
            /* fall through */
            case W8_DIALOGUE_LAYOUT_BARE:
                SetNpcDialogueLayoutMode(W8_DIALOGUE_LAYOUT_NONE);
                break;
            case W8_DIALOGUE_LAYOUT_TRANSCRIPT:
                CloseNpcDialogueTranscriptLayout();
                break;
            case W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX:
                CloseNpcDialogueOptionLayout();
                break;
            case W8_DIALOGUE_LAYOUT_TRADE:
                CloseNpcDialogueMode5Layout();
                break;
            default:
                break;
            }
            if (GetNpcDispositionBand(g_npc_interaction_state->dialogue_npc) == 0) {
                OpenNpcDialogueTranscriptLayout();
            } else {
                ShowNpcDialogueTopicMenu();
            }
            greet = 1;
        }
    }
    if (greet) {
        if (g_npc_interaction_state->dialogue_npc->record->merchant != 0) {
            QueueNpcScriptLine(0, 0, 0, 0);
            OpenNpcDialogueTranscriptLayout();
        } else {
            band = GetNpcDispositionBand(g_npc_interaction_state->dialogue_npc);
            if (g_npc_interaction_state->dialogue_npc->name_style == W8_NPC_ZANT &&
                GetFact(W8_FACT_TRANG_YOU_ARE_BUSTED) != 0 &&
                GetFact(W8_FACT_ALIGNMENT_UMPANI) == 0) {
                SetFact(W8_FACT_TRANG_YOU_ARE_BUSTED, 0, 0);
            }
            if (band == 0) {
                HandleNpcDialogueDeparture(1);
                OpenNpcDialogueTranscriptLayout();
            } else if (band == 1) {
                QueueNpcScriptLine(2, 0, 0, 0);
                ShowNpcDialogueTopicMenu();
            }
        }
    }
    RequestRedrawCombatBar();
    RequestRedraw(W8_MAIN_REDRAW_SUBMENU_BUTTONS);
    if (g_mouselook_active) {
        EnableCursorScene();
        g_mouselook_active = 0;
        gfTrackMousePos = 0;
    }
    info = GetNpcMonsterInfo(npc);
    if (info != 0) {
        g_npc_interaction_state->camera_redirected = 1;
        g_npc_interaction_state->saved_camera_pitch = g_gd_camera->m_pitch;
        g_npc_interaction_state->saved_camera_yaw = g_gd_camera->m_yaw;
        dialogue_info = GetNpcMonsterInfo(g_npc_interaction_state->dialogue_npc);
        if (dialogue_info != 0) {
            position = dialogue_info->p3D->movement.position;
            position.y += dialogue_info->p3D->movement.height_offset;
            g_gd_camera->LookAt(&position, 0);
        }
        MonsterForwardReferencePosition(info->p3D, 0);
    }
    PauseMainGameWorld();
    RequestRedraw(W8_MAIN_REDRAW_LAYOUT);
    g_npc_interaction_state->text_box_collapsed = true;
    swprintf(space, L" ");
    ShowNotice(5, space, 3, -1, 0);
    return 1;
}

/* Stage `npc` as the dialogue NPC: bind its monster's location, clear its
   transient flags, kick the script dialogue, refresh the name caption while
   the dialogue UI is already up, and pick the speaking character - the first
   occupied row. The retail unsigned skill comparison starts at 0xffffffff,
   so later rows do not replace it. Every occupied portrait then takes target pose 1.
   A stale disposition snapshot on the NPC drops its 0x1c flag. */
// FUNCTION: WIZ8 0x0056D030
void SelectNpcDialogueSpeaker(W8NpcState* npc, int flags)
{
    W8NpcInteractionState* state;
    W8MonsterInfo* info;
    W8NpcState* selected;
    int slot;
    int speaker;
    unsigned int best;

    state = g_npc_interaction_state;
    speaker = -1;
    state->target_location_id = -1;
    best = 0xffffffff;
    state->dialogue_npc = 0;
    info = GetNpcMonsterInfo(npc);
    if (info != 0) {
        state->target_location_id = info->location_id;
    }
    state->dialogue_npc = npc;
    state->dialogue_npc->flag = false;
    state->dialogue_npc->flag0 = 0;
    if (state->dialogue_npc->greeting_pending) {
        state->dialogue_npc->suspicion = 0;
    }
    BeginNpcScriptDialogue(state->dialogue_npc, 0);
    if ((state->dialogue_layout == W8_DIALOGUE_LAYOUT_TRANSCRIPT ||
         state->dialogue_layout == W8_DIALOGUE_LAYOUT_TOPIC_MENU) &&
        gXStatus.fNpcDialogueMode) {
        static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_NPC_NAME])->m_textBuffer.SetText(state->dialogue_npc->record->source_name,
                                                       g_wiz_text_bold_font);
        static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_NPC_NAME])->Invalidate(1);
    }
    state->camera_redirected = 0;
    for (slot = 0; slot < 8; ++slot) {
        if (g_status.buffers.XChar[slot].fOccupied) {
            if (speaker == -1) {
                speaker = slot;
            }
            if (g_status.buffers.Char[slot].skills[W8_SKILL_COMMUNICATION].level > best) {
                best = g_status.buffers.Char[slot].skills[W8_SKILL_COMMUNICATION].level;
                speaker = slot;
            }
        }
    }
    g_npc_interaction_state->dialogue_speaker = speaker;
    for (slot = 0; slot < 8; ++slot) {
        if (g_status.buffers.XChar[slot].fOccupied) {
            SetPortraitTargetPose(&gXStatus.monster_manager_entries[slot], 1);
        }
    }
    selected = g_npc_interaction_state->dialogue_npc;
    if (selected->dismissed_flag &&
        selected->disposition_at_open != GetNpcDispositionBand(selected)) {
        selected->dismissed_flag = false;
    }
}

/* Build the NPC dialogue UI: the option-button panel on the left, the
   secondary panel beside it, the scrolling text controller, the three
   right-hand panels and the text-input panel, then every text control each
   of them hosts. Buttons get their option masks, help ids and activation
   callbacks as they are created. */
// FUNCTION: WIZ8 0x0056D1D0
void CreateNpcDialogueControls(void)
{
    Controls* panel;
    W8NpcInteractionState* state;

    state = g_npc_interaction_state;
    state->dialogue_panels[0] = new W8NpcDialogueOptionsPanel(0x17, 0x166, 0xa4, 0x1c2, 0x1a9, 0, 0);
    state->dialogue_panels[1] = new Controls(0xa4, 0x166, 0x1dc, 0x1c2, 0x1a9, 0, 1);
    state->dialogue_panels[2] =
        new W8NpcDialogueTextController(0x1dc, 0x11b, 0x269, 0x140, 0x1a9, 0, 2, 4, 3);
    state->dialogue_panels[3] = new Controls(0x1dc, 0x12f, 0x269, 0x1c2, 0x1a9, 0, 5);
    state->dialogue_panels[4] = new Controls(0x1dc, 0x166, 0x269, 0x1c2, 0x1a9, 0, 7);
    state->dialogue_panels[5] = new Controls(0x1dc, 0x166, 0x238, 499, 0x1a9, 0, 6);
    state->dialogue_panels[6] =
        new W8NpcTypedDialoguePanel(0x1dc, 0x166, 0x269, 0x1c0, 0x1a9, 0, 0xd);

    panel = static_cast<W8NpcDialogueOptionsPanel*>(state->dialogue_panels[0]);
    state->dialogue_controls[W8_NPC_CONTROL_NPC_NAME] =
        new W8TextControl(panel, 0xffffffff, 5, 2, 0x89, 0x12, -1, -1, -1, -1, -1, -1, -1);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_NPC_NAME])->m_textBuffer.SetLayoutMode(g_W8TextBufferAlignMiddle |
                                                         g_W8TextBufferAlignCenter);
    state->dialogue_controls[W8_NPC_CONTROL_TEXT_38] =
        new W8TextControl(panel, 0xffffffff, 5, 0x47, 0x89, 0x57, -1, -1, -1, -1, -1, -1, -1);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_TEXT_38])->m_textBuffer.SetLayoutMode(g_W8TextBufferAlignMiddle |
                                                         g_W8TextBufferAlignCenter);
    state->dialogue_controls[W8_NPC_CONTROL_TEXT_1] =
        new W8TextControl(panel, 0x82, 2, 0x11, 0x45, 0x21, 0x1a9, 0, -1, 0xc, 10, 0xb, -1);
    state->dialogue_controls[W8_NPC_CONTROL_TEXT_2] =
        new W8TextControl(panel, 0x83, 0x44, 0x11, 0x87, 0x21, 0x1a9, 0, -1, 0xc, 10, 0xb, -1);
    state->dialogue_controls[W8_NPC_CONTROL_TEXT_3] =
        new W8TextControl(panel, 0x84, 2, 0x21, 0x45, 0x31, 0x1a9, 0, -1, 0xc, 10, 0xb, -1);
    state->dialogue_controls[W8_NPC_CONTROL_TEXT_4] =
        new W8TextControl(panel, 0x85, 0x44, 0x21, 0x87, 0x31, 0x1a9, 0, -1, 0xc, 10, 0xb, -1);
    state->dialogue_controls[W8_NPC_CONTROL_TEXT_5] =
        new W8TextControl(panel, 0x86, 2, 0x31, 0x45, 0x41, 0x1a9, 0, -1, 0xc, 10, 0xb, -1);
    state->dialogue_controls[W8_NPC_CONTROL_TEXT_6] =
        new W8TextControl(panel, 0x87, 0x44, 0x31, 0x87, 0x41, 0x1a9, 0, -1, 0xc, 10, 0xb, -1);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_TEXT_6])->UpdateTextBounds(0x46, 0x31, 0x89, 0x41);
    state->dialogue_controls[W8_NPC_CONTROL_TEXT_7] =
        new W8TextControl(panel, 0x88, 0x72, 0x47, 0x89, 0x57, 0x1aa, 0, 0, 4, 1, 2, 3);
    state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 0] =
        new W8TextControl(panel, 0x75, 7, 0x46, 0x17, 0x56, 0x1ab, 0, 0, 1, 2, 4, 3);
    state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 1] =
        new W8TextControl(panel, 0x76, 0x19, 0x46, 0x29, 0x56, 0x1ab, 0, 5, 6, 7, 9, 8);
    state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 2] =
        new W8TextControl(panel, 0x77, 0x2b, 0x46, 0x3b, 0x56, 0x1ab, 0, 10, 0xb, 0xc, 0xe, 0xd);
    state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 3] = new W8TextControl(panel, 0x78, 0x3e, 0x46, 0x4e, 0x56, 0x1ab, 0,
                                                     0xf, 0x10, 0x11, 0x13, 0x12);
    state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 4] = new W8TextControl(panel, 0x79, 0x50, 0x46, 0x60, 0x56, 0x1ab, 0,
                                                     0x14, 0x15, 0x16, 0x18, 0x17);
    state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 5] = new W8TextControl(panel, 0x7a, 0x62, 0x46, 0x72, 0x56, 0x1ab, 0,
                                                     0x19, 0x1a, 0x1b, 0x1d, 0x1c);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 0])->AddLayoutFlags(g_W8TextControlLayoutToggle);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 0])->EnableRegionHelp(0x7c2);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 1])->AddLayoutFlags(g_W8TextControlLayoutToggle);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 1])->EnableRegionHelp(0x7c3);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 2])->AddLayoutFlags(g_W8TextControlLayoutToggle);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 2])->EnableRegionHelp(0x7c4);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 3])->AddLayoutFlags(g_W8TextControlLayoutToggle);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 3])->EnableRegionHelp(0x7c5);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 4])->AddLayoutFlags(g_W8TextControlLayoutToggle);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 4])->EnableRegionHelp(0x7c6);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 5])->AddLayoutFlags(g_W8TextControlLayoutToggle);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 5])->EnableRegionHelp(0x7c7);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 0])
        ->m_primaryActivationCallback = ToggleNpcHandItemFilter;
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 1])
        ->m_primaryActivationCallback = ToggleNpcAccessoryItemFilter;
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 2])
        ->m_primaryActivationCallback = ToggleNpcCharacterUsabilityFilter;
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 3])
        ->m_primaryActivationCallback = ToggleNpcBodyItemFilter;
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 4])
        ->m_primaryActivationCallback = ToggleNpcOtherItemFilter;
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 5])
        ->m_primaryActivationCallback = ToggleNpcPartyUsabilityFilter;

    panel = state->dialogue_panels[1];
    state->dialogue_controls[W8_NPC_CONTROL_WIDGET] = new W8Widget(panel, 0x89, 2, 4, 0x136, 0x57);

    panel = static_cast<W8NpcDialogueTextController*>(state->dialogue_panels[2]);
    state->dialogue_controls[W8_NPC_CONTROL_SCROLL] = new W8NpcDialogueScrollWidget(panel, 0x65, 6, 6, 0x7c, 0x11);
    state->dialogue_controls[W8_NPC_CONTROL_SCROLL_UP_BUTTON] =
        new W8TextControl(panel, 0x66, 0x7e, 3, 0x88, 0xb, 0x1aa, 0, 0x14, 0x18, 0x15, 0x16, 0x17);
    state->dialogue_controls[W8_NPC_CONTROL_SCROLL_DOWN_BUTTON] = new W8TextControl(panel, 0x67, 0x7e, 0xc, 0x88, 0x14,
                                                           0x1aa, 0, 0x19, 0x1d, 0x1a, 0x1b, 0x1c);

    panel = static_cast<W8NpcTypedDialoguePanel*>(state->dialogue_panels[6]);
    state->dialogue_controls[W8_NPC_CONTROL_TEXT_12] =
        new W8TextControl(panel, 0x68, 0x11, 0x23, 0x22, 0x30, 0x1aa, 0, 5, 9, 6, 7, 8);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_TEXT_12])->EnableRegionHelp(100);
    state->dialogue_controls[W8_NPC_CONTROL_TEXT_13] =
        new W8TextControl(panel, 0x69, 0x26, 0x23, 0x37, 0x30, 0x1aa, 0, 10, 0xe, 0xb, 0xc, 0xd);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_TEXT_13])->EnableRegionHelp(0x65);
    state->dialogue_controls[W8_NPC_CONTROL_TEXT_23] =
        new W8TextControl(panel, 0x73, 9, 0x33, 0x84, 0x44, 0x1a9, 0, -1, -1, 0xe, 0xf, -1);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_TEXT_23])->m_textBuffer.SetText(gppStringList[0x741], g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_TEXT_23])->m_primaryActivationCallback = SubmitNpcDialogueInput;
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_TEXT_23])->EnableRegionHelp(0x66);
    state->dialogue_controls[W8_NPC_CONTROL_TEXT_22] =
        new W8TextControl(panel, 0x72, 9, 0x43, 0x84, 0x52, 0x1a9, 0, -1, -1, 0xe, 0xf, -1);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_TEXT_22])->m_textBuffer.SetText(gppStringList[0x742], g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_TEXT_22])->m_primaryActivationCallback = SubmitNpcWhereIsQuery;
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_TEXT_22])->EnableRegionHelp(0x67);

    panel = state->dialogue_panels[3];
    state->dialogue_controls[W8_NPC_CONTROL_SORT_BUTTON] =
        new W8TextControl(panel, 0x6b, 6, 2, 0x89, 0xe, 0x1a9, 0, 8, 9, -1, -1, -1);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_SORT_BUTTON])->AddLayoutFlags(g_W8TextControlLayoutImageLeft |
                                                g_W8TextControlLayoutTextBesideImage |
                                                g_W8TextControlLayoutToggle);
    state->dialogue_controls[W8_NPC_CONTROL_PEOPLE_BUTTON] =
        new W8TextControl(panel, 0x6d, 6, 0xf, 0x41, 0x1b, 0x1a9, 0, 8, 9, -1, -1, -1);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_PEOPLE_BUTTON])->AddLayoutFlags(
        g_W8TextControlLayoutStayLatched | g_W8TextControlLayoutImageLeft |
        g_W8TextControlLayoutTextBesideImage | g_W8TextControlLayoutToggle);
    state->dialogue_controls[W8_NPC_CONTROL_PLACES_BUTTON] =
        new W8TextControl(panel, 0x6e, 0x43, 0xf, 0x89, 0x1b, 0x1a9, 0, 8, 9, -1, -1, -1);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_PLACES_BUTTON])->AddLayoutFlags(
        g_W8TextControlLayoutStayLatched | g_W8TextControlLayoutImageLeft |
        g_W8TextControlLayoutTextBesideImage | g_W8TextControlLayoutToggle);
    state->dialogue_controls[W8_NPC_CONTROL_ITEMS_BUTTON] =
        new W8TextControl(panel, 0x6f, 6, 0x1b, 0x41, 0x27, 0x1a9, 0, 8, 9, -1, -1, -1);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_ITEMS_BUTTON])->AddLayoutFlags(
        g_W8TextControlLayoutStayLatched | g_W8TextControlLayoutImageLeft |
        g_W8TextControlLayoutTextBesideImage | g_W8TextControlLayoutToggle);
    state->dialogue_controls[W8_NPC_CONTROL_MISC_BUTTON] =
        new W8TextControl(panel, 0x70, 0x43, 0x1b, 0x89, 0x27, 0x1a9, 0, 8, 9, -1, -1, -1);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_MISC_BUTTON])->AddLayoutFlags(
        g_W8TextControlLayoutStayLatched | g_W8TextControlLayoutImageLeft |
        g_W8TextControlLayoutTextBesideImage | g_W8TextControlLayoutToggle);
    state->dialogue_controls[W8_NPC_CONTROL_ALL_BUTTON] =
        new W8TextControl(panel, 0x71, 6, 0x27, 0x41, 0x33, 0x1a9, 0, 8, 9, -1, -1, -1);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_ALL_BUTTON])->AddLayoutFlags(
        g_W8TextControlLayoutStayLatched | g_W8TextControlLayoutImageLeft |
        g_W8TextControlLayoutTextBesideImage | g_W8TextControlLayoutToggle);

    panel = state->dialogue_panels[5];
    state->dialogue_controls[W8_NPC_CONTROL_TEXT_24] =
        new W8TextControl(panel, 0x74, 5, 0x14, 0x32, 0x49, -1, -1, -1, -1, -1, -1, -1);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_TEXT_24])->m_textBuffer.SetLayoutMode(g_W8TextBufferAlignBottom |
                                                         g_W8TextBufferAlignRight);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_TEXT_24])->AddLayoutFlags(g_W8TextControlLayoutImageAtOrigin);
    state->dialogue_controls[W8_NPC_CONTROL_TEXT_31] = new W8TextControl(panel, 0x7b, 0x35, 0x36, 0x51, 0x46, 0x1aa, 0,
                                                 0x1e, 0x22, 0x1f, 0x20, 0x21);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_TEXT_31])->EnableRegionHelp(0x7c9);
    state->dialogue_controls[W8_NPC_CONTROL_TEXT_33] =
        new W8TextControl(panel, 0x7d, 0x53, 0x4d, 0x86, 0x58, -1, -1, -1, -1, -1, -1, -1);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_TEXT_33])->m_textBuffer.SetLayoutMode(g_W8TextBufferAlignRight |
                                                         g_W8TextBufferAlignMiddle);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_TEXT_33])->SetEnabled(0);
    state->dialogue_controls[W8_NPC_CONTROL_TEXT_34] =
        new W8TextControl(panel, 0x7e, 5, 0x4d, 0x51, 0x58, -1, -1, -1, -1, -1, -1, -1);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_TEXT_34])->m_textBuffer.SetLayoutMode(g_W8TextBufferAlignMiddle |
                                                         g_W8TextBufferAlignCenter);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_TEXT_34])->SetEnabled(0);
    state->dialogue_controls[W8_NPC_CONTROL_TEXT_35] =
        new W8TextControl(panel, 0x7f, 0x53, 0x3a, 0x86, 0x45, -1, -1, -1, -1, -1, -1, -1);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_TEXT_35])->m_textBuffer.SetLayoutMode(g_W8TextBufferAlignRight |
                                                         g_W8TextBufferAlignMiddle);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_TEXT_35])->SetEnabled(0);
    state->dialogue_controls[W8_NPC_CONTROL_TEXT_36] =
        new W8TextControl(panel, 0x80, 4, 4, 0x88, 0x11, -1, -1, -1, -1, -1, -1, -1);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_TEXT_36])->m_textBuffer.SetLayoutMode(g_W8TextBufferAlignMiddle |
                                                         g_W8TextBufferAlignCenter);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_TEXT_36])->SetEnabled(0);
    state->dialogue_controls[W8_NPC_CONTROL_TEXT_37] =
        new W8TextControl(panel, 0x81, 0x38, 0x22, 0x88, 0x34, -1, -1, -1, -1, -1, -1, -1);
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_TEXT_7])->m_primaryActivationCallback = BackOutNpcDialogue;
    static_cast<W8TextControl*>(state->dialogue_controls[W8_NPC_CONTROL_TEXT_7])->EnableRegionHelp(0x7c8);
}

/* The NPC dialogue per-frame update: keeps the monster's navigator angles
   fresh, services the pending layout switch, mirrors the transcript controls'
   enabled state to the field text and selection, and clears the 500ms trade
   highlight flash once it lapses. */
// FUNCTION: WIZ8 0x0056E510
void ServiceNpcDialogue(void)
{
    wchar_t field_text[200];
    W8MonsterInfo* monster_info = GetNpcMonsterInfo(g_npc_interaction_state->dialogue_npc);
    if (monster_info != 0) {
        monster_info->p3D->UpdateAngles();
    }
    if (g_npc_interaction_state->scripted_dialogue) {
        return;
    }
    if (g_npc_interaction_state->dialogue_hidden != 0 && gfRightButtonState != 0) {
        SetNpcDialogueHidden(0);
    }
    Get16BitStringFromField(0, field_text);
    if (g_npc_interaction_state->pending_layout != 0) {
        g_level_block->text_box_visible = 0;
        RegionSetEnable(0x15);
        EnableRegionInput(0x52);
        EnableRegionInput(0x53);
        EnableRegionInput(0x54);
        EnableRegionInput(0x55);
        g_level_block->action_panel_visible = 1;
        int layout = g_npc_interaction_state->pending_layout;
        SwitchNpcDialogueLayout(W8_DIALOGUE_LAYOUT_NONE);
        switch (layout) {
        case W8_DIALOGUE_LAYOUT_SERVICES:
            OpenNpcDialogueMode1Layout();
            break;
        case W8_DIALOGUE_LAYOUT_TOPIC_MENU:
            ShowNpcDialogueTopicMenu();
            break;
        case W8_DIALOGUE_LAYOUT_TRANSCRIPT:
            OpenNpcDialogueTranscriptLayout();
            break;
        case W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX:
            OpenNpcDialogueOptionLayout();
            break;
        case W8_DIALOGUE_LAYOUT_TRADE:
            OpenNpcDialogueMode5Layout();
            break;
        }
        g_npc_interaction_state->pending_layout = 0;
    }
    if (g_npc_interaction_state->dialogue_layout == W8_DIALOGUE_LAYOUT_TRANSCRIPT) {
        if (wcslen(field_text) == 0) {
            if (static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_12])->m_enabled != 0) {
                static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_12])->SetEnabled(0);
                static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_12])->W8Widget::Invalidate(1);
            }
        } else if (static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_12])->m_enabled == 0) {
            static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_12])->SetEnabled(1);
            static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_12])->W8Widget::Invalidate(1);
        }
        int selection =
            static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2])->GetSelectedTranscriptEntryIndex();
        if (selection == -1) {
            if (static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_13])->m_enabled != 0) {
                static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_13])->SetEnabled(0);
                static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_13])->W8Widget::Invalidate(1);
            }
        } else if (static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_13])->m_enabled == 0) {
            static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_13])->SetEnabled(1);
            static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_13])->W8Widget::Invalidate(1);
        }
        g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_SCROLL_UP_BUTTON]->SetEnabled(
            static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2])->HandleScrollUpCommand(1) != 0);
        g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_SCROLL_DOWN_BUTTON]->SetEnabled(
            static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2])->HandleScrollDownCommand(1) != 0);
    }
    if (static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_24]) != 0 &&
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_24])->m_textBuffer.m_highlighted != 0 &&
        GetTickCount() - g_trade_highlight_tick > 500) {
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_24])->m_textBuffer.m_highlighted = false;
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_24])->m_textBuffer.SetGeometryDirty();
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_24])->Invalidate(0);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_35])->m_textBuffer.m_highlighted = false;
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_35])->m_textBuffer.SetGeometryDirty();
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_35])->Invalidate(0);
    }
}

/* Close the NPC dialogue: retire the active layout, release the panels and
   text controls, restore the held item or target cursor, queue the parting
   character event and hand control back to the world. */
/* Close the current dialogue layout before ending, switching or returning
   from trade. The name is descriptive; the shared dispatch retains each
   layout's existing teardown API. */
static void CloseActiveNpcDialogueLayout()
{
    switch (g_npc_interaction_state->dialogue_layout) {
    case W8_DIALOGUE_LAYOUT_SERVICES:
        CloseNpcDialogueMode1Layout();
        break;
    case W8_DIALOGUE_LAYOUT_TOPIC_MENU:
        CloseNpcDialogueLayout();
        break;
    case W8_DIALOGUE_LAYOUT_TRANSCRIPT:
        CloseNpcDialogueTranscriptLayout();
        break;
    case W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX:
        CloseNpcDialogueOptionLayout();
        break;
    case W8_DIALOGUE_LAYOUT_TRADE:
        CloseNpcDialogueMode5Layout();
        break;
    case W8_DIALOGUE_LAYOUT_BARE:
        SetNpcDialogueLayoutMode(W8_DIALOGUE_LAYOUT_NONE);
        break;
    default:
        break;
    }
}

// FUNCTION: WIZ8 0x0056E800
void EndNpcDialogueSession(bool param_1)
{
    if (!gXStatus.fNpcDialogueMode) {
        return;
    }
    if (g_npc_interaction_state->scripted_dialogue) {
        gXStatus.fNpcDialogueMode = false;
        ResumeMainGameWorld();
        return;
    }
    if (gXStatus.fCampMode) {
        param_1 = 1;
    }
    if (g_npc_interaction_state->dialogue_hidden != 0) {
        SetNpcDialogueHidden(0);
    }
    if (g_npc_interaction_state->dialogue_npc->record->monster_bound == 0) {
        g_npc_interaction_state->dialogue_npc->dismissed_flag = true;
        g_npc_interaction_state->dialogue_npc->disposition_at_open =
            GetNpcDispositionBand(g_npc_interaction_state->dialogue_npc);
        W8NpcState* npc = g_npc_interaction_state->dialogue_npc;
        npc->dismissed_timer = 0;
    }
    gXStatus.fNpcDialogueMode = false;
    g_level_block->action_panel_visible = 0;
    RegionSetDisable(0x15);
    DisableRegionInput(0x52);
    DisableRegionInput(0x53);
    DisableRegionInput(0x54);
    DisableRegionInput(0x55);
    CloseActiveNpcDialogueLayout();
    RegionSetDisable(0x18);
    SelectTextBox(0);
    g_level_block->text_box_visible = 1;
    ApplyMainGameModeFlag(gXStatus.fCampMode ? g_settings.main_ui_mode
                                             : g_npc_interaction_state->saved_main_ui_mode,
                          1);
    /* The seven dialogue panels delete through the non-virtual Controls
       destructor; the thirty-nine dialogue controls through the virtual one.
       The members stay dangling until the next dialogue rebuilds them. */
    int i;
    Controls** panel = g_npc_interaction_state->dialogue_panels;
    for (i = 0; i < 7; i++) {
        delete panel[i];
    }
    W8Widget** control = g_npc_interaction_state->dialogue_controls;
    for (i = 0; i < 39; i++) {
        delete control[i];
    }
    RequestRedrawCombatBar();
    RequestRedraw(W8_MAIN_REDRAW_SUBMENU_BUTTONS);
    for (unsigned int index = 0; index < PLLength(gXStatus.plsMonsterList); index++) {
        ClearMonsterCharm(MonsterGetScriptPartByLocationIndex(index));
    }
    GetNpcMonsterInfo(g_npc_interaction_state->dialogue_npc);
    KillTextInputMode();
    ResumeMainGameWorld();
    if (!param_1) {
        if (!g_npc_interaction_state->held_item_pending) {
            SetTargetCursor(W8_CURSOR_NONE);
        } else {
            g_status.item_in_hand = g_npc_interaction_state->pending_item;
            SetItemCursor(0);
            g_npc_interaction_state->held_item_pending = 0;
        }
        W8NpcState* npc = g_npc_interaction_state->dialogue_npc;
        if (!g_npc_interaction_state->suppress_parting_reaction && npc->record->merchant == 0 &&
            !npc->is_grouped && GetNpcMonsterInfo(npc) != 0) {
            int slot = GetRandomCharacter(1, 1, -1, -1);
            if (slot != -1) {
                W8CharacterEvent* event =
                    QueueCharacterEvent(&g_status.buffers.Char[slot], g_special_event8,
                                        g_character_event_no_preempt, g_character_event_no_flags,
                                        g_character_event_full_volume);
                if (event != 0) {
                    event->dispatch_delay_ms = 1000;
                    event->dispatch_delay_start = GetTickCount();
                }
            }
        }
    }
    FlushPendingNoticeLines();
    if (g_npc_interaction_state->camera_redirected) {
        g_gd_camera->SetPitch(g_npc_interaction_state->saved_camera_pitch);
    }
    if (g_npc_interaction_state->dialogue_npc->name_style == 0xf &&
        GetFact(W8_FACT_ALIGNMENT_UMPANI) != 0) {
        g_status.trang_check_clock = g_status.world_clock;
        g_status.trang_check_pending = 1;
    }
}

/* Whether the open NPC dialogue transcript covers the party slot's
   portrait: dialogue mode up, panel invalidation not suppressed and the
   controller enabled, then the slot's band check. Portrait and
   character-update paths skip the covered rows through this. */
// FUNCTION: WIZ8 0x0056EC90
bool IsPortraitObscuredByNpcDialogue(unsigned int party_slot)
{
    if (gXStatus.fNpcDialogueMode && !g_npc_interaction_state->scripted_dialogue &&
        static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2])
                ->m_fEnabled != 0) {
        return static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2])
            ->IsSlotPortraitTranscriptCovered(party_slot);
    }
    return false;
}

/* Forward one text/cursor rectangle to the action panel while the dialogue
   screen is not suppressing panel invalidation. */
// FUNCTION: WIZ8 0x0056ECD0
void InvalidateMainGameActionPanelRect(const W8ControlsRect* rect)
{
    if (!g_npc_interaction_state->scripted_dialogue) {
        g_npc_interaction_state->dialogue_panels[1]->Invalidate(rect);
    }
}

/* Redraw each enabled dialogue panel, optionally forcing it disabled first.
   The panel_1ac dirty check snapshots its state before Redraw consumes it and
   repaints the shared text-box scroll chrome alongside. */
// FUNCTION: WIZ8 0x0056ECF0
void ActivateNpcDialoguePanels(bool active)
{
    bool redraw_scroll = false;
    if (!g_npc_interaction_state->scripted_dialogue) {
        Controls** panel = g_npc_interaction_state->dialogue_panels;
        for (int i = 0; i < 7; i++) {
            if (panel[i]->m_fEnabled) {
                if (active) {
                    panel[i]->Invalidate(0);
                }
                if (i == 1) {
                    redraw_scroll = g_npc_interaction_state->dialogue_panels[1]->m_fEnabled &&
                                    (g_npc_interaction_state->dialogue_panels[1]->m_fDirty ||
                                     g_npc_interaction_state->dialogue_panels[1]->m_fLayoutDirty);
                }
                panel[i]->Redraw();
                if (redraw_scroll) {
                    RedrawTextBoxScrollChrome();
                }
            }
        }
    }
}

/* Whether any of the seven dialogue panels is enabled with a redraw or
   layout pass still pending. */
// FUNCTION: WIZ8 0x0056ED80
bool HasNpcDialogueDirtyPanels(void)
{
    if (!g_npc_interaction_state->scripted_dialogue) {
        Controls** panel = g_npc_interaction_state->dialogue_panels;
        for (int i = 0; i < 7; i++) {
            if (panel[i]->m_fEnabled && (panel[i]->m_fDirty || panel[i]->m_fLayoutDirty)) {
                return 1;
            }
        }
    }
    return 0;
}

/* Retire the current dialogue layout mode into previous_dialogue_layout and, when nonzero,
   install `value` as the new one; either way the dialogue cursor helper gets
   re-run while the flag is set. */
// FUNCTION: WIZ8 0x0056EDD0
void SetNpcDialogueLayoutMode(W8NpcDialogueLayout value)
{
    if (value == W8_DIALOGUE_LAYOUT_NONE) {
        g_npc_interaction_state->previous_dialogue_layout =
            g_npc_interaction_state->dialogue_layout;
        g_npc_interaction_state->dialogue_layout = W8_DIALOGUE_LAYOUT_NONE;
    } else {
        g_npc_interaction_state->dialogue_layout = value;
    }
    if (g_npc_interaction_state->dialogue_hidden != 0) {
        SetNpcDialogueHidden(0);
    }
}

/* Whether the typed-text cursor of the active NPC dialogue is up; only the
   main-game action keys keep working while it owns input. */
/* Refresh the mode-1 service buttons and mode-4 item editor for the party
   slot that entered the dialogue. Mode 1 enables the heal/cure/service rows
   from what the character can actually use; mode 4 resets the editor and
   re-arms the trade prompt. */
// FUNCTION: WIZ8 0x0056EE20
void SyncNpcServiceButtons(int party_slot)
{
    W8Character* character = &g_status.buffers.Char[party_slot];
    if (gXStatus.fNpcDialogueMode) {
        if (g_npc_interaction_state->dialogue_layout == W8_DIALOGUE_LAYOUT_SERVICES) {
            static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])->SetEnabled(
                CanCharacterCastSpell(character, 3));
            static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])->W8Widget::Invalidate(1);
            static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])->SetEnabled(
                CanCharacterCastSpell(character, 0x29));
            static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])->W8Widget::Invalidate(1);
            static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])->SetEnabled(
                CharacterHasServiceItem(character));
            static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])->W8Widget::Invalidate(1);
        } else if (g_npc_interaction_state->dialogue_layout == W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX) {
            SelectNpcPartyItems();
            UpdateNpcDialogueSubMode();
        }
    }
}

// FUNCTION: WIZ8 0x0056efb0
unsigned char IsNpcDialogueCursorActive(void)
{
    if (!gXStatus.fNpcDialogueMode) {
        return 0;
    }
    return g_npc_interaction_state->dialogue_hidden;
}

/* Whether an NPC dialogue is up in the layout that posts to the main text
   box (mode 4) rather than to the dialogue's own pane. */
// FUNCTION: WIZ8 0x0056efd0
bool IsNpcDialogueTextBoxActive(void)
{
    if (!gXStatus.fNpcDialogueMode) {
        return false;
    }
    return g_npc_interaction_state->dialogue_layout == W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX;
}

// FUNCTION: WIZ8 0x0056EFF0
void TryNpcDialoguePickpocket(int party_slot)
{
    if (gXStatus.fNpcDialogueMode && g_npc_interaction_state->dialogue_hidden != 0) {
        SetNpcDialogueHidden(0);
        ResolveNpcPickpocket(party_slot);
    }
}

/* Forward mouse enter/leave and button events to the main-screen control whose
   pointer sits at W8NpcInteractionState::dialogue_controls[callback_id]. Id 0x27
   is the panel slot at +0x1a8 and is ignored. Ids 1..6 and 0x16/0x17 pass 1
   into the widget hooks; every other id passes 0. Mouse-enter always passes 0. */
// FUNCTION: WIZ8 0x0056F020
unsigned char MainScreenControlRegionEvent(const InputAtom* event, W8Region* region)
{
    unsigned int callback_id = region->callback_id;
    W8Widget* control;
    bool arg;

    if (callback_id == 0x27) {
        return 0;
    }
    control = g_npc_interaction_state->dialogue_controls[callback_id];
    if (control == 0) {
        return 0;
    }
    switch (event->usEvent) {
    case LEFT_BUTTON_DOWN:
    case LEFT_BUTTON_REPEAT:
        if ((callback_id >= 1 && callback_id <= 6) || callback_id == 0x16 || callback_id == 0x17) {
            arg = 1;
        } else {
            arg = 0;
        }
        control->OnLeftButtonDown(arg);
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        return 1;
    case LEFT_BUTTON_UP:
        if ((callback_id >= 1 && callback_id <= 6) || callback_id == 0x16 || callback_id == 0x17) {
            arg = 1;
        } else {
            arg = 0;
        }
        control->OnLeftButtonUp(arg);
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) == 0) {
            return 1;
        }
        region->flags &= ~W8_REGION_LEFT_BUTTON_HELD;
        return 1;
    case RIGHT_BUTTON_DOWN:
    case RIGHT_BUTTON_REPEAT:
        if ((callback_id >= 1 && callback_id <= 6) || callback_id == 0x16 || callback_id == 0x17) {
            arg = 1;
        } else {
            arg = 0;
        }
        control->OnRightButtonDown(arg);
        region->flags |= W8_REGION_RIGHT_BUTTON_HELD;
        return 1;
    case RIGHT_BUTTON_UP:
        if ((callback_id >= 1 && callback_id <= 6) || callback_id == 0x16 || callback_id == 0x17) {
            arg = 1;
        } else {
            arg = 0;
        }
        control->OnRightButtonUp(arg);
        if ((region->flags & W8_REGION_RIGHT_BUTTON_HELD) != 0) {
            region->flags &= ~W8_REGION_RIGHT_BUTTON_HELD;
        }
        return 1;
    case MOUSE_POS:
        if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
            if ((callback_id >= 1 && callback_id <= 6) || callback_id == 0x16 ||
                callback_id == 0x17) {
                arg = 1;
            } else {
                arg = 0;
            }
            control->OnMouseLeave(arg);
            return 1;
        }
        if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
            control->OnMouseEnter(0);
            return 1;
        }
        break;
    }
    return 0;
}

/* Region callback on the NPC-dialogue notice text boxes: the left button
   routes through the keyword/slot click helpers, the right button through the
   assay/keyword lookup helpers, and MOUSE_POS tracks the hovered line or
   notice word. Left release falls through into RIGHT_BUTTON_DOWN and also
   raises the right-held flag (retail bug). */
static void HighlightNpcDialogueLine(int line)
{
    ClearHoveredTextLine(2);
    if (line < static_cast<int>(g_status.text_box_lines_shown[2])) {
        SetHoveredTextLine(g_level_block->text_lines[2] + line, 2);
    }
    RedrawTextBox();
}

// FUNCTION: WIZ8 0x0056F1D0
unsigned char NpcDialogueTextBoxRegionEvent(const InputAtom* event, W8Region* region)
{
    int line;
    int y;

    switch (event->usEvent) {
    case LEFT_BUTTON_DBL_CLK:
        NpcDialogueTextBoxDoubleClick(static_cast<unsigned short>(event->uiParam),
                                      event->uiParam >> 16);
        return 1;
    case LEFT_BUTTON_UP:
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
            region->flags &= ~W8_REGION_LEFT_BUTTON_HELD;
            NpcDialogueTextBoxLeftUp(static_cast<unsigned short>(event->uiParam),
                                     event->uiParam >> 16);
        }
        /* fall through */
    case RIGHT_BUTTON_DOWN:
        region->flags |= W8_REGION_RIGHT_BUTTON_HELD;
        return 1;
    case LEFT_BUTTON_DOWN:
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        return 1;
    case MOUSE_POS:
        break;
    case RIGHT_BUTTON_UP:
        if ((region->flags & W8_REGION_RIGHT_BUTTON_HELD) != 0) {
            region->flags &= ~W8_REGION_RIGHT_BUTTON_HELD;
            NpcDialogueTextBoxRightUp(static_cast<unsigned short>(event->uiParam),
                                      event->uiParam >> 16);
        }
        return 1;
    default:
        return 0;
    }
    if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
        NoOp();
        ClearNoticeWordHover(3, 1);
        ClearHoveredTextLine(2);
        return 1;
    }
    if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
        g_npc_interaction_state->last_mouse_x = static_cast<unsigned short>(event->uiParam);
        g_npc_interaction_state->last_mouse_y = event->uiParam >> 16;
        NoOp();
        if (g_npc_interaction_state->dialogue_layout == W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX) {
            y = (event->uiParam >> 16) & 0xffff;
            if (y >= g_level_block->text_box_top && y <= g_level_block->text_box_bottom) {
                line = (y - g_level_block->text_box_top) / 11;
                HighlightNpcDialogueLine(line);
                g_npc_interaction_state->hovered_text_line = line;
            }
        }
        return 1;
    }
    if (static_cast<unsigned short>(event->uiParam) == g_npc_interaction_state->last_mouse_x &&
        static_cast<int>(event->uiParam >> 16) == g_npc_interaction_state->last_mouse_y) {
        return 1;
    }
    g_npc_interaction_state->last_mouse_x = static_cast<unsigned short>(event->uiParam);
    g_npc_interaction_state->last_mouse_y = event->uiParam >> 16;
    if (g_npc_interaction_state->dialogue_layout == W8_DIALOGUE_LAYOUT_TRANSCRIPT) {
        HighlightNoticeWordAt(3, static_cast<unsigned short>(event->uiParam),
                              static_cast<unsigned short>(event->uiParam >> 16));
        return 1;
    }
    if (g_npc_interaction_state->dialogue_layout == W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX) {
        y = (event->uiParam >> 16) & 0xffff;
        if (y >= g_level_block->text_box_top && y <= g_level_block->text_box_bottom) {
            line = (y - g_level_block->text_box_top) / 11;
            if (line != g_npc_interaction_state->hovered_text_line) {
                HighlightNpcDialogueLine(line);
            }
            g_npc_interaction_state->hovered_text_line = line;
        }
    }
    return 1;
}

/* Mouse-wheel line tracking on the main NPC text box: flag forces the redraw
   even when the hovered line has not changed. */
// FUNCTION: WIZ8 0x0056F490
void NpcDialogueTextBoxWheelAt(short x, unsigned short y, bool flag)
{
    int line;

    if (g_npc_interaction_state->dialogue_layout != W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX) {
        return;
    }
    if (static_cast<int>(y) < g_level_block->text_box_top ||
        static_cast<int>(y) > g_level_block->text_box_bottom) {
        return;
    }
    line = (static_cast<int>(y) - g_level_block->text_box_top) / 11;
    if (line != g_npc_interaction_state->hovered_text_line || flag) {
        HighlightNpcDialogueLine(line);
    }
    g_npc_interaction_state->hovered_text_line = line;
}

static void SelectNpcDialogueKeyword(W8NoticeWord* word, int line)
{
    wchar_t word_text[200];
    wchar_t field_text[200];
    wchar_t combined[400];
    word->keyword = W8_NOTICE_WORD_SELECTED;
    CopyNoticeWordText(word, word_text, 0xc8, 3, line);
    Get16BitStringFromField(0, field_text);
    StripNpcKeywordPunctuation(field_text);
    if (wcslen(field_text) != 0) {
        swprintf(combined, L"%s %s", field_text, word_text);
        SetInputFieldStringWith16BitString(0, combined);
    } else {
        SetInputFieldStringWith16BitString(0, word_text);
    }
    RedrawTextBoxBody(1);
}

/* Left release inside the NPC text box: in the transcript layout a hovered
   notice word becomes the selected keyword and is appended to the input field;
   in the item layouts the hovered slot is selected. */
// FUNCTION: WIZ8 0x0056F530
void NpcDialogueTextBoxLeftUp(int x, int y)
{
    wchar_t word_text[200];
    int line;
    W8NoticeWord* word;
    int slot;

    switch (g_npc_interaction_state->dialogue_layout) {
    case W8_DIALOGUE_LAYOUT_TRANSCRIPT:
        word = HitTestNoticeWord(3, x, y, &line);
        if (word == 0 || word->keyword != W8_NOTICE_WORD_HOVERED) {
            return;
        }
        if (gfKeyState[0x10] == 0) {
            ResetUsedNoticeWords(3, 1);
            word_text[0] = 0;
            SetInputFieldStringWith16BitString(0, word_text);
        }
        SelectNpcDialogueKeyword(word, line);
        return;
    case W8_DIALOGUE_LAYOUT_SERVICES:
    case W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX:
        slot = GetHoveredTextLine(2);
        if (slot == -1) {
            return;
        }
        UpdateNpcTradeSelection(slot, gfKeyState[0x10] != 0 ? 1 : 0, 1);
        return;
    default:
        break;
    }
}

/* Right release inside the NPC text box: transcript layout adds the clicked
   keyword to the dialogue transcript; the item layouts select the slot and
   open the item assay dialog. */
// FUNCTION: WIZ8 0x0056F6B0
void NpcDialogueTextBoxRightUp(int x, int y)
{
    wchar_t word_text[200];
    int line;
    W8NoticeWord* word;
    int slot;
    W8ItemInstance* item;
    W8AssayDialog* dialog;

    switch (g_npc_interaction_state->dialogue_layout) {
    case W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX:
        slot = GetHoveredTextLine(2);
        if (slot == -1) {
            return;
        }
        UpdateNpcTradeSelection(slot, gfKeyState[0x10] != 0 ? 1 : 0, 1);
        item = ResolveNpcTradeRow(slot, 0, 0, 1);
        if (item == 0) {
            return;
        }
        dialog = new W8AssayDialog(item, &g_status.buffers.Char[g_status.selected_character]);
        dialog->SetText(&g_empty_wide_string);
        dialog->SetOrigin(g_info_dialog_x, 0x48);
        dialog->m_destroy_callback = OnNpcAssayDialogClosed;
        OpenModal(dialog);
        return;
    case W8_DIALOGUE_LAYOUT_TRANSCRIPT:
        word = HitTestNoticeWord(3, x, y, &line);
        if (word == 0) {
            return;
        }
        CopyNoticeWordText(word, word_text, 0xc8, 3, line);
        AddNpcDialogueKeyword(word_text, -1, 0);
        return;
    default:
        break;
    }
}

/* Left double-click inside the NPC text box: transcript layout selects the
   keyword and submits the input line; the item layout picks the hovered line's
   slot and uses the selected item. */
// FUNCTION: WIZ8 0x0056F840
void NpcDialogueTextBoxDoubleClick(int x, int y)
{
    wchar_t word_text[200];
    int line;
    W8NoticeWord* word;
    int slot;

    switch (g_npc_interaction_state->dialogue_layout) {
    case W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX:
        break;
    case W8_DIALOGUE_LAYOUT_TRANSCRIPT:
        if (g_npc_interaction_state->modal_dialog_open) {
            return;
        }
        word = HitTestNoticeWord(3, x, y, &line);
        if (word == 0) {
            return;
        }
        if (gfKeyState[0x10] == 0) {
            ResetUsedNoticeWords(3, 1);
            word_text[0] = 0;
            SetInputFieldStringWith16BitString(0, word_text);
        }
        if (word->keyword != W8_NOTICE_WORD_SELECTED) {
            SelectNpcDialogueKeyword(word, line);
        }
        HandleNpcDialogueInput();
        return;
    default:
        return;
    }
    y &= 0xffff;
    if (y >= g_level_block->text_box_top && y <= g_level_block->text_box_bottom) {
        line = (y - g_level_block->text_box_top) / 11;
        HighlightNpcDialogueLine(line);
        g_npc_interaction_state->hovered_text_line = line;
    }
    slot = GetHoveredTextLine(2);
    if (slot != -1) {
        g_npc_interaction_state->selected_trade_row = -1;
        UpdateNpcTradeSelection(slot, 0, 1);
        if (g_npc_interaction_state->trade_item != 0) {
            ConfirmNpcTradeItem();
        }
    }
    NpcDialogueTextBoxWheelAt(0, static_cast<unsigned short>(y), true);
}

static void ShowNpcTradeGold()
{
    wchar_t text[0x20];

    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_37])
        ->SetEnabled(1);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_37])
        ->m_textBuffer.SetFontStateIndex(1);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_37])
        ->m_textBuffer.SetGeometryDirty();
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_31])
        ->SetEnabled(1);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_36])
        ->m_textBuffer.SetText(gppStringList[0x72d], g_wiz_text_font_secondary);
    swprintf(text, L"%dg", g_npc_interaction_state->trade_gold);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_35])
        ->m_textBuffer.SetText(text, g_wiz_text_font_secondary);
    g_npc_interaction_state->dialogue_panels[5]->Invalidate(0);
}

/* Select the item slot under the text-box cursor and refresh the item preview
   controls: layout 1 pulls from the equipment/backpack picker, layout 4 from
   the trade lists; the mode-2 row zero additionally fills in the purse readout. */
// FUNCTION: WIZ8 0x0056FAC0
void UpdateNpcTradeSelection(int index, int increment, int commit)
{
    W8ItemInstance* item;
    wchar_t text[204];
    wchar_t price_text[220];
    bool wants_item;

    SetSelectedTextLine(index, 2);
    switch (g_npc_interaction_state->dialogue_layout) {
    case W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX:
        break;
    case W8_DIALOGUE_LAYOUT_SERVICES:
        item = GetNpcTradeSlotItem(index);
        g_npc_interaction_state->trade_item = item;
        if (g_npc_interaction_state->trade_item != 0) {
            static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_37])->SetEnabled(1);
        }
        return;
    default:
        return;
    }
    item = ResolveNpcTradeRow(index, 1, increment, commit);
    g_npc_interaction_state->trade_item = item;
    if (g_npc_interaction_state->trade_item != 0) {
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_37])->SetEnabled(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_31])->SetEnabled(
            1 < g_npc_interaction_state->trade_item->stack_count);
        if (g_item_records[g_npc_interaction_state->trade_item->iItemNo].equip_class ==
            W8_ITEM_EQUIP_CLASS_AMMUNITION) {
            static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_31])->SetEnabled(0);
        }
        ShortenTextToWidth(text, FormatItemDisplayName(g_npc_interaction_state->trade_item, 0),
                           0x7d, g_wiz_text_font_secondary);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_36])->m_textBuffer.SetText(text,
                                                                         g_wiz_text_font_secondary);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_37])->m_textBuffer.SetFontStateIndex(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_37])->m_textBuffer.SetGeometryDirty();
        switch (g_npc_interaction_state->trade_mode) {
        case W8_NPC_TRADE_BUY:
            if (g_status.party_gold < static_cast<unsigned int>(CalculateNpcTradeStackPrice(
                                          g_npc_interaction_state->dialogue_npc,
                                          g_npc_interaction_state->trade_item->iItemNo, 1,
                                          g_npc_interaction_state->trade_quantity,
                                          g_npc_interaction_state->trade_item->identified))) {
                static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_37])->m_textBuffer.SetFontStateIndex(0);
                static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_37])->m_textBuffer.SetGeometryDirty();
            }
            swprintf(price_text, L"%dg",
                     CalculateNpcTradeStackPrice(g_npc_interaction_state->dialogue_npc,
                                                 g_npc_interaction_state->trade_item->iItemNo, 1,
                                                 g_npc_interaction_state->trade_quantity,
                                                 g_npc_interaction_state->trade_item->identified));
            break;
        case W8_NPC_TRADE_SELL: {
            wants_item = NpcAcceptsTradeItem(g_npc_interaction_state->dialogue_npc,
                                             g_npc_interaction_state->trade_item);
            if (!wants_item) {
                static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_37])->m_textBuffer.SetFontStateIndex(0);
                static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_37])->m_textBuffer.SetGeometryDirty();
            }
            int price = CalculateNpcTradeStackPrice(
                g_npc_interaction_state->dialogue_npc, g_npc_interaction_state->trade_item->iItemNo,
                0, g_npc_interaction_state->trade_quantity,
                g_npc_interaction_state->trade_item->identified);
            if (wants_item) {
                swprintf(price_text, L"%dg", price);
            } else {
                swprintf(price_text, L"---");
            }
            break;
        }
        case W8_NPC_TRADE_GIVE:
            swprintf(price_text, L" ");
            break;
        default:
            return;
        }
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_35])->m_textBuffer.SetText(price_text,
                                                                         g_wiz_text_font_secondary);
        return;
    }
    if (g_npc_interaction_state->trade_mode == W8_NPC_TRADE_GIVE && index == 0) {
        ShowNpcTradeGold();
    }
}

/* Reset the secondary dialogue editor: drop the pending item, clear the item
   notice control and both secondary labels, and restore the option row. */
// FUNCTION: WIZ8 0x0056FED0
void ResetNpcDialogueItemEditor(void)
{
    ClearSelectedTextLine(2);
    g_npc_interaction_state->trade_item = 0;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_37])->SetEnabled(0);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_24])
        ->ClearImage();
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_24])->m_textBuffer.SetText(g_dialogue_empty_text, 0);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_24])->Invalidate(1);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_35])->m_textBuffer.SetText(&g_empty_wide_string,
                                                                     g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_36])->m_textBuffer.SetText(&g_empty_wide_string,
                                                                     g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_31])->SetEnabled(0);
    g_npc_interaction_state->selected_trade_row = -1;
    g_npc_interaction_state->trade_quantity = 1;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_37])->m_textBuffer.SetFontStateIndex(-1);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_37])->m_textBuffer.SetGeometryDirty();
}

/* Retire the current dialogue layout, then open the layout `interact_id`
   selects. */
/* Close the active sub-layout and reopen the layout it replaced: mode 1 goes
   back to the remembered previous_dialogue_layout layout, the option layout returns to the
   topic menu or the transcript by reopen_topics, and mode 5 always lands on the
   transcript. */
// FUNCTION: WIZ8 0x00570000
void BackOutNpcDialogue(void)
{
    switch (g_npc_interaction_state->dialogue_layout) {
    case W8_DIALOGUE_LAYOUT_SERVICES:
        CloseNpcDialogueMode1Layout();
        switch (g_npc_interaction_state->previous_dialogue_layout) {
        case W8_DIALOGUE_LAYOUT_TOPIC_MENU:
            ShowNpcDialogueTopicMenu();
            break;
        case W8_DIALOGUE_LAYOUT_TRANSCRIPT:
            OpenNpcDialogueTranscriptLayout();
            break;
        default:
            break;
        }
        break;
    case W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX: {
        bool flag = g_npc_interaction_state->reopen_topics;
        CloseNpcDialogueOptionLayout();
        if (flag) {
            ShowNpcDialogueTopicMenu();
        } else {
            OpenNpcDialogueTranscriptLayout();
        }
        break;
    }
    case W8_DIALOGUE_LAYOUT_TRADE:
        CloseNpcDialogueMode5Layout();
        OpenNpcDialogueTranscriptLayout();
        break;
    default:
        break;
    }
}

// FUNCTION: WIZ8 0x00570120
void SwitchNpcDialogueLayout(int interact_id)
{
    CloseActiveNpcDialogueLayout();
    switch (interact_id) {
    case W8_DIALOGUE_LAYOUT_SERVICES:
        OpenNpcDialogueMode1Layout();
        return;
    case W8_DIALOGUE_LAYOUT_TOPIC_MENU:
        ShowNpcDialogueTopicMenu();
        return;
    case W8_DIALOGUE_LAYOUT_TRANSCRIPT:
        OpenNpcDialogueTranscriptLayout();
        return;
    case W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX:
        OpenNpcDialogueOptionLayout();
        return;
    case W8_DIALOGUE_LAYOUT_TRADE:
        OpenNpcDialogueMode5Layout();
        return;
    }
}

// FUNCTION: WIZ8 0x00570310
void LeaveNpcDialogueLayout(void)
{
    if (!g_npc_interaction_state->modal_dialog_open) {
        if (GetNpcDispositionBand(g_npc_interaction_state->dialogue_npc) == 0) {
            g_npc_interaction_state->suppress_parting_reaction = 0;
            g_npc_interaction_state->farewell_queued = true;
            QueueNpcScriptLine(0x5c, 0, 0, 0);
            return;
        }
        SwitchNpcDialogueLayout(W8_DIALOGUE_LAYOUT_NONE);
    }
    EndNpcDialogueSession(0);
}

// FUNCTION: WIZ8 0x00570530
void PromptNpcDispositionChange(void)
{
    W8MessageDialogBase* dialog = static_cast<W8MessageDialogBase*>(CreateDialogByKind(1));

    SetDialogPrompt(dialog, gppStringList[0x798], 0, 0);
    dialog->m_destroy_callback = OnNpcDispositionPromptClosed;
    OpenModal(dialog);
}

// FUNCTION: WIZ8 0x00570570
void OnNpcDispositionPromptClosed(W8DialogBase* dialog)
{
    if (GetDialogResult(dialog) != 0) {
        SetNpcDispositionBand(g_npc_interaction_state->dialogue_npc, 2);
        QueueNpcScriptLine(0x18, 0, 0, 0);
        QueueNpcMessageLine(W8_NPC_MSG_CLOSE_RESUME_NPC, 0);
    }
}

// FUNCTION: WIZ8 0x005705B0
void EnterNpcServiceLayout(void)
{
    SwitchNpcDialogueLayout(W8_DIALOGUE_LAYOUT_NONE);
    OpenNpcDialogueMode1Layout();
}

/* Bring up the mode-2 dialogue layout: the option panels come up, the caption
   takes the NPC's name, and the five topics get their strings and callbacks. */
// FUNCTION: WIZ8 0x00570760
void ShowNpcDialogueTopicMenu(void)
{
    g_npc_interaction_state->dialogue_layout = W8_DIALOGUE_LAYOUT_TOPIC_MENU;
    if (g_npc_interaction_state->dialogue_hidden != 0) {
        SetNpcDialogueHidden(0);
    }
    static_cast<W8NpcDialogueOptionsPanel*>(g_npc_interaction_state->dialogue_panels[0])->SetEnabled(1);
    g_npc_interaction_state->dialogue_panels[1]->SetEnabled(1);
    g_npc_interaction_state->dialogue_panels[4]->SetEnabled(1);
    RegionSetEnable(0x18);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_38])->SetActive(1);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_NPC_NAME])->m_textBuffer.SetText(
        g_npc_interaction_state->dialogue_npc->record->source_name, g_wiz_text_bold_font);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_NPC_NAME])->Invalidate(1);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_38])->m_textBuffer.SetText(gppStringList[0x726],
                                                                     g_wiz_text_bold_font);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])->m_textBuffer.SetText(gppStringList[0x727],
                                                                     g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])->m_primaryActivationCallback =
        SelectNpcDialogueService;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])->m_textBuffer.SetText(gppStringList[0x728],
                                                                     g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])->m_primaryActivationCallback = SelectNpcDialogueTalk;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_3])->m_textBuffer.SetText(gppStringList[0x729],
                                                                     g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_3])->m_primaryActivationCallback = SelectNpcDialogueExit;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])->m_textBuffer.SetText(gppStringList[0x723],
                                                                     g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])->m_primaryActivationCallback =
        PromptNpcDispositionChange;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])->m_textBuffer.SetText(gppStringList[0x724],
                                                                     g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])->m_primaryActivationCallback = EnterNpcServiceLayout;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_6])->m_textBuffer.SetText(gppStringList[0x725],
                                                                     g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_6])->m_primaryActivationCallback =
        LeaveNpcDialogueLayout;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_7])->SetActive(0);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])
        ->SetEnabled(!g_npc_interaction_state->dialogue_npc->talk_cooldown_active);
    if (!g_npc_interaction_state->dialogue_npc->trade_cooldown_active) {
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])->SetEnabled(1);
    } else {
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])->SetEnabled(0);
    }
    RequestRedraw(W8_MAIN_REDRAW_LAYOUT);
    SelectTextBox(3);
}

/* Open the mode-3 NPC dialogue layout: the caption resolves through the
   alternate-name style, the full text-control grid is loaded with its labels
   and callbacks, the transcript controller replays and re-expands, the scroll
   widgets follow the expansion state and the party portrait regions are
   parked for the duration. */
/* Fold the mode-2 menu panels away: clear the option row, drop the caption
   text, remember the layout being left and unhide the dialogue window. */
// FUNCTION: WIZ8 0x00570A20
void CloseNpcDialogueLayout(void)
{
    RegionSetDisable(0x18);
    static_cast<W8NpcDialogueOptionsPanel*>(g_npc_interaction_state->dialogue_panels[0])->SetEnabled(0);
    g_npc_interaction_state->dialogue_panels[1]->SetEnabled(0);
    g_npc_interaction_state->dialogue_panels[4]->SetEnabled(0);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_38])->SetActive(0);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_38])->m_textBuffer.SetText(&g_empty_wide_string,
                                                                     g_wiz_text_bold_font);
    SetNpcDialogueLayoutMode(W8_DIALOGUE_LAYOUT_NONE);
}

/* The "Trade" option button: hand the NPC's trade answer to ApplyNpcInteraction,
   then react to the disposition band - friendly queues the trade quote and
   returns to the transcript, neutral queues the neutral answer, hostile sets
   the hostile band and queues the refusal. */
// FUNCTION: WIZ8 0x00570AD0
void SelectNpcDialogueService(void)
{
    unsigned char band;

    ApplyNpcInteraction(g_npc_interaction_state->dialogue_npc, 1,
                        g_npc_interaction_state->dialogue_speaker, 0, 0);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])->SetEnabled(0);
    band = GetNpcDispositionBand(g_npc_interaction_state->dialogue_npc);
    if (band == 0) {
        QueueNpcScriptLine(4, 0, 0, 0);
        QueueNpcMessageLine(W8_NPC_MSG_REOPEN_TRANSCRIPT, 0);
        return;
    }
    if (band == 1) {
        QueueNpcScriptLine(6, 0, 0, 0);
        return;
    }
    SetNpcDispositionBand(g_npc_interaction_state->dialogue_npc, 2);
    QueueNpcScriptLine(0x18, 0, 0, 0);
    QueueNpcMessageLine(W8_NPC_MSG_CLOSE_RESUME_NPC, 0);
}

/* The "Talk" option button: friendly NPCs leave the dialogue outright,
   neutral ones queue the talk quote, hostile ones set the hostile band and
   queue the refusal. */
// FUNCTION: WIZ8 0x00570B80
void SelectNpcDialogueTalk(void)
{
    unsigned char band;

    ApplyNpcInteraction(g_npc_interaction_state->dialogue_npc, 0,
                        g_npc_interaction_state->dialogue_speaker, 0, 0);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])->SetEnabled(0);
    band = GetNpcDispositionBand(g_npc_interaction_state->dialogue_npc);
    if (band == 0) {
        HandleNpcDialogueDeparture(0);
        OpenNpcDialogueTranscriptLayout();
        return;
    }
    if (band == 1) {
        QueueNpcScriptLine(3, 0, 0, 0);
        return;
    }
    SetNpcDispositionBand(g_npc_interaction_state->dialogue_npc, 2);
    QueueNpcScriptLine(0x18, 0, 0, 0);
    QueueNpcMessageLine(W8_NPC_MSG_CLOSE_RESUME_NPC, 0);
}

/* The "Exit" option button: close the option row, mark the dialogue as leaving
   through the transcript and open the option layout for the farewell. */
// FUNCTION: WIZ8 0x00570C20
void SelectNpcDialogueExit(void)
{
    CloseNpcDialogueLayout();
    g_npc_interaction_state->trade_mode = W8_NPC_TRADE_GIVE;
    g_npc_interaction_state->reopen_topics = 1;
    OpenNpcDialogueOptionLayout();
}

static void SyncNpcDialogueTranscriptScrollButtons()
{
    if (static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2])->IsExpanded()) {
        g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_SCROLL_UP_BUTTON]->SetEnabled(1);
        g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_SCROLL_DOWN_BUTTON]->SetEnabled(1);
    } else {
        g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_SCROLL_UP_BUTTON]->SetEnabled(0);
        g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_SCROLL_DOWN_BUTTON]->SetEnabled(0);
    }
}

// FUNCTION: WIZ8 0x00570CF0
void OpenNpcDialogueTranscriptLayout(void)
{
    int index;

    g_npc_interaction_state->dialogue_layout = W8_DIALOGUE_LAYOUT_TRANSCRIPT;
    if (g_npc_interaction_state->dialogue_hidden != 0) {
        SetNpcDialogueHidden(0);
    }
    static_cast<W8NpcDialogueOptionsPanel*>(g_npc_interaction_state->dialogue_panels[0])->SetEnabled(1);
    g_npc_interaction_state->dialogue_panels[1]->SetEnabled(1);
    static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2])->SetEnabled(1);
    g_npc_interaction_state->dialogue_panels[3]->SetEnabled(1);
    static_cast<W8NpcTypedDialoguePanel*>(g_npc_interaction_state->dialogue_panels[6])->SetEnabled(1);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_38])->SetActive(0);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_38])->m_textBuffer.SetText(&g_empty_wide_string,
                                                                     g_wiz_text_bold_font);
    RegionSetEnable(0x18);
    RegionSetEnable(0x16);
    if (g_npc_interaction_state->dialogue_npc->name_style == 0x32) {
        swprintf(g_status.monster_name_buffer, L"Al-%s",
                 g_status.buffers.Char[g_status.sedexus_party_slot].name);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_NPC_NAME])->m_textBuffer.SetText(
            g_status.monster_name_buffer, g_wiz_text_bold_font);
    } else {
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_NPC_NAME])->m_textBuffer.SetText(
            g_npc_interaction_state->dialogue_npc->record->source_name, g_wiz_text_bold_font);
    }
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])->m_textBuffer.SetText(gppStringList[0x735],
                                                                     g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])->m_primaryActivationCallback = EnterNpcTradeOptions;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])->m_textBuffer.SetText(gppStringList[0x738],
                                                                     g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])->m_primaryActivationCallback = ShowNpcDialogueNotice;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_3])->m_textBuffer.SetText(gppStringList[0x724],
                                                                     g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_3])->m_primaryActivationCallback = EnterNpcServiceLayout;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])->m_textBuffer.SetText(gppStringList[0x737],
                                                                     g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])->m_primaryActivationCallback = RequestNpcJoinParty;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])->m_textBuffer.SetText(gppStringList[0x723],
                                                                     g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])->m_primaryActivationCallback =
        PromptNpcDispositionChange;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_6])->m_textBuffer.SetText(gppStringList[0x725],
                                                                     g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_6])->m_primaryActivationCallback =
        LeaveNpcDialogueLayout;
    static_cast<W8NpcDialogueScrollWidget*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_SCROLL])->m_primaryActivationCallback = NoOp;
    g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_SCROLL_UP_BUTTON]->m_leftButtonDownCallback =
        ScrollNpcDialogueUp;
    g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_SCROLL_DOWN_BUTTON]->m_leftButtonDownCallback =
        ScrollNpcDialogueDown;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_12])->m_primaryActivationCallback =
        SubmitNpcDialogueKeyword;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_13])->m_primaryActivationCallback =
        RefreshNpcDialogueTranscript;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_SORT_BUTTON])->m_textBuffer.SetText(gppStringList[0x740],
                                                                        g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_SORT_BUTTON])->m_primaryActivationCallback =
        SyncNpcDialogueListFilter;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_PEOPLE_BUTTON])->m_textBuffer.SetText(
        gppStringList[0x743], g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(
        g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_PEOPLE_BUTTON])
        ->m_primaryActivationCallback = SelectNpcPeopleTopics;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_PLACES_BUTTON])->m_textBuffer.SetText(
        gppStringList[0x744], g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(
        g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_PLACES_BUTTON])
        ->m_primaryActivationCallback = SelectNpcPlaceTopics;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_ITEMS_BUTTON])->m_textBuffer.SetText(gppStringList[0x745],
                                                                         g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(
        g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_ITEMS_BUTTON])
        ->m_primaryActivationCallback = SelectNpcItemTopics;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_MISC_BUTTON])->m_textBuffer.SetText(gppStringList[0x746],
                                                                        g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(
        g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_MISC_BUTTON])
        ->m_primaryActivationCallback = SelectNpcMiscTopics;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_ALL_BUTTON])->m_textBuffer.SetText(gppStringList[0x747],
                                                                       g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_ALL_BUTTON])->m_primaryActivationCallback =
        SelectNpcDialogueCategoryAll;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_7])->SetActive(0);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])->SetEnabled(1);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])->SetEnabled(1);
    static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2])->RestoreTranscriptEntries();
    static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2])->SetTranscriptCategoryFilter(
        g_npc_interaction_state->dialogue_category_filter);
    static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2])->SetTranscriptSorted(
        g_npc_interaction_state->transcript_sorted);
    static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2])->Expand();
    SyncNpcDialogueTranscriptScrollButtons();
    SyncDialogueCategoryButtons();
    if (g_npc_interaction_state->transcript_sorted != 0) {
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_SORT_BUTTON])->EnableSecondaryState(1);
    } else {
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_SORT_BUTTON])->DisableSecondaryState(1);
    }
    RequestRedraw(W8_MAIN_REDRAW_LAYOUT);
    for (index = 0; index < 8; ++index) {
        if (g_status.buffers.XChar[index].fOccupied) {
            RegionSetDisable(7 + index);
            DisableRegionSetInput(7 + index);
            DisableRegionInput(0x5a + index);
        }
    }
    if (g_npc_interaction_state->dialogue_npc->record->merchant != 0) {
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])->SetEnabled(0);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_3])->SetEnabled(0);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])->SetEnabled(0);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])->SetEnabled(0);
    }
    if (g_npc_interaction_state->dialogue_npc->name_style == 0x17) {
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])->SetEnabled(0);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_3])->SetEnabled(0);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])->SetEnabled(0);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])->SetEnabled(0);
    }
    g_npc_interaction_state->transcript_open_count++;
    SelectTextBox(3);
}

/* Tear down the mode-3 transcript layout: fold the dialogue text controller
   back up, drop the panels, then re-enable whichever occupied party rows
   still own region slots. */
// FUNCTION: WIZ8 0x00571370
void CloseNpcDialogueTranscriptLayout(void)
{
    int index;

    static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2])->SaveTranscriptEntries();
    static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2])->Collapse();
    static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2])->ClearTranscriptEntries();
    RegionSetDisable(0x18);
    RegionSetDisable(0x16);
    static_cast<W8NpcDialogueOptionsPanel*>(g_npc_interaction_state->dialogue_panels[0])->SetEnabled(0);
    g_npc_interaction_state->dialogue_panels[1]->SetEnabled(0);
    static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2])->SetEnabled(0);
    g_npc_interaction_state->dialogue_panels[3]->SetEnabled(0);
    static_cast<W8NpcTypedDialoguePanel*>(g_npc_interaction_state->dialogue_panels[6])->SetEnabled(0);
    SetNpcDialogueLayoutMode(W8_DIALOGUE_LAYOUT_NONE);
    static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2])->ClearBackground();
    ClearSurfaceRect(0x1dc, 0x11b, 0x269, 0x1c2);
    InvalidateRegion(0x1dc, 0x11b, 0x269, 0x1c2, 0);
    for (index = 0; index < 8; ++index) {
        if (g_status.buffers.XChar[index].fOccupied) {
            RegionSetEnable(7 + index);
            EnableRegionSetInput(7 + index);
            EnableRegionInput(0x5a + index);
        }
    }
}

// FUNCTION: WIZ8 0x005714D0
void RequestNpcJoinParty(void)
{
    unsigned int slot;

    ShowNotice(0xa, gppStringList[0x748], 3, GetTextBoxScrollRange(), 0);
    if (g_npc_interaction_state->dialogue_hidden != 0) {
        SetNpcDialogueHidden(0);
    }
    if (g_npc_interaction_state->dialogue_npc->record->has_group != 0) {
        slot = FindFreePartySlot(0, 2);
        if (slot == 0xffffffff) {
            QueueNpcScriptLine(0xc, 0, 0, 0);
            return;
        }
        if (CanNpcJoinParty(g_npc_interaction_state->dialogue_npc)) {
            QueueNpcScriptLine(0xd, 0, 0, 0);
            QueueNpcMessageLine(W8_NPC_MSG_FOCUS_NPC,
                                g_npc_interaction_state->dialogue_npc->name_style);
            return;
        }
    }
    QueueNpcScriptLine(0xb, 0, 0, 0);
}

// FUNCTION: WIZ8 0x005715A0
void ShowNpcDialogueNotice(void)
{
    ShowNotice(0xa, gppStringList[0x749], 3, GetTextBoxScrollRange(), 0);
    SetNpcDialogueHidden(1);
}

// FUNCTION: WIZ8 0x005715D0
void EnterNpcTradeOptions(void)
{
    CloseNpcDialogueTranscriptLayout();
    g_npc_interaction_state->trade_mode = W8_NPC_TRADE_BUY;
    OpenNpcDialogueOptionLayout();
}

// FUNCTION: WIZ8 0x005715F0
void ScrollNpcDialogueUp(void)
{
    static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2])->HandleScrollUpCommand(0);
}

// FUNCTION: WIZ8 0x00571610
void ScrollNpcDialogueDown(void)
{
    static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2])->HandleScrollDownCommand(0);
}

// FUNCTION: WIZ8 0x00571630
void SubmitNpcDialogueKeyword(void)
{
    wchar_t keyword[200];

    Get16BitStringFromField(0, keyword);
    AddNpcDialogueKeyword(keyword, -1, 0);
}

// FUNCTION: WIZ8 0x00571660
void AddNpcDialogueKeyword(wchar_t* text, signed char category, int play_chime)
{
    W8NpcDialogueTextController* controller;
    unsigned int index;

    if (wcslen(text) == 0) {
        return;
    }
    controller = static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2]);
    StripNpcKeywordPunctuation(text);
    if (category == W8_DIALOGUE_CATEGORY_ALL) {
        for (index = 0; index < gXStatus.uiItemsInDatabase; ++index) {
            if (CompareWideTextIgnoreAsciiCase(text, g_item_records[index].display_name) == 0) {
                break;
            }
        }
        if (index < gXStatus.uiItemsInDatabase) {
            category = W8_DIALOGUE_CATEGORY_ITEMS;
        } else {
            for (index = 0; index < gXStatus.uiNpcsInDatabase; ++index) {
                if (CompareWideTextIgnoreAsciiCase(text, g_npc_records[index].source_name) ==
                    0) {
                    break;
                }
            }
            if (index < gXStatus.uiNpcsInDatabase) {
                category = W8_DIALOGUE_CATEGORY_PEOPLE;
            } else {
                for (index = 0; g_dialogue_person_keywords[index][0] != 0; ++index) {
                    if (CompareWideTextIgnoreAsciiCase(text, g_dialogue_person_keywords[index]) ==
                        0) {
                        break;
                    }
                }
                if (g_dialogue_person_keywords[index][0] != 0) {
                    category = W8_DIALOGUE_CATEGORY_PEOPLE;
                } else {
                    for (index = 0;
                         index < static_cast<unsigned int>(g_dialogue_place_keyword_count);
                         ++index) {
                        if (CompareWideTextIgnoreAsciiCase(
                                text, gppStringList[g_dialogue_place_keyword_ids[index]]) == 0) {
                            break;
                        }
                    }
                    if (index < static_cast<unsigned int>(g_dialogue_place_keyword_count)) {
                        category = W8_DIALOGUE_CATEGORY_PLACES;
                    } else {
                        category = W8_DIALOGUE_CATEGORY_MISC;
                    }
                }
            }
        }
    }
    if (controller != 0) {
        if (controller->AddTranscriptEntry(text, category, play_chime) != 0) {
            if (g_npc_interaction_state->dialogue_category_filter != W8_DIALOGUE_CATEGORY_ALL &&
                g_npc_interaction_state->dialogue_category_filter != category) {
                g_npc_interaction_state->dialogue_category_filter = category;
                static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2])->SetTranscriptCategoryFilter(
                    g_npc_interaction_state->dialogue_category_filter);
                SyncDialogueCategoryButtons();
            }
            if (play_chime != 0) {
                SoundPlay(reinterpret_cast<STR>(const_cast<char*>( // reinterpret-ok: SGP text ABI
                              "Data\\Sound\\Misc\\Keyword Chime.wav")),
                          0);
            }
            controller->Collapse();
            controller->Expand();
        }
        controller->Invalidate(0);
        SyncNpcDialogueTranscriptScrollButtons();
    }
}

// FUNCTION: WIZ8 0x00571880
void RefreshNpcDialogueTranscript(void)
{
    static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2])->RemoveSelectedTranscriptEntry();
    static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2])->Collapse();
    static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2])->Expand();
    SyncNpcDialogueTranscriptScrollButtons();
}

// FUNCTION: WIZ8 0x00571920
void SyncNpcDialogueListFilter(void)
{
    if (static_cast<unsigned char>(static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_SORT_BUTTON])->m_stateFlags &
                                   g_W8TextControlStateSecondary) != 0) {
        g_npc_interaction_state->transcript_sorted = 1;
    } else {
        g_npc_interaction_state->transcript_sorted = 0;
    }
    static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2])->SetTranscriptSorted(
        g_npc_interaction_state->transcript_sorted);
}

// FUNCTION: WIZ8 0x00571960
void SelectNpcPeopleTopics(void)
{
    if (static_cast<unsigned char>(static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_PEOPLE_BUTTON])->m_stateFlags &
                                   g_W8TextControlStateSecondary) != 0) {
        g_npc_interaction_state->dialogue_category_filter = W8_DIALOGUE_CATEGORY_PEOPLE;
        SyncDialogueCategoryButtons();
    }
    static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2])->SetTranscriptCategoryFilter(
        W8_DIALOGUE_CATEGORY_PEOPLE);
}

// FUNCTION: WIZ8 0x005719A0
void SelectNpcPlaceTopics(void)
{
    if (static_cast<unsigned char>(static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_PLACES_BUTTON])->m_stateFlags &
                                   g_W8TextControlStateSecondary) != 0) {
        g_npc_interaction_state->dialogue_category_filter = W8_DIALOGUE_CATEGORY_PLACES;
        SyncDialogueCategoryButtons();
    }
    static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2])->SetTranscriptCategoryFilter(
        W8_DIALOGUE_CATEGORY_PLACES);
}

// FUNCTION: WIZ8 0x005719E0
void SelectNpcItemTopics(void)
{
    if (static_cast<unsigned char>(static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_ITEMS_BUTTON])->m_stateFlags &
                                   g_W8TextControlStateSecondary) != 0) {
        g_npc_interaction_state->dialogue_category_filter = W8_DIALOGUE_CATEGORY_ITEMS;
        SyncDialogueCategoryButtons();
    }
    static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2])->SetTranscriptCategoryFilter(
        W8_DIALOGUE_CATEGORY_ITEMS);
}

// FUNCTION: WIZ8 0x00571A20
void SelectNpcDialogueCategoryAll(void)
{
    if (static_cast<unsigned char>(static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_ALL_BUTTON])->m_stateFlags &
                                   g_W8TextControlStateSecondary) != 0) {
        g_npc_interaction_state->dialogue_category_filter = W8_DIALOGUE_CATEGORY_ALL;
        SyncDialogueCategoryButtons();
    }
    static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2])->SetTranscriptCategoryFilter(
        W8_DIALOGUE_CATEGORY_ALL);
}

// FUNCTION: WIZ8 0x00571A60
void SelectNpcMiscTopics(void)
{
    if (static_cast<unsigned char>(static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_MISC_BUTTON])->m_stateFlags &
                                   g_W8TextControlStateSecondary) != 0) {
        g_npc_interaction_state->dialogue_category_filter = W8_DIALOGUE_CATEGORY_MISC;
        SyncDialogueCategoryButtons();
    }
    static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2])->SetTranscriptCategoryFilter(
        W8_DIALOGUE_CATEGORY_MISC);
}

/* Open the mode-4 trade-option layout: the six option controls get their
   labels, callbacks and secondary state, the party gold is formatted into the
   purse readout and the option-button row is cleared before the refresh. */
// FUNCTION: WIZ8 0x00571AA0
void OpenNpcDialogueOptionLayout(void)
{
    wchar_t buffer[0x20];
    int index;

    g_npc_interaction_state->dialogue_layout = W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX;
    if (g_npc_interaction_state->dialogue_hidden != 0) {
        SetNpcDialogueHidden(0);
    }
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_38])->SetActive(0);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_38])->m_textBuffer.SetText(&g_empty_wide_string,
                                                                     g_wiz_text_bold_font);
    static_cast<W8NpcDialogueOptionsPanel*>(g_npc_interaction_state->dialogue_panels[0])->SetEnabled(1);
    g_npc_interaction_state->dialogue_panels[1]->SetEnabled(1);
    g_npc_interaction_state->dialogue_panels[5]->SetEnabled(1);
    RegionSetEnable(0x18);
    RegionSetEnable(0x17);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_34])->m_textBuffer.SetText(gppStringList[0x72d],
                                                                     g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])->m_textBuffer.SetText(gppStringList[0x73d],
                                                                     g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])
        ->m_primaryActivationCallback = SelectNpcBuyMode;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])->m_textBuffer.SetText(gppStringList[0x73e],
                                                                     g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])
        ->m_primaryActivationCallback = SelectNpcSellMode;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_3])->m_textBuffer.SetText(gppStringList[0x73f],
                                                                     g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_3])
        ->m_primaryActivationCallback = SelectNpcGiveMode;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])->m_textBuffer.SetText(gppStringList[0x72e],
                                                                     g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])
        ->m_primaryActivationCallback = SelectNpcShopliftMode;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])->m_textBuffer.SetText(gppStringList[0x72b],
                                                                     g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])
        ->m_primaryActivationCallback = SelectNpcPartyItems;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_6])->m_textBuffer.SetText(gppStringList[0x72c],
                                                                     g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_6])
        ->m_primaryActivationCallback = SelectNpcStockItems;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_24])->m_primaryActivationCallback = OpenNpcItemAssay;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_24])->m_secondaryActivationCallback = OpenNpcItemAssay;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_31])->SetEnabled(0);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_31])->m_primaryActivationCallback = ConfirmNpcTradeSlot;
    swprintf(buffer, L"%dg", g_status.party_gold);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_33])->m_textBuffer.SetText(buffer,
                                                                     g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_37])->SetEnabled(0);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_37])->m_primaryActivationCallback = ConfirmNpcTradeItem;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])->AddLayoutFlags(g_W8TextControlLayoutToggle);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_6])->AddLayoutFlags(g_W8TextControlLayoutToggle);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])->DisableSecondaryState(1);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_6])->DisableSecondaryState(1);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])->AddLayoutFlags(g_W8TextControlLayoutToggle);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])->AddLayoutFlags(g_W8TextControlLayoutToggle);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_3])->AddLayoutFlags(g_W8TextControlLayoutToggle);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])->AddLayoutFlags(g_W8TextControlLayoutToggle);
    UpdateNpcDialogueSubMode();
    g_npc_interaction_state->pending_trade_toggle = 1;
    if (g_npc_interaction_state->reopen_topics) {
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])->SetEnabled(0);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])->SetEnabled(0);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])->SetEnabled(0);
    } else {
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])->SetEnabled(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])->SetEnabled(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])->SetEnabled(1);
    }
    if (g_npc_interaction_state->dialogue_npc->record->owns_stock == 0) {
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])->SetEnabled(0);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])->SetEnabled(0);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])->SetEnabled(0);
    }
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_3])->SetEnabled(1);
    SelectTextBox(2);
    if (!gXStatus.fCampMode) {
        ResetEditorStatusLine(2);
    }
    g_npc_interaction_state->trade_filter = 0;
    for (index = 0; index < 6; ++index) {
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + index])->SetEnabled(0);
    }
    RebuildNpcTradeItemList(1);
    RequestRedraw(W8_MAIN_REDRAW_LAYOUT);
}

// FUNCTION: WIZ8 0x00571F60
void UpdateNpcDialogueSubMode(void)
{
    switch (g_npc_interaction_state->trade_mode) {
    case W8_NPC_TRADE_GIVE:
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_3])->EnableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])->DisableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])->DisableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])->DisableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_NPC_NAME])->m_textBuffer.SetText(gppStringList[0x73b],
                                                                         g_wiz_text_bold_font);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_37])->m_textBuffer.SetText(gppStringList[0x73b],
                                                                         g_wiz_text_bold_font);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])->SetEnabled(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_6])->SetEnabled(1);
        break;
    case W8_NPC_TRADE_SELL:
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])->EnableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])->DisableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_3])->DisableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])->DisableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])->SetEnabled(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_6])->SetEnabled(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_NPC_NAME])->m_textBuffer.SetText(gppStringList[0x73a],
                                                                         g_wiz_text_bold_font);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_37])->m_textBuffer.SetText(gppStringList[0x73a],
                                                                         g_wiz_text_bold_font);
        break;
    case W8_NPC_TRADE_BUY:
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])->EnableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])->DisableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_3])->DisableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])->DisableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_NPC_NAME])->m_textBuffer.SetText(gppStringList[0x73c],
                                                                         g_wiz_text_bold_font);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_37])->m_textBuffer.SetText(gppStringList[0x73c],
                                                                         g_wiz_text_bold_font);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])->SetEnabled(0);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_6])->SetEnabled(0);
        break;
    case W8_NPC_TRADE_SHOPLIFT:
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])->EnableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])->DisableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_3])->DisableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])->DisableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_NPC_NAME])->m_textBuffer.SetText(gppStringList[0x72e],
                                                                         g_wiz_text_bold_font);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_37])->m_textBuffer.SetText(gppStringList[0x72e],
                                                                         g_wiz_text_bold_font);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])->SetEnabled(0);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_6])->SetEnabled(0);
        break;
    default:
        break;
    }
    if (g_npc_interaction_state->pending_trade_toggle &&
        (g_npc_interaction_state->trade_mode == W8_NPC_TRADE_SELL ||
         g_npc_interaction_state->trade_mode == W8_NPC_TRADE_GIVE)) {
        if (g_npc_interaction_state->trade_pc_items) {
            static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_6])->DisableSecondaryState(1);
            static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_6])->m_textBuffer.SetFontStateIndex(-1);
            static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_6])->m_textBuffer.SetGeometryDirty();
            static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])->EnableSecondaryState(1);
            static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])->m_textBuffer.SetFontStateIndex(3);
            static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])->m_textBuffer.SetGeometryDirty();
        } else {
            static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])->DisableSecondaryState(1);
            static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])->m_textBuffer.SetFontStateIndex(-1);
            static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])->m_textBuffer.SetGeometryDirty();
            static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_6])->EnableSecondaryState(1);
            static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_6])->m_textBuffer.SetFontStateIndex(3);
            static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_6])->m_textBuffer.SetGeometryDirty();
        }
        g_npc_interaction_state->pending_trade_toggle = 0;
    }
    RebuildNpcTradeItemList(1);
}

/* Tear down the mode-4 option layout: the six option controls lose their
   secondary state and layout flags before everything is disabled. */
// FUNCTION: WIZ8 0x00572320
void CloseNpcDialogueOptionLayout(void)
{
    W8TextControl* text;

    ResetNpcDialogueItemEditor();
    if (!gXStatus.fCampMode) {
        g_npc_interaction_state->trade_mode = W8_NPC_TRADE_NONE;
    }
    RegionSetDisable(0x18);
    RegionSetDisable(0x17);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_31])->SetEnabled(1);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])->DisableSecondaryState(1);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])->DisableSecondaryState(1);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_3])->DisableSecondaryState(1);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])->DisableSecondaryState(1);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])->DisableSecondaryState(1);
    text = static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5]);
    text->m_textBuffer.SetFontStateIndex(-1);
    text->m_textBuffer.SetGeometryDirty();
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_6])->DisableSecondaryState(1);
    text = static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_6]);
    text->m_textBuffer.SetFontStateIndex(-1);
    text->m_textBuffer.SetGeometryDirty();
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])->RemoveLayoutFlags(g_W8TextControlLayoutToggle);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])->RemoveLayoutFlags(g_W8TextControlLayoutToggle);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_3])->RemoveLayoutFlags(g_W8TextControlLayoutToggle);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])->RemoveLayoutFlags(g_W8TextControlLayoutToggle);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])->RemoveLayoutFlags(g_W8TextControlLayoutToggle);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_6])->RemoveLayoutFlags(g_W8TextControlLayoutToggle);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])->SetEnabled(1);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])->SetEnabled(1);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_3])->SetEnabled(1);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])->SetEnabled(1);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])->SetEnabled(1);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_6])->SetEnabled(1);
    static_cast<W8NpcDialogueOptionsPanel*>(g_npc_interaction_state->dialogue_panels[0])->SetEnabled(0);
    g_npc_interaction_state->dialogue_panels[1]->SetEnabled(0);
    g_npc_interaction_state->dialogue_panels[5]->SetEnabled(0);
    SetNpcDialogueLayoutMode(W8_DIALOGUE_LAYOUT_NONE);
    g_npc_interaction_state->reopen_topics = 0;
    SelectTextBox(3);
    ScrollDialogueTextBoxToLine();
}

// FUNCTION: WIZ8 0x00572590
void OpenNpcItemAssay(void)
{
    W8AssayDialog* dialog;

    if (g_npc_interaction_state->trade_item != 0 &&
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_24])->m_imageObject != -1 &&
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_24])->m_imageObject != 0x1ac) {
        dialog = new W8AssayDialog(g_npc_interaction_state->trade_item,
                                   &g_status.buffers.Char[g_status.selected_character]);
        dialog->SetText(&g_empty_wide_string);
        dialog->SetOrigin(g_info_dialog_x, 0x48);
        dialog->m_destroy_callback = OnNpcAssayDialogClosed;
        OpenModal(dialog);
    }
}

// FUNCTION: WIZ8 0x00572670
void OnNpcAssayDialogClosed(W8DialogBase*)
{
    RequestRedraw(W8_MAIN_REDRAW_ALL);
}

// FUNCTION: WIZ8 0x00572680
void SelectNpcPartyItems(void)
{
    ResetNpcDialogueItemEditor();
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_6])->DisableSecondaryState(1);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_6])->m_textBuffer.SetFontStateIndex(-1);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_6])->m_textBuffer.SetGeometryDirty();
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])->EnableSecondaryState(1);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])->m_textBuffer.SetFontStateIndex(3);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])->m_textBuffer.SetGeometryDirty();
    RebuildNpcTradeItemList(1);
    g_npc_interaction_state->trade_pc_items = 1;
}

// FUNCTION: WIZ8 0x00572700
void SelectNpcStockItems(void)
{
    ResetNpcDialogueItemEditor();
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])->DisableSecondaryState(1);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])->m_textBuffer.SetFontStateIndex(-1);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])->m_textBuffer.SetGeometryDirty();
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_6])->EnableSecondaryState(1);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_6])->m_textBuffer.SetFontStateIndex(3);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_6])->m_textBuffer.SetGeometryDirty();
    RebuildNpcTradeItemList(1);
    g_npc_interaction_state->trade_pc_items = 0;
}

// FUNCTION: WIZ8 0x00572960
void ConfirmNpcTradeSlot(void)
{
    int slot;

    if (g_npc_interaction_state->dialogue_layout == W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX) {
        slot = GetSelectedTextLine(2);
        if (slot != -1) {
            UpdateNpcTradeSelection(slot, 0, 0);
            if (g_npc_interaction_state->trade_mode == W8_NPC_TRADE_GIVE && slot == 0) {
                OpenNpcGoldAmountDialog();
            } else if (g_npc_interaction_state->trade_item->stack_count > 1) {
                OpenNpcTradeQuantityDialog();
            }
        }
    }
}

/* Open the mode-5 dialogue layout: the smaller control set shares the
   mode-4 labels and callbacks, the purse readout is refreshed and the
   mode-4-only controls are parked. */
/* The "Gold" split-amount entry: reset the item editor, dim the six option
   buttons and open the W8SplitAmountDialog seeded with the party purse. The
   result lands back through OnNpcTradeSplitDialogDestroy. */
// FUNCTION: WIZ8 0x00572780
void OpenNpcGoldAmountDialog(void)
{
    W8SplitAmountDialog* dialog;

    ResetNpcDialogueItemEditor();
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])->DisableSecondaryState(1);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])->DisableSecondaryState(1);
    for (int index = 0; index < 6; ++index) {
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + index])->SetEnabled(0);
    }
    g_npc_interaction_state->dialogue_panels[1]->Invalidate(0);
    dialog = new W8SplitAmountDialog(g_status.party_gold);
    dialog->SetText(&g_empty_wide_string);
    dialog->SetOrigin(g_split_dialog_origin_x, g_split_dialog_origin_y);
    dialog->m_destroy_callback = OnNpcTradeSplitDialogDestroy;
    OpenModal(dialog);
}

/* Destroy callback for the trade split-amount dialog: on confirm it takes the
   entered share into trade_gold and refreshes the purse readout. */
// FUNCTION: WIZ8 0x00572870
void OnNpcTradeSplitDialogDestroy(W8DialogBase* dialog)
{
    if (static_cast<W8SplitAmountDialog*>(dialog)->m_result != g_split_dialog_confirm) {
        return;
    }
    g_npc_interaction_state->trade_gold = static_cast<W8SplitAmountDialog*>(dialog)->m_taken;
    ShowNpcTradeGold();
}

static void HighlightNpcTradeQuantity()
{
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_24])
        ->m_textBuffer.m_highlighted = true;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_24])
        ->m_textBuffer.SetGeometryDirty();
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_24])
        ->Invalidate(0);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_35])
        ->m_textBuffer.m_highlighted = true;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_35])
        ->m_textBuffer.SetGeometryDirty();
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_35])
        ->Invalidate(0);
    g_trade_highlight_tick = GetTickCount();
}

/* Resolve and select one trade-list row. trade_mode picks the pool: mode 2's
   leading row is the purse readout, the mode-3 views iterate either the
   selected character's backpack or the shared party item pool, and modes 4/5
   go through the NPC's own inventory. pick/commit drive quantity stepping,
   the click chime and the highlight tick. */
static void ShowSelectedNpcTradeItem(const W8ItemInstance* item)
{
    wchar_t count_text[32];
    const wchar_t* text;
    int image = g_item_video_objects.GetOrCreateVideoObject(item->iItemNo);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_24])
        ->SetImage(image);
    if (item->stack_count < 2) {
        text = g_dialogue_empty_text;
    } else {
        swprintf(count_text, L"%d", g_npc_interaction_state->trade_quantity);
        text = count_text;
    }
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_24])
        ->m_textBuffer.SetText(text, g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_24])
        ->Invalidate(1);
}

static void UpdatePartyTradeQuantity(const W8ItemInstance* item, int row, bool decrement)
{
    if (g_item_records[item->iItemNo].equip_class == W8_ITEM_EQUIP_CLASS_AMMUNITION) {
        g_npc_interaction_state->trade_quantity = item->stack_count;
    } else if (g_npc_interaction_state->selected_trade_row == row) {
        if (!decrement) {
            if (g_item_records[item->iItemNo].quantity_kind == W8_ITEM_QUANTITY_STACK) {
                ++g_npc_interaction_state->trade_quantity;
                if (item->stack_count < g_npc_interaction_state->trade_quantity) {
                    g_npc_interaction_state->trade_quantity = item->stack_count;
                }
            }
        } else {
            --g_npc_interaction_state->trade_quantity;
            if (g_npc_interaction_state->trade_quantity == 0) {
                g_npc_interaction_state->trade_quantity = 1;
            }
        }
    } else {
        g_npc_interaction_state->trade_quantity = 1;
    }
}

// FUNCTION: WIZ8 0x005729C0
W8ItemInstance* ResolveNpcTradeRow(int index, bool pick, char decrement, char commit)
{
    W8ItemInstance* pool;
    W8NpcItemEntry* entry;
    int selected;
    int hit;
    int i;

    selected = g_status.selected_character;
    hit = 0;
    if (g_npc_interaction_state->trade_mode != W8_NPC_TRADE_BUY &&
        g_npc_interaction_state->trade_mode != W8_NPC_TRADE_SHOPLIFT) {
        if (g_npc_interaction_state->trade_mode == W8_NPC_TRADE_GIVE) {
            g_npc_interaction_state->trade_gold = 0;
            if (index == 0) {
                g_npc_interaction_state->trade_gold = g_status.party_gold;
                static_cast<W8TextControl*>(
                    g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_24])
                    ->SetImage(0x1ac);
                static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_24])->m_textBuffer.SetText(
                    g_dialogue_empty_text, g_wiz_text_font_secondary);
                return 0;
            }
            --index;
        }
        if (static_cast<unsigned char>(static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])->m_stateFlags &
                                       g_W8TextControlStateSecondary) == 0) {
            if (static_cast<unsigned char>(
                    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_6])->m_stateFlags &
                    g_W8TextControlStateSecondary) != 0 &&
                g_status.party_item_count != 0) {
                i = 0;
                do {
                    if (g_status.party_item_pool[i].iItemNo != -1 &&
                        !NpcTradeItemAllowed(&g_status.party_item_pool[i])) {
                        if (index == hit) {
                            if (!pick) {
                                return &g_status.party_item_pool[i];
                            }
                            if (commit != 0) {
                                UpdatePartyTradeQuantity(&g_status.party_item_pool[i], hit,
                                                         decrement != 0);
                            }
                            g_npc_interaction_state->selected_trade_row = hit;
                            if (commit != 0) {
                                SoundPlay(g_button_click_1, 0);
                            }
                            ShowSelectedNpcTradeItem(&g_status.party_item_pool[i]);
                            return &g_status.party_item_pool[i];
                        }
                        ++hit;
                    }
                    ++i;
                    if (g_status.party_item_count <= static_cast<unsigned int>(i)) {
                        return 0;
                    }
                } while (true);
            }
        } else {
            W8Character* character = &g_status.buffers.Char[selected];
            i = 0;
            do {
                if (character->backpack[i].iItemNo != -1 &&
                    !NpcTradeItemAllowed(&character->backpack[i])) {
                    if (index == hit) {
                        if (!pick) {
                            return &character->backpack[i];
                        }
                        if (commit != 0) {
                            UpdatePartyTradeQuantity(&character->backpack[i], hit, decrement != 0);
                        }
                        g_npc_interaction_state->selected_trade_row = hit;
                        if (commit != 0) {
                            SoundPlay(g_button_click_1, 0);
                        }
                        ShowSelectedNpcTradeItem(&character->backpack[i]);
                        return &character->backpack[i];
                    }
                    ++hit;
                }
                ++i;
            } while (i < 8);
        }
        return 0;
    }
    i = ResolveNpcTradeStockIndex(index);
    if (i == -1) {
        return 0;
    }
    entry = GetNpcItemAt(g_npc_interaction_state->dialogue_npc, i);
    if (entry == 0) {
        return 0;
    }
    if (commit != 0) {
        if (g_item_records[entry->item.iItemNo].equip_class == W8_ITEM_EQUIP_CLASS_AMMUNITION) {
            g_npc_interaction_state->trade_quantity = entry->item.stack_count;
        } else if (g_npc_interaction_state->selected_trade_row == i) {
            if (decrement == 0) {
                if (g_item_records[entry->item.iItemNo].quantity_kind == W8_ITEM_QUANTITY_STACK) {
                    ++g_npc_interaction_state->trade_quantity;
                    if (entry->item.stack_count < g_npc_interaction_state->trade_quantity) {
                        g_npc_interaction_state->trade_quantity = entry->item.stack_count;
                    } else if (entry->item.stack_count != 0) {
                        HighlightNpcTradeQuantity();
                    }
                }
            } else {
                --g_npc_interaction_state->trade_quantity;
                if (g_npc_interaction_state->trade_quantity == 0) {
                    g_npc_interaction_state->trade_quantity = 1;
                } else if (entry->item.stack_count != 0) {
                    HighlightNpcTradeQuantity();
                }
            }
        } else {
            g_npc_interaction_state->trade_quantity = 1;
        }
    }
    g_npc_interaction_state->selected_trade_row = i;
    if (pick) {
        if (commit != 0) {
            SoundPlay(g_button_click_1, 0);
        }
        ShowSelectedNpcTradeItem(&entry->item);
    }
    return &entry->item;
}

/* Whether the current trade filter rejects `item`: the Zant NPC hides item
   0x290 until fact 0x3c, the trade_filter low bits gate character/party
   usability, and bits 0x3c select the accepted equipment slot group. */
// FUNCTION: WIZ8 0x00573190
bool NpcTradeItemAllowed(W8ItemInstance* item)
{
    W8ItemEquipSlotGroup group;

    if (g_npc_interaction_state->dialogue_npc->name_style == ',' &&
        GetFact(W8_FACT_ALIGNMENT_UMPANI) == 0 && item->iItemNo != 0x290) {
        return true;
    }
    if (g_npc_interaction_state->trade_filter == 0) {
        return false;
    }
    if (item->iItemNo != -1) {
        if ((g_npc_interaction_state->trade_filter & W8_NPC_TRADE_USABLE_BY_CHARACTER) == 0) {
            if ((g_npc_interaction_state->trade_filter & W8_NPC_TRADE_USABLE_BY_PARTY) != 0 &&
                AnyPartyMemberCanUseItem(item->iItemNo)) {
                return true;
            }
        } else {
            if (!CanCharacterUseItem(&g_status.buffers.Char[g_status.selected_character],
                                     item->iItemNo)) {
                return true;
            }
        }
        if ((g_npc_interaction_state->trade_filter & W8_NPC_TRADE_CATEGORY_MASK) == 0) {
            return false;
        }
        group = GetItemEquipSlotGroup(item->iItemNo);
        if (group == W8_ITEM_EQUIP_GROUP_HAND) {
            if ((g_npc_interaction_state->trade_filter & W8_NPC_TRADE_HAND) != 0) {
                return false;
            }
        } else if (group == W8_ITEM_EQUIP_GROUP_BODY) {
            if ((g_npc_interaction_state->trade_filter & W8_NPC_TRADE_BODY) != 0) {
                return false;
            }
        } else if (group == W8_ITEM_EQUIP_GROUP_ACCESSORY) {
            if ((g_npc_interaction_state->trade_filter & W8_NPC_TRADE_ACCESSORY) != 0) {
                return false;
            }
        } else if ((g_npc_interaction_state->trade_filter & W8_NPC_TRADE_OTHER) != 0) {
            return false;
        }
    }
    return true;
}

// FUNCTION: WIZ8 0x005732A0
void OpenNpcDialogueMode5Layout(void)
{
    wchar_t buffer[0x20];
    int index;

    g_npc_interaction_state->dialogue_layout = W8_DIALOGUE_LAYOUT_TRADE;
    if (g_npc_interaction_state->dialogue_hidden != 0) {
        SetNpcDialogueHidden(0);
    }
    static_cast<W8NpcDialogueOptionsPanel*>(g_npc_interaction_state->dialogue_panels[0])->SetEnabled(1);
    g_npc_interaction_state->dialogue_panels[1]->SetEnabled(1);
    g_npc_interaction_state->dialogue_panels[5]->SetEnabled(1);
    RegionSetEnable(0x18);
    RegionSetEnable(0x17);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_NPC_NAME])->m_textBuffer.SetText(gppStringList[0x739],
                                                                     g_wiz_text_bold_font);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])->m_textBuffer.SetText(gppStringList[0x73d],
                                                                     g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])
        ->m_primaryActivationCallback = SelectNpcBuyMode;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])->m_textBuffer.SetText(gppStringList[0x73e],
                                                                     g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])
        ->m_primaryActivationCallback = SelectNpcSellMode;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_3])->m_textBuffer.SetText(gppStringList[0x73f],
                                                                     g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_3])
        ->m_primaryActivationCallback = SelectNpcGiveMode;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])->m_textBuffer.SetText(gppStringList[0x72e],
                                                                     g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])
        ->m_primaryActivationCallback = SelectNpcShopliftMode;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_31])->SetEnabled(0);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_31])->m_primaryActivationCallback = ConfirmNpcTradeSlot;
    swprintf(buffer, L"%dg", g_status.party_gold);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_33])->m_textBuffer.SetText(buffer,
                                                                     g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_37])->SetEnabled(0);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_37])->m_primaryActivationCallback = RestockNpcTradeStock;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])->SetActive(0);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_6])->SetActive(0);
    g_npc_interaction_state->trade_filter = 0;
    for (index = 0; index < 6; ++index) {
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + index])->SetEnabled(0);
    }
    if (g_npc_interaction_state->dialogue_npc->record->owns_stock == 0) {
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])->SetEnabled(0);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])->SetEnabled(0);
    }
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_3])->SetEnabled(1);
    RequestRedraw(W8_MAIN_REDRAW_LAYOUT);
    SelectTextBox(2);
    if (!gXStatus.fCampMode) {
        ResetEditorStatusLine(2);
    }
}

/* Tear down the mode-5 dialogue layout and retire the current mode. */
// FUNCTION: WIZ8 0x00573570
void CloseNpcDialogueMode5Layout(void)
{
    RegionSetDisable(0x18);
    RegionSetDisable(0x17);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_31])->SetEnabled(1);
    static_cast<W8NpcDialogueOptionsPanel*>(g_npc_interaction_state->dialogue_panels[0])->SetEnabled(0);
    g_npc_interaction_state->dialogue_panels[1]->SetEnabled(0);
    g_npc_interaction_state->dialogue_panels[5]->SetEnabled(0);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])->SetEnabled(1);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])->SetEnabled(1);
    SetNpcDialogueLayoutMode(W8_DIALOGUE_LAYOUT_NONE);
}

/* Re-arm the six trade-filter option buttons after the list contents were
   rebuilt. */
// FUNCTION: WIZ8 0x00573630
void EnableNpcTradeFilterButtons(void)
{
    for (unsigned int index = 0; index <= 5; ++index) {
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + index])->SetEnabled(1);
    }
}

/* The six trade-filter option callbacks share one shape: when the button's
   secondary state is raised it becomes the exclusive slot-group bit in
   trade_filter (the previously active sibling is dimmed and cleared), and when it
   is lowered the bit comes off again; RebuildNpcTradeItemList(1) refreshes the list. */
static void ToggleNpcTradeCategory(int button, W8NpcTradeFilter filter)
{
    W8TextControl* control = static_cast<W8TextControl*>(
        g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + button]);
    if (static_cast<unsigned char>(control->m_stateFlags & g_W8TextControlStateSecondary) != 0) {
        int previous = g_npc_interaction_state->trade_filter & W8_NPC_TRADE_CATEGORY_MASK;
        if (previous != filter) {
            int previous_button;
            switch (previous) {
            case W8_NPC_TRADE_HAND:
                previous_button = 0;
                break;
            case W8_NPC_TRADE_BODY:
                previous_button = 3;
                break;
            case W8_NPC_TRADE_ACCESSORY:
                previous_button = 1;
                break;
            case W8_NPC_TRADE_OTHER:
                previous_button = 4;
                break;
            default:
                previous_button = -1;
                break;
            }
            if (previous_button != -1) {
                static_cast<W8TextControl*>(
                    g_npc_interaction_state
                        ->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + previous_button])
                    ->DisableSecondaryState(1);
                g_npc_interaction_state->trade_filter &= ~previous;
            }
        }
        g_npc_interaction_state->trade_filter |= filter;
    } else {
        g_npc_interaction_state->trade_filter &= ~filter;
    }
    RebuildNpcTradeItemList(1);
}

// FUNCTION: WIZ8 0x00573660
void ToggleNpcHandItemFilter(void)
{
    ToggleNpcTradeCategory(0, W8_NPC_TRADE_HAND);
}

// FUNCTION: WIZ8 0x00573730
void ToggleNpcAccessoryItemFilter(void)
{
    ToggleNpcTradeCategory(1, W8_NPC_TRADE_ACCESSORY);
}

// FUNCTION: WIZ8 0x00573800
void ToggleNpcBodyItemFilter(void)
{
    ToggleNpcTradeCategory(3, W8_NPC_TRADE_BODY);
}

// FUNCTION: WIZ8 0x005738D0
void ToggleNpcOtherItemFilter(void)
{
    ToggleNpcTradeCategory(4, W8_NPC_TRADE_OTHER);
}

/* The "usable by the selected character" toggle is exclusive with the
   "usable by anyone" toggle: option control 2 lowers bit 0x40 and raises
   bit 1 while pressed, and drops bit 1 when released. */
// FUNCTION: WIZ8 0x005739A0
void ToggleNpcCharacterUsabilityFilter(void)
{
    if (static_cast<unsigned char>(static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 2])->m_stateFlags &
                                   g_W8TextControlStateSecondary) != 0) {
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 5])->DisableSecondaryState(1);
        g_npc_interaction_state->trade_filter &= ~W8_NPC_TRADE_USABLE_BY_PARTY;
        g_npc_interaction_state->trade_filter |= W8_NPC_TRADE_USABLE_BY_CHARACTER;
        RebuildNpcTradeItemList(1);
        return;
    }
    g_npc_interaction_state->trade_filter &= ~W8_NPC_TRADE_USABLE_BY_CHARACTER;
    RebuildNpcTradeItemList(1);
}

// FUNCTION: WIZ8 0x00573A10
void ToggleNpcPartyUsabilityFilter(void)
{
    if (static_cast<unsigned char>(static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 5])->m_stateFlags &
                                   g_W8TextControlStateSecondary) != 0) {
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_OPTION_BUTTONS + 2])->DisableSecondaryState(1);
        g_npc_interaction_state->trade_filter &= ~W8_NPC_TRADE_USABLE_BY_CHARACTER;
        g_npc_interaction_state->trade_filter |= W8_NPC_TRADE_USABLE_BY_PARTY;
        RebuildNpcTradeItemList(1);
        return;
    }
    g_npc_interaction_state->trade_filter &= ~W8_NPC_TRADE_USABLE_BY_PARTY;
    RebuildNpcTradeItemList(1);
}

// FUNCTION: WIZ8 0x00573A80
void SelectNpcSellMode(void)
{
    g_npc_interaction_state->trade_mode = W8_NPC_TRADE_SELL;
    UpdateNpcDialogueSubMode();
}

// FUNCTION: WIZ8 0x00573AA0
void SelectNpcGiveMode(void)
{
    g_npc_interaction_state->trade_mode = W8_NPC_TRADE_GIVE;
    UpdateNpcDialogueSubMode();
}

// FUNCTION: WIZ8 0x00573AC0
void SelectNpcShopliftMode(void)
{
    g_npc_interaction_state->trade_mode = W8_NPC_TRADE_SHOPLIFT;
    UpdateNpcDialogueSubMode();
}

/* Open the mode-1 service layout: the caption and three service options, the
   mode-4-only controls parked, and each option enabled from what the selected
   character can actually cast or use. */
// FUNCTION: WIZ8 0x00573AE0
void OpenNpcDialogueMode1Layout(void)
{
    W8Character* character;

    g_npc_interaction_state->dialogue_layout = W8_DIALOGUE_LAYOUT_SERVICES;
    if (g_npc_interaction_state->dialogue_hidden != 0) {
        SetNpcDialogueHidden(0);
    }
    static_cast<W8NpcDialogueOptionsPanel*>(g_npc_interaction_state->dialogue_panels[0])->SetEnabled(1);
    g_npc_interaction_state->dialogue_panels[1]->SetEnabled(1);
    g_npc_interaction_state->dialogue_panels[4]->SetEnabled(1);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_38])->SetActive(0);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_38])->m_textBuffer.SetText(&g_empty_wide_string,
                                                                     g_wiz_text_bold_font);
    RegionSetEnable(0x18);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_NPC_NAME])->m_textBuffer.SetText(gppStringList[0x72f],
                                                                     g_wiz_text_bold_font);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])->m_textBuffer.SetText(gppStringList[0x730],
                                                                     g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])->m_primaryActivationCallback =
        RequestNpcSpellService3;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])->m_textBuffer.SetText(gppStringList[0x732],
                                                                     g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])->m_primaryActivationCallback =
        RequestNpcSpellService41;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])->m_textBuffer.SetText(gppStringList[0x733],
                                                                     g_wiz_text_font_secondary);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])->m_primaryActivationCallback =
        RequestNpcCharacterService;
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])->AddLayoutFlags(g_W8TextControlLayoutToggle);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])->EnableRegionHelp(0x7ca);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_3])->SetActive(0);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])->SetActive(0);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_6])->SetActive(0);
    if (g_npc_interaction_state->dialogue_layout == W8_DIALOGUE_LAYOUT_SERVICES) {
        character = &g_status.buffers.Char[g_status.selected_character];
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])->SetEnabled(CanCharacterCastSpell(character, 3));
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])->W8Widget::Invalidate(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])->SetEnabled(
            CanCharacterCastSpell(character, 0x29));
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])->W8Widget::Invalidate(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])->SetEnabled(CharacterHasServiceItem(character));
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])->W8Widget::Invalidate(1);
    }
    RequestRedraw(W8_MAIN_REDRAW_LAYOUT);
    SelectTextBox(2);
    if (!gXStatus.fCampMode) {
        ResetEditorStatusLine(2);
    }
}

/* Tear down the mode-1 dialogue layout; outside camp the mode is retired as
   well. */
// FUNCTION: WIZ8 0x00573DD0
void CloseNpcDialogueMode1Layout(void)
{
    RegionSetDisable(0x18);
    RegionSetDisable(0x17);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])->RemoveLayoutFlags(g_W8TextControlLayoutToggle);
    static_cast<W8NpcDialogueOptionsPanel*>(g_npc_interaction_state->dialogue_panels[0])->SetEnabled(0);
    g_npc_interaction_state->dialogue_panels[1]->SetEnabled(0);
    g_npc_interaction_state->dialogue_panels[4]->SetEnabled(0);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])->DisableRegionHelp();
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])->SetEnabled(1);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])->SetEnabled(1);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_3])->SetEnabled(1);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])->SetEnabled(1);
    if (!gXStatus.fCampMode) {
        SetNpcDialogueLayoutMode(W8_DIALOGUE_LAYOUT_NONE);
    }
}

// FUNCTION: WIZ8 0x00573ED0
void RequestNpcSpellService3(void)
{
    int location = g_npc_interaction_state->target_location_id;
    W8NpcDialogueLayout mode = g_npc_interaction_state->dialogue_layout;

    gXStatus.fCampMode = true;
    CloseNpcDialogueMode1Layout();
    EndNpcDialogueSession(0);
    BeginSpellCast(3, location, mode);
}

// FUNCTION: WIZ8 0x00573F10
void RequestNpcSpellService41(void)
{
    int location = g_npc_interaction_state->target_location_id;
    W8NpcDialogueLayout mode = g_npc_interaction_state->dialogue_layout;

    gXStatus.fCampMode = true;
    CloseNpcDialogueMode1Layout();
    EndNpcDialogueSession(0);
    BeginSpellCast(0x29, location, mode);
}

// FUNCTION: WIZ8 0x00573F50
void RequestNpcCharacterService(void)
{
    gXStatus.fCampMode = true;
    CloseNpcDialogueMode1Layout();
    EndNpcDialogueSession(0);
    OpenUseItemSelectView(g_status.selected_character);
}

/* Route the player's reply text while a modal answer is pending. With no
   price offer outstanding the text resolves through the current quote's
   keyword tables and queues the matching line; during a 0x12/0x1e price check
   the affirmative string spends pending_price (or runs line 0x14 when the
   party cannot pay), tells the offered fact unless opcode 0x1e suppressed it,
   and a refusal runs the pending fact's kind-0x17 decline entries. A nonzero
   echo posts the reply text back as a notice. */
/* Pick the index-th activatable item on the dialogue character: the twelve
   equipment slots first, then the eight backpack slots. The hit refreshes the
   dialogue_text_16c preview image and its stack-count text. */
// FUNCTION: WIZ8 0x00573F80
W8ItemInstance* GetNpcTradeSlotItem(int index)
{
    wchar_t count_text[32];
    wchar_t* text;
    W8Character* character;
    int image;
    int hit;
    int slot;

    character = &g_status.buffers.Char[g_status.selected_character];
    hit = 0;
    if (static_cast<unsigned char>(static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])->m_stateFlags &
                                   g_W8TextControlStateSecondary) == 0) {
        return 0;
    }
    for (slot = 0; slot < 12; ++slot) {
        if (character->EquippedItem[slot].iItemNo != -1 &&
            CanCharacterActivateItem(character, &character->EquippedItem[slot])) {
            if (index == hit) {
                image = g_item_video_objects.GetOrCreateVideoObject(
                    character->EquippedItem[slot].iItemNo);
                static_cast<W8TextControl*>(
                    g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_24])
                    ->SetImage(image);
                if (character->EquippedItem[slot].stack_count < 2) {
                    text = g_dialogue_empty_text;
                } else {
                    swprintf(count_text, L"%d", character->EquippedItem[slot].stack_count);
                    text = count_text;
                }
                static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_24])->m_textBuffer.SetText(
                    text, g_wiz_text_font_secondary);
                static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_24])->Invalidate(1);
                return &character->EquippedItem[slot];
            }
            ++hit;
        }
    }
    for (slot = 0; slot < 8; ++slot) {
        if (character->backpack[slot].iItemNo != -1 &&
            CanCharacterActivateItem(character, &character->backpack[slot])) {
            if (index == hit) {
                image =
                    g_item_video_objects.GetOrCreateVideoObject(character->backpack[slot].iItemNo);
                static_cast<W8TextControl*>(
                    g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_24])
                    ->SetImage(image);
                if (character->backpack[slot].stack_count < 2) {
                    text = g_dialogue_empty_text;
                } else {
                    swprintf(count_text, L"%d", character->backpack[slot].stack_count);
                    text = count_text;
                }
                static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_24])->m_textBuffer.SetText(
                    text, g_wiz_text_font_secondary);
                static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_24])->Invalidate(1);
                return &character->backpack[slot];
            }
            ++hit;
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x00574250
void HandleNpcDialogueReply(wchar_t* text, bool echo)
{
    wchar_t notice[200];
    int line;

    if (g_npc_interaction_state->script_busy != 0) {
        if (!g_npc_interaction_state->price_check_pending) {
            line = FindNpcReplyQuote(text);
            if (line != -1) {
                QueueNpcScriptLine(line, 0, 0, 0);
            }
        } else {
            if (CompareWideTextIgnoreAsciiCase(text, gppStringList[0x7df]) == 0) {
                if (static_cast<unsigned int>(g_npc_interaction_state->pending_price) >
                    g_status.party_gold) {
                    RunNpcScriptLine(0x14, 0);
                    g_npc_interaction_state->price_check_pending = 0;
                } else {
                    SpendPartyGold(g_npc_interaction_state->pending_price);
                    if (!g_npc_interaction_state->price_check_skip_fact) {
                        TellNpcFact(g_npc_interaction_state->dialogue_npc,
                                    g_npc_interaction_state->pending_fact);
                    }
                    RunNpcScriptLine(g_npc_interaction_state->pending_fact, 0);
                    g_npc_interaction_state->price_check_pending = 0;
                }
            } else {
                RunNpcQuoteDeclineActions(g_npc_interaction_state->pending_fact);
                g_npc_interaction_state->price_check_pending = 0;
            }
        }
        if (echo) {
            swprintf(notice, L"%s...", text);
            ShowNotice(0xa, notice, 3, GetTextBoxScrollRange(), 0);
        }
        g_npc_interaction_state->script_busy = 0;
    }
}

/* Copy the next space-separated word of `text` into `word`. Returns the text
   after the word, or 0 when no word is left. Retail expands this inline at all
   five uses in HandleNpcDialogueInput and keeps no out-of-line copy. */
static wchar_t* ReadNextWord(wchar_t* text, wchar_t* word)
{
    wchar_t* out;
    int length = 0;

    word[0] = 0;
    while (*text == L' ' && *text != 0) {
        ++text;
    }
    if (*text == L' ') {
        return 0;
    }
    out = word;
    while (*text != L' ' && *text != 0) {
        *out++ = *text++;
        ++length;
    }
    if (length == 0) {
        return 0;
    }
    word[length] = 0;
    return text;
}

/* The NPC dialogue's typed-input processor. Strips punctuation, tokenizes into
   words, and resolves the text to one or two quote ids: the "join"-style
   keywords go straight to RequestNpcJoinParty, name/place prefixes are stripped
   and resolved through FindNpcNameOrPlaceQuote, and otherwise every word pair
   then every single word is tried through FindNpcScriptQuoteByKeyword. Unresolved input
   shows a fallback notice built from the random reply tables; resolved input
   queues the script line(s). */
// FUNCTION: WIZ8 0x005743B0
void HandleNpcDialogueInput(void)
{
    wchar_t field_text[200];
    wchar_t word[200];
    wchar_t buf[200];
    wchar_t word2[200];
    wchar_t notice[200];
    wchar_t* cursor;
    int quote_id = -1;
    int second_quote_id = -1;
    bool show_fallback = true;
    bool plain_text = true;
    bool echo = false;
    int quote;
    int word_count;
    int matches;

    Get16BitStringFromField(0, field_text);
    ClearActiveField();
    StripNpcKeywordPunctuation(field_text);
    if (wcslen(field_text) == 0 && g_npc_interaction_state->script_busy == 0) {
        return;
    }
    if (g_npc_interaction_state->script_busy != 0) {
        HandleNpcDialogueReply(field_text, 1);
        return;
    }
    word_count = 0;
    cursor = field_text;
    while (cursor != 0) {
        cursor = ReadNextWord(cursor, word);
        if (cursor != 0) {
            ++word_count;
        }
    }
    if (word_count > 2) {
        show_fallback = false;
    }
    if (_wcsnicmp(field_text, gppStringList[0x76b], 4) == 0 ||
        _wcsnicmp(field_text, gppStringList[0x76c], 7) == 0) {
        RequestNpcJoinParty();
        return;
    }
    if (g_npc_interaction_state->where_is_query) {
        const wchar_t* fmt;
        if (_wcsnicmp(field_text, gppStringList[0x76d], 9) == 0 ||
            _wcsnicmp(field_text, gppStringList[0x76e], 0xa) == 0 ||
            _wcsnicmp(field_text, gppStringList[0x76f], 8) == 0) {
            fmt = g_format_s;
        } else {
            fmt = gppStringList[0x76a];
        }
        swprintf(buf, fmt, field_text);
        quote = FindNpcScriptQuoteByKeyword(buf, 0, 0);
        if (quote != -1) {
            plain_text = false;
            quote_id = quote;
        }
    } else {
        swprintf(buf, g_format_s, field_text);
        quote = FindNpcScriptQuoteByKeyword(buf, 0, 0);
        if (quote != -1) {
            quote_id = quote;
        }
    }

    if (quote_id == -1) {
        wcscpy(buf, field_text);
        if (_wcsnicmp(buf, gppStringList[0x770], 0xb) == 0) {
            wcscpy(field_text, buf + 0xb);
            show_fallback = false;
        } else if (_wcsnicmp(buf, gppStringList[0x76d], 9) == 0 ||
                   _wcsnicmp(buf, gppStringList[0x76e], 0xa) == 0 ||
                   _wcsnicmp(buf, gppStringList[0x76f], 8) == 0) {
            wcscpy(field_text, buf + 9);
            plain_text = false;
            show_fallback = false;
            quote_id = FindNpcNameOrPlaceQuote(g_npc_interaction_state->dialogue_npc, field_text);
            if (quote_id == -1) {
                quote_id = 0x76;
            }
        } else if (g_npc_interaction_state->where_is_query) {
            plain_text = false;
            quote_id = FindNpcNameOrPlaceQuote(g_npc_interaction_state->dialogue_npc, field_text);
            if (quote_id == -1) {
                quote_id = 0x76;
            }
        } else {
            plain_text = true;
        }
    }

    /* Try each adjacent word pair. */
    if (quote_id == -1) {
        cursor = field_text;
        word[0] = 0;
        while (cursor != 0) {
            if (wcslen(word) == 0) {
                cursor = ReadNextWord(cursor, word);
            } else {
                wcscpy(word, word2);
            }
            if (cursor == 0) {
                break;
            }
            cursor = ReadNextWord(cursor, word2);
            if (cursor == 0) {
                break;
            }
            swprintf(buf, g_format_s_space_s, word, word2);
            quote = FindNpcScriptQuoteByKeyword(buf, 0, 0);
            if (quote != -1) {
                quote_id = quote;
                break;
            }
        }
    }

    /* Then each single word: more than two hits echoes the text, otherwise the
       first hit (and a different second one) answer. */
    if (quote_id == -1) {
        matches = 0;
        cursor = field_text;
        while (cursor != 0) {
            cursor = ReadNextWord(cursor, word);
            if (cursor != 0 && FindNpcScriptQuoteByKeyword(word, 0, 0) != -1) {
                ++matches;
            }
        }
        if (matches > 2) {
            quote_id = 0x20;
            echo = true;
        } else if (matches > 0) {
            int found_count = 0;
            cursor = field_text;
            for (;;) {
                cursor = ReadNextWord(cursor, word);
                if (cursor == 0) {
                    break;
                }
                quote = FindNpcScriptQuoteByKeyword(word, 0, 0);
                if (quote == -1) {
                    continue;
                }
                ++found_count;
                if (found_count == 1) {
                    quote_id = quote;
                } else if (found_count == 2 && quote != quote_id) {
                    second_quote_id = quote;
                }
                if (matches == 1) {
                    break;
                }
            }
        }
    }

    if (quote_id >= 0x59 && quote_id < 0x69 && second_quote_id == -1) {
        echo = true;
    }
    if (!echo && show_fallback) {
        unsigned int roll;
        const wchar_t* fmt;
        const wchar_t* text;
        if (plain_text) {
            roll = Random(5);
            if (roll == 2 || roll == 3 || roll == 4) {
                fmt = L"%s %s?";
            } else {
                fmt = L"%s %s.";
            }
            text = gppStringList[g_dialogue_fallback_ids0[roll]];
        } else {
            roll = Random(5);
            if (roll == 3 || roll == 4) {
                fmt = L"%s %s?";
            } else {
                fmt = L"%s %s.";
            }
            text = gppStringList[g_dialogue_fallback_ids1[roll]];
        }
        swprintf(notice, fmt, text, field_text);
        ShowNotice(0xa, notice, 3, GetTextBoxScrollRange(), 0);
    } else {
        ShowNotice(0xa, field_text, 3, GetTextBoxScrollRange(), 0);
    }
    if (quote_id == -1) {
        QueueNpcScriptLine(Random(2) + 0x23, 0, 0, 0);
        return;
    }
    if (quote_id == 0x59 || second_quote_id == 0x59) {
        QueueNpcScriptLine(0x59, 1, 0, 0);
        return;
    }
    QueueNpcScriptLine(quote_id, 1, 0, 0);
    if (second_quote_id == -1) {
        return;
    }
    QueueNpcScriptLine(0x21, 0, 0, 0);
    QueueNpcScriptLine(second_quote_id, 1, 0, 0);
}

// FUNCTION: WIZ8 0x00574BB0
void HandleNpcDialogueKeyEvent(const InputAtom* event)
{
    W8NpcInteractionState* state;
    W8MGSCommand command;

    if (ShouldDeferCharacterEventForNpcScript(0)) {
        return;
    }
    state = g_npc_interaction_state;
    if (state->modal_dialog_open && event->usParam != 0x1b) {
        return;
    }
    switch (event->usParam) {
    case 0x26:
        if (state->dialogue_layout != W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX) {
            return;
        }
        ScrollTextBoxUp(1);
        NpcDialogueTextBoxWheelAt(0, static_cast<unsigned short>(event->uiParam >> 16), true);
        return;
    case 0x28:
        if (state->dialogue_layout != W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX) {
            return;
        }
        ScrollTextBoxDown(1);
        NpcDialogueTextBoxWheelAt(0, static_cast<unsigned short>(event->uiParam >> 16), true);
        return;
    case 0x1b:
        if (state->dialogue_hidden != 0) {
            SetNpcDialogueHidden(0);
            return;
        }
        if (IsNpcScriptSessionActive()) {
            TryFinishNpcVoicePlayback(1);
            return;
        }
        if (EditingText()) {
            SetInputFieldStringWith16BitString(0, &g_empty_wide_string);
            ClearActiveField();
            return;
        }
        if (g_npc_interaction_state->script_busy != 0 || !IsMessageBoxLineQueueEmpty()) {
            return;
        }
        switch (g_npc_interaction_state->dialogue_layout) {
        case W8_DIALOGUE_LAYOUT_TRANSCRIPT:
            g_npc_interaction_state->suppress_parting_reaction = 0;
            g_npc_interaction_state->farewell_queued = true;
            QueueNpcScriptLine(0x5c, 0, 0, 0);
            return;
        case W8_DIALOGUE_LAYOUT_TOPIC_MENU:
            EndNpcDialogueSession(0);
            return;
        case W8_DIALOGUE_LAYOUT_SERVICES: {
            W8NpcDialogueLayout prev = g_npc_interaction_state->previous_dialogue_layout;
            CloseNpcDialogueMode1Layout();
            if (prev == W8_DIALOGUE_LAYOUT_TOPIC_MENU) {
                ShowNpcDialogueTopicMenu();
                return;
            }
            if (prev == W8_DIALOGUE_LAYOUT_TRANSCRIPT) {
                OpenNpcDialogueTranscriptLayout();
            }
            return;
        }
        case W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX: {
            bool cursor = g_npc_interaction_state->reopen_topics;
            CloseNpcDialogueOptionLayout();
            if (cursor) {
                ShowNpcDialogueTopicMenu();
            } else {
                OpenNpcDialogueTranscriptLayout();
            }
            return;
        }
        case W8_DIALOGUE_LAYOUT_TRADE:
            CloseNpcDialogueMode5Layout();
            OpenNpcDialogueTranscriptLayout();
            return;
        default:
            return;
        }
    case 0xd:
        if (EditingText()) {
            HandleNpcDialogueInput();
        } else {
            SetActiveField(0);
        }
        return;
    default:
        command = g_mgs_keyboard->FindCommandForEvent(event);
        if (command == W8_MGS_COMMAND_JOURNAL) {
            DispatchMGSCommand(command);
            return;
        }
        if (g_npc_interaction_state->dialogue_layout == W8_DIALOGUE_LAYOUT_TRANSCRIPT) {
            return;
        }
        if (command <= W8_MGS_COMMAND_INVENTORY) {
            if (command != W8_MGS_COMMAND_INVENTORY && command != W8_MGS_COMMAND_QUIT_GAME) {
                return;
            }
        } else if (command < W8_MGS_COMMAND_SELECT_RECRUITED_1 ||
                   command > W8_MGS_COMMAND_SELECT_PC_6) {
            return;
        }
        DispatchMGSCommand(command);
        return;
    }
}

// FUNCTION: WIZ8 0x00574F90
void SetDialogueFieldKeyword(wchar_t* keyword, bool append)
{
    wchar_t field_text[200];
    wchar_t combined[200];

    Get16BitStringFromField(0, field_text);
    StripNpcKeywordPunctuation(keyword);
    if (wcslen(field_text) != 0 && append) {
        swprintf(combined, g_format_s_space_s, field_text, keyword);
        SetInputFieldStringWith16BitString(0, combined);
    } else {
        SetInputFieldStringWith16BitString(0, keyword);
    }
}

// FUNCTION: WIZ8 0x00575020
bool IsDialoguePlaceKeyword(const wchar_t* name)
{
    for (int index = 0; index < g_dialogue_place_keyword_count; ++index) {
        if (CompareWideTextIgnoreAsciiCase(
                name, gppStringList[g_dialogue_place_keyword_ids[index]]) == 0) {
            return 1;
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x00575070
void ClearNpcDialogueTranscript(void)
{
    for (int index = 0; index < g_npc_interaction_state->dialogue_transcript.count; ++index) {
        free(*g_npc_interaction_state->dialogue_transcript.GetAt(index));
    }
    g_npc_interaction_state->dialogue_transcript.Clear();
}

// FUNCTION: WIZ8 0x005750D0
unsigned char LoadNpcDialogueTranscript(unsigned int file)
{
    unsigned char version;
    unsigned int bytes_read;
    int record_count;
    int text_length;
    int index;

    for (index = 0; index < g_npc_interaction_state->dialogue_transcript.count; ++index) {
        free(*g_npc_interaction_state->dialogue_transcript.GetAt(index));
    }
    g_npc_interaction_state->dialogue_transcript.Clear();
    FileRead(file, &version, 1, &bytes_read);
    FileRead(file, &record_count, 4, &bytes_read);
    for (index = 0; index < record_count; ++index) {
        W8DialogueTranscriptRecord* record =
            static_cast<W8DialogueTranscriptRecord*>(malloc(sizeof(W8DialogueTranscriptRecord)));
        memset(record, 0, sizeof(*record));
        FileRead(file, &text_length, 4, &bytes_read);
        FileRead(file, record, text_length * 2 + 2, &bytes_read);
        FileRead(file, &record->category, 1, &bytes_read);
        g_npc_interaction_state->dialogue_transcript.Add(record);
    }
    if (version > 1) {
        FileRead(file, &g_npc_interaction_state->dialogue_category_filter, 1, &bytes_read);
        FileRead(file, &g_npc_interaction_state->transcript_sorted, 1, &bytes_read);
    }
    return 1;
}

// FUNCTION: WIZ8 0x00575290
unsigned char SaveNpcDialogueTranscript(unsigned int file)
{
    unsigned char version;
    unsigned int bytes_written;
    size_t text_length;
    int index;

    version = 2;
    FileWrite(file, &version, 1, &bytes_written);
    text_length = g_npc_interaction_state->dialogue_transcript.count;
    FileWrite(file, &text_length, 4, &bytes_written);
    for (index = 0; index < g_npc_interaction_state->dialogue_transcript.count; ++index) {
        W8DialogueTranscriptRecord* record =
            *g_npc_interaction_state->dialogue_transcript.GetAt(index);
        text_length = wcslen(record->text);
        FileWrite(file, &text_length, 4, &bytes_written);
        FileWrite(file, record, text_length * 2 + 2, &bytes_written);
        FileWrite(file, &record->category, 1, &bytes_written);
    }
    FileWrite(file, &g_npc_interaction_state->dialogue_category_filter, 1, &bytes_written);
    FileWrite(file, &g_npc_interaction_state->transcript_sorted, 1, &bytes_written);
    return 1;
}

/* Restate the five transcript category buttons so only the active
   dialogue_category_filter's button shows its secondary state. */
// FUNCTION: WIZ8 0x00575390
void SyncDialogueCategoryButtons(void)
{
    switch (g_npc_interaction_state->dialogue_category_filter) {
    case W8_DIALOGUE_CATEGORY_ITEMS:
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_PEOPLE_BUTTON])->DisableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_PLACES_BUTTON])->DisableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_ITEMS_BUTTON])->EnableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_MISC_BUTTON])->DisableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_ALL_BUTTON])->DisableSecondaryState(1);
        break;
    case W8_DIALOGUE_CATEGORY_PEOPLE:
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_PEOPLE_BUTTON])->EnableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_PLACES_BUTTON])->DisableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_ITEMS_BUTTON])->DisableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_MISC_BUTTON])->DisableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_ALL_BUTTON])->DisableSecondaryState(1);
        break;
    case W8_DIALOGUE_CATEGORY_PLACES:
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_PEOPLE_BUTTON])->DisableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_PLACES_BUTTON])->EnableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_ITEMS_BUTTON])->DisableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_MISC_BUTTON])->DisableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_ALL_BUTTON])->DisableSecondaryState(1);
        break;
    case W8_DIALOGUE_CATEGORY_MISC:
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_PEOPLE_BUTTON])->DisableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_PLACES_BUTTON])->DisableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_ITEMS_BUTTON])->DisableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_MISC_BUTTON])->EnableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_ALL_BUTTON])->DisableSecondaryState(1);
        break;
    case W8_DIALOGUE_CATEGORY_ALL:
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_PEOPLE_BUTTON])->DisableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_PLACES_BUTTON])->DisableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_ITEMS_BUTTON])->DisableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_MISC_BUTTON])->DisableSecondaryState(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_ALL_BUTTON])->EnableSecondaryState(1);
        break;
    default:
        break;
    }
}

/* A confirmed purchase returns the NPC dialogue to the layout appropriate
   for the NPC's disposition. A cancelled dialog leaves the layout alone. */
// FUNCTION: WIZ8 0x00575520
void OnNpcTradeDialogClosed(W8DialogBase* dialog)
{
    if (GetDialogResult(dialog) == 0) {
        return;
    }
    ConfirmNpcTradePurchase();
    CloseActiveNpcDialogueLayout();
    if (GetNpcDispositionBand(g_npc_interaction_state->dialogue_npc) == 0) {
        OpenNpcDialogueTranscriptLayout();
    } else {
        ShowNpcDialogueTopicMenu();
    }
}

// FUNCTION: WIZ8 0x00575710
void ConfirmNpcTradePurchase(void)
{
    if (GetNpcDispositionBand(g_npc_interaction_state->dialogue_npc) == 0) {
        SpendPartyGold(g_npc_interaction_state->trade_gold);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_24])->Invalidate(1);
        QueueNpcScriptLine(0x10, 0, 0, 0);
        RebuildNpcTradeItemList(0);
        return;
    }
    if (NpcRecordHasTradePool(g_npc_interaction_state->dialogue_npc)) {
        ApplyNpcInteraction(g_npc_interaction_state->dialogue_npc, 2,
                            g_npc_interaction_state->dialogue_speaker, 0,
                            g_npc_interaction_state->trade_gold);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_24])->Invalidate(1);
        QueueNpcScriptLine(
            GetNpcDispositionBand(g_npc_interaction_state->dialogue_npc) == 0 ? 0x10 : 7, 0, 0, 0);
        RebuildNpcTradeItemList(0);
        return;
    }
    QueueNpcScriptLine(7, 0, 0, 0);
}

// FUNCTION: WIZ8 0x00575810
unsigned char HandleNpcDialogueItem(W8ItemInstance* item)
{
    W8MessageDialogBase* dialog;
    wchar_t* message;
    bool result;
    unsigned char flag;
    int fact_result;

    result = 1;
    if (g_npc_interaction_state->trade_gold != 0 && item == 0) {
        if (g_npc_interaction_state->trade_gold != static_cast<int>(g_status.party_gold)) {
            ConfirmNpcTradePurchase();
            return 1;
        }
        dialog = static_cast<W8MessageDialogBase*>(CreateDialogByKind(1));
        dialog->SetClientExtent(0xfa, 200);
        message = FormatWideString(gppStringList[0x7d6]);
        dialog->SetMessage(message, 1, 0x32, 1, 1, 1, 1, 0, 0x15e);
        SetDialogDestroyCallback(dialog, OnNpcTradeDialogClosed);
        OpenModal(dialog);
        return 1;
    }
    if (GetNpcDispositionBand(g_npc_interaction_state->dialogue_npc) == 0 ||
        g_npc_interaction_state->dialogue_npc->record->voice_script != 0) {
        if (item != 0) {
            fact_result = FindNpcScriptItemQuote(item->iItemNo, 0, &flag);
            if (fact_result == -1) {
                if (g_npc_interaction_state->dialogue_npc->record->voice_script != 0) {
                    return 1;
                }
                if ((g_item_records[item->iItemNo].flags & W8_ITEM_FLAG_NO_DISCARD) != 0) {
                    QueueNpcScriptLine(0x11, 0, 0, 0);
                    return 1;
                }
                if (AcceptNpcDialogueItem(g_npc_interaction_state->dialogue_npc, item, 1) != 0) {
                    QueueNpcScriptLine(0x10, 0, 0, 0);
                    RemoveNpcScriptItem(item, 0, -1);
                    return result;
                }
                QueueNpcScriptLine(0x11, 0, 0, 0);
            } else {
                QueueNpcScriptLine(fact_result, 0, 0, 0);
                result = 0;
                if (flag != 0) {
                    RemoveNpcScriptItem(item, 0, -1);
                    return result;
                }
                if (g_npc_interaction_state->held_item_pending &&
                    g_npc_interaction_state->pending_item.iItemNo == item->iItemNo) {
                    g_npc_interaction_state->held_item_pending = 0;
                    AddItemToParty(&g_npc_interaction_state->pending_item, 1, 0);
                    ClearHeldItemDisplay();
                    return 0;
                }
            }
        }
    } else if (item != 0) {
        if ((g_item_records[item->iItemNo].flags & W8_ITEM_FLAG_NO_DISCARD) == 0) {
            if (!WillNpcTradeForItem(g_npc_interaction_state->dialogue_npc, item)) {
                QueueNpcScriptLine(7, 0, 0, 0);
                return 1;
            }
            fact_result = FindNpcScriptItemQuote(item->iItemNo, 0, &flag);
            if (fact_result == -1) {
                ApplyNpcInteraction(g_npc_interaction_state->dialogue_npc, 3,
                                    g_npc_interaction_state->dialogue_speaker, item, 0);
                QueueNpcScriptLine(
                    GetNpcDispositionBand(g_npc_interaction_state->dialogue_npc) == 0 ? 0x10 : 7, 0,
                    0, 0);
            } else {
                QueueNpcScriptLine(fact_result, 0, 0, 0);
                result = 0;
            }
            RemoveNpcScriptItem(item, 0, -1);
            return result;
        }
        QueueNpcScriptLine(0x11, 0, 0, 0);
        return 1;
    }
    return result;
}

/* Retail ICF shares this constant-return callback with the retained body at
   0x005b1740; this typed source callback has no separate address marker. */
unsigned char AcceptNpcDialogueItem(W8NpcState*, W8ItemInstance*, int)
{
    return 1;
}

/* While NPC script deferral holds character events during an open dialogue,
   pump Escape and left-click so layout dismissals still run. */
// FUNCTION: WIZ8 0x00575B00
void SubmitNpcWhereIsQuery(void)
{
    g_npc_interaction_state->where_is_query = 1;
    static_cast<W8NpcTypedDialoguePanel*>(g_npc_interaction_state->dialogue_panels[6])->Invalidate(0);
    HandleNpcDialogueInput();
    g_npc_interaction_state->where_is_query = 0;
}

// FUNCTION: WIZ8 0x00575B40
void SubmitNpcDialogueInput(void)
{
    g_npc_interaction_state->where_is_query = 0;
    static_cast<W8NpcTypedDialoguePanel*>(g_npc_interaction_state->dialogue_panels[6])->Invalidate(0);
    HandleNpcDialogueInput();
}

// FUNCTION: WIZ8 0x00575B70
void HandleNpcDialogueItemChoice(void)
{
    HandleNpcDialogueItem(g_npc_interaction_state->trade_item);
    if (g_npc_interaction_state->reopen_topics &&
        GetNpcDispositionBand(g_npc_interaction_state->dialogue_npc) == 0) {
        CloseNpcDialogueOptionLayout();
        OpenNpcDialogueTranscriptLayout();
        HandleNpcDialogueDeparture(0);
    }
}

// FUNCTION: WIZ8 0x00575BC0
void RefreshNpcTradePartyGold(void)
{
    wchar_t text[32];

    swprintf(text, L"%dg", g_status.party_gold);
    static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_33])->m_textBuffer.SetText(text,
                                                                     g_wiz_text_font_secondary);
}

// FUNCTION: WIZ8 0x00575C00
void RefreshNpcTradePrice(void)
{
    wchar_t text[32];

    if (g_npc_interaction_state->trade_item != 0) {
        swprintf(text, g_format_d, g_npc_interaction_state->trade_quantity);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_24])->m_textBuffer.SetText(text,
                                                                         g_wiz_text_font_secondary);
    }
}

// FUNCTION: WIZ8 0x00575C50
void DrainNpcDialogueDeferralInput(void)
{
    POINT mouse;
    InputAtom input;
    W8NpcDialogueLayout prior_layout;
    bool reopen_topics;

    if (!ShouldDeferCharacterEventForNpcScript(1) || !gXStatus.fNpcDialogueMode) {
        return;
    }
    SGPMouseGetPos(&mouse);
    MSYS_SGP_Mouse_Handler_Hook(MOUSE_POS, static_cast<unsigned short>(mouse.x),
                                static_cast<unsigned short>(mouse.y), gfLeftButtonState,
                                gfRightButtonState);
    while (DequeueEvent(&input) == 1) {
        if (input.usEvent == KEY_DOWN) {
            if (input.usParam == 0x1b) {
                if (g_npc_interaction_state->dialogue_hidden == 0) {
                    if (!IsNpcScriptSessionActive()) {
                        switch (g_npc_interaction_state->dialogue_layout) {
                        case W8_DIALOGUE_LAYOUT_TOPIC_MENU:
                        case W8_DIALOGUE_LAYOUT_TRANSCRIPT:
                            EndNpcDialogueSession(0);
                            break;
                        case W8_DIALOGUE_LAYOUT_SERVICES:
                            prior_layout = g_npc_interaction_state->previous_dialogue_layout;
                            CloseNpcDialogueMode1Layout();
                            if (prior_layout == 2) {
                                ShowNpcDialogueTopicMenu();
                            } else if (prior_layout == 3) {
                                OpenNpcDialogueTranscriptLayout();
                            }
                            break;
                        case W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX:
                            reopen_topics = g_npc_interaction_state->reopen_topics;
                            CloseNpcDialogueOptionLayout();
                            if (reopen_topics) {
                                ShowNpcDialogueTopicMenu();
                            } else {
                                OpenNpcDialogueTranscriptLayout();
                            }
                            break;
                        case W8_DIALOGUE_LAYOUT_TRADE:
                            CloseNpcDialogueMode5Layout();
                            OpenNpcDialogueTranscriptLayout();
                            break;
                        default:
                            break;
                        }
                    } else {
                        TryFinishNpcVoicePlayback(0);
                    }
                } else {
                    SetNpcDialogueHidden(0);
                }
            }
        } else if (input.usEvent == LEFT_BUTTON_DOWN) {
            TryFinishNpcVoicePlayback(0);
        }
    }
}

/* Open the modal NPC sub-dialog for a script request: option list (0x05),
   price check (0x12/0x1e) or keyword entry (0x13). The text-input stack is
   suspended while the modal is up unless the dialogue stays live; for the
   price-check opcodes the base price in the request is discounted by the
   NPC's effect percentage and the party's best haggle skill. */
// FUNCTION: WIZ8 0x00575E60
void OpenNpcDialog(W8NpcQuoteEntry* request, int aux_data)
{
    W8MonsterInfo* monster_info;
    W8NpcDialog* dialog;

    if (!gXStatus.fNpcDialogueMode || g_npc_interaction_state->scripted_dialogue) {
        InitTextInputMode();
    } else {
        SetNpcDialoguePanelVisible(0);
    }
    dialog = new W8NpcDialog(request, aux_data);
    dialog->SetText(&g_empty_wide_string);
    dialog->m_destroy_callback = OnNpcDialogClosed;
    OpenModal(dialog);
    g_npc_interaction_state->script_busy = 1;
    if (request->kind == 0x12 || request->kind == 0x1e) {
        g_npc_interaction_state->pending_fact = aux_data;
        g_npc_interaction_state->price_check_pending = 1;
        g_npc_interaction_state->pending_price = request->operand0;
        monster_info = GetNpcMonsterInfo(g_npc_interaction_state->dialogue_npc);
        if (monster_info != 0) {
            g_npc_interaction_state->pending_price -= static_cast<int>(
                monster_info->charm_strength * 0.01f * g_npc_interaction_state->pending_price);
        }
        g_npc_interaction_state->pending_price -=
            GetBestPartySkillLevel(W8_SKILL_COMMUNICATION, 0) *
            g_npc_interaction_state->pending_price / 500;
        if (g_npc_interaction_state->pending_price < 1) {
            g_npc_interaction_state->pending_price = 1;
        }
        if (g_npc_interaction_state->pending_price > 0x1e) {
            g_npc_interaction_state->pending_price =
                (g_npc_interaction_state->pending_price * 10 + 9) / 10;
        }
        if (request->kind == 0x1e) {
            g_npc_interaction_state->price_check_skip_fact = 1;
        }
    }
    SetTargetCursor(W8_CURSOR_NONE);
}

// FUNCTION: WIZ8 0x00576030
void SetNpcQuoteBubbleVisible(bool visible, const wchar_t* text, W8NpcScriptQuote* quote,
                              int quote_id, unsigned int font_palette)
{
    W8MessageBoxPayload payload;
    payload.text = 0;
    SetNpcQuoteBubbleVisible(visible, text, quote, quote_id, font_palette, 0, payload, -1);
}

// FUNCTION: WIZ8 0x00576060
void SetNpcQuoteBubbleVisible(bool visible, const wchar_t* text, W8NpcScriptQuote* quote,
                              int quote_id, unsigned int font_palette, unsigned char notice_kind,
                              W8MessageBoxPayload payload, int npc_kind)
{
    if (visible == g_npc_interaction_state->quote_visible) {
        return;
    }
    if (visible) {
        wchar_t normalized[2048];
        wchar_t error_text[200];
        unsigned short width;
        unsigned short height;

        if (g_mouselook_active) {
            EnableCursorScene();
            g_mouselook_active = 0;
            gfTrackMousePos = 0;
        }
        memset(normalized, 0, sizeof(normalized));
        wchar_t* output = normalized;
        for (unsigned int index = 0; index < wcslen(text); ++index) {
            if (text[index] == L'\n' && index > 0 && text[index - 1] != L' ') {
                *output++ = L' ';
            }
            *output++ = text[index];
        }
        g_npc_interaction_state->quote_bubble = LayoutPortraitQuoteBubble(
            -1, 0, 0, normalized, 280, 0, 0, 0, &width, &height, font_palette);
        if (g_npc_interaction_state->quote_bubble == -1) {
            if (g_npc_interaction_state->dialogue_npc != 0) {
                swprintf(error_text,
                         L"Error creating box - most likely text too large: NPC %s, quote %d",
                         g_npc_interaction_state->dialogue_npc->record->source_name, quote_id);
            } else {
                swprintf(error_text, L"Error creating box - most likely text too large");
            }
            g_npc_interaction_state->quote_bubble =
                LayoutPortraitQuoteBubble(-1, 0, 0, error_text, 300, 0, 0, 0, &width, &height, -1);
            ShowNotice(0xc, error_text, 0, GetTextBoxScrollRange(), 0);
        }
        g_npc_interaction_state->quote_width = width;
        g_npc_interaction_state->quote_height = height;
        g_npc_interaction_state->quote_x = 320 - (width >> 1);
        g_npc_interaction_state->quote_y = quote_id < 0 ? 350 - height : 20;
        SetRegionBounds(0x136, g_npc_interaction_state->quote_x, g_npc_interaction_state->quote_y,
                        g_npc_interaction_state->quote_x + g_npc_interaction_state->quote_width,
                        g_npc_interaction_state->quote_y + g_npc_interaction_state->quote_height);
        RegionSetEnable(0x25);
        EnableRegionSetInput(0x25);
        g_npc_interaction_state->quote_visible = true;
        if (notice_kind == 0) {
            W8PendingNoticeLine* line = new W8PendingNoticeLine;
            line->text = static_cast<wchar_t*>(malloc((wcslen(normalized) + 1) * sizeof(wchar_t)));
            line->npc_kind = npc_kind;
            wcscpy(line->text, normalized);
            g_npc_interaction_state->pending_notice_lines.Add(line);
        }
        g_npc_interaction_state->quote_notice_kind = notice_kind;
        g_npc_interaction_state->quote_notice_payload = payload;
        if (g_npc_interaction_state->quote_notice_kind == 3) {
            SoundPlay(reinterpret_cast<STR>(const_cast<char*>( // reinterpret-ok: SGP text ABI
                          "Data\\Sound\\Misc\\GainLevel.wav")),
                      0);
        }
        return;
    }

    bool flush_notices =
        (quote_id == 0x12 || quote_id < 0) && g_npc_interaction_state->quote_notice_kind == 0;
    switch (g_npc_interaction_state->quote_notice_kind) {
    case 1: {
        W8ExperienceNoticePayload* experience =
            g_npc_interaction_state->quote_notice_payload.experience;
        FormatNotice(0xc, 0, gppStringList[experience->alternate_message ? 0x231 : 0x232],
                     experience->amount);
        delete experience;
        break;
    }
    case 2: {
        W8SkillNoticePayload* skills = g_npc_interaction_state->quote_notice_payload.skill_notices;
        PostSkillIncreaseNotices(skills);
        delete skills;
        break;
    }
    case 3: {
        int* slot = g_npc_interaction_state->quote_notice_payload.level_up_slot;
        PostCharacterNotice(*slot, gppStringList[0x773]);
        delete slot;
        break;
    }
    }
    if (quote != 0) {
        /* index is int, not unsigned int: entry_count is an unsigned short,
           which promotes to int here, and the retail tests the count signed
           (0x00576521 mov ax,word ptr [ecx+0x9]; 0x00576525 test eax,eax;
           jle 0x00576543). An unsigned index would make the comparison
           unsigned and lose that. */
        for (int index = 0; index < quote->entry_count; ++index) {
            if (quote->entries[index].kind == 0x13 || quote->entries[index].kind == 5) {
                flush_notices = true;
            }
        }
    }
    if (flush_notices) {
        FlushPendingNoticeLines();
    }
    g_npc_interaction_state->quote_visible = false;
    if (g_npc_interaction_state->quote_bubble != -1) {
        ReleasePortraitQuoteBubble(g_npc_interaction_state->quote_bubble);
    }
    RegionSetDisable(0x25);
    DisableRegionSetInput(0x25);
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
        if (g_npc_interaction_state->quote_bubble != -1) {
            ClearSurfaceRect(
                g_npc_interaction_state->quote_x, g_npc_interaction_state->quote_y,
                g_npc_interaction_state->quote_x + g_npc_interaction_state->quote_width,
                g_npc_interaction_state->quote_y + g_npc_interaction_state->quote_height);
            InvalidateRegion(
                g_npc_interaction_state->quote_x, g_npc_interaction_state->quote_y,
                g_npc_interaction_state->quote_x + g_npc_interaction_state->quote_width,
                g_npc_interaction_state->quote_y + g_npc_interaction_state->quote_height, 0);
        }
        RequestRedraw(W8_MAIN_REDRAW_LAYOUT);
        RequestRedraw(W8_MAIN_REDRAW_PORTRAITS);
        RequestRedraw(W8_MAIN_REDRAW_PORTRAIT_PANEL);
    } else if (g_current_screen_state.id == W8_SCREEN_CAMP) {
        g_camp_screen->redraw_flags |= W8_CAMP_REDRAW_ALL;
    }
    g_npc_interaction_state->quote_bubble = -1;
}

// FUNCTION: WIZ8 0x00576650
unsigned char NpcQuoteBubbleRegionEvent(const InputAtom* event, W8Region*)
{
    if (event->usEvent != LEFT_BUTTON_DOWN) {
        return 0;
    }
    TryFinishNpcVoicePlayback(1);
    return 1;
}

// FUNCTION: WIZ8 0x00576670
void DrawNpcQuoteBubble(void)
{
    IsModalOpen();
    if (g_npc_interaction_state->quote_visible) {
        DrawPortraitQuoteBubble(g_npc_interaction_state->quote_bubble,
                                g_npc_interaction_state->quote_x, g_npc_interaction_state->quote_y,
                                -14);
    }
}

// FUNCTION: WIZ8 0x005766B0
void FlushPendingNoticeLines(void)
{
    wchar_t npc_name[100];
    int index;

    for (index = 0; index < g_npc_interaction_state->pending_notice_lines.GetCount(); ++index) {
        W8PendingNoticeLine* line = *g_npc_interaction_state->pending_notice_lines.GetAt(index);
        if (line->npc_kind != -1 &&
            line->npc_kind != g_npc_interaction_state->last_notice_npc_kind) {
            W8NpcState* npc = GetNpcState(line->npc_kind);
            if (npc != 0) {
                swprintf(npc_name, L"%s", npc->record->source_name);
                ShowNotice(1, npc_name, 3, -1, 0);
                g_npc_interaction_state->last_notice_npc_kind = line->npc_kind;
            }
        }
        ShowNotice(line->npc_kind == -1 ? 0xb : 0xf, line->text, 3, GetTextBoxScrollRange(), 0);
    }
    while (g_npc_interaction_state->pending_notice_lines.GetCount() > 0) {
        W8PendingNoticeLine* line = g_npc_interaction_state->pending_notice_lines.RemoveAt(0);
        free(line->text);
        delete line;
    }
}

// FUNCTION: WIZ8 0x005767f0
void LookAtDialogueNpc(void)
{
    W8MonsterInfo* info = GetNpcMonsterInfo(g_npc_interaction_state->dialogue_npc);
    if (info != 0) {
        srVector3T<float> position = info->p3D->movement.position;
        position.y += info->p3D->movement.height_offset;
        g_gd_camera->LookAt(&position, 0);
    }
}

/* Show (0) or hide (nonzero) the NPC dialogue UI: on show the party portrait
   region sets lose input, the dialogue regions and text controls come up, and
   the transcript expands; on hide the party sets come back, the transcript
   collapses, its background is cleared and the rectangle invalidated. The new
   state lands in dialogue_hidden. */
// FUNCTION: WIZ8 0x00576850
void SetNpcDialogueHidden(char value)
{
    int index;

    if (value != 0) {
        g_npc_interaction_state->saved_mode = g_settings.main_ui_mode;
        ApplyMainGameModeFlag(W8_MAIN_UI_MODE_PORTRAITS, 0);
        static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2])->SetEnabled(0);
        static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2])->Collapse();
        g_npc_interaction_state->dialogue_panels[3]->SetEnabled(0);
        for (index = 0; index < 8; ++index) {
            if (g_status.buffers.XChar[index].fOccupied) {
                RegionSetEnable(index + 7);
                EnableRegionSetInput(index + 7);
            }
        }
        RegionSetDisable(0x16);
        static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2])->ClearBackground();
        ClearSurfaceRect(0x1dc, 0x11b, 0x269, 0x1c2);
        InvalidateRegion(0x1dc, 0x11b, 0x269, 0x1c2, 0);
        RequestRedraw(W8_MAIN_REDRAW_PORTRAIT_1);
        RequestRedraw(W8_MAIN_REDRAW_PORTRAIT_3);
        RequestRedraw(W8_MAIN_REDRAW_PORTRAIT_5);
        RequestRedraw(W8_MAIN_REDRAW_PORTRAIT_7);
        SetInputFieldBlocksMouseCallback(0, 1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])->SetEnabled(0);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])->SetEnabled(0);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_3])->SetEnabled(0);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])->SetEnabled(0);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])->SetEnabled(0);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_6])->SetEnabled(0);
    } else {
        ApplyMainGameModeFlag(g_npc_interaction_state->saved_mode, 0);
        for (index = 0; index < 8; ++index) {
            if (g_status.buffers.XChar[index].fOccupied) {
                RegionSetDisable(index + 7);
                DisableRegionSetInput(index + 7);
            }
        }
        RegionSetEnable(0x16);
        RegionSetEnable(0x18);
        RegionSetEnable(0x15);
        EnableRegionInput(0x52);
        EnableRegionInput(0x53);
        EnableRegionInput(0x54);
        EnableRegionInput(0x55);
        g_level_block->action_panel_visible = 1;
        static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2])->SetEnabled(1);
        g_npc_interaction_state->dialogue_panels[3]->SetEnabled(1);
        static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2])->Expand();
        SyncNpcDialogueTranscriptScrollButtons();
        SetInputFieldBlocksMouseCallback(0, 0);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_1])->SetEnabled(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_2])->SetEnabled(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_3])->SetEnabled(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_4])->SetEnabled(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_5])->SetEnabled(1);
        static_cast<W8TextControl*>(g_npc_interaction_state->dialogue_controls[W8_NPC_CONTROL_TEXT_6])->SetEnabled(1);
    }
    RequestRedraw(W8_MAIN_REDRAW_LAYOUT);
    g_npc_interaction_state->dialogue_hidden = value;
}

/* Destroy callback OpenNpcDialog installs on the modal: pops or re-schemes the
   text-input level the dialog pushed, restores the quote bubble, and routes the
   choice back to the script - the picked option's label, the price-check
   yes/no string, or the typed keyword text. Inside a live dialogue the text is
   injected into input field 0 and processed as if typed; otherwise it is
   submitted to the script line queue directly. */
// FUNCTION: WIZ8 0x00576BA0
void ResolveNpcPickpocket(int party_slot)
{
    W8Character* character = &g_status.buffers.Char[party_slot];
    unsigned int gold;
    W8ItemInstance item;
    wchar_t text[200];
    int slot;

    switch (AttemptNpcPickpocket(character, g_npc_interaction_state->dialogue_npc, &item, &gold)) {
    case 0:
        swprintf(text, gppStringList[0x74d], character->name, GetItemDisplayName(&item));
        for (slot = 0; slot < 8; ++slot) {
            if (character->backpack[slot].iItemNo == -1) {
                AddItemToCharacter(character, &item, 0, 0, 0);
                DisplayNpcQuote(text, 1);
                return;
            }
        }
        AddItemToParty(&item, 0, 0);
        DisplayNpcQuote(text, 1);
        return;
    case 1:
        swprintf(text, gppStringList[0x74e], character->name, gold);
        AddPartyGold(gold, 0);
        DisplayNpcQuote(text, 1);
        return;
    case 2:
        swprintf(text, gppStringList[0x74f], character->name);
        DisplayNpcQuote(text, 0);
        return;
    case 3:
        QueueNpcScriptLine(0x17, 0, 0, 0);
        SetNpcDispositionBand(g_npc_interaction_state->dialogue_npc, 1);
        ApplyFactionChange(3, 1, g_npc_interaction_state->dialogue_npc->record->faction, -5);
        CloseNpcDialogueTranscriptLayout();
        ShowNpcDialogueTopicMenu();
        return;
    case 4:
        DisplayNpcQuote(gppStringList[0x750], 0);
        return;
    default:
        return;
    }
}

// FUNCTION: WIZ8 0x00576DA0
void QueueDialogueNpcRefusal(void)
{
    unsigned int flags = g_npc_interaction_state->dialogue_npc->refusal_flags;
    if ((flags & 1) == 0) {
        QueueNpcScriptLine(0x67, 0, 0, 0);
        g_npc_interaction_state->dialogue_npc->refusal_flags |= 1;
        return;
    }
    if ((flags & 2) == 0) {
        QueueNpcScriptLine(0x68, 0, 0, 0);
        g_npc_interaction_state->dialogue_npc->refusal_flags |= 2;
        return;
    }
    QueueNpcScriptLine(0x69, 0, 0, 0);
}

// FUNCTION: WIZ8 0x00576E20
void OnNpcDialogClosed(W8DialogBase* dialog)
{
    W8NpcDialog* npc_dialog = static_cast<W8NpcDialog*>(dialog);
    W8NpcQuoteEntry* request = npc_dialog->m_message;
    wchar_t field_text[200];
    wchar_t entry_text[1020];
    int index;

    if (!gXStatus.fNpcDialogueMode || g_npc_interaction_state->scripted_dialogue) {
        KillTextInputMode();
    } else {
        SetTextInputScheme(1);
    }
    RestoreCurrentNpcQuoteBubble();
    if (g_npc_interaction_state->dialogue_layout == W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX) {
        CloseNpcDialogueOptionLayout();
        OpenNpcDialogueTranscriptLayout();
    }
    if (request->kind == 5) {
        for (index = 0; index < request->sub_entry_count; ++index) {
            if (index == npc_dialog->m_selected_option) {
                swprintf(entry_text, L"%S", request->sub_entries[index].text);
                if (!gXStatus.fNpcDialogueMode || g_npc_interaction_state->scripted_dialogue) {
                    HandleNpcDialogueReply(entry_text, 0);
                } else {
                    Get16BitStringFromField(0, field_text);
                    StripNpcKeywordPunctuation(entry_text);
                    static_cast<void>(wcslen(field_text));
                    SetInputFieldStringWith16BitString(0, entry_text);
                    HandleNpcDialogueInput();
                }
                break;
            }
        }
    } else if (request->kind == 0x12 || request->kind == 0x1e) {
        if (npc_dialog->m_selected_option == 0) {
            wcscpy(entry_text, gppStringList[0x7df]);
        } else {
            wcscpy(entry_text, gppStringList[0x7e0]);
        }
        if (!gXStatus.fNpcDialogueMode || g_npc_interaction_state->scripted_dialogue) {
            HandleNpcDialogueReply(entry_text, 0);
        } else {
            Get16BitStringFromField(0, field_text);
            StripNpcKeywordPunctuation(entry_text);
            static_cast<void>(wcslen(field_text));
            SetInputFieldStringWith16BitString(0, entry_text);
            HandleNpcDialogueInput();
        }
    } else if (request->kind == 0x13) {
        if (!gXStatus.fNpcDialogueMode || g_npc_interaction_state->scripted_dialogue) {
            HandleNpcDialogueReply(npc_dialog->m_input_text, 0);
        } else {
            Get16BitStringFromField(0, field_text);
            StripNpcKeywordPunctuation(npc_dialog->m_input_text);
            static_cast<void>(wcslen(field_text));
            SetInputFieldStringWith16BitString(0, npc_dialog->m_input_text);
            HandleNpcDialogueInput();
        }
    }
    g_npc_interaction_state->script_busy = 0;
}

/* The camp-side mirror of SwitchNpcDialogueLayout: camp mode is raised, the current
   dialogue layout is retired, and a still-pending item goes back onto the
   item cursor. */
// FUNCTION: WIZ8 0x00577020
void CloseNpcDialogueForCamp(void)
{
    gXStatus.fCampMode = true;
    SwitchNpcDialogueLayout(W8_DIALOGUE_LAYOUT_NONE);
    EndNpcDialogueSession(0);
    if (g_npc_interaction_state->held_item_pending) {
        g_status.item_in_hand = g_npc_interaction_state->pending_item;
        SetItemCursor(0);
        return;
    }
    SetTargetCursor(W8_CURSOR_NONE);
}

// FUNCTION: WIZ8 0x00577220
void SyncDialogueNpcStateAndMarkPending(void)
{
    SyncDialogueNpcState();
    g_npc_interaction_state->pending_trade_toggle = 1;
}

// FUNCTION: WIZ8 0x00577260
void SyncDialogueNpcState(void)
{
    BeginNpcDialogueInternal(g_npc_interaction_state->dialogue_npc, 0, -1, 0, 1);
    g_npc_interaction_state->pending_layout = g_npc_interaction_state->previous_dialogue_layout;
}

/* The dialogue NPC takes the guard script when it is '*' styled and the party
   walks away; otherwise its record flags drive either a combat notice or the
   queued scripted action named by the record. While the dialogue is still up
   the named-action queue hands the speaker's name to 0x00571660 instead.
   Every occupied living character without a maxed condition practices
   communication (skill 0x16). */
// FUNCTION: WIZ8 0x00577290
void HandleNpcDialogueDeparture(unsigned char value)
{
    W8MonsterInfo* info;
    W8Character* character;
    int index;

    if (g_npc_interaction_state->dialogue_npc->name_style == 0x2a &&
        (info = GetNpcMonsterInfo(g_npc_interaction_state->dialogue_npc)) != 0) {
        info->p3D->SetScript("Guard.msf", 1);
    }
    if ((value == 0 || !g_npc_interaction_state->dialogue_npc->dismissed_flag ||
         g_npc_interaction_state->dialogue_npc->record->allow_dismissed_departure_dialogue != 0) &&
        g_npc_interaction_state->transcript_open_count < 1) {
        if (!g_npc_interaction_state->dialogue_npc->greeting_pending) {
            QueueNpcScriptLine(1, 0, 0, 0);
        } else {
            QueueNpcScriptLine(0, 0, 0, 0);
            g_npc_interaction_state->dialogue_npc->greeting_pending = 0;
            if (g_npc_interaction_state->dialogue_npc->record->monster_bound == 0 &&
                g_npc_interaction_state->dialogue_npc->record->voice_script == 0 &&
                g_npc_interaction_state->dialogue_npc->record->merchant == 0) {
                for (index = 0; index < 8; ++index) {
                    character = &g_status.buffers.Char[index];
                    if (g_status.buffers.XChar[index].fOccupied && character->hp_current != 0 &&
                        character->highest_condition < W8_CONDITION_ASLEEP) {
                        PracticeCharacterSkill(character, W8_SKILL_COMMUNICATION, 0xf, 0);
                    }
                }
            }
            if (gXStatus.fNpcDialogueMode && !g_npc_interaction_state->scripted_dialogue) {
                AddNpcDialogueKeyword(
                    g_npc_interaction_state->dialogue_npc->record->source_name, -1, 1);
                return;
            }
            if (g_npc_interaction_state->dialogue_npc->record->monster_bound == 0 &&
                (g_npc_interaction_state->dialogue_npc->record->voice_script == 0 ||
                 g_npc_interaction_state->dialogue_npc->is_present)) {
                AddDialogueTranscriptKeyword(
                    g_npc_interaction_state->dialogue_npc->record->source_name, -1);
            }
        }
    }
}

/* Shared by the main-game screen and dialog text entries; it lives with the
   main-game text helpers, not with UtilityFunctions.cpp. */
// FUNCTION: WIZ8 0x00577410
void ShortenTextToWidth(wchar_t* output, const wchar_t* text, unsigned int width, int font)
{
    wchar_t buffer[200];
    wcscpy(buffer, text);
    if (static_cast<unsigned int>(StringPixLength(buffer, font)) < width) {
        wcscpy(output, buffer);
        return;
    }
    for (int index = 0; index < static_cast<int>(wcslen(buffer)); ++index) {
        if (width <= static_cast<unsigned int>(StringPixLengthArg(font, index + 1, buffer))) {
            --index;
            while (index >= 0) {
                if (buffer[index] != L' ' && buffer[index - 1] != L' ') {
                    buffer[index] = L'\0';
                    swprintf(output, L"%s...", buffer);
                    return;
                }
                --index;
            }
            return;
        }
    }
}

// FUNCTION: WIZ8 0x00577520
void BeginScriptedWorldAction(void)
{
    g_status.world_cursor_gate = 1;
    ResetLevelDataVectors();
    SetTargetCursor(W8_CURSOR_MAP_LOAD);
}

// FUNCTION: WIZ8 0x00577540
void ClearMainGameTargetState(void)
{
    g_status.world_cursor_gate = 0;
    ClearLevelMovementStopped();
    SetTargetCursor(W8_CURSOR_NONE);
}

/* When the world-cursor gate (world_cursor_gate) is raised, discard queued input
   after refreshing the mouse-system position so stale events do not fire. */
// FUNCTION: WIZ8 0x00577560
void FlushInputWhileWorldCursorGate(void)
{
    POINT mouse;
    InputAtom input;

    if (g_status.world_cursor_gate == 0) {
        return;
    }
    SGPMouseGetPos(&mouse);
    MSYS_SGP_Mouse_Handler_Hook(MOUSE_POS, static_cast<unsigned short>(mouse.x),
                                static_cast<unsigned short>(mouse.y), gfLeftButtonState,
                                gfRightButtonState);
    while (DequeueEvent(&input) == 1) {
    }
}

// FUNCTION: WIZ8 0x005775d0
void AddDialogueTranscriptKeyword(const wchar_t* name, signed char category)
{
    wchar_t keyword[100];
    wcscpy(keyword, name);
    StripNpcKeywordPunctuation(keyword);
    if (category == -1) {
        unsigned int index;
        for (index = 0; index < gXStatus.uiItemsInDatabase; ++index) {
            if (CompareWideTextIgnoreAsciiCase(keyword, g_item_records[index].display_name) == 0) {
                category = W8_DIALOGUE_CATEGORY_ITEMS;
                break;
            }
        }
        if (category == -1) {
            for (index = 0; index < gXStatus.uiNpcsInDatabase; ++index) {
                if (CompareWideTextIgnoreAsciiCase(keyword, g_npc_records[index].source_name) ==
                    0) {
                    category = W8_DIALOGUE_CATEGORY_PEOPLE;
                    break;
                }
            }
        }
        if (category == -1) {
            for (index = 0; g_dialogue_person_keywords[index][0] != 0; ++index) {
                if (CompareWideTextIgnoreAsciiCase(keyword, g_dialogue_person_keywords[index]) ==
                    0) {
                    category = W8_DIALOGUE_CATEGORY_PEOPLE;
                    break;
                }
            }
        }
        if (category == -1) {
            category = IsDialoguePlaceKeyword(keyword) ? W8_DIALOGUE_CATEGORY_PLACES
                                                       : W8_DIALOGUE_CATEGORY_MISC;
        }
    }
    for (int index = 0; index < g_npc_interaction_state->dialogue_transcript.GetCount(); ++index) {
        if (CompareWideTextIgnoreAsciiCase(
                (*g_npc_interaction_state->dialogue_transcript.GetAt(index))->text, keyword) == 0) {
            return;
        }
    }
    W8DialogueTranscriptRecord* record =
        static_cast<W8DialogueTranscriptRecord*>(malloc(sizeof(W8DialogueTranscriptRecord)));
    memset(record, 0, sizeof(*record));
    wcscpy(record->text, keyword);
    record->category = category;
    g_npc_interaction_state->dialogue_transcript.Add(record);
}

// FUNCTION: WIZ8 0x005777c0
void RecordLevelEntryDialogueState(void)
{
    wchar_t region_name[100];
    int region = GetLevelBand(g_status.current_level);
    if (GetNpcScriptRegionName(region, region_name)) {
        AddDialogueTranscriptKeyword(region_name, W8_DIALOGUE_CATEGORY_PLACES);
    }
    if (region == 14) {
        SetFact(W8_FACT_QUEST_ASCEND_TO_CIRCLE, 0, 0);
        if (!NpcLeadHasNameStyle(0x18)) {
            SetFact(W8_FACT_VI_IS_DEAD, 1, 0);
        }
    }
}

/* Same NPC-dialogue text-box-layout predicate as IsNpcDialogueTextBoxActive,
   emitted as a second copy for the dialogue text input callers. */
// FUNCTION: WIZ8 0x00577830
bool IsNpcDialogueTextInputActive(void)
{
    return gXStatus.fNpcDialogueMode &&
           g_npc_interaction_state->dialogue_layout == W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX;
}

// FUNCTION: WIZ8 0x00577850
bool CanOpenNpcDialogue(void)
{
    return gXStatus.fNpcDialogueMode && g_npc_interaction_state->scripted_dialogue;
}

// FUNCTION: WIZ8 0x00577880
unsigned char SetNpcDialoguePanelVisible(unsigned char value)
{
    W8NpcDialogueTextController* controller;

    if (gXStatus.fNpcDialogueMode && !g_npc_interaction_state->scripted_dialogue &&
        g_npc_interaction_state->dialogue_layout == W8_DIALOGUE_LAYOUT_TRANSCRIPT) {
        if (value == 0) {
            controller = static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2]);
            controller->Collapse();
            controller = static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2]);
            controller->SetEnabled(0);
            g_npc_interaction_state->dialogue_panels[3]->SetEnabled(0);
            controller = static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2]);
            controller->ClearBackground();
            ClearSurfaceRect(0x1dc, 0x11b, 0x269, 0x1c2);
            InvalidateRegion(0x1dc, 0x11b, 0x269, 0x1c2, 0);
            RequestRedraw(W8_MAIN_REDRAW_PORTRAIT_1);
            RequestRedraw(W8_MAIN_REDRAW_PORTRAIT_3);
            RequestRedraw(W8_MAIN_REDRAW_PORTRAIT_5);
            RequestRedraw(W8_MAIN_REDRAW_PORTRAIT_7);
            RequestRedraw(W8_MAIN_REDRAW_LAYOUT);
            g_npc_interaction_state->dialogue_panel_hidden = 1;
            return 1;
        }

        controller = static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2]);
        controller->SetEnabled(1);
        g_npc_interaction_state->dialogue_panels[3]->SetEnabled(1);
        controller = static_cast<W8NpcDialogueTextController*>(g_npc_interaction_state->dialogue_panels[2]);
        controller->Expand();
        SyncNpcDialogueTranscriptScrollButtons();
        RequestRedraw(W8_MAIN_REDRAW_LAYOUT);
        g_npc_interaction_state->dialogue_panel_hidden = 0;
        return 1;
    }
    return 0;
}
// FUNCTION: WIZ8 0x00577A20
bool FinishNpcVoiceIfSessionActive(void)
{
    if (!IsNpcScriptSessionActive()) {
        return false;
    }
    TryFinishNpcVoicePlayback(0);
    return true;
}

// FUNCTION: WIZ8 0x00577A40
bool ProcessPendingEvent(void)
{
    if (IsNpcScriptSessionActive()) {
        TryFinishNpcVoicePlayback(1);
        return 1;
    }
    if (gXStatus.character_event_queue->HasActiveEvents()) {
        gXStatus.character_event_queue->CompleteFirstActiveEvent();
        return 1;
    }
    return 0;
}

// VTABLE: WIZ8 0x005ee9d0
// class W8GrowableVector<W8DialogueTranscriptRecord*>

// VTABLE: WIZ8 0x005ee9d4
// class W8GrowableVector<W8PendingNoticeLine*>

// VTABLE: WIZ8 0x005ee9d8
// class W8Vector<W8PendingNoticeLine*>

// VTABLE: WIZ8 0x005ee9dc
// class W8Vector<W8DialogueTranscriptRecord*>

// VTABLE: WIZ8 0x005ee9e0
// class W8GrowableVector<W8GrowableVector<W8GrowableVector<wchar_t*>*>*>

/* The keyword-list instantiations' wchar_t element type canonicalizes to
   unsigned short under the VC6 ABI spelling used by the PDB names. */
// VTABLE: WIZ8 0x005ee9fc
// class W8GrowableVector<unsigned short*>

// VTABLE: WIZ8 0x005eea00
// class W8GrowableVector<W8GrowableVector<unsigned short*>*>
