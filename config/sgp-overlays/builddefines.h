#ifndef WIZ8_RECOVERED_BUILDDEFINES_H
#define WIZ8_RECOVERED_BUILDDEFINES_H

/*
 * The released SGP source expects a product-owned builddefines.h that is not
 * present in the source release.  DirectDraw Calls.c does not consume any
 * product feature switches from it, so its comparison build uses this empty
 * overlay rather than guessing Wizardry-wide definitions.
 */

#endif
