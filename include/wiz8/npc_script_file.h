#ifndef WIZ8_NPC_SCRIPT_FILE_H
#define WIZ8_NPC_SCRIPT_FILE_H

#include <stddef.h>

/*
 * The NPC script (.nsf) file format loaded at 0x0055A480 and read one quote
 * record at a time by 0x0055A140. Its owning translation unit is an
 * attribution gap.
 *
 * Each file is a table of quote records. The quote audit at 0x00529660 walks
 * `Data\NPC Scripts\*.nsf` and reports every record as a "Long Quote" whose
 * `subquotes` it calls "subquote" lines. A record's `entries` are script-event
 * rows whose leading byte discriminates the kind (the dialogue owner at
 * 0x00576060 tests 0x13 and 0x05); entries carry `sub_entries` with their own
 * text.
 *
 * The recovered callers are ReloadNpcScriptResources (0x00524CA0), which loads
 * `Data\NPC Scripts\<name>.nsf` and stores the result on
 * W8NpcState::script_file, and the quote audit named above.
 *
 * The loader and the reader overwrite three record slots with the arrays they
 * allocate, so on-disk fields and runtime pointers share storage. Every offset
 * here is unaligned, which is what fixes the packing.
 */

/* Quote-entry opcodes with recovered consumers; unknown values remain unnamed. */
typedef unsigned char W8NpcQuoteEntryKind;

enum {
    W8_NPC_ENTRY_NEXT_QUOTE = 2,
    W8_NPC_ENTRY_FACT_CONDITIONAL_QUOTE = 3,
    W8_NPC_ENTRY_KEYWORD = 4,
    W8_NPC_ENTRY_OPTIONS = 5,
    W8_NPC_ENTRY_REPLY = 6,
    W8_NPC_ENTRY_SET_FACT = 7,
    W8_NPC_ENTRY_CLOSE_DIALOGUE = 8,
    W8_NPC_ENTRY_GIVE_ITEM = 9,
    W8_NPC_ENTRY_REMOVE_ITEM = 10,
    W8_NPC_ENTRY_ITEM_REQUEST = 11,
    W8_NPC_ENTRY_MONSTER_SCRIPT_LABEL = 12,
    W8_NPC_ENTRY_NPC_WORLD_ACTION = 13,
    W8_NPC_ENTRY_CLOSE_AND_RESUME = 14,
    W8_NPC_ENTRY_ITEM_MENTION = 15,
    W8_NPC_ENTRY_GIVE_GOLD = 16,
    W8_NPC_ENTRY_GIVE_EXPERIENCE = 17,
    W8_NPC_ENTRY_PRICE_CHECK = 18,
    W8_NPC_ENTRY_KEYWORD_INPUT = 19,
    W8_NPC_ENTRY_SELF_GROUP_ACTION = 20,
    W8_NPC_ENTRY_NPC_INTERACTION = 21,
    W8_NPC_ENTRY_FACTION_CHANGE = 22,
    W8_NPC_ENTRY_DECLINE_QUOTE = 23,
    W8_NPC_ENTRY_PARTY_SPEAKER_EVENT = 24,
    W8_NPC_ENTRY_PERSON_KEYWORD = 25,
    W8_NPC_ENTRY_ITEM_KEYWORD = 26,
    W8_NPC_ENTRY_PLACE_KEYWORD = 27,
    W8_NPC_ENTRY_MISC_KEYWORD = 28,
    W8_NPC_ENTRY_ALWAYS_PRICE_CHECK = 30
};

#pragma pack(push, 1)

/* One 8-byte sub-entry. Its text slot at +4 is non-zero on disk to select the
   length-prefixed narrow string that follows, and is then overwritten with the
   pointer to it. The leading dword is per-kind storage: the fact-conditional
   entry read at 0x005251F0 consumes sub-entry pairs whose +0 dwords are the
   fact id and the expected value. */
struct W8NpcQuoteSubEntry {
    int operand; /* 0x00: fact id on even slots, the expected value on odd */
    char* text;  /* 0x04 */
}; /* 0x08 */

/* One 0x12-byte entry. Byte 0 is the kind discriminator read by 0x00576060;
   the next twelve bytes come off disk untouched. The three dwords are
   per-kind operand slots: the response chooser reads operand0 as the fixed
   or low response index, operand1 == 2 as the random-selection mode and
   operand2 as the range bound / chain marker. */
struct W8NpcQuoteEntry {
    W8NpcQuoteEntryKind kind;
    int operand0;
    int operand1;
    int operand2;
    unsigned char sub_entry_count;   /* 0x0d */
    W8NpcQuoteSubEntry* sub_entries; /* 0x0e */
}; /* 0x12 */

/* One 0x0c-byte quote record. Field roles come from the reader at 0x0055A140. */
struct W8NpcScriptQuote {
    unsigned char subquote_count; /* 0x00 */
    char** subquotes;             /* 0x01: subquote_count entries */
    W8NpcQuoteEntry* entries;     /* 0x05: entry_count entries */
    unsigned short entry_count;   /* 0x09 */
    /* 0x0b: selects the alert presentation - "Data\Sound\NPCs\Dialogue
       Alert.wav" path and the portrait-message/notice flow instead of the
       per-NPC voice file. */
    unsigned char dialogue_alert;
}; /* 0x0c */

/* The 0x0e-byte header the loader reads first. A non-zero name slot on disk
   selects the length-prefixed name that follows it. */
struct W8NpcScriptFile {
    unsigned char unknown_00[4];
    unsigned short quote_count; /* 0x04 */
    char* name;                 /* 0x06 */
    W8NpcScriptQuote* quotes;   /* 0x0a */
}; /* 0x0e */

#pragma pack(pop)

static_assert(sizeof(W8NpcQuoteSubEntry) == 8, "W8NpcQuoteSubEntry_size");
static_assert(offsetof(W8NpcQuoteSubEntry, text) == 4, "W8NpcQuoteSubEntry_text");
static_assert(sizeof(W8NpcQuoteEntry) == 0x12, "W8NpcQuoteEntry_size");
static_assert(offsetof(W8NpcQuoteEntry, sub_entry_count) == 0x0d, "W8NpcQuoteEntry_count");
static_assert(offsetof(W8NpcQuoteEntry, sub_entries) == 0x0e, "W8NpcQuoteEntry_sub_entries");
static_assert(sizeof(W8NpcScriptQuote) == 0x0c, "W8NpcScriptQuote_size");
static_assert(offsetof(W8NpcScriptQuote, subquotes) == 1, "W8NpcScriptQuote_subquotes");
static_assert(offsetof(W8NpcScriptQuote, entries) == 5, "W8NpcScriptQuote_entries");
static_assert(offsetof(W8NpcScriptQuote, entry_count) == 9, "W8NpcScriptQuote_entry_count");
static_assert(sizeof(W8NpcScriptFile) == 0x0e, "W8NpcScriptFile_size");
static_assert(offsetof(W8NpcScriptFile, name) == 6, "W8NpcScriptFile_name");
static_assert(offsetof(W8NpcScriptFile, quotes) == 0x0a, "W8NpcScriptFile_quotes");

void ReleaseNpcScriptFile(W8NpcScriptFile* file);
unsigned char ReadNpcScriptQuote(int handle, W8NpcScriptQuote* quote);
W8NpcScriptFile* LoadNpcScriptFile(char* path);

#endif
