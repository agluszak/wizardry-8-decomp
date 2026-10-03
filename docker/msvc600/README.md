# VC6 compiler images

The default images use native Wine32. On an x86-64 host whose kernel rejects
Linux i386 ELF binaries, build both targets with the same emulation option:

```sh
WIZ8_EMULATE_I386=1 uv run wiz8 toolchain build vc6-sp5
```

The equivalent direct Docker commands are:

```sh
docker build --build-arg WIZ8_EMULATE_I386=1 --target product -t wizardry8-msvc600:sp5-product docker/msvc600
docker build --build-arg WIZ8_EMULATE_I386=1 --target analysis -t wizardry8-msvc600:sp5 docker/msvc600
```

This runs the pinned, original VC6 compiler and linker under QEMU, including
Wine's child processes. Wine's preloader reserves the fixed image addresses
required by VC6; invoking Unix Wine directly under QEMU can produce
`STATUS_CONFLICTING_ADDRESSES`. The Wine server stays native x86-64. Runtime
containers need no privileged mode or binfmt registration. Emulated builds
are slower; they keep the same VC6 source tree and compile/link flags.

Docker's VFS storage driver copies whole layers and needs substantially more
disk than overlay storage, especially when building both targets. Inspect
`docker info` and available disk before provisioning these images.

With VFS, run source indexing with `uv run wiz8 analyze source-index --jobs 1`
before PR checks, and run product builds separately. Each indexer worker
requires a complete writable copy of the analysis image.

Managed cloud setup forwards `CODEX_PROXY_CERT` as a BuildKit secret when it is
provided. The CA is used only during downloads, removed before each layer is
saved, and never disables TLS or archive checksum verification. Nested Docker
builds also need the session proxy's reachable address; refresh stale Docker
proxy configuration before provisioning.

Wine initialization stops the server and removes its temporary Unix sockets
before Docker snapshots the layer. Analysis provisioning uses one layer to
avoid repeated full-filesystem copies under VFS. On a 32 GB cloud workspace,
run toolchain containers one at a time and discard provisioning cache after
both images have built, retaining their tags. Check free disk before indexing
or compiling.
