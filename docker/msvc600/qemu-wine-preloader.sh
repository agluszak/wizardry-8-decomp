#!/bin/sh
set -eu

# Wine also invokes this path for child processes. Emulate the preloader,
# which reserves VC6's fixed PE image addresses before loading Unix Wine.
exec /usr/bin/qemu-i386 /usr/lib/wine/wine-preloader.real "$@"
