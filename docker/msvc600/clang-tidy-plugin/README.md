# Wizardry clang-tidy plugin

This plugin contains the project-specific checks used by `uv run wiz8 lint`.

The normal lint path owns the policy. The plugin is not a separate recovery
framework and does not maintain a second source model.

## Checks

- `wiz8-redundant-scalar-cast` flags redundant scalar conversions on changed lines.
- `wiz8-project-record-reinterpret-cast` rejects reinterpretation between modeled
  project record types.
- `wiz8-scalar-facts` collects AST facts needed by the byte-domain check.
- `wiz8-bool-like-byte` reports changed byte declarations whose recovered producers
  make them worth reviewing as possible predicates. It is review evidence, not proof
  that the historical declaration was C++ `bool`.

The wrapper computes the changed-line filter from Git and loads the plugin into the
pinned Clang 21 toolchain. `WIZ8_REDUNDANT_CAST_LINES=*` explicitly requests a
whole-tree run; normal lint limits project-specific debt checks to changed lines.

The fact collector is an implementation detail of lint. Historical bulk type-recovery
campaigns and patch generators are intentionally not part of the maintained project
workflow.

## Build

The plugin is built into the pinned VC6/Clang analysis image by
`docker/msvc600/clang-tidy-plugin/CMakeLists.txt`. The image smoke test verifies
that the registered checks can be loaded before the image is used.
