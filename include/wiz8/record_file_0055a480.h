#ifndef WIZ8_RECORD_FILE_0055A480_H
#define WIZ8_RECORD_FILE_0055A480_H

/*
 * The file format loaded at 0x0055A480 and read one record at a time by
 * 0x0055A140. Its owning translation unit is an attribution gap and its callers
 * at 0x00524CA0 and 0x00529660 are not recovered, so the subsystem is unknown
 * and both types are named for their addresses rather than guessed at.
 *
 * The loader and the reader overwrite three record slots with the arrays they
 * allocate, so on-disk fields and runtime pointers share storage. Every offset
 * here is unaligned, which is what fixes the packing.
 */

#pragma pack(push, 1)

/* One 8-byte sub-entry. Its leading slot is non-zero on disk to select the
   length-prefixed narrow string that follows, and is then overwritten with the
   pointer to it. */
typedef struct W8FileSubEntry0055A140 {
    unsigned char unknown_00[4];
    char* text;                          /* 0x04 */
} W8FileSubEntry0055A140;                /* 0x08 */

/* One 0x12-byte entry. Only the sub-entry count and array are read by recovered
   source; the leading thirteen bytes come off disk untouched. */
typedef struct W8FileEntry0055A140 {
    unsigned char unknown_00[0xd];
    unsigned char sub_entry_count;       /* 0x0d */
    W8FileSubEntry0055A140* sub_entries; /* 0x0e */
} W8FileEntry0055A140;                   /* 0x12 */

/* One 0x0c-byte record. Field roles come from the reader at 0x0055A140. */
typedef struct W8FileRecord0055A140 {
    unsigned char string_count;          /* 0x00 */
    char** strings;                      /* 0x01: string_count entries */
    W8FileEntry0055A140* entries;        /* 0x05: entry_count entries */
    unsigned short entry_count;          /* 0x09 */
    unsigned char unknown_0b;
} W8FileRecord0055A140;                  /* 0x0c */

/* The 0x0e-byte header the loader reads first. A non-zero name slot on disk
   selects the length-prefixed name that follows it. */
typedef struct W8RecordFile0055A480 {
    unsigned char unknown_00[4];
    unsigned short record_count;         /* 0x04 */
    char* name;                          /* 0x06 */
    W8FileRecord0055A140* records;       /* 0x0a */
} W8RecordFile0055A480;                  /* 0x0e */

#pragma pack(pop)

unsigned char ReadFileRecord0055A140(int handle, W8FileRecord0055A140* record);
W8RecordFile0055A480* LoadRecordFile0055A480(char* path);

#endif
