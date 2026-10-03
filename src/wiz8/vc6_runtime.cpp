/*
 * CRT-owned functions and linker-generated import thunks in the canonical
 * executable. LIBRARY marks retained CRT code; SYNTHETIC marks IAT jump stubs.
 * Neither kind claims a first-party recovered body.
 */

// LIBRARY: WIZ8 0x00401000
// __WinMainCRTStartup

// LIBRARY: WIZ8 0x005e1c30
// __aulldiv

// LIBRARY: WIZ8 0x00401180
// _onexit

// LIBRARY: WIZ8 0x004011ac
// atexit

// LIBRARY: WIZ8 0x005e1c10
// ??3@YAXPAX@Z

// LIBRARY: WIZ8 0x005e1ca0
// __allmul

// LIBRARY: WIZ8 0x005e1cf0
// __alldiv

// LIBRARY: WIZ8 0x005e1da0
// __alloca_probe

// LIBRARY: WIZ8 0x005e1dd0
// __aullshr

// LIBRARY: WIZ8 0x005e1def
// ??_L@YGXPAXIHP6EX0@Z1@Z

// LIBRARY: WIZ8 0x005e1e71
// ??_M@YGXPAXIHP6EX0@Z@Z

// LIBRARY: WIZ8 0x005e1ef1
// ?__ArrayUnwind@@YGXPAXIHP6EX0@Z@Z

// LIBRARY: WIZ8 0x0040116a
// _cfltcvt_init

// LIBRARY: WIZ8 0x004028c0
// _wcsnicmp

/* zlib 1.0.4 corpus in Wiz8.exe. Names come from docs/libraries/zlib-1.0.4.md;
   the source-oracle gate also treats the whole 0x00415910-0x0041A7ED span as
   library-owned even when an interior helper is still unnamed here. */

// LIBRARY: WIZ8 0x00415910
// inflateReset

// LIBRARY: WIZ8 0x00415960 SYMBOL
// _inflateEnd

// LIBRARY: WIZ8 0x00415ad0 SYMBOL
// _inflateInit_

// LIBRARY: WIZ8 0x00415af0 SYMBOL
// _inflate

// LIBRARY: WIZ8 0x00415f60
// deflate_stored

// LIBRARY: WIZ8 0x004165c0
// deflate_slow

// LIBRARY: WIZ8 0x00417810
// adler32

// LIBRARY: WIZ8 0x00417940
// zcalloc

// LIBRARY: WIZ8 0x00417960
// zcfree

/* Remaining zlib 1.0.4 boundaries (docs/libraries/zlib-1.0.4.md); names are
   from the pinned source archive and cross-build fingerprints. */

// LIBRARY: WIZ8 0x004159c0
// inflateInit2_

// LIBRARY: WIZ8 0x00415f10
// flush_pending

// LIBRARY: WIZ8 0x00416070
// fill_window

// LIBRARY: WIZ8 0x00416180
// read_buf

// LIBRARY: WIZ8 0x004161f0
// deflate_fast

// LIBRARY: WIZ8 0x00416450
// longest_match

// LIBRARY: WIZ8 0x004168b0
// inflate_blocks_reset

// LIBRARY: WIZ8 0x00416940
// inflate_blocks_new

// LIBRARY: WIZ8 0x004169b0
// inflate_blocks

// LIBRARY: WIZ8 0x004177d0
// inflate_blocks_free

// LIBRARY: WIZ8 0x00417970
// init_block

// LIBRARY: WIZ8 0x004179e0
// gen_codes

// LIBRARY: WIZ8 0x00417a60
// _tr_stored_block

// LIBRARY: WIZ8 0x00417b20
// _tr_flush_block

// LIBRARY: WIZ8 0x00417d40
// build_tree

// LIBRARY: WIZ8 0x00417f50
// pqdownheap

// LIBRARY: WIZ8 0x00418000
// gen_bitlen

// LIBRARY: WIZ8 0x004181e0
// build_bl_tree

// LIBRARY: WIZ8 0x00418250
// scan_tree

// LIBRARY: WIZ8 0x00418340
// send_all_trees

// LIBRARY: WIZ8 0x004185a0
// send_tree

// LIBRARY: WIZ8 0x00418b20
// _tr_tally

// LIBRARY: WIZ8 0x00418c30
// compress_block

// LIBRARY: WIZ8 0x004190a0
// set_data_type

// LIBRARY: WIZ8 0x00419110
// bi_reverse

// LIBRARY: WIZ8 0x00419140
// bi_windup

// LIBRARY: WIZ8 0x004191a0
// copy_block

// LIBRARY: WIZ8 0x00419230
// inflate_trees_bits

// LIBRARY: WIZ8 0x00419290
// huft_build

// LIBRARY: WIZ8 0x00419760
// inflate_trees_dynamic

// LIBRARY: WIZ8 0x00419850
// inflate_trees_fixed

// LIBRARY: WIZ8 0x004199c0
// inflate_trees_free

// LIBRARY: WIZ8 0x00419a00
// inflate_codes_new

// LIBRARY: WIZ8 0x00419a40
// inflate_codes

// LIBRARY: WIZ8 0x0041a340
// inflate_codes_free

// LIBRARY: WIZ8 0x0041a360
// inflate_flush

// LIBRARY: WIZ8 0x0041a4a0
// inflate_fast

/* CRT tail and import thunks inside the documented msvc-runtime range
   (tools/wiz8decomp/source_oracle.py, docs/libraries/msvc6-runtime.md). */

/* VC6 EH unwind funclets: the linker gathers every frame function's
   CxxFrameHandler cleanup funclets into this .text tail. Each entry is a
   compiler-generated local-object destructor call owned by the frame
   tables, not an authored body; the parent function carries the source.
   Ghidra names them Unwind@<address>. */
