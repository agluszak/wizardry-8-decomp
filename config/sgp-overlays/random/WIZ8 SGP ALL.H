#ifndef WIZ8_RECOVERED_SGP_ALL_H
#define WIZ8_RECOVERED_SGP_ALL_H

/*
 * Minimal product PCH surface required by Random.c.
 *
 * random.h already pulls in Types.h, Debug.h, stdlib.h and time.h, and it gates
 * PRERANDOM_GENERATOR behind JA2 -- which the Wizardry branch does not define.
 * The unit therefore compiles to exactly InitializeRandom, Random and Chance,
 * and all three are byte-exact in every unprotected Wizardry build.
 */
#include "types.h"
#include "debug.h"
#include "random.h"
#include <stdlib.h>
#include <time.h>

#endif
