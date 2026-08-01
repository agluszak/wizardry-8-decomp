# Wiz8 recovery exporter (Ghidra extension)

The Java semantic engine behind `uv run wiz8 ghidra export-cpp`. Input is the
live analyzed Ghidra program; output is recovered-style C++ text with reccmp
entity markers. It is a selected-definition exporter: it emits only the chosen
definitions — no headers, no data-type definitions, no referenced-global
declarations, and no second source authority.

The printer walks the decompiler's marked-up token tree
(`DecompileResults.getCCodeMarkup()`), never the flat pseudo-C string. Every
recognizer either rewrites a construct on positive evidence or declines, in
which case the verbatim token writer reproduces Ghidra's own rendering. An
unrecognized construct or an analyzer error therefore degrades output quality
for that spot only; it never blocks the batch.

## Build and load

`wiz8 ghidra export-cpp` compiles these sources on demand into
`build/ghidra-extension/wiz8-recovery.jar` and rebuilds only when the source
hash, the Ghidra install path, or the pinned Ghidra version changes
(`stamp.json`). Compilation needs a JDK:

- `javac` is found via `JAVA_HOME`, then `PATH`. Ghidra 12.1.2 runs on JDK 21,
  so a matching JDK is normally already present; the build compiles with
  `--release 21`.
- The compile classpath is every module jar under
  `$GHIDRA_INSTALL_DIR/Ghidra/*/*/lib/*.jar`. The actual dependencies are
  `Decompiler.jar`, `SoftwareModeling.jar`, and `Utility.jar`; the glob is
  used because it is robust and free.

The jar is attached to the running JVM with
`ClassLoader.getSystemClassLoader().addPath(jar)`. Ghidra replaces the system
class loader with `ghidra.GhidraClassLoader`; a jar passed through
`launcher.add_classpaths` would land on the parent loader, which cannot see
the Ghidra module jars, and every `ghidra.*` import would fail with
`NoClassDefFoundError`. (`launcher.add_class_files` is the pre-start
equivalent PyGhidra applies through the same `addPath` call.)

## Usage

```sh
# raw C++ on stdout
uv run wiz8 ghidra export-cpp 0x004a5e50

# several functions, an inclusive entry range, or a file target
uv run wiz8 ghidra export-cpp 0x004a5e50 0x004a5f00 0x004a6610
uv run wiz8 ghidra export-cpp 0x004a5e50:0x004a6610 --output build/export-cpp/grcycle.cpp
```

With `--output` the command prints the usual summary record instead of the
text. Note the default `DecompileOptions()` are used, so rendering can differ
slightly from a CodeBrowser window whose tool options were customized.

## Manual corpus check

Java code runs only inside the live project, so it is validated against the
recovered corpus rather than by `just check`/`just test`:

```sh
uv run wiz8 ghidra export-cpp 0x004a5e50 0x004a5f00 0x004a5f20 0x004a6610 \
    0x004aded0 0x004a6e20 0x004ae270
```

Expected: every body carries its `// FUNCTION: WIZ8 0x…` marker; the scalar
deleting destructor 0x004a5f00 prints exactly the two-line `SYNTHETIC` block
from `src/wiz8/engine_code/GrCycle.cpp` and no body; ordinary functions match
Ghidra's decompiler text byte for byte. Lifecycle lifting (constructor
initializer lists, destructor tails, typed `new`/`delete`) lands in the next
milestone; until then constructors and destructors print verbatim.
