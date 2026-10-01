#!/bin/sh
set -eu

if [ "${WIZ8_EMULATE_I386:-0}" = 1 ]; then
    # Keep the server native; only the 32-bit Wine loaders need emulation.
    export WINESERVER=/usr/lib/wine/wineserver64
    exec /usr/lib/wine/wine-preloader /usr/lib/wine/wine "$@"
fi
exec /usr/bin/wine "$@"
