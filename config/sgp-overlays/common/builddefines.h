#ifndef WIZ8_RECOVERED_BUILDDEFINES_H
#define WIZ8_RECOVERED_BUILDDEFINES_H

/*
 * The released SGP source expects a product-owned builddefines.h that is not
 * present in the source release.  No probed translation unit has yet consumed a
 * product feature switch from it, so the comparison builds share this empty
 * overlay rather than guessing Wizardry-wide definitions.  A unit that turns out
 * to need one gets its own definition in its own overlay directory, so the
 * requirement stays attributable to the unit that proved it.
 */

#endif
