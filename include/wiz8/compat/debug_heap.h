#pragma once

/* VC6 debug CRT heap fill patterns. These checks recognize poisoned storage;
   they are not valid animation pointers or monster location identifiers. */
#define WIZ8_DEBUG_FREED_HEAP_PATTERN 0xddddddddU
#define WIZ8_DEBUG_UNINITIALIZED_HEAP_PATTERN 0xcdcdcdcdU
