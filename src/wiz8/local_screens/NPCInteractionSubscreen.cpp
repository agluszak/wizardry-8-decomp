#include "line.h"
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
W8MainScreenState g_screen_state_storage_0068ee90;
/* The global object's implicit constructor and destructor; 0x0056B930 is the
   retail static initializer that runs the former and registers the latter. */
// SYNTHETIC: WIZ8 0x0056b960
// W8MainScreenState::~W8MainScreenState

// SYNTHETIC: WIZ8 0x0056b9a0
// W8MainScreenState::W8MainScreenState

// GLOBAL: WIZ8 0x00649f1c
W8MainScreenState* g_screen_state_00649f1c = &g_screen_state_storage_0068ee90;
/* 0x0068EE80: the dialogue keyword tables. Element zero is the English file
   list and element one the translated one; each file list holds one line list
   per line and each line list one word per field. */
// GLOBAL: WIZ8 0x0068EE80
W8GrowableVector<W8GrowableVector<W8GrowableVector<wchar_t*>*>*> g_keyword_lists;
/* 0x0068F0F8: both keyword files are loaded and the tables are usable. */
// GLOBAL: WIZ8 0x0068F0F8
unsigned char g_keyword_lists_loaded_68f0f8;
/* 0x0068F0F9: the keyword subsystem's active flag, written absolutely by the
   screen reset and by the keyword panel helpers. */
// GLOBAL: WIZ8 0x0068F0F9
unsigned char g_flag_68f0f9;
/* 0x0068EE58: empty wide string used to clear dialogue editor text. */
// GLOBAL: WIZ8 0x0068EE58
wchar_t g_wchar_0068ee58[4];
/* 0x0068EE60: the queued NPC script notice; see the type comment in the
   header. */
// GLOBAL: WIZ8 0x0068EE60
W8PendingNotice g_pending_notice_68ee60;
/* 0x0068EE78: GetTickCount sample for the trade-item highlight timeout. */
// GLOBAL: WIZ8 0x0068EE78
unsigned int g_trade_highlight_tick_68ee78;

/* The split-amount dialog's confirm code and its default origin; only the
   0x572780/0x572870 pair reads them. */
// GLOBAL: WIZ8 0x005EF9D4
int g_split_dialog_confirm_005ef9d4 = 1;
// GLOBAL: WIZ8 0x005EF9DC
int g_split_dialog_origin_x_005ef9dc = 0x9f;
// GLOBAL: WIZ8 0x005EF9E0
int g_split_dialog_origin_y_005ef9e0 = 0xb8;

// GLOBAL: WIZ8 0x00649f20
int g_dialogue_place_keyword_count = 15;
// GLOBAL: WIZ8 0x00649f24
int g_dialogue_place_keyword_ids[15] = {0x751, 0x752, 0x753, 0x754, 0x755, 0x756, 0x757, 0x758,
                                        0x759, 0x75a, 0x75b, 0x75c, 0x75d, 0x75e, 0x75f};
// GLOBAL: WIZ8 0x00649F64
int g_dialogue_fallback_ids_00649f64[5] = {0x760, 0x761, 0x762, 0x763, 0x764};
// GLOBAL: WIZ8 0x00649F78
int g_dialogue_fallback_ids_00649f78[5] = {0x765, 0x766, 0x767, 0x768, 0x769};
// GLOBAL: WIZ8 0x00649f8c
const wchar_t* g_dialogue_person_keywords[] = {L"BALBRAK", L"BILDUBLU", L"EWAXX",  L"KUNAR",
                                               L"PANRACK", L"RODAN",    L"RUBBLE", L"SAXX",
                                               L"SPARKLE", L"YAMIR",    L""};

/* Enabling starts text-input scheme 1 and installs the typed-dialogue field;
   disabling removes it. An already-enabled panel does none of this. */
// FUNCTION: WIZ8 0x0056BAC0
void W8MainGamePanel005EE9E4::SetEnabled(bool enable)
{
    if (!enable || !m_fEnabled) {
        Controls::SetEnabled(enable);
        if (enable) {
            InitTextInputModeWithScheme(1);
            AddTextInputField(0x1e5, 0x170, 0x7a, 0x12, 0x7f, &g_wchar_00689b34, 0xbe, 0xf, 1);
        } else {
            RemoveTextInputField(0);
        }
    }
}

/* Base redraw plus the input-frame image: drawn at y 0x19b while flag_1d9 is
   raised, else 0x18b. */
// FUNCTION: WIZ8 0x0056BB20
void W8MainGamePanel005EE9E4::Redraw()
{
    int redrawn = 0;
    int index;

    if (!m_fEnabled) {
        return;
    }
    if (m_fDirty) {
        if (m_renderTarget != -1) {
            DrawCatalogImage(-14, m_renderTarget, m_renderArg_1c, m_renderArg_20, origin_x,
                             origin_y, 2, 0);
        }
        DrawCatalogImage(-14, 0x1a9, 0, 0x10, 0x1df,
                         g_screen_state_00649f1c->flag_1d9 ? 0x19b : 0x18b, 2, 0);
        if (m_fWholeAreaDirty) {
            if (m_renderTarget != -1) {
                InvalidateCatalogImageRect(m_renderTarget, m_renderArg_1c, m_renderArg_20, origin_x,
                                           origin_y, 2);
            }
        } else {
            InvalidateRegion(m_dirtyRect.left, m_dirtyRect.top, m_dirtyRect.right,
                             m_dirtyRect.bottom, 2);
        }
        m_fDirty = 0;
        m_dirtyRect.left = -1;
        redrawn = 1;
    } else if (!m_fLayoutDirty) {
        return;
    }
    for (index = 0; index < m_controls.count; ++index) {
        if (ControlAt(index)->m_active) {
            ControlAt(index)->Redraw(redrawn);
        }
    }
    m_fLayoutDirty = 0;
}
/* Unlike the base, the six option buttons stay inactive while the expanded
   NPC dialogue layout (value_fc == W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX) is not up. */
// FUNCTION: WIZ8 0x0056BC50
void W8MainGamePanel005EE9F0::SetEnabled(bool enable)
{
    int index;

    m_fEnabled = enable;
    for (index = 0; index < m_controls.count; ++index) {
        if (enable &&
            (ControlAt(index) == g_screen_state_00649f1c->option_buttons_170[0] ||
             ControlAt(index) == g_screen_state_00649f1c->option_buttons_170[1] ||
             ControlAt(index) == g_screen_state_00649f1c->option_buttons_170[2] ||
             ControlAt(index) == g_screen_state_00649f1c->option_buttons_170[3] ||
             ControlAt(index) == g_screen_state_00649f1c->option_buttons_170[4] ||
             ControlAt(index) == g_screen_state_00649f1c->option_buttons_170[5]) &&
            g_screen_state_00649f1c->value_fc != W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX) {
            continue;
        }
        ControlAt(index)->SetActive(enable);
    }
}

/* Base redraw except the foreground catalog image is m_value_4c rather than
   m_renderArg_20 while the expanded dialogue layout is up. */
// FUNCTION: WIZ8 0x0056BD30
void W8MainGamePanel005EE9F0::Redraw()
{
    int redrawn = 0;
    int index;

    if (!m_fEnabled) {
        return;
    }
    if (m_fDirty) {
        if (m_renderTarget != -1) {
            DrawCatalogImage(-14, m_renderTarget, m_renderArg_1c,
                             g_screen_state_00649f1c->value_fc == W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX
                                 ? m_value_4c
                                 : m_renderArg_20,
                             origin_x, origin_y, 2, 0);
        }
        if (m_fWholeAreaDirty) {
            if (m_renderTarget != -1) {
                InvalidateCatalogImageRect(m_renderTarget, m_renderArg_1c, m_renderArg_20, origin_x,
                                           origin_y, 2);
            }
        } else {
            InvalidateRegion(m_dirtyRect.left, m_dirtyRect.top, m_dirtyRect.right,
                             m_dirtyRect.bottom, 2);
        }
        m_fDirty = 0;
        m_dirtyRect.left = -1;
        redrawn = 1;
    } else if (!m_fLayoutDirty) {
        return;
    }
    for (index = 0; index < m_controls.count; ++index) {
        if (ControlAt(index)->m_active) {
            ControlAt(index)->Redraw(redrawn);
        }
    }
    m_fLayoutDirty = 0;
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
            entry->count = 0;
            delete entry;
        }
        delete file;
    }
    g_keyword_lists.count = 0;
    g_keyword_lists_loaded_68f0f8 = 0;
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
    g_keyword_lists_loaded_68f0f8 = 1;
}

/* Translate a typed dialogue keyword through the loaded tables. With no
   tables loaded the input passes through verbatim; otherwise the active
   language's list is scanned and the matching English field is copied out.
   Element one holds the translated file when two loaded, falling back to the
   English list through GetAt's clamped read. */
// FUNCTION: WIZ8 0x0056c440
void TranslateDialogueKeyword0056C440(const wchar_t* source, wchar_t* destination)
{
    W8GrowableVector<W8GrowableVector<wchar_t*>*>* file;
    W8GrowableVector<wchar_t*>* entry;
    W8GrowableVector<wchar_t*>* english;
    int entry_index;
    int word_index;

    if (g_keyword_lists_loaded_68f0f8 == 0) {
        wcscpy(destination, source);
        return;
    }
    file = *g_keyword_lists.GetAt(1);
    for (entry_index = 0; entry_index < file->count; ++entry_index) {
        entry = *file->GetAt(entry_index);
        for (word_index = 0; word_index < entry->count; ++word_index) {
            if (CompareWideTextIgnoreAsciiCase00402920(source, *entry->GetAt(word_index)) == 0) {
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

    memset(static_cast<void*>(g_screen_state_00649f1c), 0, sizeof(W8MainScreenState));
    g_screen_state_00649f1c->dialogue_category_filter = unset;
    g_screen_state_00649f1c->transcript_sorted = 0;
    g_screen_state_00649f1c->flag_234 = 0;
    g_screen_state_00649f1c->value_258 = unset;
    g_screen_state_00649f1c->flag_260 = 1;
    g_status_685170.selected_party_member_2434 = 0xff;
    g_flag_68f0f9 = 0;
    ReloadKeywordLists();
}

/* Forward a monster-script notice to the targeting layer unless the screen is
   busy or this NPC kind suppresses it. The suppress flag travels as an int:
   the body forwards the whole dword without masking. */
// FUNCTION: WIZ8 0x0056C590
void ForwardNpcScriptNotice(W8NpcState* npc, W8ItemInstance* item, int line, int suppress)
{
    if (gXStatus.fNpcDialogueMode == 0 && gXStatus.fCombatMode == 0 &&
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
void QueueNpcScriptNotice(W8NpcState* npc, W8ItemInstance* item, int line, int suppress, int arg)
{
    W8MonsterInfo* info;
    unsigned char flag;

    if (FindNpcOfKind(npc->name_style) != 0 && npc->record->unknown_054 == 0) {
        return;
    }
    info = GetNpcMonsterInfo(npc);
    if (info != 0 && info->highest_condition >= 0xf) {
        return;
    }
    flag = static_cast<unsigned char>(suppress);
    if ((npc->name_style == W8_NPC_DRAZIC || npc->name_style == W8_NPC_RODAN) &&
        GetFact(W8_FACT_PEACE_ACHIEVED) != 0) {
        flag = 1;
        line = npc->name_style == W8_NPC_DRAZIC ? 0x23 : 0x1d;
    }
    if (g_flag_68f0f9 != 0) {
        return;
    }
    g_pending_notice_68ee60.flag = flag;
    g_pending_notice_68ee60.npc = npc;
    g_pending_notice_68ee60.line = line;
    g_pending_notice_68ee60.force = static_cast<unsigned char>(arg);
    if (item != 0) {
        g_pending_notice_68ee60.item = *item;
    } else {
        EmptyItemRecord(&g_pending_notice_68ee60.item, 0, 1);
    }
    QueueNpcMessageLine(W8_NPC_MSG_DISPATCH_PENDING_NOTICE, 0);
    g_flag_68f0f9 = 1;
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
void BeginNpcDialogueInternal(W8NpcState* npc, W8ItemInstance* item, int quote, int flags,
                              int force)
{
    W8MainScreenState* state;
    W8MonsterInfo* info;
    W8NpcState* bound;
    W8NpcState* selected;
    W8Character* characters;
    int slot;
    int condition;
    srVector3T<float> position;

    g_flag_68f0f9 = 0;
    g_screen_state_00649f1c->value_25c = 0;
    if (npc->name_style == 0xf || npc->name_style == 0x12) {
        characters = g_status_685170.buffers.characters;
        for (slot = 0; slot < 2; ++slot) {
            if (g_status_685170.buffers.party_rows[slot].occupied != 0) {
                bound = GetNpcState(g_status_685170.buffers.party_rows[slot].animation_0fa);
                if ((bound->name_style == 0x11 || bound->name_style == 0x10) &&
                    characters[slot].highest_condition >= 0xf) {
                    for (condition = 0; condition <= 0x12; ++condition) {
                        if (characters[slot].condition_turns[condition] != 0) {
                            RemoveCharacterCondition(slot, condition, 0);
                            if (condition == 0x12) {
                                characters[slot].hp_current = 10;
                            }
                        }
                    }
                }
            }
        }
    }
    if (gXStatus.character_event_queue->HasActiveEvents() != 0) {
        gXStatus.character_event_queue->CompleteFirstActiveEvent();
    }
    if (gXStatus.fCampMode == 0) {
        SelectNpcDialogueSpeaker(npc, flags);
    }
    if (force != 0) {
        OpenNpcDialoguePanel(npc, item, 1);
        return;
    }
    if (npc->record->unknown_054 == 0 && npc->record->flag_2ea == 0) {
        if (GetNpcDispositionBand(npc) == 2 && npc->record->unknown_056 == 0) {
            QueueNpcScriptLine(0x18, 0, 0, 0);
            return;
        }
        if (flags == 0) {
            OpenNpcDialoguePanel(npc, item, 0);
            return;
        }
    }
    if (npc->record->flag_2ea != 0) {
        if (item != 0) {
            state = g_screen_state_00649f1c;
            state->pending_item_1ed = *item;
            if (g_status_685170.item_in_cursor != 0) {
                state->flag_1f9 = 1;
            }
            HandleNpcDialogueItem(&state->pending_item_1ed);
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
    selected = g_screen_state_00649f1c->dialogue_npc;
    if (selected->name_style != 0x84 && selected->name_style != 0x85) {
        info = GetNpcMonsterInfo(selected);
        if (info != 0) {
            MonsterForwardReferencePosition(info->monster, 0);
        }
    }
    PauseMainGameWorld();
    state = g_screen_state_00649f1c;
    state->flag_252 = 1;
    gXStatus.fNpcDialogueMode = 1;
    state->value_25c = 0;
    info = GetNpcMonsterInfo(state->dialogue_npc);
    if (info == 0) {
        return;
    }
    position = info->monster->movement_0c0.position_040;
    position.y += info->monster->movement_0c0.height_offset_0b8;
    g_gd_camera_65a0f8->LookAt(&position, 0);
}

// FUNCTION: WIZ8 0x0056CA60
void BeginNpcDialogue(W8NpcState* npc, W8ItemInstance* item, int quote, int flags, int force)
{
    BeginNpcDialogueInternal(npc, item, quote, flags, force);
}

/* Dispatch the queued NPC script notice: the item goes across only while it
   still carries an id, and the flag pair at +0x14 travels as one dword. */
// FUNCTION: WIZ8 0x0056CA90
void DispatchPendingNpcScriptNotice(void)
{
    W8ItemInstance* item;
    int flags;

    item = 0;
    if (g_pending_notice_68ee60.item.item_id != -1) {
        item = &g_pending_notice_68ee60.item;
    }
    flags = *reinterpret_cast<int*>(&g_pending_notice_68ee60.flag); /* reinterpret-ok: the queued
            flag/force bytes are dispatched to BeginNpcDialogueInternal as one packed dword */
    BeginNpcDialogueInternal(g_pending_notice_68ee60.npc, item, g_pending_notice_68ee60.line, flags,
                             (flags >> 8) & 0xff);
}

/* Open the NPC dialogue panel. After the shared screen reset and the
   dialogue-UI build, a carried item goes through the pending-item path -
   inspecting it decides between the trade switch and a disposition check -
   while everything else falls to the force gate and then the
   dispatch: a record-0x056 NPC takes the plain quote, otherwise the
   disposition band picks the hostile or friendly entry. */
// FUNCTION: WIZ8 0x0056CAD0
unsigned char OpenNpcDialoguePanel(W8NpcState* npc, W8ItemInstance* item, unsigned char force)
{
    W8MainScreenState* state;
    W8MonsterInfo* info;
    W8MonsterInfo* dialogue_info;
    unsigned char band;
    wchar_t space[2];
    srVector3T<float> position;

    UpdateScreenOverlays(0);
    gXStatus.fNpcDialogueMode = 1;
    CloseMainGameOverlays();
    if (npc->record->flag_055 != 0) {
        RestockNpcInventory(npc);
    }
    state = g_screen_state_00649f1c;
    state->value_fc = W8_DIALOGUE_LAYOUT_NONE;
    if (gXStatus.fCampMode == 0) {
        state->value_104 = 0;
        state->value_100 = 0;
    }
    state->dialogue_panel_hidden = 0;
    state->flag_252 = 0;
    state->value_108 = 0;
    state->value_1c4 = -1;
    state->value_1c8 = -1;
    state->value_1cc = -1;
    state->value_1d0 = 0;
    state->value_000 = 0;
    state->flag_1d9 = 0;
    state->flag_1f9 = 0;
    state->script_busy = 0;
    state->flag_200 = 0;
    state->flag_201 = 0;
    state->dialogue_cursor_flag = 0;
    state->flag_250 = 0;
    state->flag_251 = 0;
    state->flag_229 = 0;
    state->value_22c = 0;
    state->value_238 = 0;
    state->flag_23c = 1;
    state->value_258 = -1;
    state->value_25c = 0;
    if (g_settings_6850c8.main_ui_mode != W8_MAIN_UI_MODE_PORTRAITS) {
        ApplyMainGameModeFlag(W8_MAIN_UI_MODE_PORTRAITS, 0);
    } else {
        SetViewportMode(GetMainGameViewportMode());
    }
    if (gXStatus.fCampMode == 0) {
        g_screen_state_00649f1c->value_f0 = g_settings_6850c8.main_ui_mode;
    }
    CreateNpcDialogueControls();
    SetRegionBounds(0x8a, 0x17, 0x166, 0x269, 0x1c2);
    g_level_block->flag_271 = 0;
    RegionSetEnable(0x15);
    EnableRegionInput(0x52);
    EnableRegionInput(0x53);
    EnableRegionInput(0x54);
    EnableRegionInput(0x55);
    g_level_block->action_panel_visible = 1;
    gXStatus.fCampMode = 0;
    if (item != 0) {
        state = g_screen_state_00649f1c;
        state->pending_item_1ed = *item;
        if (g_status_685170.item_in_cursor != 0) {
            state->flag_1f9 = 1;
            ClearHeldItemDisplay();
        }
        if (npc->record->unknown_056 != 0) {
            if (HandleNpcDialogueItem(&state->pending_item_1ed) == 0) {
                OpenNpcDialogueTranscriptLayout();
                goto dispatch;
            }
        } else if (HandleNpcDialogueItem(&state->pending_item_1ed) == 0) {
            switch (g_screen_state_00649f1c->value_fc) {
            case 1:
                CloseNpcDialogueMode1Layout();
                break;
            case 2:
                RegionSetDisable(0x18);
                g_screen_state_00649f1c->panel_1a8->SetEnabled(0);
                g_screen_state_00649f1c->panel_1ac->SetEnabled(0);
                g_screen_state_00649f1c->panel_1b8->SetEnabled(0);
                g_screen_state_00649f1c->dialogue_text_1a4->SetActive(0);
                g_screen_state_00649f1c->dialogue_text_1a4->m_textBuffer.SetText(
                    &g_wchar_00689b34, g_wiz_text_bold_font_683664);
            /* fall through */
            case 6:
                SetNpcDialogueLayoutMode(0);
                break;
            case 3:
                CloseNpcDialogueTranscriptLayout();
                break;
            case 4:
                CloseNpcDialogueOptionLayout();
                break;
            case 5:
                CloseNpcDialogueMode5Layout();
                break;
            }
            if (GetNpcDispositionBand(g_screen_state_00649f1c->dialogue_npc) == 0) {
                OpenNpcDialogueTranscriptLayout();
            } else {
                ShowNpcDialogueTopicMenu();
            }
            goto dispatch;
        }
    }
    if (force != 0) {
        goto tail;
    }
dispatch:
    if (g_screen_state_00649f1c->dialogue_npc->record->unknown_056 != 0) {
        QueueNpcScriptLine(0, 0, 0, 0);
        OpenNpcDialogueTranscriptLayout();
    } else {
        band = GetNpcDispositionBand(g_screen_state_00649f1c->dialogue_npc);
        if (g_screen_state_00649f1c->dialogue_npc->name_style == W8_NPC_ZANT &&
            GetFact(W8_FACT_TRANG_YOU_ARE_BUSTED) != 0 && GetFact(W8_FACT_ALIGNMENT_UMPANI) == 0) {
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
tail:
    RequestRedraw(0x100);
    RequestRedraw(0x1000);
    if (g_flag_0068edd8 != 0) {
        SetFlag603C60();
        g_flag_0068edd8 = 0;
        gfTrackMousePos = 0;
    }
    info = GetNpcMonsterInfo(npc);
    if (info != 0) {
        g_screen_state_00649f1c->flag_23d = 1;
        g_screen_state_00649f1c->saved_camera_pitch_240 = g_gd_camera_65a0f8->m_pitch;
        g_screen_state_00649f1c->saved_camera_yaw_244 = g_gd_camera_65a0f8->m_yaw;
        dialogue_info = GetNpcMonsterInfo(g_screen_state_00649f1c->dialogue_npc);
        if (dialogue_info != 0) {
            position = dialogue_info->monster->movement_0c0.position_040;
            position.y += dialogue_info->monster->movement_0c0.height_offset_0b8;
            g_gd_camera_65a0f8->LookAt(&position, 0);
        }
        MonsterForwardReferencePosition(info->monster, 0);
    }
    PauseMainGameWorld();
    RequestRedraw(0x200);
    g_screen_state_00649f1c->flag_261 = 1;
    swprintf(space, L" ");
    ShowNotice(5, space, 3, -1, 0);
    return 1;
}

/* Stage `npc` as the dialogue NPC: bind its monster's location, clear its
   transient flags, kick the script dialogue, refresh the name caption while
   the dialogue UI is already up, and pick the speaking character - the first
   occupied row, overtaken by any occupied row with a higher skill-0x16
   (communication) level. Every occupied portrait then takes target pose 1.
   A stale disposition snapshot on the NPC drops its 0x1c flag. */
// FUNCTION: WIZ8 0x0056D030
void SelectNpcDialogueSpeaker(W8NpcState* npc, int flags)
{
    W8MainScreenState* state;
    W8MonsterInfo* info;
    W8NpcState* selected;
    int slot;
    int speaker;
    unsigned int best;

    state = g_screen_state_00649f1c;
    speaker = -1;
    state->target_location_id_f8 = -1;
    best = 0xffffffff;
    state->dialogue_npc = 0;
    info = GetNpcMonsterInfo(npc);
    if (info != 0) {
        state->target_location_id_f8 = info->location_id;
    }
    state->dialogue_npc = npc;
    state->dialogue_npc->flag_22 = 0;
    state->dialogue_npc->flag_23 = 0;
    if (state->dialogue_npc->greeting_pending != 0) {
        state->dialogue_npc->flag_84 = 0;
    }
    BeginNpcScriptDialogue(state->dialogue_npc, 0);
    if ((state->value_fc == W8_DIALOGUE_LAYOUT_TRANSCRIPT ||
         state->value_fc == W8_DIALOGUE_LAYOUT_TOPIC_MENU) &&
        gXStatus.fNpcDialogueMode != 0) {
        state->dialogue_text_10c->m_textBuffer.SetText(state->dialogue_npc->record->source_name_004,
                                                       g_wiz_text_bold_font_683664);
        state->dialogue_text_10c->Invalidate(1);
    }
    state->flag_23d = 0;
    for (slot = 0; slot < 8; ++slot) {
        if (g_status_685170.buffers.party_rows[slot].occupied != 0) {
            if (speaker == -1) {
                speaker = slot;
            }
            if (g_status_685170.buffers.characters[slot].skills[0x16].level > best) {
                best = g_status_685170.buffers.characters[slot].skills[0x16].level;
                speaker = slot;
            }
        }
    }
    g_screen_state_00649f1c->dialogue_speaker = speaker;
    for (slot = 0; slot < 8; ++slot) {
        if (g_status_685170.buffers.party_rows[slot].occupied != 0) {
            SetPortraitTargetPose(&gXStatus.monster_manager_entries[slot], 1);
        }
    }
    selected = g_screen_state_00649f1c->dialogue_npc;
    if (selected->dismissed_flag != 0 &&
        selected->unknown_ef[0] != GetNpcDispositionBand(selected)) {
        selected->dismissed_flag = 0;
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
    W8MainScreenState* state;

    state = g_screen_state_00649f1c;
    state->panel_1a8 = new W8MainGamePanel005EE9F0(0x17, 0x166, 0xa4, 0x1c2, 0x1a9, 0, 0);
    state->panel_1ac = new Controls(0xa4, 0x166, 0x1dc, 0x1c2, 0x1a9, 0, 1);
    state->npc_dialogue_controller_1b0 =
        new W8NpcDialogueTextController(0x1dc, 0x11b, 0x269, 0x140, 0x1a9, 0, 2, 4, 3);
    state->npc_dialogue_panel_1b4 = new Controls(0x1dc, 0x12f, 0x269, 0x1c2, 0x1a9, 0, 5);
    state->panel_1b8 = new Controls(0x1dc, 0x166, 0x269, 0x1c2, 0x1a9, 0, 7);
    state->panel_1bc = new Controls(0x1dc, 0x166, 0x238, 499, 0x1a9, 0, 6);
    state->text_input_panel_1c0 =
        new W8MainGamePanel005EE9E4(0x1dc, 0x166, 0x269, 0x1c0, 0x1a9, 0, 0xd);

    panel = state->panel_1a8;
    state->dialogue_text_10c =
        new W8TextControl(panel, 0xffffffff, 5, 2, 0x89, 0x12, -1, -1, -1, -1, -1, -1, -1);
    state->dialogue_text_10c->m_textBuffer.SetLayoutMode(g_W8TextBufferLayoutMask005ED554 |
                                                         g_W8TextBufferLayoutMask005ED54C);
    state->dialogue_text_1a4 =
        new W8TextControl(panel, 0xffffffff, 5, 0x47, 0x89, 0x57, -1, -1, -1, -1, -1, -1, -1);
    state->dialogue_text_1a4->m_textBuffer.SetLayoutMode(g_W8TextBufferLayoutMask005ED554 |
                                                         g_W8TextBufferLayoutMask005ED54C);
    state->dialogue_text_110 =
        new W8TextControl(panel, 0x82, 2, 0x11, 0x45, 0x21, 0x1a9, 0, -1, 0xc, 10, 0xb, -1);
    state->dialogue_text_114 =
        new W8TextControl(panel, 0x83, 0x44, 0x11, 0x87, 0x21, 0x1a9, 0, -1, 0xc, 10, 0xb, -1);
    state->dialogue_text_118 =
        new W8TextControl(panel, 0x84, 2, 0x21, 0x45, 0x31, 0x1a9, 0, -1, 0xc, 10, 0xb, -1);
    state->dialogue_text_11c =
        new W8TextControl(panel, 0x85, 0x44, 0x21, 0x87, 0x31, 0x1a9, 0, -1, 0xc, 10, 0xb, -1);
    state->dialogue_text_120 =
        new W8TextControl(panel, 0x86, 2, 0x31, 0x45, 0x41, 0x1a9, 0, -1, 0xc, 10, 0xb, -1);
    state->dialogue_text_124 =
        new W8TextControl(panel, 0x87, 0x44, 0x31, 0x87, 0x41, 0x1a9, 0, -1, 0xc, 10, 0xb, -1);
    state->dialogue_text_124->UpdateTextBounds(0x46, 0x31, 0x89, 0x41);
    state->dialogue_text_128 =
        new W8TextControl(panel, 0x88, 0x72, 0x47, 0x89, 0x57, 0x1aa, 0, 0, 4, 1, 2, 3);
    state->option_buttons_170[0] =
        new W8TextControl(panel, 0x75, 7, 0x46, 0x17, 0x56, 0x1ab, 0, 0, 1, 2, 4, 3);
    state->option_buttons_170[1] =
        new W8TextControl(panel, 0x76, 0x19, 0x46, 0x29, 0x56, 0x1ab, 0, 5, 6, 7, 9, 8);
    state->option_buttons_170[2] =
        new W8TextControl(panel, 0x77, 0x2b, 0x46, 0x3b, 0x56, 0x1ab, 0, 10, 0xb, 0xc, 0xe, 0xd);
    state->option_buttons_170[3] = new W8TextControl(panel, 0x78, 0x3e, 0x46, 0x4e, 0x56, 0x1ab, 0,
                                                     0xf, 0x10, 0x11, 0x13, 0x12);
    state->option_buttons_170[4] = new W8TextControl(panel, 0x79, 0x50, 0x46, 0x60, 0x56, 0x1ab, 0,
                                                     0x14, 0x15, 0x16, 0x18, 0x17);
    state->option_buttons_170[5] = new W8TextControl(panel, 0x7a, 0x62, 0x46, 0x72, 0x56, 0x1ab, 0,
                                                     0x19, 0x1a, 0x1b, 0x1d, 0x1c);
    state->option_buttons_170[0]->AddLayoutFlags(g_W8TextControlMask005ED578);
    state->option_buttons_170[0]->EnableRegionHelp(0x7c2);
    state->option_buttons_170[1]->AddLayoutFlags(g_W8TextControlMask005ED578);
    state->option_buttons_170[1]->EnableRegionHelp(0x7c3);
    state->option_buttons_170[2]->AddLayoutFlags(g_W8TextControlMask005ED578);
    state->option_buttons_170[2]->EnableRegionHelp(0x7c4);
    state->option_buttons_170[3]->AddLayoutFlags(g_W8TextControlMask005ED578);
    state->option_buttons_170[3]->EnableRegionHelp(0x7c5);
    state->option_buttons_170[4]->AddLayoutFlags(g_W8TextControlMask005ED578);
    state->option_buttons_170[4]->EnableRegionHelp(0x7c6);
    state->option_buttons_170[5]->AddLayoutFlags(g_W8TextControlMask005ED578);
    state->option_buttons_170[5]->EnableRegionHelp(0x7c7);
    state->option_buttons_170[0]->m_primaryActivationCallback = ToggleNpcTradeFilter00573660;
    state->option_buttons_170[1]->m_primaryActivationCallback = ToggleNpcTradeFilter00573730;
    state->option_buttons_170[2]->m_primaryActivationCallback = ToggleNpcTradeFilter005739A0;
    state->option_buttons_170[3]->m_primaryActivationCallback = ToggleNpcTradeFilter00573800;
    state->option_buttons_170[4]->m_primaryActivationCallback = ToggleNpcTradeFilter005738D0;
    state->option_buttons_170[5]->m_primaryActivationCallback = ToggleNpcTradeFilter00573A10;

    panel = state->panel_1ac;
    state->dialogue_widget_12c = new W8Widget(panel, 0x89, 2, 4, 0x136, 0x57);

    panel = state->npc_dialogue_controller_1b0;
    state->dialogue_scroll_130 = new W8NpcDialogueScrollWidget(panel, 0x65, 6, 6, 0x7c, 0x11);
    state->dialogue_scroll_up_button =
        new W8TextControl(panel, 0x66, 0x7e, 3, 0x88, 0xb, 0x1aa, 0, 0x14, 0x18, 0x15, 0x16, 0x17);
    state->dialogue_scroll_down_button = new W8TextControl(panel, 0x67, 0x7e, 0xc, 0x88, 0x14,
                                                           0x1aa, 0, 0x19, 0x1d, 0x1a, 0x1b, 0x1c);

    panel = state->text_input_panel_1c0;
    state->dialogue_text_13c =
        new W8TextControl(panel, 0x68, 0x11, 0x23, 0x22, 0x30, 0x1aa, 0, 5, 9, 6, 7, 8);
    state->dialogue_text_13c->EnableRegionHelp(100);
    state->dialogue_text_140 =
        new W8TextControl(panel, 0x69, 0x26, 0x23, 0x37, 0x30, 0x1aa, 0, 10, 0xe, 0xb, 0xc, 0xd);
    state->dialogue_text_140->EnableRegionHelp(0x65);
    state->dialogue_text_168 =
        new W8TextControl(panel, 0x73, 9, 0x33, 0x84, 0x44, 0x1a9, 0, -1, -1, 0xe, 0xf, -1);
    state->dialogue_text_168->m_textBuffer.SetText(gppStringList[0x741], g_font_683660);
    state->dialogue_text_168->m_primaryActivationCallback = SubmitNpcDialogueInput00575B40;
    state->dialogue_text_168->EnableRegionHelp(0x66);
    state->dialogue_text_164 =
        new W8TextControl(panel, 0x72, 9, 0x43, 0x84, 0x52, 0x1a9, 0, -1, -1, 0xe, 0xf, -1);
    state->dialogue_text_164->m_textBuffer.SetText(gppStringList[0x742], g_font_683660);
    state->dialogue_text_164->m_primaryActivationCallback = SubmitNpcDialogueInput00575B00;
    state->dialogue_text_164->EnableRegionHelp(0x67);

    panel = state->npc_dialogue_panel_1b4;
    state->dialogue_sort_button =
        new W8TextControl(panel, 0x6b, 6, 2, 0x89, 0xe, 0x1a9, 0, 8, 9, -1, -1, -1);
    state->dialogue_sort_button->AddLayoutFlags(
        g_W8TextControlMask005ED580 | g_W8TextControlMask005ED57C | g_W8TextControlMask005ED578);
    state->dialogue_people_button =
        new W8TextControl(panel, 0x6d, 6, 0xf, 0x41, 0x1b, 0x1a9, 0, 8, 9, -1, -1, -1);
    state->dialogue_people_button->AddLayoutFlags(
        g_W8TextControlMask005ED584 | g_W8TextControlMask005ED580 | g_W8TextControlMask005ED57C |
        g_W8TextControlMask005ED578);
    state->dialogue_places_button =
        new W8TextControl(panel, 0x6e, 0x43, 0xf, 0x89, 0x1b, 0x1a9, 0, 8, 9, -1, -1, -1);
    state->dialogue_places_button->AddLayoutFlags(
        g_W8TextControlMask005ED584 | g_W8TextControlMask005ED580 | g_W8TextControlMask005ED57C |
        g_W8TextControlMask005ED578);
    state->dialogue_items_button =
        new W8TextControl(panel, 0x6f, 6, 0x1b, 0x41, 0x27, 0x1a9, 0, 8, 9, -1, -1, -1);
    state->dialogue_items_button->AddLayoutFlags(
        g_W8TextControlMask005ED584 | g_W8TextControlMask005ED580 | g_W8TextControlMask005ED57C |
        g_W8TextControlMask005ED578);
    state->dialogue_misc_button =
        new W8TextControl(panel, 0x70, 0x43, 0x1b, 0x89, 0x27, 0x1a9, 0, 8, 9, -1, -1, -1);
    state->dialogue_misc_button->AddLayoutFlags(
        g_W8TextControlMask005ED584 | g_W8TextControlMask005ED580 | g_W8TextControlMask005ED57C |
        g_W8TextControlMask005ED578);
    state->dialogue_all_button =
        new W8TextControl(panel, 0x71, 6, 0x27, 0x41, 0x33, 0x1a9, 0, 8, 9, -1, -1, -1);
    state->dialogue_all_button->AddLayoutFlags(
        g_W8TextControlMask005ED584 | g_W8TextControlMask005ED580 | g_W8TextControlMask005ED57C |
        g_W8TextControlMask005ED578);

    panel = state->panel_1bc;
    state->dialogue_text_16c =
        new W8TextControl(panel, 0x74, 5, 0x14, 0x32, 0x49, -1, -1, -1, -1, -1, -1, -1);
    state->dialogue_text_16c->m_textBuffer.SetLayoutMode(g_W8TextBufferLayoutMask005ED55C |
                                                         g_W8TextBufferLayoutMask005ED550);
    state->dialogue_text_16c->AddLayoutFlags(g_W8TextControlMask005ED594);
    state->dialogue_text_188 = new W8TextControl(panel, 0x7b, 0x35, 0x36, 0x51, 0x46, 0x1aa, 0,
                                                 0x1e, 0x22, 0x1f, 0x20, 0x21);
    state->dialogue_text_188->EnableRegionHelp(0x7c9);
    state->dialogue_text_190 =
        new W8TextControl(panel, 0x7d, 0x53, 0x4d, 0x86, 0x58, -1, -1, -1, -1, -1, -1, -1);
    state->dialogue_text_190->m_textBuffer.SetLayoutMode(g_W8TextBufferLayoutMask005ED550 |
                                                         g_W8TextBufferLayoutMask005ED554);
    state->dialogue_text_190->SetEnabled(0);
    state->dialogue_text_194 =
        new W8TextControl(panel, 0x7e, 5, 0x4d, 0x51, 0x58, -1, -1, -1, -1, -1, -1, -1);
    state->dialogue_text_194->m_textBuffer.SetLayoutMode(g_W8TextBufferLayoutMask005ED554 |
                                                         g_W8TextBufferLayoutMask005ED54C);
    state->dialogue_text_194->SetEnabled(0);
    state->dialogue_text_198 =
        new W8TextControl(panel, 0x7f, 0x53, 0x3a, 0x86, 0x45, -1, -1, -1, -1, -1, -1, -1);
    state->dialogue_text_198->m_textBuffer.SetLayoutMode(g_W8TextBufferLayoutMask005ED550 |
                                                         g_W8TextBufferLayoutMask005ED554);
    state->dialogue_text_198->SetEnabled(0);
    state->dialogue_text_19c =
        new W8TextControl(panel, 0x80, 4, 4, 0x88, 0x11, -1, -1, -1, -1, -1, -1, -1);
    state->dialogue_text_19c->m_textBuffer.SetLayoutMode(g_W8TextBufferLayoutMask005ED554 |
                                                         g_W8TextBufferLayoutMask005ED54C);
    state->dialogue_text_19c->SetEnabled(0);
    state->dialogue_text_1a0 =
        new W8TextControl(panel, 0x81, 0x38, 0x22, 0x88, 0x34, -1, -1, -1, -1, -1, -1, -1);
    state->dialogue_text_128->m_primaryActivationCallback = BackOutNpcDialogue00570000;
    state->dialogue_text_128->EnableRegionHelp(0x7c8);
}

/* The NPC dialogue per-frame update: keeps the monster's navigator angles
   fresh, services the pending layout switch, mirrors the transcript controls'
   enabled state to the field text and selection, and clears the 500ms trade
   highlight flash once it lapses. */
// FUNCTION: WIZ8 0x0056E510
void ServiceNpcDialogue0056E510(void)
{
    wchar_t field_text[200];
    W8MonsterInfo* monster_info = GetNpcMonsterInfo(g_screen_state_00649f1c->dialogue_npc);
    if (monster_info != 0) {
        monster_info->monster->UpdateAngles00453990();
    }
    if (g_screen_state_00649f1c->flag_252 != 0) {
        return;
    }
    if (g_screen_state_00649f1c->dialogue_cursor_flag != 0 && gfRightButtonState != 0) {
        SetNpcDialogueHidden(0);
    }
    Get16BitStringFromField(0, field_text);
    if (g_screen_state_00649f1c->value_238 != 0) {
        g_level_block->flag_271 = 0;
        RegionSetEnable(0x15);
        EnableRegionInput(0x52);
        EnableRegionInput(0x53);
        EnableRegionInput(0x54);
        EnableRegionInput(0x55);
        g_level_block->action_panel_visible = 1;
        int layout = g_screen_state_00649f1c->value_238;
        switch (g_screen_state_00649f1c->value_fc) {
        case 1:
            CloseNpcDialogueMode1Layout();
            break;
        case 2:
            CloseNpcDialogueLayout00570A20();
            break;
        case 3:
            CloseNpcDialogueTranscriptLayout();
            break;
        case 4:
            CloseNpcDialogueOptionLayout();
            break;
        case 5:
            CloseNpcDialogueMode5Layout();
            break;
        case 6:
            SetNpcDialogueLayoutMode(0);
            break;
        }
        switch (layout) {
        case 1:
            OpenNpcDialogueMode1Layout();
            break;
        case 2:
            ShowNpcDialogueTopicMenu();
            break;
        case 3:
            OpenNpcDialogueTranscriptLayout();
            break;
        case 4:
            OpenNpcDialogueOptionLayout();
            break;
        case 5:
            OpenNpcDialogueMode5Layout();
            break;
        }
        g_screen_state_00649f1c->value_238 = 0;
    }
    if (g_screen_state_00649f1c->value_fc == W8_DIALOGUE_LAYOUT_TRANSCRIPT) {
        if (wcslen(field_text) == 0) {
            if (g_screen_state_00649f1c->dialogue_text_13c->m_enabled != 0) {
                g_screen_state_00649f1c->dialogue_text_13c->SetEnabled(0);
                g_screen_state_00649f1c->dialogue_text_13c->W8Widget::Invalidate(1);
            }
        } else if (g_screen_state_00649f1c->dialogue_text_13c->m_enabled == 0) {
            g_screen_state_00649f1c->dialogue_text_13c->SetEnabled(1);
            g_screen_state_00649f1c->dialogue_text_13c->W8Widget::Invalidate(1);
        }
        int selection =
            g_screen_state_00649f1c->npc_dialogue_controller_1b0->GetSelectedTranscriptEntryIndex();
        if (selection == -1) {
            if (g_screen_state_00649f1c->dialogue_text_140->m_enabled != 0) {
                g_screen_state_00649f1c->dialogue_text_140->SetEnabled(0);
                g_screen_state_00649f1c->dialogue_text_140->W8Widget::Invalidate(1);
            }
        } else if (g_screen_state_00649f1c->dialogue_text_140->m_enabled == 0) {
            g_screen_state_00649f1c->dialogue_text_140->SetEnabled(1);
            g_screen_state_00649f1c->dialogue_text_140->W8Widget::Invalidate(1);
        }
        g_screen_state_00649f1c->dialogue_scroll_up_button->SetEnabled(
            g_screen_state_00649f1c->npc_dialogue_controller_1b0->HandleScrollUpCommand(1) != 0);
        g_screen_state_00649f1c->dialogue_scroll_down_button->SetEnabled(
            g_screen_state_00649f1c->npc_dialogue_controller_1b0->HandleScrollDownCommand(1) != 0);
    }
    if (g_screen_state_00649f1c->dialogue_text_16c != 0 &&
        g_screen_state_00649f1c->dialogue_text_16c->m_textBuffer.m_flag_4c != 0 &&
        GetTickCount() - g_trade_highlight_tick_68ee78 > 500) {
        g_screen_state_00649f1c->dialogue_text_16c->m_textBuffer.m_flag_4c = 0;
        g_screen_state_00649f1c->dialogue_text_16c->m_textBuffer.m_geometryDirty = 1;
        g_screen_state_00649f1c->dialogue_text_16c->Invalidate(0);
        g_screen_state_00649f1c->dialogue_text_198->m_textBuffer.m_flag_4c = 0;
        g_screen_state_00649f1c->dialogue_text_198->m_textBuffer.m_geometryDirty = 1;
        g_screen_state_00649f1c->dialogue_text_198->Invalidate(0);
    }
}

/* Close the NPC dialogue: retire the active layout, release the panels and
   text controls, restore the held item or target cursor, queue the parting
   character event and hand control back to the world. */
// FUNCTION: WIZ8 0x0056E800
void EndNpcDialogueSession0056E800(int param_1)
{
    if (gXStatus.fNpcDialogueMode == 0) {
        return;
    }
    if (g_screen_state_00649f1c->flag_252 != 0) {
        gXStatus.fNpcDialogueMode = 0;
        ResumeMainGameWorld();
        return;
    }
    if (gXStatus.fCampMode != 0) {
        param_1 = 1;
    }
    if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
        SetNpcDialogueHidden(0);
    }
    if (g_screen_state_00649f1c->dialogue_npc->record->unknown_054 == 0) {
        g_screen_state_00649f1c->dialogue_npc->dismissed_flag = 1;
        g_screen_state_00649f1c->dialogue_npc->unknown_ef[0] =
            GetNpcDispositionBand(g_screen_state_00649f1c->dialogue_npc);
        W8NpcState* npc = g_screen_state_00649f1c->dialogue_npc;
        npc->dismissed_timer = 0;
    }
    gXStatus.fNpcDialogueMode = 0;
    g_level_block->action_panel_visible = 0;
    RegionSetDisable(0x15);
    DisableRegionInput(0x52);
    DisableRegionInput(0x53);
    DisableRegionInput(0x54);
    DisableRegionInput(0x55);
    switch (g_screen_state_00649f1c->value_fc) {
    case 1:
        CloseNpcDialogueMode1Layout();
        break;
    case 2:
        RegionSetDisable(0x18);
        g_screen_state_00649f1c->panel_1a8->SetEnabled(0);
        g_screen_state_00649f1c->panel_1ac->SetEnabled(0);
        g_screen_state_00649f1c->panel_1b8->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_1a4->SetActive(0);
        g_screen_state_00649f1c->dialogue_text_1a4->m_textBuffer.SetText(
            &g_wchar_00689b34, g_wiz_text_bold_font_683664);
        g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
        g_screen_state_00649f1c->value_fc = 0;
        if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
            SetNpcDialogueHidden(0);
        }
        break;
    case 3:
        CloseNpcDialogueTranscriptLayout();
        break;
    case 4:
        CloseNpcDialogueOptionLayout();
        break;
    case 5:
        RegionSetDisable(0x18);
        RegionSetDisable(0x17);
        g_screen_state_00649f1c->dialogue_text_188->SetEnabled(1);
        g_screen_state_00649f1c->panel_1a8->SetEnabled(0);
        g_screen_state_00649f1c->panel_1ac->SetEnabled(0);
        g_screen_state_00649f1c->panel_1bc->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_110->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_text_114->SetEnabled(1);
    case 6:
        SetNpcDialogueLayoutMode(0);
        break;
    }
    RegionSetDisable(0x18);
    SelectTextBox(0);
    g_level_block->flag_271 = 1;
    ApplyMainGameModeFlag(gXStatus.fCampMode != 0 ? g_settings_6850c8.main_ui_mode
                                                  : g_screen_state_00649f1c->value_f0,
                          1);
    /* The seven dialogue panels (panel_1a8..text_input_panel_1c0) delete
       through the non-virtual Controls destructor; the thirty-nine text
       controls (dialogue_text_10c..dialogue_text_1a4) through the virtual
       one. The members stay dangling until the next dialogue rebuilds them. */
    int i;
    Controls** panel = reinterpret_cast<
        Controls**>( // reinterpret-ok: contiguous pointer-member run the retail loop walks
        &g_screen_state_00649f1c->panel_1a8);
    for (i = 0; i < 7; i++) {
        delete panel[i];
    }
    W8Widget** control = reinterpret_cast<
        W8Widget**>( // reinterpret-ok: contiguous pointer-member run the retail loop walks
        &g_screen_state_00649f1c->dialogue_text_10c);
    for (i = 0; i < 39; i++) {
        delete control[i];
    }
    RequestRedraw(0x100);
    RequestRedraw(0x1000);
    for (unsigned int index = 0; index < PLLength(gXStatus.plsMonsterList); index++) {
        ClearMonsterEffect2DE(MonsterGetScriptPartByLocationIndex(index));
    }
    GetNpcMonsterInfo(g_screen_state_00649f1c->dialogue_npc);
    KillTextInputMode();
    ResumeMainGameWorld();
    if (param_1 == 0) {
        if (g_screen_state_00649f1c->flag_1f9 == 0) {
            SetTargetCursor(-1);
        } else {
            g_status_685170.item_in_hand_235b = g_screen_state_00649f1c->pending_item_1ed;
            SetItemCursor(0);
            g_screen_state_00649f1c->flag_1f9 = 0;
        }
        W8NpcState* npc = g_screen_state_00649f1c->dialogue_npc;
        if (g_screen_state_00649f1c->flag_23c == 0 && npc->record->unknown_056 == 0 &&
            npc->is_grouped == 0 && GetNpcMonsterInfo(npc) != 0) {
            int slot = GetRandomCharacter(1, 1, -1, -1);
            if (slot != -1) {
                W8CharacterEvent* event = QueueCharacterEvent(
                    &g_status_685170.buffers.characters[slot], g_special_event_0068c534,
                    g_event_flag_005ed8e8, g_effect_argument_005ed8c8, g_effect_argument_005ed914);
                if (event != 0) {
                    event->dispatch_delay_ms = 1000;
                    event->dispatch_delay_start = GetTickCount();
                }
            }
        }
    }
    FlushPendingNoticeLines005766B0();
    if (g_screen_state_00649f1c->flag_23d != 0) {
        g_gd_camera_65a0f8->SetPitch(g_screen_state_00649f1c->saved_camera_pitch_240);
    }
    if (g_screen_state_00649f1c->dialogue_npc->name_style == 0xf && GetFact(0x3c) != 0) {
        g_status_685170.value_49b7 = g_status_685170.world_clock;
        g_status_685170.flag_49bb = 1;
    }
}

/* Whether the open NPC dialogue transcript covers the party slot's
   portrait: dialogue mode up, panel invalidation not suppressed and the
   controller enabled, then the slot's band check. Portrait and
   character-update paths skip the covered rows through this. */
// FUNCTION: WIZ8 0x0056EC90
unsigned char IsPortraitObscuredByNpcDialogue(unsigned int party_slot)
{
    if (gXStatus.fNpcDialogueMode != 0 && g_screen_state_00649f1c->flag_252 == 0 &&
        g_screen_state_00649f1c->npc_dialogue_controller_1b0->m_fEnabled != 0) {
        return g_screen_state_00649f1c->npc_dialogue_controller_1b0
            ->IsSlotPortraitTranscriptCovered(party_slot);
    }
    return 0;
}

/* Forward one text/cursor rectangle to the action panel while the dialogue
   screen is not suppressing panel invalidation. */
// FUNCTION: WIZ8 0x0056ECD0
void InvalidateMainGameActionPanelRect(const W8ControlsRect* rect)
{
    if (g_screen_state_00649f1c->flag_252 == 0) {
        g_screen_state_00649f1c->panel_1ac->Invalidate(rect);
    }
}

/* Redraw each enabled dialogue panel, optionally forcing it disabled first.
   The panel_1ac dirty check snapshots its state before Redraw consumes it and
   repaints the shared text-box scroll chrome alongside. */
// FUNCTION: WIZ8 0x0056ECF0
void ActivateNpcDialoguePanels0056ECF0(unsigned char active)
{
    bool redraw_scroll = false;
    if (g_screen_state_00649f1c->flag_252 == 0) {
        Controls** panel = reinterpret_cast<
            Controls**>( // reinterpret-ok: contiguous pointer-member run the retail loop walks
            &g_screen_state_00649f1c->panel_1a8);
        for (int i = 0; i < 7; i++) {
            if (panel[i]->m_fEnabled != 0) {
                if (active != 0) {
                    panel[i]->Invalidate(0);
                }
                if (i == 1) {
                    redraw_scroll = g_screen_state_00649f1c->panel_1ac->m_fEnabled != 0 &&
                                    (g_screen_state_00649f1c->panel_1ac->m_fDirty != 0 ||
                                     g_screen_state_00649f1c->panel_1ac->m_fLayoutDirty != 0);
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
bool HasNpcDialogueDirtyPanels0056ED80(void)
{
    if (g_screen_state_00649f1c->flag_252 == 0) {
        Controls** panel = reinterpret_cast<
            Controls**>( // reinterpret-ok: contiguous pointer-member run the retail loop walks
            &g_screen_state_00649f1c->panel_1a8);
        for (int i = 0; i < 7; i++) {
            if (panel[i]->m_fEnabled != 0 &&
                (panel[i]->m_fDirty != 0 || panel[i]->m_fLayoutDirty != 0)) {
                return 1;
            }
        }
    }
    return 0;
}

/* Retire the current dialogue layout mode into value_104 and, when nonzero,
   install `value` as the new one; either way the dialogue cursor helper gets
   re-run while the flag is set. */
// FUNCTION: WIZ8 0x0056EDD0
void SetNpcDialogueLayoutMode(int value)
{
    if (value == 0) {
        g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
        g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
    } else {
        g_screen_state_00649f1c->value_fc = value;
    }
    if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
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
void SyncNpcServiceButtons0056EE20(int party_slot)
{
    W8Character* character = &g_status_685170.buffers.characters[party_slot];
    if (gXStatus.fNpcDialogueMode != 0) {
        if (g_screen_state_00649f1c->value_fc == 1) {
            g_screen_state_00649f1c->dialogue_text_110->SetEnabled(
                CanCharacterCastSpell(character, 3));
            g_screen_state_00649f1c->dialogue_text_110->W8Widget::Invalidate(1);
            g_screen_state_00649f1c->dialogue_text_114->SetEnabled(
                CanCharacterCastSpell(character, 0x29));
            g_screen_state_00649f1c->dialogue_text_114->W8Widget::Invalidate(1);
            g_screen_state_00649f1c->dialogue_text_11c->SetEnabled(
                CharacterHasServiceItem(character));
            g_screen_state_00649f1c->dialogue_text_11c->W8Widget::Invalidate(1);
        } else if (g_screen_state_00649f1c->value_fc == 4) {
            ResetNpcDialogueItemEditor();
            g_screen_state_00649f1c->dialogue_text_124->DisableSecondaryState(1);
            g_screen_state_00649f1c->dialogue_text_124->m_textBuffer.m_fontStateIndex = -1;
            g_screen_state_00649f1c->dialogue_text_124->m_textBuffer.m_geometryDirty = 1;
            g_screen_state_00649f1c->dialogue_text_120->EnableSecondaryState(1);
            g_screen_state_00649f1c->dialogue_text_120->m_textBuffer.m_fontStateIndex = 3;
            g_screen_state_00649f1c->dialogue_text_120->m_textBuffer.m_geometryDirty = 1;
            RebuildNpcTradeItemList005ADB10(1);
            g_screen_state_00649f1c->flag_260 = 1;
            UpdateNpcDialogueSubMode();
        }
    }
}

// FUNCTION: WIZ8 0x0056efb0
unsigned char IsNpcDialogueCursorActive(void)
{
    if (gXStatus.fNpcDialogueMode == 0) {
        return 0;
    }
    return g_screen_state_00649f1c->dialogue_cursor_flag;
}

/* Whether an NPC dialogue is up in the layout that posts to the main text
   box (mode 4) rather than to the dialogue's own pane. */
// FUNCTION: WIZ8 0x0056efd0
bool IsNpcDialogueTextBoxActive(void)
{
    if (gXStatus.fNpcDialogueMode == 0) {
        return false;
    }
    return g_screen_state_00649f1c->value_fc == W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX;
}

// FUNCTION: WIZ8 0x0056EFF0
void TryNpcDialoguePickpocket0056EFF0(int party_slot)
{
    if (gXStatus.fNpcDialogueMode != 0 && g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
        SetNpcDialogueHidden(0);
        ResolveNpcPickpocket00576BA0(party_slot);
    }
}

/* Forward mouse enter/leave and button events to the main-screen control whose
   pointer sits at W8MainScreenState::dialogue_text_10c[callback_id]. Id 0x27
   is the panel slot at +0x1a8 and is ignored. Ids 1..6 and 0x16/0x17 pass 1
   into the widget hooks; every other id passes 0. Mouse-enter always passes 0. */
// FUNCTION: WIZ8 0x0056F020
unsigned char MainScreenControlRegionEvent(const InputAtom* event, W8Region* region)
{
    unsigned int callback_id = region->callback_id;
    W8Widget* control;
    unsigned char arg;

    if (callback_id == 0x27) {
        return 0;
    }
    control =
        // reinterpret-ok: retail indexes contiguous control* slots from dialogue_text_10c
        reinterpret_cast<W8Widget**>(&g_screen_state_00649f1c->dialogue_text_10c)[callback_id];
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
   notice word. The right-held flag is also raised on left release/double-click
   (retail quirk). */
// FUNCTION: WIZ8 0x0056F1D0
unsigned char NpcDialogueTextBoxRegionEvent(const InputAtom* event, W8Region* region)
{
    int line;
    int y;

    switch (event->usEvent) {
    case LEFT_BUTTON_DBL_CLK:
        NpcDialogueTextBoxDoubleClick0056F840(static_cast<short>(event->uiParam),
                                              static_cast<short>(event->uiParam >> 16));
        return 1;
    case LEFT_BUTTON_UP:
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
            region->flags &= ~W8_REGION_LEFT_BUTTON_HELD;
            NpcDialogueTextBoxLeftUp0056F530(static_cast<short>(event->uiParam),
                                             static_cast<short>(event->uiParam >> 16));
        }
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
            NpcDialogueTextBoxRightUp0056F6B0(static_cast<short>(event->uiParam),
                                              static_cast<short>(event->uiParam >> 16));
        }
        return 1;
    default:
        return 0;
    }
    if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
        NoOp();
        ClearNoticeWordHover(3, 1);
        ClearTextSlot1D8(2);
        return 1;
    }
    if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
        g_screen_state_00649f1c->value_1c4 = static_cast<unsigned short>(event->uiParam);
        g_screen_state_00649f1c->value_1c8 = event->uiParam >> 16;
        NoOp();
        if (g_screen_state_00649f1c->value_fc == W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX) {
            y = (event->uiParam >> 16) & 0xffff;
            if (y >= g_level_block->text_box_top && y <= g_level_block->text_box_bottom) {
                line = (y - g_level_block->text_box_top) / 11;
                ClearTextSlot1D8(2);
                if (line < static_cast<int>(g_status_685170.text_box_lines_shown_49a7[2])) {
                    SelectTextSlot1D8(g_level_block->text_lines[2] + line, 2);
                }
                RedrawTextBox();
                g_screen_state_00649f1c->value_1cc = line;
            }
        }
        return 1;
    }
    if (static_cast<unsigned short>(event->uiParam) == g_screen_state_00649f1c->value_1c4 &&
        static_cast<int>(event->uiParam >> 16) == g_screen_state_00649f1c->value_1c8) {
        return 1;
    }
    g_screen_state_00649f1c->value_1c4 = static_cast<unsigned short>(event->uiParam);
    g_screen_state_00649f1c->value_1c8 = event->uiParam >> 16;
    if (g_screen_state_00649f1c->value_fc == W8_DIALOGUE_LAYOUT_TRANSCRIPT) {
        HighlightNoticeWordAt(3, static_cast<short>(event->uiParam),
                              static_cast<short>(event->uiParam >> 16));
        return 1;
    }
    if (g_screen_state_00649f1c->value_fc == W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX) {
        y = (event->uiParam >> 16) & 0xffff;
        if (y >= g_level_block->text_box_top && y <= g_level_block->text_box_bottom) {
            line = (y - g_level_block->text_box_top) / 11;
            if (line != g_screen_state_00649f1c->value_1cc) {
                ClearTextSlot1D8(2);
                if (line < static_cast<int>(g_status_685170.text_box_lines_shown_49a7[2])) {
                    SelectTextSlot1D8(g_level_block->text_lines[2] + line, 2);
                }
                RedrawTextBox();
            }
            g_screen_state_00649f1c->value_1cc = line;
        }
    }
    return 1;
}

/* Mouse-wheel line tracking on the main NPC text box: flag forces the redraw
   even when the hovered line has not changed. */
// FUNCTION: WIZ8 0x0056F490
void NpcDialogueTextBoxWheelAt(short x, unsigned short y, char flag)
{
    int line;

    if (g_screen_state_00649f1c->value_fc != W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX) {
        return;
    }
    if (static_cast<int>(y) < g_level_block->text_box_top ||
        static_cast<int>(y) > g_level_block->text_box_bottom) {
        return;
    }
    line = (static_cast<int>(y) - g_level_block->text_box_top) / 11;
    if (line != g_screen_state_00649f1c->value_1cc || flag != 0) {
        ClearTextSlot1D8(2);
        if (line < static_cast<int>(g_status_685170.text_box_lines_shown_49a7[2])) {
            SelectTextSlot1D8(g_level_block->text_lines[2] + line, 2);
        }
        RedrawTextBox();
    }
    g_screen_state_00649f1c->value_1cc = line;
}

/* Left release inside the NPC text box: in the transcript layout a hovered
   notice word becomes the selected keyword and is appended to the input field;
   in the item layouts the hovered slot is selected. */
// FUNCTION: WIZ8 0x0056F530
void NpcDialogueTextBoxLeftUp0056F530(int x, int y)
{
    wchar_t word_text[200];
    wchar_t field_text[200];
    wchar_t combined[400];
    int line;
    W8NoticeWord* word;
    int slot;

    switch (g_screen_state_00649f1c->value_fc) {
    case W8_DIALOGUE_LAYOUT_TRANSCRIPT:
        word = FindNoticeWordAt(3, x, y, &line);
        if (word == 0 || word->flag_08 != 1) {
            return;
        }
        if (g_flag_006f0530 == 0) {
            ClearNoticeWordSelection(3, 1);
            word_text[0] = 0;
            SetInputFieldStringWith16BitString(0, word_text);
        }
        word->flag_08 = 2;
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
        return;
    case 1:
    case W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX:
        slot = GetTextSlot1D8(2);
        if (slot == -1) {
            return;
        }
        UpdateNpcTradeSelection0056FAC0(slot, g_flag_006f0530 != 0 ? 1 : 0, 1);
        return;
    }
}

/* Right release inside the NPC text box: transcript layout adds the clicked
   keyword to the dialogue transcript; the item layouts select the slot and
   open the item assay dialog. */
// FUNCTION: WIZ8 0x0056F6B0
void NpcDialogueTextBoxRightUp0056F6B0(int x, int y)
{
    wchar_t word_text[200];
    int line;
    W8NoticeWord* word;
    int slot;
    W8ItemInstance* item;
    W8AssayDialog* dialog;

    switch (g_screen_state_00649f1c->value_fc) {
    case W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX:
        slot = GetTextSlot1D8(2);
        if (slot == -1) {
            return;
        }
        UpdateNpcTradeSelection0056FAC0(slot, g_flag_006f0530 != 0 ? 1 : 0, 1);
        item = ResolveNpcTradeRow005729C0(slot, 0, 0, 1);
        if (item == 0) {
            return;
        }
        dialog = new W8AssayDialog(
            item, &g_status_685170.buffers.characters[g_status_685170.selected_character]);
        dialog->SetText(&g_wchar_00689b34);
        dialog->SetOrigin(g_info_dialog_x_005ef958, 0x48);
        dialog->m_destroy_callback = OnNpcAssayDialogClosed;
        OpenModal(dialog);
        return;
    case W8_DIALOGUE_LAYOUT_TRANSCRIPT:
        word = FindNoticeWordAt(3, x, y, &line);
        if (word == 0) {
            return;
        }
        CopyNoticeWordText(word, word_text, 0xc8, 3, line);
        AddNpcDialogueKeyword(word_text, -1, 0);
        return;
    }
}

/* Left double-click inside the NPC text box: transcript layout selects the
   keyword and submits the input line; the item layout picks the hovered line's
   slot and uses the selected item. */
// FUNCTION: WIZ8 0x0056F840
void NpcDialogueTextBoxDoubleClick0056F840(int x, int y)
{
    wchar_t word_text[200];
    wchar_t field_text[200];
    wchar_t combined[400];
    int line;
    W8NoticeWord* word;
    int slot;

    switch (g_screen_state_00649f1c->value_fc) {
    case W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX:
        break;
    case W8_DIALOGUE_LAYOUT_TRANSCRIPT:
        if (g_screen_state_00649f1c->flag_250 != 0) {
            return;
        }
        word = FindNoticeWordAt(3, x, y, &line);
        if (word == 0) {
            return;
        }
        if (g_flag_006f0530 == 0) {
            ClearNoticeWordSelection(3, 1);
            word_text[0] = 0;
            SetInputFieldStringWith16BitString(0, word_text);
        }
        if (word->flag_08 != 2) {
            word->flag_08 = 2;
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
        HandleNpcDialogueInput();
        return;
    default:
        return;
    }
    y &= 0xffff;
    if (y >= g_level_block->text_box_top && y <= g_level_block->text_box_bottom) {
        line = (y - g_level_block->text_box_top) / 11;
        ClearTextSlot1D8(2);
        if (line < static_cast<int>(g_status_685170.text_box_lines_shown_49a7[2])) {
            SelectTextSlot1D8(g_level_block->text_lines[2] + line, 2);
        }
        RedrawTextBox();
        g_screen_state_00649f1c->value_1cc = line;
    }
    slot = GetTextSlot1D8(2);
    if (slot != -1) {
        g_screen_state_00649f1c->value_258 = -1;
        UpdateNpcTradeSelection0056FAC0(slot, 0, 1);
        if (g_screen_state_00649f1c->value_108 != 0) {
            Function5AD290();
        }
    }
    if (g_screen_state_00649f1c->value_fc == W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX &&
        y >= g_level_block->text_box_top && y <= g_level_block->text_box_bottom) {
        line = (y - g_level_block->text_box_top) / 11;
        ClearTextSlot1D8(2);
        if (line < static_cast<int>(g_status_685170.text_box_lines_shown_49a7[2])) {
            SelectTextSlot1D8(g_level_block->text_lines[2] + line, 2);
        }
        RedrawTextBox();
        g_screen_state_00649f1c->value_1cc = line;
    }
}

/* Select the item slot under the text-box cursor and refresh the item preview
   controls: layout 1 pulls from the equipment/backpack picker, layout 4 from
   the trade lists; the mode-2 row zero additionally fills in the purse readout. */
// FUNCTION: WIZ8 0x0056FAC0
void UpdateNpcTradeSelection0056FAC0(int index, int increment, int commit)
{
    W8ItemInstance* item;
    wchar_t text[204];
    wchar_t price_text[220];
    bool wants_item;

    SelectTextSlot1E8(index, 2);
    switch (g_screen_state_00649f1c->value_fc) {
    case W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX:
        break;
    case 1:
        item = GetNpcTradeSlotItem00573F80(index);
        g_screen_state_00649f1c->value_108 = item;
        if (g_screen_state_00649f1c->value_108 != 0) {
            g_screen_state_00649f1c->dialogue_text_1a0->SetEnabled(1);
        }
        return;
    default:
        return;
    }
    item = ResolveNpcTradeRow005729C0(index, 1, increment, commit);
    g_screen_state_00649f1c->value_108 = item;
    if (g_screen_state_00649f1c->value_108 != 0) {
        g_screen_state_00649f1c->dialogue_text_1a0->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_text_188->SetEnabled(
            1 < g_screen_state_00649f1c->value_108->stack_count);
        if (g_item_records[g_screen_state_00649f1c->value_108->item_id].equip_class == 4) {
            g_screen_state_00649f1c->dialogue_text_188->SetEnabled(0);
        }
        ShortenTextToWidth00577410(text,
                                   FormatItemDisplayName(g_screen_state_00649f1c->value_108, 0),
                                   0x7d, g_font_683660);
        g_screen_state_00649f1c->dialogue_text_19c->m_textBuffer.SetText(text, g_font_683660);
        g_screen_state_00649f1c->dialogue_text_1a0->m_textBuffer.m_fontStateIndex = 1;
        g_screen_state_00649f1c->dialogue_text_1a0->m_textBuffer.m_geometryDirty = 1;
        switch (g_screen_state_00649f1c->value_100) {
        case 4:
            if (g_status_685170.party_gold < static_cast<unsigned int>(CalculateNpcTradeStackPrice(
                                                 g_screen_state_00649f1c->dialogue_npc,
                                                 g_screen_state_00649f1c->value_108->item_id, 1,
                                                 g_screen_state_00649f1c->value_254,
                                                 g_screen_state_00649f1c->value_108->identified))) {
                g_screen_state_00649f1c->dialogue_text_1a0->m_textBuffer.m_fontStateIndex = 0;
                g_screen_state_00649f1c->dialogue_text_1a0->m_textBuffer.m_geometryDirty = 1;
            }
            swprintf(price_text, L"%dg",
                     CalculateNpcTradeStackPrice(g_screen_state_00649f1c->dialogue_npc,
                                                 g_screen_state_00649f1c->value_108->item_id, 1,
                                                 g_screen_state_00649f1c->value_254,
                                                 g_screen_state_00649f1c->value_108->identified));
            break;
        case 3: {
            wants_item = NpcAcceptsTradeItem(g_screen_state_00649f1c->dialogue_npc,
                                             g_screen_state_00649f1c->value_108);
            if (wants_item == 0) {
                g_screen_state_00649f1c->dialogue_text_1a0->m_textBuffer.m_fontStateIndex = 0;
                g_screen_state_00649f1c->dialogue_text_1a0->m_textBuffer.m_geometryDirty = 1;
            }
            int price = CalculateNpcTradeStackPrice(g_screen_state_00649f1c->dialogue_npc,
                                                    g_screen_state_00649f1c->value_108->item_id, 0,
                                                    g_screen_state_00649f1c->value_254,
                                                    g_screen_state_00649f1c->value_108->identified);
            if (wants_item != 0) {
                swprintf(price_text, L"%dg", price);
            } else {
                swprintf(price_text, L"---");
            }
            break;
        }
        case 2:
            swprintf(price_text, L" ");
            break;
        default:
            return;
        }
        g_screen_state_00649f1c->dialogue_text_198->m_textBuffer.SetText(price_text, g_font_683660);
        return;
    }
    if (g_screen_state_00649f1c->value_100 == 2 && index == 0) {
        g_screen_state_00649f1c->dialogue_text_1a0->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_text_1a0->m_textBuffer.m_fontStateIndex = 1;
        g_screen_state_00649f1c->dialogue_text_1a0->m_textBuffer.m_geometryDirty = 1;
        g_screen_state_00649f1c->dialogue_text_188->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_text_19c->m_textBuffer.SetText(gppStringList[0x72d],
                                                                         g_font_683660);
        swprintf(price_text, L"%dg", g_screen_state_00649f1c->value_22c);
        g_screen_state_00649f1c->dialogue_text_198->m_textBuffer.SetText(price_text, g_font_683660);
        g_screen_state_00649f1c->panel_1bc->Invalidate(0);
    }
}

/* Reset the secondary dialogue editor: drop the pending item, clear the item
   notice control and both secondary labels, and restore the option row. */
// FUNCTION: WIZ8 0x0056FED0
void ResetNpcDialogueItemEditor(void)
{
    ClearTextSlot1E8(2);
    g_screen_state_00649f1c->value_108 = 0;
    g_screen_state_00649f1c->dialogue_text_1a0->SetEnabled(0);
    g_screen_state_00649f1c->dialogue_text_16c->m_imageObject = -1;
    g_screen_state_00649f1c->dialogue_text_16c->m_measured_w = -1;
    g_screen_state_00649f1c->dialogue_text_16c->m_measured_h = -1;
    g_screen_state_00649f1c->dialogue_text_16c->m_imageFrame = -1;
    g_screen_state_00649f1c->dialogue_text_16c->m_normalSprite = -1;
    g_screen_state_00649f1c->dialogue_text_16c->m_pressedSprite = -1;
    g_screen_state_00649f1c->dialogue_text_16c->m_textBuffer.SetText(g_wchar_0068ee58, 0);
    g_screen_state_00649f1c->dialogue_text_16c->Invalidate(1);
    g_screen_state_00649f1c->dialogue_text_198->m_textBuffer.SetText(&g_wchar_00689b34,
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_19c->m_textBuffer.SetText(&g_wchar_00689b34,
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_188->SetEnabled(0);
    g_screen_state_00649f1c->value_258 = -1;
    g_screen_state_00649f1c->value_254 = 1;
    g_screen_state_00649f1c->dialogue_text_1a0->m_textBuffer.m_fontStateIndex = -1;
    g_screen_state_00649f1c->dialogue_text_1a0->m_textBuffer.m_geometryDirty = 1;
}

/* Retire the current dialogue layout, then open the layout `interact_id`
   selects. */
/* Close the active sub-layout and reopen the layout it replaced: mode 1 goes
   back to the remembered value_104 layout, the option layout returns to the
   topic menu or the transcript by flag_229, and mode 5 always lands on the
   transcript. */
// FUNCTION: WIZ8 0x00570000
void BackOutNpcDialogue00570000(void)
{
    switch (g_screen_state_00649f1c->value_fc) {
    case 1:
        CloseNpcDialogueMode1Layout();
        switch (g_screen_state_00649f1c->value_104) {
        case 2:
            ShowNpcDialogueTopicMenu();
            break;
        case 3:
            OpenNpcDialogueTranscriptLayout();
            break;
        }
        break;
    case 4: {
        unsigned char flag = g_screen_state_00649f1c->flag_229;
        CloseNpcDialogueOptionLayout();
        if (flag != 0) {
            ShowNpcDialogueTopicMenu();
        } else {
            OpenNpcDialogueTranscriptLayout();
        }
        break;
    }
    case 5:
        RegionSetDisable(0x18);
        RegionSetDisable(0x17);
        g_screen_state_00649f1c->dialogue_text_188->SetEnabled(1);
        g_screen_state_00649f1c->panel_1a8->SetEnabled(0);
        g_screen_state_00649f1c->panel_1ac->SetEnabled(0);
        g_screen_state_00649f1c->panel_1bc->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_110->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_text_114->SetEnabled(1);
        g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
        g_screen_state_00649f1c->value_fc = 0;
        if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
            SetNpcDialogueHidden(0);
        }
        OpenNpcDialogueTranscriptLayout();
        break;
    }
}

// FUNCTION: WIZ8 0x00570120
void SwitchNpcDialogueLayout(int interact_id)
{
    switch (g_screen_state_00649f1c->value_fc) {
    case 1:
        CloseNpcDialogueMode1Layout();
        break;
    case 2:
        RegionSetDisable(0x18);
        g_screen_state_00649f1c->panel_1a8->SetEnabled(0);
        g_screen_state_00649f1c->panel_1ac->SetEnabled(0);
        g_screen_state_00649f1c->panel_1b8->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_1a4->SetActive(0);
        g_screen_state_00649f1c->dialogue_text_1a4->m_textBuffer.SetText(
            &g_wchar_00689b34, g_wiz_text_bold_font_683664);
        g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
        g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
        if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
            SetNpcDialogueHidden(0);
        }
        break;
    case 3:
        CloseNpcDialogueTranscriptLayout();
        break;
    case 4:
        CloseNpcDialogueOptionLayout();
        break;
    case 5:
        RegionSetDisable(0x18);
        RegionSetDisable(0x17);
        g_screen_state_00649f1c->dialogue_text_188->SetEnabled(1);
        g_screen_state_00649f1c->panel_1a8->SetEnabled(0);
        g_screen_state_00649f1c->panel_1ac->SetEnabled(0);
        g_screen_state_00649f1c->panel_1bc->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_110->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_text_114->SetEnabled(1);
        g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
        g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
        if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
            SetNpcDialogueHidden(0);
        }
        break;
    case 6:
        g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
        g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
        if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
            SetNpcDialogueHidden(0);
        }
        break;
    }
    switch (interact_id) {
    case 1:
        OpenNpcDialogueMode1Layout();
        return;
    case 2:
        ShowNpcDialogueTopicMenu();
        return;
    case 3:
        OpenNpcDialogueTranscriptLayout();
        return;
    case 4:
        OpenNpcDialogueOptionLayout();
        return;
    case 5:
        OpenNpcDialogueMode5Layout();
        return;
    }
}

// FUNCTION: WIZ8 0x00570310
void LeaveNpcDialogueLayout(void)
{
    if (g_screen_state_00649f1c->flag_250 == 0) {
        if (GetNpcDispositionBand(g_screen_state_00649f1c->dialogue_npc) == 0) {
            g_screen_state_00649f1c->flag_23c = 0;
            g_screen_state_00649f1c->flag_251 = 1;
            QueueNpcScriptLine(0x5c, 0, 0, 0);
            return;
        }
        switch (g_screen_state_00649f1c->value_fc) {
        case 1:
            CloseNpcDialogueMode1Layout();
            break;
        case 2:
            RegionSetDisable(0x18);
            g_screen_state_00649f1c->panel_1a8->SetEnabled(0);
            g_screen_state_00649f1c->panel_1ac->SetEnabled(0);
            g_screen_state_00649f1c->panel_1b8->SetEnabled(0);
            g_screen_state_00649f1c->dialogue_text_1a4->SetActive(0);
            g_screen_state_00649f1c->dialogue_text_1a4->m_textBuffer.SetText(
                &g_wchar_00689b34, g_wiz_text_bold_font_683664);
            g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
            g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
            if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
                SetNpcDialogueHidden(0);
            }
            break;
        case 3:
            CloseNpcDialogueTranscriptLayout();
            break;
        case 4:
            CloseNpcDialogueOptionLayout();
            break;
        case 5:
            RegionSetDisable(0x18);
            RegionSetDisable(0x17);
            g_screen_state_00649f1c->dialogue_text_188->SetEnabled(1);
            g_screen_state_00649f1c->panel_1a8->SetEnabled(0);
            g_screen_state_00649f1c->panel_1ac->SetEnabled(0);
            g_screen_state_00649f1c->panel_1bc->SetEnabled(0);
            g_screen_state_00649f1c->dialogue_text_110->SetEnabled(1);
            g_screen_state_00649f1c->dialogue_text_114->SetEnabled(1);
            g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
            g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
            if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
                SetNpcDialogueHidden(0);
            }
            break;
        case 6:
            g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
            g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
            if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
                SetNpcDialogueHidden(0);
            }
            break;
        }
    }
    EndNpcDialogueSession0056E800(0);
}

// FUNCTION: WIZ8 0x00570530
void PromptNpcDispositionChange(void)
{
    W8MessageDialogBase* dialog = static_cast<W8MessageDialogBase*>(CreateDialogByKind(1));

    SetDialogPrompt(dialog, gppStringList[0x1e60 / 4], 0, 0);
    dialog->m_destroy_callback = OnNpcDispositionPromptClosed;
    OpenModal(dialog);
}

// FUNCTION: WIZ8 0x00570570
void OnNpcDispositionPromptClosed(W8DialogBase* dialog)
{
    if (GetDialogResult(dialog) != 0) {
        SetNpcDispositionBand(g_screen_state_00649f1c->dialogue_npc, 2);
        QueueNpcScriptLine(0x18, 0, 0, 0);
        QueueNpcMessageLine(W8_NPC_MSG_CLOSE_RESUME_NPC, 0);
    }
}

// FUNCTION: WIZ8 0x005705B0
void EnterNpcServiceLayout(void)
{
    switch (g_screen_state_00649f1c->value_fc) {
    case 1:
        CloseNpcDialogueMode1Layout();
        break;
    case 2:
        RegionSetDisable(0x18);
        g_screen_state_00649f1c->panel_1a8->SetEnabled(0);
        g_screen_state_00649f1c->panel_1ac->SetEnabled(0);
        g_screen_state_00649f1c->panel_1b8->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_1a4->SetActive(0);
        g_screen_state_00649f1c->dialogue_text_1a4->m_textBuffer.SetText(
            &g_wchar_00689b34, g_wiz_text_bold_font_683664);
        g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
        g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
        if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
            SetNpcDialogueHidden(0);
        }
        break;
    case 3:
        CloseNpcDialogueTranscriptLayout();
        break;
    case 4:
        CloseNpcDialogueOptionLayout();
        break;
    case 5:
        RegionSetDisable(0x18);
        RegionSetDisable(0x17);
        g_screen_state_00649f1c->dialogue_text_188->SetEnabled(1);
        g_screen_state_00649f1c->panel_1a8->SetEnabled(0);
        g_screen_state_00649f1c->panel_1ac->SetEnabled(0);
        g_screen_state_00649f1c->panel_1bc->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_110->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_text_114->SetEnabled(1);
        g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
        g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
        if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
            SetNpcDialogueHidden(0);
        }
        break;
    case 6:
        g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
        g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
        if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
            SetNpcDialogueHidden(0);
        }
        break;
    }
    OpenNpcDialogueMode1Layout();
}

/* Bring up the mode-2 dialogue layout: the option panels come up, the caption
   takes the NPC's name, and the five topics get their strings and callbacks. */
// FUNCTION: WIZ8 0x00570760
void ShowNpcDialogueTopicMenu(void)
{
    g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_TOPIC_MENU;
    if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
        SetNpcDialogueHidden(0);
    }
    g_screen_state_00649f1c->panel_1a8->SetEnabled(1);
    g_screen_state_00649f1c->panel_1ac->SetEnabled(1);
    g_screen_state_00649f1c->panel_1b8->SetEnabled(1);
    RegionSetEnable(0x18);
    g_screen_state_00649f1c->dialogue_text_1a4->SetActive(1);
    g_screen_state_00649f1c->dialogue_text_10c->m_textBuffer.SetText(
        g_screen_state_00649f1c->dialogue_npc->record->source_name_004,
        g_wiz_text_bold_font_683664);
    g_screen_state_00649f1c->dialogue_text_10c->Invalidate(1);
    g_screen_state_00649f1c->dialogue_text_1a4->m_textBuffer.SetText(gppStringList[0x1c98 / 4],
                                                                     g_wiz_text_bold_font_683664);
    g_screen_state_00649f1c->dialogue_text_110->m_textBuffer.SetText(gppStringList[0x1c9c / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_110->m_primaryActivationCallback =
        SelectNpcDialogueService00570AD0;
    g_screen_state_00649f1c->dialogue_text_114->m_textBuffer.SetText(gppStringList[0x1ca0 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_114->m_primaryActivationCallback =
        SelectNpcDialogueTalk00570B80;
    g_screen_state_00649f1c->dialogue_text_118->m_textBuffer.SetText(gppStringList[0x1ca4 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_118->m_primaryActivationCallback =
        SelectNpcDialogueExit00570C20;
    g_screen_state_00649f1c->dialogue_text_11c->m_textBuffer.SetText(gppStringList[0x1c8c / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_11c->m_primaryActivationCallback =
        PromptNpcDispositionChange;
    g_screen_state_00649f1c->dialogue_text_120->m_textBuffer.SetText(gppStringList[0x1c90 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_120->m_primaryActivationCallback = EnterNpcServiceLayout;
    g_screen_state_00649f1c->dialogue_text_124->m_textBuffer.SetText(gppStringList[0x1c94 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_124->m_primaryActivationCallback =
        LeaveNpcDialogueLayout;
    g_screen_state_00649f1c->dialogue_text_128->SetActive(0);
    g_screen_state_00649f1c->dialogue_text_114->SetEnabled(
        g_screen_state_00649f1c->dialogue_npc->flag_c8 == 0);
    if (g_screen_state_00649f1c->dialogue_npc->flag_c9 == 0) {
        g_screen_state_00649f1c->dialogue_text_110->SetEnabled(1);
    } else {
        g_screen_state_00649f1c->dialogue_text_110->SetEnabled(0);
    }
    RequestRedraw(0x200);
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
void CloseNpcDialogueLayout00570A20(void)
{
    RegionSetDisable(0x18);
    g_screen_state_00649f1c->panel_1a8->SetEnabled(0);
    g_screen_state_00649f1c->panel_1ac->SetEnabled(0);
    g_screen_state_00649f1c->panel_1b8->SetEnabled(0);
    g_screen_state_00649f1c->dialogue_text_1a4->SetActive(0);
    g_screen_state_00649f1c->dialogue_text_1a4->m_textBuffer.SetText(&g_wchar_00689b34,
                                                                     g_wiz_text_bold_font_683664);
    g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
    g_screen_state_00649f1c->value_fc = 0;
    if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
        SetNpcDialogueHidden(0);
    }
}

/* The "Trade" option button: hand the NPC's trade answer to ApplyNpcInteraction0050A570,
   then react to the disposition band - friendly queues the trade quote and
   returns to the transcript, neutral queues the neutral answer, hostile sets
   the hostile band and queues the refusal. */
// FUNCTION: WIZ8 0x00570AD0
void SelectNpcDialogueService00570AD0(void)
{
    unsigned char band;

    ApplyNpcInteraction0050A570(g_screen_state_00649f1c->dialogue_npc, 1,
                                g_screen_state_00649f1c->dialogue_speaker, 0, 0);
    g_screen_state_00649f1c->dialogue_text_110->SetEnabled(0);
    band = GetNpcDispositionBand(g_screen_state_00649f1c->dialogue_npc);
    if (band == 0) {
        QueueNpcScriptLine(4, 0, 0, 0);
        QueueNpcMessageLine(W8_NPC_MSG_REOPEN_TRANSCRIPT, 0);
        return;
    }
    if (band == 1) {
        QueueNpcScriptLine(6, 0, 0, 0);
        return;
    }
    SetNpcDispositionBand(g_screen_state_00649f1c->dialogue_npc, 2);
    QueueNpcScriptLine(0x18, 0, 0, 0);
    QueueNpcMessageLine(W8_NPC_MSG_CLOSE_RESUME_NPC, 0);
}

/* The "Talk" option button: friendly NPCs leave the dialogue outright,
   neutral ones queue the talk quote, hostile ones set the hostile band and
   queue the refusal. */
// FUNCTION: WIZ8 0x00570B80
void SelectNpcDialogueTalk00570B80(void)
{
    unsigned char band;

    ApplyNpcInteraction0050A570(g_screen_state_00649f1c->dialogue_npc, 0,
                                g_screen_state_00649f1c->dialogue_speaker, 0, 0);
    g_screen_state_00649f1c->dialogue_text_114->SetEnabled(0);
    band = GetNpcDispositionBand(g_screen_state_00649f1c->dialogue_npc);
    if (band == 0) {
        HandleNpcDialogueDeparture(0);
        OpenNpcDialogueTranscriptLayout();
        return;
    }
    if (band == 1) {
        QueueNpcScriptLine(3, 0, 0, 0);
        return;
    }
    SetNpcDispositionBand(g_screen_state_00649f1c->dialogue_npc, 2);
    QueueNpcScriptLine(0x18, 0, 0, 0);
    QueueNpcMessageLine(W8_NPC_MSG_CLOSE_RESUME_NPC, 0);
}

/* The "Exit" option button: close the option row, mark the dialogue as leaving
   through the transcript and open the option layout for the farewell. */
// FUNCTION: WIZ8 0x00570C20
void SelectNpcDialogueExit00570C20(void)
{
    RegionSetDisable(0x18);
    g_screen_state_00649f1c->panel_1a8->SetEnabled(0);
    g_screen_state_00649f1c->panel_1ac->SetEnabled(0);
    g_screen_state_00649f1c->panel_1b8->SetEnabled(0);
    g_screen_state_00649f1c->dialogue_text_1a4->SetActive(0);
    g_screen_state_00649f1c->dialogue_text_1a4->m_textBuffer.SetText(&g_wchar_00689b34,
                                                                     g_wiz_text_bold_font_683664);
    g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
    g_screen_state_00649f1c->value_fc = 0;
    if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
        SetNpcDialogueHidden(0);
    }
    g_screen_state_00649f1c->value_100 = 2;
    g_screen_state_00649f1c->flag_229 = 1;
    OpenNpcDialogueOptionLayout();
}

// FUNCTION: WIZ8 0x00570CF0
void OpenNpcDialogueTranscriptLayout(void)
{
    int index;

    g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_TRANSCRIPT;
    if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
        SetNpcDialogueHidden(0);
    }
    g_screen_state_00649f1c->panel_1a8->SetEnabled(1);
    g_screen_state_00649f1c->panel_1ac->SetEnabled(1);
    g_screen_state_00649f1c->npc_dialogue_controller_1b0->SetEnabled(1);
    g_screen_state_00649f1c->npc_dialogue_panel_1b4->SetEnabled(1);
    g_screen_state_00649f1c->text_input_panel_1c0->SetEnabled(1);
    g_screen_state_00649f1c->dialogue_text_1a4->SetActive(0);
    g_screen_state_00649f1c->dialogue_text_1a4->m_textBuffer.SetText(&g_wchar_00689b34,
                                                                     g_wiz_text_bold_font_683664);
    RegionSetEnable(0x18);
    RegionSetEnable(0x16);
    if (g_screen_state_00649f1c->dialogue_npc->name_style == 0x32) {
        swprintf(g_status_685170.monster_name_buffer_2453, L"Al-%s",
                 g_status_685170.buffers.characters[g_status_685170.alternate_name_slot_247f].name);
        g_screen_state_00649f1c->dialogue_text_10c->m_textBuffer.SetText(
            g_status_685170.monster_name_buffer_2453, g_wiz_text_bold_font_683664);
    } else {
        g_screen_state_00649f1c->dialogue_text_10c->m_textBuffer.SetText(
            g_screen_state_00649f1c->dialogue_npc->record->source_name_004,
            g_wiz_text_bold_font_683664);
    }
    g_screen_state_00649f1c->dialogue_text_110->m_textBuffer.SetText(gppStringList[0x1cd4 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_110->m_primaryActivationCallback = EnterNpcTradeOptions;
    g_screen_state_00649f1c->dialogue_text_114->m_textBuffer.SetText(gppStringList[0x1ce0 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_114->m_primaryActivationCallback = ShowNpcDialogueNotice;
    g_screen_state_00649f1c->dialogue_text_118->m_textBuffer.SetText(gppStringList[0x1c90 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_118->m_primaryActivationCallback = EnterNpcServiceLayout;
    g_screen_state_00649f1c->dialogue_text_11c->m_textBuffer.SetText(gppStringList[0x1cdc / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_11c->m_primaryActivationCallback = RequestNpcJoinParty;
    g_screen_state_00649f1c->dialogue_text_120->m_textBuffer.SetText(gppStringList[0x1c8c / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_120->m_primaryActivationCallback =
        PromptNpcDispositionChange;
    g_screen_state_00649f1c->dialogue_text_124->m_textBuffer.SetText(gppStringList[0x1c94 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_124->m_primaryActivationCallback =
        LeaveNpcDialogueLayout;
    g_screen_state_00649f1c->dialogue_scroll_130->m_primaryActivationCallback = NoOp;
    g_screen_state_00649f1c->dialogue_scroll_up_button->m_leftButtonDownCallback =
        ScrollNpcDialogueUp;
    g_screen_state_00649f1c->dialogue_scroll_down_button->m_leftButtonDownCallback =
        ScrollNpcDialogueDown;
    g_screen_state_00649f1c->dialogue_text_13c->m_primaryActivationCallback =
        SubmitNpcDialogueKeyword;
    g_screen_state_00649f1c->dialogue_text_140->m_primaryActivationCallback =
        RefreshNpcDialogueTranscript;
    g_screen_state_00649f1c->dialogue_sort_button->m_textBuffer.SetText(gppStringList[0x1d00 / 4],
                                                                        g_font_683660);
    g_screen_state_00649f1c->dialogue_sort_button->m_primaryActivationCallback =
        SyncNpcDialogueListFilter;
    g_screen_state_00649f1c->dialogue_people_button->m_textBuffer.SetText(gppStringList[0x1d0c / 4],
                                                                          g_font_683660);
    g_screen_state_00649f1c->dialogue_people_button->m_primaryActivationCallback =
        SelectNpcDialogueCategory1;
    g_screen_state_00649f1c->dialogue_places_button->m_textBuffer.SetText(gppStringList[0x1d10 / 4],
                                                                          g_font_683660);
    g_screen_state_00649f1c->dialogue_places_button->m_primaryActivationCallback =
        SelectNpcDialogueCategory2;
    g_screen_state_00649f1c->dialogue_items_button->m_textBuffer.SetText(gppStringList[0x1d14 / 4],
                                                                         g_font_683660);
    g_screen_state_00649f1c->dialogue_items_button->m_primaryActivationCallback =
        SelectNpcDialogueCategory0;
    g_screen_state_00649f1c->dialogue_misc_button->m_textBuffer.SetText(gppStringList[0x1d18 / 4],
                                                                        g_font_683660);
    g_screen_state_00649f1c->dialogue_misc_button->m_primaryActivationCallback =
        SelectNpcDialogueCategory3;
    g_screen_state_00649f1c->dialogue_all_button->m_textBuffer.SetText(gppStringList[0x1d1c / 4],
                                                                       g_font_683660);
    g_screen_state_00649f1c->dialogue_all_button->m_primaryActivationCallback =
        SelectNpcDialogueCategoryAll;
    g_screen_state_00649f1c->dialogue_text_128->SetActive(0);
    g_screen_state_00649f1c->dialogue_text_110->SetEnabled(1);
    g_screen_state_00649f1c->dialogue_text_114->SetEnabled(1);
    g_screen_state_00649f1c->npc_dialogue_controller_1b0->RestoreTranscriptEntries();
    g_screen_state_00649f1c->npc_dialogue_controller_1b0->SetTranscriptCategoryFilter(
        g_screen_state_00649f1c->dialogue_category_filter);
    g_screen_state_00649f1c->npc_dialogue_controller_1b0->SetTranscriptSorted(
        g_screen_state_00649f1c->transcript_sorted);
    ExpandNpcDialogueTextArea(g_screen_state_00649f1c->npc_dialogue_controller_1b0);
    if (IsNpcDialogueTextExpanded(g_screen_state_00649f1c->npc_dialogue_controller_1b0)) {
        g_screen_state_00649f1c->dialogue_scroll_up_button->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_scroll_down_button->SetEnabled(1);
    } else {
        g_screen_state_00649f1c->dialogue_scroll_up_button->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_scroll_down_button->SetEnabled(0);
    }
    SyncDialogueCategoryButtons();
    if (g_screen_state_00649f1c->transcript_sorted != 0) {
        g_screen_state_00649f1c->dialogue_sort_button->EnableSecondaryState(1);
    } else {
        g_screen_state_00649f1c->dialogue_sort_button->DisableSecondaryState(1);
    }
    RequestRedraw(0x200);
    for (index = 0; index < 8; ++index) {
        if (g_status_685170.buffers.party_rows[index].occupied != 0) {
            RegionSetDisable(7 + index);
            DisableRegionSetInput(7 + index);
            DisableRegionInput(0x5a + index);
        }
    }
    if (g_screen_state_00649f1c->dialogue_npc->record->unknown_056 != 0) {
        g_screen_state_00649f1c->dialogue_text_114->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_118->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_11c->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_120->SetEnabled(0);
    }
    if (g_screen_state_00649f1c->dialogue_npc->name_style == 0x17) {
        g_screen_state_00649f1c->dialogue_text_114->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_118->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_11c->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_120->SetEnabled(0);
    }
    g_screen_state_00649f1c->value_25c++;
    SelectTextBox(3);
}

/* Tear down the mode-3 transcript layout: fold the dialogue text controller
   back up, drop the panels, then re-enable whichever occupied party rows
   still own region slots. */
// FUNCTION: WIZ8 0x00571370
void CloseNpcDialogueTranscriptLayout(void)
{
    int index;

    g_screen_state_00649f1c->npc_dialogue_controller_1b0->SaveTranscriptEntries();
    CollapseNpcDialogueTextArea(g_screen_state_00649f1c->npc_dialogue_controller_1b0);
    g_screen_state_00649f1c->npc_dialogue_controller_1b0->ClearTranscriptEntries();
    RegionSetDisable(0x18);
    RegionSetDisable(0x16);
    g_screen_state_00649f1c->panel_1a8->SetEnabled(0);
    g_screen_state_00649f1c->panel_1ac->SetEnabled(0);
    g_screen_state_00649f1c->npc_dialogue_controller_1b0->SetEnabled(0);
    g_screen_state_00649f1c->npc_dialogue_panel_1b4->SetEnabled(0);
    g_screen_state_00649f1c->text_input_panel_1c0->SetEnabled(0);
    g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
    g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
    if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
        SetNpcDialogueHidden(0);
    }
    ClearNpcDialogueTextBackground(g_screen_state_00649f1c->npc_dialogue_controller_1b0);
    ClearSurfaceRect(0x1dc, 0x11b, 0x269, 0x1c2);
    InvalidateRegion(0x1dc, 0x11b, 0x269, 0x1c2, 0);
    for (index = 0; index < 8; ++index) {
        if (g_status_685170.buffers.party_rows[index].occupied != 0) {
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

    ShowNotice(0xa, gppStringList[0x1d20 / 4], 3, GetTextBoxScrollRange(), 0);
    if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
        SetNpcDialogueHidden(0);
    }
    if (g_screen_state_00649f1c->dialogue_npc->record->has_group != 0) {
        slot = FindFreePartySlot(0, 2);
        if (slot == 0xffffffff) {
            QueueNpcScriptLine(0xc, 0, 0, 0);
            return;
        }
        if (CanNpcJoinParty(g_screen_state_00649f1c->dialogue_npc) != 0) {
            QueueNpcScriptLine(0xd, 0, 0, 0);
            QueueNpcMessageLine(W8_NPC_MSG_FOCUS_NPC,
                                g_screen_state_00649f1c->dialogue_npc->name_style);
            return;
        }
    }
    QueueNpcScriptLine(0xb, 0, 0, 0);
}

// FUNCTION: WIZ8 0x005715A0
void ShowNpcDialogueNotice(void)
{
    ShowNotice(0xa, gppStringList[0x1d24 / 4], 3, GetTextBoxScrollRange(), 0);
    SetNpcDialogueHidden(1);
}

// FUNCTION: WIZ8 0x005715D0
void EnterNpcTradeOptions(void)
{
    CloseNpcDialogueTranscriptLayout();
    g_screen_state_00649f1c->value_100 = 4;
    OpenNpcDialogueOptionLayout();
}

// FUNCTION: WIZ8 0x005715F0
void ScrollNpcDialogueUp(void)
{
    g_screen_state_00649f1c->npc_dialogue_controller_1b0->HandleScrollUpCommand(0);
}

// FUNCTION: WIZ8 0x00571610
void ScrollNpcDialogueDown(void)
{
    g_screen_state_00649f1c->npc_dialogue_controller_1b0->HandleScrollDownCommand(0);
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
    controller = g_screen_state_00649f1c->npc_dialogue_controller_1b0;
    StripNpcKeywordPunctuation(text);
    if (category == W8_DIALOGUE_CATEGORY_ALL) {
        for (index = 0; index < gXStatus.uiItemsInDatabase; ++index) {
            if (CompareWideTextIgnoreAsciiCase00402920(text, g_item_records[index].display_name) ==
                0) {
                category = W8_DIALOGUE_CATEGORY_ITEMS;
                goto classified;
            }
        }
        for (index = 0; index < gXStatus.uiNpcsInDatabase; ++index) {
            if (CompareWideTextIgnoreAsciiCase00402920(text,
                                                       g_npc_records[index].source_name_004) == 0) {
                category = W8_DIALOGUE_CATEGORY_PEOPLE;
                goto classified;
            }
        }
        for (index = 0; g_dialogue_person_keywords[index][0] != 0; ++index) {
            if (CompareWideTextIgnoreAsciiCase00402920(text, g_dialogue_person_keywords[index]) ==
                0) {
                category = W8_DIALOGUE_CATEGORY_PEOPLE;
                goto classified;
            }
        }
        for (index = 0; index < static_cast<unsigned int>(g_dialogue_place_keyword_count);
             ++index) {
            if (CompareWideTextIgnoreAsciiCase00402920(
                    text, gppStringList[g_dialogue_place_keyword_ids[index]]) == 0) {
                category = W8_DIALOGUE_CATEGORY_PLACES;
                goto classified;
            }
        }
        category = W8_DIALOGUE_CATEGORY_MISC;
    }
classified:
    if (controller != 0) {
        if (controller->AddTranscriptEntry(text, category, play_chime) != 0) {
            if (g_screen_state_00649f1c->dialogue_category_filter != W8_DIALOGUE_CATEGORY_ALL &&
                g_screen_state_00649f1c->dialogue_category_filter != category) {
                g_screen_state_00649f1c->dialogue_category_filter = category;
                g_screen_state_00649f1c->npc_dialogue_controller_1b0->SetTranscriptCategoryFilter(
                    g_screen_state_00649f1c->dialogue_category_filter);
                SyncDialogueCategoryButtons();
            }
            if (play_chime != 0) {
                SoundPlay(reinterpret_cast<STR>(const_cast<char*>( // reinterpret-ok: SGP text ABI
                              "Data\\Sound\\Misc\\Keyword Chime.wav")),
                          0);
            }
            CollapseNpcDialogueTextArea(controller);
            ExpandNpcDialogueTextArea(controller);
        }
        controller->Invalidate(0);
        if (IsNpcDialogueTextExpanded(g_screen_state_00649f1c->npc_dialogue_controller_1b0) != 0) {
            g_screen_state_00649f1c->dialogue_scroll_up_button->SetEnabled(1);
            g_screen_state_00649f1c->dialogue_scroll_down_button->SetEnabled(1);
        } else {
            g_screen_state_00649f1c->dialogue_scroll_up_button->SetEnabled(0);
            g_screen_state_00649f1c->dialogue_scroll_down_button->SetEnabled(0);
        }
    }
}

// FUNCTION: WIZ8 0x00571880
void RefreshNpcDialogueTranscript(void)
{
    g_screen_state_00649f1c->npc_dialogue_controller_1b0->RemoveSelectedTranscriptEntry();
    CollapseNpcDialogueTextArea(g_screen_state_00649f1c->npc_dialogue_controller_1b0);
    ExpandNpcDialogueTextArea(g_screen_state_00649f1c->npc_dialogue_controller_1b0);
    if (IsNpcDialogueTextExpanded(g_screen_state_00649f1c->npc_dialogue_controller_1b0)) {
        g_screen_state_00649f1c->dialogue_scroll_up_button->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_scroll_down_button->SetEnabled(1);
    } else {
        g_screen_state_00649f1c->dialogue_scroll_up_button->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_scroll_down_button->SetEnabled(0);
    }
}

// FUNCTION: WIZ8 0x00571920
void SyncNpcDialogueListFilter(void)
{
    if (static_cast<unsigned char>(g_screen_state_00649f1c->dialogue_sort_button->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        g_screen_state_00649f1c->transcript_sorted = 1;
    } else {
        g_screen_state_00649f1c->transcript_sorted = 0;
    }
    g_screen_state_00649f1c->npc_dialogue_controller_1b0->SetTranscriptSorted(
        g_screen_state_00649f1c->transcript_sorted);
}

// FUNCTION: WIZ8 0x00571960
void SelectNpcDialogueCategory1(void)
{
    if (static_cast<unsigned char>(g_screen_state_00649f1c->dialogue_people_button->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        g_screen_state_00649f1c->dialogue_category_filter = W8_DIALOGUE_CATEGORY_PEOPLE;
        SyncDialogueCategoryButtons();
    }
    g_screen_state_00649f1c->npc_dialogue_controller_1b0->SetTranscriptCategoryFilter(
        W8_DIALOGUE_CATEGORY_PEOPLE);
}

// FUNCTION: WIZ8 0x005719A0
void SelectNpcDialogueCategory2(void)
{
    if (static_cast<unsigned char>(g_screen_state_00649f1c->dialogue_places_button->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        g_screen_state_00649f1c->dialogue_category_filter = W8_DIALOGUE_CATEGORY_PLACES;
        SyncDialogueCategoryButtons();
    }
    g_screen_state_00649f1c->npc_dialogue_controller_1b0->SetTranscriptCategoryFilter(
        W8_DIALOGUE_CATEGORY_PLACES);
}

// FUNCTION: WIZ8 0x005719E0
void SelectNpcDialogueCategory0(void)
{
    if (static_cast<unsigned char>(g_screen_state_00649f1c->dialogue_items_button->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        g_screen_state_00649f1c->dialogue_category_filter = W8_DIALOGUE_CATEGORY_ITEMS;
        SyncDialogueCategoryButtons();
    }
    g_screen_state_00649f1c->npc_dialogue_controller_1b0->SetTranscriptCategoryFilter(
        W8_DIALOGUE_CATEGORY_ITEMS);
}

// FUNCTION: WIZ8 0x00571A20
void SelectNpcDialogueCategoryAll(void)
{
    if (static_cast<unsigned char>(g_screen_state_00649f1c->dialogue_all_button->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        g_screen_state_00649f1c->dialogue_category_filter = W8_DIALOGUE_CATEGORY_ALL;
        SyncDialogueCategoryButtons();
    }
    g_screen_state_00649f1c->npc_dialogue_controller_1b0->SetTranscriptCategoryFilter(
        W8_DIALOGUE_CATEGORY_ALL);
}

// FUNCTION: WIZ8 0x00571A60
void SelectNpcDialogueCategory3(void)
{
    if (static_cast<unsigned char>(g_screen_state_00649f1c->dialogue_misc_button->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        g_screen_state_00649f1c->dialogue_category_filter = W8_DIALOGUE_CATEGORY_MISC;
        SyncDialogueCategoryButtons();
    }
    g_screen_state_00649f1c->npc_dialogue_controller_1b0->SetTranscriptCategoryFilter(
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

    g_screen_state_00649f1c->value_fc = 4;
    if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
        SetNpcDialogueHidden(0);
    }
    g_screen_state_00649f1c->dialogue_text_1a4->SetActive(0);
    g_screen_state_00649f1c->dialogue_text_1a4->m_textBuffer.SetText(&g_wchar_00689b34,
                                                                     g_wiz_text_bold_font_683664);
    g_screen_state_00649f1c->panel_1a8->SetEnabled(1);
    g_screen_state_00649f1c->panel_1ac->SetEnabled(1);
    g_screen_state_00649f1c->panel_1bc->SetEnabled(1);
    RegionSetEnable(0x18);
    RegionSetEnable(0x17);
    g_screen_state_00649f1c->dialogue_text_194->m_textBuffer.SetText(gppStringList[0x1cb4 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_110->m_textBuffer.SetText(gppStringList[0x1cf4 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_110->m_primaryActivationCallback =
        SetNpcDialogueSubMode4;
    g_screen_state_00649f1c->dialogue_text_114->m_textBuffer.SetText(gppStringList[0x1cf8 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_114->m_primaryActivationCallback =
        SetNpcDialogueSubMode3;
    g_screen_state_00649f1c->dialogue_text_118->m_textBuffer.SetText(gppStringList[0x1cfc / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_118->m_primaryActivationCallback =
        SetNpcDialogueSubMode2;
    g_screen_state_00649f1c->dialogue_text_11c->m_textBuffer.SetText(gppStringList[0x1cb8 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_11c->m_primaryActivationCallback =
        SetNpcDialogueSubMode5;
    g_screen_state_00649f1c->dialogue_text_120->m_textBuffer.SetText(gppStringList[0x1cac / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_120->m_primaryActivationCallback = SelectNpcTradeMode1;
    g_screen_state_00649f1c->dialogue_text_124->m_textBuffer.SetText(gppStringList[0x1cb0 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_124->m_primaryActivationCallback = SelectNpcTradeMode0;
    g_screen_state_00649f1c->dialogue_text_16c->m_primaryActivationCallback = OpenNpcItemAssay;
    g_screen_state_00649f1c->dialogue_text_16c->m_secondaryActivationCallback = OpenNpcItemAssay;
    g_screen_state_00649f1c->dialogue_text_188->SetEnabled(0);
    g_screen_state_00649f1c->dialogue_text_188->m_primaryActivationCallback = ConfirmNpcTradeSlot;
    swprintf(buffer, L"%dg", g_status_685170.party_gold);
    g_screen_state_00649f1c->dialogue_text_190->m_textBuffer.SetText(buffer, g_font_683660);
    g_screen_state_00649f1c->dialogue_text_1a0->SetEnabled(0);
    g_screen_state_00649f1c->dialogue_text_1a0->m_primaryActivationCallback = Function5AD290;
    g_screen_state_00649f1c->dialogue_text_120->AddLayoutFlags(g_W8TextControlMask005ED578);
    g_screen_state_00649f1c->dialogue_text_124->AddLayoutFlags(g_W8TextControlMask005ED578);
    g_screen_state_00649f1c->dialogue_text_120->DisableSecondaryState(1);
    g_screen_state_00649f1c->dialogue_text_124->DisableSecondaryState(1);
    g_screen_state_00649f1c->dialogue_text_110->AddLayoutFlags(g_W8TextControlMask005ED578);
    g_screen_state_00649f1c->dialogue_text_114->AddLayoutFlags(g_W8TextControlMask005ED578);
    g_screen_state_00649f1c->dialogue_text_118->AddLayoutFlags(g_W8TextControlMask005ED578);
    g_screen_state_00649f1c->dialogue_text_11c->AddLayoutFlags(g_W8TextControlMask005ED578);
    UpdateNpcDialogueSubMode();
    g_screen_state_00649f1c->flag_234 = 1;
    if (g_screen_state_00649f1c->flag_229 != 0) {
        g_screen_state_00649f1c->dialogue_text_110->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_114->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_11c->SetEnabled(0);
    } else {
        g_screen_state_00649f1c->dialogue_text_110->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_text_114->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_text_11c->SetEnabled(1);
    }
    if (g_screen_state_00649f1c->dialogue_npc->record->flag_055 == 0) {
        g_screen_state_00649f1c->dialogue_text_110->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_114->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_11c->SetEnabled(0);
    }
    g_screen_state_00649f1c->dialogue_text_118->SetEnabled(1);
    SelectTextBox(2);
    if (gXStatus.fCampMode == 0) {
        ResetEditorStatusLine0058AA20(2);
    }
    g_screen_state_00649f1c->value_1d0 = 0;
    for (index = 0; index < 6; ++index) {
        g_screen_state_00649f1c->option_buttons_170[index]->SetEnabled(0);
    }
    RebuildNpcTradeItemList005ADB10(1);
    RequestRedraw(0x200);
}

// FUNCTION: WIZ8 0x00571F60
void UpdateNpcDialogueSubMode(void)
{
    switch (g_screen_state_00649f1c->value_100) {
    case 2:
        g_screen_state_00649f1c->dialogue_text_118->EnableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_text_110->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_text_114->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_text_11c->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_text_10c->m_textBuffer.SetText(
            gppStringList[0x1cec / 4], g_wiz_text_bold_font_683664);
        g_screen_state_00649f1c->dialogue_text_1a0->m_textBuffer.SetText(
            gppStringList[0x1cec / 4], g_wiz_text_bold_font_683664);
        g_screen_state_00649f1c->dialogue_text_120->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_text_124->SetEnabled(1);
        break;
    case 3:
        g_screen_state_00649f1c->dialogue_text_114->EnableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_text_110->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_text_118->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_text_11c->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_text_120->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_text_124->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_text_10c->m_textBuffer.SetText(
            gppStringList[0x1ce8 / 4], g_wiz_text_bold_font_683664);
        g_screen_state_00649f1c->dialogue_text_1a0->m_textBuffer.SetText(
            gppStringList[0x1ce8 / 4], g_wiz_text_bold_font_683664);
        break;
    case 4:
        g_screen_state_00649f1c->dialogue_text_110->EnableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_text_114->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_text_118->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_text_11c->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_text_10c->m_textBuffer.SetText(
            gppStringList[0x1cf0 / 4], g_wiz_text_bold_font_683664);
        g_screen_state_00649f1c->dialogue_text_1a0->m_textBuffer.SetText(
            gppStringList[0x1cf0 / 4], g_wiz_text_bold_font_683664);
        g_screen_state_00649f1c->dialogue_text_120->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_124->SetEnabled(0);
        break;
    case 5:
        g_screen_state_00649f1c->dialogue_text_11c->EnableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_text_114->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_text_118->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_text_110->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_text_10c->m_textBuffer.SetText(
            gppStringList[0x1cb8 / 4], g_wiz_text_bold_font_683664);
        g_screen_state_00649f1c->dialogue_text_1a0->m_textBuffer.SetText(
            gppStringList[0x1cb8 / 4], g_wiz_text_bold_font_683664);
        g_screen_state_00649f1c->dialogue_text_120->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_124->SetEnabled(0);
        break;
    }
    if (g_screen_state_00649f1c->flag_234 != 0 &&
        (g_screen_state_00649f1c->value_100 == 3 || g_screen_state_00649f1c->value_100 == 2)) {
        if (g_screen_state_00649f1c->flag_260 != 0) {
            g_screen_state_00649f1c->dialogue_text_124->DisableSecondaryState(1);
            g_screen_state_00649f1c->dialogue_text_124->m_textBuffer.m_fontStateIndex = -1;
            g_screen_state_00649f1c->dialogue_text_124->m_textBuffer.m_geometryDirty = 1;
            g_screen_state_00649f1c->dialogue_text_120->EnableSecondaryState(1);
            g_screen_state_00649f1c->dialogue_text_120->m_textBuffer.m_fontStateIndex = 3;
            g_screen_state_00649f1c->dialogue_text_120->m_textBuffer.m_geometryDirty = 1;
        } else {
            g_screen_state_00649f1c->dialogue_text_120->DisableSecondaryState(1);
            g_screen_state_00649f1c->dialogue_text_120->m_textBuffer.m_fontStateIndex = -1;
            g_screen_state_00649f1c->dialogue_text_120->m_textBuffer.m_geometryDirty = 1;
            g_screen_state_00649f1c->dialogue_text_124->EnableSecondaryState(1);
            g_screen_state_00649f1c->dialogue_text_124->m_textBuffer.m_fontStateIndex = 3;
            g_screen_state_00649f1c->dialogue_text_124->m_textBuffer.m_geometryDirty = 1;
        }
        g_screen_state_00649f1c->flag_234 = 0;
    }
    RebuildNpcTradeItemList005ADB10(1);
}

/* Tear down the mode-4 option layout: the six option controls lose their
   secondary state and layout flags before everything is disabled. */
// FUNCTION: WIZ8 0x00572320
void CloseNpcDialogueOptionLayout(void)
{
    W8TextControl* text;

    ResetNpcDialogueItemEditor();
    if (gXStatus.fCampMode == 0) {
        g_screen_state_00649f1c->value_100 = 0;
    }
    RegionSetDisable(0x18);
    RegionSetDisable(0x17);
    g_screen_state_00649f1c->dialogue_text_188->SetEnabled(1);
    g_screen_state_00649f1c->dialogue_text_110->DisableSecondaryState(1);
    g_screen_state_00649f1c->dialogue_text_114->DisableSecondaryState(1);
    g_screen_state_00649f1c->dialogue_text_118->DisableSecondaryState(1);
    g_screen_state_00649f1c->dialogue_text_11c->DisableSecondaryState(1);
    g_screen_state_00649f1c->dialogue_text_120->DisableSecondaryState(1);
    text = g_screen_state_00649f1c->dialogue_text_120;
    text->m_textBuffer.SetFontStateIndex(-1);
    text->m_textBuffer.SetGeometryDirty();
    g_screen_state_00649f1c->dialogue_text_124->DisableSecondaryState(1);
    text = g_screen_state_00649f1c->dialogue_text_124;
    text->m_textBuffer.SetFontStateIndex(-1);
    text->m_textBuffer.SetGeometryDirty();
    g_screen_state_00649f1c->dialogue_text_110->RemoveLayoutFlags(g_W8TextControlMask005ED578);
    g_screen_state_00649f1c->dialogue_text_114->RemoveLayoutFlags(g_W8TextControlMask005ED578);
    g_screen_state_00649f1c->dialogue_text_118->RemoveLayoutFlags(g_W8TextControlMask005ED578);
    g_screen_state_00649f1c->dialogue_text_11c->RemoveLayoutFlags(g_W8TextControlMask005ED578);
    g_screen_state_00649f1c->dialogue_text_120->RemoveLayoutFlags(g_W8TextControlMask005ED578);
    g_screen_state_00649f1c->dialogue_text_124->RemoveLayoutFlags(g_W8TextControlMask005ED578);
    g_screen_state_00649f1c->dialogue_text_110->SetEnabled(1);
    g_screen_state_00649f1c->dialogue_text_114->SetEnabled(1);
    g_screen_state_00649f1c->dialogue_text_118->SetEnabled(1);
    g_screen_state_00649f1c->dialogue_text_11c->SetEnabled(1);
    g_screen_state_00649f1c->dialogue_text_120->SetEnabled(1);
    g_screen_state_00649f1c->dialogue_text_124->SetEnabled(1);
    g_screen_state_00649f1c->panel_1a8->SetEnabled(0);
    g_screen_state_00649f1c->panel_1ac->SetEnabled(0);
    g_screen_state_00649f1c->panel_1bc->SetEnabled(0);
    g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
    g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
    if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
        SetNpcDialogueHidden(0);
    }
    g_screen_state_00649f1c->flag_229 = 0;
    SelectTextBox(3);
    Function58BA60();
}

// FUNCTION: WIZ8 0x00572590
void OpenNpcItemAssay(void)
{
    W8AssayDialog* dialog;

    if (g_screen_state_00649f1c->value_108 != 0 &&
        g_screen_state_00649f1c->dialogue_text_16c->m_imageObject != -1 &&
        g_screen_state_00649f1c->dialogue_text_16c->m_imageObject != 0x1ac) {
        dialog = new W8AssayDialog(
            g_screen_state_00649f1c->value_108,
            &g_status_685170.buffers.characters[g_status_685170.selected_character]);
        dialog->SetText(&g_wchar_00689b34);
        dialog->SetOrigin(g_info_dialog_x_005ef958, 0x48);
        dialog->m_destroy_callback = OnNpcAssayDialogClosed;
        OpenModal(dialog);
    }
}

// FUNCTION: WIZ8 0x00572670
void OnNpcAssayDialogClosed(W8DialogBase*)
{
    RequestRedraw(-1);
}

// FUNCTION: WIZ8 0x00572680
void SelectNpcTradeMode1(void)
{
    ResetNpcDialogueItemEditor();
    g_screen_state_00649f1c->dialogue_text_124->DisableSecondaryState(1);
    g_screen_state_00649f1c->dialogue_text_124->m_textBuffer.m_fontStateIndex = -1;
    g_screen_state_00649f1c->dialogue_text_124->m_textBuffer.m_geometryDirty = 1;
    g_screen_state_00649f1c->dialogue_text_120->EnableSecondaryState(1);
    g_screen_state_00649f1c->dialogue_text_120->m_textBuffer.m_fontStateIndex = 3;
    g_screen_state_00649f1c->dialogue_text_120->m_textBuffer.m_geometryDirty = 1;
    RebuildNpcTradeItemList005ADB10(1);
    g_screen_state_00649f1c->flag_260 = 1;
}

// FUNCTION: WIZ8 0x00572700
void SelectNpcTradeMode0(void)
{
    ResetNpcDialogueItemEditor();
    g_screen_state_00649f1c->dialogue_text_120->DisableSecondaryState(1);
    g_screen_state_00649f1c->dialogue_text_120->m_textBuffer.m_fontStateIndex = -1;
    g_screen_state_00649f1c->dialogue_text_120->m_textBuffer.m_geometryDirty = 1;
    g_screen_state_00649f1c->dialogue_text_124->EnableSecondaryState(1);
    g_screen_state_00649f1c->dialogue_text_124->m_textBuffer.m_fontStateIndex = 3;
    g_screen_state_00649f1c->dialogue_text_124->m_textBuffer.m_geometryDirty = 1;
    RebuildNpcTradeItemList005ADB10(1);
    g_screen_state_00649f1c->flag_260 = 0;
}

// FUNCTION: WIZ8 0x00572960
void ConfirmNpcTradeSlot(void)
{
    int slot;

    if (g_screen_state_00649f1c->value_fc == W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX) {
        slot = GetTextSlot1E8(2);
        if (slot != -1) {
            UpdateNpcTradeSelection0056FAC0(slot, 0, 0);
            if (g_screen_state_00649f1c->value_100 == 2 && slot == 0) {
                OpenNpcTradeSplitDialog00572780();
            } else if (g_screen_state_00649f1c->value_108->stack_count > 1) {
                OpenNpcTradeSplitDialog005AE040();
            }
        }
    }
}

/* Open the mode-5 dialogue layout: the smaller control set shares the
   mode-4 labels and callbacks, the purse readout is refreshed and the
   mode-4-only controls are parked. */
/* The "Gold" split-amount entry: reset the item editor, dim the six option
   buttons and open the W8SplitAmountDialog seeded with the party purse. The
   result lands back through OnNpcTradeSplitDialogDestroy00572870. */
// FUNCTION: WIZ8 0x00572780
void OpenNpcTradeSplitDialog00572780(void)
{
    W8SplitAmountDialog* dialog;

    ResetNpcDialogueItemEditor();
    g_screen_state_00649f1c->dialogue_text_110->DisableSecondaryState(1);
    g_screen_state_00649f1c->dialogue_text_114->DisableSecondaryState(1);
    for (int index = 0; index < 6; ++index) {
        g_screen_state_00649f1c->option_buttons_170[index]->SetEnabled(0);
    }
    g_screen_state_00649f1c->panel_1ac->Invalidate(0);
    dialog = new W8SplitAmountDialog(g_status_685170.party_gold);
    dialog->SetText(&g_wchar_00689b34);
    dialog->SetOrigin(g_split_dialog_origin_x_005ef9dc, g_split_dialog_origin_y_005ef9e0);
    dialog->m_destroy_callback = OnNpcTradeSplitDialogDestroy00572870;
    OpenModal(dialog);
}

/* Destroy callback for the trade split-amount dialog: on confirm it takes the
   entered share into value_22c and refreshes the purse readout. */
// FUNCTION: WIZ8 0x00572870
void OnNpcTradeSplitDialogDestroy00572870(W8DialogBase* dialog)
{
    wchar_t text[0x20];

    if (static_cast<W8SplitAmountDialog*>(dialog)->m_result_08c !=
        g_split_dialog_confirm_005ef9d4) {
        return;
    }
    g_screen_state_00649f1c->value_22c = static_cast<W8SplitAmountDialog*>(dialog)->m_taken_084;
    g_screen_state_00649f1c->dialogue_text_1a0->SetEnabled(1);
    g_screen_state_00649f1c->dialogue_text_1a0->m_textBuffer.m_fontStateIndex = 1;
    g_screen_state_00649f1c->dialogue_text_1a0->m_textBuffer.m_geometryDirty = 1;
    g_screen_state_00649f1c->dialogue_text_188->SetEnabled(1);
    g_screen_state_00649f1c->dialogue_text_19c->m_textBuffer.SetText(gppStringList[0x72d],
                                                                     g_font_683660);
    swprintf(text, L"%dg", g_screen_state_00649f1c->value_22c);
    g_screen_state_00649f1c->dialogue_text_198->m_textBuffer.SetText(text, g_font_683660);
    g_screen_state_00649f1c->panel_1bc->Invalidate(0);
}

/* Resolve and select one trade-list row. value_100 picks the pool: mode 2's
   leading row is the purse readout, the mode-3 views iterate either the
   selected character's backpack or the shared party item pool, and modes 4/5
   go through the NPC's own inventory. pick/commit drive quantity stepping,
   the click chime and the highlight tick. */
// FUNCTION: WIZ8 0x005729C0
W8ItemInstance* ResolveNpcTradeRow005729C0(int index, char pick, char decrement, char commit)
{
    wchar_t count_text[32];
    wchar_t* text;
    W8ItemInstance* pool;
    W8NpcItemEntry* entry;
    int selected;
    int hit;
    int i;
    int image;

    selected = g_status_685170.selected_character;
    hit = 0;
    if (g_screen_state_00649f1c->value_100 != 4 && g_screen_state_00649f1c->value_100 != 5) {
        if (g_screen_state_00649f1c->value_100 == 2) {
            g_screen_state_00649f1c->value_22c = 0;
            if (index == 0) {
                g_screen_state_00649f1c->value_22c = g_status_685170.party_gold;
                g_screen_state_00649f1c->dialogue_text_16c->m_imageObject = 0x1ac;
                g_screen_state_00649f1c->dialogue_text_16c->m_measured_w = -1;
                g_screen_state_00649f1c->dialogue_text_16c->m_measured_h = -1;
                g_screen_state_00649f1c->dialogue_text_16c->m_imageFrame = 0;
                g_screen_state_00649f1c->dialogue_text_16c->m_normalSprite = 0;
                g_screen_state_00649f1c->dialogue_text_16c->m_pressedSprite = 0;
                g_screen_state_00649f1c->dialogue_text_16c->m_textBuffer.SetText(g_wchar_0068ee58,
                                                                                 g_font_683660);
                return 0;
            }
            --index;
        }
        if (static_cast<unsigned char>(g_screen_state_00649f1c->dialogue_text_120->m_stateFlags &
                                       g_W8TextControlMask005ED570) == 0) {
            if (static_cast<unsigned char>(
                    g_screen_state_00649f1c->dialogue_text_124->m_stateFlags &
                    g_W8TextControlMask005ED570) != 0 &&
                g_status_685170.party_item_count_1791 != 0) {
                i = 0;
                do {
                    if (g_status_685170.party_item_pool_0021[i].item_id != -1 &&
                        NpcTradeItemAllowed00573190(&g_status_685170.party_item_pool_0021[i]) ==
                            0) {
                        if (index == hit) {
                            if (pick == 0) {
                                return &g_status_685170.party_item_pool_0021[i];
                            }
                            if (commit != 0) {
                                if (g_item_records[g_status_685170.party_item_pool_0021[i].item_id]
                                        .equip_class == 4) {
                                    g_screen_state_00649f1c->value_254 =
                                        g_status_685170.party_item_pool_0021[i].stack_count;
                                } else if (g_screen_state_00649f1c->value_258 == hit) {
                                    if (decrement == 0) {
                                        if (g_item_records[g_status_685170.party_item_pool_0021[i]
                                                               .item_id]
                                                .quantity_kind == 1) {
                                            ++g_screen_state_00649f1c->value_254;
                                            if (g_status_685170.party_item_pool_0021[i]
                                                    .stack_count <
                                                g_screen_state_00649f1c->value_254) {
                                                g_screen_state_00649f1c->value_254 =
                                                    g_status_685170.party_item_pool_0021[i]
                                                        .stack_count;
                                            }
                                        }
                                    } else {
                                        --g_screen_state_00649f1c->value_254;
                                        if (g_screen_state_00649f1c->value_254 == 0) {
                                            g_screen_state_00649f1c->value_254 = 1;
                                        }
                                    }
                                } else {
                                    g_screen_state_00649f1c->value_254 = 1;
                                }
                            }
                            g_screen_state_00649f1c->value_258 = hit;
                            if (commit != 0) {
                                SoundPlay(g_button_click_1_62a51c, 0);
                            }
                            image = g_item_video_objects_68ec68.GetOrCreateVideoObject(
                                g_status_685170.party_item_pool_0021[i].item_id);
                            g_screen_state_00649f1c->dialogue_text_16c->m_imageObject = image;
                            g_screen_state_00649f1c->dialogue_text_16c->m_measured_w = -1;
                            g_screen_state_00649f1c->dialogue_text_16c->m_measured_h = -1;
                            g_screen_state_00649f1c->dialogue_text_16c->m_imageFrame = 0;
                            g_screen_state_00649f1c->dialogue_text_16c->m_normalSprite = 0;
                            g_screen_state_00649f1c->dialogue_text_16c->m_pressedSprite = 0;
                            if (g_status_685170.party_item_pool_0021[i].stack_count < 2) {
                                text = g_wchar_0068ee58;
                            } else {
                                swprintf(count_text, L"%d", g_screen_state_00649f1c->value_254);
                                text = count_text;
                            }
                            g_screen_state_00649f1c->dialogue_text_16c->m_textBuffer.SetText(
                                text, g_font_683660);
                            g_screen_state_00649f1c->dialogue_text_16c->Invalidate(1);
                            return &g_status_685170.party_item_pool_0021[i];
                        }
                        ++hit;
                    }
                    ++i;
                    if (static_cast<unsigned int>(g_status_685170.party_item_count_1791) <=
                        static_cast<unsigned int>(i)) {
                        return 0;
                    }
                } while (true);
            }
        } else {
            W8Character* character = &g_status_685170.buffers.characters[selected];
            i = 0;
            do {
                if (character->backpack[i].item_id != -1 &&
                    NpcTradeItemAllowed00573190(&character->backpack[i]) == 0) {
                    if (index == hit) {
                        if (pick == 0) {
                            return &character->backpack[i];
                        }
                        if (commit != 0) {
                            if (g_item_records[character->backpack[i].item_id].equip_class == 4) {
                                g_screen_state_00649f1c->value_254 =
                                    character->backpack[i].stack_count;
                            } else if (g_screen_state_00649f1c->value_258 == hit) {
                                if (decrement == 0) {
                                    if (g_item_records[character->backpack[i].item_id]
                                            .quantity_kind == 1) {
                                        ++g_screen_state_00649f1c->value_254;
                                        if (character->backpack[i].stack_count <
                                            g_screen_state_00649f1c->value_254) {
                                            g_screen_state_00649f1c->value_254 =
                                                character->backpack[i].stack_count;
                                        }
                                    }
                                } else {
                                    --g_screen_state_00649f1c->value_254;
                                    if (g_screen_state_00649f1c->value_254 == 0) {
                                        g_screen_state_00649f1c->value_254 = 1;
                                    }
                                }
                            } else {
                                g_screen_state_00649f1c->value_254 = 1;
                            }
                        }
                        g_screen_state_00649f1c->value_258 = hit;
                        if (commit != 0) {
                            SoundPlay(g_button_click_1_62a51c, 0);
                        }
                        image = g_item_video_objects_68ec68.GetOrCreateVideoObject(
                            character->backpack[i].item_id);
                        g_screen_state_00649f1c->dialogue_text_16c->m_imageObject = image;
                        g_screen_state_00649f1c->dialogue_text_16c->m_measured_w = -1;
                        g_screen_state_00649f1c->dialogue_text_16c->m_measured_h = -1;
                        g_screen_state_00649f1c->dialogue_text_16c->m_imageFrame = 0;
                        g_screen_state_00649f1c->dialogue_text_16c->m_normalSprite = 0;
                        g_screen_state_00649f1c->dialogue_text_16c->m_pressedSprite = 0;
                        if (character->backpack[i].stack_count < 2) {
                            text = g_wchar_0068ee58;
                        } else {
                            swprintf(count_text, L"%d", g_screen_state_00649f1c->value_254);
                            text = count_text;
                        }
                        g_screen_state_00649f1c->dialogue_text_16c->m_textBuffer.SetText(
                            text, g_font_683660);
                        g_screen_state_00649f1c->dialogue_text_16c->Invalidate(1);
                        return &character->backpack[i];
                    }
                    ++hit;
                }
                ++i;
            } while (i < 8);
        }
        return 0;
    }
    i = ResolveNpcTradeStockIndex005ADAA0(index);
    if (i == -1) {
        return 0;
    }
    entry = GetNpcItemAt(g_screen_state_00649f1c->dialogue_npc, i);
    if (entry == 0) {
        return 0;
    }
    if (commit != 0) {
        if (g_item_records[entry->item.item_id].equip_class == 4) {
            g_screen_state_00649f1c->value_254 = entry->item.stack_count;
        } else if (g_screen_state_00649f1c->value_258 == i) {
            if (decrement == 0) {
                if (g_item_records[entry->item.item_id].quantity_kind == 1) {
                    ++g_screen_state_00649f1c->value_254;
                    if (entry->item.stack_count < g_screen_state_00649f1c->value_254) {
                        g_screen_state_00649f1c->value_254 = entry->item.stack_count;
                        goto selected;
                    }
                    if (entry->item.stack_count != 0) {
                        g_screen_state_00649f1c->dialogue_text_16c->m_textBuffer.m_flag_4c = 1;
                        g_screen_state_00649f1c->dialogue_text_16c->m_textBuffer.m_geometryDirty =
                            1;
                        g_screen_state_00649f1c->dialogue_text_16c->Invalidate(0);
                        g_screen_state_00649f1c->dialogue_text_198->m_textBuffer.m_flag_4c = 1;
                        g_screen_state_00649f1c->dialogue_text_198->m_textBuffer.m_geometryDirty =
                            1;
                        g_screen_state_00649f1c->dialogue_text_198->Invalidate(0);
                        g_trade_highlight_tick_68ee78 = GetTickCount();
                    }
                }
            } else {
                --g_screen_state_00649f1c->value_254;
                if (g_screen_state_00649f1c->value_254 == 0) {
                    g_screen_state_00649f1c->value_254 = 1;
                    goto selected;
                }
                if (entry->item.stack_count != 0) {
                    g_screen_state_00649f1c->dialogue_text_16c->m_textBuffer.m_flag_4c = 1;
                    g_screen_state_00649f1c->dialogue_text_16c->m_textBuffer.m_geometryDirty = 1;
                    g_screen_state_00649f1c->dialogue_text_16c->Invalidate(0);
                    g_screen_state_00649f1c->dialogue_text_198->m_textBuffer.m_flag_4c = 1;
                    g_screen_state_00649f1c->dialogue_text_198->m_textBuffer.m_geometryDirty = 1;
                    g_screen_state_00649f1c->dialogue_text_198->Invalidate(0);
                    g_trade_highlight_tick_68ee78 = GetTickCount();
                }
            }
        } else {
            g_screen_state_00649f1c->value_254 = 1;
        }
    }
selected:
    g_screen_state_00649f1c->value_258 = i;
    if (pick != 0) {
        if (commit != 0) {
            SoundPlay(g_button_click_1_62a51c, 0);
        }
        image = g_item_video_objects_68ec68.GetOrCreateVideoObject(entry->item.item_id);
        g_screen_state_00649f1c->dialogue_text_16c->m_imageObject = image;
        g_screen_state_00649f1c->dialogue_text_16c->m_measured_w = -1;
        g_screen_state_00649f1c->dialogue_text_16c->m_measured_h = -1;
        g_screen_state_00649f1c->dialogue_text_16c->m_imageFrame = 0;
        g_screen_state_00649f1c->dialogue_text_16c->m_normalSprite = 0;
        g_screen_state_00649f1c->dialogue_text_16c->m_pressedSprite = 0;
        if (entry->item.stack_count < 2) {
            text = g_wchar_0068ee58;
        } else {
            swprintf(count_text, L"%d", g_screen_state_00649f1c->value_254);
            text = count_text;
        }
        g_screen_state_00649f1c->dialogue_text_16c->m_textBuffer.SetText(text, g_font_683660);
        g_screen_state_00649f1c->dialogue_text_16c->Invalidate(1);
    }
    return &entry->item;
}

/* Whether the current trade filter rejects `item`: the Zant NPC hides item
   0x290 until fact 0x3c, the value_1d0 low bits gate character/party
   usability, and bits 0x3c select the accepted equipment slot group. */
// FUNCTION: WIZ8 0x00573190
unsigned char NpcTradeItemAllowed00573190(W8ItemInstance* item)
{
    int group;

    if (g_screen_state_00649f1c->dialogue_npc->name_style == ',' && GetFact(0x3c) == 0 &&
        item->item_id != 0x290) {
        return 1;
    }
    if (g_screen_state_00649f1c->value_1d0 == 0) {
        return 0;
    }
    if (item->item_id != -1) {
        if ((g_screen_state_00649f1c->value_1d0 & 1) == 0) {
            if ((g_screen_state_00649f1c->value_1d0 & 0x40) != 0 &&
                AnyPartyMemberCanUseItem(item->item_id) != 0) {
                return 1;
            }
        } else {
            if (CanCharacterUseItem(
                    &g_status_685170.buffers.characters[g_status_685170.selected_character],
                    item->item_id) == 0) {
                return 1;
            }
        }
        if ((g_screen_state_00649f1c->value_1d0 & 0x3c) == 0) {
            return 0;
        }
        group = GetItemEquipSlotGroup(item->item_id);
        if (group == 2) {
            if ((g_screen_state_00649f1c->value_1d0 & 4) != 0) {
                return 0;
            }
        } else if (group == 3) {
            if ((g_screen_state_00649f1c->value_1d0 & 8) != 0) {
                return 0;
            }
        } else if (group == 4) {
            if ((g_screen_state_00649f1c->value_1d0 & 0x10) != 0) {
                return 0;
            }
        } else if ((g_screen_state_00649f1c->value_1d0 & 0x20) != 0) {
            return 0;
        }
    }
    return 1;
}

// FUNCTION: WIZ8 0x005732A0
void OpenNpcDialogueMode5Layout(void)
{
    wchar_t buffer[0x20];
    int index;

    g_screen_state_00649f1c->value_fc = 5;
    if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
        SetNpcDialogueHidden(0);
    }
    g_screen_state_00649f1c->panel_1a8->SetEnabled(1);
    g_screen_state_00649f1c->panel_1ac->SetEnabled(1);
    g_screen_state_00649f1c->panel_1bc->SetEnabled(1);
    RegionSetEnable(0x18);
    RegionSetEnable(0x17);
    g_screen_state_00649f1c->dialogue_text_10c->m_textBuffer.SetText(gppStringList[0x1ce4 / 4],
                                                                     g_wiz_text_bold_font_683664);
    g_screen_state_00649f1c->dialogue_text_110->m_textBuffer.SetText(gppStringList[0x1cf4 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_110->m_primaryActivationCallback =
        SetNpcDialogueSubMode4;
    g_screen_state_00649f1c->dialogue_text_114->m_textBuffer.SetText(gppStringList[0x1cf8 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_114->m_primaryActivationCallback =
        SetNpcDialogueSubMode3;
    g_screen_state_00649f1c->dialogue_text_118->m_textBuffer.SetText(gppStringList[0x1cfc / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_118->m_primaryActivationCallback =
        SetNpcDialogueSubMode2;
    g_screen_state_00649f1c->dialogue_text_11c->m_textBuffer.SetText(gppStringList[0x1cb8 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_11c->m_primaryActivationCallback =
        SetNpcDialogueSubMode5;
    g_screen_state_00649f1c->dialogue_text_188->SetEnabled(0);
    g_screen_state_00649f1c->dialogue_text_188->m_primaryActivationCallback = ConfirmNpcTradeSlot;
    swprintf(buffer, L"%dg", g_status_685170.party_gold);
    g_screen_state_00649f1c->dialogue_text_190->m_textBuffer.SetText(buffer, g_font_683660);
    g_screen_state_00649f1c->dialogue_text_1a0->SetEnabled(0);
    g_screen_state_00649f1c->dialogue_text_1a0->m_primaryActivationCallback = RestockNpcTradeStock;
    g_screen_state_00649f1c->dialogue_text_120->SetActive(0);
    g_screen_state_00649f1c->dialogue_text_124->SetActive(0);
    g_screen_state_00649f1c->value_1d0 = 0;
    for (index = 0; index < 6; ++index) {
        g_screen_state_00649f1c->option_buttons_170[index]->SetEnabled(0);
    }
    if (g_screen_state_00649f1c->dialogue_npc->record->flag_055 == 0) {
        g_screen_state_00649f1c->dialogue_text_110->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_114->SetEnabled(0);
    }
    g_screen_state_00649f1c->dialogue_text_118->SetEnabled(1);
    RequestRedraw(0x200);
    SelectTextBox(2);
    if (gXStatus.fCampMode == 0) {
        ResetEditorStatusLine0058AA20(2);
    }
}

/* Tear down the mode-5 dialogue layout and retire the current mode. */
// FUNCTION: WIZ8 0x00573570
void CloseNpcDialogueMode5Layout(void)
{
    RegionSetDisable(0x18);
    RegionSetDisable(0x17);
    g_screen_state_00649f1c->dialogue_text_188->SetEnabled(1);
    g_screen_state_00649f1c->panel_1a8->SetEnabled(0);
    g_screen_state_00649f1c->panel_1ac->SetEnabled(0);
    g_screen_state_00649f1c->panel_1bc->SetEnabled(0);
    g_screen_state_00649f1c->dialogue_text_110->SetEnabled(1);
    g_screen_state_00649f1c->dialogue_text_114->SetEnabled(1);
    g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
    g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
    if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
        SetNpcDialogueHidden(0);
    }
}

/* Re-arm the six trade-filter option buttons after the list contents were
   rebuilt. */
// FUNCTION: WIZ8 0x00573630
void EnableNpcTradeFilterButtons00573630(void)
{
    for (unsigned int index = 0; index <= 5; ++index) {
        g_screen_state_00649f1c->option_buttons_170[index]->SetEnabled(1);
    }
}

/* The six trade-filter option callbacks share one shape: when the button's
   secondary state is raised it becomes the exclusive slot-group bit in
   value_1d0 (the previously active sibling is dimmed and cleared), and when it
   is lowered the bit comes off again; RebuildNpcTradeItemList005ADB10(1) refreshes the list. */
// FUNCTION: WIZ8 0x00573660
void ToggleNpcTradeFilter00573660(void)
{
    if (static_cast<unsigned char>(g_screen_state_00649f1c->option_buttons_170[0]->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        switch (g_screen_state_00649f1c->value_1d0 & 0x3c) {
        case 8:
            g_screen_state_00649f1c->option_buttons_170[3]->DisableSecondaryState(1);
            g_screen_state_00649f1c->value_1d0 &= ~8;
            break;
        case 16:
            g_screen_state_00649f1c->option_buttons_170[1]->DisableSecondaryState(1);
            g_screen_state_00649f1c->value_1d0 &= ~0x10;
            break;
        case 32:
            g_screen_state_00649f1c->option_buttons_170[4]->DisableSecondaryState(1);
            g_screen_state_00649f1c->value_1d0 &= ~0x20;
            break;
        }
        g_screen_state_00649f1c->value_1d0 |= 4;
        RebuildNpcTradeItemList005ADB10(1);
        return;
    }
    g_screen_state_00649f1c->value_1d0 &= ~4;
    RebuildNpcTradeItemList005ADB10(1);
}

// FUNCTION: WIZ8 0x00573730
void ToggleNpcTradeFilter00573730(void)
{
    if (static_cast<unsigned char>(g_screen_state_00649f1c->option_buttons_170[1]->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        switch (g_screen_state_00649f1c->value_1d0 & 0x3c) {
        case 4:
            g_screen_state_00649f1c->option_buttons_170[0]->DisableSecondaryState(1);
            g_screen_state_00649f1c->value_1d0 &= ~4;
            break;
        case 8:
            g_screen_state_00649f1c->option_buttons_170[3]->DisableSecondaryState(1);
            g_screen_state_00649f1c->value_1d0 &= ~8;
            break;
        case 32:
            g_screen_state_00649f1c->option_buttons_170[4]->DisableSecondaryState(1);
            g_screen_state_00649f1c->value_1d0 &= ~0x20;
            break;
        }
        g_screen_state_00649f1c->value_1d0 |= 0x10;
        RebuildNpcTradeItemList005ADB10(1);
        return;
    }
    g_screen_state_00649f1c->value_1d0 &= ~0x10;
    RebuildNpcTradeItemList005ADB10(1);
}

// FUNCTION: WIZ8 0x00573800
void ToggleNpcTradeFilter00573800(void)
{
    if (static_cast<unsigned char>(g_screen_state_00649f1c->option_buttons_170[3]->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        switch (g_screen_state_00649f1c->value_1d0 & 0x3c) {
        case 4:
            g_screen_state_00649f1c->option_buttons_170[0]->DisableSecondaryState(1);
            g_screen_state_00649f1c->value_1d0 &= ~4;
            break;
        case 16:
            g_screen_state_00649f1c->option_buttons_170[1]->DisableSecondaryState(1);
            g_screen_state_00649f1c->value_1d0 &= ~0x10;
            break;
        case 32:
            g_screen_state_00649f1c->option_buttons_170[4]->DisableSecondaryState(1);
            g_screen_state_00649f1c->value_1d0 &= ~0x20;
            break;
        }
        g_screen_state_00649f1c->value_1d0 |= 8;
        RebuildNpcTradeItemList005ADB10(1);
        return;
    }
    g_screen_state_00649f1c->value_1d0 &= ~8;
    RebuildNpcTradeItemList005ADB10(1);
}

// FUNCTION: WIZ8 0x005738D0
void ToggleNpcTradeFilter005738D0(void)
{
    if (static_cast<unsigned char>(g_screen_state_00649f1c->option_buttons_170[4]->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        switch (g_screen_state_00649f1c->value_1d0 & 0x3c) {
        case 4:
            g_screen_state_00649f1c->option_buttons_170[0]->DisableSecondaryState(1);
            g_screen_state_00649f1c->value_1d0 &= ~4;
            break;
        case 8:
            g_screen_state_00649f1c->option_buttons_170[3]->DisableSecondaryState(1);
            g_screen_state_00649f1c->value_1d0 &= ~8;
            break;
        case 16:
            g_screen_state_00649f1c->option_buttons_170[1]->DisableSecondaryState(1);
            g_screen_state_00649f1c->value_1d0 &= ~0x10;
            break;
        }
        g_screen_state_00649f1c->value_1d0 |= 0x20;
        RebuildNpcTradeItemList005ADB10(1);
        return;
    }
    g_screen_state_00649f1c->value_1d0 &= ~0x20;
    RebuildNpcTradeItemList005ADB10(1);
}

/* The "usable by the selected character" toggle is exclusive with the
   "usable by anyone" toggle: option_buttons_170[2] lowers bit 0x40 and raises
   bit 1 while pressed, and drops bit 1 when released. */
// FUNCTION: WIZ8 0x005739A0
void ToggleNpcTradeFilter005739A0(void)
{
    if (static_cast<unsigned char>(g_screen_state_00649f1c->option_buttons_170[2]->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        g_screen_state_00649f1c->option_buttons_170[5]->DisableSecondaryState(1);
        g_screen_state_00649f1c->value_1d0 &= ~0x40;
        g_screen_state_00649f1c->value_1d0 |= 1;
        RebuildNpcTradeItemList005ADB10(1);
        return;
    }
    g_screen_state_00649f1c->value_1d0 &= ~1;
    RebuildNpcTradeItemList005ADB10(1);
}

// FUNCTION: WIZ8 0x00573A10
void ToggleNpcTradeFilter00573A10(void)
{
    if (static_cast<unsigned char>(g_screen_state_00649f1c->option_buttons_170[5]->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        g_screen_state_00649f1c->option_buttons_170[2]->DisableSecondaryState(1);
        g_screen_state_00649f1c->value_1d0 &= ~1;
        g_screen_state_00649f1c->value_1d0 |= 0x40;
        RebuildNpcTradeItemList005ADB10(1);
        return;
    }
    g_screen_state_00649f1c->value_1d0 &= ~0x40;
    RebuildNpcTradeItemList005ADB10(1);
}

// FUNCTION: WIZ8 0x00573A80
void SetNpcDialogueSubMode3(void)
{
    g_screen_state_00649f1c->value_100 = 3;
    UpdateNpcDialogueSubMode();
}

// FUNCTION: WIZ8 0x00573AA0
void SetNpcDialogueSubMode2(void)
{
    g_screen_state_00649f1c->value_100 = 2;
    UpdateNpcDialogueSubMode();
}

// FUNCTION: WIZ8 0x00573AC0
void SetNpcDialogueSubMode5(void)
{
    g_screen_state_00649f1c->value_100 = 5;
    UpdateNpcDialogueSubMode();
}

/* Open the mode-1 service layout: the caption and three service options, the
   mode-4-only controls parked, and each option enabled from what the selected
   character can actually cast or use. */
// FUNCTION: WIZ8 0x00573AE0
void OpenNpcDialogueMode1Layout(void)
{
    W8Character* character;

    g_screen_state_00649f1c->value_fc = 1;
    if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
        SetNpcDialogueHidden(0);
    }
    g_screen_state_00649f1c->panel_1a8->SetEnabled(1);
    g_screen_state_00649f1c->panel_1ac->SetEnabled(1);
    g_screen_state_00649f1c->panel_1b8->SetEnabled(1);
    g_screen_state_00649f1c->dialogue_text_1a4->SetActive(0);
    g_screen_state_00649f1c->dialogue_text_1a4->m_textBuffer.SetText(&g_wchar_00689b34,
                                                                     g_wiz_text_bold_font_683664);
    RegionSetEnable(0x18);
    g_screen_state_00649f1c->dialogue_text_10c->m_textBuffer.SetText(gppStringList[0x1cbc / 4],
                                                                     g_wiz_text_bold_font_683664);
    g_screen_state_00649f1c->dialogue_text_110->m_textBuffer.SetText(gppStringList[0x1cc0 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_110->m_primaryActivationCallback =
        RequestNpcSpellService3;
    g_screen_state_00649f1c->dialogue_text_114->m_textBuffer.SetText(gppStringList[0x1cc8 / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_114->m_primaryActivationCallback =
        RequestNpcSpellService41;
    g_screen_state_00649f1c->dialogue_text_11c->m_textBuffer.SetText(gppStringList[0x1ccc / 4],
                                                                     g_font_683660);
    g_screen_state_00649f1c->dialogue_text_11c->m_primaryActivationCallback =
        RequestNpcCharacterService;
    g_screen_state_00649f1c->dialogue_text_11c->AddLayoutFlags(g_W8TextControlMask005ED578);
    g_screen_state_00649f1c->dialogue_text_11c->EnableRegionHelp(0x7ca);
    g_screen_state_00649f1c->dialogue_text_118->SetActive(0);
    g_screen_state_00649f1c->dialogue_text_120->SetActive(0);
    g_screen_state_00649f1c->dialogue_text_124->SetActive(0);
    if (g_screen_state_00649f1c->value_fc == 1) {
        character = &g_status_685170.buffers.characters[g_status_685170.selected_character];
        g_screen_state_00649f1c->dialogue_text_110->SetEnabled(CanCharacterCastSpell(character, 3));
        g_screen_state_00649f1c->dialogue_text_110->W8Widget::Invalidate(1);
        g_screen_state_00649f1c->dialogue_text_114->SetEnabled(
            CanCharacterCastSpell(character, 0x29));
        g_screen_state_00649f1c->dialogue_text_114->W8Widget::Invalidate(1);
        g_screen_state_00649f1c->dialogue_text_11c->SetEnabled(CharacterHasServiceItem(character));
        g_screen_state_00649f1c->dialogue_text_11c->W8Widget::Invalidate(1);
    }
    RequestRedraw(0x200);
    SelectTextBox(2);
    if (gXStatus.fCampMode == 0) {
        ResetEditorStatusLine0058AA20(2);
    }
}

/* Tear down the mode-1 dialogue layout; outside camp the mode is retired as
   well. */
// FUNCTION: WIZ8 0x00573DD0
void CloseNpcDialogueMode1Layout(void)
{
    RegionSetDisable(0x18);
    RegionSetDisable(0x17);
    g_screen_state_00649f1c->dialogue_text_11c->RemoveLayoutFlags(g_W8TextControlMask005ED578);
    g_screen_state_00649f1c->panel_1a8->SetEnabled(0);
    g_screen_state_00649f1c->panel_1ac->SetEnabled(0);
    g_screen_state_00649f1c->panel_1b8->SetEnabled(0);
    g_screen_state_00649f1c->dialogue_text_11c->DisableRegionHelp();
    g_screen_state_00649f1c->dialogue_text_110->SetEnabled(1);
    g_screen_state_00649f1c->dialogue_text_114->SetEnabled(1);
    g_screen_state_00649f1c->dialogue_text_118->SetEnabled(1);
    g_screen_state_00649f1c->dialogue_text_11c->SetEnabled(1);
    if (gXStatus.fCampMode == 0) {
        g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
        g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
        if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
            SetNpcDialogueHidden(0);
        }
    }
}

// FUNCTION: WIZ8 0x00573ED0
void RequestNpcSpellService3(void)
{
    int location = g_screen_state_00649f1c->target_location_id_f8;
    int mode = g_screen_state_00649f1c->value_fc;

    gXStatus.fCampMode = 1;
    CloseNpcDialogueMode1Layout();
    EndNpcDialogueSession0056E800(0);
    BeginSpellCast005A0110(3, location, mode);
}

// FUNCTION: WIZ8 0x00573F10
void RequestNpcSpellService41(void)
{
    int location = g_screen_state_00649f1c->target_location_id_f8;
    int mode = g_screen_state_00649f1c->value_fc;

    gXStatus.fCampMode = 1;
    CloseNpcDialogueMode1Layout();
    EndNpcDialogueSession0056E800(0);
    BeginSpellCast005A0110(0x29, location, mode);
}

// FUNCTION: WIZ8 0x00573F50
void RequestNpcCharacterService(void)
{
    gXStatus.fCampMode = 1;
    CloseNpcDialogueMode1Layout();
    EndNpcDialogueSession0056E800(0);
    OpenUseItemSelectView(g_status_685170.selected_character);
}

/* Route the player's reply text while a modal answer is pending. With no
   price offer outstanding the text resolves through the current quote's
   keyword tables and queues the matching line; during a 0x12/0x1e price check
   the affirmative string spends pending_price_204 (or runs line 0x14 when the
   party cannot pay), tells the offered fact unless opcode 0x1e suppressed it,
   and a refusal runs the pending fact's kind-0x17 decline entries. A nonzero
   echo posts the reply text back as a notice. */
/* Pick the index-th activatable item on the dialogue character: the twelve
   equipment slots first, then the eight backpack slots. The hit refreshes the
   dialogue_text_16c preview image and its stack-count text. */
// FUNCTION: WIZ8 0x00573F80
W8ItemInstance* GetNpcTradeSlotItem00573F80(int index)
{
    wchar_t count_text[32];
    wchar_t* text;
    W8Character* character;
    int image;
    int hit;
    int slot;

    character = &g_status_685170.buffers.characters[g_status_685170.selected_character];
    hit = 0;
    if (static_cast<unsigned char>(g_screen_state_00649f1c->dialogue_text_11c->m_stateFlags &
                                   g_W8TextControlMask005ED570) == 0) {
        return 0;
    }
    for (slot = 0; slot < 12; ++slot) {
        if (character->equipment[slot].item_id != -1 &&
            CanCharacterActivateItem(character, &character->equipment[slot]) != 0) {
            if (index == hit) {
                image = g_item_video_objects_68ec68.GetOrCreateVideoObject(
                    character->equipment[slot].item_id);
                g_screen_state_00649f1c->dialogue_text_16c->m_imageObject = image;
                g_screen_state_00649f1c->dialogue_text_16c->m_measured_w = -1;
                g_screen_state_00649f1c->dialogue_text_16c->m_measured_h = -1;
                g_screen_state_00649f1c->dialogue_text_16c->m_imageFrame = 0;
                g_screen_state_00649f1c->dialogue_text_16c->m_normalSprite = 0;
                g_screen_state_00649f1c->dialogue_text_16c->m_pressedSprite = 0;
                if (character->equipment[slot].stack_count < 2) {
                    text = g_wchar_0068ee58;
                } else {
                    swprintf(count_text, L"%d", character->equipment[slot].stack_count);
                    text = count_text;
                }
                g_screen_state_00649f1c->dialogue_text_16c->m_textBuffer.SetText(text,
                                                                                 g_font_683660);
                g_screen_state_00649f1c->dialogue_text_16c->Invalidate(1);
                return &character->equipment[slot];
            }
            ++hit;
        }
    }
    for (slot = 0; slot < 8; ++slot) {
        if (character->backpack[slot].item_id != -1 &&
            CanCharacterActivateItem(character, &character->backpack[slot]) != 0) {
            if (index == hit) {
                image = g_item_video_objects_68ec68.GetOrCreateVideoObject(
                    character->backpack[slot].item_id);
                g_screen_state_00649f1c->dialogue_text_16c->m_imageObject = image;
                g_screen_state_00649f1c->dialogue_text_16c->m_measured_w = -1;
                g_screen_state_00649f1c->dialogue_text_16c->m_measured_h = -1;
                g_screen_state_00649f1c->dialogue_text_16c->m_imageFrame = 0;
                g_screen_state_00649f1c->dialogue_text_16c->m_normalSprite = 0;
                g_screen_state_00649f1c->dialogue_text_16c->m_pressedSprite = 0;
                if (character->backpack[slot].stack_count < 2) {
                    text = g_wchar_0068ee58;
                } else {
                    swprintf(count_text, L"%d", character->backpack[slot].stack_count);
                    text = count_text;
                }
                g_screen_state_00649f1c->dialogue_text_16c->m_textBuffer.SetText(text,
                                                                                 g_font_683660);
                g_screen_state_00649f1c->dialogue_text_16c->Invalidate(1);
                return &character->backpack[slot];
            }
            ++hit;
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x00574250
void HandleNpcDialogueReply(wchar_t* text, char echo)
{
    wchar_t notice[200];
    int line;

    if (g_screen_state_00649f1c->script_busy != 0) {
        if (g_screen_state_00649f1c->flag_200 == 0) {
            line = FindNpcReplyQuote(text);
            if (line != -1) {
                QueueNpcScriptLine(line, 0, 0, 0);
            }
        } else {
            if (CompareWideTextIgnoreAsciiCase00402920(text, gppStringList[0x1f7c / 4]) == 0) {
                if (static_cast<unsigned int>(g_screen_state_00649f1c->pending_price_204) >
                    g_status_685170.party_gold) {
                    RunNpcScriptLine(0x14, 0);
                    g_screen_state_00649f1c->flag_200 = 0;
                } else {
                    SpendPartyGold(g_screen_state_00649f1c->pending_price_204);
                    if (g_screen_state_00649f1c->flag_201 == 0) {
                        TellNpcFact(g_screen_state_00649f1c->dialogue_npc,
                                    g_screen_state_00649f1c->pending_fact_1fc);
                    }
                    RunNpcScriptLine(g_screen_state_00649f1c->pending_fact_1fc, 0);
                    g_screen_state_00649f1c->flag_200 = 0;
                }
            } else {
                RunNpcQuoteDeclineActions(g_screen_state_00649f1c->pending_fact_1fc);
                g_screen_state_00649f1c->flag_200 = 0;
            }
        }
        if (echo != 0) {
            swprintf(notice, L"%s...", text);
            ShowNotice(0xa, notice, 3, GetTextBoxScrollRange(), 0);
        }
        g_screen_state_00649f1c->script_busy = 0;
    }
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
    wchar_t* out;
    wchar_t ch;
    int quote_id = -1;
    int second_quote_id = -1;
    bool show_fallback = true;
    bool plain_text = true;
    int quote;
    int len;
    int word_count;
    int matches;

    Get16BitStringFromField(0, field_text);
    ClearActiveField();
    StripNpcKeywordPunctuation(field_text);
    if (wcslen(field_text) == 0 && g_screen_state_00649f1c->script_busy == 0) {
        return;
    }
    if (g_screen_state_00649f1c->script_busy != 0) {
        HandleNpcDialogueReply(field_text, 1);
        return;
    }
    word_count = 0;
    cursor = field_text;
    if (cursor != 0) {
        for (;;) {
            len = 0;
            word[0] = 0;
            if (*cursor == L' ') {
                ch = L' ';
                do {
                    if (ch == 0)
                        break;
                    ch = *++cursor;
                } while (ch == L' ');
            }
            ch = *cursor;
            if (ch == L' ')
                break;
            out = word;
            do {
                if (ch == 0)
                    break;
                *out++ = ch;
                ch = *++cursor;
                ++len;
            } while (ch != L' ');
            if (len == 0)
                break;
            word[len] = 0;
            if (cursor == 0)
                break;
            ++word_count;
        }
    }
    if (word_count > 2) {
        show_fallback = false;
    }
    if (_wcsnicmp(field_text, gppStringList[0x1dac / 4], 4) == 0 ||
        _wcsnicmp(field_text, gppStringList[0x1db0 / 4], 7) == 0) {
        RequestNpcJoinParty();
        return;
    }
    if (g_screen_state_00649f1c->flag_1d9 == 1) {
        const wchar_t* fmt;
        if (_wcsnicmp(field_text, gppStringList[0x1db4 / 4], 9) == 0 ||
            _wcsnicmp(field_text, gppStringList[0x1db8 / 4], 0xa) == 0 ||
            _wcsnicmp(field_text, gppStringList[0x1dbc / 4], 8) == 0) {
            fmt = g_format_s_006068e4;
        } else {
            fmt = gppStringList[0x1da8 / 4];
        }
        swprintf(buf, fmt, field_text);
        quote = FindNpcScriptQuoteByKeyword(buf, 0, 0);
        if (quote != -1) {
            plain_text = false;
            quote_id = quote;
            goto found;
        }
    } else {
        swprintf(buf, g_format_s_006068e4, field_text);
        quote = FindNpcScriptQuoteByKeyword(buf, 0, 0);
        if (quote != -1) {
            quote_id = quote;
            goto found;
        }
    }
    wcscpy(buf, field_text);
    if (_wcsnicmp(buf, gppStringList[0x1dc0 / 4], 0xb) != 0) {
        if (_wcsnicmp(buf, gppStringList[0x1db4 / 4], 9) == 0 ||
            _wcsnicmp(buf, gppStringList[0x1db8 / 4], 0xa) == 0 ||
            _wcsnicmp(buf, gppStringList[0x1dbc / 4], 8) == 0) {
            wcscpy(field_text, buf + 9);
            plain_text = false;
            show_fallback = false;
        } else if (g_screen_state_00649f1c->flag_1d9 == 0) {
            plain_text = true;
            goto pair_scan;
        } else {
            plain_text = false;
        }
        quote_id = FindNpcNameOrPlaceQuote(g_screen_state_00649f1c->dialogue_npc, field_text);
        if (quote_id == -1) {
            quote_id = 0x76;
            goto fallback;
        }
        goto found;
    }
    wcscpy(field_text, buf + 0xb);
    show_fallback = false;
pair_scan:
    cursor = field_text;
    word[0] = 0;
    if (cursor != 0) {
        do {
            if (wcslen(word) == 0) {
                len = 0;
                word[0] = 0;
                if (*cursor == L' ') {
                    ch = L' ';
                    do {
                        if (ch == 0)
                            break;
                        ch = *++cursor;
                    } while (ch == L' ');
                }
                ch = *cursor;
                if (ch == L' ')
                    goto multi_scan;
                out = word;
                do {
                    if (ch == 0)
                        break;
                    *out++ = ch;
                    ch = *++cursor;
                    ++len;
                } while (ch != L' ');
                if (len == 0)
                    goto multi_scan;
                word[len] = 0;
            } else {
                wcscpy(word, word2);
            }
            if (cursor == 0)
                goto multi_scan;
            len = 0;
            word2[0] = 0;
            if (*cursor == L' ') {
                ch = L' ';
                do {
                    if (ch == 0)
                        break;
                    ch = *++cursor;
                } while (ch == L' ');
            }
            ch = *cursor;
            if (ch == L' ')
                goto multi_scan;
            out = word2;
            do {
                if (ch == 0)
                    break;
                *out++ = ch;
                ch = *++cursor;
                ++len;
            } while (ch != L' ');
            if (len == 0)
                goto multi_scan;
            word2[len] = 0;
            if (cursor == 0)
                goto multi_scan;
            swprintf(buf, g_format_s_space_s_00617584, word, word2);
            quote = FindNpcScriptQuoteByKeyword(buf, 0, 0);
        } while (quote == -1);
        quote_id = quote;
        goto found;
    }
multi_scan:
    matches = 0;
    cursor = field_text;
    if (cursor == 0)
        goto fallback;
    for (;;) {
        len = 0;
        word[0] = 0;
        if (*cursor == L' ') {
            ch = L' ';
            do {
                if (ch == 0)
                    break;
                ch = *++cursor;
            } while (ch == L' ');
        }
        ch = *cursor;
        if (ch == L' ')
            break;
        out = word;
        do {
            if (ch == 0)
                break;
            *out++ = ch;
            ch = *++cursor;
            ++len;
        } while (ch != L' ');
        if (len == 0)
            break;
        word[len] = 0;
        if (cursor == 0)
            break;
        quote = FindNpcScriptQuoteByKeyword(word, 0, 0);
        if (quote != -1) {
            ++matches;
        }
    }
    if (matches > 2) {
        quote_id = 0x20;
        goto echo;
    }
    if (matches <= 0) {
        goto fallback;
    }
    {
        int found_count = 0;
        cursor = field_text;
        do {
            do {
                len = 0;
                word[0] = 0;
                if (*cursor == L' ') {
                    ch = L' ';
                    do {
                        if (ch == 0)
                            break;
                        ch = *++cursor;
                    } while (ch == L' ');
                }
                ch = *cursor;
                if (ch == L' ')
                    goto found;
                out = word;
                do {
                    if (ch == 0)
                        break;
                    *out++ = ch;
                    ch = *++cursor;
                    ++len;
                } while (ch != L' ');
                if (len == 0)
                    goto found;
                word[len] = 0;
                if (cursor == 0)
                    goto found;
                quote = FindNpcScriptQuoteByKeyword(word, 0, 0);
            } while (quote == -1);
            ++found_count;
            if (found_count == 1) {
                quote_id = quote;
            } else if (found_count == 2 && quote != quote_id) {
                second_quote_id = quote;
            }
        } while (matches != 1);
    }
found:
    if (quote_id >= 0x59 && quote_id < 0x69 && second_quote_id == -1) {
        goto echo;
    }
fallback:
    if (show_fallback) {
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
            text = gppStringList[g_dialogue_fallback_ids_00649f64[roll]];
        } else {
            roll = Random(5);
            if (roll == 3 || roll == 4) {
                fmt = L"%s %s?";
            } else {
                fmt = L"%s %s.";
            }
            text = gppStringList[g_dialogue_fallback_ids_00649f78[roll]];
        }
        swprintf(notice, fmt, text, field_text);
        ShowNotice(0xa, notice, 3, GetTextBoxScrollRange(), 0);
        goto dispatch;
    }
echo:
    ShowNotice(0xa, field_text, 3, GetTextBoxScrollRange(), 0);
dispatch:
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
void HandleNpcDialogueKeyEvent00574BB0(const InputAtom* event)
{
    W8MainScreenState* state;
    int command;
    int line;
    int y;

    if (ShouldDeferCharacterEventForNpcScript(0)) {
        return;
    }
    state = g_screen_state_00649f1c;
    if (state->flag_250 != 0 && event->usParam != 0x1b) {
        return;
    }
    switch (event->usParam) {
    case 0x26:
        if (state->value_fc != W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX) {
            return;
        }
        ScrollTextBoxUp(1);
        state = g_screen_state_00649f1c;
        if (state->value_fc != W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX) {
            return;
        }
        y = (event->uiParam >> 16) & 0xffff;
        if (y < g_level_block->text_box_top || y > g_level_block->text_box_bottom) {
            return;
        }
        line = (y - g_level_block->text_box_top) / 11;
        ClearTextSlot1D8(2);
        if (line < static_cast<int>(g_status_685170.text_box_lines_shown_49a7[2])) {
            SelectTextSlot1D8(g_level_block->text_lines[2] + line, 2);
        }
        RedrawTextBox();
        state->value_1cc = line;
        return;
    case 0x28:
        if (state->value_fc != W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX) {
            return;
        }
        ScrollTextBoxDown(1);
        state = g_screen_state_00649f1c;
        if (state->value_fc != W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX) {
            return;
        }
        y = (event->uiParam >> 16) & 0xffff;
        if (y < g_level_block->text_box_top || y > g_level_block->text_box_bottom) {
            return;
        }
        line = (y - g_level_block->text_box_top) / 11;
        ClearTextSlot1D8(2);
        if (line < static_cast<int>(g_status_685170.text_box_lines_shown_49a7[2])) {
            SelectTextSlot1D8(g_level_block->text_lines[2] + line, 2);
        }
        RedrawTextBox();
        state->value_1cc = line;
        return;
    case 0x1b:
        if (state->dialogue_cursor_flag != 0) {
            SetNpcDialogueHidden(0);
            return;
        }
        if (IsNpcScriptSessionActive()) {
            TryFinishNpcVoicePlayback(1);
            return;
        }
        if (EditingText()) {
            SetInputFieldStringWith16BitString(0, &g_wchar_00689b34);
            ClearActiveField();
            return;
        }
        if (g_screen_state_00649f1c->script_busy != 0 || !IsMessageBoxLineQueueEmpty()) {
            return;
        }
        switch (g_screen_state_00649f1c->value_fc) {
        case W8_DIALOGUE_LAYOUT_TRANSCRIPT:
            g_screen_state_00649f1c->flag_23c = 0;
            g_screen_state_00649f1c->flag_251 = 1;
            QueueNpcScriptLine(0x5c, 0, 0, 0);
            return;
        case W8_DIALOGUE_LAYOUT_TOPIC_MENU:
            EndNpcDialogueSession0056E800(0);
            return;
        case 1: {
            int prev = g_screen_state_00649f1c->value_104;
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
            unsigned char cursor = g_screen_state_00649f1c->flag_229;
            CloseNpcDialogueOptionLayout();
            if (cursor != 0) {
                ShowNpcDialogueTopicMenu();
            } else {
                OpenNpcDialogueTranscriptLayout();
            }
            return;
        }
        case 5:
            RegionSetDisable(0x18);
            RegionSetDisable(0x17);
            g_screen_state_00649f1c->dialogue_text_188->SetEnabled(1);
            g_screen_state_00649f1c->panel_1a8->SetEnabled(0);
            g_screen_state_00649f1c->panel_1ac->SetEnabled(0);
            g_screen_state_00649f1c->panel_1bc->SetEnabled(0);
            g_screen_state_00649f1c->dialogue_text_110->SetEnabled(1);
            g_screen_state_00649f1c->dialogue_text_114->SetEnabled(1);
            g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
            g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
            if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
                SetNpcDialogueHidden(0);
            }
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
        if (command == 0x131) {
            DispatchMGSCommand(command);
            return;
        }
        if (g_screen_state_00649f1c->value_fc == W8_DIALOGUE_LAYOUT_TRANSCRIPT) {
            return;
        }
        if (command <= 0x132) {
            if (command != 0x132 && command != 100) {
                return;
            }
        } else if (command < 400 || command > 0x197) {
            return;
        }
        DispatchMGSCommand(command);
        return;
    }
}

// FUNCTION: WIZ8 0x00574F90
void SetDialogueFieldKeyword(wchar_t* keyword, unsigned char append)
{
    wchar_t field_text[200];
    wchar_t combined[200];

    Get16BitStringFromField(0, field_text);
    StripNpcKeywordPunctuation(keyword);
    if (wcslen(field_text) != 0 && append != 0) {
        swprintf(combined, g_format_s_space_s_00617584, field_text, keyword);
        SetInputFieldStringWith16BitString(0, combined);
    } else {
        SetInputFieldStringWith16BitString(0, keyword);
    }
}

// FUNCTION: WIZ8 0x00575020
bool IsDialoguePlaceKeyword(const wchar_t* name)
{
    for (int index = 0; index < g_dialogue_place_keyword_count; ++index) {
        if (CompareWideTextIgnoreAsciiCase00402920(
                name, gppStringList[g_dialogue_place_keyword_ids[index]]) == 0) {
            return 1;
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x00575070
void ClearNpcDialogueTranscript(void)
{
    for (int index = 0; index < g_screen_state_00649f1c->dialogue_transcript.count; ++index) {
        free(*g_screen_state_00649f1c->dialogue_transcript.GetAt(index));
    }
    g_screen_state_00649f1c->dialogue_transcript.count = 0;
}

// FUNCTION: WIZ8 0x005750D0
unsigned char LoadNpcDialogueTranscript005750D0(unsigned int file)
{
    unsigned char version;
    unsigned int bytes_read;
    int record_count;
    int text_length;
    int index;

    for (index = 0; index < g_screen_state_00649f1c->dialogue_transcript.count; ++index) {
        free(*g_screen_state_00649f1c->dialogue_transcript.GetAt(index));
    }
    g_screen_state_00649f1c->dialogue_transcript.count = 0;
    FileRead(file, &version, 1, &bytes_read);
    FileRead(file, &record_count, 4, &bytes_read);
    for (index = 0; index < record_count; ++index) {
        W8DialogueTranscriptRecord* record =
            static_cast<W8DialogueTranscriptRecord*>(malloc(sizeof(W8DialogueTranscriptRecord)));
        memset(record, 0, sizeof(*record));
        FileRead(file, &text_length, 4, &bytes_read);
        FileRead(file, record, text_length * 2 + 2, &bytes_read);
        FileRead(file, &record->category, 1, &bytes_read);
        g_screen_state_00649f1c->dialogue_transcript.Add(record);
    }
    if (version > 1) {
        FileRead(file, &g_screen_state_00649f1c->dialogue_category_filter, 1, &bytes_read);
        FileRead(file, &g_screen_state_00649f1c->transcript_sorted, 1, &bytes_read);
    }
    return 1;
}

// FUNCTION: WIZ8 0x00575290
unsigned char SaveNpcDialogueTranscript00575290(unsigned int file)
{
    unsigned char version;
    unsigned int bytes_written;
    size_t text_length;
    int index;

    version = 2;
    FileWrite(file, &version, 1, &bytes_written);
    text_length = g_screen_state_00649f1c->dialogue_transcript.count;
    FileWrite(file, &text_length, 4, &bytes_written);
    for (index = 0; index < g_screen_state_00649f1c->dialogue_transcript.count; ++index) {
        W8DialogueTranscriptRecord* record =
            *g_screen_state_00649f1c->dialogue_transcript.GetAt(index);
        text_length = wcslen(record->text);
        FileWrite(file, &text_length, 4, &bytes_written);
        FileWrite(file, record, text_length * 2 + 2, &bytes_written);
        FileWrite(file, &record->category, 1, &bytes_written);
    }
    FileWrite(file, &g_screen_state_00649f1c->dialogue_category_filter, 1, &bytes_written);
    FileWrite(file, &g_screen_state_00649f1c->transcript_sorted, 1, &bytes_written);
    return 1;
}

/* Restate the five transcript category buttons so only the active
   dialogue_category_filter's button shows its secondary state. */
// FUNCTION: WIZ8 0x00575390
void SyncDialogueCategoryButtons(void)
{
    switch (g_screen_state_00649f1c->dialogue_category_filter) {
    case W8_DIALOGUE_CATEGORY_ITEMS:
        g_screen_state_00649f1c->dialogue_people_button->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_places_button->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_items_button->EnableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_misc_button->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_all_button->DisableSecondaryState(1);
        break;
    case W8_DIALOGUE_CATEGORY_PEOPLE:
        g_screen_state_00649f1c->dialogue_people_button->EnableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_places_button->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_items_button->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_misc_button->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_all_button->DisableSecondaryState(1);
        break;
    case W8_DIALOGUE_CATEGORY_PLACES:
        g_screen_state_00649f1c->dialogue_people_button->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_places_button->EnableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_items_button->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_misc_button->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_all_button->DisableSecondaryState(1);
        break;
    case W8_DIALOGUE_CATEGORY_MISC:
        g_screen_state_00649f1c->dialogue_people_button->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_places_button->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_items_button->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_misc_button->EnableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_all_button->DisableSecondaryState(1);
        break;
    case W8_DIALOGUE_CATEGORY_ALL:
        g_screen_state_00649f1c->dialogue_people_button->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_places_button->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_items_button->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_misc_button->DisableSecondaryState(1);
        g_screen_state_00649f1c->dialogue_all_button->EnableSecondaryState(1);
        break;
    default:
        break;
    }
}

/* A confirmed purchase returns the NPC dialogue to the layout appropriate
   for the NPC's disposition. A cancelled dialog leaves the layout alone. */
// FUNCTION: WIZ8 0x00575520
void OnNpcTradeDialogClosed00575520(W8DialogBase* dialog)
{
    if (GetDialogResult(dialog) == 0) {
        return;
    }
    ConfirmNpcTradePurchase00575710();
    switch (g_screen_state_00649f1c->value_fc) {
    case 1:
        CloseNpcDialogueMode1Layout();
        break;
    case 2:
        RegionSetDisable(0x18);
        g_screen_state_00649f1c->panel_1a8->SetEnabled(0);
        g_screen_state_00649f1c->panel_1ac->SetEnabled(0);
        g_screen_state_00649f1c->panel_1b8->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_1a4->SetActive(0);
        g_screen_state_00649f1c->dialogue_text_1a4->m_textBuffer.SetText(
            &g_wchar_00689b34, g_wiz_text_bold_font_683664);
        g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
        g_screen_state_00649f1c->value_fc = 0;
        if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
            SetNpcDialogueHidden(0);
        }
        break;
    case 3:
        CloseNpcDialogueTranscriptLayout();
        break;
    case 4:
        CloseNpcDialogueOptionLayout();
        break;
    case 5:
        RegionSetDisable(0x18);
        RegionSetDisable(0x17);
        g_screen_state_00649f1c->dialogue_text_188->SetEnabled(1);
        g_screen_state_00649f1c->panel_1a8->SetEnabled(0);
        g_screen_state_00649f1c->panel_1ac->SetEnabled(0);
        g_screen_state_00649f1c->panel_1bc->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_110->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_text_114->SetEnabled(1);
    case 6:
        g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
        g_screen_state_00649f1c->value_fc = 0;
        if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
            SetNpcDialogueHidden(0);
        }
        break;
    }
    if (GetNpcDispositionBand(g_screen_state_00649f1c->dialogue_npc) == 0) {
        OpenNpcDialogueTranscriptLayout();
    } else {
        ShowNpcDialogueTopicMenu();
    }
}

// FUNCTION: WIZ8 0x00575710
void ConfirmNpcTradePurchase00575710(void)
{
    if (GetNpcDispositionBand(g_screen_state_00649f1c->dialogue_npc) == 0) {
        SpendPartyGold(g_screen_state_00649f1c->value_22c);
        g_screen_state_00649f1c->dialogue_text_16c->Invalidate(1);
        QueueNpcScriptLine(0x10, 0, 0, 0);
        RebuildNpcTradeItemList005ADB10(0);
        return;
    }
    if (NpcRecordHasValue002(g_screen_state_00649f1c->dialogue_npc) != 0) {
        ApplyNpcInteraction0050A570(g_screen_state_00649f1c->dialogue_npc, 2,
                                    g_screen_state_00649f1c->dialogue_speaker, 0,
                                    g_screen_state_00649f1c->value_22c);
        g_screen_state_00649f1c->dialogue_text_16c->Invalidate(1);
        QueueNpcScriptLine(
            GetNpcDispositionBand(g_screen_state_00649f1c->dialogue_npc) == 0 ? 0x10 : 7, 0, 0, 0);
        RebuildNpcTradeItemList005ADB10(0);
        return;
    }
    QueueNpcScriptLine(7, 0, 0, 0);
}

// FUNCTION: WIZ8 0x00575810
unsigned char HandleNpcDialogueItem(W8ItemInstance* item)
{
    W8MessageDialogBase* dialog;
    wchar_t* message;
    unsigned char result;
    unsigned char flag;
    int fact_result;

    result = 1;
    if (g_screen_state_00649f1c->value_22c != 0 && item == 0) {
        if (g_screen_state_00649f1c->value_22c != (int)g_status_685170.party_gold) {
            ConfirmNpcTradePurchase00575710();
            return 1;
        }
        dialog = static_cast<W8MessageDialogBase*>(CreateDialogByKind(1));
        dialog->SetClientExtent(0xfa, 200);
        message = FormatWideString(gppStringList[0x1f58 / 4]);
        dialog->SetMessage(message, 1, 0x32, 1, 1, 1, 1, 0, 0x15e);
        SetDialogDestroyCallback(dialog, OnNpcTradeDialogClosed00575520);
        OpenModal(dialog);
        return 1;
    }
    if (GetNpcDispositionBand(g_screen_state_00649f1c->dialogue_npc) == 0 ||
        g_screen_state_00649f1c->dialogue_npc->record->flag_2ea != 0) {
        if (item != 0) {
            fact_result = FindNpcScriptItemQuote(item->item_id, 0, &flag);
            if (fact_result == -1) {
                if (g_screen_state_00649f1c->dialogue_npc->record->flag_2ea != 0) {
                    return 1;
                }
                if ((g_item_records[item->item_id].flags_041 & 2) != 0) {
                    goto unavailable;
                }
                if (AcceptNpcDialogueItem005B1740(g_screen_state_00649f1c->dialogue_npc, item, 1) !=
                    0) {
                    QueueNpcScriptLine(0x10, 0, 0, 0);
                    goto remove;
                }
                QueueNpcScriptLine(0x11, 0, 0, 0);
            } else {
                QueueNpcScriptLine(fact_result, 0, 0, 0);
                result = 0;
                if (flag != 0) {
                remove:
                    RemoveNpcScriptItem(item, 0, -1);
                    return result;
                }
                if (g_screen_state_00649f1c->flag_1f9 != 0 &&
                    g_screen_state_00649f1c->pending_item_1ed.item_id == item->item_id) {
                    g_screen_state_00649f1c->flag_1f9 = 0;
                    AddItemToParty(&g_screen_state_00649f1c->pending_item_1ed, 1, 0);
                    ClearHeldItemDisplay();
                    return 0;
                }
            }
        }
    } else if (item != 0) {
        if ((g_item_records[item->item_id].flags_041 & 2) == 0) {
            if (WillNpcTradeForItem(g_screen_state_00649f1c->dialogue_npc, item) == 0) {
                QueueNpcScriptLine(7, 0, 0, 0);
                return 1;
            }
            fact_result = FindNpcScriptItemQuote(item->item_id, 0, &flag);
            if (fact_result == -1) {
                ApplyNpcInteraction0050A570(g_screen_state_00649f1c->dialogue_npc, 3,
                                            g_screen_state_00649f1c->dialogue_speaker, item, 0);
                QueueNpcScriptLine(
                    GetNpcDispositionBand(g_screen_state_00649f1c->dialogue_npc) == 0 ? 0x10 : 7, 0,
                    0, 0);
            } else {
                QueueNpcScriptLine(fact_result, 0, 0, 0);
                result = 0;
            }
            RemoveNpcScriptItem(item, 0, -1);
            return result;
        }
    unavailable:
        QueueNpcScriptLine(0x11, 0, 0, 0);
        return 1;
    }
    return result;
}

// FUNCTION: WIZ8 0x005b1740 FOLDED
unsigned char AcceptNpcDialogueItem005B1740(W8NpcState*, W8ItemInstance*, int)
{
    return 1;
}

/* While NPC script deferral holds character events during an open dialogue,
   pump Escape and left-click so layout dismissals still run. */
// FUNCTION: WIZ8 0x00575B00
void SubmitNpcDialogueInput00575B00(void)
{
    g_screen_state_00649f1c->flag_1d9 = 1;
    g_screen_state_00649f1c->text_input_panel_1c0->Invalidate(0);
    HandleNpcDialogueInput();
    g_screen_state_00649f1c->flag_1d9 = 0;
}

// FUNCTION: WIZ8 0x00575B40
void SubmitNpcDialogueInput00575B40(void)
{
    g_screen_state_00649f1c->flag_1d9 = 0;
    g_screen_state_00649f1c->text_input_panel_1c0->Invalidate(0);
    HandleNpcDialogueInput();
}

// FUNCTION: WIZ8 0x00575B70
void HandleNpcDialogueItemChoice00575B70(void)
{
    HandleNpcDialogueItem(g_screen_state_00649f1c->value_108);
    if (g_screen_state_00649f1c->flag_229 != 0 &&
        GetNpcDispositionBand(g_screen_state_00649f1c->dialogue_npc) == 0) {
        CloseNpcDialogueOptionLayout();
        OpenNpcDialogueTranscriptLayout();
        HandleNpcDialogueDeparture(0);
    }
}

// FUNCTION: WIZ8 0x00575BC0
void RefreshNpcTradePartyGold00575BC0(void)
{
    wchar_t text[32];

    swprintf(text, L"%dg", g_status_685170.party_gold);
    g_screen_state_00649f1c->dialogue_text_190->m_textBuffer.SetText(text, g_font_683660);
}

// FUNCTION: WIZ8 0x00575C00
void RefreshNpcTradePrice00575C00(void)
{
    wchar_t text[32];

    if (g_screen_state_00649f1c->value_108 != 0) {
        swprintf(text, g_format_d_0060aa20, g_screen_state_00649f1c->value_254);
        g_screen_state_00649f1c->dialogue_text_16c->m_textBuffer.SetText(text, g_font_683660);
    }
}

// FUNCTION: WIZ8 0x00575C50
void DrainNpcDialogueDeferralInput(void)
{
    POINT mouse;
    InputAtom input;
    int prior_layout;
    char reopen_topics;

    if (ShouldDeferCharacterEventForNpcScript(1) == 0 || gXStatus.fNpcDialogueMode == 0) {
        return;
    }
    SGPMouseGetPos(&mouse);
    MSYS_SGP_Mouse_Handler_Hook(MOUSE_POS, static_cast<unsigned short>(mouse.x),
                                static_cast<unsigned short>(mouse.y), gfLeftButtonState,
                                gfRightButtonState);
    while (DequeueEvent(&input) == 1) {
        if (input.usEvent == KEY_DOWN) {
            if (input.usParam == 0x1b) {
                if (g_screen_state_00649f1c->dialogue_cursor_flag == 0) {
                    if (IsNpcScriptSessionActive() == 0) {
                        switch (g_screen_state_00649f1c->value_fc) {
                        case 2:
                        case 3:
                            EndNpcDialogueSession0056E800(0);
                            break;
                        case 1:
                            prior_layout = g_screen_state_00649f1c->value_104;
                            CloseNpcDialogueMode1Layout();
                            if (prior_layout == 2) {
                                ShowNpcDialogueTopicMenu();
                            } else if (prior_layout == 3) {
                                OpenNpcDialogueTranscriptLayout();
                            }
                            break;
                        case 4:
                            reopen_topics = g_screen_state_00649f1c->flag_229;
                            CloseNpcDialogueOptionLayout();
                            if (reopen_topics != 0) {
                                ShowNpcDialogueTopicMenu();
                            } else {
                                OpenNpcDialogueTranscriptLayout();
                            }
                            break;
                        case 5:
                            RegionSetDisable(0x18);
                            RegionSetDisable(0x17);
                            g_screen_state_00649f1c->dialogue_text_188->SetEnabled(1);
                            g_screen_state_00649f1c->panel_1a8->SetEnabled(0);
                            g_screen_state_00649f1c->panel_1ac->SetEnabled(0);
                            g_screen_state_00649f1c->panel_1bc->SetEnabled(0);
                            g_screen_state_00649f1c->dialogue_text_110->SetEnabled(1);
                            g_screen_state_00649f1c->dialogue_text_114->SetEnabled(1);
                            g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
                            g_screen_state_00649f1c->value_fc = 0;
                            if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
                                SetNpcDialogueHidden(0);
                            }
                            OpenNpcDialogueTranscriptLayout();
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
void OpenNpcDialog(W8NpcDialogRequest* request, int aux_data)
{
    W8MonsterInfo* monster_info;
    W8NpcDialog* dialog;

    if (gXStatus.fNpcDialogueMode == 0 || g_screen_state_00649f1c->flag_252 != 0) {
        InitTextInputMode();
    } else {
        SetNpcDialoguePanelVisible(0);
    }
    dialog = new W8NpcDialog(request, aux_data);
    dialog->SetText(&g_wchar_00689b34);
    dialog->m_destroy_callback = OnNpcDialogClosed;
    OpenModal(dialog);
    g_screen_state_00649f1c->script_busy = 1;
    if (request->opcode == 0x12 || request->opcode == 0x1e) {
        g_screen_state_00649f1c->pending_fact_1fc = aux_data;
        g_screen_state_00649f1c->flag_200 = 1;
        g_screen_state_00649f1c->pending_price_204 = request->base_price;
        monster_info = GetNpcMonsterInfo(g_screen_state_00649f1c->dialogue_npc);
        if (monster_info != 0) {
            g_screen_state_00649f1c->pending_price_204 -= static_cast<int>(
                monster_info->effect_2de * 0.01f * g_screen_state_00649f1c->pending_price_204);
        }
        g_screen_state_00649f1c->pending_price_204 -=
            GetBestPartySkillLevel(0x16, 0) * g_screen_state_00649f1c->pending_price_204 / 500;
        if (g_screen_state_00649f1c->pending_price_204 < 1) {
            g_screen_state_00649f1c->pending_price_204 = 1;
        }
        if (g_screen_state_00649f1c->pending_price_204 > 0x1e) {
            g_screen_state_00649f1c->pending_price_204 =
                (g_screen_state_00649f1c->pending_price_204 * 10 + 9) / 10;
        }
        if (request->opcode == 0x1e) {
            g_screen_state_00649f1c->flag_201 = 1;
        }
    }
    SetTargetCursor(-1);
}

// FUNCTION: WIZ8 0x00576030
void SetNpcQuoteBubbleVisible(bool visible, const wchar_t* text, W8NpcScriptQuote* quote,
                              int quote_id, unsigned int font_palette)
{
    SetNpcQuoteBubbleVisible(visible, text, quote, quote_id, font_palette, 0, 0, -1);
}

// FUNCTION: WIZ8 0x00576060
void SetNpcQuoteBubbleVisible(bool visible, const wchar_t* text, W8NpcScriptQuote* quote,
                              int quote_id, unsigned int font_palette, unsigned char notice_kind,
                              void* payload, int npc_kind)
{
    if (visible == g_screen_state_00649f1c->quote_visible) {
        return;
    }
    if (visible) {
        wchar_t normalized[2048];
        wchar_t error_text[200];
        unsigned short width;
        unsigned short height;

        if (g_flag_0068edd8) {
            SetFlag603C60();
            g_flag_0068edd8 = 0;
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
        g_screen_state_00649f1c->quote_bubble = LayoutPortraitQuoteBubble(
            -1, 0, 0, normalized, 280, 0, 0, 0, &width, &height, font_palette);
        if (g_screen_state_00649f1c->quote_bubble == -1) {
            if (g_screen_state_00649f1c->dialogue_npc != 0) {
                swprintf(error_text,
                         L"Error creating box - most likely text too large: NPC %s, quote %d",
                         g_screen_state_00649f1c->dialogue_npc->record->source_name_004, quote_id);
            } else {
                swprintf(error_text, L"Error creating box - most likely text too large");
            }
            g_screen_state_00649f1c->quote_bubble =
                LayoutPortraitQuoteBubble(-1, 0, 0, error_text, 300, 0, 0, 0, &width, &height, -1);
            ShowNotice(0xc, error_text, 0, GetTextBoxScrollRange(), 0);
        }
        g_screen_state_00649f1c->quote_width = width;
        g_screen_state_00649f1c->quote_height = height;
        g_screen_state_00649f1c->quote_x = 320 - (width >> 1);
        g_screen_state_00649f1c->quote_y = quote_id < 0 ? 350 - height : 20;
        SetRegionBounds(0x136, g_screen_state_00649f1c->quote_x, g_screen_state_00649f1c->quote_y,
                        g_screen_state_00649f1c->quote_x + g_screen_state_00649f1c->quote_width,
                        g_screen_state_00649f1c->quote_y + g_screen_state_00649f1c->quote_height);
        RegionSetEnable(0x25);
        EnableRegionSetInput(0x25);
        g_screen_state_00649f1c->quote_visible = true;
        if (notice_kind == 0) {
            W8PendingNoticeLine* line = new W8PendingNoticeLine;
            line->text = static_cast<wchar_t*>(malloc((wcslen(normalized) + 1) * sizeof(wchar_t)));
            line->npc_kind = npc_kind;
            wcscpy(line->text, normalized);
            g_screen_state_00649f1c->pending_notice_lines.Add(line);
        }
        g_screen_state_00649f1c->quote_notice_kind = notice_kind;
        g_screen_state_00649f1c->quote_notice_payload.raw = payload;
        if (g_screen_state_00649f1c->quote_notice_kind == 3) {
            SoundPlay(reinterpret_cast<STR>(const_cast<char*>( // reinterpret-ok: SGP text ABI
                          "Data\\Sound\\Misc\\GainLevel.wav")),
                      0);
        }
        return;
    }

    bool flush_notices =
        (quote_id == 0x12 || quote_id < 0) && g_screen_state_00649f1c->quote_notice_kind == 0;
    switch (g_screen_state_00649f1c->quote_notice_kind) {
    case 1: {
        W8ExperienceNoticePayload* experience =
            g_screen_state_00649f1c->quote_notice_payload.experience;
        FormatNotice(0xc, 0, gppStringList[experience->alternate_message ? 0x231 : 0x232],
                     experience->amount);
        delete experience;
        break;
    }
    case 2: {
        W8SkillNoticePayload* skills = g_screen_state_00649f1c->quote_notice_payload.skills;
        for (int index = 0; index < skills->count; ++index) {
            int slot = skills->party_slots[index];
            int skill = skills->skills[index];
            W8Character* character = &g_status_685170.buffers.characters[slot];
            unsigned int value = character->skills[skill].value_02;
            if (skill == g_profession_bonus_skills[character->current_profession]) {
                value = value * 125 / 100;
            }
            PostCharacterNotice(slot, gppStringList[0x1d9],
                                gppStringList[g_character_skill_name_ids_61e454[skill]], value);
        }
        delete skills;
        break;
    }
    case 3: {
        int* slot = g_screen_state_00649f1c->quote_notice_payload.level_up_slot;
        PostCharacterNotice(*slot, gppStringList[0x773]);
        delete slot;
        break;
    }
    }
    if (quote != 0) {
        for (unsigned int index = 0; index < quote->entry_count; ++index) {
            if (quote->entries[index].kind_00 == 0x13 || quote->entries[index].kind_00 == 5) {
                flush_notices = true;
            }
        }
    }
    if (flush_notices) {
        FlushPendingNoticeLines005766B0();
    }
    g_screen_state_00649f1c->quote_visible = false;
    if (g_screen_state_00649f1c->quote_bubble != -1) {
        ReleasePortraitQuoteBubble(g_screen_state_00649f1c->quote_bubble);
    }
    RegionSetDisable(0x25);
    DisableRegionSetInput(0x25);
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
        if (g_screen_state_00649f1c->quote_bubble != -1) {
            ClearSurfaceRect(
                g_screen_state_00649f1c->quote_x, g_screen_state_00649f1c->quote_y,
                g_screen_state_00649f1c->quote_x + g_screen_state_00649f1c->quote_width,
                g_screen_state_00649f1c->quote_y + g_screen_state_00649f1c->quote_height);
            InvalidateRegion(
                g_screen_state_00649f1c->quote_x, g_screen_state_00649f1c->quote_y,
                g_screen_state_00649f1c->quote_x + g_screen_state_00649f1c->quote_width,
                g_screen_state_00649f1c->quote_y + g_screen_state_00649f1c->quote_height, 0);
        }
        RequestRedraw(0x200);
        RequestRedraw(0xff);
        RequestRedraw(0x8000);
    } else if (g_current_screen_state.id == W8_SCREEN_CAMP) {
        g_camp_screen_0069c0f4->redraw_flags |= 0x0fffffff;
    }
    g_screen_state_00649f1c->quote_bubble = -1;
}

// FUNCTION: WIZ8 0x00576650
unsigned char NpcQuoteBubbleRegionEvent(const InputAtom* event)
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
    if (g_screen_state_00649f1c->quote_visible) {
        DrawPortraitQuoteBubble(g_screen_state_00649f1c->quote_bubble,
                                g_screen_state_00649f1c->quote_x, g_screen_state_00649f1c->quote_y,
                                -14);
    }
}

// FUNCTION: WIZ8 0x005766B0
void FlushPendingNoticeLines005766B0(void)
{
    wchar_t npc_name[100];
    int index;

    for (index = 0; index < g_screen_state_00649f1c->pending_notice_lines.GetCount(); ++index) {
        W8PendingNoticeLine* line = *g_screen_state_00649f1c->pending_notice_lines.GetAt(index);
        if (line->npc_kind != -1 &&
            line->npc_kind != g_screen_state_00649f1c->last_notice_npc_kind) {
            W8NpcState* npc = GetNpcState(line->npc_kind);
            if (npc != 0) {
                swprintf(npc_name, L"%s", npc->record->source_name_004);
                ShowNotice(1, npc_name, 3, -1, 0);
                g_screen_state_00649f1c->last_notice_npc_kind = line->npc_kind;
            }
        }
        ShowNotice(line->npc_kind == -1 ? 0xb : 0xf, line->text, 3, GetTextBoxScrollRange(), 0);
    }
    while (g_screen_state_00649f1c->pending_notice_lines.GetCount() > 0) {
        W8PendingNoticeLine* line = g_screen_state_00649f1c->pending_notice_lines.RemoveAt(0);
        free(line->text);
        delete line;
    }
}

// FUNCTION: WIZ8 0x005767f0
void LookAtDialogueNpc(void)
{
    W8MonsterInfo* info = GetNpcMonsterInfo(g_screen_state_00649f1c->dialogue_npc);
    if (info != 0) {
        srVector3T<float> position = info->monster->movement_0c0.position_040;
        position.y += info->monster->movement_0c0.height_offset_0b8;
        g_gd_camera_65a0f8->LookAt(&position, 0);
    }
}

/* Show (0) or hide (nonzero) the NPC dialogue UI: on show the party portrait
   region sets lose input, the dialogue regions and text controls come up, and
   the transcript expands; on hide the party sets come back, the transcript
   collapses, its background is cleared and the rectangle invalidated. The new
   state lands in dialogue_cursor_flag. */
// FUNCTION: WIZ8 0x00576850
void SetNpcDialogueHidden(char value)
{
    int index;

    if (value != 0) {
        g_screen_state_00649f1c->saved_mode_230 = g_settings_6850c8.main_ui_mode;
        ApplyMainGameModeFlag(W8_MAIN_UI_MODE_PORTRAITS, 0);
        g_screen_state_00649f1c->npc_dialogue_controller_1b0->SetEnabled(0);
        CollapseNpcDialogueTextArea(g_screen_state_00649f1c->npc_dialogue_controller_1b0);
        g_screen_state_00649f1c->npc_dialogue_panel_1b4->SetEnabled(0);
        for (index = 0; index < 8; ++index) {
            if (g_status_685170.buffers.party_rows[index].occupied != 0) {
                RegionSetEnable(index + 7);
                EnableRegionSetInput(index + 7);
            }
        }
        RegionSetDisable(0x16);
        ClearNpcDialogueTextBackground(g_screen_state_00649f1c->npc_dialogue_controller_1b0);
        ClearSurfaceRect(0x1dc, 0x11b, 0x269, 0x1c2);
        InvalidateRegion(0x1dc, 0x11b, 0x269, 0x1c2, 0);
        RequestRedraw(2);
        RequestRedraw(8);
        RequestRedraw(0x20);
        RequestRedraw(0x80);
        SetInputFieldBlocksMouseCallback(0, 1);
        g_screen_state_00649f1c->dialogue_text_110->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_114->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_118->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_11c->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_120->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_124->SetEnabled(0);
    } else {
        ApplyMainGameModeFlag(g_screen_state_00649f1c->saved_mode_230, 0);
        for (index = 0; index < 8; ++index) {
            if (g_status_685170.buffers.party_rows[index].occupied != 0) {
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
        g_screen_state_00649f1c->npc_dialogue_controller_1b0->SetEnabled(1);
        g_screen_state_00649f1c->npc_dialogue_panel_1b4->SetEnabled(1);
        ExpandNpcDialogueTextArea(g_screen_state_00649f1c->npc_dialogue_controller_1b0);
        if (IsNpcDialogueTextExpanded(g_screen_state_00649f1c->npc_dialogue_controller_1b0) != 0) {
            g_screen_state_00649f1c->dialogue_scroll_up_button->SetEnabled(1);
            g_screen_state_00649f1c->dialogue_scroll_down_button->SetEnabled(1);
        } else {
            g_screen_state_00649f1c->dialogue_scroll_up_button->SetEnabled(0);
            g_screen_state_00649f1c->dialogue_scroll_down_button->SetEnabled(0);
        }
        SetInputFieldBlocksMouseCallback(0, 0);
        g_screen_state_00649f1c->dialogue_text_110->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_text_114->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_text_118->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_text_11c->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_text_120->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_text_124->SetEnabled(1);
    }
    RequestRedraw(0x200);
    g_screen_state_00649f1c->dialogue_cursor_flag = value;
}

// FUNCTION: WIZ8 0x00576b80
void CloseNpcDialogueIfActive(void)
{
    if (gXStatus.fNpcDialogueMode != 0) {
        EndNpcDialogueSession0056E800(0);
    }
}

/* Destroy callback OpenNpcDialog installs on the modal: pops or re-schemes the
   text-input level the dialog pushed, restores the quote bubble, and routes the
   choice back to the script - the picked option's label, the price-check
   yes/no string, or the typed keyword text. Inside a live dialogue the text is
   injected into input field 0 and processed as if typed; otherwise it is
   submitted to the script line queue directly. */
// FUNCTION: WIZ8 0x00576BA0
void ResolveNpcPickpocket00576BA0(int party_slot)
{
    W8Character* character = &g_status_685170.buffers.characters[party_slot];
    unsigned int gold;
    W8ItemInstance item;
    wchar_t text[200];
    int slot;

    switch (AttemptNpcPickpocket0050BC90(character, g_screen_state_00649f1c->dialogue_npc, &item,
                                         &gold)) {
    case 0:
        swprintf(text, gppStringList[0x74d], character->name, GetItemDisplayName(&item));
        for (slot = 0; slot < 8; ++slot) {
            if (character->backpack[slot].item_id == -1) {
                AddItemToCharacter(character, &item, 0, 0, 0);
                DisplayNpcQuote00529570(text, 1);
                return;
            }
        }
        AddItemToParty(&item, 0, 0);
        DisplayNpcQuote00529570(text, 1);
        return;
    case 1:
        swprintf(text, gppStringList[0x74e], character->name, gold);
        AddPartyGold(gold, 0);
        DisplayNpcQuote00529570(text, 1);
        return;
    case 2:
        swprintf(text, gppStringList[0x74f], character->name);
        DisplayNpcQuote00529570(text, 0);
        return;
    case 3:
        QueueNpcScriptLine(0x17, 0, 0, 0);
        SetNpcDispositionBand(g_screen_state_00649f1c->dialogue_npc, 1);
        ApplyFactionChange(3, 1, g_screen_state_00649f1c->dialogue_npc->record->faction_5f, -5);
        CloseNpcDialogueTranscriptLayout();
        ShowNpcDialogueTopicMenu();
        return;
    case 4:
        DisplayNpcQuote00529570(gppStringList[0x750], 0);
        return;
    default:
        return;
    }
}

// FUNCTION: WIZ8 0x00576DA0
void QueueDialogueNpcRefusal00576DA0(void)
{
    unsigned int flags = g_screen_state_00649f1c->dialogue_npc->refusal_flags_85;
    if ((flags & 1) == 0) {
        QueueNpcScriptLine(0x67, 0, 0, 0);
        g_screen_state_00649f1c->dialogue_npc->refusal_flags_85 |= 1;
        return;
    }
    if ((flags & 2) == 0) {
        QueueNpcScriptLine(0x68, 0, 0, 0);
        g_screen_state_00649f1c->dialogue_npc->refusal_flags_85 |= 2;
        return;
    }
    QueueNpcScriptLine(0x69, 0, 0, 0);
}

// FUNCTION: WIZ8 0x00576E20
void OnNpcDialogClosed(W8DialogBase* dialog)
{
    W8NpcDialog* npc_dialog = static_cast<W8NpcDialog*>(dialog);
    W8NpcDialogRequest* request = npc_dialog->m_message;
    wchar_t field_text[200];
    wchar_t entry_text[1020];
    int index;

    if (gXStatus.fNpcDialogueMode == 0 || g_screen_state_00649f1c->flag_252 != 0) {
        KillTextInputMode();
    } else {
        SetTextInputScheme(1);
    }
    RestoreCurrentNpcQuoteBubble();
    if (g_screen_state_00649f1c->value_fc == W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX) {
        CloseNpcDialogueOptionLayout();
        OpenNpcDialogueTranscriptLayout();
    }
    if (request->opcode == 5) {
        for (index = 0; index < request->option_count; ++index) {
            if (index == npc_dialog->m_selected_option) {
                swprintf(entry_text, L"%S", request->options[index].text);
                if (gXStatus.fNpcDialogueMode == 0 || g_screen_state_00649f1c->flag_252 != 0) {
                    HandleNpcDialogueReply(entry_text, 0);
                } else {
                    Get16BitStringFromField(0, field_text);
                    StripNpcKeywordPunctuation(entry_text);
                    goto inject;
                }
                goto done;
            }
        }
        goto done;
    }
    if (request->opcode == 0x12 || request->opcode == 0x1e) {
        if (npc_dialog->m_selected_option == 0) {
            wcscpy(entry_text, gppStringList[0x1f7c / 4]);
        } else {
            wcscpy(entry_text, gppStringList[0x1f80 / 4]);
        }
        if (gXStatus.fNpcDialogueMode == 0 || g_screen_state_00649f1c->flag_252 != 0) {
            HandleNpcDialogueReply(entry_text, 0);
        } else {
            Get16BitStringFromField(0, field_text);
            StripNpcKeywordPunctuation(entry_text);
        inject:
            static_cast<void>(wcslen(field_text));
            SetInputFieldStringWith16BitString(0, entry_text);
            HandleNpcDialogueInput();
        }
    } else if (request->opcode == 0x13) {
        if (gXStatus.fNpcDialogueMode == 0 || g_screen_state_00649f1c->flag_252 != 0) {
            HandleNpcDialogueReply(npc_dialog->m_input_text, 0);
        } else {
            Get16BitStringFromField(0, field_text);
            StripNpcKeywordPunctuation(npc_dialog->m_input_text);
            static_cast<void>(wcslen(field_text));
            SetInputFieldStringWith16BitString(0, npc_dialog->m_input_text);
            HandleNpcDialogueInput();
        }
    }
done:
    g_screen_state_00649f1c->script_busy = 0;
}

/* The camp-side mirror of SwitchNpcDialogueLayout: camp mode is raised, the current
   dialogue layout is retired, and a still-pending item goes back onto the
   item cursor. */
// FUNCTION: WIZ8 0x00577020
void CloseNpcDialogueForCamp(void)
{
    gXStatus.fCampMode = 1;
    switch (g_screen_state_00649f1c->value_fc) {
    case 1:
        CloseNpcDialogueMode1Layout();
        break;
    case 2:
        RegionSetDisable(0x18);
        g_screen_state_00649f1c->panel_1a8->SetEnabled(0);
        g_screen_state_00649f1c->panel_1ac->SetEnabled(0);
        g_screen_state_00649f1c->panel_1b8->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_1a4->SetActive(0);
        g_screen_state_00649f1c->dialogue_text_1a4->m_textBuffer.SetText(
            &g_wchar_00689b34, g_wiz_text_bold_font_683664);
        g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
        g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
        if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
            SetNpcDialogueHidden(0);
        }
        break;
    case 3:
        CloseNpcDialogueTranscriptLayout();
        break;
    case 4:
        CloseNpcDialogueOptionLayout();
        break;
    case 5:
        RegionSetDisable(0x18);
        RegionSetDisable(0x17);
        g_screen_state_00649f1c->dialogue_text_188->SetEnabled(1);
        g_screen_state_00649f1c->panel_1a8->SetEnabled(0);
        g_screen_state_00649f1c->panel_1ac->SetEnabled(0);
        g_screen_state_00649f1c->panel_1bc->SetEnabled(0);
        g_screen_state_00649f1c->dialogue_text_110->SetEnabled(1);
        g_screen_state_00649f1c->dialogue_text_114->SetEnabled(1);
        g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
        g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
        if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
            SetNpcDialogueHidden(0);
        }
        break;
    case 6:
        g_screen_state_00649f1c->value_104 = g_screen_state_00649f1c->value_fc;
        g_screen_state_00649f1c->value_fc = W8_DIALOGUE_LAYOUT_NONE;
        if (g_screen_state_00649f1c->dialogue_cursor_flag != 0) {
            SetNpcDialogueHidden(0);
        }
        break;
    }
    EndNpcDialogueSession0056E800(0);
    if (g_screen_state_00649f1c->flag_1f9 != 0) {
        g_status_685170.item_in_hand_235b = g_screen_state_00649f1c->pending_item_1ed;
        SetItemCursor(0);
        return;
    }
    SetTargetCursor(-1);
}

// FUNCTION: WIZ8 0x00577220
void SyncDialogueNpcStateAndMarkPending00577220(void)
{
    BeginNpcDialogueInternal(g_screen_state_00649f1c->dialogue_npc, 0, -1, 0, 1);
    g_screen_state_00649f1c->value_238 = g_screen_state_00649f1c->value_104;
    g_screen_state_00649f1c->flag_234 = 1;
}

// FUNCTION: WIZ8 0x00577260
void SyncDialogueNpcState00577260(void)
{
    BeginNpcDialogueInternal(g_screen_state_00649f1c->dialogue_npc, 0, -1, 0, 1);
    g_screen_state_00649f1c->value_238 = g_screen_state_00649f1c->value_104;
}

/* The dialogue NPC takes the guard script when it is '*' styled and the party
   walks away; otherwise its record flags drive either a combat notice or the
   queued scripted action named by the record. While the dialogue is still up
   the named-action queue hands the speaker's name to 0x00571660 instead.
   Every occupied living character without a maxed condition practices
   communication (skill 0x16). */
// FUNCTION: WIZ8 0x00577290
void HandleNpcDialogueDeparture(int value)
{
    W8MonsterInfo* info;
    W8Character* character;
    int index;

    if (g_screen_state_00649f1c->dialogue_npc->name_style == 0x2a &&
        (info = GetNpcMonsterInfo(g_screen_state_00649f1c->dialogue_npc)) != 0) {
        info->monster->SetScript004C7F10("Guard.msf", 1);
    }
    if ((value == 0 || g_screen_state_00649f1c->dialogue_npc->dismissed_flag == 0 ||
         g_screen_state_00649f1c->dialogue_npc->record->unknown_2ef[1] != 0) &&
        g_screen_state_00649f1c->value_25c < 1) {
        if (g_screen_state_00649f1c->dialogue_npc->greeting_pending == 0) {
            QueueNpcScriptLine(1, 0, 0, 0);
        } else {
            QueueNpcScriptLine(0, 0, 0, 0);
            g_screen_state_00649f1c->dialogue_npc->greeting_pending = 0;
            if (g_screen_state_00649f1c->dialogue_npc->record->unknown_054 == 0 &&
                g_screen_state_00649f1c->dialogue_npc->record->flag_2ea == 0 &&
                g_screen_state_00649f1c->dialogue_npc->record->unknown_056 == 0) {
                for (index = 0; index < 8; ++index) {
                    character = &g_status_685170.buffers.characters[index];
                    if (g_status_685170.buffers.party_rows[index].occupied != 0 &&
                        character->hp_current != 0 && character->highest_condition < 0xf) {
                        PracticeCharacterSkill(character, 0x16, 0xf, 0);
                    }
                }
            }
            if (gXStatus.fNpcDialogueMode != 0 && g_screen_state_00649f1c->flag_252 == 0) {
                AddNpcDialogueKeyword(
                    g_screen_state_00649f1c->dialogue_npc->record->source_name_004, -1, 1);
                return;
            }
            if (g_screen_state_00649f1c->dialogue_npc->record->unknown_054 == 0 &&
                (g_screen_state_00649f1c->dialogue_npc->record->flag_2ea == 0 ||
                 g_screen_state_00649f1c->dialogue_npc->is_present != 0)) {
                AddDialogueTranscriptKeyword(
                    g_screen_state_00649f1c->dialogue_npc->record->source_name_004, -1);
            }
        }
    }
}

/* Shared by the main-game screen and dialog text entries; it lives with the
   main-game text helpers, not with UtilityFunctions.cpp. */
// FUNCTION: WIZ8 0x00577410
void ShortenTextToWidth00577410(wchar_t* output, const wchar_t* text, unsigned int width, int font)
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
    g_status_685170.value_2435 = 1;
    ResetLevelDataVectors0041F0D0();
    SetTargetCursor(W8_CURSOR_MAP_LOAD);
}

// FUNCTION: WIZ8 0x00577540
void ClearMainGameTargetState(void)
{
    g_status_685170.value_2435 = 0;
    ClearLevelDataFlag6();
    SetTargetCursor(W8_CURSOR_NONE);
}

/* When the world-cursor gate (value_2435) is raised, discard queued input
   after refreshing the mouse-system position so stale events do not fire. */
// FUNCTION: WIZ8 0x00577560
void FlushInputWhileWorldCursorGate(void)
{
    POINT mouse;
    InputAtom input;

    if (g_status_685170.value_2435 == 0) {
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
            if (CompareWideTextIgnoreAsciiCase00402920(keyword,
                                                       g_item_records[index].display_name) == 0) {
                category = W8_DIALOGUE_CATEGORY_ITEMS;
                break;
            }
        }
        if (category == -1) {
            for (index = 0; index < gXStatus.uiNpcsInDatabase; ++index) {
                if (CompareWideTextIgnoreAsciiCase00402920(
                        keyword, g_npc_records[index].source_name_004) == 0) {
                    category = W8_DIALOGUE_CATEGORY_PEOPLE;
                    break;
                }
            }
        }
        if (category == -1) {
            for (index = 0; g_dialogue_person_keywords[index][0] != 0; ++index) {
                if (CompareWideTextIgnoreAsciiCase00402920(
                        keyword, g_dialogue_person_keywords[index]) == 0) {
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
    for (int index = 0; index < g_screen_state_00649f1c->dialogue_transcript.GetCount(); ++index) {
        if (CompareWideTextIgnoreAsciiCase00402920(
                (*g_screen_state_00649f1c->dialogue_transcript.GetAt(index))->text, keyword) == 0) {
            return;
        }
    }
    W8DialogueTranscriptRecord* record =
        static_cast<W8DialogueTranscriptRecord*>(malloc(sizeof(W8DialogueTranscriptRecord)));
    memset(record, 0, sizeof(*record));
    wcscpy(record->text, keyword);
    record->category = category;
    g_screen_state_00649f1c->dialogue_transcript.Add(record);
}

// FUNCTION: WIZ8 0x005777c0
void RecordLevelEntryDialogueState(void)
{
    wchar_t region_name[100];
    int region = GetLevelBand(g_status_685170.current_level);
    if (GetNpcScriptRegionName(region, region_name)) {
        AddDialogueTranscriptKeyword(region_name, W8_DIALOGUE_CATEGORY_PLACES);
    }
    if (region == 14) {
        SetFact(0x25b, 0, 0);
        if (!NpcLeadHasNameStyle(0x18)) {
            SetFact(0x216, 1, 0);
        }
    }
}

/* Same NPC-dialogue text-box-layout predicate as IsNpcDialogueTextBoxActive,
   emitted as a second copy for the dialogue text input callers. */
// FUNCTION: WIZ8 0x00577830
bool IsNpcDialogueTextBoxActive577830(void)
{
    return gXStatus.fNpcDialogueMode != 0 &&
           g_screen_state_00649f1c->value_fc == W8_DIALOGUE_LAYOUT_MAIN_TEXT_BOX;
}

// FUNCTION: WIZ8 0x00577850
bool CanOpenNpcDialogue(void)
{
    return gXStatus.fNpcDialogueMode != 0 && g_screen_state_00649f1c->flag_252 != 0;
}

// FUNCTION: WIZ8 0x00577880
unsigned char SetNpcDialoguePanelVisible(int value)
{
    W8NpcDialogueTextController* controller;
    unsigned char expanded;

    if (gXStatus.fNpcDialogueMode != 0 && g_screen_state_00649f1c->flag_252 == 0 &&
        g_screen_state_00649f1c->value_fc == W8_DIALOGUE_LAYOUT_TRANSCRIPT) {
        if (value == 0) {
            controller = g_screen_state_00649f1c->npc_dialogue_controller_1b0;
            CollapseNpcDialogueTextArea(controller);
            controller = g_screen_state_00649f1c->npc_dialogue_controller_1b0;
            controller->SetEnabled(0);
            g_screen_state_00649f1c->npc_dialogue_panel_1b4->SetEnabled(0);
            controller = g_screen_state_00649f1c->npc_dialogue_controller_1b0;
            ClearNpcDialogueTextBackground(controller);
            ClearSurfaceRect(0x1dc, 0x11b, 0x269, 0x1c2);
            InvalidateRegion(0x1dc, 0x11b, 0x269, 0x1c2, 0);
            RequestRedraw(2);
            RequestRedraw(8);
            RequestRedraw(0x20);
            RequestRedraw(0x80);
            RequestRedraw(0x200);
            g_screen_state_00649f1c->dialogue_panel_hidden = 1;
            return 1;
        }

        controller = g_screen_state_00649f1c->npc_dialogue_controller_1b0;
        controller->SetEnabled(1);
        g_screen_state_00649f1c->npc_dialogue_panel_1b4->SetEnabled(1);
        controller = g_screen_state_00649f1c->npc_dialogue_controller_1b0;
        ExpandNpcDialogueTextArea(controller);
        controller = g_screen_state_00649f1c->npc_dialogue_controller_1b0;
        expanded = IsNpcDialogueTextExpanded(controller);
        if (expanded == 0) {
            g_screen_state_00649f1c->dialogue_scroll_up_button->SetEnabled(0);
        } else {
            g_screen_state_00649f1c->dialogue_scroll_up_button->SetEnabled(1);
        }
        g_screen_state_00649f1c->dialogue_scroll_down_button->SetEnabled(expanded != 0);
        RequestRedraw(0x200);
        g_screen_state_00649f1c->dialogue_panel_hidden = 0;
        return 1;
    }
    return 0;
}
// FUNCTION: WIZ8 0x00577A20
unsigned char FinishNpcVoiceIfSessionActive00577A20(void)
{
    if (!IsNpcScriptSessionActive()) {
        return 0;
    }
    TryFinishNpcVoicePlayback(0);
    return 1;
}

// FUNCTION: WIZ8 0x00577A40
bool ProcessPendingEvent00577A40(void)
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

// TEMPLATE: WIZ8 0x00577a80
// W8GrowableVector<W8DialogueTranscriptRecord*>::~W8GrowableVector<W8DialogueTranscriptRecord*>

// SYNTHETIC: WIZ8 0x00577aa0
// W8GrowableVector<unsigned short*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x00577ad0
// W8GrowableVector<W8DialogueTranscriptRecord*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x00577b00
// W8Vector<W8DialogueTranscriptRecord*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x00577b20
// W8GrowableVector<W8PendingNoticeLine*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x00577b50
// W8Vector<W8PendingNoticeLine*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00577b70
// W8GrowableVector<W8PendingNoticeLine*>::~W8GrowableVector<W8PendingNoticeLine*>

// SYNTHETIC: WIZ8 0x00577b90
// W8GrowableVector<W8GrowableVector<W8GrowableVector<unsigned short*>*>*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x00577bc0
// W8GrowableVector<W8GrowableVector<unsigned short*>*>::`scalar deleting destructor'
