# Wizardry 8 recovery exporter

The Java extension behind `uv run wiz8 ghidra export-cpp` turns reviewed
Ghidra semantics into recovered-style C++ with reccmp markers. It emits only
selected definitions. It does not emit headers, duplicate declarations, or a
second analysis database.

## Authority boundaries

Ghidra owns binary analysis: functions, signatures, namespaces, types,
vtables, references, and decompiler markup. Git owns recovered declarations,
translation-unit placement, marker kind and order, and source-owned signature
hints. Generated packets and files under `build/` are disposable projections.

The exporter never reverse engineers interfaces supplied by pinned source
oracles. Source-index hints refine rendering for the current request; they do
not become durable Ghidra facts.

## Architecture

One `RecoverySession` owns classification, call-target and vtable resolution,
and caches for one export operation. `FunctionRoleResolver` maps each binary
function to its source identity and compiler emission within that session.
Class-family resolution groups related emissions and selects the single
authored body carrier without promoting deleting wrappers or thunks into
source functions.

`Msvc6Patterns` analyzes the decompiler's P-code and marked-up token tree.
`MarkupIndex`, `BlockView`, and `RenderedCall` are the navigation boundary;
recognizers do not parse the flat pseudo-C string. Rewrites are claimed
atomically. Conflicts, missing evidence, and structural failures decline or
roll back with structured pass facts. `Wiz8CxxPrinter` applies accepted claims
and renders once.

`exportFunctionPackets` is the function-export API. Each packet contains the
rendered definition and body, source/emission identity, body and canonical
targets, origin and evidence, structured pass facts, defects, resolved calls,
and relevant vtables. Python formats both command output and `recover explain`
from that packet; it contains no semantic fallback.

## Commands

```sh
# Functions or an inclusive entry range
uv run wiz8 ghidra export-cpp 0x004a5e50 0x004a6610
uv run wiz8 ghidra export-cpp 0x004a5e50:0x004a6610 --output build/export.cpp

# One class family, one translation unit, or selected data
uv run wiz8 ghidra export-cpp --class W8GrCycle
uv run wiz8 ghidra export-cpp --unit src/wiz8/engine_code/GrCycle.cpp
uv run wiz8 ghidra export-cpp --data 0x005ecf98

# Structured recognizer decisions
uv run wiz8 recover explain 0x004a5e50
uv run wiz8 recover explain --class W8GrCycle
```

The wrapper compiles these sources on demand into
`build/ghidra-extension/wiz8-recovery.jar` with JDK 21 and the pinned Ghidra
classpath. The JAR is attached after PyGhidra starts through Ghidra's class
loader. A changed JAR requires a fresh Python process because loaded JVM
classes cannot be replaced safely.

## Declines and defects

A recognizer applies only when P-code, markup, types, and ownership account
for the complete source construct. Ambiguity is a named decline and leaves
Ghidra's rendering visible. An unexpected analyzer or renderer failure is a
defect and adds `// exporter-defect:` to the result. One failed pass does not
discard independent accepted work.

Deleting destructors, adjustor thunks, template specializations, and library
entities follow the repository marker policy. Compiler emissions remain
marker-only. A class export emits at most one definition for each source
entity, including the case where a deleting wrapper is the only recoverable
carrier of an authored destructor body.

## Adding a recognizer

Use an existing domain (call, expression, lifecycle, allocation, or EH).
Derive the fact from Ghidra objects, locate only its verified markup nodes via
the shared index, and stage the complete rewrite in one claim transaction.
State every decline explicitly. Do not make a later recognizer depend on text
rendered by an earlier one.

Core recovery reconstructs mechanically justified C++ concepts. A
source-equivalent spelling used only to steer VC6 belongs to match shaping and
needs evidence from at least three unrelated retail sites or a pinned compiler
fixture family with a negative case.

## Validation

```sh
# Python, repository, Java compilation, and marker policy
just check
just test

# Pinned VC6 lifecycle family integration fixture
just lifecycle-fixture

# Zero-edit source regeneration for the stable corpus or selected functions
uv run wiz8 recover regress 0x004a5e50 0x004a5f20
```

Exporter changes must preserve packet text/body for unaffected corpus entries,
leave wrappers marker-only, and keep every rewrite accepted or rejected
atomically. `just lifecycle-fixture` is the architecture gate for constructor,
destructor, thunk, deleting-wrapper, and vtable-family behavior.
